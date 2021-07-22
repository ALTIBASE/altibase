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

#include <cmAllClient.h>


static ACI_RC (*gCmnDispatcherWaitLinkClient[CMN_DISPATCHER_IMPL_MAX])(cmnLink *aLink, cmnDirection aDirection, acp_time_t aTimeout) =
{
#if defined(CM_DISABLE_TCP) && defined(CM_DISABLE_UNIX)
    NULL,
#else
    cmnDispatcherWaitLinkSOCK,
#endif

#if defined(CM_DISABLE_IPC)
    NULL,
#else
    cmnDispatcherWaitLinkIPC,
#endif

#if defined(CM_DISABLE_IPCDA)
    NULL,
#else
    cmnDispatcherWaitLinkIPCDA,
#endif

    /* PROJ-2681 */
    cmnDispatcherWaitLinkIB
};


acp_bool_t cmnDispatcherIsSupportedImpl(cmnDispatcherImpl aImpl)
{
    switch (aImpl)
    {
#if !defined(CM_DISABLE_TCP) || !defined(CM_DISABLE_UNIX)
        case CMN_DISPATCHER_IMPL_SOCK:
            return ACP_TRUE;
#endif

#if !defined(CM_DISABLE_IPC)
        case CMN_DISPATCHER_IMPL_IPC:
            return ACP_TRUE;
#endif

#if !defined(CM_DISABLE_IPCDA)
        case CMN_DISPATCHER_IMPL_IPCDA:
            return ACP_TRUE;
#endif

        /* PROJ-2681 */
        case CMN_DISPATCHER_IMPL_IB:
            return ACP_TRUE;

        default:
            break;
    }

    return ACP_FALSE;
}

cmnDispatcherImpl cmnDispatcherImplForLinkImpl(cmnLinkImpl aLinkImpl)
{
    /*
     * Link Impl에 따른 Dispatcher Impl 반환
     */
    switch (aLinkImpl)
    {
        case CMN_LINK_IMPL_TCP:
            return CMN_DISPATCHER_IMPL_SOCK;

        case CMN_LINK_IMPL_UNIX:
            return CMN_DISPATCHER_IMPL_SOCK;

        case CMN_LINK_IMPL_IPC:
            return CMN_DISPATCHER_IMPL_IPC;

        case CMN_LINK_IMPL_IPCDA:
            return CMN_DISPATCHER_IMPL_IPCDA;

        /* PROJ-2474 SSL/TLS */
        case CMN_LINK_IMPL_SSL:
            return CMN_DISPATCHER_IMPL_SOCK;

        /* PROJ-2681 */
        case CMN_LINK_IMPL_IB:
            return CMN_DISPATCHER_IMPL_IB;

        default:
            break;
    }

    /*
     * 존재하지 않는 Link Impl인 경우
     */
    return CMN_DISPATCHER_IMPL_INVALID;
}

ACI_RC cmnDispatcherWaitLink(cmnLink *aLink, cmiDirection aDirection, acp_time_t aTimeout)
{
    cmnDispatcherImpl sImpl;

    /*
     * Dispatcher Impl 획득
     */
    sImpl = cmnDispatcherImplForLinkImpl(aLink->mImpl);

    /*
     * Dispatcher Impl 범위 검사
     */
    ACE_ASSERT((acp_sint32_t)sImpl >= CMN_DISPATCHER_IMPL_BASE);
    ACE_ASSERT(sImpl < CMN_DISPATCHER_IMPL_MAX);

    /*
     * WaitLink 함수 호출
     */
    ACI_TEST(gCmnDispatcherWaitLinkClient[sImpl](aLink, aDirection, aTimeout) != ACI_SUCCESS);

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}
