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

#include <cm.h>

#include <rp.h>

#include <rpuProperty.h>
#include <rpdMeta.h>
#include <rpnComm.h>
#include <rpnMessenger.h>
#include <rpdCatalog.h>

rpValueLen rpnMessenger::mSyncAfterMtdValueLen[QCI_MAX_COLUMN_COUNT] = { {0, 0}, };
rpValueLen rpnMessenger::mSyncPKMtdValueLen[QCI_MAX_KEY_COLUMN_COUNT] = { {0, 0}, };

/*
 *
 */
rpnMessenger::rpnMessenger( void )
{

}

/*
 *
 */
IDE_RC rpnMessenger::initializeCmBlock( void )
{
    idBool sResendBlock = ID_FALSE;

    IDE_DASSERT( mIsInitCmBlock == ID_FALSE );
    
    IDE_TEST( initializeCmLink( mSocketType ) != IDE_SUCCESS );

    IDE_TEST( cmiMakeCmBlockNull( mProtocolContext ) != IDE_SUCCESS );

    if ( mIsBlockingMode == ID_FALSE )
    {
        sResendBlock = ID_TRUE;
    }
    else
    {
         sResendBlock = ID_FALSE;
    }

    if ( rpdMeta::isUseV6Protocol( mReplication ) == ID_TRUE )
    {
        IDE_TEST_RAISE( cmiAllocCmBlockForA5( mProtocolContext,
                                              CMI_PROTOCOL_MODULE( RP ),
                                              (cmiLink *)mLink,
                                              this,
                                              sResendBlock )
                        != IDE_SUCCESS, ERR_ALLOC_CM_BLOCK );
    }
    else
    {
        IDE_TEST_RAISE( cmiAllocCmBlock( mProtocolContext,
                                         CMI_PROTOCOL_MODULE( RP ),
                                         (cmiLink *)mLink,
                                         this,
                                         sResendBlock )
                        != IDE_SUCCESS, ERR_ALLOC_CM_BLOCK );
    }

    mIsInitCmBlock = ID_TRUE;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_ALLOC_CM_BLOCK );
    {
        IDE_ERRLOG( IDE_RP_0 );
        IDE_SET( ideSetErrorCode( rpERR_ABORT_ALLOC_CM_BLOCK ) );
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
/*
 *
 */
IDE_RC rpnMessenger::initializeCmLink( RP_SOCKET_TYPE aSocketType )
{
    IDE_DASSERT( mLink == NULL );

    switch ( aSocketType )
    {
    case RP_SOCKET_TYPE_TCP:
        IDU_FIT_POINT_RAISE( "rpnMessenger::initializeCmLink::Erratic::rpERR_ABORT_ALLOC_LINK",
                             ERR_ALLOC_LINK );
        IDE_TEST_RAISE( cmiAllocLink( &mLink,
                                      CMI_LINK_TYPE_PEER_CLIENT,
                                      CMI_LINK_IMPL_TCP )
                        != IDE_SUCCESS, ERR_ALLOC_LINK );
        break;
        
    case RP_SOCKET_TYPE_UNIX:
        IDE_TEST_RAISE( cmiAllocLink( &mLink,
                                      CMI_LINK_TYPE_PEER_CLIENT,
                                      CMI_LINK_IMPL_UNIX )
                        != IDE_SUCCESS, ERR_ALLOC_LINK );
        break;
    case RP_SOCKET_TYPE_IB:
        IDE_TEST_RAISE( cmiAllocLink( &mLink,
                                      CMI_LINK_TYPE_PEER_CLIENT,
                                      CMI_LINK_IMPL_IB )
                        != IDE_SUCCESS, ERR_ALLOC_LINK );
        break;

    }

    if ( rpdMeta::isUseV6Protocol( mReplication ) == ID_TRUE )
    {
        cmiLinkSetPacketTypeA5( mLink );
    }
    else
    {
        /* do nothing */
    }

    return IDE_SUCCESS;
    
    IDE_EXCEPTION(ERR_ALLOC_LINK);
    {
        IDE_ERRLOG(IDE_RP_0);
        IDE_SET(ideSetErrorCode(rpERR_ABORT_ALLOC_LINK));
    }
    IDE_EXCEPTION_END;

    mLink =  NULL;

    return IDE_FAILURE;
}

/*
 *
 */
void rpnMessenger::destroyCmBlock( void )
{
    if ( mIsInitCmBlock == ID_TRUE )
    {
        if ( cmiFreeCmBlock( mProtocolContext ) != IDE_SUCCESS )
        {
            IDE_ERRLOG(IDE_RP_0);
            IDE_SET(ideSetErrorCode(rpERR_ABORT_FREE_CM_BLOCK));
            IDE_ERRLOG(IDE_RP_0);
        }
    }
    else
    {
        /*do nothing*/
    }
    mIsInitCmBlock = ID_FALSE;
    return;
}
/*
 *
 */
void rpnMessenger::destroyCmLink( void )
{
    if ( mLink != NULL )
    {
        if ( cmiFreeLink( mLink ) != IDE_SUCCESS )
        {
            IDE_ERRLOG( IDE_RP_0 );
            IDE_SET( ideSetErrorCode( rpERR_ABORT_FREE_LINK ) );
            IDE_ERRLOG( IDE_RP_0 );
        }
        mLink = NULL;
    }
    return;
}
/*
 *
 */
void rpnMessenger::initializeNetworkAddress( RP_SOCKET_TYPE aSocketType )
{
    (void)idlOS::memset( mLocalIP, 0x00, RP_IP_ADDR_LEN );
    mLocalPort = 0;

    (void)idlOS::memset( mRemoteIP, 0x00, RP_IP_ADDR_LEN );
    mRemotePort = 0;

    mIBLatency = RP_IB_LATENCY_NONE;

    switch ( aSocketType )
    {
        case RP_SOCKET_TYPE_TCP:
            (void)idlOS::snprintf( mLocalIP, sizeof( mLocalIP ), "127.0.0.1" );
            mLocalPort = RPU_REPLICATION_PORT_NO;
            break;

        case RP_SOCKET_TYPE_UNIX:
            (void)idlOS::snprintf( mLocalIP, sizeof( mLocalIP ), 
                                   "%s", RP_SOCKET_UNIX_DOMAIN_STR );
            (void)idlOS::snprintf( mRemoteIP, sizeof( mRemoteIP ),
                                   "%s", RP_SOCKET_UNIX_DOMAIN_STR );
            break;
        
        case RP_SOCKET_TYPE_IB:
            (void)idlOS::snprintf( mLocalIP, ID_SIZEOF( mLocalIP ), "127.0.0.1" );
            mLocalPort = RPU_REPLICATION_IB_PORT_NO;
            break;
    }
}

/*
 *
 */
IDE_RC rpnMessenger::initialize( cmiProtocolContext * aProtocolContext,
                                 RP_MESSENGER_CHECK_VERSION    aCheckVersion, 
                                 RP_SOCKET_TYPE       aSocketType,
                                 idBool             * aExitFlag,
                                 rpdReplications    * aReplication,
                                 void               * aHBTResource,
                                 idBool               aNeedLock )
{
    SChar   sName[IDU_MUTEX_NAME_LEN] = { 0, };

    mCheckVersion = aCheckVersion;
    mSocketType = aSocketType;
    mReplication = aReplication;

    mExitFlag = aExitFlag;

    mSNMapMgr = NULL;

    mLink = NULL;
    mIsInitCmBlock = ID_FALSE;
    mHBTResource = aHBTResource;
    mProtocolContext = aProtocolContext;

    if( mReplication->mReplMode == RP_CONSISTENT_MODE )
    {
        mIsFlushCommitXLog = ID_TRUE;
    }
    else
    {
        mIsFlushCommitXLog = ID_FALSE;
    }

    initializeNetworkAddress( aSocketType );

    /* BUG-38716 */
    if ( RPU_REPLICATION_SENDER_SEND_TIMEOUT > 0 ) 
    {
        mIsBlockingMode = ID_FALSE;
    }
    else
    {
        mIsBlockingMode = ID_TRUE;
    }

    /* PROJ-2453 */
    if ( aNeedLock == ID_TRUE )
    {
        idlOS::snprintf( sName, IDU_MUTEX_NAME_LEN, "RP_MESSENDER_SOCKET_MUTEX" );
        IDE_TEST( mSocketMutex.initialize( sName,
                                           IDU_MUTEX_KIND_POSIX,
                                           IDV_WAIT_INDEX_NULL )
                  != IDE_SUCCESS );
        mNeedSocketMutex = ID_TRUE;
    }
    else
    {
        mNeedSocketMutex = ID_FALSE;
    }

    if ( mProtocolContext == NULL )
    {
        IDE_TEST(iduMemMgr::malloc(IDU_MEM_RP_RPC,
                                   ID_SIZEOF(cmiProtocolContext),
                                   (void**)&mProtocolContext,
                                   IDU_MEM_IMMEDIATE)
                 != IDE_SUCCESS);
        
        IDE_TEST( cmiMakeCmBlockNull( mProtocolContext ) != IDE_SUCCESS );
    }


    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( mProtocolContext != NULL )
    {
        iduMemMgr::free(mProtocolContext);
    }

    return IDE_FAILURE;
}

/*
 *
 */
void rpnMessenger::destroy( rpnProtocolContextReleasePolicy aProtocolCtxReleasePolicy )
{
    disconnect();

    if ( mNeedSocketMutex == ID_TRUE )
    {
        (void)mSocketMutex.destroy();
    }
    else
    {
        /* nothing to do */
    }
    if ( aProtocolCtxReleasePolicy == RPN_RELEASE_PROTOCOL_CONTEXT )
    {
        (void)iduMemMgr::free( mProtocolContext );
    }
    mProtocolContext = NULL;
}

/*
 *
 */
IDE_RC rpnMessenger::connect( cmiConnectArg * aConnectArg )
{
    PDL_Time_Value     sTimeOut;
    idBool             sIsConnected = ID_FALSE;

    sTimeOut.initialize( RPU_REPLICATION_CONNECT_TIMEOUT, 0 );

    /* receiver가 restart되었을 때, 새로운 conncet가 맺어지므로 context를 초기화하여 사용해야한다.
     * 초기화하지 않으면, context의 mSeqNo가 달라 실패하게 된다.
     * (void)cmiFreeCmBlock( mProtocolContext );는 이전 연결 종료시 불려졌어야한다.
     */
    
    IDE_TEST( initializeCmBlock() != IDE_SUCCESS );
    
    if ( RPU_REPLICATION_SENDER_ENCRYPT_XLOG == 1 )
    {
        cmiEnableEncrypt( mProtocolContext );
    }
    else
    {
        cmiDisableEncrypt( mProtocolContext );
    }

    IDU_FIT_POINT_RAISE( "rpnMessenger::connect::cmiConnectLink::mProtocolContext", 
                          ERR_CONNECT,
                          cmERR_ABORT_CONNECT_ERROR,
                         "rpnMessenger::connect",
                         "mProtocolContext" );
    IDE_TEST_RAISE( cmiConnectWithoutData( mProtocolContext, aConnectArg, &sTimeOut, SO_REUSEADDR )
                    != IDE_SUCCESS, ERR_CONNECT );
    sIsConnected = ID_TRUE;

    if ( mIsBlockingMode == ID_TRUE )
    {
        IDE_TEST( cmiSetLinkBlockingMode( mLink,
                                          ID_TRUE )
                  != IDE_SUCCESS );
        
        ideLog::log( IDE_RP_0, RP_TRC_M_LINK_IS_BLOCKING );
    }
    else
    {
        IDE_TEST( cmiSetLinkBlockingMode( mLink,
                                          ID_FALSE )
                  != IDE_SUCCESS );

        ideLog::log( IDE_RP_0, RP_TRC_M_LINK_IS_NON_BLOCKING );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_CONNECT );
    {
        IDE_ERRLOG( IDE_RP_0 );
        IDE_SET(ideSetErrorCode(rpERR_ABORT_RP_SENDER_CONNECT_PEER));
    }

    IDE_EXCEPTION_END;

    IDE_PUSH();

    if(sIsConnected == ID_TRUE)
    {
        if(mLink != NULL)
        {
            (void)cmiShutdownLink(mLink, CMI_DIRECTION_RDWR);
            (void)cmiCloseLink(mLink);
        }
    }
    
    destroyCmBlock();
    destroyCmLink();
        
    IDE_POP();

    return IDE_FAILURE;
}

/*
 *
 */
IDE_RC rpnMessenger::connectThroughTcp( SChar * aHostIp, UInt aPortNo )
{
    cmiConnectArg      sConnectArg;

    idlOS::memset( &sConnectArg, 0, ID_SIZEOF(cmiConnectArg) );
    sConnectArg.mTCP.mAddr = aHostIp;
    sConnectArg.mTCP.mPort = aPortNo;
    sConnectArg.mTCP.mBindAddr = NULL;

    IDE_TEST( connect( &sConnectArg ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 *
 */
IDE_RC rpnMessenger::connectThroughUnix( SChar * aFileName )
{
    cmiConnectArg      sConnectArg;

    sConnectArg.mUNIX.mFilePath = aFileName;

    IDE_TEST( connect( &sConnectArg ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 *
 */
IDE_RC rpnMessenger::connectThroughIB( SChar * aHostIp, UInt aPortNo, rpIBLatency aIBLatency )
{
    cmiConnectArg      sConnectArg;

    idlOS::memset( &sConnectArg, 0, ID_SIZEOF(cmiConnectArg) );
    sConnectArg.mIB.mAddr    = aHostIp;
    sConnectArg.mIB.mPort    = aPortNo;
    sConnectArg.mIB.mLatency = aIBLatency;
    sConnectArg.mIB.mBindAddr = NULL;

    IDE_TEST( connect( &sConnectArg ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 *
 */
void rpnMessenger::disconnect( void )
{
    destroyCmBlock();

    if ( ( mLink != NULL ) && 
         ( rpnComm::isConnected( mLink ) == ID_TRUE ) )
    {
        if(cmiShutdownLink(mLink, CMI_DIRECTION_RDWR) != IDE_SUCCESS)
        {
            IDE_ERRLOG(IDE_RP_0);
            IDE_SET(ideSetErrorCode(rpERR_ABORT_SHUTDOWN_LINK));
            IDE_ERRLOG(IDE_RP_0);
        }
        if(cmiCloseLink(mLink) != IDE_SUCCESS)
        {
            IDE_ERRLOG(IDE_RP_0);
            IDE_SET(ideSetErrorCode(rpERR_ABORT_CLOSE_LINK));
            IDE_ERRLOG(IDE_RP_0);
        }
    }

    destroyCmLink();
}

/*
 * @brief Replication version is sent and handshake ack is received
 */
IDE_RC rpnMessenger::checkRemoteReplVersion( rpMsgReturn * aResult,
                                             SChar       * aErrMsg,
                                             UInt        * aMsgLen )
{
    rpdVersion     sVersion;
    SInt           sDummyFailbackStatus;
    UInt           sResult        = RP_MSG_DISCONNECT;
    ULong          sDummyXSN;

    if ( rpdMeta::isUseV6Protocol( mReplication ) == ID_TRUE )
    {
        sVersion.mVersion = RP_MAKE_VERSION( 6, 1, 1, REPLICATION_ENDIAN_64BIT);
    }
    else
    {
        sVersion.mVersion = RP_CURRENT_VERSION;
    }

    if ( rpnComm::sendVersion( mHBTResource, 
                               mProtocolContext,
                               mExitFlag,
                               &sVersion,
                               RPU_REPLICATION_SENDER_SEND_TIMEOUT ) 
         != IDE_SUCCESS )
    {
        IDE_ERRLOG( IDE_RP_0 );
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RP_WRITE_SOCKET ) );
        IDE_ERRLOG( IDE_RP_0 );

        *aResult = RP_MSG_DISCONNECT;

        IDE_CONT( NORMAL_EXIT );
    }

    if ( rpnComm::recvHandshakeAck( mHBTResource,
                                    mProtocolContext,
                                    mExitFlag,
                                    &sResult,
                                    &sDummyFailbackStatus,
                                    &sDummyXSN,
                                    aErrMsg,
                                    aMsgLen,
                                    RPU_REPLICATION_RECEIVE_TIMEOUT )
         != IDE_SUCCESS )
    {
        IDE_ERRLOG( IDE_RP_0 );
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RP_READ_SOCKET ) );
        IDE_ERRLOG( IDE_RP_0 );

        *aResult = RP_MSG_DISCONNECT;

        IDE_CONT( NORMAL_EXIT );
    }
    
    IDU_FIT_POINT_RAISE( "rpnMessenger::checkRemoteReplVersion::Erratic::rpERR_ABORT_UNEXPECTED_HANDSHAKE_ACK",
                         ERR_UNEXPECTED_HANDSHAKE_ACK );
    switch ( sResult )
    {
        case RP_MSG_PROTOCOL_DIFF :
            *aResult = RP_MSG_PROTOCOL_DIFF;
            break;

        case RP_MSG_OK :
            *aResult = RP_MSG_OK;
            break;

        default:
            IDE_RAISE( ERR_UNEXPECTED_HANDSHAKE_ACK );
    }

    RP_LABEL( NORMAL_EXIT );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_UNEXPECTED_HANDSHAKE_ACK );
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_UNEXPECTED_HANDSHAKE_ACK, sResult ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

void rpnMessenger::getVersionFromAck( SChar      *aMsg,
                                      UInt       aMsgLen,
                                      rpdVersion *aVersion )
{
    if ( aMsgLen == ID_SIZEOF( ULong ) ) /* If version information exists in aMsg, get it.  */
    {
        aVersion->mVersion = idlOS::ntoh ( *(ULong*)aMsg );
    }
    else /* otherwise remote version is lower than 7_4_1 version */
    {
        aVersion->mVersion = RP_MAKE_VERSION( 7, 4, 1, 0 );
    }
}

/*
 *
 */
IDE_RC rpnMessenger::handshake( rpdMeta * aMeta )
{
    rpMsgReturn sVersionResult = RP_MSG_DISCONNECT;
    UInt        sMetaResult    = RP_MSG_DISCONNECT;
    idBool      sDummyExitFlag = ID_FALSE;
    SInt        sDummyFailbackStatus;
    SChar       sBuffer[RP_ACK_MSG_LEN];
    UInt        sMsgLen;
    SInt        sTimeOutSec = RPU_REPLICATION_RECEIVE_TIMEOUT;
    ULong       sDummyXSN;

    sBuffer[0] = '\0';
    IDE_TEST( checkRemoteReplVersion( &sVersionResult, 
                                      sBuffer,
                                      &sMsgLen )
              != IDE_SUCCESS );

    switch ( sVersionResult )
    {
        case RP_MSG_DISCONNECT :
            IDE_RAISE( ERR_REPL_VERSION_DISCONNECT );

        case RP_MSG_PROTOCOL_DIFF :
            IDE_RAISE( ERR_PROTOCOL_DIFF );

        case RP_MSG_OK :
            break;

        default :
            IDE_ASSERT( 0 );
    }

    getVersionFromAck( sBuffer, 
                       sMsgLen,
                       &(aMeta->mReplication.mRemoteVersion) );
    
    rpdMeta::getProtocolCompressType( aMeta->mReplication.mRemoteVersion,
                                      &(aMeta->mReplication.mCompressType) );
    
    IDE_TEST_RAISE( aMeta->sendMeta( mHBTResource, 
                                     mProtocolContext,
                                     mExitFlag,
                                     ID_FALSE,
                                     RPU_REPLICATION_SENDER_SEND_TIMEOUT ) 
                    != IDE_SUCCESS, ERR_SEND_META );

    sBuffer[0] = '\0';

    IDU_FIT_POINT( "rpnMessenger::handshake::recvHandshakeAck::SLEEP" );
    IDE_TEST_RAISE( rpnComm::recvHandshakeAck( mHBTResource,
                                               mProtocolContext,
                                               &sDummyExitFlag,
                                               &sMetaResult,
                                               &sDummyFailbackStatus,
                                               &sDummyXSN,
                                               sBuffer,
                                               &sMsgLen,
                                               sTimeOutSec )
                    != IDE_SUCCESS, ERR_RECEIVE_META_ACK );
    
    IDU_FIT_POINT_RAISE( "rpnMessenger::handshake::Erratic::rpERR_ABORT_UNEXPECTED_HANDSHAKE_ACK",
                         ERR_UNEXPECTED_HANDSHAKE_ACK );
    switch ( sMetaResult )
    {
        case RP_MSG_DENY :
            IDE_RAISE( ERR_DENY );

        case RP_MSG_NOK :
            IDE_RAISE( ERR_NOK );

        case RP_MSG_META_DIFF :
            IDE_RAISE( ERR_META_DIFF );

        case RP_MSG_OK :
            break;

        default :
            IDE_RAISE( ERR_UNEXPECTED_HANDSHAKE_ACK );
    }
    
    if ( RPU_REPLICATION_SENDER_COMPRESS_XLOG == 1 )
    {
        cmiEnableCompress( mProtocolContext,
                           RPU_REPLICATION_SENDER_COMPRESS_XLOG_LEVEL,
                           aMeta->mReplication.mCompressType );
        cmiSetDecompressType( mProtocolContext, aMeta->mReplication.mCompressType );
        
        ideLog::log( IDE_RP_0, "[Sender] COMPRESS XLOG ON RepName : %s, CompressType : %"ID_UINT32_FMT, aMeta->mReplication.mRepName, aMeta->mReplication.mCompressType );
    }
    else
    {
        cmiDisableCompress( mProtocolContext );
        cmiSetDecompressType( mProtocolContext, aMeta->mReplication.mCompressType );
        
        ideLog::log( IDE_RP_0, "[Sender] COMPRESS XLOG OFF RepName : %s, CompressType : %"ID_UINT32_FMT, aMeta->mReplication.mRepName, aMeta->mReplication.mCompressType );
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_REPL_VERSION_DISCONNECT )
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_HANDSHAKE_DISCONNECT,
                                  "Replication Protocol Version" ) );
    }
    IDE_EXCEPTION( ERR_SEND_META )
    {
        IDE_ERRLOG( IDE_RP_0 );
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RP_WRITE_SOCKET ) );
        IDE_ERRLOG( IDE_RP_0 );
        IDE_SET( ideSetErrorCode( rpERR_ABORT_HANDSHAKE_DISCONNECT,
                                  "Metadata" ) );
    }
    IDE_EXCEPTION( ERR_RECEIVE_META_ACK )
    {
        IDE_ERRLOG( IDE_RP_0 );
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RP_READ_SOCKET ) );
        IDE_ERRLOG( IDE_RP_0 );
        IDE_SET( ideSetErrorCode( rpERR_ABORT_HANDSHAKE_DISCONNECT,
                                  "Metadata" ) );
    }
    IDE_EXCEPTION( ERR_PROTOCOL_DIFF );
    {
        IDE_SET( ideSetErrorCode( rpERR_IGNORE_RP_PROTOCOL_DIFF ) );
    }
    IDE_EXCEPTION( ERR_DENY );
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RP_REPLICATION_DENY, sBuffer ) );
    }
    IDE_EXCEPTION( ERR_NOK );
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RP_REPLICATION_NOK, sBuffer ) );
    }
    IDE_EXCEPTION( ERR_META_DIFF );
    {
        IDE_SET( ideSetErrorCode( rpERR_IGNORE_RP_META_DIFF ) );
    }
    IDE_EXCEPTION( ERR_UNEXPECTED_HANDSHAKE_ACK );
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_UNEXPECTED_HANDSHAKE_ACK, sMetaResult ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/*
 *
 */
