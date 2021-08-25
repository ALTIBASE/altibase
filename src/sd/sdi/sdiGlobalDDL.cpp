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
#include <sdDef.h>
#include <sdi.h>
#include <sdiGlobalDDL.h>

IDE_RC sdiGlobalDDL::execute( qcStatement    * aStatement )
{
/***********************************************************************
 *
 * Description :
 *    PROJ-g757 Advanced Global DDL
 *
 *    DDL 구문을 Global DDL로 수행하는 기본 executor
 *
 * Implementation :
 *      1. 기본 검사
 *
 *      2. 모든 노드로 DDL 구문을 수행한다.
 *
 *      3. Backup 테이블에 동일한 DDL 구문을 실행한다.
 *
 ***********************************************************************/
    sdiLocalMetaInfo    sLocalMetaInfo;

    SChar  sUserName[QC_MAX_OBJECT_NAME_LEN + 1]  = {0, };
    SChar  sTableName[QC_MAX_OBJECT_NAME_LEN + 1] = {0, };

    IDE_TEST( sdm::getLocalMetaInfo( &sLocalMetaInfo ) != IDE_SUCCESS );

    /* 1. 기본 검사 */
    IDE_TEST( checkProperties( aStatement,
                               sLocalMetaInfo.mKSafety ) != IDE_SUCCESS );

    IDE_TEST( initShardEnv( aStatement ) != IDE_SUCCESS );

    getTableName( aStatement, sUserName, sTableName );

    /* 2. 모든 노드로 DDL 구문을 수행한다. */
    IDE_TEST( executeDDLToAllNodes( aStatement,
                                    sLocalMetaInfo.mKSafety,
                                    sUserName,
                                    sTableName ) != IDE_SUCCESS );

    /* 3. Backup 테이블에 동일한 DDL 구문을 실행한다. */
    IDE_TEST( executeForBackup( aStatement ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdiGlobalDDL::executeCreateTable( qcStatement    * aStatement )
{
/***********************************************************************
 *
 * Description :
 *    PROJ-2757 Advanced Global DDL
 *
 *    CREATE TABLE DDL 구문을 Global DDL로 수행하는 executor
 *
 * Implementation :
 *      1. 기본 검사
 *
 *      2. 모든 노드로 DDL 구문을 실행한다.
 *
 ***********************************************************************/
    UInt            sExecCount = 0;

    /* 1. 기본 검사 */
    IDE_TEST( checkProperties( aStatement,
                               0 /* KSafety */ ) != IDE_SUCCESS );

    IDE_TEST( initShardEnv( aStatement ) != IDE_SUCCESS );

    /* 2. 모든 노드로 DDL 구문을 실행한다. */
    IDE_TEST( sdi::shardExecDirectNested( aStatement,
                                          NULL,
                                          QCI_STMTTEXT( aStatement ),
                                          QCI_STMTTEXTLEN( aStatement ),
                                          SDI_INTERNAL_OP_NOT,
                                          &sExecCount )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( sExecCount == 0, ERR_INVALID_SHARD_NODE );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_SHARD_NODE );
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDF_INVALID_SHARD_NODE ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdiGlobalDDL::executeDropIndex( qcStatement    * aStatement )
{
/***********************************************************************
 *
 * Description :
 *    PROJ-2757 Advanced Global DDL
 *
 *    DROP INDEX DDL 구문을 Global DDL로 수행하는 executor
 *
 * Implementation :
 *      1. 기본 검사
 *
 *      2. 모든 노드로 DDL 구문을 실행한다.
 *
 *      3. Backup 테이블에 동일한 DDL 구문을 실행한다.
 *
 ***********************************************************************/
    qcmTableInfo         * sInfo = NULL;
    sdiLocalMetaInfo       sLocalMetaInfo;

    SChar  sUserName[QC_MAX_OBJECT_NAME_LEN + 1]  = {0, };
    SChar  sTableName[QC_MAX_OBJECT_NAME_LEN + 1] = {0, };

    IDE_TEST( sdm::getLocalMetaInfo( &sLocalMetaInfo ) != IDE_SUCCESS );

    /* 1. 기본 검사 */
    IDE_TEST( checkProperties( aStatement,
                               sLocalMetaInfo.mKSafety ) != IDE_SUCCESS );

    IDE_TEST( initShardEnv( aStatement ) != IDE_SUCCESS );

    sInfo = ((qdDropParseTree *)aStatement->myPlan->parseTree)->tableInfo;

    idlOS::strcpy( sUserName, sInfo->tableOwnerName );
    idlOS::strcpy( sTableName, sInfo->name );

    /* 2. 모든 노드로 DDL 구문을 수행한다. */
    IDE_TEST( executeDDLToAllNodes( aStatement,
                                    sLocalMetaInfo.mKSafety,
                                    sUserName,
                                    sTableName ) != IDE_SUCCESS );

    /* 3. Backup 테이블에 동일한 DDL 구문을 실행한다. */
    IDE_TEST( executeForBackup( aStatement ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdiGlobalDDL::executeReplace( qcStatement    * aStatement )
{
/***********************************************************************
 *
 * Description :
 *    PROJ-2757 Advanced Global DDL
 *
 *    REPLACE TABLE/PARTITION DDL 구문을 Global DDL로 수행하는 executor.
 *    Non-Shard 테이블에만 허용.
 *    이중화 대상 테이블이 될 수 없음.
 *
 * Implementation :
 *      1. 기본 검사
 *
 *      2. parseTree를 참조해서 테이블/파티션 Lock
 *        2-1. REPLACE PARTITION : table에 IX Lock, partition에 X Lock
 *        2-2. REPLACE TABLE : table에 X Lock
 *
 *      3. 모든 노드로 DDL 구문을 실행한다.
 *
 ***********************************************************************/
    qdTableParseTree      * sParseTree;
    UInt                    sExecCount = 0;

    /* 1. 기본 검사 */
    IDE_TEST( checkProperties( aStatement,
                               0 /* KSafety */ ) != IDE_SUCCESS );

    IDE_TEST( initShardEnv( aStatement ) != IDE_SUCCESS );

    /* 2. parseTree를 참조해서 테이블/파티션 Lock */
    sParseTree = (qdTableParseTree *)aStatement->myPlan->parseTree;

    if ( sParseTree->mPartAttr != NULL )
    {
        /* 2-1. REPLACE PARTITION : table에 IX Lock, partition에 X Lock */
        // lock target table
        IDE_TEST( lockTableInAllShardNodes( aStatement,
                                            sParseTree->userName,
                                            sParseTree->tableName,
                                            SMI_TABLE_LOCK_IX )
                  != IDE_SUCCESS );

        // lock source table
        IDE_TEST( lockTableInAllShardNodes( aStatement,
                                            sParseTree->mSourceUserName,
                                            sParseTree->mSourceTableName,
                                            SMI_TABLE_LOCK_IX )
                  != IDE_SUCCESS );

        // lock target table.partition
        IDE_TEST( lockPartitionInAllShardNodes( aStatement,
                                                sParseTree->userName,
                                                sParseTree->tableName,
                                                sParseTree->mPartAttr->tablePartName )
                  != IDE_SUCCESS );

        // lock source table.partition
        IDE_TEST( lockPartitionInAllShardNodes( aStatement,
                                                sParseTree->mSourceUserName,
                                                sParseTree->mSourceTableName,
                                                sParseTree->mPartAttr->tablePartName )
                  != IDE_SUCCESS );
    }
    else
    {
        /* 2-2. REPLACE TABLE : table에 X Lock */
        // lock target table
        IDE_TEST( lockTableInAllShardNodes( aStatement,
                                            sParseTree->userName,
                                            sParseTree->tableName,
                                            SMI_TABLE_LOCK_X )
                  != IDE_SUCCESS );

        // lock source table
        IDE_TEST( lockTableInAllShardNodes( aStatement,
                                            sParseTree->mSourceUserName,
                                            sParseTree->mSourceTableName,
                                            SMI_TABLE_LOCK_X )
                  != IDE_SUCCESS );
    }

    ideLog::log(IDE_SD_17,"\n[sdiGlobalDDL] DDL: %s\n", QCI_STMTTEXT( aStatement ));
    /* 3. 모든 노드로 DDL 구문을 실행한다. */
    IDE_TEST( sdi::shardExecDirectNested( aStatement,
                                          NULL,
                                          QCI_STMTTEXT( aStatement ),
                                          QCI_STMTTEXTLEN( aStatement ),
                                          SDI_INTERNAL_OP_NOT,
                                          &sExecCount )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( sExecCount == 0, ERR_INVALID_SHARD_NODE );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_SHARD_NODE );
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDF_INVALID_SHARD_NODE ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdiGlobalDDL::executeSplitPartition( qcStatement    * aStatement )
{
/***********************************************************************
 *
 * Description :
 *    PROJ-2757 Advanced Global DDL
 *
 *    SPLIT PARTITION DDL 구문을 Global DDL로 수행하는 executor.
 *    단, HASH, LIST, RANGE method로 분산된 shard 테이블만 여기에서
 *    처리되고 나머지는 sdiGlobalDDL::execute에서 처리됨.
 *
 * Implementation :
 *      1. session property를 검사한다.
 *
 *      2. ShardMeta 변경을 위해 session SMN 검사
 *
 *      3. ShardMeta Lock
 *
 *      4. ShardMeta Touch
 *
 *      5. 모든 노드가 정상인지 검사한다.
 *
 *      6. SMN 구하기
 *
 *      7. 모든 노드로 DDL 구문을 실행한다.
 *
 *      8. Backup 테이블에 동일한 DDL 구문을 실행한다.
 *
 ***********************************************************************/
    idBool              sIsAlive             = ID_FALSE;
    ULong               sSMN = ID_ULONG(0);
    sdiClientInfo     * sClientInfo          = NULL;
    sdiLocalMetaInfo    sLocalMetaInfo;

    SChar  sUserName[QC_MAX_OBJECT_NAME_LEN + 1]  = {0, };
    SChar  sTableName[QC_MAX_OBJECT_NAME_LEN + 1] = {0, };

    IDE_TEST( sdm::getLocalMetaInfo( &sLocalMetaInfo ) != IDE_SUCCESS );

    /* 1. 기본 검사 */
    IDE_TEST( checkProperties( aStatement,
                               sLocalMetaInfo.mKSafety ) != IDE_SUCCESS );

    /* 2. ShardMeta 변경을 위해 session SMN 검사 */
    IDE_TEST( sdi::compareDataAndSessionSMN( aStatement )
              != IDE_SUCCESS );

    /* 3. ShardMeta Lock */
    IDE_TEST( sdiZookeeper::getShardMetaLock(
                QC_SMI_STMT(aStatement)->getTrans()->getTransID() )
              != IDE_SUCCESS );

    sdiZookeeper::globalDDLJob();

    // for qci::endTransForSession, for callAfterCommit/callAfterRollback
    aStatement->session->mQPSpecific.mFlag &= ~QC_SESSION_GLOBAL_DDL_MASK;
    aStatement->session->mQPSpecific.mFlag |= QC_SESSION_GLOBAL_DDL_TRUE;

    /* 4. ShardMeta Touch */
    sdi::setShardMetaTouched( aStatement->session );
    
    /* 5. 모든 노드가 정상인지 검사한다. */
    IDE_TEST( sdiZookeeper::checkAllNodeAlive( &sIsAlive )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( sIsAlive != ID_TRUE, ERR_CLUSTER_STATE );

    /* 6. SMN 구하기 */
    IDE_TEST( sdi::getIncreasedSMNForMetaNode(
                ( QC_SMI_STMT( aStatement ) )->getTrans() , 
                &sSMN ) 
              != IDE_SUCCESS );

    // shard linker 검사 & 초기화
    IDE_TEST( sdi::checkShardLinker( aStatement ) != IDE_SUCCESS );
        
    sClientInfo = QC_SHARD_CLIENT_INFO( aStatement->session );

    IDE_TEST_RAISE( sClientInfo == NULL, ERR_NODE_NOT_EXIST );

    getTableName( aStatement, sUserName, sTableName );

    /* 7. 모든 노드로 DDL 구문을 실행한다. */
    IDE_TEST( executeDDLToAllNodes( aStatement,
                                    sLocalMetaInfo.mKSafety,
                                    sUserName,
                                    sTableName ) != IDE_SUCCESS );

    /* 8. Backup 테이블에 동일한 DDL 구문을 실행한다. */
    IDE_TEST( executeForBackup( aStatement ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_CLUSTER_STATE )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_ZKC_DEADNODE_EXIST ) );
    }
    IDE_EXCEPTION( ERR_NODE_NOT_EXIST )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDM_SHARD_NODE_NOT_EXIST ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdiGlobalDDL::updateShardMetaForSplitPartition(
                                            qcStatement * aStatement,
                                            SChar       * aUserName,
                                            SChar       * aTableName )
{
/***********************************************************************
 *
 * Description :
 *    PROJ-2757 Advanced Global DDL
 *
 *    Coordinator 세션의 qdbAlter::executeSplitPartition 수행으로부터
 *    호출되며, SYS_SHARD.RANGES_에 새로 생성된 파티션 정보를 입력한다.
 *
 ***********************************************************************/
    SChar           sSrcPartName[QC_MAX_OBJECT_NAME_LEN + 1];
    SChar           sDstPartName[QC_MAX_OBJECT_NAME_LEN + 1];
    SChar           sSplitValue[SDI_RANGE_VARCHAR_MAX_PRECISION + 1];

    UInt                      sRowCnt = 0;
    smiStatement            * sOldStmt;
    smiStatement              sSmiStmt;
    UInt                      sSmiStmtFlag;
    SInt                      sState = 0;
    idBool                    sIsOldSessionShardMetaTouched = ID_FALSE;
    ULong                     sSMN = ID_ULONG(0);

    qdTableParseTree        * sParseTree;
    qdPartitionAttribute    * sSrcPartAttr;
    qdPartitionAttribute    * sDstPartAttr;

    aStatement->mFlag &= ~QC_STMT_SHARD_META_CHANGE_MASK;
    aStatement->mFlag |= QC_STMT_SHARD_META_CHANGE_TRUE;

    /* BUG-47623 샤드 메타 변경에 대한 trc 로그중 commit 로그를 작성하기전에 DASSERT 로 죽는 경우가 있습니다. */
    if ( ( aStatement->session->mQPSpecific.mFlag & QC_SESSION_SHARD_META_TOUCH_MASK ) ==
         QC_SESSION_SHARD_META_TOUCH_TRUE )
    {
        sIsOldSessionShardMetaTouched = ID_TRUE;
    }

    // BUG-46366
    IDE_TEST_RAISE( ( QC_SMI_STMT(aStatement)->getTrans() == NULL ) ||
                    ( ( aStatement->myPlan->parseTree->stmtKind & QCI_STMT_MASK_DML ) == QCI_STMT_MASK_DML ) ||
                    ( ( aStatement->myPlan->parseTree->stmtKind & QCI_STMT_MASK_DCL ) == QCI_STMT_MASK_DCL ),
                    ERR_INSIDE_QUERY );

    // Check Privilege
    IDE_TEST_RAISE( QCG_GET_SESSION_USER_ID(aStatement) != QCI_SYS_USER_ID,
                    ERR_NO_GRANT );

    //---------------------------------
    // Begin Statement for meta
    //---------------------------------
    IDE_TEST( sdi::getIncreasedSMNForMetaNode( ( QC_SMI_STMT( aStatement ) )->getTrans() , 
                                               &sSMN ) 
              != IDE_SUCCESS );

    sSmiStmtFlag = SMI_STATEMENT_NORMAL | SMI_STATEMENT_MEMORY_CURSOR;
    sOldStmt                = QC_SMI_STMT(aStatement);
    QC_SMI_STMT(aStatement) = &sSmiStmt;
    sState = 1;

    IDE_TEST( sSmiStmt.begin( aStatement->mStatistics,
                              sOldStmt,
                              sSmiStmtFlag )
              != IDE_SUCCESS );
    sState = 2;

    sParseTree = (qdTableParseTree *)aStatement->myPlan->parseTree;

    sSrcPartAttr = sParseTree->partTable->partAttr;
    sDstPartAttr = sParseTree->partTable->partAttr->next;

    QC_STR_COPY( sSrcPartName, sSrcPartAttr->tablePartName );
    QC_STR_COPY( sDstPartName, sDstPartAttr->tablePartName );
    QC_STR_COPY( sSplitValue, sSrcPartAttr->partKeyCond->value->position );

    IDE_TEST( sdm::updateShardMetaForSplitPartition( aStatement,
                                                     aUserName,
                                                     aTableName,
                                                     sSrcPartName,
                                                     sDstPartName,
                                                     sSplitValue,
                                                     &sRowCnt )
              != IDE_SUCCESS );
    
    //---------------------------------
    // End Statement
    //---------------------------------

    sState = 1;
    IDE_TEST( sSmiStmt.end( SMI_STATEMENT_RESULT_SUCCESS ) != IDE_SUCCESS );

    sState = 0;
    QC_SMI_STMT(aStatement) = sOldStmt;

    IDE_TEST_RAISE( sRowCnt == 0,
                    ERR_INVALID_SHARD_TABLE );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INSIDE_QUERY )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QSX_PSM_INSIDE_QUERY ) );
    }
    IDE_EXCEPTION( ERR_INVALID_SHARD_TABLE );
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDF_INVALID_SHARD_TABLE ) );
    }
    IDE_EXCEPTION( ERR_NO_GRANT )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QRC_NO_GRANT ) );
    }
    IDE_EXCEPTION_END;

    IDE_PUSH();

    switch ( sState )
    {
        case 2:
            if ( sSmiStmt.end(SMI_STATEMENT_RESULT_FAILURE) != IDE_SUCCESS )
            {
                IDE_ERRLOG(IDE_SD_1);
            }
            else
            {
                /* Nothing to do */
            }
        case 1:
            QC_SMI_STMT(aStatement) = sOldStmt;
        default:
            break;
    }

    /* BUG-47623 샤드 메타 변경에 대한 trc 로그중 commit 로그를 작성하기전에 DASSERT 로 죽는 경우가 있습니다. */
    if ( sIsOldSessionShardMetaTouched == ID_TRUE )
    {
        sdi::setShardMetaTouched( aStatement->session );
    }
    else
    {
        sdi::unsetShardMetaTouched( aStatement->session );
    }
    
    IDE_POP();
    
    return IDE_FAILURE;
}

