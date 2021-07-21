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
 
#include <idl.h>
#include <dktNotifier.h>
#include <dkaLinkerProcessMgr.h>

IDE_RC dktNotifier::initialize()
{
    mSession = NULL;
    mSessionId = DKP_LINKER_NOTIFY_SESSION_ID;
    mDtxInfoCnt = 0;
    mExit    = ID_FALSE;
    mPause   = ID_TRUE;
    mRestart = ID_FALSE;
    mRunFailoverOneCycle = ID_FALSE;

    IDU_LIST_INIT( &mDtxInfo );
    IDU_LIST_INIT( &mFailoverDtxInfo );

    IDE_TEST_RAISE( mNotifierDtxInfoMutex.initialize( (SChar *)"DKT_NOTIFIER_MUTEX", 
                                          IDU_MUTEX_KIND_POSIX, 
                                          IDV_WAIT_INDEX_NULL )
                    != IDE_SUCCESS, ERR_MUTEX_INIT );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_MUTEX_INIT );
    {
        IDE_SET( ideSetErrorCode( dkERR_FATAL_ThrMutexInit ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

void dktNotifier::addDtxInfo( DK_NOTIFY_TYPE aType, dktDtxInfo * aDtxInfo )
{
    IDE_ASSERT( mNotifierDtxInfoMutex.lock( NULL /*idvSQL* */ ) == IDE_SUCCESS );

    IDU_LIST_INIT_OBJ( &(aDtxInfo->mNode), aDtxInfo );
    
    if ( aType == DK_NOTIFY_NORMAL )
    {
        IDU_LIST_ADD_LAST( &mDtxInfo, &(aDtxInfo->mNode) );
        mDtxInfoCnt++;
    }
    else if ( aType == DK_NOTIFY_FAILOVER )
    {
        IDU_LIST_ADD_LAST( &mFailoverDtxInfo, &(aDtxInfo->mNode) );
        mFailoverDtxInfoCnt++;
    }
    else
    {
        IDE_DASSERT( 0 );
    }

    IDE_ASSERT( mNotifierDtxInfoMutex.unlock() == IDE_SUCCESS );
}

void dktNotifier::removeDtxInfo( DK_NOTIFY_TYPE aType, dktDtxInfo * aDtxInfo )
{
    IDE_ASSERT( mNotifierDtxInfoMutex.lock( NULL /*idvSQL* */ ) == IDE_SUCCESS );

    IDU_LIST_REMOVE( &(aDtxInfo->mNode) );

    aDtxInfo->removeAllBranchTx();

    aDtxInfo->finalize();
    (void)iduMemMgr::free( aDtxInfo );
    aDtxInfo = NULL;

    if ( aType == DK_NOTIFY_NORMAL )
    {
        mDtxInfoCnt--;
    }
    else if ( aType == DK_NOTIFY_FAILOVER )
    {
        mFailoverDtxInfoCnt--;
    }
    else
    {
        IDE_DASSERT( 0 );
    }

    IDE_ASSERT( mNotifierDtxInfoMutex.unlock() == IDE_SUCCESS );
}

void dktNotifier::failoverNotify()
{
    UInt           sResultCode = DKP_RC_FAILED;
    UInt           sCountFailXID = 0;
    ID_XID       * sFailXIDs = NULL;
    SInt         * sFailErrCodes = NULL;
    iduListNode  * sDtxInfoIterator = NULL;
    iduListNode  * sNext = NULL;
    dktDtxInfo   * sDtxInfo = NULL;
    UInt           sCountHeuristicXID = 0;
    ID_XID       * sHeuristicXIDs = NULL;
    dksSession   * sDummySession  = NULL;

    IDE_TEST_CONT( IDU_LIST_IS_EMPTY( &mFailoverDtxInfo ) == ID_TRUE, EXIT_LABEL );

    IDU_LIST_ITERATE_SAFE( &mFailoverDtxInfo, sDtxInfoIterator, sNext )
    {
        if ( ( mExit == ID_TRUE ) || ( mPause == ID_TRUE ) || ( mRestart == ID_TRUE ) )
        {
            break;
        }

        sDtxInfo = (dktDtxInfo *)sDtxInfoIterator->mObj;
        if ( sDtxInfo->mIsFailoverRequestNode == ID_TRUE )
        {
            if ( notifyXaResultForShard( sDtxInfo,
                                         sDummySession,
                                         &sResultCode,
                                         &sCountFailXID,
                                         &sFailXIDs,
                                         &sFailErrCodes,
                                         &sCountHeuristicXID,
                                         &sHeuristicXIDs )
                 != IDE_SUCCESS )
            {
                IDE_SET( ideSetErrorCode( dkERR_ABORT_XA_APPLY_FAIL,
                                          sDtxInfo->mResult,
                                          sDtxInfo->mGlobalTxId ) );
                continue;
            }
        }
        else
        {
            if ( askResultToRequestNode( sDtxInfo, &sResultCode ) != IDE_SUCCESS )
            {
                IDE_SET( ideSetErrorCode( dkERR_ABORT_XA_APPLY_FAIL,
                                          sDtxInfo->mResult,
                                          sDtxInfo->mGlobalTxId ) );
                continue;
            }
        }

        if ( sResultCode == DKP_RC_SUCCESS )
        {
            (void)removeEndedDtxInfo( DK_NOTIFY_FAILOVER,
                                      sDtxInfo->mLocalTxId,
                                      sDtxInfo->mGlobalTxId );
        }
    }

    mRunFailoverOneCycle = ID_TRUE;

    EXIT_LABEL:

    return;
}

IDE_RC dktNotifier::askResultToRequestNode( dktDtxInfo * aDtxInfo, UInt * aResultCode )
{
    idBool sIsCommit          = ID_FALSE;
    idBool sIsFindRequestNode = ID_FALSE;
    smSCN  sGlobalCommitSCN;

    if ( ( aDtxInfo->mResult != SMI_DTX_COMMIT ) && 
         ( aDtxInfo->mResult != SMI_DTX_ROLLBACK ) )
    {
        /* 모든 노드를 돌면서 request node 를 찾는다. 
         * 찾지 못할 경우 prepare 상태로 두고 넘어간다. */
        SMI_INIT_SCN( &sGlobalCommitSCN );
        if ( sdi::findRequestNodeNGetResultWithoutSession( &(aDtxInfo->mXID), 
                                                           &sIsFindRequestNode,
                                                           &sIsCommit,
                                                           &sGlobalCommitSCN ) 
             != IDE_SUCCESS )
        {
            IDE_ERRLOG( IDE_DK_3 );
        }

        IDE_TEST_RAISE( sIsFindRequestNode != ID_TRUE, ERR_NOT_FIND_REQUEST_NODE );
       
        aDtxInfo->globalTxResultLock();

        aDtxInfo->mResult = ( sIsCommit == ID_TRUE ) ? SMI_DTX_COMMIT : SMI_DTX_ROLLBACK;
        SM_SET_SCN( &(aDtxInfo->mGlobalCommitSCN), &sGlobalCommitSCN );
        
        aDtxInfo->globalTxResultUnlock();
    }

    if ( aDtxInfo->mResult == SMI_DTX_COMMIT )
    {
        IDE_TEST( aDtxInfo->mFailoverTrans->mSmiTrans.commit( &(aDtxInfo->mGlobalCommitSCN) ) 
                  != IDE_SUCCESS );
    }
    else
    {
        IDE_ASSERT( aDtxInfo->mFailoverTrans->mSmiTrans.rollback() == IDE_SUCCESS );
    }


    if ( aDtxInfo->mFailoverTrans->mSmiTrans.destroy( NULL ) != IDE_SUCCESS )
    {
        IDE_ERRLOG( IDE_DK_3 );
    }
    (void)iduMemMgr::free( aDtxInfo->mFailoverTrans );
    aDtxInfo->mFailoverTrans = NULL;

    *aResultCode = DKP_RC_SUCCESS;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_FIND_REQUEST_NODE );
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DK_INTERNAL_ERROR ,
                                  "[dktNotifier] askResultToRequestNode Request node not found" ) );
    }
    IDE_EXCEPTION_END;

    *aResultCode = DKP_RC_FAILED;

    return IDE_FAILURE;
}