IDE_RC rpnMessenger::handshakeAndGetResults( rpdMeta * aMeta,
                                             rpMsgReturn * aRC,
                                             SChar * aRCMsg,
                                             SInt aRCMsgLen,
                                             idBool aMetaInitFlag,
                                             SInt * aFailbackStatus,
                                             smSN * aReceiverXSN ) 
{
    rpMsgReturn sVersionResult = RP_MSG_DISCONNECT;
    UInt        sMetaResult    = RP_MSG_DISCONNECT;
    SChar       sBuffer[RP_ACK_MSG_LEN];
    UInt        sMsgLen;

    sBuffer[0] = '\0';
    *aFailbackStatus = RP_FAILBACK_NONE;

    IDE_TEST_RAISE( checkRemoteReplVersion( &sVersionResult,
                                            sBuffer,
                                            &sMsgLen )
                    != IDE_SUCCESS, ERR_CHECK_REPL_VERSION );

    switch ( sVersionResult )
    {
        case RP_MSG_DISCONNECT :
            *aRC = RP_MSG_DISCONNECT;

            idlOS::strncpy( aRCMsg,
                            "Network error during checking "
                            "replication protocol version",
                            aRCMsgLen );

            IDE_SET( ideSetErrorCode( rpERR_ABORT_HANDSHAKE_DISCONNECT,
                                    "Replication Protocol Version" ) );
            IDE_ERRLOG( IDE_RP_0 );

            IDE_CONT( NORMAL_EXIT );

        case RP_MSG_PROTOCOL_DIFF :
            *aRC = RP_MSG_PROTOCOL_DIFF;

            idlOS::strncpy( aRCMsg, sBuffer, aRCMsgLen );

            IDE_SET( ideSetErrorCode( rpERR_IGNORE_RP_PROTOCOL_DIFF ) );
            IDE_ERRLOG( IDE_RP_0 );

            IDE_CONT( NORMAL_EXIT );

        case RP_MSG_OK :
            *aRC = RP_MSG_OK;
            break;

        default :
            IDE_ASSERT( 0 );
    }
    
    getVersionFromAck( sBuffer,
                       sMsgLen,
                       &(aMeta->mReplication.mRemoteVersion) );

    checkSupportOnRemoteVersion( aMeta->mReplication.mRemoteVersion,
                                 aRC,
                                 aRCMsg,
                                 aRCMsgLen );
    IDE_TEST_CONT( *aRC != RP_MSG_OK, NORMAL_EXIT );

    rpdMeta::getProtocolCompressType( aMeta->mReplication.mRemoteVersion,
                                      &(aMeta->mReplication.mCompressType) );
    
    // Success of Check Protocol, AND check Meta.
    if ( aMeta->sendMeta( mHBTResource, 
                          mProtocolContext,
                          mExitFlag,
                          aMetaInitFlag,
                          RPU_REPLICATION_SENDER_SEND_TIMEOUT ) 
         != IDE_SUCCESS )
    {
        *aRC = RP_MSG_DISCONNECT;

        idlOS::strncpy( aRCMsg,
                        "Network error during sending metadata",
                        aRCMsgLen );

        IDE_ERRLOG( IDE_RP_0 );
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RP_WRITE_SOCKET ) );
        IDE_ERRLOG( IDE_RP_0 );
        IDE_SET( ideSetErrorCode( rpERR_ABORT_HANDSHAKE_DISCONNECT,
                                  "Metadata" ) );
        IDE_ERRLOG( IDE_RP_0 );

        IDE_CONT( NORMAL_EXIT );
    }

    // Receive ACK
    sMetaResult = RP_MSG_DISCONNECT;
    sBuffer[0] = '\0';
    if ( rpnComm::recvHandshakeAck( mHBTResource,
                                    mProtocolContext,
                                    mExitFlag,
                                    &sMetaResult,
                                    aFailbackStatus,
                                    aReceiverXSN,
                                    sBuffer,
                                    &sMsgLen,
                                    RPU_REPLICATION_RECEIVE_TIMEOUT )
         != IDE_SUCCESS )
    {
        *aRC = RP_MSG_DISCONNECT;
        idlOS::strncpy( aRCMsg,
                        "Network error during receiving metadata ACK",
                        aRCMsgLen );

        IDE_ERRLOG( IDE_RP_0 );
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RP_READ_SOCKET ) );
        IDE_ERRLOG( IDE_RP_0 );
        IDE_SET( ideSetErrorCode( rpERR_ABORT_HANDSHAKE_DISCONNECT,
                                  "Metadata" ) );
        IDE_ERRLOG( IDE_RP_0 );

        IDE_CONT( NORMAL_EXIT );
    }

    IDU_FIT_POINT_RAISE( "rpnMessenger::handshakeAndGetResults::Erratic::rpERR_ABORT_UNEXPECTED_HANDSHAKE_ACK",
                         ERR_UNEXPECTED_HANDSHAKE_ACK );
    switch ( sMetaResult )
    {
        case RP_MSG_DENY :  // PROJ-1608 recovery from replication
            *aRC = RP_MSG_DENY;
            idlOS::strncpy( aRCMsg, sBuffer, aRCMsgLen );

            IDE_SET( ideSetErrorCode( rpERR_ABORT_RP_REPLICATION_DENY, sBuffer ) );
            IDE_ERRLOG( IDE_RP_0 );
            break;

        case RP_MSG_NOK :
            *aRC = RP_MSG_NOK;
            idlOS::strncpy( aRCMsg, sBuffer, aRCMsgLen );

            IDE_SET( ideSetErrorCode( rpERR_ABORT_RP_REPLICATION_NOK, sBuffer ) );
            IDE_ERRLOG( IDE_RP_0 );
            break;

        case RP_MSG_META_DIFF :
            *aRC = RP_MSG_META_DIFF;
            idlOS::strncpy( aRCMsg, sBuffer, aRCMsgLen );

            IDE_SET( ideSetErrorCode( rpERR_IGNORE_RP_META_DIFF ) );
            IDE_ERRLOG( IDE_RP_0 );
            break;

        case RP_MSG_OK :
            *aRC = RP_MSG_OK;
            break;

        default :
            IDE_RAISE( ERR_UNEXPECTED_HANDSHAKE_ACK );
    }
    
    if ( RPU_REPLICATION_SENDER_COMPRESS_XLOG == 1 )
    {
        cmiEnableCompress( mProtocolContext,
                           RPU_REPLICATION_SENDER_COMPRESS_XLOG_LEVEL,
                           aMeta->mReplication.mCompressType );
        cmiSetDecompressType( mProtocolContext, aMeta->mReplication.mCompressType );
        
        ideLog::log( IDE_RP_0, "[Sender] COMPRESS XLOG ON RepName : %s, CompressType : %"ID_UINT32_FMT, aMeta->mReplication.mRepName, aMeta->mReplication.mCompressType );
    }
    else
    {
        cmiDisableCompress( mProtocolContext );
        cmiSetDecompressType( mProtocolContext, aMeta->mReplication.mCompressType );
        
        ideLog::log( IDE_RP_0, "[Sender] COMPRESS XLOG OFF RepName : %s, CompressType : %"ID_UINT32_FMT, aMeta->mReplication.mRepName, aMeta->mReplication.mCompressType );
    }
    
    RP_LABEL( NORMAL_EXIT );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_CHECK_REPL_VERSION );
    {
    }
    IDE_EXCEPTION( ERR_UNEXPECTED_HANDSHAKE_ACK );
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_UNEXPECTED_HANDSHAKE_ACK, sMetaResult ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/*
 *
 */
