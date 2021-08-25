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

#include <rpcResourceManager.h>
#include <rpcDDLSyncManager.h>

IDE_RC rpcResourceManager::initialize( iduMemoryClientIndex    aIndex )
{
    mDDLSyncInfo = NULL;
    mMsgType     = RP_DDL_SYNC_NONE_MSG;
    
    IDU_LIST_INIT( &mSmiStmtList );
    IDU_LIST_INIT( &mQciStmtList );
    IDU_LIST_INIT( &mSmiTransList );

    IDU_FIT_POINT("rpcResourceManager::initialize::mMemory::init");
    IDE_TEST( mMemory.init( aIndex ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

void rpcResourceManager::finalize( rpcDDLSyncManager  * aDDLSyncMgr,
                                   smiTrans           * aDDLTrans )
{
    if ( mDDLSyncInfo != NULL )
    {
        ddlSyncException( aDDLSyncMgr, aDDLTrans );
    }
    else
    {
        IDE_DASSERT( IDU_LIST_IS_EMPTY( &mSmiStmtList ) == ID_TRUE );
        IDE_DASSERT( IDU_LIST_IS_EMPTY( &mQciStmtList ) == ID_TRUE );
        IDE_DASSERT( IDU_LIST_IS_EMPTY( &mSmiTransList ) == ID_TRUE );

    }
        
    (void)rpFreeAll();
    (void)mMemory.destroy();
}

IDE_RC rpcResourceManager::smiStmtBegin( smiStatement   ** aStmt,
                                         idvSQL          * aStatistics,
                                         smiStatement    * aParent,
                                         UInt              aFlag )
{
    /*
     *  alloc, list 추가, begin()
     */
    rpcSmiStmt  *sStmtInfo = NULL;

    IDU_FIT_POINT("rpcResourceManager::smiStmtBegin::calloc::sStmtInfo");
    IDE_TEST( rpCalloc( ID_SIZEOF(rpcSmiStmt) , (void**)&sStmtInfo ) 
              != IDE_SUCCESS );

    IDU_FIT_POINT("rpcResourceManager::smiStmtBegin::begin::mStmt");
    IDE_TEST( sStmtInfo->mStmt.begin( aStatistics,
                                      aParent,
                                      aFlag )
              != IDE_SUCCESS );

    IDU_LIST_INIT_OBJ( &(sStmtInfo->mNode), (void*)sStmtInfo );
    IDU_LIST_ADD_FIRST( &mSmiStmtList, &(sStmtInfo->mNode) );

    *aStmt = &( sStmtInfo->mStmt );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpcResourceManager::smiStmtEnd( smiStatement * aStmt, UInt aFlag )
{
    /*
     *  end(), list 제거
     */
    rpcSmiStmt  * sStmtInfo = NULL;
    iduListNode * sCurNode = NULL;
    iduListNode * sNextNode = NULL;

    IDU_FIT_POINT("rpcResourceManager::smiStmtEnd::end::aStmt");
    IDE_TEST( aStmt->end( aFlag ) != IDE_SUCCESS );

    IDU_LIST_ITERATE_SAFE( &mSmiStmtList, sCurNode, sNextNode )
    {
        sStmtInfo = (rpcSmiStmt*)sCurNode->mObj;

        if( &(sStmtInfo->mStmt) == aStmt )
        {
            IDU_LIST_REMOVE( &( sStmtInfo->mNode ) );
            break;
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}
void rpcResourceManager::smiStmtEndAll()  // 전부 fail
{
    rpcSmiStmt  * sStmtInfo = NULL;
    iduListNode * sCurNode = NULL;
    iduListNode * sNextNode = NULL;

    IDU_LIST_ITERATE_SAFE( &mSmiStmtList, sCurNode, sNextNode )
    {
        sStmtInfo = (rpcSmiStmt*)sCurNode->mObj;

        if( sStmtInfo->mStmt.endForce() != IDE_SUCCESS )
        {
            ideLog::log( IDE_RP_0,
                         "[DDLResourceManager] An error occurred while end the statement(sm) : %s",
                         ideGetErrorMsg() );
        }
        IDU_LIST_REMOVE( &( sStmtInfo->mNode ) );
    }

}

IDE_RC rpcResourceManager::qciInitializeStatement( qciStatement ** aStmt,
                                                   idvSQL        * aStatistics )
{
    rpcQciStmt  *sStmtInfo = NULL;

    IDU_FIT_POINT( "rpcResourceManager::qciInitializeStatement::calloc::sStmtInfo" );
    IDE_TEST( rpCalloc( ID_SIZEOF(rpcQciStmt) , (void**)&sStmtInfo )
              != IDE_SUCCESS );

    IDU_FIT_POINT( "rpcResourceManager::qciInitializeStatement::initializeStatement::mStmt" );
    IDE_TEST( qci::initializeStatement( &(sStmtInfo->mStmt),
                                        NULL,
                                        NULL,
                                        aStatistics )
              != IDE_SUCCESS );

    IDU_LIST_INIT_OBJ( &(sStmtInfo->mNode), (void*)sStmtInfo );
    IDU_LIST_ADD_FIRST( &mQciStmtList, &(sStmtInfo->mNode) );

    *aStmt = &( sStmtInfo->mStmt );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC rpcResourceManager::qciFinalizeStatement( qciStatement * aStmt )
{
    rpcQciStmt * sStmtInfo = NULL;
    iduListNode * sCurNode = NULL;
    iduListNode * sNextNode = NULL;

    IDU_FIT_POINT( "rpcResourceManager::qciFinalizeStatement::finalizeStatement::aStmt" );
    IDE_TEST( qci::finalizeStatement( aStmt ) != IDE_SUCCESS );

    IDU_LIST_ITERATE_SAFE( &mQciStmtList, sCurNode, sNextNode )
    {
        sStmtInfo = (rpcQciStmt*)sCurNode->mObj;

        if( &(sStmtInfo->mStmt) == aStmt )
        {
            IDU_LIST_REMOVE( &( sStmtInfo->mNode ) );
            break;
        }
    }
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

void rpcResourceManager::qciFinalizeStatementAll( )
{
    rpcQciStmt * sStmtInfo = NULL;
    iduListNode * sCurNode = NULL;
    iduListNode * sNextNode = NULL;

    IDU_LIST_ITERATE_SAFE( &mQciStmtList, sCurNode, sNextNode )
    {
        sStmtInfo = (rpcQciStmt*)sCurNode->mObj;

        if( qci::finalizeStatement( &(sStmtInfo->mStmt) ) == IDE_SUCCESS )
        {
            ideLog::log( IDE_RP_0, 
                         "[DDLResourceManager] An error occurred while finalize the statement(qp) : %s",
                         ideGetErrorMsg() );
        }
        IDU_LIST_REMOVE( &( sStmtInfo->mNode ) );
    }

}

IDE_RC rpcResourceManager::smiTransBegin( smiTrans     ** aTrans,
                                          smiStatement ** aStatement,
                                          idvSQL        * aStatistics,
                                          UInt            aFlag )
{
    rpcSmiTrans  *sTransInfo = NULL;
    UInt          sStage     = 0;

    IDU_FIT_POINT( "rpcResourceManager::smiTransBegin::calloc::sTransInfo" );
    IDE_TEST( rpCalloc( ID_SIZEOF(rpcSmiTrans) , (void**)&sTransInfo ) 
              != IDE_SUCCESS );

    IDU_FIT_POINT( "rpcResourceManager::smiTransBegin::initialize::mTrans" );
    IDE_TEST( sTransInfo->mTrans.initialize() != IDE_SUCCESS );
    sStage = 1;

    IDU_FIT_POINT( "rpcResourceManager::smiTransBegin::begin::mTrans" );
    IDE_TEST( sTransInfo->mTrans.begin( aStatement,
                                        aStatistics,
                                        aFlag )
              != IDE_SUCCESS );

    IDU_LIST_INIT_OBJ( &(sTransInfo->mNode), (void*)sTransInfo );
    IDU_LIST_ADD_FIRST( &mSmiTransList, &(sTransInfo->mNode) );

    *aTrans = &(sTransInfo->mTrans);
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();

    switch ( sStage )
    {
        case 1:
            (void)sTransInfo->mTrans.destroy(NULL);
        default:
            break;
    }
    IDE_POP();

    return IDE_FAILURE;

}

IDE_RC rpcResourceManager::smiTransCommit( smiTrans *aTrans )
{
    rpcSmiTrans * sTransInfo = NULL;
    iduListNode * sCurNode = NULL;
    iduListNode * sNextNode = NULL;

    IDU_FIT_POINT( "rpcResourceManager::smiTransCommit::commit::aTrans" );
    IDE_TEST( aTrans->commit() != IDE_SUCCESS );

    if( aTrans->destroy( NULL ) != IDE_SUCCESS )
    {
        ideLog::log( IDE_RP_0,
                     "[DDLResourceManager] An error occurred while destroy the transaction(sm) : %s", 
                     ideGetErrorMsg() );
    }

    IDU_LIST_ITERATE_SAFE( &mSmiTransList, sCurNode, sNextNode )
    {
        sTransInfo = (rpcSmiTrans*)sCurNode->mObj;

        if( &(sTransInfo->mTrans) == aTrans )
        {
            IDU_LIST_REMOVE( &( sTransInfo->mNode ) );
            break;
        }
    }
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

void rpcResourceManager::smiTransRollbackAll()
{
    rpcSmiTrans * sTransInfo = NULL;
    iduListNode * sCurNode = NULL;
    iduListNode * sNextNode = NULL;


    IDU_LIST_ITERATE_SAFE( &mSmiTransList, sCurNode, sNextNode )
    {
        sTransInfo = (rpcSmiTrans*)sCurNode->mObj;

        IDE_ASSERT( sTransInfo->mTrans.rollback() == IDE_SUCCESS )

        if ( sTransInfo->mTrans.destroy( NULL ) != IDE_SUCCESS )
        {
            ideLog::log( IDE_RP_0,
                         "[DDLResourceManager] An error occurred while destroy the transaction(sm) : %s",
                         ideGetErrorMsg() );

        }
        IDU_LIST_REMOVE( &( sTransInfo->mNode ) );
    }
}

IDE_RC rpcResourceManager::rpMalloc( ULong aSize, void **aBuffer )
{
    IDU_FIT_POINT( "rpcResourceManager::rpMalloc::alloc::aBuffer" );
    IDE_TEST( mMemory.alloc( aSize, aBuffer ) != IDE_SUCCESS );

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}


IDE_RC rpcResourceManager::rpCalloc( ULong aSize, void **aBuffer )
{
    IDE_DASSERT(aSize != 0);
    IDU_FIT_POINT( "rpcResourceManager::rpMalloc::cralloc::aBuffer" );
    IDE_TEST( mMemory.cralloc( aSize, aBuffer ) != IDE_SUCCESS );

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC rpcResourceManager::rpFreeAll()
{
    IDE_TEST( mMemory.freeAll() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

void rpcResourceManager::ddlSyncException( rpcDDLSyncManager    * aDDLSyncMgr,
                                           smiTrans             * aDDLTrans )
{
    rpcDDLSyncManager * sDDLSyncMgr = NULL;
    rpcDDLSyncInfo    * sDDLSyncInfo = mDDLSyncInfo;
    rpcDDLReplInfo    * sDDLReplInfo = NULL;
    iduListNode       * sNode = NULL;
    IDE_RC              sIsSignalErr = IDE_FAILURE;
    SChar               sErrMsg[RP_MAX_MSG_LEN + 1] = { 0, };

    IDE_DASSERT( aDDLSyncMgr != NULL );

    ideLog::log( IDE_RP_0, "[DDLSyncManager] DDL sync exception start" );
    
    if ( ( ideFindErrorCode( rpERR_ABORT_FAULT_TOLERATED ) == IDE_SUCCESS ) ||
         ( ideFindErrorCode( qpERR_ABORT_FAULT_TOLERATED ) == IDE_SUCCESS ) )
    {
        sIsSignalErr = IDE_SUCCESS;
    }
   
    idlOS::strncpy( sErrMsg,
                    ideGetErrorMsg(),
                    RP_MAX_MSG_LEN );

    sDDLSyncMgr = aDDLSyncMgr;

    sDDLSyncMgr->sendDDLSyncCancel( sDDLSyncInfo );

    smiStmtEndAll();

    if ( aDDLTrans != NULL ) 
    {
        (void)sDDLSyncMgr->ddlSyncRollback( aDDLTrans, sDDLSyncInfo );
    }

    (void)sDDLSyncMgr->setSkipUpdateXSN( sDDLSyncInfo, ID_FALSE );

    if ( sIsSignalErr == IDE_SUCCESS )
    {
        iduMutexMgr::unlockAllMyThread();
    }
    sDDLSyncMgr->removeReceiverNewMeta( sDDLSyncInfo );

    qciFinalizeStatementAll();
    smiTransRollbackAll();

    if ( sDDLSyncMgr->recoverTableAccessOption( NULL, sDDLSyncInfo ) != IDE_SUCCESS )
    {
        smiStmtEndAll();
        smiTransRollbackAll();
    }

    if( mMsgType != RP_DDL_SYNC_NONE_MSG )
    {
        (void)sDDLSyncMgr->processDDLSyncMsgAck( sDDLSyncInfo,
                                                 mMsgType,
                                                 RP_DDL_SYNC_FAILURE,
                                                 sErrMsg );
    }

    sDDLSyncMgr->removeFromDDLSyncInfoList( sDDLSyncInfo );
    sDDLSyncMgr->destroyDDLSyncInfo( sDDLSyncInfo );

    if ( sIsSignalErr == IDE_SUCCESS )
    {
        iduLatch::unlockWriteAllMyThread();
    }

    ideLog::log( IDE_RP_0, "[DDLSyncManager] DDL sync exception end" );
}