void sdiGlobalDDL::clearInfo( qcStatement      * aStatement )
{
/***********************************************************************
 *
 * Description :
 *    PROJ-2757 Advanced Global DDL
 *
 *    DDL 구문을 Global DDL로 수행하기 위해 필요한 정보를 제거한다.
 *
 ***********************************************************************/
    aStatement->mShardGlobalDDLInfo.mIsAllowed    = ID_FALSE;
    aStatement->mShardGlobalDDLInfo.mIndexTBSAttr = NULL;
}

void sdiGlobalDDL::setInfo( qcStatement               * aStatement,
                            idBool                      aIsShardObject,
                            sdiDDLType                  aDDLType,
                            qcNamePosition            * aUserNamePos,
                            qcNamePosition            * aTableNamePos,
                            qdIndexPartitionAttribute * aIndexTBSAttr )
{
/***********************************************************************
 *
 * Description :
 *    PROJ-2757 Advanced Global DDL
 *
 *    DDL 구문을 Global DDL로 수행하기 위해 필요한 정보를 셋팅한다.
 *
 ***********************************************************************/
    aStatement->mShardGlobalDDLInfo.mIsAllowed     = ID_TRUE;

    /* 아래 값들은 mIsAllowed == ID_TRUE일 때만 유효하다 */
    aStatement->mShardGlobalDDLInfo.mIsShardObject = aIsShardObject;
    aStatement->mShardGlobalDDLInfo.mDDLType = aDDLType;

    if ( aUserNamePos != NULL )
    {
        SET_POSITION( aStatement->mShardGlobalDDLInfo.mUserNamePos,
                      *aUserNamePos );
    }

    if ( aTableNamePos != NULL )
    {
        SET_POSITION( aStatement->mShardGlobalDDLInfo.mTableNamePos,
                      *aTableNamePos );
    }

    aStatement->mShardGlobalDDLInfo.mIndexTBSAttr = aIndexTBSAttr;
    aStatement->myPlan->parseTree->execute = sdiGlobalDDL::execute;
}

