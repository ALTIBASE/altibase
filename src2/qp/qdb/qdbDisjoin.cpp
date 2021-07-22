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
#include <mtcDef.h>
#include <qdbCommon.h>
#include <qdbDisjoin.h>
#include <qcm.h>
#include <qcmPartition.h>
#include <qcg.h>
#include <smiDef.h>
#include <qcmTableSpace.h>
#include <qdpDrop.h>
#include <qdnTrigger.h>
#include <qdd.h>
#include <qcmProc.h>
#include <qcmPkg.h>
#include <qcmView.h>
#include <qcmAudit.h>
#include <qdpRole.h>
#include <qdbComment.h>
#include <sdi.h>

IDE_RC qdbDisjoin::validateDisjoinTable( qcStatement * aStatement )
{
/***********************************************************************
 *
 * Description :
 *    DISJOIN TABLE ... ( PARTITION ... TO TABLE ... , ... )
 *
 * Implementation :
 *    1. 테이블 dollar name 체크
 *    2. 변경 후 생성되는 테이블들의 이름이 이미 존재하는지 체크
 *    3. 테이블 존재 여부 체크
 *       3-1. 테이블에 LOCK(IS)
 *    4. 파티션드 테이블 아니면 에러
 *       4-1. 해시 파티션드 테이블이면 에러
 *    5. 파티션 이름이 제대로 적힌 건지 체크
 *       5.1. Hybrid Partitioned Table이면 에러
 *    6. ACCESS가 READ_WRITE인 테이블만 가능
 *    7. statement에 테이블의 파티션 전부 입력했는지 확인
 *    8. 인덱스, PK, UK, FK가 있으면 에러
 *    9. 테이블에 이중화가 걸려있으면 에러
 *    10. 권한, operatable 체크 (CREATE & DROP)
 *    11. compression 칼럼이 있으면 에러
 *    12. hidden 칼럼이 있으면 에러
 *    13. 보안 칼럼이 있으면 에러
 *    14. 자신에 의해 이벤트가 발생하는 트리거가 있으면 에러(SYS_TRIGGERS_에 TABLE ID가 있으면 에러)
 *
 ***********************************************************************/

    qdDisjoinTableParseTree  * sParseTree = NULL;
    qcuSqlSourceInfo           sqlInfo;
    qcmTableInfo             * sTableInfo = NULL;
    qcmColumn                * sColumn = NULL;
    qdDisjoinTable           * sDisjoin = NULL;
    qsOID                      sProcID = QS_EMPTY_OID;
    idBool                     sExist = ID_FALSE;
    UInt                       sPartCount = 0;

    SInt                       sCountDiskType = 0;
    SInt                       sCountMemType  = 0;
    SInt                       sCountVolType  = 0;
    SInt                       sTotalCount    = 0;
    UInt                       sTableType     = 0;

    sParseTree = (qdDisjoinTableParseTree *)aStatement->myPlan->parseTree;

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

    /* 2. 변경 후 생성되는 테이블들의 이름이 이미 존재하는지 체크 */
    for ( sDisjoin = sParseTree->disjoinTable;
          sDisjoin != NULL;
          sDisjoin = sDisjoin->next )
    {
        IDE_TEST( qcm::existObject( aStatement,
                                    ID_FALSE,
                                    sParseTree->userName,   /* empty name */
                                    sDisjoin->newTableName,
                                    QS_OBJECT_MAX,
                                    &(sParseTree->userID),
                                    &sProcID,
                                    &sExist )
                  != IDE_SUCCESS );

        if ( sExist == ID_TRUE )
        {
            sqlInfo.setSourceInfo( aStatement,
                                   &(sDisjoin->newTableName) );
            IDE_RAISE( ERR_DUPLICATE_TABLE_NAME );
        }
        else
        {
            /* Nothing To Do */
        }
    }

    /* 3. 테이블 존재 여부 체크 */
    IDE_TEST( qdbCommon::checkTableInfo( aStatement,
                                         sParseTree->userName,  /* empty */
                                         sParseTree->tableName,
                                         &(sParseTree->userID),
                                         &(sParseTree->tableInfo),
                                         &(sParseTree->tableHandle),
                                         &(sParseTree->tableSCN) )
              != IDE_SUCCESS);

    /* BUG-48290 shard object에 대한 DDL 차단 */
    IDE_TEST( sdi::checkShardObjectForDDL( aStatement, SDI_DDL_TYPE_DISJOIN ) != IDE_SUCCESS );

    // 파티션드 테이블에 LOCK(IS)
    IDE_TEST( smiValidateAndLockObjects( ( QC_SMI_STMT( aStatement ) )->getTrans(),
                                         sParseTree->tableHandle,
                                         sParseTree->tableSCN,
                                         SMI_TBSLV_DROP_TBS, // TBS Validation 옵션
                                         SMI_TABLE_LOCK_IS,
                                         smiGetDDLLockTimeOut((QC_SMI_STMT(aStatement))->getTrans()),
                                         ID_FALSE )
              != IDE_SUCCESS );

    sTableInfo = sParseTree->tableInfo;

    IDE_TEST( qcmPartition::getPartitionInfoList( aStatement,
                                                  QC_SMI_STMT( aStatement ),
                                                  QC_QMP_MEM( aStatement ),
                                                  sTableInfo->tableID,
                                                  & (sParseTree->partInfoList) )
              != IDE_SUCCESS );

    // 모든 파티션에 LOCK(IS)
    IDE_TEST( qcmPartition::validateAndLockPartitionInfoList( aStatement,
                                                              sParseTree->partInfoList,
                                                              SMI_TBSLV_DROP_TBS, // TBS Validation 옵션
                                                              SMI_TABLE_LOCK_IS,
                                                              smiGetDDLLockTimeOut((QC_SMI_STMT(aStatement))->getTrans()))
              != IDE_SUCCESS );

    /* PROJ-2464 hybrid partitioned table 지원 */
    sTableType = sTableInfo->tableFlag & SMI_TABLE_TYPE_MASK;

    /* 4. 파티션드 테이블 아니면 에러 */
    IDE_TEST_RAISE( sTableInfo->tablePartitionType  == QCM_NONE_PARTITIONED_TABLE,
                    ERR_DISJOIN_TABLE_NON_PART_TABLE );
    /* 4-1. 해시 파티션드 테이블이면 에러 */
    IDE_TEST_RAISE( sTableInfo->partitionMethod == QCM_PARTITION_METHOD_HASH,
                    ERR_DISJOIN_TABLE_NON_PART_TABLE );

    /* 5. 파티션 이름이 제대로 적힌 건지(존재하는지), 파티션 전체 이름을 썼는지 체크 */
    /*    + partition info list 가져와서 parse tree에 달아 놓는다. */
    IDE_TEST( checkPartitionExistByName( aStatement,
                                         sParseTree->partInfoList,
                                         sParseTree->disjoinTable )
              != IDE_SUCCESS );

    /* PROJ-2464 hybrid partitioned table 지원
     *  - 5.1. Hybrid Partitioned Table이면 에러
     */
    /* 5.1.1. Partition 구성을 검사한다. */
    qdbCommon::getTableTypeCountInPartInfoList( & sTableType,
                                                sParseTree->partInfoList,
                                                & sCountDiskType,
                                                & sCountMemType,
                                                & sCountVolType );

    /* 5.1.2. Hybrid Partitioned Table 에서는 Disjoin Table를 지원하지 않는다. */
    sTotalCount = sCountDiskType + sCountMemType + sCountVolType;

    /* 5.1.3. 모두 같은 Type인지 검사한다. Hybrid Partitioned Table가 아니다. */
    IDE_TEST_RAISE( !( ( sTotalCount == sCountDiskType ) ||
                       ( sTotalCount == sCountMemType ) ||
                       ( sTotalCount == sCountVolType ) ),
                    ERR_UNSUPPORT_ON_HYBRID_PARTITIONED_TABLE );
    
    /* TASK-7307 DML Data Consistency in Shard
     *   USABLE인 테이블만 가능: ACCESS 옵션을 따라함 */
    if ( ( QCG_CHECK_SHARD_DML_CONSISTENCY( aStatement ) == ID_TRUE ) &&
         ( sTableInfo->mIsUsable != ID_TRUE ) )
    {
        sqlInfo.setSourceInfo( aStatement,
                               &(sParseTree->tableName) );
        IDE_RAISE( ERR_JOIN_DISJOIN_NOT_USABLE );
    }
    else
    {
        /* Nothing To Do */
    }

    /* 6. READ_WRITE인 테이블만 가능 */
    if ( sTableInfo->accessOption != QCM_ACCESS_OPTION_READ_WRITE )
    {
        sqlInfo.setSourceInfo( aStatement,
                               &(sParseTree->tableName) );
        IDE_RAISE( ERR_JOIN_DISJOIN_NOT_READ_WRITE );
    }
    else
    {
        /* Nothing To Do */
    }

    /* TASK-7307 USABLE인 테이블만 가능 */
    /* 6-1. READ_WRITE인 파티션만 가능 */
    for ( sDisjoin = sParseTree->disjoinTable;
          sDisjoin != NULL;
          sDisjoin = sDisjoin->next )
    {
        if ( ( QCG_CHECK_SHARD_DML_CONSISTENCY( aStatement ) == ID_TRUE ) &&
             ( sDisjoin->oldPartInfo->mIsUsable != ID_TRUE ) )
        {
            sqlInfo.setSourceInfo( aStatement,
                                   &(sDisjoin->oldPartName) );
            IDE_RAISE( ERR_JOIN_DISJOIN_NOT_USABLE );
        }
        else if ( sDisjoin->oldPartInfo->accessOption != QCM_ACCESS_OPTION_READ_WRITE )
        {
            sqlInfo.setSourceInfo( aStatement,
                                   &(sDisjoin->oldPartName) );
            IDE_RAISE( ERR_JOIN_DISJOIN_NOT_READ_WRITE );
        }
        else
        {
            /* Nothing To Do */
        }
    }

    /* 7. statement에 테이블의 파티션 전부 입력했는지 확인 */
    IDE_TEST( qcmPartition::getPartitionCount( aStatement,
                                               sTableInfo->tableID,
                                               & sPartCount )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( sPartCount != sParseTree->partCount,
                    ERR_DISJOIN_MISS_SOME_PARTITION );

    /* 8. 인덱스, PK, Unique, FK, Trigger(자신에 의해 DML 발동하는 것만)가 있으면 에러 */
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

    /* 9. 테이블에 이중화가 걸려 있으면 에러 */
    IDE_TEST_RAISE( sTableInfo->replicationCount > 0,
                    ERR_DDL_WITH_REPLICATED_TABLE );
    //proj-1608:replicationCount가 0일 때 recovery count는 항상 0이어야 함
    IDE_DASSERT( sTableInfo->replicationRecoveryCount == 0 );

    /* 10. 권한, operatable 체크 (CREATE & DROP) */
    IDE_TEST( qdpRole::checkDDLCreateTablePriv( aStatement,
                                                sParseTree->userID )
              != IDE_SUCCESS );

    IDE_TEST( qdpRole::checkDDLDropTablePriv( aStatement,
                                                sParseTree->userID )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( QCM_IS_OPERATABLE_QP_DROP_TABLE( sTableInfo->operatableFlag ) != ID_TRUE,
                    ERR_NOT_EXIST_TABLE );

    /* 11. compression 칼럼이 있으면 에러 */
    /* 12. hidden 칼럼이 있으면 에러 */
    /* 13. 보안 칼럼이 있으면 에러 */
    for ( sColumn = sTableInfo->columns;
          sColumn != NULL;
          sColumn = sColumn->next  )
    {
        IDE_TEST_RAISE( ( sColumn->flag & QCM_COLUMN_HIDDEN_COLUMN_MASK )
                        == QCM_COLUMN_HIDDEN_COLUMN_TRUE,
                        ERR_JOIN_DISJOIN_COLUMN_SPEC );

        IDE_TEST_RAISE( ( sColumn->basicInfo->column.flag & SMI_COLUMN_COMPRESSION_MASK )
                        == SMI_COLUMN_COMPRESSION_TRUE,
                        ERR_JOIN_DISJOIN_COLUMN_SPEC );

        IDE_TEST_RAISE( ( sColumn->basicInfo->module->flag & MTD_ENCRYPT_TYPE_MASK )
                        == MTD_ENCRYPT_TYPE_TRUE,
                        ERR_JOIN_DISJOIN_COLUMN_SPEC );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( CANT_USE_RESERVED_WORD )
    {
        (void)sqlInfo.init( aStatement->qmeMem );
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDB_RESERVED_WORD_IN_OBJECT_NAME,
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
    IDE_EXCEPTION( ERR_DISJOIN_MISS_SOME_PARTITION )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDB_DISJOIN_NOT_ALL_PARTITION ) );
    }
    IDE_EXCEPTION( ERR_DISJOIN_TABLE_NON_PART_TABLE) //non-partitioned or hash table
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDB_CANNOT_JOIN_DISJOIN_HASH_OR_NON_PART_TBL ) );
    }
    IDE_EXCEPTION( ERR_JOIN_DISJOIN_TABLE_SPEC ) // pk/uk/fk/index/trigger
    {
        sqlInfo.setSourceInfo( aStatement,
                               &(sParseTree->tableName) );
        (void)sqlInfo.init( aStatement->qmeMem );
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDB_CANNOT_JOIN_DISJOIN_TABLE_SPEC,
                                  sqlInfo.getErrMessage() ) );
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION( ERR_DDL_WITH_REPLICATED_TABLE )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDB_DDL_WITH_REPLICATED_TBL ) );
    }
    IDE_EXCEPTION( ERR_JOIN_DISJOIN_NOT_USABLE )
    {
        (void)sqlInfo.init( aStatement->qmeMem );
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMX_TABLE_PARTITION_UNUSABLE,
                                  sqlInfo.getErrMessage() ) );
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION( ERR_JOIN_DISJOIN_NOT_READ_WRITE )
    {
        (void)sqlInfo.init( aStatement->qmeMem );
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDB_JOIN_DISJOIN_NOT_READ_WRITE,
                                  sqlInfo.getErrMessage() ) );
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION( ERR_NOT_EXIST_TABLE )
    {
        sqlInfo.setSourceInfo( aStatement,
                               & sParseTree->tableName );
        (void)sqlInfo.init( aStatement->qmeMem );
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QCV_NOT_EXISTS_TABLE,
                                  sqlInfo.getErrMessage() ) );
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION( ERR_JOIN_DISJOIN_COLUMN_SPEC )  // hidden/compression/encrypted column
    {
        sqlInfo.setSourceInfo( aStatement,
                               & sParseTree->tableName );
        (void)sqlInfo.init( aStatement->qmeMem );
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDB_CANNOT_JOIN_DISJOIN_COLUMN_SPEC,
                                  sqlInfo.getErrMessage() ) );
        (void)sqlInfo.fini();
    }
    /* PROJ-2464 hybrid partitioned table 지원 */
    IDE_EXCEPTION( ERR_UNSUPPORT_ON_HYBRID_PARTITIONED_TABLE );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDB_CANNOT_SUPPORT_ON_HYBRID_PARTITIONED_TABLE ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdbDisjoin::executeDisjoinTable( qcStatement * aStatement )
{
/***********************************************************************
 *
 * Description :
 *    DISJOIN TABLE ... ( PARTITION ... TO TABLE ... , ... )
 *
 * Implementation :
 *    1. table/partition info를 가져온다
 *    2. 파티션 별로 다음을 수행한다. (메타 INSERT는 기존 테이블 데이터를 참조해서 생성)
 *       2-1. 파티션을 위한 table ID를 생성.
 *       2-2. 파티션을 위한 table 데이터를 메타 테이블에 추가
 *       2-3. 파티션을 위한 column ID를 생성하고 column 데이터를 메타 테이블에 추가
 *       2-4. constraint 별로 ID 생성, 데이터를 메타에 추가
 *    3. 기존 테이블의 메타 정보를 삭제한다.
 *    4. 관련 psm, pkg, view invalid
 *    5. constraint 관련 function/procedure 정보 삭제
 *    6. smi::droptable
 *    7. 파티션(이제는 테이블) 메타 캐시 재구성
 *    8. 테이블 메타 캐쉬에서 삭제
 *
 ***********************************************************************/

    qdDisjoinTableParseTree * sParseTree = NULL;
    qcmPartitionInfoList    * sPartInfoList = NULL;
    qcmPartitionInfoList    * sOldPartInfoList = NULL;

    qcmTableInfo            * sTableInfo = NULL;
    qcmTableInfo            * sPartInfo = NULL;
    qdDisjoinTable          * sDisjoin = NULL;
    qcmColumn               * sColumn = NULL;
    qcmTableInfo           ** sNewTableInfoArr = NULL;
    smiTableSpaceAttr         sTBSAttr;

    UInt                      sTableID = 0;
    UInt                      sNewColumnID = 0;
    UInt                      sPartitionCount = 0;
    ULong                     sAllocSize = 0;
    UInt                      i = 0;

    sParseTree = (qdDisjoinTableParseTree *)aStatement->myPlan->parseTree;

    // TASK-2176
    // Table에 대한 Lock을 획득한다.
    IDE_TEST( smiValidateAndLockObjects( ( QC_SMI_STMT( aStatement ) )->getTrans(),
                                         sParseTree->tableHandle,
                                         sParseTree->tableSCN,
                                         SMI_TBSLV_DROP_TBS, // TBS Validation 옵션
                                         SMI_TABLE_LOCK_X,
                                         smiGetDDLLockTimeOut((QC_SMI_STMT(aStatement))->getTrans()),
                                         ID_FALSE )
              != IDE_SUCCESS );

    sTableInfo = sParseTree->tableInfo;  /* old table info */

    // PROJ-1502 PARTITIONED DISK TABLE
    // 모든 파티션에 LOCK(X)
    IDE_TEST( qcmPartition::validateAndLockPartitionInfoList( aStatement,
                                                              sParseTree->partInfoList,
                                                              SMI_TBSLV_DROP_TBS, // TBS Validation 옵션
                                                              SMI_TABLE_LOCK_X,
                                                              smiGetDDLLockTimeOut((QC_SMI_STMT(aStatement))->getTrans()))
              != IDE_SUCCESS );

    // 예외 처리를 위하여, Lock을 잡은 후에 Partition List를 설정한다.
    sOldPartInfoList = sParseTree->partInfoList;

    for ( sPartInfoList = sOldPartInfoList;
          sPartInfoList != NULL;
          sPartInfoList = sPartInfoList->next )
    {
        sPartitionCount++;
    }

    // 새로운 qcmPartitionInfo들의 pointer정보를 가지는 배열 생성
    sAllocSize = (ULong)sPartitionCount * ID_SIZEOF(qcmTableInfo*);
    IDU_FIT_POINT_RAISE( "qdbDisjoin::executeDisjoinTable::cralloc::sNewTableInfoArr",
                         ERR_MEMORY_ALLOCATION );
    IDE_TEST_RAISE( aStatement->qmxMem->cralloc(
                    sAllocSize,
                    (void**) & sNewTableInfoArr )
                    != IDE_SUCCESS, ERR_MEMORY_ALLOCATION );

    /* PROJ-1723 [MDW/INTEGRATOR] Altibase Plugin 개발
     DDL Statement Text의 로깅
     */
    if ( QCU_DDL_SUPPLEMENTAL_LOG == 1 )
    {
        IDE_TEST( qciMisc::writeDDLStmtTextLog( aStatement,
                                                sTableInfo->tableOwnerID,
                                                sTableInfo->name )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing to do */
    }

    /* 2. 파티션 별로 메타 copy 수행 */
    /* 필요한 table ID, column ID, constraint ID 등을 생성 후 */
    /* 기존 파티션드 테이블의 데이터를 기반으로 메타 테이블 내용을 copy */

    /* sDisjoin은 파티션 하나에 대응된다. */
    /* sPartInfo는 그 파티션의 tableInfo다. */
    for ( sDisjoin = sParseTree->disjoinTable;
          sDisjoin != NULL;
          sDisjoin = sDisjoin->next )
    {
        sPartInfo = sDisjoin->oldPartInfo;

        IDE_TEST( qcm::getNextTableID( aStatement, &sDisjoin->newTableID )
                  != IDE_SUCCESS );
        sTableID = sDisjoin->newTableID;

        /* 메타 복사 */

        /* sys_tables_ */
        IDE_TEST( qcmTablespace::getTBSAttrByID( sPartInfo->TBSID,
                                                 &sTBSAttr )
                  != IDE_SUCCESS );

        IDE_TEST( copyTableSpec( aStatement,
                                 sDisjoin,
                                 sTBSAttr.mName,
                                 sTableInfo->tableID )    /* table ID(old) */
                  != IDE_SUCCESS );

        /* sys_columns_, sys_lobs_ */
        sNewColumnID = sTableID * SMI_COLUMN_ID_MAXIMUM;
        sColumn = sTableInfo->columns;

        /* 파티션 헤더의 column ID를 수정한 뒤 메타에 새 테이블의 column spec을 추가한다. */
        /* 파티션드 테이블의 첫 번째 칼럼(qcmcolumn)과 */
        /* 새 테이블의 첫번째 칼럼 ID를 넘겨준다. */
        IDE_TEST( modifyColumnID( aStatement,
                                  sDisjoin->oldPartInfo->columns,   /* column(partition's first) */
                                  sTableInfo->columnCount,          /* column count */
                                  sNewColumnID,                     /* new column ID(first) */
                                  sPartInfo->tableHandle )          /* partition handle */
                  != IDE_SUCCESS );
        
        for ( i = 0; i < sTableInfo->columnCount; i++ )
        {
            /* 컬럼 하나의 데이터를 카피 */
            IDE_TEST( copyColumnSpec( aStatement,
                                      sParseTree->userID,               /* user ID */
                                      sTableInfo->tableID,              /* table ID(old) */
                                      sTableID,                         /* table ID(new) */
                                      sPartInfo->partitionID,           /* partition ID(old) */
                                      sColumn[i].basicInfo->column.id,  /* column ID(old) */
                                      sNewColumnID + i,                 /* column ID(new) */
                                      sColumn[i].basicInfo->module->flag ) /* column flag */
                      != IDE_SUCCESS );
        }

        /* constraint 별로 새 이름, ID를 생성하고 메타를 복사한다. */
        /* sys_constraints_ */
        /* constraint ID를 기반으로 search하고 constraint name 생성, 메타 copy */

        IDE_TEST( copyConstraintSpec( aStatement,
                                      sTableInfo->tableID,  /* old table ID */
                                      sTableID )   /* new table ID */
                  != IDE_SUCCESS );
        
        IDU_FIT_POINT_RAISE( "qdbDisjoin::executeDisjoinTable::disjoinTableLoop::fail",
                             ERR_FIT_TEST );
    }

    /* 3. 파티션드 테이블의 메타 delete */

    /* delete from partition related meta */
    for ( sDisjoin = sParseTree->disjoinTable;
          sDisjoin != NULL;
          sDisjoin = sDisjoin->next )
    {
        IDE_TEST( qdd::deleteTablePartitionFromMeta( aStatement,
                                                     sDisjoin->oldPartID )
                 != IDE_SUCCESS );
    } 

    /* delete from constraint related meta */
    IDE_TEST( qdd::deleteConstraintsFromMeta( aStatement, sTableInfo->tableID )
              != IDE_SUCCESS );

    /* delete from index related meta */
    IDE_TEST( qdd::deleteIndicesFromMeta( aStatement, sTableInfo )
              != IDE_SUCCESS);

    /* delete from table/columns related meta */
    IDE_TEST( qdd::deleteTableFromMeta( aStatement, sTableInfo->tableID )
              != IDE_SUCCESS);

    /* delete from grant meta */
    IDE_TEST( qdpDrop::removePriv4DropTable( aStatement, sTableInfo->tableID )
              != IDE_SUCCESS );

    // PROJ-1359
    IDE_TEST( qdnTrigger::dropTrigger4DropTable( aStatement, sTableInfo )
              != IDE_SUCCESS );

    /*  4. 관련 psm, pkg, view invalid */
    /* PROJ-2197 PSM Renewal */
    // related PSM
    IDE_TEST( qcmProc::relSetInvalidProcOfRelated( aStatement,
                                                   sParseTree->userID,
                                                   (SChar *)( sParseTree->tableName.stmtText +
                                                              sParseTree->tableName.offset ),
                                                   sParseTree->tableName.size,
                                                   QS_TABLE )
              != IDE_SUCCESS );

    // PROJ-1073 Package
    IDE_TEST( qcmPkg::relSetInvalidPkgOfRelated( aStatement,
                                                 sParseTree->userID,
                                                 (SChar *)( sParseTree->tableName.stmtText +
                                                            sParseTree->tableName.offset ),
                                                 sParseTree->tableName.size,
                                                 QS_TABLE )
              != IDE_SUCCESS );

    // related VIEW
    IDE_TEST( qcmView::setInvalidViewOfRelated( aStatement,
                                                sParseTree->userID,
                                                (SChar *)( sParseTree->tableName.stmtText +
                                                           sParseTree->tableName.offset ),
                                                sParseTree->tableName.size,
                                                QS_TABLE )
              != IDE_SUCCESS );

    /*  5. constraint/index 관련 function/procedure 정보 삭제 */
    /* BUG-35445 Check Constraint, Function-Based Index에서 사용 중인 Function을 변경/제거 방지 */
    IDE_TEST( qcmProc::relRemoveRelatedToConstraintByTableID(
                    aStatement,
                    sTableInfo->tableID )
              != IDE_SUCCESS );

    IDE_TEST( qcmProc::relRemoveRelatedToIndexByTableID(
                    aStatement,
                    sTableInfo->tableID )
              != IDE_SUCCESS );

    // BUG-21387 COMMENT
    IDE_TEST( qdbComment::deleteCommentTable( aStatement,
                                              sParseTree->tableInfo->tableOwnerName,
                                              sParseTree->tableInfo->name )
              != IDE_SUCCESS);

    // PROJ-2223 audit
    IDE_TEST( qcmAudit::deleteObject( aStatement,
                                      sParseTree->tableInfo->tableOwnerID,
                                      sParseTree->tableInfo->name )
              != IDE_SUCCESS );

    /*  6. smi::droptable */
    IDE_TEST( smiTable::dropTable( QC_SMI_STMT( aStatement ),
                                   sTableInfo->tableHandle,
                                   SMI_TBSLV_DROP_TBS )
              != IDE_SUCCESS);

    /* 7. 파티션(이제는 테이블) 메타 캐시 재구성 */
    for ( sDisjoin = sParseTree->disjoinTable, i = 0;
          sDisjoin != NULL;
          sDisjoin = sDisjoin->next, i++ )
    {
        sTableID = sDisjoin->newTableID;

        IDE_TEST( smiTable::touchTable( QC_SMI_STMT( aStatement ),
                                        sDisjoin->oldPartInfo->tableHandle,
                                        SMI_TBSLV_DROP_TBS )
                  != IDE_SUCCESS );

        IDE_TEST( qcm::makeAndSetQcmTableInfo(
                      QC_SMI_STMT( aStatement ), sTableID, sDisjoin->oldPartOID )
                  != IDE_SUCCESS );

        IDE_TEST( smiGetTableTempInfo(
                      smiGetTable( sDisjoin->oldPartOID ),
                      (void**)&sNewTableInfoArr[i] )
                  != IDE_SUCCESS );

        IDU_FIT_POINT_RAISE( "qdbDisjoin::executeDisjoinTable::makeAndSetQcmTableInfo::fail",
                             ERR_FIT_TEST );
    }

    /////////////////////////////////////////////////////////////////////////
    // 현재 DISJOIN TABLE 구문에서 트리거 있는 테이블을 허용하지 않으므로
    // 사용하지 않는다.
    // IDE_TEST( qdnTrigger::freeTriggerCaches4DropTable( sTableInfo )
    //           != IDE_SUCCESS );
    /////////////////////////////////////////////////////////////////////////

    /* (구) 파티션 메타 캐쉬 삭제 */
    (void)qcmPartition::destroyQcmPartitionInfoList( sOldPartInfoList );

    /* 8. 테이블 메타 캐쉬에서 삭제 */
    (void)qcm::destroyQcmTableInfo( sTableInfo );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_MEMORY_ALLOCATION )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_MEMORY_ALLOCATION ) );
    }
    IDE_EXCEPTION( ERR_FIT_TEST )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qdbDisjoin::executeDisjoinTable",
                                  "FIT test" ) );
    }
    IDE_EXCEPTION_END;

    // 실패 시 새로 만든 table info를 삭제한다.
    if ( sNewTableInfoArr != NULL )
    {
        for ( i = 0; i < sPartitionCount; i++ )
        {
            (void)qcm::destroyQcmTableInfo( sNewTableInfoArr[i] );
        }
    }
    else
    {
        /* Nothing to do */
    }

    // on fail, must restore old table info.
    qcmPartition::restoreTempInfo( sTableInfo,
                                   sOldPartInfoList,
                                   NULL );

    return IDE_FAILURE;
}

