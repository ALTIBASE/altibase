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
 * $Id: qrc.cpp 53289 2012-06-12 15:59:48Z junyoung $
 **********************************************************************/

#include <idl.h>
#include <ide.h>
#include <rpi.h>
#include <rpcExecute.h>
#include <rpdCatalog.h>
#include <rpcManager.h>
#include <qci.h>
#include <rpdLockTableManager.h>
#include <sdi.h>

/***********************************************************************
 * EXECUTE
 **********************************************************************/

qciExecuteReplicationCallback rpcExecute::mCallback =
{
    rpcExecute::executeCreate,
    rpcExecute::executeAlterAddTbl,
    rpcExecute::executeAlterDropTbl,
    rpcExecute::executeAlterAddHost,
    rpcExecute::executeAlterDropHost,
    rpcExecute::executeAlterSetHost,
    rpcExecute::executeAlterSetMode,
    rpcExecute::executeDrop,
    rpcExecute::executeStart,
    rpcExecute::executeQuickStart,
    rpcExecute::executeSync,
    rpcExecute::executeSyncCondition,
    rpcExecute::executeTempSync,
    rpcExecute::executeReset,
    rpcExecute::executeAlterSetRecovery,
    rpcExecute::executeAlterSetOfflineEnable,
    rpcExecute::executeAlterSetOfflineDisable,
    rpcExecute::executeAlterSetGapless,
    rpcExecute::executeAlterSetParallel,
    rpcExecute::executeAlterSetGrouping,
    rpcExecute::executeAlterSetDDLReplicate,
    rpcExecute::executeAlterSplitPartition,
    rpcExecute::executeAlterMergePartition,
    rpcExecute::executeAlterDropPartition,
    rpcExecute::executeFailover,
    rpcExecute::executeStop,
    rpcExecute::executeFlush,
    rpcExecute::executeFailback,
    rpcExecute::executeDeleteItemReplaceHistory
};

