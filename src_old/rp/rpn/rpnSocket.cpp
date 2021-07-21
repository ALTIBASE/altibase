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
#include <rpError.h>

#include <rpnSocket.h>

/*
 *  After calling this function, aInetAddressInfo must be freed.
 */
static IDE_RC getInetAddrInfo( SChar                 * aIPAddress,
                               UInt                    aIPAddressLength,
                               UShort                  aPortNumber,
                               acp_inet_addr_info_t ** aInetAddressInfo,
                               SInt                  * aFamily )
{
    UChar           sTempAddr[ID_SIZEOF(struct in6_addr)];
    SChar           sPortString[ IDL_IP_PORT_MAX_LEN ] = { 0, };
    acp_sint32_t    sFamily = 0;
    SInt            sAddrFlag = 0;
    acp_rc_t        sRC = ACP_RC_SUCCESS;

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

    /* inet_pton() returns a positive value on success */
    /* check to see if ipv4 address. ex) ddd.ddd.ddd.ddd */
    if ( idlOS::inet_pton( AF_INET, 
                           aIPAddress, 
                           sTempAddr ) > 0 )
    {
        sFamily = AF_INET;
    }
    /* check to see if ipv6 address. ex) h:h:h:h:h:h:h:h */
    /* bug-30835: support link-local address with zone index.
     *  before: inet_pton(inet6)
     *  after : strchr(':')
     *  inet_pton does not support zone index form. */
    else if ( idlOS::strnchr( aIPAddress, ':', aIPAddressLength ) != NULL )
    {
        sFamily = AF_INET6;
    }
    /* hostname format. ex) db.server.com */
    else
    {
        /* replication does not support hostname */
        IDE_DASSERT( 0 );
        IDE_RAISE( ERR_GET_ADDR_INFO );
    }

    /* address init */
    sRC = acpInetGetAddrInfo( aInetAddressInfo,
                              aIPAddress,
                              sPortString,
                              ACP_SOCK_STREAM,
                              sAddrFlag,
                              sFamily );
    IDE_TEST_RAISE( ( sRC != IDE_SUCCESS ) || ( *aInetAddressInfo == NULL ),
                    ERR_GET_ADDR_INFO );

    *aFamily = sFamily;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_GET_ADDR_INFO )
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_SOCK_GET_ADDR_INFO_ERROR, aIPAddress, aPortNumber ) );
    }
    IDE_EXCEPTION_END;

    *aInetAddressInfo = NULL;

    return IDE_FAILURE;
}

IDE_RC rpnSocketInitailize( rpnSocket   * aSocket,
                            SChar       * aIPAddress,
                            UInt          aIPAddressLength,
                            UShort        aPortNumber,
                            idBool        aIsBlockMode )
{
    acp_inet_addr_info_t    * sInetAddressInfo = NULL;
    SInt                      sFamily = 0;

    IDE_DASSERT( aSocket->mIsOpened == ID_FALSE );
    IDE_DASSERT( aSocket->mInetAddressInfo == NULL );

    IDE_TEST( getInetAddrInfo( aIPAddress,
                               aIPAddressLength,
                               aPortNumber,
                               &sInetAddressInfo,
                               &sFamily )
              != IDE_SUCCESS );

    IDU_FIT_POINT_RAISE( "rpnSocketInitailize::acpSockOpen::mSocket", ERR_SOCK_OPEN );
    IDE_TEST_RAISE( acpSockOpen( &(aSocket->mSocket), 
                                 sFamily,
                                 SOCK_STREAM,
                                 0 )
                    != ACP_RC_SUCCESS, ERR_SOCK_OPEN );
    aSocket->mIsOpened = ID_TRUE;

    IDE_TEST( rpnSocketSetBlockMode( aSocket,
                                     aIsBlockMode )
              != IDE_SUCCESS );

    aSocket->mInetAddressInfo = sInetAddressInfo;
    aSocket->mFamily = sFamily;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_SOCK_OPEN )
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_SOCKET_OPEN_ERROR ) );
    }
    IDE_EXCEPTION_END;

    if ( sInetAddressInfo != NULL )
    {
        acpInetFreeAddrInfo( sInetAddressInfo );
        sInetAddressInfo = NULL;
    }
    else
    {
        /* do nothing */
    }

    if ( aSocket->mIsOpened == ID_TRUE )
    {
        (void)acpSockClose( &(aSocket->mSocket) );
        aSocket->mIsOpened = ID_FALSE;
    }
    else
    {
        /* do nothing */
    }

    aSocket->mInetAddressInfo = NULL;

    return IDE_FAILURE;
}

