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
 * $Id: rpxTempSender.cpp 87362 2020-04-27 08:12:46Z yoonhee.kim $
 **********************************************************************/

#include <idl.h>
#include <ide.h>

#include <smi.h>
#include <smErrorCode.h>

#include <qci.h>

#include <rpDef.h>
#include <rpdCatalog.h>
#include <rpuProperty.h>
#include <rpcManager.h>
#include <rpxTempSender.h>
#include <rpuProperty.h>

#include <rpxSync.h>

IDE_RC rpxTempSender::initialize( cmiProtocolContext * aProtocolContext, 
                                  rpdReplications    * aReplication,
                                  rpdVersion         * aVersion,
                                  rpdReplSyncItem   ** aTempSyncItemList )
{
    UInt sStage = 0;

    mIsThreadDead = ID_FALSE;
    mExitFlag = ID_FALSE;
    mEventFlag = ID_ULONG(0);

    mProtocolContext = aProtocolContext;

    mLocalMeta.initialize();
    mRemoteMeta.initialize();
    
    idlOS::memcpy( &mRemoteMeta.mReplication, aReplication, ID_SIZEOF(rpdReplications ) );

    mRemoteVersion.mVersion = aVersion->mVersion;

    mTempSyncItemList = *aTempSyncItemList;

    idvManager::initSession( &( mSession ),
                             0 /* unuse */,
                             NULL /* unuse */ );
    idvManager::initSQL( &( mStatistics ),
                         &( mSession ),
                         &mEventFlag,
                         NULL,
                         NULL,
                         NULL,
                         IDV_OWNER_UNKNOWN );

    IDE_TEST_RAISE( mThreadJoinCV.initialize() != IDE_SUCCESS, ERR_COND_INIT );
    sStage = 1;

    IDE_TEST_RAISE( mThreadJoinMutex.initialize( (SChar *)"TEMP_SYNC_THR_JOIN_MUTEX",
                                                 IDU_MUTEX_KIND_POSIX,
                                                 IDV_WAIT_INDEX_NULL )
                    != IDE_SUCCESS, ERR_MUTEX_INIT );
    sStage = 2;

    IDE_TEST_RAISE( mExitMutex.initialize( (SChar *)"TEMP_SYNC_THR_EXIT_MUTEX",
                                           IDU_MUTEX_KIND_POSIX,
                                           IDV_WAIT_INDEX_NULL )
                    != IDE_SUCCESS, ERR_MUTEX_INIT );
    sStage = 3;

    *aTempSyncItemList = NULL;
    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_COND_INIT );
    {
        IDE_ERRLOG( IDE_RP_0 );
         IDE_SET( ideSetErrorCode( rpERR_FATAL_ThrCondInit ) );
     }
     IDE_EXCEPTION( ERR_MUTEX_INIT );
     {
         IDE_ERRLOG( IDE_RP_0 );
         IDE_SET( ideSetErrorCode( rpERR_FATAL_ThrMutexInit ) );
     }
    IDE_EXCEPTION_END;

    IDE_ERRLOG( IDE_RP_0 );

    switch ( sStage )
    {
        case 3:
            (void)mExitMutex.destroy();
        case 2:
            (void)mThreadJoinMutex.destroy();
        case 1:
            (void)mThreadJoinCV.destroy();
        default:
            break;
    }
    mRemoteMeta.finalize();
    mLocalMeta.finalize();

    mIsThreadDead = ID_TRUE;

    return IDE_FAILURE;
}

void rpxTempSender::destroy()
{
    if( mThreadJoinCV.destroy() != IDE_SUCCESS )
    {
        IDE_ERRLOG( IDE_RP_0 );
    }
    else
    {
        /* nothing to do */
    }

    if( mThreadJoinMutex.destroy() != IDE_SUCCESS )
    {
        IDE_ERRLOG( IDE_RP_0 );
    }
    else
    {
        /* Nothing to do */
    }

    if( mExitMutex.destroy() != IDE_SUCCESS )
    {
        IDE_ERRLOG( IDE_RP_0 );
    }
    else
    {
        /* Nothing to do */
    }
}

IDE_RC rpxTempSender::initializeThread()
{
    return IDE_SUCCESS;
}