void dktNotifier::notify()
{
    UInt           sResultCode = DKP_RC_FAILED;
    UInt           sCountFailXID = 0;
    ID_XID       * sFailXIDs = NULL;
    SInt         * sFailErrCodes = NULL;
    iduListNode  * sDtxInfoIterator = NULL;
    iduListNode  * sNext = NULL;
    dktDtxInfo   * sDtxInfo = NULL;
    UInt           sCountHeuristicXID = 0;
    ID_XID       * sHeuristicXIDs = NULL;
    idBool         sDblinkStarted = ID_FALSE;
    dktLinkerType  sLinkerType = DKT_LINKER_TYPE_NONE;

    IDE_TEST_CONT( IDU_LIST_IS_EMPTY( &mDtxInfo ) == ID_TRUE, EXIT_LABEL );

    if ( ( DKU_DBLINK_ENABLE == DK_ENABLE ) &&
         ( dkaLinkerProcessMgr::getLinkerStatus() != DKA_LINKER_STATUS_NON ) &&
         ( dkaLinkerProcessMgr::checkAltiLinkerEnabledFromConf() == ID_TRUE ) )
    {
        sDblinkStarted = ID_TRUE;
    }
    else
    {
        /* Nothing to do */
    }

    IDU_LIST_ITERATE_SAFE( &mDtxInfo, sDtxInfoIterator, sNext )
    {
        if ( ( mExit == ID_TRUE ) || ( mPause == ID_TRUE ) || ( mRestart == ID_TRUE ) )
        {
            break;
        }
        else
        {
            /* Nothing to do */
        }

        sDtxInfo = (dktDtxInfo *)sDtxInfoIterator->mObj;
        sLinkerType = sDtxInfo->getLinkerType();

        if ( sLinkerType == DKT_LINKER_TYPE_DBLINK )
        {
            if ( mSession != NULL )
            {
                if ( ( sDblinkStarted == ID_FALSE ) ||
                     ( mSession->mIsNeedToDisconnect == ID_TRUE ) )
                {
                    /* altilinker가 stop이거나, need to disconnect인 경우 */
                    continue;
                }
                else
                {
                    /* Nothing to do */
                }
            }
            else
            {
                /* DKT_LINKER_TYPE_DBLINK인 경우, notifyXaResult()에서 mSession을 사용한다. */
                continue;
            }
        }
        else
        {
            /* DKT_LINKER_TYPE_SHARD인 경우, notifyXaResult()에서 mSession을 사용하지 않는다. */

            if ( sDtxInfo->mIsPassivePending == ID_TRUE )
            {
                continue;
            }
        }

        if ( notifyXaResult( sDtxInfo,
                             mSession,
                             &sResultCode,
                             &sCountFailXID,
                             &sFailXIDs,
                             &sFailErrCodes,
                             &sCountHeuristicXID,
                             &sHeuristicXIDs )
             != IDE_SUCCESS )
        {
            IDE_ERRLOG(IDE_DK_3);
            IDE_SET( ideSetErrorCode( dkERR_ABORT_XA_APPLY_FAIL,
                                      sDtxInfo->mResult,
                                      sDtxInfo->mGlobalTxId ) );
            continue;
        }
        else
        {
            /*do nothing*/
        }

        if ( sResultCode == DKP_RC_SUCCESS )
        {
            writeNotifyHeuristicXIDLog( sDtxInfo,
                                        sCountHeuristicXID,
                                        sHeuristicXIDs );

            (void)removeEndedDtxInfo( DK_NOTIFY_NORMAL,
                                      sDtxInfo->mLocalTxId,
                                      sDtxInfo->mGlobalTxId );
        }
        else
        {
            /* 원격장애 시간이 긴 경우, 지나치게 많은 로깅이 발생할 것임. */
            writeNotifyFailLog( sFailXIDs,
                                sFailErrCodes,
                                sDtxInfo );
        }

        freeXaResult( sLinkerType,
                      sFailXIDs,
                      sHeuristicXIDs,
                      sFailErrCodes );

        /* init */
        sFailXIDs      = NULL;
        sFailErrCodes  = NULL;
        sHeuristicXIDs = NULL;
    }

    EXIT_LABEL:

    return;
}

