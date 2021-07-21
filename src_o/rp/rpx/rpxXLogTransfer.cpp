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
 **********************************************************************/

#include <rpnComm.h>
#include <rpxXLogTransfer.h>

rpxXLogTransfer::rpxXLogTransfer() : idtBaseThread()
{
}

IDE_RC rpxXLogTransfer::initialize( rpxReceiver * aReceiver, rpdXLogfileMgr * aXLogfileManager )
{
    UInt  sStage = 0;
    SChar sMutexName[IDU_MUTEX_NAME_LEN + 1] = { 0, };


    mExitFlag = &(aReceiver->mExitFlag);
    mLastCommitSN = SM_SN_NULL;
    mLastCommitTID = SM_NULL_TID;
    mLastWrittenSN = SM_SN_NULL;

    mIsSendAckThroughReceiver = ID_FALSE;

    mXLogfileManager = aXLogfileManager;
    mProtocolContext = NULL;

    mReceiver = aReceiver;
    mNetworkError = &(aReceiver->mNetworkError);

    mRestartSN = SM_SN_NULL;
    mLastProcessedSN = SM_SN_NULL;
    mProcessedXLogCount = 0;

    idlOS::strncpy( mRepName, aReceiver->mRepName, QCI_MAX_NAME_LEN );

    idlOS::snprintf( sMutexName,
                     IDU_MUTEX_NAME_LEN,
                     "RXT_%s_WAIT_RECEIVER_MUTEX",
                     mRepName );
    IDE_TEST_RAISE( mWaitForReceiverProcessDoneMutex.initialize( mRepName,
                                                                 IDU_MUTEX_KIND_POSIX,
                                                                 IDV_WAIT_INDEX_NULL )
                    != IDE_SUCCESS, ERR_INIT_WAIT_FOR_RECEIVER_MUTEX_VAR );
    sStage = 1;

    IDE_TEST_RAISE( mWaitForReceiverProcessDoneCV.initialize() != IDE_SUCCESS, ERR_INIT_COND_VAR );
    sStage = 2;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INIT_WAIT_FOR_RECEIVER_MUTEX_VAR )
    {
        IDE_ERRLOG( IDE_RP_0 );
        IDE_SET( ideSetErrorCode( rpERR_FATAL_ThrMutexInit ) );
        IDE_ERRLOG( IDE_RP_0 );

        IDE_CALLBACK_FATAL( "[Repl XLogTransfer] WaitForReceiver Mutex initialization error" );
    }
    IDE_EXCEPTION( ERR_INIT_COND_VAR )
    {
        IDE_SET( ideSetErrorCode( rpERR_FATAL_ThrCondInit ) );
        IDE_ERRLOG( IDE_RP_0 );

        IDE_CALLBACK_FATAL( "[Repl XLogTransfer] Condition variable initialization error" );
    }
    IDE_EXCEPTION_END;

    switch( sStage )
    {
        case 2:
            (void)mWaitForReceiverProcessDoneCV.destroy();
        case 1:
            (void)mWaitForReceiverProcessDoneMutex.destroy();
        default:
            break;
    }

    return IDE_FAILURE;
}

IDE_RC rpxXLogTransfer::finalize()
{
	*mExitFlag = ID_TRUE;

    return IDE_SUCCESS;
}

void rpxXLogTransfer::destroy()
{
    (void)mWaitForReceiverProcessDoneCV.destroy();
    (void)mWaitForReceiverProcessDoneMutex.destroy();
}

IDE_RC rpxXLogTransfer::initializeThread()
{
    return IDE_SUCCESS;
}

void rpxXLogTransfer::finalizeThread()
{
    return;
}

void rpxXLogTransfer::run()
{
    rpXLogType sXLogType = RP_X_NONE;

    while ( isExit() != ID_TRUE )
    {
        sXLogType = RP_X_NONE;

        IDE_TEST( receiveCMBufferAndWriteXLog( mProtocolContext,
                                               &sXLogType )
                  != IDE_SUCCESS );

        IDE_TEST( processXLogInXLogTransfer( sXLogType ) != IDE_SUCCESS );
    }

    finalize();

    return;

    IDE_EXCEPTION_END;
        
    IDE_ERRLOG(IDE_RP_0);

    finalize();

    return;
}

