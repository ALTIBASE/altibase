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
* $Id: 
***********************************************************************/

#include <rpcManager.h>
#include <rpdCatalog.h>
#include <qci.h>
#include <rpcDDLSyncManager.h>
#include <rpcResourceManager.h>
#include <rpsSQLExecutor.h>

IDTHREAD rpcResourceManager*     rpcDDLSyncManager::mResourceMgr;

IDE_RC rpcDDLSyncManager::initialize()
{
    idBool sInitDDLSyncListMutex     = ID_FALSE;
    idBool sInitExecutorListMutex    = ID_FALSE;
    SChar  sName[IDU_MUTEX_NAME_LEN] = { 0, };

    idlOS::snprintf( sName, IDU_MUTEX_NAME_LEN, "RP_DDL_EXECUTOR_LIST_MUTEX" );
    IDE_TEST_RAISE( mDDLExecutorListLock.initialize( sName,
                                                     IDU_MUTEX_KIND_POSIX,
                                                     IDV_WAIT_INDEX_NULL )
                    != IDE_SUCCESS, ERR_MUTEX_INIT );
    sInitExecutorListMutex = ID_TRUE;

    idlOS::snprintf( sName, IDU_MUTEX_NAME_LEN, "RP_DDL_SYNC_INFO_LIST_MUTEX" );    
    IDE_TEST_RAISE( mDDLSyncInfoListLock.initialize( sName,
                                                     IDU_MUTEX_KIND_POSIX,
                                                     IDV_WAIT_INDEX_NULL )
                    != IDE_SUCCESS, ERR_MUTEX_INIT );
    sInitDDLSyncListMutex = ID_TRUE;

    IDU_LIST_INIT( &mDDLSyncInfoList );
    IDU_LIST_INIT( &mDDLExecutorList );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_MUTEX_INIT );
    {       
        IDE_ERRLOG( IDE_RP_0 );
        IDE_SET( ideSetErrorCode( rpERR_FATAL_ThrMutexInit ) );
    }
    IDE_EXCEPTION_END;
    
    IDE_PUSH();

    if ( sInitDDLSyncListMutex == ID_TRUE )
    {
        sInitDDLSyncListMutex = ID_FALSE;
        (void)mDDLSyncInfoListLock.destroy();
    }
    else
    {
        /* nothing tyo do */
    }

    if ( sInitExecutorListMutex == ID_TRUE )
    {
        sInitExecutorListMutex = ID_FALSE;
        (void)mDDLExecutorListLock.destroy();
    }
    else
    {
        /* nothing to do */
    }
    
    IDE_POP();

    return IDE_FAILURE;
}

void rpcDDLSyncManager::finalize()
{
    cancelAndWaitAllDDLSync();

    finalizeDDLExcutorList();
    
    IDE_DASSERT( IDU_LIST_IS_EMPTY( &mDDLExecutorList ) == ID_TRUE );
    IDE_DASSERT( IDU_LIST_IS_EMPTY( &mDDLSyncInfoList ) == ID_TRUE );

    if ( mDDLSyncInfoListLock.destroy() != IDE_SUCCESS )
    {
        IDE_ERRLOG( IDE_RP_0 );
    }
    else
    {
        /* nothing to do */
    }

    if ( mDDLExecutorListLock.destroy() != IDE_SUCCESS )
    {
        IDE_ERRLOG( IDE_RP_0 );
    }
    else
    {
        /* nothing to do */
    }
}

void rpcDDLSyncManager::cancelAndWaitAllDDLSync()
{
    iduListNode   *  sSyncNode     = NULL;
    iduListNode    * sReplNode     = NULL;
    rpcDDLSyncInfo * sSyncInfo     = NULL;
    rpcDDLReplInfo * sReplInfo     = NULL;

    IDE_ASSERT( mDDLSyncInfoListLock.lock( NULL ) == IDE_SUCCESS );

    IDU_LIST_ITERATE( &mDDLSyncInfoList, sSyncNode )
    {
        sSyncInfo = (rpcDDLSyncInfo*)sSyncNode->mObj;

        IDU_LIST_ITERATE( &( sSyncInfo->mDDLReplInfoList ), sReplNode )
        {
            sReplInfo = (rpcDDLReplInfo*)sReplNode->mObj;
            *( sReplInfo->mDDLSyncExitFlag ) = ID_TRUE;
            IDU_SESSION_SET_CANCELED( *( sReplInfo->mStatistics->mSessionEvent ) );
        }
    }

    IDE_ASSERT( mDDLSyncInfoListLock.unlock() == IDE_SUCCESS ); 
    
    while( IDU_LIST_IS_EMPTY( &mDDLSyncInfoList ) != ID_TRUE ) 
    {
        idlOS::sleep( 1 );
    };
}

void rpcDDLSyncManager::finalizeDDLExcutorList()
{
    iduListNode    * sExecutorNode = NULL;
    rpxDDLExecutor * sExecutor     = NULL;

    IDE_ASSERT( mDDLExecutorListLock.lock( NULL ) == IDE_SUCCESS );

    IDU_LIST_ITERATE( &mDDLExecutorList, sExecutorNode )
    {
        sExecutor = (rpxDDLExecutor*)sExecutorNode->mObj;
        sExecutor->setExitFlag( ID_TRUE );
    }

    IDE_ASSERT( mDDLExecutorListLock.unlock() == IDE_SUCCESS );
    
    (void)realizeDDLExecutor( NULL );
}

IDE_RC rpcDDLSyncManager::ddlSyncBegin( qciStatement        * aQciStatement, 
                                        rpcResourceManager  * aResourceMgr )
{
    rpcDDLSyncInfo * sDDLSyncInfo  = NULL;
    smiStatement   * sSmiStatement = QCI_SMI_STMT( &( aQciStatement->statement ) );
    smiTrans       * sDDLTrans     = sSmiStatement->getTrans();

    ideLog::log( IDE_RP_0, "[DDLSyncManager] DDL sync begin" );
    mResourceMgr = aResourceMgr;

    IDE_TEST( allocAndInitDDLSyncInfo( RP_MASTER_DDL, &sDDLSyncInfo ) != IDE_SUCCESS );

    IDE_TEST( setDDLSyncInfoAndTableAccessReadOnly( aQciStatement, sDDLSyncInfo ) != IDE_SUCCESS );

    IDE_TEST( waitLastProcessedSN( sDDLSyncInfo ) != IDE_SUCCESS );

    IDE_TEST( rebuildStatement( aQciStatement ) != IDE_SUCCESS );

    IDE_TEST( setSkipUpdateXSN( sDDLSyncInfo, ID_TRUE ) != IDE_SUCCESS );

    IDE_TEST( setDDLSyncRollbackInfoAndTableAccessRecover( sDDLTrans, sDDLSyncInfo ) != IDE_SUCCESS );

    IDE_TEST( rebuildStatement( aQciStatement ) != IDE_SUCCESS );

    IDE_TEST( processDDLSyncMsg( sDDLSyncInfo, RP_DDL_SYNC_EXECUTE_MSG ) != IDE_SUCCESS );

    ideLog::log( IDE_RP_0, "[DDLSyncManager] DDL sync begin success" );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    ideLog::log( IDE_RP_0, "[DDLSyncManager] DDL Sync begin failure" );

    return IDE_FAILURE;
}

IDE_RC rpcDDLSyncManager::ddlSyncEnd( smiTrans * aDDLTrans )
{
    rpcDDLSyncInfo * sDDLSyncInfo;

    IDE_ASSERT( mResourceMgr != NULL );

    sDDLSyncInfo = mResourceMgr->getDDLSyncInfo();

    IDE_ASSERT( sDDLSyncInfo != NULL );

    ideLog::log( IDE_RP_0, "[DDLSyncManager] Execute DDL success" );

    IDE_TEST( ddlSyncEndCommon( aDDLTrans, sDDLSyncInfo ) != IDE_SUCCESS );

    IDE_TEST_RAISE( processDDLSyncMsg( sDDLSyncInfo, RP_DDL_SYNC_COMMIT_MSG ) 
                    != IDE_SUCCESS, ERR_AFTER_COMMIT_MSG );

    IDE_TEST_RAISE( processDDLSyncMsgAck( sDDLSyncInfo, 
                                          RP_DDL_SYNC_COMMIT_MSG,
                                          RP_DDL_SYNC_SUCCESS,
                                          NULL ) 
                    != IDE_SUCCESS, ERR_AFTER_COMMIT_MSG );
   
    removeFromDDLSyncInfoList( sDDLSyncInfo );
    destroyDDLSyncInfo( sDDLSyncInfo );
    sDDLSyncInfo = NULL;
    mResourceMgr = NULL;

    ideLog::log( IDE_RP_0, "[DDLSyncManager] DDL Sync success" );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_AFTER_COMMIT_MSG );
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RP_DDL_SYNC_ERROR_AFTER_COMMIT_MSG ) );
    }
    IDE_EXCEPTION_END;

    ideLog::log( IDE_RP_0, "[DDLSyncManager] DDL Sync failure" );

    return IDE_FAILURE;
}

IDE_RC rpcDDLSyncManager::ddlSyncBeginInternal( idvSQL              * aStatistics,
                                                cmiProtocolContext  * aProtocolContext,
                                                smiTrans            * aDDLTrans,
                                                idBool              * aExitFlag,
                                                rpdVersion          * aVersion,
                                                SChar               * aRepName,
                                                rpcResourceManager  * aResourceMgr,
                                                SChar              ** aUserName,
                                                SChar              ** aSql )
{
    rpcDDLSyncInfo * sDDLSyncInfo = NULL;

    ideLog::log( IDE_RP_0, "[DDLSyncManager] DDL sync begin" );
    mResourceMgr = aResourceMgr;

    IDE_TEST( allocAndInitDDLSyncInfo( RP_REPLICATED_DDL, &sDDLSyncInfo ) != IDE_SUCCESS );

    IDE_TEST( setDDLSyncInfoAndTableAccessReadOnlyInternal( aStatistics, 
                                                            aProtocolContext,
                                                            aExitFlag,
                                                            aVersion,
                                                            aRepName,
                                                            sDDLSyncInfo,
                                                            aSql ) 
              != IDE_SUCCESS );

    IDE_TEST( waitLastProcessedSN( sDDLSyncInfo ) != IDE_SUCCESS );

    IDE_TEST( setSkipUpdateXSN( sDDLSyncInfo, ID_TRUE ) != IDE_SUCCESS );

    IDE_TEST( setDDLSyncRollbackInfoAndTableAccessRecoverInternal( aStatistics,
                                                                   aDDLTrans, 
                                                                   sDDLSyncInfo ) 
              != IDE_SUCCESS );

    IDE_TEST( processDDLSyncMsg( sDDLSyncInfo, RP_DDL_SYNC_EXECUTE_MSG ) != IDE_SUCCESS );

    *aUserName = sDDLSyncInfo->mDDLTableInfo.mUserName;

    ideLog::log( IDE_RP_0, "[DDLSyncManager] DDL sync begin success" );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    ideLog::log( IDE_RP_0, "[DDLSyncManager] DDL sync failure : %s", ideGetErrorMsg() );

    return IDE_FAILURE;
}

IDE_RC rpcDDLSyncManager::ddlSyncEndInternal( smiTrans * aDDLTrans )
{
    SChar   sErrMsg[RP_MAX_MSG_LEN + 1] = { 0, };
    rpcDDLSyncInfo * sDDLSyncInfo = NULL;

    IDE_DASSERT( mResourceMgr != NULL );

    sDDLSyncInfo = mResourceMgr->getDDLSyncInfo();

    IDE_DASSERT( sDDLSyncInfo != NULL );

    ideLog::log( IDE_RP_0, "[DDLSyncManager] Execute DDL success" );

    IDE_TEST( ddlSyncEndCommon( aDDLTrans, sDDLSyncInfo ) != IDE_SUCCESS );

    IDE_TEST( processDDLSyncMsg( sDDLSyncInfo, RP_DDL_SYNC_COMMIT_MSG ) != IDE_SUCCESS );

    IDU_FIT_POINT_RAISE( "rpcDDLSyncManager::ddlSyncEndInternal::commit:aDDLTrans", ERR_TRANS_COMMIT );
    IDE_TEST_RAISE( aDDLTrans->commit() != IDE_SUCCESS, ERR_TRANS_COMMIT );
    ideLog::log( IDE_RP_0, "[DDLSyncManager] DDL commit on replicated transaction" );

    IDE_TEST( processDDLSyncMsgAck( sDDLSyncInfo, 
                                    RP_DDL_SYNC_COMMIT_MSG,
                                    RP_DDL_SYNC_SUCCESS,
                                    NULL ) 
              != IDE_SUCCESS );

    removeFromDDLSyncInfoList( sDDLSyncInfo );
    destroyDDLSyncInfo( sDDLSyncInfo );
    sDDLSyncInfo = NULL;
    mResourceMgr = NULL;

    ideLog::log( IDE_RP_0, "[DDLSyncManager] DDL Sync success" );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_TRANS_COMMIT );
    {
        IDE_ERRLOG( IDE_RP_0 );
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RP_INTERNAL_ARG, "[DDLSyncManager] Transaction commit failure") );
    }
    IDE_EXCEPTION_END;

    idlOS::strncpy( sErrMsg, 
                    ideGetErrorMsg(),
                    RP_MAX_MSG_LEN );

    ideLog::log( IDE_RP_0, "[DDLSyncManager] DDL sync failure : %s", sErrMsg );

    return IDE_FAILURE;
}

IDE_RC rpcDDLSyncManager::ddlSyncEndCommon( smiTrans * aDDLTrans, rpcDDLSyncInfo * aDDLSyncInfo )
{
    SChar   sErrMsg[RP_MAX_MSG_LEN + 1] = { 0, };
    qciTableInfo * sNewTableInfo        = NULL;

    IDE_TEST( getTableInfo( aDDLTrans->getStatistics(),
                            aDDLTrans,
                            aDDLSyncInfo->mDDLTableInfo.mTableName,
                            aDDLSyncInfo->mDDLTableInfo.mUserName,
                            ID_ULONG_MAX,
                            &sNewTableInfo )
              != IDE_SUCCESS );

    IDE_TEST( aDDLTrans->setExpSvpForBackupDDLTargetTableInfo( SM_OID_NULL,
                                                               0,
                                                               NULL,
                                                               sNewTableInfo->tableOID,
                                                               0,
                                                               NULL )
              != IDE_SUCCESS );

    IDE_TEST( setSkipUpdateXSN( aDDLSyncInfo, ID_FALSE ) != IDE_SUCCESS );

    if ( aDDLSyncInfo->mIsBuildNewMeta == ID_TRUE )
    {
        IDE_TEST( buildReceiverNewMeta( aDDLTrans, aDDLSyncInfo ) != IDE_SUCCESS );
    }

    IDE_TEST( processDDLSyncMsgAck( aDDLSyncInfo, 
                                    RP_DDL_SYNC_EXECUTE_RESULT_MSG,
                                    RP_DDL_SYNC_SUCCESS,
                                    NULL ) 
              != IDE_SUCCESS );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    idlOS::strncpy( sErrMsg, 
                    ideGetErrorMsg(),
                    RP_MAX_MSG_LEN );
 
    return IDE_FAILURE;
}