IDE_RC dktNotifier::notifyXaResult( dktDtxInfo  * aDtxInfo,
                                    dksSession  * aSession,
                                    UInt        * aResultCode,
                                    UInt        * aCountFailXID,
                                    ID_XID     ** aFailXIDs,
                                    SInt       ** aFailErrCodes,
                                    UInt        * aCountHeuristicXID,
                                    ID_XID     ** aHeuristicXIDs )
{
    switch ( aDtxInfo->getLinkerType() )
    {
        case DKT_LINKER_TYPE_DBLINK:
            IDE_TEST( notifyXaResultForDBLink( aDtxInfo,
                                               aSession,
                                               aResultCode,
                                               aCountFailXID,
                                               aFailXIDs,
                                               aFailErrCodes,
                                               aCountHeuristicXID,
                                               aHeuristicXIDs )
                      != IDE_SUCCESS );
            break;

        case DKT_LINKER_TYPE_SHARD:
            IDE_TEST( notifyXaResultForShard( aDtxInfo,
                                              aSession,
                                              aResultCode,
                                              aCountFailXID,
                                              aFailXIDs,
                                              aFailErrCodes,
                                              aCountHeuristicXID,
                                              aHeuristicXIDs )
                      != IDE_SUCCESS );
            break;

        default:
            break;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

void dktNotifier::freeXaResult( dktLinkerType   aLinkerType,
                                ID_XID        * aFailXIDs,
                                ID_XID        * aHeuristicXIDs,
                                SInt          * aFailErrCodes )
{
    switch ( aLinkerType )
    {
        case DKT_LINKER_TYPE_DBLINK:
            dkpProtocolMgr::freeXARecvResult( NULL,
                                              aFailXIDs,
                                              aHeuristicXIDs,
                                              aFailErrCodes );
            break;

        case DKT_LINKER_TYPE_SHARD:
            /* Nothing to do */
            break;

        default:
            break;
    }
}

IDE_RC dktNotifier::notifyXaResultForDBLink( dktDtxInfo  * aDtxInfo,
                                             dksSession  * aSession,
                                             UInt        * aResultCode,
                                             UInt        * aCountFailXID,
                                             ID_XID     ** aFailXIDs,
                                             SInt       ** aFailErrCodes,
                                             UInt        * aCountHeuristicXID,
                                             ID_XID     ** aHeuristicXIDs )
{
    SInt sReceiveTimeout = 0;

    IDE_TEST( dkaLinkerProcessMgr::getAltiLinkerReceiveTimeoutFromConf( &sReceiveTimeout )
              != IDE_SUCCESS );

    if ( aDtxInfo->mResult == SMI_DTX_COMMIT )
    {
        /* FIT POINT: PROJ-2569 Notify Before send commit request */ 
        IDU_FIT_POINT("dktNotifier::notifyXaResult::dkpProtocolMgr::sendXACommit::NOTIFY_BEFORE_SEND_COMMIT");

        IDE_TEST( dkpProtocolMgr::sendXACommit( aSession,
                                                mSessionId,
                                                aDtxInfo )
                  != IDE_SUCCESS );
                  
        /* FIT POINT: PROJ-2569 Notify after send commit request and before receive ack */ 
        IDU_FIT_POINT("dktNotifier::notifyXaResult::dkpProtocolMgr::sendXACommit::NOTIFY_AFTER_SEND_COMMIT");

        IDE_TEST_CONT( mExit == ID_TRUE, EXIT_LABEL );
        IDE_TEST_CONT( mPause == ID_TRUE, EXIT_LABEL );

        IDE_TEST( dkpProtocolMgr::recvXACommitResult( aSession,
                                                      mSessionId,
                                                      aResultCode,
                                                      sReceiveTimeout,
                                                      aCountFailXID,
                                                      aFailXIDs,
                                                      aFailErrCodes,
                                                      aCountHeuristicXID,
                                                      aHeuristicXIDs )
                  != IDE_SUCCESS );

        /* FIT POINT: PROJ-2569 Notify after receive result */ 
        IDU_FIT_POINT("dktNotifier::notifyXaResult::dkpProtocolMgr::recvXACommitResult::NOTIFY_AFTER_RECV_RESULT");
    }
    else // aDtxInfo->mResult == SMI_DTX_ROLLBACK
    {
        IDE_TEST( dkpProtocolMgr::sendXARollback( aSession,
                                                  mSessionId,
                                                  aDtxInfo )
                  != IDE_SUCCESS );
        /* FIT POINT: PROJ-2569 Notify after send rollback */ 
        IDU_FIT_POINT("dktNotifier::notifyXaResult::dkpProtocolMgr::sendXARollback::NOTIFY_AFTER_SEND_ROLLBACK");

        IDE_TEST_CONT( mExit == ID_TRUE, EXIT_LABEL );
        IDE_TEST_CONT( mPause == ID_TRUE, EXIT_LABEL );

        IDE_TEST( dkpProtocolMgr::recvXARollbackResult( aSession,
                                                        mSessionId,
                                                        aResultCode,
                                                        sReceiveTimeout,
                                                        aCountFailXID,
                                                        aFailXIDs,
                                                        aFailErrCodes,
                                                        aCountHeuristicXID,
                                                        aHeuristicXIDs )
                  != IDE_SUCCESS );
    }

    IDE_TEST( smiWriteXaEndLog( aDtxInfo->mLocalTxId,
                                aDtxInfo->mGlobalTxId )
              != IDE_SUCCESS );

    EXIT_LABEL:

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC dktNotifier::notifyXaResultForShard( dktDtxInfo    * aDtxInfo,
                                            dksSession    * aSession,
                                            UInt          * aResultCode,
                                            UInt          * aCountFailXID,
                                            ID_XID       ** aFailXIDs,
                                            SInt         ** aFailErrCodes,
                                            UInt          * aCountHeuristicXID,
                                            ID_XID       ** aHeuristicXIDs )
{
    iduListNode         * sIter = NULL;
    dktDtxBranchTxInfo  * sDtxBranchTxInfo = NULL;
    idBool                sAllSuccess = ID_TRUE;
    ID_XID              * sXID = NULL;

    DK_UNUSED( aSession );
    DK_UNUSED( aCountFailXID );
    DK_UNUSED( aFailXIDs );
    DK_UNUSED( aFailErrCodes );
    DK_UNUSED( aCountHeuristicXID );
    DK_UNUSED( aHeuristicXIDs );

    IDU_FIT_POINT( "dktNotifier::notifyXaResultForShard::sendNotify" );

    IDU_LIST_ITERATE( &(aDtxInfo->mBranchTxInfo), sIter )
    {
        sDtxBranchTxInfo = (dktDtxBranchTxInfo*)sIter->mObj;
        if ( sDtxBranchTxInfo->mIsValid == ID_TRUE )
        {
            if ( aDtxInfo->mIsFailoverRequestNode != ID_TRUE )
            {
                sXID = &(sDtxBranchTxInfo->mXID);
            }
            else
            {
                sXID = &(aDtxInfo->mGlobalXID);
            }

            if ( notifyOneBranchXaResultForShard( aDtxInfo, 
                                                  sDtxBranchTxInfo,
                                                  sXID ) 
                 != IDE_SUCCESS )
            {
                IDE_ERRLOG(IDE_DK_3);
                sAllSuccess = ID_FALSE;
            }
            else
            {
                sDtxBranchTxInfo->mIsValid = ID_FALSE;
            }
        }
    }

    IDE_TEST( sAllSuccess != ID_TRUE );

    IDE_TEST( smiWriteXaEndLog( aDtxInfo->mLocalTxId,
                                aDtxInfo->mGlobalTxId )
              != IDE_SUCCESS );

    *aResultCode = DKP_RC_SUCCESS;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    *aResultCode = DKP_RC_FAILED;

    return IDE_FAILURE;
}

IDE_RC dktNotifier::notifyOneBranchXaResultForShard( dktDtxInfo          * aDtxInfo,
                                                     dktDtxBranchTxInfo  * aDtxBranchTxInfo,
                                                     ID_XID              * aXID )
{
    sdiConnectInfo        sDataNode;
    UChar                 sXidString[DKT_2PC_XID_STRING_LEN];

    IDE_DASSERT( aDtxBranchTxInfo->mLinkerType == 'S' );

    idlOS::memset( &sDataNode, 0x00, ID_SIZEOF(sdiConnectInfo) );

    /* set connect info */
    idlOS::strncpy( sDataNode.mNodeName,
                    aDtxBranchTxInfo->mData.mNode.mNodeName,
                    SDI_NODE_NAME_MAX_SIZE );
    sDataNode.mNodeName[ SDI_NODE_NAME_MAX_SIZE ] = '\0';

    idlOS::strncpy( sDataNode.mUserName,
                    aDtxBranchTxInfo->mData.mNode.mUserName,
                    QCI_MAX_OBJECT_NAME_LEN );
    sDataNode.mUserName[ QCI_MAX_OBJECT_NAME_LEN ] = '\0';

    idlOS::strncpy( sDataNode.mUserPassword,
                    aDtxBranchTxInfo->mData.mNode.mUserPassword,
                    IDS_MAX_PASSWORD_LEN );
    sDataNode.mUserPassword[ IDS_MAX_PASSWORD_LEN ] = '\0';

    idlOS::strncpy( sDataNode.mNodeInfo.mNodeName,
                    aDtxBranchTxInfo->mData.mNode.mNodeName,
                    SDI_NODE_NAME_MAX_SIZE );
    sDataNode.mNodeInfo.mNodeName[ SDI_NODE_NAME_MAX_SIZE ] = '\0';

    idlOS::strncpy( sDataNode.mNodeInfo.mServerIP,
                    aDtxBranchTxInfo->mData.mNode.mServerIP,
                    SDI_SERVER_IP_SIZE - 1 );
    sDataNode.mNodeInfo.mServerIP[ SDI_SERVER_IP_SIZE - 1 ] = '\0';

    sDataNode.mNodeInfo.mPortNo = aDtxBranchTxInfo->mData.mNode.mPortNo;
    sDataNode.mConnectType = aDtxBranchTxInfo->mData.mNode.mConnectType;

    sDataNode.mFlag &= ~SDI_CONNECT_INITIAL_BY_NOTIFIER_MASK;
    sDataNode.mFlag |= SDI_CONNECT_INITIAL_BY_NOTIFIER_TRUE;

    sDataNode.mFlag &= ~SDI_CONNECT_USER_AUTOCOMMIT_MODE_MASK;
    sDataNode.mFlag |= SDI_CONNECT_USER_AUTOCOMMIT_MODE_OFF;

    sDataNode.mFlag &= ~SDI_CONNECT_COORD_AUTOCOMMIT_MODE_MASK;
    sDataNode.mFlag |= SDI_CONNECT_COORD_AUTOCOMMIT_MODE_OFF;

    // BUG-45411
    IDE_TEST( sdi::allocConnect( &sDataNode ) != IDE_SUCCESS );

    dktXid::copyXID( &(sDataNode.mXID), aXID );

    if ( aDtxInfo->mResult == SMI_DTX_COMMIT )
    {
        /* PROJ-2733-DistTxInfo */
        if ( SM_SCN_IS_NOT_INIT( aDtxInfo->mGlobalCommitSCN ) )
        {
            IDE_TEST_RAISE( sdi::setSCN( &sDataNode, &aDtxInfo->mGlobalCommitSCN ) != IDE_SUCCESS,
                            ERR_SET_SCN);
        }
        IDE_TEST( sdi::endPendingTran( &sDataNode, ID_TRUE ) != IDE_SUCCESS );
    }
    else
    {
        IDE_TEST( sdi::endPendingTran( &sDataNode, ID_FALSE ) != IDE_SUCCESS );
    }

     if ( IDL_LIKELY_FALSE( IDE_TRC_SD_32 != 0 ) )
     {
            (void)idaXaConvertXIDToString( NULL,
                                           &(aDtxInfo->mXID),
                                           sXidString,
                                           DKT_2PC_XID_STRING_LEN );

            ideLog::log( IDE_SD_32, "[SHARED_TX_NOTIFY] NODE NAME:%s|XID:%s Notify send success",
                                    sDataNode.mNodeName, sXidString );
    }

    /* xaClose하지 않고 바로 끊어버린다. */
    sdi::freeConnectImmediately( &sDataNode );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_SET_SCN )
    {
        /* already set error code */
        ideLog::log( IDE_SD_0, "= [DKT] notifyXaResultForShard"
                               ", %s: failure setSCN, GlobalCommitSCN : %"ID_UINT64_FMT
                               ", %s",
                     sDataNode.mNodeName,
                     aDtxInfo->mGlobalCommitSCN,
                     ideGetErrorMsg( ideGetErrorCode() ) );
    }
    IDE_EXCEPTION_END;

    if ( IDE_TRC_DK_3 )
    {
        (void)idaXaConvertXIDToString(NULL,
                                      &(aDtxInfo->mXID),
                                      sXidString,
                                      DKT_2PC_XID_STRING_LEN);

        ideLog::log( DK_TRC_LOG_FORCE,
                     "Shard global transaction [TX_ID:%"ID_XINT64_FMT
                     ", XID:%s, NODE_NAME:%s] failure",
                     aDtxInfo->mGlobalTxId,
                     sXidString,
                     sDataNode.mNodeName );
    }
    else
    {
        /* Nothing to do */
    }

    if ( sDataNode.mDbc != NULL )
    {
        sdi::freeConnectImmediately( &sDataNode );
    }
    else
    {
        /* Nothing to do */
    }

    if ( IDL_LIKELY_FALSE( IDE_TRC_SD_32 != 0 ) )                                       \
    {
        (void)idaXaConvertXIDToString( NULL,
                                       &(aDtxInfo->mXID),
                                       sXidString,
                                       DKT_2PC_XID_STRING_LEN );

        ideLog::log( IDE_SD_32, "[SHARED_TX_NOTIFY] NODE NAME:%s|XID:%s Notify send fail",
                                sDataNode.mNodeName, sXidString );
    }

    return IDE_FAILURE;
}


idBool dktNotifier::findDtxInfo( DK_NOTIFY_TYPE aType,
                                 UInt aLocalTxId, 
                                 UInt aGlobalTxId, 
                                 dktDtxInfo ** aDtxInfo )
{
    iduListNode * sDtxInfoIterator = NULL;
    dktDtxInfo  * sDtxInfo         = NULL;
    idBool        sIsFind          = ID_FALSE;
    iduList     * sDtxInfoList     = NULL;

    IDE_DASSERT( *aDtxInfo == NULL );

    if ( aType == DK_NOTIFY_NORMAL )
    {
        sDtxInfoList = &mDtxInfo;
    }
    else if ( aType == DK_NOTIFY_FAILOVER )
    {
        sDtxInfoList = &mFailoverDtxInfo;
    }
    else
    {
        IDE_DASSERT( 0 );
    }


    IDE_ASSERT( mNotifierDtxInfoMutex.lock( NULL /*idvSQL* */ ) == IDE_SUCCESS );
    IDU_LIST_ITERATE( sDtxInfoList, sDtxInfoIterator )
    {
        sDtxInfo = (dktDtxInfo *)sDtxInfoIterator->mObj;
        if ( ( sDtxInfo->mGlobalTxId == aGlobalTxId ) &&
             ( sDtxInfo->mLocalTxId == aLocalTxId ) )
        {
            *aDtxInfo = sDtxInfo;
            sIsFind = ID_TRUE;
            break;
        }
        else
        {
            /* Nothing to do */
        }
    }
    IDE_ASSERT( mNotifierDtxInfoMutex.unlock() == IDE_SUCCESS );

    return sIsFind;
}

idBool dktNotifier::findDtxInfoByXID( DK_NOTIFY_TYPE    aType,
                                      idBool            aLocked,
                                      ID_XID          * aXID,
                                      dktDtxInfo     ** aDtxInfo )
{
    iduListNode * sDtxInfoIterator = NULL;
    dktDtxInfo  * sDtxInfo         = NULL;
    idBool        sIsFind          = ID_FALSE;
    iduList     * sDtxInfoList     = NULL;

    IDE_DASSERT( *aDtxInfo == NULL );

    if ( aType == DK_NOTIFY_NORMAL )
    {
        sDtxInfoList = &mDtxInfo;
    }
    else if ( aType == DK_NOTIFY_FAILOVER )
    {
        sDtxInfoList = &mFailoverDtxInfo;
    }
    else
    {
        IDE_DASSERT( 0 );
    }

    if ( aLocked != ID_TRUE )
    {
        IDE_ASSERT( mNotifierDtxInfoMutex.lock( NULL /*idvSQL* */ ) == IDE_SUCCESS );
    }

    IDU_LIST_ITERATE( sDtxInfoList, sDtxInfoIterator )
    {
        sDtxInfo = (dktDtxInfo *)sDtxInfoIterator->mObj;
        if ( dktXid::isEqualXID( &(sDtxInfo->mXID), aXID ) == ID_TRUE )
        {
            *aDtxInfo = sDtxInfo;
            sIsFind = ID_TRUE;
            break;
        }
    }
    
    if ( aLocked != ID_TRUE )
    {
        IDE_ASSERT( mNotifierDtxInfoMutex.unlock() == IDE_SUCCESS );
    }

    return sIsFind;
}


IDE_RC dktNotifier::removeEndedDtxInfo( DK_NOTIFY_TYPE aType,
                                        UInt     aLocalTxId,
                                        UInt     aGlobalTxId )
{
    dktDtxInfo * sDtxInfo  = NULL;

    IDE_TEST_RAISE( findDtxInfo( aType, 
                                 aLocalTxId, 
                                 aGlobalTxId, 
                                 &sDtxInfo )
                    != ID_TRUE, ERR_NOT_EXIST_DTX_INFO );

    removeDtxInfo( aType, sDtxInfo );
    sDtxInfo = NULL;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_EXIST_DTX_INFO );
    {
        ideLog::log( DK_TRC_LOG_FORCE, DK_TRC_T_NOT_EXIST_DTX_INFO );
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DK_INTERNAL_ERROR,
                                  "[dktNotifier::removeEndedDtxInfo] sDtxInfo is not exist" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

idBool dktNotifier::setResultPassiveDtxInfo( ID_XID * aXID, idBool aCommit )
{
    dktDtxInfo  * sDtxInfo         = NULL;
    iduListNode * sDtxInfoIterator = NULL;
    idBool        sIsFind          = ID_FALSE;


    IDE_ASSERT( mNotifierDtxInfoMutex.lock( NULL /*idvSQL* */ ) == IDE_SUCCESS );
    IDU_LIST_ITERATE( &mDtxInfo, sDtxInfoIterator )
    {
        sDtxInfo = (dktDtxInfo *)sDtxInfoIterator->mObj;
        if ( dktXid::isEqualXID( &sDtxInfo->mXID, aXID ) == ID_TRUE )
        {
            sIsFind = ID_TRUE;

            if ( sDtxInfo->mIsPassivePending == ID_TRUE )
            {
                if ( aCommit == ID_TRUE )
                {
                    sDtxInfo->mResult = SMI_DTX_COMMIT;
                }
                else
                {
                    sDtxInfo->mResult = SMI_DTX_ROLLBACK;
                }

                sDtxInfo->mIsPassivePending = ID_FALSE;
            }

            /* continue to search more than one. */
        }
        else
        {
            /* Nothing to do */
        }
    
    }
    IDE_ASSERT( mNotifierDtxInfoMutex.unlock() == IDE_SUCCESS );

    return sIsFind;
}

idBool dktNotifier::setResultFailoverDtxInfo( ID_XID * aXID, 
                                              idBool   aCommit,
                                              smSCN  * aGlobalCommitSCN )
{
    dktDtxInfo  * sDtxInfo         = NULL;
    iduListNode * sDtxInfoIterator = NULL;
    idBool        sIsFind          = ID_FALSE;

    IDE_ASSERT( mNotifierDtxInfoMutex.lock( NULL /*idvSQL* */ ) == IDE_SUCCESS );
    
    IDU_LIST_ITERATE( &mFailoverDtxInfo, sDtxInfoIterator )
    {
        sDtxInfo = (dktDtxInfo *)sDtxInfoIterator->mObj;
        if ( dktXid::isEqualGlobalXID( &sDtxInfo->mXID, aXID ) == ID_TRUE )
        {
            sDtxInfo->globalTxResultLock();

            sIsFind = ID_TRUE;
   
            if ( aCommit == ID_TRUE )
            {
                sDtxInfo->mResult = SMI_DTX_COMMIT;
                SM_SET_SCN( &(sDtxInfo->mGlobalCommitSCN), aGlobalCommitSCN );
            }
            else
            {
                sDtxInfo->mResult = SMI_DTX_ROLLBACK;
            }

            /* Failover DtxInfo 의 빠른 처리를 위하여 Restart 시킨다. */
            mRestart = ID_TRUE;

            sDtxInfo->globalTxResultUnlock();
        }
    }

    IDE_ASSERT( mNotifierDtxInfoMutex.unlock() == IDE_SUCCESS );

    return sIsFind;
}

void dktNotifier::writeNotifyHeuristicXIDLog( dktDtxInfo   * aDtxInfo,
                                              UInt           aCountHeuristicXID,
                                              ID_XID       * aHeuristicXIDs )
{
    ideLogEntry    sLog( IDE_DK_0 );
    UChar          sXidString[DKT_2PC_XID_STRING_LEN];

    if ( ( aDtxInfo->getLinkerType() == DKT_LINKER_TYPE_DBLINK ) &&
         ( aCountHeuristicXID > 0 ) )
    {
        (void)sLog.appendFormat( "Heuristic Completed. Global Tx ID : %"ID_XINT32_FMT,
                            aDtxInfo->mGlobalTxId );
        (void)sLog.appendFormat( ", Count : %"ID_UINT32_FMT"\n", aCountHeuristicXID );
        while ( aCountHeuristicXID > 0 )
        {
            aCountHeuristicXID--;
            (void)idaXaConvertXIDToString(NULL,
                                          &(aHeuristicXIDs[aCountHeuristicXID]),
                                          sXidString,
                                          DKT_2PC_XID_STRING_LEN);

            (void)sLog.appendFormat( "XID : %s\n", sXidString );
        }
        sLog.write();
    }
    else
    {
        /* Nothing to do */
    }
}

void dktNotifier::writeNotifyFailLog( ID_XID       * aFailXIDs, 
                                      SInt         * aFailErrCodes, 
                                      dktDtxInfo   * aDtxInfo ) 
{
    UChar  sXidString[DKT_2PC_XID_STRING_LEN];

    if ( ( aDtxInfo->getLinkerType() == DKT_LINKER_TYPE_DBLINK ) &&
         ( IDE_TRC_DK_3 ) )
    {
        if ( ( aFailErrCodes != NULL ) && ( aFailXIDs != NULL ) )
        {
            (void)idaXaConvertXIDToString(NULL,
                                          &(aFailXIDs[0]),
                                          sXidString,
                                          DKT_2PC_XID_STRING_LEN);

            ideLog::log( DK_TRC_LOG_FORCE, DK_TRC_T_NOTIFIER_XA_APPLY_RESULT_CODE,
                         aDtxInfo->mResult, aDtxInfo->mGlobalTxId,
                         sXidString, aFailErrCodes[0] );
        }
        else
        {
            ideLog::log( DK_TRC_LOG_FORCE, " global transaction: %"ID_UINT32_FMT
                         " was failed with result code %"ID_UINT32_FMT,
                         aDtxInfo->mGlobalTxId, aDtxInfo->mResult );
        }
    }
    else
    {
        /* Nothing to do */
    }
}

IDE_RC dktNotifier::setResult( DK_NOTIFY_TYPE aType,
                               UInt    aLocalTxId,
                               UInt    aGlobalTxId,
                               UChar   aResult,
                               smSCN * aGlobalCommitSCN )
{
    dktDtxInfo * sDtxInfo  = NULL;

    IDE_TEST_RAISE( findDtxInfo( aType, 
                                 aLocalTxId, 
                                 aGlobalTxId, 
                                 &sDtxInfo )
                    != ID_TRUE, ERR_NOT_EXIST_DTX_INFO );

    sDtxInfo->mResult = aResult;
    if ( aGlobalCommitSCN != NULL )
    {
        SM_SET_SCN( &sDtxInfo->mGlobalCommitSCN, aGlobalCommitSCN );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_EXIST_DTX_INFO );
    {
        ideLog::log( DK_TRC_LOG_FORCE, DK_TRC_T_NOT_EXIST_DTX_INFO );
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DK_INTERNAL_ERROR ,
                                  "[dktNotifier::setResult] sDtxInfo is not exist" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

void dktNotifier::finalize()
{
    destroyDtxInfoList();
    mDtxInfoCnt = 0;

    IDE_TEST( mNotifierDtxInfoMutex.destroy() != IDE_SUCCESS );

    return;

    IDE_EXCEPTION_END;

    IDE_ERRLOG(IDE_DK_0);

    return;
}

void dktNotifier::run()
{
    while ( mExit != ID_TRUE )
    {
        IDE_CLEAR();

        idlOS::sleep(1);
        if ( mPause == ID_FALSE )
        {
            (void) failoverNotify();
            (void) notify();

            mRestart = ID_FALSE;
        }
        else
        {
            mRunFailoverOneCycle = ID_FALSE;
        }

    } /* while */

    finalize();

    return;
}

void dktNotifier::destroyDtxInfoList()
{
    iduList     * sIterator = NULL;
    iduListNode * sNext      = NULL;
    dktDtxInfo  * sDtxInfo = NULL;

    IDU_LIST_ITERATE_SAFE( &mDtxInfo, sIterator, sNext )
    {
        sDtxInfo = (dktDtxInfo *) sIterator->mObj;

        removeDtxInfo( DK_NOTIFY_NORMAL, sDtxInfo );
        sDtxInfo = NULL;
    }

    IDE_DASSERT( mDtxInfoCnt == 0 );

    IDU_LIST_ITERATE_SAFE( &mFailoverDtxInfo, sIterator, sNext )
    {
        sDtxInfo = (dktDtxInfo *) sIterator->mObj;

        removeDtxInfo( DK_NOTIFY_FAILOVER, sDtxInfo );
        sDtxInfo = NULL;
    }

    IDE_DASSERT( mFailoverDtxInfoCnt == 0 );

    return;
}

IDE_RC dktNotifier::createDtxInfo( DK_NOTIFY_TYPE aType,
                                   ID_XID      * aGlobalXID,
                                   UInt          aLocalTxId, 
                                   UInt          aGlobalTxId, 
                                   idBool        aIsRequestNode,
                                   dktDtxInfo ** aDtxInfo )
{
    dktDtxInfo * sDtxInfo = NULL;

    if ( findDtxInfo( aType,
                      aLocalTxId, 
                      aGlobalTxId, 
                      &sDtxInfo ) 
         == ID_FALSE )
    {
        IDE_TEST_RAISE( iduMemMgr::malloc( IDU_MEM_DK,
                                           ID_SIZEOF( dktDtxInfo ),
                                           (void **)&sDtxInfo,
                                           IDU_MEM_IMMEDIATE )
                        != IDE_SUCCESS, ERR_MEMORY_ALLOC_DTX_INFO_HEADER );

        IDE_TEST( sDtxInfo->initialize( aGlobalXID, 
                                        aLocalTxId, 
                                        aGlobalTxId,
                                        aIsRequestNode )
                  != IDE_SUCCESS );

        addDtxInfo( aType, sDtxInfo );
    }
    else
    {
        /* exist already. it means prepare logging twice. and it's possible. */
        sDtxInfo = NULL;
    }

    *aDtxInfo = sDtxInfo;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_MEMORY_ALLOC_DTX_INFO_HEADER );
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_MEMORY_ALLOCATION ) );
    }
    IDE_EXCEPTION_END;

    if ( sDtxInfo != NULL )
    {
        (void)iduMemMgr::free( sDtxInfo );
        sDtxInfo = NULL;
    }
    else
    {
        /* do nothing */
    }

    return IDE_FAILURE;
}

/* sm recovery redo중에 dtx info를 구성하고, Service 시작 단계에서 notifier에게 넘겨준다 */
IDE_RC  dktNotifier::manageDtxInfoListByLog( ID_XID * aXID, 
                                             UInt     aLocalTxId,
                                             UInt     aGlobalTxId,
                                             UInt     aBranchTxInfoSize,
                                             UChar  * aBranchTxInfo,
                                             smLSN  * aPrepareLSN,
                                             smSCN  * aGlobalCommitSCN,
                                             UChar    aType )
{
    dktDtxInfo * sDtxInfo = NULL;

    switch ( aType )
    {
        case SMI_DTX_PREPARE :
            /* PREPARE_REQ LOG */
            IDE_DASSERT( aBranchTxInfoSize != 0 );
            IDE_TEST( createDtxInfo( DK_NOTIFY_NORMAL,
                                     aXID, /* GlobalXID */ 
                                     aLocalTxId, 
                                     aGlobalTxId, 
                                     ID_FALSE,
                                     &sDtxInfo ) 
                      != IDE_SUCCESS );
            if ( sDtxInfo != NULL )
            {
                IDE_TEST( sDtxInfo->unserializeAndAddDtxBranchTx( aBranchTxInfo,
                                                                  aBranchTxInfoSize )
                          != IDE_SUCCESS );
                idlOS::memcpy( &(sDtxInfo->mPrepareLSN), aPrepareLSN, ID_SIZEOF( smLSN ) );
            }
            else
            {
                /* Nothing to do */
            }
            break;
        case SMI_DTX_COMMIT :
        case SMI_DTX_ROLLBACK :
            (void)setResult( DK_NOTIFY_NORMAL,
                             aLocalTxId, 
                             aGlobalTxId, 
                             aType, 
                             aGlobalCommitSCN );
            break;
        case SMI_DTX_END :
            (void)removeEndedDtxInfo( DK_NOTIFY_NORMAL, 
                                      aLocalTxId, 
                                      aGlobalTxId );
            break;
        default :
            IDE_RAISE( ERR_UNKNOWN_TYPE );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_UNKNOWN_TYPE );
    {
        ideLog::log( DK_TRC_LOG_FORCE, DK_TRC_T_UNKNOWN_DTX_TYPE );
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DK_INTERNAL_ERROR,
                                  "[dktNotifier::manageDtxInfoListByLog] Unknown Type" ) );
        IDE_ERRLOG(IDE_DK_0);
        IDE_CALLBACK_FATAL("Can't be here");
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC dktNotifier::getNotifierTransactionInfo( dktNotifierTransactionInfo ** aInfo,
                                                UInt                        * aInfoCount )
{
    UInt                 sIndex = 0;
    dktDtxBranchTxInfo * sBranchTxInfo = NULL;
    dktDtxInfo         * sDtxInfo = NULL;
    iduListNode        * sIterator       = NULL;
    iduListNode        * sBranchIterator = NULL;
    UInt                 sAllBranchTxCnt = 0;
    dktNotifierTransactionInfo * sInfo = NULL;

    IDE_ASSERT( mNotifierDtxInfoMutex.lock( NULL /*idvSQL* */ ) == IDE_SUCCESS );

    sAllBranchTxCnt = getAllBranchTxCnt();

    IDE_TEST_RAISE( iduMemMgr::calloc( IDU_MEM_DK,
                                       sAllBranchTxCnt,
                                       ID_SIZEOF( dktNotifierTransactionInfo ),
                                       (void **)&sInfo,
                                       IDU_MEM_IMMEDIATE )
                    != IDE_SUCCESS, ERR_MEMORY_ALLOC );

    IDU_LIST_ITERATE( &mDtxInfo, sIterator )
    {
        sDtxInfo = (dktDtxInfo *)sIterator->mObj;

        IDU_LIST_ITERATE( &(sDtxInfo->mBranchTxInfo), sBranchIterator )
        {
            sBranchTxInfo = (dktDtxBranchTxInfo *)sBranchIterator->mObj;

            sInfo[sIndex].mGlobalTransactionId = sDtxInfo->mGlobalTxId;
            sInfo[sIndex].mLocalTransactionId = sDtxInfo->mLocalTxId;

            if ( sDtxInfo->mIsPassivePending == ID_TRUE )
            {
                idlOS::strncpy( sInfo[sIndex].mTransactionResult, "PASSIVE", 8 );
            }
            else
            {
                if ( sDtxInfo->mResult == SMI_DTX_COMMIT )
                {
                    idlOS::strncpy( sInfo[sIndex].mTransactionResult, "COMMIT", 7 );
                }
                else
                {
                    idlOS::strncpy( sInfo[sIndex].mTransactionResult, "ROLLBACK", 9 );
                }
            }
            dktXid::copyXID( &(sInfo[sIndex].mXID),
                             &(sBranchTxInfo->mXID) );
            dktXid::copyXID( &(sInfo[sIndex].mSourceXID),
                             &(sDtxInfo->mXID) );
            idlOS::memcpy( sInfo[sIndex].mTargetInfo,
                           sBranchTxInfo->mData.mTargetName,
                           DK_NAME_LEN + 1 );

            sIndex++;
        }
    }

    IDE_ASSERT( mNotifierDtxInfoMutex.unlock() == IDE_SUCCESS );

    IDE_DASSERT( sAllBranchTxCnt == sIndex );

    *aInfo = sInfo;
    *aInfoCount = sIndex;
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_MEMORY_ALLOC )
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_MEMORY_ALLOCATION ) );
    }
    IDE_EXCEPTION_END;

    if ( sInfo != NULL )
    {
        iduMemMgr::free( sInfo );
        sInfo = NULL;
    }
    else
    {
        /* do nothing */
    }

    IDE_ASSERT( mNotifierDtxInfoMutex.unlock() == IDE_SUCCESS );

    return IDE_FAILURE;
}