IDE_RC sdiGlobalDDL::checkProperties( qcStatement * aStatement,
                                      UInt          aKSafety )
{
/***********************************************************************
 *
 * Description :
 *    PROJ-2757 Advanced Global DDL
 *
 *    Global DDL 수행할 때 기본 검사
 *
 * Implementation :
 *      1. session property를 검사한다.
 *
 *      2. replication 관련 property를 검사한다.
 *
 ***********************************************************************/
    UInt             sPropertyValue = 0;
    SInt             sPropertyStrIdx = 0;

    const SChar    * sReplPropertyStr[] = { "REPLICATION_DDL_ENABLE",
                                            "REPLICATION_SQL_APPLY_ENABLE",
                                            "REPLICATION_ALLOW_DUPLICATE_HOSTS",
                                            NULL };
    IDE_CLEAR();

    // check OS
#ifndef ALTI_CFG_OS_LINUX
    IDE_RAISE( ERR_ONLY_ACCEPT_LINUX );
#endif   
    
    // 방어 코드
    IDE_TEST_RAISE( QC_SHARD_GLOBAL_DDL_ALLOWED( aStatement ) != ID_TRUE,
                    ERR_GLOBAL_DDL );

    /* 1. session property를 검사한다. */
    IDE_TEST_RAISE( QCG_GET_SESSION_IS_AUTOCOMMIT( aStatement ) == ID_TRUE,
                    ERR_COMMIT_STATE );

    IDE_TEST_RAISE( QCG_GET_SESSION_GTX_LEVEL( aStatement ) < 2,
                    ERR_GTX_LEVEL );
    
    IDE_TEST_RAISE ( QCG_GET_SESSION_TRANSACTIONAL_DDL( aStatement) != ID_TRUE,
                     ERR_DDL_TRANSACTION );

    /* 2. replication 관련 property를 검사한다. */
    if ( aKSafety != 0 )
    {
        for( sPropertyStrIdx = 0; sReplPropertyStr[sPropertyStrIdx] != NULL; sPropertyStrIdx++ )
        {
            /* read 4byte property */
            IDE_TEST(idp::read(sReplPropertyStr[sPropertyStrIdx], &sPropertyValue) != IDE_SUCCESS);
            IDE_TEST_RAISE( sPropertyValue != 1, ERR_REPL_PROPERTY );

            IDE_TEST(idp::read("REPLICATION_DDL_ENABLE_LEVEL", &sPropertyValue) != IDE_SUCCESS);
            IDE_TEST_RAISE( sPropertyValue < 1, ERR_REPL_PROPERTY );
        }
    }
    
    return IDE_SUCCESS;
    
#ifndef ALTI_CFG_OS_LINUX
    IDE_EXCEPTION( ERR_ONLY_ACCEPT_LINUX );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDSD_ZKC_NOT_SUPPORT_OS ) );
    }