IDE_RC qdbDisjoin::copyTableSpec( qcStatement     * aStatement,
                                  qdDisjoinTable  * aDisjoin,
                                  SChar           * aTBSName,
                                  UInt              aOldTableID )
{
/***********************************************************************
 *
 *  Description : PROJ-1810 Partition Exchange
 *                원본 테이블의 table 메타 정보를 복사하여
 *                새로 생성되는 테이블 하나의 table spec을 메타에 추가한다.
 *
 *  Implementation :
 *      SYS_TABLES_에서 파티션드 테이블의 TABLE ID로 SELECT한 뒤
 *      TABLE_ID, TABLE_OID, TABLE_NAME 등만 수정해서 SYS_TABLES_에 INSERT.
 *
 *      TABLE_ID, TABLE_NAME은 선행 과정에서 구한 것을 사용하고
 *      (table ID는 execution 과정에서 생성, table name은 사용자가 명시 or 자동 생성)
 *
 *      TABLE_OID는 파티션의 OID를 사용.
 *
 *      TABLE_TYPE, IS_PARTITIONED, TEMPORARY, HIDDEN, PARALLEL_DEGREE는 고정된 값을 사용한다.
 *      ACCESS를 따라 USABLE, SHARD_FLAG는 고정된 값을 사용한다(TASK-7307)
 *      LAST_DDL_TIME은 SYSDATE를 입력한다.
 *      그 외에는 파티션드 테이블의 정보를 그대로 복사한다.
 *
 ***********************************************************************/

    SChar       sSqlStr[ QD_MAX_SQL_LENGTH + 1 ];
    vSLong      sRowCnt = 0;
    SChar       sTableName[ QC_MAX_OBJECT_NAME_LEN + 1 ];

    QC_STR_COPY( sTableName, aDisjoin->newTableName );

    idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH + 1,
                     "INSERT INTO SYS_TABLES_ "
                         "SELECT USER_ID, "
                         "INTEGER'%"ID_INT32_FMT"', "       /* TABLE_ID (new) */
                         "BIGINT'%"ID_INT64_FMT"', "        /* TABLE_OID(partition OID) */
                         "COLUMN_COUNT, "
                         "VARCHAR'%s', "                    /* TABLE_NAME(new) */
                         "CHAR'T', "                        /* TABLE_TYPE */
                         "INTEGER'0', "                     /* REPLICATION_COUNT */
                         "INTEGER'0', "                     /* REPLICATION_RECOVERY_COUNT */
                         "MAXROW, "
                         "INTEGER'%"ID_INT32_FMT"', "       /* TBS_ID(partition) */
                         "VARCHAR'%s', "                    /* TBS_NAME(partition) */
                         "PCTFREE, "
                         "PCTUSED, "
                         "INIT_TRANS, "
                         "MAX_TRANS, "
                         "INITEXTENTS, "
                         "NEXTEXTENTS, "
                         "MINEXTENTS, "
                         "MAXEXTENTS, "
                         "CHAR'F', "                        /* IS_PARTITIONED */
                         "CHAR'N', "                        /* TEMPORARY */
                         "CHAR'N', "                        /* HIDDEN */
                         "CHAR'W', "                        /* ACCESS */
                         "INTEGER'1', "                     /* PARALLEL_DEGREE */
                         "CHAR'Y', "                        /* TASK-7307 USABLE */
                         "INTEGER'0', "                     /* TASK-7307 SHARD_FLAG */
                         "CREATED, "
                         "SYSDATE "                         /* LAST_DDL_TIME */
                         "FROM SYS_TABLES_ "
                         "WHERE TABLE_ID=INTEGER'%"ID_INT32_FMT"' ", /* TABLE_ID(old) */
                     aDisjoin->newTableID,             /* TABLE_ID(new) */
                     aDisjoin->oldPartOID,             /* TABLE_OID(partition OID) */
                     sTableName,                       /* TABLE_NAME(new) */
                     (UInt)aDisjoin->oldPartInfo->TBSID,     /* TBS_ID(partition) */
                     aTBSName,                         /* TBS Name(partition) */
                     aOldTableID );                    /* TABLE_ID(old) */

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