IDE_RC rpxXLogTransfer::writeXLog( cmiProtocolContext * aProtocolContext, UShort aWriteSize, smSN aXSN )
{
    UChar * sCurrentPosition = NULL;
  
    sCurrentPosition = CMI_GET_CURRENT_POSITION_IN_READ_BLOCK( aProtocolContext );
    IDE_TEST_RAISE( mXLogfileManager->writeXLog( sCurrentPosition, aWriteSize, aXSN )!= IDE_SUCCESS, ERR_FAIL_TO_WRITE_INTO_FILE );
 
    CMI_SKIP_READ_BLOCK( aProtocolContext, aWriteSize );
  
    return IDE_SUCCESS;
  
    IDE_EXCEPTION( ERR_FAIL_TO_WRITE_INTO_FILE  )
    {
        if ( ideGetErrorCode() != rpERR_ABORT_END_THREAD )
        {
            IDE_ERRLOG( IDE_RP_0 );

            IDE_SET( ideSetErrorCode( rpERR_ABORT_RPX_FAIL_WRITE_XLOG_IN_XLOGFILE ) );
        }
    }
    IDE_EXCEPTION_END;
  
    return IDE_FAILURE;
}

IDE_RC rpxXLogTransfer::writeValuesXLog( cmiProtocolContext * aProtocolContext )
{

    UInt sRecvValueLength = 0;
    UInt sRemainDataInCmBlock  = 0;
    UInt sRemainDataOfValue = 0;
    UChar sOp = 0;

    CMI_PEEK1( aProtocolContext, sOp, 0 );
    IDE_DASSERT( sOp == CMI_PROTOCOL_OPERATION( RP, Value ) );
    IDE_TEST( writeXLog( aProtocolContext, ID_SIZEOF( sOp ), SM_SN_NULL ) != IDE_SUCCESS );

    CMI_PEEK4( aProtocolContext, &sRecvValueLength, 0  );
    IDE_TEST( writeXLog( aProtocolContext, ID_SIZEOF( sRecvValueLength ), SM_SN_NULL ) != IDE_SUCCESS );

    if( sRecvValueLength > 0 )
    {
        sRemainDataOfValue = sRecvValueLength;
        sRemainDataInCmBlock = CMI_REMAIN_DATA_IN_READ_BLOCK( aProtocolContext );

        while ( sRemainDataOfValue > 0 )
        {
            if ( sRemainDataInCmBlock == 0 )
            {
                IDE_TEST( rpnComm::readCmBlock( NULL,
                                                aProtocolContext,
                                                mExitFlag,
                                                NULL /* TimeoutFlag */,
                                                RPU_REPLICATION_CONNECT_RECEIVE_TIMEOUT )
                          != IDE_SUCCESS );
                sRemainDataInCmBlock = CMI_REMAIN_DATA_IN_READ_BLOCK( aProtocolContext );
            }
            else
            {
                if ( sRemainDataInCmBlock >= sRemainDataOfValue )
                {
                    IDE_TEST_RAISE( writeXLog( aProtocolContext, sRemainDataOfValue, SM_SN_NULL )
                                    != IDE_SUCCESS, ERR_FAIL_TO_WRITE_INTO_FILE );
                    sRemainDataOfValue = 0;
                }
                else
                {
                    IDE_TEST_RAISE( writeXLog( aProtocolContext, sRemainDataInCmBlock, SM_SN_NULL )
                                    != IDE_SUCCESS, ERR_FAIL_TO_WRITE_INTO_FILE );
                    sRemainDataOfValue = sRemainDataOfValue - sRemainDataInCmBlock;
                }
                sRemainDataInCmBlock = CMI_REMAIN_DATA_IN_READ_BLOCK( aProtocolContext );
            }
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_FAIL_TO_WRITE_INTO_FILE  )
    {
        if ( ideGetErrorCode() != rpERR_ABORT_END_THREAD )
        {
            IDE_ERRLOG( IDE_RP_0 );

            IDE_SET( ideSetErrorCode( rpERR_ABORT_RPX_FAIL_WRITE_XLOG_IN_XLOGFILE ) );
        }
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpxXLogTransfer::writeSPSetLog( cmiProtocolContext * aProtocolContext, smSN aXSN )
{

    UInt sTotalLength = 0;
    UInt sVariableLength = 0;
    UInt sVariablePostionAtCMBuffer = 24;  //variable length는 24번재 이후에 담겨있다.UChar sOp = 0;
    UChar sOp = 0;

    CMI_PEEK1( aProtocolContext, sOp, 0 );
    IDE_DASSERT( sOp == CMI_PROTOCOL_OPERATION( RP, SPSet ) );
    IDE_TEST( writeXLog( aProtocolContext, ID_SIZEOF( sOp ), SM_SN_NULL ) != IDE_SUCCESS );

    sTotalLength += sVariablePostionAtCMBuffer  + 4; //len담는 4 byte

    CMI_PEEK4( aProtocolContext, &sVariableLength, sVariablePostionAtCMBuffer );
    sTotalLength += sVariableLength ;

    IDE_TEST_RAISE( writeXLog( aProtocolContext, sTotalLength, aXSN ) != IDE_SUCCESS, ERR_FAIL_TO_WRITE_INTO_FILE );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_FAIL_TO_WRITE_INTO_FILE  )
    {
        if ( ideGetErrorCode() != rpERR_ABORT_END_THREAD )
        {
            IDE_ERRLOG( IDE_RP_0 );

            IDE_SET( ideSetErrorCode( rpERR_ABORT_RPX_FAIL_WRITE_XLOG_IN_XLOGFILE ) );
        }
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpxXLogTransfer::writeSPAbortLog( cmiProtocolContext * aProtocolContext, smSN aXSN )
{

    UInt sTotalLength = 0;
    UInt sVariableLength = 0;

    UInt sVariablePostionAtCMBuffer = 24;  //variable length는 24번재 이후에 담겨있다.
    UChar sOp = 0;

    CMI_PEEK1( aProtocolContext, sOp, 0 );
    IDE_DASSERT( sOp == CMI_PROTOCOL_OPERATION( RP, SPAbort ) );
    IDE_TEST( writeXLog( aProtocolContext, ID_SIZEOF( sOp ), SM_SN_NULL ) != IDE_SUCCESS );

    //variable length는 24번재 이후에 담겨있다.
    sTotalLength += sVariablePostionAtCMBuffer + 4; //len담는 4 byte

    CMI_PEEK4( aProtocolContext, &sVariableLength, sVariablePostionAtCMBuffer );
    sTotalLength += sVariableLength;

    IDE_TEST_RAISE( writeXLog( aProtocolContext, sTotalLength, aXSN ) != IDE_SUCCESS, ERR_FAIL_TO_WRITE_INTO_FILE );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_FAIL_TO_WRITE_INTO_FILE  )
    {
        if ( ideGetErrorCode() != rpERR_ABORT_END_THREAD )
        {
            IDE_ERRLOG( IDE_RP_0 );

            IDE_SET( ideSetErrorCode( rpERR_ABORT_RPX_FAIL_WRITE_XLOG_IN_XLOGFILE ) );
        }
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpxXLogTransfer::writeLobPartialWriteXLog( cmiProtocolContext * aProtocolContext )
{
    UChar sOp = 0;
    smSN  sXSN = SM_SN_NULL;
    SShort sXLogLength = 0;
    UInt   sRecvSize = 0;
    UInt   sRemainSize = 0;

    CMI_PEEK1( aProtocolContext, sOp, 0 );
    IDE_DASSERT( sOp == CMI_PROTOCOL_OPERATION( RP, LobPartialWrite ) );
    sXLogLength = rpnComm::rpnXLogLengthForEachType[sOp];
    
    CMI_PEEK8( aProtocolContext, &sXSN, 9 );
    CMI_PEEK4( aProtocolContext, &sRecvSize, 37 );

    IDE_TEST( writeXLog( aProtocolContext, sXLogLength + 1, sXSN ) != IDE_SUCCESS );
    
    sRemainSize = CMI_REMAIN_DATA_IN_READ_BLOCK( aProtocolContext );

    while( sRecvSize > 0 )
    {
        if ( sRemainSize == 0 )
        {
            IDE_TEST( rpnComm::readCmBlock( NULL,
                                            aProtocolContext,
                                            mExitFlag,
                                            NULL /* TimeoutFlag */,
                                            RPU_REPLICATION_CONNECT_RECEIVE_TIMEOUT )
                      != IDE_SUCCESS );
        }
        else
        {
            if ( sRemainSize >= sRecvSize )
            {
                IDE_TEST_RAISE( writeXLog( aProtocolContext, sRecvSize, SM_SN_NULL )
                                != IDE_SUCCESS, ERR_FAIL_TO_WRITE_INTO_FILE );
                sRecvSize = 0;
            }
            else
            {
                IDE_TEST_RAISE( writeXLog( aProtocolContext, sRemainSize, SM_SN_NULL )
                                != IDE_SUCCESS, ERR_FAIL_TO_WRITE_INTO_FILE );
                sRecvSize = sRecvSize - sRemainSize;
            }
        }

        sRemainSize = CMI_REMAIN_DATA_IN_READ_BLOCK( aProtocolContext );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_FAIL_TO_WRITE_INTO_FILE  )
    {
        if ( ideGetErrorCode() != rpERR_ABORT_END_THREAD )
        {
            IDE_ERRLOG( IDE_RP_0 );

            IDE_SET( ideSetErrorCode( rpERR_ABORT_RPX_FAIL_WRITE_XLOG_IN_XLOGFILE ) );
        }
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
IDE_RC rpxXLogTransfer::receiveCMBufferAndWriteXLog( cmiProtocolContext * aProtocolContext,
                                                     rpXLogType * aXLogType )
{

    UChar sOp = 0;
    smTID sTID = SM_NULL_TID;
    smSN  sXSN = SM_SN_NULL;

    SShort sXLogLength = 0;

    if ( CMI_IS_READ_ALL( aProtocolContext ) == ID_TRUE )
    {

        IDE_TEST( rpnComm::readCmBlock( NULL,
                                        aProtocolContext,
                                        mExitFlag,
                                        NULL,
                                        RPU_REPLICATION_CONNECT_RECEIVE_TIMEOUT )
                  != IDE_SUCCESS );
    }
    CMI_PEEK1( aProtocolContext, sOp, 0 );

    sXLogLength = rpnComm::rpnXLogLengthForEachType[sOp];

    //OP에 따라 가변적으로 읽는다.
    switch (sOp )
    {
        case CMI_PROTOCOL_OPERATION( RP, TrCommit ) :
        case CMI_PROTOCOL_OPERATION( RP, TrAbort ) :
        case CMI_PROTOCOL_OPERATION( RP, Handshake ) :
        case CMI_PROTOCOL_OPERATION( RP, XA_COMMIT ) :
        {
            CMI_PEEK4( aProtocolContext, (UInt *)aXLogType, 1 );
            CMI_PEEK4( aProtocolContext, &sTID, 5 );
            CMI_PEEK8( aProtocolContext, &sXSN, 9 );

            mLastCommitTID = sTID;
            mLastCommitSN = sXSN;
            mLastWrittenSN = sXSN;

            IDE_TEST( writeXLog( aProtocolContext, sXLogLength + 1, sXSN ) != IDE_SUCCESS );
            break;

        }
        case CMI_PROTOCOL_OPERATION( RP, KeepAlive ) : //recovery 용으로 기록하는 aXSN이 해당 xlog에는 필요하지 않다.
        case CMI_PROTOCOL_OPERATION( RP, Flush ) :
        case CMI_PROTOCOL_OPERATION( RP, Stop ) :
        {
            CMI_PEEK4( aProtocolContext, (UInt *)aXLogType, 1 );
            CMI_PEEK8( aProtocolContext, &sXSN, 9 );
            mLastWrittenSN = sXSN;

            IDE_TEST( writeXLog( aProtocolContext, sXLogLength + 1, sXSN ) != IDE_SUCCESS );

            break;
        }
        case CMI_PROTOCOL_OPERATION( RP, TrBegin ) :
        case CMI_PROTOCOL_OPERATION( RP, Insert ) :
        case CMI_PROTOCOL_OPERATION( RP, Update ) :
        case CMI_PROTOCOL_OPERATION( RP, Delete ) :
        case CMI_PROTOCOL_OPERATION( RP, LobCursorOpen ) :
        case CMI_PROTOCOL_OPERATION( RP, LobCursorClose ) :
        case CMI_PROTOCOL_OPERATION( RP, LobPrepare4Write ) :
        case CMI_PROTOCOL_OPERATION( RP, LobFinish2Write ) :
        case CMI_PROTOCOL_OPERATION( RP, LobTrim ) :
        { 
            CMI_PEEK4( aProtocolContext, (UInt *)aXLogType, 1 );
            CMI_PEEK8( aProtocolContext, &sXSN, 9 );
            mLastWrittenSN = sXSN;

            IDE_TEST( writeXLog( aProtocolContext, sXLogLength + 1, sXSN ) != IDE_SUCCESS );
            break;
        }
        case CMI_PROTOCOL_OPERATION( RP, LobPartialWrite ) :
        {
            CMI_PEEK4( aProtocolContext, (UInt *)aXLogType, 1 );
            CMI_PEEK8( aProtocolContext, &sXSN, 9 );
            mLastWrittenSN = sXSN;

            IDE_TEST( writeLobPartialWriteXLog( aProtocolContext ) != IDE_SUCCESS );
            break;
        }
        case CMI_PROTOCOL_OPERATION( RP, UIntID ) :
        {
            IDE_TEST( writeXLog( aProtocolContext, sXLogLength + 1, SM_SN_NULL ) != IDE_SUCCESS );
            break;
        }
        case CMI_PROTOCOL_OPERATION( RP, Value ) :
        {
            IDU_FIT_POINT( "BUG-48392" );

            IDE_TEST( writeValuesXLog(aProtocolContext ) != IDE_SUCCESS );
            break;
        }
        case CMI_PROTOCOL_OPERATION( RP, SPSet ) :
        {
            IDE_TEST( writeSPSetLog( aProtocolContext, sXSN ) != IDE_SUCCESS );
            break;
        }
        case CMI_PROTOCOL_OPERATION( RP, SPAbort ) :
        {
            IDE_TEST( writeSPAbortLog( aProtocolContext, sXSN ) != IDE_SUCCESS );
            break;
        }
        case CMI_PROTOCOL_OPERATION( RP, XA_START_REQ ) :
        case CMI_PROTOCOL_OPERATION( RP, XA_PREPARE_REQ ) :
        case CMI_PROTOCOL_OPERATION( RP, XA_PREPARE ) :
        {
            /* XLogType 4
             * XID ID_XIDDATASIZE
             * TID 4
             * SN  8 */
            CMI_PEEK4( aProtocolContext, (UInt *)aXLogType, 1 );
            /* XID 152 */
            CMI_PEEK4( aProtocolContext, &sTID, 1 + 4 );
            CMI_PEEK8( aProtocolContext, &sXSN, 1 + 4 + 4 );

            mLastCommitTID = sTID;
            mLastWrittenSN = sXSN;

            IDE_TEST( writeXLog( aProtocolContext, sXLogLength + 1, sXSN ) != IDE_SUCCESS );
            break;
        }
        
        case CMI_PROTOCOL_OPERATION( RP, XA_END ) :
        {
            CMI_PEEK4( aProtocolContext, (UInt *)aXLogType, 1 );
            /* XID 152 */
            CMI_PEEK4( aProtocolContext, &sTID, 1 + 4 );
            CMI_PEEK8( aProtocolContext, &sXSN, 1 + 4 + 4 );

            mLastCommitTID = sTID;
            mLastWrittenSN = sXSN;

            IDE_TEST( writeXLog( aProtocolContext, sXLogLength + 1, sXSN ) != IDE_SUCCESS );
            break;
        }

        default :
        {
            IDE_DASSERT( 0 );

            IDE_RAISE( ERR_PROTOCOL_OPID );
            break;
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_PROTOCOL_OPID )
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RECEIVER_UNEXPECTED_PROTOCOL,
                                  sOp ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;


}

IDE_RC rpxXLogTransfer::processXLogInXLogTransfer( const rpXLogType aXLogType )
{
    rpXLogType sType = RP_X_NONE;
    idBool sNeedSendAck = ID_FALSE;

    switch( aXLogType )
    {
        case RP_X_COMMIT:
        case RP_X_XA_COMMIT:
        case RP_X_ABORT:
            sType = RP_X_ACK_WITH_TID;
            sNeedSendAck = ID_TRUE;
            break;

       case RP_X_KEEP_ALIVE:
            sType = RP_X_ACK_WITH_TID;
            sNeedSendAck = ID_TRUE;
            break;

        case RP_X_HANDSHAKE:
            IDE_TEST( waitForReceiverProcessDone() != IDE_SUCCESS );
            break;

        case RP_X_REPL_STOP:
            /* Nothing to do */
            break;

        case RP_X_XA_START_REQ:
            sType = RP_X_XA_START_REQ_ACK;
            sNeedSendAck = ID_TRUE;
            break;

        case RP_X_XA_PREPARE:
            sType = RP_X_ACK_WITH_TID;
            sNeedSendAck = ID_TRUE;
            break;

        default :
            break;
    }

    increaseProcessedXLogCount();

    if ( sNeedSendAck == ID_TRUE ) 
    {

        IDE_TEST( buildAndSendAck( sType ) != IDE_SUCCESS );

        resetProcessedXLogCount();
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpxXLogTransfer::buildAndSendAck( rpXLogType aType )
{

    rpXLogAck sXLogAck;

    buildAck( &sXLogAck, aType );

    IDE_TEST( sendAck( &sXLogAck ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

void rpxXLogTransfer::buildAck( rpXLogAck * aAckXLog, rpXLogType aType  )
{
    IDE_DASSERT( aType != RP_X_NONE );

    aAckXLog->mAckType = aType;
    aAckXLog->mTID = mLastCommitTID;

    /* SN 관련 값들은 aLastSN으로 셋팅한다. */
    aAckXLog->mLastArrivedSN = mLastWrittenSN;

    if ( mLastWrittenSN > mLastCommitSN )
    {
        aAckXLog->mLastCommitSN = mLastWrittenSN;
    }
    else
    {
        aAckXLog->mLastCommitSN = mLastCommitSN;
    }

    aAckXLog->mRestartSN = mRestartSN;

    aAckXLog->mFlushSN = SM_SN_NULL;
    aAckXLog->mLastProcessedSN = mLastProcessedSN;

    /* set default value */ 
    aAckXLog->mAbortTxCount = 0;
    aAckXLog->mAbortTxList = NULL;
    aAckXLog->mClearTxCount = 0;
    aAckXLog->mClearTxList = NULL;

    return;
}

IDE_RC rpxXLogTransfer::sendAck( rpXLogAck * aXLogAck )
{
    //todo review
    //sendAck와 sendAckEager의 차이점은 ack에 TID가 포함되냐 안되냐의 차이이다.
    //rename으로 AckWithTID 이런식으로 변경하는 편이 좋을 것 같다.

    //The different between sencAck() and sendAckEager() is whether TID is included or not.
    //Mey be rename to sendAckWithTID()?
    IDE_TEST( rpnComm::sendAckEager( mProtocolContext,
                                    mExitFlag,
                                    aXLogAck,
                                    RPU_REPLICATION_SENDER_SEND_TIMEOUT )
                  != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( ( ideGetErrorCode() == cmERR_ABORT_CONNECTION_CLOSED ) ||
         ( ideGetErrorCode() == cmERR_ABORT_SEND_ERROR ) ||
         ( ideGetErrorCode() == rpERR_ABORT_RP_SENDER_SEND_ERROR ) ||
         ( ideGetErrorCode() == rpERR_ABORT_SEND_TIMEOUT_EXCEED ) ||
         ( ideGetErrorCode() == rpERR_ABORT_HBT_DETECT_PEER_SERVER_ERROR ) )
    {
        IDE_ERRLOG( IDE_RP_0 );
        IDE_SET(ideSetErrorCode(rpERR_ABORT_CONNECT_FAIL));

        *mNetworkError = ID_TRUE;
    }
    else
    {
        /* do nothing */
    }

    return IDE_FAILURE;
}


//Transfer -> Receiver, Sync가 시작하기 전 호출, DDL replication 이나 기타 receiver에서 작업이 필요한 경우 호출
cmiProtocolContext * rpxXLogTransfer::takeAwayNetworkResources()
{
    cmiProtocolContext * sProtocolContext = mProtocolContext;

    mProtocolContext = NULL;

    return sProtocolContext;

}

//Receiver -> Transfer, Sync가 끝난 후 호출, DDL replication 이나 기타 receiver에서 필요한 작업이 끝난 후 호출
IDE_RC rpxXLogTransfer::handOverNetworkResources( cmiProtocolContext * aProtocolContext )
{
    IDE_DASSERT( mProtocolContext == NULL );
    mProtocolContext = aProtocolContext;

    return IDE_SUCCESS;
}

IDE_RC rpxXLogTransfer::lockForWaitReceiverProcessDone()
{
    IDE_ASSERT( mWaitForReceiverProcessDoneMutex.lock(NULL) == IDE_SUCCESS );

    return IDE_SUCCESS;
}

IDE_RC rpxXLogTransfer::unlockForWaitReceiverProcessDone()
{
    IDE_ASSERT( mWaitForReceiverProcessDoneMutex.unlock() == IDE_SUCCESS );

    return IDE_SUCCESS;
}

IDE_RC rpxXLogTransfer::waitForReceiverProcessDone()
{
    PDL_Time_Value    sCheckTime;
    PDL_Time_Value    sSleepSec;

    sCheckTime.initialize();
    sSleepSec.initialize();

    sSleepSec.set( 1 );

    lockForWaitReceiverProcessDone();

    mIsSendAckThroughReceiver = ID_FALSE;

    while( mIsSendAckThroughReceiver == ID_FALSE )
    {
        sCheckTime = idlOS::gettimeofday() + sSleepSec;

        mWaitForReceiverProcessDoneCV.timedwait( &mWaitForReceiverProcessDoneMutex,
                                                 &sCheckTime,
                                                 IDU_IGNORE_TIMEDOUT  );

    }

    unlockForWaitReceiverProcessDone();

    return IDE_SUCCESS;
}

idBool rpxXLogTransfer::isWaitFromReceiverProcessDone()
{
    idBool sIsSendAckThroughReceiver = ID_FALSE;

    lockForWaitReceiverProcessDone();

    sIsSendAckThroughReceiver = ( mIsSendAckThroughReceiver == ID_FALSE ) ? ID_TRUE:ID_FALSE;

    unlockForWaitReceiverProcessDone();

    return sIsSendAckThroughReceiver;
}


IDE_RC rpxXLogTransfer::signalToWaitedTransfer()
{
    lockForWaitReceiverProcessDone();

    mIsSendAckThroughReceiver = ID_TRUE;

    mWaitForReceiverProcessDoneCV.signal();

    unlockForWaitReceiverProcessDone();

    return IDE_SUCCESS;
}

UInt rpxXLogTransfer::getLastWrittenFileNo()
{
    UInt sFileNo = 0;

    if ( mXLogfileManager != NULL )
    {
        sFileNo = mXLogfileManager->getWriteFileNoWithLock();
    }

    return sFileNo;
}

UInt rpxXLogTransfer::getLastWrittenFileOffset()
{
    UInt sFileOffset = 0;
    rpXLogLSN sWriteLSN;

    if ( mXLogfileManager != NULL )
    {
        sWriteLSN = mXLogfileManager->getWriteXLogLSNWithLock();
        RP_GET_OFFSET_FROM_XLOGLSN( sFileOffset, sWriteLSN );
    }

    return sFileOffset;

}
