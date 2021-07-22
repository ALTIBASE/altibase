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
 * $id$
 **********************************************************************/

#include <dkpProtocolMgr.h>
#include <dkaLinkerProcessMgr.h>

#include <dkErrorCode.h>

#include <dknLink.h>
#include <dktGlobalTxMgr.h>
#include <dktDtxInfo.h>

PDL_Time_Value  dkpProtocolMgr::mTV1Sec;

/**********************************************************************
 * Description : Protocol manager component 를 초기화해준다.
 **********************************************************************/
IDE_RC  dkpProtocolMgr::initializeStatic()
{
    mTV1Sec.initialize(1, 0);

    return IDE_SUCCESS;
}

/**********************************************************************
 * Description : Protocol manager component 를 정리해준다.
 **********************************************************************/
IDE_RC  dkpProtocolMgr::finalizeStatic()
{
    return IDE_SUCCESS;
}

UChar  dkpProtocolMgr::makeHeaderFlags( dksSession         * aSession,
                                        idBool               aIsTxBegin )
{
    UChar            sHdrFlags    = 0x00;

    IDE_DASSERT( aSession != NULL );

    // 항상 Data Session이어야만 한다.
    if ( aSession->mIsXA == ID_TRUE )
    {
        DKP_ADLP_PACKET_FLAGS_SET_XA( sHdrFlags );
    }
    else
    {
        /* do nothing */
    }

    if ( aIsTxBegin == ID_TRUE )
    {
        DKP_ADLP_PACKET_FLAGS_SET_TX_BEGIN( sHdrFlags );
    }
    else
    {
        /* do nothing */
    }

    return sHdrFlags;
}

/**********************************************************************
 * Description : ADLP protocol 을 통해 전송할 패킷의 cm block 에
 *               입력받은 프로토콜 common header 의 값을 setting 해준다.
 *
 * aSession         - [IN] Operation 을 수행하는 세션정보
 * aOpID            - [IN] 수행할 operation 의 id
 * aParam           - [IN] 수행할 operation 과 함께 전달되는 인자
 * aSessionID       - [IN] Operation 을 수행하는 세션의 id
 * aDataLength      - [IN] Operation 과 함께 전달되는 data 의 길이(byte)
 **********************************************************************/
IDE_RC  dkpProtocolMgr::setProtocolHeader( dksSession   *aSession,
                                           UChar         aOpID,
                                           UChar         aParam,
                                           UChar         aFlags,
                                           UInt          aSessionID,
                                           UInt          aDataLength )
{
    UChar sDummyReserved = DKP_ADLP_PACKET_RESERVED_NOT_USED;

    IDE_TEST( dknLinkCheckAndFlush( aSession->mLink,
                                    DKP_ADLP_HDR_LEN )
              != IDE_SUCCESS );

    /* Set operation id */
    IDE_TEST( dknLinkWriteOneByteNumber( aSession->mLink, &aOpID ) != IDE_SUCCESS );

    /* Set operation specific parameter */
    IDE_TEST( dknLinkWriteOneByteNumber( aSession->mLink, &aParam ) != IDE_SUCCESS );

    /* PROJ-2569 */
    IDE_TEST( dknLinkWriteOneByteNumber( aSession->mLink, &aFlags ) != IDE_SUCCESS );

    /* Set Reserved*/
    IDE_TEST( dknLinkWriteOneByteNumber( aSession->mLink, &sDummyReserved ) != IDE_SUCCESS );

    /* Set session id */
    IDE_TEST( dknLinkWriteFourByteNumber( aSession->mLink, &aSessionID ) != IDE_SUCCESS );

    /* Set length of data */
    IDE_TEST( dknLinkWriteFourByteNumber( aSession->mLink, &aDataLength ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**********************************************************************
 * Description : Cm block 을 하나 받아온다. 내부적으로 cmiRecv 를 호출.
 *
 * sProtocolContext - [IN] 받아온 cm block 을 갖고 있을 cm protocol
 *                         구조체
 * aIsTimeOut       - [OUT] 이 함수 수행시 인자로 입력받은 timeout 값
 *                          이 지나도록 cm block 을 얻어오지 못한 경우
 *                          TRUE 로 설정된다.
 *
 **********************************************************************/
IDE_RC  dkpProtocolMgr::getCmBlock( dksSession          *aSession,
                                    idBool              *aIsTimeOut,
                                    ULong                aTimeoutSec )
{
    idBool sTimeoutFlag = ID_FALSE;

    IDE_TEST( dknLinkRecv( aSession->mLink, aTimeoutSec, &sTimeoutFlag ) != IDE_SUCCESS );

    IDU_FIT_POINT_RAISE( "dkpProtocolMgr::getCmBlock::cmiRecv::ERR_TIMEOUT", ERR_TIMEOUT );
    if ( sTimeoutFlag == ID_TRUE )
    {
        *aIsTimeOut = ID_TRUE;
        IDE_RAISE( ERR_TIMEOUT );
    }
    else
    {
        /* success */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_TIMEOUT );
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKP_TIMEOUT ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/************************************************************************
 * Description : 입력받은 protocol context 가 갖고 있는 cm read block
 *               으로부터 ADLP protocol 의 common header 를 읽어 그 내용의
 *               타당성을 검증하고 더 읽어야 할 데이터의 길이를 얻어온다.
 *
 *
 * aContext         - [IN] Operation 을 수행하는 세션이 갖고 있는 CM
 *                         protocol context
 * aOperationId     - [IN] 검사하려는 operation 의 id
 *
 ************************************************************************/
IDE_RC  dkpProtocolMgr::analyzeCommonHeader( dksSession         *aSession,
                                             dkpOperationID      aOpID,
                                             UChar              *aFlags,
                                             UInt                aSessionId,
                                             UInt               *aResultCode,
                                             UInt               *aDataLen )
{
    UChar       sOperationId = DKP_UNKNOWN_OPERATION;
    UChar       sResultCode  = DKP_RC_SUCCESS;
    UChar       sFlags       = 0;
    UChar       sReserved    = 0;
    UInt        sSessionId   = 0;

    /* Operation id */
    IDE_TEST( dknLinkReadOneByteNumber( aSession->mLink, &sOperationId ) != IDE_SUCCESS );
    IDE_TEST_RAISE( (dkpOperationID)sOperationId != aOpID, ERR_CHECK_OPERATION_ID );

    /* Result code */
    IDE_TEST( dknLinkReadOneByteNumber( aSession->mLink, &sResultCode ) != IDE_SUCCESS );
    IDE_TEST_RAISE( sResultCode >= DKP_RC_MAX, ERR_CHECK_RESULT_CODE );

    *aResultCode = (UInt)sResultCode;

    /* PROJ-2569 */
    /* Flags */
    IDE_TEST( dknLinkReadOneByteNumber( aSession->mLink, &sFlags ) != IDE_SUCCESS );
    *aFlags = sFlags;

    /* reserved */
    IDE_TEST( dknLinkReadOneByteNumber( aSession->mLink, &sReserved ) != IDE_SUCCESS );

    /* Session id */
    IDE_TEST( dknLinkReadFourByteNumber( aSession->mLink, &sSessionId ) != IDE_SUCCESS );
    IDE_TEST_RAISE( sSessionId != aSessionId, ERR_CHECK_SESSION_ID );

    /* Data length */
    IDE_TEST( dknLinkReadFourByteNumber( aSession->mLink, aDataLen ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_CHECK_OPERATION_ID );
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKP_ANALYZE_ADLP_HEADER ) );
    }
    IDE_EXCEPTION( ERR_CHECK_RESULT_CODE );
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKP_ANALYZE_ADLP_HEADER ) );
    }
    IDE_EXCEPTION( ERR_CHECK_SESSION_ID );
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKP_ANALYZE_ADLP_HEADER ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/************************************************************************
 * Description : AltiLinker 프로세스에 DK 의 linker control session 과
 *               1:1 매핑되는 session 을 생성하도록 하는 operation.
 *
 *  aSession        - [IN] Operation 을 수행하는 세션정보
 *  aProdInfoStr    - [IN] Altibase server package 정보
 *  aDbCharSet      - [IN] Altibase server 의 DB character set
 *
 ************************************************************************/
IDE_RC  dkpProtocolMgr::sendCreateLinkerCtrlSession( dksSession *aSession,
                                                     SChar      *aProdInfoStr,
                                                     UInt        aDbCharSet )
{
    UInt                    i;
    UInt                    sPropertyType;
    UInt                    sServerInfoLen        = 0;
    UInt                    sPropertyLen          = 0;
    UInt                    sPacketDataLen        = 0;
    UInt                    sLinkerPropertyCnt    = 0;
    UInt                    sTargetCnt            = 0;

    /* properties */
    UInt                    sRemoteNodeRecvTimeout;
    UInt                    sQueryTimeout;
    UInt                    sNonQueryTimeout;
    UInt                    sThreadCount;
    UInt                    sThreadSleepTime;
    UInt                    sRemoteNodeSessionCount;
    UInt                    sTraceLoggingLevel;
    UInt                    sTraceLogFileSize;
    UInt                    sTraceLogFileCount;

    UChar                   sInputParam;

    dkcDblinkConfTarget   * sTargetItem      = NULL;

    idBool                  sLockFlag    = ID_FALSE;
    UChar                   sHeaderFlags = makeHeaderFlags( aSession, ID_FALSE );


    IDE_ASSERT( dkaLinkerProcessMgr::isPropertiesLoaded() == ID_TRUE );

    sLinkerPropertyCnt = DKP_PROPERTY_MAX_COUNT;

    /* Get the length of data of the 1st block */
    sPacketDataLen += ID_SIZEOF( sServerInfoLen );
    sServerInfoLen = idlOS::strlen( aProdInfoStr );
    sPacketDataLen += sServerInfoLen;
    sPacketDataLen += ID_SIZEOF( aDbCharSet );
    sPacketDataLen += ID_SIZEOF( sLinkerPropertyCnt );
    sPacketDataLen += ( ( ID_SIZEOF( sPropertyType ) +
                          ID_SIZEOF( sPropertyLen ) )
                        * ( sLinkerPropertyCnt ) );

    IDE_TEST( dkaLinkerProcessMgr::getAltiLinkerRemoteNodeReceiveTimeoutFromConf( (SInt *)&sRemoteNodeRecvTimeout )
              != IDE_SUCCESS );
    sPacketDataLen += ID_SIZEOF( sRemoteNodeRecvTimeout );

    IDE_TEST( dkaLinkerProcessMgr::getAltiLinkerQueryTimeoutFromConf( (SInt *)&sQueryTimeout )
              != IDE_SUCCESS );
    sPacketDataLen += ID_SIZEOF( sQueryTimeout );

    IDE_TEST( dkaLinkerProcessMgr::getAltiLinkerNonQueryTimeoutFromConf( (SInt *)&sNonQueryTimeout )
              != IDE_SUCCESS );
    sPacketDataLen += ID_SIZEOF( sNonQueryTimeout );

    IDE_TEST( dkaLinkerProcessMgr::getAltiLinkerThreadCountFromConf( (SInt *)&sThreadCount )
              != IDE_SUCCESS );
    sPacketDataLen += ID_SIZEOF( sThreadCount );

    IDE_TEST( dkaLinkerProcessMgr::getAltiLinkerThreadSleepTimeFromConf( (SInt *)&sThreadSleepTime )
              != IDE_SUCCESS );
    sPacketDataLen += ID_SIZEOF( sThreadSleepTime );

    IDE_TEST( dkaLinkerProcessMgr::getAltiLinkerRemoteNodeSessionCountFromConf( (SInt *)&sRemoteNodeSessionCount )
              != IDE_SUCCESS );
    sPacketDataLen += ID_SIZEOF( sRemoteNodeSessionCount );

    IDE_TEST( dkaLinkerProcessMgr::getAltiLinkerTraceLoggingLevelFromConf( (SInt *)&sTraceLoggingLevel )
              != IDE_SUCCESS );
    sPacketDataLen += ID_SIZEOF( sTraceLoggingLevel );

    IDE_TEST( dkaLinkerProcessMgr::getAltiLinkerTraceLogFileSizeFromConf( (SInt *)&sTraceLogFileSize )
              != IDE_SUCCESS );
    sPacketDataLen += ID_SIZEOF( sTraceLogFileSize );

    IDE_TEST( dkaLinkerProcessMgr::getAltiLinkerTraceLogFileCountFromConf( (SInt *)&sTraceLogFileCount )
              != IDE_SUCCESS );
    sPacketDataLen += ID_SIZEOF( sTraceLogFileCount );

    IDE_TEST( dkaLinkerProcessMgr::lockAndGetTargetListFromConf( &sTargetItem ) != IDE_SUCCESS );
    sLockFlag = ID_TRUE;

    sTargetCnt = dkaLinkerProcessMgr::getTargetItemCount( sTargetItem );
    sPacketDataLen += ID_SIZEOF( sTargetCnt );

    sPacketDataLen += ( sTargetCnt *
                        DK_MAX_TARGET_SUB_ITEM_COUNT *
                        ( ID_SIZEOF( sPropertyType )
                          + ID_SIZEOF( sPropertyLen ) ) );

    sPacketDataLen += dkaLinkerProcessMgr::getTargetsLength( sTargetItem );

    if ( sTargetItem != NULL )
    {
        sInputParam = DKP_ADLP_PACKET_PARAM_FRAGMENTED;
    }
    else
    {
        sInputParam = DKP_ADLP_PACKET_PARAM_NOT_FRAGMENTED;
    }

    /* Set ADLP protocol common header */
    IDE_TEST( setProtocolHeader( aSession,
                                 DKP_CREATE_LINKER_CTRL_SESSION,
                                 sInputParam,
                                 sHeaderFlags,
                                 DKP_LINKER_CONTROL_SESSION_ID,
                                 sPacketDataLen )
              != IDE_SUCCESS );

    /* Set product information length */
    IDE_TEST( dknLinkWriteFourByteNumber( aSession->mLink, &sServerInfoLen ) != IDE_SUCCESS );

    /* Set product information */
    IDE_TEST( dknLinkWrite( aSession->mLink, (void *)aProdInfoStr, sServerInfoLen ) != IDE_SUCCESS );

    /* Set database character set */
    IDE_TEST( dknLinkWriteFourByteNumber( aSession->mLink, &aDbCharSet ) != IDE_SUCCESS );

    /* Set the number of properties : 12 */
    IDE_TEST( dknLinkWriteFourByteNumber( aSession->mLink, &sLinkerPropertyCnt ) != IDE_SUCCESS );

    /* 0. Remote node receive timeout : TLV */
    sPropertyType = DKP_PROPERTY_REMOTE_NODE_RECEIVE_TIMEOUT;
    sPropertyLen  = ID_SIZEOF( sRemoteNodeRecvTimeout );

    IDE_TEST( dknLinkWriteFourByteNumber( aSession->mLink, &sPropertyType ) != IDE_SUCCESS );
    IDE_TEST( dknLinkWriteFourByteNumber( aSession->mLink, &sPropertyLen ) != IDE_SUCCESS );
    IDE_TEST( dknLinkWriteFourByteNumber( aSession->mLink, &sRemoteNodeRecvTimeout ) != IDE_SUCCESS );

    /* 1. Query timeout : TLV */
    sPropertyType = DKP_PROPERTY_QUERY_TIMEOUT;
    sPropertyLen  = ID_SIZEOF( sQueryTimeout );

    IDE_TEST( dknLinkWriteFourByteNumber( aSession->mLink, &sPropertyType ) != IDE_SUCCESS );
    IDE_TEST( dknLinkWriteFourByteNumber( aSession->mLink, &sPropertyLen ) != IDE_SUCCESS );
    IDE_TEST( dknLinkWriteFourByteNumber( aSession->mLink, &sQueryTimeout ) != IDE_SUCCESS );

    /* 2. Non query timeout : TLV */
    sPropertyType = DKP_PROPERTY_NON_QUERY_TIMEOUT;
    sPropertyLen  = ID_SIZEOF( sNonQueryTimeout );

    IDE_TEST( dknLinkWriteFourByteNumber( aSession->mLink, &sPropertyType ) != IDE_SUCCESS );
    IDE_TEST( dknLinkWriteFourByteNumber( aSession->mLink, &sPropertyLen ) != IDE_SUCCESS );
    IDE_TEST( dknLinkWriteFourByteNumber( aSession->mLink, &sNonQueryTimeout ) != IDE_SUCCESS );

    /* 3. Linker thread maximum count : TLV */
    sPropertyType = DKP_PROPERTY_THREAD_COUNT;
    sPropertyLen  = ID_SIZEOF( sThreadCount );

    IDE_TEST( dknLinkWriteFourByteNumber( aSession->mLink, &sPropertyType ) != IDE_SUCCESS );
    IDE_TEST( dknLinkWriteFourByteNumber( aSession->mLink, &sPropertyLen ) != IDE_SUCCESS );
    IDE_TEST( dknLinkWriteFourByteNumber( aSession->mLink, &sThreadCount ) != IDE_SUCCESS );

    /* 4. Linker thread sleep time : TLV */
    sPropertyType = DKP_PROPERTY_THREAD_SLEEP_TIME;
    sPropertyLen  = ID_SIZEOF( sThreadSleepTime );

    IDE_TEST( dknLinkWriteFourByteNumber( aSession->mLink, &sPropertyType ) != IDE_SUCCESS );
    IDE_TEST( dknLinkWriteFourByteNumber( aSession->mLink, &sPropertyLen ) != IDE_SUCCESS );
    IDE_TEST( dknLinkWriteFourByteNumber( aSession->mLink, &sThreadSleepTime ) != IDE_SUCCESS );

    /* 5. Remote node session maximum count : TLV */
    sPropertyType = DKP_PROPERTY_REMOTE_NODE_SESSION_COUNT;
    sPropertyLen  = ID_SIZEOF( sRemoteNodeSessionCount );

    IDE_TEST( dknLinkWriteFourByteNumber( aSession->mLink, &sPropertyType ) != IDE_SUCCESS );
    IDE_TEST( dknLinkWriteFourByteNumber( aSession->mLink, &sPropertyLen ) != IDE_SUCCESS );
    IDE_TEST( dknLinkWriteFourByteNumber( aSession->mLink, &sRemoteNodeSessionCount ) != IDE_SUCCESS );

    /* 6. AltiLinker process trace logging level : TLV */
    sPropertyType = DKP_PROPERTY_TRACE_LOGGING_LEVEL;
    sPropertyLen  = ID_SIZEOF( sTraceLoggingLevel );

    IDE_TEST( dknLinkWriteFourByteNumber( aSession->mLink, &sPropertyType ) != IDE_SUCCESS );
    IDE_TEST( dknLinkWriteFourByteNumber( aSession->mLink, &sPropertyLen ) != IDE_SUCCESS );
    IDE_TEST( dknLinkWriteFourByteNumber( aSession->mLink, &sTraceLoggingLevel ) != IDE_SUCCESS );

    /* 7. AltiLinker process log file size : TLV */
    sPropertyType = DKP_PROPERTY_TRACE_LOG_FILE_SIZE;
    sPropertyLen  = ID_SIZEOF( sTraceLogFileSize );

    IDE_TEST( dknLinkWriteFourByteNumber( aSession->mLink, &sPropertyType ) != IDE_SUCCESS );
    IDE_TEST( dknLinkWriteFourByteNumber( aSession->mLink, &sPropertyLen ) != IDE_SUCCESS );
    IDE_TEST( dknLinkWriteFourByteNumber( aSession->mLink, &sTraceLogFileSize ) != IDE_SUCCESS );

    /* 8. AltiLinker process log file maximum count : TLV */
    sPropertyType = DKP_PROPERTY_TRACE_LOG_FILE_COUNT;
    sPropertyLen  = ID_SIZEOF( sTraceLogFileCount );

    IDE_TEST( dknLinkWriteFourByteNumber( aSession->mLink, &sPropertyType ) != IDE_SUCCESS );
    IDE_TEST( dknLinkWriteFourByteNumber( aSession->mLink, &sPropertyLen ) != IDE_SUCCESS );
    IDE_TEST( dknLinkWriteFourByteNumber( aSession->mLink, &sTraceLogFileCount ) != IDE_SUCCESS );

    /* 9. Target : TLV */
    sPropertyType = DKP_PROPERTY_TARGETS;
    sPropertyLen  = dkaLinkerProcessMgr::getTargetsLength( sTargetItem );
    sPropertyLen += ( sTargetCnt * ( ID_SIZEOF( sPropertyType ) +
                                     ID_SIZEOF( sPropertyLen ) ) );

    IDE_TEST( dknLinkWriteFourByteNumber( aSession->mLink, &sPropertyType ) != IDE_SUCCESS );
    IDE_TEST( dknLinkWriteFourByteNumber( aSession->mLink, &sPropertyLen ) != IDE_SUCCESS );
    IDE_TEST( dknLinkWriteFourByteNumber( aSession->mLink, &sTargetCnt ) != IDE_SUCCESS );

    /* Send cm block */
    IDE_TEST( dknLinkSend( aSession->mLink ) != IDE_SUCCESS );

    /* ================ Send target informations ================ */
    for ( i = 0; i < sTargetCnt; i++ )
    {
        if ( sTargetItem != NULL )
        {
            sPacketDataLen = 0;

            if ( sTargetItem->mNext == NULL )
            {
                sInputParam = DKP_ADLP_PACKET_PARAM_NOT_FRAGMENTED;
            }
            else
            {
                sInputParam = DKP_ADLP_PACKET_PARAM_FRAGMENTED;
            }

            /* Get the length of data */
            sPacketDataLen = dkaLinkerProcessMgr::getTargetItemLength( sTargetItem );

            sPacketDataLen += ( DK_MAX_TARGET_SUB_ITEM_COUNT *
                                ( ID_SIZEOF( sPropertyType ) +
                                  ID_SIZEOF( sPropertyLen ) ) );

            /* Set ADLP protocol common header */
            IDE_TEST( setProtocolHeader( aSession,
                                         DKP_CREATE_LINKER_CTRL_SESSION,
                                         sInputParam,
                                         sHeaderFlags,
                                         DKP_LINKER_CONTROL_SESSION_ID,
                                         sPacketDataLen )
                      != IDE_SUCCESS );

            /* >>> Set TLV of sub-items */
            /* Target name */
            sPropertyType = DKP_PROPERTY_TARGET_NAME;

            if ( sTargetItem->mName != NULL )
            {
                sPropertyLen = idlOS::strlen( sTargetItem->mName );
            }
            else
            {
                sPropertyLen = 0;
            }

            IDE_TEST( dknLinkWriteFourByteNumber( aSession->mLink, &sPropertyType ) != IDE_SUCCESS );
            IDE_TEST( dknLinkWriteFourByteNumber( aSession->mLink, &sPropertyLen ) != IDE_SUCCESS );

            if ( sPropertyLen > 0 )
            {
                /* Set the type of this item */
                IDE_TEST( dknLinkWrite( aSession->mLink,
                                        (void *)sTargetItem->mName,
                                        sPropertyLen )
                          != IDE_SUCCESS );
            }
            else
            {
                /* Next item */
            }

            /* Target JDBC driver path */
            sPropertyType = DKP_PROPERTY_TARGET_JDBC_DRIVER;

            if ( sTargetItem->mJdbcDriver != NULL )
            {
                sPropertyLen = idlOS::strlen( sTargetItem->mJdbcDriver );
            }
            else
            {
                sPropertyLen = 0;
            }

            IDE_TEST( dknLinkWriteFourByteNumber( aSession->mLink, &sPropertyType ) != IDE_SUCCESS );
            IDE_TEST( dknLinkWriteFourByteNumber( aSession->mLink, &sPropertyLen ) != IDE_SUCCESS );

            if ( sPropertyLen > 0 )
            {
                /* Set the type of this item */
                IDE_TEST( dknLinkWrite( aSession->mLink,
                                        (void *)sTargetItem->mJdbcDriver,
                                        sPropertyLen )
                          != IDE_SUCCESS );
            }
            else
            {
                /* Next item */
            }

            /* Target JDBC driver class Name */
            sPropertyType = DKP_PROPERTY_TARGET_JDBC_DRIVER_CLASS_NAME;

            if ( sTargetItem->mJdbcDriverClassName != NULL )
            {
                sPropertyLen = idlOS::strlen( sTargetItem->mJdbcDriverClassName );
            }
            else
            {
                sPropertyLen = 0;
            }

            IDE_TEST( dknLinkWriteFourByteNumber( aSession->mLink, &sPropertyType ) != IDE_SUCCESS );
            IDE_TEST( dknLinkWriteFourByteNumber( aSession->mLink, &sPropertyLen ) != IDE_SUCCESS );

            if ( sPropertyLen > 0 )
            {
                /* Set the type of this item */
                IDE_TEST( dknLinkWrite( aSession->mLink,
                                        (void *)sTargetItem->mJdbcDriverClassName,
                                        sPropertyLen )
                          != IDE_SUCCESS );
            }
            else
            {
                /* Next item */
            }

            /* Target remote server information */
            sPropertyType = DKP_PROPERTY_TARGET_URL;

            if ( sTargetItem->mConnectionUrl != NULL )
            {
                sPropertyLen = idlOS::strlen( sTargetItem->mConnectionUrl );
            }
            else
            {
                sPropertyLen = 0;
            }

            IDE_TEST( dknLinkWriteFourByteNumber( aSession->mLink, &sPropertyType ) != IDE_SUCCESS );
            IDE_TEST( dknLinkWriteFourByteNumber( aSession->mLink, &sPropertyLen ) != IDE_SUCCESS );

            if ( sPropertyLen > 0 )
            {
                /* Set the type of this item */
                IDE_TEST( dknLinkWrite( aSession->mLink,
                                        (void *)sTargetItem->mConnectionUrl,
                                        sPropertyLen )
                          != IDE_SUCCESS );
            }
            else
            {
                /* Next item */
            }

            /* Target remote server user id */
            sPropertyType = DKP_PROPERTY_TARGET_USER;

            if ( sTargetItem->mUser != NULL )
            {
                sPropertyLen = idlOS::strlen( sTargetItem->mUser );
            }
            else
            {
                sPropertyLen = 0;
            }

            IDE_TEST( dknLinkWriteFourByteNumber( aSession->mLink, &sPropertyType ) != IDE_SUCCESS );
            IDE_TEST( dknLinkWriteFourByteNumber( aSession->mLink, &sPropertyLen ) != IDE_SUCCESS );

            if (sPropertyLen > 0)
            {
                /* Set the type of this item */
                IDE_TEST( dknLinkWrite( aSession->mLink,
                                        (void *)sTargetItem->mUser,
                                        sPropertyLen )
                          != IDE_SUCCESS );
            }
            else
            {
                /* Next item */
            }

            /* Target remote server password */
            sPropertyType = DKP_PROPERTY_TARGET_PASSWD;

            if ( sTargetItem->mPassword != NULL )
            {
                sPropertyLen  = idlOS::strlen( sTargetItem->mPassword );
            }
            else
            {
                sPropertyLen = 0;
            }

            IDE_TEST( dknLinkWriteFourByteNumber( aSession->mLink, &sPropertyType ) != IDE_SUCCESS );
            IDE_TEST( dknLinkWriteFourByteNumber( aSession->mLink, &sPropertyLen ) != IDE_SUCCESS );

            if (sPropertyLen > 0)
            {
                /* Set the type of this item */
                IDE_TEST( dknLinkWrite( aSession->mLink,
                                        (void *)sTargetItem->mPassword,
                                        sPropertyLen )
                          != IDE_SUCCESS );
            }
            else
            {
                /* Done */
            }

            /* XADataSource Class Name */
            sPropertyType = DKP_PROPERTY_TARGET_XADATASOURCE_CLASS_NAME;

            if ( sTargetItem->mXADataSourceClassName != NULL )
            {
                sPropertyLen  = idlOS::strlen( sTargetItem->mXADataSourceClassName );
            }
            else
            {
                sPropertyLen = 0;
            }

            IDE_TEST( dknLinkWriteFourByteNumber( aSession->mLink, &sPropertyType ) != IDE_SUCCESS );
            IDE_TEST( dknLinkWriteFourByteNumber( aSession->mLink, &sPropertyLen ) != IDE_SUCCESS );

            if (sPropertyLen > 0)
            {
                /* Set the type of this item */
                IDE_TEST( dknLinkWrite( aSession->mLink,
                                        (void *)sTargetItem->mXADataSourceClassName,
                                        sPropertyLen )
                          != IDE_SUCCESS );
            }
            else
            {
                /* Done */
            }

            /* XADataSource Url setter Name */
            sPropertyType = DKP_PROPERTY_TARGET_XADATASOURCE_URL_SETTER_NAME;

            if ( sTargetItem->mXADataSourceUrlSetterName != NULL )
            {
                sPropertyLen  = idlOS::strlen( sTargetItem->mXADataSourceUrlSetterName );
            }
            else
            {
                sPropertyLen = 0;
            }

            IDE_TEST( dknLinkWriteFourByteNumber( aSession->mLink, &sPropertyType ) != IDE_SUCCESS );
            IDE_TEST( dknLinkWriteFourByteNumber( aSession->mLink, &sPropertyLen ) != IDE_SUCCESS );

            if (sPropertyLen > 0)
            {
                /* Set the type of this item */
                IDE_TEST( dknLinkWrite( aSession->mLink,
                                        (void *)sTargetItem->mXADataSourceUrlSetterName,
                                        sPropertyLen )
                          != IDE_SUCCESS );
            }
            else
            {
                /* Done */
            }

            /* <<< Set TLV of sub-items */
            IDE_TEST( dknLinkSend( aSession->mLink ) != IDE_SUCCESS );
        }
        else
        {
            /* Error */
            IDE_RAISE( ERR_SEND );
        }

        sTargetItem = sTargetItem->mNext;
    }

    sLockFlag = ID_FALSE;
    dkaLinkerProcessMgr::unlockTargetListFromConf();

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_SEND );
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKP_CMI_SEND ) );
    }

    IDE_EXCEPTION_END;

    if ( sLockFlag == ID_TRUE )
    {
        dkaLinkerProcessMgr::unlockTargetListFromConf();
    }
    else
    {
        /* nothing to do */
    }

    return IDE_FAILURE;
}

