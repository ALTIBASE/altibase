/**
 *  Copyright (c) 1999~2017, Altibase Corp. and/or its affiliates. All rights reserved.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License, version 3,
 *  as published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _O_CMN_SOCK_CLIENT_H_
#define _O_CMN_SOCK_CLIENT_H_ 1

#if !defined(CM_DISABLE_TCP) || !defined(CM_DISABLE_UNIX)

struct cmnLink;


ACI_RC cmnSockCheck(cmnLink *aLink, acp_sock_t *aSock, acp_bool_t *aIsClosed);

ACI_RC cmnSockSetBlockingMode(acp_sock_t *aSock, acp_bool_t aBlockingMode);

ACI_RC cmnSockRecv(cmbBlock       *aBlock,
                   cmnLinkPeer    *aLink,
                   acp_sock_t     *aSock,
                   acp_uint16_t    aSize,
                   acp_time_t      aTimeout);

ACI_RC cmnSockSend(cmbBlock       *aBlock,
                   cmnLinkPeer    *aLink,
                   acp_sock_t     *aSock,
                   acp_time_t      aTimeout);

/* PROJ-2625 Semi-async Prefetch, Prefetch Auto-tuning */
ACI_RC cmnSockGetSndBufSize(acp_sock_t *aSock, acp_sint32_t *aSndBufSize);
ACI_RC cmnSockSetSndBufSize(acp_sock_t *aSock, acp_sint32_t  aSndBufSize);
ACI_RC cmnSockGetRcvBufSize(acp_sock_t *aSock, acp_sint32_t *aRcvBufSize);
ACI_RC cmnSockSetRcvBufSize(acp_sock_t *aSock, acp_sint32_t  aRcvBufSize);

#endif

#endif
