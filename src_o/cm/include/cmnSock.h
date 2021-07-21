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

#ifndef _O_CMN_SOCK_H_
#define _O_CMN_SOCK_H_ 1

#if !defined(CM_DISABLE_TCP) || !defined(CM_DISABLE_UNIX)

#include <idl.h>

struct cmnLink;


IDE_RC cmnSockCheck(cmnLink *aLink, PDL_SOCKET aHandle, idBool *aIsClosed);

IDE_RC cmnSockSetBlockingMode(PDL_SOCKET aHandle, idBool aBlockingMode);

IDE_RC cmnSockRecv(cmbBlock       *aBlock,
                   cmnLinkPeer    *aLink,
                   PDL_SOCKET      aHandle,
                   UShort          aSize,
                   PDL_Time_Value *aTimeout,
                   idvStatIndex    aStatIndex);

IDE_RC cmnSockSend(cmbBlock       *aBlock,
                   cmnLinkPeer    *aLink,
                   PDL_SOCKET      aHandle,
                   PDL_Time_Value *aTimeout,
                   idvStatIndex    aStatIndex);

/* PROJ-2625 Semi-async Prefetch, Prefetch Auto-tuning */
IDE_RC cmnSockGetSndBufSize(PDL_SOCKET aHandle, SInt *aSndBufSize);
IDE_RC cmnSockSetSndBufSize(PDL_SOCKET aHandle, SInt  aSndBufSize);
IDE_RC cmnSockGetRcvBufSize(PDL_SOCKET aHandle, SInt *aRcvBufSize);
IDE_RC cmnSockSetRcvBufSize(PDL_SOCKET aHandle, SInt  aRcvBufSize);

#endif

#endif
