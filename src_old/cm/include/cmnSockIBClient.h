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

#ifndef _O_CMN_SOCK_IB_CLIENT_H_
#define _O_CMN_SOCK_IB_CLIENT_H_ 1

struct cmnLink;

ACI_RC cmnSockCheckIB( cmnLink      *aLink,
                       acp_sint32_t *aSock,
                       acp_bool_t   *aIsClosed );

ACI_RC cmnSockSetBlockingModeIB(acp_sint32_t *aSock, acp_bool_t aBlockingMode);

ACI_RC cmnSockRecvIB(cmbBlock       *aBlock,
                     cmnLinkPeer    *aLink,
                     acp_sint32_t   *aSock,
                     acp_uint16_t    aSize,
                     acp_time_t      aTimeout);

ACI_RC cmnSockSendIB(cmbBlock       *aBlock,
                     cmnLinkPeer    *aLink,
                     acp_sint32_t   *aSock,
                     acp_time_t      aTimeout);

/* PROJ-2625 Semi-async Prefetch, Prefetch Auto-tuning */
ACI_RC cmnSockGetSndBufSizeIB(acp_sint32_t *aSock, acp_sint32_t *aSndBufSize);
ACI_RC cmnSockSetSndBufSizeIB(acp_sint32_t *aSock, acp_sint32_t  aSndBufSize);
ACI_RC cmnSockGetRcvBufSizeIB(acp_sint32_t *aSock, acp_sint32_t *aRcvBufSize);
ACI_RC cmnSockSetRcvBufSizeIB(acp_sint32_t *aSock, acp_sint32_t  aRcvBufSize);

#endif