IDE_RC rpcDDLSyncManager::allocAndInitDDLSyncInfo( rpcDDLSyncInfoType    aDDLSyncInfoType, 
                                                   rpcDDLSyncInfo     ** aDDLSyncInfo )
{
    rpcDDLSyncInfo * sDDLSyncInfo = NULL;

    IDU_FIT_POINT_RAISE( "rpcDDLSyncManager::allocAndInitDDLSyncInfo::calloc::sDDLSyncInfo", 
                         ERR_MEMORY_ALLOC_SYNC_INFO );
    IDE_TEST_RAISE( mResourceMgr->rpCalloc( ID_SIZEOF( rpcDDLSyncInfo ), (void**)&sDDLSyncInfo )
                    != IDE_SUCCESS, ERR_MEMORY_ALLOC_SYNC_INFO );

    sDDLSyncInfo->mDDLSyncExitFlag = ID_FALSE;
    sDDLSyncInfo->mDDLSyncInfoType = aDDLSyncInfoType;
    sDDLSyncInfo->mIsBuildNewMeta = ID_FALSE;
    sDDLSyncInfo->mIsSetSavePoint = ID_FALSE;

    IDU_LIST_INIT( &( sDDLSyncInfo->mDDLTableInfo.mPartInfoList ) );

    IDU_LIST_INIT( &( sDDLSyncInfo->mDDLReplInfoList ) );
    IDU_LIST_INIT_OBJ( &( sDDLSyncInfo->mNode ), (void*)sDDLSyncInfo );

    *aDDLSyncInfo = sDDLSyncInfo;
    mResourceMgr->setDDLSyncInfo( sDDLSyncInfo );

    return IDE_SUCCESS;
    
    IDE_EXCEPTION( ERR_MEMORY_ALLOC_SYNC_INFO );
    {
        IDE_ERRLOG( IDE_RP_0 );
        IDE_SET( ideSetErrorCode( rpERR_ABORT_MEMORY_ALLOC,
                                  "rpcDDLSyncManager::allocAndInitDDLSyncInfo",
                                  "sDDLSyncInfo" ) );
    }
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

void rpcDDLSyncManager::destroyDDLSyncInfo( rpcDDLSyncInfo * aDDLSyncInfo )
{   
    IDE_DASSERT( aDDLSyncInfo != NULL );
    mResourceMgr->setDDLSyncInfo( NULL );

    if ( IDU_LIST_IS_EMPTY( &( aDDLSyncInfo->mDDLTableInfo.mPartInfoList ) ) != ID_TRUE )
    {
        freePartInfoList( &( aDDLSyncInfo->mDDLTableInfo.mPartInfoList ) );
    }
    else
    {
        /* nothing to do */
    }

    if ( IDU_LIST_IS_EMPTY( &( aDDLSyncInfo->mDDLReplInfoList ) ) != ID_TRUE )
    {
        destroyDDLReplInfoList( aDDLSyncInfo );
    }
    else
    {
        /* nothing to do */
    }
}

IDE_RC rpcDDLSyncManager::setDDLSyncInfoAndTableAccessReadOnly( qciStatement   * aQciStatement, 
                                                                rpcDDLSyncInfo * aDDLSyncInfo )
{
    idvSQL         * sStatistics  = QCI_STATISTIC( &( aQciStatement->statement ) );
    const void     * sTable       = NULL;
    qciTableInfo   * sTableInfo   = NULL;
    smSCN            sSCN;
    /* PROJ-2735 DDL Transaction 의 영향으로 TableOID 를 TableOID Array 로 변경하였다.
     * DDL Sync 에서는 SrcTableOID 가 2개 이상이 될 수 없으므로 첫번째 값만 가져온다. */
    UInt             sTempOIdCount = 0;
    smOID          * sTempTableOID = qciMisc::getDDLSrcTableOIDArray( aQciStatement, &sTempOIdCount );
    smOID            sTableOID     = sTempTableOID[0];
    smiStatement   * sRootStmt     = NULL;
    smiTrans       * sBeginTrans;
    SChar            sErrMsg[RP_MAX_MSG_LEN + 1] = { 0, };

    IDU_FIT_POINT_RAISE( "rpcDDLSyncManager::setDDLSyncInfoAndTableAccessReadOnly::begin::sBeginTrans", 
                         ERR_TRANS_BEGIN );
    IDE_TEST_RAISE( mResourceMgr->smiTransBegin( &sBeginTrans,
                                                 &sRootStmt,
                                                 sStatistics,
                                                 ( SMI_ISOLATION_NO_PHANTOM     |
                                                   SMI_TRANSACTION_NORMAL       |
                                                   SMI_TRANSACTION_REPL_DEFAULT |
                                                   SMI_COMMIT_WRITE_NOWAIT ) )
                    != IDE_SUCCESS, ERR_TRANS_BEGIN );

    sTable = smiGetTable( sTableOID );
    sSCN = smiGetRowSCN( sTable );

    IDE_TEST( smiValidateAndLockObjects( sBeginTrans,
                                         sTable,
                                         sSCN,
                                         SMI_TBSLV_DDL_DML,
                                         SMI_TABLE_LOCK_IS,
                                         smiGetDDLLockTimeOut(sBeginTrans),
                                         ID_FALSE )
              != IDE_SUCCESS );

    sTableInfo = (qciTableInfo *)rpdCatalog::rpdGetTableTempInfo( sTable );
    IDE_TEST( readyDDLSync( aQciStatement, 
                            sBeginTrans, 
                            sTableInfo,
                            aDDLSyncInfo ) 
              != IDE_SUCCESS );

    IDE_TEST( processDDLSyncMsg( aDDLSyncInfo, RP_DDL_SYNC_BEGIN_MSG ) != IDE_SUCCESS );

    /* Gap 을 제거하기 위하여 Table Access Option 을 Read Only 로 변경한다. */
    IDE_TEST( setTableAccessOption( sBeginTrans,
                                    &( aDDLSyncInfo->mDDLTableInfo ), 
                                    QCI_ACCESS_OPTION_READ_ONLY ) 
              != IDE_SUCCESS );

    IDU_FIT_POINT_RAISE( "rpcDDLSyncManager::setDDLSyncInfoAndTableAccessReadOnly::commit::sBeginTrans", 
                         ERR_TRANS_COMMIT );
    IDE_TEST_RAISE( mResourceMgr->smiTransCommit( sBeginTrans )
                    != IDE_SUCCESS, ERR_TRANS_COMMIT );

    IDE_TEST( processDDLSyncMsgAck( aDDLSyncInfo, 
                                    RP_DDL_SYNC_BEGIN_MSG,
                                    RP_DDL_SYNC_SUCCESS,
                                    NULL ) 
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_TRANS_BEGIN );
    {
        IDE_ERRLOG( IDE_RP_0 );
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RP_INTERNAL_ARG, "[DDLSyncManager] Transaction begin failure") );
    }
    IDE_EXCEPTION( ERR_TRANS_COMMIT );
    {
        IDE_ERRLOG( IDE_RP_0 );
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RP_INTERNAL_ARG, "[DDLSyncManager] Transaction commit failure") );
    }
    IDE_EXCEPTION_END;

    idlOS::strncpy( sErrMsg, 
                    ideGetErrorMsg(),
                    RP_MAX_MSG_LEN );

    return IDE_FAILURE;
}

