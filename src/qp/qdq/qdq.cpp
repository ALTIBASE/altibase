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
 

/***********************************************************************/
/* $Id: qdq.cpp 91512 2021-08-21 07:50:50Z emlee $ */
/**********************************************************************/

#include <idl.h>
#include <ide.h>
#include <qcg.h>
#include <qdq.h>
#include <qmv.h>
#include <qmx.h>
#include <qdbCreate.h>
#include <qdbAlter.h>
#include <qdd.h>
#include <qcuProperty.h>
#include <qcuSqlSourceInfo.h>
#include <qcg.h>
#include <qdpDrop.h>
#include <qcmUser.h>
#include <qcmProc.h>
#include <qdpPrivilege.h>
#include <qcmAudit.h>
#include <qcmPkg.h>
#include <qcmTableSpace.h>
#include <qdpRole.h>

IDE_RC qdq::executeCreateQueue(qcStatement *aStatement)
{
#define IDE_FN "qdq::executeCreateQueue"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY("qdq::executeCreateQueue"));
    const void              * sSequenceHandle;
    SChar                     sSqlStr[QCM_MAX_SQL_STR_LEN];
    SChar                     sQueueName [ QC_MAX_OBJECT_NAME_LEN + 1 ];
    qdTableParseTree        * sParseTree;
    UInt                      sSeqTblID;
    smOID                     sSequenceOID;
    vSLong                    sRowCnt;
    
    SChar                     sTBSName[ SMI_MAX_TABLESPACE_NAME_LEN + 1 ];
    smiTableSpaceAttr         sTBSAttr;

    /* ======================== *
     * [1] Create Queue Table   *
     * ======================== */

    sParseTree = (qdTableParseTree *) aStatement->myPlan->parseTree;

    //BUG-48230: Queue Table은 MEMORY,VOLATILE 기반 테이블 이어야만 함
    IDE_TEST_RAISE( !( (sctTableSpaceMgr::isMemTableSpace(sParseTree->TBSAttr.mID)) ||
                       (sctTableSpaceMgr::isVolatileTableSpace(sParseTree->TBSAttr.mID)) ),
                    ERR_NO_MEMORY_OR_VOLATILE_TABLE );

    IDE_TEST( qdbCreate::executeCreateTable(aStatement) != IDE_SUCCESS );

    QC_STR_COPY( sQueueName, sParseTree->tableName );

    IDE_TEST(qcm::getNextTableID(aStatement, &sSeqTblID) != IDE_SUCCESS);

    IDE_TEST(
        smiTable::createSequence(
            QC_SMI_STMT( aStatement ),
            1,
            1,
            20,
            ((SLong)(ID_ULONG_MAX/2 -1)),
            1,
            SMI_SEQUENCE_CIRCULAR_ENABLE,
            &sSequenceHandle)
        != IDE_SUCCESS);

    sSequenceOID = smiGetTableId(sSequenceHandle);

    // PROJ-1502 PARTITIONED DISK TABLE
    // queue는 partition될 수 없음.
    // PROJ-1407 Temporary Table
    // queue는 temporary로 생성할 수 없음
    IDE_TEST( qcmTablespace::getTBSAttrByID( SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                                             & sTBSAttr )
              != IDE_SUCCESS );
    idlOS::memcpy( sTBSName, sTBSAttr.mName, sTBSAttr.mNameLength );
    sTBSName[sTBSAttr.mNameLength] = '\0';
    
    idlOS::snprintf(sSqlStr, ID_SIZEOF(sSqlStr),
                    "INSERT INTO SYS_TABLES_ VALUES( "
                    "INTEGER'%"ID_INT32_FMT"', "
                    "INTEGER'%"ID_INT32_FMT"', "
                    "BIGINT'%"ID_INT64_FMT"', "
                    "INTEGER'0', "
                    "VARCHAR'%s_NEXT_MSG_ID', "
                    "'W', " // W means QUEUE_SEQUENCE
                    "INTEGER'0', "
                    "INTEGER'0', "
                    "BIGINT'0', "
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
                    "INTEGER'1'," // Parallel degree
                    "CHAR'Y',"    // USABLE
                    "INTEGER'0'," // SHARD_FLAG
                    "SYSDATE, SYSDATE )",
                    sParseTree->userID,
                    sSeqTblID,
                    (ULong)sSequenceOID,
                    sQueueName,
                    SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                    sTBSName,
                    QD_MEMORY_TABLE_DEFAULT_PCTFREE,
                    QD_MEMORY_TABLE_DEFAULT_PCTUSED,
                    QD_MEMORY_TABLE_DEFAULT_INITRANS,
                    QD_MEMORY_TABLE_DEFAULT_MAXTRANS,
                    (ULong)QD_MEMORY_SEGMENT_DEFAULT_STORAGE_INITEXTENTS,
                    (ULong)QD_MEMORY_SEGMENT_DEFAULT_STORAGE_NEXTEXTENTS,
                    (ULong)QD_MEMORY_SEGMENT_DEFAULT_STORAGE_MINEXTENTS,
                    (ULong)QD_MEMORY_SEGMENT_DEFAULT_STORAGE_MAXEXTENTS );

    IDE_TEST(qcg::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                sSqlStr,
                                & sRowCnt ) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_NO_MEMORY_OR_VOLATILE_TABLE)
    {
        IDE_SET( ideSetErrorCode(qpERR_ABORT_QDQ_USE_QUEUE_TABLE_IN_DISK_TABLESPACE) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC qdq::executeDropQueue(qcStatement *aStatement)
{
#define IDE_FN "qdq::executeDropQueue"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY("qdq::executeDropQueue"));

    qdDropParseTree    * sParseTree;
    UInt                 sTableID;
    UChar                sQueueName[QC_MAX_OBJECT_NAME_LEN + 1];
    SChar                sSequenceName[QC_MAX_SEQ_NAME_LEN + 1];
    void               * sSequenceHandle;
    qcmSequenceInfo      sSequenceInfo;

    qciArgDropQueue      sArgDropQueue;

    sParseTree = (qdDropParseTree *)aStatement->myPlan->parseTree;

    //---------------------------------------
    // TASK-2176 Table에 대한 Lock을 획득한다.
    //---------------------------------------

    IDE_TEST( qcm::validateAndLockTable(aStatement,
                                        sParseTree->tableHandle,
                                        sParseTree->tableSCN,
                                        SMI_TABLE_LOCK_X)
              != IDE_SUCCESS );

    sTableID = sParseTree->tableInfo->tableID;

    //---------------------------------------
    // DROP QUEUE TABLE
    //---------------------------------------

    IDE_TEST( qdd::executeDropTable(aStatement) != IDE_SUCCESS );

    //---------------------------------------
    // DROP QUEUE SEQUENCE
    //---------------------------------------

    QC_STR_COPY( sQueueName, sParseTree->objectName );

    idlOS::snprintf(sSequenceName,
                    ID_SIZEOF(sSequenceName),
                    "%s_NEXT_MSG_ID",
                    sQueueName);

    IDE_TEST( qcm::getSequenceInfoByName( QC_SMI_STMT( aStatement ),
                                          sParseTree->userID,
                                          (UChar*)sSequenceName,
                                          idlOS::strlen(sSequenceName),
                                          &sSequenceInfo,
                                          &sSequenceHandle )
              != IDE_SUCCESS);

    IDE_TEST_RAISE(sSequenceInfo.sequenceType != QCM_QUEUE_SEQUENCE,
                   ERR_META_CRASH);

    IDE_TEST(qdd::deleteTableFromMeta(aStatement, sSequenceInfo.sequenceID)
             != IDE_SUCCESS);

    IDE_TEST(qdpDrop::removePriv4DropTable(aStatement, sSequenceInfo.sequenceID)
             != IDE_SUCCESS);

    // PROJ-2223 audit
    IDE_TEST( qcmAudit::deleteObject( aStatement,
                                      sSequenceInfo.sequenceID,
                                      sSequenceInfo.name )
              != IDE_SUCCESS );
    
    // delete from sm
    IDE_TEST( smiTable::dropSequence( QC_SMI_STMT( aStatement ),
                                      sSequenceHandle )
              != IDE_SUCCESS );

    /* PROJ-2197 PSM Renewal */
    // related PSM
    IDE_TEST(qcmProc::relSetInvalidProcOfRelated(
        aStatement,
        sParseTree->userID,
        sSequenceInfo.name,
        idlOS::strlen((SChar*)sSequenceInfo.name),
        QS_TABLE) != IDE_SUCCESS);

    // PROJ-1073 Package
    IDE_TEST(qcmPkg::relSetInvalidPkgOfRelated(
        aStatement,
        sParseTree->userID,
        sSequenceInfo.name,
        idlOS::strlen((SChar*)sSequenceInfo.name),
        QS_TABLE) != IDE_SUCCESS);


    sArgDropQueue.mTableID   = sTableID;
    sArgDropQueue.mMmSession = aStatement->session->mMmSession;
    
    // MM의 QUEUE관련 데이터 삭제요청 (commit시 삭제됨)
    IDE_TEST( qcg::mDropQueueFuncPtr( (void *)&sArgDropQueue )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_META_CRASH);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QCM_META_CRASH));
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC qdq::executeEnqueue(qcStatement *aStatement)
{
#define IDE_FN "qdq::executeEnqueue"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY("qdq::executeStart"));

    IDE_TEST( qmx::executeInsertValues(aStatement) != IDE_SUCCESS );

    IDE_TEST ( wakeupDequeue((qcStatement *)aStatement) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC qdq::wakeupDequeue(qcStatement * /*aStatement*/ )
{
    return IDE_SUCCESS;
}

IDE_RC qdq::executeDequeue(qcStatement * /*aStatement*/,
                           idBool      * /*aIsTimeOut*/ )
{
#define IDE_FN "qdq::executeDequeue"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY("qdq::executeStart"));


    return IDE_SUCCESS;
#undef IDE_FN
}

IDE_RC qdq::waitForEnqueue(qcStatement * /*aStatement */,
                           idBool      * /*aIsTimeOut */ )
{
#define IDE_FN "qdq::waitForEnqueue"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY("qdq::waitForEnqueue"));

    return IDE_SUCCESS;

#undef IDE_FN
}

IDE_RC qdq::validateAlterCompactQueue(qcStatement * aStatement)
{
/***********************************************************************
 *
 * Description : ALTER QUEUE queue_name COMPACT 구문의 validation
 *
 * Implementation :
 *    (1) Queue가 존재하는지 검사
 *    (2) 권한 검사
 *
 *    replication 안됨
 * 
 *    ALTER TABLE권한은 막혀있음! 오직 COMPACT만 됨
 *    따라서 'qdbAlter::checkOperatable()' 함수 호출할 필요없음
 *    만약 ALTER TABLE 구문으로 Queue Table을 변경하고자 한다면
 *    이 부분을 열도록 해야함
 * 
 ***********************************************************************/    
#define IDE_FN "qdq::validateAlterQueueCompact"

    qdTableParseTree    * sParseTree;
    UInt                  sTableType;
    qcuSqlSourceInfo      sqlInfo;

    sParseTree = (qdTableParseTree *)aStatement->myPlan->parseTree;

    /* BUG-30059 */
    if ( qdbCommon::containDollarInName( &(sParseTree->tableName) ) == ID_TRUE )
    {
        sqlInfo.setSourceInfo(
            aStatement,
            &(sParseTree->tableName) );

        IDE_RAISE( CANT_USE_RESERVED_WORD );
    }

    //----------------------------
    // Queue 가 존재하는지 검사
    //----------------------------
    if ( qdbCommon::checkTableInfo( aStatement,
                                    sParseTree->userName,
                                    sParseTree->tableName,
                                    &(sParseTree->userID),
                                    &(sParseTree->tableInfo),
                                    &(sParseTree->tableHandle),
                                    &(sParseTree->tableSCN))
         == IDE_SUCCESS )
    {
        // tableInfo가 있는 경우
        if( sParseTree->tableInfo->tableType == QCM_QUEUE_TABLE )
        {
            // Queue Table인지 검사
        }
        else
        {
            IDE_RAISE(ERR_NOT_FOUND_QUEUE);
        }
    }
    else
    {
        IDE_RAISE(ERR_NOT_FOUND_QUEUE);
    }

    /* BUG-
       ALTER TABLE 권한은 막혀있음
    IDE_TEST( qdbAlter::checkOperatable( aStatement,
                                         sParseTree->tableInfo )
              != IDE_SUCCESS );
    */
    
    //----------------------------
    // 테이블에 LOCK(IS)
    //----------------------------
    IDE_TEST( qcm::lockTableForDDLValidation(aStatement,
                                             sParseTree->tableHandle,
                                             sParseTree->tableSCN)
              != IDE_SUCCESS );

    if ( QCG_GET_SESSION_USER_ID(aStatement) != QC_SYSTEM_USER_ID )
    {
        // sParseTree->userID is a owner of table
        IDE_TEST_RAISE(sParseTree->userID == QC_SYSTEM_USER_ID,
                       ERR_NOT_ALTER_META);
    }
    else
    {
        if ( ( aStatement->session->mQPSpecific.mFlag
               & QC_SESSION_ALTER_META_MASK )
             == QC_SESSION_ALTER_META_DISABLE )
        {
            IDE_RAISE(ERR_NOT_ALTER_META);
        }
    }

    //----------------------------
    // 권한 검사
    //----------------------------

    IDE_TEST( qdpRole::checkDDLAlterTablePriv( aStatement,
                                               sParseTree->tableInfo )
              != IDE_SUCCESS );

    //----------------------------
    // PROJ-1723 [MDW/INTEGRATOR] Altibase Plugin 개발
    //   DDL Statement Text의 로깅
    //----------------------------
    
    if (QCU_DDL_SUPPLEMENTAL_LOG == 1)
    {
        IDE_TEST( qciMisc::writeDDLStmtTextLog( aStatement,
                                                sParseTree->tableInfo->tableOwnerID,
                                                sParseTree->tableInfo->name )
                  != IDE_SUCCESS );
    }

    //----------------------------
    // Queue Table은 MEMORY 기반 테이블 이어야만 함
    //----------------------------
    sTableType = sParseTree->tableInfo->tableFlag & SMI_TABLE_TYPE_MASK;
    IDE_TEST_RAISE( (sTableType != SMI_TABLE_MEMORY), 
                    ERR_NO_MEMORY_OR_VOLATILE_TABLE );

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_NOT_FOUND_QUEUE)
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDQ_NOT_FOUND_QUEUE));
    }
    IDE_EXCEPTION(ERR_NOT_ALTER_META);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDD_NO_DROP_META_TABLE));
    }
    IDE_EXCEPTION(ERR_NO_MEMORY_OR_VOLATILE_TABLE)
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDB_NO_MEMORY_OR_VOLATILE_TABLE));
    }
    IDE_EXCEPTION( CANT_USE_RESERVED_WORD );
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET(
            ideSetErrorCode(qpERR_ABORT_QDB_RESERVED_WORD_IN_OBJECT_NAME,
                            sqlInfo.getErrMessage() ));
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC qdq::executeCompactQueue(qcStatement * aStatement)
{
/***********************************************************************
 *
 * Description : ALTER QUEUE queue_name COMPACT 구문의 execution
 *
 * Implementation :
 *
 ***********************************************************************/       
#define IDE_FN "qdq::executeCompactQueue"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY("qdq::executeCompactQueue"));

    IDE_TEST( qdbAlter::executeCompactTable(aStatement) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;


#undef IDE_FN
}

IDE_RC qdq::executeStart(qcStatement * /*aStatement*/)
{
#define IDE_FN "qdq::executeStart"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY("qdq::executeStart"));

    return IDE_SUCCESS;

#undef IDE_FN
}

IDE_RC qdq::executeStop(qcStatement * /*aStatement*/)
{
#define IDE_FN "qdq::executeStart"

    return IDE_SUCCESS;

#undef IDE_FN
}

/* BUG-45921 */
IDE_RC qdq::validateAlterQueueSequence( qcStatement * aStatement )
{
    qcuSqlSourceInfo           sqlInfo;
    qdQueueSequenceParseTree * sParseTree;
    SLong                      sRowCount;

    sParseTree = (qdQueueSequenceParseTree *)aStatement->myPlan->parseTree;

    /* BUG-30059 */
    if ( qdbCommon::containDollarInName( &( sParseTree->mQueueName ) ) == ID_TRUE )
    {
        sqlInfo.setSourceInfo( aStatement, &( sParseTree->mQueueName ) );

        IDE_RAISE( CANT_USE_RESERVED_WORD );
    }

    /* QUEUE 확인 */
    IDE_TEST_RAISE( qdbCommon::checkTableInfo( aStatement,
                                               sParseTree->mUserName,
                                               sParseTree->mQueueName,
                                               &( sParseTree->mUserID ),
                                               &( sParseTree->mTableInfo ),
                                               &( sParseTree->mTableHandle ),
                                               &( sParseTree->mTableSCN ) )
                    != IDE_SUCCESS,
                    ERR_NOT_FOUND_QUEUE );

    /* LOCK( IS ) */
    IDE_TEST( qcm::lockTableForDDLValidation( aStatement,
                                              sParseTree->mTableHandle,
                                              sParseTree->mTableSCN )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( sParseTree->mTableInfo->tableType != QCM_QUEUE_TABLE,
                    ERR_NOT_FOUND_QUEUE );

    /* 데이터 확인 */
    IDE_TEST( smiStatistics::getTableStatNumRow( (void*) sParseTree->mTableInfo->tableHandle,
                                                 ID_TRUE, /* CurrentValue */
                                                 QC_SMI_STMT( aStatement )->getTrans(),
                                                 &( sRowCount ) )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( sRowCount != 0, ERR_NOT_EMPTY_QUEUE );

    /* SEQUENCE 확인 */
    IDE_TEST_RAISE( checkQueueSequenceInfo( aStatement,
                                            sParseTree->mUserID,
                                            sParseTree->mQueueName,
                                            &( sParseTree->mQueueSequenceInfo ),
                                            &( sParseTree->mQueueSequenceHandle ) )
                    != IDE_SUCCESS,
                    ERR_META_CRASH );

    IDE_TEST_RAISE( sParseTree->mQueueSequenceInfo.sequenceType != QCM_QUEUE_SEQUENCE,
                    ERR_META_CRASH );

    /* 권한 검사 1 */
    if ( QCG_GET_SESSION_USER_ID( aStatement ) != QC_SYSTEM_USER_ID )
    {
        /* META */
        IDE_TEST_RAISE( sParseTree->mUserID == QC_SYSTEM_USER_ID,
                        ERR_NOT_ALTER_META );
    }
    else
    {
        if ( ( aStatement->session->mQPSpecific.mFlag & QC_SESSION_ALTER_META_MASK )
             == QC_SESSION_ALTER_META_DISABLE )
        {
            IDE_RAISE( ERR_NOT_ALTER_META );
        }
        else
        {
            /* Nothing to do */
        }
    }

    /* 권한 검사 2 */
    IDE_TEST( qdpRole::checkDDLAlterTablePriv( aStatement,
                                               sParseTree->mTableInfo )
              != IDE_SUCCESS );

    IDE_TEST( qdpRole::checkDDLAlterSequencePriv( aStatement,
                                                  &( sParseTree->mQueueSequenceInfo ) )
              != IDE_SUCCESS );

    /* Replication 검사 */
    IDE_TEST_RAISE( sParseTree->mTableInfo->replicationCount > 0,
                    ERR_UNSUPPORTED );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_FOUND_QUEUE )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDQ_NOT_FOUND_QUEUE ) );
    }
    IDE_EXCEPTION( ERR_NOT_EMPTY_QUEUE )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDQ_NOT_EMPTY_QUEUE ) );
    }
    IDE_EXCEPTION( ERR_META_CRASH )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QCM_META_CRASH ) );
    }
    IDE_EXCEPTION( ERR_NOT_ALTER_META )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDD_NO_DROP_META_TABLE ) );
    }
    IDE_EXCEPTION( ERR_UNSUPPORTED )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QRC_NOT_SUPPORT_REPLICATION_DDL ) );
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