/************************************************************************
 * Description : sendCreateLinkerCtrlSession operation 의 결과를 받는다.
 *
 *  aSession        - [IN] Operation 을 수행하는 세션정보
 *  aResultCode     - [OUT] Operation 의 수행 결과로 얻는 result code
 *  aProtocolVer    - [IN] AltiLinker 프로세스에 적용된 프로토콜 버전
 *  aTimeoutSec     - [IN] cm block 을 번송받기 위한 대기시간
 *
 ************************************************************************/
IDE_RC  dkpProtocolMgr::recvCreateLinkerCtrlSessionResult( dksSession        *aSession,
                                                           UInt              *aResultCode,
                                                           UInt              *aProtocolVer,
                                                           ULong              aTimeoutSec )
{
    idBool                  sIsTimedOut = ID_FALSE;
    UInt                    i = 0;
    UInt                    sDataLen = 0;
    UInt                    sTargetNameLen    = 0;
    UInt                    sInvalidTargetCnt = 0;
    UInt                    sMsgOffset        = 0;
    SChar                   sInvalidTargetName[ DK_NAME_LEN + 1 ];
    SChar                   sMsgBuf[ DK_MSG_LEN ];
    UChar                   sFlags = 0;

    /* Get cm block from protocol context */
    IDE_TEST_RAISE( getCmBlock( aSession,
                                &sIsTimedOut,
                                aTimeoutSec )
                    != IDE_SUCCESS, ERR_DISCONNECTED );

    IDE_TEST_RAISE( sIsTimedOut == ID_TRUE, ERR_DISCONNECTED );

    /* Analyze ADLP common header */
    IDE_TEST( analyzeCommonHeader( aSession,
                                   DKP_CREATE_LINKER_CTRL_SESSION_RESULT,
                                   &sFlags,
                                   DKP_LINKER_CONTROL_SESSION_ID,
                                   aResultCode,
                                   &sDataLen )
              != IDE_SUCCESS );

    // 4 바이트 이상 아닌가??
    if ( sDataLen >= 8 )
    {
        /* Get ADLP protocol version */
        IDE_TEST( dknLinkReadFourByteNumber( aSession->mLink, aProtocolVer ) != IDE_SUCCESS );

        /* Get invalid target count */
        IDE_TEST( dknLinkReadFourByteNumber( aSession->mLink, &sInvalidTargetCnt ) != IDE_SUCCESS );

        if ( sInvalidTargetCnt >= 1 )
        {
            idlOS::memset( (void *)sMsgBuf, 0, ID_SIZEOF( sMsgBuf ));

            idlOS::snprintf( sMsgBuf,
                             ID_SIZEOF( sMsgBuf ),
                             PDL_TEXT( "Invalid target count : "
                                       "%"ID_INT32_FMT
                                       "\nTargets are : "),
                             sInvalidTargetCnt );

            sMsgOffset = idlOS::strlen( sMsgBuf );

            /* Get invalid targets */
            for ( i = 0; i < sInvalidTargetCnt; i++ )
            {
                IDE_TEST( dknLinkReadFourByteNumber( aSession->mLink, &sTargetNameLen ) != IDE_SUCCESS );
                if( sTargetNameLen > 0 )
                {
                    IDE_TEST( readLinkBufferInternal( aSession->mLink,
                                                      sInvalidTargetName,
                                                      DK_NAME_LEN + 1,
                                                      &sTargetNameLen,
                                                      ID_TRUE ) 
                              != IDE_SUCCESS );

                    if( sTargetNameLen > DK_NAME_LEN )
                    {
                        sTargetNameLen = DK_NAME_LEN;
                    }
                    sInvalidTargetName[sTargetNameLen] = '\0';                    

                    idlOS::snprintf( sMsgBuf + sMsgOffset,
                                    ( ID_SIZEOF( sMsgBuf ) - ( ID_SIZEOF(SChar) * sMsgOffset ) ),
                                    PDL_TEXT( "%s, " ),
                                    sInvalidTargetName );
                    sMsgOffset = idlOS::strlen( sMsgBuf );
                }
                else
                {
                    /*do nothing*/
                }
            }
        }
        else
        {
            /* there is no invalid target */
        }
    }
    else
    {
        /* no payload */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_DISCONNECTED );
    {
        IDE_ERRLOG( DK_TRC_LOG_FORCE );
        IDE_SET(ideSetErrorCode(dkERR_ABORT_ALTILINKER_DISCONNECTED));

        aSession->mIsNeedToDisconnect = ID_TRUE;
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/************************************************************************
 * Description : AltiLinker 프로세스에 입력받는 DK 의 linker data session
 *               과 1:1 매핑되는 session 을 생성하도록 하는 operation.
 *
 *  aSession    - [IN] Operation 을 수행하는 세션정보
 *  aSessionId  - [IN] Destroy 하려는 세션의 id
 *
 ************************************************************************/
IDE_RC  dkpProtocolMgr::sendCreateLinkerDataSession( dksSession *aSession,
                                                     UInt        aSessionId )
{
    UInt    sPacketDataLen   = 0;
    UChar   sFlags           = makeHeaderFlags( aSession, ID_FALSE );


    /* Validate parameters */
    IDE_DASSERT( aSession != NULL );
    IDE_TEST_RAISE( aSessionId == DKP_LINKER_CONTROL_SESSION_ID,
                    ERR_INPUT_LINKER_DATA_SESSION_ID );

    /* Set ADLP protocol common header */
    IDE_TEST( setProtocolHeader( aSession,
                                 DKP_CREATE_LINKER_DATA_SESSION,
                                 DKP_ADLP_PACKET_PARAM_NOT_USED,
                                 sFlags,
                                 aSessionId,
                                 sPacketDataLen )
              != IDE_SUCCESS );

    IDE_TEST( dknLinkSend( aSession->mLink ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INPUT_LINKER_DATA_SESSION_ID );
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKP_INVALID_SESSION_ID ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/************************************************************************
 * Description : sendCreateLinkerDataSession operation 의 결과를 받는다.
 *
 *  aSession    - [IN] Operation 을 수행하는 세션정보
 *  aSessionId  - [IN] AltiLinker 프로세스에 생성할 세션의 id
 *  aResultCode - [OUT] Operation 의 수행 결과로 얻는 result code
 *  aTimeoutSec - [IN] Operation timeout value
 *
 ************************************************************************/
IDE_RC  dkpProtocolMgr::recvCreateLinkerDataSessionResult( dksSession *aSession,
                                                           UInt        aSessionId,
                                                           UInt       *aResultCode,
                                                           ULong       aTimeoutSec )
{
    idBool                  sIsTimedOut = ID_FALSE;
    UInt                    sDataLen = 0;
    UChar                   sFlags = 0;

    /* Validate parameters */
    IDE_DASSERT( aSession != NULL );
    IDE_TEST_RAISE( aSessionId == DKP_LINKER_CONTROL_SESSION_ID,
                    ERR_INPUT_LINKER_DATA_SESSION_ID );

    /* Get cm block from protocol context */
    IDE_TEST_RAISE( getCmBlock( aSession,
                                &sIsTimedOut,
                                aTimeoutSec )
                    != IDE_SUCCESS, ERR_DISCONNECTED );

    IDE_TEST_RAISE( sIsTimedOut == ID_TRUE, ERR_DISCONNECTED );

    /* Analyze ADLP common header */
    IDE_TEST( analyzeCommonHeader( aSession,
                                   DKP_CREATE_LINKER_DATA_SESSION_RESULT,
                                   &sFlags,
                                   aSessionId,
                                   aResultCode,
                                   &sDataLen )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INPUT_LINKER_DATA_SESSION_ID );
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKP_INVALID_SESSION_ID ) );
    }
    IDE_EXCEPTION( ERR_DISCONNECTED );
    {
        IDE_ERRLOG( DK_TRC_LOG_FORCE );
        IDE_SET(ideSetErrorCode(dkERR_ABORT_ALTILINKER_DISCONNECTED));

        aSession->mIsNeedToDisconnect = ID_TRUE;
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/************************************************************************
 * Description : AltiLinker 프로세스에 입력받는 DK 의 linker data session
 *               과 1:1 매핑되는 session 을 생성하도록 하는 operation.
 *
 *  aSession    - [IN] Operation 을 수행하는 세션정보
 *  aSessionId  - [IN] Destroy 하려는 세션의 id
 *
 ************************************************************************/
IDE_RC  dkpProtocolMgr::sendDestroyLinkerDataSession( dksSession    *aSession,
                                                      UInt           aSessionId )
{
    UInt    sPacketDataLen   = 0;
    UChar   sFlags           = makeHeaderFlags( aSession, ID_FALSE );

    /* Validate parameters */
    IDE_DASSERT( aSession != NULL );

    IDE_TEST_RAISE( aSessionId == DKP_LINKER_CONTROL_SESSION_ID,
                    ERR_INPUT_LINKER_DATA_SESSION_ID );

    /* Set ADLP protocol common header */
    IDE_TEST( setProtocolHeader( aSession,
                                 DKP_DESTROY_LINKER_DATA_SESSION,
                                 DKP_ADLP_PACKET_PARAM_NOT_USED,
                                 sFlags,
                                 aSessionId,
                                 sPacketDataLen )
              != IDE_SUCCESS );

    IDE_TEST( dknLinkSend( aSession->mLink ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INPUT_LINKER_DATA_SESSION_ID );
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKP_INVALID_SESSION_ID ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/************************************************************************
 * Description : sendDestroyLinkerDataSession operation 의 결과를 받는다.
 *
 *  aSession    - [IN] Operation 을 수행하는 세션정보
 *  aSessionId  - [IN] Destroy 하려는 세션의 id
 *  aResultCode - [OUT] Operation 의 수행 결과로 얻는 result code
 *  aTimeoutSec - [IN] Operation timeout value
 *
 ************************************************************************/
IDE_RC  dkpProtocolMgr::recvDestroyLinkerDataSessionResult( dksSession *aSession,
                                                            UInt        aSessionId,
                                                            UInt       *aResultCode,
                                                            ULong       aTimeoutSec )
{
    idBool                  sIsTimedOut = ID_FALSE;
    UInt                    sDataLen = 0;
    UChar                   sFlags = 0;

    /* Validate parameters */
    IDE_DASSERT( aSession != NULL );
    IDE_TEST_RAISE( aSessionId == DKP_LINKER_CONTROL_SESSION_ID,
                    ERR_INPUT_LINKER_DATA_SESSION_ID );

    /* Get cm block from protocol context */
    IDE_TEST_RAISE( getCmBlock( aSession,
                                &sIsTimedOut,
                                aTimeoutSec )
                    != IDE_SUCCESS, ERR_DISCONNECTED );

    IDE_TEST_RAISE( sIsTimedOut == ID_TRUE, ERR_DISCONNECTED );

    /* Analyze ADLP common header */
    IDE_TEST( analyzeCommonHeader( aSession,
                                   DKP_DESTROY_LINKER_DATA_SESSION_RESULT,
                                   &sFlags,
                                   aSessionId,
                                   aResultCode,
                                   &sDataLen )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INPUT_LINKER_DATA_SESSION_ID );
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKP_INVALID_SESSION_ID ) );
    }
    IDE_EXCEPTION( ERR_DISCONNECTED );
    {
        IDE_ERRLOG( DK_TRC_LOG_FORCE );
        IDE_SET(ideSetErrorCode(dkERR_ABORT_ALTILINKER_DISCONNECTED));

        aSession->mIsNeedToDisconnect = ID_TRUE;
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/************************************************************************
 * Description : AltiLinker 프로세스의 현재 상태정보를 요청한다.
 *
 * aSession         - [IN] Operation 을 수행하는 세션정보
 ************************************************************************/
IDE_RC  dkpProtocolMgr::sendRequestLinkerStatus( dksSession  *aSession )
{
    UInt    sPacketDataLen = 0;
    UChar   sFlags         = makeHeaderFlags( aSession, ID_FALSE );

    /* Set ADLP protocol common header */
    IDE_TEST( setProtocolHeader( aSession,
                                 DKP_REQUEST_LINKER_STATUS,
                                 DKP_ADLP_PACKET_PARAM_NOT_USED,
                                 sFlags,
                                 DKP_LINKER_CONTROL_SESSION_ID,
                                 sPacketDataLen )
              != IDE_SUCCESS );

    IDE_TEST( dknLinkSend( aSession->mLink ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/************************************************************************
 * Description : sendRequestLinkerStatus operation 의 결과를 받는다.
 *
 * aSession         - [IN] Operation 을 수행하는 세션정보
 * aResultCode      - [OUT] Operation 의 수행 결과로 얻는 result code
 * aLinkerStatus    - [OUT] AltiLinker process 의 상태정보
 * aTimeoutSec      - [IN] CM block 의 읽기 timeout 값
 ************************************************************************/
IDE_RC  dkpProtocolMgr::recvRequestLinkerStatusResult( dksSession      *aSession,
                                                       UInt            *aResultCode,
                                                       dkaLinkerStatus *aLinkerStatus,
                                                       ULong           aTimeoutSec )
{
    idBool                  sIsTimedOut = ID_FALSE;
    UInt                    sDataLen = 0;
    UChar                   sFlags = 0;

    /* Get cm block from protocol context */
    IDE_TEST_RAISE( getCmBlock( aSession,
                                &sIsTimedOut,
                                aTimeoutSec )
                    != IDE_SUCCESS, ERR_DISCONNECTED );

    IDE_TEST_RAISE( sIsTimedOut == ID_TRUE, ERR_DISCONNECTED );

    /* Analyze ADLP common header */
    IDE_TEST( analyzeCommonHeader( aSession,
                                   DKP_REQUEST_LINKER_STATUS_RESULT,
                                   &sFlags,
                                   DKP_LINKER_CONTROL_SESSION_ID,
                                   aResultCode,
                                   &sDataLen )
              != IDE_SUCCESS );

    if ( sDataLen >= 4 )
    {
        /* Set AltiLinker status */
        IDE_TEST( dknLinkReadFourByteNumber( aSession->mLink, aLinkerStatus ) != IDE_SUCCESS );
    }
    else
    {
        /* no payload */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_DISCONNECTED );
    {
        IDE_ERRLOG( DK_TRC_LOG_FORCE );
        IDE_SET(ideSetErrorCode(dkERR_ABORT_ALTILINKER_DISCONNECTED));

        aSession->mIsNeedToDisconnect = ID_TRUE;
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/************************************************************************
 * Description : V$DBLINK_ALTILINKER_INFO performance view 를 구성하는데
 *               필요한 정보를 AltiLinker 프로세스로부터 얻기 위해 수행
 *               하는 operation.
 *
 * aSession         - [IN] Operation 을 수행하는 세션정보
 ************************************************************************/
IDE_RC  dkpProtocolMgr::sendRequestDblinkAltiLnkerStatusPvInfo( dksSession *aSession )
{
    UInt    sPacketDataLen   = 0;
    UChar   sFlags           = makeHeaderFlags( aSession, ID_FALSE );

    /* Set ADLP protocol common header */
    IDE_TEST( setProtocolHeader( aSession,
                                 DKP_REQUEST_PV_INFO_DBLINK_ALTILINKER_STATUS,
                                 DKP_ADLP_PACKET_PARAM_NOT_USED,
                                 sFlags,
                                 DKP_LINKER_CONTROL_SESSION_ID,
                                 sPacketDataLen )
              != IDE_SUCCESS );

    IDE_TEST( dknLinkSend( aSession->mLink ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/************************************************************************
 * Description : sendRequestDblinkAltiLnkerStatusPvInfo operation 의
 *               결과를 받는다.
 *
 *            ++ AltiLinker process 에 대한 상태정보들 중 아래
 *               performance view 에서 요구하는 정보들은 다음과 같다.
 *
 *               1. AltiLinker 프로세스의 현재 상태
 *               2. AltiLinker 프로세스의 start time
 *               3. 현재 알티베이스 서버와 AltiLinker 프로세스간 생성된
 *                  linker control session 과 linker data session 의 개수
 *               4. AltiLinker 프로세스가 현재 remote node 들과 맺고 있는
 *                  session 의 개수
 *               5. AltiLinker 프로세스가 현재 사용하고 있는 JVM memory
 *                  사용량
 *               6. AltiLinker 프로세스가 돌아가는 JVM 이 할당가능한 memory 의
 *                  최대 크기
 *
 *               이중에서 1, 2, 3, 6 정보들은 DK 에서 이미 알고 있는
 *               정보들이나 4, 5번에 해당하는 정보들은 정보를 요청한
 *               시점에 AltiLinker 프로세스로부터 얻어와햐 한다.
 *               따라서 이 protocol operation 은 4, 5번 정보를 얻어오기
 *               위한 operation 이다.
 *
 *  aSession         - [IN] Operation 을 수행하는 세션정보
 *  aResultCode      - [OUT] Operation 의 수행 결과로 얻는 result code
 *  aLinkerProcInfo  - [OUT] 이 operation 이 반환할 AltiLinker 프로세스
 *                           정보
 *  aTimeoutSec      - [IN] CM block 의 읽기 timeout 값
 *
 ************************************************************************/
IDE_RC  dkpProtocolMgr::recvRequestDblinkAltiLnkerStatusPvInfoResult( dksSession        *aSession,
                                                                      UInt              *aResultCode,
                                                                      dkaLinkerProcInfo *aLinkerProcInfo,
                                                                      ULong              aTimeoutSec )
{
    idBool                  sIsTimedOut = ID_FALSE;
    UInt                    sDataLen = 0;
    UChar                   sFlags = 0;

    IDE_ASSERT( aLinkerProcInfo != NULL );

    /* Get cm block from protocol context */
    IDE_TEST_RAISE( getCmBlock( aSession,
                                &sIsTimedOut,
                                aTimeoutSec )
                    != IDE_SUCCESS, ERR_DISCONNECTED );

    IDE_TEST_RAISE( sIsTimedOut == ID_TRUE, ERR_DISCONNECTED );

    /* Analyze ADLP common header */
    IDE_TEST( analyzeCommonHeader( aSession,
                                   DKP_REQUEST_PV_INFO_DBLINK_ALTILINKER_STATUS_RESULT,
                                   &sFlags,
                                   DKP_LINKER_CONTROL_SESSION_ID,
                                   aResultCode,
                                   &sDataLen )
              != IDE_SUCCESS );

    if ( sDataLen >= 12 )
    {
        /* Set AltiLinker status */
        IDE_TEST( dknLinkReadFourByteNumber( aSession->mLink, &(aLinkerProcInfo->mRemoteNodeSessionCnt) )
                  != IDE_SUCCESS );
        IDE_TEST( dknLinkReadEightByteNumber( aSession->mLink, (void *)&(aLinkerProcInfo->mJvmMemUsage) )
                  != IDE_SUCCESS );
    }
    else
    {
        /* no payload */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_DISCONNECTED );
    {
        IDE_ERRLOG( DK_TRC_LOG_FORCE );
        IDE_SET(ideSetErrorCode(dkERR_ABORT_ALTILINKER_DISCONNECTED));

        aSession->mIsNeedToDisconnect = ID_TRUE;
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/************************************************************************
 * Description : AltiLinker 프로세스에 종료명령을 내리기 전에 AltiLinker
 *               프로세스로 하여금 종료를 위한 준비를 수행할 것을 요청
 *               하기 위해 수행되는 operation.
 *
 * aSession         - [IN] Operation 을 수행하는 세션정보
 ************************************************************************/
IDE_RC  dkpProtocolMgr::sendPrepareLinkerShutdown( dksSession  *aSession )
{
    UInt    sPacketDataLen   = 0;
    UChar   sFlags           = makeHeaderFlags( aSession, ID_FALSE );

    /* Set ADLP protocol common header */
    IDE_TEST( setProtocolHeader( aSession,
                                 DKP_PREPARE_LINKER_SHUTDOWN,
                                 DKP_ADLP_PACKET_PARAM_NOT_USED,
                                 sFlags,
                                 DKP_LINKER_CONTROL_SESSION_ID,
                                 sPacketDataLen )
              != IDE_SUCCESS );

    IDE_TEST( dknLinkSend( aSession->mLink ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/************************************************************************
 * Description : sendPrepareLinkerShutdown operation 의 결과를 받는다.
 *            ++ 수행중인 remote transaction 혹은 remote statement 가
 *               존재하는 경우 aResultCode 는 DKP_RC_BUSY 로 설정된다.
 *
 * aSession         - [IN] Operation 을 수행하는 세션정보
 * aResultCode      - [OUT] Operation 의 수행 결과로 얻는 result code
 * aTimeoutSec      - [IN] CM block 의 읽기 timeout 값
 ************************************************************************/
IDE_RC  dkpProtocolMgr::recvPrepareLinkerShutdownResult( dksSession *aSession,
                                                         UInt       *aResultCode,
                                                         ULong       aTimeoutSec )
{
    idBool                  sIsTimedOut = ID_FALSE;
    UInt                    sDataLen = 0;
    UChar                   sFlags = 0;

    /* Get cm block from protocol context */
    IDE_TEST_RAISE( getCmBlock( aSession,
                                &sIsTimedOut,
                                aTimeoutSec )
                    != IDE_SUCCESS, ERR_DISCONNECTED );

    IDE_TEST_RAISE( sIsTimedOut == ID_TRUE, ERR_DISCONNECTED );

    /* Analyze ADLP common header */
    IDE_TEST( analyzeCommonHeader( aSession,
                                   DKP_PREPARE_LINKER_SHUTDOWN_RESULT,
                                   &sFlags,
                                   DKP_LINKER_CONTROL_SESSION_ID,
                                   aResultCode,
                                   &sDataLen )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_DISCONNECTED );
    {
        IDE_ERRLOG( DK_TRC_LOG_FORCE );
        IDE_SET(ideSetErrorCode(dkERR_ABORT_ALTILINKER_DISCONNECTED));

        aSession->mIsNeedToDisconnect = ID_TRUE;
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/************************************************************************
 * Description : AltiLinker 프로세스에 stack trace등의 정보를 dump하라는
 *               요청을 하기 위해 수행되는 operation.
 *
 * aSession         - [IN] Operation 을 수행하는 세션정보
 ************************************************************************/
IDE_RC  dkpProtocolMgr::sendLinkerDump( dksSession  *aSession )
{
    UInt    sPacketDataLen   = 0;
    UChar   sFlags           = makeHeaderFlags( aSession, ID_FALSE );

    /* Set ADLP protocol common header */
    IDE_TEST( setProtocolHeader( aSession,
                                 DKP_REQUEST_LINKER_DUMP,
                                 DKP_ADLP_PACKET_PARAM_NOT_USED,
                                 sFlags,
                                 DKP_LINKER_CONTROL_SESSION_ID,
                                 sPacketDataLen )
              != IDE_SUCCESS );

    IDE_TEST( dknLinkSend( aSession->mLink ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/************************************************************************
 * Description : sendLinkerDump operation 의 결과를 받는다.
 *               Operation이 성공적으로 수행되어 trace file이 남으면 success를
 *               반환한다.
 *
 * aSession         - [IN] Operation 을 수행하는 세션정보
 * aResultCode      - [OUT] Operation 의 수행 결과로 얻는 result code
 * aTimeoutSec      - [IN] CM block 의 읽기 timeout 값
 ************************************************************************/
IDE_RC  dkpProtocolMgr::recvLinkerDumpResult( dksSession *aSession,
                                              UInt       *aResultCode,
                                              ULong       aTimeoutSec )
{
    idBool                  sIsTimedOut = ID_FALSE;
    UInt                    sDataLen = 0;
    UChar                   sFlags = 0;

    /* Get cm block from protocol context */
    IDE_TEST_RAISE( getCmBlock( aSession,
                                &sIsTimedOut,
                                aTimeoutSec )
                    != IDE_SUCCESS, ERR_DISCONNECTED );

    IDE_TEST_RAISE( sIsTimedOut == ID_TRUE, ERR_DISCONNECTED );

    /* Analyze ADLP common header */
    IDE_TEST( analyzeCommonHeader( aSession,
                                   DKP_REQUEST_LINKER_DUMP_RESULT,
                                   &sFlags,
                                   DKP_LINKER_CONTROL_SESSION_ID,
                                   aResultCode,
                                   &sDataLen )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_DISCONNECTED );
    {
        IDE_ERRLOG( DK_TRC_LOG_FORCE );
        IDE_SET(ideSetErrorCode(dkERR_ABORT_ALTILINKER_DISCONNECTED));

        aSession->mIsNeedToDisconnect = ID_TRUE;
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/************************************************************************
 * Description : AltiLinker 프로세스로 하여금 스스로 종료 phase 를 수행
 *               하도록 하는 operation.
 *
 *  aSession    - [IN] Operation 을 수행하는 세션정보
 *
 ************************************************************************/
IDE_RC  dkpProtocolMgr::doLinkerShutdown( dksSession    *aSession )
{
    UInt                    sPacketDataLen = 0;

    IDE_ASSERT( aSession != NULL );

    /* Set ADLP protocol common header */
    IDE_TEST( setProtocolHeader( aSession,
                                 DKP_DO_LINKER_SHUTDOWN,
                                 DKP_ADLP_PACKET_PARAM_NOT_USED,
                                 DKP_ADLP_PACKET_FLAGS_NOT_USED,
                                 DKP_LINKER_CONTROL_SESSION_ID,
                                 sPacketDataLen )
              != IDE_SUCCESS );

    IDE_TEST( dknLinkSend( aSession->mLink ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 *
 */ 
IDE_RC dkpProtocolMgr::sendPrepareRemoteStmtInternal( dksSession       *aSession,
                                                      dkpOperationID    aOperationId,
                                                      UInt              aSessionId,
                                                      idBool            aIsTxBegin,
                                                      ID_XID           *aXID,
                                                      dkoLink          *aLinkInfo,
                                                      SLong             aRemoteStmtId,
                                                      SChar            *aRemoteStmtStr )
{
    UInt                    sTargetNameLen   = 0;
    UInt                    sRemoteUserIdLen = 0;
    UInt                    sRemotePasswdLen = 0;
    UInt                    sRemoteStmtLen   = 0;
    UInt                    sRemoteStrOffset = 0;
    UInt                    sPacketDataLen   = 0;
    UInt                    sInputParam      = 0;
    UChar                   sFlags           = makeHeaderFlags( aSession, aIsTxBegin );

    IDE_DASSERT( ( aOperationId == DKP_PREPARE_REMOTE_STMT ) ||
                 ( aOperationId == DKP_PREPARE_REMOTE_STMT_BATCH ) );
        
    /* Get the length of data */
    /* PROJ-2569 */
    // 항상 Data Session이어야만 한다.
    if ( DKP_ADLP_PACKET_FLAGS_IS_XA( sFlags ) == ID_TRUE )
    {
        sPacketDataLen += DKT_2PC_XIDDATASIZE;
    }
    else
    {
        /* do nothing */
    }

    /* Remote server (target) name */
    sPacketDataLen += ID_SIZEOF( sTargetNameLen );
    sTargetNameLen = idlOS::strlen( aLinkInfo->mTargetName );
    sPacketDataLen += sTargetNameLen;

    /* Remote server user id */
    sPacketDataLen += ID_SIZEOF( sRemoteUserIdLen );
    sRemoteUserIdLen = idlOS::strlen( aLinkInfo->mRemoteUserId );
    sPacketDataLen += sRemoteUserIdLen;

    /* Remote server password */
    sPacketDataLen += ID_SIZEOF( sRemotePasswdLen );
    sRemotePasswdLen = idlOS::strlen( aLinkInfo->mRemoteUserPasswd );
    sPacketDataLen += sRemotePasswdLen;

    /* Remote statement */
    sPacketDataLen += ID_SIZEOF( aRemoteStmtId );
    sPacketDataLen += ID_SIZEOF( sRemoteStmtLen );

    /* Check the length of remote statement string */
    sRemoteStmtLen = idlOS::strlen( aRemoteStmtStr );

    if ( sRemoteStmtLen > ( DKP_ADLP_PACKET_DATA_MAX_LEN - sPacketDataLen ) )
    {
        sInputParam      = DKP_ADLP_PACKET_PARAM_FRAGMENTED;
        sRemoteStrOffset = DKP_ADLP_PACKET_DATA_MAX_LEN - sPacketDataLen;
    }
    else
    {
        sInputParam = DKP_ADLP_PACKET_PARAM_NOT_FRAGMENTED;
        sRemoteStrOffset = sRemoteStmtLen;
    }

    sPacketDataLen += sRemoteStrOffset;


    /* Set ADLP protocol common header */
    IDE_TEST( setProtocolHeader( aSession, 
                                 aOperationId,
                                 sInputParam,
                                 sFlags,
                                 aSessionId,
                                 sPacketDataLen )
              != IDE_SUCCESS );
    /* PROJ-2569 */
    if ( DKP_ADLP_PACKET_FLAGS_IS_XA( sFlags ) == ID_TRUE )
    {
        IDE_DASSERT( aXID->gtrid_length + aXID->bqual_length
                     == DKT_2PC_XIDDATASIZE );
        IDE_TEST( dknLinkWrite( aSession->mLink,
                                (void *)aXID->data,
                                DKT_2PC_XIDDATASIZE )
                  != IDE_SUCCESS );
    }
    else
    {
        /* do nothing */
    }

    /* Set data to send with packet */
    IDE_TEST( dknLinkWriteFourByteNumber( aSession->mLink, &sTargetNameLen ) != IDE_SUCCESS );
    IDE_TEST( dknLinkWrite( aSession->mLink, (void *)aLinkInfo->mTargetName, sTargetNameLen ) != IDE_SUCCESS );

    IDE_TEST( dknLinkWriteFourByteNumber( aSession->mLink, &sRemoteUserIdLen ) != IDE_SUCCESS );
    IDE_TEST( dknLinkWrite( aSession->mLink, (void *)aLinkInfo->mRemoteUserId, sRemoteUserIdLen ) != IDE_SUCCESS );

    IDE_TEST( dknLinkWriteFourByteNumber( aSession->mLink, &sRemotePasswdLen ) != IDE_SUCCESS );
    IDE_TEST( dknLinkWrite( aSession->mLink, (void *)aLinkInfo->mRemoteUserPasswd, sRemotePasswdLen ) != IDE_SUCCESS );

    IDE_TEST( dknLinkWriteEightByteNumber( aSession->mLink, (void *)(&aRemoteStmtId) ) != IDE_SUCCESS );
    IDE_TEST( dknLinkWriteFourByteNumber( aSession->mLink, &sRemoteStmtLen ) != IDE_SUCCESS );
    IDE_TEST( dknLinkWrite( aSession->mLink, (void *)aRemoteStmtStr, sRemoteStmtLen ) != IDE_SUCCESS );

    IDE_TEST( dknLinkSend( aSession->mLink ) != IDE_SUCCESS );

    while( sInputParam == DKP_ADLP_PACKET_PARAM_FRAGMENTED )
    {
        sRemoteStmtLen -= sRemoteStrOffset;

        if ( sRemoteStmtLen > DKP_ADLP_PACKET_DATA_MAX_LEN )
        {
            sInputParam = DKP_ADLP_PACKET_PARAM_FRAGMENTED;
            sRemoteStrOffset = DKP_ADLP_PACKET_DATA_MAX_LEN;
        }
        else
        {
            sInputParam = DKP_ADLP_PACKET_PARAM_NOT_FRAGMENTED;
        }

        /* Set ADLP protocol common header */
        IDE_TEST( setProtocolHeader( aSession,
                                     DKP_PREPARE_REMOTE_STMT,
                                     sInputParam,
                                     sFlags,
                                     aSessionId,
                                     sRemoteStmtLen )
                  != IDE_SUCCESS );

        IDE_TEST( dknLinkWrite( aSession->mLink, (void *)aRemoteStmtStr, sRemoteStmtLen ) != IDE_SUCCESS );

        IDE_TEST( dknLinkSend( aSession->mLink ) != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 *
 */ 
IDE_RC dkpProtocolMgr::recvPrepareRemoteStmtResultInternal( dksSession     *aSession,
                                                            dkpOperationID  aOperationId,
                                                            UInt            aSessionId,
                                                            SLong           aRemoteStmtId,
                                                            UInt           *aResultCode, 
                                                            UShort         *aRemoteNodeSessionId, 
                                                            ULong           aTimeoutSec )
{
    idBool                  sIsTimedOut = ID_FALSE;
    UInt                    sDataLen = 0;
    SLong                   sRemoteStmtId = 0;
    UChar                   sFlags = 0;

    IDE_DASSERT( ( aOperationId == DKP_PREPARE_REMOTE_STMT_RESULT ) ||
                 ( aOperationId == DKP_PREPARE_REMOTE_STMT_BATCH_RESULT ) );

    /* Get cm block from protocol context */
    IDE_TEST_RAISE( getCmBlock( aSession,
                                &sIsTimedOut,
                                aTimeoutSec )
                    != IDE_SUCCESS, ERR_DISCONNECTED );

    IDE_TEST_RAISE( sIsTimedOut == ID_TRUE, ERR_DISCONNECTED );

    /* Analyze ADLP common header */
    IDE_TEST( analyzeCommonHeader( aSession,
                                   aOperationId,
                                   &sFlags,
                                   aSessionId,
                                   aResultCode,
                                   &sDataLen )
              != IDE_SUCCESS );

    if ( sDataLen >= 8 )
    {
        /* Get remote statement id */
        IDE_TEST( dknLinkReadEightByteNumber( aSession->mLink, (void *)&sRemoteStmtId ) != IDE_SUCCESS );
        IDE_TEST_RAISE ( sRemoteStmtId != aRemoteStmtId, ERR_REMOTE_STMT_ID );

        /* Get remote node session id */
        IDE_TEST( dknLinkReadTwoByteNumber( aSession->mLink, aRemoteNodeSessionId ) != IDE_SUCCESS );
    }
    else
    {
        /* no payload */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_DISCONNECTED );
    {
        IDE_ERRLOG( DK_TRC_LOG_FORCE );
        IDE_SET(ideSetErrorCode(dkERR_ABORT_ALTILINKER_DISCONNECTED));

        aSession->mIsNeedToDisconnect = ID_TRUE;
    }
    IDE_EXCEPTION( ERR_REMOTE_STMT_ID );
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKP_INVALID_REMOTE_STMT ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}



/************************************************************************
 * Description : AltiLinker 프로세스에 remote statement 의 prepare 를
 *               요청한다.
 *
 *  aSession        - [IN] Operation 을 수행하는 세션정보
 *  aSessionId      - [IN] Linker data session id 
 *  aLinkInfo       - [IN] Target server 로의 연결정보를 갖고 있는 구조체
 *  aRemoteStmtId   - [IN] 원격에서 수행할 remote statement 의 id
 *  aRemoteStmtStr  - [IN] Remote statement string 
 *
 ************************************************************************/
IDE_RC  dkpProtocolMgr::sendPrepareRemoteStmt( dksSession   *aSession, 
                                               UInt          aSessionId, 
                                               idBool        aIsTxBegin,
                                               ID_XID       *aXID,
                                               dkoLink      *aLinkInfo, 
                                               SLong         aRemoteStmtId, 
                                               SChar        *aRemoteStmtStr )
{
    IDE_TEST( sendPrepareRemoteStmtInternal( aSession,
                                             DKP_PREPARE_REMOTE_STMT,
                                             aSessionId,
                                             aIsTxBegin,
                                             aXID,
                                             aLinkInfo,
                                             aRemoteStmtId,
                                             aRemoteStmtStr )
              != IDE_SUCCESS );
            
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/************************************************************************
 * Description : sendPrepareRemoteStmt operation 의 결과를 받는다.
 *
 * aSession             - [IN] Operation 을 수행하는 세션정보
 * aSessionId           - [IN] Linker data session id 
 * aRemoteStmtId        - [IN] Remote statement id
 * aResultCode          - [OUT] Operation 의 수행 결과로 얻는 result code
 * aRemoteNodeSessionId - [OUT] Operation 수행결과로 얻어오는 remote
 *                              node session 의 id
 * aTimeoutSec          - [IN] CM block 의 읽기 timeout 값
 ************************************************************************/
IDE_RC dkpProtocolMgr::recvPrepareRemoteStmtResult( dksSession    *aSession, 
                                                    UInt           aSessionId,
                                                    SLong          aRemoteStmtId,
                                                    UInt          *aResultCode, 
                                                    UShort        *aRemoteNodeSessionId, 
                                                    ULong          aTimeoutSec )
{
    IDE_TEST( recvPrepareRemoteStmtResultInternal( aSession,
                                                   DKP_PREPARE_REMOTE_STMT_RESULT,
                                                   aSessionId,
                                                   aRemoteStmtId,
                                                   aResultCode, 
                                                   aRemoteNodeSessionId, 
                                                   aTimeoutSec )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 *
 */ 
IDE_RC  dkpProtocolMgr::sendPrepareRemoteStmtBatch( dksSession   *aSession, 
                                                    UInt          aSessionId,
                                                    idBool        aIsTxBegin,
                                                    ID_XID       *aXID,
                                                    dkoLink      *aLinkInfo, 
                                                    SLong         aRemoteStmtId, 
                                                    SChar        *aRemoteStmtStr )
{
    IDE_TEST( sendPrepareRemoteStmtInternal( aSession,
                                             DKP_PREPARE_REMOTE_STMT_BATCH,
                                             aSessionId,
                                             aIsTxBegin,
                                             aXID,
                                             aLinkInfo,
                                             aRemoteStmtId,
                                             aRemoteStmtStr )
              != IDE_SUCCESS );
            
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 *
 */ 
IDE_RC dkpProtocolMgr::recvPrepareRemoteStmtBatchResult( dksSession    *aSession, 
                                                         UInt           aSessionId,
                                                         SLong          aRemoteStmtId,
                                                         UInt          *aResultCode, 
                                                         UShort        *aRemoteNodeSessionId, 
                                                         ULong          aTimeoutSec )
{
    IDE_TEST( recvPrepareRemoteStmtResultInternal( aSession,
                                                   DKP_PREPARE_REMOTE_STMT_BATCH_RESULT,
                                                   aSessionId,
                                                   aRemoteStmtId,
                                                   aResultCode, 
                                                   aRemoteNodeSessionId, 
                                                   aTimeoutSec )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 *
 */ 
IDE_RC dkpProtocolMgr::sendRequestRemoteErrorInfo( dksSession    *aSession, 
                                                   UInt           aSessionId, 
                                                   SLong          aRemoteStmtId )
{
    UInt    sPacketDataLen   = 0;
    UChar   sFlags           = makeHeaderFlags( aSession, ID_FALSE );

    /* Get the length of data */
    sPacketDataLen += ID_SIZEOF( aRemoteStmtId );

    /* Set ADLP protocol common header */
    IDE_TEST( setProtocolHeader( aSession,
                                 DKP_REQUEST_REMOTE_ERROR_INFO,
                                 DKP_ADLP_PACKET_PARAM_NOT_USED,
                                 sFlags,
                                 aSessionId,
                                 sPacketDataLen )
              != IDE_SUCCESS );

    IDE_TEST( dknLinkWriteEightByteNumber( aSession->mLink, (void *)&aRemoteStmtId ) != IDE_SUCCESS );

    IDE_TEST( dknLinkSend( aSession->mLink ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/************************************************************************
 * Description : sendRequestRemoteErrorInfo operation 의 결과를 받는다.
 *
 *  aSession         - [IN] Operation 을 수행하는 세션정보
 *  aSessionId       - [IN] Operation 을 수행하는 session 의 id
 *  aResultCode      - [OUT] Operation 의 수행 결과로 얻는 result code
 *  aRemoteStmtId    - [IN] Remote statement id
 *  aErrorInfo       - [IN/OUT] Remote statement 의 에러정보
 *  aTimeoutSec      - [IN] CM block 의 읽기 timeout 값
 *
 ************************************************************************/
IDE_RC  dkpProtocolMgr::recvRequestRemoteErrorInfoResult( dksSession    *aSession,
                                                          UInt           aSessionId,
                                                          UInt          *aResultCode,
                                                          SLong          aRemoteStmtId,
                                                          dktErrorInfo  *aErrorInfo,
                                                          ULong          aTimeoutSec )
{
    idBool                  sIsTimedOut = ID_FALSE;
    SLong                   sRemoteStmtId;
    UInt                    sErrorMsgLen     = 0;
    UInt                    sDataLen         = 0;
    UChar                   sFlags = 0;

    /* Get cm block from protocol context */
    IDE_TEST_RAISE( getCmBlock( aSession,
                                &sIsTimedOut,
                                aTimeoutSec )
                    != IDE_SUCCESS, ERR_DISCONNECTED );

    IDE_TEST_RAISE( sIsTimedOut == ID_TRUE, ERR_DISCONNECTED );

    /* Analyze ADLP common header */
    IDE_TEST( analyzeCommonHeader( aSession,
                                   DKP_REQUEST_REMOTE_ERROR_INFO_RESULT,
                                   &sFlags,
                                   aSessionId,
                                   aResultCode,
                                   &sDataLen )
              != IDE_SUCCESS );

    if ( sDataLen >= 8 )
    {
        /* Get remote statement id */
        IDE_TEST( dknLinkReadEightByteNumber( aSession->mLink, (void *)&sRemoteStmtId ) != IDE_SUCCESS );
        IDE_TEST_RAISE ( sRemoteStmtId != aRemoteStmtId, ERR_INVALID_REMOTE_STMT );

        /* Get remote error info */
        IDE_TEST( dknLinkReadFourByteNumber( aSession->mLink, &(aErrorInfo->mErrorCode) ) != IDE_SUCCESS );

        IDE_TEST( dknLinkRead( aSession->mLink, (void *)aErrorInfo->mSQLState, DKT_SQL_ERROR_STATE_LEN )
                  != IDE_SUCCESS );
        aErrorInfo->mSQLState[DKT_SQL_ERROR_STATE_LEN] = '\0';

        IDE_TEST( dknLinkReadFourByteNumber( aSession->mLink, &sErrorMsgLen ) != IDE_SUCCESS );

        /* >> BUG-37588 */
        if ( sErrorMsgLen != 0 )
        {
            IDU_FIT_POINT_RAISE( "dkpProtocolMgr::recvRequestRemoteErrorInfoResult::calloc::ErrorInfo",
                                  ERR_MEMORY_ALLOC_REMOTE_ERR_INFO );
            IDE_TEST_RAISE( iduMemMgr::calloc( IDU_MEM_DK,
                                               1,
                                               (ULong)sErrorMsgLen + 1, /* BUG-37662 */
                                               (void **)&(aErrorInfo->mErrorDesc),
                                               IDU_MEM_IMMEDIATE )
                            != IDE_SUCCESS, ERR_MEMORY_ALLOC_REMOTE_ERR_INFO );

            IDE_TEST( dknLinkRead( aSession->mLink, aErrorInfo->mErrorDesc, sErrorMsgLen ) != IDE_SUCCESS );
        }
        else
        {
            /* no error description */
            sErrorMsgLen = idlOS::strlen( DKP_NO_ERROR_DESCRIPTION_STR );

            IDU_FIT_POINT_RAISE( "dkpProtocolMgr::recvRequestRemoteErrorInfoResult::calloc::ErrorInfo_NO_ERROR_DESCRIPTION",
                                  ERR_MEMORY_ALLOC_REMOTE_ERR_INFO );
            IDE_TEST_RAISE( iduMemMgr::calloc( IDU_MEM_DK,
                                               1,
                                               (ULong)sErrorMsgLen + 1, /* BUG-37662 */
                                               (void **)&(aErrorInfo->mErrorDesc),
                                               IDU_MEM_IMMEDIATE )
                            != IDE_SUCCESS, ERR_MEMORY_ALLOC_REMOTE_ERR_INFO );

            idlOS::strncpy( aErrorInfo->mErrorDesc,
                            DKP_NO_ERROR_DESCRIPTION_STR,
                            sErrorMsgLen );
        }
        /* << BUG-37588 */

        /* BUG-37662 */
        aErrorInfo->mErrorDesc[sErrorMsgLen] = '\0';
    }
    else
    {
        /* no payload */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_DISCONNECTED );
    {
        IDE_ERRLOG( DK_TRC_LOG_FORCE );
        IDE_SET(ideSetErrorCode(dkERR_ABORT_ALTILINKER_DISCONNECTED));

        aSession->mIsNeedToDisconnect = ID_TRUE;
    }
    IDE_EXCEPTION( ERR_INVALID_REMOTE_STMT );
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKM_INVALID_REMOTE_STMT ) );
    }
    IDE_EXCEPTION( ERR_MEMORY_ALLOC_REMOTE_ERR_INFO );
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_MEMORY_ALLOCATION ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/************************************************************************
 * Description : AltiLinker 프로세스에 remote statement 의 execute 를
 *               요청한다. Prepare operation 을 선행적으로 수행하지 않고
 *               이 operation 이 수행되는 경우 AltiLinker 프로세스에서
 *               이 operation 을 통해 전달받는 정보들을 이용해 prepare
 *               를 수행해야 하므로 target 정보 및 remote statement 정보
 *               들을 전달한다.
 *
 * aSession         - [IN] Operation 을 수행하는 세션정보
 * aSessionId       - [IN] Linker data session id
 * aLinkInfo        - [IN] Target server 로의 연결정보를 갖고 있는 구조체
 * aRemoteStmtInfo  - [IN] 원격에서 수행할 remote statement 정보
 ************************************************************************/
IDE_RC  dkpProtocolMgr::sendExecuteRemoteStmt( dksSession    *aSession,
                                               UInt           aSessionId,
                                               idBool         aIsTxBegin,
                                               ID_XID        *aXID,
                                               dkoLink       *aLinkInfo,
                                               SLong          aRemoteStmtId,
                                               UInt           aRemoteStmtType,
                                               SChar         *aRemoteStmtStr )
{
    UInt                    sTargetNameLen   = 0;
    UInt                    sRemoteUserIdLen = 0;
    UInt                    sRemotePasswdLen = 0;
    UInt                    sRemoteStmtLen   = 0;
    UInt                    sRemoteStrOffset = 0;
    UInt                    sPacketDataLen   = 0;
    UInt                    sInputParam = 0;
    UChar                   sFlags  = makeHeaderFlags( aSession, aIsTxBegin );

    /* Get the length of data */

    if ( DKP_ADLP_PACKET_FLAGS_IS_XA( sFlags ) == ID_TRUE )
    {
        sPacketDataLen += DKT_2PC_XIDDATASIZE;
    }
    else
    {
        /* do nothing */
    }

    /* Remote server (target) name */
    sPacketDataLen += ID_SIZEOF( sTargetNameLen );
    sTargetNameLen = idlOS::strlen( aLinkInfo->mTargetName );
    sPacketDataLen += sTargetNameLen;

    /* Remote server user id */
    sPacketDataLen += ID_SIZEOF( sRemoteUserIdLen );
    sRemoteUserIdLen = idlOS::strlen( aLinkInfo->mRemoteUserId );
    sPacketDataLen += sRemoteUserIdLen;

    /* Remote server password */
    sPacketDataLen += ID_SIZEOF( sRemotePasswdLen );
    sRemotePasswdLen = idlOS::strlen( aLinkInfo->mRemoteUserPasswd );
    sPacketDataLen += sRemotePasswdLen;

    /* Remote statement */
    sPacketDataLen += ID_SIZEOF( aRemoteStmtId );
    sPacketDataLen += ID_SIZEOF( aRemoteStmtType );
    sPacketDataLen += ID_SIZEOF( sRemoteStmtLen );

    /* Check the length of remote statement string */
    sRemoteStmtLen = idlOS::strlen( aRemoteStmtStr );

    if ( sRemoteStmtLen > ( DKP_ADLP_PACKET_DATA_MAX_LEN - sPacketDataLen ) )
    {
        sInputParam      = DKP_ADLP_PACKET_PARAM_FRAGMENTED;
        sRemoteStrOffset = DKP_ADLP_PACKET_DATA_MAX_LEN - sPacketDataLen;
    }
    else
    {
        sInputParam = DKP_ADLP_PACKET_PARAM_NOT_FRAGMENTED;
        sRemoteStrOffset = sRemoteStmtLen;
    }

    sPacketDataLen += sRemoteStrOffset;

    /* Set ADLP protocol common header */
    IDE_TEST( setProtocolHeader( aSession,
                                 DKP_EXECUTE_REMOTE_STMT,
                                 sInputParam,
                                 sFlags,
                                 aSessionId,
                                 sPacketDataLen )
              != IDE_SUCCESS );

    /* Set data to send with packet */
    if ( DKP_ADLP_PACKET_FLAGS_IS_XA( sFlags ) == ID_TRUE )
    {
        IDE_DASSERT( aXID->gtrid_length + aXID->bqual_length
                     == DKT_2PC_XIDDATASIZE );
        IDE_TEST( dknLinkWrite( aSession->mLink,
                                (void *)aXID->data,
                                DKT_2PC_XIDDATASIZE )
                  != IDE_SUCCESS );
    }
    else
    {
        /* do nothing */
    }

    IDE_TEST( dknLinkWriteFourByteNumber( aSession->mLink, &sTargetNameLen ) != IDE_SUCCESS );
    IDE_TEST( dknLinkWrite( aSession->mLink, (void *)aLinkInfo->mTargetName, sTargetNameLen ) != IDE_SUCCESS );

    IDE_TEST( dknLinkWriteFourByteNumber( aSession->mLink, &sRemoteUserIdLen ) != IDE_SUCCESS );
    IDE_TEST( dknLinkWrite( aSession->mLink, (void *)aLinkInfo->mRemoteUserId, sRemoteUserIdLen ) != IDE_SUCCESS );

    IDE_TEST( dknLinkWriteFourByteNumber( aSession->mLink, &sRemotePasswdLen ) != IDE_SUCCESS );
    IDE_TEST( dknLinkWrite( aSession->mLink, (void *)aLinkInfo->mRemoteUserPasswd, sRemotePasswdLen ) != IDE_SUCCESS );

    IDE_TEST( dknLinkWriteEightByteNumber( aSession->mLink, (void *)&aRemoteStmtId ) != IDE_SUCCESS );

    IDE_TEST( dknLinkWriteFourByteNumber( aSession->mLink, &aRemoteStmtType ) != IDE_SUCCESS );

    IDE_TEST( dknLinkWriteFourByteNumber( aSession->mLink, &sRemoteStmtLen ) != IDE_SUCCESS );

    IDE_TEST( dknLinkWrite( aSession->mLink, (void *)aRemoteStmtStr, sRemoteStrOffset ) != IDE_SUCCESS );

    IDE_TEST( dknLinkSend( aSession->mLink ) != IDE_SUCCESS );

    while ( sInputParam == DKP_ADLP_PACKET_PARAM_FRAGMENTED )
    {
        sRemoteStmtLen -= sRemoteStrOffset;

        if ( sRemoteStmtLen > DKP_ADLP_PACKET_DATA_MAX_LEN )
        {
            sInputParam = DKP_ADLP_PACKET_PARAM_FRAGMENTED;
            sRemoteStrOffset = DKP_ADLP_PACKET_DATA_MAX_LEN;
        }
        else
        {
            sInputParam = DKP_ADLP_PACKET_PARAM_NOT_FRAGMENTED;
        }
        /* Set ADLP protocol common header */
        IDE_TEST( setProtocolHeader( aSession,
                                     DKP_EXECUTE_REMOTE_STMT,
                                     sInputParam,
                                     sFlags,
                                     aSessionId,
                                     sRemoteStmtLen )
                  != IDE_SUCCESS );

        IDE_TEST( dknLinkWrite( aSession->mLink, (void *)aRemoteStmtStr, sRemoteStmtLen ) != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/************************************************************************
 * Description : sendExecuteRemoteStmt operation 의 결과를 받는다.
 *
 *  aSession             - [IN] Operation 을 수행하는 세션정보
 *  aSessionId           - [IN] Linker data session id
 *  aRemoteStmtId        - [IN] Remote statement id
 *  aResultCode          - [OUT] Operation 의 수행 결과로 얻는 result code
 *  aRemoteNodeSessionId - [OUT] Operation 수행결과로 얻어오는 remote
 *                               node session 의 id
 *  aResults             - [OUT] Execute Result Value
 *      ++ 이 값은 result code 가 SUCCESS 인 경우만 유효하다.
 *         만약 수행한 statement type 이 DML 인 경우는 row 의 갯수, DDL
 *         인 경우는 SUCCESS 인 경우 0, 수행 실패의 경우는 -1 을 반환한다.
 *         이 경우 caller 는 result code 를 확인하여 필요한 동작을 수행하
 *         도록 해야한다.
 *
 *  aTimeoutSec          - [IN] CM block 의 읽기 timeout 값
 *
 ************************************************************************/
IDE_RC  dkpProtocolMgr::recvExecuteRemoteStmtResult( dksSession    *aSession,
                                                     UInt           aSessionId,
                                                     SLong          aRemoteStmtId,
                                                     UInt          *aResultCode,
                                                     UShort        *aRemoteNodeSessionId,
                                                     UInt          *aStmtType,
                                                     SInt          *aResult,
                                                     ULong          aTimeoutSec )
{
    idBool                  sIsTimedOut = ID_FALSE;
    UInt                    sDataLen = 0;
    SLong                   sRemoteStmtId = 0;
    UChar                   sFlags = 0;

    /* Get cm block from protocol context */
    IDE_TEST_RAISE( getCmBlock( aSession,
                                &sIsTimedOut,
                                aTimeoutSec )
                    != IDE_SUCCESS, ERR_DISCONNECTED );

    IDE_TEST_RAISE( sIsTimedOut == ID_TRUE, ERR_DISCONNECTED );

    /* Analyze ADLP common header */
    IDE_TEST( analyzeCommonHeader( aSession,
                                   DKP_EXECUTE_REMOTE_STMT_RESULT,
                                   &sFlags,
                                   aSessionId,
                                   aResultCode,
                                   &sDataLen )
              != IDE_SUCCESS );

    if ( sDataLen >= 8 )
    {
        /* Get remote statement id */
        IDE_TEST( dknLinkReadEightByteNumber( aSession->mLink, (void *)&sRemoteStmtId ) != IDE_SUCCESS );
        IDE_TEST_RAISE ( sRemoteStmtId != aRemoteStmtId, ERR_REMOTE_STMT_ID );

        /* Get remote node session id */
        IDE_TEST( dknLinkReadTwoByteNumber( aSession->mLink, aRemoteNodeSessionId ) != IDE_SUCCESS );

        /* Get the execution result value */
        IDE_TEST( dknLinkReadFourByteNumber( aSession->mLink, (void *)aStmtType ) != IDE_SUCCESS );

        /* Get the execution result value */
        IDE_TEST( dknLinkReadFourByteNumber( aSession->mLink, (void *)aResult ) != IDE_SUCCESS );
    }
    else
    {
        /* no payload */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_DISCONNECTED );
    {
        IDE_ERRLOG( DK_TRC_LOG_FORCE );
        IDE_SET(ideSetErrorCode(dkERR_ABORT_ALTILINKER_DISCONNECTED));

        aSession->mIsNeedToDisconnect = ID_TRUE;
    }
    IDE_EXCEPTION( ERR_REMOTE_STMT_ID );
    {
        IDE_SET(ideSetErrorCode( dkERR_ABORT_DKM_INVALID_REMOTE_STMT ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/*
 *
 */
IDE_RC dkpProtocolMgr::sendPrepareAddBatch( dksSession           *aSession,
                                            UInt                  aSessionId,
                                            SLong                 aRemoteStmtId,
                                            dkpBatchStatementMgr *aManager )
{
    UInt                    sPacketDataLen = 0;
    UInt                    sBatchCount = 0;
    UChar                   sFlags         = makeHeaderFlags( aSession, ID_FALSE );

    sBatchCount = dkpBatchStatementMgrCountBatchStatement( aManager );
    
    /* Get the length of data */
    sPacketDataLen += ID_SIZEOF( aRemoteStmtId ) + ID_SIZEOF( sBatchCount );

    /* Set ADLP protocol common header */
    IDE_TEST( setProtocolHeader( aSession,
                                 DKP_PREPARE_ADD_BATCH,
                                 DKP_ADLP_PACKET_PARAM_NOT_USED, 
                                 sFlags,
                                 aSessionId, 
                                 sPacketDataLen ) 
              != IDE_SUCCESS );

    IDE_TEST( dknLinkWriteEightByteNumber( aSession->mLink, (void *)&aRemoteStmtId ) != IDE_SUCCESS );

    IDE_TEST( dknLinkWriteFourByteNumber( aSession->mLink, (void *)&sBatchCount ) != IDE_SUCCESS );
    
    IDE_TEST( dknLinkSend( aSession->mLink ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;    
}

/*
 *
 */  
IDE_RC dkpProtocolMgr::recvPrepareAddBatch( dksSession *aSession,
                                            UInt        aSessionId,
                                            SLong       aRemoteStmtId,
                                            UInt       *aResultCode,
                                            ULong       aTimeoutSec )
{
    idBool                  sIsTimedOut = ID_FALSE;
    UInt                    sDataLen = 0;
    SLong                   sRemoteStmtId = 0;
    UChar                   sFlags = 0;

    /* Get cm block from protocol context */
    IDE_TEST_RAISE( getCmBlock( aSession, 
                                &sIsTimedOut, 
                                aTimeoutSec ) 
                    != IDE_SUCCESS, ERR_DISCONNECTED );

    IDE_TEST_RAISE( sIsTimedOut == ID_TRUE, ERR_DISCONNECTED );

    /* Analyze ADLP common header */
    IDE_TEST( analyzeCommonHeader( aSession,
                                   DKP_PREPARE_ADD_BATCH_RESULT,
                                   &sFlags,                                   
                                   aSessionId,
                                   aResultCode,
                                   &sDataLen )
              != IDE_SUCCESS );

    if ( sDataLen >= ID_SIZEOF( SLong ) )
    {
        /* Get remote statement id */
        IDE_TEST( dknLinkReadEightByteNumber( aSession->mLink, (void *)&sRemoteStmtId ) != IDE_SUCCESS );
        IDE_TEST_RAISE ( sRemoteStmtId != aRemoteStmtId, ERR_REMOTE_STMT_ID );
    }
    else
    {
        /* no payload */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_DISCONNECTED );
    {
        IDE_ERRLOG( DK_TRC_LOG_FORCE );
        IDE_SET(ideSetErrorCode(dkERR_ABORT_ALTILINKER_DISCONNECTED));

        aSession->mIsNeedToDisconnect = ID_TRUE;
    }
    IDE_EXCEPTION( ERR_REMOTE_STMT_ID );
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKM_INVALID_REMOTE_STMT ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;    
}

/*
 *
 */ 
IDE_RC  dkpProtocolMgr::setProtocolHeader( dknPacket    *aPacket,
                                           UChar         aOpID,
                                           UChar         aParam,
                                           UChar         aFlags,                                           
                                           UInt          aSessionID,
                                           UInt          aDataLength )
{
    UChar sDummyReserved = DKP_ADLP_PACKET_RESERVED_NOT_USED;
    
    dknPacketRewind( aPacket );
    
    /* Set operation id */
    IDE_TEST( dknPacketWriteOneByteNumber( aPacket, &aOpID ) != IDE_SUCCESS );

    /* Set operation specific parameter */
    IDE_TEST( dknPacketWriteOneByteNumber( aPacket, &aParam ) != IDE_SUCCESS );

    /* PROJ-2569 */
    IDE_TEST( dknPacketWriteOneByteNumber( aPacket, &aFlags ) != IDE_SUCCESS );

    /* Set Reserved*/
    IDE_TEST( dknPacketWriteOneByteNumber( aPacket, &sDummyReserved ) != IDE_SUCCESS );
    
    /* Set session id */
    IDE_TEST( dknPacketWriteFourByteNumber( aPacket, &aSessionID ) != IDE_SUCCESS );

    /* Set length of data */
    IDE_TEST( dknPacketWriteFourByteNumber( aPacket, &aDataLength ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;    
}

/*
 *
 */ 
IDE_RC dkpProtocolMgr::sendAddBatches( dksSession           *aSession,
                                       UInt                  aSessionId,
                                       dkpBatchStatementMgr *aManager )
{
    dkpBatchStatement * sBatchStatement = NULL;
    dknPacket * sPacket = NULL;
    UInt sPacketCount = 0;
    UChar sPacketHeaderParam = 0;
    UInt sPacketDataLength = 0;
    UChar sFlags           = makeHeaderFlags( aSession, ID_FALSE );
                                           
    IDE_TEST( dkpBatchStatementMgrGetNextBatchStatement( aManager, &sBatchStatement )
              != IDE_SUCCESS );
    
    while ( sBatchStatement != NULL )
    {
        IDE_TEST( dkpBatchStatementMgrGetNextPacket( sBatchStatement, &sPacket )
                  != IDE_SUCCESS );

        sPacketCount = dkpBatchStatementMgrCountPacket( sBatchStatement );
        
        while ( sPacketCount > 0 )
        {
            IDE_DASSERT( sPacket != NULL );

            if ( sPacketCount > 1 )
            {
                sPacketHeaderParam = DKP_ADLP_PACKET_PARAM_FRAGMENTED;
            }
            else
            {
                sPacketHeaderParam = DKP_ADLP_PACKET_PARAM_NOT_FRAGMENTED;
            }

            sPacketDataLength = dknPacketGetPayloadLength( sPacket ) - DKP_ADLP_HDR_LEN;

            IDE_TEST( setProtocolHeader( sPacket,
                                         DKP_ADD_BATCH,
                                         sPacketHeaderParam,
                                         sFlags,
                                         aSessionId, 
                                         sPacketDataLength ) 
                      != IDE_SUCCESS );
            
            IDE_TEST( dknLinkSendPacket( aSession->mLink, sPacket ) != IDE_SUCCESS );
            
            IDE_TEST( dkpBatchStatementMgrGetNextPacket( sBatchStatement, &sPacket )
                      != IDE_SUCCESS );

            sPacketCount--;
        }
        
        IDE_TEST( dkpBatchStatementMgrGetNextBatchStatement( aManager, &sBatchStatement )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

/*
 *
 */
IDE_RC dkpProtocolMgr::recvAddBatchResult( dksSession    *aSession,
                                           UInt           aSessionId,
                                           SLong          aRemoteStmtId,
                                           UInt          *aResultCode,
                                           UInt          *aProcessedBatchCount,
                                           ULong          aTimeoutSec )
{
    idBool                  sIsTimedOut = ID_FALSE;
    UInt                    sDataLen = 0;
    SLong                   sRemoteStmtId = 0;
    UChar                   sFlags = 0;
    
    /* Get cm block from protocol context */
    IDE_TEST_RAISE( getCmBlock( aSession, 
                                &sIsTimedOut, 
                                aTimeoutSec ) 
                    != IDE_SUCCESS, ERR_DISCONNECTED );

    IDE_TEST_RAISE( sIsTimedOut == ID_TRUE, ERR_DISCONNECTED );

    /* Analyze ADLP common header */
    IDE_TEST( analyzeCommonHeader( aSession,
                                   DKP_ADD_BATCH_RESULT,
                                   &sFlags,
                                   aSessionId,
                                   aResultCode,
                                   &sDataLen )
              != IDE_SUCCESS );

    if ( sDataLen >= ID_SIZEOF( SLong ) + ID_SIZEOF( UInt ) )
    {
        /* Get remote statement id */
        IDE_TEST( dknLinkReadEightByteNumber( aSession->mLink, (void *)&sRemoteStmtId ) != IDE_SUCCESS );
        IDE_TEST_RAISE ( sRemoteStmtId != aRemoteStmtId, ERR_REMOTE_STMT_ID );

        IDE_TEST( dknLinkReadFourByteNumber( aSession->mLink, (void *)aProcessedBatchCount ) != IDE_SUCCESS );
    }
    else
    {
        /* no payload */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_DISCONNECTED );
    {
        IDE_ERRLOG( DK_TRC_LOG_FORCE );
        IDE_SET(ideSetErrorCode(dkERR_ABORT_ALTILINKER_DISCONNECTED));

        aSession->mIsNeedToDisconnect = ID_TRUE;
    }
    IDE_EXCEPTION( ERR_REMOTE_STMT_ID );
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKM_INVALID_REMOTE_STMT ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;   
}

/*
 *
 */ 
IDE_RC  dkpProtocolMgr::sendExecuteBatch( dksSession    *aSession, 
                                          UInt           aSessionId,
                                          SLong          aRemoteStmtId )
{
    UInt                    sPacketDataLen = 0;
    UChar                   sFlags         = makeHeaderFlags( aSession, ID_FALSE );

    /* Get the length of data */
    sPacketDataLen += ID_SIZEOF( aRemoteStmtId );

    /* Set ADLP protocol common header */
    IDE_TEST( setProtocolHeader( aSession,
                                 DKP_EXECUTE_REMOTE_STMT_BATCH,
                                 DKP_ADLP_PACKET_PARAM_NOT_USED, 
                                 sFlags,
                                 aSessionId, 
                                 sPacketDataLen ) 
              != IDE_SUCCESS );

    IDE_TEST( dknLinkWriteEightByteNumber( aSession->mLink, (void *)&aRemoteStmtId ) != IDE_SUCCESS );

    IDE_TEST( dknLinkSend( aSession->mLink ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 *
 */
IDE_RC dkpProtocolMgr::recvExecuteBatch( dksSession    *aSession, 
                                         UInt           aSessionId,
                                         SLong          aRemoteStmtId,
                                         SInt          *aResultArray,
                                         UInt          *aResultCode,
                                         ULong          aTimeoutSec )
{
    idBool                  sIsTimedOut = ID_FALSE;
    UInt                    sDataLen = 0;
    UInt                    sRemainedDataLength = 0;
    SLong                   sRemoteStmtId = 0;
    SInt                    sResultCount = 0;
    SInt                    i = 0;
    UChar                   sFlags = 0;    

    /* Get cm block from protocol context */
    IDE_TEST_RAISE( getCmBlock( aSession, 
                                &sIsTimedOut, 
                                aTimeoutSec ) 
                    != IDE_SUCCESS, ERR_DISCONNECTED );

    IDE_TEST_RAISE( sIsTimedOut == ID_TRUE, ERR_DISCONNECTED );

    /* Analyze ADLP common header */
    IDE_TEST( analyzeCommonHeader( aSession,
                                   DKP_EXECUTE_REMOTE_STMT_BATCH_RESULT,
                                   &sFlags,
                                   aSessionId,
                                   aResultCode,
                                   &sDataLen )
              != IDE_SUCCESS );

    sRemainedDataLength = sDataLen;
    if ( sDataLen >= ID_SIZEOF( sRemoteStmtId ) )
    {
        /* Get remote statement id */
        IDE_TEST( dknLinkReadEightByteNumber( aSession->mLink, (void *)&sRemoteStmtId ) != IDE_SUCCESS );
        IDE_TEST_RAISE ( sRemoteStmtId != aRemoteStmtId, ERR_REMOTE_STMT_ID );
        sRemainedDataLength -= 8;

        IDE_TEST( dknLinkReadFourByteNumber( aSession->mLink, (void *)&sResultCount ) != IDE_SUCCESS );
        sRemainedDataLength -= 4;
        
        for ( i = 0; i < sResultCount; i++ )
        {
            IDE_TEST( dknLinkReadFourByteNumber( aSession->mLink, (void *)&(aResultArray[i]) ) != IDE_SUCCESS );
            sRemainedDataLength -= 4;

            if ( ( i < sResultCount - 1 ) &&
                 ( sRemainedDataLength == 0 ) )
            {
                /* Get next cm block */
                IDE_TEST( getCmBlock( aSession, 
                                      &sIsTimedOut,
                                      aTimeoutSec ) 
                          != IDE_SUCCESS );

                /* Analyze ADLP common header */
                IDE_TEST( analyzeCommonHeader( aSession, 
                                               DKP_EXECUTE_REMOTE_STMT_BATCH_RESULT,
                                               &sFlags,
                                               aSessionId,
                                               aResultCode,
                                               &sDataLen ) 
                          != IDE_SUCCESS );

                sRemainedDataLength = sDataLen;
            }
            else
            {
                /* nothing to do */
            }
        }
    }
    else
    {
        /* no payload */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_DISCONNECTED );
    {
        IDE_ERRLOG( DK_TRC_LOG_FORCE );
        IDE_SET(ideSetErrorCode(dkERR_ABORT_ALTILINKER_DISCONNECTED));

        aSession->mIsNeedToDisconnect = ID_TRUE;
    }
    IDE_EXCEPTION( ERR_REMOTE_STMT_ID );
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKM_INVALID_REMOTE_STMT ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 *
 */ 
IDE_RC dkpProtocolMgr::sendRequestFreeRemoteStmtInternal( dksSession      *aSession,
                                                          dkpOperationID  aOperationId,
                                                          UInt            aSessionId,
                                                          SLong           aRemoteStmtId )
{
    UInt                    sPacketDataLen = 0;
    UChar                   sFlags         = makeHeaderFlags( aSession, ID_FALSE );

    IDE_DASSERT( ( aOperationId == DKP_REQUEST_FREE_REMOTE_STMT ) ||
                 ( aOperationId == DKP_REQUEST_FREE_REMOTE_STMT_BATCH ) );

    /* Get the length of data */
    sPacketDataLen += ID_SIZEOF( aRemoteStmtId );

    /* Set ADLP protocol common header */
    IDE_TEST( setProtocolHeader( aSession,
                                 aOperationId,
                                 DKP_ADLP_PACKET_PARAM_NOT_USED, 
                                 sFlags,
                                 aSessionId, 
                                 sPacketDataLen ) 
              != IDE_SUCCESS );

    IDE_TEST( dknLinkWriteEightByteNumber( aSession->mLink, (void *)&aRemoteStmtId ) != IDE_SUCCESS );

    IDE_TEST( dknLinkSend( aSession->mLink ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 *
 */ 
IDE_RC  dkpProtocolMgr::recvRequestFreeRemoteStmtResultInternal( dksSession     *aSession,
                                                                 dkpOperationID  aOperationId,
                                                                 UInt            aSessionId, 
                                                                 SLong           aRemoteStmtId, 
                                                                 UInt           *aResultCode, 
                                                                 ULong           aTimeoutSec )
{
    idBool                  sIsTimedOut = ID_FALSE;
    UInt                    sDataLen = 0;
    SLong                   sRemoteStmtId = 0;
    UChar                   sFlags = 0;

    IDE_DASSERT( ( aOperationId == DKP_REQUEST_FREE_REMOTE_STMT_RESULT ) ||
                 ( aOperationId == DKP_REQUEST_FREE_REMOTE_STMT_BATCH_RESULT ) );
    
    /* Get cm block from protocol context */
    IDE_TEST_RAISE( getCmBlock( aSession,
                                &sIsTimedOut,
                                aTimeoutSec )
                    != IDE_SUCCESS, ERR_DISCONNECTED );

    IDE_TEST_RAISE( sIsTimedOut == ID_TRUE, ERR_DISCONNECTED );

    /* Analyze ADLP common header */
    IDE_TEST( analyzeCommonHeader( aSession,
                                   aOperationId,
                                   &sFlags,
                                   aSessionId,
                                   aResultCode,
                                   &sDataLen )
              != IDE_SUCCESS );

    if ( sDataLen >= 8 )
    {
        /* Get remote statement id */
        IDE_TEST( dknLinkReadEightByteNumber( aSession->mLink, (void *)&sRemoteStmtId ) != IDE_SUCCESS );
        IDE_TEST_RAISE ( sRemoteStmtId != aRemoteStmtId, ERR_REMOTE_STMT_ID );
    }
    else
    {
        /* no payload */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_DISCONNECTED );
    {
        IDE_ERRLOG( DK_TRC_LOG_FORCE );
        IDE_SET(ideSetErrorCode(dkERR_ABORT_ALTILINKER_DISCONNECTED));

        aSession->mIsNeedToDisconnect = ID_TRUE;
    }
    IDE_EXCEPTION( ERR_REMOTE_STMT_ID );
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKM_INVALID_REMOTE_STMT ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/************************************************************************
 * Description : Remote statement 를 종료하기 위한 operation.
 * 
 *  aSession         - [IN] Operation 을 수행하는 세션정보
 *  aSessionId       - [IN] Linker data session id 
 *  aRemoteStmtId    - [IN] Remote statement id
 *
 ************************************************************************/
IDE_RC  dkpProtocolMgr::sendRequestFreeRemoteStmt( dksSession    *aSession,
                                                   UInt           aSessionId,
                                                   SLong          aRemoteStmtId )
{
    IDE_TEST( sendRequestFreeRemoteStmtInternal( aSession,
                                                 DKP_REQUEST_FREE_REMOTE_STMT,
                                                 aSessionId,
                                                 aRemoteStmtId )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/************************************************************************
 * Description : sendRequestFreeRemoteStmt operation 의 결과를 받는다.
 *
 *  aSession             - [IN] Operation 을 수행하는 세션정보
 *  aSessionId           - [IN] Linker data session id 
 *  aRemoteStmtId        - [IN] Remote statement id
 *  aResultCode          - [OUT] Operation 의 수행 결과로 얻는 result code
 *                               node session 의 id
 *  aTimeoutSec          - [IN] CM block 의 읽기 timeout 값
 *
 ************************************************************************/
IDE_RC  dkpProtocolMgr::recvRequestFreeRemoteStmtResult( dksSession    *aSession, 
                                                         UInt           aSessionId, 
                                                         SLong          aRemoteStmtId, 
                                                         UInt          *aResultCode, 
                                                         ULong          aTimeoutSec )
{
    IDE_TEST( recvRequestFreeRemoteStmtResultInternal( aSession,
                                                       DKP_REQUEST_FREE_REMOTE_STMT_RESULT,
                                                       aSessionId, 
                                                       aRemoteStmtId, 
                                                       aResultCode, 
                                                       aTimeoutSec )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 *
 */ 
IDE_RC dkpProtocolMgr::sendRequestFreeRemoteStmtBatch( dksSession    *aSession,
                                                       UInt           aSessionId,
                                                       SLong          aRemoteStmtId )
{
    IDE_TEST( sendRequestFreeRemoteStmtInternal( aSession,
                                                 DKP_REQUEST_FREE_REMOTE_STMT_BATCH,
                                                 aSessionId,
                                                 aRemoteStmtId )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 *
 */ 
IDE_RC dkpProtocolMgr::recvRequestFreeRemoteStmtBatchResult( dksSession    *aSession, 
                                                             UInt           aSessionId, 
                                                             SLong          aRemoteStmtId, 
                                                             UInt          *aResultCode, 
                                                             ULong          aTimeoutSec )
{
    IDE_TEST( recvRequestFreeRemoteStmtResultInternal( aSession,
                                                       DKP_REQUEST_FREE_REMOTE_STMT_BATCH_RESULT,
                                                       aSessionId, 
                                                       aRemoteStmtId, 
                                                       aResultCode, 
                                                       aTimeoutSec )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/************************************************************************
 * Description : Remote query 의 수행결과가 SUCCESS 인 경우, 그 결과집합
 *               인 row 들을 fetch 해오기 위한 operation.
 *
 *  aSession         - [IN] Operation 을 수행하는 세션정보
 *  aSessionId       - [IN] Linker data session id
 *  aRemoteStmtId    - [IN] Remote statement id
 *  aFetchRowCount   - [IN] Fetch 할 row 의 갯수
 *
 ************************************************************************/
IDE_RC  dkpProtocolMgr::sendFetchResultSet( dksSession    *aSession,
                                            UInt           aSessionId,
                                            SLong          aRemoteStmtId,
                                            UInt           aFetchRowCount,
                                            UInt           aBufferSize)
{
    UInt                    sPacketDataLen = 0;
    UChar                   sFlags         = makeHeaderFlags( aSession, ID_FALSE );

    /* Get the length of data */
    sPacketDataLen += ID_SIZEOF( aRemoteStmtId );
    sPacketDataLen += ID_SIZEOF( aFetchRowCount );
    sPacketDataLen += ID_SIZEOF( aBufferSize );
    /* Set ADLP protocol common header */
    IDE_TEST( setProtocolHeader( aSession,
                                 DKP_FETCH_RESULT_SET,
                                 DKP_ADLP_PACKET_PARAM_NOT_USED,
                                 sFlags,
                                 aSessionId,
                                 sPacketDataLen )
              != IDE_SUCCESS );

    IDE_TEST( dknLinkWriteEightByteNumber( aSession->mLink, (void *)&aRemoteStmtId ) != IDE_SUCCESS );
    IDE_TEST( dknLinkWriteFourByteNumber( aSession->mLink, &aFetchRowCount ) != IDE_SUCCESS );
    IDE_TEST( dknLinkWriteFourByteNumber( aSession->mLink, &aBufferSize ) != IDE_SUCCESS );

    IDE_TEST( dknLinkSend( aSession->mLink ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/************************************************************************
 * Description : sendFetchResultSet operation 의 결과를 받는다.
 *
 *  aSession        - [IN] Operation 을 수행하는 세션정보
 *  aSessionId      - [IN] Linker data session id
 *  aRemoteStmtId   - [IN] Remote statement id
 *  aRecvRowBuffer  - [IN] AltiLinker process 로부터 받은 packet 의
 *                         data 들을 옮겨담기 위한 buffer
 *  aRecvRowLen     - [IN] aRecvRowBuffer 에 담은 총 크기
 *  aRecvRowCnt     - [IN] AltiLinker process 로부터 받은 row 의 갯수
 *  aResultCode     - [OUT] Operation 의 수행 결과로 얻는 result code
 *                          node session 의 id
 *  aTimeoutSec     - [IN] CM block 의 읽기 timeout 값
 *
 ************************************************************************/
IDE_RC  dkpProtocolMgr::recvFetchResultRow( dksSession    *aSession,
                                            UInt           aSessionId,
                                            SLong          aRemoteStmtId,
                                            SChar         *aRecvRowBuffer,
                                            UInt           aRecvRowBufferSize,
                                            UInt          *aRecvRowLen,
                                            UInt          *aRecvRowCnt,
                                            UInt          *aResultCode,
                                            ULong          aTimeoutSec )
{
    idBool                  sIsTimedOut = ID_FALSE;
    UInt                    i;
    UInt                    j;
    UInt                    sDataLen;
    UInt                    sRecordLen;
    UInt                    sRecordLenSize   = 0;
    UInt                    sRecordCnt       = 0;
    UInt                    sColumnCnt       = 0;
    UInt                    sColumnValLen    = 0;
    UInt                    sColValLenSize   = 0;
    UInt                    sRecvRowLen      = 0;
    SLong                   sRemoteStmtId;
    SChar                 * sRecvRowBuffer   = NULL;
    SChar                 * sRowStartOffset  = NULL;
    UChar                   sFlags = 0;

    sRecvRowBuffer = aRecvRowBuffer;
    sColValLenSize = ID_SIZEOF( sColumnValLen );
    sRecordLenSize = ID_SIZEOF( sRecordLen );

    /* Get cm block from protocol context */
    IDE_TEST_RAISE( getCmBlock( aSession,
                                &sIsTimedOut,
                                aTimeoutSec )
                    != IDE_SUCCESS, ERR_DISCONNECTED );

    IDE_TEST_RAISE( sIsTimedOut == ID_TRUE, ERR_DISCONNECTED );

    /* Analyze ADLP common header */
    IDE_TEST( analyzeCommonHeader( aSession,
                                   DKP_FETCH_RESULT_ROW,
                                   &sFlags,
                                   aSessionId,
                                   aResultCode,
                                   &sDataLen )
              != IDE_SUCCESS );

    if ( sDataLen >= 8 )
    {
        /* Get remote statement id */
        IDE_TEST( dknLinkReadEightByteNumber( aSession->mLink, (void *)&sRemoteStmtId ) != IDE_SUCCESS );
        IDE_TEST_RAISE ( sRemoteStmtId != aRemoteStmtId, ERR_REMOTE_STMT_ID );

        if ( ( *aResultCode == DKP_RC_SUCCESS ) ||
             ( *aResultCode == DKP_RC_SUCCESS_FRAGMENTED ) )
        {
            IDE_TEST( dknLinkReadFourByteNumber( aSession->mLink, &sRecordCnt ) != IDE_SUCCESS );

            for ( i = 0; ( i < sRecordCnt ) || ( *aResultCode == DKP_RC_SUCCESS_FRAGMENTED ); i++ )
            {
                sRowStartOffset = sRecvRowBuffer;

                sRecordLen = 0;

                /* BUG-37850 */
                if ( *aRecvRowCnt != DKP_FRAGMENTED_ROW )
                {
                    sRecvRowBuffer += sRecordLenSize;
                }
                else
                {
                    /* do nothing */
                }

                IDE_TEST( dknLinkReadFourByteNumber( aSession->mLink, &sColumnCnt ) != IDE_SUCCESS );

                for ( j = 0; j < sColumnCnt; j++ )
                {
                    IDE_TEST( dknLinkReadFourByteNumber( aSession->mLink, &sColumnValLen ) != IDE_SUCCESS );

                    idlOS::memcpy( sRecvRowBuffer, (SInt *)&sColumnValLen, sColValLenSize );

                    sRecvRowBuffer += sColValLenSize;
                    aRecvRowBufferSize -= sColValLenSize;

                    if ( (SInt)sColumnValLen != -1 )
                    {
                        IDE_TEST( readLinkBufferInternal( aSession->mLink,
                                                          (void *)sRecvRowBuffer,
                                                          aRecvRowBufferSize,
                                                          &sColumnValLen,
                                                          ID_FALSE )
                                  != IDE_SUCCESS );

                        sRecvRowBuffer += sColumnValLen;
                        aRecvRowBufferSize -= sColumnValLen;

                        sRecordLen += ( sColValLenSize + sColumnValLen );
                    }
                    else
                    {
                        sRecordLen += sColValLenSize;
                    }
                }

                /* BUG-37850 */
                if ( *aRecvRowCnt != DKP_FRAGMENTED_ROW )
                {
                    idlOS::memcpy( sRowStartOffset, &sRecordLen, sRecordLenSize );
                }
                else
                {
                    /* do nothing */
                }

                sRecvRowLen += sRecordLen;

                if ( sRecordCnt == 0 )
                {
                    break;
                }
                else
                {
                    /* next record */
                }
            }

            *aRecvRowLen = sRecvRowLen;
            *aRecvRowCnt = sRecordCnt;

            /* BUG-36873 */
            aRecvRowBuffer = sRecvRowBuffer;
        }
        else
        {
            /* just pass result code to caller */
        }
    }
    else
    {
        /* no payload */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_DISCONNECTED );
    {
        IDE_ERRLOG( DK_TRC_LOG_FORCE );
        IDE_SET(ideSetErrorCode(dkERR_ABORT_ALTILINKER_DISCONNECTED));

        aSession->mIsNeedToDisconnect = ID_TRUE;
    }
    IDE_EXCEPTION( ERR_REMOTE_STMT_ID );
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKP_INVALID_REMOTE_STMT ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/************************************************************************
 * Description : Remote query 의 수행결과 얻게될 결과집합의 스키마를
 *               얻어오기 위한 operation.
 *
 *  aSession         - [IN] Operation 을 수행하는 세션정보
 *  aSessionId       - [IN] Linker data session id
 *  aRemoteStmtId    - [IN] Remote statement id
 *
 ************************************************************************/
IDE_RC  dkpProtocolMgr::sendRequestResultSetMeta( dksSession    *aSession,
                                                  UInt           aSessionId,
                                                  SLong          aRemoteStmtId )
{
    UInt                    sPacketDataLen = 0;
    UChar                   sFlags         = makeHeaderFlags( aSession, ID_FALSE );

    /* Get the length of data */
    sPacketDataLen += ID_SIZEOF( aRemoteStmtId );

    /* Set ADLP protocol common header */
    IDE_TEST( setProtocolHeader( aSession,
                                 DKP_REQUEST_RESULT_SET_META,
                                 DKP_ADLP_PACKET_PARAM_NOT_USED,
                                 sFlags,
                                 aSessionId,
                                 sPacketDataLen )
              != IDE_SUCCESS );

    IDE_TEST( dknLinkWriteEightByteNumber( aSession->mLink, (void *)&aRemoteStmtId ) != IDE_SUCCESS );

    IDE_TEST( dknLinkSend( aSession->mLink ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 * XDB's JDBC works different with HDB's JDBC. Column name includes table name when
 * ALTIBASE_XDB_SELECT_HEADER_DISPLAY = 0.
 */
static void checkAndMakeColumnName( SChar * aColumnName, UInt aColumnNameLen )
{
    SInt i = 0;
    UInt j = 0;
    UInt sCount = 0;

    for ( i = aColumnNameLen - 1; i >= 0 ; i-- )
    {
        if ( aColumnName[i] == '.' )
        {
            for ( j = 0; j < sCount; j++ )
            {
                aColumnName[j] = aColumnName[i + j + 1];
            }

            aColumnName[j] = '\0';

            break;
        }
        else
        {
            sCount++;
        }
    }
}

/************************************************************************
 * Description : sendRequestResultSetMeta operation 의 결과를 받는다.
 *
 * WARNING     : aColMeta 는 이 함수 내부에서 자원할당 후 해제하지
 *               않으므로 이 함수를 호출하는 쪽에서는 반드시 자원을
 *               해제해주어야 한다.
 *
 *  aSession        - [IN] Operation 을 수행하는 세션정보
 *  aSessionId      - [IN] Linker data session id
 *  aRemoteStmtId   - [IN] Remote statement id
 *  aResultCode     - [OUT] Operation 의 수행 결과로 얻는 result code
 *  aColumnCnt      - [OUT] Result set meta 의 column 갯수
 *  aColMeta        - [OUT] Result set meta column array
 *  aTimeoutSec     - [IN] CM block 의 읽기 timeout 값
 *
 ************************************************************************/
IDE_RC  dkpProtocolMgr::recvRequestResultSetMetaResult( dksSession    *aSession,
                                                        UInt           aSessionId,
                                                        SLong          aRemoteStmtId,
                                                        UInt          *aResultCode,
                                                        UInt          *aColumnCnt,
                                                        dkpColumn    **aColMeta,
                                                        ULong          aTimeoutSec )
{
    idBool                  sIsTimedOut = ID_FALSE;
    UInt                    sColumnCnt;
    UInt                    i;
    UInt                    sDataLen;
    UInt                    sRemainedDataLen;
    UInt                    sColumnNameLen = 0;
    UInt                    sColMetaInfoSize;
    SLong                   sRemoteStmtId;
    dkpColumn             * sColumnMetaArray;
    UChar                   sFlags = makeHeaderFlags( aSession, ID_FALSE );

    /* Get cm block from protocol context */
    IDE_TEST_RAISE( getCmBlock( aSession,
                                &sIsTimedOut,
                                aTimeoutSec )
                    != IDE_SUCCESS, ERR_DISCONNECTED );

    IDE_TEST_RAISE( sIsTimedOut == ID_TRUE, ERR_DISCONNECTED );

    /* Analyze ADLP common header */
    IDE_TEST( analyzeCommonHeader( aSession,
                                   DKP_REQUEST_RESULT_SET_META_RESULT,
                                   &sFlags,
                                   aSessionId,
                                   aResultCode,
                                   &sDataLen )
              != IDE_SUCCESS );

    if ( sDataLen >= 8 )
    {
        /* >> BUG-37588 */
        if ( ( *aResultCode == DKP_RC_SUCCESS ) ||
             ( *aResultCode == DKP_RC_SUCCESS_FRAGMENTED ) )
        {
            /* Get remote statement id */
            IDE_TEST( dknLinkReadEightByteNumber( aSession->mLink, (void *)&sRemoteStmtId ) != IDE_SUCCESS );
            IDE_TEST_RAISE ( sRemoteStmtId != aRemoteStmtId, ERR_REMOTE_STMT_ID );

            IDE_TEST( dknLinkReadFourByteNumber( aSession->mLink, &sColumnCnt ) != IDE_SUCCESS );

            sRemainedDataLen  = sDataLen;
            sRemainedDataLen -= ID_SIZEOF( aRemoteStmtId );
            sRemainedDataLen -= ID_SIZEOF( sColumnCnt );

            if ( sColumnCnt > 0 )
            {
                IDU_FIT_POINT_RAISE( "dkpProtocolMgr::recvRequestResultSetMetaResult::calloc::ColumnMetaArray",
                                     ERR_MEMORY_ALLOC_COL_META_ARRAY );
                IDE_TEST_RAISE( iduMemMgr::calloc( IDU_MEM_DK,
                                                   (SInt)sColumnCnt,
                                                   ID_SIZEOF( dkpColumn ),
                                                   (void **)&sColumnMetaArray,
                                                   IDU_MEM_IMMEDIATE )
                                != IDE_SUCCESS, ERR_MEMORY_ALLOC_COL_META_ARRAY );
            }
            else
            {
                /*
                 * XDB's JDBC Driver retun not null when PreparedStatment.getMetaData()
                 * for non select query. Column count is zero instead of null at that case.
                 */
                sColumnMetaArray = NULL;
            }

            i = 0;

            while ( i < sColumnCnt )
            {
                sColMetaInfoSize = 0;

                IDE_TEST( dknLinkReadFourByteNumber( aSession->mLink, &sColumnNameLen ) != IDE_SUCCESS );
                
                IDE_TEST( readLinkBufferInternal( aSession->mLink, 
                                                  (void *)sColumnMetaArray[i].mName,
                                                  DK_NAME_LEN + 1,
                                                  &sColumnNameLen,
                                                  ID_TRUE )
                          != IDE_SUCCESS  );

                if( sColumnNameLen > DK_NAME_LEN )
                {
                    sColumnNameLen = DK_NAME_LEN;
                }
                sColumnMetaArray[i].mName[sColumnNameLen] = '\0';   
                                   
                /* For XDB's JDBC */
                checkAndMakeColumnName( sColumnMetaArray[i].mName, sColumnNameLen );

                IDE_TEST( dknLinkReadFourByteNumber( aSession->mLink, (void *)&(sColumnMetaArray[i].mType) )
                          != IDE_SUCCESS );
                IDE_TEST( dknLinkReadFourByteNumber( aSession->mLink,  &(sColumnMetaArray[i].mPrecision) )
                          != IDE_SUCCESS );
                IDE_TEST( dknLinkReadFourByteNumber( aSession->mLink,  (void *)&(sColumnMetaArray[i].mScale) )
                          != IDE_SUCCESS );
                IDE_TEST( dknLinkReadFourByteNumber( aSession->mLink,  &(sColumnMetaArray[i].mNullable) )
                          != IDE_SUCCESS );

                sColMetaInfoSize += ( sColumnNameLen
                                      + ID_SIZEOF( sColumnNameLen )
                                      + ID_SIZEOF( sColumnMetaArray[i].mType )
                                      + ID_SIZEOF( sColumnMetaArray[i].mPrecision )
                                      + ID_SIZEOF( sColumnMetaArray[i].mScale )
                                      + ID_SIZEOF( sColumnMetaArray[i].mNullable ) );

                ++i;

                sRemainedDataLen -= sColMetaInfoSize;

                if ( ( sRemainedDataLen == 0 ) &&
                     ( *aResultCode == DKP_RC_SUCCESS_FRAGMENTED ) )
                {
                    /* Get next cm block */
                    IDE_TEST( getCmBlock( aSession,
                                          &sIsTimedOut,
                                          aTimeoutSec )
                              != IDE_SUCCESS );

                    /* Analyze ADLP common header */
                    IDE_TEST( analyzeCommonHeader( aSession,
                                                   DKP_REQUEST_RESULT_SET_META_RESULT,
                                                   &sFlags,
                                                   aSessionId,
                                                   aResultCode,
                                                   &sDataLen )
                              != IDE_SUCCESS );

                    sRemainedDataLen = sDataLen;
                }
                else
                {
                    /* Loop */
                }
            }

            *aColumnCnt = sColumnCnt;
            *aColMeta   = sColumnMetaArray;
        }
        else
        {
            /* do nothing */
        }
        /* << BUG-37588 */
    }
    else
    {
        /* no payload */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_DISCONNECTED );
    {
        IDE_ERRLOG( DK_TRC_LOG_FORCE );
        IDE_SET(ideSetErrorCode(dkERR_ABORT_ALTILINKER_DISCONNECTED));

        aSession->mIsNeedToDisconnect = ID_TRUE;
    }
    IDE_EXCEPTION( ERR_REMOTE_STMT_ID );
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKP_INVALID_REMOTE_STMT ) );
    }
    IDE_EXCEPTION( ERR_MEMORY_ALLOC_COL_META_ARRAY );
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_MEMORY_ALLOCATION ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/************************************************************************
 * Description : Remote server 에 binding 을 수행하기 위한 operation.
 *
 *  aSession         - [IN] Operation 을 수행하는 세션정보
 *  aSessionId       - [IN] Linker data session id
 *  aRemoteStmtId    - [IN] Remote statement id
 *  aBindVarIdx      - [IN] Bind variable's index number
 *  aBindVarType     - [IN] Bind variable's type
 *  aBindVarLen      - [IN] Bind variable's length
 *  aBindVarValue    - [IN] Bind variable's value
 *
 ************************************************************************/
IDE_RC  dkpProtocolMgr::sendBindRemoteVariable( dksSession    *aSession,
                                                UInt           aSessionId,
                                                SLong          aRemoteStmtId,
                                                UInt           aBindVarIdx,
                                                UInt           aBindVarType,
                                                UInt           aBindVarLen,
                                                SChar         *aBindVar )
{
    UInt    sInputParam = 0;
    UInt    sBindVarOffset   = 0;
    UInt    sRemainedDataLen = 0;
    UInt    sPacketDataLen   = 0;
    UChar   sFlags           = makeHeaderFlags( aSession, ID_FALSE );

    /* Get the length of data */
    sPacketDataLen += ID_SIZEOF( aRemoteStmtId );
    sPacketDataLen += ID_SIZEOF( aBindVarIdx );
    sPacketDataLen += ID_SIZEOF( aBindVarType );
    sPacketDataLen += ID_SIZEOF( aBindVarLen );

    sRemainedDataLen = aBindVarLen;

    if (aBindVarLen > (DKP_ADLP_PACKET_DATA_MAX_LEN - sPacketDataLen))
    {
        sInputParam    = DKP_ADLP_PACKET_PARAM_FRAGMENTED;
        sBindVarOffset = DKP_ADLP_PACKET_DATA_MAX_LEN - sPacketDataLen;
    }
    else
    {
        sInputParam    = DKP_ADLP_PACKET_PARAM_NOT_FRAGMENTED;
        sBindVarOffset = aBindVarLen;
    }

    sPacketDataLen += sBindVarOffset;

    /* Set ADLP protocol common header */
    IDE_TEST( setProtocolHeader( aSession,
                                 DKP_BIND_REMOTE_VARIABLE,
                                 sInputParam,
                                 sFlags,
                                 aSessionId,
                                 sPacketDataLen )
              != IDE_SUCCESS );

    IDE_TEST( dknLinkWriteEightByteNumber( aSession->mLink, (void *)&aRemoteStmtId ) != IDE_SUCCESS );
    IDE_TEST( dknLinkWriteFourByteNumber( aSession->mLink, &aBindVarIdx ) != IDE_SUCCESS );
    IDE_TEST( dknLinkWriteFourByteNumber( aSession->mLink, &aBindVarType ) != IDE_SUCCESS );
    IDE_TEST( dknLinkWriteFourByteNumber( aSession->mLink, &aBindVarLen ) != IDE_SUCCESS );
    IDE_TEST( dknLinkWrite( aSession->mLink, (void *)aBindVar, sBindVarOffset ) != IDE_SUCCESS );

    IDE_TEST( dknLinkSend( aSession->mLink ) != IDE_SUCCESS );

    /* Get the length of remained data to send */
    sRemainedDataLen -= sBindVarOffset;

    while( sInputParam == DKP_ADLP_PACKET_PARAM_FRAGMENTED )
    {
        if ( sRemainedDataLen > DKP_ADLP_PACKET_DATA_MAX_LEN )
        {
            sInputParam       = DKP_ADLP_PACKET_PARAM_FRAGMENTED;
            sPacketDataLen    = DKP_ADLP_PACKET_DATA_MAX_LEN;
            sBindVarOffset   += DKP_ADLP_PACKET_DATA_MAX_LEN;
            sRemainedDataLen -= DKP_ADLP_PACKET_DATA_MAX_LEN;
        }
        else
        {
            sInputParam    = DKP_ADLP_PACKET_PARAM_NOT_FRAGMENTED;
            sPacketDataLen = sRemainedDataLen;
        }

        /* Set ADLP protocol common header */
        IDE_TEST( setProtocolHeader( aSession,
                                     DKP_BIND_REMOTE_VARIABLE,
                                     sInputParam,
                                     sFlags,
                                     aSessionId,
                                     sPacketDataLen )
                  != IDE_SUCCESS );

        IDE_TEST( dknLinkWrite( aSession->mLink, (void *)(aBindVar + sBindVarOffset), sPacketDataLen )
                  != IDE_SUCCESS );

        IDE_TEST( dknLinkSend( aSession->mLink ) != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/************************************************************************
 * Description : sendBindRemoteVariable operation 의 결과를 받는다.
 *
 *  aSession        - [IN] Operation 을 수행하는 세션정보
 *  aSessionId      - [IN] Linker data session id
 *  aRemoteStmtId   - [IN] Remote statement id
 *  aResultCode     - [OUT] Operation 의 수행 결과로 얻는 result code
 *                          node session 의 id
 *  aTimeoutSec     - [IN] CM block 의 읽기 timeout 값
 *
 ************************************************************************/
IDE_RC  dkpProtocolMgr::recvBindRemoteVariableResult( dksSession    *aSession,
                                                      UInt           aSessionId,
                                                      SLong          aRemoteStmtId,
                                                      UInt          *aResultCode,
                                                      ULong          aTimeoutSec )
{
    idBool                  sIsTimedOut = ID_FALSE;
    UInt                    sDataLen = 0;
    SLong                   sRemoteStmtId = 0;
    UChar                   sFlags = 0;

    /* Get cm block from protocol context */
    IDE_TEST_RAISE( getCmBlock( aSession,
                                &sIsTimedOut,
                                aTimeoutSec )
                    != IDE_SUCCESS, ERR_DISCONNECTED );

    IDE_TEST_RAISE( sIsTimedOut == ID_TRUE, ERR_DISCONNECTED );

    /* Analyze ADLP common header */
    IDE_TEST( analyzeCommonHeader( aSession,
                                   DKP_BIND_REMOTE_VARIABLE_RESULT,
                                   &sFlags,
                                   aSessionId,
                                   aResultCode,
                                   &sDataLen )
              != IDE_SUCCESS );

    if ( sDataLen >= 8 )
    {
        /* Get remote statement id */
        IDE_TEST( dknLinkReadEightByteNumber( aSession->mLink, (void *)&sRemoteStmtId ) != IDE_SUCCESS );
        IDE_TEST_RAISE ( sRemoteStmtId != aRemoteStmtId, ERR_REMOTE_STMT_ID );
    }
    else
    {
        /* no payload */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_DISCONNECTED );
    {
        IDE_ERRLOG( DK_TRC_LOG_FORCE );
        IDE_SET(ideSetErrorCode(dkERR_ABORT_ALTILINKER_DISCONNECTED));

        aSession->mIsNeedToDisconnect = ID_TRUE;
    }
    IDE_EXCEPTION( ERR_REMOTE_STMT_ID );
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKP_INVALID_REMOTE_STMT ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/************************************************************************
 * Description : Remote node session 을 close 하기 위한 operation.
 *
 *      @ 관련 구문
 *          1. ALTER SESSION CLOSE DATABASE LINK ALL
 *          2. ALTER SESSION CLOSE DATABASE LINK dblink_name
 *
 *  aSession             - [IN] Operation 을 수행하는 세션정보
 *  aSessionId           - [IN] Linker data session id
 *  aRemoteNodeSessionId - [IN] Close 하려는 remote node session 의 id,
 *                              만약 이 값이 0 인 경우는 위 관련 구문 중
 *                              1을 수행, 아니면 2를 수행한다.
 *
 ************************************************************************/
IDE_RC  dkpProtocolMgr::sendRequestCloseRemoteNodeSession( dksSession    *aSession,
                                                           UInt           aSessionId,
                                                           UShort         aRemoteNodeSessionId )
{
    UInt                    sPacketDataLen   = 0;
    UChar                   sFlags           = makeHeaderFlags( aSession, ID_FALSE );

    /* Get the length of data */
    sPacketDataLen += ID_SIZEOF( aRemoteNodeSessionId );

    /* Set ADLP protocol common header */
    IDE_TEST( setProtocolHeader( aSession,
                                 DKP_REQUEST_CLOSE_REMOTE_NODE_SESSION,
                                 DKP_ADLP_PACKET_PARAM_NOT_USED,
                                 sFlags,
                                 aSessionId,
                                 sPacketDataLen )
              != IDE_SUCCESS );

    IDE_TEST( dknLinkWriteTwoByteNumber( aSession->mLink, &aRemoteNodeSessionId ) != IDE_SUCCESS );

    IDE_TEST( dknLinkSend( aSession->mLink ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/************************************************************************
 * Description : sendRequestCloseRemoteNodeSession operation 의 결과를
 *               받는다.
 *
 *  aSession            - [IN] Operation 을 수행하는 세션정보
 *  aSessionId          - [IN] Linker data session id
 *  aResultCode         - [OUT] Operation 의 수행 결과로 얻는 result code
 *                              node session 의 id
 *  aRemainedNodeCnt    - [OUT] Remained remote node count
 *  aTimeoutSec         - [IN] CM block 의 읽기 timeout 값
 *
 ************************************************************************/
IDE_RC  dkpProtocolMgr::recvRequestCloseRemoteNodeSessionResult( dksSession    *aSession,
                                                                 UInt           aSessionId,
                                                                 UShort         aRemoteNodeSessionId,
                                                                 UInt          *aResultCode,
                                                                 UInt          *aRemainedRemoteNodeSeesionCnt,
                                                                 ULong          aTimeoutSec )
{
    idBool                  sIsTimedOut = ID_FALSE;
    UInt                    sDataLen = 0;
    UInt                    sRemainedRemoteNodeSessionCnt = 0;
    UShort                  sRemoteNodeSessionId = 0;
    UChar                   sFlags = 0;
    
    /* Get cm block from protocol context */
    IDE_TEST_RAISE( getCmBlock( aSession,
                                &sIsTimedOut,
                                aTimeoutSec )
                    != IDE_SUCCESS, ERR_DISCONNECTED );
    
    IDE_TEST_RAISE( sIsTimedOut == ID_TRUE, ERR_DISCONNECTED );
    
    /* Analyze ADLP common header */
    IDE_TEST( analyzeCommonHeader( aSession,
                                   DKP_REQUEST_CLOSE_REMOTE_NODE_SESSION_RESULT,
                                   &sFlags,
                                   aSessionId,
                                   aResultCode,
                                   &sDataLen )
              != IDE_SUCCESS );
    
    if ( sDataLen >= 2 )
    {
        /* Get remote statement id */
        IDE_TEST( dknLinkReadTwoByteNumber( aSession->mLink, &sRemoteNodeSessionId ) != IDE_SUCCESS );
        IDE_ASSERT( sRemoteNodeSessionId == aRemoteNodeSessionId );
        
        /* Get remained remote node session count */
        IDE_TEST( dknLinkReadFourByteNumber( aSession->mLink, &sRemainedRemoteNodeSessionCnt ) != IDE_SUCCESS );
        
        *aRemainedRemoteNodeSeesionCnt = sRemainedRemoteNodeSessionCnt;
    }
    else
    {
        /* no payload */
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION( ERR_DISCONNECTED );
    {
        IDE_ERRLOG( DK_TRC_LOG_FORCE );
        IDE_SET(ideSetErrorCode(dkERR_ABORT_ALTILINKER_DISCONNECTED));
        
        aSession->mIsNeedToDisconnect = ID_TRUE;
    }
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

/************************************************************************
 * Description : Remote server 에 auto-commit mode 를 설정하기 위해
 *               수행하는 operation.
 *
 *      @ Auto Commit Mode
 *       --> 설정은 ADLP common header 의 두번째 요소값(1 byte)에 따른다.
 *              0: ON
 *              1: OFF
 *
 *  aSession         - [IN] Operation 을 수행하는 세션정보
 *  aSessionId       - [IN] Linker data session id
 *  aAutoCommitMode  - [IN] 설정할 autocommit mode
 *
 ************************************************************************/
IDE_RC  dkpProtocolMgr::sendSetAutoCommitMode( dksSession    *aSession,
                                               UInt           aSessionId,
                                               UChar          aAutoCommitMode )
{
    UInt                    sPacketDataLen   = 0;
    UChar                   sFlags           = makeHeaderFlags( aSession, ID_FALSE );
    
    /* Set ADLP protocol common header */
    IDE_TEST( setProtocolHeader( aSession,
                                 DKP_SET_AUTO_COMMIT_MODE,
                                 aAutoCommitMode,
                                 sFlags,
                                 aSessionId,
                                 sPacketDataLen )
              != IDE_SUCCESS );
    
    IDE_TEST( dknLinkSend( aSession->mLink ) != IDE_SUCCESS );
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

/************************************************************************
 * Description : sendSetAutoCommitMode operation 의 결과를 받는다.
 *
 *  aSession             - [IN] Operation 을 수행하는 세션정보
 *  aSessionId           - [IN] Linker data session id
 *  aResultCode          - [OUT] Operation 의 수행 결과로 얻는 result code
 *                               node session 의 id
 *  aTimeoutSec          - [IN] CM block 의 읽기 timeout 값
 *
 ************************************************************************/
IDE_RC  dkpProtocolMgr::recvSetAutoCommitModeResult( dksSession    *aSession,
                                                     UInt           aSessionId,
                                                     UInt          *aResultCode,
                                                     ULong          aTimeoutSec )
{
    idBool                  sIsTimedOut = ID_FALSE;
    UInt                    sDataLen = 0;
    UChar                   sFlags = 0;
    
    /* Get cm block from protocol context */
    IDE_TEST_RAISE( getCmBlock( aSession,
                                &sIsTimedOut,
                                aTimeoutSec )
                    != IDE_SUCCESS, ERR_DISCONNECTED );
    
    IDE_TEST_RAISE( sIsTimedOut == ID_TRUE, ERR_DISCONNECTED );
    
    /* Analyze ADLP common header */
    IDE_TEST( analyzeCommonHeader( aSession,
                                   DKP_SET_AUTO_COMMIT_MODE_RESULT,
                                   &sFlags,
                                   aSessionId,
                                   aResultCode,
                                   &sDataLen )
              != IDE_SUCCESS );
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION( ERR_DISCONNECTED );
    {
        IDE_ERRLOG( DK_TRC_LOG_FORCE );
        IDE_SET(ideSetErrorCode(dkERR_ABORT_ALTILINKER_DISCONNECTED));
        
        aSession->mIsNeedToDisconnect = ID_TRUE;
    }
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

/************************************************************************
 * Description : Remote node session 을 close 하기 위한 operation.
 *
 *  aSession             - [IN] Operation 을 수행하는 세션정보
 *  aSessionId           - [IN] Linker data session id
 *  aRemoteNodeSessionId - [IN] Remote node session id, 이 값이 0인 경우는
 *                              모든 remote node session 을 check 한다.
 *
 ************************************************************************/
IDE_RC  dkpProtocolMgr::sendCheckRemoteSession( dksSession    *aSession,
                                                UInt           aSessionId,
                                                UShort         aRemoteNodeSessionId )
{
    UInt    sPacketDataLen   = 0;
    UChar   sFlags           = makeHeaderFlags( aSession, ID_FALSE );
    
    /* Get the length of data */
    sPacketDataLen += ID_SIZEOF( aRemoteNodeSessionId );
    
    /* Set ADLP protocol common header */
    IDE_TEST( setProtocolHeader( aSession,
                                 DKP_CHECK_REMOTE_NODE_SESSION,
                                 DKP_ADLP_PACKET_PARAM_NOT_USED,
                                 sFlags,
                                 aSessionId,
                                 sPacketDataLen )
              != IDE_SUCCESS );
    
    IDE_TEST( dknLinkWriteTwoByteNumber( aSession->mLink, &aRemoteNodeSessionId ) != IDE_SUCCESS );
    
    IDE_TEST( dknLinkSend( aSession->mLink ) != IDE_SUCCESS );
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

/************************************************************************
 * Description : sendCheckRemoteSession operation 의 결과를 받는다.
 *
 *  aSession                 - [IN] Operation 을 수행하는 세션정보
 *  aSessionId               - [IN] Linker data session id
 *  aResultCode              - [OUT] Operation 의 결과로 얻는 result code
 *  aRemoteNodeCnt           - [OUT] Remote node session 의 개수
 *  aRemoteNodeSessionInfo   - [OUT] 모든 remote node session 의 상태정보
 *  aTimeoutSec              - [IN] CM block 의 읽기 timeout 값
 *
 ************************************************************************/
IDE_RC  dkpProtocolMgr::recvCheckRemoteSessionResult( dksSession          *aSession,
                                                      UInt                 aSessionId,
                                                      UInt                *aResultCode,
                                                      UInt                *aRemoteNodeCnt,
                                                      dksRemoteNodeInfo  **aRemoteNodeSessionInfo,
                                                      ULong                aTimeoutSec )
{
    idBool                  sIsTimedOut = ID_FALSE;
    UInt                    i = 0;
    UInt                    sDataLen = 0;
    dksRemoteNodeInfo     * sRemoteNodeSessionInfo = NULL;
    UChar                   sFlags = 0;
    
    /* Get cm block from protocol context */
    IDE_TEST_RAISE( getCmBlock( aSession,
                                &sIsTimedOut,
                                aTimeoutSec )
                    != IDE_SUCCESS, ERR_DISCONNECTED );
    
    IDE_TEST_RAISE( sIsTimedOut == ID_TRUE, ERR_DISCONNECTED );
    
    /* Analyze ADLP common header */
    IDE_TEST( analyzeCommonHeader( aSession,
                                   DKP_CHECK_REMOTE_NODE_SESSION_RESULT,
                                   &sFlags,
                                   aSessionId,
                                   aResultCode,
                                   &sDataLen )
              != IDE_SUCCESS );
    
    if ( sDataLen >= 4 )
    {
        /* Get the number of remote node sessions */
        IDE_TEST( dknLinkReadFourByteNumber( aSession->mLink, aRemoteNodeCnt ) != IDE_SUCCESS );
        
        if ( *aRemoteNodeCnt != 0 )
        {
            IDU_FIT_POINT_RAISE( "dkpProtocolMgr::recvCheckRemoteSessionResult::calloc::RemoteNodeSessionInfo",
                                 ERR_MEMORY_ALLOC_REMOTE_SESSION_INFO );
            IDE_TEST_RAISE( iduMemMgr::calloc( IDU_MEM_DK,
                                               (SInt)(*aRemoteNodeCnt),
                                               ID_SIZEOF( dksRemoteNodeInfo ),
                                               (void **)&sRemoteNodeSessionInfo,
                                               IDU_MEM_IMMEDIATE )
                            != IDE_SUCCESS, ERR_MEMORY_ALLOC_REMOTE_SESSION_INFO );
            
            /* Get remote node sessions' information */
            for ( i = 0; i < *aRemoteNodeCnt; ++i )
            {
                IDE_TEST( dknLinkReadTwoByteNumber( aSession->mLink, &(sRemoteNodeSessionInfo[i].mSessionID) )
                          != IDE_SUCCESS );
                IDE_TEST( dknLinkReadFourByteNumber( aSession->mLink, &(sRemoteNodeSessionInfo[i].mStatus) )
                          != IDE_SUCCESS );
            }
            
            *aRemoteNodeSessionInfo = sRemoteNodeSessionInfo;
        }
        else
        {
            *aRemoteNodeSessionInfo = NULL;
        }
    }
    else
    {
        /* no payload */
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION( ERR_DISCONNECTED );
    {
        IDE_ERRLOG( DK_TRC_LOG_FORCE );
        IDE_SET(ideSetErrorCode(dkERR_ABORT_ALTILINKER_DISCONNECTED));
        
        aSession->mIsNeedToDisconnect = ID_TRUE;
    }
    IDE_EXCEPTION( ERR_MEMORY_ALLOC_REMOTE_SESSION_INFO );
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_MEMORY_ALLOCATION ) );
    }
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

/************************************************************************
 * Description : Remote server 에 commit 을 수행하기 위한 operation.
 *
 *  aSession         - [IN] Operation 을 수행하는 세션정보
 *  aSessionId       - [IN] Linker data session id
 *
 ************************************************************************/
IDE_RC  dkpProtocolMgr::sendCommit( dksSession    *aSession,
                                    UInt           aSessionId )
{
    UInt                    sPacketDataLen   = 0;
    UChar                   sFlags           = makeHeaderFlags( aSession, ID_FALSE );
    
    /* Set ADLP protocol common header */
    IDE_TEST( setProtocolHeader( aSession,
                                 DKP_COMMIT,
                                 DKP_ADLP_PACKET_PARAM_NOT_USED,
                                 sFlags,
                                 aSessionId,
                                 sPacketDataLen )
              != IDE_SUCCESS );
    
    IDE_TEST( dknLinkSend( aSession->mLink ) != IDE_SUCCESS );
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

/************************************************************************
 * Description : sendCommit operation 의 결과를 받는다.
 *
 *  aSession             - [IN] Operation 을 수행하는 세션정보
 *  aSessionId           - [IN] Linker data session id
 *  aResultCode          - [OUT] Operation 의 수행 결과로 얻는 result code
 *                               node session 의 id
 *  aTimeoutSec          - [IN] CM block 의 읽기 timeout 값
 *
 ************************************************************************/
IDE_RC  dkpProtocolMgr::recvCommitResult( dksSession    *aSession,
                                          UInt           aSessionId,
                                          UInt          *aResultCode,
                                          ULong          aTimeoutSec )
{
    idBool                  sIsTimedOut = ID_FALSE;
    UInt                    sDataLen = 0;
    UChar                   sFlags = 0;
    
    /* Get cm block from protocol context */
    IDE_TEST_RAISE( getCmBlock( aSession,
                                &sIsTimedOut,
                                aTimeoutSec )
                    != IDE_SUCCESS, ERR_DISCONNECTED );
    
    IDE_TEST_RAISE( sIsTimedOut == ID_TRUE, ERR_DISCONNECTED );
    
    /* Analyze ADLP common header */
    IDE_TEST( analyzeCommonHeader( aSession,
                                   DKP_COMMIT_RESULT,
                                   &sFlags,
                                   aSessionId,
                                   aResultCode,
                                   &sDataLen )
              != IDE_SUCCESS );
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION( ERR_DISCONNECTED );
    {
        IDE_ERRLOG( DK_TRC_LOG_FORCE );
        IDE_SET(ideSetErrorCode(dkERR_ABORT_ALTILINKER_DISCONNECTED));
        
        aSession->mIsNeedToDisconnect = ID_TRUE;
    }
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

/************************************************************************
 * Description : Remote server 에 rollback 을 수행하기 위한 operation.
 *
 *  aSession         - [IN] Operation 을 수행하는 세션정보
 *  aSessionId       - [IN] Linker data session id
 *  aSavepointName   - [IN] 어디까지 rollback 을 수행할 것인가를 명시하는
 *                          savepoint name
 *  aRemoteNodeCnt   - [IN] 해당 savepoint 를 지정한 remote node 의 수
 *  aRemoteNodeIdArr - [IN] 해당 savepoint 를 지정한 remote node id 배열
 *
 ************************************************************************/
IDE_RC  dkpProtocolMgr::sendRollback( dksSession    *aSession,
                                      UInt           aSessionId,
                                      SChar         *aSavepointName,
                                      UInt           aRemoteNodeCnt,
                                      UShort        *aRemoteNodeIdArr )
{
    UChar                   sSVPFlag = 0;
    UInt                    i = 0;
    UInt                    sSavepointNameLen = 0;
    UInt                    sPacketDataLen   = 0;
    UChar                   sFlags           = makeHeaderFlags( aSession, ID_FALSE );
    
    /* Get the length of savepoint name string */
    if ( aSavepointName != NULL )
    {
        sSavepointNameLen = idlOS::strlen( aSavepointName );
    }
    else
    {
        sSavepointNameLen = 0;
    }
    
    if ( sSavepointNameLen > 0 )
    {
        sSVPFlag = DKP_TRANSACTION_ROLLBACK_TO_SAVEPOINT;
    }
    else
    {
        sSVPFlag = DKP_TRANSACTION_ROLLBACK_ALL;
    }
    
    /* Get the length of data */
    sPacketDataLen += ID_SIZEOF( sSavepointNameLen );
    sPacketDataLen += sSavepointNameLen;
    sPacketDataLen += ID_SIZEOF( aRemoteNodeCnt );
    sPacketDataLen += aRemoteNodeCnt * ID_SIZEOF( *aRemoteNodeIdArr );
    
    /* Set ADLP protocol common header */
    IDE_TEST( setProtocolHeader( aSession,
                                 DKP_ROLLBACK,
                                 sSVPFlag, /* to where rollback : ALL or SP */
                                 sFlags,
                                 aSessionId,
                                 sPacketDataLen )
              != IDE_SUCCESS );
    
    /* Set data */
    IDE_TEST( dknLinkWriteFourByteNumber( aSession->mLink, &sSavepointNameLen ) != IDE_SUCCESS );
    
    if ( sSVPFlag == DKP_TRANSACTION_ROLLBACK_TO_SAVEPOINT )
    {
        IDE_TEST( dknLinkWrite( aSession->mLink, (void *)aSavepointName, sSavepointNameLen ) != IDE_SUCCESS );
    }
    else
    {
        /* rollback all */
    }
    
    IDE_TEST( dknLinkWriteFourByteNumber( aSession->mLink, &aRemoteNodeCnt ) != IDE_SUCCESS );
    
    for ( i = 0; i < aRemoteNodeCnt; ++i )
    {
        IDE_TEST( dknLinkWriteTwoByteNumber( aSession->mLink, &aRemoteNodeIdArr[i] ) != IDE_SUCCESS );
    }
    
    IDE_TEST( dknLinkSend( aSession->mLink ) != IDE_SUCCESS );
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

/************************************************************************
 * Description : sendRollback operation 의 결과를 받는다.
 *
 *  aSession             - [IN] Operation 을 수행하는 세션정보
 *  aSessionId           - [IN] Linker data session id
 *  aResultCode          - [OUT] Operation 의 수행 결과로 얻는 result code
 *                               node session 의 id
 *  aTimeoutSec          - [IN] CM block 의 읽기 timeout 값
 *
 ************************************************************************/
IDE_RC  dkpProtocolMgr::recvRollbackResult( dksSession    *aSession,
                                            UInt           aSessionId,
                                            UInt          *aResultCode,
                                            ULong          aTimeoutSec )
{
    idBool                  sIsTimedOut = ID_FALSE;
    UInt                    sDataLen = 0;
    UChar                   sFlags = 0;
    
    /* Get cm block from protocol context */
    IDE_TEST_RAISE( getCmBlock( aSession,
                                &sIsTimedOut,
                                aTimeoutSec )
                    != IDE_SUCCESS, ERR_DISCONNECTED );
    
    IDE_TEST_RAISE( sIsTimedOut == ID_TRUE, ERR_DISCONNECTED );
    
    /* Analyze ADLP common header */
    IDE_TEST( analyzeCommonHeader( aSession,
                                   DKP_ROLLBACK_RESULT,
                                   &sFlags,
                                   aSessionId,
                                   aResultCode,
                                   &sDataLen )
              != IDE_SUCCESS );
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION( ERR_DISCONNECTED );
    {
        IDE_ERRLOG( DK_TRC_LOG_FORCE );
        IDE_SET(ideSetErrorCode(dkERR_ABORT_ALTILINKER_DISCONNECTED));
        
        aSession->mIsNeedToDisconnect = ID_TRUE;
    }
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

/************************************************************************
 * Description : Remote server 에 savepoint 를 수행하기 위한 operation.
 *
 *  aSession         - [IN] Operation 을 수행하는 세션정보
 *  aSessionId       - [IN] Linker data session id
 *  aSavepointName   - [IN] 지정할 savepoint name
 *
 ************************************************************************/
IDE_RC  dkpProtocolMgr::sendSavepoint( dksSession    *aSession,
                                       UInt           aSessionId,
                                       const SChar   *aSavepointName )
{
    UInt                    sPacketDataLen    = 0;
    UInt                    sSavepointNameLen = 0;
    UChar                   sFlags            = makeHeaderFlags( aSession, ID_FALSE );
    
    if ( aSavepointName != NULL )
    {
        /* Get the length of savepoint name string */
        sSavepointNameLen = idlOS::strlen( aSavepointName );
    }
    else
    {
        sSavepointNameLen = 0;
    }
    
    /* Get the length of data */
    sPacketDataLen += ID_SIZEOF( sSavepointNameLen );
    sPacketDataLen += sSavepointNameLen;
    
    /* Set ADLP protocol common header */
    IDE_TEST( setProtocolHeader( aSession,
                                 DKP_SAVEPOINT,
                                 DKP_ADLP_PACKET_PARAM_NOT_USED,
                                 sFlags,
                                 aSessionId,
                                 sPacketDataLen )
              != IDE_SUCCESS );
    
    /* Set data */
    IDE_TEST( dknLinkWriteFourByteNumber( aSession->mLink, &sSavepointNameLen ) != IDE_SUCCESS );
    
    if ( sSavepointNameLen != 0 )
    {
        IDE_TEST( dknLinkWrite( aSession->mLink, (void *)aSavepointName, sSavepointNameLen ) != IDE_SUCCESS );
    }
    else
    {
        /* no more data to set */
    }
    
    IDE_TEST( dknLinkSend( aSession->mLink ) != IDE_SUCCESS );
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

/************************************************************************
 * Description : sendSavepoint operation 의 결과를 받는다.
 *
 *  aSession             - [IN] Operation 을 수행하는 세션정보
 *  aSessionId           - [IN] Linker data session id
 *  aResultCode          - [OUT] Operation 의 수행 결과로 얻는 result code
 *                               node session 의 id
 *  aTimeoutSec          - [IN] CM block 의 읽기 timeout 값
 *
 ************************************************************************/
IDE_RC  dkpProtocolMgr::recvSavepointResult( dksSession    *aSession,
                                             UInt           aSessionId,
                                             UInt          *aResultCode,
                                             ULong          aTimeoutSec )
{
    idBool                  sIsTimedOut = ID_FALSE;
    UInt                    sDataLen = 0;
    UChar                   sFlags = 0;
    
    /* Get cm block from protocol context */
    IDE_TEST_RAISE( getCmBlock( aSession,
                                &sIsTimedOut,
                                aTimeoutSec )
                    != IDE_SUCCESS, ERR_DISCONNECTED );
    
    IDE_TEST_RAISE( sIsTimedOut == ID_TRUE, ERR_DISCONNECTED );
    
    /* Analyze ADLP common header */
    IDE_TEST( analyzeCommonHeader( aSession,
                                   DKP_SAVEPOINT_RESULT,
                                   &sFlags,
                                   aSessionId,
                                   aResultCode,
                                   &sDataLen )
              != IDE_SUCCESS );
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION( ERR_DISCONNECTED );
    {
        IDE_ERRLOG( DK_TRC_LOG_FORCE );
        IDE_SET(ideSetErrorCode(dkERR_ABORT_ALTILINKER_DISCONNECTED));
        
        aSession->mIsNeedToDisconnect = ID_TRUE;
    }
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

/************************************************************************
 * Description : Remote statement 를 abort 하기 위해 수행하는 operation.
 *
 *  aSession         - [IN] Operation 을 수행하는 세션정보
 *  aSessionId       - [IN] Linker data session id
 *  aRemoteStmtId    - [IN] Remote statement id
 *
 ************************************************************************/
IDE_RC  dkpProtocolMgr::sendAbortRemoteStmt( dksSession    *aSession,
                                             UInt           aSessionId,
                                             SLong          aRemoteStmtId )
{
    UInt    sPacketDataLen = 0;
    UChar   sFlags         = makeHeaderFlags( aSession, ID_FALSE );
    
    
    /* Get the length of data */
    sPacketDataLen += ID_SIZEOF( aRemoteStmtId );
    
    /* Set ADLP protocol common header */
    IDE_TEST( setProtocolHeader( aSession,
                                 DKP_ABORT_REMOTE_STMT,
                                 DKP_ADLP_PACKET_PARAM_NOT_USED,
                                 sFlags,
                                 aSessionId,
                                 sPacketDataLen )
              != IDE_SUCCESS );
    
    /* Set data */
    IDE_TEST( dknLinkWriteEightByteNumber( aSession->mLink, (void *)&aRemoteStmtId ) != IDE_SUCCESS );
    
    IDE_TEST( dknLinkSend( aSession->mLink ) != IDE_SUCCESS );
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

/************************************************************************
 * Description : sendAbortRemoteStmt operation 의 결과를 받는다.
 *
 *  aSession        - [IN] Operation 을 수행하는 세션정보
 *  aSessionId      - [IN] Linker data session id
 *  aRemoteStmtId   - [IN] Remote statement id
 *  aResultCode     - [OUT] Operation 의 수행 결과로 얻는 result code
 *                          node session 의 id
 *  aTimeoutSec     - [IN] CM block 의 읽기 timeout 값
 *
 ************************************************************************/
IDE_RC  dkpProtocolMgr::recvAbortRemoteStmtResult( dksSession    *aSession,
                                                   UInt           aSessionId,
                                                   SLong          aRemoteStmtId,
                                                   UInt          *aResultCode,
                                                   ULong          aTimeoutSec )
{
    idBool                  sIsTimedOut = ID_FALSE;
    UInt                    sDataLen;
    SLong                   sRemoteStmtId;
    UChar                   sFlags = 0;
    
    /* Get cm block from protocol context */
    IDE_TEST_RAISE( getCmBlock( aSession,
                                &sIsTimedOut,
                                aTimeoutSec )
                    != IDE_SUCCESS, ERR_DISCONNECTED );
    
    IDE_TEST_RAISE( sIsTimedOut == ID_TRUE, ERR_DISCONNECTED );
    
    /* Analyze ADLP common header */
    IDE_TEST( analyzeCommonHeader( aSession,
                                   DKP_ABORT_REMOTE_STMT_RESULT,
                                   &sFlags,
                                   aSessionId,
                                   aResultCode,
                                   &sDataLen )
              != IDE_SUCCESS );
    
    if ( sDataLen >= 8 )
    {
        /* Get remote statement id */
        IDE_TEST( dknLinkReadEightByteNumber( aSession->mLink, (void *)&sRemoteStmtId ) != IDE_SUCCESS );
        IDE_TEST_RAISE ( sRemoteStmtId != aRemoteStmtId, ERR_REMOTE_STMT_ID );
    }
    else
    {
        /* no payload */
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION( ERR_DISCONNECTED );
    {
        IDE_ERRLOG( DK_TRC_LOG_FORCE );
        IDE_SET(ideSetErrorCode(dkERR_ABORT_ALTILINKER_DISCONNECTED));
        
        aSession->mIsNeedToDisconnect = ID_TRUE;
    }
    IDE_EXCEPTION( ERR_REMOTE_STMT_ID );
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKP_INVALID_REMOTE_STMT ) );
    }
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC  dkpProtocolMgr::sendXAPrepare( dksSession    *aSession,
                                       UInt           aSessionId,
                                       dktDtxInfo    *aDtxInfo )

{
    UInt                    sPacketDataLen  = 0;
    UInt                    sTargetNameLen  = 0;
    UChar                   sFlags = makeHeaderFlags( aSession, ID_FALSE );
    iduListNode            *sIter      = NULL;
    iduListNode            *sIterNext  = NULL;
    dktDtxBranchTxInfo     *sDtxBranchTxInfo = NULL;
    
    sPacketDataLen += ID_SIZEOF(UInt);          // sRTxCnt
    
    IDU_LIST_ITERATE_SAFE( &(aDtxInfo->mBranchTxInfo), sIter, sIterNext )
    {
        sDtxBranchTxInfo = (dktDtxBranchTxInfo *)sIter->mObj;
        
        sPacketDataLen += DKT_2PC_XIDDATASIZE;
        sPacketDataLen += ID_SIZEOF(UInt);
        sPacketDataLen += idlOS::strlen( sDtxBranchTxInfo->mData.mTargetName );
    }
    
    /* Set ADLP protocol common header */
    IDE_TEST( setProtocolHeader( aSession,
                                 DKP_XA_PREPARE,
                                 DKP_ADLP_PACKET_PARAM_NOT_USED,
                                 sFlags,
                                 aSessionId,
                                 sPacketDataLen )
              != IDE_SUCCESS );
    
    /* Set data */
    IDE_TEST( dknLinkWriteFourByteNumber( aSession->mLink,
                                          (void *)&(aDtxInfo->mBranchTxCount) )
              != IDE_SUCCESS );
    
    IDU_LIST_ITERATE_SAFE( &(aDtxInfo->mBranchTxInfo), sIter, sIterNext )
    {
        sDtxBranchTxInfo = (dktDtxBranchTxInfo *)sIter->mObj;
        
        // ID_XID.data
        IDE_TEST( dknLinkWrite( aSession->mLink,
                                (void *)sDtxBranchTxInfo->mXID.data,
                                DKT_2PC_XIDDATASIZE )
                  != IDE_SUCCESS );
        
        // Target Name Length
        sTargetNameLen = idlOS::strlen( sDtxBranchTxInfo->mData.mTargetName );
        IDE_TEST( dknLinkWriteFourByteNumber( aSession->mLink,
                                              (void *)&sTargetNameLen )
                  != IDE_SUCCESS );
        
        IDE_TEST( dknLinkWrite( aSession->mLink,
                                (void *)sDtxBranchTxInfo->mData.mTargetName,
                                sTargetNameLen )
                  != IDE_SUCCESS );
    }
    
    IDE_TEST( dknLinkSend( aSession->mLink ) != IDE_SUCCESS );
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC  dkpProtocolMgr::recvXAPrepareResult( dksSession    *aSession,
                                             UInt           aSessionId,
                                             UInt          *aResultCode,
                                             ULong          aTimeoutSec,
                                             UInt          *aCountRDOnlyXID,
                                             ID_XID       **aRDOnlyXIDs,
                                             UInt          *aCountFailXID,
                                             ID_XID       **aFailXIDs,
                                             SInt         **aFailErrCodes )

{
    idBool                  sIsTimedOut = ID_FALSE;
    UInt                    sDataLen = 0;
    UChar                   sFlags = 0;
    UInt                    i = 0;
    UInt                    sCountRDOnlyXID = 0;
    UInt                    sCountFailXID   = 0;
    ID_XID                 *sRDOnlyXIDs     = NULL;
    ID_XID                 *sFailXIDs       = NULL;
    SInt                   *sFailErrCodes   = NULL;

    /* Get cm block from protocol context */
    IDE_TEST_RAISE( getCmBlock( aSession,
                                &sIsTimedOut,
                                aTimeoutSec )
                    != IDE_SUCCESS, ERR_DISCONNECTED );
    IDE_TEST_RAISE( sIsTimedOut == ID_TRUE, ERR_DISCONNECTED );
    
    /* Analyze ADLP common header */
    IDE_TEST( analyzeCommonHeader( aSession,
                                   DKP_XA_PREPARE_RESULT,
                                   &sFlags,
                                   aSessionId,
                                   aResultCode,
                                   &sDataLen )
              != IDE_SUCCESS );
    
    if ( sDataLen > 0 )
    {
        // Count of RDONLY ID_XID
        IDE_TEST( dknLinkReadFourByteNumber( aSession->mLink,
                                             (void *)&sCountRDOnlyXID )
                  != IDE_SUCCESS );
        
        if ( sCountRDOnlyXID > 0 )
        {
            // malloc ID_XID list
            IDE_TEST_RAISE( iduMemMgr::malloc( IDU_MEM_DK,
                                               ID_SIZEOF(ID_XID) * sCountRDOnlyXID,
                                               (void **)&sRDOnlyXIDs,
                                               IDU_MEM_IMMEDIATE )
                            != IDE_SUCCESS, ERR_MEM_ALLOC_RDONLY_XIDS );
            
            for ( i = 0 ; i < sCountRDOnlyXID ; i++ )
            {
                dktXid::initXID( &(sRDOnlyXIDs[i]) );
                IDE_TEST( dknLinkRead( aSession->mLink,
                                       (void *)sRDOnlyXIDs[i].data,
                                       DKT_2PC_XIDDATASIZE )
                          != IDE_SUCCESS );
            }
        }
        else
        {
            /* do nothing */
        }
        
        // Count of Failed XIDs
        IDE_TEST( dknLinkReadFourByteNumber( aSession->mLink,
                                             (void *)&sCountFailXID )
                  != IDE_SUCCESS );
        
        if ( sCountFailXID > 0 )
        {
            // malloc ID_XID list
            IDE_TEST_RAISE( iduMemMgr::malloc( IDU_MEM_DK,
                                               ID_SIZEOF(ID_XID) * sCountFailXID,
                                               (void **)&sFailXIDs,
                                               IDU_MEM_IMMEDIATE )
                            != IDE_SUCCESS, ERR_MEM_ALLOC_FAIL_XIDS );
            
            // malloc Error codes list
            IDE_TEST_RAISE( iduMemMgr::malloc( IDU_MEM_DK,
                                               ID_SIZEOF(SInt) * sCountFailXID,
                                               (void **)&sFailErrCodes,
                                               IDU_MEM_IMMEDIATE )
                            != IDE_SUCCESS, ERR_MEM_ALLOC_ERROR_CODES );
            
            for ( i = 0 ; i < sCountFailXID ; i++ )
            {
                dktXid::initXID( &(sFailXIDs[i]) );
                IDE_TEST( dknLinkRead( aSession->mLink,
                                       (void *)sFailXIDs[i].data,
                                       DKT_2PC_XIDDATASIZE )
                          != IDE_SUCCESS );
                
                // Read Error Code 4 byte
                IDE_TEST( dknLinkReadFourByteNumber( aSession->mLink,
                    (void *)&sFailErrCodes[i] )
                          != IDE_SUCCESS );
            }
        }
        else
        {
            /* do nothing */
        }
    }
    else
    {
        /* do nothing */
    }
    
    *aCountRDOnlyXID = sCountRDOnlyXID;
    *aRDOnlyXIDs     = sRDOnlyXIDs;
    
    *aCountFailXID = sCountFailXID;
    *aFailXIDs     = sFailXIDs;
    *aFailErrCodes = sFailErrCodes;
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION( ERR_DISCONNECTED )
    {
        IDE_ERRLOG( DK_TRC_LOG_FORCE );
        IDE_SET(ideSetErrorCode(dkERR_ABORT_ALTILINKER_DISCONNECTED));
        
        aSession->mIsNeedToDisconnect = ID_TRUE;
    }
    IDE_EXCEPTION( ERR_MEM_ALLOC_RDONLY_XIDS )
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_MEMORY_ALLOCATION ) );
    }
    IDE_EXCEPTION( ERR_MEM_ALLOC_FAIL_XIDS )
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_MEMORY_ALLOCATION ) );
    }
    IDE_EXCEPTION( ERR_MEM_ALLOC_ERROR_CODES )
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_MEMORY_ALLOCATION ) );
    }
    IDE_EXCEPTION_END;
    
    freeXARecvResult( sRDOnlyXIDs,
                      sFailXIDs,
                      NULL,
                      sFailErrCodes );
    
    return IDE_FAILURE;
}

void dkpProtocolMgr::freeXARecvResult( ID_XID * aRDOnlyXIDs,
                                       ID_XID * aFailXIDs,
                                       ID_XID * aHeuristicXIDs,
                                       SInt   * aFailErrCodes )
{
    if ( aRDOnlyXIDs != NULL )
    {
        (void)iduMemMgr::free( aRDOnlyXIDs );
        aRDOnlyXIDs = NULL;
    }
    else
    {
        /* Nothing to do */
    }
    if ( aFailXIDs != NULL )
    {
        (void)iduMemMgr::free( aFailXIDs );
        aFailXIDs = NULL;
    }
    else
    {
        /* Nothing to do */
    }
    if ( aHeuristicXIDs != NULL )
    {
        (void)iduMemMgr::free( aHeuristicXIDs );
        aHeuristicXIDs = NULL;
    }
    else
    {
        /* Nothing to do */
    }
    if ( aFailErrCodes != NULL )
    {
        (void)iduMemMgr::free( aFailErrCodes );
        aFailErrCodes = NULL;
    }
    else
    {
        /* Nothing to do */
    }

    return;
}

/************************************************************************
 * Description : Remote server 에 rollback 을 수행하기 위한 operation.
 *
 *  aSession         - [IN] Operation 을 수행하는 세션정보
 *  aSessionId       - [IN] Linker data session id
 *  aSavepointName   - [IN] 어디까지 rollback 을 수행할 것인가를 명시하는
 *                          savepoint name
 *  aRemoteNodeCnt   - [IN] 해당 savepoint 를 지정한 remote node 의 수
 *  aRemoteNodeIdArr - [IN] 해당 savepoint 를 지정한 remote node id 배열
 *
 ************************************************************************/
IDE_RC  dkpProtocolMgr::sendXARollback( dksSession    *aSession,
                                        UInt           aSessionId,
                                        dktDtxInfo    *aDtxInfo )
{
    UInt                    sTargetNameLen   = 0;
    UInt                    sPacketDataLen   = 0;
    UChar                   sFlags           = makeHeaderFlags( aSession, ID_FALSE );
    iduListNode            *sIter      = NULL;
    iduListNode            *sIterNext  = NULL;
    dktDtxBranchTxInfo     *sDtxBranchTxInfo = NULL;

    sPacketDataLen += ID_SIZEOF(UInt);

    IDU_LIST_ITERATE_SAFE( &(aDtxInfo->mBranchTxInfo), sIter, sIterNext )
    {
        sDtxBranchTxInfo = (dktDtxBranchTxInfo *)sIter->mObj;

        sPacketDataLen += DKT_2PC_XIDDATASIZE;
        sPacketDataLen += ID_SIZEOF(UInt);
        sPacketDataLen += idlOS::strlen( sDtxBranchTxInfo->mData.mTargetName );
    }

    /* Set ADLP protocol common header */
    IDE_TEST( setProtocolHeader( aSession,
                                 DKP_XA_ROLLBACK,
                                 DKP_TRANSACTION_ROLLBACK_ALL,
                                 sFlags,
                                 aSessionId,
                                 sPacketDataLen )
              != IDE_SUCCESS );

    /* Set data */
    IDE_TEST( dknLinkWriteFourByteNumber( aSession->mLink,
                                          (void *)&(aDtxInfo->mBranchTxCount) )
              != IDE_SUCCESS );

    IDU_LIST_ITERATE_SAFE( &(aDtxInfo->mBranchTxInfo), sIter, sIterNext )
    {
        sDtxBranchTxInfo = (dktDtxBranchTxInfo *)sIter->mObj;

        // ID_XID.data
        IDE_TEST( dknLinkWrite( aSession->mLink,
                                (void *)sDtxBranchTxInfo->mXID.data,
                                DKT_2PC_XIDDATASIZE )
                  != IDE_SUCCESS );

        // Target Name Length
        sTargetNameLen = idlOS::strlen( sDtxBranchTxInfo->mData.mTargetName );
        IDE_TEST( dknLinkWriteFourByteNumber( aSession->mLink,
                                              (void *)&sTargetNameLen )
                  != IDE_SUCCESS );

        IDE_TEST( dknLinkWrite( aSession->mLink,
                                (void *)sDtxBranchTxInfo->mData.mTargetName,
                                sTargetNameLen )
                  != IDE_SUCCESS );
    }

    IDE_TEST( dknLinkSend( aSession->mLink ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/************************************************************************
 * Description : sendRollback operation 의 결과를 받는다.
 *
 *  aSession             - [IN] Operation 을 수행하는 세션정보
 *  aSessionId           - [IN] Linker data session id
 *  aResultCode          - [OUT] Operation 의 수행 결과로 얻는 result code
 *                               node session 의 id
 *  aTimeoutSec          - [IN] CM block 의 읽기 timeout 값
 *
 ************************************************************************/
IDE_RC  dkpProtocolMgr::recvXARollbackResult( dksSession    *aSession,
                                              UInt           aSessionId,
                                              UInt          *aResultCode,
                                              ULong          aTimeoutSec,
                                              UInt          *aCountFailXID,
                                              ID_XID       **aFailXIDs,
                                              SInt         **aFailErrCodes,
                                              UInt          *aCountHeuristicXID,
                                              ID_XID       **aHeuristicXIDs )
{
    idBool                  sIsTimedOut     = ID_FALSE;
    UInt                    sDataLen        = 0;
    UChar                   sFlags = 0;
    UInt                    i = 0;
    UInt                    sCountFailXID   = 0;
    ID_XID                 *sFailXIDs       = NULL;
    SInt                   *sFailErrCodes   = NULL;
    UInt                    sCountHeuristicXID = 0;
    ID_XID                 *sHeuristicXIDs  = NULL;
    
    /* Get cm block from protocol context */
    IDE_TEST_RAISE( getCmBlock( aSession,
                                &sIsTimedOut,
                                aTimeoutSec )
                    != IDE_SUCCESS, ERR_DISCONNECTED );
    IDE_TEST_RAISE( sIsTimedOut == ID_TRUE, ERR_DISCONNECTED );
    
    /* Analyze ADLP common header */
    IDE_TEST( analyzeCommonHeader( aSession,
                                   DKP_XA_ROLLBACK_RESULT,
                                   &sFlags,
                                   aSessionId,
                                   aResultCode,
                                   &sDataLen )
              != IDE_SUCCESS );
    if ( sDataLen > 0 )
    {
        // Count of Failed XIDs
        IDE_TEST( dknLinkReadFourByteNumber( aSession->mLink,
                                             (void *)&sCountFailXID )
                  != IDE_SUCCESS );
        if ( sCountFailXID > 0 )
        {
            // malloc ID_XID list
            IDE_TEST_RAISE( iduMemMgr::malloc( IDU_MEM_DK,
                                               ID_SIZEOF(ID_XID) * sCountFailXID,
                                               (void **)&sFailXIDs,
                                               IDU_MEM_IMMEDIATE )
                            != IDE_SUCCESS, ERR_MEM_ALLOC_FAIL_XIDS );
            
            // malloc Error codes list
            IDE_TEST_RAISE( iduMemMgr::malloc( IDU_MEM_DK,
                                               ID_SIZEOF(SInt) * sCountFailXID,
                                               (void **)&sFailErrCodes,
                                               IDU_MEM_IMMEDIATE )
                            != IDE_SUCCESS, ERR_MEM_ALLOC_ERROR_CODES );
            
            for ( i = 0 ; i < sCountFailXID ; i++ )
            {
                dktXid::initXID( &(sFailXIDs[i]) );
                IDE_TEST( dknLinkRead( aSession->mLink,
                                       (void *)sFailXIDs[i].data,
                                       DKT_2PC_XIDDATASIZE )
                          != IDE_SUCCESS );
                
                // Read Error Code 4 byte
                IDE_TEST( dknLinkReadFourByteNumber( aSession->mLink,
                    (void *)&sFailErrCodes[i] )
                          != IDE_SUCCESS );
            }
        }
        else
        {
            /* do nothing */
        }
        
        // Count of Heuristic XIDs
        IDE_TEST( dknLinkReadFourByteNumber( aSession->mLink,
                                             (void *)&sCountHeuristicXID )
                  != IDE_SUCCESS );
        if ( sCountHeuristicXID > 0 )
        {
            // malloc ID_XID list
            IDE_TEST_RAISE( iduMemMgr::malloc( IDU_MEM_DK,
                                               ID_SIZEOF(ID_XID) * sCountHeuristicXID,
                                               (void **)&sHeuristicXIDs,
                                               IDU_MEM_IMMEDIATE )
                            != IDE_SUCCESS, ERR_MEM_ALLOC_FAIL_XIDS );
            
            for ( i = 0 ; i < sCountHeuristicXID ; i++ )
            {
                dktXid::initXID( &(sHeuristicXIDs[i]) );
                IDE_TEST( dknLinkRead( aSession->mLink,
                                       (void *)sHeuristicXIDs[i].data,
                                       DKT_2PC_XIDDATASIZE )
                          != IDE_SUCCESS );
            }
        }
        else
        {
            /* do nothing */
        }
    }
    else
    {
        /* do nothing */
    }
    
    *aCountFailXID = sCountFailXID;
    *aFailXIDs     = sFailXIDs;
    *aFailErrCodes = sFailErrCodes;
    
    *aCountHeuristicXID = sCountHeuristicXID;
    *aHeuristicXIDs     = sHeuristicXIDs;
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION( ERR_DISCONNECTED )
    {
        IDE_ERRLOG( DK_TRC_LOG_FORCE );
        IDE_SET( ideSetErrorCode( dkERR_ABORT_ALTILINKER_DISCONNECTED ) );
        
        aSession->mIsNeedToDisconnect = ID_TRUE;
    }
    IDE_EXCEPTION( ERR_MEM_ALLOC_FAIL_XIDS )
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_MEMORY_ALLOCATION ) );
    }
    IDE_EXCEPTION( ERR_MEM_ALLOC_ERROR_CODES )
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_MEMORY_ALLOCATION ) );
    }
    IDE_EXCEPTION_END;
    
    freeXARecvResult( NULL,
                      sFailXIDs,
                      sHeuristicXIDs,
                      sFailErrCodes );

    return IDE_FAILURE;
}

IDE_RC  dkpProtocolMgr::sendXACommit( dksSession    *aSession,
                                      UInt           aSessionId,
                                      dktDtxInfo    *aDtxInfo )
{
    UInt                    sTargetNameLen   = 0;
    UInt                    sPacketDataLen   = 0;
    UChar                   sFlags           = makeHeaderFlags( aSession, ID_FALSE );
    iduListNode            *sIter      = NULL;
    iduListNode            *sIterNext  = NULL;
    dktDtxBranchTxInfo     *sDtxBranchTxInfo = NULL;

    sPacketDataLen += ID_SIZEOF(UInt);

    IDU_LIST_ITERATE_SAFE( &(aDtxInfo->mBranchTxInfo), sIter, sIterNext )
    {
        sDtxBranchTxInfo = (dktDtxBranchTxInfo *)sIter->mObj;

        sPacketDataLen += DKT_2PC_XIDDATASIZE;
        sPacketDataLen += ID_SIZEOF(UInt);
        sPacketDataLen += idlOS::strlen( sDtxBranchTxInfo->mData.mTargetName );
    }

    /* Set ADLP protocol common header */
    IDE_TEST( setProtocolHeader( aSession,
                                 DKP_XA_COMMIT,
                                 DKP_ADLP_PACKET_PARAM_NOT_USED,
                                 sFlags,
                                 aSessionId,
                                 sPacketDataLen )
              != IDE_SUCCESS );

    IDE_TEST( dknLinkWriteFourByteNumber( aSession->mLink,
                                          &aDtxInfo->mBranchTxCount )
              != IDE_SUCCESS );

    IDU_LIST_ITERATE_SAFE( &(aDtxInfo->mBranchTxInfo), sIter, sIterNext )
    {
        sDtxBranchTxInfo = (dktDtxBranchTxInfo *)sIter->mObj;

        // ID_XID.data
        IDE_TEST( dknLinkWrite( aSession->mLink,
                                (void *)sDtxBranchTxInfo->mXID.data,
                                DKT_2PC_XIDDATASIZE )
                  != IDE_SUCCESS );

        // length of "target name"
        sTargetNameLen = idlOS::strlen( sDtxBranchTxInfo->mData.mTargetName );
        IDE_TEST( dknLinkWriteFourByteNumber( aSession->mLink,
                                              (void *)&sTargetNameLen )
                  != IDE_SUCCESS );

        // target name
        IDE_TEST( dknLinkWrite( aSession->mLink,
                                (void *)sDtxBranchTxInfo->mData.mTargetName,
                                sTargetNameLen )
                  != IDE_SUCCESS );
    }

    IDE_TEST( dknLinkSend( aSession->mLink ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/************************************************************************
 * Description : sendCommit operation 의 결과를 받는다.
 *
 *  aSession             - [IN] Operation 을 수행하는 세션정보
 *  aSessionId           - [IN] Linker data session id
 *  aResultCode          - [OUT] Operation 의 수행 결과로 얻는 result code
 *                               node session 의 id
 *  aTimeoutSec          - [IN] CM block 의 읽기 timeout 값
 *
 ************************************************************************/
IDE_RC  dkpProtocolMgr::recvXACommitResult( dksSession    *aSession,
                                            UInt           aSessionId,
                                            UInt          *aResultCode,
                                            ULong          aTimeoutSec,
                                            UInt          *aCountFailXID,
                                            ID_XID       **aFailXIDs,
                                            SInt         **aFailErrCodes,
                                            UInt          *aCountHeuristicXID,
                                            ID_XID       **aHeuristicXIDs )
{
    UInt                    i = 0;
    UChar                   sFlags = 0;
    idBool                  sIsTimedOut          = ID_FALSE;
    UInt                    sDataLen             = 0;
    UInt                    sCountFailXID        = 0;
    ID_XID                 *sFailXIDs            = NULL;
    SInt                   *sFailErrCodes        = NULL;
    UInt                    sCountHeuristicXID   = 0;
    ID_XID                 *sHeuristicXIDs       = NULL;
    
    /* Get cm block from protocol context */
    IDE_TEST_RAISE( getCmBlock( aSession,
                                &sIsTimedOut,
                                aTimeoutSec )
                    != IDE_SUCCESS, ERR_DISCONNECTED );
    
    IDE_TEST_RAISE( sIsTimedOut == ID_TRUE, ERR_DISCONNECTED );
    
    /* Analyze ADLP common header */
    IDE_TEST( analyzeCommonHeader( aSession,
                                   DKP_XA_COMMIT_RESULT,
                                   &sFlags,
                                   aSessionId,
                                   aResultCode,
                                   &sDataLen )
              != IDE_SUCCESS );
    
    if ( sDataLen > 0 )
    {
        // Count of Failed XIDs
        IDE_TEST( dknLinkReadFourByteNumber( aSession->mLink,
                                             (void *)&sCountFailXID )
                  != IDE_SUCCESS );
        if ( sCountFailXID > 0 )
        {
            // malloc ID_XID list
            IDE_TEST_RAISE( iduMemMgr::malloc( IDU_MEM_DK,
                                               ID_SIZEOF(ID_XID) * sCountFailXID,
                                               (void **)&sFailXIDs,
                                               IDU_MEM_IMMEDIATE )
                            != IDE_SUCCESS, ERR_MEM_ALLOC_FAIL_XIDS );
            
            // malloc Error codes list
            IDE_TEST_RAISE( iduMemMgr::malloc( IDU_MEM_DK,
                                               ID_SIZEOF( SInt ) * sCountFailXID,
                                               (void **)&sFailErrCodes,
                                               IDU_MEM_IMMEDIATE )
                            != IDE_SUCCESS, ERR_MEM_ALLOC_ERROR_CODES );
            
            for ( i = 0 ; i < sCountFailXID ; i++ )
            {
                dktXid::initXID( &(sFailXIDs[i]) );
                IDE_TEST( dknLinkRead( aSession->mLink,
                                       (void *)sFailXIDs[i].data,
                                       DKT_2PC_XIDDATASIZE )
                          != IDE_SUCCESS );
                
                // Read Error Code 4 byte
                IDE_TEST( dknLinkReadFourByteNumber( aSession->mLink,
                    (void *)&sFailErrCodes[i] )
                          != IDE_SUCCESS );
            }
        }
        else
        {
            /* do nothing */
        }
        
        
        // Count of Heuristic XIDs
        IDE_TEST( dknLinkReadFourByteNumber( aSession->mLink,
                                             (void *)&sCountHeuristicXID )
                  != IDE_SUCCESS );
        if ( sCountHeuristicXID > 0 )
        {
            // malloc ID_XID list
            IDE_TEST_RAISE( iduMemMgr::malloc( IDU_MEM_DK,
                                               ID_SIZEOF(ID_XID) * sCountHeuristicXID,
                                               (void **)&sHeuristicXIDs,
                                               IDU_MEM_IMMEDIATE )
                            != IDE_SUCCESS, ERR_MEM_ALLOC_FAIL_XIDS );
            
            for ( i = 0 ; i < sCountHeuristicXID ; i++ )
            {
                dktXid::initXID( &(sHeuristicXIDs[i]) );
                IDE_TEST( dknLinkRead( aSession->mLink,
                                       (void *)sHeuristicXIDs[i].data,
                                       DKT_2PC_XIDDATASIZE )
                          != IDE_SUCCESS );
            }
        }
        else
        {
            /* do nothing */
        }
    }
    else
    {
        /* do nothing */
    }

    *aCountFailXID = sCountFailXID;
    *aFailXIDs     = sFailXIDs;
    *aFailErrCodes = sFailErrCodes;
    
    *aCountHeuristicXID = sCountHeuristicXID;
    *aHeuristicXIDs     = sHeuristicXIDs;
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION( ERR_DISCONNECTED )
    {
        IDE_ERRLOG( DK_TRC_LOG_FORCE );
        IDE_SET(ideSetErrorCode(dkERR_ABORT_ALTILINKER_DISCONNECTED));
        
        aSession->mIsNeedToDisconnect = ID_TRUE;
    }
    IDE_EXCEPTION( ERR_MEM_ALLOC_FAIL_XIDS )
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_MEMORY_ALLOCATION ) );
    }
    IDE_EXCEPTION( ERR_MEM_ALLOC_ERROR_CODES )
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_MEMORY_ALLOCATION ) );
    }
    IDE_EXCEPTION_END;
    
    freeXARecvResult( NULL,
                      sFailXIDs,
                      sHeuristicXIDs,
                      sFailErrCodes );

    return IDE_FAILURE;
}

IDE_RC dkpProtocolMgr::readLinkBufferInternal( dknLink * aLink, 
                                               void    * aDestination,
                                               UInt      aDestinationLen, 
                                               UInt    * aReadBufferLen, 
                                               idBool    aIsSkipRemain )
{
    UInt    i = 0;
    UInt    sRemainLen     = 0;
    UInt    sReadBufferLen = 0;
    UChar   sTemp;
    
    IDE_TEST_RAISE( (*aReadBufferLen > aDestinationLen) && (aIsSkipRemain != ID_TRUE), ERROR_READ_BUFFER_OVERFLOW );
    
    if ( *aReadBufferLen > aDestinationLen ) 
    {
        sRemainLen = *aReadBufferLen - aDestinationLen;
        sReadBufferLen = aDestinationLen;
    }
    else
    {
        sRemainLen = 0;
        sReadBufferLen = *aReadBufferLen;
    }

    IDE_TEST( dknLinkRead( aLink, 
                           aDestination, 
                           sReadBufferLen )
              != IDE_SUCCESS );
    
    // aDestination 버퍼를 채우고 남은 data 들은 skip 처리 한다.
    for ( i = 0 ; i < sRemainLen ; i ++ )
    {
        IDE_TEST( dknLinkReadOneByteNumber( aLink, &sTemp ) != IDE_SUCCESS );
    }
    
    *aReadBufferLen = sReadBufferLen;
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION( ERROR_READ_BUFFER_OVERFLOW )
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DK_INTERNAL_ERROR, "Link data size is biger than buffer size" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/************************************************************************
 * Description : 이 remote statement 수행결과가 error 인 경우, 프로토콜
 *               을 통해 AltiLinker 프로세스로부터 원격서버의 error 정
 *               보를 받아올 수 있는데 이렇게 받아온 error 정보를 remote
 *               statement 객체에 저장하기 위해 수행되는 함수
 ************************************************************************/
IDE_RC  dkpProtocolMgr::getErrorInfoFromProtocol( dksSession   *aSession,
                                                  UInt          aSessionId,
                                                  SLong         aRemoteStmtId,
                                                  dktErrorInfo *aErrorInfo  )
{
    UInt        sResultCode = DKP_RC_SUCCESS;
    ULong       sTimeoutSec;
    SInt        sReceiveTimeout = 0;
    
    IDE_TEST( dkaLinkerProcessMgr::getAltiLinkerReceiveTimeoutFromConf( &sReceiveTimeout )
              != IDE_SUCCESS );
    
    sTimeoutSec = sReceiveTimeout;

    IDE_TEST( sendRequestRemoteErrorInfo( aSession,
                                          aSessionId, 
                                          aRemoteStmtId ) 
              != IDE_SUCCESS );
    
    IDE_TEST( recvRequestRemoteErrorInfoResult( aSession, 
                                                aSessionId, 
                                                &sResultCode, 
                                                aRemoteStmtId, 
                                                aErrorInfo, 
                                                sTimeoutSec ) 
              != IDE_SUCCESS ); 

    IDE_TEST_RAISE( sResultCode != DKP_RC_SUCCESS, ERR_RECEIVE_RESULT );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_RECEIVE_RESULT );
    {
        dkpProtocolMgr::setResultErrorCode( sResultCode, 
                                            NULL, 
                                            0, 
                                            0, 
                                            NULL );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

void dkpProtocolMgr::setResultErrorCode( UInt             aResultCode,
                                         dksSession      *aSession,
                                         UInt             aSessionId,
                                         SLong            aRemoteStmtId,
                                         dktErrorInfo    *aErrorInfo )
{    
    switch ( aResultCode )
    {
        case DKP_RC_SUCCESS:
        case DKP_RC_SUCCESS_FRAGMENTED:
        case DKP_RC_END_OF_FETCH:
            IDE_DASSERT( ID_FALSE );
            break;            
            
        case DKP_RC_FAILED_REMOTE_SERVER_ERROR:
            if ( ( aSession != NULL ) && ( aErrorInfo != NULL ) &&
                 ( aSessionId != 0 )  && ( aRemoteStmtId != 0 ) )
            {
                if ( getErrorInfoFromProtocol( aSession,
                                               aSessionId,
                                               aRemoteStmtId,
                                               aErrorInfo ) 
                      == IDE_SUCCESS )
                {                     
                    IDE_SET( ideSetErrorCode( dkERR_ABORT_DTK_REMOTE_SERVER_ERROR,
                                              aErrorInfo->mErrorCode,
                                              aErrorInfo->mSQLState,
                                              aErrorInfo->mErrorDesc ) );                     
                }
                else
                {
                    /* Nothing to do */
                }
            }
            else
            {
                IDE_SET( ideSetErrorCode( dkERR_ABORT_DKT_REMOTE_SERVER_DISCONNECT ) );
                IDE_DASSERT( ID_FALSE );
            }
            break;
            
        case DKP_RC_FAILED_STMT_TYPE_ERROR:
            IDE_SET( ideSetErrorCode( dkERR_ABORT_DKT_REMOTE_STMT_TYPE ) );
            break;
            
        case DKP_RC_FAILED_NOT_PREPARED:
            IDE_SET( ideSetErrorCode( dkERR_ABORT_DK_INTERNAL_ERROR, "JDBC PreparedStatement object is invalid." ) );
            break;            
            
        case DKP_RC_FAILED_INVALID_DATA_TYPE:
            IDE_SET( ideSetErrorCode( dkERR_ABORT_DK_INTERNAL_ERROR, "Not supported bind variable type." ) );
            break;
                        
        case DKP_RC_FAILED_INVALID_DB_CHARACTER_SET:
            IDE_SET( ideSetErrorCode( dkERR_ABORT_DK_INTERNAL_ERROR, "Not supported database character set." ) );
            break;
       
        case DKP_RC_FAILED_REMOTE_SERVER_DISCONNECT:
            IDE_SET( ideSetErrorCode( dkERR_ABORT_DKT_REMOTE_SERVER_DISCONNECT ) );
            break;

        case DKP_RC_FAILED_EXIST_SESSION:
            IDE_SET( ideSetErrorCode( dkERR_ABORT_DKS_DATA_SESSION_ALREADY_EXIST ) );
            break;
      
        case DKP_RC_FAILED_REMOTE_NODE_SESSION_COUNT_EXCESS:
            IDE_SET( ideSetErrorCode( dkERR_ABORT_DKT_REMOTE_NODE_SESSION_COUNT_EXCESS ) );
            break;
            
        case DKP_RC_FAILED_SET_SAVEPOINT:
            IDE_SET( ideSetErrorCode( dkERR_ABORT_DKM_SET_SAVEPOINT ) );
            break;

        case DKP_RC_BUSY:
            IDE_SET( ideSetErrorCode( dkERR_ABORT_DKS_REMOTE_SESSION_IS_BUSY ) );
            break;     
            
        case DKP_RC_XAEXCEPTION:
            IDE_SET( ideSetErrorCode( dkERR_ABORT_DKT_ALTILINKER_XAEXCEPTION ) );
            break;           

        case DKP_RC_FAILED_UNKNOWN_OPERATION:
        case DKP_RC_FAILED_UNKNOWN_SESSION: 
        case DKP_RC_FAILED_WRONG_PACKET:
        case DKP_RC_FAILED_OUT_OF_RANGE:
        case DKP_RC_FAILED_UNKNOWN_REMOTE_STMT:            
        default:
            IDE_SET( ideSetErrorCode( dkERR_ABORT_ADLP_PROTOCOL ) );
            break;
    }
    
    return;
}
