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
#include <ide.h>
#include <idu.h>

#include <cm.h>

#include <dkDef.h>
#include <dkErrorCode.h>
#include <dknLink.h>

#define DKN_LINK_BUFFER_SIZE              ( 32 * 1024 )

#define DKN_CMP_HEADER_A7_SIZE            ( 16 )

/*
 * Altilinker uses A7's CM header
 */
typedef struct dknCmpHeaderA7
{
    UChar  mHeaderSign;     // 0x06: A5, 0x07: A7 or higher
    UChar  mReserved1;      // 0x00
    UShort mPayloadLength;  // data length except header

    // msb(bit31): last protcol end flag (1: end, 0: continued)
    // bit0~bit30: cyclic packet seq number (0 ~ 0x7fffffff)
    UInt   mCmSeqNo;

    UShort mFlag;           /* PROJ-2296 */
    UShort mSessionID;      // mmcSession's ID, 0x00 at first

    UInt   mReserved2;

} dknCmpHeaderA7;

/*
 *
 */ 
struct dknPacketInternal
{
    dknCmpHeaderA7 * mHeader;

    acp_size_t       mBufferPosition;
    SChar          * mBuffer;
    UInt             mBufferSize;
    UInt             mBufferDataSize;
};

/*
 *
 */ 
struct dknLink
{
    acp_sock_t       mSocket;
    idBool           mSocketIsOpened;

    UShort           mSessionID;
    UInt             mNextSeqNo;

    dknCmpHeaderA7 * mReadHeader;
    dknCmpHeaderA7 * mWriteHeader;

    UInt             mReadBufferPosition;
    SChar            mReadBuffer[ DKN_LINK_BUFFER_SIZE ];
    UInt             mReadDataSize;

    acp_size_t       mWriteBufferPosition;
    SChar            mWriteBuffer[ DKN_LINK_BUFFER_SIZE ];
};

static PDL_Time_Value gTV1Sec;

/*
 *
 */
static void assignNumber2( UChar * aSource, UChar * aDestination )
{
#ifdef ENDIAN_IS_BIG_ENDIAN

    aDestination[0] = aSource[0];
    aDestination[1] = aSource[1];

#else

    aDestination[1] = aSource[0];
    aDestination[0] = aSource[1];

#endif
}

/*
 *
 */
static void assignNumber4( UChar * aSource, UChar * aDestination )
{
#ifdef ENDIAN_IS_BIG_ENDIAN

    aDestination[0] = aSource[0];
    aDestination[1] = aSource[1];
    aDestination[2] = aSource[2];
    aDestination[3] = aSource[3];

#else

    aDestination[3] = aSource[0];
    aDestination[2] = aSource[1];
    aDestination[1] = aSource[2];
    aDestination[0] = aSource[3];

#endif
}

/*
 *
 */
static void assignNumber8( UChar * aSource, UChar * aDestination )
{
#ifdef ENDIAN_IS_BIG_ENDIAN

    aDestination[0] = aSource[0];
    aDestination[1] = aSource[1];
    aDestination[2] = aSource[2];
    aDestination[3] = aSource[3];
    aDestination[4] = aSource[4];
    aDestination[5] = aSource[5];
    aDestination[6] = aSource[6];
    aDestination[7] = aSource[7];

#else

    aDestination[7] = aSource[0];
    aDestination[6] = aSource[1];
    aDestination[5] = aSource[2];
    aDestination[4] = aSource[3];
    aDestination[3] = aSource[4];
    aDestination[2] = aSource[5];
    aDestination[1] = aSource[6];
    aDestination[0] = aSource[7];

#endif
}

/*
 * aInetAddressInfo has to be freed by acpInetAddrInfo()
 */
static IDE_RC getInetAddrInfo( acp_inet_addr_info_t ** aInetAddressInfo,
                               SChar                 * aIPAddress,
                               UShort                  aPortNumber )
{
    SChar sPortString[ IDL_IP_PORT_MAX_LEN ] = { 0, };
    SInt sAddrFlag = 0;
    acp_rc_t sRC = ACP_RC_SUCCESS;

    idlOS::snprintf( sPortString, ID_SIZEOF(sPortString), "%"ID_UINT32_FMT"", aPortNumber );
    *aInetAddressInfo = NULL;

    /*
     * sun 2.8 not defined
     */
#if defined(AI_NUMERICSERV)
    sAddrFlag = ACP_INET_AI_NUMERICSERV; /* port_no */
#endif

    /*
     *  GettAddrInfo can handle IP Address
     *  MANUAL :
     *      If the AI_NUMERICHOST bit is set, it indicates that hostname should be treated as a
     *      numeric string defining an IPv4 or IPv6 address and no name resolution should be attempted
     */
    sAddrFlag |= ACP_INET_AI_NUMERICHOST;

    /* address init */
    sRC = acpInetGetAddrInfo( aInetAddressInfo,
                              aIPAddress,
                              sPortString,
                              ACP_SOCK_STREAM,
                              sAddrFlag,
                              ACP_AF_INET );
    IDE_TEST_RAISE( ( sRC != IDE_SUCCESS ) || ( *aInetAddressInfo == NULL ), 
                    ERR_GET_ADDR_INFO );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_GET_ADDR_INFO )
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKN_GET_ADDR_INFO_ERROR ) );
    }
    IDE_EXCEPTION_END;

    *aInetAddressInfo = NULL;

    return IDE_FAILURE;
}