IDE_RC rpcExecute::executeCreate(void * aQcStatement)
{
    IDE_TEST( rpi::createReplication( aQcStatement ) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpcExecute::executeAlterAddTbl(void * aQcStatement)
{
    return rpi::alterReplicationAddTable( aQcStatement );
}

IDE_RC rpcExecute::executeAlterDropTbl(void * aQcStatement)
{
    return rpi::alterReplicationDropTable( aQcStatement );
}

IDE_RC rpcExecute::executeAlterAddHost(void * aQcStatement)
{
    qriParseTree     * sParseTree;
    idBool            sIsStarted;

    sParseTree = ( qriParseTree * )QCI_PARSETREE( aQcStatement );

    // If this altering replication is already started,
    //    then ERR_REPLICATION_ALREADY_STARTED
    IDE_TEST(rpdCatalog::checkReplicationStarted(
                 aQcStatement, sParseTree->replName, &sIsStarted) != IDE_SUCCESS);
    IDE_TEST_RAISE(sIsStarted == ID_TRUE, ERR_REPLICATION_ALREADY_STARTED);

    IDE_TEST( rpi::alterReplicationAddHost( aQcStatement ) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_REPLICATION_ALREADY_STARTED)
    {
        IDE_SET(ideSetErrorCode(rpERR_ABORT_RPC_REPLICATION_ALREADY_STARTED));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

IDE_RC rpcExecute::executeAlterDropHost(void * aQcStatement)
{
    qriParseTree     * sParseTree;
    idBool            sIsStarted;

    sParseTree = ( qriParseTree * )QCI_PARSETREE( aQcStatement );

    // If this altering replication is already started,
    //    then ERR_REPLICATION_ALREADY_STARTED
    IDE_TEST(rpdCatalog::checkReplicationStarted(
                 aQcStatement, sParseTree->replName, &sIsStarted) != IDE_SUCCESS);
    IDE_TEST_RAISE(sIsStarted == ID_TRUE, ERR_REPLICATION_ALREADY_STARTED);

    IDE_TEST( rpi::alterReplicationDropHost( aQcStatement ) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_REPLICATION_ALREADY_STARTED)
    {
        IDE_SET(ideSetErrorCode(rpERR_ABORT_RPC_REPLICATION_ALREADY_STARTED));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpcExecute::executeAlterSetHost(void * aQcStatement)
{
    IDE_TEST( rpi::alterReplicationSetHost( aQcStatement ) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpcExecute::executeAlterSetMode(void * /* aQcStatement */)
{
    IDE_SET(ideSetErrorCode(rpERR_ABORT_RPC_NOT_SUPPORT_REPLICATION_DDL));

    return IDE_FAILURE;
}

IDE_RC rpcExecute::executeDrop(void * aQcStatement)
{
	qriParseTree     * sParseTree;
    idBool            sIsStarted;

    sParseTree = ( qriParseTree * )QCI_PARSETREE( aQcStatement );

    // If this dropping replication is already started,
    //    then ERR_REPLICATION_ALREADY_STARTED
    IDE_TEST(rpdCatalog::checkReplicationStarted(
                 aQcStatement, sParseTree->replName, &sIsStarted) != IDE_SUCCESS);
    IDE_TEST_RAISE(sIsStarted == ID_TRUE, ERR_REPLICATION_ALREADY_STARTED);

    IDE_TEST( rpi::dropReplication( aQcStatement ) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_REPLICATION_ALREADY_STARTED)
    {
        IDE_SET(ideSetErrorCode(rpERR_ABORT_RPC_REPLICATION_ALREADY_STARTED));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpcExecute::executeStart(void * aQcStatement)
{
    SChar          sReplName[ QC_MAX_OBJECT_NAME_LEN + 1 ];
    idBool         sTryHandshakeOnce = ID_TRUE;
    smSN           sStartSN = SM_SN_NULL;
    qriParseTree * sParseTree = ( qriParseTree * )QCI_PARSETREE( aQcStatement );

    QCI_STR_COPY( sReplName, sParseTree->replName );

    // BUG-29115
    if(sParseTree->startOption == RP_START_OPTION_SN)
    {
        sStartSN = (smSN)sParseTree->startSN;
    }

    // BUG-18714
    if(sParseTree->startOption == RP_START_OPTION_RETRY)
    {
        sTryHandshakeOnce = ID_FALSE;
    }

    /* PROJ-1915 */
    if(sParseTree->startType == RP_OFFLINE)
    {
        sTryHandshakeOnce = ID_TRUE;
    }

    if(sParseTree->startType == RP_START_CONDITIONAL)
    {
        sTryHandshakeOnce = ID_TRUE;
    }

    //BUG-22703 : Begin Statement를 수행한 후에 Hang이 걸리지 않아야 합니다.
    // mStatistics 통계 정보를 전달 합니다.
    IDE_TEST( rpi::startSenderThread( QCI_STATISTIC(aQcStatement),
                                      QCI_QMP_MEM(aQcStatement),
                                      QCI_SMI_STMT(aQcStatement),
                                      sReplName,
                                      sParseTree->startType,
                                      sTryHandshakeOnce,
                                      sStartSN,
                                      NULL,
                                      1,
                                      sParseTree->lockTable )
              != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpcExecute::executeQuickStart(void * aQcStatement)
{
    SChar         sReplName[ QC_MAX_OBJECT_NAME_LEN + 1 ];
    idBool        sTryHandshakeOnce = ID_TRUE;
    qriParseTree * sParseTree = ( qriParseTree * )QCI_PARSETREE( aQcStatement );

    QCI_STR_COPY( sReplName, sParseTree->replName );

    // BUG-18714
    if(sParseTree->startOption == RP_START_OPTION_RETRY)
    {
        sTryHandshakeOnce = ID_FALSE;
    }

    //BUG-22703 : Begin Statement를 수행한 후에 Hang이 걸리지 않아야 합니다.
    // mStatistics 통계 정보를 전달 합니다.
    IDE_TEST( rpi::startSenderThread( QCI_STATISTIC(aQcStatement),
                                      QCI_QMP_MEM(aQcStatement),
                                      QCI_SMI_STMT(aQcStatement),
                                      sReplName,
                                      RP_QUICK,
                                      sTryHandshakeOnce,
                                      SM_SN_NULL,
                                      NULL,
                                      1,
                                      sParseTree->lockTable )
              != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpcExecute::executeSyncCondition(void * aQcStatement)
{
    SChar          sReplName[ QCI_MAX_NAME_LEN + 1 ];
    qriParseTree * sParseTree = ( qriParseTree * )QCI_PARSETREE( aQcStatement );

    QCI_STR_COPY( sReplName, sParseTree->replName );

    IDE_TEST( rpi::startSenderThread( QCI_STATISTIC(aQcStatement),
                                      QCI_QMP_MEM(aQcStatement),
                                      QCI_SMI_STMT(aQcStatement),
                                      sReplName,
                                      sParseTree->startType,
                                      ID_TRUE,
                                      SM_SN_NULL,
                                      NULL,
                                      sParseTree->parallelFactor,
                                      sParseTree->lockTable )
              != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpcExecute::executeSync(void * aQcStatement)
{
    SChar          sReplName[ QC_MAX_OBJECT_NAME_LEN + 1 ];
    qriParseTree * sParseTree = ( qriParseTree * )QCI_PARSETREE( aQcStatement );
    qriReplItem  * sReplItem;
    qciSyncItems * sSyncItemList = NULL;
    qciSyncItems * sSyncItem = NULL;

    smiStatement         * sSmiStmt = QCI_SMI_STMT( aQcStatement );
    qcmPartitionInfoList * sPartInfoList = NULL;
    qcmPartitionInfoList * sTempPartInfoList = NULL;
    qcmTableInfo         * sPartInfo;
    qcmTableInfo         * sTableInfo;
    void                 * sTableHandle = NULL;
    smSCN                  sSCN;


    QCI_STR_COPY( sReplName, sParseTree->replName );

    for(sReplItem = sParseTree->replItems;
        sReplItem != NULL;
        sReplItem = sReplItem->next)
    {

        if ( sReplItem->replication_unit == RP_REPLICATION_PARTITION_UNIT )
        {
            sSyncItem = NULL;

            IDU_FIT_POINT( "rpcExecute::executeSync::alloc::SyncItem" );
            IDE_TEST( ( ( iduMemory * )QCI_QMX_MEM( aQcStatement ) )->alloc( ID_SIZEOF( qciSyncItems ),
                                                                             (void **)&sSyncItem)
                      != IDE_SUCCESS);

            idlOS::strncpy( (SChar*) sSyncItem->syncUserName,
                            sReplItem->localUserName.stmtText + sReplItem->localUserName.offset,
                            sReplItem->localUserName.size);
            sSyncItem->syncUserName [ sReplItem->localUserName.size ] = '\0';

            idlOS::strncpy( (SChar*) sSyncItem->syncTableName,
                            sReplItem->localTableName.stmtText + sReplItem->localTableName.offset,
                            sReplItem->localTableName.size);
            sSyncItem->syncTableName [ sReplItem->localTableName.size ] = '\0';
            /* PROJ-2366 */

            idlOS::strncpy( (SChar*) sSyncItem->syncPartitionName,
                            sReplItem->localPartitionName.stmtText + sReplItem->localPartitionName.offset,
                            sReplItem->localPartitionName.size);
            sSyncItem->syncPartitionName[ sReplItem->localPartitionName.size ] = '\0';
            sSyncItem->next = sSyncItemList;
            sSyncItemList   = sSyncItem;
        }
        else
        {
            IDE_TEST( qciMisc::getTableInfo( aQcStatement,
                                             sReplItem->localUserID,
                                             sReplItem->localTableName,
                                             & sTableInfo,
                                             & sSCN,
                                             & sTableHandle ) != IDE_SUCCESS );

            IDE_TEST( qciMisc::lockTableForDDLValidation( aQcStatement,
                                                          sTableHandle,
                                                          sSCN )
                      != IDE_SUCCESS);
            
            if ( sTableInfo->tablePartitionType == QCM_PARTITIONED_TABLE )
            {
                IDE_TEST( qciMisc::getPartitionInfoList( aQcStatement,
                                                         sSmiStmt,
                                                         ( iduMemory * )QCI_QMX_MEM( aQcStatement ),
                                                         sTableInfo->tableID,
                                                         &sPartInfoList )
                          != IDE_SUCCESS );

                for ( sTempPartInfoList = sPartInfoList;
                      sTempPartInfoList != NULL;
                      sTempPartInfoList = sTempPartInfoList->next )
                {
                    IDE_TEST( qciMisc::lockTableForDDLValidation( aQcStatement,
                                                                  sTempPartInfoList->partHandle,
                                                                  sTempPartInfoList->partSCN )
                              != IDE_SUCCESS );
                    
                    sPartInfo = sTempPartInfoList->partitionInfo;
                    // malloc and add partition
                    sSyncItem = NULL;

                    IDU_FIT_POINT( "rpcExecute::executeSync::alloc::SyncItemQcmTable" );
                    IDE_TEST( ( ( iduMemory * )QCI_QMX_MEM( aQcStatement ) )->alloc( ID_SIZEOF( qciSyncItems ),
                                                                                     (void **)&sSyncItem )
                              != IDE_SUCCESS);

                    idlOS::strncpy( (SChar*) sSyncItem->syncUserName,
                                    sReplItem->localUserName.stmtText + sReplItem->localUserName.offset,
                                    sReplItem->localUserName.size);
                    sSyncItem->syncUserName [ sReplItem->localUserName.size ] = '\0';

                    idlOS::strncpy( (SChar*) sSyncItem->syncTableName,
                                    sReplItem->localTableName.stmtText + sReplItem->localTableName.offset,
                                    sReplItem->localTableName.size);
                    sSyncItem->syncTableName[ sReplItem->localTableName.size ] = '\0';
                    /* PROJ-2366 */
                    idlOS::strncpy( sSyncItem->syncPartitionName,
                                    sPartInfo->name,
                                    QC_MAX_OBJECT_NAME_LEN );

                    sSyncItem->next = sSyncItemList;
                    sSyncItemList   = sSyncItem;
                }
            }
            else
            {
                sSyncItem = NULL;

                IDU_FIT_POINT( "rpcExecute::executeSync::alloc::SyncItemNonQcmTable" );
                IDE_TEST( ( ( iduMemory * )QCI_QMX_MEM( aQcStatement ) )
                        ->alloc( ID_SIZEOF( qciSyncItems ),
                                (void **)&sSyncItem)
                                != IDE_SUCCESS);

                idlOS::strncpy( (SChar*) sSyncItem->syncUserName,
                                sReplItem->localUserName.stmtText + sReplItem->localUserName.offset,
                                    sReplItem->localUserName.size);
                sSyncItem->syncUserName [ sReplItem->localUserName.size ] = '\0';

                idlOS::strncpy( (SChar*) sSyncItem->syncTableName,
                                sReplItem->localTableName.stmtText + sReplItem->localTableName.offset,
                                sReplItem->localTableName.size);
                sSyncItem->syncTableName [ sReplItem->localTableName.size ] = '\0';
                /* PROJ-2366 */
                sSyncItem->syncPartitionName[0] = '\0';
                sSyncItem->next = sSyncItemList;
                sSyncItemList   = sSyncItem;
            }
        }

    }

    //BUG-22703 : Begin Statement를 수행한 후에 Hang이 걸리지 않아야 합니다.
    // mStatistics 통계 정보를 전달 합니다.
    IDE_TEST( rpi::startSenderThread( QCI_STATISTIC(aQcStatement),
                                      QCI_QMP_MEM(aQcStatement),
                                      QCI_SMI_STMT(aQcStatement),
                                      sReplName,
                                      sParseTree->startType,
                                      ID_TRUE,
                                      SM_SN_NULL,
                                      sSyncItemList,
                                      sParseTree->parallelFactor,
                                      sParseTree->lockTable )
              != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpcExecute::executeTempSync(void * aQcStatement)
{
    IDE_TEST( rpi::startTempSync( aQcStatement ) 
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

IDE_RC rpcExecute::executeStop( void         * aQcStatement,
                                SChar        * aReplName )                               
{
    qriParseTree * sParseTree = ( qriParseTree * )QCI_PARSETREE( aQcStatement );

    /* BUG-48290 */
    IDE_TEST( sdi::checkShardReplication( (qcStatement*)aQcStatement ) != IDE_SUCCESS );

    //BUG-22703 : Begin Statement를 수행한 후에 Hang이 걸리지 않아야 합니다.
    // mStatistics 통계 정보를 전달 합니다.
    IDE_TEST( rpi::stopSenderThread( QCI_SMI_STMT( aQcStatement ),
                                     aReplName,
                                     QCI_STATISTIC( aQcStatement ),
                                     sParseTree->isImmediateStop )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

IDE_RC rpcExecute::executeReset(void * aQcStatement)
{
    SChar sReplName[ QC_MAX_OBJECT_NAME_LEN + 1 ];
    qriParseTree * sParseTree = ( qriParseTree * )QCI_PARSETREE( aQcStatement );

    QCI_STR_COPY( sReplName, sParseTree->replName );

    //BUG-22703 : Begin Statement를 수행한 후에 Hang이 걸리지 않아야 합니다.
    // mStatistics 통계 정보를 전달 합니다.
    IDE_TEST(rpi::resetReplication(QCI_SMI_STMT(aQcStatement),
                                   sReplName,
                                   QCI_STATISTIC( aQcStatement ))
             != IDE_SUCCESS);

    if ( qciMisc::getTransactionalDDL( aQcStatement ) == ID_TRUE )
    {
        qciMisc::setDDLDestInfo( aQcStatement,
                                 0,
                                 NULL,
                                 0,
                                 NULL );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
rpcExecute::executeFlush( smiStatement  * aSmiStmt,
                          SChar         * aReplName,
                          rpFlushOption * aFlushOption,
                          idvSQL        * aStatistics )
{
/***********************************************************************
 *
 * Description :
 *    To Fix PR-10590
 *    ALTER REPLICATION name FLUSH 에 대한 Execution
 *
 * Implementation :
 *
 ***********************************************************************/
    return  rpi::alterReplicationFlush( aSmiStmt,
                                        aReplName,
                                        aFlushOption,
                                        aStatistics );
}

IDE_RC rpcExecute::executeAlterSetRecovery(void * aQcStatement)
{
    return rpi::alterReplicationSetRecovery(aQcStatement);
}

/* PROJ-1915 */
IDE_RC rpcExecute::executeAlterSetOfflineEnable(void * aQcStatement)
{
    return rpi::alterReplicationSetOfflineEnable(aQcStatement);
}

IDE_RC rpcExecute::executeAlterSetOfflineDisable(void * aQcStatement)
{
    return rpi::alterReplicationSetOfflineDisable(aQcStatement);
}

/* PROJ-1969 */
IDE_RC rpcExecute::executeAlterSetGapless( void * aQcStatement )
{
    return rpi::alterReplicationSetGapless( aQcStatement );
}

IDE_RC rpcExecute::executeAlterSetParallel( void * aQcStatement )
{
    return rpi::alterReplicationSetParallel( aQcStatement );
}

IDE_RC rpcExecute::executeAlterSetGrouping( void * aQcStatement )
{
    return rpi::alterReplicationSetGrouping( aQcStatement );
}

IDE_RC rpcExecute::executeAlterSetDDLReplicate( void * aQcStatement )
{
    return rpi::alterReplicationSetDDLReplicate( aQcStatement );
}

IDE_RC rpcExecute::executeAlterSplitPartition( void         * aQcStatement,
                                               qcmTableInfo * aTableInfo,
                                               qcmTableInfo * aSrcPartInfo,
                                               qcmTableInfo * aDstPartInfo1,
                                               qcmTableInfo * aDstPartInfo2 )
{
    return rpcManager::splitPartitionForAllRepl( aQcStatement,
                                                 aTableInfo,
                                                 aSrcPartInfo,
                                                 aDstPartInfo1,
                                                 aDstPartInfo2 );
}

IDE_RC rpcExecute::executeAlterMergePartition( void         * aQcStatement,
                                               qcmTableInfo * aTableInfo,
                                               qcmTableInfo * aSrcPartInfo1,
                                               qcmTableInfo * aSrcPartInfo2,
                                               qcmTableInfo * aDstPartInfo )
{
    return rpcManager::mergePartitionForAllRepl( aQcStatement,
                                                 aTableInfo,
                                                 aSrcPartInfo1,
                                                 aSrcPartInfo2,
                                                 aDstPartInfo );
}

IDE_RC rpcExecute::executeAlterDropPartition( void          * aQcStatement,
                                              qcmTableInfo  * aTableInfo,
                                              qcmTableInfo  * aSrcPartInfo )
{
    return rpcManager::dropPartitionForAllRepl( aQcStatement,
                                                aTableInfo,
                                                aSrcPartInfo );
}


IDE_RC rpcExecute::executeFailback(void * aQcStatement)
{
    SChar          sReplName[ QC_MAX_OBJECT_NAME_LEN + 1 ];
    qriParseTree * sParseTree = ( qriParseTree * )QCI_PARSETREE( aQcStatement );

    QCI_STR_COPY( sReplName, sParseTree->replName );

    IDE_TEST( rpi::startSenderThread( QCI_STATISTIC(aQcStatement),
                                      QCI_QMP_MEM(aQcStatement),
                                      QCI_SMI_STMT( aQcStatement ),
                                      sReplName,
                                      sParseTree->startType,
                                      ID_TRUE,
                                      SM_SN_NULL,
                                      NULL,
                                      1,
                                      sParseTree->lockTable )
              != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpcExecute::executeDeleteItemReplaceHistory(void * aQcStatement)
{
    qriParseTree     * sParseTree;
    smiStatement     * sSmiStmt;
    SChar              sRepName[ QCI_MAX_NAME_LEN + 1 ];

    sParseTree = ( qriParseTree * )QCI_PARSETREE( aQcStatement );
    sSmiStmt   = QCI_SMI_STMT( aQcStatement );
    QCI_STR_COPY( sRepName, sParseTree->replName );

    IDE_TEST( rpdCatalog::removeReplItemReplaceHistory( sSmiStmt, sRepName ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
    
IDE_RC rpcExecute::executeFailover( void * aQcStatement )
{
    return rpcManager::executeFailover( aQcStatement );
}