IDE_RC dktNotifier::getShardNotifierTransactionInfo( dktNotifierTransactionInfo ** aInfo,
                                                     UInt                        * aInfoCount )
{
    UInt                 sIndex = 0;
    dktDtxBranchTxInfo * sBranchTxInfo = NULL;
    dktDtxInfo         * sDtxInfo = NULL;
    iduListNode        * sIterator       = NULL;
    iduListNode        * sBranchIterator = NULL;
    UInt                 sAllBranchTxCnt = 0;
    dktNotifierTransactionInfo * sInfo = NULL;

    IDE_ASSERT( mNotifierDtxInfoMutex.lock( NULL /*idvSQL* */ ) == IDE_SUCCESS );

    sAllBranchTxCnt = getAllShardBranchTxCnt();

    IDE_TEST_RAISE( iduMemMgr::calloc( IDU_MEM_DK,
                                       sAllBranchTxCnt,
                                       ID_SIZEOF( dktNotifierTransactionInfo ),
                                       (void **)&sInfo,
                                       IDU_MEM_IMMEDIATE )
                    != IDE_SUCCESS, ERR_MEMORY_ALLOC );

    IDU_LIST_ITERATE( &mDtxInfo, sIterator )
    {
        sDtxInfo = (dktDtxInfo *)sIterator->mObj;

        IDU_LIST_ITERATE( &(sDtxInfo->mBranchTxInfo), sBranchIterator )
        {
            sBranchTxInfo = (dktDtxBranchTxInfo *)sBranchIterator->mObj;

            sInfo[sIndex].mGlobalTransactionId = sDtxInfo->mGlobalTxId;
            sInfo[sIndex].mLocalTransactionId = sDtxInfo->mLocalTxId;
            sInfo[sIndex].mTransactionState = sDtxInfo->mResult;
            sInfo[sIndex].mGlobalCommitSCN = sDtxInfo->mGlobalCommitSCN;
            sInfo[sIndex].mIsRequestNode = 1;

            if ( sDtxInfo->mIsPassivePending == ID_TRUE )
            {
                idlOS::strncpy( sInfo[sIndex].mTransactionResult, "PASSIVE", 8 );
            }
            else
            {
                if ( sDtxInfo->mResult == SMI_DTX_COMMIT )
                {
                    idlOS::strncpy( sInfo[sIndex].mTransactionResult, "COMMIT", 7 );
                }
                else
                {
                    idlOS::strncpy( sInfo[sIndex].mTransactionResult, "ROLLBACK", 9 );
                }
            }
            dktXid::copyXID( &(sInfo[sIndex].mGlobalXID),
                             &(sDtxInfo->mGlobalXID) );
            dktXid::copyXID( &(sInfo[sIndex].mXID),
                             &(sBranchTxInfo->mXID) );
            dktXid::copyXID( &(sInfo[sIndex].mSourceXID),
                             &(sDtxInfo->mXID) );
            idlOS::memcpy( sInfo[sIndex].mTargetInfo,
                           sBranchTxInfo->mData.mTargetName,
                           DK_NAME_LEN + 1 );

            sIndex++;
        }
    }

    IDU_LIST_ITERATE( &mFailoverDtxInfo, sIterator )
    {
        sDtxInfo = (dktDtxInfo *)sIterator->mObj;

        sInfo[sIndex].mGlobalTransactionId = sDtxInfo->mGlobalTxId;
        sInfo[sIndex].mLocalTransactionId = sDtxInfo->mLocalTxId;
        sInfo[sIndex].mTransactionState = sDtxInfo->mResult;
        sInfo[sIndex].mGlobalCommitSCN = sDtxInfo->mGlobalCommitSCN;

        sInfo[sIndex].mIsRequestNode = ( sDtxInfo->mIsFailoverRequestNode == ID_TRUE ) ? 1 : 0;
        if ( sDtxInfo->mResult == SMI_DTX_COMMIT )
        {
            idlOS::strncpy( sInfo[sIndex].mTransactionResult, "COMMIT", 7 );
        }
        else if ( sDtxInfo->mResult == SMI_DTX_ROLLBACK )
        {
            idlOS::strncpy( sInfo[sIndex].mTransactionResult, "ROLLBACK", 9 );
        }
        else
        {
            idlOS::strncpy( sInfo[sIndex].mTransactionResult, "PENDING", 7 );
        }

        dktXid::copyXID( &(sInfo[sIndex].mGlobalXID),
                         &(sDtxInfo->mGlobalXID) );
        dktXid::copyXID( &(sInfo[sIndex].mSourceXID),
                         &(sDtxInfo->mXID) );

        sIndex++;
    }

    IDE_ASSERT( mNotifierDtxInfoMutex.unlock() == IDE_SUCCESS );

    IDE_DASSERT( sAllBranchTxCnt == sIndex );

    *aInfo = sInfo;
    *aInfoCount = sIndex;
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_MEMORY_ALLOC )
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_MEMORY_ALLOCATION ) );
    }
    IDE_EXCEPTION_END;

    if ( sInfo != NULL )
    {
        iduMemMgr::free( sInfo );
        sInfo = NULL;
    }
    else
    {
        /* do nothing */
    }

    IDE_ASSERT( mNotifierDtxInfoMutex.unlock() == IDE_SUCCESS );

    return IDE_FAILURE;
}