void rpnSocketFinalize( rpnSocket     * aSocket )
{
    IDE_DASSERT( aSocket->mIsOpened == ID_TRUE );
    IDE_DASSERT( aSocket->mInetAddressInfo != NULL );

    if ( aSocket->mIsOpened == ID_TRUE )
    {
        (void)acpSockClose( &(aSocket->mSocket) );
        aSocket->mIsOpened = ID_FALSE;
    }
    else
    {
        /* do nothing */
    }

    if ( aSocket->mInetAddressInfo != NULL )
    {
        acpInetFreeAddrInfo( aSocket->mInetAddressInfo );
        aSocket->mInetAddressInfo = NULL;
    }
    else
    {
        /* do nothing */
    }
}

IDE_RC rpnSocketSetBlockMode( rpnSocket     * aSocket,
                              idBool          aIsBlockMode )
{
    acp_bool_t      sIsBlockMode = ACP_TRUE;

    if ( aIsBlockMode == ID_TRUE )
    {
        sIsBlockMode = ACP_TRUE;
    }
    else
    {
        sIsBlockMode = ACP_FALSE;
    }

    IDE_TEST_RAISE( acpSockSetBlockMode( &(aSocket->mSocket),
                                         sIsBlockMode )
                    != IDE_SUCCESS, ERR_SET_BLOCK_MODE );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_SET_BLOCK_MODE )
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_SOCKET_SET_BLOCK_MODE ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC rpnSocketConnect( rpnSocket      * aSocket )
{
    acp_inet_addr_info_t        * sInetAddressInfo = NULL;
    acp_rc_t    sRC = ACP_RC_SUCCESS;

    sInetAddressInfo = aSocket->mInetAddressInfo;

    sRC = acpSockConnect( &(aSocket->mSocket),
                          sInetAddressInfo->ai_addr,
                          sInetAddressInfo->ai_addrlen );
    switch ( sRC )
    {
        case ACP_RC_SUCCESS:
            break;

        case ACP_RC_EAGAIN:
        case ACP_RC_EINPROGRESS:
        case ACP_RC_ETIMEDOUT:
            IDE_RAISE( ERR_CONNECT_TIMEOUT );
            break;

        default:
            IDE_RAISE( ERR_CONNECT_FAIL );
            break;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_CONNECT_TIMEOUT )
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_TIMEOUT_EXCEED ) );
    }
    IDE_EXCEPTION( ERR_CONNECT_FAIL )
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_CONNECT_FAIL ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpnSocketGetOpt( rpnSocket   * aSocket,
                        SInt          aLevel,
                        SInt          aOptionName,
                        void        * aOptionValue,
                        ULong       * aOptionLength )
{
    acp_sock_len_t sOptionLength = (acp_sock_len_t)*aOptionLength;

    IDE_TEST_RAISE ( acpSockGetOpt( &(aSocket->mSocket),
                                    aLevel,
                                    aOptionName,
                                    aOptionValue,
                                    &sOptionLength )
                     != ACP_RC_SUCCESS, ERR_SOCK_GET_OPT );

    *aOptionLength = (ULong)sOptionLength;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_SOCK_GET_OPT )
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_SOCKET_GET_OPTION ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

idBool rpnSocketIsConnecting( UInt  aErrorNumber )
{
    idBool  sIsConnecting = ID_FALSE;

    switch ( aErrorNumber )
    {
        case ACP_RC_EAGAIN:
        case ACP_RC_EALREADY:
        case ACP_RC_EINPROGRESS:
        case ACP_RC_ETIMEDOUT:
            sIsConnecting = ID_TRUE;
            break;

        default:
            sIsConnecting = ID_FALSE;
            break;
    }

    return sIsConnecting;
}