void rpxTempSender::finalizeThread()
{
    mIsThreadDead = ID_TRUE;
}


void rpxTempSender::finalize() 
{
    cmiLink         * sLink = NULL;
    rpdReplSyncItem * sSyncItem  = NULL;

    setExit( ID_TRUE );

    if ( mProtocolContext != NULL )
    {
        cmiGetLinkForProtocolContext( mProtocolContext, &sLink );

        if ( cmiFreeCmBlock( mProtocolContext ) != IDE_SUCCESS )
        {
            IDE_ERRLOG( IDE_RP_0 );
            IDE_SET( ideSetErrorCode( rpERR_ABORT_FREE_CM_BLOCK ) );
            IDE_ERRLOG( IDE_RP_0 );
        }

        // BUG-16258
        if(cmiShutdownLink(sLink, CMI_DIRECTION_RDWR) != IDE_SUCCESS)
        {
            IDE_ERRLOG(IDE_RP_0);
            IDE_SET(ideSetErrorCode(rpERR_ABORT_SHUTDOWN_LINK));
            IDE_ERRLOG(IDE_RP_0);
        }
        if(cmiCloseLink(sLink) != IDE_SUCCESS)
        {
            IDE_ERRLOG(IDE_RP_0);
            IDE_SET(ideSetErrorCode(rpERR_ABORT_CLOSE_LINK));
            IDE_ERRLOG(IDE_RP_0);
        }

        /* tempSender initialize 시에 executor로 부터 받은 protocol context이다. alloc은 executor에서 했지만,
         * free는 tempSender 종료 시에 한다. */
        (void)iduMemMgr::free( mProtocolContext );
        mProtocolContext = NULL;

        if(sLink != NULL)
        {
            if(cmiFreeLink(sLink) != IDE_SUCCESS)
            {
                IDE_ERRLOG(IDE_RP_0);
                IDE_SET(ideSetErrorCode(rpERR_ABORT_FREE_LINK));
                IDE_ERRLOG(IDE_RP_0);
            }
            sLink = NULL;
        }
    }

    if ( mTempSyncItemList != NULL)
    {
        sSyncItem = mTempSyncItemList;
        mTempSyncItemList = mTempSyncItemList->next;
        (void)iduMemMgr::free(sSyncItem);
    }
 
    mRemoteMeta.finalize();
    mLocalMeta.finalize();
       
    return;
                
}