IDE_RC qdbDisjoin::copyColumnSpec( qcStatement     * aStatement,
                                   UInt              aUserID,
                                   UInt              aOldTableID,
                                   UInt              aNewTableID,
                                   UInt              aOldPartID,
                                   UInt              aOldColumnID,
                                   UInt              aNewColumnID,
                                   UInt              aFlag )
{
/***********************************************************************
 *
 *  Description : PROJ-1810 Partition Exchange
 *                원본 테이블의 column, lob column 메타 정보를 복사하여
 *                새로 생성되는 테이블 하나의 column spec, lob spec을 메타에 추가한다.
 *
 *  Implementation :
 *      SYS_COLUMNS_에서 파티션드 테이블 칼럼의 COLUMN_ID로 SELECT한 뒤
 *      COLUMN_ID, TABLE_ID만 수정해서 SYS_COLUMNS_에 INSERT.
 *
 ***********************************************************************/

    SChar               sSqlStr[ QD_MAX_SQL_LENGTH + 1 ];
    vSLong              sRowCnt = 0;

    idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH + 1,
                     "INSERT INTO SYS_COLUMNS_ "
                         "SELECT INTEGER'%"ID_INT32_FMT"', " /* COLUMN_ID(new) */
                         "DATA_TYPE, "
                         "LANG_ID, "
                         "OFFSET, "
                         "SIZE, "
                         "USER_ID, "
                         "INTEGER'%"ID_INT32_FMT"', "       /* TABLE_ID(new) */
                         "PRECISION, "
                         "SCALE, "
                         "COLUMN_ORDER, "
                         "COLUMN_NAME, "
                         "IS_NULLABLE, "
                         "DEFAULT_VAL, "
                         "STORE_TYPE, "
                         "IN_ROW_SIZE, "
                         "REPL_CONDITION, "
                         "IS_HIDDEN, "
                         "IS_KEY_PRESERVED "
                         "FROM SYS_COLUMNS_ "
                         "WHERE COLUMN_ID=INTEGER'%"ID_INT32_FMT"' ",   /* COLUMN_ID(old) */
                     aNewColumnID,                          /* column id(new) */
                     aNewTableID,                           /* table id(new) */
                     aOldColumnID );                        /* column id(old) */

    IDE_TEST( qcg::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                 sSqlStr,
                                 & sRowCnt ) != IDE_SUCCESS );

    IDE_TEST_RAISE( sRowCnt != 1, ERR_META_CRASH );

    if ( ( aFlag & MTD_COLUMN_TYPE_MASK ) == MTD_COLUMN_TYPE_LOB )
    {
        /* LOB column인 경우 SYS_LOBS_에 추가로 기록한다. */
        idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH + 1,
                         "INSERT INTO SYS_LOBS_ "
                             "SELECT A.USER_ID, "
                             "INTEGER'%"ID_INT32_FMT"', "       /* TABLE_ID(new) */
                             "INTEGER'%"ID_INT32_FMT"', "       /* COLUMN_ID(new) */
                             "B.TBS_ID, "
                             "B.LOGGING, "
                             "B.BUFFER, "
                             "A.IS_DEFAULT_TBS "
                             "FROM SYS_LOBS_ A, SYS_PART_LOBS_ B "
                             "WHERE A.COLUMN_ID=INTEGER'%"ID_INT32_FMT"' AND "   /* COLUMN_ID(old) */
                             "A.COLUMN_ID=B.COLUMN_ID AND "
                             "B.PARTITION_ID=INTEGER'%"ID_INT32_FMT"' AND " /* part ID(old) */
                             "B.USER_ID=INTEGER'%"ID_INT32_FMT"' AND " /* user ID */
                             "B.TABLE_ID=INTEGER'%"ID_INT32_FMT"' ", /* table ID(old) */
                         aNewTableID,                           /* table ID(new) */
                         aNewColumnID,                          /* column ID(new) */
                         aOldColumnID,                          /* column ID(old) */
                         aOldPartID,                            /* partition ID(old) */
                         aUserID,                               /* user ID */
                         aOldTableID );                         /* table ID(old) */

        IDE_TEST( qcg::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                     sSqlStr,
                                     & sRowCnt ) != IDE_SUCCESS );

        IDE_TEST_RAISE( sRowCnt != 1, ERR_META_CRASH );
    }
    else
    {
        /* Nothing To Do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_META_CRASH )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDB_META_CRASH ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdbDisjoin::copyConstraintSpec( qcStatement     * aStatement,
                                       UInt              aOldTableID,
                                       UInt              aNewTableID )
{
/***********************************************************************
 *
 *  Description : PROJ-1810 Partition Exchange
 *                원본 테이블의 constraint 메타 정보를 복사하여
 *                새로 생성되는 테이블들의 constraint spec을 메타에 추가한다.
 *
 *  Implementation :
 *      SYS_CONSTRAINTS_ 메타를 search해서 필요한 정보들을 얻어야 한다.
 *      먼저 새롭게 생성한 constraint ID를 사용해 새로운 constraint 이름을 만든다.
 *      이런 작업들을 위해 메타에서 기존의 constraint ID, table ID를 찾아 저장한다.
 *
 *      마지막으로 기존 constraint의 table ID, constraint ID로 메타의 한 row를 select하고
 *      table ID, constraint ID, constraint name를 수정해 insert한다.
 *
 *      constraint ID, name은 앞서 만든 것을 사용한다.
 *
 *      추가로 SYS_CONSTRAINT_COLUMNS_ 메타도 마찬가지로 insert.
 *
 ***********************************************************************/


    SChar                 sSqlStr[ QD_MAX_SQL_LENGTH + 1 ];
    const void          * sRow = NULL;
    smiRange              sRange;
    smiTableCursor        sCursor;
    scGRID                sRid;
    SInt                  sStage = 0;
    vSLong                sRowCnt = 0;
    mtcColumn           * sConstrConstraintIDCol = NULL;
    mtcColumn           * sConstrTableIDCol = NULL;
    mtcColumn           * sConstrTypeCol = NULL;
    mtcColumn           * sConstrColumnCountCol = NULL;
    mtdIntegerType        sConstrConstraintID = 0;
    UInt                  sConstrType = 0;
    qtcMetaRangeColumn    sRangeColumn;
    smiCursorProperties   sCursorProperty;
    qdDisjoinConstr     * sConstrInfo = NULL;
    qdDisjoinConstr     * sFirstConstrInfo = NULL;
    qdDisjoinConstr     * sLastConstrInfo = NULL;

    sCursor.initialize();

    // constraint user_id column 정보
    IDE_TEST( smiGetTableColumns( gQcmConstraints,
                                  QCM_CONSTRAINTS_CONSTRAINT_ID_COL_ORDER,
                                  (const smiColumn**)&sConstrConstraintIDCol )
              != IDE_SUCCESS );
    // mtdModule 설정
    IDE_TEST( mtd::moduleById( &sConstrConstraintIDCol->module,
                               sConstrConstraintIDCol->type.dataTypeId )
              != IDE_SUCCESS );

    // constraint table_id column 정보
    IDE_TEST( smiGetTableColumns( gQcmConstraints,
                                  QCM_CONSTRAINTS_TABLE_ID_COL_ORDER,
                                  (const smiColumn**)&sConstrTableIDCol )
              != IDE_SUCCESS );
    // mtdModule 설정
    IDE_TEST( mtd::moduleById( &sConstrTableIDCol->module,
                               sConstrTableIDCol->type.dataTypeId )
              != IDE_SUCCESS );

    // constraint type column 정보
    IDE_TEST( smiGetTableColumns( gQcmConstraints,
                                  QCM_CONSTRAINTS_CONSTRAINT_TYPE_COL_ORDER,
                                  (const smiColumn**)&sConstrTypeCol )
              != IDE_SUCCESS );
    // mtdModule 설정
    IDE_TEST( mtd::moduleById( &sConstrTypeCol->module,
                               sConstrTypeCol->type.dataTypeId )
              != IDE_SUCCESS );

    // constraint column count column 정보
    IDE_TEST( smiGetTableColumns( gQcmConstraints,
                                  QCM_CONSTRAINTS_COLUMN_CNT_COL_ORDER,
                                  (const smiColumn**)&sConstrColumnCountCol )
              != IDE_SUCCESS );
    // mtdModule 설정
    IDE_TEST( mtd::moduleById( &sConstrColumnCountCol->module,
                               sConstrColumnCountCol->type.dataTypeId )
              != IDE_SUCCESS );

    qcm::makeMetaRangeSingleColumn( &sRangeColumn,
                                    (const mtcColumn*) sConstrTableIDCol,
                                    (const void *) & aOldTableID,
                                    &sRange );

    SMI_CURSOR_PROP_INIT_FOR_META_INDEX_SCAN( &sCursorProperty, NULL );

    // cursor open
    IDE_TEST( sCursor.open( QC_SMI_STMT( aStatement ),
                            gQcmConstraints,
                            gQcmConstraintsIndex[QCM_CONSTRAINTS_TABLEID_INDEXID_IDX_ORDER],
                            smiGetRowSCN(gQcmConstraints),
                            NULL,
                            &sRange,
                            smiGetDefaultKeyRange(),
                            smiGetDefaultFilter(),
                            QCM_META_CURSOR_FLAG,
                            SMI_SELECT_CURSOR,
                            &sCursorProperty) != IDE_SUCCESS );

    sStage = 1;

    IDE_TEST( sCursor.beforeFirst() != IDE_SUCCESS );
    
    IDE_TEST( sCursor.readRow( &sRow, &sRid, SMI_FIND_NEXT ) != IDE_SUCCESS );
    
    while ( sRow != NULL )
    {
        IDE_TEST( STRUCT_ALLOC( QC_QMX_MEM( aStatement ), 
                                qdDisjoinConstr,
                                (void**)&sConstrInfo )
                  != IDE_SUCCESS );
    
        sConstrConstraintID = *(mtdIntegerType *)
                        ((UChar *)sRow + sConstrConstraintIDCol->column.offset);

        sConstrType = *((UInt *)
                        ((UChar *)sRow + sConstrTypeCol->column.offset));

        /* copy constraint column count */
        sConstrInfo->columnCount = *((UInt *)
                                     ((UChar *)sRow + sConstrColumnCountCol->column.offset));

        /* copy old constraint ID */
        sConstrInfo->oldConstrID = (UInt)sConstrConstraintID;

        /* create new constraint ID */
        IDE_TEST( qcm::getNextConstrID( aStatement, &sConstrInfo->newConstrID )
                  != IDE_SUCCESS );

        /* create new constraint name */
        if ( sConstrType == QD_NOT_NULL )
        {
            idlOS::snprintf( sConstrInfo->newConstrName,
                             QC_MAX_OBJECT_NAME_LEN + 1,
                             "%sCON_NOT_NULL_ID%"ID_INT32_FMT"",
                             QC_SYS_OBJ_NAME_HEADER,
                             sConstrInfo->newConstrID );
        }
        else if ( sConstrType == QD_NULL )
        {
            idlOS::snprintf( sConstrInfo->newConstrName,
                             QC_MAX_OBJECT_NAME_LEN + 1,
                             "%sCON_ID%"ID_INT32_FMT"",
                             QC_SYS_OBJ_NAME_HEADER,
                             sConstrInfo->newConstrID );
        }
        else if ( sConstrType == QD_TIMESTAMP )
        {
            idlOS::snprintf( sConstrInfo->newConstrName,
                             QC_MAX_OBJECT_NAME_LEN + 1,
                             "%sCON_TIMESTAMP_ID%"ID_INT32_FMT"",
                             QC_SYS_OBJ_NAME_HEADER,
                             sConstrInfo->newConstrID );
        }
        else if ( sConstrType == QD_CHECK )
        {
            idlOS::snprintf( sConstrInfo->newConstrName,
                             QC_MAX_OBJECT_NAME_LEN + 1,
                             "%sCON_CHECK_ID%"ID_INT32_FMT"",
                             QC_SYS_OBJ_NAME_HEADER,
                             sConstrInfo->newConstrID );
        }
        else
        {
            /* not null, null, timestamp, check 이외의 constraint가 오면 오류. */
            IDE_DASSERT( 0 );
        }

        if ( sFirstConstrInfo == NULL )
        {
            sConstrInfo->next = NULL;
            sFirstConstrInfo = sConstrInfo;
            sLastConstrInfo = sConstrInfo;
        }
        else
        {
            sConstrInfo->next = NULL;
            sLastConstrInfo->next = sConstrInfo;
            sLastConstrInfo = sLastConstrInfo->next;
        }

        IDE_TEST( sCursor.readRow( &sRow, &sRid, SMI_FIND_NEXT ) != IDE_SUCCESS );
    }

    sStage = 0;

    IDE_TEST( sCursor.close() != IDE_SUCCESS );

    /* 이후 constraint id 별로 sys_constraints_에 insert한다. */
    for ( sConstrInfo = sFirstConstrInfo;
          sConstrInfo != NULL;
          sConstrInfo = sConstrInfo->next )
    {
        /* table_id, constraint_id, constraint_name를 바꿔야 한다. */
        idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH + 1,
                         "INSERT INTO SYS_CONSTRAINTS_ "
                            "SELECT USER_ID, "
                            "INTEGER'%"ID_INT32_FMT"', "            /* TABLE_ID(new) */
                            "INTEGER'%"ID_INT32_FMT"', "            /* CONSTRAINT_ID(new) */
                            "VARCHAR'%s', "                         /* CONSTRAINT_NAME(new) */
                            "CONSTRAINT_TYPE, "
                            "INDEX_ID, "
                            "COLUMN_CNT, "
                            "REFERENCED_TABLE_ID, "
                            "REFERENCED_INDEX_ID, "
                            "DELETE_RULE, "
                            "CHECK_CONDITION, "
                            "VALIDATED "
                            "FROM SYS_CONSTRAINTS_ "
                            "WHERE CONSTRAINT_ID="
                                "INTEGER'%"ID_INT32_FMT"' ", /* CONSTR_ID(old) */
                            aNewTableID,                    /* table ID(new) */
                            sConstrInfo->newConstrID,       /* constraint ID(new) */
                            sConstrInfo->newConstrName,     /* constraint name(new) */
                            sConstrInfo->oldConstrID );     /* constraint ID(old) */
    
        IDE_TEST( qcg::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                     sSqlStr,
                                     & sRowCnt ) != IDE_SUCCESS );

        IDE_TEST_RAISE( sRowCnt != 1, ERR_META_CRASH );

        /***********************************************************************
         * constraint column 메타에서 새로 바꾸는 column ID는 이렇게 찾는다.
         * 
         * column ID를 SMI_COLUMN_ID_MAXIMUM으로 mod하면 column order가 나온다.
         * new table ID에 SMI_COLUMN_ID_MAXIMUM을 곱하면
         * new table의 첫번째 컬럼의 ID가 나온다.
         * 이 둘을 더하면 new table에서 대응되는 constraint의 column ID가 나온다.
         *
         * constraint ID: 위에 SYS_CONSTRAINTS_ copy할 때 사용한 list 재활용한다.
         ***********************************************************************/

        idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH + 1,
                         "INSERT INTO SYS_CONSTRAINT_COLUMNS_ "
                            "SELECT USER_ID, "
                            "INTEGER'%"ID_INT32_FMT"', "            /* TABLE_ID(new) */
                            "INTEGER'%"ID_INT32_FMT"', "            /* CONSTRAINT_ID(new) */
                            "CONSTRAINT_COL_ORDER, "
                            "MOD(COLUMN_ID,INTEGER'%"ID_INT32_FMT"')" /* SMI COL ID_MAX */
                            "+INTEGER'%"ID_INT32_FMT"' "           /* new col ID(first) */
                            "FROM SYS_CONSTRAINT_COLUMNS_ "
                            "WHERE CONSTRAINT_ID=INTEGER'%"ID_INT32_FMT"' AND " /* CONST_ID(old) */
                            "TABLE_ID=INTEGER'%"ID_INT32_FMT"' ",    /* TABLE_ID(old) */
                            aNewTableID,                        /* table ID(new) */
                            sConstrInfo->newConstrID,       /* constraint ID(new) */
                            SMI_COLUMN_ID_MAXIMUM,
                            aNewTableID * SMI_COLUMN_ID_MAXIMUM,  /* new table's 1st col. ID */
                            sConstrInfo->oldConstrID,       /* constraint ID(old) */
                            aOldTableID );                  /* table ID(old) */

        IDE_TEST( qcg::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                     sSqlStr,
                                     & sRowCnt ) != IDE_SUCCESS );
    
        IDE_TEST_RAISE( sRowCnt != sConstrInfo->columnCount, ERR_META_CRASH );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_META_CRASH )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDB_META_CRASH ) );
    }
    IDE_EXCEPTION_END;

    switch ( sStage )
    {
        case 1 :
            sCursor.close();
            break;
        default :
            IDE_DASSERT( 0 );
    }

    return IDE_FAILURE;
}