/*
 *
 */
IDE_RC dknLinkCreate ( dknLink ** aLink )
{
    dknLink * sLink = NULL;
    SInt      sStage = 0;

    gTV1Sec.initialize( 1, 0 );

    IDE_DASSERT( aLink != NULL );

    IDE_TEST_RAISE( iduMemMgr::malloc( IDU_MEM_DK,
                                       ID_SIZEOF( *sLink ),
                                       (void **)&sLink,
                                       IDU_MEM_IMMEDIATE )
                    != IDE_SUCCESS, ERR_MEMORY_ALLOC );
    sStage = 1;

    idlOS::memset( sLink, 0, ID_SIZEOF( *sLink ) );
    
    sLink->mSocketIsOpened = ID_FALSE;

    sLink->mNextSeqNo = 0;

    sLink->mReadHeader = (dknCmpHeaderA7 *)sLink->mReadBuffer;
    sLink->mWriteHeader = (dknCmpHeaderA7 *)sLink->mWriteBuffer;

    sLink->mReadBufferPosition = DKN_CMP_HEADER_A7_SIZE;
    sLink->mReadDataSize = 0;
    sLink->mWriteBufferPosition = DKN_CMP_HEADER_A7_SIZE;

    *aLink = sLink;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_MEMORY_ALLOC )
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_MEMORY_ALLOCATION ) );
    }
    IDE_EXCEPTION_END;

    IDE_PUSH();

    switch ( sStage )
    {
        case 1:
            (void)iduMemMgr::free( sLink );
        default:
            break;
    }

    IDE_POP();

    return IDE_FAILURE;
}

/*
 *
 */
void dknLinkDestroy ( dknLink * aLink )
{
    IDE_DASSERT( aLink != NULL );

    (void)iduMemMgr::free( aLink );
    aLink = NULL;
}

static IDE_RC dknLinkSetSocketOptions( dknLink * aLink )
{
    SInt sOption;

    /* SO_KEEPALIVE */
    sOption = 1;
    (void)acpSockSetOpt( &(aLink->mSocket), SOL_SOCKET, SO_KEEPALIVE, (void *)&sOption, ID_SIZEOF(sOption));

    /* SO_REUSEADDR */
    sOption = 1;
    (void)acpSockSetOpt( &(aLink->mSocket), SOL_SOCKET, SO_REUSEADDR, (void *)&sOption, ID_SIZEOF(sOption));

    /* TCP_NODELAY */
    sOption = 1;
    (void)acpSockSetOpt( &(aLink->mSocket), IPPROTO_TCP, TCP_NODELAY, (void *)&sOption, ID_SIZEOF(sOption));

    /* SO_SNDBUF */
    sOption = DKN_LINK_BUFFER_SIZE * 2;
    (void)acpSockSetOpt( &(aLink->mSocket), SOL_SOCKET, SO_SNDBUF, (void *)&sOption, ID_SIZEOF(sOption));

#if !defined(VC_WINCE)
    /* SO_RCVBUF */
    sOption = DKN_LINK_BUFFER_SIZE * 2;
    (void)acpSockSetOpt( &(aLink->mSocket), SOL_SOCKET, SO_RCVBUF, (void *)&sOption, ID_SIZEOF(sOption));
#endif

    return IDE_SUCCESS;
}

/*
 *
 */
IDE_RC dknLinkConnect( dknLink  * aLink,
                       SChar    * aAddress,
                       UShort     aPortNumber,
                       UInt       aTimeoutSec )
{
    acp_rc_t sRC = ACP_RC_SUCCESS;
    acp_inet_addr_info_t * sInetAddressInfo = NULL;
    SInt sStage = 0;
    UInt i = 0;
    idBool sConnectedFlag = ID_FALSE;

    IDE_TEST( getInetAddrInfo( &sInetAddressInfo, aAddress, aPortNumber )
              != IDE_SUCCESS );
    IDE_DASSERT( sInetAddressInfo != NULL );
    sStage = 1;

    for ( i = 0; i < aTimeoutSec; ++i )
    {
        sRC = acpSockOpen( &(aLink->mSocket), ACP_AF_INET, SOCK_STREAM, 0 );
        IDE_TEST_RAISE( ACP_RC_NOT_SUCCESS( sRC ), ERR_SOCKET_OPEN );
        aLink->mSocketIsOpened = ID_TRUE;

        sRC = acpSockTimedConnect( &(aLink->mSocket),
                                   sInetAddressInfo->ai_addr,
                                   sInetAddressInfo->ai_addrlen,
                                   acpTimeFromSec( 1 ) );
        switch ( sRC )
        {
            case ACP_RC_SUCCESS:
                sConnectedFlag = ID_TRUE;
                i = aTimeoutSec; /* out for loop */
                break;

            case ACP_RC_ETIMEDOUT:
                /* try again */
                break;

            default:
                idlOS::sleep(1);
                break;
        }

        if ( sConnectedFlag == ID_FALSE )
        {
            (void)acpSockClose( &(aLink->mSocket) );
            aLink->mSocketIsOpened = ID_FALSE;
        }
        else
        {
            /* nothing to do */
        }
    }

    sStage = 0;
    if ( sInetAddressInfo != NULL )
    {
        acpInetFreeAddrInfo( sInetAddressInfo );
        sInetAddressInfo = NULL;
    }
    else
    {
        /* do nothing */
    }

    IDE_TEST_RAISE( sConnectedFlag == ID_FALSE, ERR_CONNECT );

    IDE_TEST( dknLinkSetSocketOptions( aLink ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_CONNECT );
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKS_LINKER_SESSION_CONNECT ) );
    }
    IDE_EXCEPTION( ERR_SOCKET_OPEN )
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKN_OPEN_SOCKET_ERROR ) );
    }
    IDE_EXCEPTION_END;

    IDE_PUSH();

    switch ( sStage )
    {
        case 1:
            if ( sInetAddressInfo != NULL )
            {
                acpInetFreeAddrInfo( sInetAddressInfo );
                sInetAddressInfo = NULL;
            }
            else
            {
                /* do nothing */
            }
        default:
            break;
    }

    if ( aLink->mSocketIsOpened == ID_TRUE )
    {
        (void)acpSockClose( &(aLink->mSocket) );
        aLink->mSocketIsOpened = ID_FALSE;
    }
    else
    {
        /* nothing to do */
    }

    IDE_POP();

    return IDE_FAILURE;
}

