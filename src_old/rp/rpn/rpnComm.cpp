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
 * $Id: rpnComm.cpp 16602 2006-06-09 01:39:30Z lswhh $
 **********************************************************************/

#include <idl.h>
#include <ide.h>

#include <rpDef.h>
#include <rpuProperty.h>
#include <rpcHBT.h>
#include <rpnComm.h>

PDL_Time_Value rpnComm::mTV1Sec;
UInt rpnComm::rpnXLogLengthForEachType[CMP_OP_RP_MAX_VER1 + 1] = {0, };

//===================================================================
//
// Name:          Constructor
//
// Return Value:
//
// Argument:
//
// Called By:
//
// Description:
//
//===================================================================

rpnComm::rpnComm()
{
}

//===================================================================
//
// Name:          initialize
//
// Return Value:  IDE_SUCCESS/IDE_FAILURE
//
// Argument:
//
// Called By:     rpnComm::initialize()
//
// Description:
//
//===================================================================

IDE_RC rpnComm::initialize()
{
    mTV1Sec.initialize(1, 0);

    return IDE_SUCCESS;
}

//===================================================================
//
// Name:          destroy
//
// Return Value: 
//
// Argument:
//
// Called By:
//
// Description:
//
//===================================================================

void rpnComm::destroy()
{
}


//===================================================================
//
// Name:          shutdown
//
// Return Value:
//
// Argument:
//
// Called By:
//
// Description:
//
//===================================================================

void rpnComm::shutdown()
{
}

void rpnComm::finalize()
{
}

/* PROJ-2725
 * 각 xlog type별 op code를 제외한 payload length 를 기록한다.
 * 단, paylaod 가  가변적인 경우 -1로 설정한다.
 */
void rpnComm::setLengthForEachXLogType()
{

    rpnXLogLengthForEachType[CMP_OP_RP_Version] = 8;
    rpnXLogLengthForEachType[CMP_OP_RP_MetaRepl] = -1;
    rpnXLogLengthForEachType[CMP_OP_RP_MetaReplTbl] = -1;
    rpnXLogLengthForEachType[CMP_OP_RP_MetaReplCol] = -1;
    rpnXLogLengthForEachType[CMP_OP_RP_MetaReplIdx] = -1;
    rpnXLogLengthForEachType[CMP_OP_RP_MetaReplIdxCol] = 8;
    rpnXLogLengthForEachType[CMP_OP_RP_HandshakeAck] = -1;
    rpnXLogLengthForEachType[CMP_OP_RP_TrBegin] = 24;
    rpnXLogLengthForEachType[CMP_OP_RP_TrCommit] = 24;
    rpnXLogLengthForEachType[CMP_OP_RP_TrAbort] = 24;
    rpnXLogLengthForEachType[CMP_OP_RP_SPSet] = -1;
    rpnXLogLengthForEachType[CMP_OP_RP_SPAbort] = -1;
    rpnXLogLengthForEachType[CMP_OP_RP_StmtBegin] = -1;
    rpnXLogLengthForEachType[CMP_OP_RP_StmtEnd] = -1;
    rpnXLogLengthForEachType[CMP_OP_RP_CursorOpen] = -1;
    rpnXLogLengthForEachType[CMP_OP_RP_CursorClose] = -1;
    rpnXLogLengthForEachType[CMP_OP_RP_Insert] = 40;
    rpnXLogLengthForEachType[CMP_OP_RP_Update] = 44;
    rpnXLogLengthForEachType[CMP_OP_RP_Delete] = 44;
    rpnXLogLengthForEachType[CMP_OP_RP_UIntID] = 4;
    rpnXLogLengthForEachType[CMP_OP_RP_Value] = -1;
    rpnXLogLengthForEachType[CMP_OP_RP_Stop] = 32;
    rpnXLogLengthForEachType[CMP_OP_RP_KeepAlive] = 32;
    rpnXLogLengthForEachType[CMP_OP_RP_Flush] = 28;
    rpnXLogLengthForEachType[CMP_OP_RP_FlushAck] = -1;
    rpnXLogLengthForEachType[CMP_OP_RP_Ack] = 52;
    rpnXLogLengthForEachType[CMP_OP_RP_LobCursorOpen] = 48;
    rpnXLogLengthForEachType[CMP_OP_RP_LobCursorClose] = 32;
    rpnXLogLengthForEachType[CMP_OP_RP_LobPrepare4Write] = 44;
    rpnXLogLengthForEachType[CMP_OP_RP_LobPartialWrite] = 40;
    rpnXLogLengthForEachType[CMP_OP_RP_LobFinish2Write] = 32;
    rpnXLogLengthForEachType[CMP_OP_RP_TxAck] = 4;
    rpnXLogLengthForEachType[CMP_OP_RP_RequestAck] = -1;
    rpnXLogLengthForEachType[CMP_OP_RP_Handshake] = 24;
    rpnXLogLengthForEachType[CMP_OP_RP_SyncPKBegin] = 24;
    rpnXLogLengthForEachType[CMP_OP_RP_SyncPK] = 36;
    rpnXLogLengthForEachType[CMP_OP_RP_SyncPKEnd] = 24;
    rpnXLogLengthForEachType[CMP_OP_RP_FailbackEnd] = 24;
    rpnXLogLengthForEachType[CMP_OP_RP_SyncTableNumber] = 4;
    rpnXLogLengthForEachType[CMP_OP_RP_SyncStart] = 4;
    rpnXLogLengthForEachType[CMP_OP_RP_SyncEnd] = 4;
    rpnXLogLengthForEachType[CMP_OP_RP_LobTrim] = 36;
    rpnXLogLengthForEachType[CMP_OP_RP_MetaReplCheck] = -1;
    rpnXLogLengthForEachType[CMP_OP_RP_MetaDictTableCount] = 4;
    rpnXLogLengthForEachType[CMP_OP_RP_AckOnDML] = 4;
    rpnXLogLengthForEachType[CMP_OP_RP_AckEager] = 52 +4; //RP_Ack + 4
    rpnXLogLengthForEachType[CMP_OP_RP_DDLSyncInfo] = -1;
    rpnXLogLengthForEachType[CMP_OP_RP_DDLSyncMsg] = 4;
    rpnXLogLengthForEachType[CMP_OP_RP_DDLSyncMsgAck] = -1;
    rpnXLogLengthForEachType[CMP_OP_RP_DDLSyncCancel] = -1;
    rpnXLogLengthForEachType[CMP_OP_RP_DDLReplicateHandshake] = 4;
    rpnXLogLengthForEachType[CMP_OP_RP_DDLReplicateQueryStatement] = -1;
    rpnXLogLengthForEachType[CMP_OP_RP_DDLReplicateExecute] = -1;
    rpnXLogLengthForEachType[CMP_OP_RP_DDLReplicateAck] = -1;
    rpnXLogLengthForEachType[CMP_OP_RP_MetaPartitionCount] = 4;
    rpnXLogLengthForEachType[CMP_OP_RP_MetaInitialize] = 4;
    rpnXLogLengthForEachType[CMP_OP_RP_TemporarySyncInfo] = -1;
    rpnXLogLengthForEachType[CMP_OP_RP_TemporarySyncItem] = -1;
    rpnXLogLengthForEachType[CMP_OP_RP_TemporarySyncHandshakeAck] = -1;
    rpnXLogLengthForEachType[CMP_OP_RP_MetaReplTblCondition] = -1;
    rpnXLogLengthForEachType[CMP_OP_RP_MetaReplTblConditionResult] = -1;
    rpnXLogLengthForEachType[CMP_OP_RP_Truncate] = 12;
    rpnXLogLengthForEachType[CMP_OP_RP_TruncateAck] = 12;
    rpnXLogLengthForEachType[CMP_OP_RP_XA_START_REQ] = 4 + 4 + 8 + 8 + 8 + 8 + 4 + ID_XIDDATASIZE, /* PROJ-2747 */
    rpnXLogLengthForEachType[CMP_OP_RP_XA_PREPARE_REQ] =  4 + 4 + 8 + 8 + 8 + 8 + 4 + ID_XIDDATASIZE,
    rpnXLogLengthForEachType[CMP_OP_RP_XA_PREPARE] = 4 + 4 + 8 + 8 + 8 + 8 + 4 + ID_XIDDATASIZE,
    rpnXLogLengthForEachType[CMP_OP_RP_XA_COMMIT] = 24 + 8;
    rpnXLogLengthForEachType[CMP_OP_RP_XA_END] = 4 + 4 + 8,
    rpnXLogLengthForEachType[CMP_OP_RP_MAX_VER1] = -1;

    /* 새로운 protocol이 추가 되었을 경우, 개발자가 직접 length를 계산해서 기입해줘야 한다. 개발자의 실수를 줄이기 위하여 DASSERT를 통해 체크한다. */
    UInt i = 0;
    for( i = 0; i < CMP_OP_RP_MAX_VER1; i++)
    {
        IDE_ASSERT( rpnXLogLengthForEachType[i] != 0 );
    }

    return;
}

//===================================================================
//
// Name:          isConnected
//
// Return Value:
//   - ID_TRUE  : 세션이 Alive 상태임
//   - ID_FALSE : 세션이 Alive 상태가 아님
//
// Argument:
//   [IN]  aLink   : 판단할 Link
//
// Called By:
//
// Description:
//   link가 현재 연결 상태인지를 판단. 여기서 Alive란 link가
//   원격으로 Connect 되어 있고, 언제든 통신이 가능한 상태를 의미함
//   따라서, Connect 이전이나, Disconnect 상태나 모두 Not Alive가 됨
//
//===================================================================
idBool rpnComm::isConnected( cmiLink * aLink )
{
    idBool    sIsDisconnect;

    IDE_TEST(aLink == NULL);

    IDE_TEST(cmiCheckLink(aLink, &sIsDisconnect) != IDE_SUCCESS);

    IDE_TEST(sIsDisconnect == ID_TRUE);

    return ID_TRUE;

    IDE_EXCEPTION_END;

    return ID_FALSE;
}