void rpxTempSender::run()
{
    smiTrans     sTrans;
    SChar        sRepName[QCI_MAX_NAME_LEN + 1] = { 0, };

    SChar        * sErrMsg      = NULL;
    UInt           sStage       = 0;
    smiStatement * spRootStmt;
    smiStatement   sSmiStmt;
    UInt           sFlag = 0;
    idBool sIsSendHandshakeAck = ID_FALSE;

    SInt        sDummyFailbackStatus;
    UInt        sResult        = RP_MSG_DISCONNECT;
    SChar       sBuffer[RP_ACK_MSG_LEN];
    UInt        sMsgLen;
    ULong       sDummyXSN;

    sBuffer[0] = '\0';

    idlOS::strncpy( sRepName,
                    mRemoteMeta.mReplication.mRepName,
                    QCI_MAX_NAME_LEN + 1 );

    IDE_TEST(sTrans.initialize() != IDE_SUCCESS);
    sStage = 1;

    sFlag = (sFlag & ~SMI_ISOLATION_MASK) | (UInt)RPU_ISOLATION_LEVEL;
    sFlag = (sFlag & ~SMI_TRANSACTION_MASK) | SMI_TRANSACTION_NORMAL;
    sFlag = (sFlag & ~SMI_TRANSACTION_REPL_MASK) | SMI_TRANSACTION_REPL_NONE;
    sFlag = (sFlag & ~SMI_COMMIT_WRITE_MASK) | SMI_COMMIT_WRITE_NOWAIT;

    IDE_TEST(sTrans.begin(&spRootStmt, &mStatistics, sFlag, SMX_NOT_REPL_TX_ID)
             != IDE_SUCCESS);
    sStage = 2;

    IDE_TEST( sSmiStmt.begin( sTrans.getStatistics(),
                              spRootStmt,
                              SMI_STATEMENT_NORMAL | SMI_STATEMENT_ALL_CURSOR )
              != IDE_SUCCESS );
    sStage = 3;

    IDE_TEST( setTableOIDAtSyncItemList( &sSmiStmt ) != IDE_SUCCESS );

    IDE_TEST( rpcManager::buildTempSyncMeta( &sSmiStmt,
                                             sRepName,
                                             NULL,
                                             mTempSyncItemList,
                                             &mLocalMeta )
              != IDE_SUCCESS );

    sStage = 2;
    IDE_TEST( sSmiStmt.end( SMI_STATEMENT_RESULT_SUCCESS ) != IDE_SUCCESS );

    sIsSendHandshakeAck = ID_TRUE;
    IDE_TEST( rpnComm::sendTempSyncHandshakeAck( mProtocolContext,
                                                 &mExitFlag,
                                                 RP_MSG_OK,
                                                 NULL,
                                                 RPU_REPLICATION_SENDER_SEND_TIMEOUT ) 
              != IDE_SUCCESS );

    mLocalMeta.mReplication.mRemoteVersion.mVersion = mRemoteVersion.mVersion;
    IDE_TEST( mLocalMeta.sendMeta( NULL,
                                   mProtocolContext,
                                   &mExitFlag,
                                   ID_TRUE, 
                                   RPU_REPLICATION_SENDER_SEND_TIMEOUT )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( rpnComm::recvHandshakeAck( NULL,
                                               mProtocolContext,
                                               &mExitFlag,
                                               &sResult,
                                               &sDummyFailbackStatus,
                                               &sDummyXSN,
                                               sBuffer,
                                               &sMsgLen,
                                               RPU_REPLICATION_RECEIVE_TIMEOUT)
                    != IDE_SUCCESS, ERR_RECEIVE_META_ACK );

    switch ( sResult )
    {
        case RP_MSG_OK :
            break;
        default :
            IDE_RAISE( ERR_UNEXPECTED_HANDSHAKE_ACK );
    }

    IDE_TEST( tempSyncStart( &sTrans ) != IDE_SUCCESS );

    IDE_TEST(sTrans.commit() != IDE_SUCCESS);
    sStage = 1;

    sStage = 0;
    IDE_TEST(sTrans.destroy( NULL ) != IDE_SUCCESS);

    finalize();

    return;
    IDE_EXCEPTION( ERR_RECEIVE_META_ACK );
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_HANDSHAKE_DISCONNECT,
                                  "[rpxTempSender::run] recvHandshakeAck" ) );
    }
    IDE_EXCEPTION( ERR_UNEXPECTED_HANDSHAKE_ACK );
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_UNEXPECTED_HANDSHAKE_ACK, sResult) );
    }

    IDE_EXCEPTION_END;

    IDE_ERRLOG( IDE_RP_0 );
    sErrMsg = ideGetErrorMsg();

    IDE_PUSH();

    if ( sIsSendHandshakeAck != ID_TRUE )
    {
        (void)rpnComm::sendTempSyncHandshakeAck( mProtocolContext,
                                                 &mExitFlag,
                                                 RP_MSG_META_DIFF, // build meta 까지의 단계에서 실패이므로 meta diff 
                                                 sErrMsg, 
                                                 RPU_REPLICATION_SENDER_SEND_TIMEOUT ); 
    }
    switch(sStage)
    {

        case 3:
            (void)sSmiStmt.end( SMI_STATEMENT_RESULT_FAILURE );
        case 2:
            IDE_ASSERT(sTrans.rollback() == IDE_SUCCESS);
        case 1:
            (void)sTrans.destroy( NULL );
        default:
            break;
    }

    finalize();

    IDE_POP();

    return;
}