/*
 *
 */
IDE_RC dknLinkDisconnect( dknLink * aLink, idBool aReuseFlag )
{
    (void)acpSockShutdown( &(aLink->mSocket), ACP_SHUT_RDWR );

    if ( aReuseFlag == ID_TRUE )
    {
        /* nothing to do */
    }
    else
    {
        (void)acpSockClose( &(aLink->mSocket) );
        aLink->mSocketIsOpened = ID_FALSE;
    }

    return IDE_SUCCESS;
}

#if !defined(ACP_MSG_DONTWAIT) && !defined(ACP_MSG_NONBLOCK)
/*
 * This codes are refered from cmnSock.c
 */
static IDE_RC checkSocketIOCTL( acp_sock_t * aSock, idBool * aIsClosed )
{
    fd_set         sFdSet;
    acp_rc_t       sRC;
    SInt           sSize = 0;

    IDE_DASSERT( aSock->mHandle != ACP_SOCK_INVALID_HANDLE );

    /* BUG-37360 [cm] The EINTR signal should be handled when the acpSockPoll function is used. */
    IDE_EXCEPTION_CONT( Restart );

    sRC = acpSockPoll( aSock, ACP_POLL_IN, ACP_TIME_IMMEDIATE );

    switch (sRC)
    {
        case ACP_RC_SUCCESS:
            /*
             * BUGBUG
             *
             *
            if (idlOS::ioctl(aSock, FIONREAD, &sSize) < 0)
            {
                *aIsClosed = ID_TRUE;
            }
            else
            {
                *aIsClosed = (sSize == 0) ? ID_TRUE : ID_FALSE;
            }
            */

            break;

        case ACP_RC_ETIMEDOUT:
            *aIsClosed = ID_FALSE;
            break;

        /* BUG-37360 [cm] The EINTR signal should be handled when the acpSockPoll function is used. */
        case ACP_RC_EINTR:
            IDE_RAISE( Restart );
            break;

        default:
            ACI_RAISE( SelectError );
            break;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( SelectError );
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKN_SELECT_SOCKET_ERROR ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
#endif /* !defined(ACP_MSG_DONTWAIT) && !defined(ACP_MSG_NONBLOCK) */

#if defined(ACP_MSG_PEEK)
/*
 * This codes are refered from cmnSock.c
 */
static IDE_RC checkSocketRECV( acp_sock_t * aSock, acp_sint32_t aFlag, idBool * aIsClosed )
{
    SChar         sBuff[1];
    acp_size_t    sSize;
    acp_rc_t      sRC;
    idBool        sBreak = ID_TRUE;

    do
    {
        sBreak = ID_TRUE;

        sRC = acpSockRecv( aSock, sBuff, 1, &sSize, aFlag );

        switch ( sRC )
        {
            case ACP_RC_SUCCESS:
                *aIsClosed = ID_FALSE;
                break;

            case ACP_RC_EOF:
                *aIsClosed = ID_TRUE;
                break;

#if (EWOULDBLOCK != EAGAIN)
            case ACP_RC_EAGAIN:
#endif
            case ACP_RC_EWOULDBLOCK:
                *aIsClosed = ID_FALSE;
                break;
#if defined (ALTI_CFG_OS_WINDOWS)
                /*
                 * BUGBUG : windows error code is directly used
                 * ACP_IS_RC_ECONNRESET may be used
                 */
            case WSAENETRESET:
            case ACP_RC_ECONNRESET:
#else
            case ACP_RC_ECONNRESET:
#endif
            case ACP_RC_ECONNREFUSED:
                *aIsClosed = ID_TRUE;
                break;

            /* BUG-35783 Add to handle SIGINT while calling recv() */
            case ACP_RC_EINTR:
                sBreak = ID_FALSE;
                break;

            default:
                IDE_RAISE( RecvError );
                break;
        }
    } while ( sBreak != ID_TRUE );

    return IDE_SUCCESS;

    IDE_EXCEPTION( RecvError );
    {
        IDE_SET( ideSetErrorCode( cmERR_ABORT_RECV_ERROR ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
#endif /* defined(ACP_MSG_PEEK) */

/*
 *
 */
IDE_RC dknLinkCheck( dknLink * aLink, idBool * aConnectedFlag )
{
    idBool sIsClosed = ID_FALSE;
    
    if ( aLink->mSocketIsOpened == ID_TRUE )
    {
#if defined(ACP_MSG_PEEK) && defined(ACP_MSG_DONTWAIT)
        IDE_TEST( checkSocketRECV( &(aLink->mSocket), ACP_MSG_PEEK | ACP_MSG_DONTWAIT, &sIsClosed ) != IDE_SUCCESS );

#elif defined(ACP_MSG_PEEK) && defined(ACP_MSG_NONBLOCK)

        IDE_TEST( checkSocketRECV( &(aLink->mSocket), ACP_MSG_PEEK | ACP_MSG_NONBLOCK, &sIsClosed) != IDE_SUCCESS );
        
// Windows CE에서는 MSG_PEEK 옵셥을 지원하지 않는다. (WSAEOPNOTSUPP)
#elif defined(ACP_MSG_PEEK) && !defined(VC_WINCE)

        IDE_TEST( checkSocketIOCTL( &(aLink->mSocket), &sIsClosed ) != IDE_SUCCESS );

#else

        IDE_TEST( checkSocketIOCTL( &(aLink->mSocket), aIsClosed ) != IDE_SUCCESS );

#endif

        if ( sIsClosed == ID_TRUE )
        {
            *aConnectedFlag = ID_FALSE;
        }
        else
        {
            *aConnectedFlag = ID_TRUE;
        }
    }
    else
    {
        *aConnectedFlag = ID_FALSE;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 *
 */
IDE_RC dknLinkCheckAndFlush( dknLink * /* aLink */, UInt /* aLen */ )
{
    /*
     * nothing to do
     */

    return IDE_SUCCESS;
}

/*
 *
 */
IDE_RC dknLinkWriteOneByteNumber( dknLink * aLink, void * aSource )
{
    IDE_TEST_RAISE( aLink->mWriteBufferPosition + 1 > DKN_LINK_BUFFER_SIZE,
                    ERROR_NO_SPACE_IN_WRITE_BUFFER );

    aLink->mWriteBuffer[ aLink->mWriteBufferPosition ] = *(UChar *)aSource;
    aLink->mWriteBufferPosition++;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERROR_NO_SPACE_IN_WRITE_BUFFER )
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKN_NOT_ENOUGH_DATA_IN_WRITE_BUFFER ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 *
 */
IDE_RC dknLinkWriteTwoByteNumber( dknLink * aLink, void * aSource )
{
    IDE_TEST_RAISE( aLink->mWriteBufferPosition + 2 > DKN_LINK_BUFFER_SIZE,
                    ERROR_NO_SPACE_IN_WRITE_BUFFER );

    assignNumber2( (UChar *)aSource, (UChar *)( aLink->mWriteBuffer + aLink->mWriteBufferPosition ) );
    aLink->mWriteBufferPosition += 2;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERROR_NO_SPACE_IN_WRITE_BUFFER )
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKN_NOT_ENOUGH_DATA_IN_WRITE_BUFFER ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 *
 */
IDE_RC dknLinkWriteFourByteNumber( dknLink * aLink, void * aSource )
{
    IDE_TEST_RAISE( aLink->mWriteBufferPosition + 4 > DKN_LINK_BUFFER_SIZE,
                    ERROR_NO_SPACE_IN_WRITE_BUFFER );

    assignNumber4( (UChar *)aSource, (UChar *)( aLink->mWriteBuffer + aLink->mWriteBufferPosition ) );
    aLink->mWriteBufferPosition += 4;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERROR_NO_SPACE_IN_WRITE_BUFFER )
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKN_NOT_ENOUGH_DATA_IN_WRITE_BUFFER ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 *
 */
IDE_RC dknLinkWriteEightByteNumber( dknLink * aLink, void * aSource )
{
    IDE_TEST_RAISE( aLink->mWriteBufferPosition + 8 > DKN_LINK_BUFFER_SIZE,
                    ERROR_NO_SPACE_IN_WRITE_BUFFER );

    assignNumber8( (UChar *)aSource, (UChar *)( aLink->mWriteBuffer + aLink->mWriteBufferPosition ) );
    aLink->mWriteBufferPosition += 8;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERROR_NO_SPACE_IN_WRITE_BUFFER )
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKN_NOT_ENOUGH_DATA_IN_WRITE_BUFFER ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 *
 */
IDE_RC dknLinkWrite( dknLink * aLink, void * aSource, UInt aLength )
{
    IDE_TEST_RAISE( aLink->mWriteBufferPosition + aLength > DKN_LINK_BUFFER_SIZE,
                    ERROR_NO_SPACE_IN_WRITE_BUFFER );

    idlOS::memcpy( aLink->mWriteBuffer + aLink->mWriteBufferPosition, aSource, aLength );
    aLink->mWriteBufferPosition += aLength;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERROR_NO_SPACE_IN_WRITE_BUFFER )
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKN_NOT_ENOUGH_DATA_IN_WRITE_BUFFER ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 *
 */
IDE_RC dknLinkReadOneByteNumber( dknLink * aLink, void * aDestination )
{
    IDE_TEST_RAISE( aLink->mReadBufferPosition + 1 > aLink->mReadDataSize,
                    ERROR_NO_SPACE_IN_READ_BUFFER );

    *(UChar *)aDestination = aLink->mReadBuffer[ aLink->mReadBufferPosition ];
    aLink->mReadBufferPosition++;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERROR_NO_SPACE_IN_READ_BUFFER )
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKN_NOT_ENOUGH_DATA_IN_READ_BUFFER ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 *
 */
IDE_RC dknLinkReadTwoByteNumber( dknLink * aLink, void * aDestination )
{
    IDE_TEST_RAISE( aLink->mReadBufferPosition + 2 > aLink->mReadDataSize,
                    ERROR_NO_SPACE_IN_READ_BUFFER );

    assignNumber2( (UChar *)( aLink->mReadBuffer + aLink->mReadBufferPosition ), (UChar *)aDestination );
    aLink->mReadBufferPosition += 2;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERROR_NO_SPACE_IN_READ_BUFFER )
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKN_NOT_ENOUGH_DATA_IN_READ_BUFFER ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 *
 */
IDE_RC dknLinkReadFourByteNumber( dknLink * aLink, void * aDestination )
{
    IDE_TEST_RAISE( aLink->mReadBufferPosition + 4 > aLink->mReadDataSize,
                    ERROR_NO_SPACE_IN_READ_BUFFER );

    assignNumber4( (UChar *)( aLink->mReadBuffer + aLink->mReadBufferPosition ), (UChar *)aDestination );
    aLink->mReadBufferPosition += 4;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERROR_NO_SPACE_IN_READ_BUFFER )
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKN_NOT_ENOUGH_DATA_IN_READ_BUFFER ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 *
 */
IDE_RC dknLinkReadEightByteNumber( dknLink * aLink, void * aDestination )
{
    IDE_TEST_RAISE( aLink->mReadBufferPosition + 8 > aLink->mReadDataSize,
                    ERROR_NO_SPACE_IN_READ_BUFFER );

    assignNumber8( (UChar *)( aLink->mReadBuffer + aLink->mReadBufferPosition ), (UChar *)aDestination );
    aLink->mReadBufferPosition += 8;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERROR_NO_SPACE_IN_READ_BUFFER )
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKN_NOT_ENOUGH_DATA_IN_READ_BUFFER ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 *
 */
IDE_RC dknLinkRead( dknLink * aLink, void * aDestination, UInt aLength )
{
    IDE_TEST_RAISE( aLink->mReadBufferPosition + aLength > aLink->mReadDataSize,
                    ERROR_NO_SPACE_IN_READ_BUFFER );

    idlOS::memcpy( aDestination, aLink->mReadBuffer + aLink->mReadBufferPosition, aLength );
    aLink->mReadBufferPosition += aLength;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERROR_NO_SPACE_IN_READ_BUFFER )
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKN_NOT_ENOUGH_DATA_IN_READ_BUFFER ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 *
 */
static void writeCmpHeaderA7( dknCmpHeaderA7 * aHeader,
                              UShort           aPayloadLength,
                              UInt             aSeqNo,
                              UShort           aSessionID )
{
    UShort sFlag = 0;

    aHeader->mHeaderSign = 0x07;
    aHeader->mReserved1 = 0x00;

    assignNumber2( (UChar *)&aPayloadLength, (UChar *)&(aHeader->mPayloadLength) );

    /* Set protocol end flag */
    aSeqNo |= 0x80000000;

    assignNumber4( (UChar *)&aSeqNo, (UChar *)&(aHeader->mCmSeqNo) );

    /* Clear compress flag */
    sFlag &= ~(0x0001);

    /* Clear encrypt flag */
    sFlag &= ~(0x0002);

    assignNumber2( (UChar *)&sFlag, (UChar *)&(aHeader->mFlag) );

    assignNumber2( (UChar *)&aSessionID, (UChar *)&(aHeader->mSessionID) );

    aHeader->mReserved2 = 0x00;
}

/*
 *
 */
IDE_RC dknLinkSend( dknLink * aLink )
{
    acp_size_t sSendSize = 0;
    acp_rc_t    sRC = ACP_RC_EINVAL;

    writeCmpHeaderA7( aLink->mWriteHeader,
                      aLink->mWriteBufferPosition - DKN_CMP_HEADER_A7_SIZE,
                      aLink->mNextSeqNo,
                      aLink->mSessionID );

    sRC = acpSockSend( &(aLink->mSocket),
                       aLink->mWriteBuffer,
                       aLink->mWriteBufferPosition,
                       &sSendSize,
                       0 );
    switch ( sRC )
    {
        case ACP_RC_SUCCESS:
            break;
        case ACP_RC_EINTR:
#if (EWOULDBLOCK != EAGAIN)
        case ACP_RC_EAGAIN:
#endif
        case ACP_RC_EWOULDBLOCK:
        default:
            IDE_RAISE( ERR_SOCK_SEND );
            break;
    }

    aLink->mNextSeqNo++;
    if ( aLink->mNextSeqNo == 0xffffffff )
    {
        aLink->mNextSeqNo = 0;
    }
    aLink->mWriteBufferPosition = DKN_CMP_HEADER_A7_SIZE;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_SOCK_SEND );
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKN_SEND_SOCKET_ERROR ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 *
 */
static IDE_RC pollSocket( dknLink * aLink, UInt aTimeoutMsec, idBool * aTimeoutFlag )
{
    acp_rc_t sRC = ACP_RC_SUCCESS;
    acp_time_t sTimeout = acpTimeFromMsec( aTimeoutMsec );

    *aTimeoutFlag = ID_FALSE;

    sRC = acpSockPoll( &(aLink->mSocket), ACP_POLL_IN, sTimeout );
    switch ( sRC )
    {
        case ACP_RC_SUCCESS:
            break;

        case ACP_RC_ETIMEDOUT:
        case ACP_RC_EINTR:
            *aTimeoutFlag = ID_TRUE;
            break;

        default:
            IDE_RAISE( ERR_SOCKET_POLL );
            break;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_SOCKET_POLL )
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKN_SELECT_SOCKET_ERROR ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 *
 */
static IDE_RC receiveSocket( dknLink    * aLink,
                             void       * aBuffer,
                             acp_size_t   aBufferSize,
                             acp_size_t * aReceiveSize )
{
    acp_rc_t    sRC = ACP_RC_SUCCESS;
    acp_size_t  sReceiveSize = 0;

    sRC = acpSockRecv( &(aLink->mSocket),
                       aBuffer,
                       aBufferSize,
                       &sReceiveSize,
                       0 );
    switch ( sRC )
    {
        case ACP_RC_SUCCESS:
            break;

        case ACP_RC_EINTR:
            /* It shoud be ignored */
            break;

        default:
            IDE_RAISE( ERR_SOCKET_RECV );
            break;
    }

    *aReceiveSize = sReceiveSize;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_SOCKET_RECV )
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKN_RECV_SOCKET_ERROR ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 *
 */
static IDE_RC checkHeaderAndGetPayloadLength( dknCmpHeaderA7 * aHeader,
                                              UShort         * aPayloadLength )
{
    UShort sPayloadLength = 0;
    UInt   sSeqNo = 0;
    UShort sFlag = 0;
    UShort sSessionID = 0;

    IDE_TEST_RAISE( aHeader->mHeaderSign != 0x07, ERROR_WRONG_HEADER_SIGN );

    assignNumber2( (UChar *)&(aHeader->mPayloadLength), (UChar *)&sPayloadLength );

    assignNumber4( (UChar *)&(aHeader->mCmSeqNo), (UChar *)&sSeqNo );

    assignNumber2( (UChar *)&(aHeader->mFlag), (UChar *)&sFlag );

    assignNumber2( (UChar *)&(aHeader->mSessionID), (UChar *)&sSessionID );

    *aPayloadLength = sPayloadLength;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERROR_WRONG_HEADER_SIGN )
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKN_WRONG_HEADER_SIGN ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

#define ONE_SECOND ( 1000 )

/*
 *
 */
IDE_RC dknLinkRecv( dknLink * aLink, ULong aTimeoutSec, idBool * aTimeoutFlag )
{
    ULong       i = 0;
    acp_size_t  sReceiveSize = 0;
    acp_size_t  sOffset = 0;
    acp_size_t  sRemainedDataLen = 0;
    idBool      sReadHeaderFlag = ID_FALSE;
    idBool      sTimeoutFlag = ID_FALSE;
    UShort      sPayloadLength = 0;

    sRemainedDataLen = DKN_CMP_HEADER_A7_SIZE;

    for ( i = 0; i < aTimeoutSec; i++ )
    {
        sTimeoutFlag = ID_FALSE;
        IDE_TEST( pollSocket( aLink, ONE_SECOND, &sTimeoutFlag ) != IDE_SUCCESS );

        if ( sTimeoutFlag == ID_TRUE )
        {
            continue;
        }
        else
        {
            /* nothing to do */
        }

        IDE_TEST( receiveSocket( aLink,
                                 aLink->mReadBuffer + sOffset,
                                 sRemainedDataLen,
                                 &sReceiveSize )
                  != IDE_SUCCESS );

        sOffset += sReceiveSize;

        IDE_DASSERT( sRemainedDataLen >= sReceiveSize );
        sRemainedDataLen -= sReceiveSize;

        if ( sRemainedDataLen == 0 )
        {
            if ( sReadHeaderFlag == ID_FALSE )
            {
                IDE_DASSERT( sOffset == DKN_CMP_HEADER_A7_SIZE );

                IDE_TEST( checkHeaderAndGetPayloadLength( aLink->mReadHeader,
                                                          &sPayloadLength )
                          != IDE_SUCCESS );

                sRemainedDataLen = sPayloadLength;
                sReadHeaderFlag = ID_TRUE;
            }
            else
            {
                break;
            }
        }
        else
        {
            /* do nothing */
        }
    }

    if ( i == aTimeoutSec )
    {
        sTimeoutFlag = ID_TRUE;
    }
    else
    {
        /* nothing to do */
    }

    aLink->mReadBufferPosition = DKN_CMP_HEADER_A7_SIZE;
    aLink->mReadDataSize = sOffset;

    *aTimeoutFlag = sTimeoutFlag;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 *
 */ 
IDE_RC dknPacketCreate( dknPacket ** aPacket, UInt aDataSize )
{
    dknPacket * sPacket = NULL;
    UInt        sPacketSize = 0;
    
    IDE_DASSERT( aDataSize <= dknPacketGetMaxDataLength() );

    sPacketSize = ID_SIZEOF( dknPacket ) + ID_SIZEOF( struct dknPacketInternal ) + aDataSize + DKN_CMP_HEADER_A7_SIZE;
        
    IDE_TEST_RAISE( iduMemMgr::malloc( IDU_MEM_DK,
                                       sPacketSize,
                                       (void **)&sPacket,
                                       IDU_MEM_IMMEDIATE )
                    != IDE_SUCCESS, ERR_MEMORY_ALLOC );

    IDU_LIST_INIT_OBJ( &(sPacket->mNode), sPacket );
    sPacket->mInternal = (struct dknPacketInternal *)((SChar *)sPacket + ID_SIZEOF( dknPacket ));
    sPacket->mInternal->mBuffer = (SChar *)((SChar *)sPacket->mInternal + ID_SIZEOF( struct dknPacketInternal ) );
    
    sPacket->mInternal->mBufferSize = aDataSize + DKN_CMP_HEADER_A7_SIZE;
    sPacket->mInternal->mHeader = (dknCmpHeaderA7 *)sPacket->mInternal->mBuffer;
    
    sPacket->mInternal->mBufferPosition = DKN_CMP_HEADER_A7_SIZE;
    sPacket->mInternal->mBufferDataSize = DKN_CMP_HEADER_A7_SIZE;
    
    *aPacket = sPacket;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_MEMORY_ALLOC )
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_MEMORY_ALLOCATION ) );
    }
    IDE_EXCEPTION_END;
    
    if ( sPacket != NULL )
    {
        (void)iduMemMgr::free( sPacket );
        sPacket = NULL;
    }
    else
    {
        /* do nothing */
    }
    
    return IDE_FAILURE;
}

/*
 *
 */ 
void dknPacketDestroy( dknPacket * aPacket )
{
    (void)iduMemMgr::free( aPacket );
    aPacket = NULL;
}

/*
 *
 */ 
IDE_RC dknPacketWriteOneByteNumber( dknPacket * aPacket, void * aSource )
{
    IDE_TEST_RAISE( aPacket->mInternal->mBufferPosition + 1 > aPacket->mInternal->mBufferSize,
                    ERROR_NO_SPACE_IN_WRITE_BUFFER );

    aPacket->mInternal->mBuffer[ aPacket->mInternal->mBufferPosition ] = *(UChar *)aSource;
    aPacket->mInternal->mBufferPosition++;

    if ( aPacket->mInternal->mBufferPosition > aPacket->mInternal->mBufferDataSize )
    {
        aPacket->mInternal->mBufferDataSize = aPacket->mInternal->mBufferPosition;
    }
    else
    {
        /* nothing to do */
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( ERROR_NO_SPACE_IN_WRITE_BUFFER )
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKN_NOT_ENOUGH_DATA_IN_WRITE_BUFFER ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 *
 */ 
IDE_RC dknPacketWriteTwoByteNumber( dknPacket * aPacket, void * aSource )
{
    IDE_TEST_RAISE( aPacket->mInternal->mBufferPosition + 2 > aPacket->mInternal->mBufferSize,
                    ERROR_NO_SPACE_IN_WRITE_BUFFER );

    assignNumber2( (UChar *)aSource, (UChar *)( aPacket->mInternal->mBuffer + aPacket->mInternal->mBufferPosition ) );
    aPacket->mInternal->mBufferPosition += 2;

    if ( aPacket->mInternal->mBufferPosition > aPacket->mInternal->mBufferDataSize )
    {
        aPacket->mInternal->mBufferDataSize = aPacket->mInternal->mBufferPosition;
    }
    else
    {
        /* nothing to do */
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( ERROR_NO_SPACE_IN_WRITE_BUFFER )
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKN_NOT_ENOUGH_DATA_IN_WRITE_BUFFER ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 *
 */ 
IDE_RC dknPacketWriteFourByteNumber( dknPacket * aPacket, void * aSource )
{
    IDE_TEST_RAISE( aPacket->mInternal->mBufferPosition + 4 > aPacket->mInternal->mBufferSize,
                    ERROR_NO_SPACE_IN_WRITE_BUFFER );

    assignNumber4( (UChar *)aSource, (UChar *)( aPacket->mInternal->mBuffer + aPacket->mInternal->mBufferPosition ) );
    aPacket->mInternal->mBufferPosition += 4;

    if ( aPacket->mInternal->mBufferPosition > aPacket->mInternal->mBufferDataSize )
    {
        aPacket->mInternal->mBufferDataSize = aPacket->mInternal->mBufferPosition;
    }
    else
    {
        /* nothing to do */
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( ERROR_NO_SPACE_IN_WRITE_BUFFER )
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKN_NOT_ENOUGH_DATA_IN_WRITE_BUFFER ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 *
 */ 
IDE_RC dknPacketWriteEightByteNumber( dknPacket * aPacket, void * aSource )
{
    IDE_TEST_RAISE( aPacket->mInternal->mBufferPosition + 8 > aPacket->mInternal->mBufferSize,
                    ERROR_NO_SPACE_IN_WRITE_BUFFER );

    assignNumber8( (UChar *)aSource, (UChar *)( aPacket->mInternal->mBuffer + aPacket->mInternal->mBufferPosition ) );
    aPacket->mInternal->mBufferPosition += 8;

    if ( aPacket->mInternal->mBufferPosition > aPacket->mInternal->mBufferDataSize )
    {
        aPacket->mInternal->mBufferDataSize = aPacket->mInternal->mBufferPosition;
    }
    else
    {
        /* nothing to do */
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( ERROR_NO_SPACE_IN_WRITE_BUFFER )
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKN_NOT_ENOUGH_DATA_IN_WRITE_BUFFER ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 *
 */ 
IDE_RC dknPacketWrite( dknPacket    * aPacket, 
                       void         * aSource, 
                       UInt           aLength )
{
    IDE_TEST_RAISE( aPacket->mInternal->mBufferPosition + aLength > aPacket->mInternal->mBufferSize,
                    ERROR_NO_SPACE_IN_WRITE_BUFFER );

    idlOS::memcpy( aPacket->mInternal->mBuffer + aPacket->mInternal->mBufferPosition, aSource, aLength );
    aPacket->mInternal->mBufferPosition += aLength;

    if ( aPacket->mInternal->mBufferPosition > aPacket->mInternal->mBufferDataSize )
    {
        aPacket->mInternal->mBufferDataSize = aPacket->mInternal->mBufferPosition;
    }
    else
    {
        /* nothing to do */
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( ERROR_NO_SPACE_IN_WRITE_BUFFER )
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKN_NOT_ENOUGH_DATA_IN_WRITE_BUFFER ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 *
 */ 
UInt dknPacketGetCapacity( dknPacket * aPacket )
{
    return aPacket->mInternal->mBufferSize - aPacket->mInternal->mBufferDataSize;
}

/*
 *
 */
UInt dknPacketGetPayloadLength( dknPacket * aPacket )
{
    return ( aPacket->mInternal->mBufferDataSize - DKN_CMP_HEADER_A7_SIZE );
}

/*
 *
 */ 
IDE_RC dknPacketSkip( dknPacket * aPacket, UInt aLength )
{
    IDE_TEST_RAISE( aPacket->mInternal->mBufferPosition + aLength > aPacket->mInternal->mBufferSize,
                    ERROR_NO_SPACE_IN_WRITE_BUFFER );

    aPacket->mInternal->mBufferPosition += aLength;

    if ( aPacket->mInternal->mBufferPosition > aPacket->mInternal->mBufferDataSize )
    {
        aPacket->mInternal->mBufferDataSize = aPacket->mInternal->mBufferPosition;
    }
    else
    {
        /* nothing to do */
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( ERROR_NO_SPACE_IN_WRITE_BUFFER )
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKN_NOT_ENOUGH_DATA_IN_WRITE_BUFFER ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;    
}

/*
 *
 */ 
void dknPacketRewind( dknPacket * aPacket )
{
    aPacket->mInternal->mBufferPosition = DKN_CMP_HEADER_A7_SIZE;
}

/*
 *
 */ 
IDE_RC dknLinkSendPacket( dknLink * aLink, dknPacket * aPacket )
{
    acp_size_t  sSendSize = 0;
    acp_rc_t    sRC = ACP_RC_EINVAL;

    writeCmpHeaderA7( aPacket->mInternal->mHeader,
                      aPacket->mInternal->mBufferDataSize - DKN_CMP_HEADER_A7_SIZE,
                      aLink->mNextSeqNo,
                      aLink->mSessionID );

    sRC = acpSockSend( &(aLink->mSocket),
                       aPacket->mInternal->mBuffer,
                       aPacket->mInternal->mBufferDataSize,
                       &sSendSize,
                       0 );
    switch ( sRC )
    {
        case ACP_RC_SUCCESS:
            break;
        case ACP_RC_EINTR:
#if (EWOULDBLOCK != EAGAIN)
        case ACP_RC_EAGAIN:
#endif
        case ACP_RC_EWOULDBLOCK:
        default:
            IDE_RAISE( ERR_SOCK_SEND );
            break;
    }

    aLink->mNextSeqNo++;
    if ( aLink->mNextSeqNo == 0xffffffff )
    {
        aLink->mNextSeqNo = 0;
    }
    else
    {
        /* do nothing */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_SOCK_SEND );
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKN_SEND_SOCKET_ERROR ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;    
}

/*
 *
 */
UInt dknPacketGetMaxDataLength( void )
{
    return DKN_LINK_BUFFER_SIZE - DKN_CMP_HEADER_A7_SIZE;
}
