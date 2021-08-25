/***********************************************************************
 * Copyright 1999-2017, Altibase Corporation or its subsidiaries.
 * All rights reserved.
 **********************************************************************/

/***********************************************************************
 * $Id$
 **********************************************************************/

#include <idl.h>
#include <ide.h>
#include <smiTableSpace.h>
#include <qc.h>
#include <qcg.h>
#include <qcm.h>
#include <qcmPartition.h>
#include <qcmDictionary.h>
#include <qcmView.h>
#include <qcmProc.h>
#include <qcmPkg.h>
#include <qcsModule.h>
#include <qdParseTree.h>
#include <qmsParseTree.h>
#include <qdbCommon.h>
#include <qdbAlter.h>
#include <qdx.h>
#include <qdn.h>
#include <qdnTrigger.h>
#include <qdpRole.h>
#include <qdbComment.h>
#include <qdbCopySwap.h>
#include <qmoPartition.h>
#include <sdi.h>
#include <sdiGlobalDDL.h>

IDE_RC qdbCopySwap::validateCreateTableFromTableSchema( qcStatement * aStatement )
{
/***********************************************************************
 * Description :
 *      PROJ-2600 Online DDL for Tablespace Alteration
 *
 *      CREATE TABLE [user_name.]target_table_name
 *          FROM TABLE SCHEMA [user_name.]source_table_name
 *          USING PREFIX name_prefix;
 *      구문의 Validation
 *
 * Implementation :
 *
 *  1. Target Table Name을 검사한다.
 *      - X$, D$, V$로 시작하지 않아야 한다.
 *      - Table Name 중복이 없어야 한다.
 *
 *  2. Table 생성 권한을 검사한다.
 *
 *  3. Target이 Temporary Table인지 검사한다.
 *      - 사용자가 TEMPORARY 옵션을 지정하지 않아야 한다.
 *
 *  4. Target Table Owner와 Source Table Owner가 같아야 한다.
 *
 *  5. Source Table Name으로 Table Info를 얻고, IS Lock을 잡는다.
 *      - Partitioned Table이면, Partition Info를 얻고, IS Lock을 잡는다.
 *      - Partitioned Table이면, Non-Partitioned Index Info를 얻고, IS Lock을 잡는다.
 *
 *  6. Source가 일반 Table인지 검사한다.
 *      - TABLE_TYPE = 'T', TEMPORARY = 'N', HIDDEN = 'N' 이어야 한다. (SYS_TABLES_)
 *
 ***********************************************************************/

    qdTableParseTree  * sParseTree      = NULL;
    qcmTableInfo      * sTableInfo      = NULL;
    UInt                sSourceUserID   = 0;
    idBool              sExist          = ID_FALSE;
    qsOID               sProcID;
    qcuSqlSourceInfo    sqlInfo;

    sParseTree = (qdTableParseTree *)aStatement->myPlan->parseTree;

    IDU_FIT_POINT( "qdbCopySwap::validateCreateTableFromTableSchema::alloc::mSourcePartTable",
                   idERR_ABORT_InsufficientMemory );

    IDE_TEST( QC_QMP_MEM( aStatement )->alloc( ID_SIZEOF( qdPartitionedTableList ),
                                               (void **) & sParseTree->mSourcePartTable )
              != IDE_SUCCESS );
    QD_SET_INIT_PART_TABLE_LIST( sParseTree->mSourcePartTable );

    /* 1. Target Table Name을 검사한다.
     *  - X$, D$, V$로 시작하지 않아야 한다.
     *  - Table Name 중복이 없어야 한다.
     */
    if ( qdbCommon::containDollarInName( & sParseTree->tableName ) == ID_TRUE )
    {
        sqlInfo.setSourceInfo( aStatement, & sParseTree->tableName );

        IDE_RAISE( CANT_USE_RESERVED_WORD );
    }
    else
    {
        /* Nothing to do */
    }

    IDE_TEST( qcm::existObject( aStatement,
                                ID_FALSE,
                                sParseTree->userName,
                                sParseTree->tableName,
                                QS_OBJECT_MAX,
                                & sParseTree->userID,
                                & sProcID,
                                & sExist )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( sExist == ID_TRUE, ERR_EXIST_OBJECT_NAME );

    /* 2. Table 생성 권한을 검사한다. */
    IDE_TEST( qdpRole::checkDDLCreateTablePriv( aStatement,
                                                sParseTree->userID )
              != IDE_SUCCESS );

    /* 3. Target이 Temporary Table인지 검사한다.
     *  - 사용자가 TEMPORARY 옵션을 지정하지 않아야 한다.
     */
    IDE_TEST_RAISE( ( sParseTree->flag & QDT_CREATE_TEMPORARY_MASK )
                                      == QDT_CREATE_TEMPORARY_TRUE,
                    ERR_NOT_SUPPORTED_TEMPORARY_TABLE_FEATURE );

    /* 4. Target Table Owner와 Source Table Owner가 같아야 한다. */
    IDE_TEST( qdbCommon::checkTableInfo( aStatement,
                                         sParseTree->mSourceUserName,
                                         sParseTree->mSourceTableName,
                                         & sSourceUserID,
                                         & sParseTree->mSourcePartTable->mTableInfo,
                                         & sParseTree->mSourcePartTable->mTableHandle,
                                         & sParseTree->mSourcePartTable->mTableSCN )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( sSourceUserID != sParseTree->userID, ERR_DIFFERENT_TABLE_OWNER );

    /* 5. Source Table Name으로 Table Info를 얻고, IS Lock을 잡는다.
     *  - Partitioned Table이면, Partition Info를 얻고, IS Lock을 잡는다.
     *  - Partitioned Table이면, Non-Partitioned Index Info를 얻고, IS Lock을 잡는다.
     */
    IDE_TEST( qcm::lockTableForDDLValidation( aStatement,
                                              sParseTree->mSourcePartTable->mTableHandle,
                                              sParseTree->mSourcePartTable->mTableSCN )
              != IDE_SUCCESS );

    sTableInfo = sParseTree->mSourcePartTable->mTableInfo;

    if ( sTableInfo->tablePartitionType == QCM_PARTITIONED_TABLE )
    {
        IDE_TEST( qdbCommon::checkAndSetAllPartitionInfo( aStatement,
                                                          sTableInfo->tableID,
                                                          & sParseTree->mSourcePartTable->mPartInfoList )
                  != IDE_SUCCESS );

        IDE_TEST( qdx::makeAndLockIndexTableList( aStatement,
                                                  ID_FALSE,
                                                  sTableInfo,
                                                  & sParseTree->mSourcePartTable->mIndexTableList )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing to do */
    }

    /* 6. Source가 일반 Table인지 검사한다.
     *  - TABLE_TYPE = 'T', TEMPORARY = 'N', HIDDEN = 'N' 이어야 한다. (SYS_TABLES_)
     */
    IDE_TEST( checkNormalUserTable( aStatement,
                                    sTableInfo,
                                    sParseTree->mSourceTableName )
              != IDE_SUCCESS );

    if (  QCG_GET_SESSION_IS_NEED_DDL_INFO( aStatement ) == ID_TRUE )
    {
        qrc::setDDLSrcInfo( aStatement,
                            ID_TRUE,
                            0,
                            NULL,
                            0,
                            NULL );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( CANT_USE_RESERVED_WORD )
    {
        (void)sqlInfo.init( QC_QME_MEM( aStatement ) );
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDB_RESERVED_WORD_IN_OBJECT_NAME,
                                  sqlInfo.getErrMessage() ) );
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION( ERR_EXIST_OBJECT_NAME )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDB_EXIST_OBJECT_NAME ) );
    }
    IDE_EXCEPTION( ERR_NOT_SUPPORTED_TEMPORARY_TABLE_FEATURE )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDB_NOT_SUPPORTED_TEMPORARY_TABLE_FEATURE ) );
    }
    IDE_EXCEPTION( ERR_DIFFERENT_TABLE_OWNER )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDB_COPY_SWAP_DIFFERENT_TABLE_OWNER ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdbCopySwap::executeCreateTableFromTableSchema( qcStatement * aStatement )
{
/***********************************************************************
 * Description :
 *      PROJ-2600 Online DDL for Tablespace Alteration
 *
 *      CREATE TABLE [user_name.]target_table_name
 *          FROM TABLE SCHEMA [user_name.]source_table_name
 *          USING PREFIX name_prefix;
 *      구문의 Execution
 *
 * Implementation :
 *
 *  1. Source의 Table에 IS Lock을 잡는다.
 *      - Partitioned Table이면, Partition에 IS Lock을 잡는다.
 *      - Partitioned Table이면, Non-Partitioned Index에 IS Lock을 잡는다.
 *
 *  2. Next Table ID를 얻는다.
 *
 *  3. Target의 Column Array를 구성한다.
 *      - Column 정보를 Source에서 복사한다.
 *      - Next Table ID를 이용하여 Column ID를 결정한다. (Column ID = Table ID * 1024 + Column Order)
 *      - Hidden Column이면 Function-based Index의 Column이므로, Hidden Column Name을 변경한다.
 *          - Hidden Column Name에 Prefix를 붙인다.
 *              - Hidden Column Name = Index Name + $ + IDX + Number
 *
 *  4. Non-Partitioned Table이면, Dictionary Table과 Dictionary Table Info List를 생성한다.
 *      - Dictionary Table을 생성한다. (SM, Meta Table, Meta Cache)
 *      - Target Table의 Column에 Dictionary Table OID를 설정한다.
 *      - Dictionary Table Info List에 Dictionary Table Info를 추가한다.
 *      - Call : qcmDictionary::createDictionaryTable()
 *
 *  5. Target Table을 생성한다. (SM, Meta Table, Meta Cache)
 *      - Source Table의 Table Option을 사용하여, Target Table을 생성한다. (SM)
 *      - Meta Table에 Target Table 정보를 추가한다. (Meta Table)
 *          - SYS_TABLES_
 *              - SM에서 얻은 Table OID가 SYS_TABLES_에 필요하다.
 *              - REPLICATION_COUNT, REPLICATION_RECOVERY_COUNT는 초기화한다.
 *              - LAST_DDL_TIME을 초기화한다. (SYSDATE)
 *          - SYS_COLUMNS_
 *          - SYS_ENCRYPTED_COLUMNS_
 *          - SYS_LOBS_
 *          - SYS_COMPRESSION_TABLES_
 *              - Call : qdbCommon::insertCompressionTableSpecIntoMeta()
 *          - SYS_PART_TABLES_
 *          - SYS_PART_KEY_COLUMNS_
 *      - Table Info를 생성하고, SM에 등록한다. (Meta Cache)
 *      - Table Info를 얻는다.
 *
 *  6. Partitioned Table이면, Partition을 생성한다.
 *      - Partition을 생성한다.
 *          - Next Table Partition ID를 얻는다.
 *          - Partition을 생성한다. (SM)
 *          - Meta Table에 Target Partition 정보를 추가한다. (Meta Table)
 *              - SYS_TABLE_PARTITIONS_
 *                  - SM에서 얻은 Partition OID가 SYS_TABLE_PARTITIONS_에 필요하다.
 *                  - REPLICATION_COUNT, REPLICATION_RECOVERY_COUNT는 초기화한다.
 *              - SYS_PART_LOBS_
 *      - Partition Info를 생성하고, SM에 등록한다. (Meta Cache)
 *      - Partition Info를 얻는다.
 *
 *  7. Target Table에 Constraint를 생성한다.
 *      - Next Constraint ID를 얻는다.
 *      - CONSTRAINT_NAME에 사용자가 지정한 Prefix를 붙여서 CONSTRAINT_NAME을 생성한다.
 *          - 생성한 CONSTRAINT_NAME이 Unique한지 확인해야 한다.
 *              - CONSTRAINT_NAME은 Unique Index의 Column이 아니다. 코드로 Unique를 검사한다.
 *      - Meta Table에 Target Table의 Constraint 정보를 추가한다. (Meta Table)
 *          - SYS_CONSTRAINTS_
 *              - Primary Key, Unique, Local Unique인 경우, Index ID가 SYS_CONSTRAINTS_에 필요하다.
 *          - SYS_CONSTRAINT_COLUMNS_
 *          - SYS_CONSTRAINT_RELATED_
 *
 *  8. Target Table에 Index를 생성한다.
 *      - Source Table의 Index 정보를 사용하여, Target Table의 Index를 생성한다. (SM)
 *          - Next Index ID를 얻는다.
 *          - INDEX_NAME에 사용자가 지정한 Prefix를 붙여서 INDEX_NAME을 생성한다.
 *          - Target Table의 Table Handle이 필요하다.
 *      - Meta Table에 Target Table의 Index 정보를 추가한다. (Meta Table)
 *          - SYS_INDICES_
 *              - INDEX_TABLE_ID는 0으로 초기화한다.
 *              - LAST_DDL_TIME을 초기화한다. (SYSDATE)
 *          - SYS_INDEX_COLUMNS_
 *          - SYS_INDEX_RELATED_
 *
 *      - Partitioned Table이면, Local Index 또는 Non-Partitioned Index를 생성한다.
 *          - Local Index를 생성한다.
 *              - Local Index를 생성한다. (SM)
 *              - Meta Table에 Target Partition 정보를 추가한다. (Meta Table)
 *                  - SYS_PART_INDICES_
 *                  - SYS_INDEX_PARTITIONS_
 *
 *          - Non-Partitioned Index를 생성한다.
 *              - INDEX_NAME으로 Index Table Name, Key Index Name, Rid Index Name을 결정한다.
 *                  - Call : qdx::checkIndexTableName()
 *              - Non-Partitioned Index를 생성한다.
 *                  - Index Table을 생성한다. (SM, Meta Table, Meta Cache)
 *                  - Index Table의 Index를 생성한다. (SM, Meta Table)
 *                  - Index Table Info를 다시 얻는다. (Meta Cache)
 *                  - Call : qdx::createIndexTable(), qdx::createIndexTableIndices()
 *              - Index Table ID를 갱신한다. (SYS_INDICES_.INDEX_TABLE_ID)
 *                  - Call : qdx::updateIndexSpecFromMeta()
 *
 *  9. Target Table에 Trigger를 생성한다.
 *      - Source Table의 Trigger 정보를 사용하여, Target Table의 Trigger를 생성한다. (SM)
 *          - TRIGGER_NAME에 사용자가 지정한 Prefix를 붙여서 TRIGGER_NAME을 생성한다.
 *          - Trigger Strings에 생성한 TRIGGER_NAME과 Target Table Name을 적용한다.
 *      - Meta Table에 Target Table의 Trigger 정보를 추가한다. (Meta Table)
 *          - SYS_TRIGGERS_
 *              - 생성한 TRIGGER_NAME을 사용한다.
 *              - SM에서 얻은 Trigger OID가 SYS_TRIGGERS_에 필요하다.
 *              - 변경한 Trigger String으로 SUBSTRING_CNT, STRING_LENGTH를 만들어야 한다.
 *              - LAST_DDL_TIME을 초기화한다. (SYSDATE)
 *          - SYS_TRIGGER_STRINGS_
 *              - 갱신한 Trigger Strings을 잘라 넣는다.
 *          - SYS_TRIGGER_UPDATE_COLUMNS_
                - 새로운 TABLE_ID를 이용하여 COLUMN_ID를 만들어야 한다.
 *          - SYS_TRIGGER_DML_TABLES_
 *
 *  10. Target Table Info와 Target Partition Info List를 다시 얻는다.
 *      - Table Info를 제거한다.
 *      - Table Info를 생성하고, SM에 등록한다.
 *      - Table Info를 얻는다.
 *      - Partition Info List를 제거한다.
 *      - Partition Info List를 생성하고, SM에 등록한다.
 *      - Partition Info List를 얻는다.
 *
 *  11. Comment를 복사한다. (Meta Table)
 *      - SYS_COMMENTS_
 *          - TABLE_NAME을 Target Table Name으로 지정한다.
 *          - Hidden Column이면 Function-based Index의 Column이므로, Hidden Column Name을 변경한다.
 *              - Hidden Column Name에 Prefix를 붙인다.
 *                  - Hidden Column Name = Index Name + $ + IDX + Number
 *          - 나머지는 그대로 복사한다.
 *
 *  12. View에 대해 Recompile & Set Valid를 수행한다.
 *      - RELATED_USER_ID = User ID, RELATED_OBJECT_NAME = Table Name, RELATED_OBJECT_TYPE = 2 인 경우에 해당한다.
 *        (SYS_VIEW_RELATED_)
 *      - Call : qcmView::recompileAndSetValidViewOfRelated()
 *
 *  13. DDL_SUPPLEMENTAL_LOG_ENABLE이 1인 경우, Supplemental Log를 기록한다.
 *      - DDL Statement Text를 기록한다.
 *          - Call : qciMisc::writeDDLStmtTextLog()
 *      - Table Meta Log Record를 기록한다.
 *          - Call : qci::mManageReplicationCallback.mWriteTableMetaLog()
 *
 *  14. Encrypted Column을 보안 모듈에 등록한다.
 *      - 보안 컬럼 생성 시, 보안 모듈에 Table Owner Name, Table Name, Column Name, Policy Name을 등록한다.
 *      - 예외 처리를 하지 않기 위해, 마지막에 수행한다.
 *      - Call : qcsModule::setColumnPolicy()
 *
 ***********************************************************************/

    SChar                     sObjectName[QC_MAX_OBJECT_NAME_LEN + 1];
    qdTableParseTree        * sParseTree            = NULL;
    qcmTableInfo            * sTableInfo            = NULL;
    qcmPartitionInfoList    * sPartInfoList         = NULL;
    qcmPartitionInfoList    * sOldPartInfoList      = NULL;
    qdIndexTableList        * sIndexTableList       = NULL;
    qdIndexTableList        * sIndexTable           = NULL;
    qcmColumn               * sColumn               = NULL;
    qcmTableInfo            * sSourceTableInfo      = NULL;
    qcmPartitionInfoList    * sSourcePartInfoList   = NULL;
    qdIndexTableList        * sSourceIndexTableList = NULL;
    qcmTableInfo            * sDicTableInfo         = NULL;
    qcmDictionaryTable      * sDictionaryTableList  = NULL;
    qcmDictionaryTable      * sDictionaryTable      = NULL;
    void                    * sTableHandle          = NULL;
    qcmPartitionInfoList    * sPartitionInfo        = NULL;
    qcmIndex                * sTableIndices         = NULL;
    qcmIndex               ** sPartIndices          = NULL;
    qdnTriggerCache        ** sTargetTriggerCache   = NULL;
    smSCN                     sSCN;
    UInt                      sTableFlag            = 0;
    UInt                      sTableID              = 0;
    smOID                     sTableOID             = SMI_NULL_OID;
    idBool                    sIsPartitioned        = ID_FALSE;
    UInt                      sPartitionCount       = 0;
    UInt                      sPartIndicesCount     = 0;
    UInt                      i                     = 0;
    qcmIndex                * sTempIndex		    = NULL;
    UInt                      sShardFlag            = 0;

    sParseTree = (qdTableParseTree *)aStatement->myPlan->parseTree;
    SMI_INIT_SCN( & sSCN );

    /* 1. Source의 Table에 IS Lock을 잡는다.
     *  - Partitioned Table이면, Partition에 IS Lock을 잡는다.
     *  - Partitioned Table이면, Non-Partitioned Index에 IS Lock을 잡는다.
     */
    IDE_TEST( qcm::validateAndLockTable( aStatement,
                                         sParseTree->mSourcePartTable->mTableHandle,
                                         sParseTree->mSourcePartTable->mTableSCN,
                                         SMI_TABLE_LOCK_IS )
              != IDE_SUCCESS );

    sSourceTableInfo = sParseTree->mSourcePartTable->mTableInfo;

    if ( sSourceTableInfo->tablePartitionType == QCM_PARTITIONED_TABLE )
    {
        IDE_TEST( qcmPartition::validateAndLockPartitionInfoList( aStatement,
                                                                  sParseTree->mSourcePartTable->mPartInfoList,
                                                                  SMI_TBSLV_DDL_DML, // TBS Validation 옵션
                                                                  SMI_TABLE_LOCK_IS,
                                                                  smiGetDDLLockTimeOut((QC_SMI_STMT(aStatement))->getTrans()))
                  != IDE_SUCCESS );

        sSourcePartInfoList = sParseTree->mSourcePartTable->mPartInfoList;

        IDE_TEST( qdx::validateAndLockIndexTableList( aStatement,
                                                      sParseTree->mSourcePartTable->mIndexTableList,
                                                      SMI_TBSLV_DDL_DML, // TBS Validation 옵션
                                                      SMI_TABLE_LOCK_IS,
                                                      smiGetDDLLockTimeOut((QC_SMI_STMT(aStatement))->getTrans()))
                  != IDE_SUCCESS );

        sSourceIndexTableList = sParseTree->mSourcePartTable->mIndexTableList;

        sIsPartitioned = ID_TRUE;
    }
    else
    {
        sIsPartitioned = ID_FALSE;
    }

    // Constraint, Index 생성을 위한 준비
    for ( sPartitionInfo = sSourcePartInfoList;
          sPartitionInfo != NULL;
          sPartitionInfo = sPartitionInfo->next )
    {
        sPartitionCount++;
    }

    // Constraint, Index 생성을 위한 준비
    if ( sSourceTableInfo->indexCount > 0 )
    {
        IDU_FIT_POINT( "qdbCopySwap::executeCreateTableFromTableSchema::alloc::sTableIndices",
                       idERR_ABORT_InsufficientMemory );

        // for partitioned index, non-partitioned index
        IDE_TEST( QC_QMX_MEM( aStatement )->alloc( ID_SIZEOF( qcmIndex ) * sSourceTableInfo->indexCount,
                                                   (void **) & sTableIndices )
                  != IDE_SUCCESS );

        idlOS::memcpy( sTableIndices,
                       sSourceTableInfo->indices,
                       ID_SIZEOF( qcmIndex ) * sSourceTableInfo->indexCount );

        // for index partition
        if ( sIsPartitioned == ID_TRUE )
        {
            IDU_FIT_POINT( "qdbCopySwap::executeCreateTableFromTableSchema::alloc::sPartIndices",
                           idERR_ABORT_InsufficientMemory );

            IDE_TEST( QC_QMX_MEM( aStatement )->alloc( ID_SIZEOF( qcmIndex * ) * sPartitionCount,
                                                       (void **) & sPartIndices )
                      != IDE_SUCCESS );

            sPartIndicesCount = sSourcePartInfoList->partitionInfo->indexCount;

            for ( sPartitionInfo = sSourcePartInfoList, i = 0;
                  sPartitionInfo != NULL;
                  sPartitionInfo = sPartitionInfo->next, i++ )
            {
                if ( sPartIndicesCount > 0 )
                {
                    IDU_FIT_POINT( "qdbCopySwap::executeCreateTableFromTableSchema::alloc::sPartIndices[i]",
                                   idERR_ABORT_InsufficientMemory );

                    IDE_TEST( QC_QMX_MEM( aStatement )->alloc( ID_SIZEOF( qcmIndex ) * sPartIndicesCount,
                                                               (void **) & sPartIndices[i] )
                              != IDE_SUCCESS );

                    idlOS::memcpy( sPartIndices[i],
                                   sPartitionInfo->partitionInfo->indices,
                                   ID_SIZEOF( qcmIndex ) * sPartIndicesCount );
                }
                else
                {
                    sPartIndices[i] = NULL;
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

    /* 2. Next Table ID를 얻는다. */
    IDE_TEST( qcm::getNextTableID( aStatement, & sTableID ) != IDE_SUCCESS );

    /* 3. Target의 Column Array를 구성한다.
     *  - Column 정보를 Source에서 복사한다.
     *  - Next Table ID를 이용하여 Column ID를 결정한다. (Column ID = Table ID * 1024 + Column Order)
     *  - Hidden Column이면 Function-based Index의 Column이므로, Hidden Column Name을 변경한다.
     *      - Hidden Column Name에 Prefix를 붙인다.
     *          - Hidden Column Name = Index Name + $ + IDX + Number
     */
    // 아래에서 Target Table을 생성할 때, qdbCommon::createTableOnSM()에서 Column 정보로 qcmColumn을 요구한다.
    //  - qdbCommon::createTableOnSM()
    //      - 실제로 필요한 정보는 mtcColumn이다.
    //      - Column ID를 새로 생성하고, Column 통계 정보를 초기화한다. (매개변수인 qcmColumn을 수정한다.)
    IDE_TEST( qcm::copyQcmColumns( QC_QMX_MEM( aStatement ),
                                   sSourceTableInfo->columns,
                                   & sParseTree->columns,
                                   sSourceTableInfo->columnCount )
              != IDE_SUCCESS );

    for ( sColumn = sParseTree->columns; sColumn != NULL; sColumn = sColumn->next )
    {
        if ( ( sColumn->flag & QCM_COLUMN_HIDDEN_COLUMN_MASK )
                            == QCM_COLUMN_HIDDEN_COLUMN_TRUE )
        {
            IDE_TEST_RAISE( ( idlOS::strlen( sColumn->name ) + sParseTree->mNamesPrefix.size ) >
                            QC_MAX_OBJECT_NAME_LEN,
                            ERR_TOO_LONG_OBJECT_NAME );

            QC_STR_COPY( sObjectName, sParseTree->mNamesPrefix );
            idlOS::strncat( sObjectName, sColumn->name, QC_MAX_OBJECT_NAME_LEN );
            idlOS::strncpy( sColumn->name, sObjectName, QC_MAX_OBJECT_NAME_LEN + 1 );
        }
        else
        {
            /* Nothing to do */
        }
    }

    /* 4. Non-Partitioned Table이면, Dictionary Table과 Dictionary Table Info List를 생성한다.
     *  - Dictionary Table을 생성한다. (SM, Meta Table, Meta Cache)
     *  - Target Table의 Column에 Dictionary Table OID를 설정한다.
     *  - Dictionary Table Info List에 Dictionary Table Info를 추가한다.
     *  - Call : qcmDictionary::createDictionaryTable()
     */
    for ( sColumn = sParseTree->columns; sColumn != NULL; sColumn = sColumn->next )
    {
        if ( ( sColumn->basicInfo->column.flag & SMI_COLUMN_COMPRESSION_MASK )
                                              == SMI_COLUMN_COMPRESSION_TRUE )
        {
            // 기존 Dictionary Table Info를 가져온다.
            sDicTableInfo = (qcmTableInfo *)smiGetTableRuntimeInfoFromTableOID(
                                            sColumn->basicInfo->column.mDictionaryTableOID );

            // Dictionary Table에서 Replication 정보를 제거한다.
            sTableFlag  = sDicTableInfo->tableFlag;
            sTableFlag &= ( ~SMI_TABLE_REPLICATION_MASK );
            sTableFlag |= SMI_TABLE_REPLICATION_DISABLE;
            sTableFlag &= ( ~SMI_TABLE_REPLICATION_TRANS_WAIT_MASK );
            sTableFlag |= SMI_TABLE_REPLICATION_TRANS_WAIT_DISABLE;

            IDE_TEST( qcmDictionary::createDictionaryTable( aStatement,
                                                            sParseTree->userID,
                                                            sColumn,
                                                            1, // aColumnCount
                                                            sSourceTableInfo->TBSID,
                                                            sDicTableInfo->maxrows,
                                                            QDB_TABLE_ATTR_MASK_ALL,
                                                            sTableFlag,
                                                            1, // aParallelDegree
                                                            & sDictionaryTableList )
                      != IDE_SUCCESS );
        }
        else
        {
            /* Nothing to do */
        }
    }

    /* 5. Target Table을 생성한다. (SM, Meta Table, Meta Cache)
     *  - Source Table의 Table Option을 사용하여, Target Table을 생성한다. (SM)
     *  - Meta Table에 Target Table 정보를 추가한다. (Meta Table)
     *      - SYS_TABLES_
     *          - SM에서 얻은 Table OID가 SYS_TABLES_에 필요하다.
     *          - REPLICATION_COUNT, REPLICATION_RECOVERY_COUNT는 초기화한다.
     *          - LAST_DDL_TIME을 초기화한다. (SYSDATE)
     *      - SYS_COLUMNS_
     *      - SYS_ENCRYPTED_COLUMNS_
     *      - SYS_LOBS_
     *      - SYS_COMPRESSION_TABLES_
     *          - Call : qdbCommon::insertCompressionTableSpecIntoMeta()
     *      - SYS_PART_TABLES_
     *      - SYS_PART_KEY_COLUMNS_
     *  - Table Info를 생성하고, SM에 등록한다. (Meta Cache)
     *  - Table Info를 얻는다.
     */

    // Table에서 Replication 정보를 제거한다.
    sTableFlag  = sSourceTableInfo->tableFlag;
    sTableFlag &= ( ~SMI_TABLE_REPLICATION_MASK );
    sTableFlag |= SMI_TABLE_REPLICATION_DISABLE;
    sTableFlag &= ( ~SMI_TABLE_REPLICATION_TRANS_WAIT_MASK );
    sTableFlag |= SMI_TABLE_REPLICATION_TRANS_WAIT_DISABLE;

    IDE_TEST( qdbCommon::createTableOnSM( aStatement,
                                          sParseTree->columns,
                                          sParseTree->userID,
                                          sTableID,
                                          sSourceTableInfo->maxrows,
                                          sSourceTableInfo->TBSID,
                                          sSourceTableInfo->segAttr,
                                          sSourceTableInfo->segStoAttr,
                                          QDB_TABLE_ATTR_MASK_ALL,
                                          sTableFlag,
                                          sSourceTableInfo->parallelDegree,
                                          & sTableOID )
              != IDE_SUCCESS );

    /* TASK-7307 DML Data Consistency in Shard */
    if ( sParseTree->mShardFlag == QCM_SHARD_FLAG_TABLE_BACKUP )
    {
        sShardFlag &= ~QCM_TABLE_SHARD_FLAG_TABLE_MASK;
        sShardFlag |= QCM_TABLE_SHARD_FLAG_TABLE_BACKUP;
    }
    else
    {
        /* Nothing to do */
    }

    IDE_TEST( qdbCommon::insertTableSpecIntoMeta( aStatement,
                                                  sIsPartitioned,
                                                  sParseTree->flag,
                                                  sParseTree->tableName,
                                                  sParseTree->userID,
                                                  sTableOID,
                                                  sTableID,
                                                  sSourceTableInfo->columnCount,
                                                  sSourceTableInfo->maxrows,
                                                  sSourceTableInfo->accessOption,
                                                  sSourceTableInfo->TBSID,
                                                  sSourceTableInfo->segAttr,
                                                  sSourceTableInfo->segStoAttr,
                                                  QCM_TEMPORARY_ON_COMMIT_NONE,
                                                  sSourceTableInfo->parallelDegree,
                                                  sShardFlag )
              != IDE_SUCCESS );

    IDE_TEST( qdbCommon::insertColumnSpecIntoMeta( aStatement,
                                                   sParseTree->userID,
                                                   sTableID,
                                                   sParseTree->columns,
                                                   ID_FALSE )
              != IDE_SUCCESS );

    // DEFAULT_VAL는 qtcNode를 요구하므로, 별도로 갱신한다.
    IDE_TEST( qdbCopySwap::updateColumnDefaultValueMeta( aStatement,
                                                         sTableID,
                                                         sSourceTableInfo->tableID,
                                                         sSourceTableInfo->columnCount )
              != IDE_SUCCESS );

    for ( sDictionaryTable = sDictionaryTableList;
          sDictionaryTable != NULL;
          sDictionaryTable = sDictionaryTable->next )
    {
        IDE_TEST( qdbCommon::insertCompressionTableSpecIntoMeta(
                      aStatement,
                      sTableID,                                           // data table id
                      sDictionaryTable->dataColumn->basicInfo->column.id, // data table's column id
                      sDictionaryTable->dictionaryTableInfo->tableID,     // dictionary table id
                      sDictionaryTable->dictionaryTableInfo->maxrows )    // dictionary table's max rows
                  != IDE_SUCCESS );
    }

    if ( sIsPartitioned == ID_TRUE )
    {
        IDE_TEST( qdbCommon::insertPartTableSpecIntoMeta( aStatement,
                                                          sParseTree->userID,
                                                          sTableID,
                                                          sSourceTableInfo->partitionMethod,
                                                          sSourceTableInfo->partKeyColCount,
                                                          sSourceTableInfo->rowMovement )
                  != IDE_SUCCESS );

        IDE_TEST( qdbCommon::insertPartKeyColumnSpecIntoMeta( aStatement,
                                                              sParseTree->userID,
                                                              sTableID,
                                                              sParseTree->columns,
                                                              sSourceTableInfo->partKeyColumns,
                                                              QCM_TABLE_OBJECT_TYPE )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing to do */
    }

    IDE_TEST( qcm::makeAndSetQcmTableInfo( QC_SMI_STMT( aStatement ),
                                           sTableID,
                                           sTableOID )
              != IDE_SUCCESS );

    IDE_TEST( qcm::getTableInfoByID( aStatement,
                                     sTableID,
                                     & sTableInfo,
                                     & sSCN,
                                     & sTableHandle )
              != IDE_SUCCESS );

    IDE_TEST( qcm::validateAndLockTable( aStatement,
                                         sTableHandle,
                                         sSCN,
                                         SMI_TABLE_LOCK_X )
              != IDE_SUCCESS );

    /* 6. Partitioned Table이면, Partition을 생성한다.
     *  - Partition을 생성한다.
     *      - Next Table Partition ID를 얻는다.
     *      - Partition을 생성한다. (SM)
     *      - Meta Table에 Target Partition 정보를 추가한다. (Meta Table)
     *          - SYS_TABLE_PARTITIONS_
     *              - SM에서 얻은 Partition OID가 SYS_TABLE_PARTITIONS_에 필요하다.
     *              - REPLICATION_COUNT, REPLICATION_RECOVERY_COUNT는 초기화한다.
     *          - SYS_PART_LOBS_
     *  - Partition Info를 생성하고, SM에 등록한다. (Meta Cache)
     *  - Partition Info를 얻는다.
     */
    if ( sIsPartitioned == ID_TRUE )
    {
        IDE_TEST( executeCreateTablePartition( aStatement,
                                               sTableInfo,
                                               sSourceTableInfo,
                                               sSourcePartInfoList,
                                               & sPartInfoList )
                  != IDE_SUCCESS );

        IDE_TEST( qcmPartition::validateAndLockPartitionInfoList( aStatement,
                                                                  sPartInfoList,
                                                                  SMI_TBSLV_DDL_DML, // TBS Validation 옵션
                                                                  SMI_TABLE_LOCK_X,
                                                                  smiGetDDLLockTimeOut((QC_SMI_STMT(aStatement))->getTrans()))
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing to do */
    }

    /* 7. Target Table에 Constraint를 생성한다.
     *  - Next Constraint ID를 얻는다.
     *  - CONSTRAINT_NAME에 사용자가 지정한 Prefix를 붙여서 CONSTRAINT_NAME을 생성한다.
     *      - 생성한 CONSTRAINT_NAME이 Unique한지 확인해야 한다.
     *          - CONSTRAINT_NAME은 Unique Index의 Column이 아니다. 코드로 Unique를 검사한다.
     *  - Meta Table에 Target Table의 Constraint 정보를 추가한다. (Meta Table)
     *      - SYS_CONSTRAINTS_
     *          - Primary Key, Unique, Local Unique인 경우, Index ID가 SYS_CONSTRAINTS_에 필요하다.
     *      - SYS_CONSTRAINT_COLUMNS_
     *      - SYS_CONSTRAINT_RELATED_
     */
    /* 8. Target Table에 Index를 생성한다.
     *  - Source Table의 Index 정보를 사용하여, Target Table의 Index를 생성한다. (SM)
     *      - Next Index ID를 얻는다.
     *      - INDEX_NAME에 사용자가 지정한 Prefix를 붙여서 INDEX_NAME을 생성한다.
     *      - Target Table의 Table Handle이 필요하다.
     *  - Meta Table에 Target Table의 Index 정보를 추가한다. (Meta Table)
     *      - SYS_INDICES_
     *          - INDEX_TABLE_ID는 0으로 초기화한다.
     *          - LAST_DDL_TIME을 초기화한다. (SYSDATE)
     *      - SYS_INDEX_COLUMNS_
     *      - SYS_INDEX_RELATED_
     *
     *  - Partitioned Table이면, Local Index 또는 Non-Partitioned Index를 생성한다.
     *      - Local Index를 생성한다.
     *          - Local Index를 생성한다. (SM)
     *          - Meta Table에 Target Partition 정보를 추가한다. (Meta Table)
     *              - SYS_PART_INDICES_
     *              - SYS_INDEX_PARTITIONS_
     *
     *      - Non-Partitioned Index를 생성한다.
     *          - INDEX_NAME으로 Index Table Name, Key Index Name, Rid Index Name을 결정한다.
     *              - Call : qdx::checkIndexTableName()
     *          - Non-Partitioned Index를 생성한다.
     *              - Index Table을 생성한다. (SM, Meta Table, Meta Cache)
     *              - Index Table의 Index를 생성한다. (SM, Meta Table)
     *              - Index Table Info를 다시 얻는다. (Meta Cache)
     *              - Call : qdx::createIndexTable(), qdx::createIndexTableIndices()
     *          - Index Table ID를 갱신한다. (SYS_INDICES_.INDEX_TABLE_ID)
     *              - Call : qdx::updateIndexSpecFromMeta()
     */
    IDE_TEST( createConstraintAndIndexFromInfo( aStatement,
                                                sSourceTableInfo,
                                                sTableInfo,
                                                sPartitionCount,
                                                sPartInfoList,
                                                SMI_INDEX_BUILD_UNCOMMITTED_ROW_ENABLE,
                                                sTableIndices,
                                                sPartIndices,
                                                sPartIndicesCount,
                                                sSourceIndexTableList,
                                                & sIndexTableList,
                                                sParseTree->mNamesPrefix )
              != IDE_SUCCESS );

    /* 9. Target Table에 Trigger를 생성한다.
     *  - Source Table의 Trigger 정보를 사용하여, Target Table의 Trigger를 생성한다. (SM)
     *      - TRIGGER_NAME에 사용자가 지정한 Prefix를 붙여서 TRIGGER_NAME을 생성한다.
     *      - Trigger Strings에 생성한 TRIGGER_NAME과 Target Table Name을 적용한다.
     *  - Meta Table에 Target Table의 Trigger 정보를 추가한다. (Meta Table)
     *      - SYS_TRIGGERS_
     *          - 생성한 TRIGGER_NAME을 사용한다.
     *          - SM에서 얻은 Trigger OID가 SYS_TRIGGERS_에 필요하다.
     *          - 변경한 Trigger String으로 SUBSTRING_CNT, STRING_LENGTH를 만들어야 한다.
     *          - LAST_DDL_TIME을 초기화한다. (SYSDATE)
     *      - SYS_TRIGGER_STRINGS_
     *          - 갱신한 Trigger Strings을 잘라 넣는다.
     *      - SYS_TRIGGER_UPDATE_COLUMNS_
     *          - 새로운 TABLE_ID를 이용하여 COLUMN_ID를 만들어야 한다.
     *      - SYS_TRIGGER_DML_TABLES_
     */
    IDE_TEST( qdnTrigger::executeCopyTable( aStatement,
                                            sSourceTableInfo,
                                            sTableInfo,
                                            sParseTree->mNamesPrefix,
                                            & sTargetTriggerCache )
              != IDE_SUCCESS );

    /* 10. Target Table Info와 Target Partition Info List를 다시 얻는다.
     *  - Table Info를 제거한다.
     *  - Table Info를 생성하고, SM에 등록한다.
     *  - Table Info를 얻는다.
     *  - Partition Info List를 제거한다.
     *  - Partition Info List를 생성하고, SM에 등록한다.
     *  - Partition Info List를 얻는다.
     */
    (void)qcm::destroyQcmTableInfo( sTableInfo );
    sTableInfo = NULL;

    IDE_TEST( qcm::makeAndSetQcmTableInfo( QC_SMI_STMT( aStatement ),
                                           sTableID,
                                           sTableOID )
              != IDE_SUCCESS );

    IDE_TEST( qcm::getTableInfoByID( aStatement,
                                     sTableID,
                                     & sTableInfo,
                                     & sSCN,
                                     & sTableHandle )
              != IDE_SUCCESS );

    if ( sIsPartitioned == ID_TRUE )
    {
        IDE_TEST( qcmPartition::makeQcmPartitionInfoByTableID( QC_SMI_STMT( aStatement ),
                                                               sTableID )
                  != IDE_SUCCESS );

        sOldPartInfoList = sPartInfoList;
        IDE_TEST( qcmPartition::getPartitionInfoList( aStatement,
                                                      QC_SMI_STMT( aStatement ),
                                                      QC_QMX_MEM( aStatement ),
                                                      sTableID,
                                                      & sPartInfoList )
                  != IDE_SUCCESS );

        (void)qcmPartition::destroyQcmPartitionInfoList( sOldPartInfoList );
    }
    else
    {
        /* Nothing to do */
    }

    /* 11. Comment를 복사한다. (Meta Table)
     *  - SYS_COMMENTS_
     *      - TABLE_NAME을 Target Table Name으로 지정한다.
     *      - Hidden Column이면 Function-based Index의 Column이므로, Hidden Column Name을 변경한다.
     *          - Hidden Column Name에 Prefix를 붙인다.
     *              - Hidden Column Name = Index Name + $ + IDX + Number
     *      - 나머지는 그대로 복사한다.
     */
    IDE_TEST( qdbComment::copyCommentsMeta( aStatement,
                                            sSourceTableInfo,
                                            sTableInfo )
              != IDE_SUCCESS );

    /* 12. View에 대해 Recompile & Set Valid를 수행한다.
     *  - RELATED_USER_ID = User ID, RELATED_OBJECT_NAME = Table Name, RELATED_OBJECT_TYPE = 2 인 경우에 해당한다.
     *    (SYS_VIEW_RELATED_)
     *  - Call : qcmView::recompileAndSetValidViewOfRelated()
     */
    IDE_TEST( qcmView::recompileAndSetValidViewOfRelated( aStatement,
                                                          sParseTree->userID,
                                                          sTableInfo->name,
                                                          idlOS::strlen( (SChar *)sTableInfo->name ),
                                                          QS_TABLE )
              != IDE_SUCCESS );

    /* 13. DDL_SUPPLEMENTAL_LOG_ENABLE이 1인 경우, Supplemental Log를 기록한다.
     *  - DDL Statement Text를 기록한다.
     *      - Call : qciMisc::writeDDLStmtTextLog()
     *  - Table Meta Log Record를 기록한다.
     *      - Call : qci::mManageReplicationCallback.mWriteTableMetaLog()
     */
    if ( QCU_DDL_SUPPLEMENTAL_LOG == 1 )
    {
        IDE_TEST( qciMisc::writeDDLStmtTextLog( aStatement,
                                                sParseTree->userID,
                                                sTableInfo->name )
                  != IDE_SUCCESS );

        IDE_TEST( qci::mManageReplicationCallback.mWriteTableMetaLog( aStatement,
                                                                      0,
                                                                      sTableOID )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing to do */
    }

    if (  QCG_GET_SESSION_IS_NEED_DDL_INFO( aStatement ) == ID_TRUE )
    {
        /* BUG-47811 DDL that requires NEED_DDL_INFO (transactional DDL, DDL Sync, DDL ASync )
         * should not have global non parititioned index. */
        IDE_TEST_RAISE( qcm::existGlobalNonPartitionedIndex( sTableInfo, &sTempIndex ) == ID_TRUE,
                        ERR_NOT_SUPPORT_GLOBAL_NON_PARTITION_INDEX );

        qrc::setDDLDestInfo( aStatement,
                             1,
                             &(sTableInfo->tableOID),
                             0,
                             NULL );
    }

    /* 14. Encrypted Column을 보안 모듈에 등록한다.
     *  - 보안 컬럼 생성 시, 보안 모듈에 Table Owner Name, Table Name, Column Name, Policy Name을 등록한다.
     *  - 예외 처리를 하지 않기 위해, 마지막에 수행한다.
     *  - Call : qcsModule::setColumnPolicy()
     */
    qdbCommon::setAllColumnPolicy( sTableInfo );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_SUPPORT_GLOBAL_NON_PARTITION_INDEX );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_ROLLBACKABLE_DDL_GLOBAL_NOT_ALLOWED_NON_PART_INDEX,
                                  sTempIndex->name ) );
    }
    IDE_EXCEPTION( ERR_TOO_LONG_OBJECT_NAME )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDB_USING_TARGET_NAMES_PREFIX_IS_TOO_LONG ) );
    }
    IDE_EXCEPTION_END;

    (void)qcm::destroyQcmTableInfo( sTableInfo );
    (void)qcmPartition::destroyQcmPartitionInfoList( sPartInfoList );

    for ( sDictionaryTable = sDictionaryTableList;
          sDictionaryTable != NULL;
          sDictionaryTable = sDictionaryTable->next )
    {
        (void)qcm::destroyQcmTableInfo( sDictionaryTable->dictionaryTableInfo );
    }

    for ( sIndexTable = sIndexTableList;
          sIndexTable != NULL;
          sIndexTable = sIndexTable->next )
    {
        (void)qcm::destroyQcmTableInfo( sIndexTable->tableInfo );
    }

    // Trigger Cache는 Table Meta Cache와 별도로 존재한다.
    if ( sSourceTableInfo != NULL )
    {
        qdnTrigger::freeTriggerCacheArray( sTargetTriggerCache,
                                           sSourceTableInfo->triggerCount );
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_FAILURE;
}

IDE_RC qdbCopySwap::validateReplaceTable( qcStatement * aStatement )
{
/***********************************************************************
 * Description :
 *      PROJ-2600 Online DDL for Tablespace Alteration
 *
 *      ALTER TABLE [user_name.]target_table_name
 *          REPLACE [user_name.]source_table_name
 *          [USING PREFIX name_prefix]
 *          [RENAME FORCE]
 *          [IGNORE FOREIGN KEY CHILD];
 *      구문의 Validation
 *
 * Implementation :
 *
 *  1. Target Table에 대해 ALTER TABLE 구문의 공통적인 Validation를 수행한다.
 *      - Table Name이 X$, D$, V$로 시작하지 않아야 한다.
 *      - User ID와 Table Info를 얻는다.
 *      - Table에 IS Lock을 잡는다.
 *      - ALTER TABLE을 실행할 수 있는 Table인지 확인한다.
 *      - Table 변경 권한을 검사한다.
 *      - DDL_SUPPLEMENTAL_LOG_ENABLE = 1인 경우, Supplemental Log를 기록한다.
 *          - DDL Statement Text를 기록한다.
 *      - Call : qdbAlter::validateAlterCommon()
 *
 *  2. Target이 Partitioned Table이면, 모든 Partition Info를 얻고 IS Lock을 잡는다.
 *      변경: BUG-47208 partitioned IS, range, range hash, list partition
 *           replace 대상 partition IS    
 *
 *  3. Target이 Partitioned Table이면, 모든 Non-Partitioned Index Info를 얻고 IS Lock을 잡는다.
 *
 *  4. Source Table에 대해 Validation를 수행한다.
 *      - Table Info를 얻는다.
 *      - Target Table Owner와 Source Table Owner가 같아야 한다.
 *      - Table에 IS Lock을 잡는다.
 *      - ALTER TABLE을 실행할 수 있는 Table인지 확인한다.
 *      - Table 변경 권한을 검사한다.
 *
 *  5. Source가 Partitioned Table이면, 모든 Partition Info를 얻고 IS Lock을 잡는다.
 *      변경: BUG-47208 partitioned IS, range, range hash, list partition
 *           replace 대상 partition IS
 *
 *  6. Source가 Partitioned Table이면, 모든 Non-Partitioned Index Info를 얻고 IS Lock을 잡는다.
 *
 *  7. Source와 Target은 다른 Table ID를 가져야 한다.
 *
 *  8. Source와 Target이 일반 Table인지 검사한다.
 *      - TABLE_TYPE = 'T', TEMPORARY = 'N', HIDDEN = 'N' 이어야 한다. (SYS_TABLES_)
 *
 *  9. Source와 Target의 Compressed Column 제약을 검사한다.
 *      - Replication이 걸린 경우, Compressed Column을 지원하지 않는다.
 *
 *  10. Encrypt Column 제약을 확인한다.
 *      - RENAME FORCE 절을 지정하지 않은 경우, Encrypt Column을 지원하지 않는다.
 *
 *  11. Foreign Key Constraint (Parent) 제약을 확인한다.
 *      - Referenced Index를 참조하는 Ref Child Info List를 만든다.
 *          - 각 Ref Child Info는 Table/Partition Ref를 가지고 있고 IS Lock을 획득했다.
 *          - Ref Child Info List에서 Self Foreign Key를 제거한다.
 *      - Referenced Index를 참조하는 Ref Child Table/Partition List를 만든다.
 *          - Ref Child Table/Partition List에서 Table 중복을 제거한다.
 *      - Referenced Index가 있으면, IGNORE FOREIGN KEY CHILD 절이 있어야 한다.
 *      - Referenced Index 역할을 할 Index가 Peer에 있는지 확인한다.
 *          - REFERENCED_INDEX_ID가 가리키는 Index의 Column Name으로 구성된 Index를 Peer에서 찾는다.
 *              - Primary이거나 Unique이어야 한다.
 *                  - Primary/Unique Key Constraint가 Index로 구현되어 있으므로, Index에서 찾는다.
 *                  - Local Unique는 Foreign Key Constraint 대상이 아니다.
 *              - Column Count가 같아야 한다.
 *              - Column Name 순서가 같아야 한다.
 *                  - Foreign Key 생성 시에는 순서가 달라도 지원하나, 여기에서는 지원하지 않는다.
 *              - Data Type, Language가 같아야 한다.
 *                  - Precision, Scale은 달라도 된다.
 *              - 참조 Call : qdnForeignKey::validateForeignKeySpec()
 *          - 대응하는 Index가 Peer에 없으면, 실패시킨다.
 *
 *  12. Replication 제약 사항을 확인한다.
 *      - 한쪽이라도 Replication 대상이면, Source와 Target 둘 다 Partitioned Table이거나 아니어야 한다.
 *          - SYS_TABLES_
 *              - REPLICATION_COUNT > 0 이면, Table이 Replication 대상이다.
 *              - IS_PARTITIONED : 'N'(Non-Partitioned Table), 'Y'(Partitioned Table)
 *      - Partitioned Table인 경우, 기본 정보가 같아야 한다.
 *          - SYS_PART_TABLES_
 *              - PARTITION_METHOD가 같아야 한다.
 *      - Partition이 Replication 대상인 경우, 양쪽에 같은 Partition이 존재해야 한다.
 *          - SYS_TABLE_PARTITIONS_
 *              - REPLICATION_COUNT > 0 이면, Partition이 Replication 대상이다.
 *              - PARTITION_NAME이 같으면, 기본 정보를 비교한다.
 *                  - PARTITION_MIN_VALUE, PARTITION_MAX_VALUE, PARTITION_ORDER가 같아야 한다.
 *      - 양쪽 Table이 같은 Replication에 속하면 안 된다.
 *          - Replication에서 Table Meta Log를 하나씩 처리하므로, 이 기능을 지원하지 않는다.
 *      변경: BUG-47208 range, range hash, list partition replace 대상 partition에 대해 위 제약 사항 체크
 *
 *  13. partition table swap validate 수행
 *
 ***********************************************************************/

    qdTableParseTree        * sParseTree            = NULL;
    smOID                   * sDDLTableOIDArray     = NULL;
    qcmTableInfo            * sTableInfo            = NULL;
    qcmTableInfo            * sSourceTableInfo      = NULL;
    qcmRefChildInfo         * sRefChildInfo         = NULL;
    qcmIndex                * sPeerIndex            = NULL;
    UInt                      sSourceUserID         = 0;

    sParseTree = (qdTableParseTree *)aStatement->myPlan->parseTree;

    IDU_FIT_POINT( "qdbCopySwap::validateReplaceTable::alloc::mSourcePartTable",
                   idERR_ABORT_InsufficientMemory );

    IDE_TEST( QC_QMP_MEM( aStatement )->alloc( ID_SIZEOF( qdPartitionedTableList ),
                                               (void **) & sParseTree->mSourcePartTable )
              != IDE_SUCCESS );
    QD_SET_INIT_PART_TABLE_LIST( sParseTree->mSourcePartTable );

    /* 1. Target Table에 대해 ALTER TABLE 구문의 공통적인 Validation를 수행한다.
     *  - Table Name이 X$, D$, V$로 시작하지 않아야 한다.
     *  - User ID와 Table Info를 얻는다.
     *  - Table에 IS Lock을 잡는다.
     *  - ALTER TABLE을 실행할 수 있는 Table인지 확인한다.
     *  - Table 변경 권한을 검사한다.
     *  - DDL_SUPPLEMENTAL_LOG_ENABLE = 1인 경우, Supplemental Log를 기록한다.
     *      - DDL Statement Text를 기록한다.
     *  - Call : qdbAlter::validateAlterCommon()
     */
    IDE_TEST( qdbAlter::validateAlterCommon( aStatement, ID_FALSE ) != IDE_SUCCESS );

    /* BUG-48290 shard object에 대한 DDL 차단(destination check) */
    IDE_TEST( sdi::checkShardObjectForDDLInternal( aStatement,
                                                   sParseTree->userName,
                                                   sParseTree->tableName,
                                                   NULL,    /* aObjectName */
                                                   ID_TRUE, /* aIsGlobalDDLAllowedOnNonShardObject */
                                                   ID_FALSE /* aIsGlobalDDLAllowedOnShardObj */ )
              != IDE_SUCCESS );

    sTableInfo = sParseTree->tableInfo;

    /* 2. Target이 Partitioned Table이면, 모든 Partition Info를 얻고 IS Lock을 잡는다. */
    /* 3. Target이 Partitioned Table이면, 모든 Non-Partitioned Index Info를 얻고 IS Lock을 잡는다. */
    if ( sTableInfo->tablePartitionType == QCM_PARTITIONED_TABLE )
    {
        if ( sParseTree->mPartAttr == NULL ) 
        {
            IDE_TEST( qdbCommon::checkAndSetAllPartitionInfo( aStatement,
                                                              sTableInfo->tableID,
                                                              & sParseTree->partTable->partInfoList )
                      != IDE_SUCCESS );

            IDE_TEST( qdx::makeAndLockIndexTableList( aStatement,
                                                      ID_FALSE,
                                                      sTableInfo,
                                                      & sParseTree->oldIndexTables )
                      != IDE_SUCCESS );
        }
        else
        {
            // Target이 HASH PARTITION 이면 전체 partition IS LOCK
            if ( sTableInfo->partitionMethod == QCM_PARTITION_METHOD_HASH )
            {
                IDE_TEST( qdbCommon::checkAndSetAllPartitionInfo( aStatement,
                                                                  sTableInfo->tableID,
                                                                  & sParseTree->partTable->partInfoList )
                          != IDE_SUCCESS );

                IDE_TEST( qdx::makeAndLockIndexTableList( aStatement,
                                                          ID_FALSE,
                                                          sTableInfo,
                                                          & sParseTree->oldIndexTables )
                          != IDE_SUCCESS );
            }
            else
            {
                IDE_TEST( qdbCommon::checkPartitionInfo( aStatement,
                                                         sTableInfo,
                                                         sParseTree->mPartAttr->tablePartName )
                          != IDE_SUCCESS );
    
                IDE_TEST( qdx::makeAndLockIndexTableList( aStatement,
                                                          ID_FALSE,
                                                          sTableInfo,
                                                          & sParseTree->oldIndexTables )
                          != IDE_SUCCESS );
            }       
        }
    }
    else
    {
        /* Nothing to do */
    }

    /* 4. Source Table에 대해 Validation를 수행한다.
     *  - Table Info를 얻는다.
     *  - Target Table Owner와 Source Table Owner가 같아야 한다.
     *  - Table에 IS Lock을 잡는다.
     *  - ALTER TABLE을 실행할 수 있는 Table인지 확인한다.
     *  - Table 변경 권한을 검사한다.
     */
    IDE_TEST( qdbCommon::checkTableInfo( aStatement,
                                         sParseTree->mSourceUserName,
                                         sParseTree->mSourceTableName,
                                         & sSourceUserID,
                                         & sParseTree->mSourcePartTable->mTableInfo,
                                         & sParseTree->mSourcePartTable->mTableHandle,
                                         & sParseTree->mSourcePartTable->mTableSCN )
              != IDE_SUCCESS );

    /* BUG-48290 sharding object에 대한 DDL 차단(source check) */
    IDE_TEST( sdi::checkShardObjectForDDLInternal( aStatement,
                                                   sParseTree->mSourceUserName,
                                                   sParseTree->mSourceTableName,
                                                   NULL,    /* aObjectName */
                                                   ID_TRUE, /* aIsGlobalDDLAllowedOnNonShardObject */
                                                   ID_FALSE /* aIsGlobalDDLAllowedOnShardObj */ )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( sSourceUserID != sParseTree->userID, ERR_DIFFERENT_TABLE_OWNER );

    IDE_TEST( qcm::lockTableForDDLValidation( aStatement,
                                              sParseTree->mSourcePartTable->mTableHandle,
                                              sParseTree->mSourcePartTable->mTableSCN )
              != IDE_SUCCESS );

    sSourceTableInfo = sParseTree->mSourcePartTable->mTableInfo;

    IDE_TEST( qdbAlter::checkOperatable( aStatement,
                                         sSourceTableInfo )
              != IDE_SUCCESS );

    IDE_TEST( qdpRole::checkDDLAlterTablePriv( aStatement,
                                               sSourceTableInfo )
              != IDE_SUCCESS );

    /* 5. Source가 Partitioned Table이면, 모든 Partition Info를 얻고 IS Lock을 잡는다. */
    /* 6. Source가 Partitioned Table이면, 모든 Non-Partitioned Index Info를 얻고 IS Lock을 잡는다. */
    if ( sSourceTableInfo->tablePartitionType == QCM_PARTITIONED_TABLE )
    {
        if ( sParseTree->mPartAttr == NULL )
        {
            IDE_TEST( qdbCommon::checkAndSetAllPartitionInfo( aStatement,
                                                              sSourceTableInfo->tableID,
                                                              & sParseTree->mSourcePartTable->mPartInfoList )
                      != IDE_SUCCESS );

            IDE_TEST( qdx::makeAndLockIndexTableList( aStatement,
                                                      ID_FALSE,
                                                      sSourceTableInfo,
                                                      & sParseTree->mSourcePartTable->mIndexTableList )
                      != IDE_SUCCESS );
        }
        else
        {
            // Source가 HASH PARTITION 이면 전체 partition IS LOCK
            if ( sSourceTableInfo->partitionMethod == QCM_PARTITION_METHOD_HASH )
            {
                IDE_TEST( qdbCommon::checkAndSetAllPartitionInfo( aStatement,
                                                                  sSourceTableInfo->tableID,
                                                                  & sParseTree->mSourcePartTable->mPartInfoList )
                          != IDE_SUCCESS );

                IDE_TEST( qdx::makeAndLockIndexTableList( aStatement,
                                                          ID_FALSE,
                                                          sSourceTableInfo,
                                                          & sParseTree->mSourcePartTable->mIndexTableList )
                          != IDE_SUCCESS );
            }
            else
            {
                IDE_TEST( qdbCommon::checkPartitionInfo( aStatement,
                                                         sSourceTableInfo,
                                                         sParseTree->mPartAttr->tablePartName )
                          != IDE_SUCCESS );
    
                IDE_TEST( qdx::makeAndLockIndexTableList( aStatement,
                                                          ID_FALSE,
                                                          sSourceTableInfo,
                                                          & sParseTree->mSourcePartTable->mIndexTableList )
                          != IDE_SUCCESS );
            }
        }
    }
    else
    {
        /* Nothing to do */
    }

    IDU_FIT_POINT( "qdbCopySwap::validateReplaceTable:replacePartitionISLock" );
    
    /* 7. Source와 Target은 다른 Table ID를 가져야 한다. */
    IDE_TEST_RAISE( sTableInfo->tableID == sSourceTableInfo->tableID,
                    ERR_SOURCE_TARGET_IS_SAME );

    /* 8. Source와 Target이 일반 Table인지 검사한다.
     *  - TABLE_TYPE = 'T', TEMPORARY = 'N', HIDDEN = 'N' 이어야 한다. (SYS_TABLES_)
     */
    IDE_TEST( checkNormalUserTable( aStatement,
                                    sTableInfo,
                                    sParseTree->tableName )
              != IDE_SUCCESS );

    IDE_TEST( checkNormalUserTable( aStatement,
                                    sSourceTableInfo,
                                    sParseTree->mSourceTableName )
              != IDE_SUCCESS );

    /* 9. Source와 Target의 Compressed Column 제약을 검사한다.
     *  - Replication이 걸린 경우, Compressed Column을 지원하지 않는다.
     */
    if ( ( sTableInfo->replicationCount > 0 ) ||
         ( sSourceTableInfo->replicationCount > 0 ) )
    {
        IDE_TEST( checkCompressedColumnForReplication( aStatement,
                                                       sTableInfo,
                                                       sParseTree->tableName )
                  != IDE_SUCCESS );

        IDE_TEST( checkCompressedColumnForReplication( aStatement,
                                                       sSourceTableInfo,
                                                       sParseTree->mSourceTableName )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing to do */
    }

    /* 10. Encrypt Column 제약을 확인한다.
     *  - RENAME FORCE 절을 지정하지 않은 경우, Encrypt Column을 지원하지 않는다.
     */
    IDE_TEST( checkEncryptColumn( sParseTree->mIsRenameForce,
                                  sTableInfo->columns )
              != IDE_SUCCESS );

    IDE_TEST( checkEncryptColumn( sParseTree->mIsRenameForce,
                                  sSourceTableInfo->columns )
              != IDE_SUCCESS );

    /* 11. Foreign Key Constraint (Parent) 제약을 확인한다.
     *  - Referenced Index를 참조하는 Ref Child Info List를 만든다.
     *      - 각 Ref Child Info는 Table/Partition Ref를 가지고 있고 IS Lock을 획득했다.
     *      - Ref Child Info List에서 Self Foreign Key를 제거한다.
     *  - Referenced Index를 참조하는 Ref Child Table/Partition List를 만든다.
     *      - Ref Child Table/Partition List에서 Table 중복을 제거한다.
     *  - Referenced Index가 있으면, IGNORE FOREIGN KEY CHILD 절이 있어야 한다.
     *  - Referenced Index 역할을 할 Index가 Peer에 있는지 확인한다.
     *      - REFERENCED_INDEX_ID가 가리키는 Index의 Column Name으로 구성된 Index를 Peer에서 찾는다.
     *          - Primary이거나 Unique이어야 한다.
     *              - Primary/Unique Key Constraint가 Index로 구현되어 있으므로, Index에서 찾는다.
     *              - Local Unique는 Foreign Key Constraint 대상이 아니다.
     *          - Column Count가 같아야 한다.
     *          - Column Name 순서가 같아야 한다.
     *              - Foreign Key 생성 시에는 순서가 달라도 지원하나, 여기에서는 지원하지 않는다.
     *          - Data Type, Language가 같아야 한다.
     *              - Precision, Scale은 달라도 된다.
     *          - 참조 Call : qdnForeignKey::validateForeignKeySpec()
     *      - 대응하는 Index가 Peer에 없으면, 실패시킨다.
     */
    IDE_TEST( getRefChildInfoList( aStatement,
                                   sTableInfo,
                                   & sParseTree->mTargetRefChildInfoList )
              != IDE_SUCCESS );

    IDE_TEST( getPartitionedTableListFromRefChildInfoList( aStatement,
                                                           sParseTree->mTargetRefChildInfoList,
                                                           & sParseTree->mRefChildPartTableList )
              != IDE_SUCCESS );

    IDE_TEST( getRefChildInfoList( aStatement,
                                   sSourceTableInfo,
                                   & sParseTree->mSourceRefChildInfoList )
              != IDE_SUCCESS );

    IDE_TEST( getPartitionedTableListFromRefChildInfoList( aStatement,
                                                           sParseTree->mSourceRefChildInfoList,
                                                           & sParseTree->mRefChildPartTableList )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( ( sParseTree->mIgnoreForeignKeyChild != ID_TRUE ) &&
                    ( ( sParseTree->mTargetRefChildInfoList != NULL ) ||
                      ( sParseTree->mSourceRefChildInfoList != NULL ) ),
                    ERR_REFERENCED_INDEX_EXISTS );

    for ( sRefChildInfo = sParseTree->mTargetRefChildInfoList;
          sRefChildInfo != NULL;
          sRefChildInfo = sRefChildInfo->next )
    {
        findPeerIndex( sTableInfo,
                       sRefChildInfo->parentIndex,
                       sSourceTableInfo,
                       & sPeerIndex );

        IDE_TEST_RAISE( sPeerIndex == NULL, ERR_PEER_INDEX_NOT_EXISTS );
    }

    for ( sRefChildInfo = sParseTree->mSourceRefChildInfoList;
          sRefChildInfo != NULL;
          sRefChildInfo = sRefChildInfo->next )
    {
        findPeerIndex( sSourceTableInfo,
                       sRefChildInfo->parentIndex,
                       sTableInfo,
                       & sPeerIndex );

        IDE_TEST_RAISE( sPeerIndex == NULL, ERR_PEER_INDEX_NOT_EXISTS );
    }

    /* 12. Replication 제약 사항을 확인한다.
     *  - 한쪽이라도 Replication 대상이면, Source와 Target 둘 다 Partitioned Table이거나 아니어야 한다.
     *      - SYS_TABLES_
     *          - REPLICATION_COUNT > 0 이면, Table이 Replication 대상이다.
     *          - IS_PARTITIONED : 'N'(Non-Partitioned Table), 'Y'(Partitioned Table)
     *  - Partitioned Table인 경우, 기본 정보가 같아야 한다.
     *      - SYS_PART_TABLES_
     *          - PARTITION_METHOD가 같아야 한다.
     *  - Partition이 Replication 대상인 경우, 양쪽에 같은 Partition이 존재해야 한다.
     *      - SYS_TABLE_PARTITIONS_
     *          - REPLICATION_COUNT > 0 이면, Partition이 Replication 대상이다.
     *          - PARTITION_NAME이 같으면, 기본 정보를 비교한다.
     *              - PARTITION_MIN_VALUE, PARTITION_MAX_VALUE, PARTITION_ORDER가 같아야 한다.
     *  - 양쪽 Table이 같은 Replication에 속하면 안 된다.
     *      - Replication에서 Table Meta Log를 하나씩 처리하므로, 이 기능을 지원하지 않는다.
     */

    if ( sParseTree->mPartAttr == NULL ) 
    {
        IDE_TEST( compareReplicationInfo( aStatement,
                                          sTableInfo,
                                          sParseTree->partTable->partInfoList,
                                          sSourceTableInfo,
                                          sParseTree->mSourcePartTable->mPartInfoList )
                  != IDE_SUCCESS );

        IDE_TEST( compareReplicationInfo( aStatement,
                                          sSourceTableInfo,
                                          sParseTree->mSourcePartTable->mPartInfoList,
                                          sTableInfo,
                                          sParseTree->partTable->partInfoList )
                  != IDE_SUCCESS );
    }
    else
    {
        if ( sTableInfo->partitionMethod == QCM_PARTITION_METHOD_HASH )
        {                    
            IDE_TEST( compareReplicationInfo( aStatement,
                                              sTableInfo,
                                              sParseTree->partTable->partInfoList,
                                              sSourceTableInfo,
                                              sParseTree->mSourcePartTable->mPartInfoList )
                      != IDE_SUCCESS );

            IDE_TEST( compareReplicationInfo( aStatement,
                                              sSourceTableInfo,
                                              sParseTree->mSourcePartTable->mPartInfoList,
                                              sTableInfo,
                                              sParseTree->partTable->partInfoList )
                      != IDE_SUCCESS );
        }
        else
        {
            IDE_TEST( compareReplicationInfoPartition( aStatement,
                                                       sParseTree,
                                                       sTableInfo,
                                                       sSourceTableInfo )
                      != IDE_SUCCESS );
            
        }
    }
                
    IDE_TEST( checkTablesExistInOneReplication( aStatement,
                                                sTableInfo,
                                                sSourceTableInfo )
              != IDE_SUCCESS );

    /* 13. partition table swap validate 수행 */
    IDE_TEST( validateReplacePartition( aStatement,
                                        sParseTree )
              != IDE_SUCCESS );

    if ( ( QCG_GET_SESSION_IS_NEED_DDL_INFO( aStatement ) == ID_TRUE ) &&
         ( aStatement->mDDLInfo.mSrcTableOIDCount == 0 ) )
    {
        IDE_TEST( QC_QMP_MEM( aStatement )->alloc( ID_SIZEOF(smOID) * 2, (void**)&sDDLTableOIDArray )
                  != IDE_SUCCESS);
        sDDLTableOIDArray[0] = sTableInfo->tableOID;
        sDDLTableOIDArray[1] = sSourceTableInfo->tableOID;

        qrc::setDDLSrcInfo( aStatement,
                            ID_TRUE,
                            2,
                            sDDLTableOIDArray,
                            0,
                            NULL );
    }
    else
    {
        /* Nothing to do */
    } 

    /* PROJ-2757 Advanced Global DDL */
    if ( ( QCG_GET_SESSION_GLOBAL_DDL( aStatement ) == ID_TRUE ) &&
         ( QCG_GET_SESSION_IS_SHARD_USER_SESSION( aStatement ) == ID_TRUE ) )
    {
        sdiGlobalDDL::setInfo( aStatement,
                               ID_FALSE, /* IsShardObject */
                               SDI_DDL_TYPE_TABLE,
                               NULL,
                               NULL );
        // rewrite
        sParseTree->common.execute = sdiGlobalDDL::executeReplace;
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_DIFFERENT_TABLE_OWNER )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDB_COPY_SWAP_DIFFERENT_TABLE_OWNER ) );
    }
    IDE_EXCEPTION( ERR_SOURCE_TARGET_IS_SAME )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDB_SELF_SWAP_DENIED ) );
    }
    IDE_EXCEPTION( ERR_REFERENCED_INDEX_EXISTS )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDB_REFERENTIAL_CONSTRAINT_EXIST ) );
    }
    IDE_EXCEPTION( ERR_PEER_INDEX_NOT_EXISTS )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDN_REFERENCED_CONSTRAINT_NOT_FOUND ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdbCopySwap::executeReplaceTable( qcStatement * aStatement )
{
/***********************************************************************
 * Description :
 *      PROJ-2600 Online DDL for Tablespace Alteration
 *
 *      ALTER TABLE [user_name.]target_table_name
 *          REPLACE [user_name.]source_table_name
 *          [USING PREFIX name_prefix]
 *          [RENAME FORCE]
 *          [IGNORE FOREIGN KEY CHILD];
 *      구문의 Execution
 *
 * Implementation :
 *
 *  1. Target의 Table에 X Lock을 잡는다.
 *      - Partitioned Table이면, Partition에 X Lock을 잡는다.
 *      - Partitioned Table이면, Non-Partitioned Index에 X Lock을 잡는다.
 *
 *  2. Source의 Table에 X Lock을 잡는다.
 *      - Partitioned Table이면, Partition에 X Lock을 잡는다.
 *      - Partitioned Table이면, Non-Partitioned Index에 X Lock을 잡는다.
 *
 *  3. Referenced Index를 참조하는 Table에 X Lock을 잡는다.
 *      - Partitioned Table이면, Partition에 X Lock을 잡는다.
 *
 *  4. Replication 대상 Table인 경우, 제약 조건을 검사하고 Receiver Thread를 중지한다.
 *      - REPLICATION_DDL_ENABLE 시스템 프라퍼티가 1 이어야 한다.
 *      - REPLICATION 세션 프라퍼티가 NONE이 아니어야 한다.
 *      - Eager Sender/Receiver Thread를 확인하고, Receiver Thread를 중지한다.
 *          - Non-Partitioned Table이면 Table OID를 얻고, Partitioned Table이면 모든 Partition OID와 Count를 얻는다.
 *          - 해당 Table 관련 Eager Sender/Receiver Thread가 없어야 한다.
 *          - 해당 Table 관련 Receiver Thread를 중지한다.
 *
 *  5. Source와 Target의 Table 기본 정보를 교환한다. (Meta Table)
 *      - SYS_TABLES_
 *          - TABLE_NAME, REPLICATION_COUNT, REPLICATION_RECOVERY_COUNT를 교환한다.
 *          - LAST_DDL_TIME을 갱신한다. (SYSDATE)
 *
 *  6. Hidden Column이면 Function-based Index의 Column이므로,
 *     사용자가 Prefix를 지정한 경우, Hidden Column Name을 변경한다. (Meta Table)
 *      - SYS_COLUMNS_
 *          - Source의 Hidden Column Name에 Prefix를 붙이고, Target의 Hidden Column Name에서 Prefix를 제거한다.
 *              - Hidden Column Name = Index Name + $ + IDX + Number
 *          - Hidden Column Name을 변경한다.
 *      - SYS_ENCRYPTED_COLUMNS_, SYS_LOBS_, SYS_COMPRESSION_TABLES_
 *          - 변경 사항 없음
 *
 *  7. Prefix를 지정한 경우, Source의 INDEX_NAME에 Prefix를 붙이고, Target의 INDEX_NAME에서 Prefix를 제거한다.
 *      - 실제 Index Name을 변경한다. (SM)
 *          - Call : smiTable::alterIndexName()
 *      - Meta Table에서 Index Name을 변경한다. (Meta Table)
 *          - SYS_INDICES_
 *              - INDEX_NAME을 변경한다.
 *              - LAST_DDL_TIME을 갱신한다. (SYSDATE)
 *          - SYS_INDEX_COLUMNS_, SYS_INDEX_RELATED_
 *              - 변경 사항 없음
 *
 *  8. Prefix를 지정한 경우, Non-Partitioned Index이 있으면 Name을 변경한다.
 *      - Non-Partitioned Index인 경우, (1) Index Table Name과 (2) Index Table의 Index Name을 변경한다.
 *          - Non-Partitioned Index는 INDEX_NAME으로 Index Table Name, Key Index Name, RID Index Name을 결정한다.
 *              - Index Table Name = $GIT_ + Index Name
 *              - Key Index Name = $GIK_ + Index Name
 *              - Rid Index Name = $GIR_ + Index Name
 *              - Call : qdx::makeIndexTableName()
 *          - Index Table Name을 변경한다. (Meta Table)
 *          - Index Table의 Index Name을 변경한다. (SM, Meta Table)
 *              - Call : smiTable::alterIndexName()
 *          - Index Table Info를 다시 얻는다. (Meta Cache)
 *
 *  9. Prefix를 지정한 경우, Source의 CONSTRAINT_NAME에 Prefix를 붙이고,
 *     Target의 CONSTRAINT_NAME에서 Prefix를 제거한다. (Meta Table)
 *      - SYS_CONSTRAINTS_
 *          - CONSTRAINT_NAME을 변경한다.
 *              - 변경한 CONSTRAINT_NAME이 Unique한지 확인해야 한다.
 *                  - CONSTRAINT_NAME은 Unique Index의 Column이 아니다.
 *      - SYS_CONSTRAINT_COLUMNS_, SYS_CONSTRAINT_RELATED_
 *          - 변경 사항 없음
 *
 *  10. 변경한 Trigger Name과 교환한 Table Name을 Trigger Strings에 반영하고 Trigger를 재생성한다.
 *      - Prefix를 지정한 경우, Source의 TRIGGER_NAME에 Prefix를 붙이고,
 *        Target의 TRIGGER_NAME에서 Prefix를 제거한다. (Meta Table)
 *          - SYS_TRIGGERS_
 *              - TRIGGER_NAME을 변경한다.
 *              - 변경한 Trigger String으로 SUBSTRING_CNT, STRING_LENGTH를 만들어야 한다.
 *              - LAST_DDL_TIME을 갱신한다. (SYSDATE)
 *      - Trigger Strings에 변경한 Trigger Name과 교환한 Table Name을 적용한다. (SM, Meta Table, Meta Cache)
 *          - Trigger Object의 Trigger 생성 구문을 변경한다. (SM)
 *              - Call : smiObject::setObjectInfo()
 *          - New Trigger Cache를 생성하고 SM에 등록한다. (Meta Cache)
 *              - Call : qdnTrigger::allocTriggerCache()
 *          - Trigger Strings을 보관하는 Meta Table을 갱신한다. (Meta Table)
 *              - SYS_TRIGGER_STRINGS_
 *                  - DELETE & INSERT로 처리한다.
 *      - Trigger를 동작시키는 Column 정보에는 변경 사항이 없다.
 *          - SYS_TRIGGER_UPDATE_COLUMNS_
 *      - 다른 Trigger가 Cycle Check에 사용하는 정보를 갱신한다. (Meta Table)
 *          - SYS_TRIGGER_DML_TABLES_
 *              - DML_TABLE_ID = Table ID 이면, (TABLE_ID와 무관하게) DML_TABLE_ID를 Peer의 Table ID로 교체한다.
 *      - 참조 Call : qdnTrigger::executeRenameTable()
 *
 *  11. Source와 Target의 Table Info를 다시 얻는다.
 *      - Table Info를 생성하고, SM에 등록한다. (Meta Cache)
 *      - Table Info를 얻는다.
 *
 *  12. Comment를 교환한다. (Meta Table)
 *      - SYS_COMMENTS_
 *          - TABLE_NAME을 교환한다.
 *          - Hidden Column이면 Function-based Index의 Column이므로,
 *            사용자가 Prefix를 지정한 경우, Hidden Column Name을 변경한다.
 *              - Source의 Hidden Column Name에 Prefix를 붙이고, Target의 Hidden Column Name에서 Prefix를 제거한다.
 *                  - Hidden Column Name = Index Name + $ + IDX + Number
 *
 *  13. 한쪽이라도 Partitioned Table이고 Replication 대상이면, Partition의 Replication 정보를 교환한다. (Meta Table)
 *      - SYS_TABLE_PARTITIONS_
 *          - PARTITION_NAME으로 Matching Partition을 선택한다.
 *          - Matching Partition의 REPLICATION_COUNT, REPLICATION_RECOVERY_COUNT를 교환하고, LAST_DDL_TIME을 갱신한다.
 *              - REPLICATION_COUNT > 0 이면, Partition이 Replication 대상이다.
 *      - Partition의 다른 정보는 변경 사항이 없다.
 *          - SYS_INDEX_PARTITIONS_
 *              - INDEX_PARTITION_NAME은 Partitioned Table의 Index 내에서만 Unique하면 되므로, Prefix가 필요하지 않다.
 *          - SYS_PART_TABLES_, SYS_PART_LOBS_, SYS_PART_KEY_COLUMNS_, SYS_PART_INDICES_
 *
 *  14. Partitioned Table이면, Partition Info를 다시 얻는다.
 *      - Partition Info를 생성하고, SM에 등록한다. (Meta Cache)
 *      - Partition Info를 얻는다.
 *
 *  15. Foreign Key Constraint (Parent)가 있으면, Referenced Index를 변경하고 Table Info를 갱신한다.
 *      - Referenced Index를 변경한다. (Meta Table)
 *          - SYS_CONSTRAINTS_
 *              - REFERENCED_INDEX_ID가 가리키는 Index의 Column Name으로 구성된 Index를 Peer에서 찾는다. (Validation과 동일)
 *              - REFERENCED_TABLE_ID와 REFERENCED_INDEX_ID를 Peer의 Table ID와 Index ID로 변경한다.
 *      - Referenced Index를 참조하는 Table의 Table Info를 갱신한다. (Meta Cache)
 *          - Partitioned Table이면, Partition Info를 갱신한다. (Meta Cache)
 *
 *  16. Package, Procedure, Function에 대해 Set Invalid를 수행한다.
 *      - RELATED_USER_ID = User ID, RELATED_OBJECT_NAME = Table Name, RELATED_OBJECT_TYPE = 2 인 경우에 해당한다.
 *        (SYS_PACKAGE_RELATED_, SYS_PROC_RELATED_)
 *      - Call : qcmProc::relSetInvalidProcOfRelated(), qcmPkg::relSetInvalidPkgOfRelated()
 *
 *  17. View에 대해 Set Invalid & Recompile & Set Valid를 수행한다.
 *      - RELATED_USER_ID = User ID, RELATED_OBJECT_NAME = Table Name, RELATED_OBJECT_TYPE = 2 인 경우에 해당한다.
 *        (SYS_VIEW_RELATED_)
 *      - Call : qcmView::setInvalidViewOfRelated(), qcmView::recompileAndSetValidViewOfRelated()
 *
 *  18. Object Privilege를 교환한다. (Meta Table)
 *      - SYS_GRANT_OBJECT_
 *          - OBJ_ID = Table ID, OBJ_TYPE = 'T' 이면, OBJ_ID만 교환한다.
 *
 *  19. Replication 대상 Table인 경우, Replication Meta Table을 수정한다. (Meta Table)
 *      - SYS_REPL_ITEMS_
 *          - SYS_REPL_ITEMS_의 TABLE_OID는 Non-Partitioned Table OID이거나 Partition OID이다.
 *              - Partitioned Table OID는 SYS_REPL_ITEMS_에 없다.
 *          - Non-Partitioned Table인 경우, Table OID를 Peer의 것으로 변경한다.
 *          - Partitioned Table인 경우, Partition OID를 Peer의 것으로 변경한다.
 *
 *  20. Replication 대상 Table이거나 DDL_SUPPLEMENTAL_LOG_ENABLE이 1인 경우, Supplemental Log를 기록한다.
 *      - Non-Partitioned Table이면 Table OID를 얻고, Partitioned Table이면 Partition OID를 얻는다.
 *          - 단, Non-Partitioned Table <-> Partitioned Table 변환인 경우, Partitioned Table에서 Table OID를 얻는다.
 *          - Partitioned Table인 경우, Peer에서 Partition Name이 동일한 Partition을 찾아서 Old OID로 사용한다.
 *              - Peer에 동일한 Partition Name이 없으면, Old OID는 0 이다.
 *      - Table Meta Log Record를 기록한다.
 *          - Call : qci::mManageReplicationCallback.mWriteTableMetaLog()
 *      - 주의 : Replication이 Swap DDL을 Peer에 수행하면 안 된다.
 *          - Copy와 Swap은 짝을 이루는데, Copy를 Peer에 수행하지 않았으므로 Swap을 Peer에 수행하지 않는다.
 *
 *  21. Old Trigger Cache를 제거한다.
 *      - Call : qdnTrigger::freeTriggerCache()
 *      - 예외 처리를 하기 위해, 마지막에 수행한다.
 *
 *  22. Table Name이 변경되었으므로, Encrypted Column을 보안 모듈에 다시 등록한다.
 *      - 보안 모듈에 등록한 기존 Encrypted Column 정보를 제거한다.
 *          - Call : qcsModule::unsetColumnPolicy()
 *      - 보안 모듈에 Table Owner Name, Table Name, Column Name, Policy Name을 등록한다.
 *          - Call : qcsModule::setColumnPolicy()
 *      - 예외 처리를 하지 않기 위해, 마지막에 수행한다.
 *
 *  23. Referenced Index를 참조하는 Table의 Old Table Info를 제거한다.
 *      - Partitioned Table이면, Partition의 Old Partition Info를 제거한다.
 *
 *  24. Prefix를 지정한 경우, Non-Partitioned Index가 있으면 Old Index Table Info를 제거한다.
 *
 *  25. Partitioned Table이면, Old Partition Info를 제거한다.
 *
 *  26. Old Table Info를 제거한다.
 *
 ***********************************************************************/

    qdTableParseTree        * sParseTree                        = NULL;
    smOID                   * sDDLTableOIDArray                 = NULL;
    qcmTableInfo            * sTableInfo                        = NULL;
    qcmPartitionInfoList    * sPartInfoList                     = NULL;
    smOID                   * sPartitionOIDs                    = NULL;
    UInt                      sPartitionCount                   = 0;
    qdIndexTableList        * sIndexTableList                   = NULL;
    qcmTableInfo            * sSourceTableInfo                  = NULL;
    qcmPartitionInfoList    * sSourcePartInfoList               = NULL;
    smOID                   * sSourcePartitionOIDs              = NULL;
    UInt                      sSourcePartitionCount             = 0;
    qdIndexTableList        * sSourceIndexTableList             = NULL;
    qdPartitionedTableList  * sRefChildPartTableList            = NULL;
    qcmTableInfo            * sNewTableInfo                     = NULL;
    qcmPartitionInfoList    * sNewPartInfoList                  = NULL;
    qcmTableInfo            * sNewSourceTableInfo               = NULL;
    qcmPartitionInfoList    * sNewSourcePartInfoList            = NULL;
    qdIndexTableList        * sNewIndexTableList                = NULL;
    qdIndexTableList        * sNewSourceIndexTableList          = NULL;
    qdnTriggerCache        ** sTriggerCache                     = NULL;
    qdnTriggerCache        ** sSourceTriggerCache               = NULL;
    qdnTriggerCache        ** sNewTriggerCache                  = NULL;
    qdnTriggerCache        ** sNewSourceTriggerCache            = NULL;
    qdPartitionedTableList  * sNewRefChildPartTableList         = NULL;
    qdPartitionedTableList  * sPartTableList                    = NULL;
    qdPartitionedTableList  * sNewPartTableList                 = NULL;
    smOID                   * sTableOIDArray                    = NULL;
    UInt                      sTableOIDCount                    = 0;
    UInt                      sDDLSupplementalLog               = QCU_DDL_SUPPLEMENTAL_LOG;
    qcmPartitionInfoList    * sTempOldPartInfoList              = NULL;
    qcmPartitionInfoList    * sTempNewPartInfoList              = NULL;
    qdIndexTableList        * sTempIndexTableList               = NULL;
    void                    * sTempTableHandle                  = NULL;
    smSCN                     sSCN;    

    SMI_INIT_SCN( & sSCN );

    sParseTree = (qdTableParseTree *)aStatement->myPlan->parseTree;

    /* 1. Target의 Table에 X Lock을 잡는다.
     *  - Partitioned Table이면, Partition에 X Lock을 잡는다.
     *  - Partitioned Table이면, Non-Partitioned Index에 X Lock을 잡는다.
     */
    IDE_TEST( qcm::validateAndLockTable( aStatement,
                                         sParseTree->tableHandle,
                                         sParseTree->tableSCN,
                                         SMI_TABLE_LOCK_X )
              != IDE_SUCCESS );

    sTableInfo = sParseTree->tableInfo;

    if ( sTableInfo->tablePartitionType == QCM_PARTITIONED_TABLE )
    {
        IDE_TEST( qcmPartition::validateAndLockPartitionInfoList( aStatement,
                                                                  sParseTree->partTable->partInfoList,
                                                                  SMI_TBSLV_DDL_DML, // TBS Validation 옵션
                                                                  SMI_TABLE_LOCK_X,
                                                                  smiGetDDLLockTimeOut((QC_SMI_STMT(aStatement))->getTrans()))
                  != IDE_SUCCESS );

        sPartInfoList = sParseTree->partTable->partInfoList;

        IDE_TEST( qdx::validateAndLockIndexTableList( aStatement,
                                                      sParseTree->oldIndexTables,
                                                      SMI_TBSLV_DDL_DML, // TBS Validation 옵션
                                                      SMI_TABLE_LOCK_X,
                                                      smiGetDDLLockTimeOut((QC_SMI_STMT(aStatement))->getTrans()))
                  != IDE_SUCCESS );

        sIndexTableList = sParseTree->oldIndexTables;

        IDE_TEST( qcmPartition::getAllPartitionOID( QC_QMX_MEM( aStatement ),
                                                    sPartInfoList,
                                                    & sPartitionOIDs,
                                                    & sPartitionCount )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing to do */
    }

    /* 2. Source의 Table에 X Lock을 잡는다.
     *  - Partitioned Table이면, Partition에 X Lock을 잡는다.
     *  - Partitioned Table이면, Non-Partitioned Index에 X Lock을 잡는다.
     */
    IDE_TEST( qcm::validateAndLockTable( aStatement,
                                         sParseTree->mSourcePartTable->mTableHandle,
                                         sParseTree->mSourcePartTable->mTableSCN,
                                         SMI_TABLE_LOCK_X )
              != IDE_SUCCESS );

    sSourceTableInfo = sParseTree->mSourcePartTable->mTableInfo;

    if ( sSourceTableInfo->tablePartitionType == QCM_PARTITIONED_TABLE )
    {
        IDE_TEST( qcmPartition::validateAndLockPartitionInfoList( aStatement,
                                                                  sParseTree->mSourcePartTable->mPartInfoList,
                                                                  SMI_TBSLV_DDL_DML, // TBS Validation 옵션
                                                                  SMI_TABLE_LOCK_X,
                                                                  smiGetDDLLockTimeOut((QC_SMI_STMT(aStatement))->getTrans()))
                  != IDE_SUCCESS );

        sSourcePartInfoList = sParseTree->mSourcePartTable->mPartInfoList;

        IDE_TEST( qdx::validateAndLockIndexTableList( aStatement,
                                                      sParseTree->mSourcePartTable->mIndexTableList,
                                                      SMI_TBSLV_DDL_DML, // TBS Validation 옵션
                                                      SMI_TABLE_LOCK_X,
                                                      smiGetDDLLockTimeOut((QC_SMI_STMT(aStatement))->getTrans()))
                  != IDE_SUCCESS );

        sSourceIndexTableList = sParseTree->mSourcePartTable->mIndexTableList;

        IDE_TEST( qcmPartition::getAllPartitionOID( QC_QMX_MEM( aStatement ),
                                                    sSourcePartInfoList,
                                                    & sSourcePartitionOIDs,
                                                    & sSourcePartitionCount )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing to do */
    }

    /* 3. Referenced Index를 참조하는 Table에 X Lock을 잡는다.
     *  - Partitioned Table이면, Partition에 X Lock을 잡는다.
     */
    for ( sPartTableList = sParseTree->mRefChildPartTableList;
          sPartTableList != NULL;
          sPartTableList = sPartTableList->mNext )
    {
        IDE_TEST( qcm::validateAndLockTable( aStatement,
                                             sPartTableList->mTableHandle,
                                             sPartTableList->mTableSCN,
                                             SMI_TABLE_LOCK_X )
                  != IDE_SUCCESS );

        IDE_TEST( qcmPartition::validateAndLockPartitionInfoList( aStatement,
                                                                  sPartTableList->mPartInfoList,
                                                                  SMI_TBSLV_DDL_DML, // TBS Validation 옵션
                                                                  SMI_TABLE_LOCK_X,
                                                                  smiGetDDLLockTimeOut((QC_SMI_STMT(aStatement))->getTrans()))
                  != IDE_SUCCESS );
    }

    sRefChildPartTableList = sParseTree->mRefChildPartTableList;

    /* 4. Replication 대상 Table인 경우, 제약 조건을 검사하고 Receiver Thread를 중지한다.
     *  - REPLICATION_DDL_ENABLE 시스템 프라퍼티가 1 이어야 한다.
     *  - REPLICATION 세션 프라퍼티가 NONE이 아니어야 한다.
     *  - Eager Sender/Receiver Thread를 확인하고, Receiver Thread를 중지한다.
     *      - Non-Partitioned Table이면 Table OID를 얻고, Partitioned Table이면 모든 Partition OID와 Count를 얻는다.
     *      - 해당 Table 관련 Eager Sender/Receiver Thread가 없어야 한다.
     *      - 해당 Table 관련 Receiver Thread를 중지한다.
     */
    if ( sTableInfo->replicationCount > 0 )
    {
        /* PROJ-1442 Replication Online 중 DDL 허용
         * Validate와 Execute는 다른 Transaction이므로, 프라퍼티 검사는 Execute에서 한다.
         */
        IDE_TEST( qci::mManageReplicationCallback.mIsDDLEnableOnReplicatedTable( 0, // aRequireLevel
                                                                                 sTableInfo )
                  != IDE_SUCCESS );

        IDE_TEST_RAISE( QC_SMI_STMT( aStatement )->getTrans()->getReplicationMode()
                        == SMI_TRANSACTION_REPL_NONE,
                        ERR_CANNOT_WRITE_REPL_INFO );

        /* PROJ-1442 Replication Online 중 DDL 허용
         * 관련 Receiver Thread 중지
         */
        if ( sTableInfo->tablePartitionType == QCM_PARTITIONED_TABLE )
        {
            sTableOIDArray = sPartitionOIDs;
            sTableOIDCount = sPartitionCount;
        }
        else
        {
            sTableOIDArray = & sTableInfo->tableOID;
            sTableOIDCount = 1;
        }

        IDE_TEST( qciMisc::checkRunningEagerReplicationByTableOID( aStatement,
                                                                   sTableOIDArray,
                                                                   sTableOIDCount )
                  != IDE_SUCCESS );

        // BUG-22703 : Begin Statement를 수행한 후에 Hang이 걸리지
        // 않아야 합니다.
        // mStatistics 통계 정보를 전달 합니다.
        IDE_TEST( qci::mManageReplicationCallback.mStopReceiverThreads( QC_SMI_STMT( aStatement ),
                                                                        QC_STATISTICS( aStatement ),
                                                                        sTableOIDArray,
                                                                        sTableOIDCount )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing to do */
    }

    if ( sSourceTableInfo->replicationCount > 0 )
    {
        /* PROJ-1442 Replication Online 중 DDL 허용
         * Validate와 Execute는 다른 Transaction이므로, 프라퍼티 검사는 Execute에서 한다.
         */
        IDE_TEST( qci::mManageReplicationCallback.mIsDDLEnableOnReplicatedTable( 0, // aRequireLevel
                                                                                 sSourceTableInfo )
                  != IDE_SUCCESS );

        IDE_TEST_RAISE( QC_SMI_STMT( aStatement )->getTrans()->getReplicationMode()
                        == SMI_TRANSACTION_REPL_NONE,
                        ERR_CANNOT_WRITE_REPL_INFO );

        /* PROJ-1442 Replication Online 중 DDL 허용
         * 관련 Receiver Thread 중지
         */
        if ( sSourceTableInfo->tablePartitionType == QCM_PARTITIONED_TABLE )
        {
            sTableOIDArray = sSourcePartitionOIDs;
            sTableOIDCount = sSourcePartitionCount;
        }
        else
        {
            sTableOIDArray = & sSourceTableInfo->tableOID;
            sTableOIDCount = 1;
        }

        IDE_TEST( qciMisc::checkRunningEagerReplicationByTableOID( aStatement,
                                                                   sTableOIDArray,
                                                                   sTableOIDCount )
                  != IDE_SUCCESS );

        // BUG-22703 : Begin Statement를 수행한 후에 Hang이 걸리지
        // 않아야 합니다.
        // mStatistics 통계 정보를 전달 합니다.
        IDE_TEST( qci::mManageReplicationCallback.mStopReceiverThreads( QC_SMI_STMT( aStatement ),
                                                                        QC_STATISTICS( aStatement ),
                                                                        sTableOIDArray,
                                                                        sTableOIDCount )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing to do */
    }

    /* 5. Source와 Target의 Table 기본 정보를 교환한다. (Meta Table)
     *  - SYS_TABLES_
     *      - TABLE_NAME, REPLICATION_COUNT, REPLICATION_RECOVERY_COUNT를 교환한다.
     *      - LAST_DDL_TIME을 갱신한다. (SYSDATE)
     *      - USABLE, SHARD_FLAG를 교환한다. (TASK-7307)
     */
    IDE_TEST( swapTablesMeta( aStatement,
                              sTableInfo->tableID,
                              sSourceTableInfo->tableID )
              != IDE_SUCCESS );

    /* 6. Hidden Column이면 Function-based Index의 Column이므로,
     * 사용자가 Prefix를 지정한 경우, Hidden Column Name을 변경한다. (Meta Table)
     *  - SYS_COLUMNS_
     *      - Source의 Hidden Column Name에 Prefix를 붙이고, Target의 Hidden Column Name에서 Prefix를 제거한다.
     *          - Hidden Column Name = Index Name + $ + IDX + Number
     *      - Hidden Column Name을 변경한다.
     *  - SYS_ENCRYPTED_COLUMNS_, SYS_LOBS_, SYS_COMPRESSION_TABLES_
     *      - 변경 사항 없음
     */
    IDE_TEST( renameHiddenColumnsMeta( aStatement,
                                       sTableInfo,
                                       sSourceTableInfo,
                                       sParseTree->mNamesPrefix )
              != IDE_SUCCESS );

    /* 7. Prefix를 지정한 경우, Source의 INDEX_NAME에 Prefix를 붙이고, Target의 INDEX_NAME에서 Prefix를 제거한다.
     *  - 실제 Index Name을 변경한다. (SM)
     *      - Call : smiTable::alterIndexName()
     *  - Meta Table에서 Index Name을 변경한다. (Meta Table)
     *      - SYS_INDICES_
     *          - INDEX_NAME을 변경한다.
     *          - LAST_DDL_TIME을 갱신한다. (SYSDATE)
     *      - SYS_INDEX_COLUMNS_, SYS_INDEX_RELATED_
     *          - 변경 사항 없음
     */
    /* 8. Prefix를 지정한 경우, Non-Partitioned Index이 있으면 Name을 변경한다.
     *  - Non-Partitioned Index인 경우, (1) Index Table Name과 (2) Index Table의 Index Name을 변경한다.
     *      - Non-Partitioned Index는 INDEX_NAME으로 Index Table Name, Key Index Name, RID Index Name을 결정한다.
     *          - Index Table Name = $GIT_ + Index Name
     *          - Key Index Name = $GIK_ + Index Name
     *          - Rid Index Name = $GIR_ + Index Name
     *          - Call : qdx::makeIndexTableName()
     *      - Index Table Name을 변경한다. (Meta Table)
     *      - Index Table의 Index Name을 변경한다. (SM, Meta Table)
     *          - Call : smiTable::alterIndexName()
     *      - Index Table Info를 다시 얻는다. (Meta Cache)
     */
    IDE_TEST( renameIndices( aStatement,
                             sTableInfo,
                             sSourceTableInfo,
                             sParseTree->mNamesPrefix,
                             sIndexTableList,
                             sSourceIndexTableList,
                             & sNewIndexTableList,
                             & sNewSourceIndexTableList )
              != IDE_SUCCESS );

    /* 9. Prefix를 지정한 경우, Source의 CONSTRAINT_NAME에 Prefix를 붙이고,
     * Target의 CONSTRAINT_NAME에서 Prefix를 제거한다. (Meta Table)
     *  - SYS_CONSTRAINTS_
     *      - CONSTRAINT_NAME을 변경한다.
     *          - 변경한 CONSTRAINT_NAME이 Unique한지 확인해야 한다.
     *              - CONSTRAINT_NAME은 Unique Index의 Column이 아니다.
     *  - SYS_CONSTRAINT_COLUMNS_, SYS_CONSTRAINT_RELATED_
     *      - 변경 사항 없음
     */
    IDE_TEST( renameConstraintsMeta( aStatement,
                                     sTableInfo,
                                     sSourceTableInfo,
                                     sParseTree->mNamesPrefix )
              != IDE_SUCCESS );

    /* 10. 변경한 Trigger Name과 교환한 Table Name을 Trigger Strings에 반영하고 Trigger를 재생성한다.
     *  - Prefix를 지정한 경우, Source의 TRIGGER_NAME에 Prefix를 붙이고,
     *    Target의 TRIGGER_NAME에서 Prefix를 제거한다. (Meta Table)
     *      - SYS_TRIGGERS_
     *          - TRIGGER_NAME을 변경한다.
     *          - 변경한 Trigger String으로 SUBSTRING_CNT, STRING_LENGTH를 만들어야 한다.
     *          - LAST_DDL_TIME을 갱신한다. (SYSDATE)
     *  - Trigger Strings에 변경한 Trigger Name과 교환한 Table Name을 적용한다. (SM, Meta Table, Meta Cache)
     *      - Trigger Object의 Trigger 생성 구문을 변경한다. (SM)
     *          - Call : smiObject::setObjectInfo()
     *      - New Trigger Cache를 생성하고 SM에 등록한다. (Meta Cache)
     *          - Call : qdnTrigger::allocTriggerCache()
     *      - Trigger Strings을 보관하는 Meta Table을 갱신한다. (Meta Table)
     *          - SYS_TRIGGER_STRINGS_
     *              - DELETE & INSERT로 처리한다.
     *  - Trigger를 동작시키는 Column 정보에는 변경 사항이 없다.
     *      - SYS_TRIGGER_UPDATE_COLUMNS_
     *  - 다른 Trigger가 Cycle Check에 사용하는 정보를 갱신한다. (Meta Table)
     *      - SYS_TRIGGER_DML_TABLES_
     *          - DML_TABLE_ID = Table ID 이면, (TABLE_ID와 무관하게) DML_TABLE_ID를 Peer의 Table ID로 교체한다.
     *  - 참조 Call : qdnTrigger::executeRenameTable()
     */
    IDE_TEST( qdnTrigger::executeSwapTable( aStatement,
                                            sSourceTableInfo,
                                            sTableInfo,
                                            sParseTree->mNamesPrefix,
                                            & sSourceTriggerCache,
                                            & sTriggerCache,
                                            & sNewSourceTriggerCache,
                                            & sNewTriggerCache )
              != IDE_SUCCESS );

    // Table, Partition의 Replication Flag를 교환한다.
    IDE_TEST( swapReplicationFlagOnTableHeader( QC_SMI_STMT( aStatement ),
                                                sTableInfo,
                                                sPartInfoList,
                                                sSourceTableInfo,
                                                sSourcePartInfoList )
              != IDE_SUCCESS );

    /* 11. Source와 Target의 Table Info를 다시 얻는다.
     *  - Table Info를 생성하고, SM에 등록한다. (Meta Cache)
     *  - Table Info를 얻는다.
     */
    IDE_TEST( qcm::makeAndSetQcmTableInfo( QC_SMI_STMT( aStatement ),
                                           sSourceTableInfo->tableID,
                                           sSourceTableInfo->tableOID )
              != IDE_SUCCESS );

    IDE_TEST( qcm::getTableInfoByID( aStatement,
                                     sSourceTableInfo->tableID,
                                     & sNewTableInfo,
                                     & sSCN,
                                     & sTempTableHandle )
              != IDE_SUCCESS );

    IDE_TEST( qcm::touchTable( QC_SMI_STMT( aStatement ),
                               sSourceTableInfo->tableID,
                               SMI_TBSLV_DDL_DML )
              != IDE_SUCCESS );

    IDE_TEST( qcm::makeAndSetQcmTableInfo( QC_SMI_STMT( aStatement ),
                                           sTableInfo->tableID,
                                           sTableInfo->tableOID )
              != IDE_SUCCESS );

    IDE_TEST( qcm::getTableInfoByID( aStatement,
                                     sTableInfo->tableID,
                                     & sNewSourceTableInfo,
                                     & sSCN,
                                     & sTempTableHandle )
              != IDE_SUCCESS );

    IDE_TEST( qcm::touchTable( QC_SMI_STMT( aStatement ),
                               sTableInfo->tableID,
                               SMI_TBSLV_DDL_DML )
              != IDE_SUCCESS );

    /* 12. Comment를 교환한다. (Meta Table)
     *  - SYS_COMMENTS_
     *      - TABLE_NAME을 교환한다.
     *      - Hidden Column이면 Function-based Index의 Column이므로,
     *        사용자가 Prefix를 지정한 경우, Hidden Column Name을 변경한다.
     *          - Source의 Hidden Column Name에 Prefix를 붙이고, Target의 Hidden Column Name에서 Prefix를 제거한다.
     *              - Hidden Column Name = Index Name + $ + IDX + Number
     */
    IDE_TEST( renameCommentsMeta( aStatement,
                                  sTableInfo,
                                  sSourceTableInfo,
                                  sParseTree->mNamesPrefix )
              != IDE_SUCCESS );

    /* 13. 한쪽이라도 Partitioned Table이고 Replication 대상이면, Partition의 Replication 정보를 교환한다. (Meta Table)
     *  - SYS_TABLE_PARTITIONS_
     *      - PARTITION_NAME으로 Matching Partition을 선택한다.
     *      - Matching Partition의 REPLICATION_COUNT, REPLICATION_RECOVERY_COUNT를 교환하고, LAST_DDL_TIME을 갱신한다.
     *          - REPLICATION_COUNT > 0 이면, Partition이 Replication 대상이다.
     *  - Partition의 다른 정보는 변경 사항이 없다.
     *      - SYS_INDEX_PARTITIONS_
     *          - INDEX_PARTITION_NAME은 Partitioned Table의 Index 내에서만 Unique하면 되므로, Prefix가 필요하지 않다.
     *      - SYS_PART_TABLES_, SYS_PART_LOBS_, SYS_PART_KEY_COLUMNS_, SYS_PART_INDICES_
     */
    IDE_TEST( swapTablePartitionsMetaForReplication( aStatement,
                                                     sTableInfo,
                                                     sSourceTableInfo )
              != IDE_SUCCESS );

    /* 14. Partitioned Table이면, Partition Info를 다시 얻는다.
     *  - Partition Info를 생성하고, SM에 등록한다. (Meta Cache)
     *  - Partition Info를 얻는다.
     */
    if ( sNewTableInfo->tablePartitionType == QCM_PARTITIONED_TABLE )
    {
        IDE_TEST( qcmPartition::makeQcmPartitionInfoByTableID( QC_SMI_STMT( aStatement ),
                                                               sNewTableInfo->tableID )
                  != IDE_SUCCESS );

        IDE_TEST( qcmPartition::getPartitionInfoList( aStatement,
                                                      QC_SMI_STMT( aStatement ),
                                                      QC_QMX_MEM( aStatement ),
                                                      sNewTableInfo->tableID,
                                                      & sNewPartInfoList )
                  != IDE_SUCCESS );

        IDE_TEST( qcmPartition::touchPartitionInfoList( QC_SMI_STMT( aStatement ),
                                                        sNewPartInfoList )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing to do */
    }

    if ( sNewSourceTableInfo->tablePartitionType == QCM_PARTITIONED_TABLE )
    {
        IDE_TEST( qcmPartition::makeQcmPartitionInfoByTableID( QC_SMI_STMT( aStatement ),
                                                               sNewSourceTableInfo->tableID )
                  != IDE_SUCCESS );

        IDE_TEST( qcmPartition::getPartitionInfoList( aStatement,
                                                      QC_SMI_STMT( aStatement ),
                                                      QC_QMX_MEM( aStatement ),
                                                      sNewSourceTableInfo->tableID,
                                                      & sNewSourcePartInfoList )
                  != IDE_SUCCESS );

        IDE_TEST( qcmPartition::touchPartitionInfoList( QC_SMI_STMT( aStatement ),
                                                        sNewSourcePartInfoList )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing to do */
    }

    /* 15. Foreign Key Constraint (Parent)가 있으면, Referenced Index를 변경하고 Table Info를 갱신한다.
     *  - Referenced Index를 변경한다. (Meta Table)
     *      - SYS_CONSTRAINTS_
     *          - REFERENCED_INDEX_ID가 가리키는 Index의 Column Name으로 구성된 Index를 Peer에서 찾는다. (Validation과 동일)
     *          - REFERENCED_TABLE_ID와 REFERENCED_INDEX_ID를 Peer의 Table ID와 Index ID로 변경한다.
     *  - Referenced Index를 참조하는 Table의 Table Info를 갱신한다. (Meta Cache)
     *      - Partitioned Table이면, Partition Info를 갱신한다. (Meta Cache)
     */
    IDE_TEST( updateSysConstraintsMetaForReferencedIndex( aStatement,
                                                          sTableInfo,
                                                          sSourceTableInfo,
                                                          sParseTree->mTargetRefChildInfoList,
                                                          sParseTree->mSourceRefChildInfoList )
              != IDE_SUCCESS );

    for ( sPartTableList = sRefChildPartTableList;
          sPartTableList != NULL;
          sPartTableList = sPartTableList->mNext )
    {
        IDU_FIT_POINT( "qdbCopySwap::executeReplaceTable::alloc::sNewPartTableList",
                       idERR_ABORT_InsufficientMemory );

        IDE_TEST( QC_QMX_MEM( aStatement )->alloc( ID_SIZEOF( qdPartitionedTableList ),
                                                   (void **) & sNewPartTableList )
                  != IDE_SUCCESS );
        QD_SET_INIT_PART_TABLE_LIST( sNewPartTableList );

        IDE_TEST( qcm::makeAndSetQcmTableInfo( QC_SMI_STMT( aStatement ),
                                               sPartTableList->mTableInfo->tableID,
                                               sPartTableList->mTableInfo->tableOID )
                  != IDE_SUCCESS );

        IDE_TEST( qcm::getTableInfoByID( aStatement,
                                         sPartTableList->mTableInfo->tableID,
                                         & sNewPartTableList->mTableInfo,
                                         & sNewPartTableList->mTableSCN,
                                         & sNewPartTableList->mTableHandle )
                  != IDE_SUCCESS );

        IDE_TEST( qcm::touchTable( QC_SMI_STMT( aStatement ),
                                   sPartTableList->mTableInfo->tableID,
                                   SMI_TBSLV_DDL_DML )
                  != IDE_SUCCESS );

        if ( sPartTableList->mTableInfo->tablePartitionType == QCM_PARTITIONED_TABLE )
        {
            IDE_TEST( qcmPartition::makeQcmPartitionInfoByTableID( QC_SMI_STMT( aStatement ),
                                                                   sNewPartTableList->mTableInfo->tableID )
                      != IDE_SUCCESS );

            IDE_TEST( qcmPartition::getPartitionInfoList( aStatement,
                                                          QC_SMI_STMT( aStatement ),
                                                          QC_QMX_MEM( aStatement ),
                                                          sNewPartTableList->mTableInfo->tableID,
                                                          & sNewPartTableList->mPartInfoList )
                      != IDE_SUCCESS );

            IDE_TEST( qcmPartition::touchPartitionInfoList( QC_SMI_STMT( aStatement ),
                                                            sNewPartTableList->mPartInfoList )
                      != IDE_SUCCESS );
        }
        else
        {
            /* Nothing to do */
        }

        if ( sNewRefChildPartTableList != NULL )
        {
            sNewPartTableList->mNext = sNewRefChildPartTableList;
        }
        else
        {
            /* Nothing to do */
        }
        sNewRefChildPartTableList = sNewPartTableList;
    }

    /* 16. Package, Procedure, Function에 대해 Set Invalid를 수행한다.
     *  - RELATED_USER_ID = User ID, RELATED_OBJECT_NAME = Table Name, RELATED_OBJECT_TYPE = 2 인 경우에 해당한다.
     *    (SYS_PACKAGE_RELATED_, SYS_PROC_RELATED_)
     *  - Call : qcmProc::relSetInvalidProcOfRelated(), qcmPkg::relSetInvalidPkgOfRelated()
     */
    IDE_TEST( qcmProc::relSetInvalidProcOfRelated( aStatement,
                                                   sTableInfo->tableOwnerID,
                                                   sTableInfo->name,
                                                   idlOS::strlen( (SChar *)sTableInfo->name ),
                                                   QS_TABLE )
              != IDE_SUCCESS );

    IDE_TEST( qcmProc::relSetInvalidProcOfRelated( aStatement,
                                                   sSourceTableInfo->tableOwnerID,
                                                   sSourceTableInfo->name,
                                                   idlOS::strlen( (SChar *)sSourceTableInfo->name ),
                                                   QS_TABLE )
              != IDE_SUCCESS );

    IDE_TEST( qcmPkg::relSetInvalidPkgOfRelated( aStatement,
                                                 sTableInfo->tableOwnerID,
                                                 sTableInfo->name,
                                                 idlOS::strlen( (SChar *)sTableInfo->name ),
                                                 QS_TABLE )
              != IDE_SUCCESS );

    IDE_TEST( qcmPkg::relSetInvalidPkgOfRelated( aStatement,
                                                 sSourceTableInfo->tableOwnerID,
                                                 sSourceTableInfo->name,
                                                 idlOS::strlen( (SChar *)sSourceTableInfo->name ),
                                                 QS_TABLE )
              != IDE_SUCCESS );

    /* 17. View에 대해 Set Invalid & Recompile & Set Valid를 수행한다.
     *  - RELATED_USER_ID = User ID, RELATED_OBJECT_NAME = Table Name, RELATED_OBJECT_TYPE = 2 인 경우에 해당한다.
     *    (SYS_VIEW_RELATED_)
     *  - Call : qcmView::setInvalidViewOfRelated(), qcmView::recompileAndSetValidViewOfRelated()
     */
    IDE_TEST( qcmView::setInvalidViewOfRelated( aStatement,
                                                sTableInfo->tableOwnerID,
                                                sTableInfo->name,
                                                idlOS::strlen( (SChar *)sTableInfo->name ),
                                                QS_TABLE )
              != IDE_SUCCESS );

    IDE_TEST( qcmView::setInvalidViewOfRelated( aStatement,
                                                sSourceTableInfo->tableOwnerID,
                                                sSourceTableInfo->name,
                                                idlOS::strlen( (SChar *)sSourceTableInfo->name ),
                                                QS_TABLE )
              != IDE_SUCCESS );

    IDE_TEST( qcmView::recompileAndSetValidViewOfRelated( aStatement,
                                                          sNewTableInfo->tableOwnerID,
                                                          sNewTableInfo->name,
                                                          idlOS::strlen( (SChar *)sNewTableInfo->name ),
                                                          QS_TABLE )
              != IDE_SUCCESS );

    IDE_TEST( qcmView::recompileAndSetValidViewOfRelated( aStatement,
                                                          sNewSourceTableInfo->tableOwnerID,
                                                          sNewSourceTableInfo->name,
                                                          idlOS::strlen( (SChar *)sNewSourceTableInfo->name ),
                                                          QS_TABLE )
              != IDE_SUCCESS );

    /* 18. Object Privilege를 교환한다. (Meta Table)
     *  - SYS_GRANT_OBJECT_
     *      - OBJ_ID = Table ID, OBJ_TYPE = 'T' 이면, OBJ_ID만 교환한다.
     */
    IDE_TEST( swapGrantObjectMeta( aStatement,
                                   sTableInfo->tableID,
                                   sSourceTableInfo->tableID )
              != IDE_SUCCESS );

    /* 19. Replication 대상 Table인 경우, Replication Meta Table을 수정한다. (Meta Table)
     *  - SYS_REPL_ITEMS_
     *      - SYS_REPL_ITEMS_의 TABLE_OID는 Non-Partitioned Table OID이거나 Partition OID이다.
     *          - Partitioned Table OID는 SYS_REPL_ITEMS_에 없다.
     *      - Non-Partitioned Table인 경우, Table OID를 Peer의 것으로 변경한다.
     *      - Partitioned Table인 경우, Partition OID를 Peer의 것으로 변경한다.
     */
    IDE_TEST( swapReplItemsMeta( aStatement,
                                 sTableInfo,
                                 sSourceTableInfo )
              != IDE_SUCCESS );

    /* 20. Replication 대상 Table이거나 DDL_SUPPLEMENTAL_LOG_ENABLE이 1인 경우, Supplemental Log를 기록한다.
     *  - Non-Partitioned Table이면 Table OID를 얻고, Partitioned Table이면 Partition OID를 얻는다.
     *      - 단, Non-Partitioned Table <-> Partitioned Table 변환인 경우, Partitioned Table에서 Table OID를 얻는다.
     *      - Partitioned Table인 경우, Peer에서 Partition Name이 동일한 Partition을 찾아서 Old OID로 사용한다.
     *          - Peer에 동일한 Partition Name이 없으면, Old OID는 0 이다.
     *  - Table Meta Log Record를 기록한다.
     *      - Call : qci::mManageReplicationCallback.mWriteTableMetaLog()
     *  - 주의 : Replication이 Swap DDL을 Peer에 수행하면 안 된다.
     *      - Copy와 Swap은 짝을 이루는데, Copy를 Peer에 수행하지 않았으므로 Swap을 Peer에 수행하지 않는다.
     */
    if ( ( sDDLSupplementalLog == 1 ) || ( sNewTableInfo->replicationCount > 0 ) )
    {
        if ( ( sTableInfo->tablePartitionType == QCM_PARTITIONED_TABLE ) &&
             ( sNewTableInfo->tablePartitionType == QCM_PARTITIONED_TABLE ) )
        {
            for ( sTempNewPartInfoList = sNewPartInfoList;
                  sTempNewPartInfoList != NULL;
                  sTempNewPartInfoList = sTempNewPartInfoList->next )
            {
                IDE_TEST( qcmPartition::findPartitionByName( sPartInfoList,
                                                             sTempNewPartInfoList->partitionInfo->name,
                                                             idlOS::strlen( sTempNewPartInfoList->partitionInfo->name ),
                                                             & sTempOldPartInfoList )
                          != IDE_SUCCESS );

                if ( sTempOldPartInfoList != NULL )
                {
                    IDE_TEST( qci::mManageReplicationCallback.mWriteTableMetaLog(
                                  aStatement,
                                  sTempOldPartInfoList->partitionInfo->tableOID,
                                  sTempNewPartInfoList->partitionInfo->tableOID )
                              != IDE_SUCCESS );
                }
                else
                {
                    IDE_TEST( qci::mManageReplicationCallback.mWriteTableMetaLog(
                                  aStatement,
                                  0,
                                  sTempNewPartInfoList->partitionInfo->tableOID )
                              != IDE_SUCCESS );
                }
            }
        }
        else
        {
            IDE_TEST( qci::mManageReplicationCallback.mWriteTableMetaLog( aStatement,
                                                                          sTableInfo->tableOID,
                                                                          sNewTableInfo->tableOID )
                      != IDE_SUCCESS );
        }
    }
    else
    {
        /* Nothing to do */
    }

    if ( sNewTableInfo->replicationCount > 0 )
    {
        /* BUG-46252 Partition Merge / Split / Replace DDL asynchronization support */
        if ( qrc::isInternalDDL( aStatement ) != ID_TRUE )
        {
            IDE_TEST( qciMisc::writeDDLStmtTextLog( aStatement,
                                                    sNewTableInfo->tableOwnerID,
                                                    sNewTableInfo->name )
                      != IDE_SUCCESS );
        }
        else
        {
            // Nothing to do
        }
    }
    else
    {
        // Nothing to do
    }

    if ( ( sDDLSupplementalLog == 1 ) || ( sNewSourceTableInfo->replicationCount > 0 ) )
    {
        if ( ( sSourceTableInfo->tablePartitionType == QCM_PARTITIONED_TABLE ) &&
             ( sNewSourceTableInfo->tablePartitionType == QCM_PARTITIONED_TABLE ) )
        {
            for ( sTempNewPartInfoList = sNewSourcePartInfoList;
                  sTempNewPartInfoList != NULL;
                  sTempNewPartInfoList = sTempNewPartInfoList->next )
            {
                IDE_TEST( qcmPartition::findPartitionByName( sSourcePartInfoList,
                                                             sTempNewPartInfoList->partitionInfo->name,
                                                             idlOS::strlen( sTempNewPartInfoList->partitionInfo->name ),
                                                             & sTempOldPartInfoList )
                          != IDE_SUCCESS );

                if ( sTempOldPartInfoList != NULL )
                {
                    IDE_TEST( qci::mManageReplicationCallback.mWriteTableMetaLog(
                                  aStatement,
                                  sTempOldPartInfoList->partitionInfo->tableOID,
                                  sTempNewPartInfoList->partitionInfo->tableOID )
                              != IDE_SUCCESS );
                }
                else
                {
                    IDE_TEST( qci::mManageReplicationCallback.mWriteTableMetaLog(
                                  aStatement,
                                  0,
                                  sTempNewPartInfoList->partitionInfo->tableOID )
                              != IDE_SUCCESS );
                }
            }
        }
        else
        {
            IDE_TEST( qci::mManageReplicationCallback.mWriteTableMetaLog( aStatement,
                                                                          sSourceTableInfo->tableOID,
                                                                          sNewSourceTableInfo->tableOID )
                      != IDE_SUCCESS );
        }
    }
    else
    {
        /* Nothing to do */
    }

    if ( QCG_GET_SESSION_IS_NEED_DDL_INFO( aStatement ) == ID_TRUE )
    {
        IDE_TEST( QC_QMP_MEM( aStatement )->alloc( ID_SIZEOF(smOID) * 2, (void**)&sDDLTableOIDArray )
                  != IDE_SUCCESS);
        sDDLTableOIDArray[0] = sNewTableInfo->tableOID;
        sDDLTableOIDArray[1] = sNewSourceTableInfo->tableOID;

        qrc::setDDLDestInfo( aStatement,
                             2,
                             sDDLTableOIDArray,
                             0,
                             NULL );
    }
    else
    {
        /* Nothing to do */
    }

    /* 21. Old Trigger Cache를 제거한다.
     *  - Call : qdnTrigger::freeTriggerCache()
     *  - 예외 처리를 하기 위해, 마지막에 수행한다.
     */
    qdnTrigger::freeTriggerCacheArray( sTriggerCache,
                                       sTableInfo->triggerCount );

    qdnTrigger::freeTriggerCacheArray( sSourceTriggerCache,
                                       sSourceTableInfo->triggerCount );

    /* 22. Table Name이 변경되었으므로, Encrypted Column을 보안 모듈에 다시 등록한다.
     *  - 보안 모듈에 등록한 기존 Encrypted Column 정보를 제거한다.
     *      - Call : qcsModule::unsetColumnPolicy()
     *  - 보안 모듈에 Table Owner Name, Table Name, Column Name, Policy Name을 등록한다.
     *      - Call : qcsModule::setColumnPolicy()
     *  - 예외 처리를 하지 않기 위해, 마지막에 수행한다.
     */
    qdbCommon::unsetAllColumnPolicy( sTableInfo );
    qdbCommon::unsetAllColumnPolicy( sSourceTableInfo );

    qdbCommon::setAllColumnPolicy( sNewTableInfo );
    qdbCommon::setAllColumnPolicy( sNewSourceTableInfo );

    /* 23. Referenced Index를 참조하는 Table의 Old Table Info를 제거한다.
     *  - Partitioned Table이면, Partition의 Old Partition Info를 제거한다.
     */
    for ( sPartTableList = sRefChildPartTableList;
          sPartTableList != NULL;
          sPartTableList = sPartTableList->mNext )
    {
        (void)qcmPartition::destroyQcmPartitionInfoList( sPartTableList->mPartInfoList );

        (void)qcm::destroyQcmTableInfo( sPartTableList->mTableInfo );
    }

    /* 24. Prefix를 지정한 경우, Non-Partitioned Index가 있으면 Old Index Table Info를 제거한다. */
    if ( QC_IS_NULL_NAME( sParseTree->mNamesPrefix ) != ID_TRUE )
    {
        for ( sTempIndexTableList = sIndexTableList;
              sTempIndexTableList != NULL;
              sTempIndexTableList = sTempIndexTableList->next )
        {
            (void)qcm::destroyQcmTableInfo( sTempIndexTableList->tableInfo );
        }

        for ( sTempIndexTableList = sSourceIndexTableList;
              sTempIndexTableList != NULL;
              sTempIndexTableList = sTempIndexTableList->next )
        {
            (void)qcm::destroyQcmTableInfo( sTempIndexTableList->tableInfo );
        }
    }
    else
    {
        /* Nothing to do */
    }

    /* 25. Partitioned Table이면, Old Partition Info를 제거한다. */
    (void)qcmPartition::destroyQcmPartitionInfoList( sPartInfoList );

    (void)qcmPartition::destroyQcmPartitionInfoList( sSourcePartInfoList );

    /* 26. Old Table Info를 제거한다. */
    (void)qcm::destroyQcmTableInfo( sTableInfo );
    (void)qcm::destroyQcmTableInfo( sSourceTableInfo );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_CANNOT_WRITE_REPL_INFO )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDB_CANNOT_WRITE_REPL_INFO ) );
    }
    IDE_EXCEPTION_END;

    // Trigger Cache는 Table Meta Cache와 별도로 존재한다.
    if ( sSourceTableInfo != NULL )
    {
        qdnTrigger::freeTriggerCacheArray( sNewTriggerCache,
                                           sSourceTableInfo->triggerCount );
    }
    else
    {
        /* Nothing to do */
    }

    if ( sTableInfo != NULL )
    {
        qdnTrigger::freeTriggerCacheArray( sNewSourceTriggerCache,
                                           sTableInfo->triggerCount );
    }
    else
    {
        /* Nothing to do */
    }

    qdnTrigger::restoreTempInfo( sTriggerCache,
                                 sTableInfo );

    qdnTrigger::restoreTempInfo( sSourceTriggerCache,
                                 sSourceTableInfo );

    for ( sPartTableList = sNewRefChildPartTableList;
          sPartTableList != NULL;
          sPartTableList = sPartTableList->mNext )
    {
        (void)qcmPartition::destroyQcmPartitionInfoList( sPartTableList->mPartInfoList );

        (void)qcm::destroyQcmTableInfo( sPartTableList->mTableInfo );
    }

    for ( sTempIndexTableList = sNewIndexTableList;
          sTempIndexTableList != NULL;
          sTempIndexTableList = sTempIndexTableList->next )
    {
        (void)qcm::destroyQcmTableInfo( sTempIndexTableList->tableInfo );
    }

    for ( sTempIndexTableList = sNewSourceIndexTableList;
          sTempIndexTableList != NULL;
          sTempIndexTableList = sTempIndexTableList->next )
    {
        (void)qcm::destroyQcmTableInfo( sTempIndexTableList->tableInfo );
    }

    (void)qcmPartition::destroyQcmPartitionInfoList( sNewPartInfoList );
    (void)qcmPartition::destroyQcmPartitionInfoList( sNewSourcePartInfoList );

    (void)qcm::destroyQcmTableInfo( sNewTableInfo );
    (void)qcm::destroyQcmTableInfo( sNewSourceTableInfo );
    
    // on failure, restore tempinfo.
    qcmPartition::restoreTempInfo( sTableInfo,
                                   sPartInfoList,
                                   sIndexTableList );

    qcmPartition::restoreTempInfo( sSourceTableInfo,
                                   sSourcePartInfoList,
                                   sSourceIndexTableList );

    for ( sPartTableList = sRefChildPartTableList;
          sPartTableList != NULL;
          sPartTableList = sPartTableList->mNext )
    {
        qcmPartition::restoreTempInfo( sPartTableList->mTableInfo,
                                       sPartTableList->mPartInfoList,
                                       NULL );
    }

    return IDE_FAILURE;
}

IDE_RC qdbCopySwap::executeReplacePartition( qcStatement * aStatement )
{
/***********************************************************************
 * Description :
 *      PROJ-2600 Online DDL for Tablespace Alteration
 *
 *      ALTER TABLE [user_name.]target_table_name
 *          REPLACE [user_name.]source_table_name
 *          PARTITION partition_name
 *          [USING PREFIX name_prefix]
 *          [RENAME FORCE]
 *          [IGNORE FOREIGN KEY CHILD];
 *      구문의 Execution
 *
 * Implementation :
 *
 *  1. Target Table에 IX Lock을 잡는다.
 *      - 지정한 Partition에 X Lock을 잡는다.
 *
 *  2. Source Table에 IX Lock을 잡는다.
 *      - 지정한 Partition에 X Lock을 잡는다.
 *
 *  3. Replication 대상 Partition인 경우, 제약 조건을 검사하고 Receiver Thread를 중지한다.
 *      - REPLICATION_DDL_ENABLE 시스템 프라퍼티가 1 이어야 한다.
 *      - REPLICATION 세션 프라퍼티가 NONE이 아니어야 한다.
 *      - Eager Sender/Receiver Thread를 확인하고, Receiver Thread를 중지한다.
 *          - 해당 Partition 관련 Eager Sender/Receiver Thread가 없어야 한다.
 *          - 해당 Partition 관련 Receiver Thread를 중지한다.
 *
 *  4. Source와 Target의 Table Partitions 기본 정보를 교환한다. (Meta Table)
 *      - SYS_TABLE_PARTITIONS_
 *          - TABLE_ID, REPLICATION_COUNT, REPLICATION_RECOVERY_COUNT를 교환한다.
 *          - LAST_DDL_TIME을 갱신한다. (SYSDATE)
 *
 *  5. Source와 Target의 Table Partitions 기본 정보를 교환한다. (Meta Table)
 *      - SYS_PART_LOBS__
 *          - TABLE_ID
 *
 *  6. Source와 Target의 Table Partitions 기본 정보를 교환한다. (Meta Table)
 *      - SYS_INDEX_PARTITIONS__
 *          - TABLE_PARTITION_ID, INDEX_PARTITION_ID를 교환한다.
 *          - LAST_DDL_TIME을 갱신한다. (SYSDATE)
 *
 *  7. Partition의 Replication Flag를 교환한다.
 *
 *  8. Partition Info를 다시 얻는다.
 *      - Partition Info를 생성하고, SM에 등록한다. (Meta Cache)
 *      - Partition Info를 얻는다.
 *
 *  9. Replication 대상 Partition인 경우, Replication Meta Table을 수정한다. (Meta Table)
 *      - SYS_REPL_ITEMS_
 *          - SYS_REPL_ITEMS_의 TABLE_OID는 Non-Partitioned Table OID이거나
 *            Partition OID이다.
 *              - Partitioned Table OID는 SYS_REPL_ITEMS_에 없다.
 *          - Target, Source Partition의 TABLE_OID(Partition OID) 교환
 *
 *  10. Replication 대상 Partition이거나 DDL_SUPPLEMENTAL_LOG_ENABLE이 1인 경우, Supplemental Log를 기록한다.
 *      - Table Meta Log Record를 기록한다.
 *          - Call : qci::mManageReplicationCallback.mWriteTableMetaLog()
 *
 *  11. Old Partition Info를 제거한다.
 *
 ***********************************************************************/

    qdTableParseTree        * sParseTree                        = NULL;
    smOID                   * sDDLTableOIDArray                 = NULL; 
    smOID                   * sDDLPartOIDArray                  = NULL; 
    qcmTableInfo            * sTableInfo                        = NULL;
    qcmTableInfo            * sSourceTableInfo                  = NULL;
    UInt                      sDDLSupplementalLog               = QCU_DDL_SUPPLEMENTAL_LOG;
    qcmTableInfo            * sTargetPartitionInfo              = NULL;
    qcmTableInfo            * sNewTargetPartitionInfo           = NULL;
    qcmTableInfo            * sSourcePartitionInfo              = NULL;
    qcmTableInfo            * sNewSourcePartitionInfo           = NULL;
    void                    * sTempTableHandle                  = NULL;
    qcmColumn               * sColumns                          = NULL;
    UInt                      i                                 = 0;
    smSCN                     sSCN;

    SMI_INIT_SCN( & sSCN );

    sParseTree = (qdTableParseTree *)aStatement->myPlan->parseTree;

    /* 1. Target Table에 IX Lock을 잡는다.
     *  - 지정한 Partition에 X Lock을 잡는다.
     */
    IDE_TEST( qcm::validateAndLockTable( aStatement,
                                         sParseTree->tableHandle,
                                         sParseTree->tableSCN,
                                         SMI_TABLE_LOCK_IX )
              != IDE_SUCCESS );

    sTableInfo = sParseTree->tableInfo;

    IDE_TEST( qcmPartition::getPartitionInfo( aStatement,
                                              sTableInfo->tableID,
                                              sParseTree->mPartAttr->tablePartName,
                                              & sTargetPartitionInfo,
                                              & sSCN,
                                              & sTempTableHandle )
              != IDE_SUCCESS );

    IDE_TEST( qcmPartition::validateAndLockOnePartition( aStatement,
                                                         sTargetPartitionInfo->tableHandle,
                                                         sSCN,
                                                         SMI_TBSLV_DDL_DML, // TBS Validation 옵션
                                                         SMI_TABLE_LOCK_X,
                                                         smiGetDDLLockTimeOut((QC_SMI_STMT(aStatement))->getTrans()))
              != IDE_SUCCESS );

    /* 2. Source Table에 IX Lock을 잡는다.
     *  - 지정한 Partition에 X Lock을 잡는다.
     */
    IDE_TEST( qcm::validateAndLockTable( aStatement,
                                         sParseTree->mSourcePartTable->mTableHandle,
                                         sParseTree->mSourcePartTable->mTableSCN,
                                         SMI_TABLE_LOCK_IX )
              != IDE_SUCCESS );

    sSourceTableInfo = sParseTree->mSourcePartTable->mTableInfo;

    IDE_TEST( qcmPartition::getPartitionInfo( aStatement,
                                              sSourceTableInfo->tableID,
                                              sParseTree->mPartAttr->tablePartName,
                                              & sSourcePartitionInfo,
                                              & sSCN,
                                              & sTempTableHandle )
              != IDE_SUCCESS );

    IDE_TEST( qcmPartition::validateAndLockOnePartition( aStatement,
                                                         sSourcePartitionInfo->tableHandle,
                                                         sSCN,
                                                         SMI_TBSLV_DDL_DML, // TBS Validation 옵션
                                                         SMI_TABLE_LOCK_X,
                                                         smiGetDDLLockTimeOut((QC_SMI_STMT(aStatement))->getTrans()))
              != IDE_SUCCESS );

    /* 3. Replication 대상 Partition인 경우, 제약 조건을 검사하고 Receiver Thread를 중지한다.
     *  - REPLICATION_DDL_ENABLE 시스템 프라퍼티가 1 이어야 한다.
     *  - REPLICATION 세션 프라퍼티가 NONE이 아니어야 한다.
     *  - Eager Sender/Receiver Thread를 확인하고, Receiver Thread를 중지한다.
     *      - 해당 Partition 관련 Eager Sender/Receiver Thread가 없어야 한다.
     *      - 해당 Partition 관련 Receiver Thread를 중지한다.
     */
    if ( sTargetPartitionInfo->replicationCount > 0 )
    {
        /* PROJ-1442 Replication Online 중 DDL 허용
         * Validate와 Execute는 다른 Transaction이므로, 프라퍼티 검사는 Execute에서 한다.
         */
        IDE_TEST( qci::mManageReplicationCallback.mIsDDLEnableOnReplicatedTable( 0, // aRequireLevel
                                                                                 sTableInfo )
                  != IDE_SUCCESS );

        IDE_TEST_RAISE( QC_SMI_STMT( aStatement )->getTrans()->getReplicationMode()
                        == SMI_TRANSACTION_REPL_NONE,
                        ERR_CANNOT_WRITE_REPL_INFO );

        /* PROJ-1442 Replication Online 중 DDL 허용
         * 관련 Receiver Thread 중지
         */
        IDE_TEST( qciMisc::checkRunningEagerReplicationByTableOID( aStatement,
                                                                   & sTargetPartitionInfo->tableOID,
                                                                   1 )
                  != IDE_SUCCESS );

        // BUG-22703 : Begin Statement를 수행한 후에 Hang이 걸리지
        // 않아야 합니다.
        // mStatistics 통계 정보를 전달 합니다.
        IDE_TEST( qci::mManageReplicationCallback.mStopReceiverThreads( QC_SMI_STMT( aStatement ),
                                                                        QC_STATISTICS( aStatement ),
                                                                        & sTargetPartitionInfo->tableOID,
                                                                        1 )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing to do */
    }

    if ( sSourcePartitionInfo->replicationCount > 0 )
    {
        /* PROJ-1442 Replication Online 중 DDL 허용
         * Validate와 Execute는 다른 Transaction이므로, 프라퍼티 검사는 Execute에서 한다.
         */
        IDE_TEST( qci::mManageReplicationCallback.mIsDDLEnableOnReplicatedTable( 0, // aRequireLevel
                                                                                 sSourceTableInfo )
                  != IDE_SUCCESS );

        IDE_TEST_RAISE( QC_SMI_STMT( aStatement )->getTrans()->getReplicationMode()
                        == SMI_TRANSACTION_REPL_NONE,
                        ERR_CANNOT_WRITE_REPL_INFO );

        /* PROJ-1442 Replication Online 중 DDL 허용
         * 관련 Receiver Thread 중지
         */
        IDE_TEST( qciMisc::checkRunningEagerReplicationByTableOID( aStatement,
                                                                   & sSourcePartitionInfo->tableOID,
                                                                   1 )
                  != IDE_SUCCESS );

        // BUG-22703 : Begin Statement를 수행한 후에 Hang이 걸리지
        // 않아야 합니다.
        // mStatistics 통계 정보를 전달 합니다.
        IDE_TEST( qci::mManageReplicationCallback.mStopReceiverThreads( QC_SMI_STMT( aStatement ),
                                                                        QC_STATISTICS( aStatement ),
                                                                        & sSourcePartitionInfo->tableOID,
                                                                        1 )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing to do */
    }

    /* 4. Source와 Target의 Table Partitions 기본 정보를 교환한다. (Meta Table)
     *  - SYS_TABLE_PARTITIONS_
     *      - TABLE_ID, REPLICATION_COUNT, REPLICATION_RECOVERY_COUNT를 교환한다.
     *      - LAST_DDL_TIME을 갱신한다. (SYSDATE)
     *      - PARITION_USABLE를 교환한다. (TASK-7307)
     */
    IDE_TEST( swapTablePartitionsMeta( aStatement,
                                       sTableInfo->tableID,
                                       sSourceTableInfo->tableID,
                                       sParseTree->mPartAttr->tablePartName )
              != IDE_SUCCESS );

    /* 5. Source와 Target의 Table Partitions 기본 정보를 교환한다. (Meta Table)
     *  - SYS_PART_LOBS__
     *      - TABLE_ID
     */
    for ( sColumns = sSourceTableInfo->columns;
          sColumns != NULL;
          sColumns = sColumns->next )
    {
        if ( ( sColumns->basicInfo->type.dataTypeId == MTD_BLOB_ID ) ||
             ( sColumns->basicInfo->type.dataTypeId == MTD_BLOB_LOCATOR_ID ) ||
             ( sColumns->basicInfo->type.dataTypeId == MTD_CLOB_ID ) ||
             ( sColumns->basicInfo->type.dataTypeId == MTD_CLOB_LOCATOR_ID ) )
        {
            break;
        }
        else
        {
            /* Nothing to do */
        }
    }

    // lob column  없으면 수행 하지 않음
    if ( sColumns != NULL )
    {
        IDE_TEST( swapPartLobs( aStatement,
                                sTableInfo->tableID,
                                sSourceTableInfo->tableID,
                                sTargetPartitionInfo->partitionID,
                                sSourcePartitionInfo->partitionID )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing to do */
    }

    /* 6. Source와 Target의 Table Partitions 기본 정보를 교환한다. (Meta Table)
     *  - SYS_INDEX_PARTITIONS__
     *      - TABLE_PARTITION_ID, INDEX_PARTITION_ID를 교환한다.
     *      - LAST_DDL_TIME을 갱신한다. (SYSDATE)
     */
    IDE_TEST( swapIndexPartitions( aStatement,
                                   sTableInfo->tableID,
                                   sSourceTableInfo->tableID,
                                   sTargetPartitionInfo,
                                   sSourcePartitionInfo )
              != IDE_SUCCESS );

    if ( ( sTargetPartitionInfo->indices != NULL ) ||
         ( sSourcePartitionInfo->indices != NULL ) )
    {
        for ( i = 0;
              i < sSourcePartitionInfo->indexCount;
              i++ )
        {
            IDE_TEST( smiTable::swapIndexID( QC_SMI_STMT( aStatement ),
                                             sTargetPartitionInfo->tableHandle,
                                             sTargetPartitionInfo->indices[i].indexHandle,
                                             sSourcePartitionInfo->tableHandle,
                                             sSourcePartitionInfo->indices[i].indexHandle )
                      != IDE_SUCCESS );

            /* BUG-46274 Partition swap error */
            IDE_TEST( smiTable::swapIndexColumns( QC_SMI_STMT( aStatement ),
                                                  sTargetPartitionInfo->indices[i].indexHandle,
                                                  sSourcePartitionInfo->indices[i].indexHandle )
                      != IDE_SUCCESS );
        }
    }
    else
    {
        // nothing to do
    }

    /* 7. Partition의 Replication Flag를 교환한다. */
    IDE_TEST( swapReplicationFlagOnPartitonTableHeader( QC_SMI_STMT( aStatement ),
                                                        sTargetPartitionInfo,
                                                        sSourcePartitionInfo )
              != IDE_SUCCESS );

    /* BUG-46274 Partition swap error */
    IDE_TEST( swapTablePartitionColumnID( aStatement,
                                          sTargetPartitionInfo,
                                          sSourcePartitionInfo )
              != IDE_SUCCESS );

    /* 8. Partition Info를 다시 얻는다.
     *  - Partition Info를 생성하고, SM에 등록한다. (Meta Cache)
     *  - Partition Info를 얻는다.
     */
    IDE_TEST( qcmPartition::makeAndSetQcmPartitionInfo( QC_SMI_STMT( aStatement ),
                                                        sSourcePartitionInfo->partitionID,
                                                        smiGetTableId( sSourcePartitionInfo->tableHandle ),
                                                        sTableInfo,
                                                        NULL )
              != IDE_SUCCESS );

    IDE_TEST( qcmPartition::getPartitionInfo( aStatement,
                                              sTableInfo->tableID,
                                              sParseTree->mPartAttr->tablePartName,
                                              & sNewTargetPartitionInfo,
                                              & sSCN,
                                              & sTempTableHandle )
              != IDE_SUCCESS );

    IDE_TEST( qcmPartition::makeAndSetQcmPartitionInfo( QC_SMI_STMT( aStatement ),
                                                        sTargetPartitionInfo->partitionID,
                                                        smiGetTableId( sTargetPartitionInfo->tableHandle ),
                                                        sSourceTableInfo,
                                                        NULL )
              != IDE_SUCCESS );

    IDE_TEST( qcmPartition::getPartitionInfo( aStatement,
                                              sSourceTableInfo->tableID,
                                              sParseTree->mPartAttr->tablePartName,
                                              & sNewSourcePartitionInfo,
                                              & sSCN,
                                              & sTempTableHandle )
              != IDE_SUCCESS );

    IDE_TEST( qcmPartition::touchPartition( QC_SMI_STMT( aStatement ),
                                            sTargetPartitionInfo->partitionID )
              != IDE_SUCCESS );

    IDE_TEST( qcmPartition::touchPartition( QC_SMI_STMT( aStatement ),
                                            sSourcePartitionInfo->partitionID )
              != IDE_SUCCESS );

    /* 9. Replication 대상 Partition인 경우, Replication Meta Table을 수정한다. (Meta Table)
     *  - SYS_REPL_ITEMS_
     *      - SYS_REPL_ITEMS_의 TABLE_OID는 Non-Partitioned Table OID이거나
     *        Partition OID이다.
     *          - Partitioned Table OID는 SYS_REPL_ITEMS_에 없다.
     *      - Target, Source Partition의 TABLE_OID(Partition OID) 교환
     */
    IDE_TEST( swapReplItemsForParititonMeta( aStatement,
                                             sTargetPartitionInfo,
                                             sSourcePartitionInfo )
              != IDE_SUCCESS );

    /* 10. Replication 대상 Partition이거나 DDL_SUPPLEMENTAL_LOG_ENABLE이 1인 경우, Supplemental Log를 기록한다.
     *  - Table Meta Log Record를 기록한다.
     *      - Call : qci::mManageReplicationCallback.mWriteTableMetaLog()
     */
    if ( ( sDDLSupplementalLog == 1 ) || ( sTargetPartitionInfo->replicationCount > 0 ) )
    {
        IDE_TEST( qci::mManageReplicationCallback.mWriteTableMetaLog(
                      aStatement,
                      sTargetPartitionInfo->tableOID,
                      sSourcePartitionInfo->tableOID )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing to do */
    }

    if ( sTargetPartitionInfo->replicationCount > 0 )
    {
        /* BUG-46252 Partition Merge / Split / Replace DDL asynchronization support */
        if ( qrc::isInternalDDL( aStatement ) != ID_TRUE )
        {
            IDE_TEST( qciMisc::writeDDLStmtTextLog( aStatement,
                                                    sTargetPartitionInfo->tableOwnerID,
                                                    sTargetPartitionInfo->name )
                      != IDE_SUCCESS );
        }
        else
        {
            // Nothing to do
        }
    }
    else
    {
        // Nothing to do
    }

    if ( ( sDDLSupplementalLog == 1 ) || ( sSourcePartitionInfo->replicationCount > 0 ) )
    {
        IDE_TEST( qci::mManageReplicationCallback.mWriteTableMetaLog(
                      aStatement,
                      sSourcePartitionInfo->tableOID,
                      sTargetPartitionInfo->tableOID )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing to do */
    }

    if ( QCG_GET_SESSION_IS_NEED_DDL_INFO( aStatement ) == ID_TRUE )
    {
        IDE_TEST( QC_QMP_MEM( aStatement )->alloc( ID_SIZEOF(smOID) * 2, (void**)&sDDLTableOIDArray )
                  != IDE_SUCCESS);
        sDDLTableOIDArray[0] = sTableInfo->tableOID;
        sDDLTableOIDArray[1] = sSourceTableInfo->tableOID;

        IDE_TEST( QC_QMX_MEM( aStatement )->alloc( ID_SIZEOF(smOID) * 2, (void**)&sDDLPartOIDArray )
                  != IDE_SUCCESS);
        sDDLPartOIDArray[0] = sTargetPartitionInfo->tableOID;
        sDDLPartOIDArray[1] = sSourcePartitionInfo->tableOID;

        qrc::setDDLDestInfo( aStatement,
                             2,
                             sDDLTableOIDArray,
                             1,
                             sDDLPartOIDArray );
    }
    else
    {
        /* Nothing to do */
    }

    /* 11. Old Partition Info를 제거한다. */
    (void)qcmPartition::destroyQcmPartitionInfo( sTargetPartitionInfo );
    (void)qcmPartition::destroyQcmPartitionInfo( sSourcePartitionInfo );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_CANNOT_WRITE_REPL_INFO )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDB_CANNOT_WRITE_REPL_INFO ) );
    }
    IDE_EXCEPTION_END;

    (void)qcmPartition::destroyQcmPartitionInfo( sNewTargetPartitionInfo );
    (void)qcmPartition::destroyQcmPartitionInfo( sNewSourcePartitionInfo );

    qcmPartition::restoreTempInfoForPartition( sTableInfo,
                                               sTargetPartitionInfo );

    qcmPartition::restoreTempInfoForPartition( sSourceTableInfo,
                                               sSourcePartitionInfo );

    return IDE_FAILURE;
}

IDE_RC qdbCopySwap::getRefChildInfoList( qcStatement      * aStatement,
                                         qcmTableInfo     * aTableInfo,
                                         qcmRefChildInfo ** aRefChildInfoList )
{
/***********************************************************************
 * Description :
 *      Referenced Index를 참조하는 Ref Child Info List를 만든다.
 *          - 각 Ref Child Info는 Table/Partition Ref를 가지고 있고 IS Lock을 획득했다.
 *          - Ref Child Info List에서 Self Foreign Key를 제거한다.
 *
 * Implementation :
 *
 ***********************************************************************/

    qcmIndex                * sIndexInfo            = NULL;
    qcmRefChildInfo         * sRefChildInfoList     = NULL;
    qcmRefChildInfo         * sRefChildInfo         = NULL;
    qcmRefChildInfo         * sNextRefChildInfo     = NULL;
    UInt                      i                     = 0;

    for ( i = 0; i < aTableInfo->uniqueKeyCount; i++ )
    {
        sIndexInfo = aTableInfo->uniqueKeys[i].constraintIndex;

        IDE_TEST( qcm::getChildKeys( aStatement,
                                     sIndexInfo,
                                     aTableInfo,
                                     & sRefChildInfo )
                  != IDE_SUCCESS );

        while ( sRefChildInfo != NULL )
        {
            sNextRefChildInfo = sRefChildInfo->next;

            if ( sRefChildInfo->isSelfRef == ID_TRUE )
            {
                /* Nothing to do */
            }
            else
            {
                if ( sRefChildInfoList == NULL )
                {
                    sRefChildInfo->next = NULL;
                }
                else
                {
                    sRefChildInfo->next = sRefChildInfoList;
                }
                sRefChildInfoList = sRefChildInfo;
            }

            sRefChildInfo = sNextRefChildInfo;
        }
    }

    *aRefChildInfoList = sRefChildInfoList;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdbCopySwap::getPartitionedTableListFromRefChildInfoList( qcStatement             * aStatement,
                                                                 qcmRefChildInfo         * aRefChildInfoList,
                                                                 qdPartitionedTableList ** aRefChildPartTableList )
{
/***********************************************************************
 * Description :
 *      Referenced Index를 참조하는 Ref Child Table/Partition List를 만든다.
 *          - Ref Child Table/Partition List에서 Table 중복을 제거한다.
 *
 * Implementation :
 *
 ***********************************************************************/

    qcmRefChildInfo         * sRefChildInfo         = NULL;
    qdPartitionedTableList  * sPartitionedTableList = *aRefChildPartTableList;
    qdPartitionedTableList  * sPartitionedTable     = NULL;
    qcmPartitionInfoList    * sPartitionInfo        = NULL;
    qmsPartitionRef         * sPartitionRef         = NULL;

    for ( sRefChildInfo = aRefChildInfoList;
          sRefChildInfo != NULL;
          sRefChildInfo = sRefChildInfo->next )
    {
        for ( sPartitionedTable = sPartitionedTableList;
              sPartitionedTable != NULL;
              sPartitionedTable = sPartitionedTable->mNext )
        {
            if ( sRefChildInfo->childTableRef->tableInfo->tableID ==
                 sPartitionedTable->mTableInfo->tableID )
            {
                break;
            }
            else
            {
                /* Nothing to do */
            }
        }

        if ( sPartitionedTable != NULL )
        {
            continue;
        }
        else
        {
            /* Nothing to do */
        }

        IDU_FIT_POINT( "qdbCopySwap::getPartitionedTableListFromRefChildInfoList::alloc::sPartitionedTable",
                       idERR_ABORT_InsufficientMemory );

        IDE_TEST( QC_QMP_MEM( aStatement )->alloc( ID_SIZEOF( qdPartitionedTableList ),
                                                   (void **) & sPartitionedTable )
                  != IDE_SUCCESS );
        QD_SET_INIT_PART_TABLE_LIST( sPartitionedTable );

        sPartitionedTable->mTableInfo   = sRefChildInfo->childTableRef->tableInfo;
        sPartitionedTable->mTableHandle = sRefChildInfo->childTableRef->tableHandle;
        sPartitionedTable->mTableSCN    = sRefChildInfo->childTableRef->tableSCN;

        for ( sPartitionRef = sRefChildInfo->childTableRef->partitionRef;
              sPartitionRef != NULL;
              sPartitionRef = sPartitionRef->next )
        {
            IDU_FIT_POINT( "qdbCopySwap::getPartitionedTableListFromRefChildInfoList::alloc::sPartitionInfo",
                           idERR_ABORT_InsufficientMemory );

            IDE_TEST( QC_QMP_MEM( aStatement )->alloc( ID_SIZEOF( qcmPartitionInfoList ),
                                                       (void **) & sPartitionInfo )
                      != IDE_SUCCESS );

            sPartitionInfo->partitionInfo = sPartitionRef->partitionInfo;
            sPartitionInfo->partHandle    = sPartitionRef->partitionHandle;
            sPartitionInfo->partSCN       = sPartitionRef->partitionSCN;

            if ( sPartitionedTable->mPartInfoList == NULL )
            {
                sPartitionInfo->next = NULL;
            }
            else
            {
                sPartitionInfo->next = sPartitionedTable->mPartInfoList;
            }

            sPartitionedTable->mPartInfoList = sPartitionInfo;
        }

        if ( sPartitionedTableList == NULL )
        {
            sPartitionedTable->mNext = NULL;
        }
        else
        {
            sPartitionedTable->mNext = sPartitionedTableList;
        }
        sPartitionedTableList = sPartitionedTable;
    }

    *aRefChildPartTableList = sPartitionedTableList;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

void qdbCopySwap::findPeerIndex( qcmTableInfo  * aMyTable,
                                 qcmIndex      * aMyIndex,
                                 qcmTableInfo  * aPeerTable,
                                 qcmIndex     ** aPeerIndex )
{
/***********************************************************************
 * Description :
 *      REFERENCED_INDEX_ID가 가리키는 Index의 Column Name으로 구성된 Index를 Peer에서 찾는다.
 *          - Primary이거나 Unique이어야 한다.
 *              - Primary/Unique Key Constraint가 Index로 구현되어 있으므로, Index에서 찾는다.
 *              - Local Unique는 Foreign Key Constraint 대상이 아니다.
 *          - Column Count가 같아야 한다.
 *          - Column Name 순서가 같아야 한다.
 *              - Foreign Key 생성 시에는 순서가 달라도 지원하나, 여기에서는 지원하지 않는다.
 *          - Data Type, Language가 같아야 한다.
 *              - Precision, Scale은 달라도 된다.
 *          - 참조 Call : qdnForeignKey::validateForeignKeySpec()
 *
 * Implementation :
 *
 ***********************************************************************/

    qcmIndex    * sPeerIndex  = NULL;
    qcmIndex    * sFoundIndex = NULL;
    qcmColumn   * sMyColumn   = NULL;
    qcmColumn   * sPeerColumn = NULL;
    UInt          sConstrType = QD_CONSTR_MAX;
    UInt          i           = 0;
    UInt          j           = 0;

    for ( i = 0; i < aPeerTable->uniqueKeyCount; i++ )
    {
        sConstrType = aPeerTable->uniqueKeys[i].constraintType;

        if ( ( sConstrType != QD_PRIMARYKEY ) && ( sConstrType != QD_UNIQUE ) )
        {
            continue;
        }
        else
        {
            /* Nothing to do */
        }

        sPeerIndex = aPeerTable->uniqueKeys[i].constraintIndex;

        if ( aMyIndex->keyColCount != sPeerIndex->keyColCount )
        {
            continue;
        }
        else
        {
            /* Nothing to do */
        }

        for ( j = 0; j < aMyIndex->keyColCount; j++ )
        {
            sMyColumn   = & aMyTable->columns[aMyIndex->keyColumns[j].column.id & SMI_COLUMN_ID_MASK];
            sPeerColumn = & aPeerTable->columns[sPeerIndex->keyColumns[j].column.id & SMI_COLUMN_ID_MASK];

            if ( idlOS::strMatch( sMyColumn->name,
                                  idlOS::strlen( sMyColumn->name ),
                                  sPeerColumn->name,
                                  idlOS::strlen( sPeerColumn->name ) ) == 0 )
            {
                /* Nothing to do */
            }
            else
            {
                break;
            }

            if ( ( sMyColumn->basicInfo->type.dataTypeId == sPeerColumn->basicInfo->type.dataTypeId ) &&
                 ( sMyColumn->basicInfo->type.languageId == sPeerColumn->basicInfo->type.languageId ) )
            {
                /* Nothing to do */
            }
            else
            {
                break;
            }
        }
        if ( j == aMyIndex->keyColCount )
        {
            sFoundIndex = sPeerIndex;
            break;
        }
        else
        {
            /* Nothing to do */
        }
    }

    *aPeerIndex = sFoundIndex;

    return;
}

IDE_RC qdbCopySwap::compareReplicationInfo( qcStatement          * aStatement,
                                            qcmTableInfo         * aMyTableInfo,
                                            qcmPartitionInfoList * aMyPartInfoList,
                                            qcmTableInfo         * aPeerTableInfo,
                                            qcmPartitionInfoList * aPeerPartInfoList )
{
/***********************************************************************
 * Description :
 *      Replication 제약 사항을 확인한다.
 *      - 한쪽이라도 Replication 대상이면, Source와 Target 둘 다 Partitioned Table이거나 아니어야 한다.
 *          - SYS_TABLES_
 *              - REPLICATION_COUNT > 0 이면, Table이 Replication 대상이다.
 *              - IS_PARTITIONED : 'N'(Non-Partitioned Table), 'Y'(Partitioned Table)
 *      - Partitioned Table인 경우, 기본 정보가 같아야 한다.
 *          - SYS_PART_TABLES_
 *              - PARTITION_METHOD가 같아야 한다.
 *      - Partition이 Replication 대상인 경우, 양쪽에 같은 Partition이 존재해야 한다.
 *          - SYS_TABLE_PARTITIONS_
 *              - REPLICATION_COUNT > 0 이면, Partition이 Replication 대상이다.
 *              - PARTITION_NAME이 같으면, 기본 정보를 비교한다.
 *                  - PARTITION_MIN_VALUE, PARTITION_MAX_VALUE, PARTITION_ORDER가 같아야 한다.
 *
 * Implementation :
 *
 ***********************************************************************/

    SChar                   * sPartCondMinValues1   = NULL;
    SChar                   * sPartCondMaxValues1   = NULL;
    SChar                   * sPartCondMinValues2   = NULL;
    SChar                   * sPartCondMaxValues2   = NULL;
    qcmPartitionInfoList    * sPartInfo1            = NULL;
    qcmPartitionInfoList    * sPartInfo2            = NULL;
    UInt                      sPartOrder1           = 0;
    UInt                      sPartOrder2           = 0;
    SInt                      sResult               = 0;

    if ( aMyTableInfo->replicationCount > 0 )
    {
        IDE_TEST_RAISE( aMyTableInfo->tablePartitionType != aPeerTableInfo->tablePartitionType,
                        ERR_TABLE_PARTITION_TYPE_MISMATCH );

        if ( aMyTableInfo->tablePartitionType == QCM_PARTITIONED_TABLE )
        {
            IDE_TEST_RAISE( aMyTableInfo->partitionMethod != aPeerTableInfo->partitionMethod,
                            ERR_PARTITION_METHOD_MISMATCH );

            IDU_FIT_POINT( "qdbCopySwap::compareReplicationInfo::STRUCT_ALLOC_WITH_SIZE::sPartCondMinValues1",
                           idERR_ABORT_InsufficientMemory );

            IDE_TEST( STRUCT_ALLOC_WITH_SIZE( QC_QMP_MEM( aStatement ),
                                              SChar,
                                              QC_MAX_PARTKEY_COND_VALUE_LEN + 1,
                                              & sPartCondMinValues1 )
                      != IDE_SUCCESS );

            IDU_FIT_POINT( "qdbCopySwap::compareReplicationInfo::STRUCT_ALLOC_WITH_SIZE::sPartCondMaxValues1",
                           idERR_ABORT_InsufficientMemory );

            IDE_TEST( STRUCT_ALLOC_WITH_SIZE( QC_QMP_MEM( aStatement ),
                                              SChar,
                                              QC_MAX_PARTKEY_COND_VALUE_LEN + 1,
                                              & sPartCondMaxValues1 )
                      != IDE_SUCCESS );

            IDU_FIT_POINT( "qdbCopySwap::compareReplicationInfo::STRUCT_ALLOC_WITH_SIZE::sPartCondMinValues2",
                           idERR_ABORT_InsufficientMemory );

            IDE_TEST( STRUCT_ALLOC_WITH_SIZE( QC_QMP_MEM( aStatement ),
                                              SChar,
                                              QC_MAX_PARTKEY_COND_VALUE_LEN + 1,
                                              & sPartCondMinValues2 )
                      != IDE_SUCCESS );

            IDU_FIT_POINT( "qdbCopySwap::compareReplicationInfo::STRUCT_ALLOC_WITH_SIZE::sPartCondMaxValues2",
                           idERR_ABORT_InsufficientMemory );

            IDE_TEST( STRUCT_ALLOC_WITH_SIZE( QC_QMP_MEM( aStatement ),
                                              SChar,
                                              QC_MAX_PARTKEY_COND_VALUE_LEN + 1,
                                              & sPartCondMaxValues2 )
                      != IDE_SUCCESS );

            for ( sPartInfo1 = aMyPartInfoList;
                  sPartInfo1 != NULL;
                  sPartInfo1 = sPartInfo1->next )
            {
                if ( sPartInfo1->partitionInfo->replicationCount > 0 )
                {
                    IDE_TEST( qcmPartition::findPartitionByName( aPeerPartInfoList,
                                                                 sPartInfo1->partitionInfo->name,
                                                                 idlOS::strlen( sPartInfo1->partitionInfo->name ),
                                                                 & sPartInfo2 )
                              != IDE_SUCCESS );

                    IDE_TEST_RAISE( sPartInfo2 == NULL, ERR_PARTITION_NAME_MISMATCH )

                    if ( aMyTableInfo->partitionMethod != QCM_PARTITION_METHOD_HASH )
                    {
                        IDE_TEST( qcmPartition::getPartMinMaxValue(
                                        QC_SMI_STMT( aStatement ),
                                        sPartInfo1->partitionInfo->partitionID,
                                        sPartCondMinValues1,
                                        sPartCondMaxValues1 )
                                  != IDE_SUCCESS );

                        IDE_TEST( qcmPartition::getPartMinMaxValue(
                                        QC_SMI_STMT( aStatement ),
                                        sPartInfo2->partitionInfo->partitionID,
                                        sPartCondMinValues2,
                                        sPartCondMaxValues2 )
                                  != IDE_SUCCESS );

                        IDE_TEST( qciMisc::comparePartCondValues( QC_STATISTICS( aStatement ),
                                                                  aMyTableInfo,
                                                                  sPartCondMinValues1,
                                                                  sPartCondMinValues2,
                                                                  & sResult )
                                  != IDE_SUCCESS );

                        IDE_TEST_RAISE( sResult != 0, ERR_PARTITION_CONDITION_MISMATCH );

                        IDE_TEST( qciMisc::comparePartCondValues( QC_STATISTICS( aStatement ),
                                                                  aMyTableInfo,
                                                                  sPartCondMaxValues1,
                                                                  sPartCondMaxValues2,
                                                                  & sResult )
                                  != IDE_SUCCESS );

                        IDE_TEST_RAISE( sResult != 0, ERR_PARTITION_CONDITION_MISMATCH );
                    }
                    else
                    {
                        IDE_TEST( qcmPartition::getPartitionOrder(
                                        QC_SMI_STMT( aStatement ),
                                        sPartInfo1->partitionInfo->tableID,
                                        (UChar *)sPartInfo1->partitionInfo->name,
                                        idlOS::strlen( sPartInfo1->partitionInfo->name ),
                                        & sPartOrder1 )
                                  != IDE_SUCCESS );

                        IDE_TEST( qcmPartition::getPartitionOrder(
                                        QC_SMI_STMT( aStatement ),
                                        sPartInfo2->partitionInfo->tableID,
                                        (UChar *)sPartInfo2->partitionInfo->name,
                                        idlOS::strlen( sPartInfo2->partitionInfo->name ),
                                        & sPartOrder2 )
                                  != IDE_SUCCESS );

                        IDE_TEST_RAISE( sPartOrder1 != sPartOrder2, ERR_PARTITION_ORDER_MISMATCH );
                    }
                }
                else
                {
                    /* Nothing to do */
                }
            } // for (aMyPartInfoList)
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

    IDE_EXCEPTION( ERR_TABLE_PARTITION_TYPE_MISMATCH )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDB_REPL_TABLE_PARTITION_TYPE_MISMATCH ) );
    }
    IDE_EXCEPTION( ERR_PARTITION_NAME_MISMATCH )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDB_REPL_PARTITION_NAME_MISMATCH,
                                  sPartInfo1->partitionInfo->name ) );
    }
    IDE_EXCEPTION( ERR_PARTITION_METHOD_MISMATCH )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDB_REPL_PARTITION_METHOD_MISMATCH ) );
    }
    IDE_EXCEPTION( ERR_PARTITION_CONDITION_MISMATCH )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDB_REPL_PARTITION_CONDITION_MISMATCH,
                                  sPartInfo1->partitionInfo->name,
                                  sPartInfo2->partitionInfo->name ) );
    }
    IDE_EXCEPTION( ERR_PARTITION_ORDER_MISMATCH )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDB_REPL_PARTITION_ORDER_MISMATCH,
                                  sPartInfo1->partitionInfo->name,
                                  sPartInfo2->partitionInfo->name ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdbCopySwap::compareReplicationInfoPartition( qcStatement          * aStatement,
                                                     qdTableParseTree     * aParseTree, 
                                                     qcmTableInfo         * aMyTableInfo,
                                                     qcmTableInfo         * aPeerTableInfo )
{
/***********************************************************************
 * Description :
 *      Replication 제약 사항을 확인한다.
 *      - 한쪽이라도 Replication 대상이면, Source와 Target 둘 다 Partitioned Table이거나 아니어야 한다.
 *          - SYS_TABLES_
 *              - REPLICATION_COUNT > 0 이면, Table이 Replication 대상이다.
 *              - IS_PARTITIONED : 'N'(Non-Partitioned Table), 'Y'(Partitioned Table)
 *      - Partitioned Table인 경우, 기본 정보가 같아야 한다.
 *          - SYS_PART_TABLES_
 *              - PARTITION_METHOD가 같아야 한다.
 *      - Partition이 Replication 대상인 경우, 양쪽에 같은 Partition이 존재해야 한다.
 *          - SYS_TABLE_PARTITIONS_
 *              - REPLICATION_COUNT > 0 이면, Partition이 Replication 대상이다.
 *              - PARTITION_NAME이 같으면, 기본 정보를 비교한다.
 *                  - PARTITION_MIN_VALUE, PARTITION_MAX_VALUE, PARTITION_ORDER가 같아야 한다.
 *      변경: BUG-47208 range, range hash, list partition replace 대상 partition에 위 제약 체크
 *
 * Implementation :
 *
 ***********************************************************************/

    SChar                   * sPartCondMinValues1   = NULL;
    SChar                   * sPartCondMaxValues1   = NULL;
    SChar                   * sPartCondMinValues2   = NULL;
    SChar                   * sPartCondMaxValues2   = NULL;
    SInt                      sResult               = 0;
    qcmTableInfo            * sSrcTempPartInfo      = NULL;
    qcmTableInfo            * sDstTempPartInfo      = NULL;
    void                    * sHandle               = NULL;    
    smSCN                     sSCN                  = SM_SCN_INIT;
    
    if (( aMyTableInfo->replicationCount > 0 ) ||
        ( aPeerTableInfo->replicationCount > 0 ))
    {
        IDE_TEST_RAISE( aMyTableInfo->tablePartitionType != aPeerTableInfo->tablePartitionType,
                        ERR_TABLE_PARTITION_TYPE_MISMATCH );

        if ( aMyTableInfo->tablePartitionType == QCM_PARTITIONED_TABLE )
        {
            IDE_TEST_RAISE( aMyTableInfo->partitionMethod != aPeerTableInfo->partitionMethod,
                            ERR_PARTITION_METHOD_MISMATCH );

            IDU_FIT_POINT( "qdbCopySwap::compareReplicationInfoPartition::STRUCT_ALLOC_WITH_SIZE::sPartCondMinValues1",
                           idERR_ABORT_InsufficientMemory );

            IDE_TEST( STRUCT_ALLOC_WITH_SIZE( QC_QMP_MEM( aStatement ),
                                              SChar,
                                              QC_MAX_PARTKEY_COND_VALUE_LEN + 1,
                                              & sPartCondMinValues1 )
                      != IDE_SUCCESS );

            IDU_FIT_POINT( "qdbCopySwap::compareReplicationInfoPartition::STRUCT_ALLOC_WITH_SIZE::sPartCondMaxValues1",
                           idERR_ABORT_InsufficientMemory );

            IDE_TEST( STRUCT_ALLOC_WITH_SIZE( QC_QMP_MEM( aStatement ),
                                              SChar,
                                              QC_MAX_PARTKEY_COND_VALUE_LEN + 1,
                                              & sPartCondMaxValues1 )
                      != IDE_SUCCESS );

            IDU_FIT_POINT( "qdbCopySwap::compareReplicationInfoPartition::STRUCT_ALLOC_WITH_SIZE::sPartCondMinValues2",
                           idERR_ABORT_InsufficientMemory );

            IDE_TEST( STRUCT_ALLOC_WITH_SIZE( QC_QMP_MEM( aStatement ),
                                              SChar,
                                              QC_MAX_PARTKEY_COND_VALUE_LEN + 1,
                                              & sPartCondMinValues2 )
                      != IDE_SUCCESS );

            IDU_FIT_POINT( "qdbCopySwap::compareReplicationInfoPartition::STRUCT_ALLOC_WITH_SIZE::sPartCondMaxValues2",
                           idERR_ABORT_InsufficientMemory );

            IDE_TEST( STRUCT_ALLOC_WITH_SIZE( QC_QMP_MEM( aStatement ),
                                              SChar,
                                              QC_MAX_PARTKEY_COND_VALUE_LEN + 1,
                                              & sPartCondMaxValues2 )
                      != IDE_SUCCESS );
            
            // partition info
            IDE_TEST( qcmPartition::getPartitionInfo( aStatement,
                                                      aMyTableInfo->tableID,
                                                      aParseTree->mPartAttr->tablePartName,
                                                      & sDstTempPartInfo,
                                                      & sSCN,
                                                      & sHandle )
                      != IDE_SUCCESS);

            IDE_TEST( qcmPartition::getPartitionInfo( aStatement,
                                                      aPeerTableInfo->tableID,
                                                      aParseTree->mPartAttr->tablePartName,
                                                      & sSrcTempPartInfo,
                                                      & sSCN,
                                                      & sHandle )
                      != IDE_SUCCESS );
        
            if (( sDstTempPartInfo->replicationCount > 0 ) ||
                ( sSrcTempPartInfo->replicationCount > 0 ))
            {                          
                if ( aMyTableInfo->partitionMethod != QCM_PARTITION_METHOD_HASH )
                {
                    IDE_TEST( qcmPartition::getPartMinMaxValue(
                                  QC_SMI_STMT( aStatement ),
                                  sDstTempPartInfo->partitionID,
                                  sPartCondMinValues1,
                                  sPartCondMaxValues1 )
                              != IDE_SUCCESS );

                    IDE_TEST( qcmPartition::getPartMinMaxValue(
                                  QC_SMI_STMT( aStatement ),
                                  sSrcTempPartInfo->partitionID,
                                  sPartCondMinValues2,
                                  sPartCondMaxValues2 )
                              != IDE_SUCCESS );

                    IDE_TEST( qciMisc::comparePartCondValues( QC_STATISTICS( aStatement ),
                                                              aMyTableInfo,
                                                              sPartCondMinValues1,
                                                              sPartCondMinValues2,
                                                              & sResult )
                              != IDE_SUCCESS );

                    IDE_TEST_RAISE( sResult != 0, ERR_PARTITION_CONDITION_MISMATCH );

                    IDE_TEST( qciMisc::comparePartCondValues( QC_STATISTICS( aStatement ),
                                                              aMyTableInfo,
                                                              sPartCondMaxValues1,
                                                              sPartCondMaxValues2,
                                                              & sResult )
                              != IDE_SUCCESS );

                    IDE_TEST_RAISE( sResult != 0, ERR_PARTITION_CONDITION_MISMATCH );
                }
                else
                {
                    // hash partition은 해당 함수를 타지 않는다.
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
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_TABLE_PARTITION_TYPE_MISMATCH )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDB_REPL_TABLE_PARTITION_TYPE_MISMATCH ) );
    }
    IDE_EXCEPTION( ERR_PARTITION_METHOD_MISMATCH )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDB_REPL_PARTITION_METHOD_MISMATCH ) );
    }
    IDE_EXCEPTION( ERR_PARTITION_CONDITION_MISMATCH )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDB_REPL_PARTITION_CONDITION_MISMATCH,
                                  sDstTempPartInfo->name,
                                  sSrcTempPartInfo->name ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdbCopySwap::executeCreateTablePartition( qcStatement           * aStatement,
                                                 qcmTableInfo          * aMyTableInfo,
                                                 qcmTableInfo          * aPeerTableInfo,
                                                 qcmPartitionInfoList  * aPeerPartInfoList,
                                                 qcmPartitionInfoList ** aMyPartInfoList )
{
/***********************************************************************
 * Description :
 *      Partition을 생성한다.
 *          - Next Table Partition ID를 얻는다.
 *          - Partition을 생성한다. (SM)
 *          - Meta Table에 Target Partition 정보를 추가한다. (Meta Table)
 *              - SYS_TABLE_PARTITIONS_
 *                  - SM에서 얻은 Partition OID가 SYS_TABLE_PARTITIONS_에 필요하다.
 *                  - REPLICATION_COUNT, REPLICATION_RECOVERY_COUNT는 초기화한다.
 *              - SYS_PART_LOBS_
 *      Partition Info를 생성하고, SM에 등록한다. (Meta Cache)
 *      Partition Info를 얻는다.
 *
 * Implementation :
 *
 ***********************************************************************/

    SChar                   * sPartCondMinValues    = NULL;
    SChar                   * sPartCondMaxValues    = NULL;
    qcmPartitionInfoList    * sPeerPartInfo         = NULL;
    qcmPartitionInfoList    * sMyPartInfoList       = NULL;
    qcmPartitionInfoList    * sPartInfoList         = NULL;
    qcmTableInfo            * sPartitionInfo        = NULL;
    qcmColumn               * sPartitionColumns     = NULL;
    void                    * sPartitionHandle      = NULL;
    UInt                      sPartitionID          = 0;
    smOID                     sPartitionOID         = SMI_NULL_OID;
    UInt                      sPartitionOrder       = 0;
    smSCN                     sSCN;
    qcNamePosition            sPartitionNamePos;

    smiSegAttr                sSegAttr;
    smiSegStorageAttr         sSegStoAttr;
    UInt                      sPartType             = 0;
    UInt                      sTableFlag            = 0;

    SMI_INIT_SCN( & sSCN );

    IDU_FIT_POINT( "qdbCopySwap::executeCreateTablePartition::STRUCT_ALLOC_WITH_SIZE::sPartCondMinValues",
                   idERR_ABORT_InsufficientMemory );

    IDE_TEST( STRUCT_ALLOC_WITH_SIZE( QC_QMX_MEM( aStatement ),
                                      SChar,
                                      QC_MAX_PARTKEY_COND_VALUE_LEN + 1,
                                      & sPartCondMinValues )
              != IDE_SUCCESS );

    IDU_FIT_POINT( "qdbCopySwap::executeCreateTablePartition::STRUCT_ALLOC_WITH_SIZE::sPartCondMaxValues",
                   idERR_ABORT_InsufficientMemory );

    IDE_TEST( STRUCT_ALLOC_WITH_SIZE( QC_QMX_MEM( aStatement ),
                                      SChar,
                                      QC_MAX_PARTKEY_COND_VALUE_LEN + 1,
                                      & sPartCondMaxValues )
              != IDE_SUCCESS );

    for ( sPeerPartInfo = aPeerPartInfoList;
          sPeerPartInfo != NULL;
          sPeerPartInfo = sPeerPartInfo->next )
    {
        IDE_TEST( qcmPartition::getNextTablePartitionID( aStatement,
                                                         & sPartitionID )
                  != IDE_SUCCESS );

        // 아래에서 Partition을 생성할 때, qdbCommon::createTableOnSM()에서 Column 정보로 qcmColumn을 요구한다.
        //  - qdbCommon::createTableOnSM()
        //      - 실제로 필요한 정보는 mtcColumn이다.
        //      - Column ID를 새로 생성하고, Column 통계 정보를 초기화한다. (매개변수인 qcmColumn을 수정한다.)
        IDE_TEST( qcm::copyQcmColumns( QC_QMX_MEM( aStatement ),
                                       sPeerPartInfo->partitionInfo->columns,
                                       & sPartitionColumns,
                                       sPeerPartInfo->partitionInfo->columnCount )
                  != IDE_SUCCESS );

        sPartType = qdbCommon::getTableTypeFromTBSID( sPeerPartInfo->partitionInfo->TBSID );

        /* PROJ-2464 hybrid partitioned table 지원
         *  Partition Meta Cache의 segAttr와 segStoAttr는 0으로 초기화되어 있다.
         */
        qdbCommon::adjustPhysicalAttr( sPartType,
                                       aPeerTableInfo->segAttr,
                                       aPeerTableInfo->segStoAttr,
                                       & sSegAttr,
                                       & sSegStoAttr,
                                       ID_TRUE /* aIsTable */ );

        // Partition에서 Replication 정보를 제거한다.
        sTableFlag  = sPeerPartInfo->partitionInfo->tableFlag;
        sTableFlag &= ( ~SMI_TABLE_REPLICATION_MASK );
        sTableFlag |= SMI_TABLE_REPLICATION_DISABLE;
        sTableFlag &= ( ~SMI_TABLE_REPLICATION_TRANS_WAIT_MASK );
        sTableFlag |= SMI_TABLE_REPLICATION_TRANS_WAIT_DISABLE;

        IDE_TEST( qdbCommon::createTableOnSM( aStatement,
                                              sPartitionColumns,
                                              aMyTableInfo->tableOwnerID,
                                              aMyTableInfo->tableID,
                                              sPeerPartInfo->partitionInfo->maxrows,
                                              sPeerPartInfo->partitionInfo->TBSID,
                                              sSegAttr,
                                              sSegStoAttr,
                                              QDB_TABLE_ATTR_MASK_ALL,
                                              sTableFlag,
                                              sPeerPartInfo->partitionInfo->parallelDegree,
                                              & sPartitionOID )
                  != IDE_SUCCESS );

        if ( aMyTableInfo->partitionMethod != QCM_PARTITION_METHOD_HASH )
        {
            IDE_TEST( qcmPartition::getPartMinMaxValue(
                            QC_SMI_STMT( aStatement ),
                            sPeerPartInfo->partitionInfo->partitionID,
                            sPartCondMinValues,
                            sPartCondMaxValues )
                      != IDE_SUCCESS );

            sPartitionOrder = QDB_NO_PARTITION_ORDER;
        }
        else
        {
            sPartCondMinValues[0] = '\0';
            sPartCondMaxValues[0] = '\0';

            IDE_TEST( qcmPartition::getPartitionOrder(
                            QC_SMI_STMT( aStatement ),
                            sPeerPartInfo->partitionInfo->tableID,
                            (UChar *)sPeerPartInfo->partitionInfo->name,
                            idlOS::strlen( sPeerPartInfo->partitionInfo->name ),
                            & sPartitionOrder )
                      != IDE_SUCCESS );
        }

        sPartitionNamePos.stmtText = sPeerPartInfo->partitionInfo->name;
        sPartitionNamePos.offset   = 0;
        sPartitionNamePos.size     = idlOS::strlen( sPeerPartInfo->partitionInfo->name );

        IDE_TEST( qdbCommon::insertTablePartitionSpecIntoMeta( aStatement,
                                                               aMyTableInfo->tableOwnerID,
                                                               aMyTableInfo->tableID,
                                                               sPartitionOID,
                                                               sPartitionID,
                                                               sPartitionNamePos,
                                                               sPartCondMinValues,
                                                               sPartCondMaxValues,
                                                               sPartitionOrder,
                                                               sPeerPartInfo->partitionInfo->TBSID,
                                                               sPeerPartInfo->partitionInfo->accessOption )
                  != IDE_SUCCESS );

        IDE_TEST( qdbCommon::insertPartLobSpecIntoMeta( aStatement,
                                                        aMyTableInfo->tableOwnerID,
                                                        aMyTableInfo->tableID,
                                                        sPartitionID,
                                                        sPeerPartInfo->partitionInfo->columns )
                  != IDE_SUCCESS );

        IDE_TEST( qcmPartition::makeAndSetQcmPartitionInfo( QC_SMI_STMT( aStatement ),
                                                            sPartitionID,
                                                            sPartitionOID,
                                                            aMyTableInfo,
                                                            NULL )
                  != IDE_SUCCESS );

        IDE_TEST( qcmPartition::getPartitionInfoByID( aStatement,
                                                      sPartitionID,
                                                      & sPartitionInfo,
                                                      & sSCN,
                                                      & sPartitionHandle )
                  != IDE_SUCCESS );

        IDU_FIT_POINT( "qdbCopySwap::executeCreateTablePartition::alloc::sPartInfoList",
                       idERR_ABORT_InsufficientMemory );

        IDE_TEST( QC_QMX_MEM( aStatement )->alloc( ID_SIZEOF( qcmPartitionInfoList ),
                                                   (void **) & sPartInfoList )
                  != IDE_SUCCESS );

        sPartInfoList->partitionInfo = sPartitionInfo;
        sPartInfoList->partHandle    = sPartitionHandle;
        sPartInfoList->partSCN       = sSCN;

        if ( sMyPartInfoList == NULL )
        {
            sPartInfoList->next = NULL;
        }
        else
        {
            sPartInfoList->next = sMyPartInfoList;
        }

        sMyPartInfoList = sPartInfoList;
    }

    *aMyPartInfoList = sMyPartInfoList;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    (void)qcmPartition::destroyQcmPartitionInfoList( sMyPartInfoList );

    return IDE_FAILURE;
}

IDE_RC qdbCopySwap::createConstraintAndIndexFromInfo( qcStatement           * aStatement,
                                                      qcmTableInfo          * aOldTableInfo,
                                                      qcmTableInfo          * aNewTableInfo,
                                                      UInt                    aPartitionCount,
                                                      qcmPartitionInfoList  * aNewPartInfoList,
                                                      UInt                    aIndexCrtFlag,
                                                      qcmIndex              * aNewTableIndex,
                                                      qcmIndex             ** aNewPartIndex,
                                                      UInt                    aNewPartIndexCount,
                                                      qdIndexTableList      * aOldIndexTables,
                                                      qdIndexTableList     ** aNewIndexTables,
                                                      qcNamePosition          aNamesPrefix )
{
/***********************************************************************
 * Description :
 *      Target Table에 Constraint를 생성한다.
 *          - Next Constraint ID를 얻는다.
 *          - CONSTRAINT_NAME에 사용자가 지정한 Prefix를 붙여서 CONSTRAINT_NAME을 생성한다.
 *              - 생성한 CONSTRAINT_NAME이 Unique한지 확인해야 한다.
 *                  - CONSTRAINT_NAME은 Unique Index의 Column이 아니다. 코드로 Unique를 검사한다.
 *          - Meta Table에 Target Table의 Constraint 정보를 추가한다. (Meta Table)
 *              - SYS_CONSTRAINTS_
 *                  - Primary Key, Unique, Local Unique인 경우, Index ID가 SYS_CONSTRAINTS_에 필요하다.
 *              - SYS_CONSTRAINT_COLUMNS_
 *              - SYS_CONSTRAINT_RELATED_
 *
 *      Target Table에 Index를 생성한다.
 *          - Source Table의 Index 정보를 사용하여, Target Table의 Index를 생성한다. (SM)
 *              - Next Index ID를 얻는다.
 *              - INDEX_NAME에 사용자가 지정한 Prefix를 붙여서 INDEX_NAME을 생성한다.
 *              - Target Table의 Table Handle이 필요하다.
 *          - Meta Table에 Target Table의 Index 정보를 추가한다. (Meta Table)
 *              - SYS_INDICES_
 *                  - INDEX_TABLE_ID는 0으로 초기화한다.
 *                  - LAST_DDL_TIME을 초기화한다. (SYSDATE)
 *              - SYS_INDEX_COLUMNS_
 *              - SYS_INDEX_RELATED_
 *
 *          - Partitioned Table이면, Local Index 또는 Non-Partitioned Index를 생성한다.
 *              - Local Index를 생성한다.
 *                  - Local Index를 생성한다. (SM)
 *                  - Meta Table에 Target Partition 정보를 추가한다. (Meta Table)
 *                      - SYS_PART_INDICES_
 *                      - SYS_INDEX_PARTITIONS_
 *
 *              - Non-Partitioned Index를 생성한다.
 *                  - INDEX_NAME으로 Index Table Name, Key Index Name, Rid Index Name을 결정한다.
 *                      - Call : qdx::checkIndexTableName()
 *                  - Non-Partitioned Index를 생성한다.
 *                      - Index Table을 생성한다. (SM, Meta Table, Meta Cache)
 *                      - Index Table의 Index를 생성한다. (SM, Meta Table)
 *                      - Index Table Info를 다시 얻는다. (Meta Cache)
 *                      - Call : qdx::createIndexTable(), qdx::createIndexTableIndices()
 *                  - Index Table ID를 갱신한다. (SYS_INDICES_.INDEX_TABLE_ID)
 *                      - Call : qdx::updateIndexSpecFromMeta()
 *
 * Implementation :
 *
 ***********************************************************************/

    mtcColumn               sNewTableIndexColumns[QC_MAX_KEY_COLUMN_COUNT];
    smiColumnList           sIndexColumnList[QC_MAX_KEY_COLUMN_COUNT];
    SChar                   sIndexTableName[QC_MAX_OBJECT_NAME_LEN + 1];
    SChar                   sKeyIndexName[QC_MAX_OBJECT_NAME_LEN + 1];
    SChar                   sRidIndexName[QC_MAX_OBJECT_NAME_LEN + 1];

    qcmColumn             * sQcmColumns                 = NULL;
    qcmIndex              * sIndex                      = NULL;
    qdIndexTableList      * sOldIndexTable              = NULL;
    qdIndexTableList      * sNewIndexTable              = NULL;
    qcmTableInfo          * sIndexTableInfo             = NULL;
    qcmIndex              * sIndexPartition             = NULL;
    qcmIndex              * sIndexTableIndex[2]         = { NULL, NULL };
    qcmPartitionInfoList  * sPartInfoList               = NULL;

    idBool                  sIsPrimary                  = ID_FALSE;
    idBool                  sIsPartitionedIndex         = ID_FALSE;
    ULong                   sDirectKeyMaxSize           = ID_ULONG( 0 );
    UInt                    sIndexPartID                = 0;
    UInt                    sFlag                       = 0;
    UInt                    i                           = 0;
    UInt                    k                           = 0;

    qcNamePosition          sUserNamePos;
    qcNamePosition          sIndexNamePos;
    qcNamePosition          sIndexTableNamePos;
    smiSegAttr              sSegAttr;
    smiSegStorageAttr       sSegStoAttr;

    // Index
    for ( i = 0; i < aOldTableInfo->indexCount; i++ )
    {
        sIndex = & aOldTableInfo->indices[i];

        // PROJ-1624 non-partitioned index
        // primary key index의 경우 non-partitioned index와 partitioned index
        // 둘 다 생성한다.
        if ( aOldTableInfo->primaryKey != NULL )
        {
            if ( aOldTableInfo->primaryKey->indexId == sIndex->indexId )
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
            /* Nothing to do */
        }

        // Next Index ID를 얻는다.
        IDE_TEST( qcm::getNextIndexID( aStatement, & aNewTableIndex[i].indexId ) != IDE_SUCCESS );

        IDE_TEST_RAISE( ( idlOS::strlen( sIndex->name ) + aNamesPrefix.size ) > QC_MAX_OBJECT_NAME_LEN,
                        ERR_TOO_LONG_OBJECT_NAME );

        // INDEX_NAME에 사용자가 지정한 Prefix를 붙여서 INDEX_NAME을 생성한다.
        QC_STR_COPY( aNewTableIndex[i].name, aNamesPrefix );
        idlOS::strncat( aNewTableIndex[i].name, sIndex->name, QC_MAX_OBJECT_NAME_LEN );

        // Index Column으로 smiColumnList를 만든다.
        IDE_TEST( makeIndexColumnList( sIndex,
                                       aNewTableInfo,
                                       sNewTableIndexColumns,
                                       sIndexColumnList )
                  != IDE_SUCCESS );

        sFlag             = smiTable::getIndexInfo( (const void *)sIndex->indexHandle );
        sSegAttr          = smiTable::getIndexSegAttr( (const void *)sIndex->indexHandle );
        sSegStoAttr       = smiTable::getIndexSegStoAttr( (const void *)sIndex->indexHandle );
        sDirectKeyMaxSize = (ULong) smiTable::getIndexMaxKeySize( (const void *)sIndex->indexHandle );
        IDE_TEST( smiTable::createIndex( QC_STATISTICS( aStatement ),
                                         QC_SMI_STMT( aStatement ),
                                         sIndex->TBSID,
                                         (const void *)aNewTableInfo->tableHandle,
                                         aNewTableIndex[i].name,
                                         aNewTableIndex[i].indexId,
                                         sIndex->indexTypeId,
                                         (const smiColumnList *)sIndexColumnList,
                                         sFlag,
                                         QD_INDEX_DEFAULT_PARALLEL_DEGREE,
                                         aIndexCrtFlag & SMI_INDEX_BUILD_UNCOMMITTED_ROW_MASK,
                                         sSegAttr,
                                         sSegStoAttr,
                                         sDirectKeyMaxSize,
                                         (const void **) & aNewTableIndex[i].indexHandle )
                  != IDE_SUCCESS );

        // PROJ-1624 global non-partitioned index
        if ( ( aNewTableInfo->tablePartitionType == QCM_PARTITIONED_TABLE ) &&
             ( sIndex->indexPartitionType != QCM_NONE_PARTITIONED_INDEX ) )
        {
            sIsPartitionedIndex = ID_TRUE;
        }
        else
        {
            sIsPartitionedIndex = ID_FALSE;
        }

        IDE_TEST( qdx::insertIndexIntoMeta( aStatement,
                                            sIndex->TBSID,
                                            sIndex->userID,
                                            aNewTableInfo->tableID,
                                            aNewTableIndex[i].indexId,
                                            aNewTableIndex[i].name,
                                            sIndex->indexTypeId,
                                            sIndex->isUnique,
                                            sIndex->keyColCount,
                                            sIndex->isRange,
                                            sIsPartitionedIndex,
                                            0, // aIndexTableID
                                            sFlag )
                  != IDE_SUCCESS );

        for ( k = 0; k < sIndex->keyColCount; k++ )
        {
            IDE_TEST( qdx::insertIndexColumnIntoMeta( aStatement,
                                                      sIndex->userID,
                                                      aNewTableIndex[i].indexId,
                                                      sIndexColumnList[k].column->id,
                                                      k,
                                                      ( ( ( sIndex->keyColsFlag[k] & SMI_COLUMN_ORDER_MASK )
                                                                                  == SMI_COLUMN_ORDER_ASCENDING )
                                                        ? ID_TRUE : ID_FALSE ),
                                                      aNewTableInfo->tableID )
                      != IDE_SUCCESS );
        }

        IDE_TEST( qdx::copyIndexRelatedMeta( aStatement,
                                             aNewTableInfo->tableID,
                                             aNewTableIndex[i].indexId,
                                             sIndex->indexId )
                  != IDE_SUCCESS );

        if ( aNewTableInfo->tablePartitionType == QCM_PARTITIONED_TABLE )
        {
            if ( sIndex->indexPartitionType == QCM_NONE_PARTITIONED_INDEX )
            {
                //--------------------------------
                // (global) non-partitioned index
                //--------------------------------

                // non-partitioned index에 해당하는 index table을 찾는다.
                IDE_TEST( qdx::findIndexTableInList( aOldIndexTables,
                                                     sIndex->indexTableID,
                                                     & sOldIndexTable )
                          != IDE_SUCCESS );

                sUserNamePos.stmtText = sIndex->userName;
                sUserNamePos.offset   = 0;
                sUserNamePos.size     = idlOS::strlen( sIndex->userName );

                sIndexNamePos.stmtText = aNewTableIndex[i].name;
                sIndexNamePos.offset   = 0;
                sIndexNamePos.size     = idlOS::strlen( aNewTableIndex[i].name );

                IDE_TEST( qdx::checkIndexTableName( aStatement,
                                                    sUserNamePos,
                                                    sIndexNamePos,
                                                    sIndexTableName,
                                                    sKeyIndexName,
                                                    sRidIndexName )
                          != IDE_SUCCESS );

                sIndexTableNamePos.stmtText = sIndexTableName;
                sIndexTableNamePos.offset   = 0;
                sIndexTableNamePos.size     = idlOS::strlen( sIndexTableName );

                // 아래에서 Partition을 생성할 때, qdbCommon::createTableOnSM()에서 Column 정보로 qcmColumn을 요구한다.
                //  - qdbCommon::createTableOnSM()
                //      - 실제로 필요한 정보는 mtcColumn이다.
                //      - Column ID를 새로 생성하고, Column 통계 정보를 초기화한다. (매개변수인 qcmColumn을 수정한다.)
                IDE_TEST( qcm::copyQcmColumns( QC_QMX_MEM( aStatement ),
                                               sOldIndexTable->tableInfo->columns,
                                               & sQcmColumns,
                                               sOldIndexTable->tableInfo->columnCount )
                          != IDE_SUCCESS );

                IDE_TEST( qdx::createIndexTable( aStatement,
                                                 sIndex->userID,
                                                 sIndexTableNamePos,
                                                 sQcmColumns,
                                                 sOldIndexTable->tableInfo->columnCount,
                                                 sOldIndexTable->tableInfo->TBSID,
                                                 sOldIndexTable->tableInfo->segAttr,
                                                 sOldIndexTable->tableInfo->segStoAttr,
                                                 QDB_TABLE_ATTR_MASK_ALL,
                                                 sOldIndexTable->tableInfo->tableFlag,
                                                 sOldIndexTable->tableInfo->parallelDegree,
                                                 & sNewIndexTable )
                          != IDE_SUCCESS );

                // link new index table
                sNewIndexTable->next = *aNewIndexTables;
                *aNewIndexTables = sNewIndexTable;

                // key index, rid index를 찾는다.
                IDE_TEST( qdx::getIndexTableIndices( sOldIndexTable->tableInfo,
                                                     sIndexTableIndex )
                          != IDE_SUCCESS );

                sFlag       = smiTable::getIndexInfo( (const void *)sIndexTableIndex[0]->indexHandle );
                sSegAttr    = smiTable::getIndexSegAttr( (const void *)sIndexTableIndex[0]->indexHandle );
                sSegStoAttr = smiTable::getIndexSegStoAttr( (const void *)sIndexTableIndex[0]->indexHandle );

                IDE_TEST( qdx::createIndexTableIndices( aStatement,
                                                        sIndex->userID,
                                                        sNewIndexTable,
                                                        NULL, // Key Column의 정렬 순서를 변경하지 않는다.
                                                        sKeyIndexName,
                                                        sRidIndexName,
                                                        sIndexTableIndex[0]->TBSID,
                                                        sIndexTableIndex[0]->indexTypeId,
                                                        sFlag,
                                                        QD_INDEX_DEFAULT_PARALLEL_DEGREE,
                                                        aIndexCrtFlag & SMI_INDEX_BUILD_UNCOMMITTED_ROW_MASK,
                                                        sSegAttr,
                                                        sSegStoAttr,
                                                        ID_ULONG(0) )
                          != IDE_SUCCESS );

                // tableInfo 재생성
                sIndexTableInfo = sNewIndexTable->tableInfo;

                IDE_TEST( qcm::makeAndSetQcmTableInfo( QC_SMI_STMT( aStatement ),
                                                       sNewIndexTable->tableID,
                                                       sNewIndexTable->tableOID )
                          != IDE_SUCCESS );

                IDE_TEST( qcm::getTableInfoByID( aStatement,
                                                 sNewIndexTable->tableID,
                                                 & sNewIndexTable->tableInfo,
                                                 & sNewIndexTable->tableSCN,
                                                 & sNewIndexTable->tableHandle )
                          != IDE_SUCCESS );

                (void)qcm::destroyQcmTableInfo( sIndexTableInfo );

                // index table id 설정
                aNewTableIndex[i].indexTableID = sNewIndexTable->tableID;

                IDE_TEST( qdx::updateIndexSpecFromMeta( aStatement,
                                                        aNewTableIndex[i].indexId,
                                                        sNewIndexTable->tableID )
                          != IDE_SUCCESS );
            }
            else
            {
                /* Nothing to do */
            }

            // primary key index의 경우 non-partitioned index와 partitioned index
            // 둘 다 생성한다.
            if ( ( sIndex->indexPartitionType != QCM_NONE_PARTITIONED_INDEX ) ||
                 ( sIsPrimary == ID_TRUE ) )
            {
                //--------------------------------
                // partitioned index
                //--------------------------------

                sFlag = smiTable::getIndexInfo( (const void *)sIndex->indexHandle );

                IDE_TEST( qdx::insertPartIndexIntoMeta( aStatement,
                                                        sIndex->userID,
                                                        aNewTableInfo->tableID,
                                                        aNewTableIndex[i].indexId,
                                                        0, // 항상 LOCAL(0)이다.
                                                        (SInt)sFlag )
                          != IDE_SUCCESS );

                for ( k = 0; k < sIndex->keyColCount; k++ )
                {
                    IDE_TEST( qdx::insertIndexPartKeyColumnIntoMeta( aStatement,
                                                                     sIndex->userID,
                                                                     aNewTableIndex[i].indexId,
                                                                     sIndexColumnList[k].column->id,
                                                                     aNewTableInfo,
                                                                     QCM_INDEX_OBJECT_TYPE )
                              != IDE_SUCCESS );
                }

                for ( sPartInfoList = aNewPartInfoList, k = 0;
                      ( sPartInfoList != NULL ) && ( k < aPartitionCount );
                      sPartInfoList = sPartInfoList->next, k++ )
                {
                    // partitioned index에 해당하는 local partition index를 찾는다.
                    IDE_TEST( qdx::findIndexIDInIndices( aNewPartIndex[k],
                                                         aNewPartIndexCount,
                                                         sIndex->indexId,
                                                         & sIndexPartition )
                              != IDE_SUCCESS );

                    /* PROJ-2464 hybrid partitioned table 지원
                     *  - Column 또는 Index 중 하나만 전달해야 한다.
                     */
                    IDE_TEST( qdbCommon::adjustIndexColumn( NULL,
                                                            sIndexPartition,
                                                            NULL,
                                                            sIndexColumnList )
                              != IDE_SUCCESS );

                    /* PROJ-2464 hybrid partitioned table 지원 */
                    sFlag       = smiTable::getIndexInfo( (const void *)sIndexPartition->indexHandle );
                    sSegAttr    = smiTable::getIndexSegAttr( (const void *)sIndexPartition->indexHandle );
                    sSegStoAttr = smiTable::getIndexSegStoAttr( (const void *)sIndexPartition->indexHandle );

                    IDE_TEST( smiTable::createIndex( QC_STATISTICS( aStatement ),
                                                     QC_SMI_STMT( aStatement ),
                                                     sIndexPartition->TBSID,
                                                     (const void *)sPartInfoList->partHandle,
                                                     sIndexPartition->name,
                                                     aNewTableIndex[i].indexId,
                                                     sIndex->indexTypeId,
                                                     sIndexColumnList,
                                                     sFlag,
                                                     QD_INDEX_DEFAULT_PARALLEL_DEGREE,
                                                     aIndexCrtFlag & SMI_INDEX_BUILD_UNCOMMITTED_ROW_MASK,
                                                     sSegAttr,
                                                     sSegStoAttr,
                                                     0, /* BUG-42124 Direct Key는 Partitioned Table 미지원 */
                                                     (const void **) & sIndexPartition->indexHandle )
                              != IDE_SUCCESS );

                    IDE_TEST( qcmPartition::getNextIndexPartitionID( aStatement,
                                                                     & sIndexPartID )
                              != IDE_SUCCESS );

                    IDE_TEST( qdx::insertIndexPartitionsIntoMeta( aStatement,
                                                                  sIndex->userID,
                                                                  aNewTableInfo->tableID,
                                                                  aNewTableIndex[i].indexId,
                                                                  sPartInfoList->partitionInfo->partitionID,
                                                                  sIndexPartID,
                                                                  sIndexPartition->name,
                                                                  NULL, // aPartMinValue (미사용)
                                                                  NULL, // aPartMaxValue (미사용)
                                                                  sIndexPartition->TBSID )
                              != IDE_SUCCESS );
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
    }

    IDE_TEST( createConstraintFromInfoAfterIndex( aStatement,
                                                  aOldTableInfo,
                                                  aNewTableInfo,
                                                  aNewTableIndex,
                                                  aNamesPrefix )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_TOO_LONG_OBJECT_NAME )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDB_USING_TARGET_NAMES_PREFIX_IS_TOO_LONG ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdbCopySwap::makeIndexColumnList( qcmIndex      * aIndex,
                                         qcmTableInfo  * aNewTableInfo,
                                         mtcColumn     * aNewTableIndexColumns,
                                         smiColumnList * aIndexColumnList )
{
    mtcColumn             * sMtcColumn                  = NULL;
    UInt                    sColumnOrder                = 0;
    UInt                    sOffset                     = 0;
    UInt                    j                           = 0;

    for ( j = 0; j < aIndex->keyColCount; j++ )
    {
        sColumnOrder = aIndex->keyColumns[j].column.id & SMI_COLUMN_ID_MASK;

        IDE_TEST( smiGetTableColumns( (const void *)aNewTableInfo->tableHandle,
                                      sColumnOrder,
                                      (const smiColumn **) & sMtcColumn )
                  != IDE_SUCCESS );

        idlOS::memcpy( & aNewTableIndexColumns[j],
                       sMtcColumn,
                       ID_SIZEOF( mtcColumn ) );
        aNewTableIndexColumns[j].column.flag = aIndex->keyColsFlag[j];

        // Disk Table이면, Index Column의 flag, offset, value를 Index에 맞게 조정한다.
        if ( ( aNewTableInfo->tableFlag & SMI_TABLE_TYPE_MASK ) == SMI_TABLE_DISK )
        {
            IDE_TEST( qdbCommon::setIndexKeyColumnTypeFlag( & aNewTableIndexColumns[j] )
                      != IDE_SUCCESS );

            if ( ( aIndex->keyColumns[j].column.flag & SMI_COLUMN_TYPE_MASK ) == SMI_COLUMN_TYPE_FIXED )
            {
                sOffset = idlOS::align( sOffset,
                                        aIndex->keyColumns[j].module->align );
                aNewTableIndexColumns[j].column.offset = sOffset;
                aNewTableIndexColumns[j].column.value = NULL;
                sOffset += aNewTableIndexColumns[j].column.size;
            }
            else
            {
                sOffset = idlOS::align( sOffset, 8 );
                aNewTableIndexColumns[j].column.offset = sOffset;
                aNewTableIndexColumns[j].column.value = NULL;
                sOffset += smiGetVariableColumnSize4DiskIndex();
            }
        }
        else
        {
            /* Nothing to do */
        }

        aIndexColumnList[j].column = (smiColumn *) & aNewTableIndexColumns[j];

        if ( j == ( aIndex->keyColCount - 1 ) )
        {
            aIndexColumnList[j].next = NULL;
        }
        else
        {
            aIndexColumnList[j].next = &aIndexColumnList[j + 1];
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdbCopySwap::createConstraintFromInfoAfterIndex( qcStatement    * aStatement,
                                                        qcmTableInfo   * aOldTableInfo,
                                                        qcmTableInfo   * aNewTableInfo,
                                                        qcmIndex       * aNewTableIndex,
                                                        qcNamePosition   aNamesPrefix )
{
    SChar                   sConstrName[QC_MAX_OBJECT_NAME_LEN + 1];

    qcmUnique             * sUnique                     = NULL;
    qcmForeignKey         * sForeign                    = NULL;
    qcmNotNull            * sNotNulls                   = NULL;
    qcmCheck              * sChecks                     = NULL;
    qcmTimeStamp          * sTimestamp                  = NULL;
    SChar                 * sCheckConditionStrForMeta   = NULL;
    idBool                  sExistSameConstrName        = ID_FALSE;
    UInt                    sConstrID                   = 0;
    UInt                    sColumnID                   = 0;
    UInt                    i                           = 0;
    UInt                    j                           = 0;
    UInt                    k                           = 0;

    qcuSqlSourceInfo        sqlInfo;

    // Primary Key, Unique Key, Local Unique Key
    for ( i = 0; i < aOldTableInfo->uniqueKeyCount; i++ )
    {
        sUnique = & aOldTableInfo->uniqueKeys[i];

        // Next Constraint ID를 얻는다.
        IDE_TEST( qcm::getNextConstrID( aStatement, & sConstrID ) != IDE_SUCCESS );

        IDE_TEST_RAISE( ( idlOS::strlen( sUnique->name ) + aNamesPrefix.size ) > QC_MAX_OBJECT_NAME_LEN,
                        ERR_TOO_LONG_OBJECT_NAME );

        // CONSTRAINT_NAME에 사용자가 지정한 Prefix를 붙여서 CONSTRAINT_NAME을 생성한다.
        QC_STR_COPY( sConstrName, aNamesPrefix );
        idlOS::strncat( sConstrName, sUnique->name, QC_MAX_OBJECT_NAME_LEN );

        // 생성한 CONSTRAINT_NAME이 Unique한지 확인해야 한다.
        // CONSTRAINT_NAME은 Unique Index의 Column이 아니다. 코드로 Unique를 검사한다.
        IDE_TEST( qdn::existSameConstrName( aStatement,
                                            sConstrName,
                                            aNewTableInfo->tableOwnerID,
                                            & sExistSameConstrName )
                  != IDE_SUCCESS );
        IDE_TEST_RAISE( sExistSameConstrName == ID_TRUE, ERR_DUP_CONSTR_NAME );

        for ( j = 0; j < aOldTableInfo->indexCount; j++ )
        {
            if ( sUnique->constraintIndex->indexId == aOldTableInfo->indices[j].indexId )
            {
                break;
            }
            else
            {
                /* Nothing to do */
            }
        }

        IDE_TEST( qdn::insertConstraintIntoMeta( aStatement,
                                                 aNewTableInfo->tableOwnerID,
                                                 aNewTableInfo->tableID,
                                                 sConstrID,
                                                 sConstrName,
                                                 sUnique->constraintType,
                                                 aNewTableIndex[j].indexId,
                                                 sUnique->constraintColumnCount,
                                                 0, // aReferencedTblID
                                                 0, // aReferencedIndexID
                                                 0, // aReferencedRule
                                                 (SChar *)"", /* PROJ-1107 Check Constraint 지원 */
                                                 ID_TRUE ) // ConstraintState의 Validate
                  != IDE_SUCCESS );

        for ( k = 0; k < sUnique->constraintColumnCount; k++ )
        {
            sColumnID = ( aNewTableInfo->tableID * SMI_COLUMN_ID_MAXIMUM )
                      + ( sUnique->constraintColumn[k] & SMI_COLUMN_ID_MASK );

            IDE_TEST( qdn::insertConstraintColumnIntoMeta( aStatement,
                                                           aNewTableInfo->tableOwnerID,
                                                           aNewTableInfo->tableID,
                                                           sConstrID,
                                                           k,
                                                           sColumnID )
                      != IDE_SUCCESS );
        }
    }

    for ( i = 0; i < aOldTableInfo->foreignKeyCount; i++ )
    {
        sForeign = & aOldTableInfo->foreignKeys[i];

        // Next Constraint ID를 얻는다.
        IDE_TEST( qcm::getNextConstrID( aStatement, & sConstrID ) != IDE_SUCCESS );

        IDE_TEST_RAISE( ( idlOS::strlen( sForeign->name ) + aNamesPrefix.size ) > QC_MAX_OBJECT_NAME_LEN,
                        ERR_TOO_LONG_OBJECT_NAME );

        // CONSTRAINT_NAME에 사용자가 지정한 Prefix를 붙여서 CONSTRAINT_NAME을 생성한다.
        QC_STR_COPY( sConstrName, aNamesPrefix );
        idlOS::strncat( sConstrName, sForeign->name, QC_MAX_OBJECT_NAME_LEN );

        // 생성한 CONSTRAINT_NAME이 Unique한지 확인해야 한다.
        // CONSTRAINT_NAME은 Unique Index의 Column이 아니다. 코드로 Unique를 검사한다.
        IDE_TEST( qdn::existSameConstrName( aStatement,
                                            sConstrName,
                                            aNewTableInfo->tableOwnerID,
                                            & sExistSameConstrName )
                  != IDE_SUCCESS );
        IDE_TEST_RAISE( sExistSameConstrName == ID_TRUE, ERR_DUP_CONSTR_NAME );

        IDE_TEST( qdn::insertConstraintIntoMeta( aStatement,
                                                 aNewTableInfo->tableOwnerID,
                                                 aNewTableInfo->tableID,
                                                 sConstrID,
                                                 sConstrName,
                                                 QD_FOREIGN,
                                                 0,
                                                 sForeign->constraintColumnCount,
                                                 sForeign->referencedTableID,
                                                 sForeign->referencedIndexID,
                                                 sForeign->referenceRule,
                                                 (SChar*)"", /* PROJ-1107 Check Constraint 지원 */
                                                 sForeign->validated ) // ConstraintState의 Validate
                  != IDE_SUCCESS );

        for ( k = 0; k < sForeign->constraintColumnCount; k++ )
        {
            sColumnID = ( aNewTableInfo->tableID * SMI_COLUMN_ID_MAXIMUM )
                      + ( sForeign->referencingColumn[k] & SMI_COLUMN_ID_MASK );

            IDE_TEST( qdn::insertConstraintColumnIntoMeta( aStatement,
                                                           aNewTableInfo->tableOwnerID,
                                                           aNewTableInfo->tableID,
                                                           sConstrID,
                                                           k,
                                                           sColumnID )
                      != IDE_SUCCESS );
        }
    }

    for ( i = 0; i < aOldTableInfo->notNullCount; i++ )
    {
        sNotNulls = & aOldTableInfo->notNulls[i];

        // Next Constraint ID를 얻는다.
        IDE_TEST( qcm::getNextConstrID( aStatement, & sConstrID ) != IDE_SUCCESS );

        IDE_TEST_RAISE( ( idlOS::strlen( sNotNulls->name ) + aNamesPrefix.size ) > QC_MAX_OBJECT_NAME_LEN,
                        ERR_TOO_LONG_OBJECT_NAME );

        // CONSTRAINT_NAME에 사용자가 지정한 Prefix를 붙여서 CONSTRAINT_NAME을 생성한다.
        QC_STR_COPY( sConstrName, aNamesPrefix );
        idlOS::strncat( sConstrName, sNotNulls->name, QC_MAX_OBJECT_NAME_LEN );

        // 생성한 CONSTRAINT_NAME이 Unique한지 확인해야 한다.
        // CONSTRAINT_NAME은 Unique Index의 Column이 아니다. 코드로 Unique를 검사한다.
        IDE_TEST( qdn::existSameConstrName( aStatement,
                                            sConstrName,
                                            aNewTableInfo->tableOwnerID,
                                            & sExistSameConstrName )
                  != IDE_SUCCESS );
        IDE_TEST_RAISE( sExistSameConstrName == ID_TRUE, ERR_DUP_CONSTR_NAME );

        IDE_TEST( qdn::insertConstraintIntoMeta( aStatement,
                                                 aNewTableInfo->tableOwnerID,
                                                 aNewTableInfo->tableID,
                                                 sConstrID,
                                                 sConstrName,
                                                 QD_NOT_NULL,
                                                 0,
                                                 sNotNulls->constraintColumnCount,
                                                 0, // aReferencedTblID
                                                 0, // aReferencedIndexID
                                                 0, // aReferencedRule
                                                 (SChar *)"", /* PROJ-1107 Check Constraint 지원 */
                                                 ID_TRUE ) // ConstraintState의 Validate
                  != IDE_SUCCESS );

        for ( k = 0; k < sNotNulls->constraintColumnCount; k++ )
        {
            sColumnID = ( aNewTableInfo->tableID * SMI_COLUMN_ID_MAXIMUM )
                      + ( sNotNulls->constraintColumn[k] & SMI_COLUMN_ID_MASK );

            IDE_TEST( qdn::insertConstraintColumnIntoMeta( aStatement,
                                                           aNewTableInfo->tableOwnerID,
                                                           aNewTableInfo->tableID,
                                                           sConstrID,
                                                           k,
                                                           sColumnID )
                      != IDE_SUCCESS );
        }
    }

    /* PROJ-1107 Check Constraint 지원 */
    for ( i = 0; i < aOldTableInfo->checkCount; i++ )
    {
        sChecks = & aOldTableInfo->checks[i];

        // Next Constraint ID를 얻는다.
        IDE_TEST( qcm::getNextConstrID( aStatement, & sConstrID ) != IDE_SUCCESS );

        IDE_TEST_RAISE( ( idlOS::strlen( sChecks->name ) + aNamesPrefix.size ) > QC_MAX_OBJECT_NAME_LEN,
                        ERR_TOO_LONG_OBJECT_NAME );

        // CONSTRAINT_NAME에 사용자가 지정한 Prefix를 붙여서 CONSTRAINT_NAME을 생성한다.
        QC_STR_COPY( sConstrName, aNamesPrefix );
        idlOS::strncat( sConstrName, sChecks->name, QC_MAX_OBJECT_NAME_LEN );

        // 생성한 CONSTRAINT_NAME이 Unique한지 확인해야 한다.
        // CONSTRAINT_NAME은 Unique Index의 Column이 아니다. 코드로 Unique를 검사한다.
        IDE_TEST( qdn::existSameConstrName( aStatement,
                                            sConstrName,
                                            aNewTableInfo->tableOwnerID,
                                            & sExistSameConstrName )
                  != IDE_SUCCESS );
        IDE_TEST_RAISE( sExistSameConstrName == ID_TRUE, ERR_DUP_CONSTR_NAME );

        IDE_TEST( qdbCommon::getStrForMeta( aStatement,
                                            sChecks->checkCondition,
                                            idlOS::strlen( sChecks->checkCondition ),
                                            & sCheckConditionStrForMeta )
                  != IDE_SUCCESS );

        IDE_TEST( qdn::insertConstraintIntoMeta( aStatement,
                                                 aNewTableInfo->tableOwnerID,
                                                 aNewTableInfo->tableID,
                                                 sConstrID,
                                                 sConstrName,
                                                 QD_CHECK,
                                                 0,
                                                 sChecks->constraintColumnCount,
                                                 0, // aReferencedTblID
                                                 0, // aReferencedIndexID
                                                 0, // aReferencedRule
                                                 sCheckConditionStrForMeta,
                                                 ID_TRUE ) // ConstraintState의 Validate
                  != IDE_SUCCESS );

        for ( k = 0; k < sChecks->constraintColumnCount; k++ )
        {
            sColumnID = ( aNewTableInfo->tableID * SMI_COLUMN_ID_MAXIMUM )
                      + ( sChecks->constraintColumn[k] & SMI_COLUMN_ID_MASK );

            IDE_TEST( qdn::insertConstraintColumnIntoMeta( aStatement,
                                                           aNewTableInfo->tableOwnerID,
                                                           aNewTableInfo->tableID,
                                                           sConstrID,
                                                           k,
                                                           sColumnID )
                      != IDE_SUCCESS );
        }

        IDE_TEST( qdn::copyConstraintRelatedMeta( aStatement,
                                                  aNewTableInfo->tableID,
                                                  sConstrID,
                                                  sChecks->constraintID )
                  != IDE_SUCCESS );
    }

    if ( aOldTableInfo->timestamp != NULL )
    {
        sTimestamp = aOldTableInfo->timestamp;

        // Next Constraint ID를 얻는다.
        IDE_TEST( qcm::getNextConstrID( aStatement, & sConstrID ) != IDE_SUCCESS );

        IDE_TEST_RAISE( ( idlOS::strlen( sTimestamp->name ) + aNamesPrefix.size ) > QC_MAX_OBJECT_NAME_LEN,
                        ERR_TOO_LONG_OBJECT_NAME );

        // CONSTRAINT_NAME에 사용자가 지정한 Prefix를 붙여서 CONSTRAINT_NAME을 생성한다.
        QC_STR_COPY( sConstrName, aNamesPrefix );
        idlOS::strncat( sConstrName, sTimestamp->name, QC_MAX_OBJECT_NAME_LEN );

        // 생성한 CONSTRAINT_NAME이 Unique한지 확인해야 한다.
        // CONSTRAINT_NAME은 Unique Index의 Column이 아니다. 코드로 Unique를 검사한다.
        IDE_TEST( qdn::existSameConstrName( aStatement,
                                            sConstrName,
                                            aNewTableInfo->tableOwnerID,
                                            & sExistSameConstrName )
                  != IDE_SUCCESS );
        IDE_TEST_RAISE( sExistSameConstrName == ID_TRUE, ERR_DUP_CONSTR_NAME );

        IDE_TEST( qdn::insertConstraintIntoMeta( aStatement,
                                                 aNewTableInfo->tableOwnerID,
                                                 aNewTableInfo->tableID,
                                                 sConstrID,
                                                 sConstrName,
                                                 QD_TIMESTAMP,
                                                 0,
                                                 sTimestamp->constraintColumnCount,
                                                 0, // aReferencedTblID
                                                 0, // aReferencedIndexID
                                                 0, // aReferencedRule
                                                 (SChar *)"", /* PROJ-1107 Check Constraint 지원 */
                                                 ID_TRUE ) // ConstraintState의 Validate
                  != IDE_SUCCESS );

        sColumnID = ( aNewTableInfo->tableID * SMI_COLUMN_ID_MAXIMUM )
                  + ( sTimestamp->constraintColumn[0] & SMI_COLUMN_ID_MASK );

        IDE_TEST( qdn::insertConstraintColumnIntoMeta( aStatement,
                                                       aNewTableInfo->tableOwnerID,
                                                       aNewTableInfo->tableID,
                                                       sConstrID,
                                                       0,
                                                       sColumnID )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_TOO_LONG_OBJECT_NAME )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDB_USING_TARGET_NAMES_PREFIX_IS_TOO_LONG ) );
    }
    IDE_EXCEPTION( ERR_DUP_CONSTR_NAME )
    {
        sqlInfo.setSourceInfo( aStatement, & aNamesPrefix );
        (void)sqlInfo.init( QC_QME_MEM( aStatement ) );
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDN_DUPLICATE_CONSTRAINT,
                                  sqlInfo.getErrMessage() ) );
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdbCopySwap::swapTablesMeta( qcStatement * aStatement,
                                    UInt          aTargetTableID,
                                    UInt          aSourceTableID )
{
/***********************************************************************
 *
 * Description :
 *      Source와 Target의 Table 기본 정보를 교환한다. (Meta Table)
 *          - SYS_TABLES_
 *              - TABLE_NAME, REPLICATION_COUNT, REPLICATION_RECOVERY_COUNT를 교환한다.
 *              - LAST_DDL_TIME을 갱신한다. (SYSDATE)
 *              - USABLE, SHARD_FLAG를 교환한다. (TASK-7307)
 *
 * Implementation :
 *
 ***********************************************************************/

    SChar  * sSqlStr = NULL;
    vSLong   sRowCnt = ID_vLONG(0);

    IDU_FIT_POINT( "qdbCopySwap::swapTablesMeta::STRUCT_ALLOC_WITH_SIZE::sSqlStr",
                   idERR_ABORT_InsufficientMemory );

    IDE_TEST( STRUCT_ALLOC_WITH_SIZE( QC_QMX_MEM( aStatement ),
                                      SChar,
                                      QD_MAX_SQL_LENGTH,
                                      & sSqlStr )
              != IDE_SUCCESS );

    idlOS::snprintf( sSqlStr,
                     QD_MAX_SQL_LENGTH,
                     "UPDATE SYS_TABLES_ A "
                     "   SET (        TABLE_NAME, REPLICATION_COUNT, REPLICATION_RECOVERY_COUNT, USABLE, SHARD_FLAG ) = "
                     "       ( SELECT TABLE_NAME, REPLICATION_COUNT, REPLICATION_RECOVERY_COUNT, USABLE, SHARD_FLAG "
                     "           FROM SYS_TABLES_ B "
                     "          WHERE B.TABLE_ID = CASE2( A.TABLE_ID = INTEGER'%"ID_INT32_FMT"', "
                     "                                    INTEGER'%"ID_INT32_FMT"', "
                     "                                    INTEGER'%"ID_INT32_FMT"' ) ), "
                     "       LAST_DDL_TIME = SYSDATE "
                     " WHERE TABLE_ID IN ( INTEGER'%"ID_INT32_FMT"', INTEGER'%"ID_INT32_FMT"' ) ",
                     aTargetTableID,
                     aSourceTableID,
                     aTargetTableID,
                     aTargetTableID,
                     aSourceTableID );

    IDE_TEST( qcg::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                 sSqlStr,
                                 & sRowCnt )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( sRowCnt != 2, ERR_META_CRASH );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_META_CRASH )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QCM_META_CRASH ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdbCopySwap::renameHiddenColumnsMeta( qcStatement    * aStatement,
                                             qcmTableInfo   * aTargetTableInfo,
                                             qcmTableInfo   * aSourceTableInfo,
                                             qcNamePosition   aNamesPrefix )
{
/***********************************************************************
 *
 * Description :
 *      Hidden Column이면 Function-based Index의 Column이므로,
 *      사용자가 Prefix를 지정한 경우, Hidden Column Name을 변경한다. (Meta Table)
 *          - SYS_COLUMNS_
 *              - Source의 Hidden Column Name에 Prefix를 붙이고, Target의 Hidden Column Name에서 Prefix를 제거한다.
 *                  - Hidden Column Name = Index Name + $ + IDX + Number
 *              - Hidden Column Name을 변경한다.
 *          - SYS_ENCRYPTED_COLUMNS_, SYS_LOBS_, SYS_COMPRESSION_TABLES_
 *              - 변경 사항 없음
 *
 * Implementation :
 *
 ***********************************************************************/

    SChar       sObjectName[QC_MAX_OBJECT_NAME_LEN + 1];
    SChar       sNamesPrefix[QC_MAX_OBJECT_NAME_LEN + 1];
    qcmColumn * sColumn            = NULL;
    UInt        sHiddenColumnCount = 0;
    SChar     * sSqlStr            = NULL;
    vSLong      sRowCnt            = ID_vLONG(0);

    if ( QC_IS_NULL_NAME( aNamesPrefix ) != ID_TRUE )
    {
        QC_STR_COPY( sNamesPrefix, aNamesPrefix );

        IDU_FIT_POINT( "qdbCopySwap::renameHiddenColumnsMeta::STRUCT_ALLOC_WITH_SIZE::sSqlStr",
                       idERR_ABORT_InsufficientMemory );

        IDE_TEST( STRUCT_ALLOC_WITH_SIZE( QC_QMX_MEM( aStatement ),
                                          SChar,
                                          QD_MAX_SQL_LENGTH,
                                          & sSqlStr )
                  != IDE_SUCCESS );

        for ( sColumn = aSourceTableInfo->columns, sHiddenColumnCount = 0;
              sColumn != NULL;
              sColumn = sColumn->next )
        {
            if ( ( sColumn->flag & QCM_COLUMN_HIDDEN_COLUMN_MASK )
                                == QCM_COLUMN_HIDDEN_COLUMN_TRUE )
            {
                IDE_TEST_RAISE( ( idlOS::strlen( sColumn->name ) + aNamesPrefix.size ) >
                                QC_MAX_OBJECT_NAME_LEN,
                                ERR_NAMES_PREFIX_IS_TOO_LONG );

                sHiddenColumnCount++;
            }
            else
            {
                /* Nothing to do */
            }
        }

        idlOS::snprintf( sSqlStr,
                         QD_MAX_SQL_LENGTH,
                         "UPDATE SYS_COLUMNS_ "
                         "   SET COLUMN_NAME = VARCHAR'%s' || COLUMN_NAME "
                         " WHERE TABLE_ID = INTEGER'%"ID_INT32_FMT"' AND "
                         "       IS_HIDDEN = CHAR'T' ",
                         sNamesPrefix,
                         aSourceTableInfo->tableID );

        IDE_TEST( qcg::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                     sSqlStr,
                                     & sRowCnt )
                  != IDE_SUCCESS );

        IDE_TEST_RAISE( sHiddenColumnCount != (UInt)sRowCnt, ERR_META_CRASH );

        for ( sColumn = aTargetTableInfo->columns, sHiddenColumnCount = 0;
              sColumn != NULL;
              sColumn = sColumn->next )
        {
            if ( ( sColumn->flag & QCM_COLUMN_HIDDEN_COLUMN_MASK )
                                == QCM_COLUMN_HIDDEN_COLUMN_TRUE )
            {
                IDE_TEST_RAISE( idlOS::strlen( sColumn->name ) <= (UInt)aNamesPrefix.size,
                                ERR_NAMES_PREFIX_IS_TOO_LONG );

                IDE_TEST_RAISE( idlOS::strMatch( sColumn->name,
                                                 aNamesPrefix.size,
                                                 aNamesPrefix.stmtText + aNamesPrefix.offset,
                                                 aNamesPrefix.size ) != 0,
                                ERR_NAMES_PREFIX_IS_TOO_LONG );

                idlOS::strncpy( sObjectName, sColumn->name + aNamesPrefix.size, QC_MAX_OBJECT_NAME_LEN + 1 );

                IDE_TEST_RAISE( idlOS::strstr( sObjectName, "$" ) == NULL,
                                ERR_NAMES_PREFIX_IS_TOO_LONG );

                sHiddenColumnCount++;
            }
            else
            {
                /* Nothing to do */
            }
        }

        idlOS::snprintf( sSqlStr,
                         QD_MAX_SQL_LENGTH,
                         "UPDATE SYS_COLUMNS_ "
                         "   SET COLUMN_NAME = SUBSTR( COLUMN_NAME, INTEGER'%"ID_INT32_FMT"' ) "
                         " WHERE TABLE_ID = INTEGER'%"ID_INT32_FMT"' AND "
                         "       IS_HIDDEN = CHAR'T' ",
                         aNamesPrefix.size + 1,
                         aTargetTableInfo->tableID );

        IDE_TEST( qcg::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                     sSqlStr,
                                     & sRowCnt )
                  != IDE_SUCCESS );

        IDE_TEST_RAISE( sHiddenColumnCount != (UInt)sRowCnt, ERR_META_CRASH );
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NAMES_PREFIX_IS_TOO_LONG )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDB_USING_TARGET_NAMES_PREFIX_IS_TOO_LONG ) );
    }
    IDE_EXCEPTION( ERR_META_CRASH )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QCM_META_CRASH ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdbCopySwap::renameIndices( qcStatement       * aStatement,
                                   qcmTableInfo      * aTargetTableInfo,
                                   qcmTableInfo      * aSourceTableInfo,
                                   qcNamePosition      aNamesPrefix,
                                   qdIndexTableList  * aTargetIndexTableList,
                                   qdIndexTableList  * aSourceIndexTableList,
                                   qdIndexTableList ** aNewTargetIndexTableList,
                                   qdIndexTableList ** aNewSourceIndexTableList )
{
/***********************************************************************
 *
 * Description :
 *      Prefix를 지정한 경우, Source의 INDEX_NAME에 Prefix를 붙이고, Target의 INDEX_NAME에서 Prefix를 제거한다.
 *          - 실제 Index Name을 변경한다. (SM)
 *              - Call : smiTable::alterIndexName()
 *          - Meta Table에서 Index Name을 변경한다. (Meta Table)
 *              - SYS_INDICES_
 *                  - INDEX_NAME을 변경한다.
 *                  - LAST_DDL_TIME을 갱신한다. (SYSDATE)
 *              - SYS_INDEX_COLUMNS_, SYS_INDEX_RELATED_
 *                  - 변경 사항 없음
 *
 *      Prefix를 지정한 경우, Non-Partitioned Index이 있으면 Name을 변경한다.
 *          - Non-Partitioned Index인 경우, (1) Index Table Name과 (2) Index Table의 Index Name을 변경한다.
 *              - Non-Partitioned Index는 INDEX_NAME으로 Index Table Name, Key Index Name, RID Index Name을 결정한다.
 *                  - Index Table Name = $GIT_ + Index Name
 *                  - Key Index Name = $GIK_ + Index Name
 *                  - Rid Index Name = $GIR_ + Index Name
 *                  - Call : qdx::makeIndexTableName()
 *              - Index Table Name을 변경한다. (Meta Table)
 *              - Index Table의 Index Name을 변경한다. (SM, Meta Table)
 *                  - Call : smiTable::alterIndexName()
 *              - Index Table Info를 다시 얻는다. (Meta Cache)
 *
 * Implementation :
 *
 ***********************************************************************/

    SChar              sNamesPrefix[QC_MAX_OBJECT_NAME_LEN + 1];

    UInt               i                        = 0;
    SChar            * sSqlStr                  = NULL;
    vSLong             sRowCnt                  = ID_vLONG(0);

    qdIndexTableList * sIndexTableList          = NULL;
    qdIndexTableList * sNewTargetIndexTableList = NULL;
    qdIndexTableList * sNewSourceIndexTableList = NULL;
    UInt               sTargetIndexTableCount   = 0;
    UInt               sSourceIndexTableCount   = 0;

    *aNewTargetIndexTableList = NULL;
    *aNewSourceIndexTableList = NULL;

    if ( QC_IS_NULL_NAME( aNamesPrefix ) != ID_TRUE )
    {
        QC_STR_COPY( sNamesPrefix, aNamesPrefix );

        for ( sIndexTableList = aTargetIndexTableList, sTargetIndexTableCount = 0;
              sIndexTableList != NULL;
              sIndexTableList = sIndexTableList->next )
        {
            sTargetIndexTableCount++;
        }

        if ( sTargetIndexTableCount > 0 )
        {
            IDU_FIT_POINT( "qdbCopySwap::renameIndices::STRUCT_CRALLOC_WITH_COUNT::sNewTargetIndexTableList",
                           idERR_ABORT_InsufficientMemory );

            IDE_TEST( STRUCT_CRALLOC_WITH_COUNT( QC_QMX_MEM( aStatement ),
                                                 qdIndexTableList,
                                                 sTargetIndexTableCount,
                                                 & sNewTargetIndexTableList )
                      != IDE_SUCCESS );
        }
        else
        {
            /* Nothing to do */
        }

        for ( sIndexTableList = aSourceIndexTableList, sSourceIndexTableCount = 0;
              sIndexTableList != NULL;
              sIndexTableList = sIndexTableList->next )
        {
            sSourceIndexTableCount++;
        }

        if ( sSourceIndexTableCount > 0 )
        {
            IDU_FIT_POINT( "qdbCopySwap::renameIndices::STRUCT_CRALLOC_WITH_COUNT::sNewSourceIndexTableList",
                           idERR_ABORT_InsufficientMemory );

            IDE_TEST( STRUCT_CRALLOC_WITH_COUNT( QC_QMX_MEM( aStatement ),
                                                 qdIndexTableList,
                                                 sSourceIndexTableCount,
                                                 & sNewSourceIndexTableList )
                      != IDE_SUCCESS );
        }
        else
        {
            /* Nothing to do */
        }

        IDU_FIT_POINT( "qdbCopySwap::renameIndices::STRUCT_ALLOC_WITH_SIZE::sSqlStr",
                       idERR_ABORT_InsufficientMemory );

        IDE_TEST( STRUCT_ALLOC_WITH_SIZE( QC_QMX_MEM( aStatement ),
                                          SChar,
                                          QD_MAX_SQL_LENGTH,
                                          & sSqlStr )
                  != IDE_SUCCESS );

        IDE_TEST( renameIndicesOnSM( aStatement,
                                     aTargetTableInfo,
                                     aSourceTableInfo,
                                     aNamesPrefix,
                                     aTargetIndexTableList,
                                     aSourceIndexTableList )
                  != IDE_SUCCESS );

        idlOS::snprintf( sSqlStr,
                         QD_MAX_SQL_LENGTH,
                         "UPDATE SYS_INDICES_ "
                         "   SET INDEX_NAME = CASE2( TABLE_ID = INTEGER'%"ID_INT32_FMT"', "
                         "                           VARCHAR'%s' || INDEX_NAME, "
                         "                           SUBSTR( INDEX_NAME, INTEGER'%"ID_INT32_FMT"' ) ), "
                         "       LAST_DDL_TIME = SYSDATE "
                         " WHERE TABLE_ID IN ( INTEGER'%"ID_INT32_FMT"', "
                         "                     INTEGER'%"ID_INT32_FMT"' ) ",
                         aSourceTableInfo->tableID,
                         sNamesPrefix,
                         aNamesPrefix.size + 1,
                         aSourceTableInfo->tableID,
                         aTargetTableInfo->tableID );

        IDE_TEST( qcg::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                     sSqlStr,
                                     & sRowCnt )
                  != IDE_SUCCESS );

        IDE_TEST_RAISE( (UInt)sRowCnt != ( aTargetTableInfo->indexCount + aSourceTableInfo->indexCount ),
                        ERR_META_CRASH );

        /* Index Table Name을 변경한다. (Meta Table) */
        idlOS::snprintf( sSqlStr,
                         QD_MAX_SQL_LENGTH,
                         "UPDATE SYS_TABLES_ A "
                         "   SET TABLE_NAME = ( SELECT VARCHAR'%s' || INDEX_NAME "
                         "                        FROM SYS_INDICES_ B "
                         "                       WHERE B.TABLE_ID IN ( INTEGER'%"ID_INT32_FMT"', "
                         "                                             INTEGER'%"ID_INT32_FMT"' ) AND "
                         "                             INDEX_TABLE_ID = A.TABLE_ID ), "
                         "       LAST_DDL_TIME = SYSDATE "
                         " WHERE TABLE_ID IN ( SELECT INDEX_TABLE_ID "
                         "                       FROM SYS_INDICES_ "
                         "                      WHERE TABLE_ID IN ( INTEGER'%"ID_INT32_FMT"', "
                         "                                          INTEGER'%"ID_INT32_FMT"' ) AND "
                         "                            INDEX_TABLE_ID != INTEGER'0' ) AND "
                         "       TABLE_TYPE = CHAR'G' ",
                         QD_INDEX_TABLE_PREFIX,
                         aSourceTableInfo->tableID,
                         aTargetTableInfo->tableID,
                         aSourceTableInfo->tableID,
                         aTargetTableInfo->tableID );

        IDE_TEST( qcg::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                     sSqlStr,
                                     & sRowCnt )
                  != IDE_SUCCESS );

        IDE_TEST_RAISE( (UInt)sRowCnt != ( sSourceIndexTableCount + sTargetIndexTableCount ), ERR_META_CRASH );

        /* Index Table의 Index Name을 변경한다. (Meta Table) */
        idlOS::snprintf( sSqlStr,
                         QD_MAX_SQL_LENGTH,
                         "UPDATE SYS_INDICES_ A "
                         "   SET INDEX_NAME = VARCHAR'%s' || "
                         "                    ( SELECT INDEX_NAME "
                         "                        FROM SYS_INDICES_ B "
                         "                       WHERE B.TABLE_ID IN ( INTEGER'%"ID_INT32_FMT"', "
                         "                                             INTEGER'%"ID_INT32_FMT"' ) AND "
                         "                             INDEX_TABLE_ID = A.TABLE_ID ), "
                         "       LAST_DDL_TIME = SYSDATE "
                         " WHERE TABLE_ID IN ( SELECT INDEX_TABLE_ID "
                         "                       FROM SYS_INDICES_ "
                         "                      WHERE TABLE_ID IN ( INTEGER'%"ID_INT32_FMT"', "
                         "                                          INTEGER'%"ID_INT32_FMT"' ) AND "
                         "                            INDEX_TABLE_ID != INTEGER'0' "
                         "                   ) AND "
                         "       SUBSTR( INDEX_NAME, 1, INTEGER'%"ID_INT32_FMT"' ) = VARCHAR'%s' AND "
                         "       INDEX_TABLE_ID = INTEGER'0' ",
                         QD_INDEX_TABLE_KEY_INDEX_PREFIX,
                         aSourceTableInfo->tableID,
                         aTargetTableInfo->tableID,
                         aSourceTableInfo->tableID,
                         aTargetTableInfo->tableID,
                         QD_INDEX_TABLE_KEY_INDEX_PREFIX_SIZE,
                         QD_INDEX_TABLE_KEY_INDEX_PREFIX );

        IDE_TEST( qcg::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                     sSqlStr,
                                     & sRowCnt )
                  != IDE_SUCCESS );

        IDE_TEST_RAISE( (UInt)sRowCnt != ( sSourceIndexTableCount + sTargetIndexTableCount ), ERR_META_CRASH );

        idlOS::snprintf( sSqlStr,
                         QD_MAX_SQL_LENGTH,
                         "UPDATE SYS_INDICES_ A "
                         "   SET INDEX_NAME = VARCHAR'%s' || "
                         "                    ( SELECT INDEX_NAME "
                         "                        FROM SYS_INDICES_ B "
                         "                       WHERE B.TABLE_ID IN ( INTEGER'%"ID_INT32_FMT"', "
                         "                                             INTEGER'%"ID_INT32_FMT"' ) AND "
                         "                             INDEX_TABLE_ID = A.TABLE_ID ), "
                         "       LAST_DDL_TIME = SYSDATE "
                         " WHERE TABLE_ID IN ( SELECT INDEX_TABLE_ID "
                         "                       FROM SYS_INDICES_ "
                         "                      WHERE TABLE_ID IN ( INTEGER'%"ID_INT32_FMT"', "
                         "                                          INTEGER'%"ID_INT32_FMT"' ) AND "
                         "                            INDEX_TABLE_ID != INTEGER'0' "
                         "                   ) AND "
                         "       SUBSTR( INDEX_NAME, 1, INTEGER'%"ID_INT32_FMT"' ) = VARCHAR'%s' AND "
                         "       INDEX_TABLE_ID = INTEGER'0' ",
                         QD_INDEX_TABLE_RID_INDEX_PREFIX,
                         aSourceTableInfo->tableID,
                         aTargetTableInfo->tableID,
                         aSourceTableInfo->tableID,
                         aTargetTableInfo->tableID,
                         QD_INDEX_TABLE_RID_INDEX_PREFIX_SIZE,
                         QD_INDEX_TABLE_RID_INDEX_PREFIX );

        IDE_TEST( qcg::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                     sSqlStr,
                                     & sRowCnt )
                  != IDE_SUCCESS );

        IDE_TEST_RAISE( (UInt)sRowCnt != ( sSourceIndexTableCount + sTargetIndexTableCount ), ERR_META_CRASH );

        /* Index Table Info를 다시 얻는다. (Meta Cache) */
        for ( sIndexTableList = aTargetIndexTableList, i = 0;
              sIndexTableList != NULL;
              sIndexTableList = sIndexTableList->next, i++ )
        {
            IDE_TEST( qcm::makeAndSetQcmTableInfo( QC_SMI_STMT( aStatement ),
                                                   sIndexTableList->tableID,
                                                   sIndexTableList->tableOID )
                      != IDE_SUCCESS );

            IDE_TEST( qcm::getTableInfoByID( aStatement,
                                             sIndexTableList->tableID,
                                             & sNewTargetIndexTableList[i].tableInfo,
                                             & sNewTargetIndexTableList[i].tableSCN,
                                             & sNewTargetIndexTableList[i].tableHandle )
                      != IDE_SUCCESS );

            sNewTargetIndexTableList[i].tableID  = sNewTargetIndexTableList[i].tableInfo->tableID;
            sNewTargetIndexTableList[i].tableOID = sNewTargetIndexTableList[i].tableInfo->tableOID;

            if ( i < ( sTargetIndexTableCount - 1 ) )
            {
                sNewTargetIndexTableList[i].next  = & sNewTargetIndexTableList[i + 1];
            }
            else
            {
                sNewTargetIndexTableList[i].next = NULL;
            }
        }

        IDE_TEST_RAISE( i != sTargetIndexTableCount, ERR_META_CRASH );

        for ( sIndexTableList = aSourceIndexTableList, i = 0;
              sIndexTableList != NULL;
              sIndexTableList = sIndexTableList->next, i++ )
        {
            IDE_TEST( qcm::makeAndSetQcmTableInfo( QC_SMI_STMT( aStatement ),
                                                   sIndexTableList->tableID,
                                                   sIndexTableList->tableOID )
                      != IDE_SUCCESS );

            IDE_TEST( qcm::getTableInfoByID( aStatement,
                                             sIndexTableList->tableID,
                                             & sNewSourceIndexTableList[i].tableInfo,
                                             & sNewSourceIndexTableList[i].tableSCN,
                                             & sNewSourceIndexTableList[i].tableHandle )
                      != IDE_SUCCESS );

            sNewSourceIndexTableList[i].tableID  = sNewSourceIndexTableList[i].tableInfo->tableID;
            sNewSourceIndexTableList[i].tableOID = sNewSourceIndexTableList[i].tableInfo->tableOID;

            if ( i < ( sSourceIndexTableCount - 1 ) )
            {
                sNewSourceIndexTableList[i].next  = & sNewSourceIndexTableList[i + 1];
            }
            else
            {
                sNewSourceIndexTableList[i].next = NULL;
            }
        }

        IDE_TEST_RAISE( i != sSourceIndexTableCount, ERR_META_CRASH );
    }
    else
    {
        /* Nothing to do */
    }

    *aNewTargetIndexTableList = sNewTargetIndexTableList;
    *aNewSourceIndexTableList = sNewSourceIndexTableList;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_META_CRASH )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QCM_META_CRASH ) );
    }
    IDE_EXCEPTION_END;

    for ( sIndexTableList = sNewTargetIndexTableList;
          sIndexTableList != NULL;
          sIndexTableList = sIndexTableList->next )
    {
        (void)qcm::destroyQcmTableInfo( sIndexTableList->tableInfo );
    }

    for ( sIndexTableList = sNewSourceIndexTableList;
          sIndexTableList != NULL;
          sIndexTableList = sIndexTableList->next )
    {
        (void)qcm::destroyQcmTableInfo( sIndexTableList->tableInfo );
    }

    for ( sIndexTableList = aTargetIndexTableList;
          sIndexTableList != NULL;
          sIndexTableList = sIndexTableList->next )
    {
        smiSetTableTempInfo( sIndexTableList->tableHandle,
                             (void *) sIndexTableList->tableInfo );
    }

    for ( sIndexTableList = aSourceIndexTableList;
          sIndexTableList != NULL;
          sIndexTableList = sIndexTableList->next )
    {
        smiSetTableTempInfo( sIndexTableList->tableHandle,
                             (void *) sIndexTableList->tableInfo );
    }

    return IDE_FAILURE;
}

IDE_RC qdbCopySwap::renameIndicesOnSM( qcStatement      * aStatement,
                                       qcmTableInfo     * aTargetTableInfo,
                                       qcmTableInfo     * aSourceTableInfo,
                                       qcNamePosition     aNamesPrefix,
                                       qdIndexTableList * aTargetIndexTableList,
                                       qdIndexTableList * aSourceIndexTableList )
{
    SChar              sIndexName[QC_MAX_OBJECT_NAME_LEN + 1];
    SChar              sIndexTableName[QC_MAX_OBJECT_NAME_LEN + 1];
    SChar              sKeyIndexName[QC_MAX_OBJECT_NAME_LEN + 1];
    SChar              sRidIndexName[QC_MAX_OBJECT_NAME_LEN + 1];

    qcmIndex         * sIndex                   = NULL;
    qcmIndex         * sIndexTableIndex[2]      = { NULL, NULL };
    qdIndexTableList * sIndexTableList          = NULL;
    UInt               i                        = 0;

    qcNamePosition     sEmptyIndexNamePos;

    SET_EMPTY_POSITION( sEmptyIndexNamePos );

    for ( i = 0; i < aSourceTableInfo->indexCount; i++ )
    {
        sIndex = & aSourceTableInfo->indices[i];

        IDE_TEST_RAISE( ( idlOS::strlen( sIndex->name ) + aNamesPrefix.size ) >
                        QC_MAX_OBJECT_NAME_LEN,
                        ERR_NAMES_PREFIX_IS_TOO_LONG );

        QC_STR_COPY( sIndexName, aNamesPrefix );
        idlOS::strncat( sIndexName, sIndex->name, QC_MAX_OBJECT_NAME_LEN );

        IDE_TEST( smiTable::alterIndexName( QC_STATISTICS( aStatement ),
                                            QC_SMI_STMT( aStatement ),
                                            aSourceTableInfo->tableHandle,
                                            sIndex->indexHandle,
                                            sIndexName )
                  != IDE_SUCCESS );

        /* Index Table의 Index Name을 변경한다. (SM) */
        if ( sIndex->indexTableID != 0 )
        {
            /* Non-Partitioned Index는 INDEX_NAME으로 Index Table Name, Key Index Name, RID Index Name을 결정한다. */
            IDE_TEST( qdx::makeIndexTableName( aStatement,
                                               sEmptyIndexNamePos,
                                               sIndexName,
                                               sIndexTableName,
                                               sKeyIndexName,
                                               sRidIndexName )
                      != IDE_SUCCESS );

            for ( sIndexTableList = aSourceIndexTableList;
                  sIndexTableList != NULL;
                  sIndexTableList = sIndexTableList->next )
            {
                if ( sIndexTableList->tableID == sIndex->indexTableID )
                {
                    IDE_TEST( qdx::getIndexTableIndices( sIndexTableList->tableInfo,
                                                         sIndexTableIndex )
                              != IDE_SUCCESS );

                    IDE_TEST( smiTable::alterIndexName( QC_STATISTICS( aStatement ),
                                                        QC_SMI_STMT( aStatement ),
                                                        sIndexTableList->tableHandle,
                                                        sIndexTableIndex[0]->indexHandle,
                                                        sKeyIndexName )
                              != IDE_SUCCESS );

                    IDE_TEST( smiTable::alterIndexName( QC_STATISTICS( aStatement ),
                                                        QC_SMI_STMT( aStatement ),
                                                        sIndexTableList->tableHandle,
                                                        sIndexTableIndex[1]->indexHandle,
                                                        sRidIndexName )
                              != IDE_SUCCESS );

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
            /* Nothing to do */
        }
    }

    for ( i = 0; i < aTargetTableInfo->indexCount; i++ )
    {
        sIndex = & aTargetTableInfo->indices[i];

        IDE_TEST_RAISE( idlOS::strlen( sIndex->name ) <= (UInt)aNamesPrefix.size,
                        ERR_NAMES_PREFIX_IS_TOO_LONG );

        IDE_TEST_RAISE( idlOS::strMatch( sIndex->name,
                                         aNamesPrefix.size,
                                         aNamesPrefix.stmtText + aNamesPrefix.offset,
                                         aNamesPrefix.size ) != 0,
                        ERR_NAMES_PREFIX_IS_TOO_LONG );

        idlOS::strncpy( sIndexName, sIndex->name + aNamesPrefix.size, QC_MAX_OBJECT_NAME_LEN + 1 );

        IDE_TEST( smiTable::alterIndexName( QC_STATISTICS( aStatement ),
                                            QC_SMI_STMT( aStatement ),
                                            aTargetTableInfo->tableHandle,
                                            sIndex->indexHandle,
                                            sIndexName )
                  != IDE_SUCCESS );

        /* Index Table의 Index Name을 변경한다. (SM) */
        if ( sIndex->indexTableID != 0 )
        {
            /* Non-Partitioned Index는 INDEX_NAME으로 Index Table Name, Key Index Name, RID Index Name을 결정한다. */
            IDE_TEST( qdx::makeIndexTableName( aStatement,
                                               sEmptyIndexNamePos,
                                               sIndexName,
                                               sIndexTableName,
                                               sKeyIndexName,
                                               sRidIndexName )
                      != IDE_SUCCESS );

            for ( sIndexTableList = aTargetIndexTableList;
                  sIndexTableList != NULL;
                  sIndexTableList = sIndexTableList->next )
            {
                if ( sIndexTableList->tableID == sIndex->indexTableID )
                {
                    IDE_TEST( qdx::getIndexTableIndices( sIndexTableList->tableInfo,
                                                         sIndexTableIndex )
                              != IDE_SUCCESS );

                    IDE_TEST( smiTable::alterIndexName( QC_STATISTICS( aStatement ),
                                                        QC_SMI_STMT( aStatement ),
                                                        sIndexTableList->tableHandle,
                                                        sIndexTableIndex[0]->indexHandle,
                                                        sKeyIndexName )
                              != IDE_SUCCESS );

                    IDE_TEST( smiTable::alterIndexName( QC_STATISTICS( aStatement ),
                                                        QC_SMI_STMT( aStatement ),
                                                        sIndexTableList->tableHandle,
                                                        sIndexTableIndex[1]->indexHandle,
                                                        sRidIndexName )
                              != IDE_SUCCESS );

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
            /* Nothing to do */
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NAMES_PREFIX_IS_TOO_LONG )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDB_USING_TARGET_NAMES_PREFIX_IS_TOO_LONG ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdbCopySwap::checkConstraintNameAfterRename( qcStatement    * aStatement,
                                                    UInt             aUserID,
                                                    SChar          * aConstraintName,
                                                    qcNamePosition   aNamesPrefix,
                                                    idBool           aAddPrefix )
{
/***********************************************************************
 *
 * Description :
 *      Prefix를 붙이거나 제거한 CONSTRAINT_NAME이 유일한지 확인한다.
 *      Meta Table에서 CONSTRAINT_NAME을 변경한 이후에 호출해야 한다.
 *
 * Implementation :
 *
 ***********************************************************************/

    SChar           sConstraintsName[QC_MAX_OBJECT_NAME_LEN + 1];
    SChar         * sSqlStr         = NULL;
    idBool          sRecordExist    = ID_FALSE;
    mtdBigintType   sResultCount    = 0;

    IDU_FIT_POINT( "qdbCopySwap::checkConstraintNameAfterRename::STRUCT_ALLOC_WITH_SIZE::sSqlStr",
                   idERR_ABORT_InsufficientMemory );

    IDE_TEST( STRUCT_ALLOC_WITH_SIZE( QC_QMX_MEM( aStatement ),
                                      SChar,
                                      QD_MAX_SQL_LENGTH,
                                      & sSqlStr )
              != IDE_SUCCESS );

    if ( aAddPrefix == ID_TRUE )
    {
        IDE_TEST_RAISE( ( idlOS::strlen( aConstraintName ) + aNamesPrefix.size ) > QC_MAX_OBJECT_NAME_LEN,
                        ERR_NAMES_PREFIX_IS_TOO_LONG );

        // CONSTRAINT_NAME에 사용자가 지정한 Prefix를 붙여서 CONSTRAINT_NAME을 생성한다.
        QC_STR_COPY( sConstraintsName, aNamesPrefix );
        idlOS::strncat( sConstraintsName, aConstraintName, QC_MAX_OBJECT_NAME_LEN );
    }
    else
    {
        IDE_TEST_RAISE( idlOS::strlen( aConstraintName ) <= (UInt)aNamesPrefix.size,
                        ERR_NAMES_PREFIX_IS_TOO_LONG );

        IDE_TEST_RAISE( idlOS::strMatch( aConstraintName,
                                         aNamesPrefix.size,
                                         aNamesPrefix.stmtText + aNamesPrefix.offset,
                                         aNamesPrefix.size ) != 0,
                        ERR_NAMES_PREFIX_IS_TOO_LONG );

        // CONSTRAINT_NAME에서 사용자가 지정한 Prefix를 제거한 CONSTRAINT_NAME을 생성한다.
        idlOS::strncpy( sConstraintsName, aConstraintName + aNamesPrefix.size, QC_MAX_OBJECT_NAME_LEN + 1 );
    }

    /* 변경한 CONSTRAINT_NAME이 하나만 존재하는지 확인한다. */
    idlOS::snprintf( sSqlStr,
                     QD_MAX_SQL_LENGTH,
                     "SELECT COUNT(*) "
                     "  FROM SYS_CONSTRAINTS_ "
                     " WHERE USER_ID = INTEGER'%"ID_INT32_FMT"' AND "
                     "       CONSTRAINT_NAME = VARCHAR'%s' ",
                     aUserID,
                     sConstraintsName );

    IDE_TEST( qcg::runSelectOneRowforDDL( QC_SMI_STMT( aStatement ),
                                          sSqlStr,
                                          & sResultCount,
                                          & sRecordExist,
                                          ID_FALSE )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( ( sRecordExist == ID_FALSE ) ||
                    ( (ULong)sResultCount != ID_ULONG(1) ),
                    ERR_META_CRASH );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NAMES_PREFIX_IS_TOO_LONG )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDB_USING_TARGET_NAMES_PREFIX_IS_TOO_LONG ) );
    }
    IDE_EXCEPTION( ERR_META_CRASH )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QCM_META_CRASH ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdbCopySwap::renameConstraintsMeta( qcStatement    * aStatement,
                                           qcmTableInfo   * aTargetTableInfo,
                                           qcmTableInfo   * aSourceTableInfo,
                                           qcNamePosition   aNamesPrefix )
{
/***********************************************************************
 *
 * Description :
 *      Prefix를 지정한 경우, Source의 CONSTRAINT_NAME에 Prefix를 붙이고,
 *      Target의 CONSTRAINT_NAME에서 Prefix를 제거한다. (Meta Table)
 *          - SYS_CONSTRAINTS_
 *              - CONSTRAINT_NAME을 변경한다.
 *                  - 변경한 CONSTRAINT_NAME이 Unique한지 확인해야 한다.
 *                      - CONSTRAINT_NAME은 Unique Index의 Column이 아니다.
 *          - SYS_CONSTRAINT_COLUMNS_, SYS_CONSTRAINT_RELATED_
 *              - 변경 사항 없음
 *
 * Implementation :
 *
 ***********************************************************************/

    SChar           sNamesPrefix[QC_MAX_OBJECT_NAME_LEN + 1];
    SChar         * sSqlStr             = NULL;
    vSLong          sRowCnt             = ID_vLONG(0);
    idBool          sRecordExist        = ID_FALSE;
    mtdBigintType   sResultCount        = 0;
    ULong           sConstraintCount    = 0;
    UInt            i                   = 0;

    if ( QC_IS_NULL_NAME( aNamesPrefix ) != ID_TRUE )
    {
        QC_STR_COPY( sNamesPrefix, aNamesPrefix );

        IDU_FIT_POINT( "qdbCopySwap::renameConstraintsMeta::STRUCT_ALLOC_WITH_SIZE::sSqlStr",
                       idERR_ABORT_InsufficientMemory );

        IDE_TEST( STRUCT_ALLOC_WITH_SIZE( QC_QMX_MEM( aStatement ),
                                          SChar,
                                          QD_MAX_SQL_LENGTH,
                                          & sSqlStr )
                  != IDE_SUCCESS );

        // Constraint Count를 얻는다.
        idlOS::snprintf( sSqlStr,
                         QD_MAX_SQL_LENGTH,
                         "SELECT COUNT(*) "
                         "  FROM SYS_CONSTRAINTS_ "
                         " WHERE TABLE_ID IN ( INTEGER'%"ID_INT32_FMT"', "
                         "                     INTEGER'%"ID_INT32_FMT"' ) ",
                         aSourceTableInfo->tableID,
                         aTargetTableInfo->tableID );

        IDE_TEST( qcg::runSelectOneRowforDDL( QC_SMI_STMT( aStatement ),
                                              sSqlStr,
                                              & sResultCount,
                                              & sRecordExist,
                                              ID_FALSE )
                  != IDE_SUCCESS );

        IDE_TEST_RAISE( sRecordExist == ID_FALSE, ERR_META_CRASH );
        sConstraintCount = (ULong)sResultCount;

        // Source의 CONSTRAINT_NAME에 Prefix를 붙이고, Target의 CONSTRAINT_NAME에서 Prefix를 제거한다.
        idlOS::snprintf( sSqlStr,
                         QD_MAX_SQL_LENGTH,
                         "UPDATE SYS_CONSTRAINTS_ "
                         "   SET CONSTRAINT_NAME = CASE2( TABLE_ID = INTEGER'%"ID_INT32_FMT"', "
                         "                                VARCHAR'%s' || CONSTRAINT_NAME, "
                         "                                SUBSTR( CONSTRAINT_NAME, INTEGER'%"ID_INT32_FMT"' ) ) "
                         " WHERE TABLE_ID IN ( INTEGER'%"ID_INT32_FMT"', "
                         "                     INTEGER'%"ID_INT32_FMT"' ) ",
                         aSourceTableInfo->tableID,
                         sNamesPrefix,
                         aNamesPrefix.size + 1,
                         aSourceTableInfo->tableID,
                         aTargetTableInfo->tableID );

        IDE_TEST( qcg::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                     sSqlStr,
                                     & sRowCnt )
                  != IDE_SUCCESS );

        IDE_TEST_RAISE( (ULong)sRowCnt != sConstraintCount, ERR_META_CRASH );

        // Primary Key, Unique Key, Local Unique Key
        for ( i = 0; i < aSourceTableInfo->uniqueKeyCount; i++ )
        {
            IDE_TEST( checkConstraintNameAfterRename( aStatement,
                                                      aSourceTableInfo->tableOwnerID,
                                                      aSourceTableInfo->uniqueKeys[i].name,
                                                      aNamesPrefix,
                                                      ID_TRUE )
                      != IDE_SUCCESS );
        }

        for ( i = 0; i < aSourceTableInfo->foreignKeyCount; i++ )
        {
            IDE_TEST( checkConstraintNameAfterRename( aStatement,
                                                      aSourceTableInfo->tableOwnerID,
                                                      aSourceTableInfo->foreignKeys[i].name,
                                                      aNamesPrefix,
                                                      ID_TRUE )
                      != IDE_SUCCESS );
        }

        for ( i = 0; i < aSourceTableInfo->notNullCount; i++ )
        {
            IDE_TEST( checkConstraintNameAfterRename( aStatement,
                                                      aSourceTableInfo->tableOwnerID,
                                                      aSourceTableInfo->notNulls[i].name,
                                                      aNamesPrefix,
                                                      ID_TRUE )
                      != IDE_SUCCESS );
        }

        for ( i = 0; i < aSourceTableInfo->checkCount; i++ )
        {
            IDE_TEST( checkConstraintNameAfterRename( aStatement,
                                                      aSourceTableInfo->tableOwnerID,
                                                      aSourceTableInfo->checks[i].name,
                                                      aNamesPrefix,
                                                      ID_TRUE )
                      != IDE_SUCCESS );
        }

        if ( aSourceTableInfo->timestamp != NULL )
        {
            IDE_TEST( checkConstraintNameAfterRename( aStatement,
                                                      aSourceTableInfo->tableOwnerID,
                                                      aSourceTableInfo->timestamp->name,
                                                      aNamesPrefix,
                                                      ID_TRUE )
                      != IDE_SUCCESS );
        }
        else
        {
            /* Nothing to do */
        }

        // Primary Key, Unique Key, Local Unique Key
        for ( i = 0; i < aTargetTableInfo->uniqueKeyCount; i++ )
        {
            IDE_TEST( checkConstraintNameAfterRename( aStatement,
                                                      aTargetTableInfo->tableOwnerID,
                                                      aTargetTableInfo->uniqueKeys[i].name,
                                                      aNamesPrefix,
                                                      ID_FALSE )
                      != IDE_SUCCESS );
        }

        for ( i = 0; i < aTargetTableInfo->foreignKeyCount; i++ )
        {
            IDE_TEST( checkConstraintNameAfterRename( aStatement,
                                                      aTargetTableInfo->tableOwnerID,
                                                      aTargetTableInfo->foreignKeys[i].name,
                                                      aNamesPrefix,
                                                      ID_FALSE )
                      != IDE_SUCCESS );
        }

        for ( i = 0; i < aTargetTableInfo->notNullCount; i++ )
        {
            IDE_TEST( checkConstraintNameAfterRename( aStatement,
                                                      aTargetTableInfo->tableOwnerID,
                                                      aTargetTableInfo->notNulls[i].name,
                                                      aNamesPrefix,
                                                      ID_FALSE )
                      != IDE_SUCCESS );
        }

        for ( i = 0; i < aTargetTableInfo->checkCount; i++ )
        {
            IDE_TEST( checkConstraintNameAfterRename( aStatement,
                                                      aTargetTableInfo->tableOwnerID,
                                                      aTargetTableInfo->checks[i].name,
                                                      aNamesPrefix,
                                                      ID_FALSE )
                      != IDE_SUCCESS );
        }

        if ( aTargetTableInfo->timestamp != NULL )
        {
            IDE_TEST( checkConstraintNameAfterRename( aStatement,
                                                      aTargetTableInfo->tableOwnerID,
                                                      aTargetTableInfo->timestamp->name,
                                                      aNamesPrefix,
                                                      ID_FALSE )
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

    IDE_EXCEPTION( ERR_META_CRASH )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QCM_META_CRASH ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdbCopySwap::renameCommentsMeta( qcStatement    * aStatement,
                                        qcmTableInfo   * aTargetTableInfo,
                                        qcmTableInfo   * aSourceTableInfo,
                                        qcNamePosition   aNamesPrefix )
{
/***********************************************************************
 *
 * Description :
 *      Comment를 교환한다. (Meta Table)
 *          - SYS_COMMENTS_
 *              - TABLE_NAME을 교환한다.
 *              - Hidden Column이면 Function-based Index의 Column이므로,
 *                사용자가 Prefix를 지정한 경우, Hidden Column Name을 변경한다.
 *                  - Source의 Hidden Column Name에 Prefix를 붙이고, Target의 Hidden Column Name에서 Prefix를 제거한다.
 *                      - Hidden Column Name = Index Name + $ + IDX + Number
 *
 * Implementation :
 *
 ***********************************************************************/

    SChar    sNamesPrefix[QC_MAX_OBJECT_NAME_LEN + 1];
    SChar  * sSqlStr = NULL;
    vSLong   sRowCnt = ID_vLONG(0);

    IDU_FIT_POINT( "qdbCopySwap::renameCommentsMeta::STRUCT_ALLOC_WITH_SIZE::sSqlStr",
                   idERR_ABORT_InsufficientMemory );

    IDE_TEST( STRUCT_ALLOC_WITH_SIZE( QC_QMX_MEM( aStatement ),
                                      SChar,
                                      QD_MAX_SQL_LENGTH,
                                      & sSqlStr )
              != IDE_SUCCESS );

    if ( QC_IS_NULL_NAME( aNamesPrefix ) != ID_TRUE )
    {
        QC_STR_COPY( sNamesPrefix, aNamesPrefix );

        /* Source의 Hidden Column Name에 Prefix를 붙인다. */
        idlOS::snprintf( sSqlStr,
                         QD_MAX_SQL_LENGTH,
                         "UPDATE SYS_COMMENTS_ "
                         "   SET COLUMN_NAME = VARCHAR'%s' || COLUMN_NAME "
                         " WHERE USER_NAME = VARCHAR'%s' AND "
                         "       TABLE_NAME = VARCHAR'%s' AND "
                         "       COLUMN_NAME IN ( SELECT SUBSTR( COLUMN_NAME, INTEGER'%"ID_INT32_FMT"' ) "
                         "                          FROM SYS_COLUMNS_ "
                         "                         WHERE TABLE_ID = INTEGER'%"ID_INT32_FMT"' AND "
                         "                               IS_HIDDEN = CHAR'T' ) ",
                         sNamesPrefix,
                         aSourceTableInfo->tableOwnerName,
                         aSourceTableInfo->name,
                         aNamesPrefix.size + 1,
                         aSourceTableInfo->tableID );

        IDE_TEST( qcg::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                     sSqlStr,
                                     & sRowCnt )
                  != IDE_SUCCESS );

        /* Target의 Hidden Column Name에서 Prefix를 제거한다. */
        idlOS::snprintf( sSqlStr,
                         QD_MAX_SQL_LENGTH,
                         "UPDATE SYS_COMMENTS_ "
                         "   SET COLUMN_NAME = SUBSTR( COLUMN_NAME, INTEGER'%"ID_INT32_FMT"' ) "
                         " WHERE USER_NAME = VARCHAR'%s' AND "
                         "       TABLE_NAME = VARCHAR'%s' AND "
                         "       COLUMN_NAME IN ( SELECT VARCHAR'%s' || COLUMN_NAME "
                         "                          FROM SYS_COLUMNS_ "
                         "                         WHERE TABLE_ID = INTEGER'%"ID_INT32_FMT"' AND "
                         "                               IS_HIDDEN = CHAR'T' ) ",
                         aNamesPrefix.size + 1,
                         aTargetTableInfo->tableOwnerName,
                         aTargetTableInfo->name,
                         sNamesPrefix,
                         aTargetTableInfo->tableID );

        IDE_TEST( qcg::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                     sSqlStr,
                                     & sRowCnt )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing to do */
    }

    /* Comment의 Table Name을 교환한다. */
    idlOS::snprintf( sSqlStr,
                     QD_MAX_SQL_LENGTH,
                     "UPDATE SYS_COMMENTS_ "
                     "   SET TABLE_NAME = CASE2( TABLE_NAME = VARCHAR'%s', "
                     "                           VARCHAR'%s', "
                     "                           VARCHAR'%s' ) "
                     " WHERE USER_NAME = VARCHAR'%s' AND "
                     "       TABLE_NAME IN ( VARCHAR'%s', VARCHAR'%s' ) ",
                     aTargetTableInfo->name,
                     aSourceTableInfo->name,
                     aTargetTableInfo->name,
                     aTargetTableInfo->tableOwnerName,
                     aTargetTableInfo->name,
                     aSourceTableInfo->name );

    IDE_TEST( qcg::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                 sSqlStr,
                                 & sRowCnt )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdbCopySwap::swapTablePartitionsMetaForReplication( qcStatement  * aStatement,
                                                           qcmTableInfo * aTargetTableInfo,
                                                           qcmTableInfo * aSourceTableInfo )
{
/***********************************************************************
 *
 * Description :
 *      한쪽이라도 Partitioned Table이고 Replication 대상이면, Partition의 Replication 정보를 교환한다. (Meta Table)
 *          - SYS_TABLE_PARTITIONS_
 *              - PARTITION_NAME으로 Matching Partition을 선택한다.
 *              - Matching Partition의 REPLICATION_COUNT, REPLICATION_RECOVERY_COUNT를 교환하고, LAST_DDL_TIME을 갱신한다.
 *                  - REPLICATION_COUNT > 0 이면, Partition이 Replication 대상이다.
 *              - PARTITION_USABLE을 교환한다. (TASK-7307)
 *          - Partition의 다른 정보는 변경 사항이 없다.
 *              - SYS_INDEX_PARTITIONS_
 *                  - INDEX_PARTITION_NAME은 Partitioned Table의 Index 내에서만 Unique하면 되므로, Prefix가 필요하지 않다.
 *              - SYS_PART_TABLES_, SYS_PART_LOBS_, SYS_PART_KEY_COLUMNS_, SYS_PART_INDICES_
 *
 * Implementation :
 *
 ***********************************************************************/

    SChar  * sSqlStr = NULL;
    vSLong   sRowCnt = ID_vLONG(0);

    if ( ( ( aTargetTableInfo->tablePartitionType == QCM_PARTITIONED_TABLE ) &&
           ( ( aTargetTableInfo->replicationCount > 0 ) ||
             ( QCM_TABLE_IS_FOR_SHARD(aTargetTableInfo->mShardFlag) == ID_TRUE ) ) ) ||
         ( ( aSourceTableInfo->tablePartitionType == QCM_PARTITIONED_TABLE ) &&
           ( ( aSourceTableInfo->replicationCount > 0 ) ||
             ( QCM_TABLE_IS_FOR_SHARD(aSourceTableInfo->mShardFlag) == ID_TRUE ) ) ) )
    {
        IDU_FIT_POINT( "qdbCopySwap::swapTablePartitionsMetaForReplication::STRUCT_ALLOC_WITH_SIZE::sSqlStr",
                       idERR_ABORT_InsufficientMemory );

        IDE_TEST( STRUCT_ALLOC_WITH_SIZE( QC_QMX_MEM( aStatement ),
                                          SChar,
                                          QD_MAX_SQL_LENGTH,
                                          & sSqlStr )
                  != IDE_SUCCESS );

        idlOS::snprintf( sSqlStr,
                         QD_MAX_SQL_LENGTH,
                         "UPDATE SYS_TABLE_PARTITIONS_ A "
                         "   SET ( PARTITION_USABLE, REPLICATION_COUNT, REPLICATION_RECOVERY_COUNT ) = "
                         "       ( SELECT PARTITION_USABLE, REPLICATION_COUNT, REPLICATION_RECOVERY_COUNT "
                         "           FROM SYS_TABLE_PARTITIONS_ C "
                         "          WHERE C.TABLE_ID IN ( INTEGER'%"ID_INT32_FMT"', "
                         "                                INTEGER'%"ID_INT32_FMT"' ) AND "
                         "                C.TABLE_ID != A.TABLE_ID AND "
                         "                C.PARTITION_NAME = A.PARTITION_NAME ), "
                         "       LAST_DDL_TIME = SYSDATE "
                         " WHERE TABLE_ID IN ( INTEGER'%"ID_INT32_FMT"', INTEGER'%"ID_INT32_FMT"' ) AND "
                         "       PARTITION_NAME IN ( SELECT PARTITION_NAME "
                         "                             FROM SYS_TABLE_PARTITIONS_ B "
                         "                            WHERE B.TABLE_ID IN ( INTEGER'%"ID_INT32_FMT"', "
                         "                                                  INTEGER'%"ID_INT32_FMT"' ) AND "
                         "                                  B.TABLE_ID != A.TABLE_ID ) ",
                         aTargetTableInfo->tableID,
                         aSourceTableInfo->tableID,
                         aTargetTableInfo->tableID,
                         aSourceTableInfo->tableID,
                         aTargetTableInfo->tableID,
                         aSourceTableInfo->tableID );

        IDE_TEST( qcg::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                     sSqlStr,
                                     & sRowCnt )
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

IDE_RC qdbCopySwap::updateSysConstraintsMetaForReferencedIndex( qcStatement     * aStatement,
                                                                qcmTableInfo    * aTargetTableInfo,
                                                                qcmTableInfo    * aSourceTableInfo,
                                                                qcmRefChildInfo * aTargetRefChildInfoList,
                                                                qcmRefChildInfo * aSourceRefChildInfoList )
{
/***********************************************************************
 *
 * Description :
 *      Referenced Index를 변경한다. (Meta Table)
 *          - SYS_CONSTRAINTS_
 *              - REFERENCED_INDEX_ID가 가리키는 Index의 Column Name으로 구성된 Index를 Peer에서 찾는다. (Validation과 동일)
 *              - REFERENCED_TABLE_ID와 REFERENCED_INDEX_ID를 Peer의 Table ID와 Index ID로 변경한다.
 *
 * Implementation :
 *
 ***********************************************************************/

    qcmRefChildInfo * sRefChildInfo = NULL;
    qcmIndex        * sPeerIndex    = NULL;
    SChar           * sSqlStr       = NULL;
    vSLong            sRowCnt       = ID_vLONG(0);

    IDU_FIT_POINT( "qdbCopySwap::updateSysConstraintsMetaForReferencedIndex::STRUCT_ALLOC_WITH_SIZE::sSqlStr",
                   idERR_ABORT_InsufficientMemory );

    IDE_TEST( STRUCT_ALLOC_WITH_SIZE( QC_QMX_MEM( aStatement ),
                                      SChar,
                                      QD_MAX_SQL_LENGTH,
                                      & sSqlStr )
              != IDE_SUCCESS );

    for ( sRefChildInfo = aTargetRefChildInfoList;
          sRefChildInfo != NULL;
          sRefChildInfo = sRefChildInfo->next )
    {
        findPeerIndex( aTargetTableInfo,
                       sRefChildInfo->parentIndex,
                       aSourceTableInfo,
                       & sPeerIndex );

        IDE_TEST_RAISE( sPeerIndex == NULL, ERR_PEER_INDEX_NOT_EXISTS );

        idlOS::snprintf( sSqlStr,
                         QD_MAX_SQL_LENGTH,
                         "UPDATE SYS_CONSTRAINTS_ "
                         "   SET REFERENCED_TABLE_ID = INTEGER'%"ID_INT32_FMT"', "
                         "       REFERENCED_INDEX_ID = INTEGER'%"ID_INT32_FMT"' "
                         " WHERE CONSTRAINT_ID = INTEGER'%"ID_INT32_FMT"' ",
                         aSourceTableInfo->tableID,
                         sPeerIndex->indexId,
                         sRefChildInfo->foreignKey->constraintID );

        IDE_TEST( qcg::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                     sSqlStr,
                                     & sRowCnt )
                  != IDE_SUCCESS );

        IDE_TEST_RAISE( (UInt)sRowCnt != 1, ERR_META_CRASH );
    }

    for ( sRefChildInfo = aSourceRefChildInfoList;
          sRefChildInfo != NULL;
          sRefChildInfo = sRefChildInfo->next )
    {
        findPeerIndex( aSourceTableInfo,
                       sRefChildInfo->parentIndex,
                       aTargetTableInfo,
                       & sPeerIndex );

        IDE_TEST_RAISE( sPeerIndex == NULL, ERR_PEER_INDEX_NOT_EXISTS );

        idlOS::snprintf( sSqlStr,
                         QD_MAX_SQL_LENGTH,
                         "UPDATE SYS_CONSTRAINTS_ "
                         "   SET REFERENCED_TABLE_ID = INTEGER'%"ID_INT32_FMT"', "
                         "       REFERENCED_INDEX_ID = INTEGER'%"ID_INT32_FMT"' "
                         " WHERE CONSTRAINT_ID = INTEGER'%"ID_INT32_FMT"' ",
                         aTargetTableInfo->tableID,
                         sPeerIndex->indexId,
                         sRefChildInfo->foreignKey->constraintID );

        IDE_TEST( qcg::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                     sSqlStr,
                                     & sRowCnt )
                  != IDE_SUCCESS );

        IDE_TEST_RAISE( (UInt)sRowCnt != 1, ERR_META_CRASH );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_PEER_INDEX_NOT_EXISTS )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDN_REFERENCED_CONSTRAINT_NOT_FOUND ) );
    }
    IDE_EXCEPTION( ERR_META_CRASH )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QCM_META_CRASH ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdbCopySwap::swapGrantObjectMeta( qcStatement * aStatement,
                                         UInt          aTargetTableID,
                                         UInt          aSourceTableID )
{
/***********************************************************************
 *
 * Description :
 *      Object Privilege를 교환한다. (Meta Table)
 *          - SYS_GRANT_OBJECT_
 *              - OBJ_ID = Table ID, OBJ_TYPE = 'T' 이면, OBJ_ID만 교환한다.
 *
 * Implementation :
 *
 ***********************************************************************/

    SChar  * sSqlStr = NULL;
    vSLong   sRowCnt = ID_vLONG(0);

    IDU_FIT_POINT( "qdbCopySwap::swapGrantObjectMeta::STRUCT_ALLOC_WITH_SIZE::sSqlStr",
                   idERR_ABORT_InsufficientMemory );

    IDE_TEST( STRUCT_ALLOC_WITH_SIZE( QC_QMX_MEM( aStatement ),
                                      SChar,
                                      QD_MAX_SQL_LENGTH,
                                      & sSqlStr )
              != IDE_SUCCESS );

    idlOS::snprintf( sSqlStr,
                     QD_MAX_SQL_LENGTH,
                     "UPDATE SYS_GRANT_OBJECT_ "
                     "   SET OBJ_ID = CASE2( OBJ_ID = BIGINT'%"ID_INT64_FMT"', "
                     "                       BIGINT'%"ID_INT64_FMT"', "
                     "                       BIGINT'%"ID_INT64_FMT"' ) "
                     " WHERE OBJ_ID IN ( BIGINT'%"ID_INT64_FMT"', BIGINT'%"ID_INT64_FMT"' ) AND "
                     "       OBJ_TYPE = CHAR'T' ",
                     (SLong)aTargetTableID,
                     (SLong)aSourceTableID,
                     (SLong)aTargetTableID,
                     (SLong)aTargetTableID,
                     (SLong)aSourceTableID );

    IDE_TEST( qcg::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                 sSqlStr,
                                 & sRowCnt )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdbCopySwap::insertReplItemMetaToReplaceHistory( qcStatement  * aStatement,
                                                        qcmTableInfo * aTargetTableInfo,
                                                        qcmTableInfo * aSourceTableInfo )
{
/***********************************************************************
 *
 * Description :
 *      Replication Meta Table을 추가한다. (Meta Table)
 *          - SYS_REPL_ITEM_REPLACE_HISTORY_
 *              - 
 *                  - 
 *              - 
 *              - 
 *
 * Implementation :
 *
 ***********************************************************************/

    SChar  * sSqlStr = NULL;
    vSLong   sDummyCnt = ID_vLONG(0);

    if ( ( aTargetTableInfo->replicationCount > 0 ) || ( aSourceTableInfo->replicationCount > 0 ) )
    {
        IDU_FIT_POINT( "qdbCopySwap::insertReplItemMetaToReplaceHistory::STRUCT_ALLOC_WITH_SIZE::sSqlStr",
                       idERR_ABORT_InsufficientMemory );

        IDE_TEST( STRUCT_ALLOC_WITH_SIZE( QC_QMX_MEM( aStatement ),
                                          SChar,
                                          QD_MAX_SQL_LENGTH,
                                          & sSqlStr )
                  != IDE_SUCCESS );

        if ( aTargetTableInfo->tablePartitionType == QCM_NONE_PARTITIONED_TABLE )
        {
            idlOS::snprintf( sSqlStr,
                             QD_MAX_SQL_LENGTH,
                             "DELETE FROM SYS_REPL_ITEM_REPLACE_HISTORY_ "
                             "       WHERE OLD_OID IN "
                             "                       ( BIGINT'%"ID_INT64_FMT"', BIGINT'%"ID_INT64_FMT"' ) "
                             "         OR  NEW_OID IN "
                             "                       ( BIGINT'%"ID_INT64_FMT"', BIGINT'%"ID_INT64_FMT"' ) ",
                             QCM_OID_TO_BIGINT( aTargetTableInfo->tableOID ),
                             QCM_OID_TO_BIGINT( aTargetTableInfo->tableOID ),
                             QCM_OID_TO_BIGINT( aSourceTableInfo->tableOID ),
                             QCM_OID_TO_BIGINT( aTargetTableInfo->tableOID ) );

            IDE_TEST( qcg::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                         sSqlStr,
                                         & sDummyCnt )
                      != IDE_SUCCESS );

            /* PROJ-2742 1:1 Consistent mode 이중화에서 두 노드 사이에 장애 발생 시 데이터 정합성을 보장 
             * Table replace시 변경 전 Table oid를 저장한다.
             * Consistent mode만 지원하므로, 추 후 지원하는 모드가 추가되면 아래 쿼리 where 조건을 수정해야 한다. 
             *  WHERE REPL_MODE=12 (consistent mode)*/
            idlOS::snprintf( sSqlStr,
                             QD_MAX_SQL_LENGTH,
                             "INSERT INTO SYS_REPL_ITEM_REPLACE_HISTORY_ ( REPLICATION_NAME, USER_NAME, TABLE_NAME, PARTITION_NAME, OLD_OID , NEW_OID )"
                             "   SELECT REPLICATION_NAME, LOCAL_USER_NAME, LOCAL_TABLE_NAME, " 
                             "          '', TABLE_OID,  CASE2( TABLE_OID = BIGINT'%"ID_INT64_FMT"', "
                             "                                             BIGINT'%"ID_INT64_FMT"'," 
                             "                                             BIGINT'%"ID_INT64_FMT"' ) " 
                             "      FROM SYS_REPL_ITEMS_ WHERE TABLE_OID IN "
                             "                                 ( BIGINT'%"ID_INT64_FMT"', BIGINT'%"ID_INT64_FMT"' ) "
                             "AND REPLICATION_NAME IN ( SELECT REPLICATION_NAME FROM SYSTEM_.SYS_REPLICATIONS_ WHERE REPL_MODE=12 )",
                             QCM_OID_TO_BIGINT( aTargetTableInfo->tableOID ),
                             QCM_OID_TO_BIGINT( aSourceTableInfo->tableOID ),
                             QCM_OID_TO_BIGINT( aTargetTableInfo->tableOID ),
                             QCM_OID_TO_BIGINT( aSourceTableInfo->tableOID ),
                             QCM_OID_TO_BIGINT( aTargetTableInfo->tableOID ) );

            IDE_TEST( qcg::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                         sSqlStr,
                                         & sDummyCnt )
                      != IDE_SUCCESS );
        }
        else
        {            
            idlOS::snprintf( sSqlStr,
                             QD_MAX_SQL_LENGTH,
                             "DELETE FROM SYS_REPL_ITEM_REPLACE_HISTORY_ "
                             "   WHERE OLD_OID IN "
                             "                 ( SELECT PARTITION_OID FROM SYS_TABLE_PARTITIONS_ " 
                             "                    WHERE TABLE_ID IN " 
                             "                           ( BIGINT'%"ID_INT32_FMT"', "
                             "                             BIGINT'%"ID_INT32_FMT"' ) ) "
                             "      OR NEW_OID IN "
                             "                 ( SELECT PARTITION_OID FROM SYS_TABLE_PARTITIONS_ " 
                             "                     WHERE TABLE_ID IN " 
                             "                           ( BIGINT'%"ID_INT32_FMT"', "
                             "                             BIGINT'%"ID_INT32_FMT"' ) ) ",
                             aTargetTableInfo->tableID,
                             aSourceTableInfo->tableID, 
                             aTargetTableInfo->tableID,
                             aSourceTableInfo->tableID ); 

            IDE_TEST( qcg::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                         sSqlStr,
                                         & sDummyCnt )
                      != IDE_SUCCESS );

            idlOS::snprintf( sSqlStr,
                             QD_MAX_SQL_LENGTH,
                             "INSERT INTO SYS_REPL_ITEM_REPLACE_HISTORY_ ( REPLICATION_NAME, USER_NAME, TABLE_NAME, PARTITION_NAME, OLD_OID , NEW_OID )"
                             "   SELECT I.REPLICATION_NAME, I.LOCAL_USER_NAME, I.LOCAL_TABLE_NAME, " 
                             "          I.LOCAL_PARTITION_NAME, I.TABLE_OID, P.PARTITION_OID " 
                             "      FROM SYS_REPL_ITEMS_ I, SYS_TABLES_ T, SYS_TABLE_PARTITIONS_ P"
                             "      WHERE I.TABLE_OID IN "
                             "            ( SELECT PARTITION_OID FROM SYS_TABLE_PARTITIONS_  WHERE TABLE_ID IN "
                             "                                                                             ( BIGINT'%"ID_INT32_FMT"', "
                             "                                                                               BIGINT'%"ID_INT32_FMT"' ) ) "
                             "     AND I.LOCAL_PARTITION_NAME=P.PARTITION_NAME"
                             "     AND I.TABLE_OID != P.PARTITION_OID"
                             "     AND T.TABLE_ID = P.TABLE_ID"
                             "     AND T.TABLE_ID IN "
                            "                   ( BIGINT'%"ID_INT32_FMT"', "
                            "                     BIGINT'%"ID_INT32_FMT"' ) "
                             "     AND REPLICATION_NAME IN ( SELECT REPLICATION_NAME FROM SYS_REPLICATIONS_ WHERE REPL_MODE=12 )",
                             aTargetTableInfo->tableID,
                             aSourceTableInfo->tableID,
                             aTargetTableInfo->tableID,
                             aSourceTableInfo->tableID ); 

            IDE_TEST( qcg::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                         sSqlStr,
                                         & sDummyCnt )
                      != IDE_SUCCESS );
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

IDE_RC qdbCopySwap::swapReplItemsMeta( qcStatement  * aStatement,
                                       qcmTableInfo * aTargetTableInfo,
                                       qcmTableInfo * aSourceTableInfo )
{
/***********************************************************************
 *
 * Description :
 *      Replication Meta Table을 수정한다. (Meta Table)
 *          - SYS_REPL_ITEMS_
 *              - SYS_REPL_ITEMS_의 TABLE_OID는 Non-Partitioned Table OID이거나 Partition OID이다.
 *                  - Partitioned Table OID는 SYS_REPL_ITEMS_에 없다.
 *              - Non-Partitioned Table인 경우, Table OID를 Peer의 것으로 변경한다.
 *              - Partitioned Table인 경우, Partition OID를 Peer의 것으로 변경한다.
 *
 * Implementation :
 *
 ***********************************************************************/

    SChar  * sSqlStr = NULL;
    vSLong   sRowCnt = ID_vLONG(0);

    IDE_TEST( insertReplItemMetaToReplaceHistory( aStatement,
                                                  aTargetTableInfo,
                                                  aSourceTableInfo )
              != IDE_SUCCESS );

    if ( ( aTargetTableInfo->replicationCount > 0 ) || ( aSourceTableInfo->replicationCount > 0 ) )
    {
        IDU_FIT_POINT( "qdbCopySwap::swapReplItemsMeta::STRUCT_ALLOC_WITH_SIZE::sSqlStr",
                       idERR_ABORT_InsufficientMemory );

        IDE_TEST( STRUCT_ALLOC_WITH_SIZE( QC_QMX_MEM( aStatement ),
                                          SChar,
                                          QD_MAX_SQL_LENGTH,
                                          & sSqlStr )
                  != IDE_SUCCESS );

        if ( aTargetTableInfo->tablePartitionType == QCM_NONE_PARTITIONED_TABLE )
        {
            idlOS::snprintf( sSqlStr,
                             QD_MAX_SQL_LENGTH,
                             "UPDATE SYS_REPL_ITEMS_ "
                             "   SET TABLE_OID = CASE2( TABLE_OID = BIGINT'%"ID_INT64_FMT"', "
                             "                          BIGINT'%"ID_INT64_FMT"', "
                             "                          BIGINT'%"ID_INT64_FMT"' ) "
                             " WHERE TABLE_OID IN ( BIGINT'%"ID_INT64_FMT"', BIGINT'%"ID_INT64_FMT"' ) ",
                             QCM_OID_TO_BIGINT( aTargetTableInfo->tableOID ),
                             QCM_OID_TO_BIGINT( aSourceTableInfo->tableOID ),
                             QCM_OID_TO_BIGINT( aTargetTableInfo->tableOID ),
                             QCM_OID_TO_BIGINT( aTargetTableInfo->tableOID ),
                             QCM_OID_TO_BIGINT( aSourceTableInfo->tableOID ) );
        }
        else
        {
            idlOS::snprintf( sSqlStr,
                             QD_MAX_SQL_LENGTH,
                             "UPDATE SYS_REPL_ITEMS_ A "
                             "   SET TABLE_OID = ( SELECT PARTITION_OID "
                             "                       FROM SYS_TABLE_PARTITIONS_ B "
                             "                      WHERE B.TABLE_ID IN ( INTEGER'%"ID_INT32_FMT"',"
                             "                                            INTEGER'%"ID_INT32_FMT"' ) AND "
                             "                            B.PARTITION_NAME = A.LOCAL_PARTITION_NAME AND "
                             "                            B.PARTITION_OID != A.TABLE_OID ) "
                             " WHERE TABLE_OID IN ( SELECT PARTITION_OID "
                             "                        FROM SYS_TABLE_PARTITIONS_ "
                             "                       WHERE TABLE_ID IN ( INTEGER'%"ID_INT32_FMT"',"
                             "                                           INTEGER'%"ID_INT32_FMT"' ) ) ",
                             aTargetTableInfo->tableID,
                             aSourceTableInfo->tableID,
                             aTargetTableInfo->tableID,
                             aSourceTableInfo->tableID );
        }

        IDE_TEST( qcg::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                     sSqlStr,
                                     & sRowCnt )
                  != IDE_SUCCESS );

        IDE_TEST_RAISE( (UInt)sRowCnt == 0, ERR_META_CRASH );
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_META_CRASH )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QCM_META_CRASH ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdbCopySwap::updateColumnDefaultValueMeta( qcStatement * aStatement,
                                                  UInt          aTargetTableID,
                                                  UInt          aSourceTableID,
                                                  UInt          aColumnCount)
{
/***********************************************************************
 *
 * Description :
 *      SYS_COLUMNS_의 DEFAULT_VAL를 복사한다.
 *
 * Implementation :
 *
 ***********************************************************************/

    SChar  * sSqlStr = NULL;
    vSLong   sRowCnt = ID_vLONG(0);

    IDU_FIT_POINT( "qdbCopySwap::updateColumnDefaultValueMeta::STRUCT_ALLOC_WITH_SIZE::sSqlStr",
                   idERR_ABORT_InsufficientMemory );

    IDE_TEST( STRUCT_ALLOC_WITH_SIZE( QC_QMX_MEM( aStatement ),
                                      SChar,
                                      QD_MAX_SQL_LENGTH,
                                      & sSqlStr )
              != IDE_SUCCESS );

    idlOS::snprintf( sSqlStr,
                     QD_MAX_SQL_LENGTH,
                     "UPDATE SYS_COLUMNS_ A "
                     "   SET DEFAULT_VAL = ( SELECT DEFAULT_VAL "
                     "                         FROM SYS_COLUMNS_ B "
                     "                        WHERE B.TABLE_ID = INTEGER'%"ID_INT32_FMT"' AND "
                     "                              B.COLUMN_ORDER = A.COLUMN_ORDER ) "
                     " WHERE TABLE_ID = INTEGER'%"ID_INT32_FMT"' ",
                     aSourceTableID,
                     aTargetTableID );

    IDE_TEST( qcg::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                 sSqlStr,
                                 & sRowCnt )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( (UInt)sRowCnt != aColumnCount, ERR_META_CRASH );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_META_CRASH )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QCM_META_CRASH ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdbCopySwap::swapReplicationFlagOnTableHeader( smiStatement         * aStatement,
                                                      qcmTableInfo         * aTargetTableInfo,
                                                      qcmPartitionInfoList * aTargetPartInfoList,
                                                      qcmTableInfo         * aSourceTableInfo,
                                                      qcmPartitionInfoList * aSourcePartInfoList )
{
/***********************************************************************
 * Description :
 *      Table, Partition의 Replication Flag를 교환한다.
 *
 *      - Table            : SMI_TABLE_REPLICATION_MASK, SMI_TABLE_REPLICATION_TRANS_WAIT_MASK
 *      - Partition        : SMI_TABLE_REPLICATION_MASK, SMI_TABLE_REPLICATION_TRANS_WAIT_MASK
 *      - Dictionary Table : SMI_TABLE_REPLICATION_MASK
 *
 * Implementation :
 *
 ***********************************************************************/

    qcmPartitionInfoList * sPartInfo1       = NULL;
    qcmPartitionInfoList * sPartInfo2       = NULL;
    qcmTableInfo         * sDicTableInfo    = NULL;
    qcmColumn            * sColumn          = NULL;
    UInt                   sTableFlag       = 0;

    if ( ( aTargetTableInfo->replicationCount > 0 ) ||
         ( aSourceTableInfo->replicationCount > 0 ) )
    {
        // Table의 Replication Flag를 교환한다.
        sTableFlag  = aTargetTableInfo->tableFlag
                    & ~( SMI_TABLE_REPLICATION_MASK | SMI_TABLE_REPLICATION_TRANS_WAIT_MASK );
        sTableFlag |= aSourceTableInfo->tableFlag
                    &  ( SMI_TABLE_REPLICATION_MASK | SMI_TABLE_REPLICATION_TRANS_WAIT_MASK );

        IDE_TEST( smiTable::modifyTableInfo( aStatement,
                                             aTargetTableInfo->tableHandle,
                                             NULL,
                                             0,
                                             NULL,
                                             0,
                                             sTableFlag,
                                             SMI_TBSLV_DDL_DML,
                                             aTargetTableInfo->maxrows,
                                             0,         /* Parallel Degree */
                                             ID_TRUE )  /* aIsInitRowTemplate */
                  != IDE_SUCCESS );

        sTableFlag  = aSourceTableInfo->tableFlag
                    & ~( SMI_TABLE_REPLICATION_MASK | SMI_TABLE_REPLICATION_TRANS_WAIT_MASK );
        sTableFlag |= aTargetTableInfo->tableFlag
                    &  ( SMI_TABLE_REPLICATION_MASK | SMI_TABLE_REPLICATION_TRANS_WAIT_MASK );

        IDE_TEST( smiTable::modifyTableInfo( aStatement,
                                             aSourceTableInfo->tableHandle,
                                             NULL,
                                             0,
                                             NULL,
                                             0,
                                             sTableFlag,
                                             SMI_TBSLV_DDL_DML,
                                             aSourceTableInfo->maxrows,
                                             0,         /* Parallel Degree */
                                             ID_TRUE )  /* aIsInitRowTemplate */
                  != IDE_SUCCESS );

        if ( aTargetTableInfo->tablePartitionType == QCM_PARTITIONED_TABLE )
        {
            // Partition의 Replication Flag를 교환한다.
            for ( sPartInfo1 = aTargetPartInfoList;
                  sPartInfo1 != NULL;
                  sPartInfo1 = sPartInfo1->next )
            {
                IDE_TEST( qcmPartition::findPartitionByName( aSourcePartInfoList,
                                                             sPartInfo1->partitionInfo->name,
                                                             idlOS::strlen( sPartInfo1->partitionInfo->name ),
                                                             & sPartInfo2 )
                          != IDE_SUCCESS );

                // 짝을 이루는 Partition 중에서 하나라도 Replication 대상이어야 한다.
                if ( sPartInfo2 == NULL )
                {
                    continue;
                }
                else
                {
                    if ( ( sPartInfo1->partitionInfo->replicationCount == 0 ) &&
                         ( sPartInfo2->partitionInfo->replicationCount == 0 ) )
                    {
                        continue;
                    }
                    else
                    {
                        /* Nothing to do */
                    }
                }

                sTableFlag  = sPartInfo1->partitionInfo->tableFlag
                            & ~( SMI_TABLE_REPLICATION_MASK | SMI_TABLE_REPLICATION_TRANS_WAIT_MASK );
                sTableFlag |= sPartInfo2->partitionInfo->tableFlag
                            &  ( SMI_TABLE_REPLICATION_MASK | SMI_TABLE_REPLICATION_TRANS_WAIT_MASK );

                IDE_TEST( smiTable::modifyTableInfo( aStatement,
                                                     sPartInfo1->partitionInfo->tableHandle,
                                                     NULL,
                                                     0,
                                                     NULL,
                                                     0,
                                                     sTableFlag,
                                                     SMI_TBSLV_DDL_DML,
                                                     sPartInfo1->partitionInfo->maxrows,
                                                     0,         /* Parallel Degree */
                                                     ID_TRUE )  /* aIsInitRowTemplate */
                          != IDE_SUCCESS );

                sTableFlag  = sPartInfo2->partitionInfo->tableFlag
                            & ~( SMI_TABLE_REPLICATION_MASK | SMI_TABLE_REPLICATION_TRANS_WAIT_MASK );
                sTableFlag |= sPartInfo1->partitionInfo->tableFlag
                            &  ( SMI_TABLE_REPLICATION_MASK | SMI_TABLE_REPLICATION_TRANS_WAIT_MASK );

                IDE_TEST( smiTable::modifyTableInfo( aStatement,
                                                     sPartInfo2->partitionInfo->tableHandle,
                                                     NULL,
                                                     0,
                                                     NULL,
                                                     0,
                                                     sTableFlag,
                                                     SMI_TBSLV_DDL_DML,
                                                     sPartInfo2->partitionInfo->maxrows,
                                                     0,         /* Parallel Degree */
                                                     ID_TRUE )  /* aIsInitRowTemplate */
                          != IDE_SUCCESS );
            }
        }
        else
        {
            // Dictionary Table에 Replication Flag를 적용한다.
            for ( sColumn = aTargetTableInfo->columns; sColumn != NULL; sColumn = sColumn->next )
            {
                if ( ( sColumn->basicInfo->column.flag & SMI_COLUMN_COMPRESSION_MASK )
                                                      == SMI_COLUMN_COMPRESSION_TRUE )
                {
                    // 기존 Dictionary Table Info를 가져온다.
                    sDicTableInfo = (qcmTableInfo *)smiGetTableRuntimeInfoFromTableOID(
                                                    sColumn->basicInfo->column.mDictionaryTableOID );

                    // Dictionary Table Info는 Table을 따른다.
                    sTableFlag = sDicTableInfo->tableFlag
                               & ~( SMI_TABLE_REPLICATION_MASK | SMI_TABLE_REPLICATION_TRANS_WAIT_MASK );
                    if ( aSourceTableInfo->replicationCount > 0 )
                    {
                        sTableFlag |= SMI_TABLE_REPLICATION_ENABLE;
                    }
                    else
                    {
                        sTableFlag |= SMI_TABLE_REPLICATION_DISABLE;
                    }

                    IDE_TEST( smiTable::modifyTableInfo( aStatement,
                                                         sDicTableInfo->tableHandle,
                                                         NULL,
                                                         0,
                                                         NULL,
                                                         0,
                                                         sTableFlag,
                                                         SMI_TBSLV_DDL_DML,
                                                         sDicTableInfo->maxrows,
                                                         0,         /* Parallel Degree */
                                                         ID_TRUE )  /* aIsInitRowTemplate */
                              != IDE_SUCCESS );
                }
                else
                {
                    /* Nothing to do */
                }
            }

            for ( sColumn = aSourceTableInfo->columns; sColumn != NULL; sColumn = sColumn->next )
            {
                if ( ( sColumn->basicInfo->column.flag & SMI_COLUMN_COMPRESSION_MASK )
                                                      == SMI_COLUMN_COMPRESSION_TRUE )
                {
                    // 기존 Dictionary Table Info를 가져온다.
                    sDicTableInfo = (qcmTableInfo *)smiGetTableRuntimeInfoFromTableOID(
                                                    sColumn->basicInfo->column.mDictionaryTableOID );

                    // Dictionary Table Info는 Table을 따른다.
                    sTableFlag = sDicTableInfo->tableFlag
                               & ~( SMI_TABLE_REPLICATION_MASK | SMI_TABLE_REPLICATION_TRANS_WAIT_MASK );
                    if ( aTargetTableInfo->replicationCount > 0 )
                    {
                        sTableFlag |= SMI_TABLE_REPLICATION_ENABLE;
                    }
                    else
                    {
                        sTableFlag |= SMI_TABLE_REPLICATION_DISABLE;
                    }

                    IDE_TEST( smiTable::modifyTableInfo( aStatement,
                                                         sDicTableInfo->tableHandle,
                                                         NULL,
                                                         0,
                                                         NULL,
                                                         0,
                                                         sTableFlag,
                                                         SMI_TBSLV_DDL_DML,
                                                         sDicTableInfo->maxrows,
                                                         0,         /* Parallel Degree */
                                                         ID_TRUE )  /* aIsInitRowTemplate */
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

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdbCopySwap::checkTablesExistInOneReplication( qcStatement  * aStatement,
                                                      qcmTableInfo * aTargetTableInfo,
                                                      qcmTableInfo * aSourceTableInfo )
{
/***********************************************************************
 * Description :
 *      양쪽 Table이 같은 Replication에 속하면 안 된다.
 *          - Replication에서 Table Meta Log를 하나씩 처리하므로, 이 기능을 지원하지 않는다.
 *
 * Implementation :
 *
 ***********************************************************************/

    SDouble         sCharBuffer[(ID_SIZEOF(UShort) + QC_MAX_NAME_LEN + 7) / 8] = { (SDouble)0.0, };
    SChar           sMsgBuffer[QC_MAX_NAME_LEN + 1] = { 0, };
    mtdCharType   * sResult         = (mtdCharType *) & sCharBuffer;
    SChar         * sSqlStr         = NULL;
    idBool          sRecordExist    = ID_FALSE;

    IDU_FIT_POINT( "qdbCopySwap::checkTablesExistInOneReplication::STRUCT_ALLOC_WITH_SIZE::sSqlStr",
                   idERR_ABORT_InsufficientMemory );

    IDE_TEST( STRUCT_ALLOC_WITH_SIZE( QC_QMX_MEM( aStatement ),
                                      SChar,
                                      QD_MAX_SQL_LENGTH,
                                      & sSqlStr )
              != IDE_SUCCESS );

    idlOS::snprintf( sSqlStr,
                     QD_MAX_SQL_LENGTH,
                     "SELECT REPLICATION_NAME "
                     "  FROM SYS_REPL_ITEMS_ "
                     " WHERE LOCAL_USER_NAME = VARCHAR'%s' AND "
                     "       LOCAL_TABLE_NAME = VARCHAR'%s' "
                     "INTERSECT "
                     "SELECT REPLICATION_NAME "
                     "  FROM SYS_REPL_ITEMS_ "
                     " WHERE LOCAL_USER_NAME = VARCHAR'%s' AND "
                     "       LOCAL_TABLE_NAME = VARCHAR'%s' ",
                     aTargetTableInfo->tableOwnerName,
                     aTargetTableInfo->name,
                     aSourceTableInfo->tableOwnerName,
                     aSourceTableInfo->name );

    IDE_TEST( qcg::runSelectOneRowforDDL( QC_SMI_STMT( aStatement ),
                                          sSqlStr,
                                          sResult,
                                          & sRecordExist,
                                          ID_FALSE )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( sRecordExist == ID_TRUE, ERR_SWAP_TABLES_EXIST_IN_SAME_REPLICATION );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_SWAP_TABLES_EXIST_IN_SAME_REPLICATION )
    {
        idlOS::memcpy( sMsgBuffer, sResult->value, sResult->length );
        sMsgBuffer[sResult->length] = '\0';

        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDB_SWAP_TABLES_EXIST_IN_SAME_REPLICATION,
                                  sMsgBuffer ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdbCopySwap::checkEncryptColumn( idBool      aIsRenameForce,
                                        qcmColumn * aColumns )
{
/***********************************************************************
 * Description :
 *      Encrypt Column 제약을 확인한다.
 *          - RENAME FORCE 절을 지정하지 않은 경우, Encrypt Column을 지원하지 않는다.
 *
 * Implementation :
 *
 ***********************************************************************/

    qcmColumn * sColumn = NULL;

    if ( aIsRenameForce != ID_TRUE )
    {
        for ( sColumn = aColumns;
              sColumn != NULL;
              sColumn = sColumn->next )
        {
            IDE_TEST_RAISE( ( sColumn->basicInfo->module->flag & MTD_ENCRYPT_TYPE_MASK )
                                                              == MTD_ENCRYPT_TYPE_TRUE,
                            ERR_EXIST_ENCRYPTED_COLUMN );
        }
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_EXIST_ENCRYPTED_COLUMN )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_EXIST_ENCRYPTED_COLUMN ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdbCopySwap::checkCompressedColumnForReplication( qcStatement    * aStatement,
                                                         qcmTableInfo   * aTableInfo,
                                                         qcNamePosition   aTableName )
{
/***********************************************************************
 * Description :
 *      Source와 Target의 Compressed Column 제약을 검사한다.
 *          - Replication이 걸린 경우, Compressed Column을 지원하지 않는다.
 *
 * Implementation :
 *
 ***********************************************************************/

    qcmColumn         * sColumn     = NULL;
    qcuSqlSourceInfo    sqlInfo;

    for ( sColumn = aTableInfo->columns;
          sColumn != NULL;
          sColumn = sColumn->next )
    {
        if ( ( sColumn->basicInfo->column.flag & SMI_COLUMN_COMPRESSION_MASK )
                                              == SMI_COLUMN_COMPRESSION_TRUE )
        {
            sqlInfo.setSourceInfo( aStatement, & aTableName );

            IDE_RAISE( ERR_REPLICATION_AND_COMPRESSED_COLUMN );
        }
        else
        {
            /* Nothing to do */
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_REPLICATION_AND_COMPRESSED_COLUMN )
    {
        (void)sqlInfo.init( QC_QME_MEM( aStatement ) );
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDB_SWAP_NOT_SUPPORT_REPLICATION_WITH_COMPRESSED_COLUMN,
                                  sqlInfo.getErrMessage() ) );
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdbCopySwap::checkNormalUserTable( qcStatement    * aStatement,
                                          qcmTableInfo   * aTableInfo,
                                          qcNamePosition   aTableName )
{
/***********************************************************************
 * Description :
 *      일반 Table인지 검사한다.
 *          - TABLE_TYPE = 'T', TEMPORARY = 'N', HIDDEN = 'N' 이어야 한다. (SYS_TABLES_)
 *
 * Implementation :
 *
 ***********************************************************************/

    qcuSqlSourceInfo    sqlInfo;

    if ( ( aTableInfo->tableType != QCM_USER_TABLE ) ||
         ( aTableInfo->temporaryInfo.type != QCM_TEMPORARY_ON_COMMIT_NONE ) ||
         ( aTableInfo->hiddenType != QCM_HIDDEN_NONE ) )
    {
        sqlInfo.setSourceInfo( aStatement, & aTableName );

        IDE_RAISE( ERR_NOT_NORMAL_USER_TABLE );
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_NORMAL_USER_TABLE )
    {
        (void)sqlInfo.init( QC_QME_MEM( aStatement ) );
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDB_COPY_SWAP_SUPPORT_NORMAL_USER_TABLE,
                                  sqlInfo.getErrMessage() ) );
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdbCopySwap::swapTablePartitionsMeta( qcStatement     * aStatement,
                                             UInt              aTargetTableID,
                                             UInt              aSourceTableID,
                                             qcNamePosition    aPartitionName )
{
/***********************************************************************
 *
 * Description :
 *      Source와 Target의 Table 기본 정보를 교환한다. (Meta Table)
 *          - SYS_TABLE_PARTITIONS_
 *              - TABLE_ID, REPLICATION_COUNT, REPLICATION_RECOVERY_COUNT를 교환한다.
 *              - LAST_DDL_TIME을 갱신한다. (SYSDATE)
 *              - PARITION_USABLE를 교환한다. (TASK-7307)
 *
 * Implementation :
 *
 ***********************************************************************/

    SChar  * sSqlStr = NULL;
    vSLong   sRowCnt = ID_vLONG(0);
    SChar    sPartitionName[QC_MAX_OBJECT_NAME_LEN + 1];

    QC_STR_COPY( sPartitionName, aPartitionName );
    
    IDU_FIT_POINT( "qdbCopySwap::swapTablePartitionsMeta::STRUCT_ALLOC_WITH_SIZE::sSqlStr",
                   idERR_ABORT_InsufficientMemory );

    IDE_TEST( STRUCT_ALLOC_WITH_SIZE( QC_QMX_MEM( aStatement ),
                                      SChar,
                                      QD_MAX_SQL_LENGTH,
                                      & sSqlStr )
              != IDE_SUCCESS );

    idlOS::snprintf( sSqlStr,
                     QD_MAX_SQL_LENGTH,
                     "UPDATE SYS_TABLE_PARTITIONS_ A "
                     "   SET (        TABLE_ID, PARTITION_USABLE, REPLICATION_COUNT, REPLICATION_RECOVERY_COUNT ) = "
                     "       ( SELECT TABLE_ID, PARTITION_USABLE, REPLICATION_COUNT, REPLICATION_RECOVERY_COUNT "
                     "           FROM SYS_TABLE_PARTITIONS_ B "
                     "          WHERE B.TABLE_ID = CASE2( A.TABLE_ID = INTEGER'%"ID_INT32_FMT"', "
                     "                                    INTEGER'%"ID_INT32_FMT"', "
                     "                                    INTEGER'%"ID_INT32_FMT"' ) "
                     "              AND PARTITION_NAME = VARCHAR'%s' ), "
                     "       LAST_DDL_TIME = SYSDATE "
                     " WHERE TABLE_ID IN ( INTEGER'%"ID_INT32_FMT"', INTEGER'%"ID_INT32_FMT"' ) "
                     " AND PARTITION_NAME = VARCHAR'%s' ",
                     aTargetTableID,
                     aSourceTableID,
                     aTargetTableID,
                     sPartitionName,
                     aTargetTableID,
                     aSourceTableID,
                     sPartitionName );

    IDE_TEST( qcg::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                 sSqlStr,
                                 & sRowCnt )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( sRowCnt != 2, ERR_META_CRASH );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_META_CRASH )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QCM_META_CRASH ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdbCopySwap::swapPartLobs( qcStatement * aStatement,
                                  UInt          aTargetTableID,
                                  UInt          aSourceTableID,
                                  UInt          aTargetPartTableID,
                                  UInt          aSourcePartTableID )
{
/***********************************************************************
 *
 * Description :
 *      Source와 Target의 Partition Table 기본 정보를 교환한다. (Meta Table)
 *          - SYS_PART_LOBS_
 *              - TABLE_ID
 *
 * Implementation :
 *
 ***********************************************************************/

    SChar  * sSqlStr = NULL;
    vSLong   sRowCnt = ID_vLONG(0);

    IDU_FIT_POINT( "qdbCopySwap::swapPartLobs::STRUCT_ALLOC_WITH_SIZE::sSqlStr",
                   idERR_ABORT_InsufficientMemory );

    IDE_TEST( STRUCT_ALLOC_WITH_SIZE( QC_QMX_MEM( aStatement ),
                                      SChar,
                                      QD_MAX_SQL_LENGTH,
                                      & sSqlStr )
              != IDE_SUCCESS );

    idlOS::snprintf( sSqlStr,
                     QD_MAX_SQL_LENGTH,
                     "UPDATE SYS_PART_LOBS_ "
                     " SET TABLE_ID = "
                     "   CASE2( TABLE_ID = INTEGER'%"ID_INT32_FMT"' AND "
                     "           PARTITION_ID = INTEGER'%"ID_INT32_FMT"' "
                     "        ,INTEGER'%"ID_INT32_FMT"', "
                     "        TABLE_ID = INTEGER'%"ID_INT32_FMT"' AND "
                     "           PARTITION_ID = INTEGER'%"ID_INT32_FMT"' "
                     "        ,INTEGER'%"ID_INT32_FMT"' ) "
                     " WHERE PARTITION_ID = INTEGER'%"ID_INT32_FMT"' OR "
                     "       PARTITION_ID = INTEGER'%"ID_INT32_FMT"' ",
                     aTargetTableID,
                     aTargetPartTableID,
                     aSourceTableID,
                     aSourceTableID,
                     aSourcePartTableID,
                     aTargetTableID,
                     aTargetPartTableID,
                     aSourcePartTableID );

    IDE_TEST( qcg::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                 sSqlStr,
                                 & sRowCnt )
              != IDE_SUCCESS );

   IDE_TEST_RAISE( (UInt)sRowCnt == 0, ERR_META_CRASH );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_META_CRASH )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QCM_META_CRASH ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdbCopySwap::swapIndexPartitions( qcStatement  * aStatement,
                                         UInt           aTargetTableID,
                                         UInt           aSourceTableID,
                                         qcmTableInfo * aTargetPartTableInfo,
                                         qcmTableInfo * aSourcePartTableInfo )
{
/***********************************************************************
 *
 * Description :
 *      Source와 Target의 Table 기본 정보를 교환한다. (Meta Table)
 *          - SYS_INDEX_PARTITIONS_
 *              - TABLE_ID, INDEX_ID, INDEX_PARTITION_NAME
 *              - LAST_DDL_TIME을 갱신한다. (SYSDATE)
 *
 * Implementation :
 *
 ***********************************************************************/

    SChar  * sSqlStr = NULL;
    vSLong   sRowCnt = ID_vLONG(0);
    UInt     sIndexCount = 0;
    
    IDU_FIT_POINT( "qdbCopySwap::swapIndexPartitions::STRUCT_ALLOC_WITH_SIZE::sSqlStr",
                   idERR_ABORT_InsufficientMemory );

    IDE_TEST( STRUCT_ALLOC_WITH_SIZE( QC_QMX_MEM( aStatement ),
                                      SChar,
                                      QD_MAX_SQL_LENGTH,
                                      & sSqlStr )
              != IDE_SUCCESS );

    for ( sIndexCount = 0;
          sIndexCount < aTargetPartTableInfo->indexCount;
          sIndexCount++ )
    {
        idlOS::snprintf( sSqlStr,
                         QD_MAX_SQL_LENGTH,
                         " UPDATE SYS_INDEX_PARTITIONS_ "
                         " SET TABLE_ID = "
                         " CASE2( TABLE_ID = INTEGER'%"ID_INT32_FMT"', "
                         " INTEGER'%"ID_INT32_FMT"', "
                         " TABLE_ID = INTEGER'%"ID_INT32_FMT"', "
                         " INTEGER'%"ID_INT32_FMT"' ), "
                         " INDEX_ID = "
                         " CASE2( INDEX_ID = INTEGER'%"ID_INT32_FMT"', "
                         " INTEGER'%"ID_INT32_FMT"', "
                         " INDEX_ID = INTEGER'%"ID_INT32_FMT"', "
                         " INTEGER'%"ID_INT32_FMT"' ), "
                         " INDEX_PARTITION_NAME = "
                         " CASE2( INDEX_PARTITION_NAME = VARCHAR'%s', "
                         " VARCHAR'%s', "
                         " INDEX_PARTITION_NAME = VARCHAR'%s', "
                         " VARCHAR'%s' ), "
                         " LAST_DDL_TIME = SYSDATE "
                         " WHERE TABLE_PARTITION_ID = INTEGER'%"ID_INT32_FMT"' "
                         " AND INDEX_PARTITION_ID = INTEGER'%"ID_INT32_FMT"' "
                         " OR TABLE_PARTITION_ID = INTEGER'%"ID_INT32_FMT"' "
                         " AND INDEX_PARTITION_ID = INTEGER'%"ID_INT32_FMT"' ",
                         aTargetTableID,
                         aSourceTableID,
                         aSourceTableID,
                         aTargetTableID,
                         aTargetPartTableInfo->indices[sIndexCount].indexId,
                         aSourcePartTableInfo->indices[sIndexCount].indexId,
                         aSourcePartTableInfo->indices[sIndexCount].indexId,
                         aTargetPartTableInfo->indices[sIndexCount].indexId,
                         aTargetPartTableInfo->indices[sIndexCount].name,
                         aSourcePartTableInfo->indices[sIndexCount].name,
                         aSourcePartTableInfo->indices[sIndexCount].name,
                         aTargetPartTableInfo->indices[sIndexCount].name,
                         aTargetPartTableInfo->partitionID,
                         aTargetPartTableInfo->indices[sIndexCount].indexPartitionID,
                         aSourcePartTableInfo->partitionID,
                         aSourcePartTableInfo->indices[sIndexCount].indexPartitionID );
        
        IDE_TEST( qcg::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                     sSqlStr,
                                     & sRowCnt )
                  != IDE_SUCCESS );

        IDE_TEST_RAISE( (UInt)sRowCnt == 0, ERR_META_CRASH );
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_META_CRASH )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QCM_META_CRASH ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdbCopySwap::insertReplItemsForParititonMetaToReplaceHistory( qcStatement  * aStatement,
                                                                     qcmTableInfo * aTargetPartTableInfo,
                                                                     qcmTableInfo * aSourcePartTableInfo )
{
/***********************************************************************
 *
 * Description :
 *      Replication Meta Table을 수정한다. (Meta Table)
 *          - SYS_REPL_ITEMS_
 *          - SYS_REPL_ITEMS_의 TABLE_OID는 Non-Partitioned Table OID이거나
 *            Partition OID이다.
 *          - Partitioned Table OID는 SYS_REPL_ITEMS_에 없다.
 *          - target, source Partitioned Table 의 TABLE_OID(Partition OID) 교환
 *
 * Implementation :
 *
 ***********************************************************************/

    SChar  * sSqlStr = NULL;
    vSLong   sDummyCnt = ID_vLONG(0);

    if ( ( aTargetPartTableInfo->replicationCount > 0 ) ||
         ( aSourcePartTableInfo->replicationCount > 0 ) )
    {
        IDU_FIT_POINT( "qdbCopySwap::insertReplItemsForParititonMetaToReplaceHistory::STRUCT_ALLOC_WITH_SIZE::sSqlStr",
                       idERR_ABORT_InsufficientMemory );

        IDE_TEST( STRUCT_ALLOC_WITH_SIZE( QC_QMX_MEM( aStatement ),
                                          SChar,
                                          QD_MAX_SQL_LENGTH,
                                          & sSqlStr )
                  != IDE_SUCCESS );
            idlOS::snprintf( sSqlStr,
                             QD_MAX_SQL_LENGTH,
                             "DELETE FROM SYS_REPL_ITEM_REPLACE_HISTORY_ "
                             "       WHERE OLD_OID IN "
                             "                       ( BIGINT'%"ID_INT64_FMT"', BIGINT'%"ID_INT64_FMT"' ) "
                             "         OR  NEW_OID IN "
                             "                       ( BIGINT'%"ID_INT64_FMT"', BIGINT'%"ID_INT64_FMT"' ) ",
                             QCM_OID_TO_BIGINT( aTargetPartTableInfo->tableOID ),
                             QCM_OID_TO_BIGINT( aTargetPartTableInfo->tableOID ),
                             QCM_OID_TO_BIGINT( aSourcePartTableInfo->tableOID ),
                             QCM_OID_TO_BIGINT( aTargetPartTableInfo->tableOID ) );

            IDE_TEST( qcg::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                         sSqlStr,
                                         & sDummyCnt )
                      != IDE_SUCCESS );

            idlOS::snprintf( sSqlStr,
                             QD_MAX_SQL_LENGTH,
                             "INSERT INTO SYS_REPL_ITEM_REPLACE_HISTORY_ ( REPLICATION_NAME, USER_NAME, TABLE_NAME, PARTITION_NAME, OLD_OID , NEW_OID )"
                             "   SELECT REPLICATION_NAME, LOCAL_USER_NAME, LOCAL_TABLE_NAME, LOCAL_PARTITION_NAME, " 
                             "          TABLE_OID,  CASE2( TABLE_OID = BIGINT'%"ID_INT64_FMT"', "
                             "                                             BIGINT'%"ID_INT64_FMT"'," 
                             "                                             BIGINT'%"ID_INT64_FMT"' ) " 
                             "      FROM SYS_REPL_ITEMS_ WHERE TABLE_OID IN "
                             "                                 ( BIGINT'%"ID_INT64_FMT"', BIGINT'%"ID_INT64_FMT"' ) "
                             "AND REPLICATION_NAME IN ( SELECT REPLICATION_NAME FROM SYSTEM_.SYS_REPLICATIONS_ WHERE REPL_MODE=12 )",
                             QCM_OID_TO_BIGINT( aTargetPartTableInfo->tableOID ),
                             QCM_OID_TO_BIGINT( aSourcePartTableInfo->tableOID ),
                             QCM_OID_TO_BIGINT( aTargetPartTableInfo->tableOID ),
                             QCM_OID_TO_BIGINT( aSourcePartTableInfo->tableOID ),
                             QCM_OID_TO_BIGINT( aTargetPartTableInfo->tableOID ) );

        IDE_TEST( qcg::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                     sSqlStr,
                                     & sDummyCnt )
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

IDE_RC qdbCopySwap::swapReplItemsForParititonMeta( qcStatement  * aStatement,
                                                   qcmTableInfo * aTargetPartTableInfo,
                                                   qcmTableInfo * aSourcePartTableInfo )
{
/***********************************************************************
 *
 * Description :
 *      Replication Meta Table을 수정한다. (Meta Table)
 *          - SYS_REPL_ITEMS_
 *          - SYS_REPL_ITEMS_의 TABLE_OID는 Non-Partitioned Table OID이거나
 *            Partition OID이다.
 *          - Partitioned Table OID는 SYS_REPL_ITEMS_에 없다.
 *          - target, source Partitioned Table 의 TABLE_OID(Partition OID) 교환
 *
 * Implementation :
 *
 ***********************************************************************/

    SChar  * sSqlStr = NULL;
    vSLong   sRowCnt = ID_vLONG(0);

    IDE_TEST( insertReplItemsForParititonMetaToReplaceHistory( aStatement,
                                                               aTargetPartTableInfo,
                                                               aSourcePartTableInfo )
              != IDE_SUCCESS );

    if ( ( aTargetPartTableInfo->replicationCount > 0 ) ||
         ( aSourcePartTableInfo->replicationCount > 0 ) )
    {
        IDU_FIT_POINT( "qdbCopySwap::swapReplItemsForParititonMeta::STRUCT_ALLOC_WITH_SIZE::sSqlStr",
                       idERR_ABORT_InsufficientMemory );

        IDE_TEST( STRUCT_ALLOC_WITH_SIZE( QC_QMX_MEM( aStatement ),
                                          SChar,
                                          QD_MAX_SQL_LENGTH,
                                          & sSqlStr )
                  != IDE_SUCCESS );

        idlOS::snprintf( sSqlStr,
                         QD_MAX_SQL_LENGTH,
                         "UPDATE SYS_REPL_ITEMS_ A "
                         " SET TABLE_OID = CASE2( TABLE_OID = INTEGER'%"ID_INT32_FMT"', "
                         "        INTEGER'%"ID_INT32_FMT"', "
                         "     TABLE_OID = INTEGER'%"ID_INT32_FMT"', "
                         "        INTEGER'%"ID_INT32_FMT"' ) "
                         " WHERE TABLE_OID = INTEGER'%"ID_INT32_FMT"' OR "
                         "       TABLE_OID = INTEGER'%"ID_INT32_FMT"' ",
                         aTargetPartTableInfo->tableOID,
                         aSourcePartTableInfo->tableOID,
                         aSourcePartTableInfo->tableOID,
                         aTargetPartTableInfo->tableOID,
                         aTargetPartTableInfo->tableOID,
                         aSourcePartTableInfo->tableOID );

        IDE_TEST( qcg::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                     sSqlStr,
                                     & sRowCnt )
                  != IDE_SUCCESS );

        IDE_TEST_RAISE( (UInt)sRowCnt == 0, ERR_META_CRASH );
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_META_CRASH )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QCM_META_CRASH ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdbCopySwap::swapReplicationFlagOnPartitonTableHeader( smiStatement  * aStatement,
                                                              qcmTableInfo  * aTargetPartTableInfo,
                                                              qcmTableInfo  * aSourcePartTableInfo )
{
/***********************************************************************
 * Description :
 *      Partition의 Replication Flag를 교환한다.
 *
  *      - Partition : SMI_TABLE_REPLICATION_MASK, SMI_TABLE_REPLICATION_TRANS_WAIT_MASK
  *
 * Implementation :
 *
 ***********************************************************************/
    
    UInt sTableFlag = 0;

    if ( ( aTargetPartTableInfo->replicationCount > 0 ) ||
         ( aSourcePartTableInfo->replicationCount > 0 ) )
    {
        // Partition의 Replication Flag를 교환한다.
        sTableFlag  = aTargetPartTableInfo->tableFlag
            & ~( SMI_TABLE_REPLICATION_MASK | SMI_TABLE_REPLICATION_TRANS_WAIT_MASK );
        sTableFlag |= aSourcePartTableInfo->tableFlag
            &  ( SMI_TABLE_REPLICATION_MASK | SMI_TABLE_REPLICATION_TRANS_WAIT_MASK );

        IDE_TEST( smiTable::modifyTableInfo( aStatement,
                                             aTargetPartTableInfo->tableHandle,
                                             NULL,
                                             0,
                                             NULL,
                                             0,
                                             sTableFlag,
                                             SMI_TBSLV_DDL_DML,
                                             aSourcePartTableInfo->maxrows,
                                             0,         /* Parallel Degree */
                                             ID_TRUE )  /* aIsInitRowTemplate */
                  != IDE_SUCCESS );

        sTableFlag  = aSourcePartTableInfo->tableFlag
            & ~( SMI_TABLE_REPLICATION_MASK | SMI_TABLE_REPLICATION_TRANS_WAIT_MASK );
        sTableFlag |= aTargetPartTableInfo->tableFlag
            &  ( SMI_TABLE_REPLICATION_MASK | SMI_TABLE_REPLICATION_TRANS_WAIT_MASK );

        IDE_TEST( smiTable::modifyTableInfo( aStatement,
                                             aSourcePartTableInfo->tableHandle,
                                             NULL,
                                             0,
                                             NULL,
                                             0,
                                             sTableFlag,
                                             SMI_TBSLV_DDL_DML,
                                             aTargetPartTableInfo->maxrows,
                                             0,         /* Parallel Degree */
                                             ID_TRUE )  /* aIsInitRowTemplate */
                  != IDE_SUCCESS );
    }
    else
    {
        // nothing to do
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdbCopySwap::validateReplacePartition( qcStatement      * aStatement,
                                              qdTableParseTree * aParseTree )
{
/***********************************************************************
 * Description :
 *      BUG-45745
 *
 * Implementation :
 *  partition table swap validate 수행
 *      1, partition table swap validate 수행
 *      2. table parttion key 관련체크
 *      3. table partiton column 관련 체크
 *      4. table partiton 속성 관련 체크
 *      변경: BUG-47208  range, range hash, list partition
 *            replace 대상 partition 속성 체크    
 *
 ***********************************************************************/

    smOID                   * sDDLTableOIDArray     = NULL;
    smOID                   * sDDLPartOIDArray      = NULL;
    qcmTableInfo            * sTableInfo            = NULL;
    qcmTableInfo            * sSourceTableInfo      = NULL;
    qcmTableInfo            * sSrcTempPartInfo      = NULL;
    qcmTableInfo            * sDstTempPartInfo      = NULL;
    void                    * sHandle               = NULL;
    qcmColumn               * sTargetColumns        = NULL;
    qcmColumn               * sSourceColumns        = NULL;
    qdPartitionAttribute    * sNewTargetPartAttr    = NULL;
    qdPartitionAttribute    * sNewSourcePartAttr    = NULL;
    UInt                      sTargetPartOrder      = 0;
    UInt                      sSourcePartOrder      = 0;
    UInt                      sTargetPartCount      = 0;
    UInt                      sSourcePartCount      = 0;    
    UInt                      sMaxValCount          = 0;
    UInt                      sTargetColumnOrder    = 0;
    UInt                      sSourceColumnOrder    = 0;    
    smSCN                     sSCN                  = SM_SCN_INIT;
    qcuSqlSourceInfo          sqlInfo;
        
    /* 1, partition table swap validate 수행 */
    if ( aParseTree->mPartAttr != NULL )
    {
        sTableInfo          = aParseTree->tableInfo;
        sSourceTableInfo    = aParseTree->mSourcePartTable->mTableInfo;

        /* 1. table parttion 존재여부 체크 */
        IDE_TEST_RAISE(( sTableInfo->tablePartitionType == QCM_NONE_PARTITIONED_TABLE ) ||
                       ( sSourceTableInfo->tablePartitionType == QCM_NONE_PARTITIONED_TABLE),
                       ERR_NOT_EXIST_PARTITION_TABLE );

        // target exist partition table
        if ( qcmPartition::getPartitionInfo( aStatement,
                                             aParseTree->tableInfo->tableID,
                                             aParseTree->mPartAttr->tablePartName,
                                             & sDstTempPartInfo,
                                             & sSCN,
                                             & sHandle )
             != IDE_SUCCESS )
        {
            sqlInfo.setSourceInfo(aStatement,
                                  & aParseTree->mPartAttr->tablePartName);
            IDE_RAISE( ERR_NOT_EXIST_TABLE_PARTITION );
        }
        else
        {
            /* Nothing to do */
        }

        // Source exist partition table
        if ( qcmPartition::getPartitionInfo( aStatement,
                                             aParseTree->mSourcePartTable->mTableInfo->tableID,
                                             aParseTree->mPartAttr->tablePartName,
                                             & sSrcTempPartInfo,
                                             & sSCN,
                                             & sHandle )
             != IDE_SUCCESS )
        {
            sqlInfo.setSourceInfo(aStatement,
                                  & aParseTree->mPartAttr->tablePartName);
            IDE_RAISE( ERR_NOT_EXIST_TABLE_PARTITION );
        }
        else
        {
            /* Nothing to do */
        }

        /* 2. table parttion key 관련체크 */
        
        // partition key column count 같아야함.
        IDE_TEST_RAISE( sTableInfo->partKeyColCount != sSourceTableInfo->partKeyColCount,
                        ERR_REPLACE_DIFFERENT_PARTITION );
        
        // partition key column의 column order 같아야함
        for ( sTargetColumns = sTableInfo->partKeyColumns,
                  sSourceColumns = sSourceTableInfo->partKeyColumns;
              sTargetColumns != NULL;
              sTargetColumns = sTargetColumns->next,
                  sSourceColumns = sSourceColumns->next )
        {
            sTargetColumnOrder = ( sTargetColumns->basicInfo->column.id & SMI_COLUMN_ID_MASK);
            sSourceColumnOrder = ( sSourceColumns->basicInfo->column.id & SMI_COLUMN_ID_MASK);

            IDE_TEST_RAISE( sTargetColumnOrder != sSourceColumnOrder,
                            ERR_REPLACE_DIFFERENT_PARTITION );
        }

        /* 3. table partiton column 관련 체크 */

        // column count, index count
        IDE_TEST_RAISE( ( sDstTempPartInfo->columnCount != sSrcTempPartInfo->columnCount ) ||
                        ( sDstTempPartInfo->indexCount != sSrcTempPartInfo->indexCount ),
                        ERR_REPLACE_DIFFERENT_PARTITION );

        // target, source column type은 같아야 함.
        sTargetColumns = sTableInfo->columns;
        sSourceColumns = sSourceTableInfo->columns;
        
        while ( sSourceColumns != NULL)
        {
            if ( qtc::isSameType( sSourceColumns->basicInfo,
                                  sTargetColumns->basicInfo ) == ID_TRUE )
            {
                // nothing to do
            }            
            else
            {
                IDE_RAISE( ERR_NOT_COMPATIBLE_TYPE );
            }

            sSourceColumns = sSourceColumns->next;
            sTargetColumns = sTargetColumns->next;
        }

        /* 4. table partiton 속성 관련 체크 */
        
        // target, source partition method 
        IDE_TEST_RAISE( ( sDstTempPartInfo->partitionMethod !=
                          sSrcTempPartInfo->partitionMethod ),
                        ERR_REPLACE_DIFFERENT_PARTITION );

        // target partition total count
        IDE_TEST( qcmPartition::getPartitionCount( aStatement,
                                                   sDstTempPartInfo->tableID,
                                                   & sTargetPartCount )
                  != IDE_SUCCESS );

        // source partition total count
        IDE_TEST( qcmPartition::getPartitionCount( aStatement,
                                                   sSrcTempPartInfo->tableID,
                                                   & sSourcePartCount )
                  != IDE_SUCCESS );

        IDE_TEST_RAISE( sTargetPartCount != sSourcePartCount,
                        ERR_REPLACE_DIFFERENT_PARTITION );
            
        // target partAttr
        IDU_FIT_POINT( "qdbCopySwap::validateReplacePartitionTable::sNewTargetPartAttr",
                       idERR_ABORT_InsufficientMemory );

        IDE_TEST( STRUCT_ALLOC_WITH_COUNT( QC_QMP_MEM( aStatement ),
                                           qdPartitionAttribute,
                                           1,
                                           & sNewTargetPartAttr )
                  != IDE_SUCCESS );

        QD_SET_INIT_PARTITION_ATTR( sNewTargetPartAttr );

        IDU_FIT_POINT( "qdbCopySwap::validateReplacePartitionTable::alterPart",
                       idERR_ABORT_InsufficientMemory );

        IDE_TEST( STRUCT_ALLOC_WITH_COUNT( QC_QMP_MEM( aStatement ),
                                           qdAlterPartition,
                                           1,
                                           & sNewTargetPartAttr->alterPart )
                  != IDE_SUCCESS );
        
        // source partAttr
        IDU_FIT_POINT( "qdbCopySwap::validateReplacePartitionTable::sNewSourcePartAttr",
                       idERR_ABORT_InsufficientMemory );

        IDE_TEST( STRUCT_ALLOC_WITH_COUNT( QC_QMP_MEM( aStatement ),
                                           qdPartitionAttribute,
                                           1,
                                           & sNewSourcePartAttr )
                  != IDE_SUCCESS );

        QD_SET_INIT_PARTITION_ATTR( sNewSourcePartAttr );

        IDU_FIT_POINT( "qdbCopySwap::validateReplacePartitionTable::alterPart",
                       idERR_ABORT_InsufficientMemory );

        IDE_TEST( STRUCT_ALLOC_WITH_COUNT( QC_QMP_MEM( aStatement ),
                                           qdAlterPartition,
                                           1,
                                           & sNewSourcePartAttr->alterPart )
                  != IDE_SUCCESS );

        if ( sSrcTempPartInfo->partitionMethod == QCM_PARTITION_METHOD_RANGE )
        {
            IDE_TEST( qdbCommon::makePartCondValList( aStatement,
                                                      sDstTempPartInfo,
                                                      sDstTempPartInfo->partitionID,
                                                      sNewTargetPartAttr )
                      != IDE_SUCCESS );

            IDE_TEST( qdbCommon::makePartCondValList( aStatement,
                                                      sSrcTempPartInfo,
                                                      sSrcTempPartInfo->partitionID,
                                                      sNewSourcePartAttr )
                      != IDE_SUCCESS );

            if (( sNewTargetPartAttr->alterPart->partKeyCondMinValStr->length != 0 ) ||
                ( sNewSourcePartAttr->alterPart->partKeyCondMinValStr->length != 0 ))
            {
                // min value
                IDE_TEST_RAISE( qmoPartition::compareRangePartition(
                                    sTableInfo->partKeyColumns,
                                    sNewTargetPartAttr->alterPart->partCondMinVal,
                                    sNewSourcePartAttr->alterPart->partCondMinVal ) != 0,
                                ERR_REPLACE_DIFFERENT_PARTITION );
            }
            else
            {
                // nothing to do
            }

            if (( sNewTargetPartAttr->alterPart->partKeyCondMaxValStr->length != 0 ) ||
                ( sNewSourcePartAttr->alterPart->partKeyCondMaxValStr->length != 0 ))
            {
                // max value
                IDE_TEST_RAISE( qmoPartition::compareRangePartition(
                                    sTableInfo->partKeyColumns,
                                    sNewTargetPartAttr->alterPart->partCondMaxVal,
                                    sNewSourcePartAttr->alterPart->partCondMaxVal ) != 0,
                                ERR_REPLACE_DIFFERENT_PARTITION );
            }
            else
            {
                // nothing to do
            }
        }
        /* BUG-46065 support range using hash */
        else if ( sSrcTempPartInfo->partitionMethod == QCM_PARTITION_METHOD_RANGE_USING_HASH )
        {
            IDE_TEST( qdbCommon::makePartCondValList( aStatement,
                                                      sDstTempPartInfo,
                                                      sDstTempPartInfo->partitionID,
                                                      sNewTargetPartAttr )
                      != IDE_SUCCESS );

            IDE_TEST( qdbCommon::makePartCondValList( aStatement,
                                                      sSrcTempPartInfo,
                                                      sSrcTempPartInfo->partitionID,
                                                      sNewSourcePartAttr )
                      != IDE_SUCCESS );

            if (( sNewTargetPartAttr->alterPart->partKeyCondMinValStr->length != 0 ) ||
                ( sNewSourcePartAttr->alterPart->partKeyCondMinValStr->length != 0 ))
            {
                // min value
                IDE_TEST_RAISE( qmoPartition::compareRangeUsingHashPartition(
                                    sNewTargetPartAttr->alterPart->partCondMinVal,
                                    sNewSourcePartAttr->alterPart->partCondMinVal ) != 0,
                                ERR_REPLACE_DIFFERENT_PARTITION );
            }
            else
            {
                // nothing to do
            }

            if (( sNewTargetPartAttr->alterPart->partKeyCondMaxValStr->length != 0 ) ||
                ( sNewSourcePartAttr->alterPart->partKeyCondMaxValStr->length != 0 ))
            {
                // max value
                IDE_TEST_RAISE( qmoPartition::compareRangeUsingHashPartition(
                                    sNewTargetPartAttr->alterPart->partCondMaxVal,
                                    sNewSourcePartAttr->alterPart->partCondMaxVal ) != 0,
                                ERR_REPLACE_DIFFERENT_PARTITION );
            }
            else
            {
                // nothing to do
            }
        }
        else if( sSrcTempPartInfo->partitionMethod == QCM_PARTITION_METHOD_HASH )
        {
            // target partition order
            IDE_TEST( qcmPartition::getPartitionOrder(
                          QC_SMI_STMT( aStatement ),
                          sDstTempPartInfo->tableID,
                          (UChar *) sDstTempPartInfo->name,
                          idlOS::strlen( sDstTempPartInfo->name ),
                          & sTargetPartOrder )
                      != IDE_SUCCESS );

            // source partition order
            IDE_TEST( qcmPartition::getPartitionOrder(
                          QC_SMI_STMT( aStatement ),
                          sSrcTempPartInfo->tableID,
                          (UChar *) sSrcTempPartInfo->name,
                          idlOS::strlen( sSrcTempPartInfo->name ),
                          & sSourcePartOrder )
                      != IDE_SUCCESS );

            IDE_TEST_RAISE( sTargetPartOrder != sSourcePartOrder,
                            ERR_REPLACE_DIFFERENT_PARTITION );
        }
        else if( sTableInfo->partitionMethod == QCM_PARTITION_METHOD_LIST )
        {
            IDE_TEST( qdbCommon::makePartCondValList( aStatement,
                                                      sDstTempPartInfo,
                                                      sDstTempPartInfo->partitionID,
                                                      sNewTargetPartAttr )
                      != IDE_SUCCESS );

            IDE_TEST( qdbCommon::makePartCondValList( aStatement,
                                                      sSrcTempPartInfo,
                                                      sSrcTempPartInfo->partitionID,
                                                      sNewSourcePartAttr )
                      != IDE_SUCCESS );
                
            if ( sNewSourcePartAttr->alterPart->partKeyCondMaxValStr->length > 0 )
            {
                for( sMaxValCount = 0;
                     sMaxValCount < sNewSourcePartAttr->alterPart->partCondMaxVal->partCondValCount;
                     sMaxValCount++ )
                {
                    IDE_TEST_RAISE( qmoPartition::compareListPartition(
                                        sTableInfo->partKeyColumns,
                                        sNewTargetPartAttr->alterPart->partCondMaxVal,
                                        sNewSourcePartAttr->alterPart->partCondMaxVal->partCondValues[sMaxValCount] )
                                    != ID_TRUE,
                                    ERR_REPLACE_DIFFERENT_PARTITION );
                }
            }
            else
            {
                // nothing to do
            }
        }
        else
        {
            // Nothing to do
        }

        if ( QCG_GET_SESSION_IS_NEED_DDL_INFO( aStatement ) == ID_TRUE )
        {
            IDE_TEST( QC_QMP_MEM ( aStatement )->alloc( ID_SIZEOF(smOID) * 2, (void**)&sDDLTableOIDArray )
                      != IDE_SUCCESS);
            sDDLTableOIDArray[0] = sTableInfo->tableOID;
            sDDLTableOIDArray[1] = sSourceTableInfo->tableOID;

            IDE_TEST( aStatement->qmxMem->alloc( ID_SIZEOF(smOID) * 2 * 1, (void**)&sDDLPartOIDArray )
                      != IDE_SUCCESS);
            sDDLPartOIDArray[0] = sDstTempPartInfo->tableOID;
            sDDLPartOIDArray[1] = sSrcTempPartInfo->tableOID;

            qrc::setDDLSrcInfo( aStatement,
                                ID_TRUE,
                                2,
                                sDDLTableOIDArray,
                                1,
                                sDDLPartOIDArray );
        }
        else
        {
            /* Nothing to do */
        } 
    }
    else
    {
        // nothing to do
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_EXIST_PARTITION_TABLE )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDB_CANNOT_ALTER_REPLACE_PARTITON_NONE_PART ) );
    }
    IDE_EXCEPTION( ERR_NOT_EXIST_TABLE_PARTITION )
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET(ideSetErrorCode( qpERR_ABORT_QCM_NOT_EXIST_TABLE_PARTITION,
                                 sqlInfo.getErrMessage()) );
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION( ERR_REPLACE_DIFFERENT_PARTITION )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDB_REPLACE_DIFFERENT_PARTITION ) );
    }         
    IDE_EXCEPTION( ERR_NOT_COMPATIBLE_TYPE )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDN_NOT_COMPATIBLE_TYPE ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdbCopySwap::swapTablePartitionColumnID( qcStatement  * aStatement,
                                                qcmTableInfo * aTargetTablePartInfo,
                                                qcmTableInfo * aSourceTablePartInfo )
{
    smiColumnList * sSrcColumnList = NULL;
    smiColumnList * sDstColumnList = NULL;
    mtcColumn     * sSrcMtcColumns = NULL;
    mtcColumn     * sDstMtcColumns = NULL;
    UInt            sColCount      = 0;
    ULong           sAllocSize     = 0;
    qcmColumn     * sColumn1       = NULL;
    qcmColumn     * sColumn2       = NULL;
    UInt            i              = 0;

    sColCount = aSourceTablePartInfo->columnCount;

    IDE_TEST_RAISE( sColCount != aTargetTablePartInfo->columnCount, ERR_UNEXPECTED );

    sAllocSize = (ULong)sColCount * ID_SIZEOF(mtcColumn);
    IDE_TEST( aStatement->qmxMem->alloc( sAllocSize,
                                         (void**)&sSrcMtcColumns )
             != IDE_SUCCESS);
    IDE_TEST( aStatement->qmxMem->alloc( sAllocSize,
                                         (void**)&sDstMtcColumns )
             != IDE_SUCCESS);

    sAllocSize = (ULong)sColCount * ID_SIZEOF(smiColumnList);
    IDE_TEST( aStatement->qmxMem->alloc( sAllocSize,
                                         (void**)&sSrcColumnList )
              != IDE_SUCCESS );
    IDE_TEST( aStatement->qmxMem->alloc( sAllocSize,
                                         (void**)&sDstColumnList )
              != IDE_SUCCESS );

    for ( sColumn1 = aSourceTablePartInfo->columns, sColumn2 = aTargetTablePartInfo->columns, i = 0;
          sColumn1 != NULL;
          sColumn1 = sColumn1->next, sColumn2 = sColumn2->next, i++ )
    {
        idlOS::memcpy( &sSrcMtcColumns[i], sColumn1->basicInfo, ID_SIZEOF(mtcColumn) );
        sSrcMtcColumns[i].column.id = sColumn2->basicInfo->column.id;
        sSrcColumnList[i].column = (const smiColumn*)(&sSrcMtcColumns[i]);

        idlOS::memcpy( &sDstMtcColumns[i], sColumn2->basicInfo, ID_SIZEOF(mtcColumn) );
        sDstMtcColumns[i].column.id = sColumn1->basicInfo->column.id;
        sDstColumnList[i].column = (const smiColumn*)(&sDstMtcColumns[i]);

        if ( i == sColCount - 1 )
        {
            sSrcColumnList[i].next = NULL;
            sDstColumnList[i].next = NULL;
        }
        else
        {
            sSrcColumnList[i].next = &sSrcColumnList[i + 1];
            sDstColumnList[i].next = &sDstColumnList[i + 1];
        }
    }

    IDE_TEST( smiTable::modifyTableInfo( QC_SMI_STMT( aStatement ),
                                         aSourceTablePartInfo->tableHandle,
                                         (const smiColumnList *)sSrcColumnList,
                                         ID_SIZEOF(mtcColumn),
                                         NULL,
                                         0,
                                         SMI_TABLE_FLAG_UNCHANGE,
                                         SMI_TBSLV_DDL_DML,
                                         0,
                                         0,         /* Parallel Degree */
                                         ID_TRUE )  /* aIsInitRowTemplate */
              != IDE_SUCCESS );

    IDE_TEST( smiTable::modifyTableInfo( QC_SMI_STMT( aStatement ),
                                         aTargetTablePartInfo->tableHandle,
                                         (const smiColumnList *)sDstColumnList,
                                         ID_SIZEOF(mtcColumn),
                                         NULL,
                                         0,
                                         SMI_TABLE_FLAG_UNCHANGE,
                                         SMI_TBSLV_DDL_DML,
                                         0,
                                         0,         /* Parallel Degree */
                                         ID_TRUE )  /* aIsInitRowTemplate */
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_UNEXPECTED )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qdbCopySwap::swapTablePartitionColumnID",
                                  "column count is not same" ));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