IDE_RC rpxTempSender::setTableOIDAtSyncItemList( smiStatement * aSmiStmt )
{
    qciStatement      sQciStatement;
    UInt              sStage       = 0;
    
    rpdReplSyncItem * sReplItem = NULL;
    UInt              sUserID = 0;

    void            * sTableHandle = NULL;
    smSCN             sSCN = SM_SCN_INIT;

    qciTableInfo         * sTableInfo = NULL;
    qcmPartitionInfoList * sTablePartInfoList = NULL;
    qcmPartitionInfoList * sTempTablePartInfoList = NULL;
    qcmTableInfo         * sPartInfo = NULL;


    idlOS::memset( &sQciStatement, 0x00, ID_SIZEOF( qciStatement ) );
    IDE_TEST( qci::initializeStatement( &sQciStatement,
                                        NULL,
                                        NULL,
                                        aSmiStmt->getTrans()->getStatistics() )
              != IDE_SUCCESS );

    sStage = 1;

    qciMisc::setSmiStmt( &sQciStatement, aSmiStmt );

    sReplItem = mTempSyncItemList;
    while( sReplItem != NULL )
    {
        IDE_TEST( qciMisc::getUserID( &( sQciStatement.statement ),
                                      sReplItem->mUserName,
                                      idlOS::strlen( sReplItem->mUserName ),
                                      &sUserID )
                  != IDE_SUCCESS );

        IDE_TEST( qciMisc::getTableHandleByName( aSmiStmt,
                                                 sUserID,
                                                 (UChar*)sReplItem->mTableName,
                                                 idlOS::strlen( sReplItem->mTableName ),
                                                 &sTableHandle,
                                                 &sSCN )
                  != IDE_SUCCESS );

        IDE_TEST( smiValidateAndLockObjects( aSmiStmt->getTrans(),
                                             sTableHandle,
                                             sSCN,
                                             SMI_TBSLV_DDL_DML,
                                             SMI_TABLE_LOCK_IS,
                                             0,
                                             ID_FALSE )
                  != IDE_SUCCESS );
        sTableInfo = (qciTableInfo *)rpdCatalog::rpdGetTableTempInfo( sTableHandle );

        if ( sReplItem->mPartitionName[0] != 0 )
        {
            IDE_TEST( qciMisc::getPartitionInfoList( (void*)&( sQciStatement.statement ),
                                                     aSmiStmt,
                                                     ( iduMemory * )QCI_QMX_MEM( &( sQciStatement.statement ) ),
                                                     sTableInfo->tableID,
                                                     &sTablePartInfoList )
                      != IDE_SUCCESS );

            for ( sTempTablePartInfoList = sTablePartInfoList;
                  sTempTablePartInfoList != NULL;
                  sTempTablePartInfoList = sTempTablePartInfoList->next )
            {
                IDE_TEST( smiValidateAndLockObjects( aSmiStmt->getTrans(),
                                                     sTempTablePartInfoList->partHandle,
                                                     sTempTablePartInfoList->partSCN,
                                                     SMI_TBSLV_DDL_DML,
                                                     SMI_TABLE_LOCK_IS,
                                                     0,
                                                     ID_FALSE )
                          != IDE_SUCCESS );

                sPartInfo = sTempTablePartInfoList->partitionInfo;
                if ( idlOS::strcmp( sReplItem->mPartitionName, sPartInfo->name ) == 0 )
                {
                    sReplItem->mTableOID = sPartInfo->tableOID;
                    break;
                }
            }
        }
        else
        {
            sReplItem->mTableOID = sTableInfo->tableOID;
        }
        sReplItem = sReplItem->next;

    }

    sStage = 0;
    IDE_TEST( qci::finalizeStatement( &sQciStatement ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    ideLog::log( IDE_RP_0, "[TempSender] Failure to set table oid (%s.%s)",
                 sReplItem->mUserName,
                 sReplItem->mTableName );
    IDE_PUSH();
    switch(sStage)
    {
        case 1:
            (void)qci::finalizeStatement( &sQciStatement ); 
        default:
            break;
    }
    IDE_POP();

    return IDE_FAILURE;
}

IDE_RC rpxTempSender::tempSyncStart( smiTrans * aTrans )

{
    rpnMessenger  sMessenger;
    idBool        sIsMessengerInitialized = ID_FALSE;
    SInt  i      = 0;
    ULong sSyncedTuples = 0;
    idBool       sIsBegin = ID_FALSE;
    smiStatement sSmiStmt;
    
    IDE_TEST( sMessenger.initialize( mProtocolContext,
                                     RP_MESSENGER_NONE,
                                     RP_SOCKET_TYPE_TCP,
                                     &mExitFlag,
                                     &( mLocalMeta.mReplication ),
                                     NULL,
                                     ID_FALSE )
              != IDE_SUCCESS );
    sIsMessengerInitialized = ID_TRUE;

    IDE_TEST( sSmiStmt.begin( aTrans->getStatistics(), 
                              aTrans->getStatement(), 
                              SMI_STATEMENT_NORMAL | SMI_STATEMENT_ALL_CURSOR )
              != IDE_SUCCESS );
    sIsBegin = ID_TRUE;

    for ( i = 0; i <  mLocalMeta.mReplication.mItemCount; i++ )
    {
        IDE_TEST( rpxSync::syncTable( &sSmiStmt,
                                      &sMessenger,
                                      &( mLocalMeta.mItems[i] ),
                                      &mExitFlag,
                                      1, /* Child Count */
                                      0, /* Child Number */
                                      &sSyncedTuples,
                                      ID_FALSE )
                  != IDE_SUCCESS );
    }

    IDE_TEST( sSmiStmt.end( SMI_STATEMENT_RESULT_SUCCESS ) );
    sIsBegin = ID_FALSE;

    IDE_TEST( sMessenger.sendStop( SM_SN_NULL,
                                   &mExitFlag,
                                   RPN_STOP_MSG_NETWORK_TIMEOUT_SEC )
              != IDE_SUCCESS );

    sIsMessengerInitialized = ID_FALSE;
    sMessenger.destroy(RPN_DO_NOT_RELEASE_PROTOCOL_CONTEXT);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sIsMessengerInitialized == ID_TRUE )
    {
        sMessenger.destroy(RPN_DO_NOT_RELEASE_PROTOCOL_CONTEXT);
    }
    if ( sIsBegin == ID_TRUE )
    {
        (void)sSmiStmt.end( SMI_STATEMENT_RESULT_FAILURE );
    }

    return IDE_FAILURE;
}

IDE_RC rpxTempSender::waitThreadJoin( idvSQL *aStatistics )
{
    PDL_Time_Value sTvCpu;
    PDL_Time_Value sTimeValue;
    idBool         sIsLock = ID_FALSE;
    UInt           sTimeoutMilliSec = 0;

    IDE_ASSERT( mThreadJoinMutex.lock( NULL /* idvSQL* */ ) == IDE_SUCCESS );
    sIsLock = ID_TRUE;

    sTimeValue.initialize( 0, 1000 );

    while( mIsThreadDead != ID_TRUE )
    {
        sTvCpu  = idlOS::gettimeofday();
        sTvCpu += sTimeValue;

        (void)mThreadJoinCV.timedwait( &mThreadJoinMutex, &sTvCpu );

        if ( aStatistics != NULL )
        {
            // BUG-22637 MM에서 QUERY_TIMEOUT, Session Closed를 설정했는지 확인
            IDE_TEST( iduCheckSessionEvent( aStatistics ) != IDE_SUCCESS );
        }
        else
        {
            // 5 Sec
            IDE_TEST_RAISE( sTimeoutMilliSec >= 5000, ERR_TIMEOUT );
            sTimeoutMilliSec++;
        }
    }

    sIsLock = ID_FALSE;
    IDE_ASSERT( mThreadJoinMutex.unlock() == IDE_SUCCESS );

    if( join() != IDE_SUCCESS )
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RP_JOIN_THREAD ) );
        IDE_ERRLOG( IDE_RP_0 );
        IDE_CALLBACK_FATAL( "[Repl TempSync Mgr] Thread join error" );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_TIMEOUT );
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RP_INTERNAL_ARG,
                                  "rpxTempSender::waitThreadJoin exceeds timeout" ) );
    }
    IDE_EXCEPTION_END;

    if( sIsLock == ID_TRUE )
    {
        sIsLock = ID_FALSE;
        IDE_ASSERT( mThreadJoinMutex.unlock() == IDE_SUCCESS );
    }
    else
    {
        /* notihng to do */
    }

    return IDE_FAILURE;
}

