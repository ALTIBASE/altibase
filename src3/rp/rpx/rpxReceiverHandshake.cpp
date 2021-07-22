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
 * $Id: rpxReceiverHandshake.cpp 90266 2021-03-19 05:23:09Z returns $
 **********************************************************************/

#include <idl.h>
#include <ide.h>

#include <smi.h>
#include <smiTrans.h>
#include <smiStatement.h>

#include <rpDef.h>
#include <rpuProperty.h>
#include <rpcManager.h>
#include <rpxReceiver.h>
#include <mtc.h>

/* HDB V6's replication version number */
#define REPLICATION_MAJOR_VERSION_HDB_V6       (6)
#define REPLICATION_MINOR_VERSION_HDB_V6       (1)

IDE_RC rpxReceiver::checkProtocol( cmiProtocolContext *aProtocolContext,
                                   idBool             *aExitFlag,
                                   rpRecvStartStatus  *aStatus,
                                   rpdVersion         *aVersion )
{
    SChar          sInformation[128];
    SChar          sBuffer[RP_ACK_MSG_LEN];

    IDE_TEST_RAISE( rpnComm::recvVersion( aProtocolContext,
                                          aExitFlag,
                                          aVersion,
                                          RPU_REPLICATION_CONNECT_RECEIVE_TIMEOUT )
                    != IDE_SUCCESS, ERR_READ );

    if ( RP_GET_MAJOR_VERSION( aVersion->mVersion ) ==
         REPLICATION_MAJOR_VERSION )
    {
        IDE_TEST_RAISE( RP_GET_MINOR_VERSION( aVersion->mVersion )
                        != REPLICATION_MINOR_VERSION, ERR_PROTOCOL );
       
        IDU_FIT_POINT_RAISE( "1.TASK-2004@rpxReceiver::checkProtocol", 
                              ERR_PROTOCOL );
    }
    else
    {
        IDE_TEST_RAISE( RP_GET_MAJOR_VERSION( aVersion->mVersion )
                        != REPLICATION_MAJOR_VERSION_HDB_V6, ERR_PROTOCOL );

        IDE_TEST_RAISE( RP_GET_MINOR_VERSION( aVersion->mVersion )
                        != REPLICATION_MINOR_VERSION_HDB_V6, ERR_PROTOCOL );
    }

    IDU_FIT_POINT( "rpxReceiver::checkProtocol::rpnComm::sendHandshakeAck" );

    idlOS::memset(sBuffer, 0, RP_ACK_MSG_LEN);
    IDE_TEST_RAISE( rpnComm::sendHandshakeAck( aProtocolContext,
                                               aExitFlag,
                                               RP_MSG_OK,
                                               RP_FAILBACK_NONE,
                                               SM_SN_NULL,
                                               sBuffer,
                                               RPU_REPLICATION_CONNECT_RECEIVE_TIMEOUT )
                    != IDE_SUCCESS, ERR_WRITE);

    *aStatus = RP_START_RECV_OK;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_READ );
    {
        // To fix BUG-4726
        // HBT 검출 사실에 대해서 좀더 검증 및 고민 필요함... ㅡㅜ
        if(ideGetErrorCode() == cmERR_ABORT_CONNECTION_CLOSED)
        {
            *aStatus = RP_START_RECV_HBT_OCCURRED;
            IDE_WARNING(IDE_RP_1, RP_TRC_RH_ERR_CHK_SOCKET_RECV);
        }
        else
        {
            // cmERR_ABORT_RECV_ERROR
            *aStatus = RP_START_RECV_NETWORK_ERROR;
            IDE_ERRLOG( IDE_RP_0 );
            IDE_SET( ideSetErrorCode( rpERR_ABORT_RP_READ_SOCKET ) );
        }
    }
    IDE_EXCEPTION( ERR_WRITE );
    {
        *aStatus = RP_START_RECV_NETWORK_ERROR;
        IDE_ERRLOG( IDE_RP_0 );
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RP_WRITE_SOCKET ) );
    }
    IDE_EXCEPTION(ERR_PROTOCOL);
    {
        switch( REPLICATION_ENDIAN_64BIT )
        {
            case 1:
                idlOS::snprintf(sInformation, 128, "Big Endian, 64 Bit" );
                break;
            case 2:
                idlOS::snprintf(sInformation, 128, "Big Endian, 32 Bit" );
                break;
            case 3:
                idlOS::snprintf(sInformation, 128, "Little Endian, 64 Bit" );
                break;
            case 4:
                idlOS::snprintf(sInformation, 128, "Little Endian, 32 Bit" );
                break;
            default :
                idlOS::snprintf(sInformation, 128, "%s", "" );
                break;
        }

        idlOS::memset( sBuffer, 0, ID_SIZEOF(sBuffer) );
        idlOS::snprintf( sBuffer, RP_ACK_MSG_LEN,
                         "Peer Replication Protocol Version is "
                         "[%"ID_INT32_FMT".%"ID_INT32_FMT".%"ID_INT32_FMT"] "
                         "%s",
                         REPLICATION_MAJOR_VERSION,
                         REPLICATION_MINOR_VERSION,
                         REPLICATION_FIX_VERSION,
                         sInformation );

        (void)rpnComm::sendHandshakeAck( aProtocolContext,
                                         aExitFlag,
                                         RP_MSG_PROTOCOL_DIFF,
                                         RP_FAILBACK_NONE,
                                         SM_SN_NULL,
                                         sBuffer,
                                         RPU_REPLICATION_CONNECT_RECEIVE_TIMEOUT );

        *aStatus = RP_START_RECV_INVALID_VERSION;
        IDE_SET( ideSetErrorCode( rpERR_IGNORE_RP_PROTOCOL_DIFF ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : Network를 종료하지 않고 Handshake를 수행한다.
 *
 ***********************************************************************/
IDE_RC rpxReceiver::handshakeWithoutReconnect( rpdXLog *aXLog )
{
    rpRecvStartStatus sStatus;
    rpdMeta           sMeta;
    rpdVersion        sVersion = { 0 };
    idBool            sDummyFlag;
    rpXLogAck         sAck;
    smiTrans          sTrans;
    smiStatement    * sRootStatement = NULL;
    UInt              sStage = 0;
    idBool            sIsTxBegin = ID_FALSE;

    idBool sIsGotNetworkResources = ID_FALSE;

    if( mMeta.getReplMode() == RP_CONSISTENT_MODE )
    {
        if ( isGottenNetworkResoucesFromXLogTransfer() == ID_FALSE )
        {
            getNetworkResourcesFromXLogTransfer();
            sIsGotNetworkResources = ID_TRUE;
        }
    }

    sMeta.initialize();

    IDE_TEST_RAISE( checkProtocol( mProtocolContext, 
                                   &mExitFlag,
                                   &sStatus,
                                   &sVersion ) 
                    != IDE_SUCCESS, ERR_NETWORK );

    IDE_TEST_RAISE( sMeta.recvMeta( mProtocolContext,
                                    &mExitFlag,
                                    sVersion,
                                    RPU_REPLICATION_RECEIVE_TIMEOUT,
                                    &sDummyFlag )
                    != IDE_SUCCESS, ERR_NETWORK );

    IDE_TEST( sTrans.initialize() != IDE_SUCCESS );
    sStage = 1;

    IDE_TEST( sTrans.begin( &sRootStatement,
                            &mStatistics,
                            SMI_TRANSACTION_NORMAL |
                            SMI_TRANSACTION_REPL_REPLICATED |
                            SMI_COMMIT_WRITE_NOWAIT |
                            (UInt)RPU_ISOLATION_LEVEL,
                            RP_UNUSED_RECEIVER_INDEX )
              != IDE_SUCCESS );
    sIsTxBegin = ID_TRUE;
    sStage = 2;

    IDE_TEST( processMetaAndSendHandshakeAck( &sTrans,
                                              &sMeta ) 
              != IDE_SUCCESS );

    sStage = 1;
    IDE_TEST( sTrans.commit() != IDE_SUCCESS )
    sIsTxBegin = ID_FALSE;

    sStage = 0;
    IDE_TEST( sTrans.destroy(NULL) != IDE_SUCCESS );

    sMeta.finalize();

    if( mMeta.getReplMode() == RP_CONSISTENT_MODE )
    {
        buildDummyXLogAckForConsistent( aXLog, &sAck);
        IDE_TEST( sendAckWithTID( &sAck) != IDE_SUCCESS );
    }

    if( sIsGotNetworkResources == ID_TRUE )
    {
        setNetworkResourcesToXLogTransfer();
        sIsGotNetworkResources = ID_FALSE;

        wakeupXLogTansfer();
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_NETWORK);
    {
        mNetworkError = ID_TRUE;
    }
    IDE_EXCEPTION_END;

    IDE_PUSH();

    switch ( sStage )
    {
        case 2:
            IDE_ASSERT( sTrans.rollback() == IDE_SUCCESS );
            sIsTxBegin = ID_FALSE;
        case 1:
            if ( sIsTxBegin == ID_TRUE )
            {
                IDE_ASSERT( sTrans.rollback() == IDE_SUCCESS );
                sIsTxBegin = ID_FALSE;
            }
            (void)sTrans.destroy( NULL );
        default:
            break;
    }

    sMeta.finalize();

    if( sIsGotNetworkResources == ID_TRUE )
    {
        setNetworkResourcesToXLogTransfer();

        wakeupXLogTansfer();
    }

    IDE_POP();

    return IDE_FAILURE;
}