#endif
    IDE_EXCEPTION( ERR_GLOBAL_DDL )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDI_UNSUPPORTED_GLOBAL_DDL ) );
    }
    IDE_EXCEPTION( ERR_COMMIT_STATE )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDF_CANNOT_EXECUTE_IN_AUTOCOMMIT_MODE ) );
    }
    IDE_EXCEPTION( ERR_GTX_LEVEL )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDF_INVALID_GTX_LEVEL ) );
    }
    IDE_EXCEPTION( ERR_REPL_PROPERTY );
    {
        IDE_SET(ideSetErrorCode( qpERR_ABORT_QDSD_INVALID_PROPERTY_FOR_SHARDING, sReplPropertyStr[sPropertyStrIdx] ));
    }
    IDE_EXCEPTION( ERR_DDL_TRANSACTION );
    {
        IDE_SET(ideSetErrorCode( sdERR_ABORT_SDC_INSUFFICIENT_ATTRIBUTE,
                                 "TANSACTIONAL_DDL = 1" ));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdiGlobalDDL::executeDDLToAllNodes( qcStatement * aStatement,
                                           UInt          aKSafety,
                                           SChar       * aUserName,
                                           SChar       * aTableName )
{
/***********************************************************************
 *
 * Description :
 *    PROJ-2757 Advanced Global DDL
 *
 *    DDL 구문을 모든 노드로 수행한다.
 *
 * Implementation :
 *
 *      1. ReplicaSetInfo를 구한다.
 *
 *      2. zookeeper에 afterCommit/Rollback pendingJob을 등록한다.
 *
 *      3. LOCK 구간을 줄이기 위해서 먼저 이중화 GAP을 제거함
 *          ALTER REPLICATION REP1 FLUSH;
 *
 *      4. data가 못들어오게 막음. 
 *          LOCK TABLE T1 IN EXCLUSIVE MODE;
 *
 *      5. DDL 수행
 *          ALTER TABLE T1 ADD COLUMN (C3 INTEGER NOT NULL );
 *
 *      6. LOCK 사이에 들어온 트랜잭션 FLUSH
 *          ALTER REPLICATION REP1 FLUSH;
 *
 ***********************************************************************/
    UInt                sExecCount = 0;
    sdiReplicaSetInfo   sReplicaSetInfo;

    // init
    sReplicaSetInfo.mCount = 0;

    if ( ( aKSafety > 0 ) &&
         ( QC_SHARD_GLOBAL_DDL_IS_SHARD_OBJECT( aStatement ) == ID_TRUE ) )
    {
        /* 1. ReplicaSetInfo를 구한다. */
        IDE_TEST( getReplicaSetInfo( aStatement,
                                     aUserName,
                                     aTableName,
                                     &sReplicaSetInfo ) != IDE_SUCCESS );

        /* 2. zookeeper에 afterCommit/Rollback pendingJob을 등록한다. */
        IDE_TEST( addPendingJob( aStatement,
                                 aKSafety,
                                 &sReplicaSetInfo ) != IDE_SUCCESS );
    }
    else
    {
        /* Nothing to do */
    }
    
    /* 3. LOCK 구간을 줄이기 위해서 먼저 이중화 GAP을 제거함 */
    IDE_TEST( flushReplication( aStatement,
                                aKSafety,
                                aUserName,
                                &sReplicaSetInfo ) != IDE_SUCCESS );

    /* 4. data가 못들어오게 막음. 
     *    모든 노드에서 table/partition에 IX Lock 또는 X Lock을 획득한다. */
    IDE_TEST( lockTable( aStatement,
                         aUserName,
                         aTableName ) != IDE_SUCCESS );

    ideLog::log(IDE_SD_17,"\n[sdiGlobalDDL] DDL: %s\n", QCI_STMTTEXT( aStatement ));
    /* 5. 모든 노드로 DDL 구문을 실행한다. */
    IDE_TEST( sdi::shardExecDirectNested( aStatement,
                                          NULL,
                                          QCI_STMTTEXT( aStatement ),
                                          QCI_STMTTEXTLEN( aStatement ),
                                          SDI_INTERNAL_OP_NOT,
                                          &sExecCount )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( sExecCount == 0, ERR_INVALID_SHARD_NODE );

    /* 6. LOCK 사이에 들어온 트랜잭션 FLUSH */
    IDE_TEST( flushReplication( aStatement,
                                aKSafety,
                                aUserName,
                                &sReplicaSetInfo ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_SHARD_NODE );
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDF_INVALID_SHARD_NODE ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdiGlobalDDL::addPendingJob( qcStatement       * aStatement,
                                    UInt                aKSafety,
                                    sdiReplicaSetInfo * aReplicaSetInfo )
{
/***********************************************************************
 *
 * Description :
 *    PROJ-2757 Advanced Global DDL
 *
 *    shard 테이블이 분산되어 있는 노드에 대한 이중화 flush를
 *    zookeeper에 pending job으로 등록한다.
 *
 * Implementation :
 *
 *      1. aReplicaSetInfo의 모든 ReplicaSet에 대한 이중화 flush 구문을
 *         zookeeper에 afterCommit JOB으로 등록한다.
 *
 ***********************************************************************/
    UInt                i = 0;
    UInt                sCnt = 0;
    SChar             * sSqlStr;
    SChar             * sSenderNode   = NULL;
    SChar             * sReceiverNode = NULL;
    SChar             * sReplName     = NULL;
    UInt                sShardDDLLockTryCount;
    UInt                sRemoteLockTimeout;

    IDE_TEST_CONT( aReplicaSetInfo->mCount <= 0, NORMAL_EXIT );

    IDE_TEST( STRUCT_ALLOC_WITH_SIZE( aStatement->qmxMem,
                                      SChar,
                                      QD_MAX_SQL_LENGTH,
                                      &sSqlStr )
              != IDE_SUCCESS);

    sShardDDLLockTryCount = QCG_GET_SESSION_SHARD_DDL_LOCK_TRY_COUNT(aStatement);
    sRemoteLockTimeout = QCG_GET_SESSION_SHARD_DDL_LOCK_TIMEOUT(aStatement);

    for ( sCnt = 0; sCnt < aReplicaSetInfo->mCount; sCnt++ )
    {
        sSenderNode = aReplicaSetInfo->mReplicaSets[sCnt].mPrimaryNodeName;

        for ( i = 0 ; i < aKSafety; i++ )
        {
            if ( i == 0 ) /* First */
            {
                sReceiverNode = aReplicaSetInfo->mReplicaSets[sCnt].mFirstBackupNodeName;
                sReplName = aReplicaSetInfo->mReplicaSets[sCnt].mFirstReplName;
            }
            else if ( i == 1 ) /* Second */
            {
                sReceiverNode = aReplicaSetInfo->mReplicaSets[sCnt].mSecondBackupNodeName;
                sReplName = aReplicaSetInfo->mReplicaSets[sCnt].mSecondReplName;
            }

            /* BackupNodeName이 설정되어 있어야만 한다. */
            if ( idlOS::strncmp( sReceiverNode,
                                 SDM_NA_STR,
                                 SDI_NODE_NAME_MAX_SIZE + 1 )
                 != 0 )
            {                        
                idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                                 "ALTER REPLICATION "QCM_SQL_STRING_SKIP_FMT" FLUSH WAIT %"ID_UINT32_FMT,
                                 sReplName,
                                 (sShardDDLLockTryCount * sRemoteLockTimeout) );

                ideLog::log(IDE_SD_17,"\n[sdiGlobalDDL] addPendingJob (%s, AfterCommit, %s)\n", sSenderNode, sSqlStr);
                (void) sdiZookeeper::addPendingJob( sSqlStr,
                                                    sSenderNode,
                                                    ZK_PENDING_JOB_AFTER_COMMIT,
                                                    QCI_STMT_MASK_DCL );

                idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                                 "ALTER REPLICATION "QCM_SQL_STRING_SKIP_FMT" FLUSH WAIT 1",
                                 sReplName );

                ideLog::log(IDE_SD_17,"\n[sdiGlobalDDL] addPendingJob (%s, AfterRollback, %s)\n", sSenderNode, sSqlStr);
                (void) sdiZookeeper::addPendingJob( sSqlStr,
                                                    sSenderNode,
                                                    ZK_PENDING_JOB_AFTER_ROLLBACK,
                                                    QCI_STMT_MASK_DCL );
            }
            else
            {
                /* Nothing to do */
            }
        }
    }

    IDE_EXCEPTION_CONT( NORMAL_EXIT );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdiGlobalDDL::lockTable( qcStatement    * aStatement,
                                SChar          * aUserName,
                                SChar          * aTableName )
{
/***********************************************************************
 *
 * Description :
 *    PROJ-2757 Advanced Global DDL
 *
 *    Shard 노드들에서 지정한 테이블에 락을 획득한다.
 *
 *    파티션 관련 DDL이면,
 *      테이블에는 IX LOCK을, 파티션에는 X LOCK을 획득한다.
 *
 ***********************************************************************/
    qdTableParseTree       * sParseTree;
    qcmPartitionInfoList   * sPartInfoList = NULL;
    smiTableLockMode         sLockMode     = SMI_TABLE_LOCK_X;

    switch ( QC_SHARD_GLOBAL_DDL_GET_DDL_TYPE( aStatement ) )
    {
        case SDI_DDL_TYPE_TABLE:
        case SDI_DDL_TYPE_INDEX:
        case SDI_DDL_TYPE_DROP:
        case SDI_DDL_TYPE_INDEX_DROP:
            sLockMode = SMI_TABLE_LOCK_X;
            break;

        case SDI_DDL_TYPE_TABLE_PARTITION:
        case SDI_DDL_TYPE_TABLE_SPLIT_PARTITION:
            sLockMode = SMI_TABLE_LOCK_IX;
            break;

        default:
            IDE_CONT( NORMAL_EXIT );
            break;
    }

    IDE_TEST( lockTableInAllShardNodes( aStatement,
                                        aUserName,
                                        aTableName,
                                        NULL,
                                        sLockMode )
              != IDE_SUCCESS );

    if ( sLockMode == SMI_TABLE_LOCK_IX )
    {
        sParseTree = (qdTableParseTree *)aStatement->myPlan->parseTree;

        if ( ( sParseTree->partTable != NULL ) &&
             ( sParseTree->partTable->partInfoList != NULL ) )
        {
            sPartInfoList = sParseTree->partTable->partInfoList;

            for ( sPartInfoList = sPartInfoList;
                  sPartInfoList != NULL;
                  sPartInfoList = sPartInfoList->next )
            {
                IDE_TEST( lockTableInAllShardNodes(
                                    aStatement,
                                    aUserName,
                                    aTableName,
                                    sPartInfoList->partitionInfo->name )
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

    IDE_EXCEPTION_CONT( NORMAL_EXIT );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdiGlobalDDL::lockTableInAllShardNodes( qcStatement      * aStatement,
                                               qcNamePosition     aUserNamePos,
                                               qcNamePosition     aTableNamePos,
                                               smiTableLockMode   aLockMode )
{
/***********************************************************************
 *
 * Description :
 *    PROJ-2757 Advanced Global DDL
 *
 *    Shard 노드들에서 지정한 테이블에 락을 획득한다.
 *
 ***********************************************************************/
    SChar  sUserName[QC_MAX_OBJECT_NAME_LEN + 1]  = {0, };
    SChar  sTableName[QC_MAX_OBJECT_NAME_LEN + 1] = {0, };

    if ( QC_IS_NULL_NAME(aUserNamePos) == ID_TRUE )
    {
        idlOS::strcpy( sUserName, QCG_GET_SESSION_USER_NAME(aStatement) );
    }
    else
    {
        QC_STR_COPY( sUserName, aUserNamePos );
    }

    QC_STR_COPY( sTableName, aTableNamePos );

    return lockTableInAllShardNodes( aStatement,
                                     sUserName,
                                     sTableName,
                                     NULL,
                                     aLockMode );
}

IDE_RC sdiGlobalDDL::lockPartitionInAllShardNodes( qcStatement      * aStatement,
                                                   qcNamePosition     aUserNamePos,
                                                   qcNamePosition     aTableNamePos,
                                                   qcNamePosition     aPartNamePos )
{
/***********************************************************************
 *
 * Description :
 *    PROJ-2757 Advanced Global DDL
 *
 *    shard 노드들에서 지정한 파티션에 Lock을 획득한다.
 *
 ***********************************************************************/
    SChar  sUserName[QC_MAX_OBJECT_NAME_LEN + 1]  = {0, };
    SChar  sTableName[QC_MAX_OBJECT_NAME_LEN + 1] = {0, };
    SChar  sPartName[QC_MAX_OBJECT_NAME_LEN + 1] = {0, };

    if ( QC_IS_NULL_NAME(aUserNamePos) == ID_TRUE )
    {
        idlOS::strcpy( sUserName, QCG_GET_SESSION_USER_NAME(aStatement) );
    }
    else
    {
        QC_STR_COPY( sUserName, aUserNamePos );
    }

    QC_STR_COPY( sTableName, aTableNamePos );
    QC_STR_COPY( sPartName, aPartNamePos );

    return lockTableInAllShardNodes( aStatement,
                                     sUserName,
                                     sTableName,
                                     sPartName,
                                     SMI_TABLE_LOCK_X );
}

IDE_RC sdiGlobalDDL::lockTableInAllShardNodes( qcStatement      * aStatement,
                                               SChar            * aUserName,
                                               SChar            * aTableName,
                                               SChar            * aPartName,
                                               smiTableLockMode   aLockMode )
{
/***********************************************************************
 *
 * Description :
 *    PROJ-2757 Advanced Global DDL
 *
 *    Global DDL이 지원되는 DDL 구문을 여러 노드로 실행하기 전에
 *    테이블 또는 파티션에 Lock을 획득한다.
 *
 *    테이블에 X LOCK 획득
 *      LOCK TABLE %s.%s IN EXCLUSIVE MODE;
 *
 *    테이블에 IX LOCK 획득
 *      LOCK TABLE %s.%s IN ROW EXCLUSIVE MODE;
 *
 *    파티션에 X LOCK 획득
 *      LOCK TABLE %s.%s PARTITION (%s) IN EXCLUSIVE MODE;
 *
 ***********************************************************************/
    SChar         * sSqlStr;
    UInt            sExecCount           = 0;
    UInt            sOffset              = 0;
    SInt            sLockTimeout         = 3;

    sLockTimeout = QCG_GET_SESSION_SHARD_DDL_LOCK_TIMEOUT(aStatement);

    IDE_TEST( STRUCT_ALLOC_WITH_SIZE( aStatement->qmxMem,
                                      SChar,
                                      QD_MAX_SQL_LENGTH,
                                      &sSqlStr )
              != IDE_SUCCESS );

    sOffset = idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                               "LOCK TABLE "QCM_SQL_STRING_SKIP_FMT"."QCM_SQL_STRING_SKIP_FMT" ",
                               aUserName,
                               aTableName );

    if ( aPartName != NULL )
    {
        sOffset += idlOS::snprintf( sSqlStr + sOffset, QD_MAX_SQL_LENGTH - sOffset,
                                    "PARTITION ("QCM_SQL_STRING_SKIP_FMT") ",
                                    aPartName );
    }
    else
    {
        /* Nothing to do */
    }

    sOffset += idlOS::snprintf( sSqlStr + sOffset, QD_MAX_SQL_LENGTH - sOffset,
                     "IN %sEXCLUSIVE MODE",
                     (aLockMode == SMI_TABLE_LOCK_IX)? "ROW " : "" );

    if ( sLockTimeout > 0 )
    {
        idlOS::snprintf( sSqlStr + sOffset, QD_MAX_SQL_LENGTH - sOffset,
                         " WAIT %"ID_INT32_FMT"",
                         sLockTimeout );
    }
    else if ( sLockTimeout == 0 )
    {
        idlOS::snprintf( sSqlStr + sOffset, QD_MAX_SQL_LENGTH - sOffset,
                         " NOWAIT" );
    }
    else
    {
        /* Nothing to do */
    }

    ideLog::log(IDE_SD_17,"\n[sdiGlobalDDL] LOCK: %s\n", sSqlStr);
    IDE_TEST( sdi::shardExecDirect( aStatement,
                                    NULL,
                                    sSqlStr,
                                    idlOS::strlen(sSqlStr),
                                    SDI_INTERNAL_OP_NOT,
                                    &sExecCount,
                                    NULL,
                                    NULL,
                                    0,
                                    NULL )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdiGlobalDDL::executeForBackup( qcStatement    * aStatement )
{
/***********************************************************************
 *
 * Description :
 *    PROJ-2757 Advanced Global DDL
 *
 *    shard backup 테이블에 대해 동일한 DDL을 수행한다.
 *
 * Implementation :
 *      1. shard table 인지 검사한다.
 *
 *      2. 원본 DDL 구문을 백업 테이블에 대한 구문으로 변환하여
 *         모든 노드로 실행한다.
 *
 ***********************************************************************/
    qdIndexParseTree  * sIndexParseTree;
    qdDropParseTree   * sDropParseTree;

    /* 1. shard table 인지 검사한다. */
    IDE_TEST_CONT( QC_SHARD_GLOBAL_DDL_IS_SHARD_OBJECT( aStatement ) != ID_TRUE,
                   NORMAL_EXIT );

    /* 2. 원본 DDL 구문을 백업 테이블에 대한 구문으로 변환하여 모든 노드로 실행한다. */
    switch ( QC_SHARD_GLOBAL_DDL_GET_DDL_TYPE( aStatement ) )
    {
        case SDI_DDL_TYPE_TABLE:
        case SDI_DDL_TYPE_TABLE_PARTITION:
        case SDI_DDL_TYPE_TABLE_SPLIT_PARTITION:
            IDE_TEST( executeBackupDDL( aStatement ) != IDE_SUCCESS );
            break;

        case SDI_DDL_TYPE_INDEX:
            sIndexParseTree = (qdIndexParseTree *)aStatement->myPlan->parseTree;

            IDE_TEST( makeAndExecuteBackupDDL(
                            aStatement,
                            2,
                            sIndexParseTree->indexName.offset,
                            sIndexParseTree->tableName.offset )
                      != IDE_SUCCESS );
            break;

        case SDI_DDL_TYPE_DROP:
        case SDI_DDL_TYPE_INDEX_DROP:
            sDropParseTree = (qdDropParseTree *)aStatement->myPlan->parseTree;

            IDE_TEST( makeAndExecuteBackupDDL(
                            aStatement,
                            1,
                            sDropParseTree->objectName.offset )
                      != IDE_SUCCESS );
            break;

        default:
            IDE_CONT( NORMAL_EXIT );
            break;
    }

    IDE_EXCEPTION_CONT( NORMAL_EXIT );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdiGlobalDDL::executeBackupDDL( qcStatement * aStatement )
{
/***********************************************************************
 *
 * Description :
 *    PROJ-2757 Advanced Global DDL
 *
 *    qdTableParseTree를 갖고 있는 statement를 위한 backup 테이블용 executor.
 *
 *    [ constraint 이름 포함 ]
 *    ALTER TABLE table_name DROP CONSTRAINT constraint_name;
 *    ALTER TABLE table_name RENAME CONSTRAINT constraint_name TO new_constraint_name;
 *    ALTER TABLE table_name DROP UNIQUE (column_constraint, ...);
 *    ALTER TABLE table_name DROP LOCAL UNIQUE (column_constraint, ...);
 *    ALTER TABLE table_name ADD COLUMN ( ... column_constraint, ... );
 *    ALTER TABLE table_name ADD CONSTRAINT constraint_name ... ;
 *
 *    [ tablespace 변경 DDL에 인덱스 이름 포함 ]
 *    ALTER TABLE table_name ALTER TABLESPACE tbs_name INDEX (index_name ..., )
 *
 *    ALTER TABLE tbl_name
 *      ALTER PARTITION part_name TABLESPACE tbs_name
 *        { INDEX ( part_index_name TABLESPACE tbs_name, ... ) }
 *        { LOB ( column_name TABLESPACE tbs_name, ... ) }
 *
 *    [ Split Partition DDL에 인덱스 이름 포함 ]
 *    ALTER TABLE tbl_name SPLIT PARTITION src_part
 *        { AT | VALUES } ( split_cond_value, ... )
 *            INTO ( PARTITION dst_part1 INDEX ( part_index_name ... ),
 *                   PARTITION dst_part2 INDEX ( part_index_name ... ) )
 *
 *    [ 그 외 DDL ]
 *    table_name만 변환하면 되는 DDL
 *
 * Implementation :
 *
 *      makeAndExecuteBackupDDL 참조
 *
 *      1. 테이블 이름 앞에 SDI_BACKUP_TABLE_PREFIX를 붙인다.
 *
 *      2. constraint 이름이 포함된 DDL 구문이면,
 *         constraint 이름 앞에 SDI_BACKUP_TABLE_PREFIX를 붙인다. 
 *
 *      3. 테이블이나 파티션의 tablespace을 변경하는 DDL에 인덱스의
 *         tablespace를 변경하는 절이 포함되어 있으면,
 *         인덱스 이름 앞에 SDI_BACKUP_TABLE_PREFIX를 붙인다.
 *
 *      4. split partition 구문에 partitioned index 이름이 포함된 DDL 이면,
 *        4-1. left Dst.에 지정한 partitioned index 이름 앞에
 *             SDI_BACKUP_TABLE_PREFIX를 붙인다.
 *        4-2. right Dst.에 지정한 partitioned index 이름 앞에
 *             SDI_BACKUP_TABLE_PREFIX를 붙인다.
 *
 *      5. 남은 문자열을 붙인다.
 *
 *      6. DDL 구문을 모든 노드로 수행한다.
 *
 ***********************************************************************/
    qdTableParseTree          * sParseTree;
    qdPartitionAttribute      * sPartAttr;
    qdIndexPartitionAttribute * sAttr;
    qdConstraintSpec          * sConstr;

    SChar            * sSqlStr;
    UInt               sExecCount           = 0;
    SInt               sPrevOffset          = 0;
    SInt               sOffset              = 0;
    SInt               sSqlStrOffset        = 0;
    SInt               sStmtLen             = 0;
    
    sParseTree = (qdTableParseTree *)aStatement->myPlan->parseTree;

    sStmtLen = QCI_STMTTEXTLEN( aStatement );

    IDE_TEST( STRUCT_ALLOC_WITH_SIZE( aStatement->qmxMem,
                                      SChar,
                                      QD_MAX_SQL_LENGTH,
                                      &sSqlStr )
              != IDE_SUCCESS );

    sOffset = sParseTree->tableName.offset;

    idlOS::strncpy( sSqlStr,
                    QCI_STMTTEXT( aStatement ),
                    sOffset );

    sSqlStrOffset = sOffset;

    /* 1. 테이블 이름 앞에 SDI_BACKUP_TABLE_PREFIX를 붙인다. */
    idlOS::strncpy( sSqlStr + sSqlStrOffset,
                    SDI_BACKUP_TABLE_PREFIX,
                    idlOS::strlen(SDI_BACKUP_TABLE_PREFIX) );

    sSqlStrOffset += idlOS::strlen(SDI_BACKUP_TABLE_PREFIX);
    sPrevOffset = sOffset;

    /* 2. constraint 이름이 포함된 DDL 구문에서
     *    constraint 이름 앞에 SDI_BACKUP_TABLE_PREFIX를 붙인다. */
    if ( sParseTree->constraints != NULL )
    {
        for ( sConstr = sParseTree->constraints;
              sConstr != NULL;
              sConstr = sConstr->next )
        {
            if ( sConstr->constrName.offset <= 0 )
            {
                continue;
            }
            sOffset = sConstr->constrName.offset;

            idlOS::strncpy( sSqlStr + sSqlStrOffset,
                            QCI_STMTTEXT( aStatement ) + sPrevOffset,
                            sOffset - sPrevOffset );

            sSqlStrOffset = sSqlStrOffset + sOffset - sPrevOffset; 

            /* 2. constraint 이름 앞에 SDI_BACKUP_TABLE_PREFIX를 붙인다. */
            idlOS::strncpy( sSqlStr + sSqlStrOffset,
                            SDI_BACKUP_TABLE_PREFIX,
                            idlOS::strlen(SDI_BACKUP_TABLE_PREFIX) );

            sSqlStrOffset += idlOS::strlen(SDI_BACKUP_TABLE_PREFIX);
            sPrevOffset = sOffset;
        }
    }
    /* 3. 테이블이나 파티션의 tablespace을 변경하는 DDL에 인덱스의
     *    tablespace를 변경하는 절이 포함되어 있으면,
     *    인덱스 이름 앞에 SDI_BACKUP_TABLE_PREFIX를 붙인다. */
    else if ( QC_SHARD_GLOBAL_DDL_INDEX_TBS_ATTR( aStatement ) != NULL )
    {
        for ( sAttr = QC_SHARD_GLOBAL_DDL_INDEX_TBS_ATTR( aStatement );
              sAttr != NULL;
              sAttr = sAttr->next )
        {
            if ( sAttr->partIndexName.offset <= 0 )
            {
                continue;
            }
            sOffset = sAttr->partIndexName.offset;

            idlOS::strncpy( sSqlStr + sSqlStrOffset,
                            QCI_STMTTEXT( aStatement ) + sPrevOffset,
                            sOffset - sPrevOffset );

            sSqlStrOffset = sSqlStrOffset + sOffset - sPrevOffset; 

            idlOS::strncpy( sSqlStr + sSqlStrOffset,
                            SDI_BACKUP_TABLE_PREFIX,
                            idlOS::strlen(SDI_BACKUP_TABLE_PREFIX) );

            sSqlStrOffset += idlOS::strlen(SDI_BACKUP_TABLE_PREFIX);
            sPrevOffset = sOffset;
        }
    }
    /* 4. split partition 구문에 partitioned index 이름이 포함된 DDL 이면, */
    else if ( QC_SHARD_GLOBAL_DDL_GET_DDL_TYPE( aStatement )
              == SDI_DDL_TYPE_TABLE_SPLIT_PARTITION )
    {
        /* 4-1. left Dst.에 지정한 partitioned index 이름 앞에
         *      SDI_BACKUP_TABLE_PREFIX를 붙인다. */
        sPartAttr = sParseTree->partTable->partAttr->next;
        for ( sAttr = sPartAttr->alterPart->indexPartAttr;
              sAttr != NULL;
              sAttr = sAttr->next )
        {
            if ( sAttr->partIndexName.offset <= 0 )
            {
                continue;
            }
            sOffset = sAttr->partIndexName.offset;

            idlOS::strncpy( sSqlStr + sSqlStrOffset,
                            QCI_STMTTEXT( aStatement ) + sPrevOffset,
                            sOffset - sPrevOffset );

            sSqlStrOffset = sSqlStrOffset + sOffset - sPrevOffset; 

            idlOS::strncpy( sSqlStr + sSqlStrOffset,
                            SDI_BACKUP_TABLE_PREFIX,
                            idlOS::strlen(SDI_BACKUP_TABLE_PREFIX) );

            sSqlStrOffset += idlOS::strlen(SDI_BACKUP_TABLE_PREFIX);
            sPrevOffset = sOffset;
        }

        /* 4-2. right Dst.에 지정한 partitioned index 이름 앞에
         *      SDI_BACKUP_TABLE_PREFIX를 붙인다. */
        sPartAttr = sParseTree->partTable->partAttr->next->next;
        for ( sAttr = sPartAttr->alterPart->indexPartAttr;
              sAttr != NULL;
              sAttr = sAttr->next )
        {
            if ( sAttr->partIndexName.offset <= 0 )
            {
                continue;
            }
            sOffset = sAttr->partIndexName.offset;

            idlOS::strncpy( sSqlStr + sSqlStrOffset,
                            QCI_STMTTEXT( aStatement ) + sPrevOffset,
                            sOffset - sPrevOffset );

            sSqlStrOffset = sSqlStrOffset + sOffset - sPrevOffset; 

            idlOS::strncpy( sSqlStr + sSqlStrOffset,
                            SDI_BACKUP_TABLE_PREFIX,
                            idlOS::strlen(SDI_BACKUP_TABLE_PREFIX) );

            sSqlStrOffset += idlOS::strlen(SDI_BACKUP_TABLE_PREFIX);
            sPrevOffset = sOffset;
        }
    }
    else
    {
        /* Nothing to do */
    }

    /* 5. 남은 문자열을 붙인다. */
    idlOS::strncpy( sSqlStr + sSqlStrOffset,
                    QCI_STMTTEXT( aStatement ) + sPrevOffset,
                    sStmtLen - sPrevOffset );

    sSqlStrOffset += sStmtLen - sPrevOffset;
    sSqlStr[sSqlStrOffset] = '\0';

    ideLog::log(IDE_SD_17,"\n[sdiGlobalDDL] BACKUP: %s\n", sSqlStr);

    /* 6. DDL 구문을 모든 노드로 수행한다. */
    IDE_TEST( sdi::shardExecDirect( aStatement,
                                    NULL,
                                    sSqlStr,
                                    idlOS::strlen(sSqlStr),
                                    SDI_INTERNAL_OP_NOT,
                                    &sExecCount,
                                    NULL,
                                    NULL,
                                    0,
                                    NULL )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( sExecCount == 0, ERR_INVALID_SHARD_NODE );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_SHARD_NODE );
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDF_INVALID_SHARD_NODE ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdiGlobalDDL::makeAndExecuteBackupDDL( qcStatement    * aStatement,
                                              SInt             aNumOffsets,
                                              ... )
{
/***********************************************************************
 *
 * Description :
 *    PROJ-2757 Advanced Global DDL
 *
 *    인자에 주어진 모든 offset 위치에 SDI_BACKUP_PREFIX를 붙인다.
 *
 * Implementation :
 *
 *      1. offset 위치 이전까지의 문자열을 복사한다.
 *
 *      2. SDI_BACKUP_TABLE_PREFIX를 붙인다.
 *
 *      3. 다음 offset 까지의 문자열을 붙인다.
 *
 *      4. (2, 3)를 반복한다.
 *
 *      5. 남은 문자열을 붙인다.
 *
 *      6. DDL 구문을 모든 노드로 수행한다.
 *
 ***********************************************************************/
    SChar         * sSqlStr;
    UInt            sExecCount           = 0;
    SInt            sPrevOffset          = 0;
    SInt            sOffset              = 0;
    SInt            sSqlStrOffset        = 0;
    SInt            sStmtLen             = 0;
    SInt            i                    = 0;
    va_list         sArgs;

    va_start( sArgs, aNumOffsets );

    sStmtLen = QCI_STMTTEXTLEN( aStatement );

    IDE_TEST( STRUCT_ALLOC_WITH_SIZE( aStatement->qmxMem,
                                      SChar,
                                      QD_MAX_SQL_LENGTH,
                                      &sSqlStr )
              != IDE_SUCCESS );

    for ( i = 0; i < aNumOffsets; i++ )
    {
        sOffset = va_arg(sArgs, SInt);

        idlOS::strncpy( sSqlStr + sSqlStrOffset,
                        QCI_STMTTEXT( aStatement ) + sPrevOffset,
                        sOffset - sPrevOffset );

        sSqlStrOffset = sSqlStrOffset + sOffset - sPrevOffset; 

        idlOS::strncpy( sSqlStr + sSqlStrOffset,
                        SDI_BACKUP_TABLE_PREFIX,
                        idlOS::strlen(SDI_BACKUP_TABLE_PREFIX) );

        sSqlStrOffset += idlOS::strlen(SDI_BACKUP_TABLE_PREFIX);
        sPrevOffset = sOffset;
    }

    va_end(sArgs);

    idlOS::strncpy( sSqlStr + sSqlStrOffset,
                    QCI_STMTTEXT( aStatement ) + sPrevOffset,
                    sStmtLen - sPrevOffset );

    sSqlStrOffset += sStmtLen - sPrevOffset;
    sSqlStr[sSqlStrOffset] = '\0';

    ideLog::log(IDE_SD_17,"\n[sdiGlobalDDL] BACKUP: %s\n", sSqlStr);
    IDE_TEST( sdi::shardExecDirect( aStatement,
                                    NULL,
                                    sSqlStr,
                                    idlOS::strlen(sSqlStr),
                                    SDI_INTERNAL_OP_NOT,
                                    &sExecCount,
                                    NULL,
                                    NULL,
                                    0,
                                    NULL )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( sExecCount == 0, ERR_INVALID_SHARD_NODE );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_SHARD_NODE );
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDF_INVALID_SHARD_NODE ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdiGlobalDDL::getReplicaSetInfo( qcStatement       * aStatement,
                                        SChar             * aUserName,
                                        SChar             * aTableName,
                                        sdiReplicaSetInfo * aReplicaSetInfo )
{
/***********************************************************************
 *
 * Description :
 *    PROJ-2757 Advanced Global DDL
 *
 *    shard 테이블 분산 정보를 이용해서 ReplicatSetInfo를 구해서
 *    aReplicaSetInfo에 반환한다.
 *
 * Implementation :
 *
 *      1. 해당 테이블의 분산 정보를 구한다. => sdm::getRangeInfo 이용
 *
 *      2. 각 Range의 ReplicaSetId를 이용해서 ReplicatSetInfo를 구한다.
 *         => sdm::getReplicaSetsByReplicaSetId
 *
 *      3. 2에서 구한 ReplicatSetInfo를 aReplicaSetInfo에 복사한다.
 *
 *      4. RANGE, HASH, LIST 일 경우 DefaultPartitionReplicaSetId 에 대해
 *         2, 3을 수행한다.
 *
 ***********************************************************************/
    smiStatement      * sOldStmt = NULL;
    sdiTableInfo        sTableInfo;
    sdiRangeInfo        sRangeInfo;
    sdiReplicaSetInfo   sFoundReplicaSetInfo;
    smiStatement        sSmiStmt;
    UInt                sSmiStmtFlag;
    UInt                sCnt = 0;
    SInt                sState = 0;
    ULong               sSMN = ID_ULONG(0);
    idBool              sIsTableFound = ID_FALSE;
    SChar               sReplicaSetVisits[SDI_NODE_MAX_COUNT + 1] = {0, };

    //---------------------------------
    // Begin Statement for meta
    //---------------------------------
    IDE_TEST( sdi::getIncreasedSMNForMetaNode(
                    ( QC_SMI_STMT( aStatement ) )->getTrans(), 
                    &sSMN ) 
              != IDE_SUCCESS );

    sSmiStmtFlag = SMI_STATEMENT_NORMAL | SMI_STATEMENT_MEMORY_CURSOR;
    sOldStmt                = QC_SMI_STMT(aStatement);
    QC_SMI_STMT(aStatement) = &sSmiStmt;
    sState = 1;

    IDE_TEST( sSmiStmt.begin( aStatement->mStatistics,
                              sOldStmt,
                              sSmiStmtFlag )
              != IDE_SUCCESS );
    sState = 2;

    IDE_TEST( sdm::getTableInfo( QC_SMI_STMT( aStatement ),
                                 aUserName,
                                 aTableName,
                                 sSMN,
                                 &sTableInfo,
                                 &sIsTableFound )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( sIsTableFound == ID_FALSE,
                    ERR_NOT_EXIST_TABLE );

    /* 1. 해당 테이블의 분산 정보를 구한다. */
    IDE_TEST( sdm::getRangeInfo( aStatement,
                                 QC_SMI_STMT( aStatement ),
                                 sSMN,
                                 &sTableInfo,
                                 &sRangeInfo,
                                 ID_FALSE, /* NeedMerge */
                                 ID_TRUE   /* NeedReplicaSet */ )
              != IDE_SUCCESS );
     
    aReplicaSetInfo->mCount = 0;

    for ( sCnt = 0; sCnt < sRangeInfo.mCount ; sCnt++ ) 
    {
        // 이미 가져온 replicaSet이면 건너뛴다.
        if ( sReplicaSetVisits[ sRangeInfo.mRanges[sCnt].mReplicaSetId ] == 1 )
        {
            continue;
        }

        /* 2. 각 Range의 ReplicaSetId를 이용해서 ReplicatSetInfo를 구한다. */
        IDE_TEST( sdm::getReplicaSetsByReplicaSetId(
                        QC_SMI_STMT( aStatement ),
                        sRangeInfo.mRanges[sCnt].mReplicaSetId,
                        sSMN,
                        &sFoundReplicaSetInfo )
                  != IDE_SUCCESS );

        IDE_DASSERT( sFoundReplicaSetInfo.mCount == 1 );

        /* 3. 2에서 구한 ReplicatSetInfo를 aReplicaSetInfo에 복사한다. */
        copyReplicaSetInfo( &sFoundReplicaSetInfo, 0,
                            aReplicaSetInfo, aReplicaSetInfo->mCount );

        aReplicaSetInfo->mCount++;

        // 가져온 ReplicaSetId 는 1로 변경
        sReplicaSetVisits[ sRangeInfo.mRanges[sCnt].mReplicaSetId ] = 1;
    }

    /* 4. RANGE, HASH, LIST 일 경우
     *    DefaultPartitionReplicaSetId 에 대해 2, 3을 수행한다. */
    switch ( sTableInfo.mSplitMethod )
    {
        case SDI_SPLIT_HASH:
        case SDI_SPLIT_RANGE:
        case SDI_SPLIT_LIST:
            if ( sReplicaSetVisits[ sTableInfo.mDefaultPartitionReplicaSetId ] == 0 )
            {
                IDE_TEST( sdm::getReplicaSetsByReplicaSetId(
                            QC_SMI_STMT( aStatement ),
                            sTableInfo.mDefaultPartitionReplicaSetId,
                            sSMN,
                            &sFoundReplicaSetInfo )
                          != IDE_SUCCESS );

                IDE_DASSERT( sFoundReplicaSetInfo.mCount == 1 )

                copyReplicaSetInfo( &sFoundReplicaSetInfo, 0,
                                    aReplicaSetInfo, aReplicaSetInfo->mCount );

                aReplicaSetInfo->mCount++;

                sReplicaSetVisits[ sRangeInfo.mRanges[sCnt].mReplicaSetId ] = 1;
            }
            break;

        default:
            break;
    }

    //---------------------------------
    // End Statement
    //---------------------------------

    sState = 1;
    IDE_TEST( sSmiStmt.end( SMI_STATEMENT_RESULT_SUCCESS ) != IDE_SUCCESS );

    sState = 0;
    QC_SMI_STMT(aStatement) = sOldStmt;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_EXIST_TABLE )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDM_SHARD_TABLE_NOT_EXIST ) );
    }
    IDE_EXCEPTION_END;
    
    IDE_PUSH();

    switch ( sState )
    {
        case 2:
            if ( sSmiStmt.end(SMI_STATEMENT_RESULT_FAILURE) != IDE_SUCCESS )
            {
                IDE_ERRLOG(IDE_SD_1);
            }
            else
            {
                /* Nothing to do */
            }
        case 1:
            QC_SMI_STMT(aStatement) = sOldStmt;
        default:
            break;
    }
    
    IDE_POP();    
    
    return IDE_FAILURE;
}

IDE_RC sdiGlobalDDL::flushReplication( qcStatement       * aStatement,
                                       UInt                aKSafety,
                                       SChar             * aUserName,
                                       sdiReplicaSetInfo * aReplicaSetInfo )
{
/***********************************************************************
 *
 * Description :
 *    PROJ-2757 Advanced Global DDL
 *
 *    shard 테이블이 분산되어 있는 노드로 이중화 flush를 수행한다.
 *
 * Implementation :
 *
 *      1. aReplicaSetInfo의 모든 ReplicaSet에 대해 flush 를 수행한다.
 *         => sdm::flushReplTable 이용
 *
 ***********************************************************************/
    smiStatement      * sOldStmt = NULL;
    smiStatement        sSmiStmt;
    UInt                sSmiStmtFlag;
    UInt                i = 0;
    UInt                sCnt = 0;
    SInt                sState = 0;
    SChar             * sSenderNode   = NULL;
    SChar             * sReceiverNode = NULL;
    SChar             * sReplName     = NULL;
    UInt                sFlushWaitSecond;
    
    IDE_TEST_CONT( ( aKSafety <= 0 ) ||
                   ( QC_SHARD_GLOBAL_DDL_IS_SHARD_OBJECT( aStatement ) != ID_TRUE ),
                   NORMAL_EXIT );

    //---------------------------------
    // Begin Statement for meta
    //---------------------------------
    sSmiStmtFlag = SMI_STATEMENT_NORMAL | SMI_STATEMENT_MEMORY_CURSOR;
    sOldStmt                = QC_SMI_STMT(aStatement);
    QC_SMI_STMT(aStatement) = &sSmiStmt;
    sState = 1;

    IDE_TEST( sSmiStmt.begin( aStatement->mStatistics,
                              sOldStmt,
                              sSmiStmtFlag )
              != IDE_SUCCESS );
    sState = 2;

    sFlushWaitSecond = QCG_GET_SESSION_SHARD_DDL_LOCK_TRY_COUNT(aStatement) *
                       QCG_GET_SESSION_SHARD_DDL_LOCK_TIMEOUT(aStatement);

    for ( sCnt = 0; sCnt < aReplicaSetInfo->mCount; sCnt++ )
    {
        sSenderNode = aReplicaSetInfo->mReplicaSets[sCnt].mPrimaryNodeName;

        for ( i = 0 ; i < aKSafety; i++ )
        {
            if ( i == 0 ) /* First */
            {
                sReceiverNode = aReplicaSetInfo->mReplicaSets[sCnt].mFirstBackupNodeName;
                sReplName = aReplicaSetInfo->mReplicaSets[sCnt].mFirstReplName;
            }
            else if ( i == 1 ) /* Second */
            {
                sReceiverNode = aReplicaSetInfo->mReplicaSets[sCnt].mSecondBackupNodeName;
                sReplName = aReplicaSetInfo->mReplicaSets[sCnt].mSecondReplName;
            }
            /* BackupNodeName이 설정되어 있어야만 한다. */
            if ( idlOS::strncmp( sReceiverNode,
                                 SDM_NA_STR,
                                 SDI_NODE_NAME_MAX_SIZE + 1 )
                 != 0 )
            {                        
                ideLog::log(IDE_SD_17,"\n[sdiGlobalDDL] FLUSH [%s] %s\n", sSenderNode, sReplName);
                IDE_TEST( sdm::flushReplTable( aStatement,
                                               sSenderNode,
                                               aUserName,
                                               sReplName,
                                               ID_TRUE,
                                               sFlushWaitSecond )
                          != IDE_SUCCESS );

            }
        }
    }

    //---------------------------------
    // End Statement
    //---------------------------------

    sState = 1;
    IDE_TEST( sSmiStmt.end( SMI_STATEMENT_RESULT_SUCCESS ) != IDE_SUCCESS );

    sState = 0;
    QC_SMI_STMT(aStatement) = sOldStmt;

    IDE_EXCEPTION_CONT( NORMAL_EXIT );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();

    switch ( sState )
    {
        case 2:
            if ( sSmiStmt.end(SMI_STATEMENT_RESULT_FAILURE) != IDE_SUCCESS )
            {
                IDE_ERRLOG(IDE_SD_1);
            }
            else
            {
                /* Nothing to do */
            }
        case 1:
            QC_SMI_STMT(aStatement) = sOldStmt;
        default:
            break;
    }
    
    IDE_POP();    
    
    return IDE_FAILURE;
}

