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
 * $Id: qdbFlashback.cpp 88963 2020-10-19 03:33:18Z jake.jang $
 **********************************************************************/

#include <idl.h>
#include <ide.h>
#include <qdbFlashback.h>
#include <qdbCommon.h>
#include <qdx.h>
#include <qcuProperty.h>
#include <qcuTemporaryObj.h>
#include <qcg.h>
#include <qcs.h>
#include <qcsModule.h>
#include <smiDef.h>
#include <qdpRole.h>
#include <qdpDrop.h>
#include <qcmDictionary.h>
#include <qdd.h>
#include <qdm.h>
#include <qcmProc.h>
#include <qcmPkg.h>
#include <qcmView.h>
#include <qcmAudit.h>

/* PROJ-2441 flashback */
IDE_RC qdbFlashback::dropToRecyclebin( qcStatement   * aStatement,
                                       qcmTableInfo  * aTableInfo )
{
/***********************************************************************
 *
 * Description :
 *      DROP TABLE (RECYCLEBIN ON) TO RECYCLEBIN
 * Implementation :
 *
 *      TABLENAME = TABLENAME || SYS_GUID_STR()
 *      TABLE_TYPE = 'R'
 *
 ***********************************************************************/

    SChar     * sSqlStr = NULL;
    vSLong      sRowCnt = 0;
    
    IDU_FIT_POINT_RAISE( "qdbFlashback::dropToRecyclebin::STRUCT_ALLOC::ERR",
                         ERR_MEMORY_ALLOCATION );
    IDE_TEST_RAISE( STRUCT_ALLOC_WITH_SIZE( aStatement->qmxMem,
                                            SChar,
                                            QD_MAX_SQL_LENGTH,
                                            &sSqlStr )
                    != IDE_SUCCESS, ERR_MEMORY_ALLOCATION );

    if ( QCU_RECYCLEBIN_FOR_NATC == 0 )
    {
        idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                         "UPDATE SYS_TABLES_ "
                         "SET TABLE_NAME = \"TABLE_NAME\"||SYS_GUID_STR(), "
                         "TABLE_TYPE = 'R', "
                         "LAST_DDL_TIME = SYSDATE "
                         "WHERE TABLE_ID = INTEGER'%"ID_INT32_FMT"'",
                         aTableInfo->tableID );
    }
    else
    {
        idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                         "UPDATE SYS_TABLES_ "
                         "SET TABLE_NAME = \"TABLE_NAME\"||'0000000000000000000000000000000'||'%"ID_INT32_FMT"', "
                         "TABLE_TYPE = 'R', "
                         "LAST_DDL_TIME = SYSDATE "
                         "WHERE TABLE_ID = INTEGER'%"ID_INT32_FMT"'",
                         QCU_RECYCLEBIN_FOR_NATC,
                         aTableInfo->tableID );
    }

    IDE_TEST( qcg::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                 sSqlStr,
                                 & sRowCnt ) != IDE_SUCCESS );

    IDE_TEST_RAISE( sRowCnt != 1, ERR_META_CRASH );

    /* function based index's hidden column HIDDEN -> F */
    idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                     "UPDATE SYS_COLUMNS_ "
                     "SET IS_HIDDEN = 'F' "
                     "WHERE TABLE_ID = INTEGER'%"ID_INT32_FMT"' "
                     "AND IS_HIDDEN = 'T' ",
                     aTableInfo->tableID );

    IDE_TEST( qcg::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                 sSqlStr,
                                 &sRowCnt) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_MEMORY_ALLOCATION )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_MEMORY_ALLOCATION ) );
    }
    IDE_EXCEPTION( ERR_META_CRASH )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QCM_META_CRASH ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdbFlashback::flashbackFromRecyclebin( qcStatement   * aStatement,
                                              qcNamePosition  aTableNamePos,
                                              UInt            aTableID )
{
/***********************************************************************
 *
 * Description :
 *      FLASHBACK TABLE ... TO BEFORE DROP ( TO RENAME ... )
 * Implementation :
 *
 *      TABLENAME = aTableNamePos
 *      TABLE_TYPE = 'T'
 *
 ***********************************************************************/

    SChar     * sSqlStr = NULL;
    SChar       sTableName[ QC_MAX_OBJECT_NAME_LEN + 1 ];
    vSLong      sRowCnt = 0;

    QC_STR_COPY( sTableName, aTableNamePos );

    IDU_FIT_POINT_RAISE( "qdbFlashback::flashbackFromRecyclebin::STRUCT_ALLOC::ERR",
                         ERR_MEMORY_ALLOCATION );
    IDE_TEST_RAISE( STRUCT_ALLOC_WITH_SIZE( aStatement->qmxMem,
                                            SChar,
                                            QD_MAX_SQL_LENGTH,
                                            &sSqlStr )
                    != IDE_SUCCESS, ERR_MEMORY_ALLOCATION );

    idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                     "UPDATE SYS_TABLES_ "
                     "SET TABLE_NAME = '%s', "
                     "TABLE_TYPE = 'T', "
                     "LAST_DDL_TIME = SYSDATE "  // fix BUG-14394
                     "WHERE TABLE_ID = INTEGER'%"ID_INT32_FMT"'",
                     sTableName,
                     aTableID );

    IDE_TEST( qcg::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                 sSqlStr,
                                 & sRowCnt ) != IDE_SUCCESS );

    IDE_TEST_RAISE( sRowCnt != 1, ERR_META_CRASH );

    return IDE_SUCCESS;
     
    IDE_EXCEPTION( ERR_MEMORY_ALLOCATION )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_MEMORY_ALLOCATION ) );
    }
    IDE_EXCEPTION( ERR_META_CRASH )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QCM_META_CRASH ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdbFlashback::validatePurgeTable( qcStatement * aStatement )
{
/***********************************************************************
 *
 * Description :
 *    PURGE TABLE ... 의 validation 수행
 *
 * Implementation :
 *    1. 테이블 존재 여부 체크
 *    2. 메타 테이블이면 에러
 *    3. PURGE OPERATABLE 체크 (TABLE_TYPE 'R')
 *    4. DROP 권한 체크
 *    5. 파티션드 테이블인 경우 하위 파티션 테이블 체크
 *
 ***********************************************************************/

    qdDropParseTree   * sParseTree = NULL;
    qcmTableInfo      * sInfo      = NULL;
    qcuSqlSourceInfo    sqlInfo;
    UInt                sTableID = 0;

    sParseTree = (qdDropParseTree *)aStatement->myPlan->parseTree;

    // original name to object name
    IDE_TEST( getTableIdByOriginalName( aStatement,
                                        sParseTree->objectName,
                                        QCG_GET_SESSION_USER_ID(aStatement),
                                        ID_TRUE, /* 오래된 테이블 */
                                        &sTableID )
              != IDE_SUCCESS );

    //original name use
    if ( sTableID != 0 )
    {
        IDE_TEST( qcm::getTableInfoByID( aStatement,
                                         sTableID,
                                         &sParseTree->tableInfo,
                                         &sParseTree->tableSCN,
                                         &sParseTree->tableHandle )
                  != IDE_SUCCESS );
    }
    else
    {
        // check table exist.
        IDE_TEST( qdbCommon::checkTableInfo( aStatement,
                                             sParseTree->userName,
                                             sParseTree->objectName,
                                             &(sParseTree->userID),
                                             &(sParseTree->tableInfo),
                                             &(sParseTree->tableHandle),
                                             &(sParseTree->tableSCN) )
                  != IDE_SUCCESS);
    }

    IDE_TEST( qcm::lockTableForDDLValidation( aStatement,
                                              sParseTree->tableHandle,
                                              sParseTree->tableSCN )
             != IDE_SUCCESS );

    sInfo = sParseTree->tableInfo;

    // PR-13725
    // CHECK OPERATABLE
    if ( QCM_IS_OPERATABLE_QP_PURGE( sInfo->operatableFlag ) != ID_TRUE )
    {
        sqlInfo.setSourceInfo( aStatement, &(sParseTree->objectName) );
        IDE_RAISE( ERR_NOT_EXIST_TABLE );
    }
    else
    {
        /* Nothing to do */
    }

    // check grant
    IDE_TEST( qdpRole::checkDDLDropTablePriv( aStatement,
                                              sParseTree->userID )
              != IDE_SUCCESS );

    // PROJ-1502 PARTITIONED DISK TABLE
    if ( sInfo->tablePartitionType == QCM_PARTITIONED_TABLE )
    {
        // 모든 파티션에 LOCK(IS)
        // 파티션 리스트를 파스트리에 달아놓는다.
        IDE_TEST( qdbCommon::checkAndSetAllPartitionInfo(
                      aStatement,
                      sInfo->tableID,
                      & (sParseTree->partInfoList) )
                  != IDE_SUCCESS );

        // PROJ-1624 non-partitioned index
        IDE_TEST( qdx::makeAndLockIndexTableList(
                      aStatement,
                      ID_FALSE,
                      sInfo,
                      &(sParseTree->oldIndexTables) )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_EXIST_TABLE )
    {
        (void)sqlInfo.init( aStatement->qmeMem );
        IDE_SET(
                 ideSetErrorCode( qpERR_ABORT_QCV_NOT_EXISTS_TABLE,
                                  sqlInfo.getErrMessage() ) );
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdbFlashback::executePurgeTable( qcStatement * aStatement )
{
/***********************************************************************
 *
 * Description :
 *    DROP TABLE ... 의 execution 수행
 *
 * Implementation :
 *    1. get table info
 *    2. 메타 테이블에서 constraint 삭제
 *    3. 메타 테이블에서 index 삭제
 *    4. 메타 테이블에서 table,column 삭제
 *    5. 메타 테이블에서 privilege 삭제
 *    -. 메타 테이블에서 trigger 삭제
 *    6. related PSM 을 invalid 상태로 변경
 *    7. related VIEW 을 invalid 상태로 변경
 *    8. Constraint와 관련된 Procedure에 대한 정보를 삭제
 *    9. Index와 관련된 Procedure에 대한 정보를 삭제
 *   10. smiTable::dropTable
 *   11. 메타 캐쉬에서 삭제
 *
 ***********************************************************************/

    qdDropParseTree         * sParseTree       = NULL;
    qcmTableInfo            * sInfo            = NULL;
    qcmPartitionInfoList    * sPartInfoList    = NULL;
    qcmPartitionInfoList    * sOldPartInfoList = NULL;
    qcmTableInfo            * sPartInfo        = NULL;
    idBool                    sIsPartitioned;
    qdIndexTableList        * sIndexTable      = NULL;
    qcmColumn               * sColumn          = NULL;
    qcmTableInfo            * sDicInfo         = NULL;

    sParseTree = (qdDropParseTree *)aStatement->myPlan->parseTree;

    // TASK-2176
    // Table에 대한 Lock을 획득한다.
    IDE_TEST( qcm::validateAndLockTable( aStatement,
                                         sParseTree->tableHandle,
                                         sParseTree->tableSCN,
                                         SMI_TABLE_LOCK_X )
              != IDE_SUCCESS );

    sInfo = sParseTree->tableInfo;

    // PROJ-1502 PARTITIONED DISK TABLE
    if ( sInfo->tablePartitionType == QCM_PARTITIONED_TABLE )
    {
        sIsPartitioned = ID_TRUE;

        // 파스트리에서 파티션 정보 리스트를 가져온다.
        sOldPartInfoList = sParseTree->partInfoList;

        // 모든 파티션에 LOCK(X)
        IDE_TEST( qcmPartition::validateAndLockPartitionInfoList( aStatement,
                                                                  sOldPartInfoList,
                                                                  SMI_TBSLV_DDL_DML, // TBS Validation 옵션
                                                                  SMI_TABLE_LOCK_X,
                                                                  smiGetDDLLockTimeOut((QC_SMI_STMT(aStatement))->getTrans()))
                  != IDE_SUCCESS );

        // PROJ-1624 non-partitioned index
        IDE_TEST( qdx::validateAndLockIndexTableList( aStatement,
                                                      sParseTree->oldIndexTables,
                                                      SMI_TBSLV_DDL_DML, // TBS Validation 옵션
                                                      SMI_TABLE_LOCK_X,
                                                      smiGetDDLLockTimeOut((QC_SMI_STMT(aStatement))->getTrans()))
                  != IDE_SUCCESS );
    }
    else
    {
        sIsPartitioned = ID_FALSE;
    }

    // PROJ-1407 Temporary table
    // session temporary table이 존재하는 경우 DDL을 할 수 없다.
    IDE_TEST_RAISE( qcuTemporaryObj::existSessionTable( sInfo ) == ID_TRUE,
                    ERR_SESSION_TEMPORARY_TABLE_EXIST );

    /* PROJ-1723 [MDW/INTEGRATOR] Altibase Plugin 개발
     DDL Statement Text의 로깅
     */
    if ( QCU_DDL_SUPPLEMENTAL_LOG == 1 )
    {
        IDE_TEST( qciMisc::writeDDLStmtTextLog( aStatement,
                                                sInfo->tableOwnerID,
                                                sInfo->name )
                  != IDE_SUCCESS );

        IDE_TEST( qci::mManageReplicationCallback.mWriteTableMetaLog(
                       aStatement,
                       smiGetTableId(sInfo->tableHandle ),
                       0 )
                  != IDE_SUCCESS);
    }
    else
    {
        /* Nothing to do */
    }

    // PROJ-1502 PARTITIONED DISK TABLE
    if ( sIsPartitioned == ID_TRUE )
    {
        // -----------------------------------------------------
        // 테이블 파티션 개수만큼 반복
        // -----------------------------------------------------
        for ( sPartInfoList = sOldPartInfoList;
              sPartInfoList != NULL;
              sPartInfoList = sPartInfoList->next )
        {
            sPartInfo = sPartInfoList->partitionInfo;

            IDE_TEST( qdd::dropTablePartition( aStatement,
                                               sPartInfo,
                                               ID_FALSE, /* aIsDropTablespace */
                                               ID_FALSE )
                      != IDE_SUCCESS );
        }

        // PROJ-1624 non-partitioned index
        for ( sIndexTable = sParseTree->oldIndexTables;
              sIndexTable != NULL;
              sIndexTable = sIndexTable->next )
        {
            IDE_TEST( qdx::dropIndexTable( aStatement,
                                           sIndexTable,
                                           ID_FALSE /* aIsDropTablespace */ )
                      != IDE_SUCCESS );
        }
    }
    else
    {
        // Nothing to do
    }

    IDE_TEST( qdd::deleteIndicesFromMeta( aStatement, sInfo )
              != IDE_SUCCESS );
    
    IDE_TEST( qdd::deleteTableFromMeta( aStatement, sInfo->tableID )
              != IDE_SUCCESS );

    IDE_TEST( qdpDrop::removePriv4DropTable( aStatement, sInfo->tableID )
              != IDE_SUCCESS );

    /* PROJ-2197 PSM Renewal */
    // related PSM
    IDE_TEST(qcmProc::relSetInvalidProcOfRelated(
                aStatement,
                sParseTree->userID,
                (SChar *) (sParseTree->objectName.stmtText + sParseTree->objectName.offset),
                sParseTree->objectName.size,
                QS_TABLE) != IDE_SUCCESS);

    // PROJ-1073 Package
    IDE_TEST(qcmPkg::relSetInvalidPkgOfRelated(
                aStatement,
                sParseTree->userID,
                (SChar *) (sParseTree->objectName.stmtText + sParseTree->objectName.offset),
                sParseTree->objectName.size,
                QS_TABLE) != IDE_SUCCESS);

    // related VIEW
    IDE_TEST(qcmView::setInvalidViewOfRelated(
                 aStatement,
                 sParseTree->userID,
                 (SChar *) (sParseTree->objectName.stmtText + sParseTree->objectName.offset),
                 sParseTree->objectName.size,
                 QS_TABLE) != IDE_SUCCESS);

    // PROJ-2223 audit
    IDE_TEST( qcmAudit::deleteObject( aStatement,
                                      sParseTree->tableInfo->tableOwnerID,
                                      sParseTree->tableInfo->name )
              != IDE_SUCCESS );

    // PROJ-2264 Dictionary table
    // SYS_COMPRESSION_TABLES_ 에서 관련 레코드를 삭제한다.
    IDE_TEST( qdd::deleteCompressionTableSpecFromMeta( aStatement,
                                                       sInfo->tableID )
              != IDE_SUCCESS );
    
    IDE_TEST( smiTable::dropTable( QC_SMI_STMT( aStatement ),
                                   sInfo->tableHandle,
                                   SMI_TBSLV_DDL_DML )
              != IDE_SUCCESS );

    // PROJ-2264 Dictionary table
    // Drop all dictionary tables
    for ( sColumn  = sInfo->columns;
          sColumn != NULL;
          sColumn  = sColumn->next )
    {
        if ( ( sColumn->basicInfo->column.flag & SMI_COLUMN_COMPRESSION_MASK )
             == SMI_COLUMN_COMPRESSION_TRUE )
        {
            IDE_TEST( qcmDictionary::removeDictionaryTable( aStatement,
                                                            sColumn )
                      != IDE_SUCCESS );
        }
        else
        {
            // Not compression column
            // Nothing to do.
        }
    }

    // BUG-36719
    // Table info 를 삭제한다.
    for ( sColumn  = sInfo->columns;
          sColumn != NULL;
          sColumn  = sColumn->next )
    {
        if ( ( sColumn->basicInfo->column.flag & SMI_COLUMN_COMPRESSION_MASK )
             == SMI_COLUMN_COMPRESSION_TRUE )
        {
            sDicInfo = (qcmTableInfo *)smiGetTableRuntimeInfoFromTableOID(
                                           sColumn->basicInfo->column.mDictionaryTableOID );

            (void)qcm::destroyQcmTableInfo( sDicInfo );
        }
        else
        {
            // Not compression column
            // Nothing to do.
        }
    }

    (void)qcm::destroyQcmTableInfo( sInfo );

    if ( sIsPartitioned == ID_TRUE )
    {
        (void)qcmPartition::destroyQcmPartitionInfoList( sOldPartInfoList );

        for ( sIndexTable = sParseTree->oldIndexTables;
              sIndexTable != NULL;
              sIndexTable = sIndexTable->next )
        {
            (void)qcm::destroyQcmTableInfo( sIndexTable->tableInfo );
        }
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_SESSION_TEMPORARY_TABLE_EXIST )
    {
        IDE_SET(ideSetErrorCode( qpERR_ABORT_QDB_TEMPORARY_TABLE_DDL_DISABLE ));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdbFlashback::validateFlashbackTable( qcStatement * aStatement )
{
/***********************************************************************
 *
 * Description :
 *    FLASHBACK TABLE ... TO BEFORE DROP ( RENAME TO ... )
 *
 * Implementation :
 *    1. 테이블 존재 여부 체크
 *    2. 테이블의 소유자가 SYSTEM_ 이면 불가능
 *    3. CREATE table 권한이 있는지 체크
 *    4. 테이블에 이중화가 걸려있으면 에러
 *       새이름 생성 (RENMAE OR ORIGINAL NAME )
 *    5. 변경후의 테이블 이름이 이미 존재하는지 체크
 *
 ***********************************************************************/

    qdTableParseTree    * sParseTree  = NULL;
    qcmTableInfo        * sInfo       = NULL;
    qsOID                 sProcID     = 0;
    UInt                  sUserID     = 0;
    qcuSqlSourceInfo      sqlInfo;
    idBool                sExist;
    qcNamePosition        sNewTableNamePos;
    UInt                  sTableID    = 0;
    SChar                 sObjectNameBuf[ QC_MAX_OBJECT_NAME_LEN + 1 ] = { 0, };

    sParseTree = (qdTableParseTree *)aStatement->myPlan->parseTree;

    // orignal name to object name
    IDE_TEST( getTableIdByOriginalName( aStatement,
                                        sParseTree->tableName,
                                        QCG_GET_SESSION_USER_ID(aStatement),
                                        ID_FALSE, /*최근 드랍된 테이블*/
                                        &sTableID )
              != IDE_SUCCESS );

    if ( sTableID != 0 )
    {
        IDE_TEST( qcm::getTableInfoByID( aStatement,
                                         sTableID,
                                         &sParseTree->tableInfo,
                                         &sParseTree->tableSCN,
                                         &sParseTree->tableHandle )
                  != IDE_SUCCESS );

        sParseTree->useOriginalName = ID_TRUE;
    }
    else
    {
        // check table exist.
        IDE_TEST( qdbCommon::checkTableInfo( aStatement,
                                             sParseTree->userName,
                                             sParseTree->tableName,
                                             &(sParseTree->userID),
                                             &(sParseTree->tableInfo),
                                             &(sParseTree->tableHandle),
                                             &(sParseTree->tableSCN) )
                 != IDE_SUCCESS);
        sParseTree->useOriginalName = ID_FALSE;
    }

    IDE_TEST( qcm::lockTableForDDLValidation( aStatement,
                                              sParseTree->tableHandle,
                                              sParseTree->tableSCN )
              != IDE_SUCCESS );

    sInfo = sParseTree->tableInfo;

    // PR-13725
    // CHECK OPERATABLE
    if ( QCM_IS_OPERATABLE_QP_FLASHBACK( sInfo->operatableFlag ) != ID_TRUE )
    {
        sqlInfo.setSourceInfo( aStatement, &(sParseTree->tableName) );
        IDE_RAISE( ERR_NOT_EXIST_TABLE );
    }
    else
    {
        /* Nothing to do */
    }

    // check grant
    IDE_TEST( qdpRole::checkDDLCreateTablePriv( aStatement,
                                                sParseTree->userID )
              != IDE_SUCCESS );

    //check new name exist
    if ( QC_IS_NULL_NAME( sParseTree->newTableName ) == ID_TRUE )
    {
        if ( sParseTree->useOriginalName == ID_TRUE )
        {
            // original name user ==> new name is tableName
            // if specified new object exists, then error
            IDE_TEST( qcm::existObject( aStatement,
                                        ID_FALSE,
                                        sParseTree->userName,
                                        sParseTree->tableName,
                                        QS_OBJECT_MAX,
                                        &sUserID,
                                        &sProcID,
                                        &sExist )
                      != IDE_SUCCESS);
        }
        else
        {
            //full name use ==> 32byte trim
            QC_STR_COPY( sObjectNameBuf, sParseTree->tableName );
            sNewTableNamePos.stmtText = sObjectNameBuf;
            sNewTableNamePos.offset = 0;
            sNewTableNamePos.size = idlOS::strlen( sObjectNameBuf ) - QDB_RECYCLEBIN_GENERATED_NAME_POSTFIX_LEN;
            
            // if specified new object exists, then error
            IDE_TEST( qcm::existObject( aStatement,
                                        ID_FALSE,
                                        sParseTree->userName,
                                        sNewTableNamePos,
                                        QS_OBJECT_MAX,
                                        &sUserID,
                                        &sProcID,
                                        &sExist )
                      != IDE_SUCCESS);
        }

    }
    else
    {
        // BUG-34388
        if (qdbCommon::containDollarInName( &(sParseTree->newTableName) ) == ID_TRUE)
        {
            sqlInfo.setSourceInfo( aStatement, &(sParseTree->newTableName) );
            IDE_RAISE(CANT_USE_RESERVED_WORD);
        }
        else
        {
            /* Nothing to do */
        }

        // if specified new object exists, then error
        IDE_TEST( qcm::existObject( aStatement,
                                    ID_FALSE,
                                    sParseTree->userName,
                                    sParseTree->newTableName,
                                    QS_OBJECT_MAX,
                                    &sUserID,
                                    &sProcID,
                                    &sExist )
                  != IDE_SUCCESS);
    }

    IDE_TEST_RAISE( sExist == ID_TRUE, ERR_EXIST_OBJECT_NAME );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_EXIST_TABLE )
    {
        (void)sqlInfo.init( aStatement->qmeMem );
        IDE_SET(
                 ideSetErrorCode( qpERR_ABORT_QCV_NOT_EXISTS_TABLE,
                                  sqlInfo.getErrMessage() ) );
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION(ERR_EXIST_OBJECT_NAME);
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDB_EXIST_OBJECT_NAME ) );
    }
    IDE_EXCEPTION( CANT_USE_RESERVED_WORD )
    {
        (void)sqlInfo.init( aStatement->qmeMem );
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDB_RESERVED_WORD_IN_OBJECT_NAME,
                                  sqlInfo.getErrMessage() ) );
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdbFlashback::executeFlashbackTable( qcStatement * aStatement )
{
/***********************************************************************
 *
 * Description :
 *    FLASHBACK TABLE ... TO BEFORE DROP (RENAME TO ...)
 *
 * Implementation :
 *    1. SYS_TABLES_ 에서 테이블 이름 변경
 *    2. qcm::touchTable
 *    3. 메타 캐쉬 재구성
 *
 ***********************************************************************/

    qdTableParseTree     * sParseTree       = NULL;
    UInt                   sTableID         = 0;
    smOID                  sTableOID        = 0;
    qcmTableInfo         * sOldTableInfo    = NULL;
    smSCN                  sSCN             = SM_SCN_INIT;
    qcmTableInfo         * sTempTableInfo   = NULL;
    void                 * sTableHandle     = NULL;
    qcNamePosition         sNewTableNamePos;
    SChar                  sObjectNameBuf[ QC_MAX_OBJECT_NAME_LEN + 1 ] = { 0, };

    SM_INIT_SCN(&sSCN);

    sParseTree = (qdTableParseTree *)aStatement->myPlan->parseTree;

    // TASK-2176
    // Table에 대한 Lock을 획득한다.
    IDE_TEST( qcm::validateAndLockTable( aStatement,
                                         sParseTree->tableHandle,
                                         sParseTree->tableSCN,
                                         SMI_TABLE_LOCK_X )
             != IDE_SUCCESS);

    sOldTableInfo = sParseTree->tableInfo;

    if ( QC_IS_NULL_NAME( sParseTree->newTableName ) == ID_TRUE )
    {
        if ( sParseTree->useOriginalName == ID_TRUE )
        {
            // original name user ==> new name is tableName
            IDE_TEST( flashbackFromRecyclebin( aStatement,
                                               sParseTree->tableName,
                                               sOldTableInfo->tableID )
                      != IDE_SUCCESS );
        }
        else
        {
            // full name use ==> 32byte trim
            QC_STR_COPY( sObjectNameBuf, sParseTree->tableName );
            sNewTableNamePos.stmtText = sObjectNameBuf;
            sNewTableNamePos.offset = 0;
            sNewTableNamePos.size = idlOS::strlen( sObjectNameBuf ) - QDB_RECYCLEBIN_GENERATED_NAME_POSTFIX_LEN;

            IDE_TEST( flashbackFromRecyclebin( aStatement,
                                               sNewTableNamePos,
                                               sOldTableInfo->tableID )
                      != IDE_SUCCESS );
        }
    }
    else
    {
        IDE_TEST( flashbackFromRecyclebin( aStatement,
                                           sParseTree->newTableName,
                                           sOldTableInfo->tableID )
                  != IDE_SUCCESS );
    }

    sTableID = sParseTree->tableInfo->tableID;
    sTableOID = smiGetTableId( sParseTree->tableInfo->tableHandle );

    IDE_TEST( qcm::touchTable( QC_SMI_STMT( aStatement ),
                               sTableID,
                               SMI_TBSLV_DDL_DML )
              != IDE_SUCCESS);

    IDE_TEST( qcm::makeAndSetQcmTableInfo( QC_SMI_STMT( aStatement ),
                                           sTableID,
                                           sTableOID )
              != IDE_SUCCESS );

    IDE_TEST( qcm::getTableInfoByID( aStatement,
                                     sTableID,
                                     &sTempTableInfo,
                                     &sSCN,
                                     &sTableHandle )
              != IDE_SUCCESS);

    // PROJ-1442 Table Meta Log Record 기록
    if ( QCU_DDL_SUPPLEMENTAL_LOG == 1 )
    {
        IDE_TEST( qci::mManageReplicationCallback.mWriteTableMetaLog(
                      aStatement,
                      sTableOID,
                      sTableOID )
                  != IDE_SUCCESS);
    }
    else
    {
        /* Nothing to do */
    }

    (void)qcm::destroyQcmTableInfo( sOldTableInfo );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    (void)qcm::destroyQcmTableInfo( sTempTableInfo );

    // on failure, restore tempinfo.
    qcmPartition::restoreTempInfo( sOldTableInfo,
                                   NULL,
                                   NULL );

    return IDE_FAILURE;
}

IDE_RC qdbFlashback::getRecyclebinUsage( qcStatement * aStatement,
                                         ULong       * aMemUseage,
                                         ULong       * aDiskUseage )
{
    SChar        * sSqlStr         = NULL;
    idBool         sRecordExist;
    mtdBigintType  sResultMemSize  = 0;
    mtdBigintType  sResultDiskSize = 0;
    
    IDU_FIT_POINT_RAISE( "qdbFlashback::getRecyclebinUsage::STRUCT_ALLOC::ERR",
                         ERR_MEMORY_ALLOCATION );
    IDE_TEST_RAISE( STRUCT_ALLOC_WITH_SIZE( aStatement->qmxMem,
                                            SChar,
                                            QD_MAX_SQL_LENGTH,
                                            &sSqlStr )
                    != IDE_SUCCESS, ERR_MEMORY_ALLOCATION );

    idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
         "SELECT NVL(SUM(MEMORY_SIZE), 0) AS BIGINT, NVL(SUM(DISK_SIZE), 0) AS BIGINT FROM SYSTEM_.SYS_RECYCLEBIN_ " );

    IDE_TEST( qcg::runSelectOneRowMultiColumnforDDL( QC_SMI_STMT( aStatement ),
                                                     sSqlStr,
                                                     &sRecordExist,
                                                     2,
                                                     &sResultMemSize,
                                                     &sResultDiskSize )
             != IDE_SUCCESS );

    IDE_TEST_RAISE( sRecordExist == ID_FALSE,
                    ERR_META_CRASH );

    *aMemUseage  = (ULong) (sResultMemSize);
    *aDiskUseage = (ULong) (sResultDiskSize);

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_MEMORY_ALLOCATION )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_MEMORY_ALLOCATION ) );
    }
    IDE_EXCEPTION( ERR_META_CRASH );
    {
        IDE_SET( ideSetErrorCode ( qpERR_ABORT_QCM_META_CRASH ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdbFlashback::checkRecyclebinSpace( qcStatement  * aStatement,
                                           qcmTableInfo * aTableInfo,
                                           idBool       * aIsMemAvailable,
                                           idBool       * aIsDiskAvailable )
{
    idBool                 sMemAvailable        = ID_TRUE;
    idBool                 sDiskAvailable       = ID_TRUE;
    ULong                  sBlockCount          = 0;
    ULong                  sMemTotalBlcokCount  = 0;
    ULong                  sDiskTotalBlcokCount = 0;
    ULong                  sMemUseage           = 0;
    ULong                  sDiskUseage          = 0;
    void                 * sTableHandle         = NULL;
    qdDropParseTree      * sParseTree           = NULL;
    qcmPartitionInfoList * sPartInfoList        = NULL;
    
    IDE_TEST( getRecyclebinUsage( aStatement, &sMemUseage, &sDiskUseage )
              != IDE_SUCCESS );

    IDE_DASSERT( QCG_GET_SESSION_RECYCLEBIN_ENABLE( aStatement ) == QCU_RECYCLEBIN_ON );

    sParseTree = (qdDropParseTree *)aStatement->myPlan->parseTree;

    if ( aTableInfo->tablePartitionType == QCM_PARTITIONED_TABLE )
    {
        /* for hybrid partition */

        /* PARTITIONED TABLE SIZE */
        sTableHandle = aTableInfo->tableHandle;
        if ( smiIsDiskTable( sTableHandle ) == ID_TRUE )
        {
            IDE_TEST( smiGetTableBlockCount( sTableHandle, &sBlockCount )
                      != IDE_SUCCESS );
            sDiskTotalBlcokCount += sBlockCount;
        }
        else
        {
            /* Nothing to do */
        }

        if ( smiIsMemTable( sTableHandle ) == ID_TRUE )
        {
            IDE_TEST( smiGetTableBlockCount( sTableHandle, &sBlockCount )
                      != IDE_SUCCESS );
            sMemTotalBlcokCount += sBlockCount;
        }
        else
        {
            /* Nothing to do */
        }

        /* TABLE PARTITION SIZE */
        for ( sPartInfoList = sParseTree->partInfoList;
              sPartInfoList != NULL;
              sPartInfoList = sPartInfoList->next )
        {
            sTableHandle = sPartInfoList->partHandle;

            IDE_TEST( smiGetTableBlockCount( sTableHandle, &sBlockCount )
                      != IDE_SUCCESS );

            if ( smiIsDiskTable( sTableHandle ) == ID_TRUE )
            {
                sDiskTotalBlcokCount += sBlockCount;
            }
            else
            {
                /* Nothing to do */
            }

            if ( smiIsMemTable( sTableHandle ) == ID_TRUE )
            {
                sMemTotalBlcokCount += sBlockCount;
            }
            else
            {
                /* Nothing to do */
            }
        }

        /* CHECK */
        if ( sDiskTotalBlcokCount != 0 )
        {

            if ( QCU_RECYCLEBIN_DISK_MAX_SIZE < ( sDiskTotalBlcokCount * (SD_PAGE_SIZE) + sDiskUseage ) )
            {
                sDiskAvailable = ID_FALSE;
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

        if ( sMemTotalBlcokCount != 0 )
        {
            if ( QCU_RECYCLEBIN_MEM_MAX_SIZE < ( sMemTotalBlcokCount * (SM_PAGE_SIZE) + sMemUseage ) )
            {
                sMemAvailable = ID_FALSE;
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
        sTableHandle = aTableInfo->tableHandle;

        if ( smiIsDiskTable( sTableHandle ) == ID_TRUE )
        {
            IDE_TEST( smiGetTableBlockCount( sTableHandle, &sBlockCount )
                      != IDE_SUCCESS );

            if ( QCU_RECYCLEBIN_DISK_MAX_SIZE < ( sBlockCount * (SD_PAGE_SIZE) + sDiskUseage ) )
            {
                sDiskAvailable = ID_FALSE;
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

        if ( smiIsMemTable( sTableHandle ) == ID_TRUE )
        {
            IDE_TEST( smiGetTableBlockCount( sTableHandle, &sBlockCount )
                      != IDE_SUCCESS );

            if ( QCU_RECYCLEBIN_MEM_MAX_SIZE < ( sBlockCount * (SM_PAGE_SIZE) + sMemUseage ) )
            {
                sMemAvailable = ID_FALSE;
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

    *aIsMemAvailable  = sMemAvailable;
    *aIsDiskAvailable = sDiskAvailable;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdbFlashback::getTableIdByOriginalName( qcStatement    * aStatement,
                                               qcNamePosition   aOriginalNamePos,
                                               UInt             aUserID,
                                               idBool           aIsOLD,
                                               UInt           * aTableID )
{
    SChar           * sSqlStr        = NULL;
    idBool            sRecordExist;
    mtdIntegerType    sTableId       = 0;
    SChar             sOriginalNameStrBuf[ QC_MAX_OBJECT_NAME_LEN + 1 ] = { 0, };

    QC_STR_COPY( sOriginalNameStrBuf, aOriginalNamePos );
    
    IDU_FIT_POINT_RAISE( "qdbFlashback::getTableIdByOriginalName::STRUCT_ALLOC::ERR",
                         ERR_MEMORY_ALLOCATION );
    IDE_TEST_RAISE( STRUCT_ALLOC_WITH_SIZE( aStatement->qmxMem,
                        SChar,
                        QD_MAX_SQL_LENGTH,
                        &sSqlStr )
                    != IDE_SUCCESS, ERR_MEMORY_ALLOCATION );

    if ( aIsOLD == ID_TRUE )
    {
        idlOS::snprintf( sSqlStr,
                         QD_MAX_SQL_LENGTH,
                         "SELECT TABLE_ID AS INTEGER "
                         "FROM SYSTEM_.SYS_TABLES_ "
                         "WHERE TABLE_NAME LIKE '%s%%' "
                         "AND LENGTHB(TABLE_NAME) = INTEGER'%"ID_INT32_FMT"' "
                         "AND TABLE_TYPE = 'R' "
                         "AND USER_ID = INTEGER'%"ID_INT32_FMT"' "
                         "ORDER BY LAST_DDL_TIME ASC LIMIT 1",
                         sOriginalNameStrBuf,
                         aOriginalNamePos.size + QDB_RECYCLEBIN_GENERATED_NAME_POSTFIX_LEN,
                         aUserID
                        );
    }
    else
    {
        idlOS::snprintf( sSqlStr,
                         QD_MAX_SQL_LENGTH,
                         "SELECT TABLE_ID AS INTEGER "
                         "FROM SYSTEM_.SYS_TABLES_ "
                         "WHERE TABLE_NAME LIKE '%s%%' "
                         "AND LENGTHB(TABLE_NAME) = INTEGER'%"ID_INT32_FMT"' "
                         "AND TABLE_TYPE = 'R' "
                         "AND USER_ID = INTEGER'%"ID_INT32_FMT"' "
                         "ORDER BY LAST_DDL_TIME DESC LIMIT 1",
                         sOriginalNameStrBuf,
                         aOriginalNamePos.size + QDB_RECYCLEBIN_GENERATED_NAME_POSTFIX_LEN,
                         aUserID
                        );
    }

    IDE_TEST( qcg::runSelectOneRowforDDL( QC_SMI_STMT( aStatement ),
                                          sSqlStr,
                                          &sTableId,
                                          &sRecordExist,
                                          ID_FALSE )
              != IDE_SUCCESS );
    if ( sRecordExist == ID_TRUE )
    {
        *aTableID = (UInt) sTableId;
    }
    else
    {
        *aTableID = 0;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_MEMORY_ALLOCATION )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_MEMORY_ALLOCATION ) );
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
