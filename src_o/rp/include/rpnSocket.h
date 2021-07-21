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
 

#ifndef _O_RPN_SOCKET_H_
#define _O_RPN_SOCKET_H_    1

#include <acp.h>

typedef struct rpnSocket
{
    acp_sock_t              mSocket;
    idBool                  mIsOpened;

    SInt                    mFamily;

    acp_inet_addr_info_t  * mInetAddressInfo;
} rpnSocket;


IDE_RC rpnSocketGetInetAddrInfo( SChar                 * aIPAddress,
                                 UShort                  aPortNumber,
                                 acp_inet_addr_info_t ** aInetAddressInfo,
                                 SInt                  * aFamily );

IDE_RC rpnSocketInitailize( rpnSocket   * aSocket,
                            SChar       * aIPAddress,
                            UInt          aIPAddressLength,
                            UShort        aPortNumber,
                            idBool        aIsBlockMode );
void rpnSocketFinalize( rpnSocket     * aSocket );


IDE_RC rpnSocketSetBlockMode( rpnSocket     * aSocket,
                              idBool          aIsBlockMode );

IDE_RC rpnSocketConnect( rpnSocket      * aSocket );

IDE_RC rpnSocketGetOpt( rpnSocket   * aSocket,
                        SInt          aLevel,
                        SInt          aOptionName,
                        void        * aOptionValue,
                        ULong       * aOptionLength );

idBool rpnSocketIsConnecting( UInt  aErrorNumber );
#endif