IDE_RC rpnMessenger::sendStop( smSN       aRestartSN,
                               idBool   * aExitFlag, 
                               UInt       aTimeoutSec )
{
    IDE_TEST( rpnComm::sendStop( mHBTResource,
                                 mProtocolContext,
                                 aExitFlag,
                                 RP_TID_NULL,
                                 SM_SN_NULL,
                                 SM_SN_NULL,
                                 aRestartSN,
                                 aTimeoutSec )
              != IDE_SUCCESS );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 *
 */
IDE_RC rpnMessenger::receiveAckDummy( void )
{
    idBool     sExitDummyFlag = ID_FALSE;
    rpXLogAck  sDummyAck;
    idBool     sIsTimeOut     = ID_FALSE;

    IDE_TEST( rpnComm::recvAck( NULL, /* memory allocator : not used */
                                mHBTResource,
                                mProtocolContext,
                                &sExitDummyFlag,
                                &sDummyAck,
                                RPU_REPLICATION_RECEIVE_TIMEOUT,
                                &sIsTimeOut )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 *
 */
IDE_RC rpnMessenger::receiveAck( rpXLogAck * aAck,
                                 idBool    * aExitFlag,
                                 ULong       aTimeOut,
                                 idBool    * aIsTimeOut )
{
    IDE_TEST( rpnComm::recvAck( NULL, /* memory allocator : not used */
                                mHBTResource,
                                mProtocolContext,
                                aExitFlag,
                                aAck,
                                aTimeOut,
                                aIsTimeOut )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 *
 */
IDE_RC rpnMessenger::sendStopAndReceiveAck( void )
{
    rpXLogAck  sDummyAck;
    idBool     sIsTimeOut = ID_FALSE;

    IDE_TEST( rpnComm::sendStop( mHBTResource,
                                 mProtocolContext,
                                 NULL,  /* exit flag */
                                 RP_TID_NULL,
                                 SM_SN_NULL,
                                 SM_SN_NULL,
                                 SM_SN_NULL,
                                 RPN_STOP_MSG_NETWORK_TIMEOUT_SEC )
              != IDE_SUCCESS );

    IDE_TEST( rpnComm::recvAck( NULL, /* memory allocator : not used */
                                mHBTResource,
                                mProtocolContext,
                                NULL, /* exit flag */
                                &sDummyAck,
                                RPN_STOP_MSG_NETWORK_TIMEOUT_SEC,
                                &sIsTimeOut )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 *
 */
IDE_RC rpnMessenger::sendSyncXLogTrCommit( smTID aTransID )
{
    IDE_TEST( rpnComm::sendTrCommit( mHBTResource,
                                     mProtocolContext,
                                     mExitFlag,
                                     aTransID,
                                     SM_SN_NULL,
                                     SM_SN_NULL,
                                     ID_FALSE,
                                     RPU_REPLICATION_SENDER_SEND_TIMEOUT )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 *
 */
IDE_RC rpnMessenger::sendSyncXLogTrAbort( smTID aTransID )
{
    IDE_TEST( rpnComm::sendTrAbort( mHBTResource,
                                    mProtocolContext,
                                    mExitFlag,
                                    aTransID,
                                    SM_SN_NULL,
                                    SM_SN_NULL,
                                    RPU_REPLICATION_SENDER_SEND_TIMEOUT )
              != IDE_SUCCESS );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 *
 */
IDE_RC rpnMessenger::syncXLogInsert( smTID       aTransID,
                                     SChar     * aTuple,
                                     mtcColumn * aMtcCol,
                                     UInt        aColCnt,
                                     smOID       aTableOID )
{
    const smiColumn * sColumn;
    UInt              sLength;
    UInt              sCID;
    UInt              i;
    smiValue          sColArray[QCI_MAX_COLUMN_COUNT];

    for ( i = 0; i < aColCnt; i ++ )
    {
        sColumn = &aMtcCol[i].column;

        sCID = sColumn->id & SMI_COLUMN_ID_MASK;
        IDE_ASSERT( sCID < QCI_MAX_COLUMN_COUNT );

        if ( ( sColumn->flag & SMI_COLUMN_COMPRESSION_MASK )
             == SMI_COLUMN_COMPRESSION_FALSE )
        {
            if ( ( sColumn->flag & SMI_COLUMN_TYPE_MASK ) == SMI_COLUMN_TYPE_FIXED )
            {
                sColArray[sCID].value = aTuple + sColumn->offset;
                // BUG-29269
                sLength = aMtcCol[i].module->actualSize( &aMtcCol[i],
                                                         sColArray[sCID].value );
                sColArray[sCID].length = sLength;
            }
            else if ( ( ( sColumn->flag & SMI_COLUMN_TYPE_MASK )
                        == SMI_COLUMN_TYPE_VARIABLE ) ||
                      ( ( sColumn->flag & SMI_COLUMN_TYPE_MASK )
                        == SMI_COLUMN_TYPE_VARIABLE_LARGE ) )
            {
                sColArray[sCID].value = smiGetVarColumn( aTuple, sColumn, &sLength );
                sColArray[sCID].length = sLength;
            }
            else
            {
                IDE_DASSERT( ( sColumn->flag & SMI_COLUMN_TYPE_MASK )
                             == SMI_COLUMN_TYPE_LOB );
                sColArray[sCID].value = NULL;
                sColArray[sCID].length = 0;
            }
        }
        else
        {
            sColArray[sCID].value = mtc::getCompressionColumn( aTuple,
                                                               sColumn,
                                                               ID_TRUE, /* aUseComlumnOffset */
                                                               &(sColArray[sCID].length) );
        }
    }

    IDE_TEST( rpnComm::sendSyncInsert( mHBTResource,
                                       mProtocolContext,
                                       mExitFlag,
                                       aTransID,
                                       0,
                                       SM_SN_NULL,
                                       SMI_STATEMENT_DEPTH_NULL,
                                       aTableOID,
                                       aColCnt,
                                       sColArray,
                                       mSyncAfterMtdValueLen,
                                       RPU_REPLICATION_SENDER_SEND_TIMEOUT )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 *
 */
IDE_RC rpnMessenger::sendXLogInsert( smTID       aTransID,
                                     SChar     * aTuple,
                                     mtcColumn * aMtcCol,
                                     UInt        aColCnt,
                                     smOID       aTableOID )
{
    const smiColumn * sColumn;
    UInt              sLength;
    UInt              sCID;
    UInt              i;
    smiValue          sColArray[QCI_MAX_COLUMN_COUNT];

    for ( i = 0; i < aColCnt; i ++ )
    {
        sColumn = &aMtcCol[i].column;

        sCID = sColumn->id & SMI_COLUMN_ID_MASK;
        IDE_ASSERT( sCID < QCI_MAX_COLUMN_COUNT );

        if ( ( sColumn->flag & SMI_COLUMN_TYPE_MASK ) == SMI_COLUMN_TYPE_FIXED )
        {
            sColArray[sCID].value = aTuple + sColumn->offset;
            // BUG-29269
            sLength = aMtcCol[i].module->actualSize( &aMtcCol[i],
                                                     sColArray[sCID].value );
            sColArray[sCID].length = sLength;
        }
        else if ( ( ( sColumn->flag & SMI_COLUMN_TYPE_MASK )
                    == SMI_COLUMN_TYPE_VARIABLE ) ||
                  ( ( sColumn->flag & SMI_COLUMN_TYPE_MASK )
                    == SMI_COLUMN_TYPE_VARIABLE_LARGE ) )
        {
            sColArray[sCID].value = smiGetVarColumn( aTuple, sColumn, &sLength );
            sColArray[sCID].length = sLength;
        }
        else
        {
            IDE_DASSERT( ( sColumn->flag & SMI_COLUMN_TYPE_MASK )
                         == SMI_COLUMN_TYPE_LOB );
            sColArray[sCID].value = NULL;
            sColArray[sCID].length = 0;
        }
    }

    IDE_TEST( rpnComm::sendInsert( mHBTResource,
                                   mProtocolContext,
                                   mExitFlag,
                                   aTransID,
                                   0,
                                   SM_SN_NULL,
                                   SMI_STATEMENT_DEPTH_NULL,
                                   aTableOID,
                                   aColCnt,
                                   sColArray,
                                   mSyncAfterMtdValueLen,
                                   RPU_REPLICATION_SENDER_SEND_TIMEOUT )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 *
 */
IDE_RC rpnMessenger::sendXLogLob( smTID               aTransID,
                                  SChar             * aTuple,
                                  scGRID              aRowGRID,
                                  mtcColumn         * aMtcCol,
                                  smOID               aTableOID,
                                  smiTableCursor    * aCursor )
{
    UInt           i;
    UInt           j;
    void         * sTable = NULL;
    smiColumn    * sColumn = NULL;
    smiColumn    * sPKColumn = NULL;
    UInt           sLength;
    UInt           sColCount;
    UInt           sCID;
    UInt           sPKCID;
    UInt           sLOBSize;
    UInt           sOffset;
    smLobLocator   sLocator;
    UInt           sPKColCnt;
    smiValue     * sPK = NULL;
    qcmIndex     * sPKIndex = NULL;
    SChar        * sPiece = NULL;
    UInt           sPieceLen = RP_BLOB_VALUSE_PIECES_SIZE_FOR_CM;
    UInt           sRemainLen;
    qcmTableInfo * sTableInfo = NULL;
    UInt           sReadLength;
    idBool         sIsLobCursorOpened = ID_FALSE;

    sTable     = (void *)smiGetTable( aTableOID );
    sTableInfo = (qcmTableInfo *)rpdCatalog::rpdGetTableTempInfo( (const void *)sTable );
    sColCount  = smiGetTableColumnCount( sTable );
    sPKIndex   = sTableInfo->primaryKey;    // SYNC는 현재 정보를 사용한다.
    sPKColCnt  = sPKIndex->keyColCount;

    /* LOB Cursor open을 위해서 Primary Key가 필요함. Primary Key를
     * 구성하기 위한 메모리 할당 */
    IDE_TEST_RAISE( iduMemMgr::malloc( IDU_MEM_RP_RPX_SYNC,
                                       ID_SIZEOF(smiValue) * sPKColCnt,
                                       (void **)&sPK,
                                       IDU_MEM_IMMEDIATE )
                    != IDE_SUCCESS, ERR_MEMORY_ALLOC_PK );

    /* 현재는 각 Piece의 길이를 32K로 하드코딩한 상태임. 추후에
     * Property 등으로 변환하는 방법을 고려해 볼 수 있음 */
    IDE_TEST_RAISE( iduMemMgr::malloc( IDU_MEM_RP_RPX_SYNC,
                                       sPieceLen,
                                       (void **)&sPiece,
                                       IDU_MEM_IMMEDIATE )
                    != IDE_SUCCESS, ERR_MEMORY_ALLOC_PIECE );

    /* Primary Key를 구성한다. 이미 readRow()를 통하여 record를 fetch한 상태이므로
     * 여기서는 Primary Key column에 대하여, smiValue array를 구성하도록 한다.
     */
    for ( i = 0; i < sPKColCnt; i ++ )
    {
        sPKCID = sPKIndex->keyColumns[i].column.id & SMI_COLUMN_ID_MASK;
        sPKColumn = &aMtcCol[sPKCID].column;

        if ( ( sPKColumn->flag & SMI_COLUMN_TYPE_MASK ) == SMI_COLUMN_TYPE_FIXED )
        {
            sPK[i].value = aTuple + sPKColumn->offset;
            sPK[i].length = sPKColumn->size;
        }
        else if ( ( ( sPKColumn->flag & SMI_COLUMN_TYPE_MASK ) == SMI_COLUMN_TYPE_VARIABLE ) ||
                  ( ( sPKColumn->flag & SMI_COLUMN_TYPE_MASK ) == SMI_COLUMN_TYPE_VARIABLE_LARGE ) )
        {
            sPK[i].value = smiGetVarColumn( aTuple,
                                            sPKColumn,
                                            &sLength );
            sPK[i].length = sLength;
        }
    }

    for ( i = 0; i < sColCount; i ++ )
    {
        sColumn = &aMtcCol[i].column;

        sCID = sColumn->id & SMI_COLUMN_ID_MASK;

        if ( ( sColumn->flag & SMI_COLUMN_TYPE_MASK ) != SMI_COLUMN_TYPE_LOB )
        {
            continue;
        }

        if ( ( SMI_MISC_TABLE_HEADER( sTable )->mFlag & SMI_TABLE_TYPE_MASK )
             == SMI_TABLE_DISK )
        {
            //fix BUG-19687
            IDU_FIT_POINT_RAISE( "rpnMessenger::sendXLogLob::Erratic::rpERR_ABORT_OPEN_LOB_CURSOR",
                                 ERR_LOB_CURSOR_OPEN ); 
            IDE_TEST_RAISE( smiLob::openLobCursorWithGRID( aCursor,
                                                           aRowGRID,
                                                           sColumn,
                                                           0,
                                                           SMI_LOB_READ_MODE,
                                                           &sLocator )
                            != IDE_SUCCESS, ERR_LOB_CURSOR_OPEN );
        }
        else
        {
            IDE_TEST_RAISE( smiLob::openLobCursorWithRow( aCursor,
                                                          aTuple,
                                                          sColumn,
                                                          0,
                                                          SMI_LOB_READ_MODE,
                                                          &sLocator )
                            != IDE_SUCCESS, ERR_LOB_CURSOR_OPEN );
        }
        sIsLobCursorOpened = ID_TRUE;

        IDE_TEST_RAISE( rpnComm::sendLobCursorOpen( mHBTResource,
                                                    mProtocolContext,
                                                    mExitFlag,
                                                    aTransID,
                                                    0,
                                                    SM_SN_NULL,
                                                    aTableOID,
                                                    sLocator,
                                                    sCID,
                                                    sPKColCnt,
                                                    sPK,
                                                    mSyncPKMtdValueLen,
                                                    RPU_REPLICATION_SENDER_SEND_TIMEOUT )
                        != IDE_SUCCESS, ERR_SEND_OPEN_LOB_CURSOR );

        IDE_TEST_RAISE( qciMisc::lobGetLength( NULL, /* idvSQL* */
                                               sLocator,
                                               &sLOBSize )
                        != IDE_SUCCESS, ERR_GET_LENGTH );

        IDE_TEST_RAISE( rpnComm::sendLobPrepare4Write( mHBTResource,
                                                       mProtocolContext,
                                                       mExitFlag,
                                                       aTransID,
                                                       0,
                                                       SM_SN_NULL,
                                                       sLocator,
                                                       0,
                                                       0,
                                                       sLOBSize,
                                                       RPU_REPLICATION_SENDER_SEND_TIMEOUT )
                        != IDE_SUCCESS, ERR_SEND_PREPARE4WRITE );

        /* 실제 LOB value를 read하여 전송하도록 한다. 한번에 모든 LOB value를 전송하려고 하면,
         * Buffer의 할당 크기가 커지는 단점이 있어서, 주어진 크기만큼 분할하여
         * read -> send 하도록 한다. 현재는 32KB로 정의해 놓았으나, 추후에 변경이
         * 가능하도록 한다.
         */
        for ( j = 0; j < (sLOBSize / sPieceLen); j ++ )
        {
            sOffset = j * sPieceLen;

            IDE_TEST_RAISE( smiLob::read( NULL, /*idvSQL* */
                                          sLocator,
                                          sOffset,
                                          sPieceLen,
                                          (UChar *)sPiece,
                                          &sReadLength )
                            != IDE_SUCCESS, ERR_LOB_READ );

            IDE_TEST_RAISE( rpnComm::sendLobPartialWrite( mHBTResource,
                                                          mProtocolContext,
                                                          mExitFlag,
                                                          aTransID,
                                                          0,
                                                          SM_SN_NULL,
                                                          sLocator,
                                                          sOffset,
                                                          sReadLength,
                                                          sPiece,
                                                          RPU_REPLICATION_SENDER_SEND_TIMEOUT )
                            != IDE_SUCCESS, ERR_SEND_PARTIAL_WRITE );
        }

        sOffset = j * sPieceLen;
        sRemainLen = sLOBSize - sOffset;

        if ( sRemainLen != 0 )
        {
            IDE_TEST_RAISE( smiLob::read( NULL, /*idvSQL* */
                                          sLocator, sOffset, sRemainLen,
                                          (UChar *)sPiece, &sReadLength )
                            != IDE_SUCCESS, ERR_LOB_READ );

            IDE_TEST_RAISE( rpnComm::sendLobPartialWrite( mHBTResource,
                                                          mProtocolContext,
                                                          mExitFlag,
                                                          aTransID,
                                                          0,
                                                          SM_SN_NULL,
                                                          sLocator,
                                                          sOffset,
                                                          sReadLength,
                                                          sPiece,
                                                          RPU_REPLICATION_SENDER_SEND_TIMEOUT )
                            != IDE_SUCCESS, ERR_SEND_PARTIAL_WRITE );
        }

        IDE_TEST_RAISE( rpnComm::sendLobFinish2Write( mHBTResource,
                                                      mProtocolContext,
                                                      mExitFlag,
                                                      aTransID,
                                                      0,
                                                      SM_SN_NULL,
                                                      sLocator,
                                                      RPU_REPLICATION_SENDER_SEND_TIMEOUT )
                        != IDE_SUCCESS, ERR_SEND_FINISH2WRITE );

        sIsLobCursorOpened = ID_FALSE;

        IDU_FIT_POINT_RAISE( "rpnMessenger::sendXLogLob::Erratic::rpERR_ABORT_CLOSE_LOB_CURSOR",
                             ERR_LOB_CURSOR_CLOSE );
        IDE_TEST_RAISE( smiLob::closeLobCursor( NULL, /* idvSQL* */
                                                sLocator )
                        != IDE_SUCCESS, ERR_LOB_CURSOR_CLOSE );

        IDE_TEST_RAISE( rpnComm::sendLobCursorClose( mHBTResource,
                                                     mProtocolContext,
                                                     mExitFlag,
                                                     aTransID,
                                                     0,
                                                     SM_SN_NULL,
                                                     sLocator,
                                                     RPU_REPLICATION_SENDER_SEND_TIMEOUT )
                        != IDE_SUCCESS, ERR_SEND_CLOSE_LOB_CURSOR );
    }

    if ( sPK != NULL )
    {
        (void)iduMemMgr::free( sPK );
        sPK = NULL;
    }
    if ( sPiece != NULL )
    {
        (void)iduMemMgr::free( sPiece );
        sPiece = NULL;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_LOB_CURSOR_OPEN )
    {
        IDE_ERRLOG( IDE_RP_0 );
        IDE_SET( ideSetErrorCode( rpERR_ABORT_OPEN_LOB_CURSOR ) );
    }
    IDE_EXCEPTION( ERR_LOB_CURSOR_CLOSE )
    {
        IDE_ERRLOG( IDE_RP_0 );
        IDE_SET( ideSetErrorCode( rpERR_ABORT_CLOSE_LOB_CURSOR ) );
    }
    IDE_EXCEPTION( ERR_GET_LENGTH )
    {
        IDE_ERRLOG( IDE_RP_0 );
        IDE_SET( ideSetErrorCode( rpERR_ABORT_LOB_GET_LENGTH ) );
    }
    IDE_EXCEPTION( ERR_LOB_READ )
    {
        IDE_ERRLOG( IDE_RP_0 );
        IDE_SET( ideSetErrorCode( rpERR_ABORT_LOB_READ ) );
    }
    IDE_EXCEPTION( ERR_SEND_OPEN_LOB_CURSOR )
    {
    }
    IDE_EXCEPTION( ERR_SEND_CLOSE_LOB_CURSOR )
    {
    }
    IDE_EXCEPTION( ERR_SEND_PREPARE4WRITE )
    {
    }
    IDE_EXCEPTION( ERR_SEND_FINISH2WRITE )
    {
    }
    IDE_EXCEPTION( ERR_SEND_PARTIAL_WRITE )
    {
    }
    IDE_EXCEPTION( ERR_MEMORY_ALLOC_PK )
    {
        IDE_ERRLOG( IDE_RP_0 );
        IDE_SET( ideSetErrorCode( rpERR_ABORT_MEMORY_ALLOC,
                                  "rpnMessenger::syncXLogLob",
                                  "sPK" ) );
    }
    IDE_EXCEPTION( ERR_MEMORY_ALLOC_PIECE )
    {
        IDE_ERRLOG( IDE_RP_0 );
        IDE_SET( ideSetErrorCode( rpERR_ABORT_MEMORY_ALLOC,
                                  "rpnMessenger::syncXLogLob",
                                  "sPiece" ) );
    }
    IDE_EXCEPTION_END;
    IDE_PUSH();

    if ( sIsLobCursorOpened == ID_TRUE )
    {
        (void)smiLob::closeLobCursor( NULL, /* idvSQL* */
                                      sLocator );
    }

    if ( sPK != NULL )
    {
        (void)iduMemMgr::free( sPK );
    }

    if ( sPiece != NULL )
    {
        (void)iduMemMgr::free( sPiece );
    }

    IDE_POP();
    return IDE_FAILURE;
}

/*
 *
 */
void rpnMessenger::setRemoteTcpAddress( SChar * aRemoteIP,
                                        SInt    aRemotePort )
{
    (void)idlOS::snprintf( mRemoteIP, sizeof( mRemoteIP ), "%s", aRemoteIP );
    mRemotePort = aRemotePort;
}

void rpnMessenger::setRemoteIBAddress(  SChar      * aRemoteIP,
                                        SInt         aRemotePort,
                                        rpIBLatency  aIBLatency )
{
    (void)idlOS::snprintf( mRemoteIP, ID_SIZEOF( mRemoteIP ), "%s", aRemoteIP );
    mRemotePort = aRemotePort;
    mIBLatency  = aIBLatency;
}


/*
 *
 */
void rpnMessenger::updateAddress( void )
{
    SChar sPort[IDL_IP_PORT_MAX_LEN];

    if ( rpnComm::isConnected( mLink ) == ID_TRUE )
    {
        // Connection Established
        (void)cmiGetLinkInfo( mLink,
                              mLocalIP,
                              sizeof( mLocalIP ),
                              CMI_LINK_INFO_LOCAL_IP_ADDRESS );
        (void)cmiGetLinkInfo( mLink,
                              mRemoteIP,
                              sizeof( mRemoteIP ),
                              CMI_LINK_INFO_REMOTE_IP_ADDRESS );
        
        // BUG-15944
        (void)idlOS::memset( sPort, 0x00, sizeof( sPort ) );
        (void)cmiGetLinkInfo( mLink,
                              sPort,
                              sizeof( sPort ),
                              CMI_LINK_INFO_LOCAL_PORT );
        mLocalPort = idlOS::atoi( sPort );
        (void)idlOS::memset( sPort, 0x00, sizeof( sPort ) );
        (void)cmiGetLinkInfo( mLink,
                              sPort,
                              sizeof( sPort ),
                              CMI_LINK_INFO_REMOTE_PORT );
        mRemotePort = idlOS::atoi( sPort );
    }
    else
    {
        /* nothing to do */
    }
}

/*
 *
 */
IDE_RC rpnMessenger::sendTrCommit( rpdTransTbl      * aTransTbl,
                                   rpdSenderInfo    * aSenderInfo,
                                   rpdLogAnalyzer   * aLogAnlz,
                                   smSN               aFlushSN )
{
    idBool sIsTimeOut  = ID_FALSE;
    idBool sIsExist    = ID_FALSE;
    rpXLogAck sReceivedAck;

    if ( ( aTransTbl->isSvpListSent( aLogAnlz->mTID ) != ID_TRUE ) &&
         ( aTransTbl->isGTrans( aLogAnlz->mTID ) != ID_TRUE ) )
    {
        // BUG-28206 불필요한 Transaction Begin을 방지
    }
    else
    {
        setLastSN( aSenderInfo, aLogAnlz->mTID, aLogAnlz->mSN );

        /* proj-1608 recovery sender는 매 commit마다
         * ack를 수신하여 해당 트랜잭션을 sn map에서 지운다.
         * recovery과정에서는 이미 수행한 트랜잭션을 다시
         * 수행하지 않도록 하기 위함이다.
         */
        if ( mSNMapMgr != NULL )
        {
            IDE_TEST( rpnComm::sendTrCommit( mHBTResource,
                                             mProtocolContext,
                                             mExitFlag,
                                             aLogAnlz->getSendTransID(),
                                             aLogAnlz->mSN,
                                             aFlushSN,
                                             ID_TRUE,
                                             RPU_REPLICATION_SENDER_SEND_TIMEOUT )
                      != IDE_SUCCESS );

            IDE_TEST( rpnComm::recvAck( NULL, /* memory allocator : not used */
                                        mHBTResource,
                                        mProtocolContext,
                                        mExitFlag,
                                        &sReceivedAck,
                                        (ULong)RPU_REPLICATION_RECEIVE_TIMEOUT,
                                        &sIsTimeOut )
                      != IDE_SUCCESS );

            IDE_TEST( sIsTimeOut == ID_TRUE );
        }
        else
        {
            if  ( aTransTbl->isGTrans( aLogAnlz->mTID ) != ID_TRUE ) 
            {
                IDE_TEST( rpnComm::sendTrCommit( mHBTResource,
                                                 mProtocolContext,
                                                 mExitFlag,
                                                 aLogAnlz->getSendTransID(),
                                                 aLogAnlz->mSN,
                                                 aFlushSN,
                                                 mIsFlushCommitXLog,
                                                 RPU_REPLICATION_SENDER_SEND_TIMEOUT )
                          != IDE_SUCCESS );
            }
            else
            {
                IDE_TEST( rpnComm::sendXaCommit( mHBTResource,
                                                 mProtocolContext,
                                                 mExitFlag,
                                                 aLogAnlz->getSendTransID(),
                                                 aLogAnlz->mSN,
                                                 aFlushSN,
                                                 aLogAnlz->mGlobalCommitSCN,
                                                 mIsFlushCommitXLog,
                                                 RPU_REPLICATION_SENDER_SEND_TIMEOUT )
                          != IDE_SUCCESS );
            }
        }
    }

    if ( mSNMapMgr != NULL )
    {
        mSNMapMgr->deleteTxByReplicatedCommitSN( aLogAnlz->mSN,
                                                 &sIsExist );

        IDE_DASSERT( sIsExist == ID_TRUE );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 *
 */
IDE_RC rpnMessenger::sendTrAbort( rpdTransTbl       * aTransTbl,
                                  rpdSenderInfo     * aSenderInfo,
                                  rpdLogAnalyzer    * aLogAnlz,
                                  smSN                aFlushSN )
{
    if ( aTransTbl->isSvpListSent( aLogAnlz->mTID ) != ID_TRUE )
    {
        // BUG-28206 불필요한 Transaction Begin을 방지
    }
    else
    {
        setLastSN( aSenderInfo, aLogAnlz->mTID, aLogAnlz->mSN );

        IDE_TEST( rpnComm::sendTrAbort( mHBTResource,
                                        mProtocolContext,
                                        mExitFlag,
                                        aLogAnlz->getSendTransID(),
                                        aLogAnlz->mSN,
                                        aFlushSN,
                                        RPU_REPLICATION_SENDER_SEND_TIMEOUT )
                 != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 *
 */
IDE_RC rpnMessenger::sendInsert( rpdTransTbl    * aTransTbl,
                                 rpdSenderInfo  * aSenderInfo,
                                 rpdLogAnalyzer * aLogAnlz,
                                 smSN             aFlushSN )
{
    setLastSN( aSenderInfo, aLogAnlz->mTID, aLogAnlz->mSN );

    // BUG-28206 불필요한 Transaction Begin을 방지
    IDE_TEST( checkAndSendSvpList( aTransTbl, aLogAnlz, aFlushSN )
              != IDE_SUCCESS );

    IDE_TEST( rpnComm::sendInsert( mHBTResource,
                                   mProtocolContext,
                                   mExitFlag,
                                   aLogAnlz->getSendTransID(),
                                   aLogAnlz->mSN,
                                   aFlushSN,
                                   aLogAnlz->mImplSPDepth,
                                   aLogAnlz->mTableOID,
                                   aLogAnlz->mRedoAnalyzedColCnt,
                                   aLogAnlz->mACols,
                                   aLogAnlz->mAMtdValueLen,
                                   RPU_REPLICATION_SENDER_SEND_TIMEOUT )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 *
 */
IDE_RC rpnMessenger::sendUpdate( rpdTransTbl        * aTransTbl,
                                 rpdSenderInfo      * aSenderInfo,
                                 rpdLogAnalyzer     * aLogAnlz,
                                 smSN                 aFlushSN )
{
    setLastSN( aSenderInfo, aLogAnlz->mTID, aLogAnlz->mSN );

    // BUG-28206 불필요한 Transaction Begin을 방지
    IDE_TEST( checkAndSendSvpList( aTransTbl, aLogAnlz, aFlushSN )
              != IDE_SUCCESS );

    IDE_TEST( rpnComm::sendUpdate( mHBTResource,
                                   mProtocolContext,
                                   mExitFlag,
                                   aLogAnlz->getSendTransID(),
                                   aLogAnlz->mSN,
                                   aFlushSN,
                                   aLogAnlz->mImplSPDepth,
                                   aLogAnlz->mTableOID,
                                   aLogAnlz->mPKColCnt,
                                   aLogAnlz->mRedoAnalyzedColCnt,
                                   aLogAnlz->mPKCols,
                                   aLogAnlz->mCIDs,
                                   aLogAnlz->mBCols,
                                   aLogAnlz->mBChainedCols,
                                   aLogAnlz->mChainedValueTotalLen,
                                   aLogAnlz->mACols,
                                   aLogAnlz->mPKMtdValueLen,
                                   aLogAnlz->mAMtdValueLen,
                                   aLogAnlz->mBMtdValueLen,
                                   RPU_REPLICATION_SENDER_SEND_TIMEOUT )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 *
 */
IDE_RC rpnMessenger::sendDelete( rpdTransTbl        * aTransTbl,
                                 rpdSenderInfo      * aSenderInfo,
                                 rpdLogAnalyzer     * aLogAnlz,
                                 smSN                 aFlushSN )
{
    setLastSN( aSenderInfo, aLogAnlz->mTID, aLogAnlz->mSN );

    // BUG-28206 불필요한 Transaction Begin을 방지
    IDE_TEST( checkAndSendSvpList( aTransTbl, aLogAnlz, aFlushSN )
              != IDE_SUCCESS );

    IDE_TEST( rpnComm::sendDelete( mHBTResource,
                                   mProtocolContext,
                                   mExitFlag,
                                   aLogAnlz->getSendTransID(),
                                   aLogAnlz->mSN,
                                   aFlushSN,
                                   aLogAnlz->mImplSPDepth,
                                   aLogAnlz->mTableOID,
                                   aLogAnlz->mPKColCnt,
                                   aLogAnlz->mPKCols,
                                   aLogAnlz->mPKMtdValueLen,
                                   aLogAnlz->mUndoAnalyzedColCnt,
                                   aLogAnlz->mCIDs,
                                   aLogAnlz->mBCols,
                                   aLogAnlz->mBChainedCols,
                                   aLogAnlz->mBMtdValueLen,
                                   aLogAnlz->mChainedValueTotalLen,
                                   RPU_REPLICATION_SENDER_SEND_TIMEOUT )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 *
 */
IDE_RC rpnMessenger::sendSPSet( rpdTransTbl         * aTransTbl,
                                rpdSenderInfo       * aSenderInfo,
                                rpdLogAnalyzer      * aLogAnlz,
                                smSN                  aFlushSN )
{
    SChar           * sPsmSvpName = NULL;
    rpSavepointType   sType       = RP_SAVEPOINT_EXPLICIT;

    if ( aTransTbl->isSvpListSent( aLogAnlz->mTID ) != ID_TRUE )
    {
        sPsmSvpName = smiGetPsmSvpName();
        if ( idlOS::strcmp( aLogAnlz->mSPName, sPsmSvpName ) == 0 )
        {
            sType = RP_SAVEPOINT_PSM;
        }

        // BUG-28206 불필요한 Transaction Begin을 방지
        IDE_TEST( aTransTbl->addLastSvpEntry( aLogAnlz->mTID,
                                              aLogAnlz->mSN,
                                              sType,
                                              aLogAnlz->mSPName,
                                              SMI_STATEMENT_DEPTH_NULL )
                  != IDE_SUCCESS );
    }
    else
    {
        setLastSN( aSenderInfo, aLogAnlz->mTID, aLogAnlz->mSN );

        IDE_TEST( rpnComm::sendSPSet( mHBTResource,
                                      mProtocolContext,
                                      mExitFlag,
                                      aLogAnlz->getSendTransID(),
                                      aLogAnlz->mSN,
                                      aFlushSN,
                                      aLogAnlz->mSPNameLen,
                                      aLogAnlz->mSPName,
                                      RPU_REPLICATION_SENDER_SEND_TIMEOUT )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 *
 */
IDE_RC rpnMessenger::sendSPAbort( rpdTransTbl       * aTransTbl,
                                  rpdSenderInfo     * aSenderInfo,
                                  rpdLogAnalyzer    * aLogAnlz,
                                  smSN                aFlushSN )
{
    smSN          sDummySN  = SM_SN_NULL;

    if ( aTransTbl->isSvpListSent( aLogAnlz->mTID ) != ID_TRUE )
    {
        // BUG-28206 불필요한 Transaction Begin을 방지
        aTransTbl->applySvpAbort( aLogAnlz->mTID, aLogAnlz->mSPName, &sDummySN );
    }
    else
    {
        setLastSN( aSenderInfo, aLogAnlz->mTID, aLogAnlz->mSN );

        IDE_TEST( rpnComm::sendSPAbort( mHBTResource,
                                        mProtocolContext,
                                        mExitFlag,
                                        aLogAnlz->getSendTransID(),
                                        aLogAnlz->mSN,
                                        aFlushSN,
                                        aLogAnlz->mSPNameLen,
                                        aLogAnlz->mSPName,
                                        RPU_REPLICATION_SENDER_SEND_TIMEOUT )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 *
 */
IDE_RC rpnMessenger::sendLobCursorOpen( rpdTransTbl     * aTransTbl,
                                        rpdSenderInfo   * aSenderInfo,
                                        rpdLogAnalyzer  * aLogAnlz,
                                        smSN              aFlushSN )
{
    setLastSN( aSenderInfo, aLogAnlz->mTID, aLogAnlz->mSN );

    // BUG-28206 불필요한 Transaction Begin을 방지
    IDE_TEST( checkAndSendSvpList( aTransTbl, aLogAnlz, aFlushSN )
              != IDE_SUCCESS );

    IDE_TEST( rpnComm::sendLobCursorOpen( mHBTResource,
                                          mProtocolContext,
                                          mExitFlag,
                                          aLogAnlz->getSendTransID(),
                                          aLogAnlz->mSN,
                                          aFlushSN,
                                          aLogAnlz->mTableOID,
                                          aLogAnlz->mLobLocator,
                                          aLogAnlz->mLobColumnID,
                                          aLogAnlz->mPKColCnt,
                                          aLogAnlz->mPKCols,
                                          aLogAnlz->mPKMtdValueLen,
                                          RPU_REPLICATION_SENDER_SEND_TIMEOUT )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 *
 */
IDE_RC rpnMessenger::sendLobCursorClose( rpdLogAnalyzer * aLogAnlz,
                                         rpdSenderInfo  * aSenderInfo,
                                         smSN             aFlushSN )
{
    setLastSN( aSenderInfo, aLogAnlz->mTID, aLogAnlz->mSN );

    IDE_TEST( rpnComm::sendLobCursorClose( mHBTResource,
                                           mProtocolContext,
                                           mExitFlag,
                                           aLogAnlz->getSendTransID(),
                                           aLogAnlz->mSN,
                                           aFlushSN,
                                           aLogAnlz->mLobLocator,
                                           RPU_REPLICATION_SENDER_SEND_TIMEOUT )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 *
 */
IDE_RC rpnMessenger::sendLobPrepareWrite( rpdLogAnalyzer    * aLogAnlz,
                                          rpdSenderInfo     * aSenderInfo,
                                          smSN                aFlushSN )
{
    setLastSN( aSenderInfo, aLogAnlz->mTID, aLogAnlz->mSN );

    IDE_TEST( rpnComm::sendLobPrepare4Write( mHBTResource,
                                             mProtocolContext,
                                             mExitFlag,
                                             aLogAnlz->getSendTransID(),
                                             aLogAnlz->mSN,
                                             aFlushSN,
                                             aLogAnlz->mLobLocator,
                                             aLogAnlz->mLobOffset,
                                             aLogAnlz->mLobOldSize,
                                             aLogAnlz->mLobNewSize,
                                             RPU_REPLICATION_SENDER_SEND_TIMEOUT )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 *
 */
IDE_RC rpnMessenger::sendLobPartialWrite( rpdLogAnalyzer    * aLogAnlz,
                                          rpdSenderInfo     * aSenderInfo,
                                          smSN                aFlushSN )
{
    setLastSN( aSenderInfo, aLogAnlz->mTID, aLogAnlz->mSN );

    IDE_TEST( rpnComm::sendLobPartialWrite( mHBTResource,
                                            mProtocolContext,
                                            mExitFlag,
                                            aLogAnlz->getSendTransID(),
                                            aLogAnlz->mSN,
                                            aFlushSN,
                                            aLogAnlz->mLobLocator,
                                            aLogAnlz->mLobOffset,
                                            aLogAnlz->mLobPieceLen,
                                            aLogAnlz->mLobPiece,
                                            RPU_REPLICATION_SENDER_SEND_TIMEOUT )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 *
 */
IDE_RC rpnMessenger::sendLobFinishWrite( rpdLogAnalyzer     * aLogAnlz,
                                         rpdSenderInfo      * aSenderInfo,
                                         smSN                 aFlushSN )
{
    setLastSN( aSenderInfo, aLogAnlz->mTID, aLogAnlz->mSN );

    IDE_TEST( rpnComm::sendLobFinish2Write( mHBTResource,
                                            mProtocolContext,
                                            mExitFlag,
                                            aLogAnlz->getSendTransID(),
                                            aLogAnlz->mSN,
                                            aFlushSN,
                                            aLogAnlz->mLobLocator,
                                            RPU_REPLICATION_SENDER_SEND_TIMEOUT )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 *
 */
IDE_RC rpnMessenger::sendLobTrim( rpdLogAnalyzer    * aLogAnlz,
                                  rpdSenderInfo     * aSenderInfo,
                                  smSN                aFlushSN )
{
    setLastSN( aSenderInfo, aLogAnlz->mTID, aLogAnlz->mSN );

    IDE_TEST( rpnComm::sendLobTrim( mHBTResource,
                                    mProtocolContext,
                                    mExitFlag,
                                    aLogAnlz->getSendTransID(),
                                    aLogAnlz->mSN,
                                    aFlushSN,
                                    aLogAnlz->mLobLocator,
                                    aLogAnlz->mLobOffset,
                                    RPU_REPLICATION_SENDER_SEND_TIMEOUT )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpnMessenger::sendXaStartReq( rpdLogAnalyzer * aLogAnlz, rpdSenderInfo * aSenderInfo )
{
    setLastSN( aSenderInfo, aLogAnlz->mTID, aLogAnlz->mSN );

    IDE_TEST( rpnComm::sendXaStartReq( mHBTResource,
                                       mProtocolContext,
                                       mExitFlag,
                                       &(aLogAnlz->mXID),
                                       aLogAnlz->getSendTransID(),
                                       aLogAnlz->mSN,
                                       RPU_REPLICATION_SENDER_SEND_TIMEOUT )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpnMessenger::sendXaPrepareReq( rpdLogAnalyzer * aLogAnlz, rpdSenderInfo * aSenderInfo )
{
    setLastSN( aSenderInfo, aLogAnlz->mTID, aLogAnlz->mSN );

    IDE_TEST( rpnComm::sendXaPrepareReq( mHBTResource,
                                         mProtocolContext,
                                         mExitFlag,
                                         &(aLogAnlz->mXID),
                                         aLogAnlz->getSendTransID(),
                                         aLogAnlz->mSN,
                                         RPU_REPLICATION_SENDER_SEND_TIMEOUT )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpnMessenger::sendXaPrepare( rpdLogAnalyzer * aLogAnlz, rpdSenderInfo * aSenderInfo )
{
    setLastSN( aSenderInfo, aLogAnlz->mTID, aLogAnlz->mSN );

    IDE_TEST( rpnComm::sendXaPrepare( mHBTResource,
                                      mProtocolContext,
                                      mExitFlag,
                                      &(aLogAnlz->mXID),
                                      aLogAnlz->getSendTransID(),
                                      aLogAnlz->mSN,
                                      RPU_REPLICATION_SENDER_SEND_TIMEOUT )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpnMessenger::sendXaEnd( rpdLogAnalyzer * aLogAnlz, rpdSenderInfo * aSenderInfo )
{
    setLastSN( aSenderInfo, aLogAnlz->mTID, aLogAnlz->mSN );

    IDE_TEST( rpnComm::sendXaEnd( mHBTResource,
                                  mProtocolContext,
                                  mExitFlag,
                                  aLogAnlz->getSendTransID(),
                                  aLogAnlz->mSN,
                                  RPU_REPLICATION_SENDER_SEND_TIMEOUT )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 *
 */
IDE_RC rpnMessenger::sendNA( rpdLogAnalyzer * aLogAnlz )
{
    IDE_SET( ideSetErrorCode( rpERR_ABORT_SENDER_INVALID_XLOG_TYPE,
                              aLogAnlz->mType ) );
    IDE_ERRLOG( IDE_RP_0 );

    return IDE_FAILURE;
}

/*
 *
 */
IDE_RC rpnMessenger::sendXLog( rpdTransTbl      * aTransTbl,
                               rpdSenderInfo    * aSenderInfo,
                               rpdLogAnalyzer   * aAnlz,
                               smSN               aFlushSN,
                               idBool             aNeedLock,
                               idBool             aNeedFlush )
{
    idBool  sIsLocked = ID_FALSE;

    if ( aNeedLock == ID_TRUE )
    {
        IDE_DASSERT( mNeedSocketMutex == ID_TRUE );

        IDE_ASSERT( mSocketMutex.lock( NULL ) == IDE_SUCCESS );
        sIsLocked = ID_TRUE;
    }
    else
    {
        /* do nothing */
    }

    switch ( aAnlz->mType )
    {
        case RP_X_COMMIT:
            IDE_TEST( sendTrCommit( aTransTbl, 
                                    aSenderInfo,
                                    aAnlz, 
                                    aFlushSN )
                      != IDE_SUCCESS );
            break;
            
        case RP_X_ABORT:
            IDE_TEST( sendTrAbort( aTransTbl, 
                                   aSenderInfo,
                                   aAnlz, 
                                   aFlushSN )
                      != IDE_SUCCESS );
            break;
            
        case RP_X_INSERT:
            IDE_TEST( sendInsert( aTransTbl, 
                                  aSenderInfo,
                                  aAnlz, 
                                  aFlushSN )
                      != IDE_SUCCESS );
            break;
            
        case RP_X_UPDATE:
            IDE_TEST( sendUpdate( aTransTbl, 
                                  aSenderInfo,
                                  aAnlz, 
                                  aFlushSN )
                      != IDE_SUCCESS );
            break;
            
        case RP_X_DELETE:
            IDE_TEST( sendDelete( aTransTbl, 
                                  aSenderInfo, 
                                  aAnlz, 
                                  aFlushSN )
                      != IDE_SUCCESS );
            break;
            
        case RP_X_SP_SET:
            IDE_TEST( sendSPSet( aTransTbl, 
                                 aSenderInfo,
                                 aAnlz, 
                                 aFlushSN )
                      != IDE_SUCCESS );
            break;
            
        case RP_X_SP_ABORT:
            IDE_TEST( sendSPAbort( aTransTbl, 
                                   aSenderInfo,
                                   aAnlz, 
                                   aFlushSN )
                      != IDE_SUCCESS );
            break;
            
        case RP_X_LOB_CURSOR_OPEN:
            IDE_TEST( sendLobCursorOpen( aTransTbl, 
                                         aSenderInfo,
                                         aAnlz, 
                                         aFlushSN )
                      != IDE_SUCCESS );
            break;
            
        case RP_X_LOB_CURSOR_CLOSE:
            IDE_TEST( sendLobCursorClose( aAnlz, 
                                          aSenderInfo,
                                          aFlushSN ) 
                      != IDE_SUCCESS );
            break;
            
        case RP_X_LOB_PREPARE4WRITE:
            IDE_TEST( sendLobPrepareWrite( aAnlz, 
                                           aSenderInfo,
                                           aFlushSN ) 
                      != IDE_SUCCESS );
            break;
            
        case RP_X_LOB_PARTIAL_WRITE:
            IDE_TEST( sendLobPartialWrite( aAnlz, 
                                           aSenderInfo,
                                           aFlushSN ) 
                      != IDE_SUCCESS );
            break;
            
        case RP_X_LOB_FINISH2WRITE:
            IDE_TEST( sendLobFinishWrite( aAnlz, 
                                          aSenderInfo,
                                          aFlushSN ) 
                      != IDE_SUCCESS );
            break;

        case RP_X_LOB_TRIM:
            IDE_TEST( sendLobTrim( aAnlz, 
                                   aSenderInfo,
                                   aFlushSN ) 
                      != IDE_SUCCESS );
            break;

        case RP_X_XA_START_REQ:
            IDE_TEST( sendXaStartReq( aAnlz, 
                                      aSenderInfo )
                      != IDE_SUCCESS );
            break;

        case RP_X_XA_PREPARE_REQ:
            IDE_TEST( sendXaPrepareReq( aAnlz, 
                                        aSenderInfo )
                      != IDE_SUCCESS );
            break;

        case RP_X_XA_PREPARE:
            IDE_TEST( sendXaPrepare( aAnlz, 
                                     aSenderInfo )
                      != IDE_SUCCESS );
            break;

        case RP_X_XA_END:
            IDE_TEST( sendXaEnd( aAnlz, 
                                 aSenderInfo )
                      != IDE_SUCCESS );
            break;

        default:
            IDE_TEST( sendNA( aAnlz ) != IDE_SUCCESS );
            break;
    }
    if ( aNeedFlush == ID_TRUE )
    {
        IDE_TEST( sendCmBlock() != IDE_SUCCESS );
    }
    else
    {
        /* do nothing */
    }

    if ( aNeedLock == ID_TRUE )
    {
        sIsLocked = ID_FALSE;
        IDE_ASSERT( mSocketMutex.unlock() == IDE_SUCCESS );
    }
    else
    {
        /* do nothing */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();

    if ( sIsLocked == ID_TRUE )
    {
        sIsLocked = ID_FALSE;
        IDE_ASSERT( mSocketMutex.unlock() == IDE_SUCCESS );
    }
    else
    {
        /* do nothing */
    }

    IDE_POP();

    return IDE_FAILURE;
}

/*
 *
 */
IDE_RC rpnMessenger::sendXLogSPSet( smTID   aTID,
                                    smSN    aSN,
                                    smSN    aFlushSN,
                                    UInt    aSpNameLen,
                                    SChar * aSpName,
                                    idBool  aNeedLock )
{
    idBool  sIsLocked = ID_FALSE;

    if ( aNeedLock == ID_TRUE )
    {
        IDE_DASSERT( mNeedSocketMutex == ID_TRUE );

        IDE_ASSERT( mSocketMutex.lock( NULL ) == IDE_SUCCESS );
        sIsLocked = ID_TRUE;
    }
    else
    {
        /* do nothing */
    }

    IDE_TEST( rpnComm::sendSPSet( mHBTResource,
                                  mProtocolContext,
                                  mExitFlag,
                                  aTID,
                                  aSN,
                                  aFlushSN,
                                  aSpNameLen,
                                  aSpName,
                                  RPU_REPLICATION_SENDER_SEND_TIMEOUT )
              != IDE_SUCCESS );

    if ( aNeedLock == ID_TRUE )
    {
        sIsLocked = ID_FALSE;
        IDE_ASSERT( mSocketMutex.unlock() == IDE_SUCCESS );
    }
    else
    {
        /* do nothing */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();

    if ( sIsLocked == ID_TRUE )
    {
        sIsLocked = ID_FALSE;
        IDE_ASSERT( mSocketMutex.unlock() == IDE_SUCCESS );
    }
    else
    {
        /* do nothing */
    }

    IDE_POP();

    return IDE_FAILURE;
}

/*
 *
 */
IDE_RC rpnMessenger::sendXLogKeepAlive( smSN    aSN,
                                        smSN    aFlushSN,
                                        smSN    aRestartSN,
                                        idBool  aNeedLock )
{
    idBool  sIsLocked = ID_FALSE;

    if ( aNeedLock == ID_TRUE )
    {
        IDE_DASSERT( mNeedSocketMutex == ID_TRUE );

        IDE_ASSERT( mSocketMutex.lock( NULL ) == IDE_SUCCESS );
        sIsLocked = ID_TRUE;
    }
    else
    {
        /* do nothing */
    }
    
    IDE_TEST( rpnComm::sendKeepAlive( mHBTResource,
                                      mProtocolContext,
                                      mExitFlag,
                                      RP_TID_NULL,
                                      aSN,
                                      aFlushSN,
                                      aRestartSN,
                                      RPU_REPLICATION_SENDER_SEND_TIMEOUT )
              != IDE_SUCCESS );

    if ( aNeedLock == ID_TRUE )
    {
        sIsLocked = ID_FALSE;
        IDE_ASSERT( mSocketMutex.unlock() == IDE_SUCCESS );
    }
    else
    {
        /* do nothing */
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    IDE_PUSH();

    if ( sIsLocked == ID_TRUE )
    {
        sIsLocked = ID_FALSE;
        IDE_ASSERT( mSocketMutex.unlock() == IDE_SUCCESS );
    }
    else
    {
        /* do nothing */
    }

    IDE_POP();
    
    return IDE_FAILURE;
}

/*
 *
 */
IDE_RC rpnMessenger::sendXLogHandshake( smTID   aTID,
                                        smSN    aSN,
                                        smSN    aFlushSN,
                                        idBool  aNeedLock )
{
    idBool  sIsLocked = ID_FALSE;

    if ( aNeedLock == ID_TRUE )
    {
        IDE_DASSERT( mNeedSocketMutex == ID_TRUE );

        IDE_ASSERT( mSocketMutex.lock( NULL ) == IDE_SUCCESS );
        sIsLocked = ID_TRUE;
    }
    else
    {
        /* do nothing */
    }
    
    IDE_TEST( rpnComm::sendHandshake( mHBTResource,
                                      mProtocolContext,
                                      mExitFlag,
                                      aTID,
                                      aSN,
                                      aFlushSN,
                                      RPU_REPLICATION_SENDER_SEND_TIMEOUT )
              != IDE_SUCCESS );

    if ( aNeedLock == ID_TRUE )
    {
        sIsLocked = ID_FALSE;
        IDE_ASSERT( mSocketMutex.unlock() == IDE_SUCCESS );
    }
    else
    {
        /* do nothing */
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();

    if ( sIsLocked == ID_TRUE )
    {
        sIsLocked = ID_FALSE;
        IDE_ASSERT( mSocketMutex.unlock() == IDE_SUCCESS );
    }
    else
    {
        /* do nothing */
    }

    IDE_POP();

    return IDE_FAILURE;
}

/*
 *
 */
IDE_RC rpnMessenger::sendXLogSyncPKBegin( void )
{
    IDE_TEST( rpnComm::sendSyncPKBegin( mHBTResource,
                                        mProtocolContext,
                                        mExitFlag,
                                        RP_TID_NULL,
                                        SM_SN_NULL,
                                        SM_SN_NULL,
                                        RPU_REPLICATION_SENDER_SEND_TIMEOUT )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 *
 */
IDE_RC rpnMessenger::sendXLogSyncPK( ULong        aTableOID,
                                     UInt         aPKColCnt,
                                     smiValue   * aPKCols,
                                     rpValueLen * aPKLen )
{
    IDE_TEST( rpnComm::sendSyncPK( mHBTResource,
                                   mProtocolContext,
                                   mExitFlag,
                                   RP_TID_NULL,
                                   SM_SN_NULL,
                                   SM_SN_NULL,
                                   aTableOID,
                                   aPKColCnt,
                                   aPKCols,
                                   aPKLen,
                                   RPU_REPLICATION_SENDER_SEND_TIMEOUT )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 *
 */
IDE_RC rpnMessenger::sendXLogSyncPKEnd( void )
{
    IDE_TEST( rpnComm::sendSyncPKEnd( mHBTResource,
                                      mProtocolContext,
                                      mExitFlag,
                                      RP_TID_NULL,
                                      SM_SN_NULL,
                                      SM_SN_NULL,
                                      RPU_REPLICATION_SENDER_SEND_TIMEOUT )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 *
 */
IDE_RC rpnMessenger::sendXLogFailbackEnd( void )
{
    IDE_TEST( rpnComm::sendFailbackEnd( mHBTResource,
                                        mProtocolContext,
                                        mExitFlag,
                                        RP_TID_NULL,
                                        SM_SN_NULL,
                                        SM_SN_NULL,
                                        RPU_REPLICATION_SENDER_SEND_TIMEOUT )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 *
 */
IDE_RC rpnMessenger::sendXLogDelete( ULong        aTableOID,
                                     UInt         aPKColCnt,
                                     smiValue   * aPKCols,
                                     rpValueLen * aPKLen )
{
    IDE_TEST( rpnComm::sendDelete( mHBTResource,
                                   mProtocolContext,
                                   mExitFlag,
                                   RP_TID_NULL,
                                   0,
                                   SM_SN_NULL,
                                   SMI_STATEMENT_DEPTH_NULL,
                                   aTableOID,
                                   aPKColCnt,
                                   aPKCols,
                                   aPKLen,
                                   0,
                                   NULL,
                                   NULL,
                                   NULL,
                                   NULL,
                                   NULL,
                                   RPU_REPLICATION_SENDER_SEND_TIMEOUT )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 *
 */
IDE_RC rpnMessenger::sendXLogTrCommit( void )
{
    IDE_TEST( rpnComm::sendTrCommit( mHBTResource,
                                     mProtocolContext,
                                     mExitFlag,
                                     RP_TID_NULL,
                                     SM_SN_NULL,
                                     SM_SN_NULL,
                                     ID_TRUE,
                                     RPU_REPLICATION_SENDER_SEND_TIMEOUT )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 *
 */
IDE_RC rpnMessenger::sendXLogTrAbort( void )
{
    IDE_TEST( rpnComm::sendTrAbort( mHBTResource,
                                    mProtocolContext,
                                    mExitFlag,
                                    RP_TID_NULL,
                                    SM_SN_NULL,
                                    SM_SN_NULL,
                                    RPU_REPLICATION_SENDER_SEND_TIMEOUT )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 *
 */
void rpnMessenger::setSnMapMgr( rprSNMapMgr * aSNMapMgr )
{
    mSNMapMgr = aSNMapMgr;
}


/*
 *
 */
void rpnMessenger::setHBTResource( void * aHBTResource )
{
    mHBTResource = aHBTResource;
}

/*
 * @brief 불필요한 Transaction Begin을 방지
 */
IDE_RC rpnMessenger::checkAndSendSvpList( rpdTransTbl       * aTransTbl,
                                          rpdLogAnalyzer    * aLogAnlz,
                                          smSN                aFlushSN )
{
    rpdSavepointEntry * sSavepointEntry = NULL;
    UInt                sNameLen;

    if ( aTransTbl->isSvpListSent( aLogAnlz->mTID ) != ID_TRUE )
    {
        if ( aLogAnlz->mTID == aLogAnlz->getSendTransID() )
        {
            aTransTbl->getFirstSvpEntry( aLogAnlz->mTID, &sSavepointEntry );
            while ( sSavepointEntry != NULL )
            {
                sNameLen = idlOS::strlen( sSavepointEntry->mName );
                IDE_TEST( sendXLogSPSet( aLogAnlz->getSendTransID(),
                                         sSavepointEntry->mSN,
                                         aFlushSN,
                                         sNameLen,
                                         sSavepointEntry->mName,
                                         ID_FALSE ) /* aNeedLock */
                          != IDE_SUCCESS);

                aTransTbl->removeSvpEntry( sSavepointEntry );
                aTransTbl->getFirstSvpEntry( aLogAnlz->mTID, &sSavepointEntry );
            }
        }
        else
        {
            /* do nothing */
        }

        aTransTbl->setSvpListSent( aLogAnlz->mTID );
    }
    else
    {
        /* do nothing */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpnMessenger::sendSyncTableNumber( UInt aSyncTableNumber )
{
    IDE_TEST_RAISE( rpnComm::sendSyncTableNumber( mHBTResource,
                                                  mProtocolContext,
                                                  mExitFlag,
                                                  aSyncTableNumber,
                                                  RPU_REPLICATION_SENDER_SEND_TIMEOUT )
                    != IDE_SUCCESS, ERR_SEND_SYNC_TABLE_NUMBER );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_SEND_SYNC_TABLE_NUMBER )
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_SEND_SYNC_TABLES_NUMBER ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpnMessenger::sendSyncTableInfo( rpdMetaItem * aItem )
{
    IDE_TEST_RAISE( aItem == NULL, ERR_NOT_EXIST_SYNC_TABLE );

    IDE_TEST_RAISE( rpnComm::sendMetaReplTbl( mHBTResource,
                                              mProtocolContext,
                                              mExitFlag,
                                              aItem,
                                              RPU_REPLICATION_SENDER_SEND_TIMEOUT )
                    != IDE_SUCCESS, ERR_SEND_SYNC_TABLES );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_EXIST_SYNC_TABLE )
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_NOT_EXIST_SYNC_TABLE ) );
    }
    IDE_EXCEPTION( ERR_SEND_SYNC_TABLES )
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_SEND_SYNC_TABLES ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpnMessenger::sendSyncStart( void )
{
    IDE_TEST( rpnComm::sendSyncStart( mHBTResource,
                                      mProtocolContext,
                                      mExitFlag,
                                      RPU_REPLICATION_SENDER_SEND_TIMEOUT ) 
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpnMessenger::sendSyncEnd( void )
{
    IDE_TEST( rpnComm::sendSyncEnd( mHBTResource,
                                    mProtocolContext,
                                    mExitFlag,
                                    RPU_REPLICATION_SENDER_SEND_TIMEOUT ) 
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpnMessenger::sendFlush( void )
{
    IDE_TEST( rpnComm::sendFlush( mHBTResource,
                                  mProtocolContext,
                                  mExitFlag,
                                  SM_NULL_TID,
                                  SM_SN_NULL,
                                  SM_SN_NULL,
                                  0,
                                  RPU_REPLICATION_SENDER_SEND_TIMEOUT)
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* PROJ-2453 */
IDE_RC rpnMessenger::sendAckOnDML( void )
{
    IDE_TEST( rpnComm::sendAckOnDML( mHBTResource,
                                     mProtocolContext,
                                     mExitFlag,
                                     RPU_REPLICATION_SENDER_SEND_TIMEOUT )
              != IDE_SUCCESS );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpnMessenger::sendCmBlock( void )
{
     IDE_TEST( rpnComm::sendCmBlock( mHBTResource,
                                     mProtocolContext,
                                     mExitFlag,
                                     ID_TRUE,
                                     RPU_REPLICATION_SENDER_SEND_TIMEOUT )
               != IDE_SUCCESS );
     return IDE_SUCCESS;

     IDE_EXCEPTION_END;

     return IDE_FAILURE;
}

IDE_RC rpnMessenger::sendDDLASyncStart( UInt aType )
{
    IDE_TEST( rpnComm::sendDDLASyncStart( mHBTResource,
                                          mProtocolContext,
                                          mExitFlag,
                                          aType,
                                          RPU_REPLICATION_SENDER_SEND_TIMEOUT )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpnMessenger::recvDDLASyncStartAck( UInt * aType )
{
    IDE_TEST( rpnComm::recvDDLASyncStartAck( mProtocolContext,
                                             mExitFlag,
                                             aType,
                                             RPU_REPLICATION_RECEIVE_TIMEOUT )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpnMessenger::sendDDLASyncExecute( UInt    aType,
                                          SChar * aUserName,
                                          UInt    aDDLEnableLevel,
                                          UInt    aTargetCount,
                                          SChar * aTargetTableName,
                                          SChar * aTargetPartNames,
                                          smSN    aDDLCommitSN,
                                          SChar * aDDLStmt )
{
    rpdVersion sVersion;

    sVersion.mVersion = RP_CURRENT_VERSION;

    IDE_TEST( rpnComm::sendDDLASyncExecute( mHBTResource,
                                            mProtocolContext,
                                            mExitFlag,
                                            aType,
                                            aUserName,
                                            aDDLEnableLevel,
                                            aTargetCount,
                                            aTargetTableName,
                                            aTargetPartNames,
                                            aDDLCommitSN,
                                            &sVersion,
                                            aDDLStmt,
                                            RPU_REPLICATION_SENDER_SEND_TIMEOUT )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

     return IDE_FAILURE;
}

IDE_RC rpnMessenger::recvDDLASyncExecuteAck( UInt  * aType,
                                             UInt  * aIsSuccess, 
                                             UInt  * aErrCode,
                                             SChar * aErrMsg )
{
    IDE_TEST( rpnComm::recvDDLASyncExecuteAck( mProtocolContext,
                                               mExitFlag,
                                               aType,
                                               aIsSuccess,
                                               aErrCode,
                                               aErrMsg,
                                               RPU_REPLICATION_RECEIVE_TIMEOUT )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

void rpnMessenger::setLastSN( rpdSenderInfo     * aSenderInfo,
                              smTID               aTransID,
                              smSN                aSN )
{
    if ( aSenderInfo != NULL )
    {
        aSenderInfo->setLastSN( aTransID, aSN );
    }
    else
    {
        /* do nothing */
    }
}

ULong rpnMessenger::getSendDataSize( void )
{
    return mProtocolContext->mSendDataSize;
}

ULong rpnMessenger::getSendDataCount( void )
{
    return mProtocolContext->mSendDataCount;
}

void rpnMessenger::checkSupportOnRemoteVersion( rpdVersion     aRemoteVersion,
                                                rpMsgReturn  * aRC,
                                                SChar        * aRCMsg,
                                                SInt           aRCMsgLen )
{
    switch ( mCheckVersion )
    {
        case RP_MESSENGER_CONDITIONAL:
            if( aRemoteVersion.mVersion < RP_CONDITIONAL_START_VERSION )
            {
                *aRC = RP_MSG_PROTOCOL_DIFF;
                (void)idlOS::snprintf( aRCMsg, 
                                       aRCMsgLen, 
                                       "This is not supported on remote server. It should be higher than 7.4.7 [Remote server replication protocol version:%"ID_INT32_FMT".%"ID_INT32_FMT".%"ID_INT32_FMT"]",
                                       RP_GET_MAJOR_VERSION( aRemoteVersion.mVersion ),
                                       RP_GET_MINOR_VERSION( aRemoteVersion.mVersion ),
                                       RP_GET_FIX_VERSION( aRemoteVersion.mVersion ) ); 
            }
            break;

        case RP_MESSENGER_NONE:
        default:
            break;
    }
}

IDE_RC rpnMessenger::communicateConditionInfo( rpdConditionItemInfo  * aSendConditionInfo,
                                               SInt                    aItemCount,
                                               rpdConditionActInfo   * aRecvConditionActInfo )
{
    IDE_DASSERT( aSendConditionInfo != NULL );
    IDE_DASSERT( aItemCount > 0 );

    IDE_TEST_RAISE( rpnComm::sendConditionInfo( mHBTResource, 
                                                mProtocolContext,
                                                mExitFlag,
                                                aSendConditionInfo,
                                                aItemCount,
                                                RPU_REPLICATION_SENDER_SEND_TIMEOUT ) 
                    != IDE_SUCCESS, ERR_SEND_ITEM_CONDITION_INFO );
    
   
    IDE_TEST_RAISE( rpnComm::recvConditionInfoResult( mProtocolContext,
                                                      mExitFlag,
                                                      aRecvConditionActInfo,
                                                      RPU_REPLICATION_RECEIVE_TIMEOUT ) 
                    != IDE_SUCCESS, ERR_RECV_ITEM_CONDITION_INFO );
  
    return IDE_SUCCESS;


    IDE_EXCEPTION( ERR_SEND_ITEM_CONDITION_INFO )
    {
        IDE_ERRLOG( IDE_RP_0 );
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RP_WRITE_SOCKET ) );
        IDE_ERRLOG( IDE_RP_0 );
        IDE_SET( ideSetErrorCode( rpERR_ABORT_HANDSHAKE_DISCONNECT,
                                  "rpnMessenger::communicateConditionInfo" ) );
    }
    IDE_EXCEPTION( ERR_RECV_ITEM_CONDITION_INFO )
    {
        IDE_ERRLOG( IDE_RP_0 );
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RP_READ_SOCKET ) );
        IDE_ERRLOG( IDE_RP_0 );
        IDE_SET( ideSetErrorCode( rpERR_ABORT_HANDSHAKE_DISCONNECT,
                                  "rpnMessenger::communicateConditionInfo" ) );
    }

    IDE_EXCEPTION_END;
 
    return IDE_FAILURE;

}

IDE_RC rpnMessenger::sendTruncate( ULong aTableOID )
{
    IDE_TEST_RAISE( rpnComm::sendTruncate( mHBTResource, 
                                           mProtocolContext,
                                           mExitFlag,
                                           aTableOID,
                                           RPU_REPLICATION_SENDER_SEND_TIMEOUT ) 
                    != IDE_SUCCESS, ERR_SEND_CONDITION_TRUNCATE );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_SEND_CONDITION_TRUNCATE )
    {
        IDE_ERRLOG( IDE_RP_0 );
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RP_WRITE_SOCKET ) );
        IDE_ERRLOG( IDE_RP_0 );
        IDE_SET( ideSetErrorCode( rpERR_ABORT_HANDSHAKE_DISCONNECT,
                                  "rpnMessenger::sendTruncate" ) );
    }
    IDE_EXCEPTION_END;
 
    return IDE_FAILURE;
}