/* notifier가 가진 모든 branchTx의 갯수를 반환. 호출하는 곳에서 mutex를 잡아야한다. */
UInt dktNotifier::getAllBranchTxCnt()
{
    dktDtxInfo         * sDtxInfo = NULL;
    iduListNode        * sIterator       = NULL;
    UInt                 sAllBranchTxCnt = 0;

    IDU_LIST_ITERATE( &mDtxInfo, sIterator )
    {
        sDtxInfo = (dktDtxInfo *)sIterator->mObj;
        sAllBranchTxCnt += sDtxInfo->mBranchTxCount;
    }

    return sAllBranchTxCnt;
}
 
UInt dktNotifier::getAllShardBranchTxCnt()
{
    dktDtxInfo  * sDtxInfo        = NULL;
    iduListNode * sIterator       = NULL;
    UInt          sAllBranchTxCnt = 0;

    IDU_LIST_ITERATE( &mDtxInfo, sIterator )
    {
        sDtxInfo = (dktDtxInfo *)sIterator->mObj;
        sAllBranchTxCnt += sDtxInfo->mBranchTxCount;
    }

    IDU_LIST_ITERATE( &mFailoverDtxInfo, sIterator )
    {
        sDtxInfo = (dktDtxInfo *)sIterator->mObj;
        sAllBranchTxCnt += 1;
    }

    return sAllBranchTxCnt;
}
  