IDE_RC rpcDDLSyncManager::setDDLSyncInfoAndTableAccessReadOnlyInternal( idvSQL              * aStatistics, 
                                                                        cmiProtocolContext  * aProtocolContext,
                                                                        idBool              * aExitFlag,
                                                                        rpdVersion          * aVersion,
                                                                        SChar               * aRepName,
                                                                        rpcDDLSyncInfo      * aDDLSyncInfo, 
                                                                        SChar              ** aSql )
{
    smiTrans       * sBeginTrans  = NULL;
    smiStatement   * sRootStmt    = NULL;

    IDU_FIT_POINT_RAISE( "rpcDDLSyncManager::setDDLSyncInfoAndTableAccessReadOnlyInternal::begin::sBeginTrans", 
                         ERR_TRANS_BEGIN );
    IDE_TEST_RAISE( mResourceMgr->smiTransBegin( &sBeginTrans,
                                                 &sRootStmt,
                                                 aStatistics,
                                                 ( SMI_ISOLATION_NO_PHANTOM     |
                                                   SMI_TRANSACTION_NORMAL       |
                                                   SMI_TRANSACTION_REPL_DEFAULT |
                                                   SMI_COMMIT_WRITE_NOWAIT ) )
                    != IDE_SUCCESS, ERR_TRANS_BEGIN );

    IDE_TEST( readyDDLSyncInternal( aProtocolContext,
                                    sBeginTrans,
                                    aExitFlag,
                                    aVersion,
                                    aRepName,
                                    aDDLSyncInfo,
                                    aSql )
              != IDE_SUCCESS );

    IDE_TEST( processDDLSyncMsg( aDDLSyncInfo, RP_DDL_SYNC_BEGIN_MSG ) != IDE_SUCCESS );

    IDE_TEST( setTableAccessOption( sBeginTrans,
                                    &( aDDLSyncInfo->mDDLTableInfo ), 
                                    QCI_ACCESS_OPTION_READ_ONLY ) 
              != IDE_SUCCESS );

    IDU_FIT_POINT_RAISE( "rpcDDLSyncManager::setDDLSyncInfoAndTableAccessReadOnlyInternal::commit::sBeginTrans", 
                         ERR_TRANS_BEGIN );
    IDE_TEST_RAISE( mResourceMgr->smiTransCommit( sBeginTrans ) 
                    != IDE_SUCCESS, ERR_TRANS_COMMIT );    

    IDE_TEST( processDDLSyncMsgAck( aDDLSyncInfo, 
                                    RP_DDL_SYNC_BEGIN_MSG,
                                    RP_DDL_SYNC_SUCCESS,
                                    NULL )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_TRANS_BEGIN );
    {
        IDE_ERRLOG( IDE_RP_0 );
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RP_INTERNAL_ARG, "[DDLSyncManager] Transaction begin failure") );
    }
    IDE_EXCEPTION( ERR_TRANS_COMMIT );
    {
        IDE_ERRLOG( IDE_RP_0 );
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RP_INTERNAL_ARG, "[DDLSyncManager] Transaction commit failure") );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpcDDLSyncManager::setSkipUpdateXSN( rpcDDLSyncInfo * aDDLSyncInfo, idBool aIsSkip )
{
    iduListNode    * sNode     = NULL;
    rpcDDLReplInfo * sInfo     = NULL;

    IDE_DASSERT( aDDLSyncInfo != NULL );

    IDU_LIST_ITERATE( &( aDDLSyncInfo->mDDLReplInfoList ), sNode )
    {
        sInfo = (rpcDDLReplInfo*)sNode->mObj;

        IDE_TEST( rpcManager::setSkipUpdateXSN( sInfo->mRepName, aIsSkip ) 
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpcDDLSyncManager::waitLastProcessedSN( rpcDDLSyncInfo * aDDLSyncInfo )
{
    iduListNode    * sNode     = NULL;
    rpcDDLReplInfo * sInfo     = NULL;
    smSN             sLastSN   = SM_SN_NULL;

    IDE_DASSERT( aDDLSyncInfo != NULL );
    IDE_DASSERT( IDU_LIST_IS_EMPTY( &( aDDLSyncInfo->mDDLReplInfoList ) ) != ID_TRUE );

    IDE_TEST( processDDLSyncMsg( aDDLSyncInfo, RP_DDL_SYNC_WAIT_DML_FOR_DDL_MSG ) != IDE_SUCCESS );

    IDE_TEST( smiGetLastValidGSN( &sLastSN ) != IDE_SUCCESS );
    IDE_DASSERT( sLastSN != SM_SN_NULL );

    IDU_LIST_ITERATE( &( aDDLSyncInfo->mDDLReplInfoList ), sNode )
    {
        sInfo = (rpcDDLReplInfo*)sNode->mObj;

        IDE_TEST( rpcManager::waitLastProcessedSN( sInfo->mStatistics,
                                                   sInfo->mDDLSyncExitFlag,
                                                   sInfo->mRepName,
                                                   sLastSN )
                  != IDE_SUCCESS );
    }

    IDE_TEST( processDDLSyncMsgAck( aDDLSyncInfo, 
                                    RP_DDL_SYNC_WAIT_DML_FOR_DDL_MSG,
                                    RP_DDL_SYNC_SUCCESS,
                                    NULL ) 
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
 
    return IDE_FAILURE;
}

IDE_RC rpcDDLSyncManager::setDDLSyncRollbackInfoAndTableAccessRecoverCommon( smiTrans       * aDDLTrans, 
                                                                             rpcDDLSyncInfo * aDDLSyncInfo )
{
    IDU_FIT_POINT_RAISE( "rpcDDLSyncManager::setDDLSyncRollbackInfoAndTableAccessRecoverCommon::savepoint::aDDLTrans", 
                         ERR_SET_SVP );
    IDE_TEST_RAISE( aDDLTrans->savepoint( RP_DDL_SYNC_SAVEPOINT_NAME ) != IDE_SUCCESS, ERR_SET_SVP );
    aDDLSyncInfo->mIsSetSavePoint = ID_TRUE;

    IDE_TEST( ddlSyncTableLock( aDDLTrans, 
                                aDDLSyncInfo,
                                smiGetDDLLockTimeOut(aDDLTrans) )
       
              != IDE_SUCCESS );

    IDE_TEST( aDDLTrans->setExpSvpForBackupDDLTargetTableInfo( aDDLSyncInfo->mDDLTableInfo.mTableOID, 
                                                               0,
                                                               NULL,
                                                               SM_OID_NULL,
                                                               0,
                                                               NULL ) 
              != IDE_SUCCESS );

    IDE_TEST( setTableAccessOption( aDDLTrans,
                                    &( aDDLSyncInfo->mDDLTableInfo ), 
                                    QCI_ACCESS_OPTION_NONE ) 
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_SET_SVP );
    {
        IDE_ERRLOG( IDE_RP_0 );
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RP_INTERNAL_ARG, 
                                  "[DDLSyncManager] smiTrans set(savepoint) error in run()") );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpcDDLSyncManager::setDDLSyncRollbackInfoAndTableAccessRecover( smiTrans       * aDDLTrans, 
                                                                       rpcDDLSyncInfo * aDDLSyncInfo )
{
    IDE_TEST( processDDLSyncMsg( aDDLSyncInfo, RP_DDL_SYNC_READY_EXECUTE_MSG ) != IDE_SUCCESS );

    IDE_TEST( setDDLSyncRollbackInfoAndTableAccessRecoverCommon( aDDLTrans, aDDLSyncInfo ) != IDE_SUCCESS );

    IDE_TEST( processDDLSyncMsgAck( aDDLSyncInfo, 
                                    RP_DDL_SYNC_READY_EXECUTE_MSG,
                                    RP_DDL_SYNC_SUCCESS,
                                    NULL )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpcDDLSyncManager::setDDLSyncRollbackInfoAndTableAccessRecoverInternal( idvSQL         * aStatistics,
                                                                               smiTrans       * aDDLTrans, 
                                                                               rpcDDLSyncInfo * aDDLSyncInfo )
{
    smiStatement * sRootStmt   = NULL;

    IDE_TEST( processDDLSyncMsg( aDDLSyncInfo, RP_DDL_SYNC_READY_EXECUTE_MSG ) != IDE_SUCCESS );

    IDU_FIT_POINT_RAISE( "rpcDDLSyncManager::setDDLSyncRollbackInfoAndTableAccessRecoverInternal::begin::aDDLTrans", 
                         ERR_TRANS_BEGIN );
    IDE_TEST_RAISE( aDDLTrans->begin( &sRootStmt,
                                      aStatistics,
                                      ( SMI_ISOLATION_NO_PHANTOM     |
                                        SMI_TRANSACTION_NORMAL       |
                                        SMI_TRANSACTION_REPL_DEFAULT |
                                        SMI_COMMIT_WRITE_NOWAIT) )
                    != IDE_SUCCESS, ERR_TRANS_BEGIN );

    IDE_TEST( setDDLSyncRollbackInfoAndTableAccessRecoverCommon( aDDLTrans, aDDLSyncInfo ) != IDE_SUCCESS );

    IDE_TEST( processDDLSyncMsgAck( aDDLSyncInfo, 
                                    RP_DDL_SYNC_READY_EXECUTE_MSG,
                                    RP_DDL_SYNC_SUCCESS,
                                    NULL )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_TRANS_BEGIN );
    {
        IDE_ERRLOG( IDE_RP_0 );
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RP_INTERNAL_ARG, "[DDLSyncManager] Transaction begin failure") );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpcDDLSyncManager::readyDDLSyncCommon( cmiProtocolContext * aProtocolContext,
                                              smiTrans           * aTrans, 
                                              qciTableInfo       * aTableInfo,
                                              UInt                 aDDLSyncPartInfoCount,
                                              SChar                aDDLSyncPartInfoNames[][QC_MAX_OBJECT_NAME_LEN + 1],
                                              rpcDDLSyncInfo     * aDDLSyncInfo,
                                              SChar              * aRepName,
                                              UInt                 aRepCount )
{
    IDE_TEST( qciMisc::checkRollbackAbleDDLEnable( aTrans, 
                                                   aTableInfo->tableOID,
                                                   ID_TRUE )
              != IDE_SUCCESS );

    IDE_TEST( setDDLSyncInfo( aTrans,
                              aDDLSyncInfo,
                              aTableInfo,
                              aDDLSyncPartInfoCount,
                              aDDLSyncPartInfoNames )
              != IDE_SUCCESS );

    IDE_TEST( addToDDLSyncInfoList( aProtocolContext, /* ProtocolContext */
                                    aTrans,
                                    aDDLSyncInfo, 
                                    aRepName,
                                    aRepCount ) 
              != IDE_SUCCESS );
    IDE_DASSERT( IDU_LIST_IS_EMPTY( &( aDDLSyncInfo->mDDLReplInfoList ) ) != ID_TRUE );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpcDDLSyncManager::readyDDLSync( qciStatement   * aQciStatement, 
                                        smiTrans       * aTrans,
                                        qciTableInfo   * aTableInfo,
                                        rpcDDLSyncInfo * aDDLSyncInfo )
{
    UInt    sDDLSyncPartInfoCount  = 0;
    smOID * sDDLSyncPartTableOIDs  = qciMisc::getDDLSrcPartTableOIDArray( aQciStatement, &sDDLSyncPartInfoCount );
    SChar   sDDLSyncPartInfoNames[RP_DDL_INFO_PART_OID_COUNT][QC_MAX_OBJECT_NAME_LEN + 1];

    IDE_TEST_RAISE( QCI_STMTTEXTLEN( aQciStatement ) > RP_DDL_SQL_BUFFER_MAX, ERR_SQL_LENGTH );

    idlOS::memset( sDDLSyncPartInfoNames, 
                   0, 
                   ID_SIZEOF(SChar) * RP_DDL_INFO_PART_OID_COUNT * QC_MAX_OBJECT_NAME_LEN + 1 );

    if ( sDDLSyncPartTableOIDs != NULL )
    {
        IDE_TEST( getPartitionNamesByOID( aTrans,
                                          sDDLSyncPartTableOIDs,
                                          sDDLSyncPartInfoCount,
                                          sDDLSyncPartInfoNames )
                  != IDE_SUCCESS );
    }

    IDE_TEST( readyDDLSyncCommon( NULL,
                                  aTrans,
                                  aTableInfo,
                                  sDDLSyncPartInfoCount,
                                  sDDLSyncPartInfoNames,
                                  aDDLSyncInfo,
                                  NULL,
                                  RPU_REPLICATION_MAX_COUNT ) 
              != IDE_SUCCESS );

    IDE_TEST( connectForDDLSync( aDDLSyncInfo ) != IDE_SUCCESS );
    ideLog::log( IDE_RP_0, "[DDLSyncManager] Connect remote success" );

    IDE_TEST( sendDDLSyncInfo( aDDLSyncInfo,
                               QCI_STMTTEXT( aQciStatement ),
                               sDDLSyncPartInfoCount,
                               sDDLSyncPartInfoNames )
              != IDE_SUCCESS );
    
    IDE_TEST( recvDDLSyncInfoAck( aDDLSyncInfo ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_SQL_LENGTH );
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RP_DDL_SYNC_SQL_LENGTH_ERROR, RP_DDL_SQL_BUFFER_MAX ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpcDDLSyncManager::readyDDLSyncInternal( cmiProtocolContext  * aProtocolContext,
                                                smiTrans            * aTrans,
                                                idBool              * aExitFlag,
                                                rpdVersion          * aVersion,
                                                SChar               * aRepName,
                                                rpcDDLSyncInfo      * aDDLSyncInfo,
                                                SChar              ** aSql )
{
    idBool           sDoSendAck   = ID_FALSE;
    qciTableInfo   * sTableInfo   = NULL;
    SChar  sErrMsg[RP_MAX_MSG_LEN + 1] = { 0, };
    SChar  sUserName[QC_MAX_OBJECT_NAME_LEN + 1 ]   = { 0, };
    SChar  sTableName[QC_MAX_OBJECT_NAME_LEN + 1 ]  = { 0, };
    UInt   sDDLSyncPartInfoCount = 0;
    SChar  sDDLSyncPartInfoNames[RP_DDL_INFO_PART_OID_COUNT][QC_MAX_OBJECT_NAME_LEN + 1];

    idlOS::memset( sDDLSyncPartInfoNames, 
                   0, 
                   ID_SIZEOF(SChar) * RP_DDL_INFO_PART_OID_COUNT * QC_MAX_OBJECT_NAME_LEN + 1 );

    sDoSendAck = ID_TRUE;
    IDE_TEST( recvDDLSyncInfo( aProtocolContext,
                               aExitFlag,
                               aVersion,
                               aRepName,
                               sUserName,
                               sTableName,
                               &sDDLSyncPartInfoCount,
                               sDDLSyncPartInfoNames,
                               aSql )
              != IDE_SUCCESS );
    ideLog::log( IDE_RP_0, "[DDLSyncManager] Receive <%s> DDL query : %s", aRepName, *aSql );

    IDE_TEST( getTableInfo( aTrans->getStatistics(),
                            aTrans,
                            sTableName, 
                            sUserName,
                            smiGetDDLLockTimeOut(aTrans),
                            &sTableInfo )
              != IDE_SUCCESS );

    IDE_TEST( readyDDLSyncCommon( aProtocolContext,
                                  aTrans,
                                  sTableInfo,
                                  sDDLSyncPartInfoCount,
                                  sDDLSyncPartInfoNames,
                                  aDDLSyncInfo,
                                  aRepName,
                                  1 ) 
              != IDE_SUCCESS );

    sDoSendAck = ID_FALSE;
    IDE_TEST( sendDDLSyncInfoAck( aProtocolContext, 
                                  aExitFlag,
                                  RP_DDL_SYNC_SUCCESS,
                                  NULL )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    idlOS::strncpy( sErrMsg, 
                    ideGetErrorMsg(),
                    RP_MAX_MSG_LEN );
    sErrMsg[RP_MAX_MSG_LEN] = '\0';

    IDE_PUSH();

    if ( sDoSendAck == ID_TRUE )
    {
        sDoSendAck = ID_FALSE;
        (void)sendDDLSyncInfoAck( aProtocolContext, 
                                  aExitFlag,
                                  RP_DDL_SYNC_FAILURE,
                                  sErrMsg );
    }
    else
    {
        /* nothing to do */
    }

    IDE_POP();

    return IDE_FAILURE;
}

IDE_RC rpcDDLSyncManager::buildReceiverNewMeta( smiTrans * aTrans, rpcDDLSyncInfo * aDDLSyncInfo )
{
    iduListNode    * sNode  = NULL;
    rpcDDLReplInfo * sInfo  = NULL;

    smiStatement * sStatement         = NULL;
    idBool         sIsBegin           = ID_FALSE;

    IDE_DASSERT( aDDLSyncInfo != NULL );
    IDE_DASSERT( IDU_LIST_IS_EMPTY( &( aDDLSyncInfo->mDDLReplInfoList ) ) != ID_TRUE );

    IDU_LIST_ITERATE( &( aDDLSyncInfo->mDDLReplInfoList ), sNode )
    {
        sInfo = (rpcDDLReplInfo*)sNode->mObj;

        IDE_TEST( mResourceMgr->smiStmtBegin( &sStatement,
                                              sInfo->mStatistics,
                                              aTrans->getStatement(),
                                              SMI_STATEMENT_NORMAL | SMI_STATEMENT_ALL_CURSOR ) 
                  != IDE_SUCCESS );
        sIsBegin = ID_TRUE;

        IDE_TEST( rpcManager::buildReceiverNewMeta( sStatement, sInfo->mRepName ) != IDE_SUCCESS );

        IDE_TEST( mResourceMgr->smiStmtEnd( sStatement, SMI_STATEMENT_RESULT_SUCCESS ) != IDE_SUCCESS );
        sIsBegin = ID_FALSE;
    }

    ideLog::log( IDE_RP_0, "[DDLSyncManager] Create receiver new meta success" );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();

    if ( sIsBegin == ID_TRUE )
    {
        (void)mResourceMgr->smiStmtEnd( sStatement, SMI_STATEMENT_RESULT_FAILURE );
    }
    
    IDE_POP();

    ideLog::log( IDE_RP_0, "[DDLSyncManager] Create receiver new meta failure" );

    return IDE_FAILURE;
}

IDE_RC rpcDDLSyncManager::ddlSyncRollback( smiTrans * aDDLTrans, rpcDDLSyncInfo * aDDLSyncInfo )
{

    if ( aDDLTrans->isBegin() == ID_TRUE )
    {
        if ( aDDLSyncInfo->mIsSetSavePoint == ID_TRUE )
        {
            IDE_TEST( aDDLTrans->rollback( RP_DDL_SYNC_SAVEPOINT_NAME ) != IDE_SUCCESS );
            ideLog::log( IDE_RP_0, "[DDLSyncManager] Rollback to savepoint <%s> success", RP_DDL_SYNC_SAVEPOINT_NAME );
        }
        
        if ( aDDLSyncInfo->mDDLSyncInfoType != RP_MASTER_DDL )
        {
            IDE_ASSERT( aDDLTrans->rollback() == IDE_SUCCESS );
            ideLog::log( IDE_RP_0, "[DDLSyncManager] Rollback  success" );
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    ideLog::log( IDE_RP_0, "[DDLSyncManager] Rollback to savepoint <%s> failure", RP_DDL_SYNC_SAVEPOINT_NAME );

    IDE_PUSH();

    if ( aDDLSyncInfo->mDDLSyncInfoType != RP_MASTER_DDL ) 
    {
        IDE_ASSERT( aDDLTrans->rollback() == IDE_SUCCESS );
        ideLog::log( IDE_RP_0, "[DDLSyncManager] Rollback  success" );
    }
    else
    {
        /* nothing to do */
    }

    IDE_POP();
    return IDE_FAILURE;
}

IDE_RC rpcDDLSyncManager::addToDDLSyncInfoList( cmiProtocolContext * aProtocolContext,
                                                smiTrans           * aTrans,
                                                rpcDDLSyncInfo     * aDDLSyncInfo, 
                                                SChar              * aRepName,
                                                SInt                 aMaxReplCount )
{
    idBool sIsLock        = ID_FALSE;

    IDE_DASSERT( aDDLSyncInfo != NULL );

    IDE_TEST( createAndAddDDLReplInfoList( aProtocolContext,
                                           aTrans,
                                           aDDLSyncInfo, 
                                           aRepName,
                                           aMaxReplCount ) 
              != IDE_SUCCESS );
    IDE_DASSERT( IDU_LIST_IS_EMPTY( &( aDDLSyncInfo->mDDLReplInfoList ) ) != ID_TRUE );

    IDE_ASSERT( mDDLSyncInfoListLock.lock( NULL ) == IDE_SUCCESS );    
    sIsLock = ID_TRUE;

    IDE_TEST( checkAlreadyDDLSync( aTrans, aDDLSyncInfo ) != IDE_SUCCESS );

    IDU_LIST_ADD_LAST( &mDDLSyncInfoList, &( aDDLSyncInfo->mNode ) );

    sIsLock = ID_FALSE;
    IDE_ASSERT( mDDLSyncInfoListLock.unlock() == IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sIsLock == ID_TRUE )
    {
        sIsLock = ID_FALSE;
        IDE_ASSERT( mDDLSyncInfoListLock.unlock() == IDE_SUCCESS );    
    }

    return IDE_FAILURE;
}

void rpcDDLSyncManager::removeFromDDLSyncInfoList( rpcDDLSyncInfo * aDDLSyncInfo )
{
    IDE_ASSERT( mDDLSyncInfoListLock.lock( NULL ) == IDE_SUCCESS );    

    IDU_LIST_REMOVE( &( aDDLSyncInfo->mNode ) );

    IDE_ASSERT( mDDLSyncInfoListLock.unlock() == IDE_SUCCESS );
}


IDE_RC rpcDDLSyncManager::connectForDDLSync( rpcDDLSyncInfo * aDDLSyncInfo )
{
    iduListNode    * sNode  = NULL;
    rpcDDLReplInfo * sInfo  = NULL;

    IDE_DASSERT( aDDLSyncInfo != NULL );
    IDE_DASSERT( IDU_LIST_IS_EMPTY( &( aDDLSyncInfo->mDDLReplInfoList ) ) != ID_TRUE );

    IDU_LIST_ITERATE( &( aDDLSyncInfo->mDDLReplInfoList ), sNode )
    {
        sInfo = (rpcDDLReplInfo*)sNode->mObj;

        IDE_TEST( connect( sInfo, &( sInfo->mProtocolContext ) ) != IDE_SUCCESS );
    }
        
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpcDDLSyncManager::connect( rpcDDLReplInfo * aInfo, cmiProtocolContext ** aProtocolContext )
{
    cmiLink            * sLink                   = NULL;
    cmiConnectArg        sConnectArg;
    rpMsgReturn          sResult                 = RP_MSG_DISCONNECT;
    SChar                sBuffer[RP_ACK_MSG_LEN] = { 0, };
    cmiProtocolContext * sProtocolContext        = NULL;
    PDL_Time_Value       sConnWaitTime;
    rpdReplications      sDummyRepl;
    UInt                 sDummyMsgLen            = 0;

    idlOS::memset( &sConnectArg, 0, ID_SIZEOF(cmiConnectArg) );

    IDU_FIT_POINT_RAISE( "rpcDDLSyncManager::connect::malloc::sProtocolContext", ERR_MEMORY_ALLOC_CONTEXT );
    IDE_TEST_RAISE( mResourceMgr->rpCalloc( ID_SIZEOF( cmiProtocolContext ), (void**)&sProtocolContext )
                    != IDE_SUCCESS, ERR_MEMORY_ALLOC_CONTEXT );

    IDE_TEST( cmiMakeCmBlockNull( sProtocolContext ) != IDE_SUCCESS );

    if ( aInfo->mConnType == RP_SOCKET_TYPE_TCP )
    {
        sConnectArg.mTCP.mAddr = aInfo->mHostIp;
        sConnectArg.mTCP.mPort = aInfo->mPortNo;
        sConnectArg.mTCP.mBindAddr = NULL;

        IDU_FIT_POINT_RAISE( "rpcDDLSyncManager::connect::cmiAllocLink::sLink", ERR_ALLOC_LINK );
        IDE_TEST_RAISE( cmiAllocLink( &sLink, 
                                      CMI_LINK_TYPE_PEER_CLIENT, 
                                      CMI_LINK_IMPL_TCP )
                        != IDE_SUCCESS, ERR_ALLOC_LINK );
    }
    else if ( aInfo->mConnType == RP_SOCKET_TYPE_IB )
    {   
        sConnectArg.mIB.mAddr = aInfo->mHostIp;
        sConnectArg.mIB.mPort = aInfo->mPortNo;
        sConnectArg.mIB.mLatency = aInfo->mIBLatency;
        sConnectArg.mIB.mBindAddr = NULL;

        IDU_FIT_POINT_RAISE( "rpcDDLSyncManager::connect::cmiAllocLink::sLink", ERR_ALLOC_LINK );
        IDE_TEST_RAISE( cmiAllocLink( &sLink, 
                                      CMI_LINK_TYPE_PEER_CLIENT, 
                                      CMI_LINK_IMPL_IB )
                        != IDE_SUCCESS, ERR_ALLOC_LINK );
    }
    else
    {
        IDE_DASSERT( 0 );
    }
    
    IDU_FIT_POINT_RAISE( "rpcDDLSyncManager::connect::cmiAllocCmBlock::sLink", ERR_ALLOC_CM_BLOCK );
    IDE_TEST_RAISE( cmiAllocCmBlock( sProtocolContext,
                                     CMI_PROTOCOL_MODULE( RP ),
                                     (cmiLink *)sLink,
                                     this ) 
                    != IDE_SUCCESS, ERR_ALLOC_CM_BLOCK );

    sConnWaitTime.initialize( RPU_REPLICATION_CONNECT_TIMEOUT, 0 );

    IDE_TEST( cmiConnectWithoutData( sProtocolContext,
                                     &sConnectArg,
                                     &sConnWaitTime,
                                     SO_REUSEADDR )
              != IDE_SUCCESS );

    /* BUG-46528 V6Protocol. DDLSync 는 V6 Protocol 을 지원하지 않기 때문에 항상 Unset 시킨다.  */
    idlOS::memset( &sDummyRepl, 0, ID_SIZEOF(rpdReplications) );
    sDummyRepl.mOptions = sDummyRepl.mOptions | RP_OPTION_V6_PROTOCOL_UNSET;
    IDE_TEST( rpcManager::checkRemoteNormalReplVersion( sProtocolContext, 
                                                        aInfo->mDDLSyncExitFlag,
                                                        &sDummyRepl,
                                                        &sResult, 
                                                        sBuffer,
                                                        &sDummyMsgLen )
              != IDE_SUCCESS );

    switch( sResult )
    {
        case RP_MSG_DISCONNECT :
            IDE_SET( ideSetErrorCode( rpERR_ABORT_HANDSHAKE_DISCONNECT,
                                      "Replication Protocol Version" ) );
            IDE_ERRLOG( IDE_RP_0 );
            break;

        case RP_MSG_PROTOCOL_DIFF :
            IDE_SET( ideSetErrorCode( rpERR_IGNORE_RP_PROTOCOL_DIFF ) );
            IDE_ERRLOG( IDE_RP_0 );
            break;

        case RP_MSG_OK :
            if ( aInfo->mConnType == RP_SOCKET_TYPE_TCP )
            {
                IDE_TEST( rpcHBT::registHost( &( aInfo->mHBT ),
                                              aInfo->mHostIp,
                                              aInfo->mPortNo )
                          != IDE_SUCCESS );
            }
            break;

        default :
            IDE_ASSERT( 0 );
    }

    *aProtocolContext = sProtocolContext;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_MEMORY_ALLOC_CONTEXT );
    {
        IDE_ERRLOG( IDE_RP_0 );
        IDE_SET( ideSetErrorCode( rpERR_ABORT_MEMORY_ALLOC,
                                  "rpcDDLSyncManager::connect",
                                  "sProtocolContext" ) );
    }
    IDE_EXCEPTION( ERR_ALLOC_LINK );
    {
        IDE_ERRLOG( IDE_RP_0 );
        IDE_SET( ideSetErrorCode( rpERR_ABORT_ALLOC_LINK ) );
    }
    IDE_EXCEPTION( ERR_ALLOC_CM_BLOCK );
    {
        IDE_ERRLOG( IDE_RP_0 );
        IDE_SET( ideSetErrorCode( rpERR_ABORT_ALLOC_CM_BLOCK ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpcDDLSyncManager::checkDDLSyncRemoteVersion( rpdVersion * aVersion )
{
    IDU_FIT_POINT_RAISE( "rpcDDLSyncManager::checkDDLSyncRemoteVersion::RP_GET_MAJOR_VERSION::mVersion",
                      ERR_PROTOCOL );
    IDE_TEST_RAISE( RP_GET_MAJOR_VERSION( aVersion->mVersion ) != REPLICATION_MAJOR_VERSION, 
                    ERR_PROTOCOL );
    
    IDU_FIT_POINT_RAISE( "rpcDDLSyncManager::checkDDLSyncRemoteVersion::RP_GET_MINOR_VERSION::mVersion",
                      ERR_PROTOCOL );
    IDE_TEST_RAISE( RP_GET_MINOR_VERSION( aVersion->mVersion ) != REPLICATION_MINOR_VERSION, 
                    ERR_PROTOCOL );

    IDU_FIT_POINT_RAISE( "rpcDDLSyncManager::checkDDLSyncRemoteVersion::RP_GET_FIX_VERSION::mVersion",
                      ERR_PROTOCOL );
    IDE_TEST_RAISE( RP_GET_FIX_VERSION( aVersion->mVersion ) != REPLICATION_FIX_VERSION, 
                    ERR_PROTOCOL );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_PROTOCOL );
    {
        IDE_SET( ideSetErrorCode( rpERR_IGNORE_RP_PROTOCOL_DIFF ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpcDDLSyncManager::setDDLSyncInfo( smiTrans       * aTrans, 
                                          rpcDDLSyncInfo * aDDLSyncInfo, 
                                          qciTableInfo   * aTableInfo,
                                          UInt             aDDLSyncPartInfoCount,
                                          SChar            aDDLSyncPartInfoNames[][QC_MAX_OBJECT_NAME_LEN + 1] )
{
    iduList                sDDLSyncPartInfoList;
    UInt                   sTablePartInfoCount   = 0;

    IDU_LIST_INIT( &sDDLSyncPartInfoList );

    if ( aTableInfo->tablePartitionType == QCM_PARTITIONED_TABLE )
    {
        IDE_TEST( getDDLSyncPartInfoList( aTrans,
                                          aTableInfo->tableID,
                                          &sTablePartInfoCount,
                                          aDDLSyncPartInfoCount,
                                          aDDLSyncPartInfoNames,
                                          &sDDLSyncPartInfoList )
                  != IDE_SUCCESS );
    }
    else
    {
        /* nothing to do */
    }

    setDDLTableInfo( &( aDDLSyncInfo->mDDLTableInfo ),
                     aTableInfo,
                     sTablePartInfoCount,
                     &sDDLSyncPartInfoList );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

void rpcDDLSyncManager::setDDLTableInfo( rpcDDLTableInfo * aDDLTableInfo,
                                         qciTableInfo    * aTableInfo,
                                         UInt              aTablePartInfoCount,
                                         iduList         * aDDLSyncPartInfoList )
{
    aDDLTableInfo->mTableID       = aTableInfo->tableID;
    aDDLTableInfo->mTableOID      = aTableInfo->tableOID;
    aDDLTableInfo->mAccessOption  = (qciAccessOption)( aTableInfo->accessOption );

    idlOS::strncpy( aDDLTableInfo->mTableName, 
                    aTableInfo->name, 
                    QC_MAX_OBJECT_NAME_LEN + 1 );

    idlOS::strncpy( aDDLTableInfo->mUserName, 
                    aTableInfo->tableOwnerName, 
                    QC_MAX_OBJECT_NAME_LEN + 1 );

    aDDLTableInfo->mPartInfoCount = aTablePartInfoCount;
    aDDLTableInfo->mIsDDLSyncTable = ID_FALSE;

    if ( IDU_LIST_IS_EMPTY( aDDLSyncPartInfoList ) != ID_TRUE )
    {
        IDU_LIST_JOIN_LIST( &( aDDLTableInfo->mPartInfoList ), aDDLSyncPartInfoList );
    }
    else
    {
        /* nothing to do */
    }
}

rpxDDLExecutor * rpcDDLSyncManager::findDDLExecutorByNameWithoutLock( SChar * aRepName )
{
    iduListNode    * sExecutorNode = NULL;
    rpxDDLExecutor * sExecutor     = NULL;

    IDE_DASSERT( aRepName != NULL );

    IDU_LIST_ITERATE( &mDDLExecutorList, sExecutorNode )
    {
        sExecutor = (rpxDDLExecutor*)sExecutorNode->mObj;
        if( idlOS::strncmp( sExecutor->getRepName(),
                            aRepName,
                            QC_MAX_NAME_LEN )
            == 0 )
        {
            break;
        }
        else
        {
            sExecutor = NULL;
        }
    }
        
    return sExecutor;
}

rpcDDLSyncInfo * rpcDDLSyncManager::findDDLSyncInfoByTableOIDWithoutLock( smOID aTableOID )
{
    idBool            sIsFind   = ID_FALSE;
    iduListNode     * sSyncNode = NULL;
    rpcDDLSyncInfo  * sSyncInfo = NULL;
    rpcDDLSyncInfo  * sFindInfo = NULL;
    iduListNode     * sTblNode  = NULL;
    rpcDDLTableInfo * sTblInfo  = NULL;

    IDU_LIST_ITERATE( &mDDLSyncInfoList, sSyncNode )
    {
        sSyncInfo = (rpcDDLSyncInfo*)sSyncNode->mObj;

        if ( sSyncInfo->mDDLTableInfo.mTableOID == aTableOID )
        {
            sIsFind = ID_TRUE;
        }
        else
        {
            IDU_LIST_ITERATE( &( sSyncInfo->mDDLTableInfo.mPartInfoList ), sTblNode )
            {
                sTblInfo = (rpcDDLTableInfo*)sTblNode->mObj;
                if ( sTblInfo->mTableOID == aTableOID )
                {
                    sIsFind = ID_TRUE;
                    break;
                }
            }
        }

        if ( sIsFind == ID_TRUE )
        {
            sFindInfo = sSyncInfo;
            break;
        }
    }

    return sFindInfo;
}

rpcDDLReplInfo * rpcDDLSyncManager::findDDLReplInfoByName( SChar * aRepName )
{
    rpcDDLReplInfo * sFindInfo = NULL;

    IDE_ASSERT( mDDLSyncInfoListLock.lock( NULL ) == IDE_SUCCESS );    

    sFindInfo =  findDDLReplInfoByNameWithoutLock( aRepName );
        
    IDE_ASSERT( mDDLSyncInfoListLock.unlock() == IDE_SUCCESS );    

    return sFindInfo;
}

rpcDDLReplInfo * rpcDDLSyncManager::findDDLReplInfoByNameWithoutLock( SChar * aRepName )
{
    iduListNode    * sSyncNode = NULL;
    rpcDDLSyncInfo * sSyncInfo = NULL;
    iduListNode    * sReplNode = NULL;
    rpcDDLReplInfo * sReplInfo = NULL;
    rpcDDLReplInfo * sFindInfo = NULL;

    IDU_LIST_ITERATE( &mDDLSyncInfoList, sSyncNode )
    {
        sSyncInfo = (rpcDDLSyncInfo*)sSyncNode->mObj;

        IDU_LIST_ITERATE( &( sSyncInfo->mDDLReplInfoList ), sReplNode )
        {
            sReplInfo = (rpcDDLReplInfo*)sReplNode->mObj;
            if( idlOS::strncmp( sReplInfo->mRepName,
                                aRepName,
                                QC_MAX_NAME_LEN )
                == 0 )
            {
                sFindInfo = sReplInfo;

                break;
            }
            else
            {
                /* nothing to do */
            }
        }

        if ( sFindInfo != NULL )
        {
            break;
        }
        else
        {
            /* nothing to do */
        }
    }
        
    return sFindInfo;
}

void rpcDDLSyncManager::setIsBuildNewMeta( smOID aTableOID, idBool aIsBuild )
{
    rpcDDLSyncInfo * sDDLSyncInfo = NULL;

    IDE_ASSERT( mDDLSyncInfoListLock.lock( NULL ) == IDE_SUCCESS );    

    sDDLSyncInfo = findDDLSyncInfoByTableOIDWithoutLock( aTableOID );
    if ( sDDLSyncInfo != NULL )
    {
        sDDLSyncInfo->mIsBuildNewMeta = aIsBuild;
    }
    else
    {
        /* nothing to do */
    }
    
    IDE_ASSERT( mDDLSyncInfoListLock.unlock() == IDE_SUCCESS );    
}

IDE_RC rpcDDLSyncManager::checkAlreadyDDLSync( smiTrans * aTrans, rpcDDLSyncInfo * aDDLSyncInfo )
{
    rpdReplications   sReplication;
    smiStatement    * sSmiStmt      = NULL;
    SInt              sItemIndex    = 0;
    iduListNode     * sNode         = NULL;
    rpcDDLReplInfo  * sInfo         = NULL;
    rpdMetaItem     * sReplItems    = NULL;
    rpcDDLSyncInfo  * sDDLSyncInfo  = NULL;
    UInt              sRepLen       = 0;
    UInt              sMsgBufferLen = 0;
    SChar             sMsgBuffer[RP_MAX_MSG_LEN + 1] = { 0, };
    smiTableCursor  * sCursor       = NULL;
    idBool            sIsBegin      = ID_FALSE;

    IDE_DASSERT( aDDLSyncInfo != NULL );
    IDE_DASSERT( IDU_LIST_IS_EMPTY( &( aDDLSyncInfo->mDDLReplInfoList ) ) != ID_TRUE );

    IDE_TEST( mResourceMgr->smiStmtBegin( &sSmiStmt,
                                          aTrans->getStatistics(),
                                          aTrans->getStatement(),
                                          SMI_STATEMENT_NORMAL |
                                          SMI_STATEMENT_MEMORY_CURSOR )
              != IDE_SUCCESS );
    sIsBegin = ID_TRUE;
    
    IDE_TEST_RAISE( mResourceMgr->rpCalloc( ID_SIZEOF( smiTableCursor ),
                                            (void**)&sCursor )
                    != IDE_SUCCESS, ERR_MEMORY_ALLOC_CURSOR );

    IDU_LIST_ITERATE( &( aDDLSyncInfo->mDDLReplInfoList ), sNode )
    {
        sInfo = (rpcDDLReplInfo*)sNode->mObj;

        IDE_TEST( rpdCatalog::selectReplWithCursor( sSmiStmt,
                                                    sInfo->mRepName,
                                                    &sReplication,
                                                    ID_FALSE,
                                                    sCursor )
                  != IDE_SUCCESS);

        IDE_TEST_RAISE( mResourceMgr->rpCalloc( ( ID_SIZEOF( rpdMetaItem ) * sReplication.mItemCount ), 
                                                (void**)&sReplItems )
                        != IDE_SUCCESS, ERR_MEMORY_ALLOC_META_ITEM );

        IDE_TEST( rpdCatalog::selectReplItemsWithCursor( sSmiStmt,
                                                         sReplication.mRepName,
                                                         sReplItems,
                                                         sReplication.mItemCount,
                                                         sCursor,
                                                         ID_FALSE )
                  != IDE_SUCCESS );
        
        for ( sItemIndex = 0; sItemIndex < sReplication.mItemCount; sItemIndex++ )
        {
            sDDLSyncInfo = findDDLSyncInfoByTableOIDWithoutLock( sReplItems[sItemIndex].mItem.mTableOID );
            IDE_TEST_RAISE( sDDLSyncInfo != NULL, ERR_ALREADY_DDL_SYNC );
        }
    }

    IDE_TEST( mResourceMgr->smiStmtEnd( sSmiStmt, SMI_STATEMENT_RESULT_SUCCESS ) != IDE_SUCCESS );
    sIsBegin = ID_FALSE;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_ALREADY_DDL_SYNC );
    {
        IDU_LIST_ITERATE( &( sDDLSyncInfo->mDDLReplInfoList ), sNode )
        {
            sInfo = (rpcDDLReplInfo*)sNode->mObj;
            sRepLen = strlen( sInfo->mRepName );

            if ( ( sMsgBufferLen + sRepLen + 1 ) <= RP_MAX_MSG_LEN )
            {
                idlOS::strncat( sMsgBuffer, 
                                sInfo->mRepName, 
                                sRepLen );
                idlOS::strncat( sMsgBuffer, 
                                " ",
                                1 );

                sMsgBufferLen = sMsgBufferLen + sRepLen + 1;
            }
            else
            {
                break;
            }
        }
        sMsgBuffer[sMsgBufferLen] = '\0';

        IDE_SET( ideSetErrorCode( rpERR_ABORT_RP_ALREADY_DDL_SYNC, sMsgBuffer ) );
    }
    IDE_EXCEPTION( ERR_MEMORY_ALLOC_META_ITEM );
    {
        IDE_ERRLOG( IDE_RP_0 );
        IDE_SET( ideSetErrorCode( rpERR_ABORT_MEMORY_ALLOC,
                                  "rpcDDLSyncManager::checkAlreadyDDLSyncTable",
                                  "sReplItems" ) );
    }
    IDE_EXCEPTION( ERR_MEMORY_ALLOC_CURSOR );
    {
        IDE_ERRLOG( IDE_RP_0 );
        IDE_SET( ideSetErrorCode( rpERR_ABORT_MEMORY_ALLOC,
                                  "rpcDDLSyncManager::checkAlreadyDDLSyncTable",
                                  "sCursor" ) );
    }

    IDE_EXCEPTION_END;

    IDE_PUSH();

    if ( sIsBegin == ID_TRUE )
    {
        (void)mResourceMgr->smiStmtEnd( sSmiStmt, SMI_STATEMENT_RESULT_FAILURE );
    }

    IDE_POP();

    return IDE_FAILURE;
}

IDE_RC rpcDDLSyncManager::setTableAccessOption( smiTrans        * aTrans,
                                                rpcDDLTableInfo * aDDLTableInfo,
                                                qciAccessOption   aAccessOption ) 
{
    UInt               sUserID = 0;
    iduListNode      * sNode   = NULL;
    rpcDDLTableInfo  * sInfo   = NULL;
    qciAccessOption    sAccessOption = QCM_ACCESS_OPTION_NONE;

    IDU_FIT_POINT("rpcDDLSyncManager::setTableAccessOption::_FT_");

    IDE_TEST( qciMisc::getUserIdByName( aDDLTableInfo->mUserName, &sUserID ) != IDE_SUCCESS );

    if ( IDU_LIST_IS_EMPTY( &( aDDLTableInfo->mPartInfoList ) ) == ID_TRUE )
    {
        if ( aAccessOption != QCI_ACCESS_OPTION_NONE )
        {
            sAccessOption = aAccessOption;
        }
        else
        {
            sAccessOption = aDDLTableInfo->mAccessOption;
        }            

        if ( sAccessOption != QCI_ACCESS_OPTION_NONE )
        {
            IDE_TEST( runAccessTable( aTrans,
                                      sUserID,
                                      aDDLTableInfo->mTableName,
                                      NULL,
                                      sAccessOption ) 
                      != IDE_SUCCESS );

        }
    }
    else
    {
        IDU_LIST_ITERATE( &( aDDLTableInfo->mPartInfoList ), sNode )
        {
            sInfo = (rpcDDLTableInfo*)sNode->mObj;

            if ( sInfo->mIsDDLSyncTable == ID_TRUE )
            {
                if ( aAccessOption != QCI_ACCESS_OPTION_NONE )
                {
                    sAccessOption = aAccessOption;
                }
                else
                {
                    sAccessOption = sInfo->mAccessOption;
                }


                if ( sAccessOption != QCI_ACCESS_OPTION_NONE )
                {
                    IDE_TEST( runAccessTable( aTrans,
                                              sUserID,
                                              aDDLTableInfo->mTableName,
                                              sInfo->mTableName,
                                              sAccessOption ) 
                              != IDE_SUCCESS );

                }
            }
            else
            {
                /* nothing to do */
            }
        }
    }


    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpcDDLSyncManager::recoverTableAccessOption( idvSQL * aStatistics, rpcDDLSyncInfo * aDDLSyncInfo )
{
    smiTrans     * sTrans = NULL;
    smiStatement * sRootStatement  = NULL;


    IDU_FIT_POINT_RAISE( "rpcDDLSyncManager::recoverTableAccessOption::begin::sTrans", ERR_TRANS_BEGIN );
    IDE_TEST_RAISE( mResourceMgr->smiTransBegin( &sTrans,
                                                &sRootStatement,
                                                aStatistics,
                                                ( SMI_ISOLATION_NO_PHANTOM     |
                                                  SMI_TRANSACTION_NORMAL       |
                                                  SMI_TRANSACTION_REPL_NONE    |
                                                  SMI_COMMIT_WRITE_NOWAIT ) )
                    != IDE_SUCCESS, ERR_TRANS_BEGIN );

    IDE_TEST( setTableAccessOption( sTrans,
                                    &( aDDLSyncInfo->mDDLTableInfo ), 
                                    QCI_ACCESS_OPTION_NONE ) 
              != IDE_SUCCESS );

    IDU_FIT_POINT_RAISE( "rpcDDLSyncManager::recoverTableAccessOption::commit::sTrans", ERR_TRANS_COMMIT );
    IDE_TEST_RAISE( mResourceMgr->smiTransCommit( sTrans ) != IDE_SUCCESS, ERR_TRANS_COMMIT );

    ideLog::log( IDE_RP_0, "[DDLSyncManager] Table access option recover success" );

    return IDE_SUCCESS;
    
    IDE_EXCEPTION( ERR_TRANS_BEGIN );
    {
        IDE_ERRLOG( IDE_RP_0 );
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RP_INTERNAL_ARG, "[DDLSyncManager] Transaction begin failure") );
    }
    IDE_EXCEPTION( ERR_TRANS_COMMIT );
    {
        IDE_ERRLOG( IDE_RP_0 );
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RP_INTERNAL_ARG, "[DDLSyncManager] Transaction commit failure") );
    }
    IDE_EXCEPTION_END;

    ideLog::log( IDE_RP_0, "[DDLSyncManager] Table access option recover failrue" );

    return IDE_FAILURE;
}

IDE_RC rpcDDLSyncManager::runAccessTable( smiTrans        * aTrans,
                                          UInt              aUserID,
                                          SChar           * aTableName,
                                          SChar           * aPartName,
                                          qciAccessOption   aAccessOption )
{
    smiStatement * sStatement;
    SChar         sBuffer[1024] = { 0, };
    const SChar * sAccessOption = NULL;
    idBool        sIsBegin      = ID_FALSE;

    switch( aAccessOption )
    {
        case QCI_ACCESS_OPTION_READ_ONLY :
            sAccessOption = "ONLY";
            break;
        case QCI_ACCESS_OPTION_READ_WRITE :
            sAccessOption = "WRITE";
            break;
        case QCI_ACCESS_OPTION_READ_APPEND :
            sAccessOption = "APPEND";
            break;
        default : /* QCI_ACCESS_OPTION_NONE */
            IDE_DASSERT( 0 );
            break;
    }

    if ( aPartName == NULL )
    {
        idlOS::snprintf( sBuffer, ID_SIZEOF(sBuffer),
                         "ALTER TABLE %s ACCESS READ %s",
                         aTableName,
                         sAccessOption );
    }
    else
    {
        idlOS::snprintf( sBuffer, ID_SIZEOF(sBuffer),
                         "ALTER TABLE %s ACCESS PARTITION %s READ %s",
                         aTableName,
                         aPartName,
                         sAccessOption );
    }

    IDE_TEST( mResourceMgr->smiStmtBegin( &sStatement, 
                                          aTrans->getStatistics(),
                                          aTrans->getStatement(),
                                          SMI_STATEMENT_NORMAL | SMI_STATEMENT_ALL_CURSOR ) != IDE_SUCCESS );
    sIsBegin = ID_TRUE;

    IDE_TEST( qciMisc::runDDLforInternal( aTrans->getStatistics(),
                                          sStatement,
                                          aUserID,
                                          QCI_SESSION_INTERNAL_DDL_TRUE,
                                          sBuffer )
              != IDE_SUCCESS );

    IDE_TEST( mResourceMgr->smiStmtEnd( sStatement, SMI_STATEMENT_RESULT_SUCCESS ) != IDE_SUCCESS );
    sIsBegin = ID_FALSE;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();

    if ( sIsBegin == ID_TRUE )
    {
        (void)mResourceMgr->smiStmtEnd( sStatement, SMI_STATEMENT_RESULT_FAILURE );
    }

    IDE_POP();

    return IDE_FAILURE;
}

IDE_RC rpcDDLSyncManager::runDDL( idvSQL   * aStatistics,
                                  SChar    * aSql,
                                  SChar    * aUserName,
                                  smiStatement * aSmiStmt )
{
    UInt         sUserID           = 0;

    IDE_TEST( qciMisc::getUserIdByName( aUserName, &sUserID ) != IDE_SUCCESS );
   
    IDE_TEST( qciMisc::runDDLforInternal( aStatistics,
                                          aSmiStmt,
                                          sUserID,
                                          QCI_SESSION_INTERNAL_DDL_SYNC_TRUE,
                                          aSql )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

void rpcDDLSyncManager::removeReceiverNewMeta( rpcDDLSyncInfo * aDDLSyncInfo )
{
    iduListNode    * sNode  = NULL;
    rpcDDLReplInfo * sInfo  = NULL;

    IDE_DASSERT( aDDLSyncInfo != NULL );

    if ( aDDLSyncInfo->mIsBuildNewMeta == ID_TRUE )
    {
        IDU_LIST_ITERATE( &( aDDLSyncInfo->mDDLReplInfoList ), sNode )
        {
            sInfo = (rpcDDLReplInfo*)sNode->mObj;

            rpcManager::removeReceiverNewMeta( sInfo->mRepName );
        }
    }
}

IDE_RC rpcDDLSyncManager::rebuildStatement( qciStatement * aQciStatement ) 
{
    idvSQL       * sStatistics    = QCI_STATISTIC( &( aQciStatement->statement ) );
    idBool         sIsBegin       = ID_FALSE;
    smiStatement * sSmiStatement  = QCI_SMI_STMT( &( aQciStatement->statement ) );
    smiTrans     * sTrans         = sSmiStatement->getTrans();
    smiStatement * sParentSmiStmt = sTrans->getStatement();   
    UInt           sSqlLen                         = QCI_STMTTEXTLEN( aQciStatement );
    SChar          sSql[RP_DDL_SQL_BUFFER_MAX + 1] = { 0, };
    qciSQLPlanCacheContext  sPlanCacheContext;

    sPlanCacheContext.mPlanCacheInMode     = QCI_SQL_PLAN_CACHE_IN_OFF;
    sPlanCacheContext.mSharedPlanMemory    = NULL;
    sPlanCacheContext.mPrepPrivateTemplate = NULL;

    if ( sSqlLen > RP_DDL_SQL_BUFFER_MAX )
    {
        sSqlLen = RP_DDL_SQL_BUFFER_MAX;
    }
    else
    {
        /* nothing to do */
    }

    idlOS::strncpy( sSql,
                    QCI_STMTTEXT( aQciStatement ), 
                    sSqlLen + 1 );
    sSql[sSqlLen] = '\0';

    IDU_FIT_POINT("rpcDDLSyncManager::rebuildStatement::_FT_");

    IDE_TEST( rpsSQLExecutor::hardRebuild( sStatistics,
                                           aQciStatement, 
                                           sParentSmiStmt,
                                           sSmiStatement,
                                           &sIsBegin,
                                           &sPlanCacheContext,
                                           sSql,
                                           sSqlLen,
                                           sSmiStatement->mFlag )
              != IDE_SUCCESS );

    IDE_TEST( sSmiStatement->end( SMI_STATEMENT_RESULT_SUCCESS ) != IDE_SUCCESS );
    sIsBegin = ID_FALSE;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();

    if ( sIsBegin == ID_TRUE )
    {
        (void)sSmiStatement->end( SMI_STATEMENT_RESULT_FAILURE );
        sIsBegin = ID_FALSE;
    }
    else
    {
        /* nothing to do */
    }

    IDE_POP();

    return IDE_FAILURE;
}

IDE_RC rpcDDLSyncManager::sendDDLSyncInfo( rpcDDLSyncInfo  * aDDLSyncInfo, 
                                           SChar           * aSql,
                                           UInt              aDDLSyncPartInfoCount,
                                           SChar             aDDLSyncPartInfoNames[][QC_MAX_OBJECT_NAME_LEN + 1] )

{
    iduListNode    * sNode   = NULL;
    rpcDDLReplInfo * sInfo   = NULL;
    idBool           sIsSuccess = ID_TRUE;
    UInt             sSqlLen    = idlOS::strlen( aSql );
    UInt             sDDLLevel  = RPU_REPLICATION_DDL_ENABLE_LEVEL;
    SChar            sErrMsg[RP_MAX_MSG_LEN + 1] = { 0, };

    IDE_DASSERT( aDDLSyncInfo != NULL );
    IDE_DASSERT( IDU_LIST_IS_EMPTY( &( aDDLSyncInfo->mDDLReplInfoList ) ) != ID_TRUE );

    IDU_LIST_ITERATE( &( aDDLSyncInfo->mDDLReplInfoList ), sNode )
    {
        sInfo = (rpcDDLReplInfo*)sNode->mObj;

        if ( rpnComm::sendDDLSyncInfo( sInfo->mHBT,
                                       sInfo->mProtocolContext,
                                       sInfo->mDDLSyncExitFlag,
                                       (UInt)RP_DDL_SYNC_INFO_MSG,
                                       sInfo->mRepName,
                                       aDDLSyncInfo->mDDLTableInfo.mUserName,
                                       aDDLSyncInfo->mDDLTableInfo.mTableName,
                                       aDDLSyncPartInfoCount,
                                       aDDLSyncPartInfoNames,
                                       sDDLLevel,                                        
                                       (UInt)RP_DDL_SYNC_SQL_MSG,
                                       aSql,
                                       sSqlLen,
                                       RPU_REPLICATION_SENDER_SEND_TIMEOUT )
             != IDE_SUCCESS )
        {
            sIsSuccess = ID_FALSE;

            addErrorList( sErrMsg,
                          sInfo->mRepName,
                          ideGetErrorMsg() );
        }
        else
        {
            /* nothing to do */
        }
    }

    IDE_TEST_RAISE( sIsSuccess != ID_TRUE, ERR_NETWORK );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NETWORK );
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RP_DDL_SYNC_RECEIVE_ERROR, sErrMsg ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpcDDLSyncManager::recvDDLSyncInfo( cmiProtocolContext   * aProtocolContext,
                                           idBool               * aExitFlag,
                                           rpdVersion           * aVersion,
                                           SChar                * aRepName,
                                           SChar                * aUserName,
                                           SChar                * aTableName,
                                           UInt                 * aDDLSyncPartInfoCount,
                                           SChar                  aDDLSyncPartInfoNames[][QC_MAX_OBJECT_NAME_LEN + 1],
                                           SChar               ** aSql )
{
    UInt  sDDLEnableLevel = 0;

    IDE_TEST( rpnComm::recvDDLSyncInfo( aProtocolContext,
                                        aExitFlag,
                                        (UInt)RP_DDL_SYNC_INFO_MSG,
                                        aRepName,
                                        aUserName,
                                        aTableName,
                                        RP_DDL_INFO_PART_OID_COUNT,
                                        aDDLSyncPartInfoCount,
                                        QC_MAX_OBJECT_NAME_LEN,
                                        aDDLSyncPartInfoNames,
                                        &sDDLEnableLevel,
                                        (UInt)RP_DDL_SYNC_SQL_MSG,
                                        aSql,
                                        RPU_REPLICATION_RECEIVE_TIMEOUT )
              != IDE_SUCCESS );

    IDE_TEST( rpcDDLSyncManager::checkDDLSyncRemoteVersion( aVersion ) != IDE_SUCCESS );

    IDE_TEST_RAISE( RPU_REPLICATION_DDL_SYNC == 0, ERR_REPLICATION_DDL_SYNC_DISABLED );
    IDE_TEST_RAISE( RPU_REPLICATION_DDL_ENABLE == 0, ERR_REPLICATION_DDL_DISABLED );
    IDE_TEST_RAISE( sDDLEnableLevel < RPU_REPLICATION_DDL_ENABLE_LEVEL, ERR_REPLICATION_DDL_ENABLE_LEVEL );

    IDE_TEST_RAISE( idlOS::strlen( *aSql ) > RP_DDL_SQL_BUFFER_MAX + 1, ERR_SQL_LENGTH );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_REPLICATION_DDL_DISABLED )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDB_REPLICATION_DDL_DISABLED ) );
    }
    IDE_EXCEPTION( ERR_REPLICATION_DDL_SYNC_DISABLED );
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RP_DDL_SYNC_DISABLE ) );
    }
    IDE_EXCEPTION( ERR_SQL_LENGTH );
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RP_DDL_SYNC_SQL_LENGTH_ERROR, RP_DDL_SQL_BUFFER_MAX ) );
    }
    IDE_EXCEPTION( ERR_REPLICATION_DDL_ENABLE_LEVEL )
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_REPLICATION_DDL_ENABLE_LEVEL,
                                  RPU_REPLICATION_DDL_ENABLE_LEVEL ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpcDDLSyncManager::sendDDLSyncInfoAck( cmiProtocolContext   * aProtocolContext,
                                              idBool               * aExitFlag,
                                              rpcDDLResult           aResult,
                                              SChar                * aErrMsg )
{
    IDE_TEST( rpnComm::sendDDLSyncInfoAck( NULL,
                                           aProtocolContext,
                                           aExitFlag,
                                           (UInt)RP_DDL_SYNC_INFO_MSG,
                                           aResult,
                                           aErrMsg,
                                           RPU_REPLICATION_SENDER_SEND_TIMEOUT )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpcDDLSyncManager::recvDDLSyncInfoAck( rpcDDLSyncInfo * aDDLSyncInfo )
{
    iduListNode    * sNode = NULL;
    rpcDDLReplInfo * sInfo = NULL;
    rpcDDLResult     sResult = RP_DDL_SYNC_SUCCESS;
    idBool           sIsSuccess = ID_TRUE;
    SChar            sErrMsg[RP_MAX_MSG_LEN + 1]     = { 0, };
    SChar            sRecvErrMsg[RP_MAX_MSG_LEN + 1] = { 0, };

    IDE_DASSERT( aDDLSyncInfo != NULL );
    IDE_DASSERT( IDU_LIST_IS_EMPTY( &( aDDLSyncInfo->mDDLReplInfoList ) ) != ID_TRUE );

    IDU_LIST_ITERATE( &( aDDLSyncInfo->mDDLReplInfoList ), sNode )
    {
        sInfo = (rpcDDLReplInfo*)sNode->mObj;

        if ( rpnComm::recvDDLSyncInfoAck( sInfo->mProtocolContext,
                                          sInfo->mDDLSyncExitFlag,
                                          (UInt)RP_DDL_SYNC_INFO_MSG,
                                          (UInt*)&sResult,
                                          sRecvErrMsg,
                                          RPU_REPLICATION_RECEIVE_TIMEOUT )
                  != IDE_SUCCESS )
        {
            sIsSuccess = ID_FALSE;

            addErrorList( sErrMsg,
                          sInfo->mRepName,
                          ideGetErrorMsg() );
        }
        else
        {
            if ( sResult != RP_DDL_SYNC_SUCCESS )
            {
                sIsSuccess = ID_FALSE;

                addErrorList( sErrMsg,
                              sInfo->mRepName,
                              sRecvErrMsg );
            }
            else
            {
                /* nothing to do */
            }
        }
    }

    IDE_TEST_RAISE( sIsSuccess != ID_TRUE, ERR_NETWORK );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NETWORK );
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RP_DDL_SYNC_RECEIVE_ERROR, sErrMsg ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC rpcDDLSyncManager::processDDLSyncMsg( rpcDDLSyncInfo * aDDLSyncInfo, RP_DDL_SYNC_MSG_TYPE aMsgType )
{
    if ( aDDLSyncInfo->mDDLSyncInfoType == RP_MASTER_DDL )
    {
        IDE_TEST( sendDDLSyncMsg( aDDLSyncInfo, aMsgType ) != IDE_SUCCESS );
    }
    else
    {
        IDE_TEST( recvDDLSyncMsg( aDDLSyncInfo, aMsgType ) != IDE_SUCCESS );
    }

    mResourceMgr->setMsgType( aMsgType );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpcDDLSyncManager::sendDDLSyncMsg( rpcDDLSyncInfo * aDDLSyncInfo, RP_DDL_SYNC_MSG_TYPE aMsgType )
{
    iduListNode    * sNode = NULL;
    rpcDDLReplInfo * sInfo = NULL;

    IDE_DASSERT( aDDLSyncInfo != NULL );
    IDE_DASSERT( IDU_LIST_IS_EMPTY( &( aDDLSyncInfo->mDDLReplInfoList ) ) != ID_TRUE );

    IDU_LIST_ITERATE( &( aDDLSyncInfo->mDDLReplInfoList ), sNode )
    {
        sInfo = (rpcDDLReplInfo*)sNode->mObj;

        IDE_TEST ( rpnComm::sendDDLSyncMsg( sInfo->mHBT,
                                            sInfo->mProtocolContext,
                                            sInfo->mDDLSyncExitFlag,
                                            (UInt)aMsgType,
                                            RPU_REPLICATION_SENDER_SEND_TIMEOUT )
                   != IDE_SUCCESS );

        if ( aMsgType == RP_DDL_SYNC_COMMIT_MSG )
        {
            ideLog::log( IDE_RP_0, "Send commit message to replication <%s>", sInfo->mRepName );
        }
        else
        {
            /* nothing to do */
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpcDDLSyncManager::recvDDLSyncMsg( rpcDDLSyncInfo * aDDLSyncInfo, RP_DDL_SYNC_MSG_TYPE aMsgType )
{
    iduListNode    * sNode = NULL;
    rpcDDLReplInfo * sInfo = NULL;

    IDE_DASSERT( aDDLSyncInfo != NULL );

    sNode = IDU_LIST_GET_FIRST( &( aDDLSyncInfo->mDDLReplInfoList ) );
    IDE_DASSERT( sNode != NULL );

    sInfo = (rpcDDLReplInfo*)sNode->mObj;
    IDE_TEST( rpnComm::recvDDLSyncMsg( sInfo->mProtocolContext,
                                       sInfo->mDDLSyncExitFlag,
                                       (UInt)aMsgType,
                                       RPU_REPLICATION_RECEIVE_TIMEOUT )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpcDDLSyncManager::processDDLSyncMsgAck( rpcDDLSyncInfo       * aDDLSyncInfo, 
                                                RP_DDL_SYNC_MSG_TYPE   aMsgType,
                                                rpcDDLResult           aResult,
                                                SChar                * aErrMsg )
{
    mResourceMgr->setMsgType( RP_DDL_SYNC_NONE_MSG );
    if( aMsgType == RP_DDL_SYNC_EXECUTE_MSG )
    {
        aMsgType = RP_DDL_SYNC_EXECUTE_RESULT_MSG;
    }

    if ( aDDLSyncInfo->mDDLSyncInfoType == RP_MASTER_DDL )
    {
        IDE_TEST( recvDDLSyncMsgAck( aDDLSyncInfo, aMsgType ) != IDE_SUCCESS );
    }
    else
    {
        IDE_TEST( sendDDLSyncMsgAck( aDDLSyncInfo, 
                                     aMsgType, 
                                     aResult,
                                     aErrMsg ) 
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpcDDLSyncManager::sendDDLSyncMsgAck( rpcDDLSyncInfo       * aDDLSyncInfo, 
                                             RP_DDL_SYNC_MSG_TYPE   aMsgType,
                                             rpcDDLResult           aResult,
                                             SChar                * aErrMsg )
{
    iduListNode    * sNode = NULL;
    rpcDDLReplInfo * sInfo = NULL;

    IDE_DASSERT( aDDLSyncInfo != NULL );

    sNode = IDU_LIST_GET_FIRST( &( aDDLSyncInfo->mDDLReplInfoList ) );
    IDE_DASSERT( sNode != NULL );

    sInfo = (rpcDDLReplInfo*)sNode->mObj;
    IDE_TEST( rpnComm::sendDDLSyncMsgAck( sInfo->mHBT,
                                          sInfo->mProtocolContext,
                                          sInfo->mDDLSyncExitFlag,
                                          (UInt)aMsgType,
                                          aResult,
                                          aErrMsg,
                                          RPU_REPLICATION_SENDER_SEND_TIMEOUT )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpcDDLSyncManager::recvDDLSyncMsgAck( rpcDDLSyncInfo * aDDLSyncInfo, RP_DDL_SYNC_MSG_TYPE aMsgType )
{
    iduListNode    * sNode = NULL;
    rpcDDLReplInfo * sInfo = NULL;
    rpcDDLResult     sResult    = RP_DDL_SYNC_SUCCESS;
    idBool           sIsSuccess = ID_TRUE;
    SChar            sErrMsg[RP_MAX_MSG_LEN + 1]     = { 0, };
    SChar            sRecvErrMsg[RP_MAX_MSG_LEN + 1] = { 0, };

    IDE_DASSERT( aDDLSyncInfo != NULL );
    IDE_DASSERT( IDU_LIST_IS_EMPTY( &( aDDLSyncInfo->mDDLReplInfoList ) ) != ID_TRUE );

    IDU_LIST_ITERATE( &( aDDLSyncInfo->mDDLReplInfoList ), sNode )
    {
        sInfo = (rpcDDLReplInfo*)sNode->mObj;

        sResult = RP_DDL_SYNC_SUCCESS;

        if ( rpnComm::recvDDLSyncMsgAck( sInfo->mProtocolContext,
                                         sInfo->mDDLSyncExitFlag,
                                         (UInt)aMsgType,
                                         (UInt*)&sResult,
                                         sRecvErrMsg,
                                         RPU_REPLICATION_RECEIVE_TIMEOUT )
             != IDE_SUCCESS )
        {
            sIsSuccess = ID_FALSE;

            addErrorList( sErrMsg,
                          sInfo->mRepName,
                          ideGetErrorMsg() );
        }
        else
        {
            if ( sResult != RP_DDL_SYNC_SUCCESS )
            {
                sIsSuccess = ID_FALSE;

                addErrorList( sErrMsg,
                              sInfo->mRepName,
                              sRecvErrMsg );
            }
            else
            {
                /* nothing to do */
            }
        }
    }

    IDE_TEST_RAISE( sIsSuccess != ID_TRUE, ERR_NETWORK );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NETWORK );
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RP_DDL_SYNC_RECEIVE_ERROR, sErrMsg ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC rpcDDLSyncManager::ddlSyncTableLock( smiTrans       * aTrans, 
                                            rpcDDLSyncInfo * aDDLSyncInfo,
                                            ULong            aTimeout )
{
    smSCN              sSCN;
    const void       * sTable    = NULL;
    smiTableLockMode   sLockMode = SMI_TABLE_NLOCK;
    iduListNode      * sNode     = NULL;
    rpcDDLTableInfo  * sInfo     = NULL;
    UInt               sDDLSyncTableCount = 0;

    sTable = smiGetTable( aDDLSyncInfo->mDDLTableInfo.mTableOID );
    sSCN = smiGetRowSCN( sTable );

    IDU_LIST_ITERATE( &( aDDLSyncInfo->mDDLTableInfo.mPartInfoList ), sNode )
    {
        sInfo = (rpcDDLTableInfo*)sNode->mObj;

        if ( sInfo->mIsDDLSyncTable == ID_TRUE )
        {
            sDDLSyncTableCount += 1;
        }
        else
        {
            /* nothing to do */
        }
    }

    if ( aDDLSyncInfo->mDDLTableInfo.mPartInfoCount == sDDLSyncTableCount )
    {
        sLockMode = SMI_TABLE_LOCK_X; 
    }
    else
    {
        sLockMode = SMI_TABLE_LOCK_IX;
    }

    IDE_TEST( smiValidateAndLockObjects( aTrans,
                                         sTable,
                                         sSCN,
                                         SMI_TBSLV_DDL_DML,
                                         sLockMode,
                                         aTimeout,                                      
                                         ID_FALSE )
              != IDE_SUCCESS );

    IDU_LIST_ITERATE( &( aDDLSyncInfo->mDDLTableInfo.mPartInfoList ), sNode )
    {
        sInfo = (rpcDDLTableInfo*)sNode->mObj;

        sTable = smiGetTable( sInfo->mTableOID );
        sSCN = smiGetRowSCN( sTable );

        if ( sInfo->mIsDDLSyncTable == ID_TRUE )
        {
            sLockMode = SMI_TABLE_LOCK_X;
        }
        else
        {
            sLockMode = SMI_TABLE_LOCK_IS;
        }

        IDE_TEST( smiValidateAndLockObjects( aTrans,
                                             sTable,
                                             sSCN,
                                             SMI_TBSLV_DDL_DML,
                                             sLockMode,
                                             aTimeout,
                                             ID_FALSE )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpcDDLSyncManager::getPartitionNamesByOID( smiTrans * aTrans,
                                                  smOID    * aPartTableOIDs,
                                                  UInt       aDDLSyncPartInfoCount,
                                                  SChar      aDDLSyncPartInfoNames[][QC_MAX_OBJECT_NAME_LEN + 1] )
{
    UInt           i                     = 0;
    const void   * sTable                = NULL;
    qciTableInfo * sTableInfo            = NULL;
    smSCN          sSCN;

    for ( i = 0; i < aDDLSyncPartInfoCount; i++ )
    {
        if ( aPartTableOIDs[i] != SM_OID_NULL )
        {
            sTable = smiGetTable( aPartTableOIDs[i] );
            sSCN = smiGetRowSCN( sTable );

            IDE_TEST( smiValidateAndLockObjects( aTrans,
                                                 sTable,
                                                 sSCN,
                                                 SMI_TBSLV_DDL_DML,
                                                 SMI_TABLE_LOCK_IS,
                                                 smiGetDDLLockTimeOut(aTrans),
                                                 ID_FALSE )
                      != IDE_SUCCESS );

            sTableInfo = (qciTableInfo *)rpdCatalog::rpdGetTableTempInfo( sTable );

            idlOS::strncpy( aDDLSyncPartInfoNames[i], 
                            sTableInfo->name, 
                            QC_MAX_OBJECT_NAME_LEN + 1 );
        }
        else
        {
            aDDLSyncPartInfoNames[i][0] = '\0';
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpcDDLSyncManager::getDDLSyncPartInfoList( smiTrans * aTrans,
                                                  UInt       aTableID,
                                                  UInt     * aTablePartInfoCount,
                                                  UInt       aDDLSyncPartInfoCount,
                                                  SChar      aDDLSyncPartInfoNames[][QC_MAX_OBJECT_NAME_LEN + 1],
                                                  iduList  * aDDLSyncPartInfoList )
{
    idBool         sIsInit               = ID_FALSE;
    idBool         sIsBegin              = ID_FALSE;
    UInt           sTablePartInfoCount   = 0;
    idBool         sIsDDLSyncPartition   = ID_FALSE;
    qciTableInfo * sTableInfo            = NULL;
    qciStatement * sQciStatement         = NULL;
    smiStatement * sStatement            = NULL;
    qciPartitionInfoList * sTablePartInfoList     = NULL;
    qciPartitionInfoList * sTempTablePartInfoList = NULL;
    iduList                sDDLSyncPartInfoList;

    IDU_LIST_INIT( &sDDLSyncPartInfoList );

    IDE_TEST( mResourceMgr->qciInitializeStatement( &sQciStatement, NULL ) != IDE_SUCCESS );
    sIsInit = ID_TRUE;

    IDE_TEST( mResourceMgr->smiStmtBegin( &sStatement,
                                          aTrans->getStatistics(),
                                          aTrans->getStatement(),
                                          SMI_STATEMENT_NORMAL | SMI_STATEMENT_ALL_CURSOR ) != IDE_SUCCESS );
    sIsBegin = ID_TRUE;

    qciMisc::setSmiStmt( sQciStatement, sStatement );

    IDE_TEST( qciMisc::getPartitionInfoList( &( sQciStatement->statement ),
                                             sStatement,
                                             ( iduMemory * )QCI_QMX_MEM( &( sQciStatement->statement ) ),
                                             aTableID,
                                             &sTablePartInfoList )
              != IDE_SUCCESS );

    IDE_TEST( qciMisc::validateAndLockPartitionInfoList( sQciStatement,
                                                         sTablePartInfoList,
                                                         SMI_TBSLV_DDL_DML, // TBS Validation 옵션
                                                         SMI_TABLE_LOCK_IS,
                                                         smiGetDDLLockTimeOut(QCI_SMI_STMT(&(sQciStatement->statement))->getTrans()))
              != IDE_SUCCESS );
      
    for ( sTempTablePartInfoList = sTablePartInfoList;
          sTempTablePartInfoList != NULL;
          sTempTablePartInfoList = sTempTablePartInfoList->next )
    {
        sTableInfo = sTempTablePartInfoList->partitionInfo; 
        sIsDDLSyncPartition = isDDLSyncPartition( sTableInfo, 
                                                  aDDLSyncPartInfoCount,
                                                  aDDLSyncPartInfoNames );

        IDE_TEST( addDDLTableInfo( sTableInfo->name,
                                   sTableInfo->partitionID,
                                   sTableInfo->tableOID,
                                   sTableInfo->accessOption,
                                   sIsDDLSyncPartition,
                                   &sDDLSyncPartInfoList )
                  != IDE_SUCCESS );

        sTablePartInfoCount +=1 ;
    }

    IDE_TEST( mResourceMgr->smiStmtEnd( sStatement, SMI_STATEMENT_RESULT_SUCCESS ) != IDE_SUCCESS );
    sIsBegin = ID_FALSE;

    sIsInit = ID_FALSE;
    IDE_TEST( mResourceMgr->qciFinalizeStatement( sQciStatement ) != IDE_SUCCESS );

    *aTablePartInfoCount = sTablePartInfoCount;
    IDU_LIST_JOIN_LIST( aDDLSyncPartInfoList, &sDDLSyncPartInfoList );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();
    
    if ( sIsBegin == ID_TRUE )
    {
        (void)mResourceMgr->smiStmtEnd( sStatement, SMI_STATEMENT_RESULT_FAILURE );
    }

    if ( sIsInit == ID_TRUE )
    {
        (void)mResourceMgr->qciFinalizeStatement( sQciStatement );
    }
    
    IDE_POP();

    return IDE_FAILURE;
}

idBool rpcDDLSyncManager::isDDLSyncPartition( qciTableInfo * aTableInfo, 
                                              UInt           aDDLSyncPartInfoCount,
                                              SChar          aDDLSyncPartInfoNames[][QC_MAX_OBJECT_NAME_LEN + 1] )
{
    UInt   i = 0;
    idBool sIsDDLSyncPartition = ID_FALSE;
    
    IDE_TEST_CONT( aTableInfo->replicationCount == 0, NORMAL_EXIT );

    if ( aDDLSyncPartInfoCount > 0 )
    {
        for ( i = 0 ; i < aDDLSyncPartInfoCount; i++ )
        {
            if ( idlOS::strMatch( aTableInfo->name,
                                  idlOS::strlen( aTableInfo->name ),
                                  aDDLSyncPartInfoNames[i],
                                  idlOS::strlen( aDDLSyncPartInfoNames[i] ) ) == 0 )
            {
                sIsDDLSyncPartition = ID_TRUE;
                break;
            }
            else
            {
                sIsDDLSyncPartition = ID_FALSE;
            }
        }
    }
    else
    {
        sIsDDLSyncPartition = ID_TRUE;
    }

    RP_LABEL( NORMAL_EXIT );

    return sIsDDLSyncPartition;
}

IDE_RC rpcDDLSyncManager::addDDLTableInfo( SChar           * aName,
                                           UInt              aTableID,
                                           smOID             aTableOID,
                                           qciAccessOption   aAccessOption,
                                           idBool            aIsDDLSyncTable,
                                           iduList         * aPartInfoList )

{
    rpcDDLTableInfo * sDDLTableInfo = NULL;

    IDU_FIT_POINT_RAISE( "rpcDDLSyncManager::addDDLTableInfo::calloc::sDDLTableInfo",
                         ERR_MEMORY_ALLOC_SYNC_INFO );
    IDE_TEST_RAISE( mResourceMgr->rpCalloc( ID_SIZEOF( rpcDDLTableInfo ), (void**)&sDDLTableInfo )
                    != IDE_SUCCESS, ERR_MEMORY_ALLOC_SYNC_INFO );

    sDDLTableInfo->mTableID = aTableID;
    sDDLTableInfo->mTableOID = aTableOID;
    sDDLTableInfo->mAccessOption = aAccessOption;
    sDDLTableInfo->mIsDDLSyncTable = aIsDDLSyncTable;

    idlOS::strncpy( sDDLTableInfo->mTableName, 
                    aName, 
                    QC_MAX_OBJECT_NAME_LEN + 1 );

    IDU_LIST_INIT_OBJ( &( sDDLTableInfo->mNode ), (void*)sDDLTableInfo );
    IDU_LIST_ADD_LAST( aPartInfoList, &( sDDLTableInfo->mNode ) ); 

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_MEMORY_ALLOC_SYNC_INFO );
    {
        IDE_ERRLOG( IDE_RP_0 );
        IDE_SET( ideSetErrorCode( rpERR_ABORT_MEMORY_ALLOC,
                                  "rpxDDLExec::addDDLTableInfo",
                                  "sPartTableInfo" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

void rpcDDLSyncManager::freePartInfoList( iduList * aPartInfoList )
{
    iduListNode      * sNode  = NULL;
    iduListNode      * sDummy = NULL;
    rpcDDLTableInfo  * sInfo  = NULL;

    IDU_LIST_ITERATE_SAFE( aPartInfoList, sNode, sDummy )
    {
        sInfo = (rpcDDLTableInfo*)sNode->mObj;

        IDU_LIST_REMOVE( &( sInfo->mNode ) );
    }
}

IDE_RC rpcDDLSyncManager::createAndAddDDLReplInfoList( cmiProtocolContext * aProtocolContext,
                                                       smiTrans           * aTrans,
                                                       rpcDDLSyncInfo     * aDDLSyncInfo, 
                                                       SChar              * aRepName,
                                                       SInt                 aReplCount )
{
    smiStatement    * sSmiStmt      = NULL;
    smiTableCursor  * sCursor       = NULL;
    SInt              i             = 0;
    SInt              sReplCount    = 0;
    rpdReplications * sReplications = NULL;
    idBool            sIsBegin      = ID_FALSE;
    IDE_ASSERT( aDDLSyncInfo != NULL );
    IDE_TEST( mResourceMgr->smiStmtBegin( &sSmiStmt,
                                          aTrans->getStatistics(),
                                          aTrans->getStatement(),
                                          SMI_STATEMENT_NORMAL |
                                          SMI_STATEMENT_MEMORY_CURSOR )
              != IDE_SUCCESS );
    sIsBegin = ID_TRUE;

    IDU_FIT_POINT_RAISE( "rpcDDLSyncManager::createAndAddDDLReplInfoList::calloc::sReplications",
                         ERR_CALLOC_REPLICATIONS );
    IDE_TEST_RAISE( mResourceMgr->rpCalloc( ( ID_SIZEOF( rpdReplications  ) * aReplCount ), 
                                            (void**)&sReplications )
                    != IDE_SUCCESS, ERR_CALLOC_REPLICATIONS );

    IDE_TEST_RAISE( mResourceMgr->rpCalloc( ( ID_SIZEOF( smiTableCursor ) ), 
                                            (void**)&sCursor )
                    != IDE_SUCCESS, ERR_CALLOC_CURSOR );


    if ( aDDLSyncInfo->mDDLSyncInfoType == RP_MASTER_DDL )
    {
        IDE_TEST( rpdCatalog::selectAllReplicationsWithCursor( sSmiStmt,
                                                               sReplications,
                                                               &sReplCount,
                                                               sCursor )
                  != IDE_SUCCESS );

    }
    else
    {
        IDE_ASSERT( aRepName != NULL );
        IDE_TEST( rpdCatalog::selectReplWithCursor( sSmiStmt,
                                                    aRepName,
                                                    sReplications,
                                                    ID_FALSE,
                                                    sCursor )
                  != IDE_SUCCESS);
        sReplCount = 1;
    }

    for ( i = 0 ; i < sReplCount ; i++ )
    {
        if ( ( sReplications[i].mOptions & RP_OPTION_DDL_REPLICATE_MASK )
             != RP_OPTION_DDL_REPLICATE_SET )
        {
            IDE_TEST( createAndAddDDLReplInfo( aProtocolContext,
                                               sSmiStmt,
                                               aDDLSyncInfo,
                                               &( sReplications[i] ) )
                      != IDE_SUCCESS );
        }
    }

    IDU_FIT_POINT_RAISE( "rpcDDLSyncManager::createAndAddDDLReplInfoList::IDU_LIST_IS_EMPTY::mDDLReplInfoList",
                         ERR_ADD_REPL_INFO_LIST );
    IDE_TEST_RAISE( IDU_LIST_IS_EMPTY( &( aDDLSyncInfo->mDDLReplInfoList ) ) == ID_TRUE, 
                    ERR_ADD_REPL_INFO_LIST );

    IDE_TEST( mResourceMgr->smiStmtEnd( sSmiStmt, 
                                        SMI_STATEMENT_RESULT_SUCCESS ) != IDE_SUCCESS );
    sIsBegin = ID_FALSE;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_ADD_REPL_INFO_LIST );
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RP_INTERNAL_ARG, "DDL sync target replication items not found" ) );
    }
    IDE_EXCEPTION( ERR_CALLOC_REPLICATIONS );
    {
        IDE_ERRLOG( IDE_RP_0 );
        IDE_SET( ideSetErrorCode( rpERR_ABORT_MEMORY_ALLOC,
                                  "rpcDDLSyncManager::createAndAddDDLReplInfoList",
                                  "sReplications" ) );
    }
    IDE_EXCEPTION( ERR_CALLOC_CURSOR );
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_MEMORY_ALLOC,
                                  "rpcDDLSyncManager::createAndAddDDLReplInfoList",
                                  "sCursor" ) );
    }

    IDE_EXCEPTION_END;

    IDE_PUSH();
    
    if ( sIsBegin == ID_TRUE )
    {
        (void)mResourceMgr->smiStmtEnd( sSmiStmt, SMI_STATEMENT_RESULT_FAILURE );
    }

    IDE_POP();

    return IDE_FAILURE;
}

IDE_RC rpcDDLSyncManager::createAndAddDDLReplInfo( cmiProtocolContext * aProtocolContext,
                                                   smiStatement       * aSmiStmt,
                                                   rpcDDLSyncInfo     * aDDLSyncInfo,
                                                   rpdReplications    * aReplications )
{
    rpdMetaItem * sReplMetaItems = NULL;
    smiTableCursor * sCursor = NULL;

    IDU_FIT_POINT_RAISE( "rpcDDLSyncManager::createAndAddDDLReplInfo::calloc::sReplMetaItems",
                         ERR_MEMORY_ALLOC_META_ITEM );
    IDE_TEST_RAISE( mResourceMgr->rpCalloc( ( ID_SIZEOF( rpdMetaItem  ) * aReplications->mItemCount ), 
                                            (void**)&sReplMetaItems )
                    != IDE_SUCCESS, ERR_MEMORY_ALLOC_META_ITEM );
    
    IDE_TEST_RAISE( mResourceMgr->rpCalloc( ( ID_SIZEOF( smiTableCursor ) ),
                                            (void**)&sCursor )
                                        != IDE_SUCCESS, ERR_MEMORY_ALLOC_CURSOR );

    IDE_TEST( rpdCatalog::selectReplItemsWithCursor( aSmiStmt,
                                                     aReplications->mRepName,
                                                     sReplMetaItems,
                                                     aReplications->mItemCount,
                                                     sCursor,
                                                     ID_FALSE )
              != IDE_SUCCESS );

    IDE_TEST( checkAndAddReplInfo( aSmiStmt->getTrans()->getStatistics(),
                                   aProtocolContext,
                                   aSmiStmt,
                                   aDDLSyncInfo,
                                   sReplMetaItems,
                                   aReplications )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_MEMORY_ALLOC_META_ITEM );
    {
        IDE_ERRLOG( IDE_RP_0 );
        IDE_SET( ideSetErrorCode( rpERR_ABORT_MEMORY_ALLOC,
                                  "rpcDDLSyncManager::createAndAddDDLReplInfo",
                                  "sReplMetaItems" ) );
    }
    IDE_EXCEPTION( ERR_MEMORY_ALLOC_CURSOR );
    {
        IDE_ERRLOG( IDE_RP_0 );
        IDE_SET( ideSetErrorCode( rpERR_ABORT_MEMORY_ALLOC,
                                  "rpcDDLSyncManager::createAndAddDDLReplInfo",
                                  "sCursor" ) );
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpcDDLSyncManager::checkAndAddReplInfo( idvSQL             * aStatistics,
                                               cmiProtocolContext * aProtocolContext,
                                               smiStatement       * aSmiStmt,
                                               rpcDDLSyncInfo     * aDDLSyncInfo,
                                               rpdMetaItem        * aReplMetaItems,
                                               rpdReplications    * aReplication )

{
    iduListNode      * sNode  = NULL;
    rpcDDLTableInfo  * sInfo  = NULL;
    SInt               sLastUsedHostNo = 0;
    rpdReplItems     * sReplItem       = NULL;
    rpdReplHosts     * sReplHosts      = NULL;

    if ( IDU_LIST_IS_EMPTY( &( aDDLSyncInfo->mDDLTableInfo.mPartInfoList ) ) == ID_TRUE )
    {
        sReplItem = rpcManager::searchReplItem( aReplMetaItems,
                                                aReplication->mItemCount,
                                                aDDLSyncInfo->mDDLTableInfo.mTableOID );
    }
    else
    {
        IDU_LIST_ITERATE( &( aDDLSyncInfo->mDDLTableInfo.mPartInfoList ), sNode )
        {
            sInfo = (rpcDDLTableInfo*)sNode->mObj;

            if ( sInfo->mIsDDLSyncTable == ID_TRUE )
            {
                sReplItem = rpcManager::searchReplItem( aReplMetaItems,
                                                        aReplication->mItemCount,
                                                        sInfo->mTableOID );

                if ( sReplItem != NULL )
                {
                    break;
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
    }

    if ( sReplItem != NULL )
    {
        IDE_TEST_RAISE( aReplication->mReplMode == RP_EAGER_MODE, ERR_EAGER_MODE );

        IDE_TEST_RAISE( aReplication->mRole != RP_ROLE_REPLICATION, ERR_DDL_SYNC_WITH_PROPAGATION );

        IDE_TEST( rpdMeta::checkLocalNRemoteName( sReplItem ) != IDE_SUCCESS );

        IDE_TEST( rpcManager::getReplHosts( aSmiStmt,
                                            aReplication,
                                            &sReplHosts )
                  != IDE_SUCCESS );

        IDE_TEST( rpdCatalog::getIndexByAddr( aReplication->mLastUsedHostNo,
                                              sReplHosts,
                                              aReplication->mHostCount,
                                              &sLastUsedHostNo )
                 != IDE_SUCCESS);

        if ( duplicateDDLReplInfo( aDDLSyncInfo,
                                   &sReplHosts[sLastUsedHostNo] ) == ID_FALSE )
        {
            IDE_TEST( addDDLReplInfo( aStatistics,
                                      aProtocolContext,
                                      aDDLSyncInfo,
                                      &sReplHosts[sLastUsedHostNo] )
                      != IDE_SUCCESS );
        }
    }
    else
    {
        /* nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_DDL_SYNC_WITH_PROPAGATION );
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RP_DDL_SYNC_WITH_PROPAGATION ) );
    }
    IDE_EXCEPTION( ERR_EAGER_MODE );
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RP_DDL_SYNC_NOT_SUPPORT_EAGER ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpcDDLSyncManager::addDDLReplInfo( idvSQL             * aStatistics,
                                          cmiProtocolContext * aProtocolContext,
                                          rpcDDLSyncInfo     * aDDLSyncInfo, 
                                          rpdReplHosts       * aReplHosts )
{
    rpcDDLReplInfo * sDDLReplInfo   = NULL;

    IDU_FIT_POINT_RAISE( "rpcDDLSyncManager::addDDLReplInfo::calloc::sDDLReplInfo", ERR_CALLOC );
    IDE_TEST_RAISE( mResourceMgr->rpCalloc( ID_SIZEOF( rpcDDLReplInfo ), (void**)&sDDLReplInfo ) 
                    != IDE_SUCCESS, ERR_CALLOC );

    sDDLReplInfo->mHBT = NULL;
    sDDLReplInfo->mProtocolContext = aProtocolContext;
    sDDLReplInfo->mDDLSyncExitFlag = &( aDDLSyncInfo->mDDLSyncExitFlag );
    sDDLReplInfo->mStatistics = aStatistics;

    idlOS::strncpy( sDDLReplInfo->mRepName,
                    aReplHosts->mRepName,
                    QC_MAX_NAME_LEN + 1 );

    idlOS::strncpy( sDDLReplInfo->mHostIp,
                    aReplHosts->mHostIp,
                    QC_MAX_IP_LEN + 1 );
    sDDLReplInfo->mPortNo = aReplHosts->mPortNo;
    sDDLReplInfo->mConnType = aReplHosts->mConnType;
    sDDLReplInfo->mIBLatency = aReplHosts->mIBLatency;

    IDU_LIST_INIT_OBJ( &( sDDLReplInfo->mNode ), (void*)sDDLReplInfo );
    IDU_LIST_ADD_LAST( &( aDDLSyncInfo->mDDLReplInfoList ), &( sDDLReplInfo->mNode ) );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_CALLOC )
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_MEMORY_ALLOC,
                                  "rpcDDLSyncManager::addDDLReplInfo",
                                  "sDDLReplInfo" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

void rpcDDLSyncManager::destroyDDLReplInfoList( rpcDDLSyncInfo * aDDLSyncInfo )
{
    iduListNode    * sNode  = NULL;
    iduListNode    * sDummy = NULL;
    rpcDDLReplInfo * sInfo  = NULL;

    IDE_DASSERT( aDDLSyncInfo != NULL );

    IDU_LIST_ITERATE_SAFE( &( aDDLSyncInfo->mDDLReplInfoList ), sNode, sDummy )
    {
        sInfo = (rpcDDLReplInfo*)sNode->mObj;

        IDU_LIST_REMOVE( &( sInfo->mNode ) );

        if ( sInfo->mHBT != NULL )
        {
            rpcHBT::unregistHost( sInfo->mHBT );
            sInfo->mHBT = NULL;
        }
        else
        {
            /* nothing to do */
        }

        if ( aDDLSyncInfo->mDDLSyncInfoType != RP_MASTER_DDL )
        {
            /* DDL_REPLICATED 타입은 ProtocolContext 를 Executor 로부터 받았으므로
             * 이에 대한 관리는 Executor 에게 맡긴다. */
            sInfo->mProtocolContext = NULL;
        }
        else
        {
            if ( sInfo->mProtocolContext != NULL )
            {
                destroyProtocolContext( sInfo->mProtocolContext );
                sInfo->mProtocolContext = NULL;
            }
            else
            {
                /* notihng to do */
            }
        }
    }
}

void rpcDDLSyncManager::addToDDLExecutorList( void * aDDLExecutor )
{
    rpxDDLExecutor * sDDLExecutor = (rpxDDLExecutor*)aDDLExecutor;

    IDE_ASSERT( mDDLExecutorListLock.lock( NULL ) == IDE_SUCCESS );

    IDU_LIST_ADD_LAST( &mDDLExecutorList, &( sDDLExecutor->mNode ) ); 

    IDE_ASSERT( mDDLExecutorListLock.unlock() == IDE_SUCCESS );
}


IDE_RC rpcDDLSyncManager::recvDDLInfoAndCreateDDLExecutor( cmiProtocolContext * aProtocolContext, 
                                                           rpdVersion         * aVersion )
{
    IDE_TEST( createAndStartDDLExecutor( aProtocolContext, aVersion ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpcDDLSyncManager::createAndStartDDLExecutor( cmiProtocolContext * aProtocolContext, rpdVersion * aVersion ) 
{
    rpxDDLExecutor * sDDLExecutor = NULL;

    IDU_FIT_POINT_RAISE( "rpcDDLSyncManager::createAndStartDDLExecutor::malloc::sDDLExecutor",
                         ERR_MEMORY_ALLOC_EXECUTOR );
    IDE_TEST_RAISE( iduMemMgr::malloc( IDU_MEM_RP_RPC,
                                       ID_SIZEOF( rpxDDLExecutor ),
                                       (void**)&sDDLExecutor,
                                       IDU_MEM_IMMEDIATE )
                    != IDE_SUCCESS, ERR_MEMORY_ALLOC_EXECUTOR );

    new ( sDDLExecutor ) rpxDDLExecutor;

    IDE_TEST( sDDLExecutor->initialize( aProtocolContext, aVersion ) != IDE_SUCCESS ); 

    addToDDLExecutorList( sDDLExecutor );

    IDE_TEST( sDDLExecutor->start() != IDE_SUCCESS );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_MEMORY_ALLOC_EXECUTOR );
    {
        IDE_ERRLOG( IDE_RP_0 );
        IDE_SET( ideSetErrorCode( rpERR_ABORT_MEMORY_ALLOC,
                                  "rpcDDLSyncManager::createAndStartDDLExecutor",
                                  "sDDLExecutor" ) );
    }
    IDE_EXCEPTION_END;

    IDE_PUSH();

    if ( sDDLExecutor != NULL )
    {
        IDE_ASSERT( mDDLExecutorListLock.lock( NULL ) == IDE_SUCCESS );

        sDDLExecutor->setExitFlag( ID_TRUE );

        IDE_ASSERT( mDDLExecutorListLock.unlock() == IDE_SUCCESS );
    }
    else
    {
        /* nothing to do */
    }

    IDE_POP();

    return IDE_FAILURE;
}

IDE_RC rpcDDLSyncManager::realizeDDLExecutor( idvSQL * aStatistics )
{
    idBool           sLock     = ID_FALSE;
    iduListNode    * sNode     = NULL;
    iduListNode    * sDummy    = NULL;
    rpxDDLExecutor * sExecutor = NULL;

    IDE_ASSERT( mDDLExecutorListLock.lock( NULL ) == IDE_SUCCESS );
    sLock = ID_TRUE;

    IDU_LIST_ITERATE_SAFE( &mDDLExecutorList, sNode, sDummy )
    {
        sExecutor = (rpxDDLExecutor*)sNode->mObj;
        if ( sExecutor->isExit() == ID_TRUE )
        {
            IDE_TEST( sExecutor->waitThreadJoin( aStatistics )
                      != IDE_SUCCESS );

            IDU_LIST_REMOVE( sNode );

            sExecutor->destroy();
            (void)iduMemMgr::free( sExecutor );
            sExecutor = NULL;
        }
        else
        {
            /* nothing to do */
        }
    }

    sLock = ID_FALSE;
    IDE_ASSERT( mDDLExecutorListLock.unlock() == IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();

    if ( sLock == ID_TRUE )
    {
        sLock = ID_FALSE;
        IDE_ASSERT( mDDLExecutorListLock.unlock() == IDE_SUCCESS );
    }
    else
    {
        /* nothing to do */
    }

    IDE_POP();

    return IDE_FAILURE;
}

void rpcDDLSyncManager::sendDDLSyncCancel( rpcDDLSyncInfo * aDDLSyncInfo )
{
    iduListNode        * sNode            = NULL;
    rpcDDLReplInfo     * sInfo            = NULL;
    cmiProtocolContext * sProtocolContext = NULL;

    IDE_DASSERT( aDDLSyncInfo != NULL );

    IDE_TEST_CONT( IDU_LIST_IS_EMPTY( &( aDDLSyncInfo->mDDLReplInfoList ) ) == ID_TRUE, NORMAL_EXIT );

    IDU_LIST_ITERATE( &( aDDLSyncInfo->mDDLReplInfoList ), sNode )
    {
        sInfo = (rpcDDLReplInfo*)sNode->mObj;

        if ( sInfo->mHBT != NULL )
        {        
            if( connect( sInfo, &sProtocolContext ) == IDE_SUCCESS )
            {
                if( rpnComm::sendDDLSyncCancel( sInfo->mHBT,
                                                sProtocolContext,
                                                sInfo->mDDLSyncExitFlag,
                                                sInfo->mRepName,
                                                RPU_REPLICATION_SENDER_SEND_TIMEOUT )
                    != IDE_SUCCESS )
                {
                    /* nothing to do */
                }
                else
                {
                    ideLog::log( IDE_RP_0, "[DDLSyncManager] Send to replication <%s> DDL sync cancel", 
                                 sInfo->mRepName );
                }
            }
            else
            {
                IDE_ERRLOG( IDE_RP_0 );
            }
        }
    }

    RP_LABEL( NORMAL_EXIT );
}

IDE_RC rpcDDLSyncManager::recvAndSetDDLSyncCancel( cmiProtocolContext * aProtocolContext, idBool * aExitFlag )
{
    SChar sRepName[QC_MAX_NAME_LEN + 1] = { 0, };

    IDE_TEST( rpnComm::recvDDLSyncCancel( aProtocolContext, 
                                          aExitFlag,
                                          sRepName,
                                          RPU_REPLICATION_RECEIVE_TIMEOUT )
              != IDE_SUCCESS );
    IDE_DASSERT( sRepName != NULL );

    ideLog::log( IDE_RP_0, "[DDLSyncManager] Receive DDL sync cancel <%s>", sRepName );

    setDDLSyncCancelEvent( sRepName );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

void rpcDDLSyncManager::setDDLSyncCancelEvent( SChar * aRepName  )
{
    setDDLSyncInfoCancelEvent( aRepName );

    setDDLExecutorCancelEvent( aRepName );
}

void rpcDDLSyncManager::setDDLSyncInfoCancelEvent( SChar * aRepName )
{
    rpcDDLReplInfo * sReplInfo = NULL;

    IDE_ASSERT( mDDLSyncInfoListLock.lock( NULL ) == IDE_SUCCESS );    

    sReplInfo = findDDLReplInfoByNameWithoutLock( aRepName );
    if ( sReplInfo != NULL )
    {
        *( sReplInfo->mDDLSyncExitFlag ) = ID_TRUE;
        IDU_SESSION_SET_CANCELED( *( sReplInfo->mStatistics->mSessionEvent ) );

        ideLog::log( IDE_RP_0, "[DDLSyncManager] Set Replication<%s> cancel flag", aRepName ); 
    }

    IDE_ASSERT( mDDLSyncInfoListLock.unlock() == IDE_SUCCESS );   
}

void rpcDDLSyncManager::setDDLExecutorCancelEvent( SChar * aRepName )
{
    rpxDDLExecutor * sExecutor = NULL;

    IDE_ASSERT( mDDLExecutorListLock.lock( NULL ) == IDE_SUCCESS );

    sExecutor = findDDLExecutorByNameWithoutLock( aRepName );
    if ( sExecutor != NULL )
    {        
        sExecutor->setExitFlag( ID_TRUE );
        IDU_SESSION_SET_CANCELED( *( sExecutor->getStatistics()->mSessionEvent ) );
    }

    IDE_ASSERT( mDDLExecutorListLock.unlock() == IDE_SUCCESS );
}

IDE_RC rpcDDLSyncManager::getTableInfo( idvSQL        * aStatistics,
                                        smiTrans      * aTrans,
                                        SChar         * aTableName, 
                                        SChar         * aUserName,
                                        ULong           aTimeout,
                                        qciTableInfo ** aTableInfo )
{
    UInt           sUserID    = 0;
    void         * sTable     = NULL;
    qciTableInfo * sTableInfo = NULL;
    smSCN          sSCN       = SM_SCN_INIT;
    idBool         sIsInit    = ID_FALSE;
    idBool         sIsBegin   = ID_FALSE;
    smiStatement * sSmiStatement = NULL;
    qciStatement * sQciStatement = NULL;

    IDE_TEST( mResourceMgr->qciInitializeStatement( &sQciStatement, 
                                                    aTrans->getStatistics() ) 
              != IDE_SUCCESS );
    sIsInit = ID_TRUE;

    IDE_TEST( mResourceMgr->smiStmtBegin( &sSmiStatement,
                                          aStatistics,
                                          aTrans->getStatement(),
                                          SMI_STATEMENT_NORMAL | SMI_STATEMENT_ALL_CURSOR ) != IDE_SUCCESS );
    sIsBegin = ID_TRUE;

    qciMisc::setSmiStmt( sQciStatement, sSmiStatement );

    IDE_TEST( qciMisc::getUserID( &( sQciStatement->statement ),
                                  aUserName,
                                  idlOS::strlen( aUserName ),
                                  &sUserID )
              != IDE_SUCCESS );

    IDE_TEST( qciMisc::getTableHandleByName( sSmiStatement,
                                             sUserID,
                                             (UChar*)aTableName,
                                             idlOS::strlen( aTableName ),
                                             &sTable,
                                             &sSCN )
              != IDE_SUCCESS );
    
    IDE_TEST( smiValidateAndLockObjects( aTrans,
                                         sTable,
                                         sSCN,
                                         SMI_TBSLV_DDL_DML,
                                         SMI_TABLE_LOCK_IS,
                                         aTimeout,
                                         ID_FALSE )
              != IDE_SUCCESS );

    sTableInfo = (qciTableInfo *)rpdCatalog::rpdGetTableTempInfo( sTable );

    IDE_TEST( mResourceMgr->smiStmtEnd( sSmiStatement, 
                                        SMI_STATEMENT_RESULT_SUCCESS ) != IDE_SUCCESS );
    sIsBegin = ID_FALSE;

    sIsInit = ID_FALSE;
    IDE_TEST( mResourceMgr->qciFinalizeStatement( sQciStatement ) 
              != IDE_SUCCESS );

    * aTableInfo = sTableInfo;
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();

    if ( sIsBegin == ID_TRUE )
    {
        (void)mResourceMgr->smiStmtEnd( sSmiStatement, SMI_STATEMENT_RESULT_FAILURE );
    }

    if ( sIsInit == ID_TRUE )
    {
        (void)mResourceMgr->qciFinalizeStatement( sQciStatement );
    }
    
    IDE_POP();

    return IDE_FAILURE;
}

void rpcDDLSyncManager::addErrorList( SChar * aDest,
                                      SChar * aRepName,
                                      SChar * aErrMsg )
{
    UInt sDestLen    = idlOS::strlen( aDest );
    UInt sErrMsgLen  = idlOS::strlen( aErrMsg );
    UInt sRepNameLen = idlOS::strlen( aRepName );
    SChar * sErrMsg  = aDest + sDestLen;

    idlOS::snprintf( sErrMsg,
                     RP_MAX_MSG_LEN - sDestLen,
                     "\n%s : %s",
                     aRepName,
                     aErrMsg ); 
    aDest[sDestLen + 1 + sErrMsgLen + 3 + sRepNameLen] = '\0';
}

void rpcDDLSyncManager::destroyProtocolContext( cmiProtocolContext * aProtocolContext )
{
    cmiLink * sLink = NULL;

    IDE_DASSERT( aProtocolContext != NULL );

    (void)cmiGetLinkForProtocolContext( aProtocolContext, &sLink );

    if ( cmiIsAllocCmBlock ( aProtocolContext ) == ID_TRUE )
    {
        (void)cmiFreeCmBlock( aProtocolContext );
    }

    if ( sLink != NULL )
    {
        (void)cmiShutdownLink( sLink, CMI_DIRECTION_RDWR );
        (void)cmiCloseLink( sLink );
        (void)cmiFreeLink( sLink );
        sLink = NULL;
    }
}

idBool rpcDDLSyncManager::duplicateDDLReplInfo( rpcDDLSyncInfo     * aDDLSyncInfo, 
                                                rpdReplHosts       * aReplHosts )
{
    rpcDDLReplInfo * sInfo          = NULL;   
    iduListNode    * sNode          = NULL;
    idBool           sIsExist       = ID_FALSE;
  
    IDU_LIST_ITERATE( &( aDDLSyncInfo->mDDLReplInfoList ), sNode )
    {
        sInfo = (rpcDDLReplInfo*)sNode->mObj;
        
        if ( ( (UInt)sInfo->mPortNo == aReplHosts->mPortNo ) &&
             ( idlOS::strncmp( sInfo->mHostIp,
                               aReplHosts->mHostIp,
                               QC_MAX_IP_LEN ) == 0 ) )
        {
            sIsExist = ID_TRUE;
            break;
        }        
    }
    
    return sIsExist;
}
