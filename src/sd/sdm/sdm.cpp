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
#include <sdi.h>
#include <sdm.h>
#include <mtc.h>
#include <qcmTableInfo.h>
#include <qcmUser.h>
#include <qcg.h>
#include <qcmProc.h>
#include <qdsd.h>
#include <sduVersion.h>
#include <sdmShardPin.h>
#include <sdiZookeeper.h>

IDE_RC sdm::createMeta( qcStatement * aStatement )
{
    SChar        * sCrtMetaSql[] = {
        (SChar*) "CREATE USER SYS_SHARD IDENTIFIED BY MANAGER"
        ,
        (SChar*) "CREATE SEQUENCE SYS_SHARD.NEXT_NODE_ID MAXVALUE "
QCM_META_SEQ_MAXVALUE_STR" CYCLE"
        ,
        (SChar*) "CREATE SEQUENCE SYS_SHARD.NEXT_SHARD_ID MAXVALUE "
QCM_META_SEQ_MAXVALUE_STR" CYCLE"
        ,
        (SChar*) "CREATE TABLE SYS_SHARD.VERSION_ ( "
"MAJOR_VER          INTEGER, "
"MINOR_VER          INTEGER, "
"PATCH_VER          INTEGER )"
" SHARD META"
        ,
        (SChar*) "CREATE TABLE SYS_SHARD.LOCAL_META_INFO_ ( "
"SHARD_NODE_ID                  INTEGER NOT NULL, "
"SHARDED_DB_NAME                VARCHAR("SDI_SHARDED_DB_NAME_MAX_SIZE_STR") FIXED NOT NULL, "
"NODE_NAME                      VARCHAR("SDI_NODE_NAME_MAX_SIZE_STR") FIXED, "
"HOST_IP                        VARCHAR("SDI_SERVER_IP_SIZE_STR") FIXED, "
"PORT_NO                        INTEGER, "
"INTERNAL_HOST_IP               VARCHAR("SDI_SERVER_IP_SIZE_STR") FIXED, "
"INTERNAL_PORT_NO               INTEGER, "
"INTERNAL_REPLICATION_HOST_IP   VARCHAR("SDI_SERVER_IP_SIZE_STR") FIXED, "
"INTERNAL_REPLICATION_PORT_NO   INTEGER, "
"INTERNAL_CONN_TYPE             INTEGER, "
"K_SAFETY                       INTEGER, "
"REPLICATION_MODE               INTEGER, "
"PARALLEL_COUNT                 INTEGER )"
" SHARD META"
        ,
        (SChar*) "CREATE TABLE SYS_SHARD.GLOBAL_META_INFO_ ( "
"ID                 INTEGER, "
"SHARD_META_NUMBER  BIGINT )"
" SHARD META"
        ,
        (SChar*) "CREATE TABLE SYS_SHARD.NODES_ ( "
"NODE_ID            INTEGER                             NOT NULL, "
"NODE_NAME          VARCHAR("SDI_NODE_NAME_MAX_SIZE_STR")  FIXED NOT NULL, "
"HOST_IP            VARCHAR("SDI_SERVER_IP_SIZE_STR")                   FIXED NOT NULL, "
"PORT_NO            INTEGER                             NOT NULL, "
"ALTERNATE_HOST_IP  VARCHAR("SDI_SERVER_IP_SIZE_STR")                   FIXED, "
"ALTERNATE_PORT_NO  INTEGER, "
"INTERNAL_HOST_IP            VARCHAR("SDI_SERVER_IP_SIZE_STR")          FIXED NOT NULL, "
"INTERNAL_PORT_NO            INTEGER                    NOT NULL, "
"INTERNAL_ALTERNATE_HOST_IP  VARCHAR("SDI_SERVER_IP_SIZE_STR")          FIXED, "
"INTERNAL_ALTERNATE_PORT_NO  INTEGER, "
"INTERNAL_CONN_TYPE          INTEGER, "
"SMN                BIGINT                              NOT NULL )"
" SHARD META"
        ,
        (SChar*) "CREATE TABLE SYS_SHARD.REPLICA_SETS_ ( "
"REPLICA_SET_ID             INTEGER                             NOT NULL, "
//"REPLICA_SET_NAME           VARCHAR("SDI_REPLICA_SET_NAME_MAX_SIZE_STR")  FIXED NOT NULL, "
"PRIMARY_NODE_NAME          VARCHAR("SDI_NODE_NAME_MAX_SIZE_STR")  FIXED NOT NULL, "
"FIRST_BACKUP_NODE_NAME     VARCHAR("SDI_NODE_NAME_MAX_SIZE_STR")  FIXED NOT NULL, "
"SECOND_BACKUP_NODE_NAME    VARCHAR("SDI_NODE_NAME_MAX_SIZE_STR")  FIXED NOT NULL, "
"STOP_FIRST_BACKUP_NODE_NAME  VARCHAR("SDI_NODE_NAME_MAX_SIZE_STR")  FIXED NOT NULL, "
"STOP_SECOND_BACKUP_NODE_NAME VARCHAR("SDI_NODE_NAME_MAX_SIZE_STR")  FIXED NOT NULL, "
"FIRST_REPLICATION_NAME     VARCHAR("SDI_REPLICATION_NAME_MAX_SIZE_STR") FIXED NOT NULL, "
"FIRST_REPL_FROM_NODE_NAME  VARCHAR("SDI_NODE_NAME_MAX_SIZE_STR")  FIXED NOT NULL, "
"FIRST_REPL_TO_NODE_NAME    VARCHAR("SDI_NODE_NAME_MAX_SIZE_STR")  FIXED NOT NULL, "
"SECOND_REPLICATION_NAME    VARCHAR("SDI_REPLICATION_NAME_MAX_SIZE_STR") FIXED NOT NULL, "
"SECOND_REPL_FROM_NODE_NAME  VARCHAR("SDI_NODE_NAME_MAX_SIZE_STR") FIXED NOT NULL, "
"SECOND_REPL_TO_NODE_NAME    VARCHAR("SDI_NODE_NAME_MAX_SIZE_STR") FIXED NOT NULL, "
"SMN                BIGINT                              NOT NULL )"
" SHARD META"
        ,
        (SChar*) "CREATE TABLE SYS_SHARD.FAILOVER_HISTORY_ ( "
"REPLICA_SET_ID             INTEGER                             NOT NULL, "
//"REPLICA_SET_NAME           VARCHAR("SDI_REPLICA_SET_NAME_MAX_SIZE_STR")  FIXED NOT NULL, "
"PRIMARY_NODE_NAME          VARCHAR("SDI_NODE_NAME_MAX_SIZE_STR")  FIXED NOT NULL, "
"FIRST_BACKUP_NODE_NAME     VARCHAR("SDI_NODE_NAME_MAX_SIZE_STR")  FIXED NOT NULL, "
"SECOND_BACKUP_NODE_NAME    VARCHAR("SDI_NODE_NAME_MAX_SIZE_STR")  FIXED NOT NULL, "
"STOP_FIRST_BACKUP_NODE_NAME  VARCHAR("SDI_NODE_NAME_MAX_SIZE_STR")  FIXED NOT NULL, "
"STOP_SECOND_BACKUP_NODE_NAME VARCHAR("SDI_NODE_NAME_MAX_SIZE_STR")  FIXED NOT NULL, "
"FIRST_REPLICATION_NAME     VARCHAR("SDI_REPLICATION_NAME_MAX_SIZE_STR") FIXED NOT NULL, "
"FIRST_REPL_FROM_NODE_NAME  VARCHAR("SDI_NODE_NAME_MAX_SIZE_STR")  FIXED NOT NULL, "
"FIRST_REPL_TO_NODE_NAME    VARCHAR("SDI_NODE_NAME_MAX_SIZE_STR")  FIXED NOT NULL, "
"SECOND_REPLICATION_NAME    VARCHAR("SDI_REPLICATION_NAME_MAX_SIZE_STR") FIXED NOT NULL, "
"SECOND_REPL_FROM_NODE_NAME  VARCHAR("SDI_NODE_NAME_MAX_SIZE_STR") FIXED NOT NULL, "
"SECOND_REPL_TO_NODE_NAME    VARCHAR("SDI_NODE_NAME_MAX_SIZE_STR") FIXED NOT NULL, "
"SMN                BIGINT                              NOT NULL )"
" SHARD META"
        ,
        (SChar*) "CREATE TABLE SYS_SHARD.OBJECTS_ ( "
"SHARD_ID                         INTEGER  NOT NULL, "
"USER_NAME                        VARCHAR("QCM_META_OBJECT_NAME_LEN") FIXED NOT NULL, "
"OBJECT_NAME                      VARCHAR("QCM_META_OBJECT_NAME_LEN") FIXED NOT NULL, "
"OBJECT_TYPE                      CHAR(1)  NOT NULL, "
"SPLIT_METHOD                     CHAR(1)  NOT NULL, "
"KEY_COLUMN_NAME                  VARCHAR("QCM_META_OBJECT_NAME_LEN") FIXED, "
"SUB_SPLIT_METHOD                 CHAR(1), "
"SUB_KEY_COLUMN_NAME              VARCHAR("QCM_META_OBJECT_NAME_LEN") FIXED, "
"DEFAULT_NODE_ID                  INTEGER, "
"DEFAULT_PARTITION_NAME           VARCHAR("QCM_META_OBJECT_NAME_LEN") FIXED, "
"DEFAULT_PARTITION_REPLICA_SET_ID INTEGER  NOT NULL, "
"SMN                              BIGINT   NOT NULL )"
" SHARD META"
        ,
        (SChar*) "CREATE TABLE SYS_SHARD.RANGES_ ( "
"SHARD_ID           INTEGER  NOT NULL, "
"PARTITION_NAME     VARCHAR("QCM_META_OBJECT_NAME_LEN") FIXED NOT NULL, "
"VALUE              VARCHAR("SDI_RANGE_VARCHAR_MAX_PRECISION_STR") FIXED NOT NULL, "
"SUB_VALUE          VARCHAR("SDI_RANGE_VARCHAR_MAX_PRECISION_STR") FIXED NOT NULL, "
"NODE_ID            INTEGER  NOT NULL, "
"SMN                BIGINT   NOT NULL,"
"REPLICA_SET_ID     INTEGER  NOT NULL )"
" SHARD META"
        ,
        (SChar*) "CREATE TABLE SYS_SHARD.CLONES_ ( "
"SHARD_ID           INTEGER  NOT NULL, "
"NODE_ID            INTEGER  NOT NULL, "
"SMN                BIGINT   NOT NULL,"
"REPLICA_SET_ID     INTEGER  NOT NULL )"
" SHARD META"
                ,
        (SChar*) "CREATE TABLE SYS_SHARD.SOLOS_ ( "
"SHARD_ID           INTEGER  NOT NULL, "
"NODE_ID            INTEGER  NOT NULL, "
"SMN                BIGINT   NOT NULL, "
"REPLICA_SET_ID     INTEGER  NOT NULL )"
" SHARD META"

        ,
        (SChar*) "CREATE TABLE SYS_SHARD.REBUILD_STATE_ ( VALUE INTEGER )"
" SHARD META"
        ,
        (SChar*) "ALTER TABLE SYS_SHARD.LOCAL_META_INFO_ ADD CONSTRAINT LOCAL_META_INFO_PK "
"PRIMARY KEY ( SHARD_NODE_ID )"
        ,
        (SChar*) "ALTER TABLE SYS_SHARD.GLOBAL_META_INFO_ ADD CONSTRAINT GLOBAL_META_INFO_PK "
"PRIMARY KEY ( ID )"
        ,
        (SChar*) "ALTER TABLE SYS_SHARD.NODES_ ADD CONSTRAINT NODES_PK "
"PRIMARY KEY ( NODE_ID, SMN )"
        ,
        (SChar*) "CREATE UNIQUE INDEX SYS_SHARD.NODES_INDEX1 ON "
"SYS_SHARD.NODES_ ( NODE_NAME, SMN )"
        ,
                (SChar*) "CREATE UNIQUE INDEX SYS_SHARD.NODES_INDEX2 ON "
"SYS_SHARD.NODES_ ( SMN, NODE_ID )"
        ,
        (SChar*) "CREATE UNIQUE INDEX SYS_SHARD.NODES_INDEX3 ON "   
"SYS_SHARD.NODES_ ( HOST_IP, PORT_NO, SMN )"                             
        ,
/*
 * nullable인 alternate_... columns를 위하여 function-based index를 구성한다.
 */     
        (SChar*) "CREATE UNIQUE INDEX SYS_SHARD.NODES_INDEX4 ON "   
"SYS_SHARD.NODES_ ( CASE WHEN ALTERNATE_HOST_IP IS NOT NULL "
"                         AND ALTERNATE_PORT_NO IS NOT NULL "
"                        THEN ALTERNATE_HOST_IP||','||ALTERNATE_PORT_NO||','||SMN END )"
        ,
        (SChar*) "CREATE UNIQUE INDEX SYS_SHARD.NODES_INDEX5 ON "   
"SYS_SHARD.NODES_ ( INTERNAL_HOST_IP, INTERNAL_PORT_NO, SMN )"                             
        ,
        (SChar*) "CREATE UNIQUE INDEX SYS_SHARD.NODES_INDEX6 ON "   
"SYS_SHARD.NODES_ ( CASE WHEN INTERNAL_ALTERNATE_HOST_IP IS NOT NULL "
"                         AND INTERNAL_ALTERNATE_PORT_NO IS NOT NULL "
"                        THEN INTERNAL_ALTERNATE_HOST_IP||','||INTERNAL_ALTERNATE_PORT_NO||','||SMN END )"
        ,
        (SChar*) "CREATE UNIQUE INDEX SYS_SHARD.NODES_INDEX7 ON "
"SYS_SHARD.NODES_ ( SMN, NODE_NAME )"
        ,
        (SChar*) "ALTER TABLE SYS_SHARD.REPLICA_SETS_ ADD CONSTRAINT REPLICA_SETS_PK "
"PRIMARY KEY ( REPLICA_SET_ID, SMN )"
        ,
        (SChar*) "CREATE INDEX SYS_SHARD.REPLICA_SETS_INDEX1 ON "
"SYS_SHARD.REPLICA_SETS_ ( SMN, PRIMARY_NODE_NAME )"
        ,
        (SChar*) "ALTER TABLE SYS_SHARD.FAILOVER_HISTORY_ ADD CONSTRAINT FAILOVER_HISTORY_PK "
"PRIMARY KEY ( REPLICA_SET_ID, SMN )"
        ,
        (SChar*) "CREATE INDEX SYS_SHARD.FAILOVER_HISTORY_INDEX1 ON "
"SYS_SHARD.FAILOVER_HISTORY_ ( PRIMARY_NODE_NAME, SMN )"
        ,
        (SChar*) "CREATE INDEX SYS_SHARD.FAILOVER_HISTORY_INDEX2 ON "
"SYS_SHARD.FAILOVER_HISTORY_ ( SMN )"
        ,
        (SChar*) "ALTER TABLE SYS_SHARD.OBJECTS_ ADD CONSTRAINT OBJECTS_PK "
"PRIMARY KEY ( SHARD_ID, SMN )"
        ,
        (SChar*) "CREATE UNIQUE INDEX SYS_SHARD.OBJECTS_INDEX1 ON "
"SYS_SHARD.OBJECTS_ ( SMN, USER_NAME, OBJECT_NAME )"
        ,
        (SChar*) "ALTER TABLE SYS_SHARD.RANGES_ ADD CONSTRAINT RANGES_PK "
"PRIMARY KEY ( SHARD_ID, SMN, VALUE, SUB_VALUE )"
        ,
        (SChar*) "ALTER TABLE SYS_SHARD.CLONES_ ADD CONSTRAINT CLONES_PK "
"PRIMARY KEY ( SHARD_ID, SMN, NODE_ID )"
        ,
        (SChar*) "ALTER TABLE SYS_SHARD.SOLOS_ ADD CONSTRAINT SOLOS_PK "
"PRIMARY KEY ( SHARD_ID, SMN )"
        ,
        (SChar*) "INSERT INTO SYS_SHARD.VERSION_ VALUES ("
SHARD_MAJOR_VERSION_STR", "
SHARD_MINOR_VERSION_STR", "
SHARD_PATCH_VERSION_STR")"
        ,
        (SChar*) "INSERT INTO SYS_SHARD.GLOBAL_META_INFO_ VALUES ( INTEGER'0', BIGINT'1' )"
        ,
/* The last item should be NULL */
        NULL    };

    SChar        * sPostStrArg1MetaSql[][2] = {
        { (SChar*) "INSERT INTO SYS_SHARD.LOCAL_META_INFO_ VALUES ( "
                        "INTEGER'"SDM_DEFAULT_SHARD_NODE_ID_STR"', "
                        "VARCHAR'%s', "
                        "NULL, "
                        "NULL, "
                        "NULL, "
                        "NULL, "
                        "NULL, "
                        "NULL, "
                        "NULL, "
                        "NULL, "
                        "NULL, "
                        "NULL, "
                        "NULL )" ,
          NULL
        },
        /* The last item should be NULL */
        {
          NULL ,
          NULL
        }
    };
    smiTrans       sTrans;
    smiStatement   sSmiStmt;
    smiStatement * sDummySmiStmt;
    SChar        * sSqlStr;
    vSLong         sRowCnt;
    UInt           sStage = 0;
    UInt           i;
    UInt           sShardUserID = ID_UINT_MAX;

    IDE_TEST( STRUCT_ALLOC_WITH_SIZE( aStatement->qmxMem,
                                      SChar,
                                      QD_MAX_SQL_LENGTH,
                                      &sSqlStr )
              != IDE_SUCCESS );

    IDE_TEST( sTrans.initialize() != IDE_SUCCESS );
    sStage = 1;

    for (i = 0; sCrtMetaSql[i] != NULL; i++)
    {
        IDE_TEST( sTrans.begin( &sDummySmiStmt,
                                NULL,
                                (SMI_ISOLATION_NO_PHANTOM     |
                                 SMI_TRANSACTION_NORMAL       |
                                 SMI_TRANSACTION_REPL_DEFAULT |
                                 SMI_COMMIT_WRITE_NOWAIT))
                  != IDE_SUCCESS );
        sStage = 2;

        IDE_TEST( sSmiStmt.begin( NULL,
                                  sDummySmiStmt,
                                  (SMI_STATEMENT_NORMAL |
                                   SMI_STATEMENT_MEMORY_CURSOR) )
                  != IDE_SUCCESS );
        sStage = 3;

        idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                         "%s",
                         sCrtMetaSql[i] );

        IDE_TEST( qciMisc::runSQLforShardMeta( &sSmiStmt,
                                               sSqlStr,
                                               &sRowCnt )
                  != IDE_SUCCESS );

        sStage = 2;
        IDE_TEST( sSmiStmt.end( SMI_STATEMENT_RESULT_SUCCESS )
                  != IDE_SUCCESS );

        sStage = 1;
        IDE_TEST( sTrans.commit() != IDE_SUCCESS );

        ideLog::log( IDE_SD_17, "[SHARD_META] %s\n", sCrtMetaSql[i] );
    }

    /* sharded database name insert to local_meta_info_ */
    sPostStrArg1MetaSql[0][1] = smiGetDBName();

    for (i = 0; (sPostStrArg1MetaSql[i][0] != NULL) && (sPostStrArg1MetaSql[i][1] != NULL); i++)
    {
        IDE_TEST( sTrans.begin( &sDummySmiStmt,
                                NULL,
                                (SMI_ISOLATION_NO_PHANTOM     |
                                 SMI_TRANSACTION_NORMAL       |
                                 SMI_TRANSACTION_REPL_DEFAULT |
                                 SMI_COMMIT_WRITE_NOWAIT))
                  != IDE_SUCCESS );
        sStage = 2;

        IDE_TEST( sSmiStmt.begin( NULL,
                                  sDummySmiStmt,
                                  (SMI_STATEMENT_NORMAL |
                                   SMI_STATEMENT_MEMORY_CURSOR) )
                  != IDE_SUCCESS );
        sStage = 3;

        idlOS::snprintf( sSqlStr,
                         QD_MAX_SQL_LENGTH,
                         sPostStrArg1MetaSql[i][0],
                         sPostStrArg1MetaSql[i][1]);

        IDE_TEST( qciMisc::runSQLforShardMeta( &sSmiStmt,
                                               sSqlStr,
                                               &sRowCnt )
                  != IDE_SUCCESS );

        sStage = 2;
        IDE_TEST( sSmiStmt.end( SMI_STATEMENT_RESULT_SUCCESS )
                  != IDE_SUCCESS );

        sStage = 1;
        IDE_TEST( sTrans.commit() != IDE_SUCCESS );

        ideLog::log( IDE_SD_17, "[SHARD_META] %s\n", sSqlStr );
    }

    sStage = 0;
    IDE_TEST( sTrans.destroy( NULL ) != IDE_SUCCESS );

    /* ShardUserID를 세팅한다. */
    IDE_TEST( getShardUserID( &sShardUserID ) != IDE_SUCCESS );
    sdi::setShardUserID( sShardUserID );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch ( sStage )
    {
        case 3:
            ( void )sSmiStmt.end( SMI_STATEMENT_RESULT_FAILURE );
        case 2:
            ( void )sTrans.commit();
        case 1:
            ( void )sTrans.destroy( NULL );
        default:
            break;
    }

    ideLog::log( IDE_SD_1, "[SHARD_META_ERROR] Failure. errorcode 0x%05"ID_XINT32_FMT" %s\n",
                           E_ERROR_CODE(ideGetErrorCode()),
                           ideGetErrorMsg(ideGetErrorCode()));
    return IDE_FAILURE;
}

IDE_RC sdm::checkIsInCluster( qcStatement * aStatement,
                              idBool      * aIsInCluster )
{
    SChar         sSqlStr[1024];
    idBool        sRecordExist = ID_FALSE;
    mtdBigintType sResultCount = 0;

    idlOS::snprintf( sSqlStr, 1024,
                     "SELECT NODE_ID FROM SYS_SHARD.NODES_");

    IDE_TEST( qcg::runSelectOneRowforDDL( QC_SMI_STMT( aStatement ),
                                          sSqlStr,
                                          &sResultCount,
                                          &sRecordExist,
                                          ID_FALSE )
              != IDE_SUCCESS );

    *aIsInCluster = sRecordExist;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdm::checkMetaVersion( smiStatement * aSmiStmt )
{
    UInt                 sStage = 0;
    smiTableCursor       sCursor;
    const void         * sRow;
    UInt                 sFlag = QCM_META_CURSOR_FLAG;
    const void         * sSdmVersion;
    mtcColumn          * sSdmVersionMajorVerColumn;
    mtcColumn          * sSdmVersionMinorVerColumn;
    mtcColumn          * sSdmVersionPatchVerColumn;
    mtdIntegerType       sMajorVer;
    mtdIntegerType       sMinorVer;
    mtdIntegerType       sPatchVer;

    scGRID               sRid; // Disk Table을 위한 Record IDentifier
    smiCursorProperties  sCursorProperty;

    IDE_TEST( getMetaTableAndIndex( aSmiStmt,
                                    SDM_VERSION,
                                    & sSdmVersion,
                                    NULL )
              != IDE_SUCCESS );

    IDE_TEST( smiGetTableColumns( sSdmVersion,
                                  SDM_VERSION_MAJOR_VER_COL_ORDER,
                                  (const smiColumn**)&sSdmVersionMajorVerColumn )
              != IDE_SUCCESS );

    IDE_TEST( smiGetTableColumns( sSdmVersion,
                                  SDM_VERSION_MINOR_VER_COL_ORDER,
                                  (const smiColumn**)&sSdmVersionMinorVerColumn )
              != IDE_SUCCESS );

    IDE_TEST( smiGetTableColumns( sSdmVersion,
                                  SDM_VERSION_PATCH_VER_COL_ORDER,
                                  (const smiColumn**)&sSdmVersionPatchVerColumn )
              != IDE_SUCCESS );

    sCursor.initialize();

    SMI_CURSOR_PROP_INIT_FOR_META_FULL_SCAN(&sCursorProperty, NULL);

    IDE_TEST(sCursor.open(
                 aSmiStmt,
                 sSdmVersion,
                 NULL,
                 smiGetRowSCN(sSdmVersion),
                 NULL,
                 smiGetDefaultKeyRange(),
                 smiGetDefaultKeyRange(),
                 smiGetDefaultFilter(),
                 sFlag,
                 SMI_SELECT_CURSOR,
                 &sCursorProperty) != IDE_SUCCESS);
    sStage = 1;

    IDE_TEST(sCursor.beforeFirst() != IDE_SUCCESS);

    IDE_TEST(sCursor.readRow(&sRow, &sRid, SMI_FIND_NEXT) != IDE_SUCCESS);
    // 1건도 없으면 에러
    IDE_TEST_RAISE( sRow == NULL, ERR_CHECK_META_VERSION );

    sMajorVer = *(mtdIntegerType*)((SChar *)sRow + sSdmVersionMajorVerColumn->column.offset );
    sMinorVer = *(mtdIntegerType*)((SChar *)sRow + sSdmVersionMinorVerColumn->column.offset );
    sPatchVer = *(mtdIntegerType*)((SChar *)sRow + sSdmVersionPatchVerColumn->column.offset );

    IDE_TEST(sCursor.readRow(&sRow, &sRid, SMI_FIND_NEXT) != IDE_SUCCESS);
    // 1건이어야 한다.
    IDE_TEST_RAISE( sRow != NULL, ERR_CHECK_META_VERSION );

    sStage = 0;
    IDE_TEST(sCursor.close() != IDE_SUCCESS);

    IDE_TEST_RAISE( ( sMajorVer != SHARD_MAJOR_VERSION ) ||
                    ( sMinorVer != SHARD_MINOR_VERSION ) ||
                    ( sPatchVer != SHARD_PATCH_VERSION ),
                    ERR_MISMATCH_META_VERSION );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_CHECK_META_VERSION )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDM_CHECK_META_VERSION ) );
    }
    IDE_EXCEPTION( ERR_MISMATCH_META_VERSION )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDM_MISMATCH_META_VERSION ) );
    }
    IDE_EXCEPTION_END;

    if ( sStage == 1 )
    {
        (void)sCursor.close();
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_FAILURE;
}

IDE_RC sdm::resetMetaNodeId( qcStatement * aStatement, sdiLocalMetaInfo * aMetaNodeInfo )
{
    smiTrans       sTrans;
    smiStatement   sSmiStmt;
    smiStatement * sDummySmiStmt = NULL;
    SChar        * sSqlStr       = NULL;
    vSLong         sRowCnt       = 0;
    UInt           sStage        = 0;

    IDE_TEST( STRUCT_ALLOC_WITH_SIZE( aStatement->qmxMem,
                                      SChar,
                                      QD_MAX_SQL_LENGTH,
                                      &sSqlStr )
              != IDE_SUCCESS );

    IDE_TEST( sTrans.initialize() != IDE_SUCCESS );
    sStage = 1;

    IDE_TEST( sTrans.begin( &sDummySmiStmt,
                            NULL,
                            ( SMI_ISOLATION_NO_PHANTOM     |
                              SMI_TRANSACTION_NORMAL       |
                              SMI_TRANSACTION_REPL_DEFAULT |
                              SMI_COMMIT_WRITE_NOWAIT ) )
              != IDE_SUCCESS );
    sStage = 2;

    IDE_TEST( sSmiStmt.begin( NULL,
                              sDummySmiStmt,
                              ( SMI_STATEMENT_NORMAL |
                                SMI_STATEMENT_MEMORY_CURSOR ) )
              != IDE_SUCCESS );
    sStage = 3;

    IDE_TEST( checkMetaVersion( &sSmiStmt )
              != IDE_SUCCESS );

    idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                     "UPDATE SYS_SHARD.LOCAL_META_INFO_ SET SHARD_NODE_ID = "
                     QCM_SQL_INT32_FMT,
                     aMetaNodeInfo->mShardNodeId );

    IDE_TEST( qciMisc::runDMLforDDL( &sSmiStmt,
                                     sSqlStr,
                                     &sRowCnt )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( sRowCnt != 1, ERR_UPDATE );

    IDE_TEST( sSmiStmt.end( SMI_STATEMENT_RESULT_SUCCESS )
              != IDE_SUCCESS );
    sStage = 2;

    IDE_TEST( sTrans.commit() != IDE_SUCCESS );
    sStage = 1;

    sStage = 0;
    IDE_TEST( sTrans.destroy( NULL ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_UPDATE );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QCM_INTERNAL_ARG,
                                  "[sdm::resetMetaNodeId] ERR_UPDATED_COUNT_IS_NOT_1" ) ); 
    }
    IDE_EXCEPTION_END;

    IDE_PUSH();

    switch ( sStage )
    {
        case 3:
            ( void )sSmiStmt.end( SMI_STATEMENT_RESULT_FAILURE );
            /* fall through */
        case 2:
            ( void )sTrans.rollback();
            /* fall through */
        case 1:
            ( void )sTrans.destroy( NULL );
            /* fall through */
        default:
            break;
    }

    IDE_POP();

    ideLog::log( IDE_SD_1, "[SHARD_META_ERROR] Failure. errorcode 0x%05"ID_XINT32_FMT" %s\n",
                           E_ERROR_CODE(ideGetErrorCode()),
                           ideGetErrorMsg(ideGetErrorCode()));
    return IDE_FAILURE;
}

IDE_RC sdm::getMetaTableAndIndex( smiStatement * aSmiStmt,
                                  const SChar  * aMetaTableName,
                                  const void  ** aTableHandle,
                                  const void  ** aIndexHandle )
{
    UInt        sUserID;
    smSCN       sTableSCN;
    //mtcColumn * sColumn;
    //UInt        sColumnCnt;
    UInt        sIndexCnt;
    UInt        i;

    IDE_TEST_RAISE( qcmUser::getUserID( NULL,
                                        aSmiStmt,
                                        SDM_USER,
                                        idlOS::strlen(SDM_USER),
                                        & sUserID ) != IDE_SUCCESS,
                    ERR_META_HANDLE );

    IDE_TEST_RAISE( qcm::getTableHandleByName( aSmiStmt,
                                               sUserID,
                                               (UChar *)aMetaTableName,
                                               idlOS::strlen(aMetaTableName),
                                               (void**)aTableHandle,
                                               &sTableSCN ) != IDE_SUCCESS,
                    ERR_META_HANDLE );

    if ( aIndexHandle != NULL )
    {
        sIndexCnt = smiGetTableIndexCount( *aTableHandle );

        for ( i = 0; i < sIndexCnt; i++ )
        {
            aIndexHandle[i] = smiGetTableIndexes( *aTableHandle,
                                                  i );
        }
        for ( ; i < SDM_MAX_META_INDICES; i++ )
        {
            aIndexHandle[i] = NULL;
        }
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_META_HANDLE )
    {
        if ( ideGetErrorCode() == qpERR_ABORT_QCM_NOT_EXIST_USER )
        {
            IDE_SET( ideSetErrorCode(sdERR_ABORT_SDM_SHARD_META_NOT_CREATED,
                                     SDM_USER) );
        }
        else if ( ideGetErrorCode() == qpERR_ABORT_QCM_NOT_EXIST_TABLE )
        {
            IDE_SET( ideSetErrorCode(sdERR_ABORT_SDM_SHARD_META_NOT_CREATED,
                                     aMetaTableName) );
        }
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdm::getNextNodeID( qcStatement * aStatement,
                           UInt        * aNodeID,
                           ULong         aSMN )
{
    UInt     sUserID;
    void   * sSequenceHandle = NULL;
    idBool   sExist;
    idBool   sFirst;
    SLong    sSeqVal = 0;
    SLong    sSeqValFirst = 0;

    sExist = ID_TRUE;
    sFirst = ID_TRUE;

    IDE_TEST_RAISE( qcmUser::getUserID( QC_STATISTICS( aStatement ),
                                        QC_SMI_STMT( aStatement ),
                                        SDM_USER,
                                        idlOS::strlen(SDM_USER),
                                        & sUserID ) != IDE_SUCCESS,
                    ERR_META_HANDLE );

    IDE_TEST_RAISE( qciMisc::getSequenceHandleByName(
                        QC_SMI_STMT( aStatement ),
                        sUserID,
                        (UChar*)SDM_SEQUENCE_NODE_NAME,
                        idlOS::strlen(SDM_SEQUENCE_NODE_NAME),
                        &sSequenceHandle ) != IDE_SUCCESS,
                    ERR_META_HANDLE );

    while ( sExist == ID_TRUE )
    {
        IDE_TEST( smiTable::readSequence( QC_SMI_STMT( aStatement ),
                                          sSequenceHandle,
                                          SMI_SEQUENCE_NEXT,
                                          &sSeqVal,
                                          NULL )
                 != IDE_SUCCESS );

        // sSeqVal은 비록 SLong이지만, sequence를 생성할 때
        // max를 integer max를 안넘도록 하였기 때문에
        // 여기서 overflow체크는 하지 않는다.
        IDE_TEST( searchNodeID( QC_SMI_STMT( aStatement ),
                                (SInt)sSeqVal,
                                aSMN,
                                &sExist )
                  != IDE_SUCCESS );

        if ( sFirst == ID_TRUE )
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

    *aNodeID = sSeqVal;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_META_HANDLE )
    {
        if ( ideGetErrorCode() == qpERR_ABORT_QCM_NOT_EXIST_USER )
        {
            IDE_SET( ideSetErrorCode(sdERR_ABORT_SDM_SHARD_META_NOT_CREATED,
                                     SDM_USER) );
        }
        else if ( ideGetErrorCode() == qpERR_ABORT_QCM_NOT_EXIST_SEQUENCE )
        {
            IDE_SET( ideSetErrorCode(sdERR_ABORT_SDM_SHARD_META_NOT_CREATED,
                                     SDM_SEQUENCE_NODE_NAME) );
        }
    }
    IDE_EXCEPTION( ERR_OBJECTS_OVERFLOW )
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QCM_MAXIMUM_OBJECT_NUMBER_EXCEED,
                                "shard node",
                                QCM_META_SEQ_MAXVALUE) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdm::searchNodeID( smiStatement * aSmiStmt,
                          SInt           aNodeID,
                          ULong          aSMN,
                          idBool       * aExist )
{
    UInt                 sStage = 0;
    smiRange             sRange;
    smiTableCursor       sCursor;
    const void         * sRow;
    UInt                 sFlag = QCM_META_CURSOR_FLAG;
    const void         * sSdmNodes;
    const void         * sSdmNodesIndex[SDM_MAX_META_INDICES];
    mtcColumn          * sSdmNodeIDColumn;
    qtcMetaRangeColumn   sNodeIDRange;
    mtcColumn          * sSdmSMNColumn;
    qtcMetaRangeColumn   sSMNRange;

    scGRID              sRid; // Disk Table을 위한 Record IDentifier
    smiCursorProperties sCursorProperty;

    IDE_TEST( getMetaTableAndIndex( aSmiStmt,
                                    SDM_NODES,
                                    & sSdmNodes,
                                    sSdmNodesIndex )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( sSdmNodesIndex[SDM_NODES_IDX1_ORDER] == NULL,
                    ERR_META_HANDLE );

    IDE_TEST( smiGetTableColumns( sSdmNodes,
                                  SDM_NODES_NODE_ID_COL_ORDER,
                                  (const smiColumn**)&sSdmNodeIDColumn )
              != IDE_SUCCESS );

    IDE_TEST( smiGetTableColumns( sSdmNodes,
                                  SDM_NODES_SMN_COL_ORDER,
                                  (const smiColumn**)&sSdmSMNColumn )
              != IDE_SUCCESS );

    // mtdModule 설정
    IDE_TEST( mtd::moduleById( &(sSdmNodeIDColumn->module),
                               sSdmNodeIDColumn->type.dataTypeId )
              != IDE_SUCCESS );

    // mtlModule 설정
    IDE_TEST( mtl::moduleById( &sSdmNodeIDColumn->language,
                               sSdmNodeIDColumn->type.languageId )
              != IDE_SUCCESS );

    // mtdModule 설정
    IDE_TEST( mtd::moduleById( &(sSdmSMNColumn->module),
                               sSdmSMNColumn->type.dataTypeId )
              != IDE_SUCCESS );

    // mtlModule 설정
    IDE_TEST( mtl::moduleById( &sSdmSMNColumn->language,
                               sSdmSMNColumn->type.languageId )
              != IDE_SUCCESS );

    qciMisc::makeMetaRangeDoubleColumn(
        &sNodeIDRange,
        &sSMNRange,
        (const mtcColumn *) sSdmNodeIDColumn,
        (const void *) &aNodeID,
        (const mtcColumn *) sSdmSMNColumn,
        (const void *) &aSMN,        
        &sRange);

    sCursor.initialize();

    SMI_CURSOR_PROP_INIT_FOR_META_INDEX_SCAN(&sCursorProperty, NULL);

    IDE_TEST(sCursor.open(
                 aSmiStmt,
                 sSdmNodes,
                 sSdmNodesIndex[SDM_NODES_IDX1_ORDER],
                 smiGetRowSCN(sSdmNodes),
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

    if ( sRow == NULL )
    {
        *aExist = ID_FALSE;
    }
    else
    {
        *aExist = ID_TRUE;
    }

    sStage = 0;
    IDE_TEST(sCursor.close() != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_META_HANDLE )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDM_SHARD_META_NOT_CREATED,
                                  SDM_NODES ) );
    }
    IDE_EXCEPTION_END;

    if ( sStage == 1 )
    {
        (void)sCursor.close();
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_FAILURE;
}

IDE_RC sdm::getNextShardID( qcStatement * aStatement,
                            UInt        * aShardID,
                            ULong         aSMN )
{
    UInt     sUserID;
    void   * sSequenceHandle = NULL;
    idBool   sExist;
    idBool   sFirst;
    SLong    sSeqVal = 0;
    SLong    sSeqValFirst = 0;

    sExist = ID_TRUE;
    sFirst = ID_TRUE;

    IDE_TEST_RAISE( qcmUser::getUserID( QC_STATISTICS( aStatement ),
                                        QC_SMI_STMT( aStatement ),
                                        SDM_USER,
                                        idlOS::strlen(SDM_USER),
                                        & sUserID ) != IDE_SUCCESS,
                    ERR_META_HANDLE );

    IDE_TEST_RAISE( qciMisc::getSequenceHandleByName(
                        QC_SMI_STMT( aStatement ),
                        sUserID,
                        (UChar*)SDM_SEQUENCE_SHARD_NAME,
                        idlOS::strlen(SDM_SEQUENCE_SHARD_NAME),
                        &sSequenceHandle ) != IDE_SUCCESS,
                    ERR_META_HANDLE );

    while ( sExist == ID_TRUE )
    {
        IDE_TEST( smiTable::readSequence( QC_SMI_STMT( aStatement ),
                                          sSequenceHandle,
                                          SMI_SEQUENCE_NEXT,
                                          &sSeqVal,
                                          NULL )
                 != IDE_SUCCESS );

        // sSeqVal은 비록 SLong이지만, sequence를 생성할 때
        // max를 integer max를 안넘도록 하였기 때문에
        // 여기서 overflow체크는 하지 않는다.
        IDE_TEST( searchShardID( QC_SMI_STMT( aStatement ),
                                 (SInt)sSeqVal,
                                 aSMN,
                                 &sExist )
                  != IDE_SUCCESS );

        if ( sFirst == ID_TRUE )
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

    *aShardID = sSeqVal;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_META_HANDLE )
    {
        if ( ideGetErrorCode() == qpERR_ABORT_QCM_NOT_EXIST_USER )
        {
            IDE_SET( ideSetErrorCode(sdERR_ABORT_SDM_SHARD_META_NOT_CREATED,
                                     SDM_USER) );
        }
        else if ( ideGetErrorCode() == qpERR_ABORT_QCM_NOT_EXIST_SEQUENCE )
        {
            IDE_SET( ideSetErrorCode(sdERR_ABORT_SDM_SHARD_META_NOT_CREATED,
                                     SDM_SEQUENCE_SHARD_NAME) );
        }
    }
    IDE_EXCEPTION( ERR_OBJECTS_OVERFLOW )
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QCM_MAXIMUM_OBJECT_NUMBER_EXCEED,
                                "shard object",
                                QCM_META_SEQ_MAXVALUE) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdm::searchShardID( smiStatement * aSmiStmt,
                           SInt           aShardID,
                           ULong          aSMN,
                           idBool       * aExist )
{
    UInt                 sStage = 0;
    smiRange             sRange;
    smiTableCursor       sCursor;
    const void         * sRow;
    UInt                 sFlag = QCM_META_CURSOR_FLAG;
    const void         * sSdmObjects;
    const void         * sSdmObjectsIndex[SDM_MAX_META_INDICES];
    mtcColumn          * sSdmShardIDColumn;
    qtcMetaRangeColumn   sShardIDRange;
    mtcColumn          * sSdmSMNColumn;
    qtcMetaRangeColumn   sSMNRange;

    scGRID              sRid; // Disk Table을 위한 Record IDentifier
    smiCursorProperties sCursorProperty;

    IDE_TEST( getMetaTableAndIndex( aSmiStmt,
                                    SDM_OBJECTS,
                                    & sSdmObjects,
                                    sSdmObjectsIndex )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( sSdmObjectsIndex[SDM_OBJECTS_IDX1_ORDER] == NULL,
                    ERR_META_HANDLE );

    IDE_TEST( smiGetTableColumns( sSdmObjects,
                                  SDM_OBJECTS_SHARD_ID_COL_ORDER,
                                  (const smiColumn**)&sSdmShardIDColumn )
              != IDE_SUCCESS );
    IDE_TEST( smiGetTableColumns( sSdmObjects,
                                  SDM_OBJECTS_SMN_COL_ORDER,
                                  (const smiColumn**)&sSdmSMNColumn )
              != IDE_SUCCESS );

    // mtdModule 설정
    IDE_TEST( mtd::moduleById( &(sSdmShardIDColumn->module),
                               sSdmShardIDColumn->type.dataTypeId )
              != IDE_SUCCESS );

    // mtlModule 설정
    IDE_TEST( mtl::moduleById( &sSdmShardIDColumn->language,
                               sSdmShardIDColumn->type.languageId )
              != IDE_SUCCESS );

    // mtdModule 설정
    IDE_TEST( mtd::moduleById( &(sSdmSMNColumn->module),
                               sSdmSMNColumn->type.dataTypeId )
              != IDE_SUCCESS );

    // mtlModule 설정
    IDE_TEST( mtl::moduleById( &sSdmSMNColumn->language,
                               sSdmSMNColumn->type.languageId )
              != IDE_SUCCESS );

    qciMisc::makeMetaRangeDoubleColumn(
        &sShardIDRange,
        &sSMNRange,
        (const mtcColumn *) sSdmShardIDColumn,
        (const void *) &aShardID,
        (const mtcColumn *) sSdmSMNColumn,
        (const void *) &aSMN,
        &sRange);

    sCursor.initialize();

    SMI_CURSOR_PROP_INIT_FOR_META_INDEX_SCAN(&sCursorProperty, NULL);

    IDE_TEST(sCursor.open(
                 aSmiStmt,
                 sSdmObjects,
                 sSdmObjectsIndex[SDM_OBJECTS_IDX1_ORDER],
                 smiGetRowSCN(sSdmObjects),
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

    if ( sRow == NULL )
    {
        *aExist = ID_FALSE;
    }
    else
    {
        *aExist = ID_TRUE;
    }

    sStage = 0;
    IDE_TEST(sCursor.close() != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_META_HANDLE )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDM_SHARD_META_NOT_CREATED,
                                  SDM_OBJECTS ) );
    }
    IDE_EXCEPTION_END;

    if ( sStage == 1 )
    {
        (void)sCursor.close();
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_FAILURE;
}

// insert node
IDE_RC sdm::insertNode( qcStatement * aStatement,
                        UInt        * aNodeID,
                        SChar       * aNodeName,
                        UInt          aPortNo,
                        SChar       * aHostIP,
                        UInt          aInternalPortNo,
                        SChar       * aInternalHostIP,
                        UInt          aConnType,
                        UInt        * aRowCnt )
{
    SChar      * sSqlStr;
    vSLong       sRowCnt;
    UInt         sNodeID;
    sdiGlobalMetaInfo sMetaNodeInfo = { SDI_NULL_SMN };

    IDE_TEST( checkMetaVersion( QC_SMI_STMT( aStatement ) )
              != IDE_SUCCESS );

    IDE_TEST( makeShardMeta4NewSMN(aStatement) != IDE_SUCCESS );


    IDE_TEST( getGlobalMetaInfoCore( QC_SMI_STMT(aStatement),
                                         &sMetaNodeInfo ) != IDE_SUCCESS );

    IDE_TEST( STRUCT_ALLOC_WITH_SIZE( aStatement->qmxMem,
                                      SChar,
                                      QD_MAX_SQL_LENGTH,
                                      &sSqlStr )
              != IDE_SUCCESS);

    /* 원래 SET_NODE에 node id가 없을 경우에는 실패해야 하나                *
     * shard2의 testcase diff를 줄이기 위해 node id가 없는 경우에는         *
     * 기존과 마찬가지로 각 노드의 sequence에서 node id를 가져오도록 한다.  *
     * 이 경우에는 각 노드간 node id가 동일하지 않을 수 있다.               */
    if( *aNodeID == SDI_NODE_NULL_ID )
    {
        IDE_TEST( getNextNodeID( aStatement,
                                 &sNodeID,
                                 sMetaNodeInfo.mShardMetaNumber )
                  != IDE_SUCCESS );

        // SHARD DDL 지원으로 node id가 null인경우는 없다.
        // shard2 test case에서 명시적으로 SET_NODE() 수행한다.
        *aNodeID = sNodeID;
    }
    else
    {
        sNodeID = *aNodeID;
    }

    if ( ( aInternalPortNo == 0 ) || ( aInternalHostIP[0] == '\0' ) )
    {
        idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                         "INSERT INTO SYS_SHARD.NODES_ "
                         "VALUES ( "
                         QCM_SQL_INT32_FMT", "     /* NODE_ID                    */
                         QCM_SQL_VARCHAR_FMT", "   /* NODE_NAME                  */
                         QCM_SQL_VARCHAR_FMT", "   /* HOST_IP                    */
                         QCM_SQL_INT32_FMT", "     /* PORT_NO                    */
                         "NULL, "                  /* ALTERNATE_HOST_IP          */
                         "NULL, "                  /* ALTERNATE_PORT_NO          */
                         QCM_SQL_VARCHAR_FMT", "   /* INTERNAL_HOST_IP           */
                         QCM_SQL_INT32_FMT", "     /* INTERNAL_PORT_NO           */
                         "NULL, "                  /* INTERNAL_ALTERNATE_HOST_IP */
                         "NULL, "                  /* INTERNAL_ALTERNATE_PORT_NO */
                         QCM_SQL_INT32_FMT", "     /* INTERNAL_CONN_TYPE         */
                         QCM_SQL_BIGINT_FMT" ) ",  /* SMN */
                         sNodeID,
                         aNodeName,
                         aHostIP,
                         aPortNo,
                         aHostIP,
                         aPortNo,
                         aConnType,
                         sMetaNodeInfo.mShardMetaNumber );
    }
    else
    {
        if( SDU_SHARD_ZOOKEEPER_TEST == 0 )
        {
            idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                             "INSERT INTO SYS_SHARD.NODES_ "
                             "VALUES ( "
                             QCM_SQL_INT32_FMT", "     /* NODE_ID                    */
                             QCM_SQL_VARCHAR_FMT", "   /* NODE_NAME                  */
                             QCM_SQL_VARCHAR_FMT", "   /* HOST_IP                    */
                             QCM_SQL_INT32_FMT", "     /* PORT_NO                    */
                             QCM_SQL_VARCHAR_FMT", "   /* ALTERNATE_HOST_IP          */
                             QCM_SQL_INT32_FMT", "     /* ALTERNATE_PORT_NO          */
                             QCM_SQL_VARCHAR_FMT", "   /* INTERNAL_HOST_IP           */
                             QCM_SQL_INT32_FMT", "     /* INTERNAL_PORT_NO           */
                             QCM_SQL_VARCHAR_FMT", "   /* INTERNAL_ALTERNATE_HOST_IP */
                             QCM_SQL_INT32_FMT", "     /* INTERNAL_ALTERNATE_PORT_NO */
                             QCM_SQL_INT32_FMT", "     /* INTERNAL_CONN_TYPE         */
                             QCM_SQL_BIGINT_FMT" ) ",  /* SMN */
                             sNodeID,
                             aNodeName,
                             aHostIP,
                             aPortNo,
                             aInternalHostIP,
                             aInternalPortNo,
                             aHostIP,
                             aPortNo,
                             aInternalHostIP,
                             aInternalPortNo,
                             aConnType,
                             sMetaNodeInfo.mShardMetaNumber );
        }
        else
        {
            idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                             "INSERT INTO SYS_SHARD.NODES_ "
                             "VALUES ( "
                             QCM_SQL_INT32_FMT", "     /* NODE_ID                    */
                             QCM_SQL_VARCHAR_FMT", "   /* NODE_NAME                  */
                             QCM_SQL_VARCHAR_FMT", "   /* HOST_IP                    */
                             QCM_SQL_INT32_FMT", "     /* PORT_NO                    */
                             "NULL, "                  /* ALTERNATE_HOST_IP          */
                             "NULL, "                  /* ALTERNATE_PORT_NO          */
                             QCM_SQL_VARCHAR_FMT", "   /* INTERNAL_HOST_IP           */
                             QCM_SQL_INT32_FMT", "     /* INTERNAL_PORT_NO           */
                             "NULL, "                  /* INTERNAL_ALTERNATE_HOST_IP */
                             "NULL, "                  /* INTERNAL_ALTERNATE_PORT_NO */
                             QCM_SQL_INT32_FMT", "     /* INTERNAL_CONN_TYPE         */
                             QCM_SQL_BIGINT_FMT" ) ",  /* SMN */
                             sNodeID,
                             aNodeName,
                             aHostIP,
                             aPortNo,
                             aInternalHostIP, // INTERNAL_HOST_IP
                             aInternalPortNo, // INTERNAL_PORT_NO
                             aConnType,
                             sMetaNodeInfo.mShardMetaNumber );
        }
    }

    IDE_TEST( qciMisc::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                     sSqlStr,
                                     & sRowCnt )
              != IDE_SUCCESS );

    if ( aRowCnt != NULL )
    {
        *aRowCnt = (UInt)sRowCnt;
    }
    else
    {
        /* Nothing to do */
    }

    sdi::setShardMetaTouched( aStatement->session );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

// delete node
IDE_RC sdm::deleteNode( qcStatement * aStatement,
                        SChar       * aNodeName,
                        UInt        * aRowCnt )
{
    SChar      * sSqlStr;
    vSLong       sRowCnt;

    sdiGlobalMetaInfo sMetaNodeInfo = { SDI_NULL_SMN };

    IDE_TEST( checkMetaVersion( QC_SMI_STMT( aStatement ) )
              != IDE_SUCCESS );


    IDE_TEST( makeShardMeta4NewSMN(aStatement) != IDE_SUCCESS );

    IDE_TEST( getGlobalMetaInfoCore( QC_SMI_STMT(aStatement),
                                         &sMetaNodeInfo ) != IDE_SUCCESS );

    IDE_TEST( STRUCT_ALLOC_WITH_SIZE( aStatement->qmxMem,
                                      SChar,
                                      QD_MAX_SQL_LENGTH,
                                      &sSqlStr )
              != IDE_SUCCESS);
    
    // BUG-46294
    IDE_TEST(checkReferenceObj( aStatement,
                                aNodeName,
                                sMetaNodeInfo.mShardMetaNumber )
             != IDE_SUCCESS);
      
    idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                     "DELETE FROM SYS_SHARD.NODES_ WHERE NODE_NAME = "QCM_SQL_VARCHAR_FMT" "
                     "AND SMN = "QCM_SQL_BIGINT_FMT,
                     aNodeName,
                     sMetaNodeInfo.mShardMetaNumber);

    IDE_TEST( qciMisc::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                     sSqlStr,
                                     & sRowCnt )
              != IDE_SUCCESS );

    if ( aRowCnt != NULL )
    {
        *aRowCnt = (UInt)sRowCnt;
    }
    else
    {
        /* Nothing to do */
    }

    sdi::setShardMetaTouched( aStatement->session );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdm::updateNodeInternal( qcStatement * aStatement,
                                SChar       * aNodeName,
                                SChar       * aHostIP,
                                UInt          aPortNo,
                                SChar       * aAlternateHostIP,
                                UInt          aAlternatePortNo,
                                UInt          aConnType,
                                UInt        * aRowCnt )
{
    SChar      * sSqlStr   = NULL;
    vSLong       sRowCnt   = 0;
    sdiGlobalMetaInfo sMetaNodeInfo = { SDI_NULL_SMN };

    IDE_TEST( checkMetaVersion( QC_SMI_STMT( aStatement ) )
              != IDE_SUCCESS );

    IDE_TEST( makeShardMeta4NewSMN(aStatement) != IDE_SUCCESS );

    IDE_TEST( getGlobalMetaInfoCore( QC_SMI_STMT(aStatement),
                                         &sMetaNodeInfo ) != IDE_SUCCESS );

    IDE_TEST( STRUCT_ALLOC_WITH_SIZE( aStatement->qmxMem,
                                      SChar,
                                      QD_MAX_SQL_LENGTH,
                                      &sSqlStr )
              != IDE_SUCCESS );

    if ( ( aAlternateHostIP[0] == '\0' ) || ( aAlternatePortNo == 0 ) )
    {
        idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                         "UPDATE SYS_SHARD.NODES_ SET "
                         "INTERNAL_HOST_IP = "QCM_SQL_VARCHAR_FMT", "
                         "INTERNAL_PORT_NO = "QCM_SQL_INT32_FMT", "
                         "INTERNAL_ALTERNATE_HOST_IP = NULL, "
                         "INTERNAL_ALTERNATE_PORT_NO = NULL, "
                         "INTERNAL_CONN_TYPE = "QCM_SQL_INT32_FMT" "
                         "WHERE NODE_NAME = "QCM_SQL_VARCHAR_FMT" "
                         "AND SMN = "QCM_SQL_BIGINT_FMT,
                         aHostIP,
                         aPortNo,
                         aConnType,
                         aNodeName,
                         sMetaNodeInfo.mShardMetaNumber );
    }
    else
    {
        idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                         "UPDATE SYS_SHARD.NODES_ SET "
                         "INTERNAL_HOST_IP = "QCM_SQL_VARCHAR_FMT", "
                         "INTERNAL_PORT_NO = "QCM_SQL_INT32_FMT", "
                         "INTERNAL_ALTERNATE_HOST_IP = "QCM_SQL_VARCHAR_FMT", "
                         "INTERNAL_ALTERNATE_PORT_NO = "QCM_SQL_INT32_FMT", "
                         "INTERNAL_CONN_TYPE = "QCM_SQL_INT32_FMT" "
                         "WHERE NODE_NAME = "QCM_SQL_VARCHAR_FMT" "
                         "AND SMN = "QCM_SQL_BIGINT_FMT,
                         aHostIP,
                         aPortNo,
                         aAlternateHostIP,
                         aAlternatePortNo,
                         aConnType,
                         aNodeName,
                         sMetaNodeInfo.mShardMetaNumber );
    }

    IDE_TEST( qciMisc::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                     sSqlStr,
                                     & sRowCnt )
              != IDE_SUCCESS );

    if ( aRowCnt != NULL )
    {
        *aRowCnt = (UInt)sRowCnt;
    }
    else
    {
        /* Nothing to do */
    }

    sdi::setShardMetaTouched( aStatement->session );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdm::updateNodeExternal( qcStatement * aStatement,
                                SChar       * aNodeName,
                                SChar       * aHostIP,
                                UInt          aPortNo,
                                SChar       * aAlternateHostIP,
                                UInt          aAlternatePortNo,
                                UInt        * aRowCnt )
{
    SChar      * sSqlStr   = NULL;
    vSLong       sRowCnt   = 0;
    sdiGlobalMetaInfo sMetaNodeInfo = { SDI_NULL_SMN };

    IDE_TEST( checkMetaVersion( QC_SMI_STMT( aStatement ) )
              != IDE_SUCCESS );

    IDE_TEST( makeShardMeta4NewSMN(aStatement) != IDE_SUCCESS );

    IDE_TEST( getGlobalMetaInfoCore( QC_SMI_STMT(aStatement),
                                         &sMetaNodeInfo ) != IDE_SUCCESS );

    IDE_TEST( STRUCT_ALLOC_WITH_SIZE( aStatement->qmxMem,
                                      SChar,
                                      QD_MAX_SQL_LENGTH,
                                      &sSqlStr )
              != IDE_SUCCESS );

    if ( ( aAlternateHostIP[0] == '\0' ) || ( aAlternatePortNo == 0 ) )
    {
        idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                         "UPDATE SYS_SHARD.NODES_ SET "
                         "HOST_IP = "QCM_SQL_VARCHAR_FMT", "
                         "PORT_NO = "QCM_SQL_INT32_FMT", "
                         "ALTERNATE_HOST_IP = NULL, "
                         "ALTERNATE_PORT_NO = NULL "
                         "WHERE NODE_NAME = "QCM_SQL_VARCHAR_FMT" "
                         "AND SMN = "QCM_SQL_BIGINT_FMT,
                         aHostIP,
                         aPortNo,
                         aNodeName,
                         sMetaNodeInfo.mShardMetaNumber );
    }
    else
    {
        idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                         "UPDATE SYS_SHARD.NODES_ SET "
                         "HOST_IP = "QCM_SQL_VARCHAR_FMT", "
                         "PORT_NO = "QCM_SQL_INT32_FMT", "
                         "ALTERNATE_HOST_IP = "QCM_SQL_VARCHAR_FMT", "
                         "ALTERNATE_PORT_NO = "QCM_SQL_INT32_FMT" "
                         "WHERE NODE_NAME = "QCM_SQL_VARCHAR_FMT" "
                         "AND SMN = "QCM_SQL_BIGINT_FMT,
                         aHostIP,
                         aPortNo,
                         aAlternateHostIP,
                         aAlternatePortNo,
                         aNodeName,
                         sMetaNodeInfo.mShardMetaNumber );
    }

    IDE_TEST( qciMisc::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                     sSqlStr,
                                     & sRowCnt )
              != IDE_SUCCESS );

    if ( aRowCnt != NULL )
    {
        *aRowCnt = (UInt)sRowCnt;
    }
    else
    {
        /* Nothing to do */
    }

    sdi::setShardMetaTouched( aStatement->session );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

// insert replica set
IDE_RC sdm::insertReplicaSet( qcStatement * aStatement,
                              UInt          aNodeID,
                              SChar       * aPrimaryNodeName,
                              UInt        * aRowCnt )
{
    SChar       sFirstBackupNodeNameBuf[SDI_NODE_NAME_MAX_SIZE+1];
    SChar     * sFirstBackupNodeName;
    SChar       sSecondBackupNodeNameBuf[SDI_NODE_NAME_MAX_SIZE+1];
    SChar     * sSecondBackupNodeName;
    SChar     * sFirstReplName;
    SChar       sFirstReplNameBuf[SDI_REPLICATION_NAME_MAX_SIZE+1];
    SChar     * sFirstReplFromNodeName;
    SChar     * sFirstReplToNodeName;
    SChar     * sSecondReplName;
    SChar       sSecondReplNameBuf[SDI_REPLICATION_NAME_MAX_SIZE+1];
    SChar     * sSecondReplFromNodeName;
    SChar     * sSecondReplToNodeName;
    SChar     * sSqlStr;
    vSLong      sRowCnt;
    UInt        sReplicaSetID = aNodeID;

    sdiGlobalMetaInfo sMetaNodeInfo = { SDI_NULL_SMN };

    IDE_TEST( checkMetaVersion( QC_SMI_STMT( aStatement ) )
              != IDE_SUCCESS );

    IDE_TEST( makeShardMeta4NewSMN(aStatement) != IDE_SUCCESS );

    IDE_TEST( getGlobalMetaInfoCore( QC_SMI_STMT(aStatement),
                                     &sMetaNodeInfo ) != IDE_SUCCESS );

    IDE_TEST( STRUCT_ALLOC_WITH_SIZE( aStatement->qmxMem,
                                      SChar,
                                      QD_MAX_SQL_LENGTH,
                                      &sSqlStr )
              != IDE_SUCCESS);

    if( getNthBackupNodeName(QC_SMI_STMT( aStatement ),
                             aPrimaryNodeName,
                             sFirstBackupNodeNameBuf,
                             0,
                             sMetaNodeInfo.mShardMetaNumber) == IDE_SUCCESS )
    {
        /* set first backup replication information */
        sFirstBackupNodeName = sFirstBackupNodeNameBuf;
        sFirstReplFromNodeName = aPrimaryNodeName;
        sFirstReplToNodeName = sFirstBackupNodeNameBuf;
    }
    else
    {
        /* first backup을 복제 할 수 있는 충분한 수의 node가 아직 등록되지 않았음 */
        sFirstBackupNodeName = SDM_NA_STR;
        sFirstReplFromNodeName = SDM_NA_STR;
        sFirstReplToNodeName = SDM_NA_STR;
    }

    sdi::makeReplName(sReplicaSetID, SDI_BACKUP_1, sFirstReplNameBuf);
    sFirstReplName = sFirstReplNameBuf;

    if( getNthBackupNodeName(QC_SMI_STMT( aStatement ),
                             aPrimaryNodeName,
                             sSecondBackupNodeNameBuf,
                             1,
                             sMetaNodeInfo.mShardMetaNumber) == IDE_SUCCESS )
    {
        /* set second backup replication information */
        sSecondBackupNodeName = sSecondBackupNodeNameBuf;
        sSecondReplFromNodeName = aPrimaryNodeName;
        sSecondReplToNodeName = sSecondBackupNodeNameBuf;
    }
    else
    {
        /* second backup을 복제 할 수 있는 충분한 수의 node가 아직 등록되지 않았음 */
        sSecondBackupNodeName = SDM_NA_STR;
        sSecondReplFromNodeName = SDM_NA_STR;
        sSecondReplToNodeName = SDM_NA_STR;
    }

    sdi::makeReplName(sReplicaSetID, SDI_BACKUP_2, sSecondReplNameBuf);
    sSecondReplName = sSecondReplNameBuf;

    idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                     "INSERT INTO SYS_SHARD.REPLICA_SETS_ "
                     "VALUES ( "
                     QCM_SQL_INT32_FMT", "     /* REPLICA_SET_ID               */
                     QCM_SQL_VARCHAR_FMT", "   /* PRIMARY_NODE_NAME            */
                     QCM_SQL_VARCHAR_FMT", "   /* FIRST_BACKUP_NODE_NAME       */
                     QCM_SQL_VARCHAR_FMT", "   /* SECOND_BACKUP_NODE_NAME      */
                     QCM_SQL_VARCHAR_FMT", "   /* STOP_FIRST_BACKUP_NODE_NAME  */
                     QCM_SQL_VARCHAR_FMT", "   /* STOP_SECOND_BACKUP_NODE_NAME */
                     QCM_SQL_VARCHAR_FMT", "   /* FIRST_REPLICATION_NAME       */
                     QCM_SQL_VARCHAR_FMT", "   /* FIRST_REPL_FROM_NODE_NAME    */
                     QCM_SQL_VARCHAR_FMT", "   /* FIRST_REPL_TO_NODE_NAME      */
                     QCM_SQL_VARCHAR_FMT", "   /* SECOND_REPLICATION_NAME      */
                     QCM_SQL_VARCHAR_FMT", "   /* SECOND_REPL_FROM_NODE_NAME   */
                     QCM_SQL_VARCHAR_FMT", "   /* SECOND_REPL_TO_NODE_NAME     */
                     QCM_SQL_BIGINT_FMT" ) ",  /* SMN */
                     sReplicaSetID,
                     aPrimaryNodeName,
                     sFirstBackupNodeName,
                     sSecondBackupNodeName,
                     SDM_NA_STR,
                     SDM_NA_STR,
                     sFirstReplName,
                     sFirstReplFromNodeName,
                     sFirstReplToNodeName,
                     sSecondReplName,
                     sSecondReplFromNodeName,
                     sSecondReplToNodeName,
                     sMetaNodeInfo.mShardMetaNumber );

    IDE_TEST( qciMisc::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                     sSqlStr,
                                     & sRowCnt )
              != IDE_SUCCESS );

    if ( aRowCnt != NULL )
    {
        *aRowCnt = (UInt)sRowCnt;
    }
    else
    {
        /* Nothing to do */
    }

    sdi::setShardMetaTouched( aStatement->session );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

// replica sets reorganization
IDE_RC sdm::reorganizeReplicaSet( qcStatement * aStatement,
                                  SChar       * /*aPrimaryNodeName*/,
                                  UInt        * aRowCnt )
{
    SInt        i = 0;

    SChar     * sSqlStr;
    vSLong      sRowCnt = 0;
    sdiNodeInfo  sNodeInfo;
    sdiReplicaSetInfo sReplicaSetInfo;
    SInt s1stBackupIdx = 0;
    SInt s2ndBackupIdx = 0;
    SChar * s1stBackupNodeName = NULL;
    SChar * s2ndBackupNodeName = NULL;
    idBool sIsUpdated = ID_FALSE;
    sdiLocalMetaInfo sLocalMetaInfo;

    sdiGlobalMetaInfo sMetaNodeInfo = { SDI_NULL_SMN };

    IDE_TEST( checkMetaVersion( QC_SMI_STMT( aStatement ) )
              != IDE_SUCCESS );

    IDE_TEST( makeShardMeta4NewSMN(aStatement) != IDE_SUCCESS );

    IDE_TEST( getGlobalMetaInfoCore( QC_SMI_STMT(aStatement),
                                     &sMetaNodeInfo ) != IDE_SUCCESS );

    IDE_TEST( STRUCT_ALLOC_WITH_SIZE( aStatement->qmxMem,
                                      SChar,
                                      QD_MAX_SQL_LENGTH,
                                      &sSqlStr )
              != IDE_SUCCESS);


    IDE_TEST( getInternalNodeInfoSortedByName( QC_SMI_STMT( aStatement ),
                                               &sNodeInfo,
                                               sMetaNodeInfo.mShardMetaNumber,
                                               SDM_SORT_ASC) != IDE_SUCCESS);

    IDE_TEST( getAllReplicaSetInfoSortedByPName( QC_SMI_STMT( aStatement ),
                                                 &sReplicaSetInfo,
                                                 sMetaNodeInfo.mShardMetaNumber )
              != IDE_SUCCESS);

    //IDE_TEST_RAISE(sNodeInfo->mCount != sReplicaSetInfo->mCount, ERR_COUNT_MISMATCH);
    IDE_TEST( sdm::getLocalMetaInfo( &sLocalMetaInfo ) != IDE_SUCCESS );
    /*
     * 현재는 nodes_와 replica_sets_이 동일한 개수로 동일한 위치에 존재해야하므로 아래와 같이 처리 가능하다.
     * 향후에 failover/failback이 발생하면 로직이 변경되어야 한다.
     */
    for( i = 0; i < sNodeInfo.mCount; i++ )
    {
        s1stBackupIdx = sdi::getBackupNodeIdx(i, sNodeInfo.mCount, SDI_BACKUP_1ST);
        s2ndBackupIdx = sdi::getBackupNodeIdx(i, sNodeInfo.mCount, SDI_BACKUP_2ND);

        if ( s1stBackupIdx != SDI_NODE_NULL_IDX )
        {
            if ( sLocalMetaInfo.mKSafety >= 1 )
            {
                s1stBackupNodeName = sNodeInfo.mNodes[s1stBackupIdx].mNodeName;
            }
            else
            {
                s1stBackupNodeName = SDM_NA_STR;
            }
        }
        else
        {
            s1stBackupNodeName = SDM_NA_STR;
        }

        if ( s2ndBackupIdx != SDI_NODE_NULL_IDX )
        {
            if ( sLocalMetaInfo.mKSafety >= 2 )
            {
                s2ndBackupNodeName = sNodeInfo.mNodes[s2ndBackupIdx].mNodeName;
            }
            else
            {
                s2ndBackupNodeName = SDM_NA_STR;
            }
        }
        else
        {
            s2ndBackupNodeName = SDM_NA_STR;
        }

        IDE_TEST(checkAndUpdateBackupInfo(aStatement,
                                          sNodeInfo.mNodes[i].mNodeName,
                                          s1stBackupNodeName,
                                          s2ndBackupNodeName,
                                          &(sReplicaSetInfo.mReplicaSets[i]),
                                          &sIsUpdated)
                 != IDE_SUCCESS);
        if ( sIsUpdated == ID_TRUE )
        {
            sdi::setShardMetaTouched( aStatement->session );
            sRowCnt++;
        }
    }

    *aRowCnt = sRowCnt;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdm::checkAndUpdateBackupInfo( qcStatement * aStatement,
                                      SChar * aNewPrimaryNodeName,
                                      SChar * aNew1stBackupNodeName,
                                      SChar * aNew2ndBackupNodeName,
                                      sdiReplicaSet * aOldReplicaSet,
                                      idBool         * aOutIsUpdated )
{
    SChar sSqlStr[4096];
    SChar sErrBuf[128] = {0,};
    //SChar s1stRepName[SDI_REPLICATION_NAME_MAX_SIZE] = {0,};
    //SChar s2ndRepName[SDI_REPLICATION_NAME_MAX_SIZE] = {0,};
    vSLong  sRowCnt = 0;

    /*
    sdi::makeReplName(aOldReplicaSet->mReplicaSetId, 1, s1stRepName);
    sdi::makeReplName(aOldReplicaSet->mReplicaSetId, 2, s2ndRepName);
    */
    IDE_TEST_RAISE ( idlOS::strncmp(aNewPrimaryNodeName,
                        aOldReplicaSet->mPrimaryNodeName,
                        SDI_NODE_NAME_MAX_SIZE + 1) != 0, ERR_PRIMARY_NODE_NAME );

    if ( ( idlOS::strncmp( aNew1stBackupNodeName,
                           aOldReplicaSet->mFirstBackupNodeName,
                           SDI_NODE_NAME_MAX_SIZE + 1 ) != 0 ) ||
         ( idlOS::strncmp( aNew2ndBackupNodeName,
                           aOldReplicaSet->mSecondBackupNodeName,
                           SDI_NODE_NAME_MAX_SIZE + 1 ) != 0 ) )
    {
        idlOS::snprintf( sSqlStr, 4096,
                         "UPDATE SYS_SHARD.REPLICA_SETS_ SET "
                         "  FIRST_BACKUP_NODE_NAME = "QCM_SQL_VARCHAR_FMT", "
                         "  SECOND_BACKUP_NODE_NAME = "QCM_SQL_VARCHAR_FMT", "
                         "  STOP_FIRST_BACKUP_NODE_NAME = "QCM_SQL_VARCHAR_FMT", "
                         "  STOP_SECOND_BACKUP_NODE_NAME = "QCM_SQL_VARCHAR_FMT", "                         
                         "  FIRST_REPL_FROM_NODE_NAME = "QCM_SQL_VARCHAR_FMT", "
                         "  FIRST_REPL_TO_NODE_NAME = "QCM_SQL_VARCHAR_FMT", "
                         "  SECOND_REPL_FROM_NODE_NAME = "QCM_SQL_VARCHAR_FMT", "
                         "  SECOND_REPL_TO_NODE_NAME = "QCM_SQL_VARCHAR_FMT" "
                         " WHERE REPLICA_SET_ID = "QCM_SQL_INT32_FMT" AND "
                         "       SMN = "QCM_SQL_INT32_FMT,
                         aNew1stBackupNodeName,
                         aNew2ndBackupNodeName,
                         SDM_NA_STR,
                         SDM_NA_STR,                         
                         aNewPrimaryNodeName,
                         aNew1stBackupNodeName,
                         aNewPrimaryNodeName,
                         aNew2ndBackupNodeName,
                         aOldReplicaSet->mReplicaSetId,
                         aOldReplicaSet->mSMN);

        IDE_TEST( qciMisc::runDMLforDDL( QC_SMI_STMT( aStatement ) ,
                                         sSqlStr,
                                         &sRowCnt )
                  != IDE_SUCCESS );

        IDE_TEST_RAISE( sRowCnt != 1, ERR_ROW_CNT );
        *aOutIsUpdated = ID_TRUE;
    }
    else
    {
        *aOutIsUpdated = ID_FALSE;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_PRIMARY_NODE_NAME )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR , "sdm::checkAndUpdateBackupInfo", "node and replica primary node name mismatch") );
    }
    IDE_EXCEPTION( ERR_ROW_CNT )
    {
        idlOS::snprintf(sErrBuf, 128,"updated count is %"ID_INT32_FMT, sRowCnt);
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR , "sdm::checkAndUpdateBackupInfo", sErrBuf) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdm::checkReferenceObj( qcStatement * aStatement,
                               SChar       * aNodeName,
                               ULong         aSMN )
{
    SChar sSqlStr[4096];
    idBool        sRecordExist = ID_FALSE;
    mtdBigintType sResultCount = 0;

    idlOS::snprintf( sSqlStr, 4096,
                     "SELECT NODE_ID "
                     "  FROM SYS_SHARD.NODES_ "
                     " WHERE NODE_NAME = "QCM_SQL_VARCHAR_FMT
                     "   AND SMN = "QCM_SQL_BIGINT_FMT
                     "   AND NODE_ID IN ( SELECT DEFAULT_NODE_ID FROM SYS_SHARD.OBJECTS_ WHERE SMN = "QCM_SQL_BIGINT_FMT
                     "          UNION ALL SELECT NODE_ID FROM SYS_SHARD.RANGES_ WHERE SMN = "QCM_SQL_BIGINT_FMT
                     "          UNION ALL SELECT NODE_ID FROM SYS_SHARD.CLONES_ WHERE SMN = "QCM_SQL_BIGINT_FMT
                     "          UNION ALL SELECT NODE_ID FROM SYS_SHARD.SOLOS_ WHERE SMN = "QCM_SQL_BIGINT_FMT" )",
                     aNodeName,
                     aSMN,
                     aSMN,
                     aSMN,
                     aSMN,
                     aSMN );

    IDE_TEST( qcg::runSelectOneRowforDDL( QC_SMI_STMT( aStatement ),
                                          sSqlStr,
                                          &sResultCount,
                                          &sRecordExist,
                                          ID_FALSE )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( sRecordExist == ID_TRUE, ERR_EXIST_REFERENCES_NODE )

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_EXIST_REFERENCES_NODE )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDM_EXIST_REFERENCES_NODE ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

// delete replica set
IDE_RC sdm::deleteReplicaSet( qcStatement * aStatement,
                              SChar       * aPrimaryNodeName,
                              idBool        aIsForce,
                              UInt        * aRowCnt )
{
    SChar      * sSqlStr;
    vSLong       sRowCnt;

    sdiGlobalMetaInfo sMetaNodeInfo = { SDI_NULL_SMN };

    IDE_TEST( checkMetaVersion( QC_SMI_STMT( aStatement ) )
              != IDE_SUCCESS );


    IDE_TEST( makeShardMeta4NewSMN(aStatement) != IDE_SUCCESS );

    IDE_TEST( getGlobalMetaInfoCore( QC_SMI_STMT(aStatement),
                                         &sMetaNodeInfo ) != IDE_SUCCESS );

    IDE_TEST( STRUCT_ALLOC_WITH_SIZE( aStatement->qmxMem,
                                      SChar,
                                      QD_MAX_SQL_LENGTH,
                                      &sSqlStr )
              != IDE_SUCCESS);

    if ( aIsForce != ID_TRUE )
    {
        IDE_TEST( checkReferenceObj( aStatement,
                                     aPrimaryNodeName,
                                     sMetaNodeInfo.mShardMetaNumber )
                  != IDE_SUCCESS );
    }

    idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                     "DELETE FROM SYS_SHARD.REPLICA_SETS_ WHERE PRIMARY_NODE_NAME = "QCM_SQL_VARCHAR_FMT" "
                     "AND SMN = "QCM_SQL_BIGINT_FMT,
                     aPrimaryNodeName,
                     sMetaNodeInfo.mShardMetaNumber);

    IDE_TEST( qciMisc::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                     sSqlStr,
                                     & sRowCnt )
              != IDE_SUCCESS );

    if ( aRowCnt != NULL )
    {
        *aRowCnt = (UInt)sRowCnt;
    }
    else
    {
        /* Nothing to do */
    }

    sdi::setShardMetaTouched( aStatement->session );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdm::updateLocalMetaInfo( qcStatement  * aStatement,
                                 sdiShardNodeId aShardNodeID,
                                 SChar        * aNodeName,
                                 SChar        * aHostIP,
                                 UInt           aPortNo,
                                 SChar        * aInternalHostIP,
                                 UInt           aInternalPortNo,
                                 SChar        * aInternalReplHostIP,
                                 UInt           aInternalReplPortNo,
                                 UInt           aInternalConnType,
                                 UInt         * aRowCnt )
{
    SChar      * sSqlStr   = NULL;
    vSLong       sRowCnt   = 0;

    IDE_TEST( checkMetaVersion( QC_SMI_STMT( aStatement ) )
              != IDE_SUCCESS );

    IDE_TEST( STRUCT_ALLOC_WITH_SIZE( aStatement->qmxMem,
                                      SChar,
                                      QD_MAX_SQL_LENGTH,
                                      &sSqlStr )
              != IDE_SUCCESS );

    idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                     "UPDATE SYS_SHARD.LOCAL_META_INFO_ SET "
                     "SHARD_NODE_ID = "QCM_SQL_INT32_FMT", "
                     "NODE_NAME = "QCM_SQL_VARCHAR_FMT", "
                     "HOST_IP = "QCM_SQL_VARCHAR_FMT", "
                     "PORT_NO = "QCM_SQL_INT32_FMT", "
                     "INTERNAL_HOST_IP = "QCM_SQL_VARCHAR_FMT", "
                     "INTERNAL_PORT_NO = "QCM_SQL_INT32_FMT", "
                     "INTERNAL_REPLICATION_HOST_IP = "QCM_SQL_VARCHAR_FMT", "
                     "INTERNAL_REPLICATION_PORT_NO = "QCM_SQL_INT32_FMT", "
                     "INTERNAL_CONN_TYPE = "QCM_SQL_INT32_FMT" "
                     "WHERE SHARDED_DB_NAME = "QCM_SQL_VARCHAR_FMT,
                     aShardNodeID,
                     aNodeName,
                     aHostIP,
                     aPortNo,
                     aInternalHostIP,
                     aInternalPortNo,
                     aInternalReplHostIP,
                     aInternalReplPortNo,
                     aInternalConnType,
                     smiGetDBName());

    IDE_TEST( qciMisc::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                     sSqlStr,
                                     & sRowCnt )
              != IDE_SUCCESS );

    if ( aRowCnt != NULL )
    {
        *aRowCnt = (UInt)sRowCnt;
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdm::updateLocalMetaInfoForReplication( qcStatement  * aStatement,
                                               UInt           aKSafety,
                                               SChar        * aReplicationMode,
                                               UInt           aParallelCount,
                                               UInt         * aRowCnt )
{
    SChar      * sSqlStr   = NULL;
    vSLong       sRowCnt   = 0;
    UInt         sReplicationMode = 0;

    IDE_DASSERT(aReplicationMode != NULL);
    IDE_TEST( checkMetaVersion( QC_SMI_STMT( aStatement ) )
              != IDE_SUCCESS );

    IDE_TEST( STRUCT_ALLOC_WITH_SIZE( aStatement->qmxMem,
                                      SChar,
                                      QD_MAX_SQL_LENGTH,
                                      &sSqlStr )
              != IDE_SUCCESS );

    if ( idlOS::strncmp(SDI_REP_MODE_CONSISTENT_STR,
                        (const char*)aReplicationMode,
                        SDI_REPLICATION_MODE_MAX_SIZE) == 0 )
    {
        sReplicationMode = SDI_REP_MODE_CONSISTENT_CODE;
    }
    else if ( idlOS::strncmp(SDI_REP_MODE_NOWAIT_STR,
                           (const char*)aReplicationMode,
                           SDI_REPLICATION_MODE_MAX_SIZE) == 0 )
    {
        sReplicationMode = SDI_REP_MODE_NOWAIT_CODE;
    }
    else if ( idlOS::strncmp(SDI_REP_MODE_NULL_STR,
                           (const char*)aReplicationMode,
                           SDI_REPLICATION_MODE_MAX_SIZE) == 0 )
    {
        sReplicationMode = SDI_REP_MODE_NULL_CODE;
    }
    else
    {
        IDE_RAISE(ERR_REP_MODE);
    }

    idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                     "UPDATE SYS_SHARD.LOCAL_META_INFO_ SET "
                     "K_SAFETY = "QCM_SQL_INT32_FMT", "
                     "REPLICATION_MODE = "QCM_SQL_INT32_FMT", "
                     "PARALLEL_COUNT = "QCM_SQL_INT32_FMT" "
                     "WHERE SHARDED_DB_NAME = "QCM_SQL_VARCHAR_FMT,
                     aKSafety,
                     sReplicationMode,
                     aParallelCount,
                     smiGetDBName());

    IDE_TEST( qciMisc::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                     sSqlStr,
                                     & sRowCnt )
              != IDE_SUCCESS );

    if ( aRowCnt != NULL )
    {
        *aRowCnt = (UInt)sRowCnt;
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_REP_MODE );
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDF_SHARD_REP_ARG_WRONG, aReplicationMode ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* insert shard procedure */
IDE_RC sdm::insertProcedure( qcStatement * aStatement,
                             SChar       * aUserName,
                             SChar       * aProcName,
                             SChar       * aSplitMethod,
                             SChar       * aKeyParamName,
                             SChar       * aSubSplitMethod,
                             SChar       * aSubKeyParamName,
                             SChar       * aDefaultNodeName,
                             UInt        * aRowCnt,
                             qsOID         aProcOID,
                             UInt          /*aUserID*/ )
{
    SChar         * sSqlStr;
    vSLong          sRowCnt;
    UInt            sShardID;
    sdiTableInfo    sTableInfo;
    sdiNode         sNode;
    idBool          sIsTableFound = ID_FALSE;
    sdiGlobalMetaInfo sMetaNodeInfo = { SDI_NULL_SMN };

    sdiReplicaSetInfo sReplicaSetInfo;
    UInt              sReplicaSetId = SDI_REPLICASET_NULL_ID;

    IDE_TEST( checkMetaVersion( QC_SMI_STMT( aStatement ) )
              != IDE_SUCCESS );

    IDE_TEST( makeShardMeta4NewSMN(aStatement) != IDE_SUCCESS );

    IDE_TEST( getGlobalMetaInfoCore( QC_SMI_STMT(aStatement),
                                         &sMetaNodeInfo ) != IDE_SUCCESS );

    IDE_TEST( getTableInfo( QC_SMI_STMT( aStatement ),
                            aUserName,
                            aProcName,
                            sMetaNodeInfo.mShardMetaNumber,
                            &sTableInfo,
                            &sIsTableFound )
              != IDE_SUCCESS );

    // 이미 동일한 테이블이 존재한다면 에러처리한다.
    IDE_TEST_RAISE( sIsTableFound == ID_TRUE,
                    ERR_EXIST_SHARD_OBJECT );

    // BUG-46301
    IDE_TEST( validateParamBeforeInsert( aProcOID,
                                         aUserName,
                                         aProcName,
                                         aSplitMethod,
                                         aKeyParamName,
                                         aSubSplitMethod,
                                         aSubKeyParamName )
              != IDE_SUCCESS );

    IDE_TEST( STRUCT_ALLOC_WITH_SIZE( aStatement->qmxMem,
                                      SChar,
                                      QD_MAX_SQL_LENGTH,
                                      &sSqlStr )
              != IDE_SUCCESS );

    IDE_TEST( getNextShardID( aStatement,
                              &sShardID,
                              sMetaNodeInfo.mShardMetaNumber )
              != IDE_SUCCESS );

    if ( aSplitMethod[0] != 'C' )
    {
        if ( aDefaultNodeName[0] != '\0' )
        {
            IDE_TEST( getNodeByName( QC_SMI_STMT( aStatement ),
                                     aDefaultNodeName,
                                     sMetaNodeInfo.mShardMetaNumber,
                                     &sNode )
                      != IDE_SUCCESS );

            IDE_TEST( sdm::getReplicaSetsByPName( QC_SMI_STMT( aStatement),
                                                  sNode.mNodeName,
                                                  sMetaNodeInfo.mShardMetaNumber,
                                                  &sReplicaSetInfo )
                      != IDE_SUCCESS );

            /* Shard Object 추가는 모든 Node가 Valid 한 상황에서만 호출된다.
             * ReplicaSet의 PrimaryNodeName와 지정된 Node가 일치하는 경우가 항상 1개 있어야 한다. */
            IDE_TEST_RAISE( sReplicaSetInfo.mCount != 1, ERR_SHARD_META );

            sReplicaSetId = sReplicaSetInfo.mReplicaSets[0].mReplicaSetId;

            idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                             "INSERT INTO SYS_SHARD.OBJECTS_ VALUES( "
                             QCM_SQL_INT32_FMT", "
                             QCM_SQL_VARCHAR_FMT", "
                             QCM_SQL_VARCHAR_FMT", "
                             "'P', "
                             QCM_SQL_VARCHAR_FMT", "
                             QCM_SQL_VARCHAR_FMT", "
                             QCM_SQL_VARCHAR_FMT", "
                             QCM_SQL_VARCHAR_FMT", "
                             QCM_SQL_INT32_FMT", "
                             "NULL, " // default partition name
                             QCM_SQL_INT32_FMT", "
                             QCM_SQL_BIGINT_FMT" )",
                             sShardID,
                             aUserName,
                             aProcName,
                             aSplitMethod,
                             aKeyParamName,
                             aSubSplitMethod,
                             aSubKeyParamName,
                             sNode.mNodeId,
                             sReplicaSetId, /* DefaultNode와 같은 ReplicaSet을 가진다. */
                             sMetaNodeInfo.mShardMetaNumber);
        }
        else
        {
            idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                             "INSERT INTO SYS_SHARD.OBJECTS_ VALUES( "
                             QCM_SQL_INT32_FMT", "
                             QCM_SQL_VARCHAR_FMT", "
                             QCM_SQL_VARCHAR_FMT", "
                             "'P', "
                             QCM_SQL_VARCHAR_FMT", "
                             QCM_SQL_VARCHAR_FMT", "
                             QCM_SQL_VARCHAR_FMT", "
                             QCM_SQL_VARCHAR_FMT", "
                             "NULL, " // default node id
                             "NULL, " // default partition name
                             QCM_SQL_INT32_FMT", "
                             QCM_SQL_BIGINT_FMT" )",
                             sShardID,
                             aUserName,
                             aProcName,
                             aSplitMethod,
                             aKeyParamName,
                             aSubSplitMethod,
                             aSubKeyParamName,
                             sReplicaSetId, /* SDI_REPLICASET_NULL_ID = 0 */
                             sMetaNodeInfo.mShardMetaNumber);
        }
    }
    else
    {
        // Clone table의 경우 shard key column이 없기 때문에,
        // key column name을 NULL로 입력한다.
        idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                         "INSERT INTO SYS_SHARD.OBJECTS_ VALUES( "
                         QCM_SQL_INT32_FMT", "
                         QCM_SQL_VARCHAR_FMT", "
                         QCM_SQL_VARCHAR_FMT", "
                         "'P', "
                         QCM_SQL_VARCHAR_FMT", "
                         "NULL, "
                         "NULL, " // sub split method
                         "NULL, " // sub shard key
                         "NULL, " // default node id
                         "NULL, " // default partition name
                         QCM_SQL_INT32_FMT", "
                         QCM_SQL_BIGINT_FMT")",
                         sShardID,
                         aUserName,
                         aProcName,
                         aSplitMethod,
                         sReplicaSetId, /* SDI_REPLICASET_NULL_ID = 0 */
                         sMetaNodeInfo.mShardMetaNumber);
    }

    IDE_TEST( qciMisc::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                     sSqlStr,
                                     & sRowCnt ) != IDE_SUCCESS );

    if ( aRowCnt != NULL )
    {
        *aRowCnt = (UInt)sRowCnt;
    }
    else
    {
        /* Nothing to do */
    }

    sdi::setShardMetaTouched( aStatement->session );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_SHARD_META )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                  "sdm::insertProcedure",
                                  "NodeName is not found in ReplicaSet" ) );
    }
    IDE_EXCEPTION( ERR_EXIST_SHARD_OBJECT )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDM_AREADY_EXIST_SHARD_OBJECT ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* insert shard table */
IDE_RC sdm::insertTable( qcStatement * aStatement,
                         SChar       * aUserName,
                         SChar       * aTableName,
                         SChar       * aSplitMethod,
                         SChar       * aKeyColumnName,
                         SChar       * aSubSplitMethod,
                         SChar       * aSubKeyColumnName,
                         SChar       * aDefaultNodeName,
                         SChar       * aDefaultPartitionName,
                         UInt        * aRowCnt )
{
    SChar         * sSqlStr;
    vSLong          sRowCnt;
    UInt            sShardID;
    UInt            sUserID;
    void          * sTableHandle;
    smSCN           sTableSCN;
    sdiTableInfo    sTableInfo;
    sdiNode         sNode;
    idBool          sIsTableFound = ID_FALSE;
    sdiGlobalMetaInfo sMetaNodeInfo = { SDI_NULL_SMN };

    sdiReplicaSetInfo sReplicaSetInfo;
    UInt              sReplicaSetId = SDI_REPLICASET_NULL_ID;

    IDE_TEST( checkMetaVersion( QC_SMI_STMT( aStatement ) )
              != IDE_SUCCESS );

    IDE_TEST( makeShardMeta4NewSMN(aStatement) != IDE_SUCCESS );

    IDE_TEST( getGlobalMetaInfoCore( QC_SMI_STMT(aStatement),
                                         &sMetaNodeInfo ) != IDE_SUCCESS );

    IDE_TEST( qcmUser::getUserID( NULL,
                                  QC_SMI_STMT( aStatement ),
                                  aUserName,
                                  idlOS::strlen( aUserName ),
                                  &sUserID )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( sUserID == QC_SYSTEM_USER_ID,
                    ERR_SYSTEM_OBJECT );

    IDE_TEST( qcm::getTableHandleByName( QC_SMI_STMT(aStatement),
                                         sUserID,
                                         (UChar*)aTableName,
                                         idlOS::strlen( aTableName ),
                                         &sTableHandle,
                                         &sTableSCN )
              != IDE_SUCCESS );

    IDE_TEST( getTableInfo( QC_SMI_STMT( aStatement ),
                            aUserName,
                            aTableName,
                            sMetaNodeInfo.mShardMetaNumber,
                            &sTableInfo,
                            &sIsTableFound )
              != IDE_SUCCESS );

    // 이미 동일한 테이블이 존재한다면 에러처리한다.
    IDE_TEST_RAISE( sIsTableFound == ID_TRUE,
                    ERR_EXIST_SHARD_OBJECT );

    // BUG-46301
    IDE_TEST( validateColumnBeforeInsert( aStatement,
                                          sUserID,
                                          aTableName,
                                          aSplitMethod,
                                          aKeyColumnName,
                                          aSubSplitMethod,
                                          aSubKeyColumnName )
              != IDE_SUCCESS );

    IDE_TEST( STRUCT_ALLOC_WITH_SIZE( aStatement->qmxMem,
                                      SChar,
                                      QD_MAX_SQL_LENGTH,
                                      &sSqlStr )
              != IDE_SUCCESS );

    IDE_TEST( getNextShardID( aStatement,
                              &sShardID,
                              sMetaNodeInfo.mShardMetaNumber )
              != IDE_SUCCESS );

    if ( ( aSplitMethod[0] != 'C' ) && ( aSplitMethod[0] != 'S' ) )
    {
        if ( aDefaultNodeName[0] != '\0' )
        {
            IDE_TEST( getNodeByName( QC_SMI_STMT( aStatement ),
                                     aDefaultNodeName,
                                     sMetaNodeInfo.mShardMetaNumber,
                                     &sNode )
                      != IDE_SUCCESS );

            IDE_TEST( sdm::getReplicaSetsByPName( QC_SMI_STMT( aStatement),
                                                  sNode.mNodeName,
                                                  sMetaNodeInfo.mShardMetaNumber,
                                                  &sReplicaSetInfo )
                      != IDE_SUCCESS );

            /* Shard Object 추가는 모든 Node가 Valid 한 상황에서만 호출된다.
             * ReplicaSet의 PrimaryNodeName와 지정된 Node가 일치하는 경우가 항상 1개 있어야 한다. */
            IDE_TEST_RAISE( sReplicaSetInfo.mCount != 1, ERR_SHARD_META );

            sReplicaSetId = sReplicaSetInfo.mReplicaSets[0].mReplicaSetId;

            idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                             "INSERT INTO SYS_SHARD.OBJECTS_ VALUES( "
                             QCM_SQL_INT32_FMT", "
                             QCM_SQL_VARCHAR_FMT", "
                             QCM_SQL_VARCHAR_FMT", "
                             "'T', "
                             QCM_SQL_VARCHAR_FMT", "
                             QCM_SQL_VARCHAR_FMT", "
                             QCM_SQL_VARCHAR_FMT", " // sub split method
                             QCM_SQL_VARCHAR_FMT", " // sub shard key
                             QCM_SQL_INT32_FMT", "   // default node id
                             QCM_SQL_VARCHAR_FMT", " // default partition name
                             QCM_SQL_INT32_FMT", "   // Default Partition ReplicaSetID
                             QCM_SQL_BIGINT_FMT" )",
                             sShardID,
                             aUserName,
                             aTableName,
                             aSplitMethod,
                             aKeyColumnName,
                             aSubSplitMethod,
                             aSubKeyColumnName,
                             sNode.mNodeId,
                             aDefaultPartitionName,
                             sReplicaSetId,
                             sMetaNodeInfo.mShardMetaNumber );
        }
        else
        {
            idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                             "INSERT INTO SYS_SHARD.OBJECTS_ VALUES( "
                             QCM_SQL_INT32_FMT", "
                             QCM_SQL_VARCHAR_FMT", "
                             QCM_SQL_VARCHAR_FMT", "
                             "'T', "
                             QCM_SQL_VARCHAR_FMT", "
                             QCM_SQL_VARCHAR_FMT", "
                             QCM_SQL_VARCHAR_FMT", " // sub split method
                             QCM_SQL_VARCHAR_FMT", " // sub shard key
                             "NULL, "                // default node id
                             QCM_SQL_VARCHAR_FMT", " // default partition name
                             QCM_SQL_INT32_FMT", "   // Default Partition ReplicaSetID
                             QCM_SQL_BIGINT_FMT")",
                             sShardID,
                             aUserName,
                             aTableName,
                             aSplitMethod,
                             aKeyColumnName,
                             aSubSplitMethod,
                             aSubKeyColumnName,
                             aDefaultPartitionName,
                             sReplicaSetId,
                             sMetaNodeInfo.mShardMetaNumber);
        }
    }
    else
    {
        // Clone, solo table의 경우 shard key column이 없기 때문에,
        // key column name을 NULL로 입력한다.
        idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                         "INSERT INTO SYS_SHARD.OBJECTS_ VALUES( "
                         QCM_SQL_INT32_FMT", "
                         QCM_SQL_VARCHAR_FMT", "
                         QCM_SQL_VARCHAR_FMT", "
                         "'T', "
                         QCM_SQL_VARCHAR_FMT", "
                         "NULL, "
                         "NULL, "                // sub split method
                         "NULL, "                // sub shard key
                         "NULL, "                // default node id
                         "NULL, "                // default partition name
                         QCM_SQL_INT32_FMT", "   // Default Partition ReplicaSetID
                         QCM_SQL_BIGINT_FMT")",
                         sShardID,
                         aUserName,
                         aTableName,
                         aSplitMethod,
                         sReplicaSetId,
                         sMetaNodeInfo.mShardMetaNumber);
    }

    IDE_TEST( qciMisc::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                     sSqlStr,
                                     & sRowCnt ) != IDE_SUCCESS );

    if ( aRowCnt != NULL )
    {
        *aRowCnt = (UInt)sRowCnt;
    }
    else
    {
        /* Nothing to do */
    }

    sdi::setShardMetaTouched( aStatement->session );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_SHARD_META )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                  "sdm::insertTable",
                                  "NodeName is not found in ReplicaSet" ) );
    }
    IDE_EXCEPTION( ERR_SYSTEM_OBJECT )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDM_SYSTEM_OBJECT ) );
    }
    IDE_EXCEPTION( ERR_EXIST_SHARD_OBJECT )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDM_AREADY_EXIST_SHARD_OBJECT ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC sdm::updateDefaultNodeAndPartititon( qcStatement         * aStatement,
                                            SChar               * aUserName,
                                            SChar               * aTableName,
                                            SChar               * aDefaultNodeName,
                                            SChar               * aDefaultPartitionName,
                                            UInt                * aRowCnt,
                                            sdiInternalOperation  aInternalOp )
{
    sdiTableInfo    sTableInfo;
    SChar         * sSqlStr;
    sdiNode         sNode;
    vSLong          sRowCnt;
    idBool          sIsTableFound = ID_FALSE;
    sdiGlobalMetaInfo sMetaNodeInfo = { SDI_NULL_SMN };

    sdiReplicaSetInfo sReplicaSetInfo;
    UInt              sReplicaSetId = SDI_REPLICASET_NULL_ID;

    IDE_TEST( checkMetaVersion( QC_SMI_STMT( aStatement ) )
              != IDE_SUCCESS );

    IDE_TEST( makeShardMeta4NewSMN(aStatement) != IDE_SUCCESS );

    IDE_TEST( getGlobalMetaInfoCore( QC_SMI_STMT(aStatement),
                                         &sMetaNodeInfo ) != IDE_SUCCESS );

    IDE_TEST( getTableInfo( QC_SMI_STMT( aStatement ),
                            aUserName,
                            aTableName,
                            sMetaNodeInfo.mShardMetaNumber,
                            &sTableInfo,
                            &sIsTableFound )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( sIsTableFound == ID_FALSE,
                    ERR_NOT_EXIST_TABLE );

    SDI_INIT_NODE(&sNode);
    if ( aDefaultNodeName[0] != '\0' )
    {
        IDE_TEST( getNodeByName( QC_SMI_STMT( aStatement ),
                                 aDefaultNodeName,
                                 sMetaNodeInfo.mShardMetaNumber,
                                 &sNode )
                  != IDE_SUCCESS );
    }

    switch ( aInternalOp )
    {
        case SDI_INTERNAL_OP_NOT:
        case SDI_INTERNAL_OP_NORMAL:
            if ( sNode.mNodeId != SDI_NODE_NULL_ID )
            {
                /* DefaultNode 가 설정되면 해당 Node의 ReplicaSet 정보를 가져와야 한다. */
                IDE_TEST( sdm::getReplicaSetsByPName( QC_SMI_STMT( aStatement),
                                                      sNode.mNodeName,
                                                      sMetaNodeInfo.mShardMetaNumber,
                                                      &sReplicaSetInfo )
                          != IDE_SUCCESS );

                /* ReplicaSet의 PrimaryNodeName와 지정된 Node가 일치하는 경우가 항상 1개 있어야 한다. */
                IDE_TEST_RAISE( sReplicaSetInfo.mCount != 1, ERR_SHARD_META );

                sReplicaSetId = sReplicaSetInfo.mReplicaSets[0].mReplicaSetId;
            }
            else
            {
                sReplicaSetId = SDI_REPLICASET_NULL_ID;
            }
            break;

        case SDI_INTERNAL_OP_FAILOVER:
        case SDI_INTERNAL_OP_FAILBACK:
        case SDI_INTERNAL_OP_DROPFORCE:
            /* Failover/Failback에서는 ReplicaSet을 변경하지 않는다. */
            sReplicaSetId = sTableInfo.mDefaultPartitionReplicaSetId;
            break;

        default :
            IDE_DASSERT( 0 );
            break;
    }

    IDE_TEST( STRUCT_ALLOC_WITH_SIZE( aStatement->qmxMem,
                                      SChar,
                                      QD_MAX_SQL_LENGTH,
                                      &sSqlStr )
              != IDE_SUCCESS);

    if ( sNode.mNodeId != SDI_NODE_NULL_ID )
    {
        idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                         "UPDATE SYS_SHARD.OBJECTS_ SET "
                         "  DEFAULT_NODE_ID = "QCM_SQL_INT32_FMT", "
                         "  DEFAULT_PARTITION_NAME = "QCM_SQL_VARCHAR_FMT", "
                         "  DEFAULT_PARTITION_REPLICA_SET_ID = "QCM_SQL_INT32_FMT
                         "  WHERE SHARD_ID = "
                         QCM_SQL_INT32_FMT" "
                         "AND SMN = "QCM_SQL_BIGINT_FMT,
                         sNode.mNodeId,
                         aDefaultPartitionName,
                         sReplicaSetId,
                         sTableInfo.mShardID,
                         sMetaNodeInfo.mShardMetaNumber);
    }
    else
    {
        idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                         "UPDATE SYS_SHARD.OBJECTS_ SET "
                         "  DEFAULT_NODE_ID = NULL, "
                         "  DEFAULT_PARTITION_NAME = "QCM_SQL_VARCHAR_FMT", "
                         "  DEFAULT_PARTITION_REPLICA_SET_ID = "QCM_SQL_INT32_FMT
                         "  WHERE SHARD_ID = "
                         QCM_SQL_INT32_FMT" "
                         "AND SMN = "QCM_SQL_BIGINT_FMT,
                         aDefaultPartitionName,
                         sReplicaSetId,
                         sTableInfo.mShardID,
                         sMetaNodeInfo.mShardMetaNumber);
    }

    IDE_TEST( qciMisc::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                     sSqlStr,
                                     & sRowCnt )
              != IDE_SUCCESS );

    if ( aRowCnt != NULL )
    {
        *aRowCnt = (UInt)sRowCnt;
    }
    else
    {
        /* Nothing to do */
    }

    sdi::setShardMetaTouched( aStatement->session );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_SHARD_META )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                  "sdm::updateDefaultNodeAndPartititon",
                                  "NodeName is not found in ReplicaSet" ) );
    }
    IDE_EXCEPTION( ERR_NOT_EXIST_TABLE )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDM_SHARD_TABLE_NOT_EXIST ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

// delete shard table
IDE_RC sdm::deleteObject( qcStatement * aStatement,
                          SChar       * aUserName,
                          SChar       * aTableName,
                          UInt        * aRowCnt )
{
    sdiTableInfo    sTableInfo;
    SChar         * sSqlStr;
    vSLong          sRowCnt;
    idBool          sIsTableFound = ID_FALSE;
    sdiGlobalMetaInfo sMetaNodeInfo = { SDI_NULL_SMN };

    IDE_TEST( checkMetaVersion( QC_SMI_STMT( aStatement ) )
              != IDE_SUCCESS );


    IDE_TEST( makeShardMeta4NewSMN(aStatement) != IDE_SUCCESS );

    IDE_TEST( getGlobalMetaInfoCore( QC_SMI_STMT(aStatement),
                                         &sMetaNodeInfo ) != IDE_SUCCESS );

    IDE_TEST( getTableInfo( QC_SMI_STMT( aStatement ),
                            aUserName,
                            aTableName,
                            sMetaNodeInfo.mShardMetaNumber,
                            &sTableInfo,
                            &sIsTableFound )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( sIsTableFound == ID_FALSE,
                    ERR_NOT_EXIST_TABLE );

    IDE_TEST( STRUCT_ALLOC_WITH_SIZE( aStatement->qmxMem,
                                      SChar,
                                      QD_MAX_SQL_LENGTH,
                                      &sSqlStr )
              != IDE_SUCCESS);

    // remove shard distribution info
    idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                     "DELETE FROM SYS_SHARD.RANGES_ WHERE SHARD_ID = "
                     QCM_SQL_INT32_FMT" "
                     "AND SMN = "QCM_SQL_BIGINT_FMT,
                     sTableInfo.mShardID,
                     sMetaNodeInfo.mShardMetaNumber );

    IDE_TEST( qciMisc::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                     sSqlStr,
                                     & sRowCnt )
              != IDE_SUCCESS );

    idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                     "DELETE FROM SYS_SHARD.CLONES_ WHERE SHARD_ID = "
                     QCM_SQL_INT32_FMT" "
                     "AND SMN = "QCM_SQL_BIGINT_FMT,
                     sTableInfo.mShardID,
                     sMetaNodeInfo.mShardMetaNumber );

    IDE_TEST( qciMisc::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                     sSqlStr,
                                     & sRowCnt )
              != IDE_SUCCESS );

    idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                     "DELETE FROM SYS_SHARD.SOLOS_ WHERE SHARD_ID = "
                     QCM_SQL_INT32_FMT" "
                     "AND SMN = "QCM_SQL_BIGINT_FMT,
                     sTableInfo.mShardID,
                     sMetaNodeInfo.mShardMetaNumber);

    IDE_TEST( qciMisc::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                     sSqlStr,
                                     & sRowCnt )
              != IDE_SUCCESS );

    idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                     "DELETE FROM SYS_SHARD.OBJECTS_ WHERE SHARD_ID = "
                     QCM_SQL_INT32_FMT" "
                     "AND SMN = "QCM_SQL_BIGINT_FMT,
                     sTableInfo.mShardID,
                     sMetaNodeInfo.mShardMetaNumber);

    IDE_TEST( qciMisc::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                     sSqlStr,
                                     & sRowCnt )
              != IDE_SUCCESS );

    if ( aRowCnt != NULL )
    {
        *aRowCnt = (UInt)sRowCnt;
    }
    else
    {
        /* Nothing to do */
    }

    sdi::setShardMetaTouched( aStatement->session );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_EXIST_TABLE )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDM_SHARD_TABLE_NOT_EXIST ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdm::deleteObjectByID( qcStatement * aStatement,
                              UInt          aShardID,
                              UInt        * aRowCnt )
{
    SChar         * sSqlStr;
    vSLong          sRowCnt;
    sdiGlobalMetaInfo sMetaNodeInfo = { SDI_NULL_SMN };

    IDE_TEST( checkMetaVersion( QC_SMI_STMT( aStatement ) )
              != IDE_SUCCESS );


    IDE_TEST( makeShardMeta4NewSMN(aStatement) != IDE_SUCCESS );

    IDE_TEST( getGlobalMetaInfoCore( QC_SMI_STMT(aStatement),
                                         &sMetaNodeInfo ) != IDE_SUCCESS );

    IDE_TEST( STRUCT_ALLOC_WITH_SIZE( aStatement->qmxMem,
                                      SChar,
                                      QD_MAX_SQL_LENGTH,
                                      &sSqlStr )
              != IDE_SUCCESS);

    // delete shard distribution info
    idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                     "DELETE FROM SYS_SHARD.RANGES_ WHERE SHARD_ID = "
                     QCM_SQL_INT32_FMT" "
                     " AND SMN = "QCM_SQL_BIGINT_FMT,
                     aShardID,
                     sMetaNodeInfo.mShardMetaNumber);

    IDE_TEST( qciMisc::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                     sSqlStr,
                                     & sRowCnt )
              != IDE_SUCCESS );

    idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                     "DELETE FROM SYS_SHARD.CLONES_ WHERE SHARD_ID = "
                     QCM_SQL_INT32_FMT" "
                     " AND SMN = "QCM_SQL_BIGINT_FMT,
                     aShardID,
                     sMetaNodeInfo.mShardMetaNumber);

    IDE_TEST( qciMisc::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                     sSqlStr,
                                     & sRowCnt )
              != IDE_SUCCESS );

    idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                     "DELETE FROM SYS_SHARD.SOLOS_ WHERE SHARD_ID = "
                     QCM_SQL_INT32_FMT" "
                     " AND SMN = "QCM_SQL_BIGINT_FMT,
                     aShardID,
                     sMetaNodeInfo.mShardMetaNumber);

    IDE_TEST( qciMisc::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                     sSqlStr,
                                     & sRowCnt )
              != IDE_SUCCESS );

    idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                     "DELETE FROM SYS_SHARD.OBJECTS_ WHERE SHARD_ID = "
                     QCM_SQL_INT32_FMT" "
                     " AND SMN = "QCM_SQL_BIGINT_FMT,
                     aShardID,
                     sMetaNodeInfo.mShardMetaNumber);

    IDE_TEST( qciMisc::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                     sSqlStr,
                                     & sRowCnt )
              != IDE_SUCCESS );

    if ( aRowCnt != NULL )
    {
        *aRowCnt = (UInt)sRowCnt;
    }
    else
    {
        /* Nothing to do */
    }

    sdi::setShardMetaTouched( aStatement->session );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

// insert range info
IDE_RC sdm::insertRange( qcStatement * aStatement,
                         SChar       * aUserName,
                         SChar       * aTableName,
                         SChar       * aPartitionName,
                         SChar       * aValue,
                         SChar       * aSubValue,
                         SChar       * aNodeName,
                         SChar       * aSetFuncType,
                         UInt        * aRowCnt )
{
    SChar         * sSqlStr = NULL;
    vSLong          sRowCnt = 0;
    sdiNode         sNode;
    sdiTableInfo    sTableInfo;
    idBool          sIsTableFound = ID_FALSE;
    
    sdiGlobalMetaInfo sMetaNodeInfo = { SDI_NULL_SMN };
    sdiReplicaSetInfo sReplicaSetInfo;
    UInt              sReplicaSetId;

    SDI_INIT_TABLE_INFO( &sTableInfo );
    SDI_INIT_NODE(&sNode);

    IDE_TEST( checkMetaVersion( QC_SMI_STMT( aStatement ) )
              != IDE_SUCCESS );

    IDE_TEST( makeShardMeta4NewSMN(aStatement) != IDE_SUCCESS );

    IDE_TEST( getGlobalMetaInfoCore( QC_SMI_STMT(aStatement),
                                         &sMetaNodeInfo ) != IDE_SUCCESS );

    IDE_TEST( getNodeByName( QC_SMI_STMT( aStatement ),
                             aNodeName,
                             sMetaNodeInfo.mShardMetaNumber,
                             &sNode )
              != IDE_SUCCESS );

    IDE_TEST( getTableInfo( QC_SMI_STMT( aStatement ),
                            aUserName,
                            aTableName,
                            sMetaNodeInfo.mShardMetaNumber,
                            &sTableInfo,
                            &sIsTableFound )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( sIsTableFound == ID_FALSE,
                    ERR_NOT_EXIST_TABLE );

    if ( aSetFuncType[0] == 'H' ) // from DBMS_SHARD.SET_SHARD_HASH()
    {
        IDE_TEST_RAISE( ( ( sTableInfo.mSplitMethod != SDI_SPLIT_HASH ) ||
                          ( sTableInfo.mSubKeyExists != ID_FALSE ) ),
                        ERR_INVALID_RANGE_FUNCTION );
    }
    else if ( aSetFuncType[0] == 'R' ) // from DBMS_SHARD.SET_SHARD_RANGE()
    {
        IDE_TEST_RAISE( ( ( sTableInfo.mSplitMethod != SDI_SPLIT_RANGE ) ||
                          ( sTableInfo.mSubKeyExists != ID_FALSE ) ),
                        ERR_INVALID_RANGE_FUNCTION );
    }
    else if ( aSetFuncType[0] == 'L' ) // from DBMS_SHARD.SET_SHARD_LIST()
    {
        IDE_TEST_RAISE( ( ( sTableInfo.mSplitMethod != SDI_SPLIT_LIST ) ||
                          ( sTableInfo.mSubKeyExists != ID_FALSE ) ),
                        ERR_INVALID_RANGE_FUNCTION );
    }
    else if ( aSetFuncType[0] == 'P' ) // from DBMS_SHARD.SET_SHARD_COMP()
    {
        IDE_TEST_RAISE( sTableInfo.mSubKeyExists != ID_TRUE, ERR_INVALID_RANGE_FUNCTION );

        IDE_TEST_RAISE( !( sdi::getSplitType( sTableInfo.mSplitMethod ) == SDI_SPLIT_TYPE_DIST ),
                        ERR_INVALID_RANGE_FUNCTION );

        IDE_TEST_RAISE( !( sdi::getSplitType( sTableInfo.mSubSplitMethod ) == SDI_SPLIT_TYPE_DIST ),
                        ERR_INVALID_RANGE_FUNCTION );
    }
    else
    {
        IDE_RAISE( ERR_INVALID_RANGE_FUNCTION );
    }

    // BUG-46619
    IDE_TEST( validateRangeCountBeforeInsert( aStatement,
                                              &sTableInfo,
                                              sMetaNodeInfo.mShardMetaNumber )
              != IDE_SUCCESS );

    IDE_TEST( sdm::getReplicaSetsByPName( QC_SMI_STMT( aStatement),
                                          sNode.mNodeName,
                                          sMetaNodeInfo.mShardMetaNumber,
                                          &sReplicaSetInfo )
              != IDE_SUCCESS );

    /* ReplicaSet의 PrimaryNodeName와 지정된 Node가 일치하는 경우가 항상 1개 있어야 한다. */
    IDE_TEST_RAISE( sReplicaSetInfo.mCount != 1, ERR_SHARD_META );

    sReplicaSetId = sReplicaSetInfo.mReplicaSets[0].mReplicaSetId;

    IDE_TEST( STRUCT_ALLOC_WITH_SIZE( aStatement->qmxMem,
                                      SChar,
                                      QD_MAX_SQL_LENGTH,
                                      &sSqlStr )
              != IDE_SUCCESS);

    if ( sTableInfo.mSubKeyExists == ID_FALSE )
    {
        idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                         "INSERT INTO SYS_SHARD.RANGES_ VALUES( "
                         QCM_SQL_INT32_FMT", "    //shardID
                         QCM_SQL_VARCHAR_FMT", "  //PARTITION_NAME
                         "%s, "                   //aValue
                         QCM_SQL_VARCHAR_FMT", "  //aSubValue
                         QCM_SQL_INT32_FMT", "    //aNodeId
                         QCM_SQL_BIGINT_FMT", "   //SMN
                         QCM_SQL_INT32_FMT")",    //ReplicaSetId
                         sTableInfo.mShardID,
                         aPartitionName,
                         aValue,
                         SDM_NA_STR,
                         sNode.mNodeId,
                         sMetaNodeInfo.mShardMetaNumber,
                         sReplicaSetId );
    }
    else
    {
        idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                         "INSERT INTO SYS_SHARD.RANGES_ VALUES( "
                         QCM_SQL_INT32_FMT", "    //shardID
                         QCM_SQL_VARCHAR_FMT", "  //PARTITION_NAME
                         "%s, "                   //aValue
                         QCM_SQL_VARCHAR_FMT", "  //aSubValue
                         QCM_SQL_INT32_FMT", "    //aNodeId
                         QCM_SQL_BIGINT_FMT", "   //SMN
                         QCM_SQL_INT32_FMT")",    //ReplicaSetId
                         sTableInfo.mShardID,
                         aPartitionName,
                         aValue,
                         aSubValue,
                         sNode.mNodeId,
                         sMetaNodeInfo.mShardMetaNumber,
                         sReplicaSetId);
    }
    
    IDE_TEST( qciMisc::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                     sSqlStr,
                                     & sRowCnt )
              != IDE_SUCCESS );

    if ( aRowCnt != NULL )
    {
        *aRowCnt = (UInt)sRowCnt;
    }
    else
    {
        /* Nothing to do */
    }

    sdi::setShardMetaTouched( aStatement->session );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_SHARD_META )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                  "sdm::insertRange",
                                  "NodeName is not found in ReplicaSet" ) );
    }
    IDE_EXCEPTION( ERR_NOT_EXIST_TABLE )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDM_SHARD_TABLE_NOT_EXIST ) );
    }
    IDE_EXCEPTION( ERR_INVALID_RANGE_FUNCTION )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDM_INVALID_RANGE_FUNCTION ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

// update range info
IDE_RC sdm::updateRange( qcStatement         * aStatement,
                         SChar               * aUserName,
                         SChar               * aTableName,
                         SChar               * aOldNodeName,
                         SChar               * aNewNodeName,
                         SChar               * aPartitionName,
                         SChar               * aValue,
                         SChar               * aSubValue,
                         UInt                * aRowCnt,
                         sdiInternalOperation  aInternalOp )
{
    SChar         * sSqlStr = NULL;
    vSLong          sRowCnt = 0;

    sdiNode         sNewNode;
    sdiNode         sOldNode;
    sdiTableInfo    sTableInfo;
    idBool          sIsTableFound = ID_FALSE;

    sdiReplicaSetInfo sReplicaSetInfo;
    UInt              sOldReplicaSetId = SDI_REPLICASET_NULL_ID;
    UInt              sNewReplicaSetId = SDI_REPLICASET_NULL_ID;
    UInt              sCnt = 0;
    idBool            sOldIsFound = ID_FALSE;
    idBool            sNewIsFound = ID_FALSE;
    sdiShardObjectType         sShardTableType = SDI_NON_SHARD_OBJECT;
    
    sdiGlobalMetaInfo sMetaNodeInfo = { SDI_NULL_SMN };
    sdiRangeInfo      sRangeInfo;
    sdiValue          sValueStr;

    SChar             sObjectValue[SDI_RANGE_VARCHAR_MAX_PRECISION + 1];

    SDI_INIT_TABLE_INFO( &sTableInfo );

    IDE_TEST( STRUCT_ALLOC_WITH_SIZE( aStatement->qmxMem,
                                      SChar,
                                      QD_MAX_SQL_LENGTH,
                                      &sSqlStr )
              != IDE_SUCCESS);

    IDE_TEST( checkMetaVersion( QC_SMI_STMT( aStatement ) )
              != IDE_SUCCESS );


    IDE_TEST( makeShardMeta4NewSMN(aStatement) != IDE_SUCCESS );

    IDE_TEST( getGlobalMetaInfoCore( QC_SMI_STMT(aStatement),
                                         &sMetaNodeInfo ) != IDE_SUCCESS );

    // get tableInfo
    IDE_TEST( getTableInfo( QC_SMI_STMT( aStatement ),
                            aUserName,
                            aTableName,
                            sMetaNodeInfo.mShardMetaNumber,
                            &sTableInfo,
                            &sIsTableFound )
              != IDE_SUCCESS );

    // 테이블이 존재하지 않는다면 에러
    IDE_TEST_RAISE( sIsTableFound == ID_FALSE,
                    ERR_SHARD_OBJECT_DOES_NOT_EXIST );

    // get oldNodeInfo
    SDI_INIT_NODE(&sOldNode);
    IDE_TEST( getNodeByName( QC_SMI_STMT( aStatement ),
                             aOldNodeName,
                             sMetaNodeInfo.mShardMetaNumber,
                             &sOldNode )
              != IDE_SUCCESS );

    // get newNodeInfo
    SDI_INIT_NODE(&sNewNode);
    if ( aNewNodeName[0] != '\0' )
    {
        IDE_TEST( getNodeByName( QC_SMI_STMT( aStatement ),
                                 aNewNodeName,
                                 sMetaNodeInfo.mShardMetaNumber,
                                 &sNewNode )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }

    IDE_TEST( sdm::getAllReplicaSetInfoSortedByPName( QC_SMI_STMT( aStatement ),
                                                      &sReplicaSetInfo,
                                                      sMetaNodeInfo.mShardMetaNumber ) 
              != IDE_SUCCESS );

    switch( aInternalOp )
    {
        case SDI_INTERNAL_OP_NOT:
        case SDI_INTERNAL_OP_NORMAL:
            for ( sCnt = 0; sCnt < sReplicaSetInfo.mCount; sCnt++ )
            {
                if ( idlOS::strncmp( sReplicaSetInfo.mReplicaSets[sCnt].mPrimaryNodeName,
                                     sOldNode.mNodeName,
                                     SDI_NODE_NAME_MAX_SIZE + 1 ) == 0 )
                {
                    sOldReplicaSetId = sReplicaSetInfo.mReplicaSets[sCnt].mReplicaSetId;
                    sOldIsFound = ID_TRUE;
                }
                if ( idlOS::strncmp( sReplicaSetInfo.mReplicaSets[sCnt].mPrimaryNodeName,
                                     sNewNode.mNodeName,
                                     SDI_NODE_NAME_MAX_SIZE + 1 ) == 0 )
                {
                    sNewReplicaSetId = sReplicaSetInfo.mReplicaSets[sCnt].mReplicaSetId;
                    sNewIsFound = ID_TRUE;
                }
            }

            if ( idlOS::strncmp( sOldNode.mNodeName,
                                 SDI_NODE_DEALLOC_NAME,
                                 SDI_NODE_NAME_MAX_SIZE + 1 ) != 0 )
            {
                /* Shard Object 추가는 모든 Node가 Valid 한 상황에서만 호출된다.
                 * ReplicaSet의 PrimaryNodeName과 현재 Node가 일치하는 경우가 항상 1개 있어야 한다. */
                IDE_TEST_RAISE( sOldIsFound != ID_TRUE , ERR_SHARD_META );
            }
            else
            {
                /* Dealloc의 경우 ReplicaSetId는 RangeInfo를 확인해야 한다. */
                IDE_TEST( setKeyDataType( aStatement,
                                          &sTableInfo ) != IDE_SUCCESS );

                IDE_TEST( sdm::getRangeInfo ( aStatement,
                                              QC_SMI_STMT( aStatement ),
                                              sMetaNodeInfo.mShardMetaNumber,
                                              &sTableInfo,
                                              &sRangeInfo,
                                              ID_FALSE )
                          != IDE_SUCCESS );

                sShardTableType = sdi::getShardObjectType( &sTableInfo );

                switch( sShardTableType )
                {
                    case SDI_SINGLE_SHARD_KEY_DIST_OBJECT:
                        for ( sCnt = 0; sCnt < sRangeInfo.mCount ; sCnt++ )
                        {
                            if ( sTableInfo.mObjectType == 'T' ) /* Table */
                            {
                                if ( idlOS::strncmp( sRangeInfo.mRanges[sCnt].mPartitionName,
                                                     aPartitionName,
                                                     QC_MAX_OBJECT_NAME_LEN + 1 ) == 0 )
                                {
                                    sOldReplicaSetId = sRangeInfo.mRanges[sCnt].mReplicaSetId;
                                    break;
                                }
                            }
                            else /* Procedure */
                            {
                                if ( sTableInfo.mSplitMethod == SDI_SPLIT_HASH )
                                {
                                    IDE_TEST( sdi::getValueStr( MTD_INTEGER_ID,
                                                                &sRangeInfo.mRanges[sCnt].mValue,
                                                                &sValueStr )
                                              != IDE_SUCCESS );
                                }
                                else
                                {
                                    IDE_TEST( sdi::getValueStr( sTableInfo.mKeyDataType,
                                                                &sRangeInfo.mRanges[sCnt].mValue,
                                                                &sValueStr )
                                              != IDE_SUCCESS );
                                }

                                if ( sValueStr.mCharMax.value[0] == '\'' )
                                {
                                    // INPUT ARG ('''A''') => 'A' => '''A'''
                                    idlOS::snprintf( sObjectValue,
                                                     SDI_RANGE_VARCHAR_MAX_PRECISION + 1,
                                                     "''%s''",
                                                     sValueStr.mCharMax.value );
                                }
                                else
                                {
                                    // INPUT ARG ('A') => A => 'A'
                                    idlOS::snprintf( sObjectValue,
                                                     SDI_RANGE_VARCHAR_MAX_PRECISION + 1,
                                                     "'%s'",
                                                     sValueStr.mCharMax.value );
                                }

                                if ( idlOS::strncmp( sObjectValue,
                                                     aValue,
                                                     SDI_RANGE_VARCHAR_MAX_PRECISION + 1 ) == 0 )
                                {
                                    sOldReplicaSetId = sRangeInfo.mRanges[sCnt].mReplicaSetId;
                                    break;
                                }
                            }
                        }
                        break;
                    case SDI_SOLO_DIST_OBJECT:
                        for ( sCnt = 0; sCnt < sRangeInfo.mCount ; sCnt++ )
                        {
                            sOldReplicaSetId = sRangeInfo.mRanges[sCnt].mReplicaSetId;
                        }
                        break;
                    case SDI_CLONE_DIST_OBJECT:
                        for ( sCnt = 0; sCnt < sRangeInfo.mCount ; sCnt++ )
                        {
                            if ( sRangeInfo.mRanges[sCnt].mNodeId == sOldNode.mNodeId )
                            { 
                                sOldReplicaSetId = sRangeInfo.mRanges[sCnt].mReplicaSetId;
                            }
                        }
                        break;
                    case SDI_NON_SHARD_OBJECT:
                    case SDI_COMPOSITE_SHARD_KEY_DIST_OBJECT:
                    default:
                        IDE_RAISE( ERR_SHARD_TYPE );
                        break;
                }
            }
            if ( sNewNode.mNodeId != SDI_NODE_NULL_ID )
            {
                IDE_TEST_RAISE( sNewIsFound != ID_TRUE , ERR_SHARD_META );
            }
            break;
        case SDI_INTERNAL_OP_FAILOVER:
        case SDI_INTERNAL_OP_FAILBACK:
        case SDI_INTERNAL_OP_DROPFORCE:
            /* FailOver/FailBack은 연결되어 있는 ReplicaSetID를 변경하지 않는다. */
            /* FailOver에서는 FailOver 가능한 Partition만 넘어온다.
             * 다만 ReplicaSet은 특정 Node에 여럿 연결되어 있을수 있기 때문에
             * RangeInfo로 확인하여야 한다. */
            IDE_TEST( setKeyDataType( aStatement,
                                      &sTableInfo ) != IDE_SUCCESS );

            IDE_TEST( sdm::getRangeInfo ( aStatement,
                                          QC_SMI_STMT( aStatement ),
                                          sMetaNodeInfo.mShardMetaNumber,
                                          &sTableInfo,
                                          &sRangeInfo,
                                          ID_FALSE )
                      != IDE_SUCCESS );

            sShardTableType = sdi::getShardObjectType( &sTableInfo );

            switch( sShardTableType )
            {
                case SDI_SINGLE_SHARD_KEY_DIST_OBJECT:
                    for ( sCnt = 0; sCnt < sRangeInfo.mCount ; sCnt++ )
                    {
                        if ( sTableInfo.mObjectType == 'T' ) /* Table */
                        {
                            if ( idlOS::strncmp( sRangeInfo.mRanges[sCnt].mPartitionName,
                                                 aPartitionName,
                                                 QC_MAX_OBJECT_NAME_LEN + 1 ) == 0 )
                            {
                                sOldReplicaSetId = sRangeInfo.mRanges[sCnt].mReplicaSetId;
                                sNewReplicaSetId = sRangeInfo.mRanges[sCnt].mReplicaSetId;
                                break;
                            }
                        }
                        else /* Procedure */
                        {
                            if ( sTableInfo.mSplitMethod == SDI_SPLIT_HASH )
                            {
                                IDE_TEST( sdi::getValueStr( MTD_INTEGER_ID,
                                                            &sRangeInfo.mRanges[sCnt].mValue,
                                                            &sValueStr )
                                          != IDE_SUCCESS );
                            }
                            else
                            {
                                IDE_TEST( sdi::getValueStr( sTableInfo.mKeyDataType,
                                                            &sRangeInfo.mRanges[sCnt].mValue,
                                                            &sValueStr )
                                          != IDE_SUCCESS );
                            }

                            if ( sValueStr.mCharMax.value[0] == '\'' )
                            {
                                // INPUT ARG ('''A''') => 'A' => '''A'''
                                idlOS::snprintf( sObjectValue,
                                                 SDI_RANGE_VARCHAR_MAX_PRECISION + 1,
                                                 "''%s''",
                                                 sValueStr.mCharMax.value );
                            }
                            else
                            {
                                // INPUT ARG ('A') => A => 'A'
                                idlOS::snprintf( sObjectValue,
                                                 SDI_RANGE_VARCHAR_MAX_PRECISION + 1,
                                                 "'%s'",
                                                 sValueStr.mCharMax.value );
                            }

                            if ( idlOS::strncmp( sObjectValue,
                                                 aValue,
                                                 SDI_RANGE_VARCHAR_MAX_PRECISION + 1 ) == 0 )
                            {
                                sOldReplicaSetId = sRangeInfo.mRanges[sCnt].mReplicaSetId;
                                sNewReplicaSetId = sRangeInfo.mRanges[sCnt].mReplicaSetId;
                                break;
                            }
                        }
                    }
                    break;
                case SDI_SOLO_DIST_OBJECT:
                    for ( sCnt = 0; sCnt < sRangeInfo.mCount ; sCnt++ )
                    {
                        sOldReplicaSetId = sRangeInfo.mRanges[sCnt].mReplicaSetId;
                        sNewReplicaSetId = sRangeInfo.mRanges[sCnt].mReplicaSetId;
                    }
                    break;
                case SDI_CLONE_DIST_OBJECT:
                    for ( sCnt = 0; sCnt < sRangeInfo.mCount ; sCnt++ )
                    {
                        if ( sRangeInfo.mRanges[sCnt].mNodeId == sOldNode.mNodeId )
                        { 
                            sOldReplicaSetId = sRangeInfo.mRanges[sCnt].mReplicaSetId;
                            sNewReplicaSetId = sRangeInfo.mRanges[sCnt].mReplicaSetId;
                        }
                    }
                    break;
                case SDI_NON_SHARD_OBJECT:
                case SDI_COMPOSITE_SHARD_KEY_DIST_OBJECT:
                default:
                    IDE_RAISE( ERR_SHARD_TYPE );
                    break;
            }
            break;

        default :
            IDE_DASSERT( 0 );
            break;
    }

    switch( sdi::getShardObjectType(&sTableInfo) )
    {
        case SDI_SINGLE_SHARD_KEY_DIST_OBJECT:
            IDE_TEST_RAISE( aSubValue[0] != '\0', ERR_ARGUMENT_NOT_APPLICABLE );
            if ( sNewNode.mNodeId == SDI_NODE_NULL_ID )
            {
                idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                                 "DELETE "
                                 "  FROM SYS_SHARD.RANGES_ "
                                 " WHERE VALUE = %s " //aValue
                                 "   AND PARTITION_NAME = "
                                 QCM_SQL_VARCHAR_FMT // partition name
                                 "   AND SHARD_ID = "
                                 QCM_SQL_INT32_FMT   // shardId
                                 "   AND NODE_ID = "
                                 QCM_SQL_INT32_FMT  // oldNodeId
                                 "   AND SMN = "
                                 QCM_SQL_BIGINT_FMT // SMN
                                 "   AND REPLICA_SET_ID = "
                                 QCM_SQL_INT32_FMT, // REPLICA_SET_ID
                                 aValue,
                                 aPartitionName,
                                 sTableInfo.mShardID,
                                 sOldNode.mNodeId,
                                 sMetaNodeInfo.mShardMetaNumber,
                                 sOldReplicaSetId );
            }
            else
            {
                idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                                 "UPDATE SYS_SHARD.RANGES_ "
                                 "   SET NODE_ID = "
                                 QCM_SQL_INT32_FMT", "   // newNodeId
                                 "   REPLICA_SET_ID = "
                                 QCM_SQL_INT32_FMT  // New_REPLICA_SET_ID
                                 " WHERE VALUE = %s " // aValue
                                 "   AND PARTITION_NAME = "
                                 QCM_SQL_VARCHAR_FMT // partition name
                                 "   AND SHARD_ID = "
                                 QCM_SQL_INT32_FMT   // shardId
                                 "   AND NODE_ID = "
                                 QCM_SQL_INT32_FMT   // oldNodeId
                                 "   AND SMN = "
                                 QCM_SQL_BIGINT_FMT  // SMN
                                 "   AND REPLICA_SET_ID = "
                                 QCM_SQL_INT32_FMT,  // Old_REPLICA_SET_ID
                                 sNewNode.mNodeId,
                                 sNewReplicaSetId,
                                 aValue,
                                 aPartitionName,
                                 sTableInfo.mShardID,
                                 sOldNode.mNodeId,
                                 sMetaNodeInfo.mShardMetaNumber,
                                 sOldReplicaSetId );
            }
            break;

        case SDI_COMPOSITE_SHARD_KEY_DIST_OBJECT:
            IDE_TEST_RAISE( aSubValue[0] == '\0', ERR_ARGUMENT_NOT_APPLICABLE );
            if ( sNewNode.mNodeId == SDI_NODE_NULL_ID )
            {
                idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                                 "DELETE "
                                 "  FROM SYS_SHARD.RANGES_ "
                                 " WHERE VALUE = %s " // aValue
                                 "   AND SUB_VALUE = "
                                 QCM_SQL_VARCHAR_FMT // aSubValue
                                 "   AND SHARD_ID = "
                                 QCM_SQL_INT32_FMT   // shardId
                                 "   AND NODE_ID = "
                                 QCM_SQL_INT32_FMT   // oldNodeId
                                 "   AND SMN = "
                                 QCM_SQL_BIGINT_FMT  // SMN
                                 "   AND REPLICA_SET_ID = "
                                 QCM_SQL_INT32_FMT,  // REPLICA_SET_ID
                                 aValue,
                                 aSubValue,
                                 sTableInfo.mShardID,
                                 sOldNode.mNodeId,
                                 sMetaNodeInfo.mShardMetaNumber,
                                 sOldReplicaSetId );
            }
            else
            {
                idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                                 "UPDATE SYS_SHARD.RANGES_ "
                                 "   SET NODE_ID = "
                                 QCM_SQL_INT32_FMT", " // newNodeId
                                 "   REPLICA_SET_ID = "
                                 QCM_SQL_INT32_FMT   // New_REPLICA_SET_ID
                                 " WHERE VALUE = %s " // aValue
                                 "   AND SUB_VALUE = "
                                 QCM_SQL_VARCHAR_FMT // aSubValue
                                 "   AND SHARD_ID = "
                                 QCM_SQL_INT32_FMT   // shardId
                                 "   AND NODE_ID = "
                                 QCM_SQL_INT32_FMT   // oldNodeId
                                 "   AND SMN = "
                                 QCM_SQL_BIGINT_FMT  // SMN
                                 "   AND REPLICA_SET_ID = "
                                 QCM_SQL_INT32_FMT,  // Old_REPLICA_SET_ID
                                 sNewNode.mNodeId,
                                 sNewReplicaSetId,
                                 aValue,
                                 aSubValue,
                                 sTableInfo.mShardID,
                                 sOldNode.mNodeId,
                                 sMetaNodeInfo.mShardMetaNumber,
                                 sOldReplicaSetId );
            }
            break;

        case SDI_CLONE_DIST_OBJECT:
            if ( sNewNode.mNodeId == SDI_NODE_NULL_ID )
            {
                // NewNodeName이 없다. 해당 거주노드를 제거
                idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                                 "DELETE "
                                 "  FROM SYS_SHARD.CLONES_ "
                                 " WHERE SHARD_ID = "
                                 QCM_SQL_INT32_FMT  // shardId
                                 "   AND NODE_ID = "
                                 QCM_SQL_INT32_FMT  // oldNodeId
                                 "   AND SMN = "
                                 QCM_SQL_BIGINT_FMT // SMN
                                 "   AND REPLICA_SET_ID = "
                                 QCM_SQL_INT32_FMT, // REPLICA_SET_ID
                                 sTableInfo.mShardID,
                                 sOldNode.mNodeId,
                                 sMetaNodeInfo.mShardMetaNumber,
                                 sOldReplicaSetId );
            }
            else
            {
                // NewNodeName이 있다. 해당 거주노드를 newNodeName으로 갱신
                idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                                 "UPDATE SYS_SHARD.CLONES_ "
                                 "   SET NODE_ID = "
                                 QCM_SQL_INT32_FMT", " // nodeId
                                 "   REPLICA_SET_ID = "
                                 QCM_SQL_INT32_FMT  // New_REPLICA_SET_ID
                                 " WHERE SHARD_ID = "
                                 QCM_SQL_INT32_FMT   // shardId
                                 "   AND NODE_ID = "
                                 QCM_SQL_INT32_FMT   // oldNodeId
                                 "   AND SMN = "
                                 QCM_SQL_BIGINT_FMT // SMN
                                 "   AND REPLICA_SET_ID = "
                                 QCM_SQL_INT32_FMT, // REPLICA_SET_ID
                                 sNewNode.mNodeId,
                                 sNewReplicaSetId,
                                 sTableInfo.mShardID,
                                 sOldNode.mNodeId,
                                 sMetaNodeInfo.mShardMetaNumber,
                                 sOldReplicaSetId );
            }
            break;

        case SDI_SOLO_DIST_OBJECT:
            if ( sNewNode.mNodeId == SDI_NODE_NULL_ID )
            {
                // NewNodeName이 없다. 해당 거주노드를 제거
                idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                                 "DELETE "
                                 "  FROM SYS_SHARD.SOLOS_ "
                                 " WHERE SHARD_ID = "
                                 QCM_SQL_INT32_FMT   // shardId
                                 "   AND NODE_ID = "
                                 QCM_SQL_INT32_FMT   // oldNodeId
                                 "   AND SMN = "
                                 QCM_SQL_BIGINT_FMT  // SMN
                                 "   AND REPLICA_SET_ID = "
                                 QCM_SQL_INT32_FMT,  // REPLICA_SET_ID
                                 sTableInfo.mShardID,
                                 sOldNode.mNodeId,
                                 sMetaNodeInfo.mShardMetaNumber,
                                 sOldReplicaSetId );

            }
            else
            {
                // NewNodeName이 있다. 해당 거주노드를 newNodeName으로 갱신
                idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                                 "UPDATE SYS_SHARD.SOLOS_ "
                                 "   SET NODE_ID = "
                                 QCM_SQL_INT32_FMT", "  // nodeId
                                 "   REPLICA_SET_ID = "
                                 QCM_SQL_INT32_FMT  // New-REPLICA_SET_ID
                                 " WHERE SHARD_ID = "
                                 QCM_SQL_INT32_FMT  // shardId
                                 "   AND NODE_ID = "
                                 QCM_SQL_INT32_FMT  // oldNodeId
                                 "   AND SMN = "
                                 QCM_SQL_BIGINT_FMT  // SMN
                                 "   AND REPLICA_SET_ID = "
                                 QCM_SQL_INT32_FMT,  // Old_REPLICA_SET_ID
                                 sNewNode.mNodeId,
                                 sNewReplicaSetId,
                                 sTableInfo.mShardID,
                                 sOldNode.mNodeId,
                                 sMetaNodeInfo.mShardMetaNumber,
                                 sOldReplicaSetId );
            }
            break;

        case SDI_NON_SHARD_OBJECT:
        default:
            IDE_RAISE(ERR_ARGUMENT_NOT_APPLICABLE);
            break;
    }

    IDE_TEST( qciMisc::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                     sSqlStr,
                                     & sRowCnt )
              != IDE_SUCCESS );

    if ( aRowCnt != NULL )
    {
        *aRowCnt = (UInt)sRowCnt;
    }
    else
    {
        /* Nothing to do */
    }

    sdi::setShardMetaTouched( aStatement->session );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_SHARD_TYPE )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                  "sdm::updateRange",
                                  "Invalid Shard Type" ) );
    }
    IDE_EXCEPTION( ERR_SHARD_META )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                  "sdm::updateRange",
                                  "NodeName is not found in ReplicaSet" ) );
    }
    IDE_EXCEPTION( ERR_ARGUMENT_NOT_APPLICABLE );
    {
        IDE_SET(ideSetErrorCode(mtERR_ABORT_ARGUMENT_NOT_APPLICABLE));
    }
    IDE_EXCEPTION( ERR_SHARD_OBJECT_DOES_NOT_EXIST )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDM_SHARD_TABLE_NOT_EXIST ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

// insert clone info
IDE_RC sdm::insertClone( qcStatement * aStatement,
                         SChar       * aUserName,
                         SChar       * aTableName,
                         SChar       * aNodeName,
                         UInt          aReplicaSetId,
                         UInt        * aRowCnt,
                         sdiInternalOperation  aInternalOp )

{
    SChar         * sSqlStr;
    vSLong          sRowCnt;

    sdiNode         sNode;
    sdiTableInfo    sTableInfo;
    idBool          sIsTableFound = ID_FALSE;

    sdiGlobalMetaInfo sMetaNodeInfo = { SDI_NULL_SMN };

    sdiReplicaSetInfo sReplicaSetInfo;
    UInt              sReplicaSetId = SDI_REPLICASET_NULL_ID;

    IDE_TEST( checkMetaVersion( QC_SMI_STMT( aStatement ) )
              != IDE_SUCCESS );

    IDE_TEST( makeShardMeta4NewSMN(aStatement) != IDE_SUCCESS );

    IDE_TEST( getGlobalMetaInfoCore( QC_SMI_STMT(aStatement),
                                         &sMetaNodeInfo ) != IDE_SUCCESS );

    IDE_TEST( getNodeByName( QC_SMI_STMT( aStatement ),
                             aNodeName,
                             sMetaNodeInfo.mShardMetaNumber,
                             &sNode )
              != IDE_SUCCESS );

    IDE_TEST( getTableInfo( QC_SMI_STMT( aStatement ),
                            aUserName,
                            aTableName,
                            sMetaNodeInfo.mShardMetaNumber,
                            &sTableInfo,
                            &sIsTableFound )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( sIsTableFound == ID_FALSE,
                    ERR_NOT_EXIST_TABLE );

    IDE_TEST_RAISE( sTableInfo.mSplitMethod != SDI_SPLIT_CLONE,
                    ERR_INVALID_RANGE_FUNCTION );

    // BUG-46619
    IDE_TEST( validateRangeCountBeforeInsert( aStatement,
                                              &sTableInfo,
                                              sMetaNodeInfo.mShardMetaNumber )
              != IDE_SUCCESS );

    switch ( aInternalOp )
    {
        case SDI_INTERNAL_OP_NOT:
        case SDI_INTERNAL_OP_NORMAL:
        case SDI_INTERNAL_OP_FAILOVER:
            IDE_TEST( sdm::getReplicaSetsByPName( QC_SMI_STMT( aStatement),
                                                  sNode.mNodeName,
                                                  sMetaNodeInfo.mShardMetaNumber,
                                                  &sReplicaSetInfo )
                      != IDE_SUCCESS );

            /* ReplicaSet의 PrimaryNodeName와 지정된 Node가 일치하는 경우가 항상 1개 있어야 한다. */
            IDE_TEST_RAISE( sReplicaSetInfo.mCount != 1, ERR_SHARD_META );

            sReplicaSetId = sReplicaSetInfo.mReplicaSets[0].mReplicaSetId;
            break;
        case SDI_INTERNAL_OP_FAILBACK:
            /* Failback에서는 지정한 ReplicaSetID와 연결하여야 한다. */
            sReplicaSetId = aReplicaSetId;
            break;

        case SDI_INTERNAL_OP_DROPFORCE:
        default :
            IDE_DASSERT( 0 );
            break;
    }

    IDE_TEST( STRUCT_ALLOC_WITH_SIZE( aStatement->qmxMem,
                                      SChar,
                                      QD_MAX_SQL_LENGTH,
                                      &sSqlStr )
              != IDE_SUCCESS);

    idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                     "INSERT INTO SYS_SHARD.CLONES_ VALUES( "
                     QCM_SQL_INT32_FMT", "  //shardID
                     QCM_SQL_INT32_FMT", "  //aNodeId
                     QCM_SQL_BIGINT_FMT", "
                     QCM_SQL_INT32_FMT") ",  // ReplicaSetId
                     sTableInfo.mShardID,
                     sNode.mNodeId,
                     sMetaNodeInfo.mShardMetaNumber,
                     sReplicaSetId );

    IDE_TEST( qciMisc::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                     sSqlStr,
                                     & sRowCnt )
              != IDE_SUCCESS );

    if ( aRowCnt != NULL )
    {
        *aRowCnt = (UInt)sRowCnt;
    }
    else
    {
        /* Nothing to do */
    }

    sdi::setShardMetaTouched( aStatement->session );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_SHARD_META )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                  "sdm::insertClone",
                                  "NodeName is not found in ReplicaSet" ) );
    }
    IDE_EXCEPTION( ERR_NOT_EXIST_TABLE )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDM_SHARD_TABLE_NOT_EXIST ) );
    }
    IDE_EXCEPTION( ERR_INVALID_RANGE_FUNCTION )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDM_INVALID_RANGE_FUNCTION ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

// insert solo info
IDE_RC sdm::insertSolo( qcStatement * aStatement,
                        SChar       * aUserName,
                        SChar       * aTableName,
                        SChar       * aNodeName,
                        UInt        * aRowCnt )
{
    SChar         * sSqlStr;
    vSLong          sRowCnt;

    sdiNode         sNode;
    sdiTableInfo    sTableInfo;
    idBool          sIsTableFound = ID_FALSE;

    sdiGlobalMetaInfo sMetaNodeInfo = { SDI_NULL_SMN };

    sdiReplicaSetInfo sReplicaSetInfo;
    UInt              sReplicaSetId;

    IDE_TEST( makeShardMeta4NewSMN(aStatement) != IDE_SUCCESS );

    IDE_TEST( getGlobalMetaInfoCore( QC_SMI_STMT(aStatement),
                                         &sMetaNodeInfo ) != IDE_SUCCESS );

    IDE_TEST( getNodeByName( QC_SMI_STMT( aStatement ),
                             aNodeName,
                             sMetaNodeInfo.mShardMetaNumber,
                             &sNode )
              != IDE_SUCCESS );

    IDE_TEST( getTableInfo( QC_SMI_STMT( aStatement ),
                            aUserName,
                            aTableName,
                            sMetaNodeInfo.mShardMetaNumber,
                            &sTableInfo,
                            &sIsTableFound )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( sIsTableFound == ID_FALSE,
                    ERR_NOT_EXIST_TABLE );

    IDE_TEST_RAISE( sTableInfo.mSplitMethod != SDI_SPLIT_SOLO,
                    ERR_INVALID_RANGE_FUNCTION );

    IDE_TEST( sdm::getReplicaSetsByPName( QC_SMI_STMT( aStatement),
                                          sNode.mNodeName,
                                          sMetaNodeInfo.mShardMetaNumber,
                                          &sReplicaSetInfo )
              != IDE_SUCCESS );

    /* ReplicaSet의 PrimaryNodeName와 지정된 Node가 일치하는 경우가 항상 1개 있어야 한다. */
    IDE_TEST_RAISE( sReplicaSetInfo.mCount != 1, ERR_SHARD_META );

    sReplicaSetId = sReplicaSetInfo.mReplicaSets[0].mReplicaSetId;

    // BUG-46619
    IDE_TEST( validateRangeCountBeforeInsert( aStatement,
                                              &sTableInfo,
                                              sMetaNodeInfo.mShardMetaNumber )
              != IDE_SUCCESS );

    IDE_TEST( STRUCT_ALLOC_WITH_SIZE( aStatement->qmxMem,
                                      SChar,
                                      QD_MAX_SQL_LENGTH,
                                      &sSqlStr )
              != IDE_SUCCESS);

    idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                     "INSERT INTO SYS_SHARD.SOLOS_ VALUES( "
                     QCM_SQL_INT32_FMT", "  //shardID
                     QCM_SQL_INT32_FMT", "  //aNodeId
                     QCM_SQL_BIGINT_FMT", " //SMN
                     QCM_SQL_INT32_FMT") ", //ReplicaSetId
                     sTableInfo.mShardID,
                     sNode.mNodeId,
                     sMetaNodeInfo.mShardMetaNumber,
                     sReplicaSetId );

    IDE_TEST( qciMisc::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                     sSqlStr,
                                     & sRowCnt )
              != IDE_SUCCESS );

    if ( aRowCnt != NULL )
    {
        *aRowCnt = (UInt)sRowCnt;
    }
    else
    {
        /* Nothing to do */
    }

    sdi::setShardMetaTouched( aStatement->session );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_SHARD_META )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                  "sdm::insertSolo",
                                  "NodeName is not found in ReplicaSet" ) );
    }
    IDE_EXCEPTION( ERR_NOT_EXIST_TABLE )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDM_SHARD_TABLE_NOT_EXIST ) );
    }
    IDE_EXCEPTION( ERR_INVALID_RANGE_FUNCTION )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDM_INVALID_RANGE_FUNCTION ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdm::getNodeByName( smiStatement * aSmiStmt,
                           SChar        * aNodeName,
                           ULong          aSMN,
                           sdiNode      * aNode )
{
    idBool            sIsCursorOpen = ID_FALSE;
    const void      * sRow          = NULL;
    scGRID            sRid;
    const void      * sSdmNodes;
    const void      * sSdmNodesIndex[SDM_MAX_META_INDICES];
    mtcColumn       * sNodeIDColumn;
    mtcColumn       * sNodeNameColumn;
    mtcColumn       * sPortColumn;
    mtcColumn       * sHostIPColumn;
    mtcColumn       * sAlternatePortColumn;
    mtcColumn       * sAlternateHostIPColumn;
    mtcColumn       * sConnectTypeColumn;
    mtcColumn       * sSMNColumn;

    qcNameCharBuffer  sNameValueBuffer;
    mtdCharType     * sNameValue = ( mtdCharType * ) & sNameValueBuffer;

    mtdIntegerType    sID;
    mtdCharType     * sName;
    mtdIntegerType    sPort;
    mtdCharType     * sHostIP;
    mtdIntegerType    sAlternatePort;
    mtdCharType     * sAlternateHostIP;
    mtdIntegerType    sConnectType;

    qtcMetaRangeColumn sNodeNameRange;
    qtcMetaRangeColumn sSMNRange;
    smiRange           sRange;

    smiTableCursor       sCursor;
    smiCursorProperties  sCursorProperty;

    IDE_TEST_RAISE( idlOS::strlen(aNodeName) > QC_MAX_OBJECT_NAME_LEN,
                    ERR_NOT_EXIST_NODE );

    if ( idlOS::strncmp( aNodeName,
                         SDI_NODE_DEALLOC_NAME,
                         SDI_NODE_NAME_MAX_SIZE + 1 ) == 0 ) 
    {
        idlOS::strncpy( aNode->mNodeName,
                        aNodeName,
                        SDI_NODE_NAME_MAX_SIZE + 1 );
        aNode->mNodeId = SDI_NODE_DEALLOC_ID;

        IDE_CONT( normal_exit );
    } 

    IDE_TEST( getMetaTableAndIndex( aSmiStmt,
                                    SDM_NODES,
                                    & sSdmNodes,
                                    sSdmNodesIndex )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( sSdmNodesIndex[SDM_NODES_IDX2_ORDER] == NULL,
                    ERR_META_HANDLE );

    IDE_TEST( smiGetTableColumns( sSdmNodes,
                                  SDM_NODES_NODE_ID_COL_ORDER,
                                  (const smiColumn**)&sNodeIDColumn )
              != IDE_SUCCESS );
    IDE_TEST( smiGetTableColumns( sSdmNodes,
                                  SDM_NODES_NODE_NAME_COL_ORDER,
                                  (const smiColumn**)&sNodeNameColumn )
              != IDE_SUCCESS );
    IDE_TEST( smiGetTableColumns( sSdmNodes,
                                  SDM_NODES_PORT_NO_COL_ORDER,
                                  (const smiColumn**)&sPortColumn )
              != IDE_SUCCESS );
    IDE_TEST( smiGetTableColumns( sSdmNodes,
                                  SDM_NODES_HOST_IP_COL_ORDER,
                                  (const smiColumn**)&sHostIPColumn )
              != IDE_SUCCESS );
    IDE_TEST( smiGetTableColumns( sSdmNodes,
                                  SDM_NODES_ALTERNATE_PORT_NO_COL_ORDER,
                                  (const smiColumn**)&sAlternatePortColumn )
              != IDE_SUCCESS );
    IDE_TEST( smiGetTableColumns( sSdmNodes,
                                  SDM_NODES_ALTERNATE_HOST_IP_COL_ORDER,
                                  (const smiColumn**)&sAlternateHostIPColumn )
              != IDE_SUCCESS );

    IDE_TEST( smiGetTableColumns( sSdmNodes,
                                  SDM_NODES_INTERNAL_CONNECT_TYPE,
                                  (const smiColumn**)&sConnectTypeColumn )
              != IDE_SUCCESS );

    IDE_TEST( smiGetTableColumns( sSdmNodes,
                                  SDM_NODES_SMN_COL_ORDER,
                                  (const smiColumn**)&sSMNColumn )
              != IDE_SUCCESS );

    // mtdModule 설정
    IDE_TEST( mtd::moduleById( &(sNodeNameColumn->module),
                               sNodeNameColumn->type.dataTypeId )
              != IDE_SUCCESS );

    // mtlModule 설정
    IDE_TEST( mtl::moduleById( &sNodeNameColumn->language,
                               sNodeNameColumn->type.languageId )
              != IDE_SUCCESS );
    // mtdModule 설정
    IDE_TEST( mtd::moduleById( &(sSMNColumn->module),
                               sSMNColumn->type.dataTypeId )
              != IDE_SUCCESS );

    // mtlModule 설정
    IDE_TEST( mtl::moduleById( &sSMNColumn->language,
                               sSMNColumn->type.languageId )
              != IDE_SUCCESS );

    qciMisc::setVarcharValue( sNameValue,
                              NULL,
                              aNodeName,
                              idlOS::strlen(aNodeName) );

    qciMisc::makeMetaRangeDoubleColumn(
        &sNodeNameRange,
        &sSMNRange,
        sNodeNameColumn,
        (const void *) sNameValue,
        sSMNColumn,
        (const void *) &aSMN,
        &sRange );

    sCursor.initialize();

    /* PROJ-2622 */
    SMI_CURSOR_PROP_INIT_FOR_META_INDEX_SCAN( &sCursorProperty, NULL );

    IDE_TEST( sCursor.open(
                  aSmiStmt,
                  sSdmNodes,
                  sSdmNodesIndex[SDM_NODES_IDX2_ORDER],
                  smiGetRowSCN( sSdmNodes ),
                  NULL,
                  &sRange,
                  smiGetDefaultKeyRange(),
                  smiGetDefaultFilter(),
                  QCM_META_CURSOR_FLAG,
                  SMI_SELECT_CURSOR,
                  &sCursorProperty )
              != IDE_SUCCESS );

    sIsCursorOpen = ID_TRUE;

    IDE_TEST( sCursor.beforeFirst() != IDE_SUCCESS );

    IDE_TEST( sCursor.readRow( &sRow, &sRid, SMI_FIND_NEXT )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( sRow == NULL, ERR_NOT_EXIST_NODE );

    sID = *(mtdIntegerType*)((SChar *)sRow + sNodeIDColumn->column.offset );
    sName = (mtdCharType*)((SChar *)sRow + sNodeNameColumn->column.offset );

    sPort = *(mtdIntegerType*)((SChar *)sRow + sPortColumn->column.offset );
    sHostIP = (mtdCharType*)((SChar *)sRow + sHostIPColumn->column.offset );
    sAlternatePort = *(mtdIntegerType*)((SChar *)sRow + sAlternatePortColumn->column.offset );
    sAlternateHostIP = (mtdCharType*)((SChar *)sRow + sAlternateHostIPColumn->column.offset );
    sConnectType = *(mtdIntegerType*)((SChar *)sRow + sConnectTypeColumn->column.offset );

    IDE_TEST_RAISE( sHostIP->length > SDI_SERVER_IP_SIZE, IP_STR_OVERFLOW );
    IDE_TEST_RAISE( sAlternateHostIP->length > SDI_SERVER_IP_SIZE, IP_STR_OVERFLOW );
    IDE_TEST_RAISE( sName->length > SDI_NODE_NAME_MAX_SIZE,  NODE_NAME_OVERFLOW );

    aNode->mNodeId = sID;

    idlOS::memcpy( aNode->mNodeName,
                   sName->value,
                   sName->length );
    aNode->mNodeName[sName->length] = '\0';

    /* External Server(network) information */
    idlOS::memcpy( aNode->mServerIP,
                   sHostIP->value,
                   sHostIP->length );
    aNode->mServerIP[sHostIP->length] = '\0';
    aNode->mPortNo = (UShort)sPort;

    IDE_TEST( mtd::moduleById( &(sAlternateHostIPColumn->module),
                               sAlternateHostIPColumn->type.dataTypeId )
              != IDE_SUCCESS );
    if ( sAlternateHostIPColumn->module->isNull(sAlternateHostIPColumn,
                                                sAlternateHostIP) == ID_TRUE )
    {
        aNode->mAlternateServerIP[0] = '\0';
    }
    else
    {
        idlOS::memcpy( aNode->mAlternateServerIP,
                       sAlternateHostIP->value,
                       sAlternateHostIP->length );
        aNode->mAlternateServerIP[sAlternateHostIP->length] = '\0';
    }

    IDE_TEST( mtd::moduleById( &(sAlternatePortColumn->module),
                               sAlternatePortColumn->type.dataTypeId )
              != IDE_SUCCESS );

    if ( sAlternatePortColumn->module->isNull(sAlternatePortColumn,
                                              &sAlternatePort) == ID_TRUE )
    {
        aNode->mAlternatePortNo = 0;
    }
    else
    {
        aNode->mAlternatePortNo = (UShort)sAlternatePort;
    }

    /* Internal Connection Type */
    aNode->mConnectType = (UShort)sConnectType;
    aNode->mSMN = aSMN;

    sIsCursorOpen = ID_FALSE;
    IDE_TEST( sCursor.close() != IDE_SUCCESS );

    IDE_EXCEPTION_CONT( normal_exit );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_META_HANDLE )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDM_SHARD_META_NOT_CREATED,
                                  SDM_NODES ) );
    }
    IDE_EXCEPTION( ERR_NOT_EXIST_NODE )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDM_SHARD_NODE_NOT_EXIST ) );
    }
    IDE_EXCEPTION( IP_STR_OVERFLOW )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QRC_INVALID_HOST_IP_PORT ) );
    }
    IDE_EXCEPTION( NODE_NAME_OVERFLOW )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDF_SHARD_NODE_NAME_TOO_LONG ) );
    }
    IDE_EXCEPTION_END;

    if ( sIsCursorOpen == ID_TRUE )
    {
        (void)sCursor.close();
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_FAILURE;
}

IDE_RC sdm::getNodeByID( smiStatement * aSmiStmt,
                         UInt           aNodeID,
                         ULong          aSMN,
                         sdiNode      * aNode )
{
    idBool            sIsCursorOpen = ID_FALSE;
    const void      * sRow          = NULL;
    scGRID            sRid;
    const void      * sSdmNodes;
    const void      * sSdmNodesIndex[SDM_MAX_META_INDICES];
    mtcColumn       * sNodeIDColumn;
    mtcColumn       * sNodeNameColumn;
    mtcColumn       * sPortColumn;
    mtcColumn       * sHostIPColumn;
    mtcColumn       * sAlternatePortColumn;
    mtcColumn       * sAlternateHostIPColumn;
    mtcColumn       * sConnectTypeColumn;
    mtcColumn       * sSMNColumn;

    mtdIntegerType    sID;
    mtdCharType     * sName;
    mtdIntegerType    sPort;
    mtdCharType     * sHostIP;
    mtdIntegerType    sAlternatePort;
    mtdCharType     * sAlternateHostIP;
    mtdIntegerType    sConnectType;

    qtcMetaRangeColumn   sNodeIDRange;
    qtcMetaRangeColumn sSMNRange;
    smiRange           sRange;

    smiTableCursor       sCursor;
    smiCursorProperties  sCursorProperty;

    IDE_TEST_RAISE( aNodeID == SDI_NODE_NULL_ID,
                    ERR_NOT_EXIST_NODE );

    if ( aNodeID == SDI_NODE_DEALLOC_ID ) 
    {
        idlOS::strncpy( aNode->mNodeName,
                        SDI_NODE_DEALLOC_NAME,
                        SDI_NODE_NAME_MAX_SIZE + 1 );
        aNode->mNodeId = SDI_NODE_DEALLOC_ID;

        IDE_CONT( normal_exit );
    } 

    IDE_TEST( getMetaTableAndIndex( aSmiStmt,
                                    SDM_NODES,
                                    & sSdmNodes,
                                    sSdmNodesIndex )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( sSdmNodesIndex[SDM_NODES_IDX1_ORDER] == NULL,
                    ERR_META_HANDLE );

    IDE_TEST( smiGetTableColumns( sSdmNodes,
                                  SDM_NODES_NODE_ID_COL_ORDER,
                                  (const smiColumn**)&sNodeIDColumn )
              != IDE_SUCCESS );
    IDE_TEST( smiGetTableColumns( sSdmNodes,
                                  SDM_NODES_NODE_NAME_COL_ORDER,
                                  (const smiColumn**)&sNodeNameColumn )
              != IDE_SUCCESS );
    IDE_TEST( smiGetTableColumns( sSdmNodes,
                                  SDM_NODES_PORT_NO_COL_ORDER,
                                  (const smiColumn**)&sPortColumn )
              != IDE_SUCCESS );
    IDE_TEST( smiGetTableColumns( sSdmNodes,
                                  SDM_NODES_HOST_IP_COL_ORDER,
                                  (const smiColumn**)&sHostIPColumn )
              != IDE_SUCCESS );
    IDE_TEST( smiGetTableColumns( sSdmNodes,
                                  SDM_NODES_ALTERNATE_PORT_NO_COL_ORDER,
                                  (const smiColumn**)&sAlternatePortColumn )
              != IDE_SUCCESS );
    IDE_TEST( smiGetTableColumns( sSdmNodes,
                                  SDM_NODES_ALTERNATE_HOST_IP_COL_ORDER,
                                  (const smiColumn**)&sAlternateHostIPColumn )
              != IDE_SUCCESS );

    IDE_TEST( smiGetTableColumns( sSdmNodes,
                                  SDM_NODES_INTERNAL_CONNECT_TYPE,
                                  (const smiColumn**)&sConnectTypeColumn )
              != IDE_SUCCESS );

    IDE_TEST( smiGetTableColumns( sSdmNodes,
                                  SDM_NODES_SMN_COL_ORDER,
                                  (const smiColumn**)&sSMNColumn )
              != IDE_SUCCESS );

    // mtdModule 설정
    IDE_TEST( mtd::moduleById( &(sNodeIDColumn->module),
                               sNodeIDColumn->type.dataTypeId )
              != IDE_SUCCESS );    

    // mtlModule 설정
    IDE_TEST( mtl::moduleById( &sNodeIDColumn->language,
                               sNodeIDColumn->type.languageId )
              != IDE_SUCCESS );

    // mtdModule 설정
    IDE_TEST( mtd::moduleById( &(sSMNColumn->module),
                               sSMNColumn->type.dataTypeId )
              != IDE_SUCCESS );

    // mtlModule 설정
    IDE_TEST( mtl::moduleById( &sSMNColumn->language,
                               sSMNColumn->type.languageId )
              != IDE_SUCCESS );

    qciMisc::makeMetaRangeDoubleColumn(
        &sNodeIDRange,
        &sSMNRange,
        (const mtcColumn *) sNodeIDColumn,
        (const void *) &aNodeID,
        (const mtcColumn *) sSMNColumn,
        (const void *) &aSMN,        
        &sRange);

    sCursor.initialize();

    /* PROJ-2622 */
    SMI_CURSOR_PROP_INIT_FOR_META_INDEX_SCAN( &sCursorProperty, NULL );

    IDE_TEST( sCursor.open(
                  aSmiStmt,
                  sSdmNodes,
                  sSdmNodesIndex[SDM_NODES_IDX1_ORDER],
                  smiGetRowSCN( sSdmNodes ),
                  NULL,
                  &sRange,
                  smiGetDefaultKeyRange(),
                  smiGetDefaultFilter(),
                  QCM_META_CURSOR_FLAG,
                  SMI_SELECT_CURSOR,
                  &sCursorProperty )
              != IDE_SUCCESS );

    sIsCursorOpen = ID_TRUE;

    IDE_TEST( sCursor.beforeFirst() != IDE_SUCCESS );

    IDE_TEST( sCursor.readRow( &sRow, &sRid, SMI_FIND_NEXT )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( sRow == NULL, ERR_NOT_EXIST_NODE );

    sID = *(mtdIntegerType*)((SChar *)sRow + sNodeIDColumn->column.offset );
    sName = (mtdCharType*)((SChar *)sRow + sNodeNameColumn->column.offset );

    sPort = *(mtdIntegerType*)((SChar *)sRow + sPortColumn->column.offset );
    sHostIP = (mtdCharType*)((SChar *)sRow + sHostIPColumn->column.offset );
    sAlternatePort = *(mtdIntegerType*)((SChar *)sRow + sAlternatePortColumn->column.offset );
    sAlternateHostIP = (mtdCharType*)((SChar *)sRow + sAlternateHostIPColumn->column.offset );
    sConnectType = *(mtdIntegerType*)((SChar *)sRow + sConnectTypeColumn->column.offset );

    IDE_TEST_RAISE( sHostIP->length > SDI_SERVER_IP_SIZE, IP_STR_OVERFLOW );
    IDE_TEST_RAISE( sAlternateHostIP->length > SDI_SERVER_IP_SIZE, IP_STR_OVERFLOW );
    IDE_TEST_RAISE( sName->length > SDI_NODE_NAME_MAX_SIZE,  NODE_NAME_OVERFLOW );

    aNode->mNodeId = sID;

    idlOS::memcpy( aNode->mNodeName,
                   sName->value,
                   sName->length );
    aNode->mNodeName[sName->length] = '\0';

    /* External Server(network) information */
    idlOS::memcpy( aNode->mServerIP,
                   sHostIP->value,
                   sHostIP->length );
    aNode->mServerIP[sHostIP->length] = '\0';
    aNode->mPortNo = (UShort)sPort;

    IDE_TEST( mtd::moduleById( &(sAlternateHostIPColumn->module),
                               sAlternateHostIPColumn->type.dataTypeId )
              != IDE_SUCCESS );
    if ( sAlternateHostIPColumn->module->isNull(sAlternateHostIPColumn,
                                                sAlternateHostIP) == ID_TRUE )
    {
        aNode->mAlternateServerIP[0] = '\0';
    }
    else
    {
        idlOS::memcpy( aNode->mAlternateServerIP,
                       sAlternateHostIP->value,
                       sAlternateHostIP->length );
        aNode->mAlternateServerIP[sAlternateHostIP->length] = '\0';
    }

    IDE_TEST( mtd::moduleById( &(sAlternatePortColumn->module),
                               sAlternatePortColumn->type.dataTypeId )
              != IDE_SUCCESS );

    if ( sAlternatePortColumn->module->isNull(sAlternatePortColumn,
                                              &sAlternatePort) == ID_TRUE )
    {
        aNode->mAlternatePortNo = 0;
    }
    else
    {
        aNode->mAlternatePortNo = (UShort)sAlternatePort;
    }

    /* Internal Connection Type */
    aNode->mConnectType = (UShort)sConnectType;

    sIsCursorOpen = ID_FALSE;
    IDE_TEST( sCursor.close() != IDE_SUCCESS );

    IDE_EXCEPTION_CONT( normal_exit );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_META_HANDLE )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDM_SHARD_META_NOT_CREATED,
                                  SDM_NODES ) );
    }
    IDE_EXCEPTION( ERR_NOT_EXIST_NODE )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDM_SHARD_NODE_NOT_EXIST ) );
    }
    IDE_EXCEPTION( IP_STR_OVERFLOW )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QRC_INVALID_HOST_IP_PORT ) );
    }
    IDE_EXCEPTION( NODE_NAME_OVERFLOW )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDF_SHARD_NODE_NAME_TOO_LONG ) );
    }
    IDE_EXCEPTION_END;

    if ( sIsCursorOpen == ID_TRUE )
    {
        (void)sCursor.close();
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_FAILURE;
}

IDE_RC sdm::getExternalNodeInfo( smiStatement * aSmiStmt,
                                 sdiNodeInfo  * aNodeInfo,
                                 ULong          aSMN )
{
    idBool            sIsCursorOpen = ID_FALSE;
    const void      * sRow          = NULL;
    scGRID            sRid;
    const void      * sSdmNodes;
    const void      * sSdmNodesIndex[SDM_MAX_META_INDICES];
    mtcColumn       * sNodeIDColumn;
    mtcColumn       * sNodeNameColumn;
    mtcColumn       * sPortColumn;
    mtcColumn       * sHostIPColumn;
    mtcColumn       * sAlternatePortColumn;
    mtcColumn       * sAlternateHostIPColumn;
    mtcColumn       * sSMNColumn;

    UShort            sCount = 0;

    mtdIntegerType    sID;
    mtdCharType     * sName;
    mtdIntegerType    sPort;
    mtdCharType     * sHostIP;
    mtdIntegerType    sAlternatePort;
    mtdCharType     * sAlternateHostIP;

    smiTableCursor       sCursor;
    smiCursorProperties  sCursorProperty;

    qtcMetaRangeColumn sSMNRange;
    smiRange           sRange;

    /* init */
    aNodeInfo->mCount = 0;

    IDE_TEST( checkMetaVersion( aSmiStmt )
              != IDE_SUCCESS );

    IDE_TEST( getMetaTableAndIndex( aSmiStmt,
                                    SDM_NODES,
                                    & sSdmNodes,
                                    sSdmNodesIndex )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( sSdmNodesIndex[SDM_NODES_IDX3_ORDER] == NULL,
                    ERR_META_HANDLE );

    IDE_TEST( smiGetTableColumns( sSdmNodes,
                                  SDM_NODES_NODE_ID_COL_ORDER,
                                  (const smiColumn**)&sNodeIDColumn )
              != IDE_SUCCESS );
    IDE_TEST( smiGetTableColumns( sSdmNodes,
                                  SDM_NODES_NODE_NAME_COL_ORDER,
                                  (const smiColumn**)&sNodeNameColumn )
              != IDE_SUCCESS );
    IDE_TEST( smiGetTableColumns( sSdmNodes,
                                  SDM_NODES_PORT_NO_COL_ORDER,
                                  (const smiColumn**)&sPortColumn )
              != IDE_SUCCESS );
    IDE_TEST( smiGetTableColumns( sSdmNodes,
                                  SDM_NODES_HOST_IP_COL_ORDER,
                                  (const smiColumn**)&sHostIPColumn )
              != IDE_SUCCESS );
    IDE_TEST( smiGetTableColumns( sSdmNodes,
                                  SDM_NODES_ALTERNATE_PORT_NO_COL_ORDER,
                                  (const smiColumn**)&sAlternatePortColumn )
              != IDE_SUCCESS );
    IDE_TEST( smiGetTableColumns( sSdmNodes,
                                  SDM_NODES_ALTERNATE_HOST_IP_COL_ORDER,
                                  (const smiColumn**)&sAlternateHostIPColumn )
              != IDE_SUCCESS );
    IDE_TEST( smiGetTableColumns( sSdmNodes,
                                  SDM_NODES_SMN_COL_ORDER,
                                  (const smiColumn**)&sSMNColumn )
              != IDE_SUCCESS );

    // mtdModule 설정
    IDE_TEST( mtd::moduleById( &(sSMNColumn->module),
                               sSMNColumn->type.dataTypeId )
              != IDE_SUCCESS );

    // mtlModule 설정
    IDE_TEST( mtl::moduleById( &sSMNColumn->language,
                               sSMNColumn->type.languageId )
              != IDE_SUCCESS );

    qciMisc::makeMetaRangeSingleColumn(
        &sSMNRange,
        (const mtcColumn *) sSMNColumn,
        (const void *) &aSMN,
        &sRange );

    sCursor.initialize();

    /* PROJ-2622 */
    SMI_CURSOR_PROP_INIT_FOR_META_INDEX_SCAN( &sCursorProperty, NULL );

    IDE_TEST( sCursor.open(
                  aSmiStmt,
                  sSdmNodes,
                  sSdmNodesIndex[SDM_NODES_IDX3_ORDER],
                  smiGetRowSCN( sSdmNodes ),
                  NULL,
                  &sRange,
                  smiGetDefaultKeyRange(),
                  smiGetDefaultFilter(),
                  QCM_META_CURSOR_FLAG,
                  SMI_SELECT_CURSOR,
                  &sCursorProperty )
              != IDE_SUCCESS );

    sIsCursorOpen = ID_TRUE;

    IDE_TEST( sCursor.beforeFirst() != IDE_SUCCESS );

    IDE_TEST( sCursor.readRow( &sRow, &sRid, SMI_FIND_NEXT )
              != IDE_SUCCESS );

    /* set mtdModule for null compare, nullable column */
    IDE_TEST( mtd::moduleById( &(sAlternateHostIPColumn->module),
                               sAlternateHostIPColumn->type.dataTypeId )
              != IDE_SUCCESS );
    IDE_TEST( mtd::moduleById( &(sAlternatePortColumn->module),
                               sAlternatePortColumn->type.dataTypeId )
              != IDE_SUCCESS );

    while ( sRow != NULL )
    {
        sID = *(mtdIntegerType*)((SChar *)sRow + sNodeIDColumn->column.offset );
        sName = (mtdCharType*)((SChar *)sRow + sNodeNameColumn->column.offset );
        sPort = *(mtdIntegerType*)((SChar *)sRow + sPortColumn->column.offset );
        sHostIP = (mtdCharType*)((SChar *)sRow + sHostIPColumn->column.offset );
        sAlternatePort = *(mtdIntegerType*)((SChar *)sRow + sAlternatePortColumn->column.offset );
        sAlternateHostIP = (mtdCharType*)((SChar *)sRow + sAlternateHostIPColumn->column.offset );

        IDE_TEST_RAISE( sHostIP->length > SDI_SERVER_IP_SIZE, IP_STR_OVERFLOW );
        IDE_TEST_RAISE( sAlternateHostIP->length > SDI_SERVER_IP_SIZE, IP_STR_OVERFLOW );
        IDE_TEST_RAISE( sName->length > SDI_NODE_NAME_MAX_SIZE,  NODE_NAME_OVERFLOW );

        aNodeInfo->mNodes[sCount].mNodeId = sID;

        idlOS::memcpy( aNodeInfo->mNodes[sCount].mNodeName,
                       sName->value,
                       sName->length );
        aNodeInfo->mNodes[sCount].mNodeName[sName->length] = '\0';

        /* External Server(network) information */
        idlOS::memcpy( aNodeInfo->mNodes[sCount].mServerIP,
                       sHostIP->value,
                       sHostIP->length );
        aNodeInfo->mNodes[sCount].mServerIP[sHostIP->length] = '\0';
        aNodeInfo->mNodes[sCount].mPortNo = (UShort)sPort;

        if ( sAlternateHostIPColumn->module->isNull(sAlternateHostIPColumn,
                                                    sAlternateHostIP) == ID_TRUE )
        {
            aNodeInfo->mNodes[sCount].mAlternateServerIP[0] = '\0';
        }
        else
        {
            idlOS::memcpy( aNodeInfo->mNodes[sCount].mAlternateServerIP,
                           sAlternateHostIP->value,
                           sAlternateHostIP->length );
            aNodeInfo->mNodes[sCount].mAlternateServerIP[sAlternateHostIP->length] = '\0';
        }

        if ( sAlternatePortColumn->module->isNull(sAlternatePortColumn,
                                                  &sAlternatePort) == ID_TRUE )
        {
            aNodeInfo->mNodes[sCount].mAlternatePortNo = 0;
        }
        else
        {
            aNodeInfo->mNodes[sCount].mAlternatePortNo = (UShort)sAlternatePort;
        }

        aNodeInfo->mNodes[sCount].mConnectType = SDI_NODE_CONNECT_TYPE_DEFAULT;
        aNodeInfo->mNodes[sCount].mSMN = aSMN;

        sCount++;

        if ( sCount > SDI_NODE_MAX_COUNT )
        {
            IDE_RAISE( BUFFER_OVERFLOW );
        }
        else
        {
            /* Nothing to do */
        }

        IDE_TEST( sCursor.readRow( &sRow, &sRid, SMI_FIND_NEXT )
                  != IDE_SUCCESS );
    }

    sIsCursorOpen = ID_FALSE;
    IDE_TEST( sCursor.close() != IDE_SUCCESS );

    aNodeInfo->mCount = sCount;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_META_HANDLE )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDM_SHARD_META_NOT_CREATED,
                                  SDM_NODES ) );
    }
    IDE_EXCEPTION( BUFFER_OVERFLOW )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDM_SHARD_NODE_OVERFLOW ) );
    }
    IDE_EXCEPTION( IP_STR_OVERFLOW )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QRC_INVALID_HOST_IP_PORT ) );
    }
    IDE_EXCEPTION( NODE_NAME_OVERFLOW )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDF_SHARD_NODE_NAME_TOO_LONG ) );
    }
    IDE_EXCEPTION_END;

    if ( sIsCursorOpen == ID_TRUE )
    {
        (void)sCursor.close();
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_FAILURE;
}

IDE_RC sdm::getInternalNodeInfo( smiStatement * aSmiStmt,
                                 sdiNodeInfo  * aNodeInfo,
                                 ULong          aSMN )
{
    idBool            sIsCursorOpen = ID_FALSE;
    const void      * sRow          = NULL;
    scGRID            sRid;
    const void      * sSdmNodes;
    const void      * sSdmNodesIndex[SDM_MAX_META_INDICES];
    mtcColumn       * sNodeIDColumn;
    mtcColumn       * sNodeNameColumn;
    mtcColumn       * sPortColumn;
    mtcColumn       * sHostIPColumn;
    mtcColumn       * sAlternatePortColumn;
    mtcColumn       * sAlternateHostIPColumn;
    mtcColumn       * sConnectTypeColumn;
    mtcColumn       * sSMNColumn;

    UShort            sCount = 0;

    mtdIntegerType    sID;
    mtdCharType     * sName;
    mtdIntegerType    sPort;
    mtdCharType     * sHostIP;
    mtdIntegerType    sAlternatePort;
    mtdCharType     * sAlternateHostIP;
    mtdIntegerType    sConnectType;

    smiTableCursor       sCursor;
    smiCursorProperties  sCursorProperty;

    qtcMetaRangeColumn sSMNRange;
    smiRange           sRange;

    /* init */
    aNodeInfo->mCount = 0;

    IDE_TEST( checkMetaVersion( aSmiStmt )
              != IDE_SUCCESS );

    IDE_TEST( getMetaTableAndIndex( aSmiStmt,
                                    SDM_NODES,
                                    & sSdmNodes,
                                    sSdmNodesIndex )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( sSdmNodesIndex[SDM_NODES_IDX3_ORDER] == NULL,
                    ERR_META_HANDLE );

    IDE_TEST( smiGetTableColumns( sSdmNodes,
                                  SDM_NODES_NODE_ID_COL_ORDER,
                                  (const smiColumn**)&sNodeIDColumn )
              != IDE_SUCCESS );
    IDE_TEST( smiGetTableColumns( sSdmNodes,
                                  SDM_NODES_NODE_NAME_COL_ORDER,
                                  (const smiColumn**)&sNodeNameColumn )
              != IDE_SUCCESS );
    IDE_TEST( smiGetTableColumns( sSdmNodes,
                                  SDM_NODES_INTERNAL_PORT_NO_COL_ORDER,
                                  (const smiColumn**)&sPortColumn )
              != IDE_SUCCESS );
    IDE_TEST( smiGetTableColumns( sSdmNodes,
                                  SDM_NODES_INTERNAL_HOST_IP_COL_ORDER,
                                  (const smiColumn**)&sHostIPColumn )
              != IDE_SUCCESS );
    IDE_TEST( smiGetTableColumns( sSdmNodes,
                                  SDM_NODES_INTERNAL_ALTERNATE_PORT_NO_COL_ORDER,
                                  (const smiColumn**)&sAlternatePortColumn )
              != IDE_SUCCESS );
    IDE_TEST( smiGetTableColumns( sSdmNodes,
                                  SDM_NODES_INTERNAL_ALTERNATE_HOST_IP_COL_ORDER,
                                  (const smiColumn**)&sAlternateHostIPColumn )
              != IDE_SUCCESS );

    IDE_TEST( smiGetTableColumns( sSdmNodes,
                                  SDM_NODES_INTERNAL_CONNECT_TYPE,
                                  (const smiColumn**)&sConnectTypeColumn )
              != IDE_SUCCESS );

    IDE_TEST( smiGetTableColumns( sSdmNodes,
                                  SDM_NODES_SMN_COL_ORDER,
                                  (const smiColumn**)&sSMNColumn )
              != IDE_SUCCESS );

    // mtdModule 설정
    IDE_TEST( mtd::moduleById( &(sSMNColumn->module),
                               sSMNColumn->type.dataTypeId )
              != IDE_SUCCESS );

    // mtlModule 설정
    IDE_TEST( mtl::moduleById( &sSMNColumn->language,
                               sSMNColumn->type.languageId )
              != IDE_SUCCESS );

    qciMisc::makeMetaRangeSingleColumn(
        &sSMNRange,
        (const mtcColumn *) sSMNColumn,
        (const void *) &aSMN,
        &sRange );

    sCursor.initialize();

    /* PROJ-2622 */
    SMI_CURSOR_PROP_INIT_FOR_META_INDEX_SCAN( &sCursorProperty, NULL );

    IDE_TEST( sCursor.open(
                  aSmiStmt,
                  sSdmNodes,
                  sSdmNodesIndex[SDM_NODES_IDX3_ORDER],
                  smiGetRowSCN( sSdmNodes ),
                  NULL,
                  &sRange,
                  smiGetDefaultKeyRange(),
                  smiGetDefaultFilter(),
                  QCM_META_CURSOR_FLAG,
                  SMI_SELECT_CURSOR,
                  &sCursorProperty )
              != IDE_SUCCESS );

    sIsCursorOpen = ID_TRUE;

    IDE_TEST( sCursor.beforeFirst() != IDE_SUCCESS );

    IDE_TEST( sCursor.readRow( &sRow, &sRid, SMI_FIND_NEXT )
              != IDE_SUCCESS );

    /* set mtdModule for null compare, nullable column */
    IDE_TEST( mtd::moduleById( &(sAlternateHostIPColumn->module),
                               sAlternateHostIPColumn->type.dataTypeId )
              != IDE_SUCCESS );
    IDE_TEST( mtd::moduleById( &(sAlternatePortColumn->module),
                               sAlternatePortColumn->type.dataTypeId )
              != IDE_SUCCESS );

    while ( sRow != NULL )
    {
        sID = *(mtdIntegerType*)((SChar *)sRow + sNodeIDColumn->column.offset );
        sName = (mtdCharType*)((SChar *)sRow + sNodeNameColumn->column.offset );
        sPort = *(mtdIntegerType*)((SChar *)sRow + sPortColumn->column.offset );
        sHostIP = (mtdCharType*)((SChar *)sRow + sHostIPColumn->column.offset );
        sAlternatePort = *(mtdIntegerType*)((SChar *)sRow + sAlternatePortColumn->column.offset );
        sAlternateHostIP = (mtdCharType*)((SChar *)sRow + sAlternateHostIPColumn->column.offset );
        sConnectType = *(mtdIntegerType*)((SChar *)sRow + sConnectTypeColumn->column.offset );

        IDE_TEST_RAISE( sHostIP->length > SDI_SERVER_IP_SIZE, IP_STR_OVERFLOW );
        IDE_TEST_RAISE( sAlternateHostIP->length > SDI_SERVER_IP_SIZE, IP_STR_OVERFLOW );
        IDE_TEST_RAISE( sName->length > SDI_NODE_NAME_MAX_SIZE,  NODE_NAME_OVERFLOW );

        aNodeInfo->mNodes[sCount].mNodeId = sID;

        idlOS::memcpy( aNodeInfo->mNodes[sCount].mNodeName,
                       sName->value,
                       sName->length );
        aNodeInfo->mNodes[sCount].mNodeName[sName->length] = '\0';

        /* Internal Server(network) information */
        idlOS::memcpy( aNodeInfo->mNodes[sCount].mServerIP,
                       sHostIP->value,
                       sHostIP->length );
        aNodeInfo->mNodes[sCount].mServerIP[sHostIP->length] = '\0';
        aNodeInfo->mNodes[sCount].mPortNo = (UShort)sPort;

        if ( sAlternateHostIPColumn->module->isNull(sAlternateHostIPColumn,
                                                    sAlternateHostIP) == ID_TRUE )
        {
            aNodeInfo->mNodes[sCount].mAlternateServerIP[0] = '\0';
        }
        else
        {
            idlOS::memcpy( aNodeInfo->mNodes[sCount].mAlternateServerIP,
                           sAlternateHostIP->value,
                           sAlternateHostIP->length );
            aNodeInfo->mNodes[sCount].mAlternateServerIP[sAlternateHostIP->length] = '\0';
        }

        if ( sAlternatePortColumn->module->isNull(sAlternatePortColumn,
                                                  &sAlternatePort) == ID_TRUE )
        {
            aNodeInfo->mNodes[sCount].mAlternatePortNo = 0;
        }
        else
        {
            aNodeInfo->mNodes[sCount].mAlternatePortNo = (UShort)sAlternatePort;
        }

        aNodeInfo->mNodes[sCount].mConnectType = (sdiInternalNodeConnectType)sConnectType;
        aNodeInfo->mNodes[sCount].mSMN = aSMN;

        sCount++;

        if ( sCount > SDI_NODE_MAX_COUNT )
        {
            IDE_RAISE( BUFFER_OVERFLOW );
        }
        else
        {
            /* Nothing to do */
        }

        IDE_TEST( sCursor.readRow( &sRow, &sRid, SMI_FIND_NEXT )
                  != IDE_SUCCESS );
    }

    sIsCursorOpen = ID_FALSE;
    IDE_TEST( sCursor.close() != IDE_SUCCESS );

    aNodeInfo->mCount = sCount;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_META_HANDLE )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDM_SHARD_META_NOT_CREATED,
                                  SDM_NODES ) );
    }
    IDE_EXCEPTION( BUFFER_OVERFLOW )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDM_SHARD_NODE_OVERFLOW ) );
    }
    IDE_EXCEPTION( IP_STR_OVERFLOW )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QRC_INVALID_HOST_IP_PORT ) );
    }
    IDE_EXCEPTION( NODE_NAME_OVERFLOW )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDF_SHARD_NODE_NAME_TOO_LONG ) );
    }
    IDE_EXCEPTION_END;

    if ( sIsCursorOpen == ID_TRUE )
    {
        (void)sCursor.close();
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_FAILURE;
}

extern "C" int
sdmCompareNodeNameAsc( const void* aElem1, const void* aElem2 )
{
    return idlOS::strncmp( ((sdiNode*)aElem1)->mNodeName,
                          ((sdiNode*)aElem2)->mNodeName,
                          SDI_NODE_NAME_MAX_SIZE );
}

extern "C" int
sdmCompareNodeNameDesc( const void* aElem1, const void* aElem2 )
{
    return idlOS::strncmp( ((sdiNode*)aElem2)->mNodeName,
                          ((sdiNode*)aElem1)->mNodeName,
                          SDI_NODE_NAME_MAX_SIZE );
}

IDE_RC sdm::getInternalNodeInfoSortedByName( smiStatement * aStatement,
                                             sdiNodeInfo  * aNodeInfo,
                                             ULong          aSMN,
                                             sdmSortOrder   aOrder )
{
    IDE_DASSERT(aNodeInfo != NULL);
    IDE_TEST( sdm::getInternalNodeInfo( aStatement,
                                        aNodeInfo,
                                        aSMN )
              != IDE_SUCCESS );

    if ( aOrder == SDM_SORT_ASC )
    {
        idlOS::qsort( aNodeInfo->mNodes,
                      aNodeInfo->mCount,
                      ID_SIZEOF(sdiNode),
                      sdmCompareNodeNameAsc  );
    }
    else /* SDM_SORT_DESC */
    {
        idlOS::qsort( aNodeInfo->mNodes,
                      aNodeInfo->mCount,
                      ID_SIZEOF(sdiNode),
                      sdmCompareNodeNameDesc  );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdm::getAllReplicaSetInfoSortedByPName( smiStatement       * aSmiStmt,
                                               sdiReplicaSetInfo  * aReplicaSetsInfo,
                                               ULong                aSMN )
{
    idBool            sIsCursorOpen = ID_FALSE;
    const void      * sRow          = NULL;
    scGRID            sRid;
    const void      * sSdmReplicaSets;
    const void      * sSdmReplicaSetsIdx[SDM_MAX_META_INDICES];
    mtcColumn       * sReplicaSetIDColumn;
    mtcColumn       * sPrimaryNodeNameColumn;
    mtcColumn       * sFirstBackupNodeNameColumn;
    mtcColumn       * sSecondBackupNodeNameColumn;
    mtcColumn       * sStopFirstBackupNodeNameColumn;
    mtcColumn       * sStopSecondBackupNodeNameColumn;    
    mtcColumn       * sFirstReplicationNameColumn;
    mtcColumn       * sFirstReplFromNodeNameColumn;
    mtcColumn       * sFirstReplToNodeNameColumn;
    mtcColumn       * sSecondReplicaiontNameColumn;
    mtcColumn       * sSecondReplFromNodeNameColumn;
    mtcColumn       * sSecondReplToNodeNameColumn;
    mtcColumn       * sSMNColumn;

    UShort            sCount = 0;

    mtdIntegerType    sReplicaSetID;
    mtdCharType     * sPrimaryNodeName;
    mtdCharType     * sFirstBackupNodeName;
    mtdCharType     * sSecondBackupNodeName;
    mtdCharType     * sStopFirstBackupNodeName;
    mtdCharType     * sStopSecondBackupNodeName;
    mtdCharType     * sFirstReplicationName;
    mtdCharType     * sFirstReplFromNodeName;
    mtdCharType     * sFirstReplToNodeName;
    mtdCharType     * sSecondReplicaiontName;
    mtdCharType     * sSecondReplFromNodeName;
    mtdCharType     * sSecondReplToNodeName;
    mtdIntegerType    sSMN;

    smiTableCursor       sCursor;
    smiCursorProperties  sCursorProperty;

    qtcMetaRangeColumn sSMNRange;
    smiRange           sRange;

    /* init */
    aReplicaSetsInfo->mCount = 0;

    IDE_TEST( checkMetaVersion( aSmiStmt )
              != IDE_SUCCESS );

    IDE_TEST( getMetaTableAndIndex( aSmiStmt,
                                    SDM_REPLICA_SETS,
                                    & sSdmReplicaSets,
                                    sSdmReplicaSetsIdx )
              != IDE_SUCCESS );

    /* primary_node_name index */
    IDE_TEST_RAISE( sSdmReplicaSetsIdx[SDM_REPLICA_SETS_IDX2_ORDER] == NULL,
                    ERR_META_HANDLE );
    
    IDE_TEST( smiGetTableColumns( sSdmReplicaSets,
                                  SDM_REPLICA_SETS_REPLICA_SET_ID_COL_ORDER,
                                  (const smiColumn**)&sReplicaSetIDColumn )
              != IDE_SUCCESS );
    IDE_TEST( smiGetTableColumns( sSdmReplicaSets,
                                  SDM_REPLICA_SETS_PRIMARY_NODE_NAME_COL_ORDER,
                                  (const smiColumn**)&sPrimaryNodeNameColumn )
              != IDE_SUCCESS );
    IDE_TEST( smiGetTableColumns( sSdmReplicaSets,
                                  SDM_REPLICA_SETS_FIRST_BACKUP_NODE_NAME_COL_ORDER,
                                  (const smiColumn**)&sFirstBackupNodeNameColumn )
              != IDE_SUCCESS );
    IDE_TEST( smiGetTableColumns( sSdmReplicaSets,
                                  SDM_REPLICA_SETS_SECOND_BACKUP_NODE_NAME_COL_ORDER,
                                  (const smiColumn**)&sSecondBackupNodeNameColumn )
              != IDE_SUCCESS );
    IDE_TEST( smiGetTableColumns( sSdmReplicaSets,
                                  SDM_REPLICA_SETS_STOP_FIRST_BACKUP_NODE_NAME_COL_ORDER,
                                  (const smiColumn**)&sStopFirstBackupNodeNameColumn )
              != IDE_SUCCESS );
    IDE_TEST( smiGetTableColumns( sSdmReplicaSets,
                                  SDM_REPLICA_SETS_STOP_SECOND_BACKUP_NODE_NAME_COL_ORDER,
                                  (const smiColumn**)&sStopSecondBackupNodeNameColumn )
              != IDE_SUCCESS );    
    IDE_TEST( smiGetTableColumns( sSdmReplicaSets,
                                  SDM_REPLICA_SETS_FIRST_REPLICATION_NAME_COL_ORDER,
                                  (const smiColumn**)&sFirstReplicationNameColumn )
              != IDE_SUCCESS );
    IDE_TEST( smiGetTableColumns( sSdmReplicaSets,
                                  SDM_REPLICA_SETS_FIRST_REPL_FROM_NODE_NAME_COL_ORDER,
                                  (const smiColumn**)&sFirstReplFromNodeNameColumn )
              != IDE_SUCCESS );
    IDE_TEST( smiGetTableColumns( sSdmReplicaSets,
                                  SDM_REPLICA_SETS_FIRST_REPL_TO_NODE_NAME_COL_ORDER,
                                  (const smiColumn**)&sFirstReplToNodeNameColumn )
              != IDE_SUCCESS );
    IDE_TEST( smiGetTableColumns( sSdmReplicaSets,
                                  SDM_REPLICA_SETS_SECOND_REPLICATION_NAME_COL_ORDER,
                                  (const smiColumn**)&sSecondReplicaiontNameColumn )
              != IDE_SUCCESS );
    IDE_TEST( smiGetTableColumns( sSdmReplicaSets,
                                  SDM_REPLICA_SETS_SECOND_REPL_FROM_NODE_NAME_COL_ORDER,
                                  (const smiColumn**)&sSecondReplFromNodeNameColumn )
              != IDE_SUCCESS );
    IDE_TEST( smiGetTableColumns( sSdmReplicaSets,
                                  SDM_REPLICA_SETS_SECOND_REPL_TO_NODE_NAME_COL_ORDER,
                                  (const smiColumn**)&sSecondReplToNodeNameColumn )
              != IDE_SUCCESS );

    IDE_TEST( smiGetTableColumns( sSdmReplicaSets,
                                  SDM_REPLICA_SETS_SMN_COL_ORDER,
                                  (const smiColumn**)&sSMNColumn )
              != IDE_SUCCESS );

    // mtdModule 설정
    IDE_TEST( mtd::moduleById( &(sSMNColumn->module),
                               sSMNColumn->type.dataTypeId )
              != IDE_SUCCESS );

    // mtlModule 설정
    IDE_TEST( mtl::moduleById( &sSMNColumn->language,
                               sSMNColumn->type.languageId )
              != IDE_SUCCESS );

    qciMisc::makeMetaRangeSingleColumn(
        &sSMNRange,
        (const mtcColumn *) sSMNColumn,
        (const void *) &aSMN,
        &sRange );

    sCursor.initialize();

    /* PROJ-2622 */
    SMI_CURSOR_PROP_INIT_FOR_META_INDEX_SCAN( &sCursorProperty, NULL );

    IDE_TEST( sCursor.open(
                  aSmiStmt,
                  sSdmReplicaSets,
                  sSdmReplicaSetsIdx[SDM_REPLICA_SETS_IDX2_ORDER],
                  smiGetRowSCN( sSdmReplicaSets ),
                  NULL,
                  &sRange,
                  smiGetDefaultKeyRange(),
                  smiGetDefaultFilter(),
                  QCM_META_CURSOR_FLAG,
                  SMI_SELECT_CURSOR,
                  &sCursorProperty )
              != IDE_SUCCESS );

    sIsCursorOpen = ID_TRUE;

    IDE_TEST( sCursor.beforeFirst() != IDE_SUCCESS );

    IDE_TEST( sCursor.readRow( &sRow, &sRid, SMI_FIND_NEXT )
              != IDE_SUCCESS );

    while ( sRow != NULL )
    {
        sReplicaSetID = *(mtdIntegerType*)((SChar *)sRow + sReplicaSetIDColumn->column.offset );
        sPrimaryNodeName = (mtdCharType*)((SChar *)sRow + sPrimaryNodeNameColumn->column.offset );
        sFirstBackupNodeName = (mtdCharType*)((SChar *)sRow + sFirstBackupNodeNameColumn->column.offset );
        sSecondBackupNodeName = (mtdCharType*)((SChar *)sRow + sSecondBackupNodeNameColumn->column.offset );
        sStopFirstBackupNodeName = (mtdCharType*)((SChar *)sRow + sStopFirstBackupNodeNameColumn->column.offset );
        sStopSecondBackupNodeName = (mtdCharType*)((SChar *)sRow + sStopSecondBackupNodeNameColumn->column.offset );                
        sFirstReplicationName = (mtdCharType*)((SChar *)sRow + sFirstReplicationNameColumn->column.offset );
        sFirstReplFromNodeName = (mtdCharType*)((SChar *)sRow + sFirstReplFromNodeNameColumn->column.offset );
        sFirstReplToNodeName = (mtdCharType*)((SChar *)sRow + sFirstReplToNodeNameColumn->column.offset );
        sSecondReplicaiontName = (mtdCharType*)((SChar *)sRow + sSecondReplicaiontNameColumn->column.offset );
        sSecondReplFromNodeName = (mtdCharType*)((SChar *)sRow + sSecondReplFromNodeNameColumn->column.offset );
        sSecondReplToNodeName = (mtdCharType*)((SChar *)sRow + sSecondReplToNodeNameColumn->column.offset );
        sSMN = *(mtdIntegerType*)((SChar *)sRow + sSMNColumn->column.offset );

        aReplicaSetsInfo->mReplicaSets[sCount].mReplicaSetId= sReplicaSetID;

        idlOS::memcpy( aReplicaSetsInfo->mReplicaSets[sCount].mPrimaryNodeName,
                       sPrimaryNodeName->value,
                       sPrimaryNodeName->length );
        aReplicaSetsInfo->mReplicaSets[sCount].mPrimaryNodeName[sPrimaryNodeName->length] = '\0';

        idlOS::memcpy( aReplicaSetsInfo->mReplicaSets[sCount].mFirstBackupNodeName,
                       sFirstBackupNodeName->value,
                       sFirstBackupNodeName->length );
        aReplicaSetsInfo->mReplicaSets[sCount].mFirstBackupNodeName[sFirstBackupNodeName->length] = '\0';

        idlOS::memcpy( aReplicaSetsInfo->mReplicaSets[sCount].mSecondBackupNodeName,
                       sSecondBackupNodeName->value,
                       sSecondBackupNodeName->length );
        aReplicaSetsInfo->mReplicaSets[sCount].mSecondBackupNodeName[sSecondBackupNodeName->length] = '\0';

        idlOS::memcpy( aReplicaSetsInfo->mReplicaSets[sCount].mFirstReplName,
                       sFirstReplicationName->value,
                       sFirstReplicationName->length );
        aReplicaSetsInfo->mReplicaSets[sCount].mFirstReplName[sFirstReplicationName->length] = '\0';

        idlOS::memcpy( aReplicaSetsInfo->mReplicaSets[sCount].mStopFirstBackupNodeName,
                       sStopFirstBackupNodeName->value,
                       sStopFirstBackupNodeName->length );
        aReplicaSetsInfo->mReplicaSets[sCount].mStopFirstBackupNodeName[sStopFirstBackupNodeName->length] = '\0';

        idlOS::memcpy( aReplicaSetsInfo->mReplicaSets[sCount].mStopSecondBackupNodeName,
                       sStopSecondBackupNodeName->value,
                       sStopSecondBackupNodeName->length );
        aReplicaSetsInfo->mReplicaSets[sCount].mStopSecondBackupNodeName[sStopSecondBackupNodeName->length] = '\0';        

        idlOS::memcpy( aReplicaSetsInfo->mReplicaSets[sCount].mFirstReplFromNodeName,
                       sFirstReplFromNodeName->value,
                       sFirstReplFromNodeName->length );
        aReplicaSetsInfo->mReplicaSets[sCount].mFirstReplFromNodeName[sFirstReplFromNodeName->length] = '\0';

        idlOS::memcpy( aReplicaSetsInfo->mReplicaSets[sCount].mFirstReplToNodeName,
                       sFirstReplToNodeName->value,
                       sFirstReplToNodeName->length );
        aReplicaSetsInfo->mReplicaSets[sCount].mFirstReplToNodeName[sFirstReplToNodeName->length] = '\0';

        idlOS::memcpy( aReplicaSetsInfo->mReplicaSets[sCount].mSecondReplName,
                       sSecondReplicaiontName->value,
                       sSecondReplicaiontName->length );
        aReplicaSetsInfo->mReplicaSets[sCount].mSecondReplName[sSecondReplicaiontName->length] = '\0';

        idlOS::memcpy( aReplicaSetsInfo->mReplicaSets[sCount].mSecondReplFromNodeName,
                       sSecondReplFromNodeName->value,
                       sSecondReplFromNodeName->length );
        aReplicaSetsInfo->mReplicaSets[sCount].mSecondReplFromNodeName[sSecondReplFromNodeName->length] = '\0';

        idlOS::memcpy( aReplicaSetsInfo->mReplicaSets[sCount].mSecondReplToNodeName,
                       sSecondReplToNodeName->value,
                       sSecondReplToNodeName->length );
        aReplicaSetsInfo->mReplicaSets[sCount].mSecondReplToNodeName[sSecondReplToNodeName->length] = '\0';

        aReplicaSetsInfo->mReplicaSets[sCount].mSMN = sSMN;

        sCount++;

        if ( sCount > SDI_NODE_MAX_COUNT )
        {
            IDE_RAISE( BUFFER_OVERFLOW );
        }
        else
        {
            /* Nothing to do */
        }

        IDE_TEST( sCursor.readRow( &sRow, &sRid, SMI_FIND_NEXT )
                  != IDE_SUCCESS );
    }

    sIsCursorOpen = ID_FALSE;
    IDE_TEST( sCursor.close() != IDE_SUCCESS );

    aReplicaSetsInfo->mCount = sCount;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_META_HANDLE )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDM_SHARD_META_NOT_CREATED,
                                  SDM_NODES ) );
    }
    IDE_EXCEPTION( BUFFER_OVERFLOW )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDM_SHARD_NODE_OVERFLOW ) );
    }
    IDE_EXCEPTION_END;

    if ( sIsCursorOpen == ID_TRUE )
    {
        (void)sCursor.close();
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_FAILURE;
}

IDE_RC sdm::getReplicaSetsByPName( smiStatement       * aSmiStmt,
                                   SChar              * aPrimaryNodeName,
                                   ULong                aSMN,
                                   sdiReplicaSetInfo  * aReplicaSetsInfo )
{
    idBool            sIsCursorOpen = ID_FALSE;
    const void      * sRow          = NULL;
    scGRID            sRid;
    const void      * sSdmReplicaSets;
    const void      * sSdmReplicaSetsIdx[SDM_MAX_META_INDICES];
    mtcColumn       * sReplicaSetIDColumn;
    mtcColumn       * sPrimaryNodeNameColumn;
    mtcColumn       * sFirstBackupNodeNameColumn;
    mtcColumn       * sSecondBackupNodeNameColumn;
    mtcColumn       * sStopFirstBackupNodeNameColumn;
    mtcColumn       * sStopSecondBackupNodeNameColumn;        
    mtcColumn       * sFirstReplicationNameColumn;
    mtcColumn       * sFirstReplFromNodeNameColumn;
    mtcColumn       * sFirstReplToNodeNameColumn;
    mtcColumn       * sSecondReplicaiontNameColumn;
    mtcColumn       * sSecondReplFromNodeNameColumn;
    mtcColumn       * sSecondReplToNodeNameColumn;
    mtcColumn       * sSMNColumn;

    UShort            sCount = 0;

    qcNameCharBuffer  sPrimaryNameValueBuffer;
    mtdCharType     * sPrimaryNameValue = ( mtdCharType * ) & sPrimaryNameValueBuffer;

    mtdIntegerType    sReplicaSetID;
    mtdCharType     * sPrimaryNodeName;
    mtdCharType     * sFirstBackupNodeName;
    mtdCharType     * sSecondBackupNodeName;
    mtdCharType     * sStopFirstBackupNodeName;
    mtdCharType     * sStopSecondBackupNodeName;    
    mtdCharType     * sFirstReplicationName;
    mtdCharType     * sFirstReplFromNodeName;
    mtdCharType     * sFirstReplToNodeName;
    mtdCharType     * sSecondReplicaiontName;
    mtdCharType     * sSecondReplFromNodeName;
    mtdCharType     * sSecondReplToNodeName;

    smiTableCursor       sCursor;
    smiCursorProperties  sCursorProperty;

    qtcMetaRangeColumn sPrimaryNodeNameRange;
    qtcMetaRangeColumn sSMNRange;
    smiRange           sRange;

    /* init */
    aReplicaSetsInfo->mCount = 0;

    IDE_TEST_RAISE( idlOS::strlen(aPrimaryNodeName) > SDI_NODE_NAME_MAX_SIZE,
                    NODE_NAME_OVERFLOW );

    IDE_TEST( checkMetaVersion( aSmiStmt )
              != IDE_SUCCESS );

    IDE_TEST( getMetaTableAndIndex( aSmiStmt,
                                    SDM_REPLICA_SETS,
                                    & sSdmReplicaSets,
                                    sSdmReplicaSetsIdx )
              != IDE_SUCCESS );

    /* primary_node_name index */
    IDE_TEST_RAISE( sSdmReplicaSetsIdx[SDM_REPLICA_SETS_IDX2_ORDER] == NULL,
                    ERR_META_HANDLE );

    IDE_TEST( smiGetTableColumns( sSdmReplicaSets,
                                  SDM_REPLICA_SETS_REPLICA_SET_ID_COL_ORDER,
                                  (const smiColumn**)&sReplicaSetIDColumn )
              != IDE_SUCCESS );
    IDE_TEST( smiGetTableColumns( sSdmReplicaSets,
                                  SDM_REPLICA_SETS_PRIMARY_NODE_NAME_COL_ORDER,
                                  (const smiColumn**)&sPrimaryNodeNameColumn )
              != IDE_SUCCESS );
    IDE_TEST( smiGetTableColumns( sSdmReplicaSets,
                                  SDM_REPLICA_SETS_FIRST_BACKUP_NODE_NAME_COL_ORDER,
                                  (const smiColumn**)&sFirstBackupNodeNameColumn )
              != IDE_SUCCESS );
    IDE_TEST( smiGetTableColumns( sSdmReplicaSets,
                                  SDM_REPLICA_SETS_SECOND_BACKUP_NODE_NAME_COL_ORDER,
                                  (const smiColumn**)&sSecondBackupNodeNameColumn )
              != IDE_SUCCESS );
    IDE_TEST( smiGetTableColumns( sSdmReplicaSets,
                                  SDM_REPLICA_SETS_STOP_FIRST_BACKUP_NODE_NAME_COL_ORDER,
                                  (const smiColumn**)&sStopFirstBackupNodeNameColumn )
              != IDE_SUCCESS );
    IDE_TEST( smiGetTableColumns( sSdmReplicaSets,
                                  SDM_REPLICA_SETS_STOP_SECOND_BACKUP_NODE_NAME_COL_ORDER,
                                  (const smiColumn**)&sStopSecondBackupNodeNameColumn )
              != IDE_SUCCESS );    
    IDE_TEST( smiGetTableColumns( sSdmReplicaSets,
                                  SDM_REPLICA_SETS_FIRST_REPLICATION_NAME_COL_ORDER,
                                  (const smiColumn**)&sFirstReplicationNameColumn )
              != IDE_SUCCESS );
    IDE_TEST( smiGetTableColumns( sSdmReplicaSets,
                                  SDM_REPLICA_SETS_FIRST_REPL_FROM_NODE_NAME_COL_ORDER,
                                  (const smiColumn**)&sFirstReplFromNodeNameColumn )
              != IDE_SUCCESS );
    IDE_TEST( smiGetTableColumns( sSdmReplicaSets,
                                  SDM_REPLICA_SETS_FIRST_REPL_TO_NODE_NAME_COL_ORDER,
                                  (const smiColumn**)&sFirstReplToNodeNameColumn )
              != IDE_SUCCESS );
    IDE_TEST( smiGetTableColumns( sSdmReplicaSets,
                                  SDM_REPLICA_SETS_SECOND_REPLICATION_NAME_COL_ORDER,
                                  (const smiColumn**)&sSecondReplicaiontNameColumn )
              != IDE_SUCCESS );
    IDE_TEST( smiGetTableColumns( sSdmReplicaSets,
                                  SDM_REPLICA_SETS_SECOND_REPL_FROM_NODE_NAME_COL_ORDER,
                                  (const smiColumn**)&sSecondReplFromNodeNameColumn )
              != IDE_SUCCESS );
    IDE_TEST( smiGetTableColumns( sSdmReplicaSets,
                                  SDM_REPLICA_SETS_SECOND_REPL_TO_NODE_NAME_COL_ORDER,
                                  (const smiColumn**)&sSecondReplToNodeNameColumn )
              != IDE_SUCCESS );

    IDE_TEST( smiGetTableColumns( sSdmReplicaSets,
                                  SDM_REPLICA_SETS_SMN_COL_ORDER,
                                  (const smiColumn**)&sSMNColumn )
              != IDE_SUCCESS );

    // mtdModule 설정
    IDE_TEST( mtd::moduleById( &(sPrimaryNodeNameColumn->module),
                               sPrimaryNodeNameColumn->type.dataTypeId )
              != IDE_SUCCESS );

    // mtlModule 설정
    IDE_TEST( mtl::moduleById( &sPrimaryNodeNameColumn->language,
                               sPrimaryNodeNameColumn->type.languageId )
              != IDE_SUCCESS );

    // mtdModule 설정
    IDE_TEST( mtd::moduleById( &(sSMNColumn->module),
                               sSMNColumn->type.dataTypeId )
              != IDE_SUCCESS );

    // mtlModule 설정
    IDE_TEST( mtl::moduleById( &sSMNColumn->language,
                               sSMNColumn->type.languageId )
              != IDE_SUCCESS );

    qciMisc::setVarcharValue( sPrimaryNameValue,
                              NULL,
                              aPrimaryNodeName,
                              idlOS::strlen(aPrimaryNodeName) );

    qciMisc::makeMetaRangeDoubleColumn(
        &sSMNRange,
        &sPrimaryNodeNameRange,
        sSMNColumn,
        (const void *) &aSMN,
        sPrimaryNodeNameColumn,
        (const void *) sPrimaryNameValue,
        &sRange );

    sCursor.initialize();

    SMI_CURSOR_PROP_INIT_FOR_META_INDEX_SCAN( &sCursorProperty, NULL );

    IDE_TEST( sCursor.open(
                  aSmiStmt,
                  sSdmReplicaSets,
                  sSdmReplicaSetsIdx[SDM_REPLICA_SETS_IDX2_ORDER],
                  smiGetRowSCN( sSdmReplicaSets ),
                  NULL,
                  &sRange,
                  smiGetDefaultKeyRange(),
                  smiGetDefaultFilter(),
                  QCM_META_CURSOR_FLAG,
                  SMI_SELECT_CURSOR,
                  &sCursorProperty )
              != IDE_SUCCESS );

    sIsCursorOpen = ID_TRUE;

    IDE_TEST( sCursor.beforeFirst() != IDE_SUCCESS );

    IDE_TEST( sCursor.readRow( &sRow, &sRid, SMI_FIND_NEXT )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( sRow == NULL, ERR_NOT_EXIST_REPLICA_SET );

    while ( sRow != NULL )
    {
        sReplicaSetID = *(mtdIntegerType*)((SChar *)sRow + sReplicaSetIDColumn->column.offset );
        sPrimaryNodeName = (mtdCharType*)((SChar *)sRow + sPrimaryNodeNameColumn->column.offset );
        sFirstBackupNodeName = (mtdCharType*)((SChar *)sRow + sFirstBackupNodeNameColumn->column.offset );
        sSecondBackupNodeName = (mtdCharType*)((SChar *)sRow + sSecondBackupNodeNameColumn->column.offset );
        sStopFirstBackupNodeName = (mtdCharType*)((SChar *)sRow + sStopFirstBackupNodeNameColumn->column.offset );
        sStopSecondBackupNodeName = (mtdCharType*)((SChar *)sRow + sStopSecondBackupNodeNameColumn->column.offset );                
        sFirstReplicationName = (mtdCharType*)((SChar *)sRow + sFirstReplicationNameColumn->column.offset );
        sFirstReplFromNodeName = (mtdCharType*)((SChar *)sRow + sFirstReplFromNodeNameColumn->column.offset );
        sFirstReplToNodeName = (mtdCharType*)((SChar *)sRow + sFirstReplToNodeNameColumn->column.offset );
        sSecondReplicaiontName = (mtdCharType*)((SChar *)sRow + sSecondReplicaiontNameColumn->column.offset );
        sSecondReplFromNodeName = (mtdCharType*)((SChar *)sRow + sSecondReplFromNodeNameColumn->column.offset );
        sSecondReplToNodeName = (mtdCharType*)((SChar *)sRow + sSecondReplToNodeNameColumn->column.offset );

        aReplicaSetsInfo->mReplicaSets[sCount].mReplicaSetId= sReplicaSetID;

        idlOS::memcpy( aReplicaSetsInfo->mReplicaSets[sCount].mPrimaryNodeName,
                       sPrimaryNodeName->value,
                       sPrimaryNodeName->length );
        aReplicaSetsInfo->mReplicaSets[sCount].mPrimaryNodeName[sPrimaryNodeName->length] = '\0';

        idlOS::memcpy( aReplicaSetsInfo->mReplicaSets[sCount].mFirstBackupNodeName,
                       sFirstBackupNodeName->value,
                       sFirstBackupNodeName->length );
        aReplicaSetsInfo->mReplicaSets[sCount].mFirstBackupNodeName[sFirstBackupNodeName->length] = '\0';

        idlOS::memcpy( aReplicaSetsInfo->mReplicaSets[sCount].mSecondBackupNodeName,
                       sSecondBackupNodeName->value,
                       sSecondBackupNodeName->length );
        aReplicaSetsInfo->mReplicaSets[sCount].mSecondBackupNodeName[sSecondBackupNodeName->length] = '\0';

        idlOS::memcpy( aReplicaSetsInfo->mReplicaSets[sCount].mStopFirstBackupNodeName,
                       sStopFirstBackupNodeName->value,
                       sStopFirstBackupNodeName->length );
        aReplicaSetsInfo->mReplicaSets[sCount].mStopFirstBackupNodeName[sStopFirstBackupNodeName->length] = '\0';

        idlOS::memcpy( aReplicaSetsInfo->mReplicaSets[sCount].mStopSecondBackupNodeName,
                       sStopSecondBackupNodeName->value,
                       sStopSecondBackupNodeName->length );
        aReplicaSetsInfo->mReplicaSets[sCount].mStopSecondBackupNodeName[sStopSecondBackupNodeName->length] = '\0';                
        idlOS::memcpy( aReplicaSetsInfo->mReplicaSets[sCount].mFirstReplName,
                       sFirstReplicationName->value,
                       sFirstReplicationName->length );
        aReplicaSetsInfo->mReplicaSets[sCount].mFirstReplName[sFirstReplicationName->length] = '\0';

        idlOS::memcpy( aReplicaSetsInfo->mReplicaSets[sCount].mFirstReplFromNodeName,
                       sFirstReplFromNodeName->value,
                       sFirstReplFromNodeName->length );
        aReplicaSetsInfo->mReplicaSets[sCount].mFirstReplFromNodeName[sFirstReplFromNodeName->length] = '\0';

        idlOS::memcpy( aReplicaSetsInfo->mReplicaSets[sCount].mFirstReplToNodeName,
                       sFirstReplToNodeName->value,
                       sFirstReplToNodeName->length );
        aReplicaSetsInfo->mReplicaSets[sCount].mFirstReplToNodeName[sFirstReplToNodeName->length] = '\0';

        idlOS::memcpy( aReplicaSetsInfo->mReplicaSets[sCount].mSecondReplName,
                       sSecondReplicaiontName->value,
                       sSecondReplicaiontName->length );
        aReplicaSetsInfo->mReplicaSets[sCount].mSecondReplName[sSecondReplicaiontName->length] = '\0';

        idlOS::memcpy( aReplicaSetsInfo->mReplicaSets[sCount].mSecondReplFromNodeName,
                       sSecondReplFromNodeName->value,
                       sSecondReplFromNodeName->length );
        aReplicaSetsInfo->mReplicaSets[sCount].mSecondReplFromNodeName[sSecondReplFromNodeName->length] = '\0';

        idlOS::memcpy( aReplicaSetsInfo->mReplicaSets[sCount].mSecondReplToNodeName,
                       sSecondReplToNodeName->value,
                       sSecondReplToNodeName->length );
        aReplicaSetsInfo->mReplicaSets[sCount].mSecondReplToNodeName[sSecondReplToNodeName->length] = '\0';

        sCount++;

        if ( sCount > SDI_NODE_MAX_COUNT )
        {
            IDE_RAISE( BUFFER_OVERFLOW );
        }
        else
        {
            /* Nothing to do */
        }

        IDE_TEST( sCursor.readRow( &sRow, &sRid, SMI_FIND_NEXT )
                  != IDE_SUCCESS );
    }

    sIsCursorOpen = ID_FALSE;
    IDE_TEST( sCursor.close() != IDE_SUCCESS );

    aReplicaSetsInfo->mCount = sCount;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_META_HANDLE )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDM_SHARD_META_NOT_CREATED,
                                  SDM_NODES ) );
    }
    IDE_EXCEPTION( BUFFER_OVERFLOW )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDM_SHARD_NODE_OVERFLOW ) );
    }
    IDE_EXCEPTION( NODE_NAME_OVERFLOW )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDF_SHARD_NODE_NAME_TOO_LONG ) );
    }
    IDE_EXCEPTION( ERR_NOT_EXIST_REPLICA_SET )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,"sdm::getReplicaSetsByPName","no replica set" ) );
    }
    IDE_EXCEPTION_END;

    if ( sIsCursorOpen == ID_TRUE )
    {
        (void)sCursor.close();
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_FAILURE;
}

IDE_RC sdm::getReplicaSetsByReplicaSetId( smiStatement       * aSmiStmt,
                                          UInt                 aReplicaSetId,
                                          ULong                aSMN,
                                          sdiReplicaSetInfo  * aReplicaSetsInfo )
{
    idBool            sIsCursorOpen = ID_FALSE;
    const void      * sRow          = NULL;
    scGRID            sRid;
    const void      * sSdmReplicaSets;
    const void      * sSdmReplicaSetsIdx[SDM_MAX_META_INDICES];
    mtcColumn       * sReplicaSetIDColumn;
    mtcColumn       * sPrimaryNodeNameColumn;
    mtcColumn       * sFirstBackupNodeNameColumn;
    mtcColumn       * sSecondBackupNodeNameColumn;
    mtcColumn       * sStopFirstBackupNodeNameColumn;
    mtcColumn       * sStopSecondBackupNodeNameColumn;    
    mtcColumn       * sFirstReplicationNameColumn;
    mtcColumn       * sFirstReplFromNodeNameColumn;
    mtcColumn       * sFirstReplToNodeNameColumn;
    mtcColumn       * sSecondReplicaiontNameColumn;
    mtcColumn       * sSecondReplFromNodeNameColumn;
    mtcColumn       * sSecondReplToNodeNameColumn;
    mtcColumn       * sSMNColumn;

    UShort            sCount = 0;

    mtdIntegerType    sReplicaSetID;
    mtdCharType     * sPrimaryNodeName;
    mtdCharType     * sFirstBackupNodeName;
    mtdCharType     * sSecondBackupNodeName;
    mtdCharType     * sStopFirstBackupNodeName;
    mtdCharType     * sStopSecondBackupNodeName;        
    mtdCharType     * sFirstReplicationName;
    mtdCharType     * sFirstReplFromNodeName;
    mtdCharType     * sFirstReplToNodeName;
    mtdCharType     * sSecondReplicaiontName;
    mtdCharType     * sSecondReplFromNodeName;
    mtdCharType     * sSecondReplToNodeName;

    smiTableCursor       sCursor;
    smiCursorProperties  sCursorProperty;

    qtcMetaRangeColumn sReplicaSetIdRange;
    qtcMetaRangeColumn sSMNRange;
    smiRange           sRange;

    IDE_TEST( checkMetaVersion( aSmiStmt )
              != IDE_SUCCESS );

    IDE_TEST( getMetaTableAndIndex( aSmiStmt,
                                    SDM_REPLICA_SETS,
                                    & sSdmReplicaSets,
                                    sSdmReplicaSetsIdx )
              != IDE_SUCCESS );

    /* primary_node_name index */
    IDE_TEST_RAISE( sSdmReplicaSetsIdx[SDM_REPLICA_SETS_IDX1_ORDER] == NULL,
                    ERR_META_HANDLE );

    IDE_TEST( smiGetTableColumns( sSdmReplicaSets,
                                  SDM_REPLICA_SETS_REPLICA_SET_ID_COL_ORDER,
                                  (const smiColumn**)&sReplicaSetIDColumn )
              != IDE_SUCCESS );
    IDE_TEST( smiGetTableColumns( sSdmReplicaSets,
                                  SDM_REPLICA_SETS_PRIMARY_NODE_NAME_COL_ORDER,
                                  (const smiColumn**)&sPrimaryNodeNameColumn )
              != IDE_SUCCESS );
    IDE_TEST( smiGetTableColumns( sSdmReplicaSets,
                                  SDM_REPLICA_SETS_FIRST_BACKUP_NODE_NAME_COL_ORDER,
                                  (const smiColumn**)&sFirstBackupNodeNameColumn )
              != IDE_SUCCESS );
    IDE_TEST( smiGetTableColumns( sSdmReplicaSets,
                                  SDM_REPLICA_SETS_SECOND_BACKUP_NODE_NAME_COL_ORDER,
                                  (const smiColumn**)&sSecondBackupNodeNameColumn )
              != IDE_SUCCESS );
    IDE_TEST( smiGetTableColumns( sSdmReplicaSets,
                                  SDM_REPLICA_SETS_STOP_FIRST_BACKUP_NODE_NAME_COL_ORDER,
                                  (const smiColumn**)&sStopFirstBackupNodeNameColumn )
              != IDE_SUCCESS );
    IDE_TEST( smiGetTableColumns( sSdmReplicaSets,
                                  SDM_REPLICA_SETS_STOP_SECOND_BACKUP_NODE_NAME_COL_ORDER,
                                  (const smiColumn**)&sStopSecondBackupNodeNameColumn )
              != IDE_SUCCESS );        
    IDE_TEST( smiGetTableColumns( sSdmReplicaSets,
                                  SDM_REPLICA_SETS_FIRST_REPLICATION_NAME_COL_ORDER,
                                  (const smiColumn**)&sFirstReplicationNameColumn )
              != IDE_SUCCESS );
    IDE_TEST( smiGetTableColumns( sSdmReplicaSets,
                                  SDM_REPLICA_SETS_FIRST_REPL_FROM_NODE_NAME_COL_ORDER,
                                  (const smiColumn**)&sFirstReplFromNodeNameColumn )
              != IDE_SUCCESS );
    IDE_TEST( smiGetTableColumns( sSdmReplicaSets,
                                  SDM_REPLICA_SETS_FIRST_REPL_TO_NODE_NAME_COL_ORDER,
                                  (const smiColumn**)&sFirstReplToNodeNameColumn )
              != IDE_SUCCESS );
    IDE_TEST( smiGetTableColumns( sSdmReplicaSets,
                                  SDM_REPLICA_SETS_SECOND_REPLICATION_NAME_COL_ORDER,
                                  (const smiColumn**)&sSecondReplicaiontNameColumn )
              != IDE_SUCCESS );
    IDE_TEST( smiGetTableColumns( sSdmReplicaSets,
                                  SDM_REPLICA_SETS_SECOND_REPL_FROM_NODE_NAME_COL_ORDER,
                                  (const smiColumn**)&sSecondReplFromNodeNameColumn )
              != IDE_SUCCESS );
    IDE_TEST( smiGetTableColumns( sSdmReplicaSets,
                                  SDM_REPLICA_SETS_SECOND_REPL_TO_NODE_NAME_COL_ORDER,
                                  (const smiColumn**)&sSecondReplToNodeNameColumn )
              != IDE_SUCCESS );

    IDE_TEST( smiGetTableColumns( sSdmReplicaSets,
                                  SDM_REPLICA_SETS_SMN_COL_ORDER,
                                  (const smiColumn**)&sSMNColumn )
              != IDE_SUCCESS );

    // mtdModule 설정
    IDE_TEST( mtd::moduleById( &(sReplicaSetIDColumn->module),
                               sReplicaSetIDColumn->type.dataTypeId )
              != IDE_SUCCESS );

    // mtlModule 설정
    IDE_TEST( mtl::moduleById( &sReplicaSetIDColumn->language,
                               sReplicaSetIDColumn->type.languageId )
              != IDE_SUCCESS );

    // mtdModule 설정
    IDE_TEST( mtd::moduleById( &(sSMNColumn->module),
                               sSMNColumn->type.dataTypeId )
              != IDE_SUCCESS );

    // mtlModule 설정
    IDE_TEST( mtl::moduleById( &sSMNColumn->language,
                               sSMNColumn->type.languageId )
              != IDE_SUCCESS );

    qciMisc::makeMetaRangeDoubleColumn(
        &sReplicaSetIdRange,
        &sSMNRange,
        sReplicaSetIDColumn,
        (const void *) &aReplicaSetId,
        sSMNColumn,
        (const void *) &aSMN,
        &sRange );

    sCursor.initialize();

    SMI_CURSOR_PROP_INIT_FOR_META_INDEX_SCAN( &sCursorProperty, NULL );

    IDE_TEST( sCursor.open(
                  aSmiStmt,
                  sSdmReplicaSets,
                  sSdmReplicaSetsIdx[SDM_REPLICA_SETS_IDX1_ORDER],
                  smiGetRowSCN( sSdmReplicaSets ),
                  NULL,
                  &sRange,
                  smiGetDefaultKeyRange(),
                  smiGetDefaultFilter(),
                  QCM_META_CURSOR_FLAG,
                  SMI_SELECT_CURSOR,
                  &sCursorProperty )
              != IDE_SUCCESS );

    sIsCursorOpen = ID_TRUE;

    IDE_TEST( sCursor.beforeFirst() != IDE_SUCCESS );

    IDE_TEST( sCursor.readRow( &sRow, &sRid, SMI_FIND_NEXT )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( sRow == NULL, ERR_NOT_EXIST_REPLICA_SET );

    while ( sRow != NULL )
    {
        sReplicaSetID = *(mtdIntegerType*)((SChar *)sRow + sReplicaSetIDColumn->column.offset );
        sPrimaryNodeName = (mtdCharType*)((SChar *)sRow + sPrimaryNodeNameColumn->column.offset );
        sFirstBackupNodeName = (mtdCharType*)((SChar *)sRow + sFirstBackupNodeNameColumn->column.offset );
        sSecondBackupNodeName = (mtdCharType*)((SChar *)sRow + sSecondBackupNodeNameColumn->column.offset );
        sStopFirstBackupNodeName = (mtdCharType*)((SChar *)sRow + sStopFirstBackupNodeNameColumn->column.offset );
        sStopSecondBackupNodeName = (mtdCharType*)((SChar *)sRow + sStopSecondBackupNodeNameColumn->column.offset );                
        sFirstReplicationName = (mtdCharType*)((SChar *)sRow + sFirstReplicationNameColumn->column.offset );
        sFirstReplFromNodeName = (mtdCharType*)((SChar *)sRow + sFirstReplFromNodeNameColumn->column.offset );
        sFirstReplToNodeName = (mtdCharType*)((SChar *)sRow + sFirstReplToNodeNameColumn->column.offset );
        sSecondReplicaiontName = (mtdCharType*)((SChar *)sRow + sSecondReplicaiontNameColumn->column.offset );
        sSecondReplFromNodeName = (mtdCharType*)((SChar *)sRow + sSecondReplFromNodeNameColumn->column.offset );
        sSecondReplToNodeName = (mtdCharType*)((SChar *)sRow + sSecondReplToNodeNameColumn->column.offset );

        aReplicaSetsInfo->mReplicaSets[sCount].mReplicaSetId= sReplicaSetID;

        idlOS::memcpy( aReplicaSetsInfo->mReplicaSets[sCount].mPrimaryNodeName,
                       sPrimaryNodeName->value,
                       sPrimaryNodeName->length );
        aReplicaSetsInfo->mReplicaSets[sCount].mPrimaryNodeName[sPrimaryNodeName->length] = '\0';

        idlOS::memcpy( aReplicaSetsInfo->mReplicaSets[sCount].mFirstBackupNodeName,
                       sFirstBackupNodeName->value,
                       sFirstBackupNodeName->length );
        aReplicaSetsInfo->mReplicaSets[sCount].mFirstBackupNodeName[sFirstBackupNodeName->length] = '\0';

        idlOS::memcpy( aReplicaSetsInfo->mReplicaSets[sCount].mSecondBackupNodeName,
                       sSecondBackupNodeName->value,
                       sSecondBackupNodeName->length );
        aReplicaSetsInfo->mReplicaSets[sCount].mSecondBackupNodeName[sSecondBackupNodeName->length] = '\0';

        idlOS::memcpy( aReplicaSetsInfo->mReplicaSets[sCount].mStopFirstBackupNodeName,
                       sStopFirstBackupNodeName->value,
                       sStopFirstBackupNodeName->length );
        aReplicaSetsInfo->mReplicaSets[sCount].mStopFirstBackupNodeName[sStopFirstBackupNodeName->length] = '\0';

        idlOS::memcpy( aReplicaSetsInfo->mReplicaSets[sCount].mStopSecondBackupNodeName,
                       sStopSecondBackupNodeName->value,
                       sStopSecondBackupNodeName->length );
        aReplicaSetsInfo->mReplicaSets[sCount].mStopSecondBackupNodeName[sStopSecondBackupNodeName->length] = '\0';        
        
        idlOS::memcpy( aReplicaSetsInfo->mReplicaSets[sCount].mFirstReplName,
                       sFirstReplicationName->value,
                       sFirstReplicationName->length );
        aReplicaSetsInfo->mReplicaSets[sCount].mFirstReplName[sFirstReplicationName->length] = '\0';

        idlOS::memcpy( aReplicaSetsInfo->mReplicaSets[sCount].mFirstReplFromNodeName,
                       sFirstReplFromNodeName->value,
                       sFirstReplFromNodeName->length );
        aReplicaSetsInfo->mReplicaSets[sCount].mFirstReplFromNodeName[sFirstReplFromNodeName->length] = '\0';

        idlOS::memcpy( aReplicaSetsInfo->mReplicaSets[sCount].mFirstReplToNodeName,
                       sFirstReplToNodeName->value,
                       sFirstReplToNodeName->length );
        aReplicaSetsInfo->mReplicaSets[sCount].mFirstReplToNodeName[sFirstReplToNodeName->length] = '\0';

        idlOS::memcpy( aReplicaSetsInfo->mReplicaSets[sCount].mSecondReplName,
                       sSecondReplicaiontName->value,
                       sSecondReplicaiontName->length );
        aReplicaSetsInfo->mReplicaSets[sCount].mSecondReplName[sSecondReplicaiontName->length] = '\0';

        idlOS::memcpy( aReplicaSetsInfo->mReplicaSets[sCount].mSecondReplFromNodeName,
                       sSecondReplFromNodeName->value,
                       sSecondReplFromNodeName->length );
        aReplicaSetsInfo->mReplicaSets[sCount].mSecondReplFromNodeName[sSecondReplFromNodeName->length] = '\0';

        idlOS::memcpy( aReplicaSetsInfo->mReplicaSets[sCount].mSecondReplToNodeName,
                       sSecondReplToNodeName->value,
                       sSecondReplToNodeName->length );
        aReplicaSetsInfo->mReplicaSets[sCount].mSecondReplToNodeName[sSecondReplToNodeName->length] = '\0';

        sCount++;

        if ( sCount > SDI_NODE_MAX_COUNT )
        {
            IDE_RAISE( BUFFER_OVERFLOW );
        }
        else
        {
            /* Nothing to do */
        }

        IDE_TEST( sCursor.readRow( &sRow, &sRid, SMI_FIND_NEXT )
                  != IDE_SUCCESS );
    }

    sIsCursorOpen = ID_FALSE;
    IDE_TEST( sCursor.close() != IDE_SUCCESS );

    aReplicaSetsInfo->mCount = sCount;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_META_HANDLE )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDM_SHARD_META_NOT_CREATED,
                                  SDM_NODES ) );
    }
    IDE_EXCEPTION( BUFFER_OVERFLOW )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDM_SHARD_NODE_OVERFLOW ) );
    }
    IDE_EXCEPTION( ERR_NOT_EXIST_REPLICA_SET )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,"sdm::getReplicaSetsByPName","no replica set" ) );
    }
    IDE_EXCEPTION_END;

    if ( sIsCursorOpen == ID_TRUE )
    {
        (void)sCursor.close();
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_FAILURE;
}


IDE_RC sdm::getTableInfo( smiStatement * aSmiStmt,
                          SChar        * aUserName,
                          SChar        * aTableName,
                          ULong          aSMN,
                          sdiTableInfo * aTableInfo,
                          idBool       * aIsTableFound )
{
    idBool            sIsCursorOpen = ID_FALSE;
    idBool            sIsTableFound = ID_FALSE;
    const void      * sRow          = NULL;
    scGRID            sRid;
    const void      * sSdmObjects;
    const void      * sSdmObjectsIndex[SDM_MAX_META_INDICES];
    mtcColumn       * sShardIDColumn;
    mtcColumn       * sUserNameColumn;
    mtcColumn       * sObjectNameColumn;
    mtcColumn       * sObjectTypeColumn;
    mtcColumn       * sSplitMethodColumn;
    mtcColumn       * sKeyColumnNameColumn;
    mtcColumn       * sSubSplitMethodColumn;
    mtcColumn       * sSubKeyColumnNameColumn;
    mtcColumn       * sDefaultNodeIDColumn;
    mtcColumn       * sDefaultPartitionNameColumn;
    mtcColumn       * sDefaultReplicaSetIdColumn;
    mtcColumn       * sSMNColumn;

    mtdCharType     * sUserName;
    mtdCharType     * sObjectName;
    mtdCharType     * sObjectType;
    mtdCharType     * sSplitMethod;
    mtdCharType     * sKeyColumnName;
    mtdCharType     * sSubSplitMethod;
    mtdCharType     * sSubKeyColumnName;
    mtdIntegerType    sDefaultNodeID;
    mtdCharType     * sDefaultPartitionName;
    mtdIntegerType    sDefaultReplicaSetId;

    qcNameCharBuffer   sUserNameBuffer;
    mtdCharType      * sUserNameBuf = ( mtdCharType * ) & sUserNameBuffer;
    qcNameCharBuffer   sObjectNameBuffer;
    mtdCharType      * sObjectNameBuf = ( mtdCharType * ) & sObjectNameBuffer;

    qtcMetaRangeColumn sUserNameRange;
    qtcMetaRangeColumn sTableNameRange;
    qtcMetaRangeColumn sSMNRange;
    smiRange           sRange;

    smiTableCursor       sCursor;
    smiCursorProperties  sCursorProperty;

    /* init */
    SDI_INIT_TABLE_INFO( aTableInfo );

    IDE_TEST_CONT( idlOS::strlen(aUserName) > QC_MAX_OBJECT_NAME_LEN,
                   LABEL_TABLE_NOT_FOUND );
    IDE_TEST_CONT( idlOS::strlen(aTableName) > QC_MAX_OBJECT_NAME_LEN,
                   LABEL_TABLE_NOT_FOUND );

    IDE_TEST( checkMetaVersion( aSmiStmt )
              != IDE_SUCCESS );

    IDE_TEST( getMetaTableAndIndex( aSmiStmt,
                                    SDM_OBJECTS,
                                    & sSdmObjects,
                                    sSdmObjectsIndex )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( sSdmObjectsIndex[SDM_OBJECTS_IDX2_ORDER] == NULL,
                    ERR_META_HANDLE );

    IDE_TEST( smiGetTableColumns( sSdmObjects,
                                  SDM_OBJECTS_SHARD_ID_COL_ORDER,
                                  (const smiColumn**)&sShardIDColumn )
              != IDE_SUCCESS );
    IDE_TEST( smiGetTableColumns( sSdmObjects,
                                  SDM_OBJECTS_USER_NAME_COL_ORDER,
                                  (const smiColumn**)&sUserNameColumn )
              != IDE_SUCCESS );
    IDE_TEST( smiGetTableColumns( sSdmObjects,
                                  SDM_OBJECTS_OBJECT_NAME_COL_ORDER,
                                  (const smiColumn**)&sObjectNameColumn )
              != IDE_SUCCESS );
    IDE_TEST( smiGetTableColumns( sSdmObjects,
                                  SDM_OBJECTS_OBJECT_TYPE_COL_ORDER,
                                  (const smiColumn**)&sObjectTypeColumn )
              != IDE_SUCCESS );
    IDE_TEST( smiGetTableColumns( sSdmObjects,
                                  SDM_OBJECTS_SPLIT_METHOD_COL_ORDER,
                                  (const smiColumn**)&sSplitMethodColumn )
              != IDE_SUCCESS );
    IDE_TEST( smiGetTableColumns( sSdmObjects,
                                  SDM_OBJECTS_KEY_COLUMN_NAME_COL_ORDER,
                                  (const smiColumn**)&sKeyColumnNameColumn )
              != IDE_SUCCESS );
    IDE_TEST( smiGetTableColumns( sSdmObjects,
                                  SDM_OBJECTS_SUB_SPLIT_METHOD_COL_ORDER,
                                  (const smiColumn**)&sSubSplitMethodColumn )
              != IDE_SUCCESS );
    IDE_TEST( smiGetTableColumns( sSdmObjects,
                                  SDM_OBJECTS_SUB_KEY_COLUMN_NAME_COL_ORDER,
                                  (const smiColumn**)&sSubKeyColumnNameColumn )
              != IDE_SUCCESS );
    IDE_TEST( smiGetTableColumns( sSdmObjects,
                                  SDM_OBJECTS_DEFAULT_NODE_ID_COL_ORDER,
                                  (const smiColumn**)&sDefaultNodeIDColumn )
              != IDE_SUCCESS );
    IDE_TEST( smiGetTableColumns( sSdmObjects,
                                  SDM_OBJECTS_DEFAULT_PARTITION_ID_COL_ORDER,
                                  (const smiColumn**)&sDefaultPartitionNameColumn )
              != IDE_SUCCESS );

    IDE_TEST( smiGetTableColumns( sSdmObjects,
                                  SDM_OBJECTS_DEFAULT_PARTITION_REPLICA_SET_ID_COL_ORDER,
                                  (const smiColumn**)&sDefaultReplicaSetIdColumn )
              != IDE_SUCCESS );
    IDE_TEST( smiGetTableColumns( sSdmObjects,
                                  SDM_OBJECTS_SMN_COL_ORDER,
                                  (const smiColumn**)&sSMNColumn )
              != IDE_SUCCESS );

    // mtdModule 설정
    IDE_TEST( mtd::moduleById( &(sUserNameColumn->module),
                               sUserNameColumn->type.dataTypeId )
              != IDE_SUCCESS );

    // mtlModule 설정
    IDE_TEST( mtl::moduleById( &sUserNameColumn->language,
                               sUserNameColumn->type.languageId )
              != IDE_SUCCESS );

    // mtdModule 설정
    IDE_TEST( mtd::moduleById( &(sObjectNameColumn->module),
                               sObjectNameColumn->type.dataTypeId )
              != IDE_SUCCESS );

    // mtlModule 설정
    IDE_TEST( mtl::moduleById( &sObjectNameColumn->language,
                               sObjectNameColumn->type.languageId )
              != IDE_SUCCESS );

    // mtdModule 설정
    IDE_TEST( mtd::moduleById( &(sSMNColumn->module),
                               sSMNColumn->type.dataTypeId )
              != IDE_SUCCESS );

    // mtlModule 설정
    IDE_TEST( mtl::moduleById( &sSMNColumn->language,
                               sSMNColumn->type.languageId )
              != IDE_SUCCESS );

    qtc::setVarcharValue( sUserNameBuf,
                          NULL,
                          (SChar*)aUserName,
                          idlOS::strlen(aUserName) );

    qtc::setVarcharValue( sObjectNameBuf,
                          NULL,
                          (SChar*)aTableName,
                          idlOS::strlen(aTableName) );

    qcm::makeMetaRangeTripleColumn(
        &sUserNameRange,
        &sTableNameRange,
        &sSMNRange,
        (const mtcColumn *) sSMNColumn,
        (const void *) &aSMN,
        (const mtcColumn *) sUserNameColumn,
        (const void *) sUserNameBuf,
        (const mtcColumn *) sObjectNameColumn,
        (const void *) sObjectNameBuf,
        &sRange );

    sCursor.initialize();

    SMI_CURSOR_PROP_INIT_FOR_META_INDEX_SCAN( &sCursorProperty, NULL );

    IDE_TEST( sCursor.open(
                  aSmiStmt,
                  sSdmObjects,
                  sSdmObjectsIndex[SDM_OBJECTS_IDX2_ORDER],
                  smiGetRowSCN( sSdmObjects ),
                  NULL,
                  &sRange,
                  smiGetDefaultKeyRange(),
                  smiGetDefaultFilter(),
                  QCM_META_CURSOR_FLAG,
                  SMI_SELECT_CURSOR,
                  &sCursorProperty )
              != IDE_SUCCESS );

    sIsCursorOpen = ID_TRUE;

    IDE_TEST( sCursor.beforeFirst() != IDE_SUCCESS );

    IDE_TEST( sCursor.readRow( &sRow, &sRid, SMI_FIND_NEXT )
              != IDE_SUCCESS );

    IDE_TEST_CONT( sRow == NULL, LABEL_TABLE_NOT_FOUND );

    sIsTableFound = ID_TRUE;

    aTableInfo->mShardID =
        *(mtdIntegerType*)((SChar *)sRow + sShardIDColumn->column.offset );

    sUserName = (mtdCharType*)((SChar *)sRow + sUserNameColumn->column.offset );
    idlOS::memcpy( aTableInfo->mUserName, sUserName->value, sUserName->length );
    aTableInfo->mUserName[sUserName->length] = '\0';

    sObjectName = (mtdCharType*)((SChar *)sRow + sObjectNameColumn->column.offset );
    idlOS::memcpy( aTableInfo->mObjectName, sObjectName->value, sObjectName->length );
    aTableInfo->mObjectName[sObjectName->length] = '\0';

    sObjectType = (mtdCharType*)((SChar *)sRow + sObjectTypeColumn->column.offset );
    if ( sObjectType->length == 1 )
    {
        aTableInfo->mObjectType = sObjectType->value[0];
    }
    else
    {
        aTableInfo->mObjectType = 0;
    }

    sSplitMethod = (mtdCharType*)((SChar *)sRow + sSplitMethodColumn->column.offset );
    if ( idlOS::strMatch( (SChar*)sSplitMethod->value, sSplitMethod->length,
                          "H", 1 ) == 0 )
    {
        aTableInfo->mSplitMethod = SDI_SPLIT_HASH;
    }
    else if ( idlOS::strMatch( (SChar*)sSplitMethod->value, sSplitMethod->length,
                               "R", 1 ) == 0 )
    {
        aTableInfo->mSplitMethod = SDI_SPLIT_RANGE;
    }
    else if ( idlOS::strMatch( (SChar*)sSplitMethod->value, sSplitMethod->length,
                               "L", 1 ) == 0 )
    {
        aTableInfo->mSplitMethod = SDI_SPLIT_LIST;
    }
    else if ( idlOS::strMatch( (SChar*)sSplitMethod->value, sSplitMethod->length,
                               "C", 1 ) == 0 )
    {
        aTableInfo->mSplitMethod = SDI_SPLIT_CLONE;
    }
    else if ( idlOS::strMatch( (SChar*)sSplitMethod->value, sSplitMethod->length,
                               "S", 1 ) == 0 )
    {
        aTableInfo->mSplitMethod = SDI_SPLIT_SOLO;
    }
    else
    {
        ideLog::log( IDE_SD_1, "[SHARD_META_ERROR] Failure. getTableInfo : unknown SPLIT_METHOD !!!\n");
        IDE_RAISE( ERR_META_CRASH );
    }

    sKeyColumnName = (mtdCharType*)((SChar *)sRow + sKeyColumnNameColumn->column.offset );
    idlOS::memcpy( aTableInfo->mKeyColumnName, sKeyColumnName->value, sKeyColumnName->length );
    aTableInfo->mKeyColumnName[sKeyColumnName->length] = '\0';

    /* PROJ-2655 Composite shard key */
    sSubSplitMethod = (mtdCharType*)((SChar *)sRow + sSubSplitMethodColumn->column.offset );

    if ( sSubSplitMethod->length == 1 )
    {
        if ( idlOS::strMatch( (SChar*)sSubSplitMethod->value, sSubSplitMethod->length,
                              "H", 1 ) == 0 )
        {
            aTableInfo->mSubSplitMethod = SDI_SPLIT_HASH;
        }
        else if ( idlOS::strMatch( (SChar*)sSubSplitMethod->value, sSubSplitMethod->length,
                                   "R", 1 ) == 0 )
        {
            aTableInfo->mSubSplitMethod = SDI_SPLIT_RANGE;
        }
        else if ( idlOS::strMatch( (SChar*)sSubSplitMethod->value, sSubSplitMethod->length,
                                   "L", 1 ) == 0 )
        {
            aTableInfo->mSubSplitMethod = SDI_SPLIT_LIST;
        }
        else if ( idlOS::strMatch( (SChar*)sSubSplitMethod->value, sSubSplitMethod->length,
                                   "C", 1 ) == 0 )
        {
            aTableInfo->mSubSplitMethod = SDI_SPLIT_CLONE;
        }
        else if ( idlOS::strMatch( (SChar*)sSubSplitMethod->value, sSubSplitMethod->length,
                                   "S", 1 ) == 0 )
        {
            aTableInfo->mSubSplitMethod = SDI_SPLIT_SOLO;
        }
        else
        {
            ideLog::log( IDE_SD_1, "[SHARD_META_ERROR] Failure. getTableInfo : unknown SPLIT_METHOD !!!\n");
            IDE_RAISE( ERR_META_CRASH );
        }

        aTableInfo->mSubKeyExists = ID_TRUE;
    }
    else if ( sSubSplitMethod->length > 1 )
    {
        ideLog::log( IDE_SD_1, "[SHARD_META_ERROR] Failure. getTableInfo : unknown SPLIT_METHOD !!!\n");
        IDE_RAISE( ERR_META_CRASH );
    }
    else // sSubSplitMethod->length == 0
    {
        /* SDI_INIT_TABLE_INFO()에서 초기화 되어있지만, 다시 설정 */
        aTableInfo->mSubKeyExists   = ID_FALSE;
        aTableInfo->mSubSplitMethod = SDI_SPLIT_NONE;
    }

    sSubKeyColumnName = (mtdCharType*)((SChar *)sRow + sSubKeyColumnNameColumn->column.offset );
    idlOS::memcpy( aTableInfo->mSubKeyColumnName, sSubKeyColumnName->value, sSubKeyColumnName->length );
    aTableInfo->mSubKeyColumnName[sSubKeyColumnName->length] = '\0';

    sDefaultNodeID =
        *(mtdIntegerType*)((SChar *)sRow + sDefaultNodeIDColumn->column.offset );
    if ( sDefaultNodeID == MTD_INTEGER_NULL )
    {
        aTableInfo->mDefaultNodeId = SDI_NODE_NULL_ID;
    }
    else
    {
        aTableInfo->mDefaultNodeId = (UInt) sDefaultNodeID;
    }

    sDefaultPartitionName = (mtdCharType*)((SChar *)sRow + sDefaultPartitionNameColumn->column.offset );
    idlOS::memcpy( aTableInfo->mDefaultPartitionName, sDefaultPartitionName->value, sDefaultPartitionName->length );
    aTableInfo->mDefaultPartitionName[sDefaultPartitionName->length] = '\0';

    sDefaultReplicaSetId = *(mtdIntegerType*)((SChar *)sRow + sDefaultReplicaSetIdColumn->column.offset );
    aTableInfo->mDefaultPartitionReplicaSetId = (UInt) sDefaultReplicaSetId;

    IDE_EXCEPTION_CONT( LABEL_TABLE_NOT_FOUND )

    if ( sIsCursorOpen == ID_TRUE )
    {
        sIsCursorOpen = ID_FALSE;
        IDE_TEST( sCursor.close() != IDE_SUCCESS );
    }
    else
    {
        /* Nothing to do */
    }

    *aIsTableFound = sIsTableFound;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_META_HANDLE )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDM_SHARD_META_NOT_CREATED,
                                  SDM_OBJECTS ) );
    }
    IDE_EXCEPTION( ERR_META_CRASH )
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QCM_META_CRASH));
    }
    IDE_EXCEPTION_END;

    if ( sIsCursorOpen == ID_TRUE )
    {
        (void)sCursor.close();
    }
    else
    {
        /* Nothing to do */
    }

    *aIsTableFound = sIsTableFound;

    return IDE_FAILURE;
}

IDE_RC sdm::getTableInfoByID( smiStatement * aSmiStmt,
                              UInt           aShardID,
                              ULong          aSMN,
                              sdiTableInfo * aTableInfo,
                              idBool       * aIsTableFound )
{
    idBool            sIsCursorOpen = ID_FALSE;
    idBool            sIsTableFound = ID_FALSE;
    const void      * sRow          = NULL;
    scGRID            sRid;
    const void      * sSdmObjects;
    const void      * sSdmObjectsIndex[SDM_MAX_META_INDICES];
    mtcColumn       * sShardIDColumn;
    mtcColumn       * sUserNameColumn;
    mtcColumn       * sObjectNameColumn;
    mtcColumn       * sObjectTypeColumn;
    mtcColumn       * sSplitMethodColumn;
    mtcColumn       * sKeyColumnNameColumn;
    mtcColumn       * sSubSplitMethodColumn;
    mtcColumn       * sSubKeyColumnNameColumn;
    mtcColumn       * sDefaultNodeIDColumn;
    mtcColumn       * sDefaultPartitionNameColumn;
    mtcColumn       * sDefaultReplicaSetIdColumn;
    mtcColumn       * sSMNColumn;

    mtdCharType     * sUserName;
    mtdCharType     * sObjectName;
    mtdCharType     * sObjectType;
    mtdCharType     * sSplitMethod;
    mtdCharType     * sKeyColumnName;
    mtdCharType     * sSubSplitMethod;
    mtdCharType     * sSubKeyColumnName;
    mtdIntegerType    sDefaultNodeID;
    mtdCharType     * sDefaultPartitionName;
    mtdIntegerType    sDefaultReplicaSetId;

    qtcMetaRangeColumn sShardIDRange;
    qtcMetaRangeColumn sSMNRange;
    smiRange           sRange;

    smiTableCursor       sCursor;
    smiCursorProperties  sCursorProperty;

    /* init */
    SDI_INIT_TABLE_INFO( aTableInfo );

    IDE_TEST( checkMetaVersion( aSmiStmt )
              != IDE_SUCCESS );

    IDE_TEST( getMetaTableAndIndex( aSmiStmt,
                                    SDM_OBJECTS,
                                    & sSdmObjects,
                                    sSdmObjectsIndex )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( sSdmObjectsIndex[SDM_OBJECTS_IDX1_ORDER] == NULL,
                    ERR_META_HANDLE );

    IDE_TEST( smiGetTableColumns( sSdmObjects,
                                  SDM_OBJECTS_SHARD_ID_COL_ORDER,
                                  (const smiColumn**)&sShardIDColumn )
              != IDE_SUCCESS );
    IDE_TEST( smiGetTableColumns( sSdmObjects,
                                  SDM_OBJECTS_USER_NAME_COL_ORDER,
                                  (const smiColumn**)&sUserNameColumn )
              != IDE_SUCCESS );
    IDE_TEST( smiGetTableColumns( sSdmObjects,
                                  SDM_OBJECTS_OBJECT_NAME_COL_ORDER,
                                  (const smiColumn**)&sObjectNameColumn )
              != IDE_SUCCESS );
    IDE_TEST( smiGetTableColumns( sSdmObjects,
                                  SDM_OBJECTS_OBJECT_TYPE_COL_ORDER,
                                  (const smiColumn**)&sObjectTypeColumn )
              != IDE_SUCCESS );
    IDE_TEST( smiGetTableColumns( sSdmObjects,
                                  SDM_OBJECTS_SPLIT_METHOD_COL_ORDER,
                                  (const smiColumn**)&sSplitMethodColumn )
              != IDE_SUCCESS );
    IDE_TEST( smiGetTableColumns( sSdmObjects,
                                  SDM_OBJECTS_KEY_COLUMN_NAME_COL_ORDER,
                                  (const smiColumn**)&sKeyColumnNameColumn )
              != IDE_SUCCESS );
    IDE_TEST( smiGetTableColumns( sSdmObjects,
                                  SDM_OBJECTS_SUB_SPLIT_METHOD_COL_ORDER,
                                  (const smiColumn**)&sSubSplitMethodColumn )
              != IDE_SUCCESS );
    IDE_TEST( smiGetTableColumns( sSdmObjects,
                                  SDM_OBJECTS_SUB_KEY_COLUMN_NAME_COL_ORDER,
                                  (const smiColumn**)&sSubKeyColumnNameColumn )
              != IDE_SUCCESS );
    IDE_TEST( smiGetTableColumns( sSdmObjects,
                                  SDM_OBJECTS_DEFAULT_NODE_ID_COL_ORDER,
                                  (const smiColumn**)&sDefaultNodeIDColumn )
              != IDE_SUCCESS );
    IDE_TEST( smiGetTableColumns( sSdmObjects,
                                  SDM_OBJECTS_DEFAULT_PARTITION_ID_COL_ORDER,
                                  (const smiColumn**)&sDefaultPartitionNameColumn )
              != IDE_SUCCESS );
    IDE_TEST( smiGetTableColumns( sSdmObjects,
                                  SDM_OBJECTS_DEFAULT_PARTITION_REPLICA_SET_ID_COL_ORDER,
                                  (const smiColumn**)&sDefaultReplicaSetIdColumn )
              != IDE_SUCCESS );
    IDE_TEST( smiGetTableColumns( sSdmObjects,
                                  SDM_OBJECTS_SMN_COL_ORDER,
                                  (const smiColumn**)&sSMNColumn )
              != IDE_SUCCESS );

    // mtdModule 설정
    IDE_TEST( mtd::moduleById( &(sShardIDColumn->module),
                               sShardIDColumn->type.dataTypeId )
              != IDE_SUCCESS );

    // mtlModule 설정
    IDE_TEST( mtl::moduleById( &sShardIDColumn->language,
                               sShardIDColumn->type.languageId )
              != IDE_SUCCESS );

    // mtdModule 설정
    IDE_TEST( mtd::moduleById( &(sSMNColumn->module),
                               sSMNColumn->type.dataTypeId )
              != IDE_SUCCESS );

    // mtlModule 설정
    IDE_TEST( mtl::moduleById( &sSMNColumn->language,
                               sSMNColumn->type.languageId )
              != IDE_SUCCESS );

    qcm::makeMetaRangeDoubleColumn(
        &sShardIDRange,
        &sSMNRange,
        (const mtcColumn *) sShardIDColumn,
        (const void *) &aShardID,
        (const mtcColumn *) sSMNColumn,
        (const void *) &aSMN,
        &sRange );

    sCursor.initialize();

    SMI_CURSOR_PROP_INIT_FOR_META_INDEX_SCAN( &sCursorProperty, NULL );

    IDE_TEST( sCursor.open(
                  aSmiStmt,
                  sSdmObjects,
                  sSdmObjectsIndex[SDM_OBJECTS_IDX1_ORDER],
                  smiGetRowSCN( sSdmObjects ),
                  NULL,
                  &sRange,
                  smiGetDefaultKeyRange(),
                  smiGetDefaultFilter(),
                  QCM_META_CURSOR_FLAG,
                  SMI_SELECT_CURSOR,
                  &sCursorProperty )
              != IDE_SUCCESS );

    sIsCursorOpen = ID_TRUE;

    IDE_TEST( sCursor.beforeFirst() != IDE_SUCCESS );

    IDE_TEST( sCursor.readRow( &sRow, &sRid, SMI_FIND_NEXT )
              != IDE_SUCCESS );

    IDE_TEST_CONT( sRow == NULL, LABEL_TABLE_NOT_FOUND );

    sIsTableFound = ID_TRUE;

    aTableInfo->mShardID =
        *(mtdIntegerType*)((SChar *)sRow + sShardIDColumn->column.offset );

    sUserName = (mtdCharType*)((SChar *)sRow + sUserNameColumn->column.offset );
    idlOS::memcpy( aTableInfo->mUserName, sUserName->value, sUserName->length );
    aTableInfo->mUserName[sUserName->length] = '\0';

    sObjectName = (mtdCharType*)((SChar *)sRow + sObjectNameColumn->column.offset );
    idlOS::memcpy( aTableInfo->mObjectName, sObjectName->value, sObjectName->length );
    aTableInfo->mObjectName[sObjectName->length] = '\0';

    sObjectType = (mtdCharType*)((SChar *)sRow + sObjectTypeColumn->column.offset );
    if ( sObjectType->length == 1 )
    {
        aTableInfo->mObjectType = sObjectType->value[0];
    }
    else
    {
        aTableInfo->mObjectType = 0;
    }

    sSplitMethod = (mtdCharType*)((SChar *)sRow + sSplitMethodColumn->column.offset );
    if ( idlOS::strMatch( (SChar*)sSplitMethod->value, sSplitMethod->length,
                          "H", 1 ) == 0 )
    {
        aTableInfo->mSplitMethod = SDI_SPLIT_HASH;
    }
    else if ( idlOS::strMatch( (SChar*)sSplitMethod->value, sSplitMethod->length,
                               "R", 1 ) == 0 )
    {
        aTableInfo->mSplitMethod = SDI_SPLIT_RANGE;
    }
    else if ( idlOS::strMatch( (SChar*)sSplitMethod->value, sSplitMethod->length,
                               "L", 1 ) == 0 )
    {
        aTableInfo->mSplitMethod = SDI_SPLIT_LIST;
    }
    else if ( idlOS::strMatch( (SChar*)sSplitMethod->value, sSplitMethod->length,
                               "C", 1 ) == 0 )
    {
        aTableInfo->mSplitMethod = SDI_SPLIT_CLONE;
    }
    else if ( idlOS::strMatch( (SChar*)sSplitMethod->value, sSplitMethod->length,
                               "S", 1 ) == 0 )
    {
        aTableInfo->mSplitMethod = SDI_SPLIT_SOLO;
    }
    else
    {
        ideLog::log( IDE_SD_1, "[SHARD_META_ERROR] Failure. getTableInfo : unknown SPLIT_METHOD !!!\n");
        IDE_RAISE( ERR_META_CRASH );
    }

    sKeyColumnName = (mtdCharType*)((SChar *)sRow + sKeyColumnNameColumn->column.offset );
    idlOS::memcpy( aTableInfo->mKeyColumnName, sKeyColumnName->value, sKeyColumnName->length );
    aTableInfo->mKeyColumnName[sKeyColumnName->length] = '\0';

    /* PROJ-2655 Composite shard key */
    sSubSplitMethod = (mtdCharType*)((SChar *)sRow + sSubSplitMethodColumn->column.offset );

    if ( sSubSplitMethod->length == 1 )
    {
        if ( idlOS::strMatch( (SChar*)sSubSplitMethod->value, sSubSplitMethod->length,
                              "H", 1 ) == 0 )
        {
            aTableInfo->mSubSplitMethod = SDI_SPLIT_HASH;
        }
        else if ( idlOS::strMatch( (SChar*)sSubSplitMethod->value, sSubSplitMethod->length,
                                   "R", 1 ) == 0 )
        {
            aTableInfo->mSubSplitMethod = SDI_SPLIT_RANGE;
        }
        else if ( idlOS::strMatch( (SChar*)sSubSplitMethod->value, sSubSplitMethod->length,
                                   "L", 1 ) == 0 )
        {
            aTableInfo->mSubSplitMethod = SDI_SPLIT_LIST;
        }
        else if ( idlOS::strMatch( (SChar*)sSubSplitMethod->value, sSubSplitMethod->length,
                                   "C", 1 ) == 0 )
        {
            aTableInfo->mSubSplitMethod = SDI_SPLIT_CLONE;
        }
        else if ( idlOS::strMatch( (SChar*)sSubSplitMethod->value, sSubSplitMethod->length,
                                   "S", 1 ) == 0 )
        {
            aTableInfo->mSubSplitMethod = SDI_SPLIT_SOLO;
        }
        else
        {
            ideLog::log( IDE_SD_1, "[SHARD_META_ERROR] Failure. getTableInfo : unknown SPLIT_METHOD !!!\n");
            IDE_RAISE( ERR_META_CRASH );
        }

        aTableInfo->mSubKeyExists = ID_TRUE;
    }
    else if ( sSubSplitMethod->length > 1 )
    {
        ideLog::log( IDE_SD_1, "[SHARD_META_ERROR] Failure. getTableInfo : unknown SPLIT_METHOD !!!\n");
        IDE_RAISE( ERR_META_CRASH );
    }
    else // sSubSplitMethod->length == 0
    {
        /* SDI_INIT_TABLE_INFO()에서 초기화 되어있지만, 다시 설정 */
        aTableInfo->mSubKeyExists   = ID_FALSE;
        aTableInfo->mSubSplitMethod = SDI_SPLIT_NONE;
    }

    sSubKeyColumnName = (mtdCharType*)((SChar *)sRow + sSubKeyColumnNameColumn->column.offset );
    idlOS::memcpy( aTableInfo->mSubKeyColumnName, sSubKeyColumnName->value, sSubKeyColumnName->length );
    aTableInfo->mSubKeyColumnName[sSubKeyColumnName->length] = '\0';

    sDefaultNodeID =
        *(mtdIntegerType*)((SChar *)sRow + sDefaultNodeIDColumn->column.offset );
    if ( sDefaultNodeID == MTD_INTEGER_NULL )
    {
        aTableInfo->mDefaultNodeId = SDI_NODE_NULL_ID;
    }
    else
    {
        aTableInfo->mDefaultNodeId = (UInt) sDefaultNodeID;
    }

    sDefaultPartitionName = (mtdCharType*)((SChar *)sRow + sDefaultPartitionNameColumn->column.offset );
    idlOS::memcpy( aTableInfo->mDefaultPartitionName, sDefaultPartitionName->value, sDefaultPartitionName->length );
    aTableInfo->mDefaultPartitionName[sDefaultPartitionName->length] = '\0';

    sDefaultReplicaSetId = *(mtdIntegerType*)((SChar *)sRow + sDefaultReplicaSetIdColumn->column.offset );
    aTableInfo->mDefaultPartitionReplicaSetId = (UInt) sDefaultReplicaSetId;

    IDE_EXCEPTION_CONT( LABEL_TABLE_NOT_FOUND )

    if ( sIsCursorOpen == ID_TRUE )
    {
        sIsCursorOpen = ID_FALSE;
        IDE_TEST( sCursor.close() != IDE_SUCCESS );
    }
    else
    {
        /* Nothing to do */
    }

    *aIsTableFound = sIsTableFound;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_META_HANDLE )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDM_SHARD_META_NOT_CREATED,
                                  SDM_OBJECTS ) );
    }
    IDE_EXCEPTION( ERR_META_CRASH )
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QCM_META_CRASH));
    }
    IDE_EXCEPTION_END;

    if ( sIsCursorOpen == ID_TRUE )
    {
        (void)sCursor.close();
    }
    else
    {
        /* Nothing to do */
    }

    *aIsTableFound = sIsTableFound;

    return IDE_FAILURE;
}


IDE_RC sdm::getRangeInfo( qcStatement  * aStatement,
                          smiStatement * aSmiStmt,
                          ULong          aSMN,
                          sdiTableInfo * aTableInfo,
                          sdiRangeInfo * aRangeInfo,
                          idBool         aNeedMerge,
                          idBool         aNeedReplicaSet )
{
    switch ( aTableInfo->mSplitMethod )
    {
        case SDI_SPLIT_HASH:
        case SDI_SPLIT_RANGE:
        case SDI_SPLIT_LIST:
            IDE_TEST( getRange( aStatement,
                                aSmiStmt,
                                aSMN,
                                aTableInfo,
                                aRangeInfo,
                                aNeedMerge,
                                aNeedReplicaSet )
                      != IDE_SUCCESS );
            break;
        case SDI_SPLIT_CLONE:
            IDE_TEST( getClone( aStatement,
                                aSmiStmt,
                                aSMN,
                                aTableInfo,
                                aRangeInfo )
                      != IDE_SUCCESS );
            break;
        case SDI_SPLIT_SOLO:
            IDE_TEST( getSolo( aStatement,
                               aSmiStmt,
                               aSMN,
                               aTableInfo,
                               aRangeInfo,
                               aNeedMerge )
                      != IDE_SUCCESS );
            break;
        default:
            IDE_DASSERT( 0 );
            break;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdm::getRange( qcStatement  * aStatement,
                      smiStatement * aSmiStmt,
                      ULong          aSMN,
                      sdiTableInfo * aTableInfo,
                      sdiRangeInfo * aRangeInfo,
                      idBool         aNeedMerge,
                      idBool         aNeedReplicaSet )
{
    idBool            sIsCursorOpen = ID_FALSE;
    const void      * sRow          = NULL;
    scGRID            sRid;
    const void      * sSdmRanges;    
    const void      * sSdmRangesIndex[SDM_MAX_META_INDICES];
    mtcColumn       * sShardIDColumn;
    mtcColumn       * sPartitionNameColumn;
    mtcColumn       * sValueColumn;
    mtcColumn       * sSubValueColumn;
    mtcColumn       * sNodeIDColumn;
    mtcColumn       * sSMNColumn;
    mtcColumn       * sReplicaSetIdColumn;

    qtcMetaRangeColumn sShardIDRange;
    qtcMetaRangeColumn sSMNRange;
    smiRange           sRange;

    UInt              sCount = 0;

    mtdCharType     * sPartitionName;
    mtdCharType     * sValue;
    mtdCharType     * sSubValue;
    mtdIntegerType    sNodeID;
    mtdIntegerType    sReplicaSetId;

    smiTableCursor       sCursor;
    smiCursorProperties  sCursorProperty;

    /* init */
    aRangeInfo->mCount = 0;
    aRangeInfo->mRanges = NULL;

    IDE_TEST( checkMetaVersion( aSmiStmt )
              != IDE_SUCCESS );

    IDE_TEST( getMetaTableAndIndex( aSmiStmt,
                                    SDM_RANGES,
                                    & sSdmRanges,
                                    sSdmRangesIndex )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( sSdmRangesIndex[SDM_RANGES_IDX1_ORDER] == NULL,
                    ERR_META_HANDLE );

    IDE_TEST( smiGetTableColumns( sSdmRanges,
                                  SDM_RANGES_SHARD_ID_COL_ORDER,
                                  (const smiColumn**)&sShardIDColumn )
              != IDE_SUCCESS );
    IDE_TEST( smiGetTableColumns( sSdmRanges,
                                  SDM_RANGES_PARTITION_NAME_COL_ORDER,
                                  (const smiColumn**)&sPartitionNameColumn )
              != IDE_SUCCESS );
    IDE_TEST( smiGetTableColumns( sSdmRanges,
                                  SDM_RANGES_VALUE_ID_COL_ORDER,
                                  (const smiColumn**)&sValueColumn )
              != IDE_SUCCESS );
    IDE_TEST( smiGetTableColumns( sSdmRanges,
                                  SDM_RANGES_SUB_VALUE_ID_COL_ORDER,
                                  (const smiColumn**)&sSubValueColumn )
              != IDE_SUCCESS );
    IDE_TEST( smiGetTableColumns( sSdmRanges,
                                  SDM_RANGES_NODE_ID_COL_ORDER,
                                  (const smiColumn**)&sNodeIDColumn )
              != IDE_SUCCESS );
    IDE_TEST( smiGetTableColumns( sSdmRanges,
                                  SDM_RANGES_SMN_COL_ORDER,
                                  (const smiColumn**)&sSMNColumn )
              != IDE_SUCCESS );
    IDE_TEST( smiGetTableColumns( sSdmRanges,
                                  SDM_RANGES_REPLICA_SET_ID_COL_ORDER,
                                  (const smiColumn**)&sReplicaSetIdColumn )
              != IDE_SUCCESS );

    // mtdModule 설정
    IDE_TEST( mtd::moduleById( &(sShardIDColumn->module),
                               sShardIDColumn->type.dataTypeId )
              != IDE_SUCCESS );

    // mtlModule 설정
    IDE_TEST( mtl::moduleById( &sShardIDColumn->language,
                               sShardIDColumn->type.languageId )
              != IDE_SUCCESS );

    // mtdModule 설정
    IDE_TEST( mtd::moduleById( &(sSMNColumn->module),
                               sSMNColumn->type.dataTypeId )
              != IDE_SUCCESS );

    // mtlModule 설정
    IDE_TEST( mtl::moduleById( &sSMNColumn->language,
                               sSMNColumn->type.languageId )
              != IDE_SUCCESS );

    qciMisc::makeMetaRangeDoubleColumn(
        &sShardIDRange,
        &sSMNRange,
        sShardIDColumn,
        (const void *)&(aTableInfo->mShardID),
        sSMNColumn,
        (const void *)&aSMN,
        &sRange );

    sCursor.initialize();

    SMI_CURSOR_PROP_INIT_FOR_META_INDEX_SCAN( &sCursorProperty, NULL );

    IDE_TEST( sCursor.open(
                  aSmiStmt,
                  sSdmRanges,
                  sSdmRangesIndex[SDM_RANGES_IDX1_ORDER],
                  smiGetRowSCN( sSdmRanges ),
                  NULL,
                  &sRange,
                  smiGetDefaultKeyRange(),
                  smiGetDefaultFilter(),
                  QCM_META_CURSOR_FLAG,
                  SMI_SELECT_CURSOR,
                  &sCursorProperty )
              != IDE_SUCCESS );

    sIsCursorOpen = ID_TRUE;

    IDE_TEST( sCursor.beforeFirst() != IDE_SUCCESS );

    IDE_TEST( sCursor.readRow( &sRow, &sRid, SMI_FIND_NEXT )
              != IDE_SUCCESS );

    // BUG-45718
    // Get record count
    while ( sRow != NULL )
    {
        sCount++;

        IDE_TEST( sCursor.readRow( &sRow, &sRid, SMI_FIND_NEXT )
                  != IDE_SUCCESS );
    }

    // Alloc memory as much as record count
    if ( sCount > 0 )
    {
        IDE_TEST_RAISE( sCount > SDI_RANGE_MAX_COUNT, RANGE_COUNT_OVERFLOW );

        IDE_TEST( QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF(sdiRange) * sCount,
                                                 (void**) & aRangeInfo->mRanges )
                  != IDE_SUCCESS );

        IDE_TEST( sCursor.beforeFirst() != IDE_SUCCESS );

        IDE_TEST( sCursor.readRow( &sRow, &sRid, SMI_FIND_NEXT )
                  != IDE_SUCCESS );

        // Set ranges
        while ( sRow != NULL )
        {
            sPartitionName = (mtdCharType*)((SChar *)sRow + sPartitionNameColumn->column.offset );
            sValue = (mtdCharType*)((SChar *)sRow + sValueColumn->column.offset );
            sSubValue = (mtdCharType*)((SChar *)sRow + sSubValueColumn->column.offset );
            sNodeID  = *(mtdIntegerType*)((SChar *)sRow + sNodeIDColumn->column.offset );
            sReplicaSetId = *(mtdIntegerType*)((SChar *)sRow + sReplicaSetIdColumn->column.offset );

            IDE_TEST( mtd::moduleById( &(sPartitionNameColumn->module),
                                       sPartitionNameColumn->type.dataTypeId )
                      != IDE_SUCCESS );

            if ( sPartitionNameColumn->module->isNull(sPartitionNameColumn, sPartitionName) != ID_TRUE )
            {
                IDE_TEST_RAISE( sPartitionName->length > QC_MAX_OBJECT_NAME_LEN, ERR_PART_NAME_OVERFLOW );
                idlOS::memcpy( aRangeInfo->mRanges[aRangeInfo->mCount].mPartitionName, sPartitionName->value, sPartitionName->length );
                aRangeInfo->mRanges[aRangeInfo->mCount].mPartitionName[sPartitionName->length] = '\0';
            }
            else
            {
                aRangeInfo->mRanges[aRangeInfo->mCount].mPartitionName[0] = '\0';
            }
            aRangeInfo->mRanges[aRangeInfo->mCount].mNodeId       = (UInt)sNodeID;
            aRangeInfo->mRanges[aRangeInfo->mCount].mReplicaSetId = (UInt)sReplicaSetId;

            IDE_TEST_CONT( aNeedReplicaSet == ID_TRUE, SKIP_RANGE_VALUE );

            // shard key의 range value string을 알맞은 data type으로 변환
            if ( aTableInfo->mSplitMethod == SDI_SPLIT_HASH )
            {
                // hash는 integer로 변환
                IDE_TEST( convertRangeValue( (SChar*)sValue->value,
                                             sValue->length,
                                             MTD_INTEGER_ID,
                                             &(aRangeInfo->mRanges[aRangeInfo->mCount].mValue) )
                          != IDE_SUCCESS );
            }
            else
            {
                // range, list는 해당 key type으로 변환
                IDE_TEST( convertRangeValue( (SChar*)sValue->value,
                                             sValue->length,
                                             aTableInfo->mKeyDataType,
                                             &(aRangeInfo->mRanges[aRangeInfo->mCount].mValue) )
                          != IDE_SUCCESS );
            }

            // sub-shard key의 range value string을 알맞은 data type으로 변환
            if ( aTableInfo->mSubKeyExists == ID_TRUE )
            {
                if ( aTableInfo->mSubSplitMethod == SDI_SPLIT_HASH )
                {
                    // hash는 integer로 변환
                    IDE_TEST( convertRangeValue( (SChar*)sSubValue->value,
                                                 sSubValue->length,
                                                 MTD_INTEGER_ID,
                                                 &(aRangeInfo->mRanges[aRangeInfo->mCount].mSubValue) )
                              != IDE_SUCCESS );
                }
                else
                {
                    // range, list는 해당 key type으로 변환
                    IDE_TEST( convertRangeValue( (SChar*)sSubValue->value,
                                                 sSubValue->length,
                                                 aTableInfo->mSubKeyDataType,
                                                 &(aRangeInfo->mRanges[aRangeInfo->mCount].mSubValue) )
                              != IDE_SUCCESS );
                }
            }
            else
            {
                /* Nothing to do. */
            }

            IDE_EXCEPTION_CONT( SKIP_RANGE_VALUE );

            aRangeInfo->mCount++;

            IDE_TEST( sCursor.readRow( &sRow, &sRid, SMI_FIND_NEXT )
                      != IDE_SUCCESS );
        }
    }
    else
    {
        // Nothing to do.
    }

    sIsCursorOpen = ID_FALSE;
    IDE_TEST( sCursor.close() != IDE_SUCCESS );

    IDE_TEST_CONT( aNeedReplicaSet == ID_TRUE, NORMAL_EXIT );

    // 숫자 타입으로 변환된 string의 경우
    // index를 이용한 정렬이 올바르지 않아 추가 정렬이 필요하다.
    IDE_TEST( shardRangeSort( aTableInfo->mSplitMethod,
                              aTableInfo->mKeyDataType,
                              aTableInfo->mSubKeyExists,
                              aTableInfo->mSubSplitMethod,
                              aTableInfo->mSubKeyDataType,
                              aRangeInfo )
              != IDE_SUCCESS );

    if ( aNeedMerge == ID_TRUE )
    {
        // 정렬 후 중복된 분산 정의를 합쳐준다.
        IDE_TEST( shardEliminateDuplication( aTableInfo,
                                             aRangeInfo )
                  != IDE_SUCCESS );
    }

    IDE_EXCEPTION_CONT( NORMAL_EXIT );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_META_HANDLE )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDM_SHARD_META_NOT_CREATED,
                                  SDM_RANGES ) );
    }
    IDE_EXCEPTION( RANGE_COUNT_OVERFLOW )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDM_SHARD_RANGE_OVERFLOW ) );
    }
    IDE_EXCEPTION( ERR_PART_NAME_OVERFLOW )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR , "sdm::getRange", "partition name length overflow") );
    }
    IDE_EXCEPTION_END;

    if ( sIsCursorOpen == ID_TRUE )
    {
        (void)sCursor.close();
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_FAILURE;
}

IDE_RC sdm::getClone( qcStatement  * aStatement,
                      smiStatement * aSmiStmt,
                      ULong          aSMN,
                      sdiTableInfo * aTableInfo,
                      sdiRangeInfo * aRangeInfo )
{
    idBool               sIsCursorOpen = ID_FALSE;
    const void         * sRow          = NULL;
    scGRID               sRid;
    const void         * sSdmClone;
    const void         * sSdmCloneIndex[SDM_MAX_META_INDICES];
    mtcColumn          * sShardIDColumn;

    mtcColumn          * sNodeIDColumn;
    mtcColumn          * sSMNColumn;
    mtcColumn          * sReplicaSetIdColumn;

    qtcMetaRangeColumn   sShardIDRange;
    qtcMetaRangeColumn   sSMNRange;
    smiRange             sRange;

    UInt                 sCount    = 0;

    mtdIntegerType       sNodeID;
    mtdIntegerType       sReplicaSetId;

    smiTableCursor       sCursor;
    smiCursorProperties  sCursorProperty;

    /* init */
    aRangeInfo->mCount = 0;
    aRangeInfo->mRanges = NULL;

    IDE_TEST( checkMetaVersion( aSmiStmt )
              != IDE_SUCCESS );

    IDE_TEST( getMetaTableAndIndex( aSmiStmt,
                                    SDM_CLONES,
                                    & sSdmClone,
                                    sSdmCloneIndex )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( sSdmCloneIndex[SDM_CLONES_IDX1_ORDER] == NULL,
                    ERR_META_HANDLE );

    IDE_TEST( smiGetTableColumns( sSdmClone,
                                  SDM_CLONES_SHARD_ID_COL_ORDER,
                                  (const smiColumn**)&sShardIDColumn )
              != IDE_SUCCESS );

    IDE_TEST( smiGetTableColumns( sSdmClone,
                                  SDM_CLONES_NODE_ID_COL_ORDER,
                                  (const smiColumn**)&sNodeIDColumn )
              != IDE_SUCCESS );

    IDE_TEST( smiGetTableColumns( sSdmClone,
                                  SDM_CLONES_SMN_COL_ORDER,
                                  (const smiColumn**)&sSMNColumn )
              != IDE_SUCCESS );

    IDE_TEST( smiGetTableColumns( sSdmClone,
                                  SDM_CLONES_REPLICA_SET_ID_COL_ORDER,
                                  (const smiColumn**)&sReplicaSetIdColumn )
              != IDE_SUCCESS );

    // mtdModule 설정
    IDE_TEST( mtd::moduleById( &(sShardIDColumn->module),
                               sShardIDColumn->type.dataTypeId )
              != IDE_SUCCESS );

    // mtlModule 설정
    IDE_TEST( mtl::moduleById( &sShardIDColumn->language,
                               sShardIDColumn->type.languageId )
              != IDE_SUCCESS );

    // mtdModule 설정
    IDE_TEST( mtd::moduleById( &(sSMNColumn->module),
                               sSMNColumn->type.dataTypeId )
              != IDE_SUCCESS );

    // mtlModule 설정
    IDE_TEST( mtl::moduleById( &sSMNColumn->language,
                               sSMNColumn->type.languageId )
              != IDE_SUCCESS );

    qciMisc::makeMetaRangeDoubleColumn(
        &sShardIDRange,
        &sSMNRange,
        sShardIDColumn,
        (const void *)&(aTableInfo->mShardID),
        sSMNColumn,
        (const void *)&aSMN,
        &sRange );

    sCursor.initialize();

    SMI_CURSOR_PROP_INIT_FOR_META_INDEX_SCAN( &sCursorProperty, NULL );

    IDE_TEST( sCursor.open(
                  aSmiStmt,
                  sSdmClone,
                  sSdmCloneIndex[SDM_CLONES_IDX1_ORDER],
                  smiGetRowSCN( sSdmClone ),
                  NULL,
                  &sRange,
                  smiGetDefaultKeyRange(),
                  smiGetDefaultFilter(),
                  QCM_META_CURSOR_FLAG,
                  SMI_SELECT_CURSOR,
                  &sCursorProperty )
              != IDE_SUCCESS );

    sIsCursorOpen = ID_TRUE;

    IDE_TEST( sCursor.beforeFirst() != IDE_SUCCESS );

    IDE_TEST( sCursor.readRow( &sRow, &sRid, SMI_FIND_NEXT )
              != IDE_SUCCESS );

    // BUG-45718
    // Get record count
    while ( sRow != NULL )
    {
        sCount++;

        IDE_TEST( sCursor.readRow( &sRow, &sRid, SMI_FIND_NEXT )
                  != IDE_SUCCESS );
    }

    if ( sCount > 0 )
    {
        IDE_TEST_RAISE( sCount > SDI_RANGE_MAX_COUNT, RANGE_COUNT_OVERFLOW );

        // Alloc memory as much as record count
        IDE_TEST( QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF(sdiRange) * sCount,
                                                 (void**) & aRangeInfo->mRanges )
                  != IDE_SUCCESS );

        IDE_TEST( sCursor.beforeFirst() != IDE_SUCCESS );

        IDE_TEST( sCursor.readRow( &sRow, &sRid, SMI_FIND_NEXT )
                  != IDE_SUCCESS );

        // Set ranges
        while ( sRow != NULL )
        {
            sNodeID        = *(mtdIntegerType*)((SChar *)sRow + sNodeIDColumn->column.offset );
            sReplicaSetId  = *(mtdIntegerType*)((SChar *)sRow + sReplicaSetIdColumn->column.offset );

            aRangeInfo->mRanges[aRangeInfo->mCount].mNodeId = (UInt)sNodeID;
            // Clone table 에서 partition name 및 hash value는 의미 없다. '\0'과 max로 설정
            aRangeInfo->mRanges[aRangeInfo->mCount].mPartitionName[0] = '\0';
            aRangeInfo->mRanges[aRangeInfo->mCount].mValue.mHashMax = (UInt)SDI_RANGE_MAX_COUNT;
            aRangeInfo->mRanges[aRangeInfo->mCount].mReplicaSetId = (UInt)sReplicaSetId;

            aRangeInfo->mCount++;

            IDE_TEST( sCursor.readRow( &sRow, &sRid, SMI_FIND_NEXT )
                      != IDE_SUCCESS );
        }
    }
    else
    {
        // Nothing to do.
    }

    sIsCursorOpen = ID_FALSE;
    IDE_TEST( sCursor.close() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_META_HANDLE )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDM_SHARD_META_NOT_CREATED,
                                  SDM_CLONES ) );
    }
    IDE_EXCEPTION( RANGE_COUNT_OVERFLOW )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDM_SHARD_RANGE_OVERFLOW ) );
    }
    IDE_EXCEPTION_END;

    if ( sIsCursorOpen == ID_TRUE )
    {
        (void)sCursor.close();
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_FAILURE;
}

IDE_RC sdm::getSolo( qcStatement  * aStatement,
                     smiStatement * aSmiStmt,
                     ULong          aSMN,
                     sdiTableInfo * aTableInfo,
                     sdiRangeInfo * aRangeInfo,
                     idBool         aNeedMerge )
{
    idBool               sIsCursorOpen = ID_FALSE;
    const void         * sRow          = NULL;
    scGRID               sRid;
    const void         * sSdmSolo;
    const void         * sSdmSoloIndex[SDM_MAX_META_INDICES];
    mtcColumn          * sShardIDColumn;

    mtcColumn          * sNodeIDColumn;
    mtcColumn          * sSMNColumn;
    mtcColumn          * sReplicaSetIdColumn;

    qtcMetaRangeColumn   sShardIDRange;
    qtcMetaRangeColumn   sSMNRange;
    smiRange             sRange;

    UInt                 sCount    = 0;

    mtdIntegerType       sNodeID;
    mtdIntegerType       sReplicaSetId;

    smiTableCursor       sCursor;
    smiCursorProperties  sCursorProperty;

    /* init */
    aRangeInfo->mCount = 0;
    aRangeInfo->mRanges = NULL;

    IDE_TEST( getMetaTableAndIndex( aSmiStmt,
                                    SDM_SOLOS,
                                    & sSdmSolo,
                                    sSdmSoloIndex )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( sSdmSoloIndex[SDM_SOLOS_IDX1_ORDER] == NULL,
                    ERR_META_HANDLE );

    IDE_TEST( smiGetTableColumns( sSdmSolo,
                                  SDM_SOLOS_SHARD_ID_COL_ORDER,
                                  (const smiColumn**)&sShardIDColumn )
              != IDE_SUCCESS );

    IDE_TEST( smiGetTableColumns( sSdmSolo,
                                  SDM_SOLOS_NODE_ID_COL_ORDER,
                                  (const smiColumn**)&sNodeIDColumn )
              != IDE_SUCCESS );

    IDE_TEST( smiGetTableColumns( sSdmSolo,
                                  SDM_SOLOS_SMN_COL_ORDER,
                                  (const smiColumn**)&sSMNColumn )
              != IDE_SUCCESS );

    IDE_TEST( smiGetTableColumns( sSdmSolo,
                                  SDM_SOLOS_REPLICA_SET_ID_COL_ORDER,
                                  (const smiColumn**)&sReplicaSetIdColumn )
              != IDE_SUCCESS );

    // mtdModule 설정
    IDE_TEST( mtd::moduleById( &(sShardIDColumn->module),
                               sShardIDColumn->type.dataTypeId )
              != IDE_SUCCESS );

    // mtlModule 설정
    IDE_TEST( mtl::moduleById( &sShardIDColumn->language,
                               sShardIDColumn->type.languageId )
              != IDE_SUCCESS );

    // mtdModule 설정
    IDE_TEST( mtd::moduleById( &(sSMNColumn->module),
                               sSMNColumn->type.dataTypeId )
              != IDE_SUCCESS );

    // mtlModule 설정
    IDE_TEST( mtl::moduleById( &sSMNColumn->language,
                               sSMNColumn->type.languageId )
              != IDE_SUCCESS );

    qciMisc::makeMetaRangeDoubleColumn(
        &sShardIDRange,
        &sSMNRange,
        sShardIDColumn,
        (const void *)&(aTableInfo->mShardID),
        sSMNColumn,
        (const void *)&aSMN,
        &sRange );

    sCursor.initialize();

    SMI_CURSOR_PROP_INIT_FOR_META_INDEX_SCAN( &sCursorProperty, NULL );

    IDE_TEST( sCursor.open(
                  aSmiStmt,
                  sSdmSolo,
                  sSdmSoloIndex[SDM_SOLOS_IDX1_ORDER],
                  smiGetRowSCN( sSdmSolo ),
                  NULL,
                  &sRange,
                  smiGetDefaultKeyRange(),
                  smiGetDefaultFilter(),
                  QCM_META_CURSOR_FLAG,
                  SMI_SELECT_CURSOR,
                  &sCursorProperty )
              != IDE_SUCCESS );

    sIsCursorOpen = ID_TRUE;

    IDE_TEST( sCursor.beforeFirst() != IDE_SUCCESS );

    IDE_TEST( sCursor.readRow( &sRow, &sRid, SMI_FIND_NEXT )
              != IDE_SUCCESS );

    // BUG-45718
    // Get record count
    while ( sRow != NULL )
    {
        sCount++;

        IDE_TEST( sCursor.readRow( &sRow, &sRid, SMI_FIND_NEXT )
                  != IDE_SUCCESS );
    }

    if ( sCount > 0 )
    {
        IDE_TEST_RAISE( sCount > 1, RANGE_COUNT_OVERFLOW );

        // Alloc memory as much as record count
        IDE_TEST( QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF(sdiRange) * sCount,
                                                 (void**) & aRangeInfo->mRanges )
                  != IDE_SUCCESS );

        IDE_TEST( sCursor.beforeFirst() != IDE_SUCCESS );

        IDE_TEST( sCursor.readRow( &sRow, &sRid, SMI_FIND_NEXT )
                  != IDE_SUCCESS );

        // Set ranges
        while ( sRow != NULL )
        {
            sNodeID  = *(mtdIntegerType*)((SChar *)sRow + sNodeIDColumn->column.offset );
            sReplicaSetId  = *(mtdIntegerType*)((SChar *)sRow + sReplicaSetIdColumn->column.offset );

            aRangeInfo->mRanges[aRangeInfo->mCount].mNodeId = (UInt)sNodeID;
            // Solo table 에서 partition name 및 hash value는 의미 없다. '\0'과 max로 설정
            aRangeInfo->mRanges[aRangeInfo->mCount].mPartitionName[0] = '\0';
            aRangeInfo->mRanges[aRangeInfo->mCount].mValue.mHashMax = (UInt)SDI_RANGE_MAX_COUNT;
            aRangeInfo->mRanges[aRangeInfo->mCount].mReplicaSetId = (UInt)sReplicaSetId;

            aRangeInfo->mCount++;

            IDE_TEST( sCursor.readRow( &sRow, &sRid, SMI_FIND_NEXT )
                      != IDE_SUCCESS );
        }
    }
    else
    {
        // Nothing to do.
    }

    sIsCursorOpen = ID_FALSE;
    IDE_TEST( sCursor.close() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_META_HANDLE )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDM_SHARD_META_NOT_CREATED,
                                  SDM_SOLOS ) );
    }
    IDE_EXCEPTION( RANGE_COUNT_OVERFLOW )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDM_SHARD_RANGE_OVERFLOW ) );
    }
    IDE_EXCEPTION_END;

    if ( sIsCursorOpen == ID_TRUE )
    {
        (void)sCursor.close();
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_FAILURE;
}

IDE_RC sdm::convertRangeValue( SChar       * aValue,
                               UInt          aLength,
                               UInt          aKeyType,
                               sdiValue    * aRangeValue )
{
    SChar           sBuf[SDI_RANGE_VARCHAR_MAX_PRECISION + 1];
    qcNamePosition  sPosition;
    SLong           sLongVal;

    IDE_DASSERT( aLength <= SDI_RANGE_VARCHAR_MAX_PRECISION );

    if ( ( aKeyType == MTD_SMALLINT_ID ) ||
         ( aKeyType == MTD_INTEGER_ID ) ||
         ( aKeyType == MTD_BIGINT_ID ) )
    {
        sPosition.stmtText = aValue;
        sPosition.offset   = 0;
        sPosition.size     = aLength;

        IDE_TEST_RAISE( qtc::getBigint( sPosition.stmtText,
                                        &sLongVal,
                                        &sPosition ) != IDE_SUCCESS,
                        ERR_INVALID_RANGE_VALUE );

        if ( aKeyType == MTD_SMALLINT_ID )
        {
            IDE_TEST_RAISE( ( sLongVal < MTD_SMALLINT_MINIMUM ) ||
                            ( sLongVal > MTD_SMALLINT_MAXIMUM ),
                            ERR_INVALID_RANGE_VALUE );

            aRangeValue->mSmallintMax = (SShort) sLongVal;
        }
        else if ( aKeyType == MTD_INTEGER_ID )
        {
            IDE_TEST_RAISE( ( sLongVal < MTD_INTEGER_MINIMUM ) ||
                            ( sLongVal > MTD_INTEGER_MAXIMUM ),
                            ERR_INVALID_RANGE_VALUE );

            aRangeValue->mIntegerMax = (SInt) sLongVal;
        }
        else
        {
            IDE_TEST_RAISE( sLongVal == MTD_BIGINT_NULL,
                            ERR_INVALID_RANGE_VALUE );

            aRangeValue->mBigintMax = sLongVal;
        }
    }
    else if ( ( aKeyType == MTD_CHAR_ID ) ||
              ( aKeyType == MTD_VARCHAR_ID ) )
    {
        aRangeValue->mCharMax.length = (UShort)aLength;
        idlOS::memcpy( (void*) aRangeValue->mCharMax.value,
                       (void*) aValue,
                       aLength );
        aRangeValue->mCharMax.value[aLength] = '\0';
    }
    else
    {
        IDE_DASSERT(0);
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_RANGE_VALUE )
    {
        idlOS::memcpy( sBuf, aValue, aLength );
        sBuf[aLength] = '\0';

        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDF_INVALID_RANGE_VALUE, sBuf ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdm::compareKeyData( UInt       aKeyDataType,
                            sdiValue * aValue1,
                            sdiValue * aValue2,
                            SShort   * aResult )
{
    const mtdModule * sKeyModule   = NULL;

    mtdValueInfo      sValueInfo1;
    mtdValueInfo      sValueInfo2;

    SInt              sCompResult = ID_SINT_MAX;

    IDE_TEST( mtd::moduleById( &sKeyModule,
                               aKeyDataType )
              != IDE_SUCCESS );

    switch ( aKeyDataType )
    {
        case MTD_SMALLINT_ID :
        {
            sValueInfo1.column = NULL;
            sValueInfo1.value  = &(aValue1->mSmallintMax);
            sValueInfo1.flag   = MTD_OFFSET_USELESS;

            sValueInfo2.column = NULL;
            sValueInfo2.value  = &(aValue2->mSmallintMax);
            sValueInfo2.flag   = MTD_OFFSET_USELESS;
            break;
        }
        case MTD_INTEGER_ID :
        {
            sValueInfo1.column = NULL;
            sValueInfo1.value  = &(aValue1->mIntegerMax);
            sValueInfo1.flag   = MTD_OFFSET_USELESS;

            sValueInfo2.column = NULL;
            sValueInfo2.value  = &(aValue2->mIntegerMax);
            sValueInfo2.flag   = MTD_OFFSET_USELESS;
            break;
        }
        case MTD_BIGINT_ID :
        {
            sValueInfo1.column = NULL;
            sValueInfo1.value  = &(aValue1->mBigintMax);
            sValueInfo1.flag   = MTD_OFFSET_USELESS;

            sValueInfo2.column = NULL;
            sValueInfo2.value  = &(aValue2->mBigintMax);
            sValueInfo2.flag   = MTD_OFFSET_USELESS;
            break;
        }
        case MTD_CHAR_ID :
        case MTD_VARCHAR_ID :
        {
            sValueInfo1.column = NULL;
            sValueInfo1.value  = &(aValue1->mCharMax);
            sValueInfo1.flag   = MTD_OFFSET_USELESS;

            sValueInfo2.column = NULL;
            sValueInfo2.value  = &(aValue2->mCharMax);
            sValueInfo2.flag   = MTD_OFFSET_USELESS;
            break;
        }
        default :
        {
            // unexpected error raise
            IDE_DASSERT(0);
            break;
        }
    }

    sCompResult = sKeyModule->logicalCompare[MTD_COMPARE_ASCENDING]( &sValueInfo1, &sValueInfo2 );

    if ( sCompResult == 0 )
    {
        *aResult = 0;
    }
    else if ( sCompResult > 0 )
    {
        *aResult = 1;
    }
    else // ( sCompResult < 0 )
    {
        *aResult = -1;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdm::shardRangeSort( sdiSplitMethod   aSplitMethod,
                            UInt             aKeyDataType,
                            idBool           aSubKeyExists,
                            sdiSplitMethod   aSubSplitMethod,
                            UInt             aSubKeyDataType,
                            sdiRangeInfo   * aRangeInfo )
{
    /*
     * PROJ-2655 Composite shard key
     * 
     * Index에 의해 보장 되지 않는
     * Data type conversion( e.x. string to integer ) 후 의 rangeInfo의 range들을 정렬한다.
     *
     */

    UInt     sKeyDataType = MTD_UNDEF_ID;
    UInt     sSubKeyDataType = MTD_UNDEF_ID;

    SInt     i = 0;
    SInt     j = 0;

    sdiRange sTmp;

    SShort   sCompare    = ID_SSHORT_MAX; // -1 : less, 0 : equal, 1 : greater

    /* Set key data type */
    if ( aSplitMethod == SDI_SPLIT_HASH )
    {
        sKeyDataType = MTD_INTEGER_ID;
    }
    else
    {
        sKeyDataType = aKeyDataType;
    }

    if ( aSubKeyExists == ID_TRUE )
    {
        if ( aSubSplitMethod == SDI_SPLIT_HASH )
        {
            sSubKeyDataType = MTD_INTEGER_ID;
        }
        else
        {
            sSubKeyDataType = aSubKeyDataType;
        }
    }
    else
    {
        /* Nothing to do. */
    }

    /* Insertion sort */
    for ( i = 0; i < ( aRangeInfo->mCount - 1 ); i++ )
    {
        idlOS::memcpy( (void*)&sTmp,
                       (void*)&aRangeInfo->mRanges[i + 1],
                       ID_SIZEOF(sdiRange) );

        for ( j = i; j > -1; j-- )
        {
            // compare shard key
            IDE_TEST( compareKeyData( sKeyDataType,
                                      &aRangeInfo->mRanges[j].mValue, // A
                                      &sTmp.mValue,                   // B
                                      &sCompare )                     // A = B : 0, A > B : 1, A < B : -1
                      != IDE_SUCCESS );

            if ( sCompare == 0 )
            {
                if ( aSubKeyExists == ID_TRUE )
                {
                    // compare sub-shard key
                    IDE_TEST( compareKeyData( sSubKeyDataType,
                                              &aRangeInfo->mRanges[j].mSubValue,
                                              &sTmp.mSubValue,
                                              &sCompare )
                              != IDE_SUCCESS );

                    if ( sCompare == 1 )
                    {
                        idlOS::memcpy( (void*)&aRangeInfo->mRanges[j + 1],
                                       (void*)&aRangeInfo->mRanges[j],
                                       ID_SIZEOF(sdiRange) );
                        if ( j == 0 )
                        {
                            idlOS::memcpy( (void*)&aRangeInfo->mRanges[j],
                                           (void*)&sTmp,
                                           ID_SIZEOF(sdiRange) );
                        }
                        else
                        {
                            /* Nothing to do. */
                        }
                    }
                    else if ( sCompare == -1 )
                    {
                        idlOS::memcpy( (void*)&aRangeInfo->mRanges[j + 1],
                                       (void*)&sTmp,
                                       ID_SIZEOF(sdiRange) );
                        break;
                    }
                    else
                    {
                        // Sub-shard key 까지 equal 일 수 없다. ( unique index )
                        IDE_RAISE( ERR_DUPLICATED );
                    }
                }
                else
                {
                    // shard key가 equal일 수 없다. ( unique index )
                    IDE_RAISE( ERR_DUPLICATED );
                }
            }
            else if ( sCompare == 1 )
            {
                idlOS::memcpy( (void*)&aRangeInfo->mRanges[j + 1],
                               (void*)&aRangeInfo->mRanges[j],
                               ID_SIZEOF(sdiRange) );

                if ( j == 0 )
                {
                    idlOS::memcpy( (void*)&aRangeInfo->mRanges[j],
                                   (void*)&sTmp,
                                   ID_SIZEOF(sdiRange) );
                }
                else
                {
                    /* Nothing to do. */
                }
            }
            else
            {
                IDE_DASSERT( sCompare == -1 );

                idlOS::memcpy( (void*)&aRangeInfo->mRanges[j + 1],
                               (void*)&sTmp,
                               ID_SIZEOF(sdiRange) );
                break;
            }
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_DUPLICATED )
    {
        IDE_SET(ideSetErrorCode(sdERR_ABORT_SDM_DUPLICATED_RANGE_VALUE));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdm::shardEliminateDuplication( sdiTableInfo * aTableInfo,
                                       sdiRangeInfo * aRangeInfo )
{
    UShort   i;

    UShort   sNewCount = 0;
    UShort   sSplitCombination = 0;

    SShort   sCompare;
    UInt     sKeyDataType;

    /*
     * PROJ-2655 Composite shard key
     *
     * Value 에 대해 sort가 된 상태의 range value에 대해
     * 중첩 정의 된 range value를 합쳐준다.
     *
     * e.x ) RANGE [100][200][300][400]
     *       NODE  [ A ][ B ][ B ][ C ]
     *
     *   ->  RANGE [100][300][400]
     *       NODE  [ A ][ B ][ C ]
     *
     * + 합치는 기준 +
     *
     * sSplitCombination | split method  | sub split method | process ( 정렬되어 있는 상태에서~ )
     *--------------------------------------------------------------------------------------------------------
     *         1         | RANGE or HASH |        -         | 노드가 같으면 합칠 수 있다.
     *         2         | LIST          | RANGE or HASH    | Value가 같으면서 노드가 같으면 합칠 수 있다.
     *         3         | RANGE or HASH | RANGE or HASH    | Value가 같으면서 노드가 같으면 합칠 수 있다.
     *--------------------------------------------------------------------------------------------------------
     *
     * + 목록에 없는 조합은 중복제거 하면 안된다.
     *
     *    e.x. ) RANGE & LIST etc...
     *
     */

    /* Set split combination type */
    if ( aTableInfo->mSplitMethod == SDI_SPLIT_LIST )
    {
        // SDI_SPLIT_LIST
        if ( aTableInfo->mSubKeyExists == ID_TRUE )
        {
            if ( ( aTableInfo->mSubSplitMethod == SDI_SPLIT_HASH ) ||
                 ( aTableInfo->mSubSplitMethod == SDI_SPLIT_RANGE ) )
            {
                sSplitCombination = 2;
            }
            else if ( aTableInfo->mSubSplitMethod == SDI_SPLIT_LIST )
            {
                /* Nothing to do. */
            }
            else
            {
                // SDI_SPLIT_NONE or SDI_SPLIT_CLONE or SDI_SPLIT_SOLO etc...
                ideLog::log( IDE_SD_1, "[SHARD_META_ERROR] Failure. shardEliminateDuplication : invalid SPLIT_METHOD !!!\n");
                IDE_RAISE( ERR_META_CRASH );
            }
        }
        else
        {
            /* Nothing to do. */
        }
    }
    else if ( ( aTableInfo->mSplitMethod == SDI_SPLIT_HASH ) ||
              ( aTableInfo->mSplitMethod == SDI_SPLIT_RANGE ) )
    {
        if ( aTableInfo->mSubKeyExists == ID_TRUE )
        {
            if ( ( aTableInfo->mSubSplitMethod == SDI_SPLIT_HASH ) ||
                 ( aTableInfo->mSubSplitMethod == SDI_SPLIT_RANGE ) )
            {
                sSplitCombination = 3;
            }
            else if ( aTableInfo->mSubSplitMethod == SDI_SPLIT_LIST )                
            {
                /* Nothing to do. */
            }
            else
            {
                // SDI_SPLIT_NONE or SDI_SPLIT_CLONE or SDI_SPLIT_SOLO or etc...
                ideLog::log( IDE_SD_1, "[SHARD_META_ERROR] Failure. shardEliminateDuplication : invalid SPLIT_METHOD !!!\n");
                IDE_RAISE( ERR_META_CRASH );
            }
        }
        else
        {
            sSplitCombination = 1;
        }
    }
    else
    {
        // SDI_SPLIT_NONE or SDI_SPLIT_CLONE or SDI_SPLIT_SOLO
        IDE_DASSERT(0);
    }

    /* Eliminate duplicated range */
    if ( ( aRangeInfo->mCount > 1 ) && ( sSplitCombination > 0 ) )
    {   
        for ( i = 1; i < aRangeInfo->mCount; i++ )
        {
            if ( sSplitCombination == 1 )
            {
                if ( aRangeInfo->mRanges[i].mNodeId != aRangeInfo->mRanges[sNewCount].mNodeId )
                {
                    sNewCount++;
                }
                else
                {
                    /* Nothing to do. */
                }
            }
            else if ( ( sSplitCombination == 2  ) || ( sSplitCombination == 3 ) )
            {

                /* Set key data type */
                if ( aTableInfo->mSplitMethod == SDI_SPLIT_HASH )
                {
                    sKeyDataType = MTD_INTEGER_ID;
                }
                else
                {
                    sKeyDataType = aTableInfo->mKeyDataType;
                }
                
                // compare first shard key
                IDE_TEST( compareKeyData( sKeyDataType,
                                          &aRangeInfo->mRanges[i].mValue,
                                          &aRangeInfo->mRanges[sNewCount].mValue,
                                          &sCompare )
                          != IDE_SUCCESS );

                if ( sCompare != 0 )
                {
                    sNewCount++;
                }
                else
                {
                    if ( aRangeInfo->mRanges[i].mNodeId != aRangeInfo->mRanges[sNewCount].mNodeId )
                    {
                        sNewCount++;
                    }
                    else
                    {
                        /* Nothing to do. */
                    }
                }
            }
            else
            {
                ideLog::log( IDE_SD_1, "[SHARD_META_ERROR] Failure. shardEliminateDuplication : invalid SPLIT_METHOD !!!\n");
                IDE_RAISE( ERR_META_CRASH );
            }

            aRangeInfo->mRanges[sNewCount].mNodeId = aRangeInfo->mRanges[i].mNodeId;
            aRangeInfo->mRanges[sNewCount].mValue  = aRangeInfo->mRanges[i].mValue;

            if ( aTableInfo->mSubKeyExists == ID_TRUE )
            {
                aRangeInfo->mRanges[sNewCount].mSubValue = aRangeInfo->mRanges[i].mSubValue;
            }
            else
            {
                /* Nothing to do. */
            }
        }

        aRangeInfo->mCount = ( sNewCount + 1 );
    }
    else
    {
        /* Nothing to do. */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_META_CRASH )
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QCM_META_CRASH));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdm::getLocalMetaInfo( sdiLocalMetaInfo * aLocalMetaInfo )
{
    smiTrans       sTrans;
    smiStatement   sSmiStmt;
    smiStatement * sDummySmiStmt = NULL;
    UInt           sStage        = 0;
    idBool         sDummyIsKSafetyNULL = ID_FALSE;

    IDE_DASSERT( aLocalMetaInfo != NULL );

    IDE_TEST( sTrans.initialize() != IDE_SUCCESS );
    sStage = 1;

    IDE_TEST( sTrans.begin( &sDummySmiStmt, NULL )
              != IDE_SUCCESS );
    sStage = 2;

    IDE_TEST( sSmiStmt.begin( NULL,
                              sDummySmiStmt,
                              ( SMI_STATEMENT_UNTOUCHABLE |
                                SMI_STATEMENT_MEMORY_CURSOR ) )
              != IDE_SUCCESS );
    sStage = 3;

    IDE_TEST( getLocalMetaInfoCore( &sSmiStmt, aLocalMetaInfo,
                                    &sDummyIsKSafetyNULL  ) != IDE_SUCCESS );

    IDE_TEST( sSmiStmt.end( SMI_STATEMENT_RESULT_SUCCESS ) != IDE_SUCCESS );
    sStage = 2;

    IDE_TEST( sTrans.commit() != IDE_SUCCESS );
    sStage = 1;

    sStage = 0;
    IDE_TEST( sTrans.destroy( NULL ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();

    switch ( sStage )
    {
        case 3:
            ( void )sSmiStmt.end( SMI_STATEMENT_RESULT_FAILURE );
            /* fall through */
        case 2:
            ( void )sTrans.rollback();
            /* fall through */
        case 1:
            ( void )sTrans.destroy( NULL );
            /* fall through */
        default:
            break;
    }

    IDE_POP();

    ideLog::log( IDE_SD_1, "[SHARD META_ERROR] Failure. errorcode 0x%05"ID_XINT32_FMT" %s\n",
                           E_ERROR_CODE(ideGetErrorCode()),
                           ideGetErrorMsg(ideGetErrorCode()));

    return IDE_FAILURE;
}

IDE_RC sdm::getLocalMetaInfoAndCheckKSafety( sdiLocalMetaInfo * aLocalMetaInfo, 
                                             idBool * aOutIsKSafetyNULL )
{
    smiTrans       sTrans;
    smiStatement   sSmiStmt;
    smiStatement * sDummySmiStmt = NULL;
    UInt           sStage        = 0;
    idBool         sIsKSafetyNULL = ID_FALSE;
    
    IDE_DASSERT( aLocalMetaInfo != NULL );
    
    IDE_TEST( sTrans.initialize() != IDE_SUCCESS );
    sStage = 1;
    
    IDE_TEST( sTrans.begin( &sDummySmiStmt, NULL )
              != IDE_SUCCESS );
    sStage = 2;
    
    IDE_TEST( sSmiStmt.begin( NULL,
                              sDummySmiStmt,
                              ( SMI_STATEMENT_UNTOUCHABLE |
                                SMI_STATEMENT_MEMORY_CURSOR ) )
              != IDE_SUCCESS );
    sStage = 3;
    
    IDE_TEST( getLocalMetaInfoCore( &sSmiStmt, aLocalMetaInfo,
                                    &sIsKSafetyNULL  ) != IDE_SUCCESS );
    
    IDE_TEST( sSmiStmt.end( SMI_STATEMENT_RESULT_SUCCESS ) != IDE_SUCCESS );
    sStage = 2;
    
    IDE_TEST( sTrans.commit() != IDE_SUCCESS );
    sStage = 1;
    
    sStage = 0;
    IDE_TEST( sTrans.destroy( NULL ) != IDE_SUCCESS );
    
    *aOutIsKSafetyNULL = sIsKSafetyNULL;
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    IDE_PUSH();
    
    switch ( sStage )
    {
    case 3:
        ( void )sSmiStmt.end( SMI_STATEMENT_RESULT_FAILURE );
            /* fall through */
    case 2:
        ( void )sTrans.rollback();
            /* fall through */
    case 1:
        ( void )sTrans.destroy( NULL );
            /* fall through */
    default:
        break;
    }
    
    IDE_POP();
    
    ideLog::log( IDE_SD_1, "[SHARD_META_ERROR] Failure. errorcode 0x%05"ID_XINT32_FMT" %s\n",
                 E_ERROR_CODE(ideGetErrorCode()),
                 ideGetErrorMsg(ideGetErrorCode()));
    
    return IDE_FAILURE;
}

IDE_RC sdm::getLocalMetaInfoCore( smiStatement     * aSmiStmt,
                                  sdiLocalMetaInfo * aLocalMetaInfo,
                                  idBool           * aOutIsKSafetyNULL )
{
    /*
     *  "SYS_SHARD.LOCAL_META_INFO_ Meta Table"
     *  "SHARD_NODE_ID      INTEGER, "
     *  "SHARDED_DB_NAME    VARCHAR(40), "
     *  "NODE_NAME          VARCHAR(10), "
     *  "HOST_IP            VARCHAR(64), "
     *  "PORT_NO            INTEGER, "
     *  "INTERNAL_HOST_IP   VARCHAR(64), "
     *  "INTERNAL_PORT_NO   INTEGER, "
     *  "INTERNAL_REPLICATION_HOST_IP   VARCHAR(64), "
     *  "INTERNAL_REPLICATION_PORT_NO   INTEGER, "
     *  "INTERNAL_CONN_TYPE INTEGER, "
     *  "K_SAFETY           INTEGER, "
     *  "REPLICATION_MODE   INTEGER, "
     *  "PARALLEL_COUNT     INTEGER )"
     */
    idBool            sIsCursorOpen     = ID_FALSE;
    const void      * sRow              = NULL;
    scGRID            sRid;
    const void      * sSdmLocalMetaInfo  = NULL;
    mtcColumn       * sMtcColumn = NULL;
    mtdIntegerType    sIntData;
    mtdCharType      *sCharData;

    smiTableCursor       sCursor;
    smiCursorProperties  sCursorProperty;
    idBool               sIsKSafetyNULL = ID_FALSE;

    IDE_TEST( checkMetaVersion( aSmiStmt )
              != IDE_SUCCESS );

    IDE_TEST( getMetaTableAndIndex( aSmiStmt,
                                    SDM_LOCAL_META_INFO,
                                    &sSdmLocalMetaInfo,
                                    NULL )
              != IDE_SUCCESS );

    sCursor.initialize();

    SMI_CURSOR_PROP_INIT_FOR_META_FULL_SCAN( &sCursorProperty, NULL );

    IDE_TEST( sCursor.open( aSmiStmt,
                            sSdmLocalMetaInfo,
                            NULL,
                            smiGetRowSCN( sSdmLocalMetaInfo ),
                            NULL,
                            smiGetDefaultKeyRange(),
                            smiGetDefaultKeyRange(),
                            smiGetDefaultFilter(),
                            QCM_META_CURSOR_FLAG,
                            SMI_SELECT_CURSOR,
                            &sCursorProperty )
              != IDE_SUCCESS );

    sIsCursorOpen = ID_TRUE;

    IDE_TEST( sCursor.beforeFirst() != IDE_SUCCESS );

    IDE_TEST( sCursor.readRow( &sRow, &sRid, SMI_FIND_NEXT )
              != IDE_SUCCESS );

    // 1건도 없으면 에러
    IDE_TEST_RAISE( sRow == NULL, ERR_CHECK_SHARD_NODE_INFO );
    /* Shard Node ID */
    IDE_TEST( smiGetTableColumns( sSdmLocalMetaInfo,
                                  SDM_LOCAL_META_INFO_SHARD_NODE_ID_COL_ORDER,
                                  (const smiColumn**)&sMtcColumn )
              != IDE_SUCCESS );
    sIntData = *(mtdIntegerType*)( (SChar *)sRow + sMtcColumn->column.offset );
    aLocalMetaInfo->mShardNodeId = (sdiShardNodeId)sIntData;

    /* Sharded DB Name */
    IDE_TEST( smiGetTableColumns( sSdmLocalMetaInfo,
                                  SDM_LOCAL_META_INFO_SHARDED_DB_NAME_COL_ORDER,
                                  (const smiColumn**)&sMtcColumn )
              != IDE_SUCCESS );
    sCharData = (mtdCharType*)( (SChar *)sRow + sMtcColumn->column.offset );
    idlOS::memcpy( aLocalMetaInfo->mShardedDBName,
                   sCharData->value,
                   sCharData->length );
    aLocalMetaInfo->mShardedDBName[sCharData->length] = '\0';

    /* Node Name*/
    IDE_TEST( smiGetTableColumns( sSdmLocalMetaInfo,
                                  SDM_LOCAL_META_INFO_NODE_NAME_COL_ORDER,
                                  (const smiColumn**)&sMtcColumn )
              != IDE_SUCCESS );
    sCharData = (mtdCharType*)( (SChar *)sRow + sMtcColumn->column.offset );
    idlOS::memcpy( aLocalMetaInfo->mNodeName,
                   sCharData->value,
                   sCharData->length );
    aLocalMetaInfo->mNodeName[sCharData->length] = '\0';
    /* Host IP*/
    IDE_TEST( smiGetTableColumns( sSdmLocalMetaInfo,
                                  SDM_LOCAL_META_INFO_HOST_IP_COL_ORDER,
                                  (const smiColumn**)&sMtcColumn )
              != IDE_SUCCESS );
    sCharData = (mtdCharType*)( (SChar *)sRow + sMtcColumn->column.offset );
    idlOS::memcpy( aLocalMetaInfo->mHostIP,
                   sCharData->value,
                   sCharData->length );
    aLocalMetaInfo->mHostIP[sCharData->length] = '\0';
    /* PortNo */
    IDE_TEST( smiGetTableColumns( sSdmLocalMetaInfo,
                                  SDM_LOCAL_META_INFO_PORT_NO_COL_ORDER,
                                  (const smiColumn**)&sMtcColumn )
              != IDE_SUCCESS );
    sIntData = *(mtdIntegerType*)( (SChar *)sRow + sMtcColumn->column.offset );
    aLocalMetaInfo->mPortNo = (UShort)sIntData;
    /* Internal Host IP*/
    IDE_TEST( smiGetTableColumns( sSdmLocalMetaInfo,
                                  SDM_LOCAL_META_INFO_INTERNAL_HOST_IP_COL_ORDER,
                                  (const smiColumn**)&sMtcColumn )
              != IDE_SUCCESS );
    sCharData = (mtdCharType*)( (SChar *)sRow + sMtcColumn->column.offset );
    idlOS::memcpy( aLocalMetaInfo->mInternalHostIP,
                   sCharData->value,
                   sCharData->length );
    aLocalMetaInfo->mInternalHostIP[sCharData->length] = '\0';
    /* Internal PortNo */
    IDE_TEST( smiGetTableColumns( sSdmLocalMetaInfo,
                                  SDM_LOCAL_META_INFO_INTERNAL_PORT_NO_COL_ORDER,
                                  (const smiColumn**)&sMtcColumn )
              != IDE_SUCCESS );
    sIntData = *(mtdIntegerType*)( (SChar *)sRow + sMtcColumn->column.offset );
    aLocalMetaInfo->mInternalPortNo = (UShort)sIntData;

    /* Internal Replication Host IP*/
    IDE_TEST( smiGetTableColumns( sSdmLocalMetaInfo,
                                  SDM_LOCAL_META_INFO_INTERNAL_REPLICATION_HOST_IP_COL_ORDER,
                                  (const smiColumn**)&sMtcColumn )
              != IDE_SUCCESS );
    sCharData = (mtdCharType*)( (SChar *)sRow + sMtcColumn->column.offset );
    idlOS::memcpy( aLocalMetaInfo->mInternalRPHostIP,
                   sCharData->value,
                   sCharData->length );
    aLocalMetaInfo->mInternalRPHostIP[sCharData->length] = '\0';
    /* Internal Replication PortNo */
    IDE_TEST( smiGetTableColumns( sSdmLocalMetaInfo,
                                  SDM_LOCAL_META_INFO_INTERNAL_REPLICATION_PORT_NO_COL_ORDER,
                                  (const smiColumn**)&sMtcColumn )
              != IDE_SUCCESS );
    sIntData = *(mtdIntegerType*)( (SChar *)sRow + sMtcColumn->column.offset );
    aLocalMetaInfo->mInternalRPPortNo = (UShort)sIntData;

    /* Internal Conn Type */
    IDE_TEST( smiGetTableColumns( sSdmLocalMetaInfo,
                                  SDM_LOCAL_META_INFO_INTERNAL_CONN_TYPE_COL_ORDER,
                                  (const smiColumn**)&sMtcColumn )
              != IDE_SUCCESS );
    sIntData = *(mtdIntegerType*)( (SChar *)sRow + sMtcColumn->column.offset );
    aLocalMetaInfo->mInternalConnType = (UInt)sIntData;

    /* K-Safety */
    IDE_TEST( smiGetTableColumns( sSdmLocalMetaInfo,
                                  SDM_LOCAL_META_INFO_K_SAFETY_COL_ORDER,
                                  (const smiColumn**)&sMtcColumn )
              != IDE_SUCCESS );
    sIntData = *(mtdIntegerType*)( (SChar *)sRow + sMtcColumn->column.offset );
    aLocalMetaInfo->mKSafety = (UInt)sIntData;

    IDE_TEST( mtd::moduleById( &(sMtcColumn->module),
                               sMtcColumn->type.dataTypeId )
              != IDE_SUCCESS );

    if ( sMtcColumn->module->isNull(sMtcColumn,
                                    &aLocalMetaInfo->mKSafety) == ID_TRUE )
    {
        sIsKSafetyNULL = ID_TRUE;
    }
    else
    {
        sIsKSafetyNULL = ID_FALSE;
    }
    /* Replication Mode */
    IDE_TEST( smiGetTableColumns( sSdmLocalMetaInfo,
                                  SDM_LOCAL_META_INFO_REPLICATION_MODE_COL_ORDER,
                                  (const smiColumn**)&sMtcColumn )
              != IDE_SUCCESS );
    sIntData = *(mtdIntegerType*)( (SChar *)sRow + sMtcColumn->column.offset );
    aLocalMetaInfo->mReplicationMode = (UInt)sIntData;

    /* Parallel Count */
    IDE_TEST( smiGetTableColumns( sSdmLocalMetaInfo,
                                  SDM_LOCAL_META_INFO_PARALLEL_COUNT_COL_ORDER,
                                  (const smiColumn**)&sMtcColumn )
              != IDE_SUCCESS );
    sIntData = *(mtdIntegerType*)( (SChar *)sRow + sMtcColumn->column.offset );
    aLocalMetaInfo->mParallelCount = (UInt)sIntData;

    IDE_TEST(sCursor.readRow(&sRow, &sRid, SMI_FIND_NEXT) != IDE_SUCCESS);
    // 1건이어야 한다.
    IDE_TEST_RAISE( sRow != NULL, ERR_CHECK_SHARD_NODE_INFO );

    sIsCursorOpen = ID_FALSE;
    IDE_TEST( sCursor.close() != IDE_SUCCESS );
    *aOutIsKSafetyNULL = sIsKSafetyNULL;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_CHECK_SHARD_NODE_INFO )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDM_INVALID_SHARD_NODE_INFO ) );
    }
    IDE_EXCEPTION_END;

    IDE_PUSH();

    if ( sIsCursorOpen == ID_TRUE )
    {
        (void)sCursor.close();
    }
    else
    {
        /* Nothing to do */
    }

    IDE_POP();

    return IDE_FAILURE;
}

idBool sdm::isShardMetaCreated( smiStatement * aSmiStmt )
{
    const void * sSdmVersion;

    IDE_TEST( getMetaTableAndIndex( aSmiStmt,
                                    SDM_VERSION,
                                    &sSdmVersion,
                                    NULL )
              != IDE_SUCCESS );

    return ID_TRUE;

    IDE_EXCEPTION_END;

    IDE_CLEAR();

    return ID_FALSE;
}

IDE_RC sdm::getGlobalMetaInfo( sdiGlobalMetaInfo * aMetaNodeInfo )
{
    smiTrans       sTrans;
    smiStatement   sSmiStmt;
    smiStatement * sDummySmiStmt = NULL;
    UInt           sStage        = 0;

    IDE_DASSERT( aMetaNodeInfo != NULL );

    IDE_TEST( sTrans.initialize() != IDE_SUCCESS );
    sStage = 1;

    IDE_TEST( sTrans.begin( &sDummySmiStmt, NULL ) != IDE_SUCCESS );
    sStage = 2;

    IDE_TEST( sSmiStmt.begin( NULL,
                              sDummySmiStmt,
                              ( SMI_STATEMENT_UNTOUCHABLE |
                                SMI_STATEMENT_MEMORY_CURSOR ) )
              != IDE_SUCCESS );
    sStage = 3;

    IDE_TEST( getGlobalMetaInfoCore( &sSmiStmt, aMetaNodeInfo ) != IDE_SUCCESS );

    IDE_TEST( sSmiStmt.end( SMI_STATEMENT_RESULT_SUCCESS ) != IDE_SUCCESS );
    sStage = 2;

    IDE_TEST( sTrans.commit() != IDE_SUCCESS );
    sStage = 1;

    sStage = 0;
    IDE_TEST( sTrans.destroy( NULL ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();

    switch ( sStage )
    {
        case 3:
            ( void )sSmiStmt.end( SMI_STATEMENT_RESULT_FAILURE );
            /* fall through */
        case 2:
            ( void )sTrans.rollback();
            /* fall through */
        case 1:
            ( void )sTrans.destroy( NULL );
            /* fall through */
        default:
            break;
    }

    IDE_POP();

    ideLog::log( IDE_SD_1, "[SHARD_META_ERROR] Failure. errorcode 0x%05"ID_XINT32_FMT" %s\n",
                           E_ERROR_CODE( ideGetErrorCode() ),
                           ideGetErrorMsg( ideGetErrorCode() ) );

    return IDE_FAILURE;
}

IDE_RC sdm::getGlobalMetaInfoCore( smiStatement          * aSmiStmt,
                                   sdiGlobalMetaInfo * aMetaNodeInfo )
{
    const void          * sRow                   = NULL;
    scGRID                sRid;
    const void          * sSdmMetaNodeInfo       = NULL;
    idBool                sIsCursorOpen          = ID_FALSE;

    mtcColumn           * sShardMetaNumberColumn = NULL;
    mtdBigintType         sShardMetaNumber       = ID_LONG(0);

    smiTableCursor        sCursor;
    smiCursorProperties   sCursorProperty;

    IDE_TEST( checkMetaVersion( aSmiStmt ) != IDE_SUCCESS );

    IDE_TEST( getMetaTableAndIndex( aSmiStmt,
                                    SDM_GLOBAL_META_INFO,
                                    &sSdmMetaNodeInfo,
                                    NULL )
              != IDE_SUCCESS );

    IDE_TEST( smiGetTableColumns( sSdmMetaNodeInfo,
                                  SDM_GLOBAL_META_INFO_SHARD_META_NUMBER_COL_ORDER,
                                  (const smiColumn **)&sShardMetaNumberColumn )
              != IDE_SUCCESS );

    sCursor.initialize();

    SMI_CURSOR_PROP_INIT_FOR_META_FULL_SCAN( &sCursorProperty, NULL );

    IDE_TEST( sCursor.open( aSmiStmt,
                            sSdmMetaNodeInfo,
                            NULL,
                            smiGetRowSCN( sSdmMetaNodeInfo ),
                            NULL,
                            smiGetDefaultKeyRange(),
                            smiGetDefaultKeyRange(),
                            smiGetDefaultFilter(),
                            QCM_META_CURSOR_FLAG,
                            SMI_SELECT_CURSOR,
                            &sCursorProperty )
              != IDE_SUCCESS );
    sIsCursorOpen = ID_TRUE;

    IDE_TEST( sCursor.beforeFirst() != IDE_SUCCESS );

    IDE_TEST( sCursor.readRow( &sRow, &sRid, SMI_FIND_NEXT ) != IDE_SUCCESS );

    IDE_TEST_RAISE( sRow == NULL, ERR_META_NODE_INFO_ROW_COUNT );

    sShardMetaNumber = *(mtdBigintType *)( (SChar *)sRow + sShardMetaNumberColumn->column.offset );

    IDE_TEST( sCursor.readRow( &sRow, &sRid, SMI_FIND_NEXT ) != IDE_SUCCESS );

    IDE_TEST_RAISE( sRow != NULL, ERR_META_NODE_INFO_ROW_COUNT );

    sIsCursorOpen = ID_FALSE;
    IDE_TEST( sCursor.close() != IDE_SUCCESS );

    aMetaNodeInfo->mShardMetaNumber = (ULong)sShardMetaNumber;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_META_NODE_INFO_ROW_COUNT )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                  "sdm::getGlobalMetaNodeInfoCore",
                                  "row count is not 1" ) );
    }
    IDE_EXCEPTION_END;

    IDE_PUSH();

    if ( sIsCursorOpen == ID_TRUE )
    {
        (void)sCursor.close();
    }
    else
    {
        /* Nothing to do */
    }

    IDE_POP();

    return IDE_FAILURE;
}

IDE_RC sdm::makeShardMeta4NewSMN( qcStatement * aStatement )
{
    SChar                 * sSqlStr;
    vSLong                  sRowCnt;
    sdiGlobalMetaInfo   sMetaNodeInfo = { SDI_NULL_SMN };
    sdiGlobalMetaInfo   sMetaNodeInfoWithAnotherTx = { SDI_NULL_SMN };
    IDE_TEST( checkMetaVersion( QC_SMI_STMT( aStatement ) )
              != IDE_SUCCESS );

    IDE_TEST( STRUCT_ALLOC_WITH_SIZE( aStatement->qmxMem,
                                      SChar,
                                      QD_MAX_SQL_LENGTH,
                                      &sSqlStr )
              != IDE_SUCCESS);

    /* Acquire record lock for checking SMN */
    idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                     "UPDATE SYS_SHARD.GLOBAL_META_INFO_ "
                     "   SET SHARD_META_NUMBER = SHARD_META_NUMBER" );

    IDE_TEST( qciMisc::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                     sSqlStr,
                                     & sRowCnt )
              != IDE_SUCCESS );

    /* Meta SMN of current transaction */
    IDE_TEST ( getGlobalMetaInfoCore( QC_SMI_STMT(aStatement),
                                          &sMetaNodeInfo ) != IDE_SUCCESS );

    /* Meta SMN of another transaction */
    IDE_TEST ( getGlobalMetaInfo ( &sMetaNodeInfoWithAnotherTx ) != IDE_SUCCESS );

    /* BUG-46884
     * SMN의 lock을 획득한 후에 currentTx의 SMN과 newTx의 SMN을 비교하여 같으면,
     * 내가 currentTx에서 shard meta를 건드리는 첫 statement 수행이다.
     */
    if ( sMetaNodeInfo.mShardMetaNumber == sMetaNodeInfoWithAnotherTx.mShardMetaNumber )
    {
        /* increase shard meta number */
        idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                         "UPDATE SYS_SHARD.GLOBAL_META_INFO_ "
                         "   SET SHARD_META_NUMBER = SHARD_META_NUMBER + 1" );

        IDE_TEST( qciMisc::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                         sSqlStr,
                                         & sRowCnt )
                  != IDE_SUCCESS );

        /* get increased shard meta number */
        IDE_TEST( getGlobalMetaInfoCore( QC_SMI_STMT(aStatement),
                                             &sMetaNodeInfo ) != IDE_SUCCESS );

        sMetaNodeInfo.mShardMetaNumber--;

        /* copy nodes_ with increased SMN */
        idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                         "INSERT INTO SYS_SHARD.NODES_ "
                         "     ( NODE_ID, NODE_NAME, HOST_IP, PORT_NO, ALTERNATE_HOST_IP, ALTERNATE_PORT_NO, "
                         "       INTERNAL_HOST_IP, INTERNAL_PORT_NO, "
                         "       INTERNAL_ALTERNATE_HOST_IP, INTERNAL_ALTERNATE_PORT_NO, INTERNAL_CONN_TYPE, SMN ) "
                         "SELECT NODE_ID, NODE_NAME, HOST_IP, PORT_NO, ALTERNATE_HOST_IP, ALTERNATE_PORT_NO, "
                         "       INTERNAL_HOST_IP, INTERNAL_PORT_NO, "
                         "       INTERNAL_ALTERNATE_HOST_IP, INTERNAL_ALTERNATE_PORT_NO, INTERNAL_CONN_TYPE, SMN + 1 "
                         "  FROM SYS_SHARD.NODES_ "
                         " WHERE SMN = "QCM_SQL_BIGINT_FMT,
                         sMetaNodeInfo.mShardMetaNumber );

        IDE_TEST( qciMisc::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                         sSqlStr,
                                         & sRowCnt )
                  != IDE_SUCCESS );

        /* copy objects_ with increased SMN */
        idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                         "INSERT INTO SYS_SHARD.OBJECTS_ "
                         "      ( SHARD_ID, USER_NAME, OBJECT_NAME, OBJECT_TYPE, SPLIT_METHOD, KEY_COLUMN_NAME, "
                         "        SUB_SPLIT_METHOD, SUB_KEY_COLUMN_NAME, DEFAULT_NODE_ID, "
                         "        DEFAULT_PARTITION_NAME, DEFAULT_PARTITION_REPLICA_SET_ID, SMN ) "
                         "SELECT  SHARD_ID, USER_NAME, OBJECT_NAME, OBJECT_TYPE, SPLIT_METHOD, KEY_COLUMN_NAME, "
                         "        SUB_SPLIT_METHOD, SUB_KEY_COLUMN_NAME, DEFAULT_NODE_ID, "
                         "        DEFAULT_PARTITION_NAME, DEFAULT_PARTITION_REPLICA_SET_ID, SMN + 1 "
                         "  FROM SYS_SHARD.OBJECTS_ "
                         " WHERE SMN = "QCM_SQL_BIGINT_FMT,
                         sMetaNodeInfo.mShardMetaNumber );

        IDE_TEST( qciMisc::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                         sSqlStr,
                                         & sRowCnt )
                  != IDE_SUCCESS );

        /* copy ranges_ with increased SMN */
        idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                         "INSERT INTO SYS_SHARD.RANGES_ "
                         "     ( SHARD_ID, PARTITION_NAME, VALUE, SUB_VALUE, NODE_ID, SMN, REPLICA_SET_ID )"
                         "SELECT SHARD_ID, PARTITION_NAME, VALUE, SUB_VALUE, NODE_ID, SMN + 1, REPLICA_SET_ID "
                         "  FROM SYS_SHARD.RANGES_ "
                         " WHERE SMN = "QCM_SQL_BIGINT_FMT,
                         sMetaNodeInfo.mShardMetaNumber );

        IDE_TEST( qciMisc::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                         sSqlStr,
                                         & sRowCnt )
                  != IDE_SUCCESS );

        /* copy clones_ with increased SMN */
        idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                         "INSERT INTO SYS_SHARD.CLONES_ "
                         "     ( SHARD_ID, NODE_ID, SMN, REPLICA_SET_ID )"
                         "SELECT SHARD_ID, NODE_ID, SMN + 1, REPLICA_SET_ID "
                         "  FROM SYS_SHARD.CLONES_ "
                         " WHERE SMN = "QCM_SQL_BIGINT_FMT,
                         sMetaNodeInfo.mShardMetaNumber );

        IDE_TEST( qciMisc::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                         sSqlStr,
                                         & sRowCnt )
                  != IDE_SUCCESS );

        /* copy solos_ with increased SMN */
        idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                         "INSERT INTO SYS_SHARD.SOLOS_ "
                         "     ( SHARD_ID, NODE_ID, SMN, REPLICA_SET_ID )"
                         "SELECT SHARD_ID, NODE_ID, SMN + 1, REPLICA_SET_ID "
                         "  FROM SYS_SHARD.SOLOS_ "
                         " WHERE SMN = "QCM_SQL_BIGINT_FMT,
                         sMetaNodeInfo.mShardMetaNumber );

        IDE_TEST( qciMisc::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                         sSqlStr,
                                         & sRowCnt )
                  != IDE_SUCCESS );

        /* copy replica_sets_ with increased SMN */
        idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                         "INSERT INTO SYS_SHARD.REPLICA_SETS_ "
                         "     ( REPLICA_SET_ID, PRIMARY_NODE_NAME, FIRST_BACKUP_NODE_NAME, SECOND_BACKUP_NODE_NAME, "
                         "       STOP_FIRST_BACKUP_NODE_NAME, STOP_SECOND_BACKUP_NODE_NAME, "
                         "       FIRST_REPLICATION_NAME, FIRST_REPL_FROM_NODE_NAME, FIRST_REPL_TO_NODE_NAME, "
                         "       SECOND_REPLICATION_NAME, SECOND_REPL_FROM_NODE_NAME, SECOND_REPL_TO_NODE_NAME, SMN ) "
                         "SELECT REPLICA_SET_ID, PRIMARY_NODE_NAME, FIRST_BACKUP_NODE_NAME, SECOND_BACKUP_NODE_NAME, "
                         "       STOP_FIRST_BACKUP_NODE_NAME, STOP_SECOND_BACKUP_NODE_NAME, "
                         "       FIRST_REPLICATION_NAME, FIRST_REPL_FROM_NODE_NAME, FIRST_REPL_TO_NODE_NAME, "
                         "       SECOND_REPLICATION_NAME, SECOND_REPL_FROM_NODE_NAME, SECOND_REPL_TO_NODE_NAME, SMN + 1 "
                         "  FROM SYS_SHARD.REPLICA_SETS_ "
                         " WHERE SMN = "QCM_SQL_BIGINT_FMT,
                         sMetaNodeInfo.mShardMetaNumber );

        IDE_TEST( qciMisc::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                         sSqlStr,
                                         & sRowCnt )
                  != IDE_SUCCESS );
    }
    else
    {
        // Current transactioin의 이전 statement가 newSMN을 이미 생성한 경우
        IDE_TEST_RAISE( sMetaNodeInfo.mShardMetaNumber < sMetaNodeInfoWithAnotherTx.mShardMetaNumber,
                        ERR_SHARD_META_CHANGE_PROCESS_CRASH );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_SHARD_META_CHANGE_PROCESS_CRASH )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                  "sdm::makeShardMeta4NewSMN",
                                  "The shard meta modification was failed on the current transaction, please ROLLBACK." ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdm::deleteOldSMN( qcStatement * aStatement,
                          ULong       * aSMN )
{
    SChar                 * sSqlStr;
    vSLong                  sRowCnt;
    sdiGlobalMetaInfo   sMetaNodeInfo = { SDI_NULL_SMN };

    IDE_TEST( checkMetaVersion( QC_SMI_STMT( aStatement ) )
              != IDE_SUCCESS );

    IDE_TEST( getGlobalMetaInfoCore( QC_SMI_STMT(aStatement),
                                         &sMetaNodeInfo ) != IDE_SUCCESS );

    if ( aSMN == NULL )
    {
        sMetaNodeInfo.mShardMetaNumber--;
    }
    else
    {
        // Current SMN은 삭제 할 수 없다.
        IDE_TEST_RAISE( *aSMN >= sMetaNodeInfo.mShardMetaNumber, ERR_CANNOT_DELETE_CURRENT_SMN );
        sMetaNodeInfo.mShardMetaNumber = *aSMN;
    }

    IDE_TEST( STRUCT_ALLOC_WITH_SIZE( aStatement->qmxMem,
                                      SChar,
                                      QD_MAX_SQL_LENGTH,
                                      &sSqlStr )
              != IDE_SUCCESS);

    idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                     "DELETE FROM SYS_SHARD.RANGES_ WHERE SMN <= "QCM_SQL_BIGINT_FMT,
                     sMetaNodeInfo.mShardMetaNumber);

    IDE_TEST( qciMisc::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                     sSqlStr,
                                     & sRowCnt )
              != IDE_SUCCESS );

    idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                     "DELETE FROM SYS_SHARD.CLONES_ WHERE SMN <= "QCM_SQL_BIGINT_FMT,
                     sMetaNodeInfo.mShardMetaNumber);

    IDE_TEST( qciMisc::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                     sSqlStr,
                                     & sRowCnt )
              != IDE_SUCCESS );

    idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                     "DELETE FROM SYS_SHARD.SOLOS_ WHERE SMN <= "QCM_SQL_BIGINT_FMT,
                     sMetaNodeInfo.mShardMetaNumber);

    IDE_TEST( qciMisc::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                     sSqlStr,
                                     & sRowCnt )
              != IDE_SUCCESS );

    idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                     "DELETE FROM SYS_SHARD.OBJECTS_ WHERE SMN <= "QCM_SQL_BIGINT_FMT,
                     sMetaNodeInfo.mShardMetaNumber);

    IDE_TEST( qciMisc::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                     sSqlStr,
                                     & sRowCnt )
              != IDE_SUCCESS );

    idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                     "DELETE FROM SYS_SHARD.NODES_ WHERE SMN <= "QCM_SQL_BIGINT_FMT,
                     sMetaNodeInfo.mShardMetaNumber);

    IDE_TEST( qciMisc::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                     sSqlStr,
                                     & sRowCnt )
              != IDE_SUCCESS );

    idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                     "DELETE FROM SYS_SHARD.REPLICA_SETS_ WHERE SMN <= "QCM_SQL_BIGINT_FMT,
                     sMetaNodeInfo.mShardMetaNumber);

    IDE_TEST( qciMisc::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                     sSqlStr,
                                     & sRowCnt )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_CANNOT_DELETE_CURRENT_SMN )
    {
        IDE_SET( ideSetErrorCode(sdERR_ABORT_SDF_CANNOT_DELETE_CURRENT_SMN) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdm::updateSMN( qcStatement * aStatement,
                       ULong         aSMN )
{
    SChar       * sSqlStr = NULL;
    vSLong        sRowCnt = 0;    

    IDE_TEST( STRUCT_ALLOC_WITH_SIZE( aStatement->qmxMem,
                                      SChar,
                                      QD_MAX_SQL_LENGTH,
                                      &sSqlStr )
              != IDE_SUCCESS);

    idlOS::snprintf( sSqlStr, 
                     QD_MAX_SQL_LENGTH, 
                     "UPDATE SYS_SHARD.GLOBAL_META_INFO_ "
                     "SET SHARD_META_NUMBER = "QCM_SQL_BIGINT_FMT,
                     aSMN );

    IDE_TEST( qciMisc::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                     sSqlStr,
                                     & sRowCnt )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( sRowCnt != 1, ERR_EXECUTE );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_EXECUTE )
    {
        ideLog::log( IDE_SD_31, "[DEBUG] Statement(%s)"
                               " affects %"ID_INT64_FMT" row(s)",
                               sSqlStr, sRowCnt );

        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDF_INVALID_META_CHANGE ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdm::getShardUserID( UInt * aShardUserID )
{
    smiTrans       sTrans;
    smiStatement   sSmiStmt;
    smiStatement * sDummySmiStmt = NULL;
    UInt           sStage = 0;

    IDE_DASSERT( aShardUserID != NULL );

    IDE_TEST( sTrans.initialize() != IDE_SUCCESS );
    sStage = 1;

    IDE_TEST( sTrans.begin( &sDummySmiStmt, NULL ) != IDE_SUCCESS );
    sStage = 2;

    IDE_TEST( sSmiStmt.begin( NULL,
                              sDummySmiStmt,
                              ( SMI_STATEMENT_UNTOUCHABLE |
                                SMI_STATEMENT_MEMORY_CURSOR ) )
              != IDE_SUCCESS );
    sStage = 3;

    IDE_TEST( qcmUser::getUserID( NULL,
                                  &sSmiStmt,
                                  SDM_USER,
                                  idlOS::strlen(SDM_USER),
                                  aShardUserID )
              != IDE_SUCCESS );

    IDE_TEST( sSmiStmt.end( SMI_STATEMENT_RESULT_SUCCESS ) != IDE_SUCCESS );
    sStage = 2;

    IDE_TEST( sTrans.commit() != IDE_SUCCESS );
    sStage = 1;

    sStage = 0;
    IDE_TEST( sTrans.destroy( NULL ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();

    switch ( sStage )
    {
        case 3:
            ( void )sSmiStmt.end( SMI_STATEMENT_RESULT_FAILURE );
            /* fall through */
        case 2:
            ( void )sTrans.rollback();
            /* fall through */
        case 1:
            ( void )sTrans.destroy( NULL );
            /* fall through */
        default:
            break;
    }

    IDE_POP();

    ideLog::log( IDE_SD_1, "[SHARD_META_ERROR] Failure. errorcode 0x%05"ID_XINT32_FMT" %s\n",
                           E_ERROR_CODE( ideGetErrorCode() ),
                           ideGetErrorMsg( ideGetErrorCode() ) );

    return IDE_FAILURE;
}

IDE_RC sdm::validateRangeCountBeforeInsert( qcStatement  * aStatement,
                                            sdiTableInfo * aTableInfo,
                                            ULong          aSMN )
{
    const void         * sSdmRangesIndex[SDM_MAX_META_INDICES];
    const void         * sSdmRanges = NULL;
    mtcColumn          * sShardIDColumn = NULL;
    qtcMetaRangeColumn   sShardIDRange;
    mtcColumn          * sSMNColumn;
    qtcMetaRangeColumn   sSMNRange;
    smiRange             sRange;
    scGRID               sRid;
    const void         * sRow = NULL;
    smiTableCursor       sCursor;
    smiCursorProperties  sCursorProperty;
    idBool               sIsCursorOpen = ID_FALSE;
    UInt                 sExistCount = 0;

    IDE_TEST( getMetaTableAndIndex( QC_SMI_STMT( aStatement ),
                                    ( aTableInfo->mSplitMethod == SDI_SPLIT_SOLO  ) ? SDM_SOLOS:
                                    ( aTableInfo->mSplitMethod == SDI_SPLIT_CLONE ) ? SDM_CLONES:
                                    SDM_RANGES,
                                    & sSdmRanges,
                                    sSdmRangesIndex )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( sSdmRangesIndex[ ( aTableInfo->mSplitMethod == SDI_SPLIT_SOLO  ) ? SDM_SOLOS_IDX1_ORDER:
                                     ( aTableInfo->mSplitMethod == SDI_SPLIT_CLONE ) ? SDM_CLONES_IDX1_ORDER:
                                     SDM_RANGES_IDX1_ORDER ] == NULL,
                    ERR_META_HANDLE );

    IDE_TEST( smiGetTableColumns( sSdmRanges,
                                  ( aTableInfo->mSplitMethod == SDI_SPLIT_SOLO  ) ? SDM_SOLOS_SHARD_ID_COL_ORDER:
                                  ( aTableInfo->mSplitMethod == SDI_SPLIT_CLONE ) ? SDM_CLONES_SHARD_ID_COL_ORDER:
                                  SDM_RANGES_SHARD_ID_COL_ORDER,
                                  (const smiColumn**)&sShardIDColumn )
              != IDE_SUCCESS );

    IDE_TEST( smiGetTableColumns( sSdmRanges,
                                  ( aTableInfo->mSplitMethod == SDI_SPLIT_SOLO  ) ? SDM_SOLOS_SMN_COL_ORDER:
                                  ( aTableInfo->mSplitMethod == SDI_SPLIT_CLONE ) ? SDM_CLONES_SMN_COL_ORDER:
                                  SDM_RANGES_SMN_COL_ORDER,
                                  (const smiColumn**)&sSMNColumn )
              != IDE_SUCCESS );

    // mtdModule 설정
    IDE_TEST( mtd::moduleById( &(sShardIDColumn->module),
                               sShardIDColumn->type.dataTypeId )
              != IDE_SUCCESS );

    // mtlModule 설정
    IDE_TEST( mtl::moduleById( &sShardIDColumn->language,
                               sShardIDColumn->type.languageId )
              != IDE_SUCCESS );

    // mtdModule 설정
    IDE_TEST( mtd::moduleById( &(sSMNColumn->module),
                               sSMNColumn->type.dataTypeId )
              != IDE_SUCCESS );

    // mtlModule 설정
    IDE_TEST( mtl::moduleById( &sSMNColumn->language,
                               sSMNColumn->type.languageId )
              != IDE_SUCCESS );

    qciMisc::makeMetaRangeDoubleColumn(
        &sShardIDRange,
        &sSMNRange,
        sShardIDColumn,
        (const void *)&(aTableInfo->mShardID),
        sSMNColumn,
        (const void *)&aSMN,
        &sRange );

    sCursor.initialize();

    SMI_CURSOR_PROP_INIT_FOR_META_INDEX_SCAN( &sCursorProperty, NULL );

    IDE_TEST( sCursor.open(
                  QC_SMI_STMT( aStatement ),
                  sSdmRanges,
                  sSdmRangesIndex[( aTableInfo->mSplitMethod == SDI_SPLIT_SOLO  ) ? SDM_SOLOS_IDX1_ORDER:
                                  ( aTableInfo->mSplitMethod == SDI_SPLIT_CLONE ) ? SDM_CLONES_IDX1_ORDER:
                                  SDM_RANGES_IDX1_ORDER],
                  smiGetRowSCN( sSdmRanges ),
                  NULL,
                  &sRange,
                  smiGetDefaultKeyRange(),
                  smiGetDefaultFilter(),
                  QCM_META_CURSOR_FLAG,
                  SMI_SELECT_CURSOR,
                  &sCursorProperty )
              != IDE_SUCCESS );

    sIsCursorOpen = ID_TRUE;

    IDE_TEST( sCursor.beforeFirst() != IDE_SUCCESS );
    IDE_TEST( sCursor.readRow( &sRow, &sRid, SMI_FIND_NEXT )
              != IDE_SUCCESS );

    while ( sRow != NULL )
    {
        sExistCount++;
        IDE_TEST( sCursor.readRow( &sRow, &sRid, SMI_FIND_NEXT )
                  != IDE_SUCCESS );
    }

    if ( aTableInfo->mSplitMethod == SDI_SPLIT_SOLO )
    {
        IDE_TEST_RAISE( sExistCount >= 1, RANGE_COUNT_OVERFLOW );
    }
    else
    {
        IDE_TEST_RAISE( sExistCount >= SDI_RANGE_MAX_COUNT, RANGE_COUNT_OVERFLOW );
    }

    sIsCursorOpen = ID_FALSE;
    IDE_TEST( sCursor.close() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_META_HANDLE )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDM_SHARD_META_NOT_CREATED,
                                  ( aTableInfo->mSplitMethod == SDI_SPLIT_SOLO  ) ? SDM_SOLOS:
                                  ( aTableInfo->mSplitMethod == SDI_SPLIT_CLONE ) ? SDM_CLONES:
                                  SDM_RANGES ) );
    }
    IDE_EXCEPTION( RANGE_COUNT_OVERFLOW )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDM_SHARD_RANGE_OVERFLOW ) );
    }
    IDE_EXCEPTION_END;

    if ( sIsCursorOpen == ID_TRUE )
    {
        (void)sCursor.close();
    }

    return IDE_FAILURE;
}

IDE_RC sdm::validateParamBeforeInsert( qsOID   aProcOID,
                                       SChar * aUserName,
                                       SChar * aProcName,
                                       SChar * aSplitMethod,
                                       SChar * aKeyName,
                                       SChar * aSubSplitMethod,
                                       SChar * aSubKeyName )
{
    qsxProcInfo     * sProcInfo = NULL;
    qsVariableItems * sParaDecls = NULL;

    idBool sKeyFound = ID_FALSE;
    idBool sSubKeyFound = ID_FALSE;

    UInt   i = 0;

    SChar sParaDeclNameStr[QC_MAX_OBJECT_NAME_LEN + 1];

    IDE_DASSERT( idlOS::strlen( aSplitMethod ) == 1 );

    IDE_TEST( qsxProc::getProcInfo( aProcOID, &sProcInfo ) != IDE_SUCCESS );

    IDE_DASSERT( sProcInfo != NULL );

    if ( ( aSplitMethod[0] == 'H' ) ||  
         ( aSplitMethod[0] == 'R' ) ||
         ( aSplitMethod[0] == 'L' ) )
    {
        if ( idlOS::strlen( aSubSplitMethod ) == 1 )
        {
            IDE_DASSERT( ( aSubSplitMethod[0] == 'H' ) ||  
                         ( aSubSplitMethod[0] == 'R' ) ||
                         ( aSubSplitMethod[0] == 'L' ) );
        }
        else
        {
            IDE_DASSERT( aSubSplitMethod[0] == '\0' );
            // BUG-48749 찾을 필요가 없으면 ID_TRUE로 변경해서 찾지 않도록 한다.
            sSubKeyFound = ID_TRUE;
        }
    }
    else
    {
        IDE_DASSERT( ( aSplitMethod[0] == 'C' ) || ( aSplitMethod[0] == 'S' ) );
        // BUG-48749 찾을 필요가 없으면 ID_TRUE로 변경해서 찾지 않도록 한다.
        sKeyFound = ID_TRUE;
        sSubKeyFound = ID_TRUE;
    }

    for ( sParaDecls = sProcInfo->planTree->paraDecls, i = 0;
          sParaDecls != NULL;
          sParaDecls = sParaDecls->next, i++ )
    {
        if ( sKeyFound == ID_FALSE )
        {
            if ( idlOS::strMatch( sParaDecls->name.stmtText + sParaDecls->name.offset,
                                  sParaDecls->name.size,
                                  aKeyName,
                                  idlOS::strlen( aKeyName ) ) == 0 )
            {
                sKeyFound = ID_TRUE;
                IDE_TEST_RAISE( sParaDecls->itemType != QS_VARIABLE, ERR_INVALID_SHARD_KEY_TYPE );
                IDE_TEST_RAISE( ((qsVariables*)sParaDecls)->inOutType != QS_IN, ERR_INVALID_SHARD_KEY_TYPE );
                IDE_TEST_RAISE( isSupportDataType( sProcInfo->planTree->paramModules[i]->id ) == ID_FALSE,
                                ERR_INVALID_SHARD_KEY_TYPE );
            }
        }

        if ( sSubKeyFound == ID_FALSE )
        {
            if ( idlOS::strMatch( sParaDecls->name.stmtText + sParaDecls->name.offset,
                          sParaDecls->name.size,
                          aSubKeyName,
                          idlOS::strlen( aSubKeyName ) ) == 0 )
            {
                sSubKeyFound = ID_TRUE;
                IDE_TEST_RAISE( sParaDecls->itemType != QS_VARIABLE, ERR_INVALID_SHARD_KEY_TYPE );
                IDE_TEST_RAISE( ((qsVariables*)sParaDecls)->inOutType != QS_IN, ERR_INVALID_SHARD_KEY_TYPE );
                IDE_TEST_RAISE( isSupportDataType( sProcInfo->planTree->paramModules[i]->id ) == ID_FALSE,
                                ERR_INVALID_SHARD_SUBKEY_TYPE );
            }
        }

        // BUG-48749 User defined type의 parameter를 갖는 procedure는
        //           shard procedure로 등록할 수 없도록 제한합니다.
        IDE_TEST_RAISE( ((qsVariables*)sParaDecls)->variableType != QS_PRIM_TYPE,
                        ERR_NOT_SUPPORTED_TYPE );
    }

    IDE_TEST_RAISE( sKeyFound == ID_FALSE,
                    ERR_NOT_EXIST_SHARD_KEY );
    // BUG-48749 
    // Sub key는 미지원 하지만 관련 코드는 유지함.
    IDE_TEST_RAISE( sSubKeyFound == ID_FALSE,
                    ERR_NOT_EXIST_SHARD_KEY );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_EXIST_SHARD_KEY )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDM_SHARD_KEY_COLUMN_NOT_EXIST,
                                  aUserName,
                                  aProcName,
                                  ( sKeyFound == ID_FALSE )? aKeyName: aSubKeyName ) );
    }
    IDE_EXCEPTION( ERR_INVALID_SHARD_KEY_TYPE )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDM_UNSUPPORTED_SHARD_KEY_COLUMN_TYPE,
                                  aUserName,
                                  aProcName,
                                  aKeyName ) );
    }
    IDE_EXCEPTION( ERR_INVALID_SHARD_SUBKEY_TYPE )
    {
        // BUG-48749 
        // Sub key는 미지원 하지만 관련 코드는 유지함.
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDM_UNSUPPORTED_SHARD_KEY_COLUMN_TYPE,
                                  aUserName,
                                  aProcName,
                                  aSubKeyName ) );
    }
    IDE_EXCEPTION( ERR_NOT_SUPPORTED_TYPE )
    {
        idlOS::strncpy( sParaDeclNameStr,
                        (sParaDecls->name.stmtText + sParaDecls->name.offset),
                        sParaDecls->name.size );
        sParaDeclNameStr[sParaDecls->name.size] = '\0';
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDM_UNSUPPORTED_SHARD_PROCEDURE_PARAMETER_TYPE,
                                  aUserName,
                                  aProcName,
                                  sParaDeclNameStr ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdm::validateColumnBeforeInsert( qcStatement  * aStatement,
                                        UInt           aUserID,
                                        SChar        * aTableName,
                                        SChar        * aSplitMethod,
                                        SChar        * aKeyName,
                                        SChar        * aSubSplitMethod,
                                        SChar        * aSubKeyName )
{
    smSCN          sTableSCN;
    void         * sTableHandle = NULL;
    qcmTableInfo * sTableInfo = NULL;
    idBool         sKeyFound = ID_FALSE;
    idBool         sSubKeyFound = ID_FALSE;
    UInt           i = 0;

    IDE_DASSERT( idlOS::strlen( aSplitMethod ) == 1 );

    IDE_TEST( qciMisc::getTableInfo( aStatement,
                                     aUserID,
                                     (UChar*)aTableName,
                                     idlOS::strlen( aTableName ),
                                     &sTableInfo,
                                     &sTableSCN,
                                     &sTableHandle )
              != IDE_SUCCESS );

    IDE_DASSERT( sTableInfo != NULL );

    if ( ( aSplitMethod[0] == 'H' ) ||  
         ( aSplitMethod[0] == 'R' ) ||
         ( aSplitMethod[0] == 'L' ) )
    {
        for ( i = 0; i < sTableInfo->columnCount; i++ )
        {
            if ( idlOS::strMatch( sTableInfo->columns[i].name,
                                  idlOS::strlen( sTableInfo->columns[i].name ),
                                  aKeyName,
                                  idlOS::strlen( aKeyName ) ) == 0 )
            {
                sKeyFound = ID_TRUE;
                IDE_TEST_RAISE( isSupportDataType( sTableInfo->columns[i].basicInfo->module->id ) == ID_FALSE,
                                ERR_INVALID_SHARD_KEY_TYPE );
                break;
            }
        }

        IDE_TEST_RAISE( sKeyFound == ID_FALSE, ERR_NOT_EXIST_SHARD_KEY );

        if ( idlOS::strlen( aSubSplitMethod ) == 1 )
        {
            IDE_DASSERT( ( aSubSplitMethod[0] == 'H' ) ||  
                         ( aSubSplitMethod[0] == 'R' ) ||
                         ( aSubSplitMethod[0] == 'L' ) );

            for ( i = 0; i < sTableInfo->columnCount; i++ )
            {
                if ( idlOS::strMatch( sTableInfo->columns[i].name,
                                      idlOS::strlen( sTableInfo->columns[i].name ),
                                      aSubKeyName,
                                      idlOS::strlen( aSubKeyName ) ) == 0 )
                {
                    sSubKeyFound = ID_TRUE;
                    IDE_TEST_RAISE( isSupportDataType( sTableInfo->columns[i].basicInfo->module->id ) == ID_FALSE,
                                    ERR_INVALID_SHARD_KEY_TYPE );
                    break;
                }
            }

            IDE_TEST_RAISE( sSubKeyFound == ID_FALSE, ERR_NOT_EXIST_SHARD_KEY );
        }
        else
        {
            IDE_DASSERT( aSubSplitMethod[0] == '\0' );
        }
    }
    else
    {
        IDE_DASSERT( ( aSplitMethod[0] == 'C' ) || ( aSplitMethod[0] == 'S' ) );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_EXIST_SHARD_KEY )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDM_SHARD_KEY_COLUMN_NOT_EXIST,
                                  sTableInfo->tableOwnerName,
                                  sTableInfo->name,
                                  ( sKeyFound == ID_FALSE )? aKeyName: aSubKeyName ) );
    }
    IDE_EXCEPTION( ERR_INVALID_SHARD_KEY_TYPE )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDM_UNSUPPORTED_SHARD_KEY_COLUMN_TYPE,
                                  sTableInfo->tableOwnerName,
                                  sTableInfo->name,
                                  ( sSubKeyFound == ID_TRUE )? aSubKeyName: aKeyName ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

idBool sdm::isSupportDataType( UInt aModuleID )
{
    idBool sResult = ID_FALSE;

    if ( ( aModuleID == MTD_SMALLINT_ID ) ||
         ( aModuleID == MTD_INTEGER_ID  ) ||
         ( aModuleID == MTD_BIGINT_ID   ) ||
         ( aModuleID == MTD_CHAR_ID     ) ||
         ( aModuleID == MTD_VARCHAR_ID  ) )
    {
        sResult = ID_TRUE;
    }

    return sResult;
}

/*
* 파티션 메타에서 파티션 split method를 가져와서 매칭 되는 샤드 split method를 aOutSplitMethod에 반환한다.
*/
IDE_RC sdm::getSplitMethodByPartition( smiStatement   * aStatement,
                                       SChar          * aUserName,
                                       SChar          * aTableName,
                                       sdiSplitMethod * aOutSplitMethod )
{
    UInt           sUserID = 0;
    smSCN          sTableSCN;
    void         * sTableHandle = NULL;
    qcmTableInfo * sTableInfo = NULL;
    sdiSplitMethod sShardSplitMethod = SDI_SPLIT_NONE;

    smiStatement   sSmiStmt;
    UInt           sSmiStmtFlag = 0;
    SInt           sState = 0;

    qcStatement    sStatement;

    sSmiStmtFlag = SMI_STATEMENT_NORMAL | SMI_STATEMENT_MEMORY_CURSOR;
    IDE_TEST( qcg::allocStatement( &sStatement,
                                   NULL,
                                   NULL,
                                   NULL )
              != IDE_SUCCESS );
    sState = 1;

    qcg::setSmiStmt(&sStatement, &sSmiStmt);

    IDE_TEST( sSmiStmt.begin( NULL,
                              aStatement,
                              sSmiStmtFlag )
              != IDE_SUCCESS );
    sState = 2;

    IDE_TEST_RAISE( qcmUser::getUserID( NULL,
                                        &sSmiStmt,
                                        aUserName,
                                        idlOS::strlen(aUserName),
                                        & sUserID ) != IDE_SUCCESS,
                    ERR_META_HANDLE );

    IDE_TEST( qcm::getTableHandleByName( &sSmiStmt,
                                         sUserID,
                                         (UChar *)aTableName,
                                         idlOS::strlen(aTableName),
                                         (void**)&sTableHandle,
                                         &sTableSCN )
              != IDE_SUCCESS );

    IDE_TEST(smiValidateAndLockObjects( sSmiStmt.getTrans(),
                                        sTableHandle,
                                        sTableSCN,
                                        SMI_TBSLV_DDL_DML,
                                        SMI_TABLE_LOCK_IS,
                                        ID_ULONG_MAX,
                                        ID_FALSE)
             != IDE_SUCCESS);

    IDE_TEST( smiGetTableTempInfo( sTableHandle,
                                   (void**)&sTableInfo )
              != IDE_SUCCESS );

    switch (sTableInfo->partitionMethod)
    {
        case QCM_PARTITION_METHOD_RANGE:
            sShardSplitMethod = SDI_SPLIT_RANGE;
            break;
        case QCM_PARTITION_METHOD_LIST:
            sShardSplitMethod = SDI_SPLIT_LIST;
            break;
        case QCM_PARTITION_METHOD_RANGE_USING_HASH:
            sShardSplitMethod = SDI_SPLIT_HASH;
            break;
        case QCM_PARTITION_METHOD_NONE:
            sShardSplitMethod = SDI_SPLIT_NONE;
            break;
        case QCM_PARTITION_METHOD_HASH:
            IDE_RAISE(ERR_DO_NOT_MATCH_SPLIT_METHOD);
            break;
        default:
            IDE_RAISE(ERR_DO_NOT_MATCH_SPLIT_METHOD);
            break;
    }

    IDE_TEST( sSmiStmt.end( SMI_STATEMENT_RESULT_SUCCESS ) != IDE_SUCCESS );
    sState = 1;

    sState = 0;
    IDE_TEST( qcg::freeStatement( &sStatement ) != IDE_SUCCESS );

    *aOutSplitMethod = sShardSplitMethod;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_DO_NOT_MATCH_SPLIT_METHOD )
    {
        IDE_SET( ideSetErrorCode(sdERR_ABORT_SDM_DO_NOT_MATCH_SPLIT_METHOD) );
    }
    IDE_EXCEPTION( ERR_META_HANDLE )
    {
        if ( ideGetErrorCode() == qpERR_ABORT_QCM_NOT_EXIST_USER )
        {
            IDE_SET( ideSetErrorCode(sdERR_ABORT_SDM_SHARD_META_NOT_CREATED,
                                     SDM_USER) );
        }
        else if ( ideGetErrorCode() == qpERR_ABORT_QCM_NOT_EXIST_TABLE )
        {
            IDE_SET( ideSetErrorCode(sdERR_ABORT_SDM_SHARD_META_NOT_CREATED,
                                     aTableName) );
        }
    }
    IDE_EXCEPTION_END;

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
            (void) qcg::freeStatement( &sStatement );
        default:
            break;
    }

    return IDE_FAILURE;
}

/*
* 파티션 메타에서 파티션 Key를 가져와서 샤드 key를 선택한다.
*/
IDE_RC sdm::getShardKeyStrByPartition( smiStatement * aStatement,
                                       SChar        * aUserName,
                                       SChar        * aTableName,
                                       UInt           aSharKeyBufLen,
                                       SChar        * aOutShardKeyBuf )
{
    UInt   sUserID = 0;
    smSCN  sTableSCN;
    void * sTableHandle = NULL;
    UInt   sPartitionKeyLen = 0;
    qcmTableInfo * sTableInfo = NULL;

    qcmPartitionInfoList * sTablePartInfoList     = NULL;
    qcmPartitionInfoList * sTempTablePartInfoList     = NULL;
    qcmTableInfo         * sPartInfo           = NULL;

    smiStatement           sSmiStmt;
    UInt                   sSmiStmtFlag = 0;
    SInt                   sState = 0;

    qcStatement            sStatement;

    sSmiStmtFlag = SMI_STATEMENT_NORMAL | SMI_STATEMENT_MEMORY_CURSOR;

    IDE_TEST( qcg::allocStatement( &sStatement,
                                   NULL,
                                   NULL,
                                   NULL )
              != IDE_SUCCESS );
    sState = 1;

    qcg::setSmiStmt(&sStatement, &sSmiStmt);

    IDE_TEST( sSmiStmt.begin( NULL,
                              aStatement,
                              sSmiStmtFlag )
              != IDE_SUCCESS );
    sState = 2;

    IDE_TEST_RAISE( qcmUser::getUserID( NULL,
                                        &sSmiStmt,
                                        aUserName,
                                        idlOS::strlen(aUserName),
                                        & sUserID ) != IDE_SUCCESS,
                    ERR_META_HANDLE );

    IDE_TEST( qcm::getTableHandleByName( &sSmiStmt,
                                         sUserID,
                                         (UChar *)aTableName,
                                         idlOS::strlen(aTableName),
                                         (void**)&sTableHandle,
                                         &sTableSCN ) 
              != IDE_SUCCESS );

    IDE_TEST(smiValidateAndLockObjects( sSmiStmt.getTrans(),
                                        sTableHandle,
                                        sTableSCN,
                                        SMI_TBSLV_DDL_DML,
                                        SMI_TABLE_LOCK_IS,
                                        ID_ULONG_MAX,
                                        ID_FALSE)
             != IDE_SUCCESS);

    IDE_TEST( smiGetTableTempInfo( sTableHandle,
                                   (void**)&sTableInfo )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( sTableInfo->tablePartitionType == QCM_NONE_PARTITIONED_TABLE, ERR_GET_SHARD_KEY );

    IDE_TEST( qcmPartition::getPartitionInfoList( &sStatement,
                                                  &sSmiStmt,
                                                  (iduMemory *)QCI_QMX_MEM( &sStatement ),
                                                  sTableInfo->tableID,
                                                  &sTablePartInfoList,
                                                  ID_TRUE /* aIsRealPart */ )
              != IDE_SUCCESS );

    aOutShardKeyBuf[0] = '\0';
    for ( sTempTablePartInfoList = sTablePartInfoList;
          sTempTablePartInfoList != NULL;
          sTempTablePartInfoList = sTempTablePartInfoList->next )
    {
        sPartInfo = sTempTablePartInfoList->partitionInfo;

        IDE_TEST_RAISE(sPartInfo->partKeyColCount != 1, ERR_PARTITION_KEY_COUNT);
        sPartitionKeyLen = idlOS::strlen(sPartInfo->partKeyColumns[0].name);
        IDE_TEST_RAISE(aSharKeyBufLen < sPartitionKeyLen + 1, ERR_OUT_SHARKEY_BUFLEN);
        if ( aOutShardKeyBuf[0] == '\0' )
        {
            idlOS::strncpy(aOutShardKeyBuf, sPartInfo->partKeyColumns[0].name, sPartitionKeyLen);
        }
        else
        {
            IDE_TEST_RAISE(idlOS::strncmp(aOutShardKeyBuf,
                                          sPartInfo->partKeyColumns[0].name,
                                          sPartitionKeyLen) != 0,
                           ERR_PARTITION_KEY_DIFF);
        }

    }

    aOutShardKeyBuf[sPartitionKeyLen] = '\0';

    IDE_TEST( sSmiStmt.end( SMI_STATEMENT_RESULT_SUCCESS ) != IDE_SUCCESS );
    sState = 1;

    sState = 0;
    IDE_TEST( qcg::freeStatement( &sStatement ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_GET_SHARD_KEY )
    {
        IDE_SET( ideSetErrorCode(sdERR_ABORT_SDM_DO_NOT_MATCH_SPLIT_METHOD) );
    }
    IDE_EXCEPTION( ERR_PARTITION_KEY_COUNT )
    {
        IDE_SET( ideSetErrorCode(sdERR_ABORT_SDM_PARTITION_KEY_COUNT) );
    }
    IDE_EXCEPTION( ERR_OUT_SHARKEY_BUFLEN )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                  "sdm::getShardKeyByPartition",
                                  "Out Shard Key Buffer is too short." ) );
    }
    IDE_EXCEPTION( ERR_META_HANDLE )
    {
        if ( ideGetErrorCode() == qpERR_ABORT_QCM_NOT_EXIST_USER )
        {
            IDE_SET( ideSetErrorCode(sdERR_ABORT_SDM_SHARD_META_NOT_CREATED,
                                     SDM_USER) );
        }
        else if ( ideGetErrorCode() == qpERR_ABORT_QCM_NOT_EXIST_TABLE )
        {
            IDE_SET( ideSetErrorCode(sdERR_ABORT_SDM_SHARD_META_NOT_CREATED,
                                     aTableName) );
        }
    }
    IDE_EXCEPTION( ERR_PARTITION_KEY_DIFF )
    {
        IDE_SET( ideSetErrorCode(sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                 "sdm::getShardKeyStrByPartition","partition key name is diffrent") );

    }
    IDE_EXCEPTION_END;

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
            (void) qcg::freeStatement( &sStatement );
        default:
            break;
    }
    return IDE_FAILURE;

}
/***************************************************************************
* input: aUserName, aTableName, aValue, aIsDefault
* output: aOutTablePartitionType, aOutPartitionName
*
* input 값을 이용해서 aValue에 해당하는 파티션의 이름을 aOutPartitionName에 얻어오고,
* non-partitioned table인 경우 aOutTablePartitionType에 받아온다.
* aTableName에 aValue에 해당하는 파티션이 없는 경우에 aOutPartitionName에 SDM_NA_STR을 반환한다.
* list 파티션의 경우에도 동일해야만 파티션 이름을 찾을 수 있으며 그렇지 않은 경우 에러이다.
***************************************************************************/
IDE_RC sdm::getPartitionNameByValue( smiStatement            * aStatement,
                                     SChar                   * aUserName,
                                     SChar                   * aTableName,
                                     SChar                   * aValue,
                                     qcmTablePartitionType   * aOutTablePartitionType,
                                     SChar                   * aOutPartitionName )
{
    qcmTableInfo         * sTableInfo = NULL;
    void                 * sTableHandle = NULL;
    qcmPartitionInfoList * sPartInfoList = NULL;
    qcmPartitionInfoList * sTempPartInfoList = NULL;
    qcmTableInfo         * sPartInfo = NULL;
    SChar                  sPartCondMinValue[QC_MAX_PARTKEY_COND_VALUE_LEN + 1];
    SChar                  sPartCondMaxValue[QC_MAX_PARTKEY_COND_VALUE_LEN + 1];
    SChar                * sPartitionName = NULL;
    SInt                   sCompareResult = -1;
    qmsPartCondValList     sPartCondVal;
    qcCondValueCharBuffer  sBuffer;
    mtdCharType          * sPartKeyCondValueStr = (mtdCharType*) & sBuffer;
    qcmTablePartitionType  sTablePartitionType = QCM_PARTITIONED_TABLE;

    smiStatement           sSmiStmt;
    UInt                   sSmiStmtFlag = 0;
    SInt                   sState = 0;

    qcStatement            sStatement;

    sSmiStmtFlag = SMI_STATEMENT_NORMAL | SMI_STATEMENT_MEMORY_CURSOR;
    IDE_TEST( qcg::allocStatement( &sStatement,
                                   NULL,
                                   NULL,
                                   NULL )
              != IDE_SUCCESS );
    sState = 1;

    qcg::setSmiStmt(&sStatement, &sSmiStmt);

    IDE_TEST( sSmiStmt.begin( NULL,
                              aStatement,
                              sSmiStmtFlag )
              != IDE_SUCCESS );
    sState = 2;

    IDE_TEST( qciMisc::getTableInfoAndLock( &sStatement,
                                            aUserName,
                                            aTableName,
                                            SMI_TABLE_LOCK_IS,
                                            ID_ULONG_MAX,
                                            &sTableInfo,
                                            &sTableHandle ) 
              != IDE_SUCCESS );

    sTablePartitionType = sTableInfo->tablePartitionType;
    IDE_TEST_CONT( sTablePartitionType != QCM_PARTITIONED_TABLE, SKIP_NON_PARTITIONED_TBL );

    switch (sTableInfo->partitionMethod)
    {
        case QCM_PARTITION_METHOD_RANGE:
        case QCM_PARTITION_METHOD_RANGE_USING_HASH:
        case QCM_PARTITION_METHOD_LIST:
            /* do nothing: go next step */
            break;
        case QCM_PARTITION_METHOD_HASH:
        case QCM_PARTITION_METHOD_NONE:
            IDE_RAISE(ERR_METHOD);
            break;
        default:
            IDE_DASSERT(0);
            IDE_RAISE(ERR_METHOD);
            break;
    }

    IDE_TEST( qcmPartition::getPartitionInfoList( &sStatement,
                                                  &sSmiStmt,
                                                  ( iduMemory * )QCI_QMX_MEM( &sStatement ),
                                                  sTableInfo->tableID,
                                                  &sPartInfoList,
                                                  ID_TRUE /* aIsRealPart */ )
              != IDE_SUCCESS );

    IDE_TEST( qcmPartition::validateAndLockPartitionInfoList( &sStatement,
                                                              sPartInfoList,
                                                              SMI_TBSLV_DDL_DML,
                                                              SMI_TABLE_LOCK_IS,
                                                              smiGetDDLLockTimeOut(QC_SMI_STMT(&sStatement)->getTrans()))
              != IDE_SUCCESS );

    for( sTempPartInfoList = sPartInfoList;
         sTempPartInfoList != NULL;
         sTempPartInfoList = sTempPartInfoList->next )
    {
        sPartInfo = sTempPartInfoList->partitionInfo;

        IDE_DASSERT(sPartInfo->tablePartitionType == QCM_TABLE_PARTITION);

        sPartCondMinValue[0] = '\0';
        sPartCondMaxValue[0] = '\0';
        sCompareResult = -1; /* not match */
        /* partition method는 sys_part_tables에만 존재하므로 각 partition에서 봐야 함
         * 파티션 메소드 별로 동일한지 비교함.
         * list partition의 경우 자동 확장은 1개의 파티션에 1나의 조건만 들어있을 때에만 가능하다.
         * 그렇지 않은 경우에는 확장 할 수 없으므로 에러를 반환한다. */
        switch (sPartInfo->partitionMethod)
        {
            case QCM_PARTITION_METHOD_RANGE:
            case QCM_PARTITION_METHOD_RANGE_USING_HASH:
            case QCM_PARTITION_METHOD_LIST:
                IDE_TEST(qcmPartition::getPartMinMaxValue( &sSmiStmt,
                                                           sPartInfo->partitionID,
                                                           sPartCondMinValue,
                                                           sPartCondMaxValue )
                         != IDE_SUCCESS);
                //일단  split method range와 hash 만 고려하고 있음. list는 고민 후 추후 처리가 필요함.
                IDE_TEST(qciMisc::comparePartCondValues( (idvSQL*)NULL,
                                                         sTableInfo,
                                                         aValue,
                                                         sPartCondMaxValue,
                                                         &sCompareResult) != IDE_SUCCESS);

                qtc::setVarcharValue( sPartKeyCondValueStr,
                                      NULL,
                                      (SChar*)sPartCondMaxValue,
                                      idlOS::strlen( sPartCondMaxValue) );

                IDE_TEST( qcmPartition::getPartCondVal( &sStatement,
                                                        sTableInfo->partKeyColumns,
                                                        sTableInfo->partitionMethod,
                                                        &sPartCondVal,
                                                        NULL, /* aPartCondValStmtText */
                                                        NULL, /* aPartCondValNode */
                                                        sPartKeyCondValueStr )
                          != IDE_SUCCESS );
                break;
            case QCM_PARTITION_METHOD_HASH:
            case QCM_PARTITION_METHOD_NONE:
                IDE_RAISE(ERR_METHOD);
                break;
            default:
                IDE_DASSERT(0);
                IDE_RAISE(ERR_METHOD);
                break;
        }

        if ( sCompareResult == 0 ) /* match */
        {
            sPartitionName = (SChar*)sPartInfo->name;
            break;
        }
        else
        {
            sPartitionName = NULL;
        }
    }

    IDE_EXCEPTION_CONT(SKIP_NON_PARTITIONED_TBL);

    IDE_TEST( sSmiStmt.end( SMI_STATEMENT_RESULT_SUCCESS ) != IDE_SUCCESS );
    sState = 1;

    sState = 0;
    IDE_TEST( qcg::freeStatement( &sStatement ) != IDE_SUCCESS );

    *aOutTablePartitionType = sTablePartitionType;

    if ( sPartitionName != NULL )
    {
        idlOS::strncpy( aOutPartitionName,
                        sPartitionName,
                        QC_MAX_OBJECT_NAME_LEN + 1);
    }
    else
    {
        idlOS::strncpy( aOutPartitionName,
                        SDM_NA_STR,
                        QC_MAX_OBJECT_NAME_LEN + 1);
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_METHOD )
    {
        IDE_SET( ideSetErrorCode(sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                 "sdm::getPartitionNameByValue","partition method error") );

    }
    IDE_EXCEPTION_END;

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
            (void) qcg::freeStatement( &sStatement );
        default:
            break;
    }
    return IDE_FAILURE;
}

/***************************************************************************
* input: aUserName, aTableName, aPartitionName
* output: aOutValueBuf
*
* input 값(aPartitionName)을 이용해서 aValue에 해당하는 파티션의 Max 값을 aOutValueBuf에 복사해 준다.
* list 파티션의 경우에는 한 개 초과 일때 에러를 반환한다.
*
***************************************************************************/
IDE_RC sdm::getPartitionValueByName( smiStatement  * aStatement,
                                     SChar         * aUserName,
                                     SChar         * aTableName,
                                     SChar         * aPartitionName,
                                     SInt            aOutValueBufLen,
                                     SChar         * aOutValueBuf,
                                     idBool        * aOutIsDefault )
{
    qcmTableInfo         * sTableInfo = NULL;
    void                 * sTableHandle = NULL;
    qcmPartitionInfoList * sPartInfoList = NULL;
    qcmPartitionInfoList * sTempPartInfoList = NULL;
    qcmTableInfo         * sPartInfo = NULL;
    SChar                  sPartCondMinValue[QC_MAX_PARTKEY_COND_VALUE_LEN + 1];
    SChar                  sPartCondMaxValue[QC_MAX_PARTKEY_COND_VALUE_LEN + 1];
    SChar                * sValue = NULL;
    SInt                   sValueLen = 0;
    qmsPartCondValList     sPartCondVal;
    qcCondValueCharBuffer  sBuffer;
    mtdCharType          * sPartKeyCondValueStr = (mtdCharType*) & sBuffer;
    idBool                 sIsDefault = ID_FALSE;

    smiStatement           sSmiStmt;
    UInt                   sSmiStmtFlag = 0;
    SInt                   sState = 0;

    qcStatement            sStatement;

    sSmiStmtFlag = SMI_STATEMENT_NORMAL | SMI_STATEMENT_MEMORY_CURSOR;

    IDE_TEST( qcg::allocStatement( &sStatement,
                                   NULL,
                                   NULL,
                                   NULL )
              != IDE_SUCCESS );
    sState = 1;

    qcg::setSmiStmt(&sStatement, &sSmiStmt);

    IDE_TEST( sSmiStmt.begin( NULL,
                              aStatement,
                              sSmiStmtFlag )
              != IDE_SUCCESS );
    sState = 2;

    IDE_TEST( qciMisc::getTableInfoAndLock( &sStatement,
                                            aUserName,
                                            aTableName,
                                            SMI_TABLE_LOCK_IS,
                                            ID_ULONG_MAX,
                                            &sTableInfo,
                                            &sTableHandle) != IDE_SUCCESS );

    IDE_TEST_RAISE( sTableInfo->tablePartitionType != QCM_PARTITIONED_TABLE, ERR_NOT_PARTITIONED_TBL );

    switch (sTableInfo->partitionMethod)
    {
        case QCM_PARTITION_METHOD_RANGE:
        case QCM_PARTITION_METHOD_RANGE_USING_HASH:
        case QCM_PARTITION_METHOD_LIST:
            /* do nothing: go next step */
            break;
        case QCM_PARTITION_METHOD_HASH:
        case QCM_PARTITION_METHOD_NONE:
            IDE_RAISE(ERR_METHOD);
            break;
        default:
            IDE_DASSERT(0);
            IDE_RAISE(ERR_METHOD);
            break;
    }

    IDE_TEST( qcmPartition::getPartitionInfoList( &sStatement,
                                                  &sSmiStmt,
                                                  ( iduMemory * )QCI_QMX_MEM( &sStatement ),
                                                  sTableInfo->tableID,
                                                  &sPartInfoList,
                                                  ID_TRUE /* aIsRealPart */ )
              != IDE_SUCCESS );

    IDE_TEST( qcmPartition::validateAndLockPartitionInfoList( &sStatement,
                                                              sPartInfoList,
                                                              SMI_TBSLV_DDL_DML,
                                                              SMI_TABLE_LOCK_IS,
                                                              smiGetDDLLockTimeOut(QC_SMI_STMT(&sStatement)->getTrans()))
              != IDE_SUCCESS );

    for( sTempPartInfoList = sPartInfoList;
         sTempPartInfoList != NULL;
         sTempPartInfoList = sTempPartInfoList->next )
    {
        sPartInfo = sTempPartInfoList->partitionInfo;

        IDE_DASSERT(sPartInfo->tablePartitionType == QCM_TABLE_PARTITION);

        sPartCondMinValue[0] = '\0';
        sPartCondMaxValue[0] = '\0';
        sValue = NULL;
        sIsDefault = ID_FALSE;

        if ( idlOS::strncmp(aPartitionName, (SChar*)sPartInfo->name, QC_MAX_OBJECT_NAME_LEN) == 0 )
        {
            /* partition method는 sys_part_tables에만 존재하므로 각 partition에서 봐야 함
             * 파티션 메소드 별로 동일한지 비교함.
             * list partition의 경우 자동 확장은 1개의 파티션에 1나의 조건만 들어있을 때에만 가능하다.
             * 그렇지 않은 경우에는 확장 할 수 없으므로 에러를 반환한다. */
            switch (sPartInfo->partitionMethod)
            {
                case QCM_PARTITION_METHOD_RANGE:
                case QCM_PARTITION_METHOD_RANGE_USING_HASH:
                case QCM_PARTITION_METHOD_LIST:
                    IDE_TEST(qcmPartition::getPartMinMaxValue( &sSmiStmt,
                                                               sPartInfo->partitionID,
                                                               sPartCondMinValue,
                                                               sPartCondMaxValue)
                             != IDE_SUCCESS);

                    qtc::setVarcharValue( sPartKeyCondValueStr,
                                          NULL,
                                          (SChar*)sPartCondMaxValue,
                                          idlOS::strlen( sPartCondMaxValue) );

                    IDE_TEST( qcmPartition::getPartCondVal( &sStatement,
                                                            sTableInfo->partKeyColumns,
                                                            sTableInfo->partitionMethod,
                                                            &sPartCondVal,
                                                            NULL, /* aPartCondValStmtText */
                                                            NULL, /* aPartCondValNode */
                                                            sPartKeyCondValueStr )
                              != IDE_SUCCESS );

                    /* list partition의 경우 하나의 파티션에 조건이 한 개 보다 많으면 에러를 반환한다.
                     * range, list, range hash에서 조건이 없으면 default 파티션이다. */
                    IDE_TEST_RAISE( sPartCondVal.partCondValCount > 1 , ERR_PART_COND_COUNT);

                    if ( sPartCondVal.partCondValCount == 0 )
                    {
                        IDE_DASSERT(sPartCondMaxValue[0] == '\0');
                        sIsDefault = ID_TRUE;
                    }
                    else
                    {
                        sIsDefault = ID_FALSE;
                    }
                    sValue = sPartCondMaxValue;
                    break;
                case QCM_PARTITION_METHOD_HASH:
                case QCM_PARTITION_METHOD_NONE:
                    IDE_RAISE(ERR_METHOD);
                    break;
                default:
                    IDE_DASSERT(0);
                    IDE_RAISE(ERR_METHOD);
                    break;
            }

            break;
        }
        else
        {
            /* do nothing: next partition */
        }
    }

    IDE_TEST( sSmiStmt.end( SMI_STATEMENT_RESULT_SUCCESS ) != IDE_SUCCESS );
    sState = 1;

    sState = 0;
    IDE_TEST( qcg::freeStatement( &sStatement ) != IDE_SUCCESS );

    if ( sIsDefault == ID_TRUE )
    {
        aOutValueBuf[0] = '\0';
        *aOutIsDefault = ID_TRUE;
    }
    else
    {
        IDE_TEST_RAISE(sValue == NULL, ERR_NO_FOUND_VALUE);
        IDE_TEST_RAISE(aOutValueBufLen < SDI_RANGE_VARCHAR_MAX_PRECISION + 1, ERR_BUF_LEN);
        sValueLen = (SInt)idlOS::strlen(sValue);
        IDE_TEST_RAISE( sValueLen + 2 > aOutValueBufLen , ERR_BUF_LEN);
        
        idlOS::snprintf( aOutValueBuf,
                         sValueLen + 1,
                         sValue );
        
        *aOutIsDefault = ID_FALSE;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_PARTITIONED_TBL )
    {
        IDE_SET( ideSetErrorCode(sdERR_ABORT_SDM_UNSUPPORTED_SHARD_TABLE_TYPE) );
    }
    IDE_EXCEPTION( ERR_METHOD )
    {
        IDE_SET( ideSetErrorCode(sdERR_ABORT_SDM_DO_NOT_MATCH_SPLIT_METHOD) );
    }
    IDE_EXCEPTION( ERR_PART_COND_COUNT )
    {
        IDE_SET( ideSetErrorCode(sdERR_ABORT_SDM_PARTITION_KEY_COND_COUNT, aPartitionName) );
    }
    IDE_EXCEPTION( ERR_NO_FOUND_VALUE )
    {
        IDE_SET( ideSetErrorCode(sdERR_ABORT_SDM_SHARD_PARTITION_NOT_EXIST, aTableName, aPartitionName) );
    }
    IDE_EXCEPTION( ERR_BUF_LEN )
    {
        IDE_SET( ideSetErrorCode(sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                 "sdm::getPartitionValueByName","out value buffer length error") );
    }
    IDE_EXCEPTION_END;

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
            (void) qcg::freeStatement( &sStatement );
        default:
            break;
    }

    return IDE_FAILURE;
}

// add Repl Table
IDE_RC sdm::addReplTable( qcStatement * aStatement,
                          SChar       * aNodeName,
                          SChar       * aReplName,
                          SChar       * aUserName,
                          SChar       * aTableName,
                          SChar       * aPartitionName,
                          sdmReplDirectionalRole aRole,
                          idBool        aIsNewTrans )
{
    SChar      * sSqlStr;
    UInt         sUserID = QC_EMPTY_USER_ID;
    SChar        sTableNameStr[QC_MAX_OBJECT_NAME_LEN + 1];
    SChar        sTargetTableNameStr[QC_MAX_OBJECT_NAME_LEN + 1];
    UInt         sExecCount = 0;
    sdiLocalMetaInfo    sLocalMetaInfo;
    
    switch (aRole)
    {
        case SDM_REPL_SENDER:
            idlOS::strncpy( sTableNameStr, aTableName, QC_MAX_OBJECT_NAME_LEN + 1 );
            idlOS::snprintf( sTargetTableNameStr,
                             QC_MAX_OBJECT_NAME_LEN+1,
                             "%s%s",
                             SDI_BACKUP_TABLE_PREFIX,
                             aTableName );
            break;
        case SDM_REPL_RECEIVER:
            idlOS::strncpy( sTargetTableNameStr, aTableName, QC_MAX_OBJECT_NAME_LEN + 1 );
            idlOS::snprintf( sTableNameStr,
                             QC_MAX_OBJECT_NAME_LEN+1,
                             "%s%s",
                             SDI_BACKUP_TABLE_PREFIX,
                             aTableName );
            break;
        case SDM_REPL_CLONE:
            /* CLONE Table은 Failover/Failback 시에만 OriTable to OriTable로 맺어져야한다. */
            idlOS::strncpy( sTableNameStr, aTableName, QC_MAX_OBJECT_NAME_LEN + 1 );
            idlOS::strncpy( sTargetTableNameStr, aTableName, QC_MAX_OBJECT_NAME_LEN + 1 );
            break;
        default:
            IDE_DASSERT(0);
            break;
    }

    IDE_TEST( STRUCT_ALLOC_WITH_SIZE( aStatement->qmxMem,
                                      SChar,
                                      QD_MAX_SQL_LENGTH,
                                      &sSqlStr )
              != IDE_SUCCESS);

    if ( idlOS::strncmp( aPartitionName, SDM_NA_STR, QC_MAX_OBJECT_NAME_LEN + 1) != 0 )
    {
        idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                         "alter replication "QCM_SQL_STRING_SKIP_FMT" add table from "QCM_SQL_STRING_SKIP_FMT"."QCM_SQL_STRING_SKIP_FMT" partition "QCM_SQL_STRING_SKIP_FMT" to "QCM_SQL_STRING_SKIP_FMT"."QCM_SQL_STRING_SKIP_FMT" partition "QCM_SQL_STRING_SKIP_FMT"",
                         aReplName,           /* REPLICATION_NAME  */
                         aUserName,           /* Src UserName      */
                         sTableNameStr,       /* Src TableName     */
                         aPartitionName,      /* Src PartitionName */
                         aUserName,           /* Dst UserName      */
                         sTargetTableNameStr, /* Dst TableName     */
                         aPartitionName       /* Dst PartitionName */ );

    }
    else
    {
        idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                         "alter replication "QCM_SQL_STRING_SKIP_FMT" add table from "QCM_SQL_STRING_SKIP_FMT"."QCM_SQL_STRING_SKIP_FMT" to "QCM_SQL_STRING_SKIP_FMT"."QCM_SQL_STRING_SKIP_FMT" ",
                         aReplName,           /* REPLICATION_NAME */
                         aUserName,           /* Src UserName     */
                         sTableNameStr,       /* Src TableName    */
                         aUserName,           /* Dst UserName     */
                         sTargetTableNameStr  /* Dst TableName    */ );
    }

    IDE_TEST( qciMisc::getUserIdByName( aUserName, &sUserID ) != IDE_SUCCESS );

    IDE_TEST( sdi::getLocalMetaInfo( &sLocalMetaInfo ) != IDE_SUCCESS );

    ideLog::log(IDE_SD_17,"[SHARD INTERNAL SQL]: %s, %s",aNodeName, sSqlStr);


    if ( aIsNewTrans == ID_TRUE )
    {
        IDE_TEST( sdi::shardExecTempDDLWithNewTrans( aStatement,
                                                     aNodeName,
                                                     sSqlStr,
                                                     idlOS::strlen(sSqlStr))
                  != IDE_SUCCESS );

        // add item 실패 시 revert 처리
        if ( idlOS::strncmp( aPartitionName, SDM_NA_STR, QC_MAX_OBJECT_NAME_LEN + 1) != 0 )
        {
            idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                             "alter replication "QCM_SQL_STRING_SKIP_FMT" drop table from "QCM_SQL_STRING_SKIP_FMT"."QCM_SQL_STRING_SKIP_FMT" partition "QCM_SQL_STRING_SKIP_FMT" to "QCM_SQL_STRING_SKIP_FMT"."QCM_SQL_STRING_SKIP_FMT" partition "QCM_SQL_STRING_SKIP_FMT"",
                             aReplName,           /* REPLICATION_NAME  */
                             aUserName,           /* Src UserName      */
                             sTableNameStr,       /* Src TableName     */
                             aPartitionName,      /* Src PartitionName */
                             aUserName,           /* Dst UserName      */
                             sTargetTableNameStr, /* Dst TableName     */
                             aPartitionName       /* Dst PartitionName */ );
        }
        else
        {
            idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                             "alter replication "QCM_SQL_STRING_SKIP_FMT" drop table from "QCM_SQL_STRING_SKIP_FMT"."QCM_SQL_STRING_SKIP_FMT" to "QCM_SQL_STRING_SKIP_FMT"."QCM_SQL_STRING_SKIP_FMT" ",
                             aReplName,           /* REPLICATION_NAME */
                             aUserName,           /* Src UserName     */
                             sTableNameStr,       /* Src TableName    */
                             aUserName,           /* Dst UserName     */
                             sTargetTableNameStr  /* Dst TableName    */ );
        }

        if (( aStatement->session->mQPSpecific.mFlag & QC_SESSION_SHARD_DDL_MASK ) ==
            QC_SESSION_SHARD_DDL_TRUE )
        {
            (void)sdiZookeeper::addPendingJob( sSqlStr,
                                               aNodeName,
                                               ZK_PENDING_JOB_AFTER_ROLLBACK,
                                               QCI_STMT_MASK_DDL );
       
        }
        else
        {
            // shard package에서의 예외처리를 위해 
            (void)sdiZookeeper::addRevertJob( sSqlStr,
                                              aNodeName,
                                              ZK_REVERT_JOB_REPL_ITEM_DROP );
        }
    }
    else
    {
        if ( idlOS::strncmp( sLocalMetaInfo.mNodeName, 
                             aNodeName,
                             SDI_NODE_NAME_MAX_SIZE + 1 ) == 0 )
        {
            ideLog::log(IDE_SD_17,"[SHARD_META] Internal SQL: %s", sSqlStr);
            IDE_TEST( qciMisc::runRollbackableInternalDDL( aStatement,
                                                           QC_SMI_STMT( aStatement ),
                                                           sUserID,
                                                           sSqlStr )
                      != IDE_SUCCESS );
        
        }   
        else
        {
            IDE_TEST( sdi::checkShardLinker( aStatement ) != IDE_SUCCESS );

            if ( aStatement->session->mQPSpecific.mClientInfo != NULL )
            {
        
                IDE_TEST( sdi::shardExecDirect( aStatement,
                                                aNodeName,
                                                sSqlStr,
                                                idlOS::strlen(sSqlStr),
                                                SDI_INTERNAL_OP_NORMAL,
                                                &sExecCount,
                                                NULL,
                                                NULL,
                                                0,
                                                NULL )
                          != IDE_SUCCESS );
            }
        }
    }
        
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

// add Repl Table
IDE_RC sdm::dropReplTable( qcStatement * aStatement,
                           SChar       * aNodeName,
                           SChar       * aReplName,
                           SChar       * aUserName,
                           SChar       * aTableName,
                           SChar       * aPartitionName,
                           sdmReplDirectionalRole aRole,
                           idBool        aIsNewTrans )
{
    SChar      * sSqlStr;
    UInt         sUserID = QC_EMPTY_USER_ID;
    SChar        sTableNameStr[QC_MAX_OBJECT_NAME_LEN + 1];
    SChar        sTargetTableNameStr[QC_MAX_OBJECT_NAME_LEN + 1];
    SChar        sSqlRemoteStr[QD_MAX_SQL_LENGTH + 1];
    sdiLocalMetaInfo    sLocalMetaInfo;
    
    switch (aRole)
    {
        case SDM_REPL_SENDER:
            idlOS::strncpy( sTableNameStr, aTableName, QC_MAX_OBJECT_NAME_LEN + 1 );
            idlOS::snprintf( sTargetTableNameStr,
                             QC_MAX_OBJECT_NAME_LEN+1,
                             "%s%s",
                             SDI_BACKUP_TABLE_PREFIX,
                             aTableName );
            break;
        case SDM_REPL_RECEIVER:
            idlOS::strncpy( sTargetTableNameStr, aTableName, QC_MAX_OBJECT_NAME_LEN + 1 );
            idlOS::snprintf( sTableNameStr,
                             QC_MAX_OBJECT_NAME_LEN+1,
                             "%s%s",
                             SDI_BACKUP_TABLE_PREFIX,
                             aTableName );
            break;
        case SDM_REPL_CLONE:
            /* CLONE Table은 Failover/Failback 시에만 OriTable to OriTable로 맺어져 있다. */
            idlOS::strncpy( sTableNameStr, aTableName, QC_MAX_OBJECT_NAME_LEN + 1 );
            idlOS::strncpy( sTargetTableNameStr, aTableName, QC_MAX_OBJECT_NAME_LEN + 1 );
            break;

        default:
            IDE_DASSERT(0);
            break;
    }

    IDE_TEST( STRUCT_ALLOC_WITH_SIZE( aStatement->qmxMem,
                                      SChar,
                                      QD_MAX_SQL_LENGTH,
                                      &sSqlStr )
              != IDE_SUCCESS);

    if ( idlOS::strncmp( aPartitionName, SDM_NA_STR, QC_MAX_OBJECT_NAME_LEN + 1 ) != 0 )
    {
        idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                         "alter replication "QCM_SQL_STRING_SKIP_FMT" drop table from "QCM_SQL_STRING_SKIP_FMT"."QCM_SQL_STRING_SKIP_FMT" partition "QCM_SQL_STRING_SKIP_FMT" to "QCM_SQL_STRING_SKIP_FMT"."QCM_SQL_STRING_SKIP_FMT" partition "QCM_SQL_STRING_SKIP_FMT"",
                         aReplName,           /* REPLICATION_NAME  */
                         aUserName,           /* Src UserName      */
                         sTableNameStr,       /* Src TableName     */
                         aPartitionName,      /* Src PartitionName */
                         aUserName,           /* Dst UserName      */
                         sTargetTableNameStr, /* Dst TableName     */
                         aPartitionName       /* Dst PartitionName */ );

    }
    else
    {
        idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                         "alter replication "QCM_SQL_STRING_SKIP_FMT" drop table from "QCM_SQL_STRING_SKIP_FMT"."QCM_SQL_STRING_SKIP_FMT" to "QCM_SQL_STRING_SKIP_FMT"."QCM_SQL_STRING_SKIP_FMT" ",
                         aReplName,           /* REPLICATION_NAME */
                         aUserName,           /* Src UserName     */
                         sTableNameStr,       /* Src TableName    */
                         aUserName,           /* Dst UserName     */
                         sTargetTableNameStr  /* Dst TableName    */ );
    }
    
    if ( aIsNewTrans == ID_TRUE )
    {
        IDE_TEST( sdi::getLocalMetaInfo( &sLocalMetaInfo ) != IDE_SUCCESS );
        
        if ( idlOS::strncmp( sLocalMetaInfo.mNodeName, 
                             aNodeName,
                             SDI_NODE_NAME_MAX_SIZE + 1 ) == 0 )
        {
            ideLog::log(IDE_SD_17,"[SHARD_META] Internal SQL: %s", sSqlStr);
            IDE_TEST( sdi::shardExecTempDMLOrDCLWithNewTrans( aStatement,
                                                              sSqlStr )
                      != IDE_SUCCESS );
        
        }   
        else
        {
            idlOS::snprintf( sSqlRemoteStr, QD_MAX_SQL_LENGTH,
                             "EXEC DBMS_SHARD.EXECUTE_IMMEDIATE( '%s', '%s' ) ",
                             sSqlStr,
                             aNodeName );
            
            IDE_TEST( sdi::shardExecTempDMLOrDCLWithNewTrans( aStatement,
                                                              sSqlRemoteStr )
                      != IDE_SUCCESS );
        }
    }
    else
    {
        IDE_TEST( qciMisc::getUserIdByName( aUserName, &sUserID ) != IDE_SUCCESS );
        ideLog::log(IDE_SD_17,"[SHARD_META] Internal SQL: %s", sSqlStr);
        IDE_TEST( qciMisc::runRollbackableInternalDDL( aStatement,
                                                       QC_SMI_STMT( aStatement ),
                                                       sUserID,
                                                       sSqlStr )
                  != IDE_SUCCESS );
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC sdm::getNthBackupNodeName( smiStatement * aSmiStmt,
                                  SChar        * aNodeName,
                                  SChar        * aBakNodeName,
                                  UInt           aNth,
                                  ULong          aSMN )
{
    const void          * sRow                   = NULL;
    scGRID                sRid;
    const void          * sSdmNodes              = NULL;
    const void          * sSdmNodesIndex[SDM_MAX_META_INDICES];

    idBool                sIsCursorOpen          = ID_FALSE;

    mtcColumn           * sNodeNameColumn = NULL;

    smiRange              sRange;
    mtdCharType         * sName;
    mtcColumn           * sSMNColumn;
    qtcMetaRangeColumn    sSMNRange;

    smiTableCursor        sCursor;
    smiCursorProperties   sCursorProperty;

    SChar                 sFstNodeName[SDI_NODE_NAME_MAX_SIZE + 1];
    SChar                 sSecNodeName[SDI_NODE_NAME_MAX_SIZE + 1];
    SChar                 sCurNodeName[SDI_NODE_NAME_MAX_SIZE + 1];
    UInt                  sCnt = 0;
    UInt                  sNth = aNth + 1;
    idBool                sIsFound = ID_FALSE;
    idBool                sIsDone  = ID_FALSE;

    IDE_TEST( getMetaTableAndIndex( aSmiStmt,
                                    SDM_NODES,
                                    &sSdmNodes,
                                    sSdmNodesIndex )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( sSdmNodesIndex[SDM_NODES_IDX8_ORDER] == NULL,
                    ERR_META_HANDLE );


    IDE_TEST( smiGetTableColumns( sSdmNodes,
                                  SDM_NODES_NODE_NAME_COL_ORDER,
                                  (const smiColumn **)&sNodeNameColumn )
              != IDE_SUCCESS );

    IDE_TEST( smiGetTableColumns( sSdmNodes,
                                  SDM_NODES_SMN_COL_ORDER,
                                  (const smiColumn **)&sSMNColumn )
              != IDE_SUCCESS );

    // mtdModule 설정
    IDE_TEST( mtd::moduleById( &(sNodeNameColumn->module),
                               sNodeNameColumn->type.dataTypeId )
              != IDE_SUCCESS );

    // mtlModule 설정
    IDE_TEST( mtl::moduleById( &sNodeNameColumn->language,
                               sNodeNameColumn->type.languageId )
              != IDE_SUCCESS );

    // mtdModule 설정
    IDE_TEST( mtd::moduleById( &(sSMNColumn->module),
                               sSMNColumn->type.dataTypeId )
              != IDE_SUCCESS );

    // mtlModule 설정
    IDE_TEST( mtl::moduleById( &sSMNColumn->language,
                               sSMNColumn->type.languageId )
              != IDE_SUCCESS );

    qciMisc::makeMetaRangeSingleColumn(
        &sSMNRange,
        (const mtcColumn*)sSMNColumn,
        (const void *) &aSMN,
        &sRange );

    sCursor.initialize();

    SMI_CURSOR_PROP_INIT_FOR_META_INDEX_SCAN( &sCursorProperty, NULL );


    IDE_TEST( sCursor.open( aSmiStmt,
                            sSdmNodes,
                            sSdmNodesIndex[SDM_NODES_IDX8_ORDER],
                            smiGetRowSCN( sSdmNodes ),
                            NULL,
                            &sRange,
                            smiGetDefaultKeyRange(),
                            smiGetDefaultFilter(),
                            QCM_META_CURSOR_FLAG,
                            SMI_SELECT_CURSOR,
                            &sCursorProperty )
              != IDE_SUCCESS );
    sIsCursorOpen = ID_TRUE;

    IDE_TEST( sCursor.beforeFirst() != IDE_SUCCESS );

    IDE_TEST( sCursor.readRow( &sRow, &sRid, SMI_FIND_NEXT )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( sRow == NULL, ERR_NOT_EXIST_NODE );

    while( sRow != NULL )
    {
        sName = (mtdCharType*)((SChar *)sRow + sNodeNameColumn->column.offset );

        idlOS::memcpy( sCurNodeName,
                       sName->value,
                       sName->length );
        sCurNodeName[sName->length] = '\0';
        sCnt++;

        if ( sCnt == 1 )
        {
            idlOS::memcpy( sFstNodeName,
                           sName->value,
                           sName->length );
            sFstNodeName[sName->length] = '\0';
        }
        else if ( sCnt == 2 )
        {
            idlOS::memcpy( sSecNodeName,
                           sName->value,
                           sName->length );
            sSecNodeName[sName->length] = '\0';
        }

        if ( idlOS::strncmp( sCurNodeName, aNodeName, SDI_NODE_NAME_MAX_SIZE + 1 ) == 0 )
        {
            sIsFound = ID_TRUE;
        }

        if ( sIsFound == ID_TRUE )
        {
            if ( sNth == 0 )
            {
                idlOS::memcpy( aBakNodeName,
                               sName->value,
                               sName->length );
                aBakNodeName[sName->length] = '\0';
                sIsDone = ID_TRUE;
                break;
            }
            else
            {
                sNth--;
            }
        }

        IDE_TEST( sCursor.readRow( &sRow, &sRid, SMI_FIND_NEXT )
                  != IDE_SUCCESS );
    }

    if ( sIsDone == ID_FALSE )
    {
        /* 내 Node의 위치를 찾았으나 List의 끝에 도달 했다. */
        if ( sIsFound == ID_TRUE )
        {
            if ( sNth == 0 )
            {
                /* FstNode가 원하던 Node 이다.
                 * 다만 내 Node가 아니여야함. */
                IDE_TEST_RAISE( sCnt < 2, ERR_NOT_EXIST_NODE );
                idlOS::memcpy( aBakNodeName,
                               sFstNodeName,
                               SDI_NODE_NAME_MAX_SIZE+1 );
            }
            else if ( sNth == 1 )
            {
                /* SecNode가 원하던 Node 이다.
                 * sNth 가 1이려면 처음 요청값이 2 이어야만 하기 때문에
                 * 최소 Node 갯수는 3 이어야 한다. */
                IDE_TEST_RAISE( sCnt < 3, ERR_NOT_EXIST_NODE );

                idlOS::memcpy( aBakNodeName,
                               sSecNodeName,
                               SDI_NODE_NAME_MAX_SIZE+1 );
            }
        }
        else
        {
            IDE_RAISE( ERR_NOT_EXIST_NODE );
        }
    }

    sIsCursorOpen = ID_FALSE;
    IDE_TEST( sCursor.close() != IDE_SUCCESS );


    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_META_HANDLE )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDM_SHARD_META_NOT_CREATED,
                                  SDM_NODES ) );
    }
    IDE_EXCEPTION( ERR_NOT_EXIST_NODE )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDM_SHARD_NODE_NOT_EXIST ) );
    }
    IDE_EXCEPTION_END;

    IDE_PUSH();

    if ( sIsCursorOpen == ID_TRUE )
    {
        (void)sCursor.close();
    }
    else
    {
        /* Nothing to do */
    }

    IDE_POP();

    return IDE_FAILURE;
}


/*
 * ReplicaSet 정보를 이용하여 내 Node가 Send하는 ReplName과 대상 NodeName을 얻어 온다.
 *
 * [IN]  aReplicaSetInfo    전체 ReplicaSet 정보
 * [IN]  aNodeName          내 Node의 이름
 * [OUT] aOutReplicaSetInfo 내 Node에서 Send하는 ReplicaSet의 모음
 */
IDE_RC sdm::findSendReplInfoFromReplicaSet( sdiReplicaSetInfo   * aReplicaSetInfo,
                                            SChar               * aNodeName,
                                            sdiReplicaSetInfo   * aOutReplicaSetInfo )
{
    UInt sCnt     = 0;
    UInt sFindCnt = 0;

    IDE_TEST_RAISE( aReplicaSetInfo->mCount >= SDI_NODE_MAX_COUNT, INVALID_RPSET_COUNT ); 

    for ( sCnt = 0; sCnt < aReplicaSetInfo->mCount; sCnt++ )
    {
        if ( idlOS::strncmp( aReplicaSetInfo->mReplicaSets[sCnt].mPrimaryNodeName, 
                             aNodeName,
                             SDI_NODE_NAME_MAX_SIZE + 1 ) == 0 )
        {
            idlOS::memcpy( &aOutReplicaSetInfo->mReplicaSets[sFindCnt],
                           &aReplicaSetInfo->mReplicaSets[sCnt],
                           ID_SIZEOF( sdiReplicaSet ) );
            sFindCnt++;
        }
    }

    aOutReplicaSetInfo->mCount = sFindCnt;

    return IDE_SUCCESS;

    IDE_EXCEPTION( INVALID_RPSET_COUNT )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                  "sdm::findSendReplInfoFromReplicaSet",
                                  "Invalid ReplicaSet Count" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 * ReplicaSet 정보를 이용하여 내 Node가 Recv중인 ReplName과 NodeName을 얻어 온다.
 *
 * [IN]  aReplicaSetInfo    전체 ReplicaSet 정보
 * [IN]  aNodeName          내 Node의 이름
 * [IN]  aNth               이번에 찾으려는 값이 First( 0 )인지 Second( 1 )  인지 여부
 * [OUT] aOutReplicaSetInfo 내 Node에서 Recv중인 ReplicaSet의 모음
 */
IDE_RC sdm::findRecvReplInfoFromReplicaSet( sdiReplicaSetInfo   * aReplicaSetInfo,
                                            SChar               * aNodeName,
                                            UInt                  aNth,
                                            sdiReplicaSetInfo   * aOutReplicaSetInfo )
{
    UInt sCnt     = 0;
    UInt sFindCnt = 0;

    /* aNth 는 0,1 만 가능 */
    IDE_TEST_RAISE( aNth > 1 , ERR_ARG );
    IDE_TEST_RAISE( aReplicaSetInfo->mCount >= SDI_NODE_MAX_COUNT, INVALID_RPSET_COUNT ); 

    for ( sCnt = 0; sCnt < aReplicaSetInfo->mCount; sCnt++ )
    {
        if ( aNth == 0 ) /* First */
        {
            if ( idlOS::strncmp( aReplicaSetInfo->mReplicaSets[sCnt].mFirstBackupNodeName,
                                 aNodeName,
                                 SDI_NODE_NAME_MAX_SIZE + 1 ) == 0 )
            {
                idlOS::memcpy( &aOutReplicaSetInfo->mReplicaSets[sFindCnt],
                               &aReplicaSetInfo->mReplicaSets[sCnt],
                               ID_SIZEOF( sdiReplicaSet ) );

                sFindCnt++;
            }
        }
        else if ( aNth == 1 ) /* Second */
        {
            if ( idlOS::strncmp( aReplicaSetInfo->mReplicaSets[sCnt].mSecondBackupNodeName,
                                 aNodeName,
                                 SDI_NODE_NAME_MAX_SIZE + 1 ) == 0 )
            {
                idlOS::memcpy( &aOutReplicaSetInfo->mReplicaSets[sFindCnt],
                               &aReplicaSetInfo->mReplicaSets[sCnt],
                               ID_SIZEOF( sdiReplicaSet ) );

                sFindCnt++;
            }
        }
    }

    aOutReplicaSetInfo->mCount = sFindCnt;

    return IDE_SUCCESS;

    IDE_EXCEPTION( INVALID_RPSET_COUNT )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                  "sdm::findRecvReplInfoFromReplicaSet",
                                  "Invalid ReplicaSet Count" ) );
    }
    IDE_EXCEPTION( ERR_ARG )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                  "sdm::findRecvReplInfoFromReplicaSet",
                                  "aNth is invalid" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdm::dropReplicationItemsPerRPSet( qcStatement * aStatement,
                                          SChar       * aReplName,
                                          SChar       * aNodeName,
                                          SChar       * aUserName,
                                          SChar       * aTableName,
                                          sdmReplDirectionalRole aRole,
                                          ULong         aSMN,
                                          UInt          aReplicaSetId )
{
    qcStatement             * sStatement = aStatement;
    smiStatement            * sOldStmt;
    smiStatement              sSmiStmt;
    UInt                      sSmiStmtFlag;
    SInt                      sState = 0;

    idBool                    sIsFound      = ID_FALSE;
    idBool                    sIsFoundTable = ID_FALSE;
    idBool                    sExistKey     = ID_FALSE;
    idBool                    sExistSubKey  = ID_FALSE;
    UInt                      sKeyType;
    UInt                      sSubKeyType;
    UInt                      sUserID;
    smSCN                     sTableSCN;
    void                    * sTableHandle = NULL;
    qcmTableInfo            * sQcmTableInfo = NULL;
    UInt                      i = 0;

    sdiTableInfo              sTableInfo;
    sdiRangeInfo              sRangeInfo;
    sdiNode                   sNodeInfo;

    SChar                     sPartitionName[ QC_MAX_OBJECT_NAME_LEN + 1];

    idlOS::strncpy( sPartitionName, SDM_NA_STR, QC_MAX_OBJECT_NAME_LEN + 1 );

    sSmiStmtFlag = SMI_STATEMENT_NORMAL | SMI_STATEMENT_MEMORY_CURSOR;
    sOldStmt                = QC_SMI_STMT(sStatement);
    QC_SMI_STMT(sStatement) = &sSmiStmt;
    sState = 1;

    IDE_TEST( sSmiStmt.begin( sStatement->mStatistics,
                              sOldStmt,
                              sSmiStmtFlag )
              != IDE_SUCCESS );
    sState = 2;

    IDE_TEST( qciMisc::getUserID( NULL,
                                  QC_SMI_STMT( sStatement ),
                                  aUserName,
                                  idlOS::strlen( aUserName ),
                                  &sUserID )
              != IDE_SUCCESS );

    IDE_TEST( qciMisc::getTableInfo( sStatement,
                                     sUserID,
                                     (UChar*)aTableName,
                                     idlOS::strlen( aTableName ),
                                     &sQcmTableInfo,
                                     &sTableSCN,
                                     &sTableHandle )
              != IDE_SUCCESS );

    IDE_TEST( sdm::getTableInfo( QC_SMI_STMT( sStatement ),
                                 aUserName,
                                 aTableName,
                                 aSMN, //sMetaNodeInfo.mShardMetaNumber,
                                 &sTableInfo,
                                 &sIsFoundTable )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( sIsFoundTable == ID_FALSE,
                    ERR_NOT_EXIST_TABLE );

    for ( i = 0; i < sQcmTableInfo->columnCount; i++ )
    {
        if ( idlOS::strMatch( sQcmTableInfo->columns[i].name,
                              idlOS::strlen(sQcmTableInfo->columns[i].name),
                              sTableInfo.mKeyColumnName,
                              idlOS::strlen(sTableInfo.mKeyColumnName) ) == 0 )
        {
            sKeyType = sQcmTableInfo->columns[i].basicInfo->module->id;
            IDE_TEST_RAISE( sdm::isSupportDataType( sKeyType ) == ID_FALSE,
                            ERR_INVALID_SHARD_KEY_TYPE );

            sTableInfo.mKeyDataType = sKeyType;
            sTableInfo.mKeyColOrder = (UShort)i;
            sExistKey = ID_TRUE;

            if ( ( sTableInfo.mSubKeyExists == ID_FALSE ) ||
                 ( sExistSubKey == ID_TRUE ) )
            {
                break;
            }
        }

        /* PROJ-2655 Composite shard key */
        if ( sTableInfo.mSubKeyExists == ID_TRUE )
        { 
            if ( idlOS::strMatch( sQcmTableInfo->columns[i].name,
                                  idlOS::strlen(sQcmTableInfo->columns[i].name),
                                  sTableInfo.mSubKeyColumnName,
                                  idlOS::strlen(sTableInfo.mSubKeyColumnName) ) == 0 )
            {   
                sExistSubKey = ID_TRUE;

                sSubKeyType = sQcmTableInfo->columns[i].basicInfo->module->id;
                IDE_TEST_RAISE( sdm::isSupportDataType( sSubKeyType ) == ID_FALSE,
                                ERR_INVALID_SHARD_KEY_TYPE );

                sTableInfo.mSubKeyDataType = sSubKeyType;
                sTableInfo.mSubKeyColOrder = (UShort)i;

                if ( sExistKey == ID_TRUE )
                {
                    break;
                }
            }
        }
    }

    IDE_TEST( sdm::getRangeInfo ( sStatement,
                                  QC_SMI_STMT( sStatement ),
                                  aSMN,
                                  &sTableInfo,
                                  &sRangeInfo,
                                  ID_FALSE ) 
              != IDE_SUCCESS );

    IDE_TEST( sdm::getNodeByName( QC_SMI_STMT( sStatement ),
                                  aNodeName,
                                  aSMN,
                                  &sNodeInfo )
              != IDE_SUCCESS );

    sState = 1;
    IDE_TEST( sSmiStmt.end( SMI_STATEMENT_RESULT_SUCCESS ) != IDE_SUCCESS );

    sState = 0;
    QC_SMI_STMT(sStatement) = sOldStmt;

    /* PartitionName의 NULL 값은 $$N/A 이다 */
    idlOS::strncpy( sPartitionName ,
                    SDM_NA_STR,
                    QC_MAX_OBJECT_NAME_LEN + 1 );

    for ( i = 0; i < sRangeInfo.mCount ; i++ )
    {
        if ( ( sRangeInfo.mRanges[i].mNodeId == sNodeInfo.mNodeId ) &&
             ( sRangeInfo.mRanges[i].mReplicaSetId == aReplicaSetId ) )
        {
            /* 제거해야 하는 Partition을 찾았다. */
            switch ( sdi::getShardObjectType( &sTableInfo ) )
            {
                case SDI_SINGLE_SHARD_KEY_DIST_OBJECT:
                    /* KEY_DIST Table은 Partition 단위 이중화이다. */
                    idlOS::strncpy ( sPartitionName,
                                     sRangeInfo.mRanges[i].mPartitionName,
                                     QC_MAX_OBJECT_NAME_LEN + 1 );
                    sIsFound = ID_TRUE;
                    break;
                case SDI_COMPOSITE_SHARD_KEY_DIST_OBJECT:
                case SDI_SOLO_DIST_OBJECT:
                    /* COMPOSITE Table 과 SOLO Table은 Table 단위 이중화이다. */
                    idlOS::strncpy( sPartitionName ,
                                    SDM_NA_STR,
                                    QC_MAX_OBJECT_NAME_LEN + 1 );
                    sIsFound = ID_TRUE;
                    break;
                case SDI_CLONE_DIST_OBJECT:
                    /* Clone table은 이중화를 생성하지 않는다. */
                    break;
                case SDI_NON_SHARD_OBJECT:
                default:
                    /* Shard 객체 값이 잘못되었다. */
                    IDE_RAISE( INVALID_SHARD_TYPE );
                    break;
            }

            if ( sIsFound == ID_TRUE )
            {
                IDE_TEST( dropReplTable( aStatement,
                                         aNodeName,
                                         aReplName,
                                         aUserName,
                                         aTableName,
                                         sPartitionName,
                                         aRole,
                                         ID_FALSE )
                          != IDE_SUCCESS );

                sIsFound = ID_FALSE;
            }
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( INVALID_SHARD_TYPE )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                  "sdm::dropReplicationItemsPerRPSet",
                                  "Invalid Shard Object Type Detected" ) );
    }
    IDE_EXCEPTION( ERR_INVALID_SHARD_KEY_TYPE )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDM_UNSUPPORTED_SHARD_KEY_COLUMN_TYPE,
                                  sQcmTableInfo->tableOwnerName,
                                  sQcmTableInfo->name,
                                  ( sExistSubKey == ID_TRUE )? sTableInfo.mSubKeyColumnName:
                                                               sTableInfo.mKeyColumnName ) );
    }
    IDE_EXCEPTION( ERR_NOT_EXIST_TABLE )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDM_SHARD_TABLE_NOT_EXIST ) );
    }

    IDE_EXCEPTION_END;

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
            QC_SMI_STMT(sStatement) = sOldStmt;
        default:
            break;
    }
    
    return IDE_FAILURE;
}

IDE_RC sdm::addReplicationItemUsingRPSets( qcStatement       * aStatement,
                                           sdiReplicaSetInfo * aReplicaSetInfo,
                                           SChar             * aNodeName,
                                           SChar             * aUserName,
                                           SChar             * aTableName,
                                           SChar             * aPartitionName,
                                           UInt                aNth,
                                           sdmReplDirectionalRole aRole )
{
    sdiLocalMetaInfo sLocalMetaInfo;
    UInt             sCnt = 0;

    IDE_TEST( sdi::getLocalMetaInfo( &sLocalMetaInfo ) != IDE_SUCCESS );
    
    /* 0,1만 들어올수 있는 상태 */
    IDE_TEST_RAISE( aNth > 1, ERR_ARG );

    for ( sCnt = 0; sCnt < aReplicaSetInfo->mCount; sCnt++ )
    {
        switch (aRole)
        {
            case SDM_REPL_SENDER:
                if ( aNth == 0 ) /* First */
                {
                    /* BackupNodeName이 설정되어 있어야만 한다. */
                    if ( idlOS::strncmp( aReplicaSetInfo->mReplicaSets[sCnt].mFirstBackupNodeName,
                                         SDM_NA_STR,
                                         SDI_NODE_NAME_MAX_SIZE + 1)
                         != 0 )
                    {
                        IDE_TEST( sdm::addReplTable( aStatement,
                                                     sLocalMetaInfo.mNodeName,
                                                     aReplicaSetInfo->mReplicaSets[sCnt].mFirstReplName,
                                                     aUserName,
                                                     aTableName,
                                                     aPartitionName,
                                                     aRole,
                                                     ID_TRUE )
                                  != IDE_SUCCESS );
                    }
                }
                else if ( aNth == 1 ) /* Second */
                {
                    /* BackupNodeName이 설정되어 있어야만 한다. */
                    if ( idlOS::strncmp( aReplicaSetInfo->mReplicaSets[sCnt].mSecondBackupNodeName,
                                         SDM_NA_STR,
                                         SDI_NODE_NAME_MAX_SIZE + 1)
                         != 0 )
                    {
                        IDE_TEST( sdm::addReplTable( aStatement,
                                                     sLocalMetaInfo.mNodeName,
                                                     aReplicaSetInfo->mReplicaSets[sCnt].mSecondReplName,
                                                     aUserName,
                                                     aTableName,
                                                     aPartitionName,
                                                     aRole,
                                                     ID_TRUE )
                                  != IDE_SUCCESS );
                    }
                }
                break;
            case SDM_REPL_RECEIVER:
                if ( aNth == 0 ) /* First */
                {
                    /* 지정된 Node 정보와 일치 해야 한다. */
                    if ( idlOS::strncmp( aReplicaSetInfo->mReplicaSets[sCnt].mPrimaryNodeName,
                                         aNodeName,
                                         SDI_NODE_NAME_MAX_SIZE + 1 )
                         == 0 )
                    {
                        IDE_TEST( sdm::addReplTable( aStatement,
                                                     sLocalMetaInfo.mNodeName,
                                                     aReplicaSetInfo->mReplicaSets[sCnt].mFirstReplName,
                                                     aUserName,
                                                     aTableName,
                                                     aPartitionName,
                                                     aRole,
                                                     ID_TRUE )
                                  != IDE_SUCCESS );
                    }
                }
                else if ( aNth == 1 ) /* Second */
                {
                    /* 지정된 Node 정보와 일치 해야 한다. */
                    if ( idlOS::strncmp( aReplicaSetInfo->mReplicaSets[sCnt].mPrimaryNodeName,
                                         aNodeName,
                                         SDI_NODE_NAME_MAX_SIZE + 1 )
                         == 0 )
                    {
                        IDE_TEST( sdm::addReplTable( aStatement,
                                                     sLocalMetaInfo.mNodeName,
                                                     aReplicaSetInfo->mReplicaSets[sCnt].mSecondReplName,
                                                     aUserName,
                                                     aTableName,
                                                     aPartitionName,
                                                     aRole,
                                                     ID_TRUE )
                                  != IDE_SUCCESS );
                    }
                }
                break;
            default:
                IDE_DASSERT(0);
                break;
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_ARG )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                  "sdm::addReplicationItemUsingRPSets",
                                  "aNth is invalid" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdm::flushReplicationItemUsingRPSets( qcStatement       * aStatement,
                                             sdiReplicaSetInfo * aReplicaSetInfo,
                                             SChar             * aUserName,
                                             SChar             * aNodeName,
                                             UInt                aNth,
                                             sdmReplDirectionalRole aRole )
{
    UInt             sCnt = 0;
    
    /* 0,1만 들어올수 있는 상태 */
    IDE_TEST_RAISE( aNth > 1, ERR_ARG );

    for ( sCnt = 0; sCnt < aReplicaSetInfo->mCount; sCnt++ )
    {
        switch (aRole)
        {
            case SDM_REPL_SENDER:
                if ( aNth == 0 ) /* First */
                {
                    /* BackupNodeName이 설정되어 있어야만 한다. */
                    if ( idlOS::strncmp( aReplicaSetInfo->mReplicaSets[sCnt].mFirstBackupNodeName,
                                         SDM_NA_STR,
                                         SDI_NODE_NAME_MAX_SIZE + 1)
                         != 0 )
                    {                        
                        IDE_TEST( sdm::flushReplTable( aStatement,
                                                       aNodeName,
                                                       aUserName,
                                                       aReplicaSetInfo->mReplicaSets[sCnt].mFirstReplName,
                                                       ID_TRUE )
                                  != IDE_SUCCESS );

                    }
                }
                else if ( aNth == 1 ) /* Second */
                {
                    /* BackupNodeName이 설정되어 있어야만 한다. */
                    if ( idlOS::strncmp( aReplicaSetInfo->mReplicaSets[sCnt].mSecondBackupNodeName,
                                         SDM_NA_STR,
                                         SDI_NODE_NAME_MAX_SIZE + 1)
                         != 0 )
                    {
                        IDE_TEST( sdm::flushReplTable( aStatement,
                                                       aNodeName,
                                                       aUserName,
                                                       aReplicaSetInfo->mReplicaSets[sCnt].mSecondReplName,
                                                       ID_TRUE )
                                  != IDE_SUCCESS );
                    }
                }
                break;
            case SDM_REPL_RECEIVER:
                // nothing to do
                break;
            default:
                IDE_DASSERT(0);
                break;
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_ARG )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                  "sdm::flushReplicationItemUsingRPSets",
                                  "aNth is invalid" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdm::dropReplicationItemUsingRPSets( qcStatement       * aStatement,
                                            sdiReplicaSetInfo * aReplicaSetInfo,
                                            SChar             * aUserName,
                                            SChar             * aTableName,
                                            UInt                aNth,
                                            sdmReplDirectionalRole aRole,
                                            UInt                aSMN )
{
    UInt sCnt = 0;

    /* 0,1만 들어올수 있는 상태 */
    IDE_TEST_RAISE( aNth > 1, ERR_ARG );

    for ( sCnt = 0; sCnt < aReplicaSetInfo->mCount; sCnt++ )
    {
        switch (aRole)
        {
            case SDM_REPL_SENDER:
                if ( aNth == 0 ) /* First */
                {
                    if ( idlOS::strncmp( aReplicaSetInfo->mReplicaSets[sCnt].mFirstBackupNodeName,
                                         SDM_NA_STR,
                                         SDI_NODE_NAME_MAX_SIZE + 1 ) != 0 )
                    {
                        IDE_TEST( sdm::dropReplicationItemsPerRPSet( aStatement,
                                                                     aReplicaSetInfo->mReplicaSets[sCnt].mFirstReplName,
                                                                     aReplicaSetInfo->mReplicaSets[sCnt].mPrimaryNodeName,
                                                                     aUserName,
                                                                     aTableName,
                                                                     aRole,
                                                                     aSMN,
                                                                     aReplicaSetInfo->mReplicaSets[sCnt].mReplicaSetId )
                                  != IDE_SUCCESS );
                    }
                }
                else if ( aNth == 1 ) /* Second */
                {
                    if ( idlOS::strncmp( aReplicaSetInfo->mReplicaSets[sCnt].mSecondBackupNodeName,
                                         SDM_NA_STR,
                                         SDI_NODE_NAME_MAX_SIZE + 1) != 0 )
                    {
                        IDE_TEST( sdm::dropReplicationItemsPerRPSet( aStatement,
                                                                     aReplicaSetInfo->mReplicaSets[sCnt].mSecondReplName,
                                                                     aReplicaSetInfo->mReplicaSets[sCnt].mPrimaryNodeName,
                                                                     aUserName,
                                                                     aTableName,
                                                                     aRole,
                                                                     aSMN,
                                                                     aReplicaSetInfo->mReplicaSets[sCnt].mReplicaSetId )
                                  != IDE_SUCCESS );
                    }
                }
                break;
            case SDM_REPL_RECEIVER:
                if ( aNth == 0 ) /* First */
                {
                    if ( idlOS::strncmp( aReplicaSetInfo->mReplicaSets[sCnt].mPrimaryNodeName,
                         SDM_NA_STR,
                         SDI_NODE_NAME_MAX_SIZE + 1 ) != 0 )
                    {
                        IDE_TEST( sdm::dropReplicationItemsPerRPSet( aStatement,
                                                                     aReplicaSetInfo->mReplicaSets[sCnt].mFirstReplName,
                                                                     aReplicaSetInfo->mReplicaSets[sCnt].mPrimaryNodeName,
                                                                     aUserName,
                                                                     aTableName,
                                                                     aRole,
                                                                     aSMN,
                                                                     aReplicaSetInfo->mReplicaSets[sCnt].mReplicaSetId )
                                  != IDE_SUCCESS );
                    }
                }
                else if ( aNth == 1 ) /* Second */
                {
                    if ( idlOS::strncmp( aReplicaSetInfo->mReplicaSets[sCnt].mPrimaryNodeName,
                         SDM_NA_STR,
                         SDI_NODE_NAME_MAX_SIZE + 1 ) != 0 )
                    {
                        IDE_TEST( sdm::dropReplicationItemsPerRPSet( aStatement,
                                                                     aReplicaSetInfo->mReplicaSets[sCnt].mSecondReplName,
                                                                     aReplicaSetInfo->mReplicaSets[sCnt].mPrimaryNodeName,
                                                                     aUserName,
                                                                     aTableName,
                                                                     aRole,
                                                                     aSMN,
                                                                     aReplicaSetInfo->mReplicaSets[sCnt].mReplicaSetId )
                                  != IDE_SUCCESS );
                    }
                }
                break;
            default:
                IDE_DASSERT(0);
                break;
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_ARG )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                  "sdm::dropReplicationItemUsingRPSets",
                                  "aNth is invalid" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC sdm::getTableInfoAllObject( qcStatement        * aStatement,
                                   sdiTableInfoList  ** aTableInfoList,
                                   ULong                aSMN )
{
    idBool            sIsCursorOpen = ID_FALSE;
    const void      * sRow          = NULL;
    scGRID            sRid;
    const void      * sSdmObjects;
    const void      * sSdmObjectsIndex[SDM_MAX_META_INDICES];
    mtcColumn       * sShardIDColumn;
    mtcColumn       * sUserNameColumn;
    mtcColumn       * sObjectNameColumn;
    mtcColumn       * sObjectTypeColumn;
    mtcColumn       * sSplitMethodColumn;
    mtcColumn       * sKeyColumnNameColumn;
    mtcColumn       * sSubSplitMethodColumn;
    mtcColumn       * sSubKeyColumnNameColumn;
    mtcColumn       * sDefaultNodeIDColumn;
    mtcColumn       * sDefaultPartitionNameColumn;
    mtcColumn       * sDefaultReplicaSetIdColumn;
    mtcColumn       * sSMNColumn;

    mtdCharType     * sUserName;
    mtdCharType     * sObjectName;
    mtdCharType     * sObjectType;
    mtdCharType     * sSplitMethod;
    mtdCharType     * sKeyColumnName;
    mtdCharType     * sSubSplitMethod;
    mtdCharType     * sSubKeyColumnName;
    mtdIntegerType    sDefaultNodeID;
    mtdCharType     * sDefaultPartitionName;
    mtdIntegerType    sDefaultReplicaSetId;

    qtcMetaRangeColumn sSMNRange;
    smiRange           sRange;

    smiTableCursor       sCursor;
    smiCursorProperties  sCursorProperty;

    sdiTableInfo         sTableInfo;
    sdiTableInfoList   * sTableInfoList     = NULL;
    sdiTableInfoList   * sIterTableInfoList = NULL;
    
    SDI_INIT_TABLE_INFO( &sTableInfo );

    IDE_TEST( sdm::getMetaTableAndIndex( QC_SMI_STMT( aStatement ),
                                         SDM_OBJECTS,
                                         & sSdmObjects,
                                         sSdmObjectsIndex )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( sSdmObjectsIndex[SDM_OBJECTS_IDX2_ORDER] == NULL,
                    ERR_META_HANDLE );

    IDE_TEST( smiGetTableColumns( sSdmObjects,
                                  SDM_OBJECTS_SHARD_ID_COL_ORDER,
                                  (const smiColumn**)&sShardIDColumn )
              != IDE_SUCCESS );
    IDE_TEST( smiGetTableColumns( sSdmObjects,
                                  SDM_OBJECTS_USER_NAME_COL_ORDER,
                                  (const smiColumn**)&sUserNameColumn )
              != IDE_SUCCESS );
    IDE_TEST( smiGetTableColumns( sSdmObjects,
                                  SDM_OBJECTS_OBJECT_NAME_COL_ORDER,
                                  (const smiColumn**)&sObjectNameColumn )
              != IDE_SUCCESS );
    IDE_TEST( smiGetTableColumns( sSdmObjects,
                                  SDM_OBJECTS_OBJECT_TYPE_COL_ORDER,
                                  (const smiColumn**)&sObjectTypeColumn )
              != IDE_SUCCESS );
    IDE_TEST( smiGetTableColumns( sSdmObjects,
                                  SDM_OBJECTS_SPLIT_METHOD_COL_ORDER,
                                  (const smiColumn**)&sSplitMethodColumn )
              != IDE_SUCCESS );
    IDE_TEST( smiGetTableColumns( sSdmObjects,
                                  SDM_OBJECTS_KEY_COLUMN_NAME_COL_ORDER,
                                  (const smiColumn**)&sKeyColumnNameColumn )
              != IDE_SUCCESS );
    IDE_TEST( smiGetTableColumns( sSdmObjects,
                                  SDM_OBJECTS_SUB_SPLIT_METHOD_COL_ORDER,
                                  (const smiColumn**)&sSubSplitMethodColumn )
              != IDE_SUCCESS );
    IDE_TEST( smiGetTableColumns( sSdmObjects,
                                  SDM_OBJECTS_SUB_KEY_COLUMN_NAME_COL_ORDER,
                                  (const smiColumn**)&sSubKeyColumnNameColumn )
              != IDE_SUCCESS );
    IDE_TEST( smiGetTableColumns( sSdmObjects,
                                  SDM_OBJECTS_DEFAULT_NODE_ID_COL_ORDER,
                                  (const smiColumn**)&sDefaultNodeIDColumn )
              != IDE_SUCCESS );
    IDE_TEST( smiGetTableColumns( sSdmObjects,
                                  SDM_OBJECTS_DEFAULT_PARTITION_ID_COL_ORDER,
                                  (const smiColumn**)&sDefaultPartitionNameColumn )
              != IDE_SUCCESS );
    IDE_TEST( smiGetTableColumns( sSdmObjects,
                                  SDM_OBJECTS_DEFAULT_PARTITION_REPLICA_SET_ID_COL_ORDER,
                                  (const smiColumn**)&sDefaultReplicaSetIdColumn )
              != IDE_SUCCESS );
    IDE_TEST( smiGetTableColumns( sSdmObjects,
                                  SDM_OBJECTS_SMN_COL_ORDER,
                                  (const smiColumn**)&sSMNColumn )
              != IDE_SUCCESS );
    
    // mtdModule 설정
    IDE_TEST( mtd::moduleById( &(sSMNColumn->module),
                               sSMNColumn->type.dataTypeId )
              != IDE_SUCCESS );

    // mtlModule 설정
    IDE_TEST( mtl::moduleById( &sSMNColumn->language,
                               sSMNColumn->type.languageId )
              != IDE_SUCCESS );

    qciMisc::makeMetaRangeSingleColumn( &sSMNRange,
                                        (const mtcColumn *) sSMNColumn,
                                        (const void *) &aSMN,
                                        &sRange);

    sCursor.initialize();
    
    SMI_CURSOR_PROP_INIT_FOR_META_INDEX_SCAN( &sCursorProperty, NULL );

    IDE_TEST( sCursor.open(
                  QC_SMI_STMT( aStatement ),
                  sSdmObjects,
                  sSdmObjectsIndex[SDM_OBJECTS_IDX2_ORDER],
                  smiGetRowSCN( sSdmObjects ),
                  NULL,
                  &sRange,
                  smiGetDefaultKeyRange(),
                  smiGetDefaultFilter(),
                  QCM_META_CURSOR_FLAG,
                  SMI_SELECT_CURSOR,
                  &sCursorProperty )
              != IDE_SUCCESS );

    sIsCursorOpen = ID_TRUE;

    IDE_TEST( sCursor.beforeFirst() != IDE_SUCCESS );

    IDE_TEST( sCursor.readRow( &sRow, &sRid, SMI_FIND_NEXT )
              != IDE_SUCCESS );

    sIsCursorOpen = ID_TRUE;

    IDE_TEST( sCursor.beforeFirst() != IDE_SUCCESS );

    IDE_TEST( sCursor.readRow( &sRow, &sRid, SMI_FIND_NEXT )
              != IDE_SUCCESS );
    
    while( sRow != NULL )
    {
        sTableInfo.mShardID =
            *(mtdIntegerType*)((SChar *)sRow + sShardIDColumn->column.offset );

        sUserName = (mtdCharType*)((SChar *)sRow + sUserNameColumn->column.offset );
        idlOS::memcpy( sTableInfo.mUserName, sUserName->value, sUserName->length );
        sTableInfo.mUserName[sUserName->length] = '\0';

        sObjectName = (mtdCharType*)((SChar *)sRow + sObjectNameColumn->column.offset );
        idlOS::memcpy( sTableInfo.mObjectName, sObjectName->value, sObjectName->length );
        sTableInfo.mObjectName[sObjectName->length] = '\0';

        sObjectType = (mtdCharType*)((SChar *)sRow + sObjectTypeColumn->column.offset );
        if ( sObjectType->length == 1 )
        {
            sTableInfo.mObjectType = sObjectType->value[0];
        }
        else
        {
            sTableInfo.mObjectType = 0;
        }

        sSplitMethod = (mtdCharType*)((SChar *)sRow + sSplitMethodColumn->column.offset );
        if ( idlOS::strMatch( (SChar*)sSplitMethod->value, sSplitMethod->length,
                              "H", 1 ) == 0 )
        {
            sTableInfo.mSplitMethod = SDI_SPLIT_HASH;
        }
        else if ( idlOS::strMatch( (SChar*)sSplitMethod->value, sSplitMethod->length,
                                   "R", 1 ) == 0 )
        {
            sTableInfo.mSplitMethod = SDI_SPLIT_RANGE;
        }
        else if ( idlOS::strMatch( (SChar*)sSplitMethod->value, sSplitMethod->length,
                                   "L", 1 ) == 0 )
        {
            sTableInfo.mSplitMethod = SDI_SPLIT_LIST;
        }
        else if ( idlOS::strMatch( (SChar*)sSplitMethod->value, sSplitMethod->length,
                                   "C", 1 ) == 0 )
        {
            sTableInfo.mSplitMethod = SDI_SPLIT_CLONE;
        }
        else if ( idlOS::strMatch( (SChar*)sSplitMethod->value, sSplitMethod->length,
                                   "S", 1 ) == 0 )
        {
            sTableInfo.mSplitMethod = SDI_SPLIT_SOLO;
        }
        else
        {
            ideLog::log( IDE_SD_1, "[SHARD_META_ERROR] Failure. getTableInfo : unknown SPLIT_METHOD !!!\n");
            IDE_RAISE( ERR_META_CRASH );
        }

        sKeyColumnName = (mtdCharType*)((SChar *)sRow + sKeyColumnNameColumn->column.offset );
        idlOS::memcpy( sTableInfo.mKeyColumnName, sKeyColumnName->value, sKeyColumnName->length );
        sTableInfo.mKeyColumnName[sKeyColumnName->length] = '\0';

        /* PROJ-2655 Composite shard key */
        sSubSplitMethod = (mtdCharType*)((SChar *)sRow + sSubSplitMethodColumn->column.offset );

        if ( sSubSplitMethod->length == 1 )
        {
            if ( idlOS::strMatch( (SChar*)sSubSplitMethod->value, sSubSplitMethod->length,
                                  "H", 1 ) == 0 )
            {
                sTableInfo.mSubSplitMethod = SDI_SPLIT_HASH;
            }
            else if ( idlOS::strMatch( (SChar*)sSubSplitMethod->value, sSubSplitMethod->length,
                                       "R", 1 ) == 0 )
            {
                sTableInfo.mSubSplitMethod = SDI_SPLIT_RANGE;
            }
            else if ( idlOS::strMatch( (SChar*)sSubSplitMethod->value, sSubSplitMethod->length,
                                       "L", 1 ) == 0 )
            {
                sTableInfo.mSubSplitMethod = SDI_SPLIT_LIST;
            }
            else if ( idlOS::strMatch( (SChar*)sSubSplitMethod->value, sSubSplitMethod->length,
                                       "C", 1 ) == 0 )
            {
                sTableInfo.mSubSplitMethod = SDI_SPLIT_CLONE;
            }
            else if ( idlOS::strMatch( (SChar*)sSubSplitMethod->value, sSubSplitMethod->length,
                                       "S", 1 ) == 0 )
            {
                sTableInfo.mSubSplitMethod = SDI_SPLIT_SOLO;
            }
            else
            {
                ideLog::log( IDE_SD_1, "[SHARD_META_ERROR] Failure. getTableInfo : unknown SPLIT_METHOD !!!\n");
                IDE_RAISE( ERR_META_CRASH );
            }

            sTableInfo.mSubKeyExists = ID_TRUE;
        }
        else if ( sSubSplitMethod->length > 1 )
        {
            ideLog::log( IDE_SD_1, "[SHARD_META_ERROR] Failure. getTableInfo : unknown SPLIT_METHOD !!!\n");
            IDE_RAISE( ERR_META_CRASH );
        }
        else // sSubSplitMethod->length == 0
        {
            /* SDI_INIT_TABLE_INFO()에서 초기화 되어있지만, 다시 설정 */
            sTableInfo.mSubKeyExists   = ID_FALSE;
            sTableInfo.mSubSplitMethod = SDI_SPLIT_NONE;
        }

        sSubKeyColumnName = (mtdCharType*)((SChar *)sRow + sSubKeyColumnNameColumn->column.offset );
        idlOS::memcpy( sTableInfo.mSubKeyColumnName, sSubKeyColumnName->value, sSubKeyColumnName->length );
        sTableInfo.mSubKeyColumnName[sSubKeyColumnName->length] = '\0';

        sDefaultNodeID =
            *(mtdIntegerType*)((SChar *)sRow + sDefaultNodeIDColumn->column.offset );
        if ( sDefaultNodeID == MTD_INTEGER_NULL )
        {
            sTableInfo.mDefaultNodeId = SDI_NODE_NULL_ID;
        }
        else
        {
            sTableInfo.mDefaultNodeId = (UInt) sDefaultNodeID;
        }

        sDefaultPartitionName = (mtdCharType*)((SChar *)sRow + sDefaultPartitionNameColumn->column.offset );
        idlOS::memcpy( sTableInfo.mDefaultPartitionName, sDefaultPartitionName->value, sDefaultPartitionName->length );
        sTableInfo.mDefaultPartitionName[sDefaultPartitionName->length] = '\0';

        sDefaultReplicaSetId = *(mtdIntegerType*)((SChar *)sRow + sDefaultReplicaSetIdColumn->column.offset );
        sTableInfo.mDefaultPartitionReplicaSetId = (UInt) sDefaultReplicaSetId;

        IDE_TEST( setKeyDataType( aStatement,
                                  &sTableInfo ) != IDE_SUCCESS );

        IDE_TEST( QC_QMX_MEM(aStatement)->alloc( ID_SIZEOF(sdiTableInfoList) +
                                                 ID_SIZEOF(sdiTableInfo),
                                                 (void**) &sTableInfoList )
                  != IDE_SUCCESS );

        sTableInfoList->mTableInfo =
            (sdiTableInfo*)((UChar*)sTableInfoList + ID_SIZEOF(sdiTableInfoList));

        idlOS::memcpy( sTableInfoList->mTableInfo,
                       &sTableInfo,
                       ID_SIZEOF(sdiTableInfo) );

        sTableInfoList->mNext = NULL;            

        if( sIterTableInfoList == NULL )
        {
            sIterTableInfoList = sTableInfoList;
        }
        else
        {
            sTableInfoList->mNext = sIterTableInfoList;
            sIterTableInfoList = sTableInfoList;
        }
    
        IDE_TEST( sCursor.readRow( &sRow, &sRid, SMI_FIND_NEXT )
                  != IDE_SUCCESS );
    }

    *aTableInfoList = sTableInfoList;
    
    if ( sIsCursorOpen == ID_TRUE )
    {
        sIsCursorOpen = ID_FALSE;
        IDE_TEST( sCursor.close() != IDE_SUCCESS );
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_META_HANDLE )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDM_SHARD_META_NOT_CREATED,
                                  SDM_OBJECTS ) );
    }
    IDE_EXCEPTION( ERR_META_CRASH )
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QCM_META_CRASH));
    }
    IDE_EXCEPTION_END;

    if ( sIsCursorOpen == ID_TRUE )
    {
        (void)sCursor.close();
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_FAILURE;
}

IDE_RC sdm::setKeyDataType( qcStatement  * aStatement,
                            sdiTableInfo * aTableInfo )
{
    UInt                      sKeyType;
    UInt                      sSubKeyType;
    UInt                      sUserID;
    smSCN                     sTableSCN;
    void                    * sTableHandle = NULL;
    qcmTableInfo            * sQcmTableInfo = NULL;
    UInt                      i = 0;
    idBool                    sExistKey     = ID_FALSE;
    idBool                    sExistSubKey  = ID_FALSE;

    qsOID                     sProcOID;
    qsxProcInfo             * sProcInfo = NULL;
    qsVariableItems         * sParaDecls;

    IDE_TEST( qciMisc::getUserID( NULL,
                                  QC_SMI_STMT( aStatement ),
                                  aTableInfo->mUserName,
                                  idlOS::strlen( aTableInfo->mUserName ),
                                  &sUserID )
              != IDE_SUCCESS );

    if ( aTableInfo->mObjectType == 'T' ) /* Table */
    {
        IDE_TEST( qciMisc::getTableInfo( aStatement,
                                         sUserID,
                                         (UChar*)aTableInfo->mObjectName,
                                         idlOS::strlen( aTableInfo->mObjectName ),
                                         &sQcmTableInfo,
                                         &sTableSCN,
                                         &sTableHandle )
                  != IDE_SUCCESS );
        
        for ( i = 0; i < sQcmTableInfo->columnCount; i++ )
        {
            if ( idlOS::strMatch( sQcmTableInfo->columns[i].name,
                                  idlOS::strlen(sQcmTableInfo->columns[i].name),
                                  aTableInfo->mKeyColumnName,
                                  idlOS::strlen(aTableInfo->mKeyColumnName) ) == 0 )
            {
                sKeyType = sQcmTableInfo->columns[i].basicInfo->module->id;
                IDE_TEST_RAISE( sdm::isSupportDataType( sKeyType ) == ID_FALSE,
                                ERR_INVALID_SHARD_KEY_TYPE );

                aTableInfo->mKeyDataType = sKeyType;
                aTableInfo->mKeyColOrder = (UShort)i;
                sExistKey = ID_TRUE;

                if ( ( aTableInfo->mSubKeyExists == ID_FALSE ) ||
                     ( sExistSubKey == ID_TRUE ) )
                {
                    break;
                }
            }

            /* PROJ-2655 Composite shard key */
            if ( aTableInfo->mSubKeyExists == ID_TRUE )
            { 
                if ( idlOS::strMatch( sQcmTableInfo->columns[i].name,
                                      idlOS::strlen(sQcmTableInfo->columns[i].name),
                                      aTableInfo->mSubKeyColumnName,
                                      idlOS::strlen(aTableInfo->mSubKeyColumnName) ) == 0 )
                {   
                    sExistSubKey = ID_TRUE;

                    sSubKeyType = sQcmTableInfo->columns[i].basicInfo->module->id;
                    IDE_TEST_RAISE( sdm::isSupportDataType( sSubKeyType ) == ID_FALSE,
                                    ERR_INVALID_SHARD_KEY_TYPE );

                    aTableInfo->mSubKeyDataType = sSubKeyType;
                    aTableInfo->mSubKeyColOrder = (UShort)i;

                    if ( sExistKey == ID_TRUE )
                    {
                        break;
                    }
                }
            }
        }
    }
    else /* Procedure */
    {
        IDE_TEST( qcmProc::getProcExistWithEmptyByNamePtr( aStatement,
                                                           sUserID,
                                                           aTableInfo->mObjectName,
                                                           idlOS::strlen( aTableInfo->mObjectName ),
                                                           &sProcOID )
                  != IDE_SUCCESS );

        // BUG-48616
        IDE_TEST_RAISE( sProcOID == QS_EMPTY_OID, ERR_PROCEDURE_NOT_EXIST );
        
        IDE_TEST( qsxProc::getProcInfo( sProcOID, &sProcInfo ) != IDE_SUCCESS );

        if ( sdi::getSplitType( aTableInfo->mSplitMethod ) == SDI_SPLIT_TYPE_DIST )
        {
            for ( sParaDecls = sProcInfo->planTree->paraDecls, i = 0;
                  sParaDecls != NULL;
                  sParaDecls = sParaDecls->next, i++ )
            {
                if ( idlOS::strMatch( sParaDecls->name.stmtText + sParaDecls->name.offset,
                                      sParaDecls->name.size,
                                      aTableInfo->mKeyColumnName,
                                      idlOS::strlen( aTableInfo->mKeyColumnName ) ) == 0 )
                {
                    IDE_TEST_RAISE( sParaDecls->itemType != QS_VARIABLE,
                                    ERR_INVALID_SHARD_KEY_TYPE );
                    IDE_TEST_RAISE( ((qsVariables*)sParaDecls)->inOutType != QS_IN,
                                    ERR_INVALID_SHARD_KEY_TYPE );

                    sKeyType = sProcInfo->planTree->paramModules[i]->id;
                    IDE_TEST_RAISE( isSupportDataType( sKeyType ) == ID_FALSE,
                                    ERR_INVALID_SHARD_KEY_TYPE );

                    aTableInfo->mKeyDataType = sKeyType;
                    aTableInfo->mKeyColOrder = (UShort)i;
                    sExistKey = ID_TRUE;

                    if ( ( aTableInfo->mSubKeyExists == ID_FALSE ) ||
                         ( sExistSubKey == ID_TRUE ) )
                    {
                        break;
                    }
                }

                if ( aTableInfo->mSubKeyExists == ID_TRUE )
                {
                    if ( idlOS::strMatch( sParaDecls->name.stmtText + sParaDecls->name.offset,
                                          sParaDecls->name.size,
                                          aTableInfo->mSubKeyColumnName,
                                          idlOS::strlen( aTableInfo->mSubKeyColumnName ) ) == 0 )
                    {
                        sExistSubKey = ID_TRUE;
                        IDE_TEST_RAISE( sParaDecls->itemType != QS_VARIABLE,
                                        ERR_INVALID_SHARD_KEY_TYPE );
                        IDE_TEST_RAISE( ((qsVariables*)sParaDecls)->inOutType != QS_IN,
                                        ERR_INVALID_SHARD_KEY_TYPE );

                        sSubKeyType = sProcInfo->planTree->paramModules[i]->id;
                        IDE_TEST_RAISE( isSupportDataType( sSubKeyType ) == ID_FALSE,
                                        ERR_INVALID_SHARD_KEY_TYPE );

                        aTableInfo->mSubKeyDataType = sSubKeyType;
                        aTableInfo->mSubKeyColOrder = (UShort)i;

                        if ( sExistKey == ID_TRUE )
                        {
                            break;
                        }
                    }
                }
            }

            IDE_TEST_RAISE( sExistKey == ID_FALSE, ERR_NOT_EXIST_SHARD_KEY );

            IDE_TEST_RAISE( ( ( aTableInfo->mSubKeyExists == ID_TRUE ) &&
                              ( sExistSubKey == ID_FALSE ) ),
                            ERR_NOT_EXIST_SUB_SHARD_KEY );
        }
        else
        {
            /* Clone/Solo는 KeyDataType 가 필요 없다. */
        }
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_EXIST_SHARD_KEY )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDM_SHARD_KEY_COLUMN_NOT_EXIST,
                                  aTableInfo->mUserName,
                                  aTableInfo->mObjectName,
                                  aTableInfo->mKeyColumnName ) );
    }
    IDE_EXCEPTION( ERR_NOT_EXIST_SUB_SHARD_KEY )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDM_SHARD_KEY_COLUMN_NOT_EXIST,
                                  aTableInfo->mUserName,
                                  aTableInfo->mObjectName,
                                  aTableInfo->mSubKeyColumnName ) );
    }
    IDE_EXCEPTION( ERR_INVALID_SHARD_KEY_TYPE )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDM_UNSUPPORTED_SHARD_KEY_COLUMN_TYPE,
                                  aTableInfo->mUserName,
                                  aTableInfo->mObjectName,
                                  ( sExistSubKey == ID_TRUE )? aTableInfo->mSubKeyColumnName:
                                                               aTableInfo->mKeyColumnName ) );
    }
    IDE_EXCEPTION( ERR_PROCEDURE_NOT_EXIST )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDI_SHARD_OBJECT_NOT_EXIST,
                                  aTableInfo->mObjectName ));
    }
    IDE_EXCEPTION_END;

   
    return IDE_FAILURE;
}


IDE_RC sdm::insertFailoverHistory( qcStatement * aStatement,
                                   SChar       * aNodeName,
                                   ULong         aSMN,
                                   UInt        * aRowCnt )
{
    SChar      * sSqlStr;
    vSLong       sRowCnt;

    smiStatement            * sOldStmt = NULL;
    smiStatement              sSmiStmt;
    UInt                      sSmiStmtFlag;
    SInt                      sState = 0;

    sSmiStmtFlag = SMI_STATEMENT_NORMAL | SMI_STATEMENT_MEMORY_CURSOR;
    sOldStmt                = QC_SMI_STMT(aStatement);
    QC_SMI_STMT(aStatement) = &sSmiStmt;
    sState = 1;

    IDE_TEST( sSmiStmt.begin( aStatement->mStatistics,
                              sOldStmt,
                              sSmiStmtFlag )
              != IDE_SUCCESS );
    sState = 2;

    IDE_TEST( checkMetaVersion( QC_SMI_STMT( aStatement ) )
              != IDE_SUCCESS );

    IDE_TEST( STRUCT_ALLOC_WITH_SIZE( aStatement->qmxMem,
                                      SChar,
                                      QD_MAX_SQL_LENGTH,
                                      &sSqlStr )
              != IDE_SUCCESS);

    /* PRIMARY_NODE_NAME 이 변경되는 ReplicaSet을 Backup */
    idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                     "INSERT INTO SYS_SHARD.FAILOVER_HISTORY_ "
                     "  ( REPLICA_SET_ID, PRIMARY_NODE_NAME, FIRST_BACKUP_NODE_NAME, SECOND_BACKUP_NODE_NAME, "
                     "    STOP_FIRST_BACKUP_NODE_NAME, STOP_SECOND_BACKUP_NODE_NAME, "
                     "    FIRST_REPLICATION_NAME, FIRST_REPL_FROM_NODE_NAME, FIRST_REPL_TO_NODE_NAME, "
                     "    SECOND_REPLICATION_NAME, SECOND_REPL_FROM_NODE_NAME, SECOND_REPL_TO_NODE_NAME, SMN ) "
                     "SELECT REPLICA_SET_ID, PRIMARY_NODE_NAME, FIRST_BACKUP_NODE_NAME, SECOND_BACKUP_NODE_NAME, "
                     "      FIRST_REPLICATION_NAME, FIRST_REPL_FROM_NODE_NAME, FIRST_REPL_TO_NODE_NAME, "
                     "      STOP_FIRST_BACKUP_NODE_NAME, STOP_SECOND_BACKUP_NODE_NAME, "
                     "      SECOND_REPLICATION_NAME, SECOND_REPL_FROM_NODE_NAME, SECOND_REPL_TO_NODE_NAME, SMN "
                     "  FROM SYS_SHARD.REPLICA_SETS_ "
                     " WHERE SMN = "QCM_SQL_BIGINT_FMT" "
                     " AND PRIMARY_NODE_NAME = "QCM_SQL_VARCHAR_FMT,
                     aSMN,
                     aNodeName );

    IDE_TEST( qciMisc::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                     sSqlStr,
                                     & sRowCnt )
              != IDE_SUCCESS );

    /* FIRST_BACKUP_NODE_NAME 이 변경되는 ReplicaSet을 Backup */
    idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                     "INSERT INTO SYS_SHARD.FAILOVER_HISTORY_ "
                     "  ( REPLICA_SET_ID, PRIMARY_NODE_NAME, FIRST_BACKUP_NODE_NAME, SECOND_BACKUP_NODE_NAME, "
                     "    STOP_FIRST_BACKUP_NODE_NAME, STOP_SECOND_BACKUP_NODE_NAME, "
                     "    FIRST_REPLICATION_NAME, FIRST_REPL_FROM_NODE_NAME, FIRST_REPL_TO_NODE_NAME, "
                     "    SECOND_REPLICATION_NAME, SECOND_REPL_FROM_NODE_NAME, SECOND_REPL_TO_NODE_NAME, SMN ) "
                     "SELECT REPLICA_SET_ID, PRIMARY_NODE_NAME, FIRST_BACKUP_NODE_NAME, SECOND_BACKUP_NODE_NAME, "
                     "      FIRST_REPLICATION_NAME, FIRST_REPL_FROM_NODE_NAME, FIRST_REPL_TO_NODE_NAME, "
                     "      STOP_FIRST_BACKUP_NODE_NAME, STOP_SECOND_BACKUP_NODE_NAME, "
                     "      SECOND_REPLICATION_NAME, SECOND_REPL_FROM_NODE_NAME, SECOND_REPL_TO_NODE_NAME, SMN "
                     "  FROM SYS_SHARD.REPLICA_SETS_ "
                     " WHERE SMN = "QCM_SQL_BIGINT_FMT" "
                     " AND FIRST_BACKUP_NODE_NAME = "QCM_SQL_VARCHAR_FMT,
                     aSMN,
                     aNodeName );

    IDE_TEST( qciMisc::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                     sSqlStr,
                                     & sRowCnt )
              != IDE_SUCCESS );

    /* SECOND_BACKUP_NODE_NAME이 변경되는 ReplicaSet을 Backup */
    idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                     "INSERT INTO SYS_SHARD.FAILOVER_HISTORY_ "
                     "  ( REPLICA_SET_ID, PRIMARY_NODE_NAME, FIRST_BACKUP_NODE_NAME, SECOND_BACKUP_NODE_NAME, "
                     "    STOP_FIRST_BACKUP_NODE_NAME, STOP_SECOND_BACKUP_NODE_NAME, "
                     "    FIRST_REPLICATION_NAME, FIRST_REPL_FROM_NODE_NAME, FIRST_REPL_TO_NODE_NAME, "
                     "    SECOND_REPLICATION_NAME, SECOND_REPL_FROM_NODE_NAME, SECOND_REPL_TO_NODE_NAME, SMN ) "
                     "SELECT REPLICA_SET_ID, PRIMARY_NODE_NAME, FIRST_BACKUP_NODE_NAME, SECOND_BACKUP_NODE_NAME, "
                     "      STOP_FIRST_BACKUP_NODE_NAME, STOP_SECOND_BACKUP_NODE_NAME, "
                     "      FIRST_REPLICATION_NAME, FIRST_REPL_FROM_NODE_NAME, FIRST_REPL_TO_NODE_NAME, "
                     "      SECOND_REPLICATION_NAME, SECOND_REPL_FROM_NODE_NAME, SECOND_REPL_TO_NODE_NAME, SMN "
                     "  FROM SYS_SHARD.REPLICA_SETS_ "
                     " WHERE SMN = "QCM_SQL_BIGINT_FMT" "
                     " AND SECOND_BACKUP_NODE_NAME = "QCM_SQL_VARCHAR_FMT,
                     aSMN,
                     aNodeName );

    IDE_TEST( qciMisc::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                     sSqlStr,
                                     & sRowCnt )
              != IDE_SUCCESS );

    sState = 1;
    IDE_TEST( sSmiStmt.end( SMI_STATEMENT_RESULT_SUCCESS ) != IDE_SUCCESS );

    sState = 0;
    QC_SMI_STMT(aStatement) = sOldStmt;

    if ( aRowCnt != NULL )
    {
        *aRowCnt = (UInt)sRowCnt;
    }
    else
    {
        /* Nothing to do */
    }

    sdi::setShardMetaTouched( aStatement->session );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

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
 

    return IDE_FAILURE;
}


// delete FailOverHistory
IDE_RC sdm::deleteFailoverHistory( qcStatement * aStatement,
                                   ULong         /* aSMN */,
                                   UInt          aReplicaSetId,
                                   UInt        * aRowCnt )
{
    SChar      * sSqlStr;
    vSLong       sRowCnt;
    sdiGlobalMetaInfo sMetaNodeInfo = { SDI_NULL_SMN };

    IDE_TEST( checkMetaVersion( QC_SMI_STMT( aStatement ) )
              != IDE_SUCCESS );

    IDE_TEST( getGlobalMetaInfoCore( QC_SMI_STMT(aStatement),
                                         &sMetaNodeInfo ) != IDE_SUCCESS );

    IDE_TEST( STRUCT_ALLOC_WITH_SIZE( aStatement->qmxMem,
                                      SChar,
                                      QD_MAX_SQL_LENGTH,
                                      &sSqlStr )
              != IDE_SUCCESS);

    idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                     "DELETE FROM SYS_SHARD.FAILOVER_HISTORY_ "
                     " WHERE SMN = "QCM_SQL_BIGINT_FMT" "
                     " AND REPLICA_SET_ID = "QCM_SQL_INT32_FMT,
                     sMetaNodeInfo.mShardMetaNumber,
                     aReplicaSetId );

    IDE_TEST( qciMisc::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                     sSqlStr,
                                     & sRowCnt )
              != IDE_SUCCESS );

    if ( aRowCnt != NULL )
    {
        *aRowCnt = (UInt)sRowCnt;
    }
    else
    {
        /* Nothing to do */
    }

    sdi::setShardMetaTouched( aStatement->session );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* aNodeName에서 해당 ReplicaSet의 Failover가 가능한지 확인
 * [IN] aReplicaSetInfo : 전체 ReplicaSet 정보 PName 정렬됨
 * [IN] aReplicaSetId   : Failover 대상 Node에 연결되어 있던 ReplicaSetId
 * [IN] aNodeName       : Failover를 수행하는 Node의 NodeName  */
idBool sdm::checkFailoverAvailable( sdiReplicaSetInfo * aReplicaSetInfo,
                                    UInt                aReplicaSetId,
                                    SChar             * aNodeName )
{
    UInt                i = 0;
    idBool              sCanFailover = ID_FALSE;
    sdiReplicaSet     * sTargetReplicaSet = NULL;


    /* find matched ReplicaSet */
    for ( i= 0; i < aReplicaSetInfo->mCount; i++ )
    {
        if ( aReplicaSetInfo->mReplicaSets[i].mReplicaSetId == aReplicaSetId )
        {
            sTargetReplicaSet = &aReplicaSetInfo->mReplicaSets[i];
            break;
        }
    }

    /* Target ReplicaSet은 항상 있어야 한다. */
    IDE_DASSERT( sTargetReplicaSet != NULL );

    /* check can failover */
    if ( idlOS::strncmp( sTargetReplicaSet->mFirstBackupNodeName,
                         aNodeName,
                         SDI_NODE_NAME_MAX_SIZE + 1 ) == 0 )
    {
        sCanFailover = ID_TRUE;
    }

    if ( idlOS::strncmp( sTargetReplicaSet->mSecondBackupNodeName,
                         aNodeName,
                         SDI_NODE_NAME_MAX_SIZE + 1 ) == 0 )
    {
        sCanFailover = ID_TRUE;
    }

    return sCanFailover;
}

IDE_RC sdm::updateReplicaSet4Failover( qcStatement        * aStatement,
                                       SChar              * aOldNodeName,
                                       SChar              * aNewNodeName )
{
    SChar             * sSqlStr;
    vSLong              sRowCnt;

    sdiReplicaSetInfo   sReplicaSetInfo;

    sdiReplicaSet     * sReplicaSet = NULL;
    UInt                i = 0;

    smiStatement      * sOldStmt = NULL;
    smiStatement        sSmiStmt;
    UInt                sSmiStmtFlag;
    SInt                sState = 0;

    sdiGlobalMetaInfo sMetaNodeInfo = { SDI_NULL_SMN };

    sSmiStmtFlag = SMI_STATEMENT_NORMAL | SMI_STATEMENT_MEMORY_CURSOR;
    sOldStmt                = QC_SMI_STMT(aStatement);
    QC_SMI_STMT(aStatement) = &sSmiStmt;
    sState = 1;

    IDE_TEST( sSmiStmt.begin( aStatement->mStatistics,
                              sOldStmt,
                              sSmiStmtFlag )
              != IDE_SUCCESS );
    sState = 2;

    IDE_TEST( STRUCT_ALLOC_WITH_SIZE( aStatement->qmxMem,
                                      SChar,
                                      QD_MAX_SQL_LENGTH,
                                      &sSqlStr )
              != IDE_SUCCESS);

    IDE_TEST( checkMetaVersion( QC_SMI_STMT( aStatement ) )
              != IDE_SUCCESS );

    IDE_TEST( makeShardMeta4NewSMN(aStatement) != IDE_SUCCESS );

    IDE_TEST( getGlobalMetaInfoCore( QC_SMI_STMT(aStatement),
                                     &sMetaNodeInfo ) != IDE_SUCCESS );

    /* get ReplicaSet Info */
    IDE_TEST( sdm::getAllReplicaSetInfoSortedByPName( QC_SMI_STMT( aStatement ),
                                                      &sReplicaSetInfo,
                                                      sMetaNodeInfo.mShardMetaNumber )
              != IDE_SUCCESS );

    /* 여러 ReplicaSet이 한 Node에 Bind 되어 있을수 있다. */
    for ( i = 0 ; i < sReplicaSetInfo.mCount; i++ )
    {
        /* Find Old Node Primary ReplicaSet */
        if ( idlOS::strncmp( sReplicaSetInfo.mReplicaSets[i].mPrimaryNodeName,
                             aOldNodeName,
                             SDI_NODE_NAME_MAX_SIZE + 1 ) == 0 )
        {
            sReplicaSet = &sReplicaSetInfo.mReplicaSets[i];

            /* PRIMARY UPDATE */
            /* TargetNode를 Failover 가능하면 First/Second 를 맞춰서 NA로 변경하고 Primary를
             * FailoverNode로 변경한다. */
            if ( idlOS::strncmp( sReplicaSet->mFirstBackupNodeName,
                                 aNewNodeName,
                                 SDI_NODE_NAME_MAX_SIZE + 1 ) == 0 )
            {
                idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                                 "UPDATE SYS_SHARD.REPLICA_SETS_ "
                                 "   SET PRIMARY_NODE_NAME        = "QCM_SQL_VARCHAR_FMT", "
                                 "   FIRST_BACKUP_NODE_NAME       = "QCM_SQL_VARCHAR_FMT", "   
                                 "   STOP_FIRST_BACKUP_NODE_NAME  = "QCM_SQL_VARCHAR_FMT    
                                 "   WHERE PRIMARY_NODE_NAME      = "QCM_SQL_VARCHAR_FMT    
                                 "         AND REPLICA_SET_ID     = "QCM_SQL_UINT32_FMT     
                                 "         AND SMN                = "QCM_SQL_BIGINT_FMT,    
                                 aNewNodeName,
                                 SDM_NA_STR,
                                 sReplicaSet->mFirstBackupNodeName,
                                 aOldNodeName,
                                 sReplicaSet->mReplicaSetId,
                                 sMetaNodeInfo.mShardMetaNumber );

                IDE_TEST( qciMisc::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                                 sSqlStr,
                                                 & sRowCnt )
                          != IDE_SUCCESS );
                /* Primary의 변경은 RP Stop이 없다. */
            }
            else if ( idlOS::strncmp( sReplicaSet->mSecondBackupNodeName,
                                      aNewNodeName,
                                      SDI_NODE_NAME_MAX_SIZE + 1 ) == 0 )
            {
                idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                                 "UPDATE SYS_SHARD.REPLICA_SETS_ "
                                 "   SET PRIMARY_NODE_NAME         = "QCM_SQL_VARCHAR_FMT", "
                                 "   SECOND_BACKUP_NODE_NAME       = "QCM_SQL_VARCHAR_FMT", "
                                 "   STOP_SECOND_BACKUP_NODE_NAME  = "QCM_SQL_VARCHAR_FMT    
                                 "   WHERE PRIMARY_NODE_NAME       = "QCM_SQL_VARCHAR_FMT    
                                 "         AND REPLICA_SET_ID      = "QCM_SQL_UINT32_FMT     
                                 "         AND SMN                 = "QCM_SQL_BIGINT_FMT,    
                                 aNewNodeName,
                                 SDM_NA_STR,
                                 sReplicaSet->mSecondBackupNodeName,
                                 aOldNodeName,
                                 sReplicaSet->mReplicaSetId,
                                 sMetaNodeInfo.mShardMetaNumber );

                IDE_TEST( qciMisc::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                                 sSqlStr,
                                                 & sRowCnt )
                          != IDE_SUCCESS );
                /* Primary의 변경은 RP Stop이 없다. */
            }
        }
    }

    for ( i = 0 ; i < sReplicaSetInfo.mCount; i++ )
    {
        /* Find First To OldNode ReplicaSet */
        if ( idlOS::strncmp( sReplicaSetInfo.mReplicaSets[i].mFirstBackupNodeName,
                             aOldNodeName,
                             SDI_NODE_NAME_MAX_SIZE + 1 ) == 0 )
        {
            /* R2HA 하나 밖에 없어야 정상인데 ... check 해야 하나? */
            sReplicaSet = &sReplicaSetInfo.mReplicaSets[i];

            /* FIRST UPDATE */
            /* First To TargetNode 중인 ReplicaSet의 First 를 NA로 변경한다. */
            idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                             "UPDATE SYS_SHARD.REPLICA_SETS_ "
                             "   SET FIRST_BACKUP_NODE_NAME   = "QCM_SQL_VARCHAR_FMT", "
                             "   STOP_FIRST_BACKUP_NODE_NAME  = "QCM_SQL_VARCHAR_FMT    
                             "   WHERE FIRST_BACKUP_NODE_NAME = "QCM_SQL_VARCHAR_FMT 
                             "   AND REPLICA_SET_ID           = "QCM_SQL_UINT32_FMT     
                             "   AND SMN                      = "QCM_SQL_BIGINT_FMT, 
                             SDM_NA_STR,
                             sReplicaSet->mFirstBackupNodeName,
                             aOldNodeName,
                             sReplicaSet->mReplicaSetId,
                             sMetaNodeInfo.mShardMetaNumber );

            IDE_TEST( qciMisc::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                             sSqlStr,
                                             & sRowCnt )
                      != IDE_SUCCESS );

            /* To OldNode는 RP Stop이 필요하다. */
            (void)qdsd::stopReplicationForInternal( aStatement,
                                                    sReplicaSetInfo.mReplicaSets[i].mPrimaryNodeName,
                                                    sReplicaSetInfo.mReplicaSets[i].mFirstReplName );
        }

        /* Find Second To OldNode ReplicaSet */
        if ( idlOS::strncmp( sReplicaSetInfo.mReplicaSets[i].mSecondBackupNodeName,
                             aOldNodeName,
                             SDI_NODE_NAME_MAX_SIZE + 1 ) == 0 )
        {
            /* R2HA 하나 밖에 없어야 정상인데 ... check 해야 하나? */
            sReplicaSet = &sReplicaSetInfo.mReplicaSets[i];

            /* SECOND UPDATE */
            /* Second To TargetNode 중인 ReplicaSet의 Second 를 NA로 변경한다. */
            idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                             "UPDATE SYS_SHARD.REPLICA_SETS_ "
                             "   SET SECOND_BACKUP_NODE_NAME   = "QCM_SQL_VARCHAR_FMT", "
                             "   STOP_SECOND_BACKUP_NODE_NAME  = "QCM_SQL_VARCHAR_FMT    
                             "   WHERE SECOND_BACKUP_NODE_NAME = "QCM_SQL_VARCHAR_FMT 
                             "   AND REPLICA_SET_ID            = "QCM_SQL_UINT32_FMT     
                             "   AND SMN                       = "QCM_SQL_BIGINT_FMT, 
                             SDM_NA_STR,
                             sReplicaSet->mSecondBackupNodeName,
                             aOldNodeName,
                             sReplicaSet->mReplicaSetId,
                             sMetaNodeInfo.mShardMetaNumber );

            IDE_TEST( qciMisc::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                             sSqlStr,
                                             & sRowCnt )
                      != IDE_SUCCESS );
            /* To OldNode는 RP Stop이 필요하다. */
            (void)qdsd::stopReplicationForInternal( aStatement,
                                                    sReplicaSetInfo.mReplicaSets[i].mPrimaryNodeName,
                                                    sReplicaSetInfo.mReplicaSets[i].mSecondReplName );
        }
    }

    sState = 1;
    IDE_TEST( sSmiStmt.end( SMI_STATEMENT_RESULT_SUCCESS ) != IDE_SUCCESS );

    sState = 0;
    QC_SMI_STMT(aStatement) = sOldStmt;

    sdi::setShardMetaTouched( aStatement->session );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

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

    return IDE_FAILURE;
}

IDE_RC sdm::backupReplicaSet4Failover( qcStatement        * aStatement,
                                       SChar              * aTargetNodeName,
                                       SChar              * aFailoverNodeName,
                                       ULong                aSMN )
{
    SChar             * sSqlStr;
    vSLong              sRowCnt;

    sdiReplicaSetInfo   sReplicaSetInfo;

    sdiReplicaSet     * sReplicaSet = NULL;
    UInt                i = 0;

    smiStatement      * sOldStmt = NULL;
    smiStatement        sSmiStmt;
    UInt                sSmiStmtFlag;
    SInt                sState = 0;

    sdiGlobalMetaInfo sMetaNodeInfo = { ID_ULONG(0) };

    sSmiStmtFlag = SMI_STATEMENT_NORMAL | SMI_STATEMENT_MEMORY_CURSOR;
    sOldStmt                = QC_SMI_STMT(aStatement);
    QC_SMI_STMT(aStatement) = &sSmiStmt;
    sState = 1;

    IDE_TEST( sSmiStmt.begin( aStatement->mStatistics,
                              sOldStmt,
                              sSmiStmtFlag )
              != IDE_SUCCESS );
    sState = 2;

    IDE_TEST( STRUCT_ALLOC_WITH_SIZE( aStatement->qmxMem,
                                      SChar,
                                      QD_MAX_SQL_LENGTH,
                                      &sSqlStr )
              != IDE_SUCCESS);

    IDE_TEST( checkMetaVersion( QC_SMI_STMT( aStatement ) )
              != IDE_SUCCESS );

    IDE_TEST( getGlobalMetaInfoCore( QC_SMI_STMT(aStatement),
                                     &sMetaNodeInfo ) != IDE_SUCCESS );

    /* get ReplicaSet Info */
    IDE_TEST( sdm::getAllReplicaSetInfoSortedByPName( QC_SMI_STMT( aStatement ),
                                                      &sReplicaSetInfo,
                                                      sMetaNodeInfo.mShardMetaNumber )
              != IDE_SUCCESS );

    /* 여러 ReplicaSet이 한 Node에 Bind 되어 있을수 있다. */
    for ( i = 0 ; i < sReplicaSetInfo.mCount; i++ )
    {
        /* Find Target Node Primary ReplicaSet */
        if ( idlOS::strncmp( sReplicaSetInfo.mReplicaSets[i].mPrimaryNodeName,
                             aTargetNodeName,
                             SDI_NODE_NAME_MAX_SIZE + 1 ) == 0 )
        {
            sReplicaSet = &sReplicaSetInfo.mReplicaSets[i];

            /* PRIMARY UPDATE */
            /* TargetNode를 Failover 가능하면 First/Second 를 맞춰서 NA로 변경하고 Primary를
             * FailoverNode로 변경된다.
             * 변경은 추후에 하고 여기선 Backup만 수행 */
            if ( idlOS::strncmp( sReplicaSet->mFirstBackupNodeName,
                                 aFailoverNodeName,
                                 SDI_NODE_NAME_MAX_SIZE + 1 ) == 0 )
            {
                idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                                 "INSERT INTO SYS_SHARD.FAILOVER_HISTORY_ "
                                 "VALUES ( "
                                 QCM_SQL_INT32_FMT", "     /* REPLICA_SET_ID               */
                                 QCM_SQL_VARCHAR_FMT", "   /* PRIMARY_NODE_NAME            */
                                 QCM_SQL_VARCHAR_FMT", "   /* FIRST_BACKUP_NODE_NAME       */
                                 QCM_SQL_VARCHAR_FMT", "   /* SECOND_BACKUP_NODE_NAME      */
                                 QCM_SQL_VARCHAR_FMT", "   /* STOP_FIRST_BACKUP_NODE_NAME  */
                                 QCM_SQL_VARCHAR_FMT", "   /* STOP_SECOND_BACKUP_NODE_NAME */
                                 QCM_SQL_VARCHAR_FMT", "   /* FIRST_REPLICATION_NAME       */
                                 QCM_SQL_VARCHAR_FMT", "   /* FIRST_REPL_FROM_NODE_NAME    */
                                 QCM_SQL_VARCHAR_FMT", "   /* FIRST_REPL_TO_NODE_NAME      */
                                 QCM_SQL_VARCHAR_FMT", "   /* SECOND_REPLICATION_NAME      */
                                 QCM_SQL_VARCHAR_FMT", "   /* SECOND_REPL_FROM_NODE_NAME   */
                                 QCM_SQL_VARCHAR_FMT", "   /* SECOND_REPL_TO_NODE_NAME     */
                                 QCM_SQL_BIGINT_FMT" ) ",  /* SMN */
                                 sReplicaSet->mReplicaSetId,
                                 sReplicaSet->mPrimaryNodeName,
                                 sReplicaSet->mFirstBackupNodeName,
                                 sReplicaSet->mSecondBackupNodeName,
                                 sReplicaSet->mStopFirstBackupNodeName,
                                 sReplicaSet->mStopSecondBackupNodeName,
                                 sReplicaSet->mFirstReplName,
                                 sReplicaSet->mFirstReplFromNodeName,
                                 sReplicaSet->mFirstReplToNodeName,
                                 sReplicaSet->mSecondReplName,
                                 sReplicaSet->mSecondReplFromNodeName,
                                 sReplicaSet->mSecondReplToNodeName,
                                 aSMN );

                IDE_TEST( qciMisc::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                                 sSqlStr,
                                                 & sRowCnt )
                          != IDE_SUCCESS );
            }
            else if ( idlOS::strncmp( sReplicaSet->mSecondBackupNodeName,
                                      aFailoverNodeName,
                                      SDI_NODE_NAME_MAX_SIZE + 1 ) == 0 )
            {
                idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                                 "INSERT INTO SYS_SHARD.FAILOVER_HISTORY_ "
                                 "VALUES ( "
                                 QCM_SQL_INT32_FMT", "     /* REPLICA_SET_ID               */
                                 QCM_SQL_VARCHAR_FMT", "   /* PRIMARY_NODE_NAME            */
                                 QCM_SQL_VARCHAR_FMT", "   /* FIRST_BACKUP_NODE_NAME       */
                                 QCM_SQL_VARCHAR_FMT", "   /* SECOND_BACKUP_NODE_NAME      */
                                 QCM_SQL_VARCHAR_FMT", "   /* STOP_FIRST_BACKUP_NODE_NAME  */
                                 QCM_SQL_VARCHAR_FMT", "   /* STOP_SECOND_BACKUP_NODE_NAME */
                                 QCM_SQL_VARCHAR_FMT", "   /* FIRST_REPLICATION_NAME       */
                                 QCM_SQL_VARCHAR_FMT", "   /* FIRST_REPL_FROM_NODE_NAME    */
                                 QCM_SQL_VARCHAR_FMT", "   /* FIRST_REPL_TO_NODE_NAME      */
                                 QCM_SQL_VARCHAR_FMT", "   /* SECOND_REPLICATION_NAME      */
                                 QCM_SQL_VARCHAR_FMT", "   /* SECOND_REPL_FROM_NODE_NAME   */
                                 QCM_SQL_VARCHAR_FMT", "   /* SECOND_REPL_TO_NODE_NAME     */
                                 QCM_SQL_BIGINT_FMT" ) ",  /* SMN */
                                 sReplicaSet->mReplicaSetId,
                                 sReplicaSet->mPrimaryNodeName,
                                 sReplicaSet->mFirstBackupNodeName,
                                 sReplicaSet->mSecondBackupNodeName,
                                 sReplicaSet->mStopFirstBackupNodeName,
                                 sReplicaSet->mStopSecondBackupNodeName,
                                 sReplicaSet->mFirstReplName,
                                 sReplicaSet->mFirstReplFromNodeName,
                                 sReplicaSet->mFirstReplToNodeName,
                                 sReplicaSet->mSecondReplName,
                                 sReplicaSet->mSecondReplFromNodeName,
                                 sReplicaSet->mSecondReplToNodeName,
                                 aSMN );

                IDE_TEST( qciMisc::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                                 sSqlStr,
                                                 & sRowCnt )
                          != IDE_SUCCESS );
            }
        }
    }

    for ( i = 0 ; i < sReplicaSetInfo.mCount; i++ )
    {
        /* Find First To TargetNode ReplicaSet */
        if ( idlOS::strncmp( sReplicaSetInfo.mReplicaSets[i].mFirstBackupNodeName,
                             aTargetNodeName,
                             SDI_NODE_NAME_MAX_SIZE + 1 ) == 0 )
        {
            /* R2HA 하나 밖에 없어야 정상인데 ... check 해야 하나? */
            sReplicaSet = &sReplicaSetInfo.mReplicaSets[i];

            /* FIRST UPDATE */
            /* First To TargetNode 중인 ReplicaSet의 First 를 NA로 변경된다.
             * 변경은 추후에 하고 여기선 Backup만 수행 */
            idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                             "INSERT INTO SYS_SHARD.FAILOVER_HISTORY_ "
                             "VALUES ( "
                             QCM_SQL_INT32_FMT", "     /* REPLICA_SET_ID               */
                             QCM_SQL_VARCHAR_FMT", "   /* PRIMARY_NODE_NAME            */
                             QCM_SQL_VARCHAR_FMT", "   /* FIRST_BACKUP_NODE_NAME       */
                             QCM_SQL_VARCHAR_FMT", "   /* SECOND_BACKUP_NODE_NAME      */
                             QCM_SQL_VARCHAR_FMT", "   /* STOP_FIRST_BACKUP_NODE_NAME  */
                             QCM_SQL_VARCHAR_FMT", "   /* STOP_SECOND_BACKUP_NODE_NAME */
                             QCM_SQL_VARCHAR_FMT", "   /* FIRST_REPLICATION_NAME       */
                             QCM_SQL_VARCHAR_FMT", "   /* FIRST_REPL_FROM_NODE_NAME    */
                             QCM_SQL_VARCHAR_FMT", "   /* FIRST_REPL_TO_NODE_NAME      */
                             QCM_SQL_VARCHAR_FMT", "   /* SECOND_REPLICATION_NAME      */
                             QCM_SQL_VARCHAR_FMT", "   /* SECOND_REPL_FROM_NODE_NAME   */
                             QCM_SQL_VARCHAR_FMT", "   /* SECOND_REPL_TO_NODE_NAME     */
                             QCM_SQL_BIGINT_FMT" ) ",  /* SMN */
                             sReplicaSet->mReplicaSetId,
                             sReplicaSet->mPrimaryNodeName,
                             sReplicaSet->mFirstBackupNodeName,
                             sReplicaSet->mSecondBackupNodeName,
                             sReplicaSet->mStopFirstBackupNodeName,
                             sReplicaSet->mStopSecondBackupNodeName,
                             sReplicaSet->mFirstReplName,
                             sReplicaSet->mFirstReplFromNodeName,
                             sReplicaSet->mFirstReplToNodeName,
                             sReplicaSet->mSecondReplName,
                             sReplicaSet->mSecondReplFromNodeName,
                             sReplicaSet->mSecondReplToNodeName,
                             aSMN );

            IDE_TEST( qciMisc::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                             sSqlStr,
                                             & sRowCnt )
                      != IDE_SUCCESS );
        }

        /* Find Second To TargetNode ReplicaSet */
        if ( idlOS::strncmp( sReplicaSetInfo.mReplicaSets[i].mSecondBackupNodeName,
                             aTargetNodeName,
                             SDI_NODE_NAME_MAX_SIZE + 1 ) == 0 )
        {
            /* R2HA 하나 밖에 없어야 정상인데 ... check 해야 하나? */
            sReplicaSet = &sReplicaSetInfo.mReplicaSets[i];

            /* SECOND UPDATE */
            /* Second To TargetNode 중인 ReplicaSet의 Second 를 NA로 변경된다.
             * 변경은 추후에 하고 여기선 Backup만 수행 */
            idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                             "INSERT INTO SYS_SHARD.FAILOVER_HISTORY_ "
                             "VALUES ( "
                             QCM_SQL_INT32_FMT", "     /* REPLICA_SET_ID               */
                             QCM_SQL_VARCHAR_FMT", "   /* PRIMARY_NODE_NAME            */
                             QCM_SQL_VARCHAR_FMT", "   /* FIRST_BACKUP_NODE_NAME       */
                             QCM_SQL_VARCHAR_FMT", "   /* SECOND_BACKUP_NODE_NAME      */
                             QCM_SQL_VARCHAR_FMT", "   /* STOP_FIRST_BACKUP_NODE_NAME  */
                             QCM_SQL_VARCHAR_FMT", "   /* STOP_SECOND_BACKUP_NODE_NAME */
                             QCM_SQL_VARCHAR_FMT", "   /* FIRST_REPLICATION_NAME       */
                             QCM_SQL_VARCHAR_FMT", "   /* FIRST_REPL_FROM_NODE_NAME    */
                             QCM_SQL_VARCHAR_FMT", "   /* FIRST_REPL_TO_NODE_NAME      */
                             QCM_SQL_VARCHAR_FMT", "   /* SECOND_REPLICATION_NAME      */
                             QCM_SQL_VARCHAR_FMT", "   /* SECOND_REPL_FROM_NODE_NAME   */
                             QCM_SQL_VARCHAR_FMT", "   /* SECOND_REPL_TO_NODE_NAME     */
                             QCM_SQL_BIGINT_FMT" ) ",  /* SMN */
                             sReplicaSet->mReplicaSetId,
                             sReplicaSet->mPrimaryNodeName,
                             sReplicaSet->mFirstBackupNodeName,
                             sReplicaSet->mSecondBackupNodeName,
                             sReplicaSet->mStopFirstBackupNodeName,
                             sReplicaSet->mStopSecondBackupNodeName,
                             sReplicaSet->mFirstReplName,
                             sReplicaSet->mFirstReplFromNodeName,
                             sReplicaSet->mFirstReplToNodeName,
                             sReplicaSet->mSecondReplName,
                             sReplicaSet->mSecondReplFromNodeName,
                             sReplicaSet->mSecondReplToNodeName,
                             aSMN );

            IDE_TEST( qciMisc::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                             sSqlStr,
                                             & sRowCnt )
                      != IDE_SUCCESS );
        }
    }

    sState = 1;
    IDE_TEST( sSmiStmt.end( SMI_STATEMENT_RESULT_SUCCESS ) != IDE_SUCCESS );

    sState = 0;
    QC_SMI_STMT(aStatement) = sOldStmt;

    sdi::setShardMetaTouched( aStatement->session );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

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

    return IDE_FAILURE;
}

IDE_RC sdm::validateShardObjectTable( smiStatement   * aStatement,
                                      SChar          * aUserName,
                                      SChar          * aTableName,
                                      sdfArgument    * aArgumentList,
                                      sdiSplitMethod   aSplitMethod,
                                      UInt             aPartitionCnt,
                                      idBool         * aExistIndex )
{
    UInt                   sUserID = 0;
    smSCN                  sTableSCN;
    smiStatement           sSmiStmt;
    UInt                   sSmiStmtFlag = 0;
    UInt                   sPartitionCnt = 0;
    SInt                   sState = 0;
    qcStatement            sStatement;
    void                 * sTableHandle = NULL;
    qcmTableInfo         * sTableInfo = NULL;
    qcmPartitionInfoList * sTablePartInfoList = NULL;
    qcmPartitionInfoList * sTempTablePartInfoList = NULL;
    qcmTableInfo         * sPartInfo = NULL;
    sdfArgument          * sIterator = NULL;
    idBool                 sFound = ID_FALSE;
    sdiLocalMetaInfo       sLocalMetaInfo;
    qcmIndex             * sTempIndex = NULL;
    
    sSmiStmtFlag = SMI_STATEMENT_NORMAL | SMI_STATEMENT_MEMORY_CURSOR;

    IDE_TEST( qcg::allocStatement( &sStatement,
                                   NULL,
                                   NULL,
                                   NULL )
              != IDE_SUCCESS );
    sState = 1;

    qcg::setSmiStmt(&sStatement, &sSmiStmt);

    IDE_TEST( sSmiStmt.begin( NULL,
                              aStatement,
                              sSmiStmtFlag )
              != IDE_SUCCESS );
    sState = 2;

    IDE_TEST_RAISE( qcmUser::getUserID( NULL,
                                        &sSmiStmt,
                                        aUserName,
                                        idlOS::strlen(aUserName),
                                        & sUserID ) != IDE_SUCCESS,
                    ERR_META_HANDLE );

    IDE_TEST( qcm::getTableHandleByName( &sSmiStmt,
                                         sUserID,
                                         (UChar *)aTableName,
                                         idlOS::strlen(aTableName),
                                         (void**)&sTableHandle,
                                         &sTableSCN ) 
              != IDE_SUCCESS );

    IDE_TEST(smiValidateAndLockObjects( sSmiStmt.getTrans(),
                                        sTableHandle,
                                        sTableSCN,
                                        SMI_TBSLV_DDL_DML,
                                        SMI_TABLE_LOCK_IS,
                                        ID_ULONG_MAX,
                                        ID_FALSE)
             != IDE_SUCCESS);

    IDE_TEST( smiGetTableTempInfo( sTableHandle,
                                   (void**)&sTableInfo )
              != IDE_SUCCESS );

    IDE_TEST( sdm::getLocalMetaInfo( &sLocalMetaInfo ) != IDE_SUCCESS );
    
    if ( sLocalMetaInfo.mKSafety > 0 )
    {
        // volatile table 은 허용하지 않는다.
        IDE_TEST_RAISE( smiTableSpace::isVolatileTableSpaceType( sTableInfo->TBSType ) == ID_TRUE,
                        ERR_NOT_SUPPORTED_VOLATILE_TABLE );
    }
                
    // CHECK VIEW
    IDE_TEST_RAISE( ( sTableInfo->tableType == QCM_VIEW ) ||
                    ( sTableInfo->tableType == QCM_MVIEW_VIEW ), ERR_CANT_VIEW );

    // CHECK NON PARTITIONED INDEX
    IDE_TEST_RAISE( qcm::existGlobalNonPartitionedIndex( sTableInfo, &sTempIndex ) == ID_TRUE,
                    ERR_NOT_SUPPORT_GLOBAL_NON_PARTITION_INDEX );

    if ( sTableInfo->tablePartitionType == QCM_NONE_PARTITIONED_TABLE )
    {
        // CHECK PRIMARY KEY
        IDE_TEST_RAISE( sTableInfo->primaryKey == NULL, ERR_NOT_EXIST_PRIMARY_KEY );
    }
    else if ( sTableInfo->tablePartitionType == QCM_PARTITIONED_TABLE )
    {
        if (( aSplitMethod == SDI_SPLIT_RANGE ) ||
            ( aSplitMethod == SDI_SPLIT_LIST ) ||
            ( aSplitMethod == SDI_SPLIT_HASH ))
        {
            // GET PARTITION COUNT
            IDE_TEST( qcmPartition::getPartitionCount4SmiStmt( &sSmiStmt,
                                                               sTableInfo->tableID,
                                                               &sPartitionCnt )
                      != IDE_SUCCESS );

            IDE_TEST_RAISE( sPartitionCnt != aPartitionCnt, ERR_DIFF_PARTTION_COUNT );
        }
        
        IDE_TEST( qcmPartition::getPartitionInfoList( &sStatement,
                                                      &sSmiStmt,
                                                      (iduMemory *)QCI_QMX_MEM( &sStatement ),
                                                      sTableInfo->tableID,
                                                      &sTablePartInfoList,
                                                      ID_TRUE /* aIsRealPart */ )
                  != IDE_SUCCESS );
    
        for ( sTempTablePartInfoList = sTablePartInfoList;
              sTempTablePartInfoList != NULL;
              sTempTablePartInfoList = sTempTablePartInfoList->next )
        {
            sPartInfo = sTempTablePartInfoList->partitionInfo;

            if (( aSplitMethod == SDI_SPLIT_RANGE ) ||
                ( aSplitMethod == SDI_SPLIT_LIST ) ||
                ( aSplitMethod == SDI_SPLIT_HASH ))
            {
                // CHAEK PARTITION
                sFound = ID_FALSE;
        
                for ( sIterator = aArgumentList;
                      sIterator != NULL;
                      sIterator = sIterator->mNext )
                {
                    if ( idlOS::strMatch( sIterator->mArgument1,
                                          idlOS::strlen(sIterator->mArgument1) + 1,
                                          sPartInfo->name,
                                          idlOS::strlen(sPartInfo->name) + 1 ) == 0 )
                    {
                        IDE_TEST_RAISE( sFound == ID_TRUE, ERR_DUPLICATION_PARTITION );
                        sFound = ID_TRUE;
                    }
                }
        
                IDE_TEST_RAISE( sFound != ID_TRUE, ERR_PARTITION_NOT_FOUND );
            }   
        
            // CHECK PRIMARY KEY
            if ( sPartInfo->primaryKey != NULL)
            {
                IDE_TEST_RAISE( qciMisc::intersectColumn(
                                    (UInt*) smiGetIndexColumns( sPartInfo->primaryKey->indexHandle ),
                                    sPartInfo->primaryKey->keyColCount,
                                    &sPartInfo->partKeyColumns[0].basicInfo->column.id,
                                    1) != ID_TRUE,
                                ERR_NOT_EXIST_PRIMARY_KEY);
            }
            else
            {
                IDE_RAISE( ERR_NOT_EXIST_PRIMARY_KEY );
            }
        }
    }

    // CHECK INDEX
    // primary check는 이미 되었고 primary key는 index count에 포함. 1 보다 커야함
    if ( sTableInfo->indexCount > 1 )
    {
        *aExistIndex = ID_TRUE;
    }
    
    IDE_TEST( sSmiStmt.end( SMI_STATEMENT_RESULT_SUCCESS ) != IDE_SUCCESS );
    sState = 1;

    sState = 0;
    IDE_TEST( qcg::freeStatement( &sStatement ) != IDE_SUCCESS );

    return IDE_SUCCESS;
    
    IDE_EXCEPTION( ERR_NOT_EXIST_PRIMARY_KEY )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDM_NOT_EXISTS_PRIMARY_KEY ));
    }
    IDE_EXCEPTION( ERR_CANT_VIEW )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDM_USE_SHARD_TABLE_IN_VIEW,
                                  sTableInfo->name ));
    }    
    IDE_EXCEPTION( ERR_META_HANDLE )
    {
        if ( ideGetErrorCode() == qpERR_ABORT_QCM_NOT_EXIST_USER )
        {
            IDE_SET( ideSetErrorCode(sdERR_ABORT_SDM_SHARD_META_NOT_CREATED,
                                     SDM_USER) );
        }
        else if ( ideGetErrorCode() == qpERR_ABORT_QCM_NOT_EXIST_TABLE )
        {
            IDE_SET( ideSetErrorCode(sdERR_ABORT_SDM_SHARD_META_NOT_CREATED,
                                     aTableName) );
        }
    }
    IDE_EXCEPTION( ERR_DIFF_PARTTION_COUNT )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDM_MISSMATCHED_PARTITION_COUNT ));
    }
    IDE_EXCEPTION( ERR_PARTITION_NOT_FOUND )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDM_TABLE_PARTITION,
                                  sPartInfo->name ));
    }
    IDE_EXCEPTION( ERR_DUPLICATION_PARTITION )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDM_DUPLICATED_TABLE_PARTITION,
                                  sPartInfo->name ));
    }
    IDE_EXCEPTION( ERR_NOT_SUPPORTED_VOLATILE_TABLE )
    {    
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QRC_CANNOT_USE_VOLATILE_TABLE )); 
    }
    IDE_EXCEPTION( ERR_NOT_SUPPORT_GLOBAL_NON_PARTITION_INDEX );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_ROLLBACKABLE_DDL_GLOBAL_NOT_ALLOWED_NON_PART_INDEX,
                                  sTempIndex->name ) );
    }

    IDE_EXCEPTION_END;

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
            (void) qcg::freeStatement( &sStatement );
        default:
            break;
    }
    return IDE_FAILURE;

}

IDE_RC sdm::flushReplTable( qcStatement * aStatement,
                            SChar       * aNodeName,
                            SChar       * aUserName,
                            SChar       * aReplName,
                            idBool        aIsNewTrans,
                            UInt          aWaitSecond )
{
    SChar      * sSqlStr;
    UInt         sUserID = QC_EMPTY_USER_ID;
    SChar        sSqlRemoteStr[QD_MAX_SQL_LENGTH + 1];
    UInt         sExecCount = 0;
    sdiLocalMetaInfo    sLocalMetaInfo;
    
    IDE_TEST( STRUCT_ALLOC_WITH_SIZE( aStatement->qmxMem,
                                      SChar,
                                      QD_MAX_SQL_LENGTH,
                                      &sSqlStr )
              != IDE_SUCCESS);

    if ( aWaitSecond > 0 )
    {
        idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                         "ALTER REPLICATION "QCM_SQL_STRING_SKIP_FMT" FLUSH WAIT %"ID_UINT32_FMT,
                         aReplName,
                         aWaitSecond );
    }
    else
    {
        idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                         "ALTER REPLICATION "QCM_SQL_STRING_SKIP_FMT" FLUSH ",
                         aReplName );
    }

    IDE_TEST( qciMisc::getUserIdByName( aUserName, &sUserID ) != IDE_SUCCESS );

    IDE_TEST( sdi::getLocalMetaInfo( &sLocalMetaInfo ) != IDE_SUCCESS );

    ideLog::log(IDE_SD_17,"[flush SHARD INTERNAL SQL]: %s, %s",aNodeName, sSqlStr);

    if ( aIsNewTrans == ID_TRUE )
    {
        if ( idlOS::strncmp( sLocalMetaInfo.mNodeName, 
                             aNodeName,
                             SDI_NODE_NAME_MAX_SIZE + 1 ) == 0 )
        {
            ideLog::log(IDE_SD_17,"[SHARD_META] Internal SQL: %s", sSqlStr);
            IDE_TEST( sdi::shardExecTempDMLOrDCLWithNewTrans( aStatement,
                                                              sSqlStr )
                      != IDE_SUCCESS );
        
        }   
        else
        {
            idlOS::snprintf( sSqlRemoteStr, QD_MAX_SQL_LENGTH,
                             "EXEC DBMS_SHARD.EXECUTE_IMMEDIATE( '%s', '"QCM_SQL_STRING_SKIP_FMT"' ) ",
                             sSqlStr,
                             aNodeName );
            
            IDE_TEST( sdi::shardExecTempDMLOrDCLWithNewTrans( aStatement,
                                                              sSqlRemoteStr )
                      != IDE_SUCCESS );
        }
    }
    else
    {
            
        if ( idlOS::strncmp( sLocalMetaInfo.mNodeName, 
                             aNodeName,
                             SDI_NODE_NAME_MAX_SIZE + 1 ) == 0 )
        {
            ideLog::log(IDE_SD_17,"[SHARD_META] Internal SQL: %s", sSqlStr);
            IDE_TEST( qciMisc::runDCLforInternal( aStatement,
                                                  sSqlStr,
                                                  aStatement->session->mMmSession )
                      != IDE_SUCCESS );
        }   
        else
        {
            IDE_TEST( sdi::checkShardLinker( aStatement ) != IDE_SUCCESS );

            if ( aStatement->session->mQPSpecific.mClientInfo != NULL )
            {
        
                IDE_TEST( sdi::shardExecDirect( aStatement,
                                                aNodeName,
                                                sSqlStr,
                                                idlOS::strlen(sSqlStr),
                                                SDI_INTERNAL_OP_NORMAL,
                                                &sExecCount,
                                                NULL,
                                                NULL,
                                                0,
                                                NULL )
                          != IDE_SUCCESS );
            }
        }
    }
        
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* PROJ-2748 Shard Failback */
IDE_RC sdm::getFailoverHistoryWithPrimaryNodename( smiStatement       * aSmiStmt,
                                                   sdiReplicaSetInfo  * aReplicaSetsInfo,
                                                   SChar              * aPrimaryNodeName,
                                                   ULong                aSMN )
{
    idBool            sIsCursorOpen = ID_FALSE;
    const void      * sRow          = NULL;
    scGRID            sRid;
    const void      * sFailoverHistory;
    const void      * sFailoverHistoryIdx[SDM_MAX_META_INDICES];
    mtcColumn       * sReplicaSetIDColumn;
    mtcColumn       * sPrimaryNodeNameColumn;
    mtcColumn       * sFirstBackupNodeNameColumn;
    mtcColumn       * sSecondBackupNodeNameColumn;
    mtcColumn       * sStopFirstBackupNodeNameColumn;
    mtcColumn       * sStopSecondBackupNodeNameColumn;
    mtcColumn       * sFirstReplicationNameColumn;
    mtcColumn       * sFirstReplFromNodeNameColumn;
    mtcColumn       * sFirstReplToNodeNameColumn;
    mtcColumn       * sSecondReplicaiontNameColumn;
    mtcColumn       * sSecondReplFromNodeNameColumn;
    mtcColumn       * sSecondReplToNodeNameColumn;
    mtcColumn       * sSMNColumn;

    qcNameCharBuffer  sNameValueBuffer;
    mtdCharType     * sPrimaryNodeNameValue = ( mtdCharType * ) & sNameValueBuffer;

    UShort            sCount = 0;

    mtdIntegerType    sReplicaSetID;
    mtdCharType     * sPrimaryNodeName;
    mtdCharType     * sFirstBackupNodeName;
    mtdCharType     * sSecondBackupNodeName;
    mtdCharType     * sStopFirstBackupNodeName;
    mtdCharType     * sStopSecondBackupNodeName;
    mtdCharType     * sFirstReplicationName;
    mtdCharType     * sFirstReplFromNodeName;
    mtdCharType     * sFirstReplToNodeName;
    mtdCharType     * sSecondReplicaiontName;
    mtdCharType     * sSecondReplFromNodeName;
    mtdCharType     * sSecondReplToNodeName;
    mtdIntegerType    sSMN;

    smiTableCursor       sCursor;
    smiCursorProperties  sCursorProperty;

    qtcMetaRangeColumn sPrimaryNodeNameRange;
    qtcMetaRangeColumn sSMNRange;
    smiRange           sRange;

    /* init */
    aReplicaSetsInfo->mCount = 0;

    IDE_TEST( checkMetaVersion( aSmiStmt )
              != IDE_SUCCESS );

    IDE_TEST( getMetaTableAndIndex( aSmiStmt,
                                    SDM_FAILOVER_HISTORY_,
                                    &sFailoverHistory,
                                    sFailoverHistoryIdx )
              != IDE_SUCCESS );

    /* primary_node_name index */
    IDE_TEST_RAISE( sFailoverHistoryIdx[SDM_FAILOVER_HISTORY_IDX2_ORDER] == NULL,
                    ERR_META_HANDLE );

    IDE_TEST( smiGetTableColumns( sFailoverHistory,
                                  SDM_FAILOVER_HISTORY_REPLICA_SET_ID_COL_ORDER,
                                  (const smiColumn**)&sReplicaSetIDColumn )
              != IDE_SUCCESS );
    IDE_TEST( smiGetTableColumns( sFailoverHistory,
                                  SDM_FAILOVER_HISTORY_PRIMARY_NODE_NAME_COL_ORDER,
                                  (const smiColumn**)&sPrimaryNodeNameColumn )
              != IDE_SUCCESS );
    IDE_TEST( smiGetTableColumns( sFailoverHistory,
                                  SDM_FAILOVER_HISTORY_FIRST_BACKUP_NODE_NAME_COL_ORDER,
                                  (const smiColumn**)&sFirstBackupNodeNameColumn )
              != IDE_SUCCESS );
    IDE_TEST( smiGetTableColumns( sFailoverHistory,
                                  SDM_FAILOVER_HISTORY_SECOND_BACKUP_NODE_NAME_COL_ORDER,
                                  (const smiColumn**)&sSecondBackupNodeNameColumn )
              != IDE_SUCCESS );
    IDE_TEST( smiGetTableColumns( sFailoverHistory,
                                  SDM_FAILOVER_HISTORY_STOP_FIRST_BACKUP_NODE_NAME_COL_ORDER,
                                  (const smiColumn**)&sStopFirstBackupNodeNameColumn )
              != IDE_SUCCESS );
    IDE_TEST( smiGetTableColumns( sFailoverHistory,
                                  SDM_FAILOVER_HISTORY_STOP_SECOND_BACKUP_NODE_NAME_COL_ORDER,
                                  (const smiColumn**)&sStopSecondBackupNodeNameColumn )
              != IDE_SUCCESS );
    IDE_TEST( smiGetTableColumns( sFailoverHistory,
                                  SDM_FAILOVER_HISTORY_FIRST_REPLICATION_NAME_COL_ORDER,
                                  (const smiColumn**)&sFirstReplicationNameColumn )
              != IDE_SUCCESS );
    IDE_TEST( smiGetTableColumns( sFailoverHistory,
                                  SDM_FAILOVER_HISTORY_FIRST_REPL_FROM_NODE_NAME_COL_ORDER,
                                  (const smiColumn**)&sFirstReplFromNodeNameColumn )
              != IDE_SUCCESS );
    IDE_TEST( smiGetTableColumns( sFailoverHistory,
                                  SDM_FAILOVER_HISTORY_FIRST_REPL_TO_NODE_NAME_COL_ORDER,
                                  (const smiColumn**)&sFirstReplToNodeNameColumn )
              != IDE_SUCCESS );
    IDE_TEST( smiGetTableColumns( sFailoverHistory,
                                  SDM_FAILOVER_HISTORY_SECOND_REPLICATION_NAME_COL_ORDER,
                                  (const smiColumn**)&sSecondReplicaiontNameColumn )
              != IDE_SUCCESS );
    IDE_TEST( smiGetTableColumns( sFailoverHistory,
                                  SDM_FAILOVER_HISTORY_SECOND_REPL_FROM_NODE_NAME_COL_ORDER,
                                  (const smiColumn**)&sSecondReplFromNodeNameColumn )
              != IDE_SUCCESS );
    IDE_TEST( smiGetTableColumns( sFailoverHistory,
                                  SDM_FAILOVER_HISTORY_SECOND_REPL_TO_NODE_NAME_COL_ORDER,
                                  (const smiColumn**)&sSecondReplToNodeNameColumn )
              != IDE_SUCCESS );

    IDE_TEST( smiGetTableColumns( sFailoverHistory,
                                  SDM_FAILOVER_HISTORY_SMN_COL_ORDER,
                                  (const smiColumn**)&sSMNColumn )
              != IDE_SUCCESS );

    // mtdModule 설정
    IDE_TEST( mtd::moduleById( &(sPrimaryNodeNameColumn->module),
                               sPrimaryNodeNameColumn->type.dataTypeId )
              != IDE_SUCCESS );

    // mtlModule 설정
    IDE_TEST( mtl::moduleById( &sPrimaryNodeNameColumn->language,
                               sPrimaryNodeNameColumn->type.languageId )
              != IDE_SUCCESS );

    // mtdModule 설정
    IDE_TEST( mtd::moduleById( &(sSMNColumn->module),
                               sSMNColumn->type.dataTypeId )
              != IDE_SUCCESS );

    // mtlModule 설정
    IDE_TEST( mtl::moduleById( &sSMNColumn->language,
                               sSMNColumn->type.languageId )
              != IDE_SUCCESS );

    qciMisc::setVarcharValue( sPrimaryNodeNameValue,
                              NULL,
                              aPrimaryNodeName,
                              idlOS::strlen(aPrimaryNodeName) );

    qciMisc::makeMetaRangeDoubleColumn(
        &sPrimaryNodeNameRange,
        &sSMNRange,
        sPrimaryNodeNameColumn,
        (const void *) sPrimaryNodeNameValue,
        sSMNColumn,
        (const void *) &aSMN,
        &sRange );

    sCursor.initialize();

    /* PROJ-2622 */
    SMI_CURSOR_PROP_INIT_FOR_META_INDEX_SCAN( &sCursorProperty, NULL );

    IDE_TEST( sCursor.open(
                  aSmiStmt,
                  sFailoverHistory,
                  sFailoverHistoryIdx[SDM_FAILOVER_HISTORY_IDX2_ORDER],
                  smiGetRowSCN( sFailoverHistory ),
                  NULL,
                  &sRange,
                  smiGetDefaultKeyRange(),
                  smiGetDefaultFilter(),
                  QCM_META_CURSOR_FLAG,
                  SMI_SELECT_CURSOR,
                  &sCursorProperty )
              != IDE_SUCCESS );

    sIsCursorOpen = ID_TRUE;

    IDE_TEST( sCursor.beforeFirst() != IDE_SUCCESS );

    IDE_TEST( sCursor.readRow( &sRow, &sRid, SMI_FIND_NEXT )
              != IDE_SUCCESS );

    while ( sRow != NULL )
    {
        sReplicaSetID = *(mtdIntegerType*)((SChar *)sRow + sReplicaSetIDColumn->column.offset );
        sPrimaryNodeName = (mtdCharType*)((SChar *)sRow + sPrimaryNodeNameColumn->column.offset );
        sFirstBackupNodeName = (mtdCharType*)((SChar *)sRow + sFirstBackupNodeNameColumn->column.offset );
        sSecondBackupNodeName = (mtdCharType*)((SChar *)sRow + sSecondBackupNodeNameColumn->column.offset );
        sStopFirstBackupNodeName = (mtdCharType*)((SChar *)sRow + sStopFirstBackupNodeNameColumn->column.offset );
        sStopSecondBackupNodeName = (mtdCharType*)((SChar *)sRow + sStopSecondBackupNodeNameColumn->column.offset );
        sFirstReplicationName = (mtdCharType*)((SChar *)sRow + sFirstReplicationNameColumn->column.offset );
        sFirstReplFromNodeName = (mtdCharType*)((SChar *)sRow + sFirstReplFromNodeNameColumn->column.offset );
        sFirstReplToNodeName = (mtdCharType*)((SChar *)sRow + sFirstReplToNodeNameColumn->column.offset );
        sSecondReplicaiontName = (mtdCharType*)((SChar *)sRow + sSecondReplicaiontNameColumn->column.offset );
        sSecondReplFromNodeName = (mtdCharType*)((SChar *)sRow + sSecondReplFromNodeNameColumn->column.offset );
        sSecondReplToNodeName = (mtdCharType*)((SChar *)sRow + sSecondReplToNodeNameColumn->column.offset );
        sSMN = *(mtdIntegerType*)((SChar *)sRow + sSMNColumn->column.offset );

        aReplicaSetsInfo->mReplicaSets[sCount].mReplicaSetId= sReplicaSetID;

        idlOS::memcpy( aReplicaSetsInfo->mReplicaSets[sCount].mPrimaryNodeName,
                       sPrimaryNodeName->value,
                       sPrimaryNodeName->length );
        aReplicaSetsInfo->mReplicaSets[sCount].mPrimaryNodeName[sPrimaryNodeName->length] = '\0';

        idlOS::memcpy( aReplicaSetsInfo->mReplicaSets[sCount].mFirstBackupNodeName,
                       sFirstBackupNodeName->value,
                       sFirstBackupNodeName->length );
        aReplicaSetsInfo->mReplicaSets[sCount].mFirstBackupNodeName[sFirstBackupNodeName->length] = '\0';

        idlOS::memcpy( aReplicaSetsInfo->mReplicaSets[sCount].mSecondBackupNodeName,
                       sSecondBackupNodeName->value,
                       sSecondBackupNodeName->length );
        aReplicaSetsInfo->mReplicaSets[sCount].mSecondBackupNodeName[sSecondBackupNodeName->length] = '\0';

        idlOS::memcpy( aReplicaSetsInfo->mReplicaSets[sCount].mFirstReplName,
                       sFirstReplicationName->value,
                       sFirstReplicationName->length );
        aReplicaSetsInfo->mReplicaSets[sCount].mFirstReplName[sFirstReplicationName->length] = '\0';

        idlOS::memcpy( aReplicaSetsInfo->mReplicaSets[sCount].mStopFirstBackupNodeName,
                       sStopFirstBackupNodeName->value,
                       sStopFirstBackupNodeName->length );
        aReplicaSetsInfo->mReplicaSets[sCount].mStopFirstBackupNodeName[sStopFirstBackupNodeName->length] = '\0';

        idlOS::memcpy( aReplicaSetsInfo->mReplicaSets[sCount].mStopSecondBackupNodeName,
                       sStopSecondBackupNodeName->value,
                       sStopSecondBackupNodeName->length );
        aReplicaSetsInfo->mReplicaSets[sCount].mStopSecondBackupNodeName[sStopSecondBackupNodeName->length] = '\0'; 

        idlOS::memcpy( aReplicaSetsInfo->mReplicaSets[sCount].mFirstReplFromNodeName,
                       sFirstReplFromNodeName->value,
                       sFirstReplFromNodeName->length );
        aReplicaSetsInfo->mReplicaSets[sCount].mFirstReplFromNodeName[sFirstReplFromNodeName->length] = '\0';

        idlOS::memcpy( aReplicaSetsInfo->mReplicaSets[sCount].mFirstReplToNodeName,
                       sFirstReplToNodeName->value,
                       sFirstReplToNodeName->length );
        aReplicaSetsInfo->mReplicaSets[sCount].mFirstReplToNodeName[sFirstReplToNodeName->length] = '\0';

        idlOS::memcpy( aReplicaSetsInfo->mReplicaSets[sCount].mSecondReplName,
                       sSecondReplicaiontName->value,
                       sSecondReplicaiontName->length );
        aReplicaSetsInfo->mReplicaSets[sCount].mSecondReplName[sSecondReplicaiontName->length] = '\0';

        idlOS::memcpy( aReplicaSetsInfo->mReplicaSets[sCount].mSecondReplFromNodeName,
                       sSecondReplFromNodeName->value,
                       sSecondReplFromNodeName->length );
        aReplicaSetsInfo->mReplicaSets[sCount].mSecondReplFromNodeName[sSecondReplFromNodeName->length] = '\0';

        idlOS::memcpy( aReplicaSetsInfo->mReplicaSets[sCount].mSecondReplToNodeName,
                       sSecondReplToNodeName->value,
                       sSecondReplToNodeName->length );
        aReplicaSetsInfo->mReplicaSets[sCount].mSecondReplToNodeName[sSecondReplToNodeName->length] = '\0';

        aReplicaSetsInfo->mReplicaSets[sCount].mSMN = sSMN;

        sCount++;

        if ( sCount > SDI_NODE_MAX_COUNT )
        {
            IDE_RAISE( BUFFER_OVERFLOW );
        }
        else
        {
            /* Nothing to do */
        }

        IDE_TEST( sCursor.readRow( &sRow, &sRid, SMI_FIND_NEXT )
                  != IDE_SUCCESS );
    }

    sIsCursorOpen = ID_FALSE;
    IDE_TEST( sCursor.close() != IDE_SUCCESS );

    aReplicaSetsInfo->mCount = sCount;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_META_HANDLE )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDM_SHARD_META_NOT_CREATED,
                                  SDM_FAILOVER_HISTORY_ ) );
    }
    IDE_EXCEPTION( BUFFER_OVERFLOW )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDM_SHARD_NODE_OVERFLOW ) );
    }
    IDE_EXCEPTION_END;

    if ( sIsCursorOpen == ID_TRUE )
    {
        (void)sCursor.close();
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_FAILURE;
}

IDE_RC sdm::getFailoverHistoryWithSMN( smiStatement       * aSmiStmt,
                                       sdiReplicaSetInfo  * aReplicaSetsInfo,
                                       ULong                aSMN )
{
    idBool            sIsCursorOpen = ID_FALSE;
    const void      * sRow          = NULL;
    scGRID            sRid;
    const void      * sFailoverHistory;
    const void      * sFailoverHistoryIdx[SDM_MAX_META_INDICES];
    mtcColumn       * sReplicaSetIDColumn;
    mtcColumn       * sPrimaryNodeNameColumn;
    mtcColumn       * sFirstBackupNodeNameColumn;
    mtcColumn       * sSecondBackupNodeNameColumn;
    mtcColumn       * sStopFirstBackupNodeNameColumn;
    mtcColumn       * sStopSecondBackupNodeNameColumn;
    mtcColumn       * sFirstReplicationNameColumn;
    mtcColumn       * sFirstReplFromNodeNameColumn;
    mtcColumn       * sFirstReplToNodeNameColumn;
    mtcColumn       * sSecondReplicaiontNameColumn;
    mtcColumn       * sSecondReplFromNodeNameColumn;
    mtcColumn       * sSecondReplToNodeNameColumn;
    mtcColumn       * sSMNColumn;

    UShort            sCount = 0;

    mtdIntegerType    sReplicaSetID;
    mtdCharType     * sPrimaryNodeName;
    mtdCharType     * sFirstBackupNodeName;
    mtdCharType     * sSecondBackupNodeName;
    mtdCharType     * sStopFirstBackupNodeName;
    mtdCharType     * sStopSecondBackupNodeName;
    mtdCharType     * sFirstReplicationName;
    mtdCharType     * sFirstReplFromNodeName;
    mtdCharType     * sFirstReplToNodeName;
    mtdCharType     * sSecondReplicaiontName;
    mtdCharType     * sSecondReplFromNodeName;
    mtdCharType     * sSecondReplToNodeName;
    mtdIntegerType    sSMN;

    smiTableCursor       sCursor;
    smiCursorProperties  sCursorProperty;

    qtcMetaRangeColumn sSMNRange;
    smiRange           sRange;

    /* init */
    aReplicaSetsInfo->mCount = 0;

    IDE_TEST( checkMetaVersion( aSmiStmt )
              != IDE_SUCCESS );

    IDE_TEST( getMetaTableAndIndex( aSmiStmt,
                                    SDM_FAILOVER_HISTORY_,
                                    &sFailoverHistory,
                                    sFailoverHistoryIdx )
              != IDE_SUCCESS );

    /* SMN index */
    IDE_TEST_RAISE( sFailoverHistoryIdx[SDM_FAILOVER_HISTORY_IDX3_ORDER] == NULL,
                    ERR_META_HANDLE );

    IDE_TEST( smiGetTableColumns( sFailoverHistory,
                                  SDM_FAILOVER_HISTORY_REPLICA_SET_ID_COL_ORDER,
                                  (const smiColumn**)&sReplicaSetIDColumn )
              != IDE_SUCCESS );
    IDE_TEST( smiGetTableColumns( sFailoverHistory,
                                  SDM_FAILOVER_HISTORY_PRIMARY_NODE_NAME_COL_ORDER,
                                  (const smiColumn**)&sPrimaryNodeNameColumn )
              != IDE_SUCCESS );
    IDE_TEST( smiGetTableColumns( sFailoverHistory,
                                  SDM_FAILOVER_HISTORY_FIRST_BACKUP_NODE_NAME_COL_ORDER,
                                  (const smiColumn**)&sFirstBackupNodeNameColumn )
              != IDE_SUCCESS );
    IDE_TEST( smiGetTableColumns( sFailoverHistory,
                                  SDM_FAILOVER_HISTORY_SECOND_BACKUP_NODE_NAME_COL_ORDER,
                                  (const smiColumn**)&sSecondBackupNodeNameColumn )
              != IDE_SUCCESS );
    IDE_TEST( smiGetTableColumns( sFailoverHistory,
                                  SDM_FAILOVER_HISTORY_STOP_FIRST_BACKUP_NODE_NAME_COL_ORDER,
                                  (const smiColumn**)&sStopFirstBackupNodeNameColumn )
              != IDE_SUCCESS );
    IDE_TEST( smiGetTableColumns( sFailoverHistory,
                                  SDM_FAILOVER_HISTORY_STOP_SECOND_BACKUP_NODE_NAME_COL_ORDER,
                                  (const smiColumn**)&sStopSecondBackupNodeNameColumn )
              != IDE_SUCCESS );
    IDE_TEST( smiGetTableColumns( sFailoverHistory,
                                  SDM_FAILOVER_HISTORY_FIRST_REPLICATION_NAME_COL_ORDER,
                                  (const smiColumn**)&sFirstReplicationNameColumn )
              != IDE_SUCCESS );
    IDE_TEST( smiGetTableColumns( sFailoverHistory,
                                  SDM_FAILOVER_HISTORY_FIRST_REPL_FROM_NODE_NAME_COL_ORDER,
                                  (const smiColumn**)&sFirstReplFromNodeNameColumn )
              != IDE_SUCCESS );
    IDE_TEST( smiGetTableColumns( sFailoverHistory,
                                  SDM_FAILOVER_HISTORY_FIRST_REPL_TO_NODE_NAME_COL_ORDER,
                                  (const smiColumn**)&sFirstReplToNodeNameColumn )
              != IDE_SUCCESS );
    IDE_TEST( smiGetTableColumns( sFailoverHistory,
                                  SDM_FAILOVER_HISTORY_SECOND_REPLICATION_NAME_COL_ORDER,
                                  (const smiColumn**)&sSecondReplicaiontNameColumn )
              != IDE_SUCCESS );
    IDE_TEST( smiGetTableColumns( sFailoverHistory,
                                  SDM_FAILOVER_HISTORY_SECOND_REPL_FROM_NODE_NAME_COL_ORDER,
                                  (const smiColumn**)&sSecondReplFromNodeNameColumn )
              != IDE_SUCCESS );
    IDE_TEST( smiGetTableColumns( sFailoverHistory,
                                  SDM_FAILOVER_HISTORY_SECOND_REPL_TO_NODE_NAME_COL_ORDER,
                                  (const smiColumn**)&sSecondReplToNodeNameColumn )
              != IDE_SUCCESS );

    IDE_TEST( smiGetTableColumns( sFailoverHistory,
                                  SDM_FAILOVER_HISTORY_SMN_COL_ORDER,
                                  (const smiColumn**)&sSMNColumn )
              != IDE_SUCCESS );

    // mtdModule 설정
    IDE_TEST( mtd::moduleById( &(sSMNColumn->module),
                               sSMNColumn->type.dataTypeId )
              != IDE_SUCCESS );

    // mtlModule 설정
    IDE_TEST( mtl::moduleById( &sSMNColumn->language,
                               sSMNColumn->type.languageId )
              != IDE_SUCCESS );

    qciMisc::makeMetaRangeSingleColumn(
        &sSMNRange,
        (const mtcColumn *) sSMNColumn,
        (const void *) &aSMN,
        &sRange );

    sCursor.initialize();

    /* PROJ-2622 */
    SMI_CURSOR_PROP_INIT_FOR_META_INDEX_SCAN( &sCursorProperty, NULL );

    IDE_TEST( sCursor.open(
                  aSmiStmt,
                  sFailoverHistory,
                  sFailoverHistoryIdx[SDM_FAILOVER_HISTORY_IDX3_ORDER],
                  smiGetRowSCN( sFailoverHistory ),
                  NULL,
                  &sRange,
                  smiGetDefaultKeyRange(),
                  smiGetDefaultFilter(),
                  QCM_META_CURSOR_FLAG,
                  SMI_SELECT_CURSOR,
                  &sCursorProperty )
              != IDE_SUCCESS );

    sIsCursorOpen = ID_TRUE;

    IDE_TEST( sCursor.beforeFirst() != IDE_SUCCESS );

    IDE_TEST( sCursor.readRow( &sRow, &sRid, SMI_FIND_NEXT )
              != IDE_SUCCESS );

    while ( sRow != NULL )
    {
        sReplicaSetID = *(mtdIntegerType*)((SChar *)sRow + sReplicaSetIDColumn->column.offset );
        sPrimaryNodeName = (mtdCharType*)((SChar *)sRow + sPrimaryNodeNameColumn->column.offset );
        sFirstBackupNodeName = (mtdCharType*)((SChar *)sRow + sFirstBackupNodeNameColumn->column.offset );
        sSecondBackupNodeName = (mtdCharType*)((SChar *)sRow + sSecondBackupNodeNameColumn->column.offset );
        sStopFirstBackupNodeName = (mtdCharType*)((SChar *)sRow + sStopFirstBackupNodeNameColumn->column.offset );
        sStopSecondBackupNodeName = (mtdCharType*)((SChar *)sRow + sStopSecondBackupNodeNameColumn->column.offset );
        sFirstReplicationName = (mtdCharType*)((SChar *)sRow + sFirstReplicationNameColumn->column.offset );
        sFirstReplFromNodeName = (mtdCharType*)((SChar *)sRow + sFirstReplFromNodeNameColumn->column.offset );
        sFirstReplToNodeName = (mtdCharType*)((SChar *)sRow + sFirstReplToNodeNameColumn->column.offset );
        sSecondReplicaiontName = (mtdCharType*)((SChar *)sRow + sSecondReplicaiontNameColumn->column.offset );
        sSecondReplFromNodeName = (mtdCharType*)((SChar *)sRow + sSecondReplFromNodeNameColumn->column.offset );
        sSecondReplToNodeName = (mtdCharType*)((SChar *)sRow + sSecondReplToNodeNameColumn->column.offset );
        sSMN = *(mtdIntegerType*)((SChar *)sRow + sSMNColumn->column.offset );

        aReplicaSetsInfo->mReplicaSets[sCount].mReplicaSetId= sReplicaSetID;

        idlOS::memcpy( aReplicaSetsInfo->mReplicaSets[sCount].mPrimaryNodeName,
                       sPrimaryNodeName->value,
                       sPrimaryNodeName->length );
        aReplicaSetsInfo->mReplicaSets[sCount].mPrimaryNodeName[sPrimaryNodeName->length] = '\0';

        idlOS::memcpy( aReplicaSetsInfo->mReplicaSets[sCount].mFirstBackupNodeName,
                       sFirstBackupNodeName->value,
                       sFirstBackupNodeName->length );
        aReplicaSetsInfo->mReplicaSets[sCount].mFirstBackupNodeName[sFirstBackupNodeName->length] = '\0';

        idlOS::memcpy( aReplicaSetsInfo->mReplicaSets[sCount].mSecondBackupNodeName,
                       sSecondBackupNodeName->value,
                       sSecondBackupNodeName->length );
        aReplicaSetsInfo->mReplicaSets[sCount].mSecondBackupNodeName[sSecondBackupNodeName->length] = '\0';

        idlOS::memcpy( aReplicaSetsInfo->mReplicaSets[sCount].mFirstReplName,
                       sFirstReplicationName->value,
                       sFirstReplicationName->length );
        aReplicaSetsInfo->mReplicaSets[sCount].mFirstReplName[sFirstReplicationName->length] = '\0';

        idlOS::memcpy( aReplicaSetsInfo->mReplicaSets[sCount].mStopFirstBackupNodeName,
                       sStopFirstBackupNodeName->value,
                       sStopFirstBackupNodeName->length );
        aReplicaSetsInfo->mReplicaSets[sCount].mStopFirstBackupNodeName[sStopFirstBackupNodeName->length] = '\0';

        idlOS::memcpy( aReplicaSetsInfo->mReplicaSets[sCount].mStopSecondBackupNodeName,
                       sStopSecondBackupNodeName->value,
                       sStopSecondBackupNodeName->length );
        aReplicaSetsInfo->mReplicaSets[sCount].mStopSecondBackupNodeName[sStopSecondBackupNodeName->length] = '\0'; 

        idlOS::memcpy( aReplicaSetsInfo->mReplicaSets[sCount].mFirstReplFromNodeName,
                       sFirstReplFromNodeName->value,
                       sFirstReplFromNodeName->length );
        aReplicaSetsInfo->mReplicaSets[sCount].mFirstReplFromNodeName[sFirstReplFromNodeName->length] = '\0';

        idlOS::memcpy( aReplicaSetsInfo->mReplicaSets[sCount].mFirstReplToNodeName,
                       sFirstReplToNodeName->value,
                       sFirstReplToNodeName->length );
        aReplicaSetsInfo->mReplicaSets[sCount].mFirstReplToNodeName[sFirstReplToNodeName->length] = '\0';

        idlOS::memcpy( aReplicaSetsInfo->mReplicaSets[sCount].mSecondReplName,
                       sSecondReplicaiontName->value,
                       sSecondReplicaiontName->length );
        aReplicaSetsInfo->mReplicaSets[sCount].mSecondReplName[sSecondReplicaiontName->length] = '\0';

        idlOS::memcpy( aReplicaSetsInfo->mReplicaSets[sCount].mSecondReplFromNodeName,
                       sSecondReplFromNodeName->value,
                       sSecondReplFromNodeName->length );
        aReplicaSetsInfo->mReplicaSets[sCount].mSecondReplFromNodeName[sSecondReplFromNodeName->length] = '\0';

        idlOS::memcpy( aReplicaSetsInfo->mReplicaSets[sCount].mSecondReplToNodeName,
                       sSecondReplToNodeName->value,
                       sSecondReplToNodeName->length );
        aReplicaSetsInfo->mReplicaSets[sCount].mSecondReplToNodeName[sSecondReplToNodeName->length] = '\0';

        aReplicaSetsInfo->mReplicaSets[sCount].mSMN = sSMN;

        sCount++;

        if ( sCount > SDI_NODE_MAX_COUNT )
        {
            IDE_RAISE( BUFFER_OVERFLOW );
        }
        else
        {
            /* Nothing to do */
        }

        IDE_TEST( sCursor.readRow( &sRow, &sRid, SMI_FIND_NEXT )
                  != IDE_SUCCESS );
    }

    sIsCursorOpen = ID_FALSE;
    IDE_TEST( sCursor.close() != IDE_SUCCESS );

    aReplicaSetsInfo->mCount = sCount;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_META_HANDLE )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDM_SHARD_META_NOT_CREATED,
                                  SDM_FAILOVER_HISTORY_ ) );
    }
    IDE_EXCEPTION( BUFFER_OVERFLOW )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDM_SHARD_NODE_OVERFLOW ) );
    }
    IDE_EXCEPTION_END;

    if ( sIsCursorOpen == ID_TRUE )
    {
        (void)sCursor.close();
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_FAILURE;
}

/* FailoverHistory에 SMN 값 보다 큰 Row가 존재하는지 확인한다. */
IDE_RC sdm::checkFailoverHistoryOverSMN( smiStatement       * aSmiStmt,
                                         ULong                aSMN,
                                         idBool             * aIsExist )
{
    idBool            sIsCursorOpen = ID_FALSE;
    const void      * sRow          = NULL;
    scGRID            sRid;
    const void      * sFailoverHistory;
    const void      * sFailoverHistoryIdx[SDM_MAX_META_INDICES];
/*    mtcColumn       * sReplicaSetIDColumn;
    mtcColumn       * sPrimaryNodeNameColumn;
    mtcColumn       * sFirstBackupNodeNameColumn;
    mtcColumn       * sSecondBackupNodeNameColumn;
    mtcColumn       * sStopFirstBackupNodeNameColumn;
    mtcColumn       * sStopSecondBackupNodeNameColumn;
    mtcColumn       * sFirstReplicationNameColumn;
    mtcColumn       * sFirstReplFromNodeNameColumn;
    mtcColumn       * sFirstReplToNodeNameColumn;
    mtcColumn       * sSecondReplicaiontNameColumn;
    mtcColumn       * sSecondReplFromNodeNameColumn;
    mtcColumn       * sSecondReplToNodeNameColumn;*/
    mtcColumn       * sSMNColumn;

/*    mtdIntegerType    sReplicaSetID;
    mtdCharType     * sPrimaryNodeName;
    mtdCharType     * sFirstBackupNodeName;
    mtdCharType     * sSecondBackupNodeName;
    mtdCharType     * sStopFirstBackupNodeName;
    mtdCharType     * sStopSecondBackupNodeName;
    mtdCharType     * sFirstReplicationName;
    mtdCharType     * sFirstReplFromNodeName;
    mtdCharType     * sFirstReplToNodeName;
    mtdCharType     * sSecondReplicaiontName;
    mtdCharType     * sSecondReplFromNodeName;
    mtdCharType     * sSecondReplToNodeName;*/
    mtdIntegerType    sSMN;

    smiTableCursor       sCursor;
    smiCursorProperties  sCursorProperty;

    idBool            sIsExist = ID_FALSE;

    IDE_TEST( checkMetaVersion( aSmiStmt )
              != IDE_SUCCESS );

    IDE_TEST( getMetaTableAndIndex( aSmiStmt,
                                    SDM_FAILOVER_HISTORY_,
                                    &sFailoverHistory,
                                    sFailoverHistoryIdx )
              != IDE_SUCCESS );

    /* SMN index */

/*    IDE_TEST( smiGetTableColumns( sFailoverHistory,
                                  SDM_FAILOVER_HISTORY_REPLICA_SET_ID_COL_ORDER,
                                  (const smiColumn**)&sReplicaSetIDColumn )
              != IDE_SUCCESS );
    IDE_TEST( smiGetTableColumns( sFailoverHistory,
                                  SDM_FAILOVER_HISTORY_PRIMARY_NODE_NAME_COL_ORDER,
                                  (const smiColumn**)&sPrimaryNodeNameColumn )
              != IDE_SUCCESS );
    IDE_TEST( smiGetTableColumns( sFailoverHistory,
                                  SDM_FAILOVER_HISTORY_FIRST_BACKUP_NODE_NAME_COL_ORDER,
                                  (const smiColumn**)&sFirstBackupNodeNameColumn )
              != IDE_SUCCESS );
    IDE_TEST( smiGetTableColumns( sFailoverHistory,
                                  SDM_FAILOVER_HISTORY_SECOND_BACKUP_NODE_NAME_COL_ORDER,
                                  (const smiColumn**)&sSecondBackupNodeNameColumn )
              != IDE_SUCCESS );
    IDE_TEST( smiGetTableColumns( sFailoverHistory,
                                  SDM_FAILOVER_HISTORY_STOP_FIRST_BACKUP_NODE_NAME_COL_ORDER,
                                  (const smiColumn**)&sStopFirstBackupNodeNameColumn )
              != IDE_SUCCESS );
    IDE_TEST( smiGetTableColumns( sFailoverHistory,
                                  SDM_FAILOVER_HISTORY_STOP_SECOND_BACKUP_NODE_NAME_COL_ORDER,
                                  (const smiColumn**)&sStopSecondBackupNodeNameColumn )
              != IDE_SUCCESS );
    IDE_TEST( smiGetTableColumns( sFailoverHistory,
                                  SDM_FAILOVER_HISTORY_FIRST_REPLICATION_NAME_COL_ORDER,
                                  (const smiColumn**)&sFirstReplicationNameColumn )
              != IDE_SUCCESS );
    IDE_TEST( smiGetTableColumns( sFailoverHistory,
                                  SDM_FAILOVER_HISTORY_FIRST_REPL_FROM_NODE_NAME_COL_ORDER,
                                  (const smiColumn**)&sFirstReplFromNodeNameColumn )
              != IDE_SUCCESS );
    IDE_TEST( smiGetTableColumns( sFailoverHistory,
                                  SDM_FAILOVER_HISTORY_FIRST_REPL_TO_NODE_NAME_COL_ORDER,
                                  (const smiColumn**)&sFirstReplToNodeNameColumn )
              != IDE_SUCCESS );
    IDE_TEST( smiGetTableColumns( sFailoverHistory,
                                  SDM_FAILOVER_HISTORY_SECOND_REPLICATION_NAME_COL_ORDER,
                                  (const smiColumn**)&sSecondReplicaiontNameColumn )
              != IDE_SUCCESS );
    IDE_TEST( smiGetTableColumns( sFailoverHistory,
                                  SDM_FAILOVER_HISTORY_SECOND_REPL_FROM_NODE_NAME_COL_ORDER,
                                  (const smiColumn**)&sSecondReplFromNodeNameColumn )
              != IDE_SUCCESS );
    IDE_TEST( smiGetTableColumns( sFailoverHistory,
                                  SDM_FAILOVER_HISTORY_SECOND_REPL_TO_NODE_NAME_COL_ORDER,
                                  (const smiColumn**)&sSecondReplToNodeNameColumn )
              != IDE_SUCCESS );
*/
    IDE_TEST( smiGetTableColumns( sFailoverHistory,
                                  SDM_FAILOVER_HISTORY_SMN_COL_ORDER,
                                  (const smiColumn**)&sSMNColumn )
              != IDE_SUCCESS );

    // mtdModule 설정
    IDE_TEST( mtd::moduleById( &(sSMNColumn->module),
                               sSMNColumn->type.dataTypeId )
              != IDE_SUCCESS );

    // mtlModule 설정
    IDE_TEST( mtl::moduleById( &sSMNColumn->language,
                               sSMNColumn->type.languageId )
              != IDE_SUCCESS );

    sCursor.initialize();

    /* PROJ-2622 */
    SMI_CURSOR_PROP_INIT_FOR_META_FULL_SCAN( &sCursorProperty, NULL );

    IDE_TEST( sCursor.open(
                  aSmiStmt,
                  sFailoverHistory,
                  NULL,
                  smiGetRowSCN( sFailoverHistory ),
                  NULL,
                  smiGetDefaultKeyRange(),
                  smiGetDefaultKeyRange(),
                  smiGetDefaultFilter(),
                  QCM_META_CURSOR_FLAG,
                  SMI_SELECT_CURSOR,
                  &sCursorProperty )
              != IDE_SUCCESS );

    sIsCursorOpen = ID_TRUE;

    IDE_TEST( sCursor.beforeFirst() != IDE_SUCCESS );

    IDE_TEST( sCursor.readRow( &sRow, &sRid, SMI_FIND_NEXT )
              != IDE_SUCCESS );

    while ( sRow != NULL )
    {
/*        sReplicaSetID = *(mtdIntegerType*)((SChar *)sRow + sReplicaSetIDColumn->column.offset );
        sPrimaryNodeName = (mtdCharType*)((SChar *)sRow + sPrimaryNodeNameColumn->column.offset );
        sFirstBackupNodeName = (mtdCharType*)((SChar *)sRow + sFirstBackupNodeNameColumn->column.offset );
        sSecondBackupNodeName = (mtdCharType*)((SChar *)sRow + sSecondBackupNodeNameColumn->column.offset );
        sStopFirstBackupNodeName = (mtdCharType*)((SChar *)sRow + sStopFirstBackupNodeNameColumn->column.offset );
        sStopSecondBackupNodeName = (mtdCharType*)((SChar *)sRow + sStopSecondBackupNodeNameColumn->column.offset );
        sFirstReplicationName = (mtdCharType*)((SChar *)sRow + sFirstReplicationNameColumn->column.offset );
        sFirstReplFromNodeName = (mtdCharType*)((SChar *)sRow + sFirstReplFromNodeNameColumn->column.offset );
        sFirstReplToNodeName = (mtdCharType*)((SChar *)sRow + sFirstReplToNodeNameColumn->column.offset );
        sSecondReplicaiontName = (mtdCharType*)((SChar *)sRow + sSecondReplicaiontNameColumn->column.offset );
        sSecondReplFromNodeName = (mtdCharType*)((SChar *)sRow + sSecondReplFromNodeNameColumn->column.offset );
        sSecondReplToNodeName = (mtdCharType*)((SChar *)sRow + sSecondReplToNodeNameColumn->column.offset );*/
        sSMN = *(mtdIntegerType*)((SChar *)sRow + sSMNColumn->column.offset );

        if ( (ULong)sSMN > aSMN )
        {
            sIsExist = ID_TRUE;
            break;
        }

        IDE_TEST( sCursor.readRow( &sRow, &sRid, SMI_FIND_NEXT )
                  != IDE_SUCCESS );
    }

    sIsCursorOpen = ID_FALSE;
    IDE_TEST( sCursor.close() != IDE_SUCCESS );

    *aIsExist = sIsExist;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sIsCursorOpen == ID_TRUE )
    {
        (void)sCursor.close();
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_FAILURE;
}


/* PROJ-2748 Shard Failback */
/* aNodeName에서 해당 ReplicaSet의 Failback을 수행해야하는지 확인
 * [IN] aReplicaSetInfo : 전체 ReplicaSet 정보 PName 정렬됨
 * [IN] aReplicaSetId   : Failback 대상 Partition에 연결되어 있던 ReplicaSetId
 * [IN] aNodeName       : Failback를 수행하는 Node의 NodeName  */
idBool sdm::checkFailbackAvailable( sdiReplicaSetInfo * aReplicaSetInfo,
                                    UInt                aReplicaSetId,
                                    SChar             * aNodeName )
{
    UInt                i = 0;
    idBool              sNeedFailback = ID_FALSE;
    sdiReplicaSet     * sTargetReplicaSet = NULL;


    /* find matched ReplicaSet */
    for ( i= 0; i < aReplicaSetInfo->mCount; i++ )
    {
        if ( aReplicaSetInfo->mReplicaSets[i].mReplicaSetId == aReplicaSetId )
        {
            sTargetReplicaSet = &aReplicaSetInfo->mReplicaSets[i];
            break;
        }
    }

    /* Target ReplicaSet은 항상 있어야 한다. */
    IDE_DASSERT( sTargetReplicaSet != NULL );

    /* check can failback */
    if ( idlOS::strncmp( sTargetReplicaSet->mPrimaryNodeName,
                         aNodeName,
                         SDI_NODE_NAME_MAX_SIZE + 1 ) != 0 )
    {
        sNeedFailback = ID_TRUE;
    }

    return sNeedFailback;
}

IDE_RC sdm::updateReplicaSet4Failback( qcStatement        * aStatement,
                                       SChar              * aOldNodeName,
                                       SChar              * aNewNodeName )
{
    SChar             * sSqlStr;
    vSLong              sRowCnt;

    sdiReplicaSetInfo   sFailoverHistoryInfo;

    sdiReplicaSet     * sReplicaSet = NULL;
    UInt                i = 0;
    ULong               sFailbackSMN = 0;

    smiStatement      * sOldStmt = NULL;
    smiStatement        sSmiStmt;
    UInt                sSmiStmtFlag;
    SInt                sState = 0;

    SChar               sRecentDeadNode[SDI_NODE_NAME_MAX_SIZE + 1] = {0,};

    sdiGlobalMetaInfo   sMetaNodeInfo = { ID_ULONG(0) };

    idBool              sIsAlive = ID_FALSE;

    sSmiStmtFlag = SMI_STATEMENT_NORMAL | SMI_STATEMENT_MEMORY_CURSOR;
    sOldStmt                = QC_SMI_STMT(aStatement);
    QC_SMI_STMT(aStatement) = &sSmiStmt;
    sState = 1;

    IDE_TEST( sSmiStmt.begin( aStatement->mStatistics,
                              sOldStmt,
                              sSmiStmtFlag )
              != IDE_SUCCESS );
    sState = 2;

    IDE_TEST( STRUCT_ALLOC_WITH_SIZE( aStatement->qmxMem,
                                      SChar,
                                      QD_MAX_SQL_LENGTH,
                                      &sSqlStr )
              != IDE_SUCCESS);

    IDE_TEST( checkMetaVersion( QC_SMI_STMT( aStatement ) )
              != IDE_SUCCESS );

    IDE_TEST( makeShardMeta4NewSMN(aStatement) != IDE_SUCCESS );

    IDE_TEST( getGlobalMetaInfoCore( QC_SMI_STMT(aStatement),
                                     &sMetaNodeInfo ) != IDE_SUCCESS );

    /* 여기까지 왔으면 RecentDeadNode는 내가 맞으니까 확인할 필요는 없고 FailbackSMN 만 필요하다. */
    IDE_TEST( sdiZookeeper::checkRecentDeadNode( sRecentDeadNode, &sFailbackSMN, NULL ) != IDE_SUCCESS );

    /* get FailoverHistory */
    IDE_TEST( sdm::getFailoverHistoryWithSMN( QC_SMI_STMT( aStatement),
                                              &sFailoverHistoryInfo,
                                              sFailbackSMN ) != IDE_SUCCESS );

    /* FailoverHistory에 값이 없을 수도 있다. K-safety = 0 등 */

    /* 여러 ReplicaSet이 한 Node에 Bind 되어 있을수 있다. */
    for ( i = 0 ; i < sFailoverHistoryInfo.mCount; i++ )
    {
        /* Find Old Node Primary ReplicaSet */
        if ( idlOS::strncmp( sFailoverHistoryInfo.mReplicaSets[i].mPrimaryNodeName,
                             aNewNodeName,
                             SDI_NODE_NAME_MAX_SIZE + 1 ) == 0 )
        {
            sReplicaSet = &sFailoverHistoryInfo.mReplicaSets[i];

            /* PRIMARY UPDATE */
            /* TargetNode를 Failover 가능하면 First/Second 를 맞춰서 NA로 변경하고 Primary를
             * FailoverNode로 변경한다. */
            if ( idlOS::strncmp( sReplicaSet->mFirstBackupNodeName,
                                 aOldNodeName,
                                 SDI_NODE_NAME_MAX_SIZE + 1 ) == 0 )
            {
                idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                                 "UPDATE SYS_SHARD.REPLICA_SETS_ "
                                 "   SET PRIMARY_NODE_NAME        = "QCM_SQL_VARCHAR_FMT", "
                                 "   FIRST_BACKUP_NODE_NAME       = "QCM_SQL_VARCHAR_FMT", "
                                 "   STOP_FIRST_BACKUP_NODE_NAME  = "QCM_SQL_VARCHAR_FMT
                                 "   WHERE REPLICA_SET_ID         = "QCM_SQL_UINT32_FMT
                                 "         AND SMN                = "QCM_SQL_BIGINT_FMT,
                                 aNewNodeName,
                                 aOldNodeName,
                                 SDM_NA_STR,
                                 sReplicaSet->mReplicaSetId,
                                 sMetaNodeInfo.mShardMetaNumber );

                IDE_TEST( qciMisc::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                                 sSqlStr,
                                                 & sRowCnt )
                          != IDE_SUCCESS );
                /* Primary의 변경은 RP Stop이 없다. */
            }
            else if ( idlOS::strncmp( sReplicaSet->mSecondBackupNodeName,
                                      aOldNodeName,
                                      SDI_NODE_NAME_MAX_SIZE + 1 ) == 0 )
            {
                idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                                 "UPDATE SYS_SHARD.REPLICA_SETS_ "
                                 "   SET PRIMARY_NODE_NAME         = "QCM_SQL_VARCHAR_FMT", "
                                 "   SECOND_BACKUP_NODE_NAME       = "QCM_SQL_VARCHAR_FMT", "
                                 "   STOP_SECOND_BACKUP_NODE_NAME  = "QCM_SQL_VARCHAR_FMT
                                 "   WHERE REPLICA_SET_ID      = "QCM_SQL_UINT32_FMT
                                 "         AND SMN                 = "QCM_SQL_BIGINT_FMT,
                                 aNewNodeName,
                                 aOldNodeName,
                                 SDM_NA_STR,
                                 sReplicaSet->mReplicaSetId,
                                 sMetaNodeInfo.mShardMetaNumber );

                IDE_TEST( qciMisc::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                                 sSqlStr,
                                                 & sRowCnt )
                          != IDE_SUCCESS );
                /* Primary의 변경은 RP Stop이 없다. */
            }
            else
            {
                /* Normal Failback/Failback Sync를 구별할수 있으면 좋은데 ... 아직 없음.
                 * 일단 위에도 FailoverHistory와 같은 값으로 변경하는 것이기 때문에 
                 * 확인 하나만 더 하는 것일 뿐 내용은 같다 */
                /* DROP FORCE로 인해 FailoverHistory가 수정된 이후의  FAILBACK 일 경우 
                 * aFailbackFromNode 와 FailoverHistory에 기록된 FromNode 값이 일치하지 않는다.
                 * FailoverHistory에 기록된 정보로 되돌려주어야 한다. */
                idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                                 "UPDATE SYS_SHARD.REPLICA_SETS_ "
                                 "   SET PRIMARY_NODE_NAME         = "QCM_SQL_VARCHAR_FMT", "
                                 "   FIRST_BACKUP_NODE_NAME        = "QCM_SQL_VARCHAR_FMT", "
                                 "   STOP_FIRST_BACKUP_NODE_NAME   = "QCM_SQL_VARCHAR_FMT", "
                                 "   SECOND_BACKUP_NODE_NAME       = "QCM_SQL_VARCHAR_FMT", "
                                 "   STOP_SECOND_BACKUP_NODE_NAME  = "QCM_SQL_VARCHAR_FMT
                                 "   WHERE REPLICA_SET_ID          = "QCM_SQL_UINT32_FMT
                                 "         AND SMN                 = "QCM_SQL_BIGINT_FMT,
                                 sReplicaSet->mPrimaryNodeName,
                                 sReplicaSet->mFirstBackupNodeName,
                                 sReplicaSet->mStopFirstBackupNodeName,
                                 sReplicaSet->mSecondBackupNodeName,
                                 sReplicaSet->mStopSecondBackupNodeName,
                                 sReplicaSet->mReplicaSetId,
                                 sMetaNodeInfo.mShardMetaNumber );

                IDE_TEST( qciMisc::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                                 sSqlStr,
                                                 & sRowCnt )
                          != IDE_SUCCESS );
            }
        }
    }

    for ( i = 0 ; i < sFailoverHistoryInfo.mCount; i++ )
    {
        /* Find First To OldNode ReplicaSet */
        if ( idlOS::strncmp( sFailoverHistoryInfo.mReplicaSets[i].mFirstBackupNodeName,
                             aNewNodeName,
                             SDI_NODE_NAME_MAX_SIZE + 1 ) == 0 )
        {
            /* R2HA 하나 밖에 없어야 정상인데 ... check 해야 하나? */
            sReplicaSet = &sFailoverHistoryInfo.mReplicaSets[i];

            IDE_TEST( sdiZookeeper::checkNodeAlive( sReplicaSet->mPrimaryNodeName,
                                                    &sIsAlive ) != IDE_SUCCESS );

            if ( sIsAlive == ID_TRUE )
            {
                /* FIRST UPDATE */
                /* First To TargetNode 중인 ReplicaSet의 First 를 NA로 변경한다. */
                idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                                 "UPDATE SYS_SHARD.REPLICA_SETS_ "
                                 "   SET FIRST_BACKUP_NODE_NAME   = "QCM_SQL_VARCHAR_FMT", "
                                 "   STOP_FIRST_BACKUP_NODE_NAME  = "QCM_SQL_VARCHAR_FMT
                                 "   WHERE REPLICA_SET_ID           = "QCM_SQL_UINT32_FMT
                                 "   AND SMN                      = "QCM_SQL_BIGINT_FMT,
                                 aNewNodeName,
                                 SDM_NA_STR,
                                 sReplicaSet->mReplicaSetId,
                                 sMetaNodeInfo.mShardMetaNumber );

                IDE_TEST( qciMisc::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                                 sSqlStr,
                                                 & sRowCnt )
                          != IDE_SUCCESS );

                /* To OldNode는 RP Stop이 필요하다. */
            }
        }

        /* Find Second To OldNode ReplicaSet */
        if ( idlOS::strncmp( sFailoverHistoryInfo.mReplicaSets[i].mSecondBackupNodeName,
                             aNewNodeName,
                             SDI_NODE_NAME_MAX_SIZE + 1 ) == 0 )
        {
            /* R2HA 하나 밖에 없어야 정상인데 ... check 해야 하나? */
            sReplicaSet = &sFailoverHistoryInfo.mReplicaSets[i];

            IDE_TEST( sdiZookeeper::checkNodeAlive( sReplicaSet->mPrimaryNodeName,
                                                    &sIsAlive ) != IDE_SUCCESS );

            if ( sIsAlive == ID_TRUE )
            {
                /* SECOND UPDATE */
                /* Second To TargetNode 중인 ReplicaSet의 Second 를 NA로 변경한다. */
                idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                                 "UPDATE SYS_SHARD.REPLICA_SETS_ "
                                 "   SET SECOND_BACKUP_NODE_NAME   = "QCM_SQL_VARCHAR_FMT", "
                                 "   STOP_SECOND_BACKUP_NODE_NAME  = "QCM_SQL_VARCHAR_FMT
                                 "   WHERE REPLICA_SET_ID            = "QCM_SQL_UINT32_FMT
                                 "   AND SMN                       = "QCM_SQL_BIGINT_FMT,
                                 aNewNodeName,
                                 SDM_NA_STR,
                                 sReplicaSet->mReplicaSetId,
                                 sMetaNodeInfo.mShardMetaNumber );

                IDE_TEST( qciMisc::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                                 sSqlStr,
                                                 & sRowCnt )
                          != IDE_SUCCESS );
                /* To OldNode는 RP Stop이 필요하다. */
            }
        }
    }

    sState = 1;
    IDE_TEST( sSmiStmt.end( SMI_STATEMENT_RESULT_SUCCESS ) != IDE_SUCCESS );

    sState = 0;
    QC_SMI_STMT(aStatement) = sOldStmt;

    sdi::setShardMetaTouched( aStatement->session );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

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

    return IDE_FAILURE;
}

IDE_RC sdm::updateReplicaSet4DropForce( qcStatement        * aStatement,
                                        SChar              * /*aOldNodeName*/,
                                        SChar              * aNewNodeName )
{
    SChar             * sSqlStr;
    vSLong              sRowCnt;

    sdiReplicaSetInfo   sFailoverHistoryInfo;

    ULong               sFailbackSMN = 0;

    smiStatement      * sOldStmt = NULL;
    smiStatement        sSmiStmt;
    UInt                sSmiStmtFlag;
    SInt                sState = 0;

    SChar               sRecentDeadNode[SDI_NODE_NAME_MAX_SIZE + 1] = {0,};

    SChar               sMyNextNodeName[SDI_NODE_NAME_MAX_SIZE + 1] = {0,};

    ZKState             sZKState = ZK_NONE;

    sdiGlobalMetaInfo   sMetaNodeInfo = { ID_ULONG(0) };

    sdiReplicaSetInfo   sReplicaSetInfo;

    SChar               sFailoverToNodeName[SDI_NODE_NAME_MAX_SIZE + 1];
    SInt                sDataLength;
    SChar               sBuffer[SDI_ZKC_BUFFER_SIZE] = {0,};

    sSmiStmtFlag = SMI_STATEMENT_NORMAL | SMI_STATEMENT_MEMORY_CURSOR;
    sOldStmt                = QC_SMI_STMT(aStatement);
    QC_SMI_STMT(aStatement) = &sSmiStmt;
    sState = 1;

    IDE_TEST( sSmiStmt.begin( aStatement->mStatistics,
                              sOldStmt,
                              sSmiStmtFlag )
              != IDE_SUCCESS );
    sState = 2;

    IDE_TEST( STRUCT_ALLOC_WITH_SIZE( aStatement->qmxMem,
                                      SChar,
                                      QD_MAX_SQL_LENGTH,
                                      &sSqlStr )
              != IDE_SUCCESS);

    IDE_TEST( checkMetaVersion( QC_SMI_STMT( aStatement ) )
              != IDE_SUCCESS );

    IDE_TEST( makeShardMeta4NewSMN(aStatement) != IDE_SUCCESS );

    IDE_TEST( getGlobalMetaInfoCore( QC_SMI_STMT(aStatement),
                                     &sMetaNodeInfo ) != IDE_SUCCESS );

    /* 여기까지 왔으면 RecentDeadNode는 내가 맞으니까 확인할 필요는 없고 FailbackSMN 만 필요하다. */
    IDE_TEST( sdiZookeeper::checkRecentDeadNode( sRecentDeadNode, &sFailbackSMN, NULL ) != IDE_SUCCESS );

    sDataLength = SDI_ZKC_BUFFER_SIZE;
    IDE_TEST( sdiZookeeper::getNodeInfo( aNewNodeName,
                                         (SChar*)SDI_ZKC_PATH_NODE_FAILOVER_TO,
                                         sBuffer,
                                         &sDataLength ) != IDE_SUCCESS );

    IDE_TEST_RAISE( sDataLength <= 0 , Not_failed_over );

    idlOS::strncpy( sFailoverToNodeName,
                    sBuffer,
                    SDI_NODE_NAME_MAX_SIZE + 1 );

    IDE_TEST( sdm::getAllReplicaSetInfoSortedByPName( QC_SMI_STMT( aStatement ),
                                                      &sReplicaSetInfo,
                                                      sMetaNodeInfo.mShardMetaNumber )
              != IDE_SUCCESS );

    /* get FailoverHistory */
    IDE_TEST( sdm::getFailoverHistoryWithSMN( QC_SMI_STMT( aStatement),
                                              &sFailoverHistoryInfo,
                                              sFailbackSMN ) != IDE_SUCCESS );

    /* Find Target's MainReplicaSet ID */
    IDE_TEST( sdiZookeeper::getNextNode( aNewNodeName, 
                                         sMyNextNodeName, 
                                         &sZKState ) != IDE_SUCCESS );

    /* DROP FORCE에 의해 제거되는 Node 정보는 Failover 여부와 상관없이 
     * 모든 ReplicaSet 을 대상으로 이루어 져야 한다. */
    idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                     "UPDATE SYS_SHARD.REPLICA_SETS_ "
                     "   SET FIRST_BACKUP_NODE_NAME   = "QCM_SQL_VARCHAR_FMT
                     "   WHERE FIRST_BACKUP_NODE_NAME = "QCM_SQL_VARCHAR_FMT
                     "         AND SMN                = "QCM_SQL_BIGINT_FMT,
                     SDM_NA_STR,
                     aNewNodeName,
                     sMetaNodeInfo.mShardMetaNumber );

    IDE_TEST( qciMisc::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                     sSqlStr,
                                     & sRowCnt )
              != IDE_SUCCESS );

    idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                     "UPDATE SYS_SHARD.REPLICA_SETS_ "
                     "   SET STOP_FIRST_BACKUP_NODE_NAME   = "QCM_SQL_VARCHAR_FMT
                     "   WHERE STOP_FIRST_BACKUP_NODE_NAME = "QCM_SQL_VARCHAR_FMT
                     "         AND SMN                     = "QCM_SQL_BIGINT_FMT,
                     SDM_NA_STR,
                     aNewNodeName,
                     sMetaNodeInfo.mShardMetaNumber );

    IDE_TEST( qciMisc::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                     sSqlStr,
                                     & sRowCnt )
              != IDE_SUCCESS );

    idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                     "UPDATE SYS_SHARD.REPLICA_SETS_ "
                     "   SET SECOND_BACKUP_NODE_NAME   = "QCM_SQL_VARCHAR_FMT
                     "   WHERE SECOND_BACKUP_NODE_NAME = "QCM_SQL_VARCHAR_FMT
                     "         AND SMN                = "QCM_SQL_BIGINT_FMT,
                     SDM_NA_STR,
                     aNewNodeName,
                     sMetaNodeInfo.mShardMetaNumber );

    IDE_TEST( qciMisc::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                     sSqlStr,
                                     & sRowCnt )
              != IDE_SUCCESS );

    idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                     "UPDATE SYS_SHARD.REPLICA_SETS_ "
                     "   SET STOP_SECOND_BACKUP_NODE_NAME   = "QCM_SQL_VARCHAR_FMT
                     "   WHERE STOP_SECOND_BACKUP_NODE_NAME = "QCM_SQL_VARCHAR_FMT
                     "         AND SMN                     = "QCM_SQL_BIGINT_FMT,
                     SDM_NA_STR,
                     aNewNodeName,
                     sMetaNodeInfo.mShardMetaNumber );

    IDE_TEST( qciMisc::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                     sSqlStr,
                                     & sRowCnt )
              != IDE_SUCCESS );
    ideLog::log(IDE_SD_0,"[SHARD_DROP_FORCE] RESET REPLICASET UPDATED");

    sState = 1;
    IDE_TEST( sSmiStmt.end( SMI_STATEMENT_RESULT_SUCCESS ) != IDE_SUCCESS );

    sState = 0;
    QC_SMI_STMT(aStatement) = sOldStmt;

    sdi::setShardMetaTouched( aStatement->session );

    return IDE_SUCCESS;

    IDE_EXCEPTION( Not_failed_over )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDM_NOT_FAILEDOVER ) );
    }
    IDE_EXCEPTION_END;

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

    return IDE_FAILURE;
}

IDE_RC sdm::touchMeta( qcStatement        * aStatement )
{
    smiStatement      * sOldStmt = NULL;
    smiStatement        sSmiStmt;
    UInt                sSmiStmtFlag;
    SInt                sState = 0;

    sSmiStmtFlag = SMI_STATEMENT_NORMAL | SMI_STATEMENT_MEMORY_CURSOR;
    sOldStmt                = QC_SMI_STMT(aStatement);
    QC_SMI_STMT(aStatement) = &sSmiStmt;
    sState = 1;

    IDE_TEST( sSmiStmt.begin( aStatement->mStatistics,
                              sOldStmt,
                              sSmiStmtFlag )
              != IDE_SUCCESS );
    sState = 2;

    IDE_TEST( checkMetaVersion( QC_SMI_STMT( aStatement ) )
              != IDE_SUCCESS );

    IDE_TEST( makeShardMeta4NewSMN(aStatement) != IDE_SUCCESS );

    sState = 1;
    IDE_TEST( sSmiStmt.end( SMI_STATEMENT_RESULT_SUCCESS ) != IDE_SUCCESS );

    sState = 0;
    QC_SMI_STMT(aStatement) = sOldStmt;

    sdi::setShardMetaTouched( aStatement->session );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

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

    return IDE_FAILURE;
}


/* PROJ-2748 Shard Failback */

/* TASK-7307 DML Data Consistency in Shard */
IDE_RC sdm::alterUsable( qcStatement * aStatement,
                         SChar       * aUserName,
                         SChar       * aTableName,
                         SChar       * aPartitionName,
                         idBool        aIsUsable,
                         idBool        aIsNewTrans )
{
    smiStatement              sSmiStmt;
    smiStatement            * sOldStmt;
    UInt                      sSmiStmtFlag;
    SInt                      sState = 0;
    SChar                   * sSqlStr = NULL;
    UInt                      sUserID = QC_EMPTY_USER_ID;
    sdiLocalMetaInfo          sLocalMetaInfo;

    IDE_TEST( STRUCT_ALLOC_WITH_SIZE( aStatement->qmxMem,
                                      SChar,
                                      QD_MAX_SQL_LENGTH,
                                      &sSqlStr )
              != IDE_SUCCESS );

    if ( ( aPartitionName == NULL ) ||
         ( aPartitionName[0] == '\0' ) ||
         ( idlOS::strncmp( aPartitionName, SDM_NA_STR, QC_MAX_OBJECT_NAME_LEN + 1 ) == 0 ) )
    {
        idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                         "alter table "QCM_SQL_STRING_SKIP_FMT"."QCM_SQL_STRING_SKIP_FMT" access %s",
                         aUserName,
                         aTableName,
                         (aIsUsable == ID_TRUE)? "usable":"unusable" );
    }
    else
    {
        idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                         "alter table "QCM_SQL_STRING_SKIP_FMT"."QCM_SQL_STRING_SKIP_FMT" access partition "QCM_SQL_STRING_SKIP_FMT"  %s",
                         aUserName,
                         aTableName,
                         aPartitionName,
                         (aIsUsable == ID_TRUE)? "usable":"unusable" );
    }

    IDE_TEST( qciMisc::getUserIdByName( aUserName, &sUserID ) != IDE_SUCCESS );

    IDE_TEST( sdi::getLocalMetaInfo( &sLocalMetaInfo ) != IDE_SUCCESS );

    sSmiStmtFlag = SMI_STATEMENT_NORMAL | SMI_STATEMENT_MEMORY_CURSOR;
    sOldStmt                = QC_SMI_STMT(aStatement);
    QC_SMI_STMT(aStatement) = &sSmiStmt;
    sState = 1;

    IDE_TEST( sSmiStmt.begin( aStatement->mStatistics,
                              sOldStmt,
                              sSmiStmtFlag )
              != IDE_SUCCESS );
    sState = 2;

    ideLog::log(IDE_SD_17,"[SHARD INTERNAL SQL]: %s", sSqlStr);
    if ( aIsNewTrans == ID_TRUE )
    {
        IDE_TEST( sdi::shardExecTempDMLOrDCLWithNewTrans( aStatement,
                                                          sSqlStr )
                  != IDE_SUCCESS );
    }
    else
    {
        IDE_TEST( qciMisc::runRollbackableInternalDDL( aStatement,
                                                       QC_SMI_STMT( aStatement ),
                                                       sUserID,
                                                       sSqlStr )
                  != IDE_SUCCESS );
    }

    sState = 1;
    IDE_TEST( sSmiStmt.end( SMI_STATEMENT_RESULT_SUCCESS ) != IDE_SUCCESS );

    sState = 0;
    QC_SMI_STMT(aStatement) = sOldStmt;

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

IDE_RC sdm::alterShardFlag( qcStatement      * aStatement,
                            SChar            * aUserName,
                            SChar            * aTableName,
                            sdiSplitMethod     aSdSplitMethod, 
                            idBool             aIsNewTrans )
{
    smiStatement              sSmiStmt;
    smiStatement            * sOldStmt;
    UInt                      sSmiStmtFlag;
    UInt                      sUserID = QC_EMPTY_USER_ID;
    SInt                      sState  = 0;
    SChar                   * sSqlStr;
    sdiLocalMetaInfo          sLocalMetaInfo;

    IDE_TEST( STRUCT_ALLOC_WITH_SIZE( aStatement->qmxMem,
                                      SChar,
                                      QD_MAX_SQL_LENGTH,
                                      &sSqlStr )
              != IDE_SUCCESS );

    switch( aSdSplitMethod )
    {
        case SDI_SPLIT_RANGE:
        case SDI_SPLIT_LIST:
        case SDI_SPLIT_HASH:
            idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                             "alter table "QCM_SQL_STRING_SKIP_FMT"."QCM_SQL_STRING_SKIP_FMT" shard split",
                             aUserName,
                             aTableName);
            break;

        case SDI_SPLIT_CLONE:
            idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                             "alter table "QCM_SQL_STRING_SKIP_FMT"."QCM_SQL_STRING_SKIP_FMT" shard clone",
                             aUserName,
                             aTableName);
            break;

        case SDI_SPLIT_SOLO:
            idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                             "alter table "QCM_SQL_STRING_SKIP_FMT"."QCM_SQL_STRING_SKIP_FMT" shard solo",
                             aUserName,
                             aTableName);
            break;

        case SDI_SPLIT_NONE:
            idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                             "alter table "QCM_SQL_STRING_SKIP_FMT"."QCM_SQL_STRING_SKIP_FMT" shard none",
                             aUserName,
                             aTableName);
            break;

        default:
            IDE_CONT( normal_exit );
            break;
    }

    IDE_TEST( qciMisc::getUserIdByName( aUserName, &sUserID ) != IDE_SUCCESS );

    IDE_TEST( sdi::getLocalMetaInfo( &sLocalMetaInfo ) != IDE_SUCCESS );

    sSmiStmtFlag = SMI_STATEMENT_NORMAL | SMI_STATEMENT_MEMORY_CURSOR;
    sOldStmt                = QC_SMI_STMT(aStatement);
    QC_SMI_STMT(aStatement) = &sSmiStmt;
    sState = 1;

    IDE_TEST( sSmiStmt.begin( aStatement->mStatistics,
                              sOldStmt,
                              sSmiStmtFlag )
              != IDE_SUCCESS );
    sState = 2;

    ideLog::log(IDE_SD_17,"[SHARD INTERNAL SQL]: %s", sSqlStr);
    if ( aIsNewTrans == ID_TRUE )
    {
        IDE_TEST( sdi::shardExecTempDMLOrDCLWithNewTrans( aStatement,
                                                          sSqlStr )
                  != IDE_SUCCESS );
    }
    else
    {
        IDE_TEST( qciMisc::runRollbackableInternalDDL( aStatement,
                                                       QC_SMI_STMT( aStatement ),
                                                       sUserID,
                                                       sSqlStr )
                  != IDE_SUCCESS );
    }

    sState = 1;
    IDE_TEST( sSmiStmt.end( SMI_STATEMENT_RESULT_SUCCESS ) != IDE_SUCCESS );

    sState = 0;
    QC_SMI_STMT(aStatement) = sOldStmt;

    if ( ( aIsNewTrans == ID_TRUE ) &&
         ( aSdSplitMethod != SDI_SPLIT_NONE ) )
    {
        // 실패 시 revert 처리
        idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                         "alter table "QCM_SQL_STRING_SKIP_FMT"."QCM_SQL_STRING_SKIP_FMT" shard none",
                         aUserName,
                         aTableName );
    
        if (( aStatement->session->mQPSpecific.mFlag & QC_SESSION_SHARD_DDL_MASK ) ==
            QC_SESSION_SHARD_DDL_TRUE )
        {
            (void)sdiZookeeper::addPendingJob( sSqlStr,
                                               sLocalMetaInfo.mNodeName,
                                               ZK_PENDING_JOB_AFTER_ROLLBACK,
                                               QCI_STMT_MASK_DDL );
        }
        else
        {
            // shard package에서의 예외처리를 위해 
            (void)sdiZookeeper::addRevertJob( sSqlStr,
                                              sLocalMetaInfo.mNodeName,
                                              ZK_REVERT_JOB_TABLE_ALTER );
        }
    }

    IDE_EXCEPTION_CONT( normal_exit );

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

IDE_RC sdm::updateShardMetaForSplitPartition( qcStatement * aStatement,
                                              SChar       * aUserName,
                                              SChar       * aTableName,
                                              SChar       * aSrcPartName,
                                              SChar       * aDstPartName,
                                              SChar       * aValue,
                                              UInt        * aRowCnt )
{
/***********************************************************************
 *
 * Description :
 *    PROJ-2757 Advanced Global DDL
 *
 *    Shard 테이블에 대해 SPLIT PARTITION DDL 구믄을 Global DDL로 수행하면서 
 *    새로 생성되는 파티션 정보를 SYS_SHARD.RANGES_에 입력한다.
 *
 ***********************************************************************/
    vSLong          sRowCnt = 0;
    sdiNode         sNode;
    sdiTableInfo    sTableInfo;
    idBool          sIsTableFound = ID_FALSE;

    sdiGlobalMetaInfo sMetaNodeInfo = { SDI_NULL_SMN };
    SChar           * sSqlStr = NULL;

    SDI_INIT_TABLE_INFO( &sTableInfo );
    SDI_INIT_NODE(&sNode);

    IDE_TEST( checkMetaVersion( QC_SMI_STMT( aStatement ) )
              != IDE_SUCCESS );

    IDE_TEST( makeShardMeta4NewSMN(aStatement) != IDE_SUCCESS );

    IDE_TEST( getGlobalMetaInfoCore( QC_SMI_STMT(aStatement),
                                     &sMetaNodeInfo )
              != IDE_SUCCESS );

    IDE_TEST( getTableInfo( QC_SMI_STMT( aStatement ),
                            aUserName,
                            aTableName,
                            sMetaNodeInfo.mShardMetaNumber,
                            &sTableInfo,
                            &sIsTableFound )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( sIsTableFound == ID_FALSE,
                    ERR_NOT_EXIST_TABLE );

    // BUG-46619
    IDE_TEST( validateRangeCountBeforeInsert( aStatement,
                                              &sTableInfo,
                                              sMetaNodeInfo.mShardMetaNumber )
              != IDE_SUCCESS );

    IDE_TEST( STRUCT_ALLOC_WITH_SIZE( aStatement->qmxMem,
                                      SChar,
                                      QD_MAX_SQL_LENGTH,
                                      &sSqlStr )
              != IDE_SUCCESS);

    idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                     "INSERT INTO SYS_SHARD.RANGES_ "
                     "SELECT SHARD_ID, "      //shardID
                     QCM_SQL_VARCHAR_FMT", "  //PARTITION_NAME
                     "%s, "                   //aValue
                     QCM_SQL_VARCHAR_FMT", "  //aSubValue
                     "NODE_ID, "              //aNodeId
                     QCM_SQL_BIGINT_FMT", "   //SMN
                     "REPLICA_SET_ID "        //ReplicaSetId
                     "FROM SYS_SHARD.RANGES_ "
                     "WHERE SHARD_ID="QCM_SQL_INT32_FMT" "
                     "AND PARTITION_NAME="QCM_SQL_VARCHAR_FMT" "
                     "AND SMN="QCM_SQL_BIGINT_FMT" "
                     "UNION "
                     "SELECT SHARD_ID, "      //shardID
                     QCM_SQL_VARCHAR_FMT", "  //PARTITION_NAME
                     "%s, "                   //aValue
                     QCM_SQL_VARCHAR_FMT", "  //aSubValue
                     "DEFAULT_NODE_ID, "      //aNodeId
                     QCM_SQL_BIGINT_FMT", "   //SMN
                     "DEFAULT_PARTITION_REPLICA_SET_ID "  //ReplicaSetId
                     "FROM SYS_SHARD.OBJECTS_ "
                     "WHERE SHARD_ID="QCM_SQL_INT32_FMT" "
                     "AND DEFAULT_PARTITION_NAME="QCM_SQL_VARCHAR_FMT" "
                     "AND SMN="QCM_SQL_BIGINT_FMT,
                     aDstPartName,
                     aValue,
                     SDM_NA_STR,
                     sMetaNodeInfo.mShardMetaNumber,
                     sTableInfo.mShardID,
                     aSrcPartName,
                     sMetaNodeInfo.mShardMetaNumber,
                     aDstPartName,
                     aValue,
                     SDM_NA_STR,
                     sMetaNodeInfo.mShardMetaNumber,
                     sTableInfo.mShardID,
                     aSrcPartName,
                     sMetaNodeInfo.mShardMetaNumber );
    
    IDE_TEST( qciMisc::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                     sSqlStr,
                                     & sRowCnt )
              != IDE_SUCCESS );

    if ( aRowCnt != NULL )
    {
        *aRowCnt = (UInt)sRowCnt;
    }
    else
    {
        /* Nothing to do */
    }

    sdi::setShardMetaTouched( aStatement->session );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_EXIST_TABLE )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDM_SHARD_TABLE_NOT_EXIST ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