IDE_RC qdbDisjoin::checkPartitionExistByName( qcStatement          * aStatement,
                                              qcmPartitionInfoList * aPartInfoList,
                                              qdDisjoinTable       * aDisjoin )
{
/***********************************************************************
 *
 *  Description : PROJ-1810 Partition Exchange
 *                테이블 ID로 모든 파티션을 가져온다.
 *
 *  Implementation :
 *      DISJOIN SQL statement에서 사용자가 파티션을 지정한 순서와
 *      part info list에 파티션 info가 저장되는 순서는 다를 수 있다.
 *      때문에 새 테이블 이름과 info를 매치시켜주기 위해
 *      part info의 이름과 parse tree에 사용자가 준 값을 비교해서
 *      일치하는 파티션 info, ID, OID를 disjoinTable에 달아준다.
 *
 ***********************************************************************/

    qcuSqlSourceInfo          sqlInfo;
    qcmTableInfo            * sPartInfo = NULL;
    qcmPartitionInfoList    * sPartInfoList = NULL;
    qdDisjoinTable          * sDisjoin = NULL;
    idBool                    sFound = ID_FALSE;

    /* 사용자가 입력한 파티션 이름으로 찾아서 */
    /* qdDisjoinTable 구조체에 파티션 ID, OID, info를 달아 놓는다. */
    for ( sDisjoin = aDisjoin;
          sDisjoin != NULL;
          sDisjoin = sDisjoin->next )
    {
        sFound = ID_FALSE;
        /* 주어진 이름의 파티션이 있는지 찾아본다. */
        for ( sPartInfoList = aPartInfoList;
              sPartInfoList != NULL;
              sPartInfoList = sPartInfoList->next )
        {
            sPartInfo = sPartInfoList->partitionInfo;

            // BUG-47599 default partition이 없는 경우 에러
            IDE_TEST_RAISE ( idlOS::strlen( sPartInfo->name ) == QD_EMPTY_PARTITION_NAME_SIZE,
                             ERR_UNSUPPORTED_SYNTAX );

            /* 이름으로 파티션 비교 */
            if ( idlOS::strMatch( sPartInfo->name,
                                  idlOS::strlen( sPartInfo->name ),
                                  (SChar*)( sDisjoin->oldPartName.stmtText + sDisjoin->oldPartName.offset ),
                                  (UInt)sDisjoin->oldPartName.size ) == 0 )
            {
                /* 찾아내면 파티션 ID, 파티션 OID, 파티션 info를 저장한다. */
                sDisjoin->oldPartID = sPartInfo->partitionID;
                sDisjoin->oldPartOID = sPartInfo->tableOID;
                sDisjoin->oldPartInfo = sPartInfo;
                sFound = ID_TRUE;
                break;
            }
            else
            {
                /* Nothing To Do */
            }
        }

        /* 찾아내지 못하면 error */
        if ( sFound == ID_FALSE )
        {
            sqlInfo.setSourceInfo( aStatement,
                                   &(sDisjoin->oldPartName) );
            IDE_RAISE( ERR_NOT_EXIST_TABLE_PARTITION );
        }
        else
        {
            /* Nothing To Do */
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_EXIST_TABLE_PARTITION )
    {
        (void)sqlInfo.init( aStatement->qmeMem );
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QCM_NOT_EXIST_TABLE_PARTITION,
                                  sqlInfo.getErrMessage() ) );
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION( ERR_UNSUPPORTED_SYNTAX )
    {
        IDE_SET( ideSetErrorCode(qpERR_ABORT_QCP_NOT_SUPPORTED_SYNTAX) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdbDisjoin::modifyColumnID( qcStatement     * aStatement,
                                   qcmColumn       * aColumn,
                                   UInt              aColCount,
                                   UInt              aNewColumnID,
                                   void            * aHandle )  /* table or partition */
{
/***********************************************************************
 *
 *  Description : PROJ-1810 Partition Exchange
 *                새 column ID를 테이블로 바뀌는 파티션의 헤더에 반영한다.
 *                executeDisjoin, executeJoin에서 호출된다.
 *
 *  Implementation :
 *      새 column ID로 mtcColumn을 만들고 smiColumnList 생성
 *      생성한 smiColumnList와 aHandle로 modifyTableInfo
 *
 ***********************************************************************/

    qcmColumn        * sColumn = NULL;
    mtcColumn        * sNewMtcColumns = NULL;
    smiColumnList    * sSmiColumnList = NULL;
    ULong              sAllocSize = 0;
    UInt               i = 0;

    sAllocSize = (ULong)aColCount * ID_SIZEOF(mtcColumn);
    IDU_FIT_POINT( "qdbDisjoin::modifyColumnID::alloc::sNewMtcColumns",
                    idERR_ABORT_InsufficientMemory );
    IDE_TEST( aStatement->qmxMem->alloc( sAllocSize,
                                         (void**)&sNewMtcColumns )
             != IDE_SUCCESS);

    sAllocSize = (ULong)aColCount * ID_SIZEOF(smiColumnList);
    IDU_FIT_POINT( "qdbDisjoin::modifyColumnID::alloc::sSmiColumnList",
                    idERR_ABORT_InsufficientMemory );
    IDE_TEST( aStatement->qmxMem->alloc( sAllocSize,
                                         (void**)&sSmiColumnList )
              != IDE_SUCCESS );

    /* partition column별로 mtcColumn을 복사하고 column ID만 바꿔준다. */
    /* mtcColumn으로 smiColumnList를 만든다. */
    for ( sColumn = aColumn, i = 0;
          sColumn != NULL;
          sColumn = sColumn->next, i++ )
    {
        idlOS::memcpy( &sNewMtcColumns[i], sColumn->basicInfo, ID_SIZEOF(mtcColumn) );
        sNewMtcColumns[i].column.id = aNewColumnID + i;
        sSmiColumnList[i].column = (const smiColumn*)(&sNewMtcColumns[i]);

        if ( i == aColCount - 1 )
        {
            sSmiColumnList[i].next = NULL;
        }
        else
        {
            sSmiColumnList[i].next = &sSmiColumnList[i + 1];
        }
    }

    IDE_DASSERT( sSmiColumnList != NULL );

    /* table header에서 column의 ID만 변경. */
    IDE_TEST( smiTable::modifyTableInfo( QC_SMI_STMT( aStatement ),
                                         aHandle,
                                         sSmiColumnList,
                                         ID_SIZEOF(mtcColumn),
                                         NULL,
                                         0,
                                         SMI_TABLE_FLAG_UNCHANGE,
                                         SMI_TBSLV_DROP_TBS )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