IDE_RC dktNotifier::addUnCompleteGlobalTxList( iduList * aGlobalTxList )
{
    ID_XID      * sXID     = NULL;
    iduListNode * sNode    = NULL;
    iduListNode * sDummy   = NULL;
    dktDtxInfo  * sDtxInfo = NULL;
    dkiUnCompleteGlobalTxInfo * sGlobalTxNode = NULL;

    if ( IDU_LIST_IS_EMPTY( aGlobalTxList ) != ID_TRUE )
    {
        IDU_LIST_ITERATE_SAFE( aGlobalTxList, sNode, sDummy )
        {
            sGlobalTxNode = (dkiUnCompleteGlobalTxInfo*)sNode->mObj;
 
            if ( sGlobalTxNode->mIsRequestNode == ID_TRUE )
            {
                sXID = &(sGlobalTxNode->mXID);
            }
            else
            {
                sXID = NULL;
            }
          
            /* Failover list 는 xid 로 구분하기 때문에globaltxid 나 localtxid 가 의미가 없다 */
            IDE_TEST( createDtxInfo( DK_NOTIFY_FAILOVER,
                                     sXID,
                                     dktXid::getLocalTxIDFromXID( &(sGlobalTxNode->mXID) ),
                                     dktXid::getGlobalTxIDFromXID( &(sGlobalTxNode->mXID) ),
                                     sGlobalTxNode->mIsRequestNode,
                                     &sDtxInfo )
                      != IDE_SUCCESS );

            if ( sDtxInfo != NULL )
            {
                sDtxInfo->mResult = sGlobalTxNode->mResultType;
                if ( sGlobalTxNode->mIsRequestNode == ID_TRUE )
                {
                    if ( sdi::getAllBranchWithoutSession( (void*)sDtxInfo ) != IDE_SUCCESS )
                    {
                        IDE_ERRLOG( IDE_SD_0 );
                    }
                }

                if ( sDtxInfo->mResult == SMI_DTX_COMMIT )
                {
                    SM_SET_SCN( &sDtxInfo->mGlobalCommitSCN, &(sGlobalTxNode->mGlobalCommitSCN) );
                }
                sDtxInfo->mFailoverTrans = sGlobalTxNode->mTrans;
                sGlobalTxNode->mTrans = NULL;
            }
            else
            {
                /* Already Created */
            }
            sDtxInfo = NULL;
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

void dktNotifier::waitUntilFailoverNotifierRunOneCycle()
{
    PDL_Time_Value  sTimeValue;
    sTimeValue.initialize( 0, 1000 );

    while ( mExit != ID_TRUE )
    {
        if ( ( mPause != ID_TRUE ) && ( mRunFailoverOneCycle == ID_TRUE ) )
        {
            break;
        }

        idlOS::sleep( sTimeValue );
    }
}