//===================================================================
//
// Name:          send/recv
//
// Return Value:  IDE_SUCCESS/IDE_FAILURE
//
// Argument:
//
// Called By:
//
// Description:
//
//===================================================================
IDE_RC rpnComm::sendVersion( void               * aHBTResource,
                             cmiProtocolContext * aProtocolContext,
                             idBool             * aExitFlag,
                             rpdVersion         * aReplVersion,
                             UInt                 aTimeoutSec )
{
    cmpPacketType sPacketType;

    sPacketType = cmiGetPacketType( aProtocolContext );

    switch ( sPacketType )
    {
        case CMP_PACKET_TYPE_A5:
            IDE_TEST( sendVersionA5( aHBTResource,
                                     aProtocolContext,
                                     aExitFlag,
                                     aReplVersion,
                                     aTimeoutSec )
                      != IDE_SUCCESS );
            break;

        case CMP_PACKET_TYPE_A7:
        case CMP_PACKET_TYPE_UNKNOWN: /* 읽기 전 */
            IDE_TEST( sendVersionA7( aHBTResource,
                                     aProtocolContext,
                                     aExitFlag,
                                     aReplVersion,
                                     aTimeoutSec )
                      != IDE_SUCCESS );
            break;

        default:
            IDE_RAISE( UNKNOWN_PACKET_TYPE );
            break;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( UNKNOWN_PACKET_TYPE )
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_INVALID_PACKET_TYPE ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 * 이중화 프로토콜 버전 정보를 제일 먼저 주고 받는다.
 *
 * 그러나 HDB V6의 경우에는 이중화 프로토콜 이전에 CM의 Base
 * Protocol의 handshake 과정을 거친다. readCmBlock()에서 사용하는
 * cmiRecv() 함수에서 이 과정을 처리 후, cmiReadProtocol()을 사용할 수
 * 있게 준비가 된다.
 *
 * 이 후에는 패킷 타입에 따라서 cmiRecv(), cmiReadProtocol()을 사용해야
 * 한다.
 */
IDE_RC rpnComm::recvVersion( cmiProtocolContext * aProtocolContext,
                             idBool             * aExitFlag,
                             rpdVersion         * aReplVersion,
                             ULong                aTimeoutSec )
{
    cmpPacketType sPacketType;
    
    sPacketType = cmiGetPacketType( aProtocolContext );

    switch ( sPacketType )
    {
        case CMP_PACKET_TYPE_A5:
            /* nothing to do */
            break;
            
        case CMP_PACKET_TYPE_A7:
        case CMP_PACKET_TYPE_UNKNOWN: /* 읽기 전 */
            IDE_TEST( readCmBlock( NULL,
                                   aProtocolContext,
                                   aExitFlag,
                                   NULL /* TimeoutFlag */,
                                   aTimeoutSec )
                      != IDE_SUCCESS );
            break;
    }

    sPacketType = cmiGetPacketType( aProtocolContext );
    switch ( sPacketType )
    {
        case CMP_PACKET_TYPE_A5:
            IDE_TEST( recvVersionA5( aProtocolContext,
                                     aExitFlag,
                                     aReplVersion,
                                     aTimeoutSec )
                      != IDE_SUCCESS );
            break;
            
        case CMP_PACKET_TYPE_A7:
            IDE_TEST( recvVersionA7( aProtocolContext,
                                     aExitFlag,
                                     aReplVersion )
                      != IDE_SUCCESS );
            break;
            
        case CMP_PACKET_TYPE_UNKNOWN:
            IDE_RAISE( UNKNOWN_PACKET_TYPE );
            break;
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( UNKNOWN_PACKET_TYPE )
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_INVALID_PACKET_TYPE ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpnComm::sendMetaDictTableCount( void               * aHBTResource,
                                        cmiProtocolContext * aProtocolContext,
                                        idBool             * aExitFlag,
                                        SInt               * aDictTableCount,
                                        UInt                 aTimeoutSec )
{
    cmpPacketType sPacketType;

    sPacketType = cmiGetPacketType( aProtocolContext );

    switch ( sPacketType )
    {
        case CMP_PACKET_TYPE_A5:
            IDE_RAISE( UNKNOWN_PACKET_TYPE );
            break;

        case CMP_PACKET_TYPE_A7:
            IDE_TEST( sendMetaDictTableCountA7( aHBTResource,
                                                aProtocolContext,
                                                aExitFlag,
                                                aDictTableCount,
                                                aTimeoutSec )
                      != IDE_SUCCESS );
            break;
        case CMP_PACKET_TYPE_UNKNOWN:
            IDE_RAISE( UNKNOWN_PACKET_TYPE );
            break;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( UNKNOWN_PACKET_TYPE )
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_INVALID_PACKET_TYPE ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpnComm::recvMetaDictTableCount( cmiProtocolContext * aProtocolContext,
                                        idBool             * aExitFlag,
                                        SInt               * aDictTableCount,
                                        ULong                aTimeoutSec )
{
    cmpPacketType sPacketType;

    sPacketType = cmiGetPacketType( aProtocolContext );

    switch ( sPacketType )
    {
        case CMP_PACKET_TYPE_A5:
            IDE_RAISE( UNKNOWN_PACKET_TYPE );
            break;

        case CMP_PACKET_TYPE_A7:
            IDE_TEST( recvMetaDictTableCountA7( aProtocolContext,
                                                aExitFlag,
                                                aDictTableCount,
                                                aTimeoutSec )
                      != IDE_SUCCESS );
            break;
        case CMP_PACKET_TYPE_UNKNOWN:
            IDE_RAISE( UNKNOWN_PACKET_TYPE );
            break;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( UNKNOWN_PACKET_TYPE )
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_INVALID_PACKET_TYPE ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC rpnComm::sendMetaRepl( void               * aHBTResource,
                              cmiProtocolContext * aProtocolContext,
                              idBool             * aExitFlag,
                              rpdReplications    * aRepl,
                              UInt                 aTimeoutSec )
{
    cmpPacketType sPacketType;

    sPacketType = cmiGetPacketType( aProtocolContext );

    switch ( sPacketType )
    {
        case CMP_PACKET_TYPE_A5:
            IDE_TEST( sendMetaReplA5( aHBTResource,
                                      aProtocolContext,
                                      aExitFlag,
                                      aRepl,
                                      aTimeoutSec )
                      != IDE_SUCCESS );
            break;

        case CMP_PACKET_TYPE_A7:
            IDE_TEST( sendMetaReplA7( aHBTResource,
                                      aProtocolContext,
                                      aExitFlag,
                                      aRepl,
                                      aTimeoutSec )
                      != IDE_SUCCESS );
            break;

        case CMP_PACKET_TYPE_UNKNOWN:
        default:
            IDE_RAISE( UNKNOWN_PACKET_TYPE );
            break;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( UNKNOWN_PACKET_TYPE )
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_INVALID_PACKET_TYPE ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpnComm::recvMetaRepl( cmiProtocolContext * aProtocolContext,
                              idBool             * aExitFlag,
                              rpdReplications    * aRepl,
                              ULong                aTimeoutSec )
{
    cmpPacketType sPacketType;
    
    sPacketType = cmiGetPacketType( aProtocolContext );

    switch ( sPacketType )
    {
        case CMP_PACKET_TYPE_A5:
            IDE_TEST( recvMetaReplA5( aProtocolContext,
                                      aExitFlag,
                                      aRepl,
                                      aTimeoutSec )
                      != IDE_SUCCESS );
            break;
            
        case CMP_PACKET_TYPE_A7:
            IDE_TEST( recvMetaReplA7( aProtocolContext,
                                      aExitFlag,
                                      aRepl,
                                      aTimeoutSec )
                      != IDE_SUCCESS );
            break;
            
        case CMP_PACKET_TYPE_UNKNOWN:

            IDE_RAISE( UNKNOWN_PACKET_TYPE );
            break;
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( UNKNOWN_PACKET_TYPE )
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_INVALID_PACKET_TYPE ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpnComm::sendTempSyncMetaRepl( void               * aHBTResource,
                                      cmiProtocolContext * aProtocolContext,
                                      idBool             * aExitFlag,
                                      rpdReplications    * aRepl,
                                      UInt                 aTimeoutSec )
{
    cmpPacketType sPacketType;

    sPacketType = cmiGetPacketType( aProtocolContext );

    switch ( sPacketType )
    {
        case CMP_PACKET_TYPE_A7:
            IDE_TEST( sendTempSyncMetaReplA7( aHBTResource,
                                              aProtocolContext,
                                              aExitFlag,
                                              aRepl,
                                              aTimeoutSec )
                      != IDE_SUCCESS );
            break;

        case CMP_PACKET_TYPE_A5:
        case CMP_PACKET_TYPE_UNKNOWN:
        default:
            IDE_RAISE( UNKNOWN_PACKET_TYPE );
            break;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( UNKNOWN_PACKET_TYPE )
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_INVALID_PACKET_TYPE ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpnComm::recvTempSyncMetaRepl( cmiProtocolContext * aProtocolContext,
                                      idBool             * aExitFlag,
                                      rpdReplications    * aRepl,
                                      ULong                aTimeoutSec )
{
    cmpPacketType sPacketType;
    
    sPacketType = cmiGetPacketType( aProtocolContext );

    switch ( sPacketType )
    {
        case CMP_PACKET_TYPE_A7:
            IDE_TEST( recvTempSyncMetaReplA7( aProtocolContext,
                                              aExitFlag,
                                              aRepl,
                                              aTimeoutSec )
                      != IDE_SUCCESS );
            break;
            
        case CMP_PACKET_TYPE_A5:
        case CMP_PACKET_TYPE_UNKNOWN:

            IDE_RAISE( UNKNOWN_PACKET_TYPE );
            break;
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( UNKNOWN_PACKET_TYPE )
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_INVALID_PACKET_TYPE ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpnComm::sendTempSyncReplItem( void               * aHBTResource,
                                      cmiProtocolContext * aProtocolContext,
                                      idBool             * aExitFlag,
                                      rpdReplSyncItem    * aTempSyncItem,
                                      UInt                 aTimeoutSec )
{
    cmpPacketType sPacketType;

    sPacketType = cmiGetPacketType( aProtocolContext );

    switch ( sPacketType )
    {
        case CMP_PACKET_TYPE_A7:
            IDE_TEST( sendTempSyncReplItemA7( aHBTResource,
                                              aProtocolContext,
                                              aExitFlag,
                                              aTempSyncItem,
                                              aTimeoutSec )
                      != IDE_SUCCESS );
            break;

        case CMP_PACKET_TYPE_A5:
        case CMP_PACKET_TYPE_UNKNOWN:
        default:
            IDE_RAISE( UNKNOWN_PACKET_TYPE );
            break;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( UNKNOWN_PACKET_TYPE )
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_INVALID_PACKET_TYPE ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpnComm::recvTempSyncReplItem( cmiProtocolContext * aProtocolContext,
                                      idBool             * aExitFlag,
                                      rpdReplSyncItem    * aTempSyncItem,
                                      ULong                aTimeoutSec )
{
    cmpPacketType sPacketType;
    
    sPacketType = cmiGetPacketType( aProtocolContext );

    switch ( sPacketType )
    {
        case CMP_PACKET_TYPE_A7:
            IDE_TEST( recvTempSyncReplItemA7( aProtocolContext,
                                              aExitFlag,
                                              aTempSyncItem,
                                              aTimeoutSec )
                      != IDE_SUCCESS );
            break;
            
        case CMP_PACKET_TYPE_A5:
        case CMP_PACKET_TYPE_UNKNOWN:

            IDE_RAISE( UNKNOWN_PACKET_TYPE );
            break;
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( UNKNOWN_PACKET_TYPE )
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_INVALID_PACKET_TYPE ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpnComm::sendMetaReplTbl( void               * aHBTResource,
                                 cmiProtocolContext * aProtocolContext,
                                 idBool             * aExitFlag,
                                 rpdMetaItem        * aItem,
                                 UInt                 aTimeoutSec )
{
    cmpPacketType sPacketType;

    sPacketType = cmiGetPacketType( aProtocolContext );

    switch ( sPacketType )
    {
        case CMP_PACKET_TYPE_A5:
            IDE_TEST( sendMetaReplTblA5( aHBTResource,
                                         aProtocolContext,
                                         aExitFlag,
                                         aItem,
                                         aTimeoutSec )
                      != IDE_SUCCESS );
            break;

        case CMP_PACKET_TYPE_A7:
            IDE_TEST( sendMetaReplTblA7( aHBTResource,
                                         aProtocolContext,
                                         aExitFlag,
                                         aItem,
                                         aTimeoutSec )
                      != IDE_SUCCESS );
            break;

        case CMP_PACKET_TYPE_UNKNOWN:
        default:
            IDE_RAISE( UNKNOWN_PACKET_TYPE );
            break;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( UNKNOWN_PACKET_TYPE )
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_INVALID_PACKET_TYPE ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpnComm::recvMetaReplTbl( cmiProtocolContext * aProtocolContext,
                                 idBool             * aExitFlag,
                                 rpdMetaItem        * aItem,
                                 ULong                aTimeoutSec )
{
    cmpPacketType sPacketType;
    
    sPacketType = cmiGetPacketType( aProtocolContext );

    switch ( sPacketType )
    {
        case CMP_PACKET_TYPE_A5:
            IDE_TEST( recvMetaReplTblA5( aProtocolContext,
                                         aExitFlag,
                                         aItem,
                                         aTimeoutSec )
                      != IDE_SUCCESS );
            break;
            
        case CMP_PACKET_TYPE_A7:
            IDE_TEST( recvMetaReplTblA7( aProtocolContext,
                                         aExitFlag,
                                         aItem,
                                         aTimeoutSec )
                      != IDE_SUCCESS );
            break;
            
        case CMP_PACKET_TYPE_UNKNOWN:
            IDE_RAISE( UNKNOWN_PACKET_TYPE );
            break;
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( UNKNOWN_PACKET_TYPE )
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_INVALID_PACKET_TYPE ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpnComm::sendMetaReplCol( void               * aHBTResource,
                                 cmiProtocolContext * aProtocolContext,
                                 idBool             * aExitFlag,
                                 rpdColumn          * aColumn,
                                 rpdVersion           aRemoteVersion ,
                                 UInt                 aTimeoutSec )
{
    cmpPacketType sPacketType;

    sPacketType = cmiGetPacketType( aProtocolContext );

    switch ( sPacketType )
    {
        case CMP_PACKET_TYPE_A5:
            IDE_TEST( sendMetaReplColA5( aHBTResource,
                                         aProtocolContext,
                                         aExitFlag,
                                         aColumn,
                                         aTimeoutSec )
                      != IDE_SUCCESS );
            break;

        case CMP_PACKET_TYPE_A7:
            IDE_TEST( sendMetaReplColA7( aHBTResource,
                                         aProtocolContext,
                                         aExitFlag,
                                         aColumn,
                                         aRemoteVersion,
                                         aTimeoutSec )
                      != IDE_SUCCESS );
            break;

        case CMP_PACKET_TYPE_UNKNOWN:
        default:
            IDE_RAISE( UNKNOWN_PACKET_TYPE );
            break;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( UNKNOWN_PACKET_TYPE )
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_INVALID_PACKET_TYPE ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpnComm::recvMetaReplCol( cmiProtocolContext * aProtocolContext,
                                 idBool             * aExitFlag,
                                 rpdColumn          * aColumn,
                                 rpdVersion           aRemoteVersion,
                                 ULong                aTimeoutSec )
{
    cmpPacketType sPacketType;
    
    sPacketType = cmiGetPacketType( aProtocolContext );

    switch ( sPacketType )
    {
        case CMP_PACKET_TYPE_A5:
            IDE_TEST( recvMetaReplColA5( aProtocolContext,
                                         aExitFlag,
                                         aColumn,
                                         aTimeoutSec )
                      != IDE_SUCCESS );
            break;
            
        case CMP_PACKET_TYPE_A7:
            IDE_TEST( recvMetaReplColA7( aProtocolContext,
                                         aExitFlag,
                                         aColumn,
                                         aRemoteVersion,
                                         aTimeoutSec )
                      != IDE_SUCCESS );
            break;
            
        case CMP_PACKET_TYPE_UNKNOWN:
            IDE_RAISE( UNKNOWN_PACKET_TYPE );
            break;
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( UNKNOWN_PACKET_TYPE )
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_INVALID_PACKET_TYPE ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpnComm::sendMetaReplIdx( void               * aHBTResource,
                                 cmiProtocolContext * aProtocolContext,
                                 idBool             * aExitFlag,
                                 qcmIndex           * aIndex,
                                 UInt                 aTimeoutSec )
{
    cmpPacketType sPacketType;

    sPacketType = cmiGetPacketType( aProtocolContext );

    switch ( sPacketType )
    {
        case CMP_PACKET_TYPE_A5:
            IDE_TEST( sendMetaReplIdxA5( aHBTResource,
                                         aProtocolContext,
                                         aExitFlag,
                                         aIndex,
                                         aTimeoutSec )
                      != IDE_SUCCESS );
            break;

        case CMP_PACKET_TYPE_A7:
            IDE_TEST( sendMetaReplIdxA7( aHBTResource,
                                         aProtocolContext,
                                         aExitFlag,
                                         aIndex,
                                         aTimeoutSec )
                      != IDE_SUCCESS );
            break;

        case CMP_PACKET_TYPE_UNKNOWN:
        default:
            IDE_RAISE( UNKNOWN_PACKET_TYPE );
            break;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( UNKNOWN_PACKET_TYPE )
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_INVALID_PACKET_TYPE ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpnComm::recvMetaReplIdx( cmiProtocolContext * aProtocolContext,
                                 idBool             * aExitFlag,
                                 qcmIndex           * aIndex,
                                 ULong                aTimeoutSec )
{
    cmpPacketType sPacketType;
    
    sPacketType = cmiGetPacketType( aProtocolContext );

    switch ( sPacketType )
    {
        case CMP_PACKET_TYPE_A5:
            IDE_TEST( recvMetaReplIdxA5( aProtocolContext,
                                         aExitFlag,
                                         aIndex,
                                         aTimeoutSec )
                      != IDE_SUCCESS );
            break;
            
        case CMP_PACKET_TYPE_A7:
            IDE_TEST( recvMetaReplIdxA7( aProtocolContext,
                                         aExitFlag,
                                         aIndex,
                                         aTimeoutSec )
                      != IDE_SUCCESS );
            break;
            
        case CMP_PACKET_TYPE_UNKNOWN:
            IDE_RAISE( UNKNOWN_PACKET_TYPE );
            break;
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( UNKNOWN_PACKET_TYPE )
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_INVALID_PACKET_TYPE ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
IDE_RC rpnComm::sendMetaReplIdxCol( void                * aHBTResource,
                                    cmiProtocolContext  * aProtocolContext,
                                    idBool              * aExitFlag,
                                    UInt                  aColumnID,
                                    UInt                  aKeyColumnFlag,
                                    UInt                  aTimeoutSec )
{
    cmpPacketType sPacketType;

    sPacketType = cmiGetPacketType( aProtocolContext );

    switch ( sPacketType )
    {
        case CMP_PACKET_TYPE_A5:
            IDE_TEST( sendMetaReplIdxColA5( aHBTResource,
                                            aProtocolContext,
                                            aExitFlag,
                                            aColumnID,
                                            aKeyColumnFlag,
                                            aTimeoutSec )
                      != IDE_SUCCESS );
            break;

        case CMP_PACKET_TYPE_A7:
            IDE_TEST( sendMetaReplIdxColA7( aHBTResource,
                                            aProtocolContext,
                                            aExitFlag,
                                            aColumnID,
                                            aKeyColumnFlag,
                                            aTimeoutSec )
                      != IDE_SUCCESS );
            break;

        case CMP_PACKET_TYPE_UNKNOWN:
        default:
            IDE_RAISE( UNKNOWN_PACKET_TYPE );
            break;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( UNKNOWN_PACKET_TYPE )
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_INVALID_PACKET_TYPE ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpnComm::recvMetaReplIdxCol( cmiProtocolContext * aProtocolContext,
                                    idBool             * aExitFlag,
                                    UInt               * aColumnID,
                                    UInt               * aKeyColumnFlag,
                                    ULong                aTimeoutSec )
{
    cmpPacketType sPacketType;
    
    sPacketType = cmiGetPacketType( aProtocolContext );


    switch ( sPacketType )
    {
        case CMP_PACKET_TYPE_A5:
            IDE_TEST( recvMetaReplIdxColA5( aProtocolContext,
                                            aExitFlag,
                                            aColumnID,
                                            aKeyColumnFlag,
                                            aTimeoutSec )
                      != IDE_SUCCESS );
            break;
            
        case CMP_PACKET_TYPE_A7:
            IDE_TEST( recvMetaReplIdxColA7( aProtocolContext,
                                            aExitFlag,
                                            aColumnID,
                                            aKeyColumnFlag,
                                            aTimeoutSec )
                      != IDE_SUCCESS );
            break;
            
        case CMP_PACKET_TYPE_UNKNOWN:
            IDE_RAISE( UNKNOWN_PACKET_TYPE );
            break;
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( UNKNOWN_PACKET_TYPE )
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_INVALID_PACKET_TYPE ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/********************************************************************
 * Description  :
 *  send check Meta Data
 *
 * Argument :
 *  aHBTResource        [IN] : HBT Status
 *  aProtocolContext    [IN] : cm protocol context structure
 *  aCheck              [IN] : aCheck will be sent 
 * ********************************************************************/
IDE_RC rpnComm::sendMetaReplCheck( void                 * aHBTResource,
                                   cmiProtocolContext   * aProtocolContext,
                                   idBool               * aExitFlag,
                                   qcmCheck             * aCheck,
                                   UInt                   aTimeoutSec ) 
{
    cmpPacketType sPacketType;

    sPacketType = cmiGetPacketType( aProtocolContext );

    switch ( sPacketType )
    {
        case CMP_PACKET_TYPE_A5:
            IDE_TEST( sendMetaReplCheckA5( aHBTResource,
                                           aProtocolContext,
                                           aExitFlag,
                                           aCheck,
                                           aTimeoutSec )
                      != IDE_SUCCESS );
            break;

        case CMP_PACKET_TYPE_A7:
            IDE_TEST( sendMetaReplCheckA7( aHBTResource,
                                           aProtocolContext,
                                           aExitFlag,
                                           aCheck,
                                           aTimeoutSec )
                      != IDE_SUCCESS );
            break;

        case CMP_PACKET_TYPE_UNKNOWN:
        default:
            IDE_RAISE( UNKNOWN_PACKET_TYPE );
            break;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( UNKNOWN_PACKET_TYPE )
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_INVALID_PACKET_TYPE ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/********************************************************************
 * Description  :
 *  recv Check Meta Data
 *
 * Argument :
 *  aProtocolContext    [IN]  : cm protocol context structure
 *  aCheck              [OUT] : aCheck will be recieved
 *  aTimeoutSec         [IN]  : when date is read from cm layer, it will set
 *                              timeout using this argument
 * ********************************************************************/
IDE_RC rpnComm::recvMetaReplCheck( cmiProtocolContext   * aProtocolContext,
                                   idBool               * aExitFlag,
                                   qcmCheck             * aCheck,
                                   const ULong            aTimeoutSec )
{
    cmpPacketType sPacketType;
    
    sPacketType = cmiGetPacketType( aProtocolContext );

    switch ( sPacketType )
    {
        case CMP_PACKET_TYPE_A7:
            IDE_TEST( recvMetaReplCheckA7( aProtocolContext,
                                           aExitFlag,
                                           aCheck,
                                           aTimeoutSec )
                      != IDE_SUCCESS );
            break;
            
        case CMP_PACKET_TYPE_UNKNOWN:
        case CMP_PACKET_TYPE_A5: 
            IDE_RAISE( UNKNOWN_PACKET_TYPE );
            break;

        default:
            IDE_RAISE( UNKNOWN_PACKET_TYPE );
            break;

    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( UNKNOWN_PACKET_TYPE )
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_INVALID_PACKET_TYPE ) );
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/********************************************************************
 * Description  :
 *  recv Check Meta Data and data sava qcmCheck Struct     
 *
 * Argument :
 *  aProtocolContext    [IN]  : cm protocol context structure
 *  aCheck              [OUT] : aCheck will be recieved
 *  aTimeoutSec         [IN]  : when date is read from cm layer, it will set
 *                              timeout using this argument
 * ********************************************************************/
IDE_RC rpnComm::sendHandshakeAck( cmiProtocolContext    * aProtocolContext,
                                  idBool                * aExitFlag,
                                  UInt                    aResult,
                                  SInt                    aFailbackStatus,
                                  ULong                   aXSN,
                                  const SChar           * aMsg,
                                  UInt                    aTimeoutSec )
{
    cmpPacketType sPacketType;

    sPacketType = cmiGetPacketType( aProtocolContext );

    switch ( sPacketType )
    {
        case CMP_PACKET_TYPE_A5:
            IDE_TEST( sendHandshakeAckA5( aProtocolContext,
                                          aExitFlag,
                                          aResult,
                                          aFailbackStatus,
                                          aXSN,
                                          aMsg,
                                          aTimeoutSec )
                      != IDE_SUCCESS );
            break;
            
        case CMP_PACKET_TYPE_A7:
            IDE_TEST( sendHandshakeAckA7( aProtocolContext,
                                          aExitFlag,
                                          aResult,
                                          aFailbackStatus,
                                          aXSN,
                                          aMsg,
                                          aTimeoutSec )
                      != IDE_SUCCESS );
            break;
            
        case CMP_PACKET_TYPE_UNKNOWN:
            IDE_RAISE( UNKNOWN_PACKET_TYPE );
            break;
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( UNKNOWN_PACKET_TYPE )
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_INVALID_PACKET_TYPE ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;            
}

IDE_RC rpnComm::recvHandshakeAck( void               * aHBTResource,
                                  cmiProtocolContext * aProtocolContext,
                                  idBool             * aExitFlag,
                                  UInt               * aResult,
                                  SInt               * aFailbackStatus,
                                  ULong              * aXSN,
                                  SChar              * aMsg,
                                  UInt               * aMsgLen,
                                  ULong                aTimeOut )
{
    cmpPacketType sPacketType;

    sPacketType = cmiGetPacketType( aProtocolContext );

    switch ( sPacketType )
    {
        case CMP_PACKET_TYPE_A5:
            IDE_TEST( recvHandshakeAckA5( aProtocolContext,
                                          aExitFlag,
                                          aResult,
                                          aFailbackStatus,
                                          aXSN,
                                          aMsg,
                                          aMsgLen,
                                          aTimeOut )
                      != IDE_SUCCESS );
            break;

        case CMP_PACKET_TYPE_A7:
        case CMP_PACKET_TYPE_UNKNOWN:
            IDE_TEST( recvHandshakeAckA7( aHBTResource,
                                          aProtocolContext,
                                          aExitFlag,
                                          aResult,
                                          aFailbackStatus,
                                          aXSN,
                                          aMsg,
                                          aMsgLen,
                                          aTimeOut )
                      != IDE_SUCCESS );
            break;

        default:
            IDE_RAISE( UNKNOWN_PACKET_TYPE );
            break;
    }

    /* A7 은 HandshakeAck 를 받은 뒤 상대의 Packet Type 을 알 수 있다. */
    sPacketType = cmiGetPacketType( aProtocolContext );
    IDE_TEST_RAISE( sPacketType == CMP_PACKET_TYPE_UNKNOWN, UNKNOWN_PACKET_TYPE );

    return IDE_SUCCESS;

    IDE_EXCEPTION( UNKNOWN_PACKET_TYPE )
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_INVALID_PACKET_TYPE ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpnComm::sendTempSyncHandshakeAck( cmiProtocolContext    * aProtocolContext,
                                          idBool                * aExitFlag,
                                          UInt                    aResult,
                                          SChar                 * aMsg,
                                          UInt                    aTimeoutSec )
{
    cmpPacketType sPacketType;

    sPacketType = cmiGetPacketType( aProtocolContext );

    switch ( sPacketType )
    {
        case CMP_PACKET_TYPE_A7:
            IDE_TEST( sendTempSyncHandshakeAckA7( aProtocolContext,
                                                  aExitFlag,
                                                  aResult,
                                                  aMsg,
                                                  aTimeoutSec )
                      != IDE_SUCCESS );
            break;
            
        case CMP_PACKET_TYPE_A5:
        case CMP_PACKET_TYPE_UNKNOWN:
            IDE_RAISE( UNKNOWN_PACKET_TYPE );
            break;
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( UNKNOWN_PACKET_TYPE )
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_INVALID_PACKET_TYPE ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;            
}

IDE_RC rpnComm::recvTempSyncHandshakeAck( cmiProtocolContext * aProtocolContext,
                                          idBool             * aExitFlag,
                                          UInt               * aResult,
                                          SChar              * aMsg,
                                          UInt               * aMsgLen,
                                          ULong                aTimeOut )
{
    cmpPacketType sPacketType;

    sPacketType = cmiGetPacketType( aProtocolContext );

    switch ( sPacketType )
    {
        case CMP_PACKET_TYPE_A5:
            IDE_RAISE( UNKNOWN_PACKET_TYPE );
            break;

        case CMP_PACKET_TYPE_A7:
        case CMP_PACKET_TYPE_UNKNOWN:
            IDE_TEST( recvTempSyncHandshakeAckA7( aProtocolContext,
                                                  aExitFlag,
                                                  aResult,
                                                  aMsg,
                                                  aMsgLen,
                                                  aTimeOut )
                      != IDE_SUCCESS );
            break;

        default:
            IDE_RAISE( UNKNOWN_PACKET_TYPE );
            break;
    }

    /* A7 은 HandshakeAck 를 받은 뒤 상대의 Packet Type 을 알 수 있다. */
    sPacketType = cmiGetPacketType( aProtocolContext );
    IDE_TEST_RAISE( sPacketType == CMP_PACKET_TYPE_UNKNOWN, UNKNOWN_PACKET_TYPE );

    return IDE_SUCCESS;

    IDE_EXCEPTION( UNKNOWN_PACKET_TYPE )
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_INVALID_PACKET_TYPE ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpnComm::recvXLog( iduMemAllocator    * aAllocator,
                          rpxReceiverReadContext aReadContext,
                          idBool             * aExitFlag,
                          rpdMeta            * aMeta,     // BUG-20506
                          rpdXLog            * aXLog,
                          ULong                aTimeOutSec )

{
    cmpPacketType sPacketType;
    
    if( aReadContext.mCurrentMode == RPX_RECEIVER_READ_NETWORK )
    {
        sPacketType = cmiGetPacketType( aReadContext.mCMContext );

        switch ( sPacketType )
        {
            case CMP_PACKET_TYPE_A5:
                IDE_TEST( recvXLogA5( aAllocator,
                                      aReadContext.mCMContext,
                                      aExitFlag,
                                      aMeta,
                                      aXLog,
                                      aTimeOutSec )
                          != IDE_SUCCESS );
                break;

            case CMP_PACKET_TYPE_A7:
                IDE_TEST( recvXLogA7( aAllocator,
                                      aReadContext,
                                      aExitFlag,
                                      aMeta,
                                      aXLog,
                                      aTimeOutSec )
                          != IDE_SUCCESS );
                break;

            case CMP_PACKET_TYPE_UNKNOWN:
                IDE_RAISE( UNKNOWN_PACKET_TYPE );
                break;
        }
    }
    else if ( aReadContext.mCurrentMode == RPX_RECEIVER_READ_XLOGFILE )
    {
        IDE_TEST( recvXLogA7( aAllocator,
                              aReadContext,
                              aExitFlag,
                              aMeta,
                              aXLog,
                              aTimeOutSec )
                  != IDE_SUCCESS );
    }
    else
    {
        IDE_RAISE( ERR_INTERNAL );
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( UNKNOWN_PACKET_TYPE )
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_INVALID_PACKET_TYPE ) );
    }
    IDE_EXCEPTION( ERR_INTERNAL )
    {
    	IDE_SET( ideSetErrorCode( rpERR_ABORT_RPN_UNABLE_READ_CONTEXT) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpnComm::sendTrBegin( void                * aHBTResource,
                             cmiProtocolContext  * aProtocolContext,
                             idBool              * aExitFlag,
                             smTID                 aTID,
                             smSN                  aSN,
                             smSN                  aSyncSN,
                             UInt                  aTimeoutSec )
{
    cmpPacketType sPacketType;

    sPacketType = cmiGetPacketType( aProtocolContext );

    switch ( sPacketType )
    {
        case CMP_PACKET_TYPE_A5:
            IDE_TEST( sendTrBeginA5( aHBTResource,
                                     aProtocolContext,
                                     aExitFlag,
                                     aTID,
                                     aSN,
                                     aSyncSN,
                                     aTimeoutSec )
                      != IDE_SUCCESS );
            break;

        case CMP_PACKET_TYPE_A7:
            IDE_TEST( sendTrBeginA7( aHBTResource,
                                     aProtocolContext,
                                     aExitFlag,
                                     aTID,
                                     aSN,
                                     aSyncSN,
                                     aTimeoutSec )
                      != IDE_SUCCESS );
            break;

        case CMP_PACKET_TYPE_UNKNOWN:
        default:
            IDE_RAISE( UNKNOWN_PACKET_TYPE );
            break;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( UNKNOWN_PACKET_TYPE )
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_INVALID_PACKET_TYPE ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpnComm::sendTrCommit( void               * aHBTResource,
                              cmiProtocolContext * aProtocolContext,
                              idBool             * aExitFlag,
                              smTID                aTID,
                              smSN                 aSN,
                              smSN                 aSyncSN,
                              idBool               aForceFlush,
                              UInt                 aTimeoutSec )
{
    cmpPacketType sPacketType;

    sPacketType = cmiGetPacketType( aProtocolContext );

    switch ( sPacketType )
    {
        case CMP_PACKET_TYPE_A5:
            IDE_TEST( sendTrCommitA5( aHBTResource,
                                      aProtocolContext,
                                      aExitFlag,
                                      aTID,
                                      aSN,
                                      aSyncSN,
                                      aForceFlush,
                                      aTimeoutSec )
                      != IDE_SUCCESS );
            break;

        case CMP_PACKET_TYPE_A7:
            IDE_TEST( sendTrCommitA7( aHBTResource,
                                      aProtocolContext,
                                      aExitFlag,
                                      aTID,
                                      aSN,
                                      aSyncSN,
                                      aForceFlush,
                                      aTimeoutSec )
                      != IDE_SUCCESS );
            break;

        case CMP_PACKET_TYPE_UNKNOWN:
        default:
            IDE_RAISE( UNKNOWN_PACKET_TYPE );
            break;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( UNKNOWN_PACKET_TYPE )
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_INVALID_PACKET_TYPE ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpnComm::sendXaCommit( void               * aHBTResource,
                              cmiProtocolContext * aProtocolContext,
                              idBool             * aExitFlag,
                              smTID                aTID,
                              smSN                 aSN,
                              smSN                 aSyncSN,
                              smSCN                aGlobalCommitSCN,
                              idBool               aForceFlush,
                              UInt                 aTimeoutSec )
{
    cmpPacketType sPacketType;

    sPacketType = cmiGetPacketType( aProtocolContext );

    switch ( sPacketType )
    {
        case CMP_PACKET_TYPE_A7:
            IDE_TEST( sendXaCommitA7( aHBTResource,
                                      aProtocolContext,
                                      aExitFlag,
                                      aTID,
                                      aSN,
                                      aSyncSN,
                                      aGlobalCommitSCN,
                                      aForceFlush,
                                      aTimeoutSec )
                      != IDE_SUCCESS );
            break;

        case CMP_PACKET_TYPE_A5:
        case CMP_PACKET_TYPE_UNKNOWN:
        default:
            IDE_RAISE( UNKNOWN_PACKET_TYPE );
            break;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( UNKNOWN_PACKET_TYPE )
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_INVALID_PACKET_TYPE ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpnComm::sendTrAbort( void               * aHBTResource,
                             cmiProtocolContext * aProtocolContext,
                             idBool             * aExitFlag,
                             smTID                aTID,
                             smSN                 aSN,
                             smSN                 aSyncSN,
                             UInt                 aTimeoutSec )
{
    cmpPacketType sPacketType;

    sPacketType = cmiGetPacketType( aProtocolContext );

    switch ( sPacketType )
    {
        case CMP_PACKET_TYPE_A5:
            IDE_TEST( sendTrAbortA5( aHBTResource,
                                     aProtocolContext,
                                     aExitFlag,
                                     aTID,
                                     aSN,
                                     aSyncSN,
                                     aTimeoutSec )
                      != IDE_SUCCESS );
            break;

        case CMP_PACKET_TYPE_A7:
            IDE_TEST( sendTrAbortA7( aHBTResource,
                                     aProtocolContext,
                                     aExitFlag,
                                     aTID,
                                     aSN,
                                     aSyncSN,
                                     aTimeoutSec )
                      != IDE_SUCCESS );
            break;

        case CMP_PACKET_TYPE_UNKNOWN:
        default:
            IDE_RAISE( UNKNOWN_PACKET_TYPE );
            break;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( UNKNOWN_PACKET_TYPE )
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_INVALID_PACKET_TYPE ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpnComm::sendSPSet( void               * aHBTResource,
                           cmiProtocolContext * aProtocolContext,
                           idBool             * aExitFlag,
                           smTID                aTID,
                           smSN                 aSN,
                           smSN                 aSyncSN,
                           UInt                 aSPNameLen,
                           SChar              * aSPName,
                           UInt                 aTimeoutSec )
{
    cmpPacketType sPacketType;

    sPacketType = cmiGetPacketType( aProtocolContext );

    switch ( sPacketType )
    {
        case CMP_PACKET_TYPE_A5:
            IDE_TEST( sendSPSetA5( aHBTResource,
                                   aProtocolContext,
                                   aExitFlag,
                                   aTID,
                                   aSN,
                                   aSyncSN,
                                   aSPNameLen,
                                   aSPName,
                                   aTimeoutSec )
                      != IDE_SUCCESS );
            break;

        case CMP_PACKET_TYPE_A7:
            IDE_TEST( sendSPSetA7( aHBTResource,
                                   aProtocolContext,
                                   aExitFlag,
                                   aTID,
                                   aSN,
                                   aSyncSN,
                                   aSPNameLen,
                                   aSPName,
                                   aTimeoutSec )
                      != IDE_SUCCESS );
            break;

        case CMP_PACKET_TYPE_UNKNOWN:
        default:
            IDE_RAISE( UNKNOWN_PACKET_TYPE );
            break;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( UNKNOWN_PACKET_TYPE )
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_INVALID_PACKET_TYPE ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpnComm::sendSPAbort( void               * aHBTResource,
                             cmiProtocolContext * aProtocolContext,
                             idBool             * aExitFlag,
                             smTID                aTID,
                             smSN                 aSN,
                             smSN                 aSyncSN,
                             UInt                 aSPNameLen,
                             SChar              * aSPName,
                             UInt                 aTimeoutSec )
{
    cmpPacketType sPacketType;

    sPacketType = cmiGetPacketType( aProtocolContext );

    switch ( sPacketType )
    {
        case CMP_PACKET_TYPE_A5:
            IDE_TEST( sendSPAbortA5( aHBTResource,
                                     aProtocolContext,
                                     aExitFlag,
                                     aTID,
                                     aSN,
                                     aSyncSN,
                                     aSPNameLen,
                                     aSPName,
                                     aTimeoutSec )
                      != IDE_SUCCESS );
            break;

        case CMP_PACKET_TYPE_A7:
            IDE_TEST( sendSPAbortA7( aHBTResource,
                                     aProtocolContext,
                                     aExitFlag,
                                     aTID,
                                     aSN,
                                     aSyncSN,
                                     aSPNameLen,
                                     aSPName,
                                     aTimeoutSec )
                      != IDE_SUCCESS );
            break;

        case CMP_PACKET_TYPE_UNKNOWN:
        default :
            IDE_RAISE( UNKNOWN_PACKET_TYPE );
            break;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( UNKNOWN_PACKET_TYPE )
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_INVALID_PACKET_TYPE ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpnComm::sendStmtBegin( void                   * /*aHBTResource*/,
                               cmiProtocolContext     * /*aCmiProtocolContext*/,
                               idBool                 * /*aExitFlag*/,
                               rpdLogAnalyzer         * /*aLA*/,
                               UInt                     /*aTimeoutSec*/ )
{
    /* Not Yet Support */
    IDE_DASSERT(ID_TRUE == ID_FALSE);

    return IDE_FAILURE;
}

IDE_RC rpnComm::sendStmtEnd( void                   * /*aHBTResource*/,
                             cmiProtocolContext     * /*aCmiProtocolContext*/,
                             idBool                 * /*aExitFlag*/,
                             rpdLogAnalyzer         * /*aLA*/,
                             UInt                     /*aTimeoutSec*/ )
{
    /* Not Yet Support */
    IDE_DASSERT(ID_TRUE == ID_FALSE);

    return IDE_FAILURE;
}

IDE_RC rpnComm::sendCursorOpen( void                   * /*aHBTResource*/,
                                cmiProtocolContext     * /*aCmiProtocolContext*/,
                                idBool                 * /*aExitFlag*/,
                                rpdLogAnalyzer         * /*aLA*/,
                                UInt                     /*aTimeoutSec*/ )
{
    /* Not Yet Support */
    IDE_DASSERT(ID_TRUE == ID_FALSE);

    return IDE_FAILURE;
}

IDE_RC rpnComm::sendCursorClose( void                   * /*aHBTResource*/,
                                 cmiProtocolContext     * /*aCmiProtocolContext*/,
                                 idBool                 * /*aExitFlag*/,
                                 rpdLogAnalyzer         * /*aLA*/,
                                 UInt                     /*aTimeoutSec*/ )
{
    /* Not Yet Support */
    IDE_DASSERT(ID_TRUE == ID_FALSE);

    return IDE_FAILURE;
}

IDE_RC rpnComm::sendInsert( void               * aHBTResource,
                            cmiProtocolContext * aProtocolContext,
                            idBool             * aExitFlag,
                            smTID                aTID,
                            smSN                 aSN,
                            smSN                 aSyncSN,
                            UInt                 aImplSPDepth,
                            ULong                aTableOID,
                            UInt                 aColCnt,
                            smiValue           * aACols,
                            rpValueLen         * aALen,
                            UInt                 aTimeoutSec )
{
    cmpPacketType sPacketType;

    sPacketType = cmiGetPacketType( aProtocolContext );

    switch ( sPacketType )
    {
        case CMP_PACKET_TYPE_A5:
            IDE_TEST( sendInsertA5( aHBTResource,
                                    aProtocolContext,
                                    aExitFlag,
                                    aTID,
                                    aSN,
                                    aSyncSN,
                                    aImplSPDepth,
                                    aTableOID,
                                    aColCnt,
                                    aACols,
                                    aALen,
                                    aTimeoutSec )
                      != IDE_SUCCESS );
            break;

        case CMP_PACKET_TYPE_A7:
            IDE_TEST( sendInsertA7( aHBTResource,
                                    aProtocolContext,
                                    aExitFlag,
                                    aTID,
                                    aSN,
                                    aSyncSN,
                                    aImplSPDepth,
                                    aTableOID,
                                    aColCnt,
                                    aACols,
                                    aALen,
                                    aTimeoutSec )
                      != IDE_SUCCESS );
            break;

        case CMP_PACKET_TYPE_UNKNOWN:
        default :
            IDE_RAISE( UNKNOWN_PACKET_TYPE );
            break;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( UNKNOWN_PACKET_TYPE )
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_INVALID_PACKET_TYPE ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpnComm::sendSyncInsert( void               * aHBTResource,
                                cmiProtocolContext * aProtocolContext,
                                idBool             * aExitFlag,
                                smTID                aTID,
                                smSN                 aSN,
                                smSN                 aSyncSN,
                                UInt                 aImplSPDepth,
                                ULong                aTableOID,
                                UInt                 aColCnt,
                                smiValue           * aACols,
                                rpValueLen         * aALen,
                                UInt                 aTimeoutSec )
{
    UInt i;
    UInt sType;
    UChar sOpCode;

    IDE_TEST( validateA7Protocol( aProtocolContext ) != IDE_SUCCESS );
    
    sType= RP_X_SYNC_INSERT;

    /* Validate Parameters */
    IDE_DASSERT(aImplSPDepth <= SMI_STATEMENT_DEPTH_MAX);
    IDE_DASSERT(aColCnt <= QCI_MAX_COLUMN_COUNT);
    IDE_DASSERT(aACols != NULL);

    IDE_TEST( checkAndFlush( aHBTResource,
                             aProtocolContext, 
                             aExitFlag,
                             1 + 40, 
                             ID_TRUE,
                             aTimeoutSec )
              != IDE_SUCCESS );

    /* Replication XLog Header Set */
    sOpCode = CMI_PROTOCOL_OPERATION( RP, Insert );
    CMI_WR1( aProtocolContext, sOpCode );
    CMI_WR4( aProtocolContext, &(sType) );
    CMI_WR4( aProtocolContext, &(aTID) );
    CMI_WR8( aProtocolContext, &(aSN) );
    CMI_WR8( aProtocolContext, &(aSyncSN) );
    /* Additional Information     */
    CMI_WR4( aProtocolContext, &(aImplSPDepth) );
    CMI_WR8( aProtocolContext, &(aTableOID) );
    CMI_WR4( aProtocolContext, &(aColCnt) );

    /* Send Value repeatedly */
    for(i = 0; i < aColCnt; i ++)
    {
        IDE_TEST( sendValueForA7( aHBTResource,
                                  aProtocolContext, 
                                  aExitFlag,
                                  &aACols[i], 
                                  aALen[i],
                                  aTimeoutSec )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpnComm::sendUpdate( void            * aHBTResource,
                            cmiProtocolContext *aProtocolContext,
                            idBool          * aExitFlag,
                            smTID             aTID,
                            smSN              aSN,
                            smSN              aSyncSN,
                            UInt              aImplSPDepth,
                            ULong             aTableOID,
                            UInt              aPKColCnt,
                            UInt              aUpdateColCnt,
                            smiValue        * aPKCols,
                            UInt            * aCIDs,
                            smiValue        * aBCols,
                            smiChainedValue * aBChainedCols, // PROJ-1705
                            UInt            * aBChainedColsTotalLen, /* BUG-33722 */
                            smiValue        * aACols,
                            rpValueLen      * aPKLen,
                            rpValueLen      * aALen,
                            rpValueLen      * aBMtdValueLen,
                            UInt              aTimeoutSec )

{
    cmpPacketType sPacketType;

    sPacketType = cmiGetPacketType( aProtocolContext );

    switch ( sPacketType )
    {
        case CMP_PACKET_TYPE_A5:
            IDE_TEST( sendUpdateA5( aHBTResource,
                                    aProtocolContext,
                                    aExitFlag,
                                    aTID,
                                    aSN,
                                    aSyncSN,
                                    aImplSPDepth,
                                    aTableOID,
                                    aPKColCnt,
                                    aUpdateColCnt,
                                    aPKCols,
                                    aCIDs,
                                    aBCols,
                                    aBChainedCols,
                                    aBChainedColsTotalLen,
                                    aACols,
                                    aPKLen,
                                    aALen,
                                    aBMtdValueLen,
                                    aTimeoutSec )
                      != IDE_SUCCESS );
            break;

        case CMP_PACKET_TYPE_A7:
            IDE_TEST( sendUpdateA7( aHBTResource,
                                    aProtocolContext,
                                    aExitFlag,
                                    aTID,
                                    aSN,
                                    aSyncSN,
                                    aImplSPDepth,
                                    aTableOID,
                                    aPKColCnt,
                                    aUpdateColCnt,
                                    aPKCols,
                                    aCIDs,
                                    aBCols,
                                    aBChainedCols,
                                    aBChainedColsTotalLen,
                                    aACols,
                                    aPKLen,
                                    aALen,
                                    aBMtdValueLen,
                                    aTimeoutSec )
                      != IDE_SUCCESS );
            break;

        case CMP_PACKET_TYPE_UNKNOWN:
        default :
            IDE_RAISE( UNKNOWN_PACKET_TYPE );
            break;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( UNKNOWN_PACKET_TYPE )
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_INVALID_PACKET_TYPE ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpnComm::sendDelete( void              * aHBTResource,
                            cmiProtocolContext *aProtocolContext,
                            idBool            * aExitFlag,
                            smTID               aTID,
                            smSN                aSN,
                            smSN                aSyncSN,
                            UInt                aImplSPDepth,
                            ULong               aTableOID,
                            UInt                aPKColCnt,
                            smiValue           *aPKCols,
                            rpValueLen         *aPKLen,
                            UInt                aColCnt,
                            UInt              * aCIDs,
                            smiValue          * aBCols,
                            smiChainedValue   * aBChainedCols, // PROJ-1705
                            rpValueLen        * aBMtdValueLen,
                            UInt              * aBChainedColsTotalLen,
                            UInt                aTimeoutSec )
{
    cmpPacketType sPacketType;

    sPacketType = cmiGetPacketType( aProtocolContext );

    switch ( sPacketType )
    {
        case CMP_PACKET_TYPE_A5:
            IDE_TEST( sendDeleteA5( aHBTResource,
                                    aProtocolContext,
                                    aExitFlag,
                                    aTID,
                                    aSN,
                                    aSyncSN,
                                    aImplSPDepth,
                                    aTableOID,
                                    aPKColCnt,
                                    aPKCols,
                                    aPKLen,
                                    aColCnt,
                                    aCIDs,
                                    aBCols,
                                    aBChainedCols,
                                    aBMtdValueLen,
                                    aBChainedColsTotalLen,
                                    aTimeoutSec )
                      != IDE_SUCCESS );
            break;

        case CMP_PACKET_TYPE_A7:
            IDE_TEST( sendDeleteA7( aHBTResource,
                                    aProtocolContext,
                                    aExitFlag,
                                    aTID,
                                    aSN,
                                    aSyncSN,
                                    aImplSPDepth,
                                    aTableOID,
                                    aPKColCnt,
                                    aPKCols,
                                    aPKLen,
                                    aColCnt,
                                    aCIDs,
                                    aBCols,
                                    aBChainedCols,
                                    aBMtdValueLen,
                                    aBChainedColsTotalLen,
                                    aTimeoutSec )
                      != IDE_SUCCESS );
            break;

        case CMP_PACKET_TYPE_UNKNOWN:
        default :
            IDE_RAISE( UNKNOWN_PACKET_TYPE );
            break;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( UNKNOWN_PACKET_TYPE )
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_INVALID_PACKET_TYPE ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC rpnComm::sendStop( void               * aHBTResource,
                          cmiProtocolContext * aProtocolContext,
                          idBool             * aExitFlag,
                          smTID                aTID,
                          smSN                 aSN,
                          smSN                 aSyncSN,
                          smSN                 aRestartSN,
                          UInt                 aTimeoutSec )
{
    cmpPacketType sPacketType;

    sPacketType = cmiGetPacketType( aProtocolContext );

    switch ( sPacketType )
    {
        case CMP_PACKET_TYPE_A5:
            IDE_TEST( sendStopA5( aHBTResource,
                                  aProtocolContext,
                                  aExitFlag,
                                  aTID,
                                  aSN,
                                  aSyncSN,
                                  aRestartSN,
                                  aTimeoutSec )
                      != IDE_SUCCESS );
            break;

        case CMP_PACKET_TYPE_A7:
            IDE_TEST( sendStopA7( aHBTResource,
                                  aProtocolContext,
                                  aExitFlag,
                                  aTID,
                                  aSN,
                                  aSyncSN,
                                  aRestartSN,
                                  aTimeoutSec )
                      != IDE_SUCCESS );
            break;

        case CMP_PACKET_TYPE_UNKNOWN:
        default :
            IDE_RAISE( UNKNOWN_PACKET_TYPE );
            break;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( UNKNOWN_PACKET_TYPE )
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_INVALID_PACKET_TYPE ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpnComm::sendKeepAlive( void               * aHBTResource,
                               cmiProtocolContext * aProtocolContext,
                               idBool             * aExitFlag,
                               smTID                aTID,
                               smSN                 aSN,
                               smSN                 aSyncSN,
                               smSN                 aRestartSN,
                               UInt                 aTimeoutSec )
{
    cmpPacketType sPacketType;

    sPacketType = cmiGetPacketType( aProtocolContext );

    switch ( sPacketType )
    {
        case CMP_PACKET_TYPE_A5:
            IDE_TEST( sendKeepAliveA5( aHBTResource,
                                       aProtocolContext,
                                       aExitFlag,
                                       aTID,
                                       aSN,
                                       aSyncSN,
                                       aRestartSN,
                                       aTimeoutSec )
                      != IDE_SUCCESS );
            break;

        case CMP_PACKET_TYPE_A7:
            IDE_TEST( sendKeepAliveA7( aHBTResource,
                                       aProtocolContext,
                                       aExitFlag,
                                       aTID,
                                       aSN,
                                       aSyncSN,
                                       aRestartSN,
                                       aTimeoutSec )
                      != IDE_SUCCESS );
            break;

        case CMP_PACKET_TYPE_UNKNOWN:
        default :
            IDE_RAISE( UNKNOWN_PACKET_TYPE );
            break;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( UNKNOWN_PACKET_TYPE )
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_INVALID_PACKET_TYPE ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpnComm::sendFlush( void               * aHBTResource,
                           cmiProtocolContext * aProtocolContext,
                           idBool             * aExitFlag,
                           smTID                aTID,
                           smSN                 aSN,
                           smSN                 aSyncSN,
                           UInt                 aFlushOption,
                           UInt                 aTimeoutSec )
{
    cmpPacketType sPacketType;

    sPacketType = cmiGetPacketType( aProtocolContext );

    switch ( sPacketType )
    {
        case CMP_PACKET_TYPE_A5:
            IDE_TEST( sendFlushA5( aHBTResource,
                                   aProtocolContext,
                                   aExitFlag,
                                   aTID,
                                   aSN,
                                   aSyncSN,
                                   aFlushOption,
                                   aTimeoutSec )
                      != IDE_SUCCESS );
            break;

        case CMP_PACKET_TYPE_A7:
            IDE_TEST( sendFlushA7( aHBTResource,
                                   aProtocolContext,
                                   aExitFlag,
                                   aTID,
                                   aSN,
                                   aSyncSN,
                                   aFlushOption,
                                   aTimeoutSec )
                      != IDE_SUCCESS );
            break;

        case CMP_PACKET_TYPE_UNKNOWN:
        default :
            IDE_RAISE( UNKNOWN_PACKET_TYPE );
            break;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( UNKNOWN_PACKET_TYPE )
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_INVALID_PACKET_TYPE ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC rpnComm::sendAckEager( cmiProtocolContext * aProtocolContext,
                              idBool             * aExitFlag,
                              rpXLogAck          * aAck,
                              UInt                 aTimeoutSec )
{
    UInt i = 0;
    UChar sOpCode = CMI_PROTOCOL_OPERATION( RP, AckEager );

    IDE_TEST( checkAndFlush( NULL,
                             aProtocolContext,
                             aExitFlag,
                             1 + 56, 
                             ID_TRUE,
                             aTimeoutSec )
              != IDE_SUCCESS );

    /* Replication XLog Header Set */
    CMI_WR1( aProtocolContext, sOpCode );
    CMI_WR4( aProtocolContext, &(aAck->mAckType) );
    CMI_WR4( aProtocolContext, &(aAck->mAbortTxCount) );
    CMI_WR4( aProtocolContext, &(aAck->mClearTxCount) );
    CMI_WR8( aProtocolContext, &(aAck->mRestartSN) );
    CMI_WR8( aProtocolContext, &(aAck->mLastCommitSN) );
    CMI_WR8( aProtocolContext, &(aAck->mLastArrivedSN) );
    CMI_WR8( aProtocolContext, &(aAck->mLastProcessedSN) );
    CMI_WR8( aProtocolContext, &(aAck->mFlushSN) );
    CMI_WR4( aProtocolContext, &(aAck->mTID) );

    for ( i = 0 ; i < aAck->mAbortTxCount ; i++ )
    {
        IDE_TEST( sendTxAckA7( aProtocolContext,
                               aExitFlag,
                               aAck->mAbortTxList[i].mTID,
                               aAck->mAbortTxList[i].mSN,
                               aTimeoutSec )
                  != IDE_SUCCESS );
    }

    for ( i = 0 ; i < aAck->mClearTxCount ; i++ )
    {
        IDE_TEST( sendTxAckA7( aProtocolContext,
                               aExitFlag,
                               aAck->mClearTxList[i].mTID,
                               aAck->mClearTxList[i].mSN,
                               aTimeoutSec )
                  != IDE_SUCCESS );
    }

    IDE_TEST( sendCmBlock( NULL,    /* aHBTResource */
                           aProtocolContext, 
                           aExitFlag,
                           ID_TRUE,
                           aTimeoutSec ) 
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

IDE_RC rpnComm::sendAck( cmiProtocolContext * aProtocolContext,
                         idBool             * aExitFlag,
                         rpXLogAck          * aAck,
                         UInt                 aTimeoutSec )
{
    cmpPacketType sPacketType;
    
    sPacketType = cmiGetPacketType( aProtocolContext );

    switch ( sPacketType )
    {
        case CMP_PACKET_TYPE_A5:
            IDE_TEST( sendAckA5( aProtocolContext, 
                                 aExitFlag,
                                 aAck,
                                 aTimeoutSec ) 
                      != IDE_SUCCESS );
            break;
            
        case CMP_PACKET_TYPE_A7:
            IDE_TEST( sendAckA7( aProtocolContext, 
                                 aExitFlag,
                                 aAck,
                                 aTimeoutSec ) 
                      != IDE_SUCCESS );
            break;

        case CMP_PACKET_TYPE_UNKNOWN:
            IDE_RAISE( UNKNOWN_PACKET_TYPE );
            break;
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( UNKNOWN_PACKET_TYPE )
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_INVALID_PACKET_TYPE ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* recvAck는 readBlock를 사용하므로 sender의 writeBlock과 겹치치 않는다. 동시성 문제 없음 */
IDE_RC rpnComm::recvAck( iduMemAllocator    * /*aAllocator*/,
                         void               * aHBTResource,
                         cmiProtocolContext * aProtocolContext,
                         idBool             * aExitFlag,
                         rpXLogAck          * aAck,
                         ULong                aTimeoutSec,
                         idBool             * aIsTimeoutSec )
{
    cmpPacketType sPacketType = cmiGetPacketType( aProtocolContext );

    switch ( sPacketType )
    {
        case CMP_PACKET_TYPE_A5:
            IDE_TEST( recvAckA5( NULL, /*aAllocator*/
                                 aProtocolContext,
                                 aExitFlag,
                                 aAck,
                                 aTimeoutSec,
                                 aIsTimeoutSec )
                      != IDE_SUCCESS );
            break;

        case CMP_PACKET_TYPE_A7:
            IDE_TEST( recvAckA7( NULL, /*aAllocator*/
                                 aHBTResource,
                                 aProtocolContext,
                                 aExitFlag,
                                 aAck,
                                 aTimeoutSec,
                                 aIsTimeoutSec )
                      != IDE_SUCCESS );
            break;

        case CMP_PACKET_TYPE_UNKNOWN:
            /* fall through */
        default:
            IDE_RAISE( UNKNOWN_PACKET_TYPE );
            break;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( UNKNOWN_PACKET_TYPE )
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_INVALID_PACKET_TYPE ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpnComm::sendLobCursorOpen( void               * aHBTResource,
                                   cmiProtocolContext * aProtocolContext,
                                   idBool             * aExitFlag,
                                   smTID                aTID,
                                   smSN                 aSN,
                                   smSN                 aSyncSN,
                                   ULong                aTableOID,
                                   ULong                aLobLocator,
                                   UInt                 aLobColumnID,
                                   UInt                 aPKColCnt,
                                   smiValue           * aPKCols,
                                   rpValueLen         * aPKLen,
                                   UInt                 aTimeoutSec )
{
    cmpPacketType sPacketType;

    sPacketType = cmiGetPacketType( aProtocolContext );

    switch ( sPacketType )
    {
        case CMP_PACKET_TYPE_A5:
            IDE_TEST( sendLobCursorOpenA5( aHBTResource,
                                           aProtocolContext,
                                           aExitFlag,
                                           aTID,
                                           aSN,
                                           aSyncSN,
                                           aTableOID,
                                           aLobLocator,
                                           aLobColumnID,
                                           aPKColCnt,
                                           aPKCols,
                                           aPKLen,
                                           aTimeoutSec )
                      != IDE_SUCCESS );
            break;

        case CMP_PACKET_TYPE_A7:
            IDE_TEST( sendLobCursorOpenA7( aHBTResource,
                                           aProtocolContext,
                                           aExitFlag,
                                           aTID,
                                           aSN,
                                           aSyncSN,
                                           aTableOID,
                                           aLobLocator,
                                           aLobColumnID,
                                           aPKColCnt,
                                           aPKCols,
                                           aPKLen,
                                           aTimeoutSec )
                      != IDE_SUCCESS );
            break;

        case CMP_PACKET_TYPE_UNKNOWN:
        default :
            IDE_RAISE( UNKNOWN_PACKET_TYPE );
            break;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( UNKNOWN_PACKET_TYPE )
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_INVALID_PACKET_TYPE ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpnComm::sendLobCursorClose( void                * aHBTResource,
                                    cmiProtocolContext  * aProtocolContext,
                                    idBool              * aExitFlag,
                                    smTID                 aTID,
                                    smSN                  aSN,
                                    smSN                  aSyncSN,
                                    ULong                 aLobLocator,
                                    UInt                  aTimeoutSec )
{
    cmpPacketType sPacketType;

    sPacketType = cmiGetPacketType( aProtocolContext );

    switch ( sPacketType )
    {
        case CMP_PACKET_TYPE_A5:
            IDE_TEST( sendLobCursorCloseA5( aHBTResource,
                                            aProtocolContext,
                                            aExitFlag,
                                            aTID,
                                            aSN,
                                            aSyncSN,
                                            aLobLocator,
                                            aTimeoutSec )
                      != IDE_SUCCESS );
            break;

        case CMP_PACKET_TYPE_A7:
            IDE_TEST( sendLobCursorCloseA7( aHBTResource,
                                            aProtocolContext,
                                            aExitFlag,
                                            aTID,
                                            aSN,
                                            aSyncSN,
                                            aLobLocator,
                                            aTimeoutSec )
                      != IDE_SUCCESS );
            break;

        case CMP_PACKET_TYPE_UNKNOWN:
        default :
            IDE_RAISE( UNKNOWN_PACKET_TYPE );
            break;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( UNKNOWN_PACKET_TYPE )
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_INVALID_PACKET_TYPE ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpnComm::sendLobPrepare4Write( void               * aHBTResource,
                                      cmiProtocolContext * aProtocolContext,
                                      idBool             * aExitFlag,
                                      smTID                aTID,
                                      smSN                 aSN,
                                      smSN                 aSyncSN,
                                      ULong                aLobLocator,
                                      UInt                 aLobOffset,
                                      UInt                 aLobOldSize,
                                      UInt                 aLobNewSize,
                                      UInt                 aTimeoutSec )
{
    cmpPacketType sPacketType;

    sPacketType = cmiGetPacketType( aProtocolContext );

    switch ( sPacketType )
    {
        case CMP_PACKET_TYPE_A5:
            IDE_TEST( sendLobPrepare4WriteA5( aHBTResource,
                                              aProtocolContext,
                                              aExitFlag,
                                              aTID,
                                              aSN,
                                              aSyncSN,
                                              aLobLocator,
                                              aLobOffset,
                                              aLobOldSize,
                                              aLobNewSize,
                                              aTimeoutSec )
                      != IDE_SUCCESS );
            break;

        case CMP_PACKET_TYPE_A7:
            IDE_TEST( sendLobPrepare4WriteA7( aHBTResource,
                                              aProtocolContext,
                                              aExitFlag,
                                              aTID,
                                              aSN,
                                              aSyncSN,
                                              aLobLocator,
                                              aLobOffset,
                                              aLobOldSize,
                                              aLobNewSize,
                                              aTimeoutSec )
                      != IDE_SUCCESS );
            break;

        case CMP_PACKET_TYPE_UNKNOWN:
        default :
            IDE_RAISE( UNKNOWN_PACKET_TYPE );
            break;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( UNKNOWN_PACKET_TYPE )
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_INVALID_PACKET_TYPE ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpnComm::sendLobTrim( void               * aHBTResource,
                             cmiProtocolContext * aProtocolContext,
                             idBool             * aExitFlag,
                             smTID                aTID,
                             smSN                 aSN,
                             smSN                 aSyncSN,
                             ULong                aLobLocator,
                             UInt                 aLobOffset,
                             UInt                 aTimeoutSec )
{
    cmpPacketType sPacketType;

    sPacketType = cmiGetPacketType( aProtocolContext );

    switch ( sPacketType )
    {
        case CMP_PACKET_TYPE_A5:
            IDE_TEST( sendLobTrimA5( aHBTResource,
                                     aProtocolContext,
                                     aExitFlag,
                                     aTID,
                                     aSN,
                                     aSyncSN,
                                     aLobLocator,
                                     aLobOffset,
                                     aTimeoutSec )
                      != IDE_SUCCESS );
            break;

        case CMP_PACKET_TYPE_A7:

            IDE_TEST( sendLobTrimA7( aHBTResource,
                                     aProtocolContext,
                                     aExitFlag,
                                     aTID,
                                     aSN,
                                     aSyncSN,
                                     aLobLocator,
                                     aLobOffset,
                                     aTimeoutSec )
                      != IDE_SUCCESS );
            break;

        case CMP_PACKET_TYPE_UNKNOWN:
        default :
            IDE_RAISE( UNKNOWN_PACKET_TYPE );
            break;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( UNKNOWN_PACKET_TYPE )
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_INVALID_PACKET_TYPE ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpnComm::recvLobTrim( iduMemAllocator     * /*aAllocator*/,
                             idBool              * /* aExitFlag */,
                             rpxReceiverReadContext aReadContext,
                             rpdXLog             * aXLog,
                             ULong                /*aTimeOutSec*/ )
{
    UInt * sType = (UInt*)&(aXLog->mType);

    if ( aReadContext.mCurrentMode == RPX_RECEIVER_READ_NETWORK )
    {
        IDE_TEST( validateA7Protocol( aReadContext.mCMContext ) != IDE_SUCCESS );

        /* Get Argument XLog Hdr */
        CMI_RD4( aReadContext.mCMContext, sType );
        CMI_RD4( aReadContext.mCMContext, &(aXLog->mTID) );
        CMI_RD8( aReadContext.mCMContext, &(aXLog->mSN) );
        CMI_RD8( aReadContext.mCMContext, &(aXLog->mSyncSN) );
        /* Get Lob Information */
        CMI_RD8( aReadContext.mCMContext, &(aXLog->mLobPtr->mLobLocator) );
        CMI_RD4( aReadContext.mCMContext, &(aXLog->mLobPtr->mLobOffset) );
    }
    else if ( aReadContext.mCurrentMode == RPX_RECEIVER_READ_XLOGFILE )
    {
        aReadContext.mXLogfileContext->readXLog( &(aXLog->mType),         4 );
        aReadContext.mXLogfileContext->readXLog( &(aXLog->mTID),          4 );
        aReadContext.mXLogfileContext->readXLog( &(aXLog->mSN),           8 );
        aReadContext.mXLogfileContext->readXLog( &(aXLog->mSyncSN),       8 );
        aReadContext.mXLogfileContext->readXLog( &(aXLog->mLobPtr->mLobLocator), 8 );
        aReadContext.mXLogfileContext->readXLog( &(aXLog->mLobPtr->mLobOffset), 4 );
    }
    else
    {
        IDE_RAISE( ERR_INTERNAL );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INTERNAL )
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RPN_UNABLE_READ_CONTEXT ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpnComm::sendLobPartialWrite( void               * aHBTResource,
                                     cmiProtocolContext * aProtocolContext,
                                     idBool             * aExitFlag,
                                     smTID                aTID,
                                     smSN                 aSN,
                                     smSN                 aSyncSN,
                                     ULong                aLobLocator,
                                     UInt                 aLobOffset,
                                     UInt                 aLobPieceLen,
                                     SChar              * aLobPiece,
                                     UInt                 aTimeoutSec )
{
    cmpPacketType sPacketType;

    sPacketType = cmiGetPacketType( aProtocolContext );

    switch ( sPacketType )
    {
        case CMP_PACKET_TYPE_A5:
            IDE_TEST( sendLobPartialWriteA5( aHBTResource,
                                             aProtocolContext,
                                             aExitFlag,
                                             aTID,
                                             aSN,
                                             aSyncSN,
                                             aLobLocator,
                                             aLobOffset,
                                             aLobPieceLen,
                                             aLobPiece,
                                             aTimeoutSec )
                      != IDE_SUCCESS );
            break;

        case CMP_PACKET_TYPE_A7:
            IDE_TEST( sendLobPartialWriteA7( aHBTResource,
                                             aProtocolContext,
                                             aExitFlag,
                                             aTID,
                                             aSN,
                                             aSyncSN,
                                             aLobLocator,
                                             aLobOffset,
                                             aLobPieceLen,
                                             aLobPiece,
                                             aTimeoutSec )
                      != IDE_SUCCESS );
            break;

        case CMP_PACKET_TYPE_UNKNOWN:
        default :
            IDE_RAISE( UNKNOWN_PACKET_TYPE );
            break;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( UNKNOWN_PACKET_TYPE )
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_INVALID_PACKET_TYPE ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpnComm::sendLobFinish2Write( void               * aHBTResource,
                                     cmiProtocolContext * aProtocolContext,
                                     idBool             * aExitFlag,
                                     smTID                aTID,
                                     smSN                 aSN,
                                     smSN                 aSyncSN,
                                     ULong                aLobLocator,
                                     UInt                 aTimeoutSec )
{
    cmpPacketType sPacketType;

    sPacketType = cmiGetPacketType( aProtocolContext );

    switch ( sPacketType )
    {
        case CMP_PACKET_TYPE_A5:
            IDE_TEST( sendLobFinish2WriteA5( aHBTResource,
                                             aProtocolContext,
                                             aExitFlag,
                                             aTID,
                                             aSN,
                                             aSyncSN,
                                             aLobLocator,
                                             aTimeoutSec )
                      != IDE_SUCCESS );
            break;

        case CMP_PACKET_TYPE_A7:
            IDE_TEST( sendLobFinish2WriteA7( aHBTResource,
                                             aProtocolContext,
                                             aExitFlag,
                                             aTID,
                                             aSN,
                                             aSyncSN,
                                             aLobLocator,
                                             aTimeoutSec )
                      != IDE_SUCCESS );
            break;

        case CMP_PACKET_TYPE_UNKNOWN:
        default :
            IDE_RAISE( UNKNOWN_PACKET_TYPE );
            break;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( UNKNOWN_PACKET_TYPE )
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_INVALID_PACKET_TYPE ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpnComm::sendHandshake( void               * aHBTResource,
                               cmiProtocolContext * aProtocolContext,
                               idBool             * aExitFlag,
                               smTID                aTID,
                               smSN                 aSN,
                               smSN                 aSyncSN,
                               UInt                 aTimeoutSec )
{
    cmpPacketType sPacketType;

    sPacketType = cmiGetPacketType( aProtocolContext );

    switch ( sPacketType )
    {
        case CMP_PACKET_TYPE_A5:
            IDE_TEST( sendHandshakeA5( aHBTResource,
                                       aProtocolContext,
                                       aExitFlag,
                                       aTID,
                                       aSN,
                                       aSyncSN,
                                       aTimeoutSec )
                      != IDE_SUCCESS );
            break;

        case CMP_PACKET_TYPE_A7:
            IDE_TEST( sendHandshakeA7( aHBTResource,
                                       aProtocolContext,
                                       aExitFlag,
                                       aTID,
                                       aSN,
                                       aSyncSN,
                                       aTimeoutSec )
                      != IDE_SUCCESS );
            break;

        case CMP_PACKET_TYPE_UNKNOWN:
        default :
            IDE_RAISE( UNKNOWN_PACKET_TYPE );
            break;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( UNKNOWN_PACKET_TYPE )
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_INVALID_PACKET_TYPE ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpnComm::allocNullValue(iduMemAllocator * /*aAllocator*/,
                               iduMemory       * aMemory,
                               smiValue        * aValue,
                               const mtcColumn * aColumn,
                               const mtdModule * aModule)
{
    IDE_ASSERT( aMemory != NULL );
    IDE_ASSERT( aValue  != NULL );
    IDE_ASSERT( aColumn != NULL );
    IDE_ASSERT( aModule != NULL );

    aValue->value = NULL;

    aValue->length = aModule->actualSize(aColumn,
                                         aModule->staticNull);

    if(aValue->length > 0)
    {
        IDU_FIT_POINT_RAISE( "rpnComm::allocNullValue::malloc::Value", 
                              ERR_MEMORY_ALLOC_VALUE,
                              rpERR_ABORT_MEMORY_ALLOC,
                              "rpnComm::allocNullValue",
                              "Value" );
        IDE_TEST_RAISE( aMemory->alloc( aValue->length,
                                        (void **)&aValue->value ) 
                        != IDE_SUCCESS, ERR_MEMORY_ALLOC_VALUE );

        idlOS::memcpy((void *)aValue->value,
                      aModule->staticNull,
                      aValue->length);
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_MEMORY_ALLOC_VALUE);
    {
        IDE_ERRLOG(IDE_RP_0);
        IDE_SET(ideSetErrorCode(rpERR_ABORT_MEMORY_ALLOC,
                                "rpnComm::allocNullValue",
                                "aValue->value"));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpnComm::sendSyncPKBegin( void               * aHBTResource,
                                 cmiProtocolContext * aProtocolContext,
                                 idBool             * aExitFlag,
                                 smTID                aTID,
                                 smSN                 aSN,
                                 smSN                 aSyncSN,
                                 UInt                 aTimeoutSec )
{
    cmpPacketType sPacketType;

    sPacketType = cmiGetPacketType( aProtocolContext );

    switch ( sPacketType )
    {
        case CMP_PACKET_TYPE_A5:
            IDE_TEST( sendSyncPKBeginA5( aHBTResource,
                                         aProtocolContext,
                                         aExitFlag,
                                         aTID,
                                         aSN,
                                         aSyncSN,
                                         aTimeoutSec )
                      != IDE_SUCCESS );
            break;

        case CMP_PACKET_TYPE_A7:
            IDE_TEST( sendSyncPKBeginA7( aHBTResource,
                                         aProtocolContext,
                                         aExitFlag,
                                         aTID,
                                         aSN,
                                         aSyncSN,
                                         aTimeoutSec )
                      != IDE_SUCCESS );
            break;

        case CMP_PACKET_TYPE_UNKNOWN:
        default :
            IDE_RAISE( UNKNOWN_PACKET_TYPE );
            break;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( UNKNOWN_PACKET_TYPE )
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_INVALID_PACKET_TYPE ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpnComm::sendSyncPK( void               * aHBTResource,
                            cmiProtocolContext * aProtocolContext,
                            idBool             * aExitFlag,
                            smTID                aTID,
                            smSN                 aSN,
                            smSN                 aSyncSN,
                            ULong                aTableOID,
                            UInt                 aPKColCnt,
                            smiValue           * aPKCols,
                            rpValueLen         * aPKLen,
                            UInt                 aTimeoutSec )
{
    cmpPacketType sPacketType;

    sPacketType = cmiGetPacketType( aProtocolContext );

    switch ( sPacketType )
    {
        case CMP_PACKET_TYPE_A5:
            IDE_TEST( sendSyncPKA5( aHBTResource,
                                    aProtocolContext,
                                    aExitFlag,
                                    aTID,
                                    aSN,
                                    aSyncSN,
                                    aTableOID,
                                    aPKColCnt,
                                    aPKCols,
                                    aPKLen,
                                    aTimeoutSec )
                      != IDE_SUCCESS );
            break;

        case CMP_PACKET_TYPE_A7:
            IDE_TEST( sendSyncPKA7( aHBTResource,
                                    aProtocolContext,
                                    aExitFlag,
                                    aTID,
                                    aSN,
                                    aSyncSN,
                                    aTableOID,
                                    aPKColCnt,
                                    aPKCols,
                                    aPKLen,
                                    aTimeoutSec )
                      != IDE_SUCCESS );
            break;

        case CMP_PACKET_TYPE_UNKNOWN:
        default :
            IDE_RAISE( UNKNOWN_PACKET_TYPE );
            break;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( UNKNOWN_PACKET_TYPE )
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_INVALID_PACKET_TYPE ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpnComm::sendSyncPKEnd( void               * aHBTResource,
                               cmiProtocolContext * aProtocolContext,
                               idBool             * aExitFlag,
                               smTID                aTID,
                               smSN                 aSN,
                               smSN                 aSyncSN,
                               UInt                 aTimeoutSec )
{
    cmpPacketType sPacketType;

    sPacketType = cmiGetPacketType( aProtocolContext );

    switch ( sPacketType )
    {
        case CMP_PACKET_TYPE_A5:
            IDE_TEST( sendSyncPKEndA5( aHBTResource,
                                       aProtocolContext,
                                       aExitFlag,
                                       aTID,
                                       aSN,
                                       aSyncSN,
                                       aTimeoutSec )
                      != IDE_SUCCESS );
            break;

        case CMP_PACKET_TYPE_A7:
            IDE_TEST( sendSyncPKEndA7( aHBTResource,
                                       aProtocolContext,
                                       aExitFlag,
                                       aTID,
                                       aSN,
                                       aSyncSN,
                                       aTimeoutSec )
                      != IDE_SUCCESS );
            break;

        case CMP_PACKET_TYPE_UNKNOWN:
        default :
            IDE_RAISE( UNKNOWN_PACKET_TYPE );
            break;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( UNKNOWN_PACKET_TYPE )
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_INVALID_PACKET_TYPE ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpnComm::sendFailbackEnd( void               * aHBTResource,
                                 cmiProtocolContext * aProtocolContext,
                                 idBool             * aExitFlag,
                                 smTID                aTID,
                                 smSN                 aSN,
                                 smSN                 aSyncSN,
                                 UInt                 aTimeoutSec )
{
    cmpPacketType sPacketType;

    sPacketType = cmiGetPacketType( aProtocolContext );

    switch ( sPacketType )
    {
        case CMP_PACKET_TYPE_A5:
            IDE_TEST( sendFailbackEndA5( aHBTResource,
                                         aProtocolContext,
                                         aExitFlag,
                                         aTID,
                                         aSN,
                                         aSyncSN,
                                         aTimeoutSec )
                      != IDE_SUCCESS );
            break;

        case CMP_PACKET_TYPE_A7:
            IDE_TEST( sendFailbackEndA7( aHBTResource,
                                         aProtocolContext,
                                         aExitFlag,
                                         aTID,
                                         aSN,
                                         aSyncSN,
                                         aTimeoutSec )
                      != IDE_SUCCESS );
            break;

        case CMP_PACKET_TYPE_UNKNOWN:
        default :
            IDE_RAISE( UNKNOWN_PACKET_TYPE );
            break;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( UNKNOWN_PACKET_TYPE )
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_INVALID_PACKET_TYPE ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpnComm::readCmBlock( void               * aHBTResource,
                             cmiProtocolContext * aProtocolContext,
                             idBool             * aExitFlag,
                             idBool             * aIsTimeOut,
                             ULong                aTimeoutSec )
{
    ULong i;

    IDE_TEST_RAISE( CMI_IS_READ_ALL( aProtocolContext ) != ID_TRUE,
                    ERR_NOT_READ_ALL );

    for(i = 0; i < aTimeoutSec; i ++)
    {
        /* Read CM Block */
        IDU_FIT_POINT_RAISE( "rpnComm::readCmBlock::cmiRecv::ERR_READ",
                              ERR_READ );
        IDU_FIT_POINT_RAISE( "rpnComm::readCmBlock::cmiRecv::ERR_EXIT",
                              ERR_EXIT );
        if ( cmiRecv( aProtocolContext,
                      NULL,
                      &mTV1Sec,
                      NULL) == IDE_SUCCESS )
        {
            break;
        }
        IDE_TEST_RAISE( ideGetErrorCode() != cmERR_ABORT_TIMED_OUT,
                        ERR_READ );
        IDE_CLEAR();

        /* Check HBT Status */
        if ( aHBTResource != NULL )
        {
            IDU_FIT_POINT_RAISE( "rpnComm::readCmBlock::checkFault::ERR_HBT",
                                  ERR_HBT );
            IDE_TEST_RAISE( rpcHBT::checkFault( aHBTResource ) == ID_TRUE,
                            ERR_HBT );
        }
        else
        {
            /* do nothing */
        }

        if ( aExitFlag != NULL )
        {
            IDE_TEST_RAISE( *aExitFlag == ID_TRUE, ERR_EXIT );
        }
    }

    IDU_FIT_POINT_RAISE( "rpnComm::readCmBlock::cmiRecv::ERR_TIMEOUT", 
                          ERR_TIMEOUT );
    if ( ( aIsTimeOut != NULL ) && ( i >= aTimeoutSec ) )
    {
        *aIsTimeOut = ID_TRUE;
    }
    else
    {
        IDE_TEST_RAISE( i >= aTimeoutSec, ERR_TIMEOUT );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_READ_ALL )
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_DATA_STILL_REMAIN_IN_BUFFER ) );
    }
    IDE_EXCEPTION( ERR_READ );
    {
        /* Nothing to do */
    }
    IDE_EXCEPTION( ERR_HBT )
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_HBT_DETECT_PEER_SERVER_ERROR ) );
    }
    IDE_EXCEPTION( ERR_EXIT );
    {
        IDE_SET( ideSetErrorCode( rpERR_IGNORE_EXIT_FLAG_SET ) );
    }
    IDE_EXCEPTION( ERR_TIMEOUT );
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_TIMEOUT_EXCEED ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpnComm::sendSyncTableNumber( void               * aHBTResource,
                                     cmiProtocolContext * aProtocolContext,
                                     idBool             * aExitFlag,
                                     UInt                 aSyncTableNumber,
                                     UInt                 aTimeoutSec )
{
    UChar sOpCode;

    IDE_TEST( validateA7Protocol( aProtocolContext ) != IDE_SUCCESS );
    
    IDE_TEST( checkAndFlush( aHBTResource,
                             aProtocolContext, 
                             aExitFlag,
                             1 + 4, 
                             ID_TRUE,
                             aTimeoutSec )
              != IDE_SUCCESS );

    /* Replication SyncTableNumber Information Set */
    sOpCode = CMI_PROTOCOL_OPERATION( RP, SyncTableNumber );
    CMI_WR1( aProtocolContext, sOpCode );
    CMI_WR4( aProtocolContext, &(aSyncTableNumber) );

    IDU_FIT_POINT( "rpnComm::sendSyncTableNumber::cmiSend::ERR_SEND" );
    IDE_TEST( sendCmBlock( aHBTResource,
                           aProtocolContext, 
                           aExitFlag,
                           ID_TRUE,
                           aTimeoutSec )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpnComm::recvSyncTableNumber( cmiProtocolContext * aProtocolContext,
                                     UInt * aSyncTableNumber,
                                     ULong aTimeoutSec )
{
    UChar sOpCode;

    IDE_TEST( validateA7Protocol( aProtocolContext ) != IDE_SUCCESS );
    
    IDE_TEST( readCmBlock( NULL,
                           aProtocolContext,
                           NULL /* ExitFlag */,
                           NULL /* TimeoutFlag */,
                           aTimeoutSec )
              != IDE_SUCCESS );

    /* Check Operation Type */
    CMI_RD1( aProtocolContext, sOpCode );

    IDE_TEST_RAISE( sOpCode != CMI_PROTOCOL_OPERATION( RP, SyncTableNumber ),
                    ERR_CHECK_OPERATION_TYPE );

    CMI_RD4( aProtocolContext, aSyncTableNumber );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_CHECK_OPERATION_TYPE );
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RP_WRONG_OPERATION_TYPE,
                                  sOpCode ) );
        IDE_ERRLOG( IDE_RP_0 );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpnComm::sendSyncStart( void               * aHBTResource,
                               cmiProtocolContext * aProtocolContext,
                               idBool             * aExitFlag,
                               UInt                 aTimeoutSec )
{
    UChar sOpCode;
    UInt sType;

    sType = RP_X_SYNC_START;

    IDE_TEST( checkAndFlush( aHBTResource,
                             aProtocolContext,
                             aExitFlag,
                             1 + 4,
                             ID_TRUE,
                             aTimeoutSec )
              != IDE_SUCCESS );

    sOpCode = CMI_PROTOCOL_OPERATION( RP, SyncStart );
    CMI_WR1( aProtocolContext, sOpCode );
    CMI_WR4( aProtocolContext, &(sType) );

    IDU_FIT_POINT( "rpnComm::sendSyncStart::cmiSend::ERR_SEND" );
    IDE_TEST( sendCmBlock( aHBTResource,
                           aProtocolContext, 
                           aExitFlag,
                           ID_TRUE,
                           aTimeoutSec ) 
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpnComm::recvSyncStart( iduMemAllocator     * /*aAllocator*/,
                               idBool              * /*aExitFlag*/,
                               cmiProtocolContext  * aProtocolContext,
                               rpdXLog             * aXLog,
                               ULong                /*aTimeOutSec*/ )
{
    UInt * sType = (UInt*)&(aXLog->mType);

    IDE_TEST( validateA7Protocol( aProtocolContext ) != IDE_SUCCESS );
    
    /* Get Argument XLog Hdr */
    CMI_RD4( aProtocolContext, sType );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpnComm::sendSyncEnd( void               * aHBTResource,
                             cmiProtocolContext * aProtocolContext,
                             idBool             * aExitFlag,
                             UInt                 aTimeoutSec )
{
    UChar sOpCode;
    UInt sType;

    IDE_TEST( validateA7Protocol( aProtocolContext ) != IDE_SUCCESS );
    
    sType = RP_X_SYNC_END;

    IDE_TEST( checkAndFlush( aHBTResource,
                             aProtocolContext,
                             aExitFlag,
                             1 + 4,
                             ID_TRUE,
                             aTimeoutSec )
              != IDE_SUCCESS );

    sOpCode = CMI_PROTOCOL_OPERATION( RP, SyncEnd );
    CMI_WR1( aProtocolContext, sOpCode );
    CMI_WR4( aProtocolContext, &(sType) );

    IDU_FIT_POINT( "rpnComm::sendSyncEnd::cmiSend::ERR_SEND" );
    IDE_TEST( sendCmBlock( aHBTResource,
                           aProtocolContext, 
                           aExitFlag,
                           ID_TRUE,
                           aTimeoutSec ) 
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpnComm::recvSyncEnd( iduMemAllocator     * /*aAllocator*/,
                             idBool              * /*aExitFlag*/,
                             cmiProtocolContext  * aProtocolContext,
                             rpdXLog             * aXLog,
                             ULong                /*aTimeOutSec*/ )
{
    UInt * sType = (UInt*)&(aXLog->mType);

    IDE_TEST( validateA7Protocol( aProtocolContext ) != IDE_SUCCESS );
    
    /* Get Argument XLog Hdr */
    CMI_RD4( aProtocolContext, sType );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpnComm::readCmProtocol( cmiProtocolContext * aProtocolContext,
                                cmiProtocol        * aProtocol,
                                idBool             * aExitFlag,
                                idBool             * aIsTimeOut,
                                ULong                aTimeoutSec )
{
    ULong i = 0;
    idBool sIsEnd = ID_FALSE;
    
    for ( i = 0; i < aTimeoutSec; i++ )
    {
        IDU_FIT_POINT_RAISE( "rpnComm::readCmProtocol::cmiReadProtocol::ERR_READ",
                              ERR_READ );
        IDU_FIT_POINT_RAISE( "rpnComm::readCmProtocol::cmiReadProtocol::ERR_EXIT",
                              ERR_EXIT );
        if ( cmiReadProtocol( aProtocolContext,
                              aProtocol,
                              &mTV1Sec,
                              &sIsEnd )
             == IDE_SUCCESS )
        {
            break;
        }
        
        IDE_TEST_RAISE( ideGetErrorCode() != cmERR_ABORT_TIMED_OUT,
                        ERR_READ );
        IDE_CLEAR();

        if ( aExitFlag != NULL )
        {
            IDE_TEST_RAISE( *aExitFlag == ID_TRUE, ERR_EXIT );
        }
    }

    IDU_FIT_POINT_RAISE( "rpnComm::readCmProtocol::cmiReadProtocol::ERR_TIMEOUT",
                          ERR_TIMEOUT );
    if ( ( aIsTimeOut != NULL ) && ( i >= aTimeoutSec ) )
    {
        *aIsTimeOut = ID_TRUE;
    }
    else
    {
        IDE_TEST_RAISE( i >= aTimeoutSec, ERR_TIMEOUT );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_READ );
    {
        /* Nothing to do */
    }
    IDE_EXCEPTION( ERR_EXIT );
    {
        IDE_SET( ideSetErrorCode( rpERR_IGNORE_EXIT_FLAG_SET ) );
    }
    IDE_EXCEPTION( ERR_TIMEOUT );
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_TIMEOUT_EXCEED ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpnComm::writeProtocol( void                 * /*aHBTResource*/,
                               cmiProtocolContext   * aProtocolContext,
                               cmiProtocol          * aProtocol)
{
    cmiLink * sLink = NULL;

    IDE_TEST( cmiGetLinkForProtocolContext( aProtocolContext,
                                            &sLink )
              != IDE_SUCCESS );

    if( cmiIsLinkBlockingMode( sLink ) == ID_TRUE )
    {
        /*blocking socket*/
        IDE_TEST_RAISE( cmiWriteProtocol( aProtocolContext,
                                          aProtocol )
                        != IDE_SUCCESS, ERR_WRITE );
    }
    else
    {
        if ( cmiWriteProtocol( aProtocolContext,
                               aProtocol,
                               &mTV1Sec ) 
             != IDE_SUCCESS)
        {
            /* check cmiWriteProtocol error ( which is adding pending list ) */
            IDE_TEST_RAISE( cmiIsResendBlock( aProtocolContext )
                            != ID_TRUE, ERR_NETWORK );
        }
        else
        {
            /* do nothing */
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_WRITE );
    {
        IDE_ERRLOG( IDE_RP_0 );
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RP_SENDER_SEND_ERROR ) );
    }
    IDE_EXCEPTION( ERR_NETWORK );
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RP_SENDER_SEND_ERROR ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpnComm::flushPedningBlock( void                 * aHBTResource,
                                   cmiProtocolContext   * aProtocolContext,
                                   idBool               * aExitFlag,
                                   UInt                   aTimeoutSec )
{
    UInt        i = 0;
    idBool      sIsSendSuccess = ID_TRUE;
    cmiLink   * sLink = NULL;

    for ( i = 0; i < aTimeoutSec ; i++ )
    {
        if ( cmiFlushPendingBlock( aProtocolContext, &mTV1Sec ) == IDE_SUCCESS )
        {
            sIsSendSuccess = ID_TRUE;
            break;
        }
        else
        {
            sIsSendSuccess = ID_FALSE;

            if( ideGetErrorCode() != cmERR_ABORT_TIMED_OUT )
            {
                idlOS::sleep( mTV1Sec );
            }

            IDE_CLEAR();

            IDE_TEST( cmiGetLinkForProtocolContext( aProtocolContext,
                                                    &sLink )
                      != IDE_SUCCESS );

            /* check Link Status */
            IDE_TEST_RAISE( isConnected( sLink ) != ID_TRUE, ERR_NETWORK );

            /* check HBT Status */
            if ( aHBTResource != NULL )
            {
                IDE_TEST_RAISE( rpcHBT::checkFault( aHBTResource ) == ID_TRUE,
                                ERR_HBT );
            }

            if ( aExitFlag != NULL )
            {
                IDE_TEST_RAISE( *aExitFlag == ID_TRUE, ERR_EXIT );
            }
        }
    }   /* end for */

    IDE_TEST_RAISE( sIsSendSuccess != ID_TRUE, ERR_TIMEOUT );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_TIMEOUT )
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_SEND_TIMEOUT_EXCEED ) );
    }
    IDE_EXCEPTION( ERR_HBT )
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_HBT_DETECT_PEER_SERVER_ERROR ) );
    }
    IDE_EXCEPTION( ERR_NETWORK )
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RP_SENDER_SEND_ERROR ) );
    }
    IDE_EXCEPTION( ERR_EXIT )
    {
        IDE_SET( ideSetErrorCode( rpERR_IGNORE_EXIT_FLAG_SET ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpnComm::flushProtocol( void               * aHBTResource,
                               cmiProtocolContext * aProtocolContext,
                               idBool             * aExitFlag,
                               idBool               aIsEnd,
                               UInt                 aTimeoutSec )
{
    cmiLink *sLink = NULL;

    IDE_TEST( cmiGetLinkForProtocolContext( aProtocolContext,
                                            &sLink )
              != IDE_SUCCESS );

    if ( cmiIsLinkBlockingMode( sLink ) == ID_TRUE )
    {
        /*blocking socket*/
        IDE_TEST_RAISE( cmiFlushProtocol( aProtocolContext,
                                          aIsEnd,
                                          NULL )
                        != IDE_SUCCESS, ERR_FLUSH );
    }
    else
    {
        /*non-blocking socket*/

        /* Flush Writed Block */
        if ( cmiFlushProtocol( aProtocolContext, aIsEnd, &mTV1Sec ) == IDE_SUCCESS )
        {
            /* do nothing */
        }
        else
        {
            /* check cmiFlushProtocol error ( which is adding pending list ) */
            IDE_TEST_RAISE( cmiIsResendBlock( aProtocolContext )
                            != ID_TRUE, ERR_NETWORK );
        }

        /*
         *  Protocol 의 끝이면 못 보낸 패킷들을 모두다 전송
         */
        if ( aIsEnd == ID_TRUE )
        {
            IDE_TEST( flushPedningBlock( aHBTResource,
                                         aProtocolContext,
                                         aExitFlag,
                                         aTimeoutSec )
                      != IDE_SUCCESS );
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_FLUSH );
    {
        IDE_ERRLOG( IDE_RP_0 );
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RP_SENDER_SEND_ERROR ) );
    }
    IDE_EXCEPTION( ERR_NETWORK )
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RP_SENDER_SEND_ERROR ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpnComm::sendCmBlock( void                * aHBTResource,
                             cmiProtocolContext  * aContext,
                             idBool              * aExitFlag,
                             idBool                aIsEnd,
                             UInt                  aTimeoutSec )
{
    cmiLink         * sLink = NULL;
    idBool            sIsBlockingMode = ID_TRUE;

    /*
     *  Check Blocking and nonBlocking Socket
     */
    IDE_TEST( cmiGetLinkForProtocolContext( aContext,
                                            &sLink )
              != IDE_SUCCESS );

    sIsBlockingMode = cmiIsLinkBlockingMode( sLink );

    if ( sIsBlockingMode == ID_TRUE )
    {
        IDE_TEST_RAISE( cmiSend( aContext, aIsEnd, NULL ) != IDE_SUCCESS, ERR_SEND );
    }
    else
    {
        if ( cmiSend( aContext, aIsEnd, &mTV1Sec ) == IDE_SUCCESS )
        {
            /* do nothing */
        }
        else
        {
            /* check cmiSend error ( which is adding pending list ) */
            IDE_TEST_RAISE( cmiIsResendBlock( aContext ) != ID_TRUE,
                            ERR_NETWORK );
        }

        /*
         *  Protocol 의 끝이면 못 보낸 패킷들을 모두다 전송
         */
        if ( aIsEnd == ID_TRUE )
        {
            IDE_TEST( flushPedningBlock( aHBTResource,
                                         aContext,
                                         aExitFlag,
                                         aTimeoutSec )
                      != IDE_SUCCESS );

        }
        else
        {
            /* do nothing */
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION ( ERR_SEND )
    {
        IDE_ERRLOG (IDE_RP_0 );
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RP_SENDER_SEND_ERROR ) );
    }
    IDE_EXCEPTION( ERR_NETWORK )
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RP_SENDER_SEND_ERROR ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC rpnComm::sendAckOnDML( void               * aHBTResource,
                              cmiProtocolContext * aProtocolContext,
                              idBool             * aExitFlag,
                              UInt                 aTimeoutSec )
{
    UInt sType = RP_X_ACK_ON_DML;
    UChar sOpCode = CMI_PROTOCOL_OPERATION( RP, AckOnDML );
    IDE_TEST( validateA7Protocol( aProtocolContext ) != IDE_SUCCESS );
     
    IDE_TEST( checkAndFlush( aHBTResource,
                             aProtocolContext, 
                             aExitFlag,
                             1 + 4,  
                             ID_TRUE,
                             aTimeoutSec )
              != IDE_SUCCESS );
 
    /* Replication XLog Header Set */
    CMI_WR1( aProtocolContext, sOpCode );
    CMI_WR4( aProtocolContext, &(sType) );
      
    IDE_TEST( sendCmBlock( aHBTResource,
                           aProtocolContext, 
                           aExitFlag,
                           ID_TRUE,
                           aTimeoutSec ) 
              != IDE_SUCCESS );
 
    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
 
    return IDE_FAILURE;
}

IDE_RC rpnComm::checkAndFlush( void                    * aHBTResource,
                               cmiProtocolContext      * aProtocolContext,
                               idBool                  * aExitFlag,
                               UInt                      aLen,
                               idBool                    aIsEnd,
                               UInt                      aTimeoutSec )
{
    UInt    sRemainSpaceInCmBlock = 0;;

    sRemainSpaceInCmBlock = CMI_REMAIN_SPACE_IN_WRITE_BLOCK( aProtocolContext );

    if ( sRemainSpaceInCmBlock > aLen )
    {
        /* do nothing */
    }
    else
    {
        IDE_TEST( sendCmBlock( aHBTResource,
                               aProtocolContext,
                               aExitFlag,
                               aIsEnd,
                               aTimeoutSec )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpnComm::validateA7Protocol( cmiProtocolContext * aProtocolContext )
{
#if defined(DEBUG)
    cmpPacketType sPacketType;
    
    sPacketType = cmiGetPacketType( aProtocolContext );

    switch ( sPacketType )
    {
        case CMP_PACKET_TYPE_A7:
            /* nothing to do */
            break;
            
        case CMP_PACKET_TYPE_A5:
        case CMP_PACKET_TYPE_UNKNOWN:
            IDE_RAISE( ERROR_VALIDATION );
            break;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERROR_VALIDATION )
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_INVALID_PACKET_TYPE ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;    
#else
    ACP_UNUSED( aProtocolContext );
    
    return IDE_SUCCESS;
#endif
}

IDE_RC rpnComm::recvOperationInfo( cmiProtocolContext * aProtocolContext,
                                   idBool             * aExitFlag,
                                   UChar              * aOpCode,
                                   ULong                aTimeoutSec )
{
    cmpPacketType sPacketType;

    sPacketType = cmiGetPacketType( aProtocolContext );

    switch ( sPacketType )
    {
        case CMP_PACKET_TYPE_A5:
            *aOpCode = CMI_PROTOCOL_OPERATION( RP, MetaRepl );
            break;

        case CMP_PACKET_TYPE_A7:
            IDE_TEST( recvOperationInfoA7( aProtocolContext,
                                           aExitFlag,
                                           aOpCode,
                                           aTimeoutSec )
                      != IDE_SUCCESS );
            break;

        case CMP_PACKET_TYPE_UNKNOWN:

            IDE_RAISE( UNKNOWN_PACKET_TYPE );
            break;

        default:
            IDE_RAISE( UNKNOWN_PACKET_TYPE );
            break;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( UNKNOWN_PACKET_TYPE )
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_INVALID_PACKET_TYPE ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpnComm::sendDDLSyncInfo( void                * aHBTResource,
                                 cmiProtocolContext  * aProtocolContext,
                                 idBool              * aExitFlag,
                                 UInt                  aInfoMsgType,
                                 SChar               * aRepName,
                                 SChar               * aUserName,
                                 SChar               * aTableName,
                                 UInt                  aDDLSyncPartInfoCount,
                                 SChar                 aDDLSyncPartInfoNames[][QC_MAX_OBJECT_NAME_LEN + 1],
                                 UInt                  aDDLEnableLevel,
                                 UInt                  aSQLMsgType,
                                 SChar               * aSql,
                                 UInt                  aSqlLen,
                                 ULong                 aTimeout )

{
    UChar sOpCode;
    UInt  i                    = 0;
    UInt  sRepNameLen          = 0;
    UInt  sUserNameLen         = 0;
    UInt  sTableNameLen        = 0;
    UInt  sPartTableNameLenSum = 0;
    UInt  sPartTableNameLen    = 0;

    sRepNameLen       = idlOS::strlen( aRepName );
    sUserNameLen      = idlOS::strlen( aUserName );
    sTableNameLen     = idlOS::strlen( aTableName );

    for ( i = 0; i < aDDLSyncPartInfoCount; i++ )
    {
        sPartTableNameLenSum += idlOS::strlen( aDDLSyncPartInfoNames[i] );
    }

    IDE_TEST( validateA7Protocol( aProtocolContext ) != IDE_SUCCESS );

    IDE_TEST( checkAndFlush( aHBTResource,
                             aProtocolContext,
                             aExitFlag,
                             1 + 4 + 
                             4 + sRepNameLen +
                             4 + sUserNameLen + 
                             4 + sTableNameLen +
                             4 + 4 * aDDLSyncPartInfoCount + sPartTableNameLenSum + 
                             4,   
                             ID_TRUE,
                             aTimeout )
              != IDE_SUCCESS );

    sOpCode = CMI_PROTOCOL_OPERATION( RP, DDLSyncInfo );
    CMI_WR1( aProtocolContext, sOpCode );
    CMI_WR4( aProtocolContext, (UInt*)&aInfoMsgType ); /* sub op code */

    CMI_WR4( aProtocolContext, &sRepNameLen );
    CMI_WCP( aProtocolContext, (UChar *)aRepName, sRepNameLen );

    CMI_WR4( aProtocolContext, &sUserNameLen );
    CMI_WCP( aProtocolContext, (UChar *)aUserName, sUserNameLen );

    CMI_WR4( aProtocolContext, &sTableNameLen );
    CMI_WCP( aProtocolContext, (UChar *)aTableName, sTableNameLen );

    CMI_WR4( aProtocolContext, &aDDLSyncPartInfoCount );
    for ( i = 0; i < aDDLSyncPartInfoCount; i++ )
    {
        sPartTableNameLen = idlOS::strlen( aDDLSyncPartInfoNames[i] );

        IDE_DASSERT( sPartTableNameLen > 0 );
        CMI_WR4( aProtocolContext, &( sPartTableNameLen ) );
        CMI_WCP( aProtocolContext, (UChar *)aDDLSyncPartInfoNames[i], sPartTableNameLen );
    }

    CMI_WR4( aProtocolContext, &aDDLEnableLevel );

    IDE_TEST( sendDDLSyncSQL( aHBTResource,
                              aProtocolContext,
                              aExitFlag,
                              aSQLMsgType,
                              aSqlLen,
                              aSql,
                              aTimeout )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

IDE_RC rpnComm::recvDDLSyncInfo( cmiProtocolContext  * aProtocolContext,
                                 idBool              * aExitFlag,
                                 UInt                  aInfoMsgType,
                                 SChar               * aRepName,
                                 SChar               * aUserName,
                                 SChar               * aTableName,
                                 UInt                  aDDLSyncMaxPartInfoCount,
                                 UInt                * aDDLSyncPartInfoCount,
                                 UInt                  aDDLSyncMaxPartNameLen,
                                 SChar                 aDDLSyncPartInfoNames[][QC_MAX_OBJECT_NAME_LEN + 1],
                                 UInt                * aDDLEnableLevel,
                                 UInt                  aSQLMsgType,
                                 SChar              ** aSql,
                                 ULong                 aRecvTimeout )

{
    UChar   sOpCode;
    SChar * sSql              = NULL;
    UInt    i                 = 0;
    UInt    sMsgType          = 0;
    UInt    sRepNameLen       = 0;
    UInt    sUserNameLen      = 0;
    UInt    sTableNameLen     = 0;
    UInt    sPartTableNameLen = 0;
    SChar   sErrMsg[RP_MAX_MSG_LEN + 1] = { 0, };

    if ( CMI_IS_READ_ALL( aProtocolContext ) == ID_TRUE )
    {
        IDE_TEST( readCmBlock( NULL,
                               aProtocolContext,
                               aExitFlag,
                               NULL /* TimeoutFlag */,
                               aRecvTimeout )
                  != IDE_SUCCESS );
    }
    else
    {
        /* nothing to do */
    }

    /* Check Operation Type */
    CMI_RD1( aProtocolContext, sOpCode );
    IDE_TEST_RAISE( sOpCode != CMI_PROTOCOL_OPERATION( RP, DDLSyncInfo ),
                    ERR_CHECK_OPERATION_TYPE );
    CMI_RD4( aProtocolContext, (UInt*)&sMsgType );
    IDE_TEST_RAISE( sMsgType != aInfoMsgType, ERR_CHECK_MSG_TYPE );

    CMI_RD4( aProtocolContext, &( sRepNameLen ) );
    CMI_RCP( aProtocolContext, (UChar *)aRepName, sRepNameLen );
    aRepName[sRepNameLen] = '\0';

    CMI_RD4( aProtocolContext, &( sUserNameLen ) );
    CMI_RCP( aProtocolContext, (UChar *)aUserName, sUserNameLen );
    aUserName[sUserNameLen] = '\0';

    CMI_RD4( aProtocolContext, &( sTableNameLen ) );
    CMI_RCP( aProtocolContext, (UChar *)aTableName, sTableNameLen );
    aTableName[sTableNameLen] = '\0';

    CMI_RD4( aProtocolContext, aDDLSyncPartInfoCount );
    IDU_FIT_POINT_RAISE( "rpnComm::recvDDLSyncInfo::ERR_TOO_MANY_PART_INFO", ERR_TOO_MANY_PART_INFO );
    IDE_TEST_RAISE( *aDDLSyncPartInfoCount > aDDLSyncMaxPartInfoCount, ERR_TOO_MANY_PART_INFO );

    for( i = 0; i < *aDDLSyncPartInfoCount; i++ )
    {
        CMI_RD4( aProtocolContext, &( sPartTableNameLen ) );
        IDE_DASSERT( sPartTableNameLen > 0 );

        IDU_FIT_POINT_RAISE( "rpnComm::recvDDLSyncInfo::ERR_TOO_LONG_PART_NAME", ERR_TOO_LONG_PART_NAME );
        IDE_TEST_RAISE( sPartTableNameLen > aDDLSyncMaxPartNameLen, ERR_TOO_LONG_PART_NAME );

        CMI_RCP( aProtocolContext, (UChar *)( aDDLSyncPartInfoNames[i] ), sPartTableNameLen );
        aDDLSyncPartInfoNames[i][sPartTableNameLen] = '\0';
    }

    CMI_RD4( aProtocolContext, aDDLEnableLevel );

    IDE_TEST( recvDDLSyncSQL( aProtocolContext,
                              aExitFlag,
                              aSQLMsgType,
                              &sSql,
                              aRecvTimeout )
              != IDE_SUCCESS );

    *aSql = sSql;

    return IDE_SUCCESS;
    
    IDE_EXCEPTION( ERR_TOO_MANY_PART_INFO );
    {
        idlOS::snprintf( sErrMsg, 
                         RP_MAX_MSG_LEN,
                         "[%s] Too many received partition count",
                         aRepName ); 

        IDE_SET( ideSetErrorCode( rpERR_ABORT_RP_INTERNAL_ARG, sErrMsg ) );
    }
    IDE_EXCEPTION( ERR_TOO_LONG_PART_NAME );
    {
        idlOS::snprintf( sErrMsg, 
                         RP_MAX_MSG_LEN,
                         "[%s] Too long received partition name",
                         aRepName ); 

        IDE_SET( ideSetErrorCode( rpERR_ABORT_RP_INTERNAL_ARG, sErrMsg ) );
    }
    IDE_EXCEPTION( ERR_CHECK_OPERATION_TYPE );
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RP_WRONG_OPERATION_TYPE, sOpCode ) );
    }
    IDE_EXCEPTION( ERR_CHECK_MSG_TYPE );
    {
        idlOS::snprintf( sErrMsg, 
                         RP_MAX_MSG_LEN,
                         "[%s] Invalid message type of replication DDL SYNC protocol. "
                         "(Receive type [%"ID_UINT32_FMT"] expected type [%"ID_UINT32_FMT"])", 
                         aRepName,
                         sMsgType,
                         aInfoMsgType ); 

        IDE_SET( ideSetErrorCode( rpERR_ABORT_RP_INTERNAL_ARG, sErrMsg ) );
    }
    IDE_EXCEPTION_END;

    IDE_PUSH();

    if ( sSql != NULL )
    {
        (void)iduMemMgr::free( sSql );
        sSql = NULL;
    }
    else
    {
        /* nothing to do */
    }

    IDE_POP();

    return IDE_FAILURE;
}

IDE_RC rpnComm::sendDDLSyncSQL(  void               * aHBTResource,
                                 cmiProtocolContext * aProtocolContext,
                                 idBool             * aExitFlag,
                                 UInt                 aMsgType,
                                 UInt                 aSqlLen,
                                 SChar              * aSql,
                                 ULong                aTimeout )
{
    UChar sOpCode;

    IDE_TEST( checkAndFlush( aHBTResource,
                             aProtocolContext,
                             aExitFlag,
                             1 + 4 + 4 + aSqlLen,
                             ID_TRUE,
                             aTimeout )
              != IDE_SUCCESS );

    sOpCode = CMI_PROTOCOL_OPERATION( RP, DDLSyncInfo );
    CMI_WR1( aProtocolContext, sOpCode );
    CMI_WR4( aProtocolContext, (UInt*)&aMsgType ); /* sub op code */

    CMI_WR4( aProtocolContext, &aSqlLen );
    if ( aSqlLen > 0 )
    {
        CMI_WCP( aProtocolContext, (UChar *)aSql, aSqlLen );
    }
    else
    {
        /* nothing to do */
    }

    IDE_TEST( sendCmBlock( aHBTResource,
                           aProtocolContext,
                           aExitFlag,
                           ID_TRUE,
                           aTimeout )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpnComm::recvDDLSyncSQL( cmiProtocolContext  * aProtocolContext,
                                idBool              * aExitFlag,
                                UInt                  aMsgType,
                                SChar              ** aSql,
                                ULong                 aRecvTimeout )
{
    UChar   sOpCode;
    UInt    sMsgType = 0;
    UInt    sSqlLen  = 0;
    SChar * sSql     = NULL;
    SChar   sErrMsg[RP_MAX_MSG_LEN + 1] = { 0, };

    if ( CMI_IS_READ_ALL( aProtocolContext ) == ID_TRUE )
    {
        IDE_TEST( readCmBlock( NULL,
                               aProtocolContext,
                               aExitFlag,
                               NULL /* TimeoutFlag */,
                               aRecvTimeout )
                  != IDE_SUCCESS );
    }
    else
    {
        /* nothing to do */
    }

    CMI_RD1( aProtocolContext, sOpCode );
    IDE_TEST_RAISE( sOpCode != CMI_PROTOCOL_OPERATION( RP, DDLSyncInfo ),
                    ERR_CHECK_OPERATION_TYPE );
    CMI_RD4( aProtocolContext, (UInt*)&sMsgType );
    IDE_TEST_RAISE( sMsgType != aMsgType, ERR_CHECK_MSG_TYPE );

    CMI_RD4( aProtocolContext, &sSqlLen );
    if ( sSqlLen > 0 )
    {
        IDU_FIT_POINT( "rpnComm::recvDDLSyncSQL::malloc::sSql",
                       rpERR_ABORT_MEMORY_ALLOC,
                       "rpnComm::recvDDLSyncSQL",
                       "sSql" ); 
        IDE_TEST( iduMemMgr::malloc( IDU_MEM_RP_RPN,
                                     sSqlLen + 1,
                                     (void**)&( sSql ),
                                     IDU_MEM_IMMEDIATE )
                 != IDE_SUCCESS );

        CMI_RCP( aProtocolContext, (UChar *)sSql, sSqlLen );
        sSql[sSqlLen] = '\0';
    }
    else
    {
        /* nothing to do */
    }

    *aSql = sSql;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_CHECK_OPERATION_TYPE );
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RP_WRONG_OPERATION_TYPE, sOpCode ) );
    }
    IDE_EXCEPTION( ERR_CHECK_MSG_TYPE );
    {
        idlOS::snprintf( sErrMsg, 
                         RP_MAX_MSG_LEN,
                         "Invalid message type of replication DDL SYNC protocol. (Receive type [%u] expected type [%u])",
                         sMsgType,
                         aMsgType ); 

        IDE_SET( ideSetErrorCode( rpERR_ABORT_RP_INTERNAL_ARG, sErrMsg ) );
    }
    IDE_EXCEPTION_END;

    if ( sSql != NULL )
    {
        (void)iduMemMgr::free( sSql );
        sSql = NULL;
    }
    else
    {
        /* nothing to do */
    }

    return IDE_FAILURE;
}

IDE_RC rpnComm::sendDDLSyncInfoAck( void               * aHBTResource,
                                    cmiProtocolContext * aProtocolContext,
                                    idBool             * aExitFlag,
                                    UInt                 aMsgType,
                                    UInt                 aResult,
                                    SChar              * aErrMsg,
                                    ULong                aTimeout )
{
    UChar sOpCode;
    UInt  sErrMsgLen  = 0;
    UInt  sSendMsgLen = 0;

    if ( aErrMsg != NULL )
    {
        sErrMsgLen = idlOS::strlen( aErrMsg );
    }
    else
    {
        sErrMsgLen = 0;
    }

    if ( sErrMsgLen > RP_MAX_MSG_LEN )
    {
        sSendMsgLen = RP_MAX_MSG_LEN;
    }
    else
    {
        sSendMsgLen = sErrMsgLen;
    }

    IDE_TEST( checkAndFlush( aHBTResource,
                             aProtocolContext,
                             aExitFlag,
                             1 + 4 + 4 + 4 + sSendMsgLen,
                             ID_TRUE,
                             aTimeout )
              != IDE_SUCCESS );

    sOpCode = CMI_PROTOCOL_OPERATION( RP, DDLSyncMsgAck );
    CMI_WR1( aProtocolContext, sOpCode );
    CMI_WR4( aProtocolContext, (UInt*)&aMsgType ); /* sub op code */

    CMI_WR4( aProtocolContext, &aResult );

    CMI_WR4( aProtocolContext, &sSendMsgLen );
    if ( sSendMsgLen > 0 )
    {
        CMI_WCP( aProtocolContext, (UChar *)aErrMsg, sSendMsgLen );
    }
    else
    {
        /* nothing to do */
    }

    IDE_TEST( sendCmBlock( aHBTResource,
                           aProtocolContext,
                           aExitFlag,
                           ID_TRUE,
                           aTimeout )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpnComm::recvDDLSyncInfoAck( cmiProtocolContext * aProtocolContext,
                                    idBool             * aExitFlag,
                                    UInt                 aMsgType,
                                    UInt               * aResult,
                                    SChar              * aErrMsg,
                                    ULong                aRecvTimeout )
{
    UChar sOpCode;    
    UInt  sMsgType       = 0;
    UInt  sErrMsgLen     = 0;
    UInt  sErrMsgRecvLen = 0;
    SChar sErrMsg[RP_MAX_MSG_LEN + 1] = { 0, };

    if ( CMI_IS_READ_ALL( aProtocolContext ) == ID_TRUE )
    {
        IDE_TEST( readCmBlock( NULL,
                               aProtocolContext,
                               aExitFlag,
                               NULL /* TimeoutFlag */,
                               aRecvTimeout )
                  != IDE_SUCCESS );
    }
    else
    {
        /* nothing to do */
    }

    CMI_RD1( aProtocolContext, sOpCode );
    IDE_TEST_RAISE( sOpCode != CMI_PROTOCOL_OPERATION( RP, DDLSyncMsgAck ), ERR_CHECK_OPERATION_TYPE );
    CMI_RD4( aProtocolContext, (UInt*)&sMsgType );
    IDE_TEST_RAISE( sMsgType != aMsgType, ERR_CHECK_MSG_TYPE );

    CMI_RD4( aProtocolContext, aResult );

    CMI_RD4( aProtocolContext, &( sErrMsgRecvLen ) );
    if ( sErrMsgRecvLen > 0 )
    {
        /* 서로 다른 버전일 경우 MSG_LENGTH 가 다를 수 있어 죽을 수 있기 때문에
         * 받고나서도 한번 더 확인한다. */
        if ( sErrMsgRecvLen > RP_MAX_MSG_LEN )
        {
            sErrMsgLen = RP_MAX_MSG_LEN;
        }
        else
        {
            sErrMsgLen = sErrMsgRecvLen;
        }

        CMI_RCP( aProtocolContext, (UChar *)aErrMsg, sErrMsgLen );
        aErrMsg[sErrMsgLen] = '\0';

        if ( sErrMsgRecvLen > sErrMsgLen )
        {
            /* 나머지는 Skip */
            CMI_SKIP_READ_BLOCK( aProtocolContext, sErrMsgRecvLen - sErrMsgLen );
        }
        else
        {
            /* nothing to do */
        }
    }
    else
    {
        aErrMsg[0] = '\0';
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_CHECK_OPERATION_TYPE );
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RP_WRONG_OPERATION_TYPE, sOpCode ) );
    }
    IDE_EXCEPTION( ERR_CHECK_MSG_TYPE );
    {
        idlOS::snprintf( sErrMsg, 
                         RP_MAX_MSG_LEN,
                         "Invalid message type of replication DDL SYNC protocol. "
                         "(Receive type [%"ID_UINT32_FMT"] expected type [%"ID_UINT32_FMT"])", 
                         sMsgType,
                         aMsgType ); 

        IDE_SET( ideSetErrorCode( rpERR_ABORT_RP_INTERNAL_ARG, sErrMsg ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpnComm::sendDDLSyncMsg( void               * aHBTResource,
                                cmiProtocolContext * aProtocolContext,
                                idBool             * aExitFlag,
                                UInt                 aMsgType,
                                ULong                aTimeout )
{
    UChar sOpCode;

    IDE_TEST( checkAndFlush( aHBTResource,
                             aProtocolContext,
                             aExitFlag,
                             1 + 4,
                             ID_TRUE,
                             aTimeout )
              != IDE_SUCCESS );

    sOpCode = CMI_PROTOCOL_OPERATION( RP, DDLSyncMsg );
    CMI_WR1( aProtocolContext, sOpCode );
    CMI_WR4( aProtocolContext, (UInt*)&aMsgType );

    IDE_TEST( sendCmBlock( aHBTResource,
                           aProtocolContext,
                           aExitFlag,
                           ID_TRUE,
                           aTimeout )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC rpnComm::recvDDLSyncMsg( cmiProtocolContext * aProtocolContext,
                                idBool             * aExitFlag,
                                UInt                 aMsgType,
                                ULong                aRecvTimeout )
{
    UChar sOpCode;
    UInt  sMsgType = 0;
    SChar sErrMsg[RP_MAX_MSG_LEN + 1] = { 0, };

    if ( CMI_IS_READ_ALL( aProtocolContext ) == ID_TRUE )
    {
        IDE_TEST( readCmBlock( NULL,
                               aProtocolContext,
                               aExitFlag,
                               NULL /* TimeoutFlag */,
                               aRecvTimeout )
                  != IDE_SUCCESS );
    }
    else
    {
        /* nothing to do */
    }

    CMI_RD1( aProtocolContext, sOpCode );
    IDE_TEST_RAISE( sOpCode != CMI_PROTOCOL_OPERATION( RP, DDLSyncMsg ), ERR_CHECK_OPERATION_TYPE );
    CMI_RD4( aProtocolContext, (UInt*)&sMsgType );
    IDE_TEST_RAISE( sMsgType != aMsgType, ERR_CHECK_MSG_TYPE );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_CHECK_OPERATION_TYPE );
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RP_WRONG_OPERATION_TYPE, sOpCode ) );
    }
    IDE_EXCEPTION( ERR_CHECK_MSG_TYPE );
    {
        idlOS::snprintf( sErrMsg, 
                         RP_MAX_MSG_LEN,
                         "Invalid message type of replication DDL SYNC protocol. "
                         "(Receive type [%"ID_UINT32_FMT"] expected type [%"ID_UINT32_FMT"])", 
                         sMsgType,
                         aMsgType ); 

        IDE_SET( ideSetErrorCode( rpERR_ABORT_RP_INTERNAL_ARG, sErrMsg ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpnComm::sendDDLSyncMsgAck( void               * aHBTResource,
                                   cmiProtocolContext * aProtocolContext,
                                   idBool             * aExitFlag,
                                   UInt                 aMsgType,
                                   UInt                 aResult,
                                   SChar              * aErrMsg,
                                   ULong                aTimeout )
{
    UChar sOpCode;
    UInt  sErrMsgLen  = 0;    
    UInt  sSendMsgLen = 0;

    if ( aErrMsg != NULL )
    {
        sErrMsgLen = idlOS::strlen( aErrMsg );
    }
    else
    {
        sErrMsgLen = 0;
    }

    if ( sErrMsgLen > RP_MAX_MSG_LEN )
    {
        sSendMsgLen = RP_MAX_MSG_LEN;
    }
    else
    {
        sSendMsgLen = sErrMsgLen;
    }

    IDE_TEST( checkAndFlush( aHBTResource,
                             aProtocolContext,
                             aExitFlag,
                             1 + 4 +
                             4 +
                             4 + sSendMsgLen,
                             ID_TRUE,
                             aTimeout )
              != IDE_SUCCESS );

    sOpCode = CMI_PROTOCOL_OPERATION( RP, DDLSyncMsgAck );
    CMI_WR1( aProtocolContext, sOpCode );
    CMI_WR4( aProtocolContext, (UInt*)&aMsgType ); /* sub op code */
 
    CMI_WR4( aProtocolContext, &aResult );

    CMI_WR4( aProtocolContext, (UInt*)&sSendMsgLen ); /* sub op code */
    if ( sSendMsgLen > 0 )
    {
        CMI_WCP( aProtocolContext, (UChar *)aErrMsg, sSendMsgLen );
    }
    else
    {
        /* nothing to do */
    }

    IDE_TEST( sendCmBlock( aHBTResource,
                           aProtocolContext,
                           aExitFlag,
                           ID_TRUE,
                           aTimeout )
              != IDE_SUCCESS );


    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpnComm::recvDDLSyncMsgAck( cmiProtocolContext * aProtocolContext,
                                   idBool             * aExitFlag,
                                   UInt                 aMsgType,
                                   UInt               * aResult,
                                   SChar              * aErrMsg,
                                   ULong                aRecvTimeout )
{
    UChar sOpCode;    
    UInt  sMsgType       = 0;
    UInt  sErrMsgLen     = 0;
    UInt  sErrMsgRecvLen = 0;
    SChar sErrMsg[RP_MAX_MSG_LEN + 1] = { 0, };

    if ( CMI_IS_READ_ALL( aProtocolContext ) == ID_TRUE )
    {
        IDE_TEST( readCmBlock( NULL,
                               aProtocolContext,
                               aExitFlag,
                               NULL /* TimeoutFlag */,
                               aRecvTimeout )
                  != IDE_SUCCESS );
    }
    else
    {
        /* nothing to do */
    }

    CMI_RD1( aProtocolContext, sOpCode );
    IDE_TEST_RAISE( sOpCode != CMI_PROTOCOL_OPERATION( RP, DDLSyncMsgAck ), ERR_CHECK_OPERATION_TYPE );

    CMI_RD4( aProtocolContext, (UInt*)&sMsgType );
    IDE_TEST_RAISE( sMsgType != aMsgType, ERR_CHECK_MSG_TYPE );

    CMI_RD4( aProtocolContext, aResult );

    CMI_RD4( aProtocolContext, &( sErrMsgRecvLen ) );
    if ( sErrMsgRecvLen > 0 )
    {
        /* 서로 다른 버전일 경우 MSG_LENGTH 가 다를 수 있어 죽을 수 있기 때문에
         * 받고나서도 한번 더 확인한다. */
        if ( sErrMsgRecvLen > RP_MAX_MSG_LEN )
        {
            sErrMsgLen = RP_MAX_MSG_LEN;
        }
        else
        {
            sErrMsgLen = sErrMsgRecvLen;
        }

        CMI_RCP( aProtocolContext, (UChar *)aErrMsg, sErrMsgLen );
        aErrMsg[sErrMsgLen] = '\0';

        if ( sErrMsgRecvLen > sErrMsgLen )
        {
            /* 나머지는 Skip */
            CMI_SKIP_READ_BLOCK( aProtocolContext, sErrMsgRecvLen - sErrMsgLen );
        }
        else
        {
            /* nothing to do */
        }
    }
    else
    {
        aErrMsg[0] = '\0';
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_CHECK_OPERATION_TYPE );
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RP_WRONG_OPERATION_TYPE, sOpCode ) );
    }
    IDE_EXCEPTION( ERR_CHECK_MSG_TYPE );
    {
        idlOS::snprintf( sErrMsg, 
                         RP_MAX_MSG_LEN,
                         "Invalid message type of replication DDL SYNC protocol. "
                         "(Receive type [%"ID_UINT32_FMT"] expected type [%"ID_UINT32_FMT"])", 
                         sMsgType,
                         aMsgType ); 

        IDE_SET( ideSetErrorCode( rpERR_ABORT_RP_INTERNAL_ARG, sErrMsg ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpnComm::sendDDLSyncCancel( void               * aHBTResource,
                                   cmiProtocolContext * aProtocolContext,
                                   idBool             * aExitFlag,
                                   SChar              * aRepName,
                                   ULong                aTimeout )
{
    UChar sOpCode;
    UInt  sRepNameLen = 0;

    sOpCode = CMI_PROTOCOL_OPERATION( RP, DDLSyncCancel );
    sRepNameLen = idlOS::strlen( aRepName );

    IDE_TEST( checkAndFlush( aHBTResource,
                             aProtocolContext,
                             aExitFlag,
                             1 + 
                             4 + sRepNameLen,
                             ID_TRUE,
                             aTimeout )
              != IDE_SUCCESS );

    CMI_WR1( aProtocolContext, sOpCode );

    CMI_WR4( aProtocolContext, &sRepNameLen );
    CMI_WCP( aProtocolContext, (UChar*)aRepName, sRepNameLen );

    IDE_TEST( sendCmBlock( aHBTResource,
                           aProtocolContext,
                           aExitFlag,
                           ID_TRUE,
                           aTimeout )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpnComm::recvDDLSyncCancel( cmiProtocolContext * aProtocolContext, 
                                   idBool             * aExitFlag,
                                   SChar              * aRepName,
                                   ULong                aRecvTimeout )
{
    UChar sOpCode;
    UInt  sRepNameLen = 0;

    if ( CMI_IS_READ_ALL( aProtocolContext ) == ID_TRUE )
    {
        IDE_TEST( readCmBlock( NULL,
                               aProtocolContext,
                               aExitFlag,
                               NULL /* TimeoutFlag */,
                               aRecvTimeout )
                  != IDE_SUCCESS );
    }
    else
    {
        /* nothing to do */
    }

    CMI_RD1( aProtocolContext, sOpCode );
    IDE_TEST_RAISE( sOpCode != CMI_PROTOCOL_OPERATION( RP, DDLSyncCancel ), ERR_CHECK_OPERATION_TYPE );

    CMI_RD4( aProtocolContext, &sRepNameLen );
    if ( sRepNameLen > 0 )
    {
        CMI_RCP( aProtocolContext, (UChar*)aRepName, sRepNameLen );
    }
    else
    {
        /* nothing to do */
    }
    aRepName[sRepNameLen] = '\0';

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_CHECK_OPERATION_TYPE );
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RP_WRONG_OPERATION_TYPE, sOpCode ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpnComm::sendDDLASyncStart( void               * aHBTResource,
                                   cmiProtocolContext * aProtocolContext,
                                   idBool             * aExitFlag,
                                   UInt                 aType,
                                   ULong                aSendTimeout )
{
    IDE_TEST_RAISE( rpuProperty::isUseV6Protocol() == ID_TRUE, ERR_NOT_SUPPORT );

    IDE_TEST( sendDDLASyncStartA7( aHBTResource,
                                   aProtocolContext,
                                   aExitFlag,
                                   aType,
                                   aSendTimeout )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_SUPPORT );
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RP_INTERNAL_ARG, "DDL ASynchronization only support A7") );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpnComm::sendDDLASyncStartAck( cmiProtocolContext * aProtocolContext,
                                      idBool             * aExitFlag,
                                      UInt                 aType,
                                      ULong                aSendTimeout )
{
    IDE_TEST_RAISE( rpuProperty::isUseV6Protocol() == ID_TRUE, ERR_NOT_SUPPORT );

    IDE_TEST( sendDDLASyncStartAckA7( aProtocolContext,
                                      aExitFlag,
                                      aType,
                                      aSendTimeout )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_SUPPORT );
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RP_INTERNAL_ARG, "DDL ASynchronization only support A7") );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpnComm::recvDDLASyncStartAck( cmiProtocolContext * aProtocolContext,
                                      idBool             * aExitFlag,
                                      UInt               * aType,
                                      ULong                aRecvTimeout )
{
    cmpPacketType sPacketType;

    sPacketType = cmiGetPacketType( aProtocolContext );

    IDE_TEST_RAISE( sPacketType != CMP_PACKET_TYPE_A7, ERR_NOT_SUPPORT );

    IDE_TEST( recvDDLASyncStartAckA7( aProtocolContext,
                                      aExitFlag,
                                      aType,
                                      aRecvTimeout )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_SUPPORT );
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RP_INTERNAL_ARG, "DDL ASynchronization only support A7") );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpnComm::sendDDLASyncExecute( void               * aHBTResource,
                                     cmiProtocolContext * aProtocolContext,
                                     idBool             * aExitFlag,
                                     UInt                 aType,
                                     SChar              * aUserName,
                                     UInt                 aDDLEnableLevel,
                                     UInt                 aTargetCount,
                                     SChar              * aTargetTableName,
                                     SChar              * aTargetPartNames,
                                     smSN                 aDDLCommitSN,
                                     rpdVersion         * aReplVersion,
                                     SChar              * aDDLStmt,
                                     ULong                aSendTimeout )


{
    IDE_TEST_RAISE( rpuProperty::isUseV6Protocol() == ID_TRUE, ERR_NOT_SUPPORT );

    IDE_TEST( sendDDLASyncExecuteA7( aHBTResource,
                                     aProtocolContext,
                                     aExitFlag,
                                     aType,
                                     aUserName,
                                     aDDLEnableLevel,
                                     aTargetCount,
                                     aTargetTableName,
                                     aTargetPartNames,
                                     aDDLCommitSN,
                                     aReplVersion,
                                     aDDLStmt,
                                     aSendTimeout )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_SUPPORT );
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RP_INTERNAL_ARG, "DDL ASynchronization only support A7") );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

IDE_RC rpnComm::recvDDLASyncExecute( cmiProtocolContext  * aProtocolContext,
                                     idBool              * aExitFlag,
                                     UInt                * aType,
                                     SChar               * aUserName,
                                     UInt                * aDDLEnableLevel,
                                     UInt                * aTargetCount,
                                     SChar               * aTargetTableName,
                                     SChar              ** aTargetPartNames,
                                     smSN                * aDDLCommitSN,
                                     rpdVersion          * aReplVersion,
                                     SChar              ** aDDLStmt,
                                     ULong                 aRecvTimeout )
{
    cmpPacketType sPacketType;

    sPacketType = cmiGetPacketType( aProtocolContext );

    IDE_TEST_RAISE( sPacketType != CMP_PACKET_TYPE_A7, ERR_NOT_SUPPORT );

    IDE_TEST( recvDDLASyncExecuteA7( aProtocolContext,
                                     aExitFlag,
                                     aType,
                                     aUserName,
                                     aDDLEnableLevel,
                                     aTargetCount,
                                     aTargetTableName,
                                     aTargetPartNames,
                                     aDDLCommitSN,
                                     aReplVersion,
                                     aDDLStmt,
                                     aRecvTimeout )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_SUPPORT );
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RP_INTERNAL_ARG, "DDL ASynchronization only support A7") );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

IDE_RC rpnComm::sendDDLASyncExecuteAck( cmiProtocolContext * aProtocolContext,
                                        idBool             * aExitFlag,
                                        UInt                 aType,
                                        UInt                 aIsSuccess,
                                        UInt                 aErrCode,
                                        SChar              * aErrMsg,
                                        ULong                aSendTimeout )
{
    IDE_TEST_RAISE( rpuProperty::isUseV6Protocol() == ID_TRUE, ERR_NOT_SUPPORT );

    IDE_TEST( sendDDLASyncExecuteAckA7( aProtocolContext,
                                        aExitFlag,
                                        aType,
                                        aIsSuccess,
                                        aErrCode,
                                        aErrMsg,
                                        aSendTimeout )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_SUPPORT );
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RP_INTERNAL_ARG, "DDL ASynchronization only support A7") );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpnComm::recvDDLASyncExecuteAck( cmiProtocolContext * aProtocolContext,
                                        idBool             * aExitFlag,
                                        UInt               * aType,
                                        UInt               * aIsSuccess,
                                        UInt               * aErrCode,
                                        SChar              * aErrMsg,
                                        ULong                aRecvTimeout )
{
    cmpPacketType sPacketType;

    sPacketType = cmiGetPacketType( aProtocolContext );

    IDU_FIT_POINT_RAISE( "rpnComm::recvDDLASyncExecuteAck::ERR_NOT_SUPPORT", ERR_NOT_SUPPORT );
    IDE_TEST_RAISE( sPacketType != CMP_PACKET_TYPE_A7, ERR_NOT_SUPPORT );

    IDE_TEST( recvDDLASyncExecuteAckA7( aProtocolContext,
                                        aExitFlag,
                                        aType,
                                        aIsSuccess,
                                        aErrCode,
                                        aErrMsg,
                                        aRecvTimeout )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_SUPPORT );
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RP_INTERNAL_ARG, "DDL ASynchronization only support A7") );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

void rpnComm::destroyProtocolContext( cmiProtocolContext * aProtocolContext )
{
    cmiLink * sLink = NULL;

    IDE_DASSERT( aProtocolContext != NULL );

    sLink = (cmiLink*)( aProtocolContext->mLink );

    (void)cmiFreeCmBlock( aProtocolContext );

    (void)cmiShutdownLink( sLink, CMI_DIRECTION_RDWR );
    (void)cmiCloseLink( sLink );

    (void)cmiFreeLink( sLink );
    sLink = NULL;

    (void)iduMemMgr::free( aProtocolContext );
    aProtocolContext = NULL;
}

IDE_RC rpnComm::sendMetaPartitionCount( void               * aHBTResource,
                                        cmiProtocolContext * aProtocolContext,
                                        idBool             * aExitFlag,
                                        UInt               * aCount,
                                        UInt                 aTimeoutSec )
{
    cmpPacketType sPacketType;

    sPacketType = cmiGetPacketType( aProtocolContext );

    switch ( sPacketType )
    {
        case CMP_PACKET_TYPE_A5:
            IDE_RAISE( UNKNOWN_PACKET_TYPE );
            break;

        case CMP_PACKET_TYPE_A7:
            IDE_TEST( sendMetaPartitionCountA7( aHBTResource,
                                                aProtocolContext,
                                                aExitFlag,
                                                aCount,
                                                aTimeoutSec )
                      != IDE_SUCCESS );
            break;
        case CMP_PACKET_TYPE_UNKNOWN:
            IDE_RAISE( UNKNOWN_PACKET_TYPE );
            break;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( UNKNOWN_PACKET_TYPE )
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_INVALID_PACKET_TYPE ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
IDE_RC rpnComm::recvMetaPartitionCount( cmiProtocolContext * aProtocolContext,
                                        idBool             * aExitFlag,
                                        UInt               * aCnt,
                                        ULong                aTimeoutSec )
{
    cmpPacketType sPacketType;

    sPacketType = cmiGetPacketType( aProtocolContext );

    switch ( sPacketType )
    {
        case CMP_PACKET_TYPE_A5:
            IDE_RAISE( UNKNOWN_PACKET_TYPE );
            break;

        case CMP_PACKET_TYPE_A7:
            IDE_TEST( recvMetaPartitionCountA7( aProtocolContext,
                                                aExitFlag,
                                                aCnt,
                                                aTimeoutSec )
                      != IDE_SUCCESS );
            break;
        case CMP_PACKET_TYPE_UNKNOWN:
            IDE_RAISE( UNKNOWN_PACKET_TYPE );
            break;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( UNKNOWN_PACKET_TYPE )
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_INVALID_PACKET_TYPE ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpnComm::sendMetaInitFlag( void               * aHBTResource,
                                  cmiProtocolContext * aProtocolContext,
                                  idBool             * aExitFlag,
                                  idBool               aMetaInitFlag,
                                  UInt                 aTimeoutSec )
{
    cmpPacketType sPacketType;

    sPacketType = cmiGetPacketType( aProtocolContext );

    switch ( sPacketType )
    {
        case CMP_PACKET_TYPE_A5:
            IDE_RAISE( UNKNOWN_PACKET_TYPE );
            break;

        case CMP_PACKET_TYPE_A7:
            IDE_TEST( sendMetaInitFlagA7( aHBTResource,
                                          aProtocolContext,
                                          aExitFlag,
                                          aMetaInitFlag,
                                          aTimeoutSec )
                      != IDE_SUCCESS );
            break;
        case CMP_PACKET_TYPE_UNKNOWN:
            IDE_RAISE( UNKNOWN_PACKET_TYPE );
            break;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( UNKNOWN_PACKET_TYPE )
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_INVALID_PACKET_TYPE ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpnComm::recvMetaInitFlag( cmiProtocolContext * aProtocolContext,
                                  idBool             * aExitFlag,
                                  idBool             * aMetaInitFlag,
                                  ULong                aTimeoutSec )
{
    cmpPacketType sPacketType;

    sPacketType = cmiGetPacketType( aProtocolContext );

    switch ( sPacketType )
    {
        case CMP_PACKET_TYPE_A5:
            IDE_RAISE( UNKNOWN_PACKET_TYPE );
            break;

        case CMP_PACKET_TYPE_A7:
            IDE_TEST( recvMetaInitFlagA7( aProtocolContext,
                                          aExitFlag,
                                          aMetaInitFlag,
                                          aTimeoutSec )
                      != IDE_SUCCESS );
            break;
        case CMP_PACKET_TYPE_UNKNOWN:
            IDE_RAISE( UNKNOWN_PACKET_TYPE );
            break;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( UNKNOWN_PACKET_TYPE )
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_INVALID_PACKET_TYPE ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpnComm::sendConditionInfo( void                 * aHBTResource,
                                   cmiProtocolContext   * aProtocolContext,
                                   idBool               * aExitFlag,
                                   rpdConditionItemInfo * aConditionInfo,
                                   UInt                   aConditionCnt,
                                   UInt                   aTimeoutSec )
{
    cmpPacketType sPacketType;

    sPacketType = cmiGetPacketType( aProtocolContext );

    switch ( sPacketType )
    {
        case CMP_PACKET_TYPE_A5:
            IDE_RAISE( UNKNOWN_PACKET_TYPE );
            break;

        case CMP_PACKET_TYPE_A7:
            IDE_TEST( sendConditionInfoA7( aHBTResource,
                                           aProtocolContext,
                                           aExitFlag,
                                           aConditionInfo,
                                           aConditionCnt,
                                           aTimeoutSec )
                      != IDE_SUCCESS );
            break;
        case CMP_PACKET_TYPE_UNKNOWN:
            IDE_RAISE( UNKNOWN_PACKET_TYPE );
            break;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( UNKNOWN_PACKET_TYPE )
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_INVALID_PACKET_TYPE ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpnComm::recvConditionInfo( cmiProtocolContext    * aProtocolContext,
                                   idBool                * aExitFlag,
                                   rpdConditionItemInfo  * aConditionInfo,
                                   UInt                  * aConditionCnt,
                                   UInt                    aTimeoutSec )
{
    cmpPacketType sPacketType;

    sPacketType = cmiGetPacketType( aProtocolContext );

    switch ( sPacketType )
    {
        case CMP_PACKET_TYPE_A5:
            IDE_RAISE( UNKNOWN_PACKET_TYPE );
            break;

        case CMP_PACKET_TYPE_A7:
            IDE_TEST( recvConditionInfoA7( aProtocolContext,
                                           aExitFlag,
                                           aConditionInfo,
                                           aConditionCnt,
                                           aTimeoutSec )
                      != IDE_SUCCESS );
            break;
        case CMP_PACKET_TYPE_UNKNOWN:
            IDE_RAISE( UNKNOWN_PACKET_TYPE );
            break;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( UNKNOWN_PACKET_TYPE )
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_INVALID_PACKET_TYPE ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpnComm::sendConditionInfoResult( cmiProtocolContext  * aProtocolContext,
                                         idBool              * aExitFlag,
                                         rpdConditionActInfo * aConditionInfo,
                                         UInt                  aConditionCnt,
                                         UInt                  aTimeoutSec )
{
    cmpPacketType sPacketType;

    sPacketType = cmiGetPacketType( aProtocolContext );

    switch ( sPacketType )
    {
        case CMP_PACKET_TYPE_A5:
            IDE_RAISE( UNKNOWN_PACKET_TYPE );
            break;

        case CMP_PACKET_TYPE_A7:
            IDE_TEST( sendConditionInfoResultA7( aProtocolContext,
                                                 aExitFlag,
                                                 aConditionInfo,
                                                 aConditionCnt,
                                                 aTimeoutSec )
                      != IDE_SUCCESS );
            break;
        case CMP_PACKET_TYPE_UNKNOWN:
            IDE_RAISE( UNKNOWN_PACKET_TYPE );
            break;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( UNKNOWN_PACKET_TYPE )
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_INVALID_PACKET_TYPE ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpnComm::recvConditionInfoResult( cmiProtocolContext   * aProtocolContext,
                                         idBool               * aExitFlag,
                                         rpdConditionActInfo  * aConditionInfo,
                                         UInt                   aTimeoutSec )
{
    cmpPacketType sPacketType;

    sPacketType = cmiGetPacketType( aProtocolContext );

    switch ( sPacketType )
    {
        case CMP_PACKET_TYPE_A5:
            IDE_RAISE( UNKNOWN_PACKET_TYPE );
            break;

        case CMP_PACKET_TYPE_A7:
            IDE_TEST( recvConditionInfoResultA7( aProtocolContext,
                                                 aExitFlag,
                                                 aConditionInfo,
                                                 aTimeoutSec )
                      != IDE_SUCCESS );
            break;
        case CMP_PACKET_TYPE_UNKNOWN:
            IDE_RAISE( UNKNOWN_PACKET_TYPE );
            break;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( UNKNOWN_PACKET_TYPE )
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_INVALID_PACKET_TYPE ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpnComm::sendTruncate( void               * aHBTResource,
                              cmiProtocolContext * aProtocolContext,
                              idBool             * aExitFlag,
                              ULong                aTableOID,
                              UInt                 aTimeoutSec )
{
    cmpPacketType sPacketType;

    sPacketType = cmiGetPacketType( aProtocolContext );

    switch ( sPacketType )
    {
        case CMP_PACKET_TYPE_A5:
            IDE_RAISE( UNKNOWN_PACKET_TYPE );
            break;

        case CMP_PACKET_TYPE_A7:
            IDE_TEST( sendTruncateA7( aHBTResource,
                                      aProtocolContext,
                                      aExitFlag,
                                      aTableOID,
                                      aTimeoutSec )
                      != IDE_SUCCESS );
            break;
        case CMP_PACKET_TYPE_UNKNOWN:
            IDE_RAISE( UNKNOWN_PACKET_TYPE );
            break;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( UNKNOWN_PACKET_TYPE )
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_INVALID_PACKET_TYPE ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpnComm::recvTruncate( cmiProtocolContext * aProtocolContext,
                              idBool             * aExitFlag,
                              rpdMeta            * aMeta, 
                              rpdXLog            * aXLog )
{
    cmpPacketType sPacketType;

    sPacketType = cmiGetPacketType( aProtocolContext );

    switch ( sPacketType )
    {
        case CMP_PACKET_TYPE_A5:
            IDE_RAISE( UNKNOWN_PACKET_TYPE );
            break;

        case CMP_PACKET_TYPE_A7:
            IDE_TEST( recvTruncateA7( aProtocolContext,
                                      aExitFlag,
                                      aMeta,
                                      aXLog)
                      != IDE_SUCCESS );
            break;
        case CMP_PACKET_TYPE_UNKNOWN:
            IDE_RAISE( UNKNOWN_PACKET_TYPE );
            break;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( UNKNOWN_PACKET_TYPE )
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_INVALID_PACKET_TYPE ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpnComm::sendXaStartReq( void               * aHBTResource,
                                cmiProtocolContext * aProtocolContext,
                                idBool             * aExitFlag,
                                ID_XID             * aXID,
                                smTID                aTID,
                                smSN                 aSN,
                                ULong                aSendTimeout )
{
    cmpPacketType sPacketType;

    sPacketType = cmiGetPacketType( aProtocolContext );

    switch ( sPacketType )
    {
        case CMP_PACKET_TYPE_A7:
            IDE_TEST( sendXaStartReqA7( aHBTResource,
                                        aProtocolContext,
                                        aExitFlag,
                                        aXID,
                                        aTID,
                                        aSN,
                                        aSendTimeout )
                      != IDE_SUCCESS );
            break;

        case CMP_PACKET_TYPE_A5:
        case CMP_PACKET_TYPE_UNKNOWN:
        default :
            IDE_RAISE( UNKNOWN_PACKET_TYPE );
            break;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( UNKNOWN_PACKET_TYPE )
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_INVALID_PACKET_TYPE ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpnComm::sendXaPrepareReq( void               * aHBTResource,
                                  cmiProtocolContext * aProtocolContext,
                                  idBool             * aExitFlag,
                                  ID_XID             * aXID,
                                  smTID                aTID,
                                  smSN                 aSN,
                                  ULong                aSendTimeout )
{
    cmpPacketType sPacketType;

    sPacketType = cmiGetPacketType( aProtocolContext );

    switch ( sPacketType )
    {
        case CMP_PACKET_TYPE_A7:
            IDE_TEST( sendXaPrepareReqA7( aHBTResource,
                                          aProtocolContext,
                                          aExitFlag,
                                          aXID,
                                          aTID,
                                          aSN,
                                          aSendTimeout )
                      != IDE_SUCCESS );
            break;

        case CMP_PACKET_TYPE_A5:
        case CMP_PACKET_TYPE_UNKNOWN:
        default :
            IDE_RAISE( UNKNOWN_PACKET_TYPE );
            break;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( UNKNOWN_PACKET_TYPE )
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_INVALID_PACKET_TYPE ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC rpnComm::sendXaPrepare( void               * aHBTResource,
                               cmiProtocolContext * aProtocolContext,
                               idBool             * aExitFlag,
                               ID_XID             * aXID,
                               smTID                aTID,
                               smSN                 aSN,
                               ULong                aSendTimeout )
{
    cmpPacketType sPacketType;

    sPacketType = cmiGetPacketType( aProtocolContext );

    switch ( sPacketType )
    {
        case CMP_PACKET_TYPE_A7:
            IDE_TEST( sendXaPrepareA7( aHBTResource,
                                       aProtocolContext,
                                       aExitFlag,
                                       aXID,
                                       aTID,
                                       aSN,
                                       aSendTimeout )
                      != IDE_SUCCESS );
            break;

        case CMP_PACKET_TYPE_A5:
        case CMP_PACKET_TYPE_UNKNOWN:
        default :
            IDE_RAISE( UNKNOWN_PACKET_TYPE );
            break;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( UNKNOWN_PACKET_TYPE )
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_INVALID_PACKET_TYPE ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpnComm::sendXaEnd( void               * aHBTResource,
                           cmiProtocolContext * aProtocolContext,
                           idBool             * aExitFlag,
                           smTID                aTID,
                           smSN                 aSN,
                           ULong                aSendTimeout )
{
    cmpPacketType sPacketType;

    sPacketType = cmiGetPacketType( aProtocolContext );

    switch ( sPacketType )
    {
        case CMP_PACKET_TYPE_A7:
            IDE_TEST( sendXaEndA7( aHBTResource,
                                   aProtocolContext,
                                   aExitFlag,
                                   aTID,
                                   aSN,
                                   aSendTimeout )
                      != IDE_SUCCESS );
            break;

        case CMP_PACKET_TYPE_A5:
        case CMP_PACKET_TYPE_UNKNOWN:
        default :
            IDE_RAISE( UNKNOWN_PACKET_TYPE );
            break;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( UNKNOWN_PACKET_TYPE )
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_INVALID_PACKET_TYPE ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