IDE_RC qdq::executeAlterQueueSequence( qcStatement * aStatement )
{
    qdQueueSequenceParseTree * sParseTree;

    sParseTree = (qdQueueSequenceParseTree *)aStatement->myPlan->parseTree;

    IDE_TEST( qcm::validateAndLockTable( aStatement,
                                         sParseTree->mTableHandle,
                                         sParseTree->mTableSCN,
                                         SMI_TABLE_LOCK_X )
              != IDE_SUCCESS );

    IDE_TEST( smiTable::resetSequence( QC_SMI_STMT( aStatement ),
                                       sParseTree->mQueueSequenceHandle )
             != IDE_SUCCESS );

    IDE_TEST( updateQueueSequenceFromMeta( aStatement,
                                           sParseTree->mUserID,
                                           sParseTree->mQueueName,
                                           sParseTree->mQueueSequenceInfo.sequenceID,
                                           smiGetTableId( sParseTree->mQueueSequenceHandle ),
                                           0,
                                           1 ) /* PROJ-1071 default parallel degree = 1 */
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdq::checkQueueSequenceInfo( qcStatement      * aStatement,
                                    UInt               aUserID,
                                    qcNamePosition     aQueueName,
                                    qcmSequenceInfo  * aQueueSequenceInfo,
                                    void            ** aQueueSequenceHandle )
{
    SChar sQueueName[QC_MAX_OBJECT_NAME_LEN + 1];
    SChar sQueueSequenceName[QC_MAX_SEQ_NAME_LEN + 1];

    /* Queue Sequence 이름 생성 */
    QC_STR_COPY( sQueueName, aQueueName );

    idlOS::snprintf( sQueueSequenceName,
                     ID_SIZEOF( sQueueSequenceName ),
                    "%s_NEXT_MSG_ID",
                    sQueueName );

    /* Queue Sequence 찾기 */
    IDE_TEST( qcm::getSequenceInfoByName( QC_SMI_STMT( aStatement ),
                                          aUserID,
                                          (UChar*) sQueueSequenceName,
                                          idlOS::strlen( sQueueSequenceName ),
                                          aQueueSequenceInfo,
                                          aQueueSequenceHandle )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdq::updateQueueSequenceFromMeta( qcStatement    * aStatement,
                                         UInt             aUserID,
                                         qcNamePosition   aQueueNamePos,
                                         UInt             aQueueSequenceID,
                                         smOID            aQueueSequenceOID,
                                         SInt             aColumnCount,
                                         UInt             aParallelDegree )
{
    SChar  * sSqlStr;
    vSLong   sRowCnt;
    SChar    sQueueName[QC_MAX_OBJECT_NAME_LEN + 1];
    SChar    sQueueSequenceName[QC_MAX_SEQ_NAME_LEN + 1];

    /* Queue Sequence 이름 생성 */
    QC_STR_COPY( sQueueName, aQueueNamePos );

    idlOS::snprintf( sQueueSequenceName,
                     ID_SIZEOF( sQueueSequenceName ),
                    "%s_NEXT_MSG_ID",
                    sQueueName );

    IDU_FIT_POINT( "qdq::updateQueueSequenceFromMeta::alloc::sSqlStr",
                   idERR_ABORT_InsufficientMemory );

    IDE_TEST( STRUCT_ALLOC_WITH_SIZE( aStatement->qmxMem,
                                      SChar,
                                      QD_MAX_SQL_LENGTH,
                                      & sSqlStr )
              != IDE_SUCCESS );

    idlOS::snprintf( sSqlStr,
                     QD_MAX_SQL_LENGTH,
                     "UPDATE SYS_TABLES_ "
                     "SET USER_ID = INTEGER'%"ID_INT32_FMT"', "
                     "TABLE_NAME = '%s', "
                     "TABLE_OID = BIGINT'%"ID_INT64_FMT"', "
                     "COLUMN_COUNT = INTEGER'%"ID_INT32_FMT"', "
                     "PARALLEL_DEGREE = INTEGER'%"ID_INT32_FMT"', "
                     "LAST_DDL_TIME = SYSDATE "  // fix BUG-14394
                     "WHERE TABLE_ID = INTEGER'%"ID_INT32_FMT"'",
                     aUserID,
                     sQueueSequenceName,
                     (ULong)aQueueSequenceOID,
                     aColumnCount,
                     aParallelDegree,
                     aQueueSequenceID );

    IDE_TEST( qcg::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                 sSqlStr,
                                 & sRowCnt )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( sRowCnt != 1, ERR_META_CRASH );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_META_CRASH )
    {
        IDE_SET(ideSetErrorCode( qpERR_ABORT_QCM_META_CRASH ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdq::validateAlterAllowDelete( qcStatement * aStatement )
{
/***********************************************************************
 *
 * Description : ALTER QUEUE queue_name DELETE ON/OFF 구문의 validation
 *
 * Implementation :
 *
 ***********************************************************************/    
    qdTableParseTree    * sParseTree = NULL;
    UInt                  sTableType;
    qcuSqlSourceInfo      sqlInfo;
    qdTableAttrFlagList * sAttrFlag = NULL;

    sParseTree = (qdTableParseTree *)aStatement->myPlan->parseTree;

    /* BUG-30059 */
    if ( qdbCommon::containDollarInName( &(sParseTree->tableName) ) == ID_TRUE )
    {
        sqlInfo.setSourceInfo( aStatement,
                               &(sParseTree->tableName) );

        IDE_RAISE( CANT_USE_RESERVED_WORD );
    }

    //----------------------------
    // Queue 가 존재하는지 검사
    //----------------------------
    if ( qdbCommon::checkTableInfo( aStatement,
                                    sParseTree->userName,
                                    sParseTree->tableName,
                                    &(sParseTree->userID),
                                    &(sParseTree->tableInfo),
                                    &(sParseTree->tableHandle),
                                    &(sParseTree->tableSCN))
         == IDE_SUCCESS )
    {
        // tableInfo가 있는 경우
        if ( sParseTree->tableInfo->tableType == QCM_QUEUE_TABLE )
        {
            // Queue Table인지 검사
        }
        else
        {
            IDE_RAISE( ERR_NOT_FOUND_QUEUE );
        }
    }
    else
    {
        IDE_RAISE( ERR_NOT_FOUND_QUEUE );
    }

    //----------------------------
    // 테이블에 LOCK(IS)
    //----------------------------
    IDE_TEST( qcm::lockTableForDDLValidation( aStatement,
                                              sParseTree->tableHandle,
                                              sParseTree->tableSCN )
              != IDE_SUCCESS );

    /* 권한 검사 1 */
    if ( QCG_GET_SESSION_USER_ID( aStatement ) != QC_SYSTEM_USER_ID )
    {
        // sParseTree->userID is a owner of table
        IDE_TEST_RAISE( sParseTree->userID == QC_SYSTEM_USER_ID,
                        ERR_NOT_ALTER_META );
    }
    else
    {
        IDE_TEST_RAISE( ( aStatement->session->mQPSpecific.mFlag & QC_SESSION_ALTER_META_MASK )
                        == QC_SESSION_ALTER_META_DISABLE,
                        ERR_NOT_ALTER_META );
    }

    /* 권한 검사 2 */
    IDE_TEST( qdpRole::checkDDLAlterTablePriv( aStatement,
                                               sParseTree->tableInfo )
              != IDE_SUCCESS );

    //----------------------------
    // PROJ-1723 [MDW/INTEGRATOR] Altibase Plugin 개발
    //   DDL Statement Text의 로깅
    //----------------------------
    if ( QCU_DDL_SUPPLEMENTAL_LOG == 1 )
    {
        IDE_TEST( qciMisc::writeDDLStmtTextLog( aStatement,
                                                sParseTree->tableInfo->tableOwnerID,
                                                sParseTree->tableInfo->name )
                  != IDE_SUCCESS );
    }

    //----------------------------
    // Queue Table은 MEMORY,VOLATILE 기반 테이블 이어야만 함
    //----------------------------
    sTableType = sParseTree->tableInfo->tableFlag & SMI_TABLE_TYPE_MASK;
    IDE_TEST_RAISE( !( (sTableType == SMI_TABLE_MEMORY) ||
                       (sTableType == SMI_TABLE_VOLATILE) ), 
                    ERR_NO_MEMORY_OR_VOLATILE_TABLE );

    //----------------------------
    // check Attribute
    //----------------------------
    sAttrFlag = sParseTree->tableAttrFlagList;

    /* 변경할 속성은 반드시 존재해야한다. */
    IDE_TEST_RAISE( sAttrFlag == NULL, ERR_NOT_ALLOW_ATTRIBUTE );
    /* List내에 오직 하나의 Attribuate Flag만 있어야 함 */
    IDE_TEST_RAISE( sAttrFlag->next != NULL, ERR_NOT_ALLOW_ATTRIBUTE );
    /* QUEUE 관련 attribute인지 MASK 확인 */
    IDE_TEST_RAISE( sAttrFlag->attrMask != SMI_TABLE_QUEUE_ALLOW_DELETE_MASK,
                    ERR_NOT_ALLOW_ATTRIBUTE );

    sParseTree->tableAttrMask = 0;
    sParseTree->tableAttrValue = 0;

    sParseTree->tableAttrMask  |= sAttrFlag->attrMask;
    sParseTree->tableAttrValue |= sAttrFlag->attrValue;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_FOUND_QUEUE )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDQ_NOT_FOUND_QUEUE ) );
    }
    IDE_EXCEPTION( ERR_NOT_ALTER_META )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDD_NO_DROP_META_TABLE ) );
    }
    IDE_EXCEPTION( ERR_NO_MEMORY_OR_VOLATILE_TABLE )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDB_NO_MEMORY_OR_VOLATILE_TABLE ) );
    }
    IDE_EXCEPTION( CANT_USE_RESERVED_WORD );
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDB_RESERVED_WORD_IN_OBJECT_NAME,
                                  sqlInfo.getErrMessage() ) );
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION( ERR_NOT_ALLOW_ATTRIBUTE )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qdq::validateAlterAllowDelete",
                                  "Invalid table attribute in QUEUE" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
