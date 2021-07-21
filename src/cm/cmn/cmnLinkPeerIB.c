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

typedef struct cmnLinkPeerIB
{
    cmnLinkPeer     mLinkPeer;

    cmnLinkDescIB   mDesc;

    acp_uint32_t    mDispatchInfo;
    cmbBlock       *mPendingBlock;
} cmnLinkPeerIB;

extern cmnIB gIB;

ACI_RC cmnLinkPeerInitializeIB(cmnLink *aLink)
{
    cmnLinkPeerIB *sLink = (cmnLinkPeerIB *)aLink;

    sLink->mDesc.mSock         = CMN_INVALID_SOCKET_HANDLE;
    sLink->mDesc.mBlockingMode = ACP_TRUE;
    sLink->mDesc.mLatency      = 0;
    sLink->mDesc.mConChkSpin   = 0;

    sLink->mDispatchInfo       = 0;
    sLink->mPendingBlock       = NULL;

    return ACI_SUCCESS;
}

ACI_RC cmnLinkPeerFinalizeIB(cmnLink *aLink)
{
    cmnLinkPeerIB *sLink = (cmnLinkPeerIB *)aLink;
    cmbPool       *sPool = sLink->mLinkPeer.mPool;

    ACI_TEST(aLink->mOp->mClose(aLink) != ACI_SUCCESS);

    if (sLink->mPendingBlock != NULL)
    {
        ACI_TEST(sPool->mOp->mFreeBlock(sPool, sLink->mPendingBlock) != ACI_SUCCESS);
    }
    else
    {
        /* nothing to do */
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC cmnLinkPeerCloseIB(cmnLink *aLink)
{
    cmnLinkPeerIB *sLink = (cmnLinkPeerIB *)aLink;

    if (sLink->mDesc.mSock != CMN_INVALID_SOCKET_HANDLE)
    {
        ACI_TEST(gIB.mFuncs.rclose(sLink->mDesc.mSock) < 0);

        sLink->mDesc.mSock = CMN_INVALID_SOCKET_HANDLE;
    }
    else
    {
        /* nothing to do */
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC cmnLinkPeerGetSockIB(cmnLink *aLink, void **aSock)
{
    cmnLinkPeerIB *sLink = (cmnLinkPeerIB *)aLink;

    *(acp_sint32_t **)aSock = &sLink->mDesc.mSock;

    return ACI_SUCCESS;
}

ACI_RC cmnLinkPeerGetDispatchInfoIB(cmnLink *aLink, void *aDispatchInfo)
{
    cmnLinkPeerIB *sLink = (cmnLinkPeerIB *)aLink;

    *(acp_uint32_t *)aDispatchInfo = sLink->mDispatchInfo;

    return ACI_SUCCESS;
}

ACI_RC cmnLinkPeerSetDispatchInfoIB(cmnLink *aLink, void *aDispatchInfo)
{
    cmnLinkPeerIB *sLink = (cmnLinkPeerIB *)aLink;

    sLink->mDispatchInfo = *(acp_uint32_t *)aDispatchInfo;

    return ACI_SUCCESS;
}

ACI_RC cmnLinkPeerSetBlockingModeIB(cmnLinkPeer *aLink, acp_bool_t aBlockingMode)
{
    cmnLinkPeerIB *sLink = (cmnLinkPeerIB *)aLink;

    ACI_TEST(cmnSockSetBlockingModeIB(&sLink->mDesc.mSock, aBlockingMode) != ACI_SUCCESS);

    sLink->mDesc.mBlockingMode = aBlockingMode;

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC cmnLinkPeerGetInfoIB( cmnLinkPeer    *aLink,
                             acp_char_t     *aBuf,
                             acp_uint32_t    aBufLen,
                             cmnLinkInfoKey  aKey )
{
    cmnLinkPeerIB      *sLink = (cmnLinkPeerIB *)aLink;
    acp_sock_len_t      sAddrLen;
    acp_rc_t            sRet = ACP_RC_SUCCESS;

    acp_sock_addr_storage_t  sAddr;
    acp_char_t               sAddrStr[ACP_INET_IP_ADDR_MAX_LEN];
    acp_char_t               sPortStr[ACP_INET_IP_PORT_MAX_LEN];

    /* proj-1538 ipv6: use getnameinfo */
    switch (aKey)
    {
        case CMN_LINK_INFO_ALL:
        case CMN_LINK_INFO_REMOTE_ADDRESS:
        case CMN_LINK_INFO_REMOTE_IP_ADDRESS:
        case CMN_LINK_INFO_REMOTE_PORT:

            ACI_TEST_RAISE(
                acpInetGetNameInfo((acp_sock_addr_t *)&sLink->mDesc.mAddr,
                                   sLink->mDesc.mAddrLen,
                                   sAddrStr,
                                   ACI_SIZEOF(sAddrStr),
                                   ACP_INET_NI_NUMERICHOST) != ACP_RC_SUCCESS,
                GetNameInfoError);

            ACI_TEST_RAISE(
                acpInetGetServInfo((acp_sock_addr_t *)&sLink->mDesc.mAddr,
                                   sLink->mDesc.mAddrLen,
                                   sPortStr,
                                   ACI_SIZEOF(sPortStr),
                                   ACP_INET_NI_NUMERICSERV) != ACP_RC_SUCCESS,
                GetNameInfoError);
            break;

        case CMN_LINK_INFO_LOCAL_ADDRESS:
        case CMN_LINK_INFO_LOCAL_IP_ADDRESS:
        case CMN_LINK_INFO_LOCAL_PORT:

            sAddrLen = ACI_SIZEOF(sAddr);
            ACI_TEST_RAISE(gIB.mFuncs.rgetsockname(sLink->mDesc.mSock,
                                                   (acp_sock_addr_t *)&sAddr,
                                                   (acp_sock_len_t *)&sAddrLen) != 0,
                           GetSockNameError);

            ACI_TEST_RAISE(
                acpInetGetNameInfo((acp_sock_addr_t *)&sAddr,
                                   sAddrLen,
                                   sAddrStr,
                                   ACI_SIZEOF(sAddrStr),
                                   ACP_INET_NI_NUMERICHOST) != ACP_RC_SUCCESS,
                GetNameInfoError);

            ACI_TEST_RAISE(
                acpInetGetServInfo((acp_sock_addr_t *)&sAddr,
                                   sAddrLen,
                                   sPortStr, ACI_SIZEOF(sPortStr),
                                   ACP_INET_NI_NUMERICSERV) != ACP_RC_SUCCESS,
                GetNameInfoError);
            break;

        case CMN_LINK_INFO_IMPL:
        case CMN_LINK_INFO_REMOTE_SOCKADDR:
            break;

        default:
            ACI_RAISE(UnsupportedLinkInfoKey);
            break;
    }

    switch (aKey)
    {
        case CMN_LINK_INFO_ALL:
            sRet = acpSnprintf(aBuf, aBufLen, "IB %s:%s", sAddrStr, sPortStr);
            break;

        case CMN_LINK_INFO_IMPL:
            sRet = acpSnprintf(aBuf, aBufLen, "IB");
            break;

        case CMN_LINK_INFO_LOCAL_ADDRESS:
        case CMN_LINK_INFO_REMOTE_ADDRESS:
            sRet = acpSnprintf(aBuf, aBufLen, "%s:%s", sAddrStr, sPortStr);
            break;

        case CMN_LINK_INFO_LOCAL_IP_ADDRESS:
        case CMN_LINK_INFO_REMOTE_IP_ADDRESS:
            sRet = acpSnprintf(aBuf, aBufLen, "%s", sAddrStr);
            break;

        case CMN_LINK_INFO_LOCAL_PORT:
        case CMN_LINK_INFO_REMOTE_PORT:
            sRet = acpSnprintf(aBuf, aBufLen, "%s", sPortStr);
            break;

        /* proj-1538 ipv6 */
        case CMN_LINK_INFO_REMOTE_SOCKADDR:
            ACI_TEST_RAISE(aBufLen < sLink->mDesc.mAddrLen, StringTruncated);
            acpMemCpy(aBuf, &sLink->mDesc.mAddr, sLink->mDesc.mAddrLen);
            break;

        default:
            ACI_RAISE(UnsupportedLinkInfoKey);
            break;
    }

    ACI_TEST_RAISE(sRet < 0, StringOutputError);
    ACI_TEST_RAISE((acp_uint32_t)sRet >= aBufLen, StringTruncated);

    return ACI_SUCCESS;

    ACI_EXCEPTION(StringOutputError);
    {
        ACI_SET(aciSetErrorCode(cmERR_ABORT_STRING_OUTPUT_ERROR));
    }
    ACI_EXCEPTION(StringTruncated);
    {
        ACI_SET(aciSetErrorCode(cmERR_IGNORE_STRING_TRUNCATED));
    }
    ACI_EXCEPTION(GetSockNameError);
    {
        ACI_SET(aciSetErrorCode(cmERR_ABORT_GETSOCKNAME_ERROR));
    }
    ACI_EXCEPTION(GetNameInfoError);
    {
        ACI_SET(aciSetErrorCode(cmERR_ABORT_GETNAMEINFO_ERROR));
    }
    ACI_EXCEPTION(UnsupportedLinkInfoKey);
    {
        ACI_SET(aciSetErrorCode(cmERR_ABORT_UNSUPPORTED_LINK_INFO_KEY));
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC cmnLinkPeerGetDescIB(cmnLinkPeer *aLink, void *aDesc)
{
    cmnLinkPeerIB *sLink = (cmnLinkPeerIB *)aLink;

    *(cmnLinkDescIB **)aDesc = &sLink->mDesc;

    return ACI_SUCCESS;
}

/* PROJ-2625 Semi-async Prefetch, Prefetch Auto-tuning */
ACI_RC cmnLinkPeerGetSndBufSizeIB(cmnLinkPeer *aLink, acp_sint32_t *aSndBufSize)
{
    cmnLinkPeerIB *sLink = (cmnLinkPeerIB *)aLink;

    ACI_TEST(cmnSockGetSndBufSizeIB(&sLink->mDesc.mSock, aSndBufSize) != ACI_SUCCESS);

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC cmnLinkPeerSetSndBufSizeIB(cmnLinkPeer *aLink, acp_sint32_t aSndBufSize)
{
    cmnLinkPeerIB *sLink = (cmnLinkPeerIB *)aLink;

    ACI_TEST(cmnSockSetSndBufSizeIB(&sLink->mDesc.mSock, aSndBufSize) != ACI_SUCCESS);

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC cmnLinkPeerGetRcvBufSizeIB(cmnLinkPeer *aLink, acp_sint32_t *aRcvBufSize)
{
    cmnLinkPeerIB *sLink = (cmnLinkPeerIB *)aLink;

    ACI_TEST(cmnSockGetRcvBufSizeIB(&sLink->mDesc.mSock, aRcvBufSize) != ACI_SUCCESS);

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC cmnLinkPeerSetRcvBufSizeIB(cmnLinkPeer *aLink, acp_sint32_t aRcvBufSize)
{
    cmnLinkPeerIB *sLink = (cmnLinkPeerIB *)aLink;

    ACI_TEST(cmnSockSetRcvBufSizeIB(&sLink->mDesc.mSock, aRcvBufSize) != ACI_SUCCESS);

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

static ACI_RC cmnLinkPeerConnByIP(cmnLinkPeer          *aLink,
                                  cmnLinkConnectArg    *aConnectArg,
                                  acp_time_t            aTimeout,
                                  acp_sint32_t          aOption,
                                  acp_inet_addr_info_t *aAddr,
                                  acp_inet_addr_info_t *aBindAddr);

static ACI_RC cmnLinkPeerConnByName(cmnLinkPeer          *aLink,
                                    cmnLinkConnectArg    *aConnectArg,
                                    acp_time_t            aTimeout,
                                    acp_sint32_t          aOption,
                                    acp_inet_addr_info_t *aAddr,
                                    acp_inet_addr_info_t *aBindAddr);

static acp_sint32_t cmnConnectTimedWait(acp_sint32_t     aSock,
                                        struct sockaddr *aAddr,
                                        int              aAddrLen,
                                        acp_time_t       aTimeout,
                                        acp_bool_t       aBlockingMode);

ACI_RC cmnLinkPeerConnectIB(cmnLinkPeer        *aLink,
                             cmnLinkConnectArg *aConnectArg,
                             acp_time_t         aTimeout,
                             acp_sint32_t       aOption)
{
    acp_inet_addr_info_t  *sBindAddr  = NULL;
    acp_inet_addr_info_t  *sAddr = NULL;
    acp_bool_t             sAddrIsIP = ACP_FALSE;

    /* error, if the libraryr for IB was failed to load */
    ACI_TEST_RAISE(gIB.mRdmaCmHandle.mHandle == NULL, FailedToLoadIBLibrary);

    /* bind */
    if (aConnectArg->mIB.mBindAddr != NULL)
    {
        ACI_TEST(cmnGetAddrInfo(&sBindAddr, NULL,
                                aConnectArg->mIB.mBindAddr, 0)
                 != ACI_SUCCESS);
    }
    else
    {
        /* no binding */
    }

    /* *********************************************************
     * proj-1538 ipv6: use getaddrinfo()
     * *********************************************************/
    ACI_TEST(cmnGetAddrInfo(&sAddr, &sAddrIsIP,
                            aConnectArg->mIB.mAddr,
                            aConnectArg->mIB.mPort)
             != ACI_SUCCESS);

    /* in case that a user inputs the IP address directly */
    if (sAddrIsIP == ACP_TRUE)
    {
        ACI_TEST(cmnLinkPeerConnByIP(aLink, aConnectArg, aTimeout, aOption, sAddr, sBindAddr)
                 != ACI_SUCCESS);
    }
    /* in case that a user inputs the hostname instead of IP */
    else
    {
        ACI_TEST(cmnLinkPeerConnByName(aLink, aConnectArg, aTimeout, aOption, sAddr, sBindAddr)
                 != ACI_SUCCESS);
    }

    if (sAddr != NULL)
    {
        acpInetFreeAddrInfo(sAddr);
    }
    else
    {
        /* nothing to do */
    }

    if (sBindAddr != NULL)
    {
        acpInetFreeAddrInfo(sBindAddr);
    }
    else
    {
        /* nothing to do */
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION(FailedToLoadIBLibrary)
    {
        ACI_SET(aciSetErrorCode(cmERR_ABORT_DLSYM,
                                CMN_IB_RDMACM_LIB_NAME,
                                gIB.mLibErrorMsg));
    }
    ACI_EXCEPTION_END;

    if (sAddr != NULL)
    {
        acpInetFreeAddrInfo(sAddr);
    }
    else
    {
        /* nothing to do */
    }

    if (sBindAddr != NULL)
    {
        acpInetFreeAddrInfo(sBindAddr);
    }
    else
    {
        /* nothing to do */
    }

    return ACI_FAILURE;
}

/* proj-1538 ipv6 */
/* in case that a user inputs the IP address directly */
static ACI_RC cmnLinkPeerConnByIP(cmnLinkPeer          *aLink,
                                  cmnLinkConnectArg    *aConnectArg,
                                  acp_time_t            aTimeout,
                                  acp_sint32_t          aOption,
                                  acp_inet_addr_info_t *aAddr,
                                  acp_inet_addr_info_t *aBindAddr)
{
    cmnLinkPeerIB *sLink = (cmnLinkPeerIB *)aLink;
    acp_sint32_t   sSock;
    acp_sint32_t   sOption;
    acp_sint32_t   sRet;
    acp_char_t     sErrMsg[256];

    sSock = gIB.mFuncs.rsocket(aAddr->ai_family, SOCK_STREAM, 0);

    ACI_TEST_RAISE(sSock == CMN_INVALID_SOCKET_HANDLE, SocketError);

    /* BUG-44271 */
    if (aBindAddr != NULL)
    {
        /* SO_REUSEADDR */
        /* BUG-34045 Client's socket listen function can not use reuse address socket option */
        sOption = 1;

        if (gIB.mFuncs.rsetsockopt(sSock,
                                   SOL_SOCKET,
                                   SO_REUSEADDR,
                                   &sOption,
                                   ACI_SIZEOF(sOption)) < 0)
        {
            acpSnprintf(sErrMsg, ACI_SIZEOF(sErrMsg), "SO_REUSEADDR: %"ACI_INT32_FMT, errno);
            ACI_RAISE(SetSockOptError);
        }
        else
        {
            /* nothing to do */
        }

        sRet = gIB.mFuncs.rbind(sLink->mDesc.mSock,
                                (acp_sock_addr_t*)aBindAddr->ai_addr,
                                aBindAddr->ai_addrlen);

        ACI_TEST_RAISE(sRet < 0, BindError);
    }
    else
    {
        /* no binding */
    }

    sRet = cmnConnectTimedWait(sSock,
                               (acp_sock_addr_t *)aAddr->ai_addr,
                               aAddr->ai_addrlen,
                               aTimeout,
                               sLink->mDesc.mBlockingMode);

    ACI_TEST_RAISE(sRet != 0, ConnectError);

    /* save fd and IP address into link info */
    sLink->mDesc.mSock = sSock;

    acpMemCpy(&sLink->mDesc.mAddr, aAddr->ai_addr, aAddr->ai_addrlen);
    sLink->mDesc.mAddrLen = aAddr->ai_addrlen;

    /* socket 초기화 */
    sLink->mDesc.mLatency = aConnectArg->mIB.mLatency;
    sLink->mDesc.mConChkSpin = aConnectArg->mIB.mConChkSpin;

    ACI_TEST(aLink->mPeerOp->mSetOptions(aLink, aOption) != ACI_SUCCESS);

    return ACI_SUCCESS;

    ACI_EXCEPTION(SocketError)
    {
        ACI_SET(aciSetErrorCode(cmERR_ABORT_IB_RSOCKET_OPEN_ERROR, errno));
    }
    ACI_EXCEPTION(SetSockOptError);
    {
        ACI_SET(aciSetErrorCode(cmERR_ABORT_IB_RSETSOCKOPT_ERROR, sErrMsg));
    }
    ACI_EXCEPTION(BindError)
    {
        ACI_SET(aciSetErrorCode(cmERR_ABORT_IB_RBIND_ERROR, errno));
    }
    ACI_EXCEPTION(ConnectError)
    {
        /* bug-30835: support link-local address with zone index.
         * On linux, link-local addr without zone index makes EINVAL.
         *  .EINVAL   : Invalid argument to connect()
         *  .others   : Client unable to establish connection.
         * caution: acpSockTimedConnect does not change errno.
         *  Instead, it returns errno directly. */
        if (errno == EINVAL)
        {
            ACI_SET(aciSetErrorCode(cmERR_ABORT_IB_RCONNECT_INVALIDARG));
        }
        else
        {
            ACI_SET(aciSetErrorCode(cmERR_ABORT_IB_RCONNECT_ERROR, errno));
        }
    }
    ACI_EXCEPTION_END;

    // BUG-24170 [CM] cmiConnect 실패 시, cmiConnect 내에서 close
    if (sSock != CMN_INVALID_SOCKET_HANDLE)
    {
        (void)gIB.mFuncs.rclose(sSock);
    }
    sLink->mDesc.mSock = CMN_INVALID_SOCKET_HANDLE;

    return ACI_FAILURE;
}

/* proj-1538 ipv6 */
/* in case that a user inputs the hostname instead of IP */
static ACI_RC cmnLinkPeerConnByName(cmnLinkPeer          *aLink,
                                    cmnLinkConnectArg    *aConnectArg,
                                    acp_time_t            aTimeout,
                                    acp_sint32_t          aOption,
                                    acp_inet_addr_info_t *aAddr,
                                    acp_inet_addr_info_t *aBindAddr)
{
    cmnLinkPeerIB         *sLink = (cmnLinkPeerIB *)aLink;
    acp_inet_addr_info_t  *sAddr = NULL;

#define CM_HOST_IP_MAX_COUNT 10
    acp_inet_addr_info_t  *sAddrListV4[CM_HOST_IP_MAX_COUNT];
    acp_inet_addr_info_t  *sAddrListV6[CM_HOST_IP_MAX_COUNT];
    acp_inet_addr_info_t **sAddrListFinal = NULL;

    acp_sint32_t           sAddrCntV4 = 0;
    acp_sint32_t           sAddrCntV6 = 0;
    acp_sint32_t           sAddrCntFinal = 0;

    acp_sint32_t           sIdx    = 0;
    acp_sint32_t           sTryCnt = 0;
    acp_sint32_t           sSock;
    acp_sint32_t           sOption;
    acp_sint32_t           sRet = -1;
    acp_char_t             sErrMsg[256];

    acpMemSet(sAddrListV4, 0x00, ACI_SIZEOF(sAddrListV4));
    acpMemSet(sAddrListV6, 0x00, ACI_SIZEOF(sAddrListV6));

    for (sAddr = aAddr;
         (sAddr != NULL) && (sAddrCntFinal < CM_HOST_IP_MAX_COUNT);
         sAddr = sAddr->ai_next)
    {
        /* IPv4 */
        if (sAddr->ai_family == AF_INET)
        {
            sAddrListV4[sAddrCntV4++] = sAddr;
            sAddrCntFinal++;
        }
        /* IPv6 */
        else if (sAddr->ai_family == AF_INET6)
        {
            sAddrListV6[sAddrCntV6++] = sAddr;
            sAddrCntFinal++;
        }
    }

    /* if prefer IPv4, then order is v4 -> v6 */
    if (aConnectArg->mIB.mPreferIPv6 == 0)
    {
        for (sIdx = 0;
             (sIdx < sAddrCntV6) && (sAddrCntFinal < CM_HOST_IP_MAX_COUNT);
             sIdx++)
        {
            sAddrListV4[sAddrCntV4] = sAddrListV6[sIdx];
            sAddrCntV4++;
        }
        sAddrListFinal = &sAddrListV4[0];
    }
    /* if prefer IPv6, then order is v6 -> v4 */
    else
    {
        for (sIdx = 0;
             (sIdx < sAddrCntV4) && (sAddrCntFinal < CM_HOST_IP_MAX_COUNT);
             sIdx++)
        {
            sAddrListV6[sAddrCntV6] = sAddrListV4[sIdx];
            sAddrCntV6++;
        }
        sAddrListFinal = &sAddrListV6[0];
    }

    sTryCnt = 0;
    for (sIdx = 0; sIdx < sAddrCntFinal; sIdx++)
    {
        sTryCnt++;
        sAddr = sAddrListFinal[sIdx];

        sSock = gIB.mFuncs.rsocket(sAddr->ai_family,
                                    ACP_SOCK_STREAM,
                                    0);

        ACI_TEST_RAISE(sSock == CMN_INVALID_SOCKET_HANDLE, SocketError);

        /* BUG-44271 */
        if (aBindAddr != NULL)
        {
            /* SO_REUSEADDR */
            /* BUG-34045 Client's socket listen function can not use reuse address socket option */
            sOption = 1;

            if (gIB.mFuncs.rsetsockopt(sSock,
                                       SOL_SOCKET,
                                       SO_REUSEADDR,
                                       &sOption,
                                       ACI_SIZEOF(sOption)) < 0)
            {
                acpSnprintf(sErrMsg, ACI_SIZEOF(sErrMsg), "SO_REUSEADDR: %"ACI_INT32_FMT, errno);
                ACI_RAISE(SetSockOptError);
            }
            else
            {
                /* nothing to do */
            }

            sRet = gIB.mFuncs.rbind(sSock,
                                    (acp_sock_addr_t*)aBindAddr->ai_addr,
                                    aBindAddr->ai_addrlen);

            ACI_TEST_RAISE(sRet != 0, BindError);
        }
        else
        {
            /* no binding */
        }

        sRet = cmnConnectTimedWait(sSock,
                                   (acp_sock_addr_t *)sAddr->ai_addr,
                                   sAddr->ai_addrlen,
                                   aTimeout,
                                   sLink->mDesc.mBlockingMode);
        if (ACP_RC_IS_SUCCESS(sRet))
        {
            break;
        }

        if (sSock != CMN_INVALID_SOCKET_HANDLE)
        {
            (void)gIB.mFuncs.rclose(sSock);
            sSock = CMN_INVALID_SOCKET_HANDLE;
        }
    }

    ACI_TEST_RAISE((sRet != 0) || (sTryCnt == 0), ConnectError);

    /* save fd and IP address into link info */
    sLink->mDesc.mSock = sSock;

    acpMemCpy(&sLink->mDesc.mAddr, sAddr->ai_addr, sAddr->ai_addrlen);
    sLink->mDesc.mAddrLen = sAddr->ai_addrlen;

    /* socket 초기화 */
    sLink->mDesc.mLatency = aConnectArg->mIB.mLatency;
    sLink->mDesc.mConChkSpin = aConnectArg->mIB.mConChkSpin;

    ACI_TEST(aLink->mPeerOp->mSetOptions(aLink, aOption) != ACI_SUCCESS);

    return ACI_SUCCESS;

    ACI_EXCEPTION(SocketError)
    {
        ACI_SET(aciSetErrorCode(cmERR_ABORT_IB_RSOCKET_OPEN_ERROR, errno));
    }
    ACI_EXCEPTION(SetSockOptError);
    {
        ACI_SET(aciSetErrorCode(cmERR_ABORT_IB_RSETSOCKOPT_ERROR, sErrMsg));
    }
    ACI_EXCEPTION(BindError)
    {
        ACI_SET(aciSetErrorCode(cmERR_ABORT_IB_RBIND_ERROR, errno));
    }
    ACI_EXCEPTION(ConnectError)
    {
        /* bug-30835: support link-local address with zone index.
         * On linux, link-local addr without zone index makes EINVAL. */
        if (errno == EINVAL)
        {
            ACI_SET(aciSetErrorCode(cmERR_ABORT_IB_RCONNECT_INVALIDARG));
        }
        else
        {
            ACI_SET(aciSetErrorCode(cmERR_ABORT_IB_RCONNECT_ERROR, errno));
        }
    }
    ACI_EXCEPTION_END;

    /* BUG-24170 [CM] cmiConnect 실패 시, cmiConnect 내에서 close */
    if (sLink->mDesc.mSock != CMN_INVALID_SOCKET_HANDLE)
    {
        (void)gIB.mFuncs.rclose(sLink->mDesc.mSock);
        sLink->mDesc.mSock = CMN_INVALID_SOCKET_HANDLE;
    }

    return ACI_FAILURE;
}

/* It's similar to acpSockTimedConnect(). */
static acp_sint32_t cmnConnectTimedWait(acp_sint32_t     aSock,
                                        struct sockaddr *aAddr,
                                        int              aAddrLen,
                                        acp_time_t       aTimeout,
                                        acp_bool_t       aBlockingMode)
{
    acp_sint32_t    sRet;
    acp_sint32_t    sError = 0;
    acp_sint32_t    sLen;
    struct pollfd   sPollFd;
    acp_sint32_t    sTimeout = (aTimeout != ACP_TIME_INFINITE) ? acpTimeToMsec(aTimeout) : -1;
    acp_bool_t      sChangedBlockingMode = ACP_FALSE;

    if (aBlockingMode == ACP_TRUE)
    {
        /* change non-blocking mode only when connecting */
        ACI_TEST(cmnSockSetBlockingModeIB(&aSock, ACP_FALSE) != ACI_SUCCESS);

        sChangedBlockingMode = ACP_TRUE;
    }
    else
    {
        /* already non-blocking mode */
    }

    sRet = gIB.mFuncs.rconnect(aSock, aAddr, aAddrLen);
    if (sRet != 0)
    {
        /* error, if the connection isn't now in progress */
        ACI_TEST((sRet < 0) && (errno != EINPROGRESS) && (errno != EWOULDBLOCK)); 

        /**
         * [CAUTION]
         *
         * Berkeley socket 과 다르게 blocking mode 이더라도
         * rconnect() 함수는 'connecting' 상태로 반환될 수 있음.
         */

        /* wait until connecting */
        sPollFd.fd      = aSock;
        sPollFd.events  = POLLIN | POLLOUT;
        sPollFd.revents = 0;

        sRet = gIB.mFuncs.rpoll(&sPollFd, 1, sTimeout);
        ACI_TEST(sRet < 0); /* error */

        ACI_TEST_RAISE(sRet == 0, TimedOut); /* timed out */

        /* error, if it was not occured either read or write event */
        ACI_TEST(sPollFd.revents == 0);

        /* check socket error */
        sLen = (acp_sint32_t)sizeof(sError);

        ACI_TEST(gIB.mFuncs.rgetsockopt(aSock,
                                        SOL_SOCKET,
                                        SO_ERROR,
                                        &sError,
                                        (acp_sock_len_t *)&sLen) < 0);

        ACI_TEST(sError != 0);
    }
    else
    {
        /* connected successfully */
    }

    if (sChangedBlockingMode == ACP_TRUE)
    {
        (void)cmnSockSetBlockingModeIB(&aSock, ACP_TRUE);
    }
    else
    {
        /* not changed */
    }

    return 0;

    ACI_EXCEPTION(TimedOut)
    {
        sError = ETIMEDOUT;
    }
    ACI_EXCEPTION_END;

    if (sChangedBlockingMode == ACP_TRUE)
    {
        (void)cmnSockSetBlockingModeIB(&aSock, ACP_TRUE);
    }
    else
    {
        /* nothing to do */
    }

    if (sError != 0)
    {
        errno = sError;
    }
    else
    {
        /* nothing to do */
    }

    return -1;
}

ACI_RC cmnLinkPeerAllocChannelIB(cmnLinkPeer *aLink, acp_sint32_t *aChannelID)
{
    ACP_UNUSED(aLink);
    ACP_UNUSED(aChannelID);

    return ACI_SUCCESS;
}

ACI_RC cmnLinkPeerHandshakeIB(cmnLinkPeer *aLink)
{
    ACP_UNUSED(aLink);

    return ACI_SUCCESS;
}

ACI_RC cmnLinkPeerSetOptionsIB(cmnLinkPeer *aLink, acp_sint32_t aOption)
{
    cmnLinkPeerIB *sLink = (cmnLinkPeerIB *)aLink;
    acp_sint32_t   sOption;
    struct linger  sLingerOption;

    /*
     * SO_KEEPALIVE 세팅
     */
    sOption = 1;

    (void)gIB.mFuncs.rsetsockopt(sLink->mDesc.mSock,
                                 SOL_SOCKET,
                                 SO_KEEPALIVE,
                                 &sOption,
                                 ACI_SIZEOF(sOption));

    /*
     * BUG-26484: 추가로 설정할 소켓 옵션을 지정
     */
    if (aOption == SO_LINGER)
    {
        /*
         * 연속으로 연결했다 끊기를 반복하면 더 이상 연결할 수 없게된다.
         * 일반적으로 소켓은 close해도 TIME_WAIT 상태로 일정시간 대기하기 때문이다.
         * SO_LINGER 옵션 추가. (SO_REUSEADDR 옵션으로는 잘 안될 수도 있다;)
         *
         * SO_LINGER 세팅
        */
        sLingerOption.l_onoff  = 1;
        sLingerOption.l_linger = 0;

        (void)gIB.mFuncs.rsetsockopt(sLink->mDesc.mSock,
                                     SOL_SOCKET,
                                     SO_LINGER,
                                     &sLingerOption,
                                     ACI_SIZEOF(sLingerOption));
    }
    else if (aOption == SO_REUSEADDR)
    {
        /*
         * SO_REUSEADDR 세팅
         */
        sOption = 1;

        (void)gIB.mFuncs.rsetsockopt(sLink->mDesc.mSock,
                                     SOL_SOCKET,
                                     SO_REUSEADDR,
                                     &sOption,
                                     ACI_SIZEOF(sOption));
    }
    else
    {
        /* ignore unsupported option */
    }

    /*
     * TCP_NODELAY 세팅
     */
    sOption = 1;

    (void)gIB.mFuncs.rsetsockopt(sLink->mDesc.mSock,
                                 IPPROTO_TCP,
                                 TCP_NODELAY,
                                 &sOption,
                                 ACI_SIZEOF(sOption));

    /* BUG-22028
     * SO_SNDBUF 세팅 (대역폭 * 지연율) * 2
     */
    sOption = CMB_BLOCK_DEFAULT_SIZE * 2;

    (void)gIB.mFuncs.rsetsockopt(sLink->mDesc.mSock,
                                 SOL_SOCKET,
                                 SO_SNDBUF,
                                 &sOption,
                                 ACI_SIZEOF(sOption));

    /*
     * BUG-22028
     * SO_RCVBUF 세팅 (대역폭 * 지연율) * 2
     */
    sOption = CMB_BLOCK_DEFAULT_SIZE * 2;

    (void)gIB.mFuncs.rsetsockopt(sLink->mDesc.mSock,
                                 SOL_SOCKET,
                                 SO_RCVBUF,
                                 &sOption,
                                 ACI_SIZEOF(sOption));

    /**
     * RDMA_LATENCY
     */
    if (sLink->mDesc.mLatency != 0)
    {
        sOption = sLink->mDesc.mLatency;

        (void)gIB.mFuncs.rsetsockopt(sLink->mDesc.mSock,
                                     SOL_RDMA,
                                     RDMA_LATENCY,
                                     &sOption,
                                     ACI_SIZEOF(sOption));
    }
    else
    {
        /* default RDMA_LATENCY = 0 in rsockets */
    }

    /* RDMA_CONCHKSPIN */
    if (sLink->mDesc.mConChkSpin!= 0)
    {
        sOption = sLink->mDesc.mConChkSpin;

        (void)gIB.mFuncs.rsetsockopt(sLink->mDesc.mSock,
                                     SOL_RDMA,
                                     RDMA_CONCHKSPIN,
                                     &sOption,
                                     ACI_SIZEOF(sOption));
    }
    else
    {
        /* default RDMA_CONCHKSPIN = 0 in rsockets */
    }

    return ACI_SUCCESS;
}

ACI_RC cmnLinkPeerShutdownIB(cmnLinkPeer     *aLink,
                             cmnDirection     aDirection,
                             cmnShutdownMode  aMode)
{
    cmnLinkPeerIB *sLink = (cmnLinkPeerIB *)aLink;
    acp_sint32_t   sRet;

    ACP_UNUSED(aMode);

    /* skip, if the library for IB was failed to load */
    ACI_TEST_RAISE((gIB.mRdmaCmHandle.mHandle == NULL) ||
                   (sLink->mDesc.mSock == CMN_INVALID_SOCKET_HANDLE),  /* BUG-46161 */
                   SkipShutdown);

    sRet = gIB.mFuncs.rshutdown(sLink->mDesc.mSock, aDirection);

    ACI_TEST_RAISE((sRet != 0) && (errno != ENOTCONN), ShutdownError);

    ACI_EXCEPTION_CONT(SkipShutdown);

    return ACI_SUCCESS;

    ACI_EXCEPTION(ShutdownError);
    {
        ACI_SET(aciSetErrorCode(cmERR_ABORT_IB_RSOCKET_SHUTDOWN_FAILED));
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC cmnLinkPeerRecvIB( cmnLinkPeer  *aLink,
                          cmbBlock    **aBlock,
                          cmpHeader    *aHeader,
                          acp_time_t    aTimeout )
{
    cmnLinkPeerIB  *sLink  = (cmnLinkPeerIB *)aLink;
    cmbPool        *sPool  = aLink->mPool;
    cmbBlock       *sBlock = NULL;
    cmpHeader       sHeader;
    acp_uint16_t    sPacketSize = 0;
    cmpPacketType   sPacketType = aLink->mLink.mPacketType;

    /* not accept A5 protocol */
    ACI_TEST_RAISE(sPacketType == CMP_PACKET_TYPE_A5, UnsupportedNetworkProtocol);

    /*
     * Pending Block있으면 사용 그렇지 않으면 Block 할당
     */
    /* proj_2160 cm_type removal */
    /* A7 or CMP_PACKET_TYPE_UNKNOWN: block already allocated. */
    sBlock = *aBlock;

    if (sLink->mPendingBlock != NULL)
    {
        acpMemCpy(sBlock->mData,
                  sLink->mPendingBlock->mData,
                  sLink->mPendingBlock->mDataSize);
        sBlock->mDataSize = sLink->mPendingBlock->mDataSize;

        ACI_TEST(sPool->mOp->mFreeBlock(sPool, sLink->mPendingBlock) != ACI_SUCCESS);
        sLink->mPendingBlock = NULL;
    }

    /*
     * Protocol Header Size 크기 이상 읽음
     */
    ACI_TEST_RAISE(cmnSockRecvIB(sBlock,
                                 aLink,
                                 &sLink->mDesc.mSock,
                                 CMP_HEADER_SIZE,
                                 aTimeout) != ACI_SUCCESS, SockRecvError);

    /*
     * Protocol Header 해석
     */
    ACI_TEST(cmpHeaderRead(aLink, &sHeader, sBlock) != ACI_SUCCESS);
    sPacketSize = sHeader.mA7.mPayloadLength + CMP_HEADER_SIZE;

    /*
     * 패킷 크기 이상 읽음
     */
    ACI_TEST_RAISE(cmnSockRecvIB(sBlock,
                                 aLink,
                                 &sLink->mDesc.mSock,
                                 sPacketSize,
                                 aTimeout) != ACI_SUCCESS, SockRecvError);

    /*
     * 패킷 크기 이상 읽혔으면 현재 패킷 이후의 데이터를 Pending Block으로 옮김
     */
    if (sBlock->mDataSize > sPacketSize)
    {
        ACI_TEST(sPool->mOp->mAllocBlock(sPool, &sLink->mPendingBlock) != ACI_SUCCESS);
        ACI_TEST(cmbBlockMove(sLink->mPendingBlock, sBlock, sPacketSize) != ACI_SUCCESS);
    }

    /*
     * Block과 Header를 돌려줌
     */
    /* proj_2160 cm_type removal
     *  Do not use mLink.mPacketType. instead, use sPacketType.
     * cuz, mLink's value could be changed in cmpHeaderRead().
     * and, this if-stmt shows explicitly that
     * it needs to return block ptr in only A5. */

    *aHeader = sHeader;

    return ACI_SUCCESS;
    
    ACI_EXCEPTION(UnsupportedNetworkProtocol)
    {
        ACI_SET(aciSetErrorCode(cmERR_ABORT_UNSUPPORTED_NETWORK_PROTOCOL));
    }
    /* BUG-39127  If a network timeout occurs during replication sync 
     * then it fails by communication protocol error. */
    ACI_EXCEPTION(SockRecvError)
    {
        if ( sBlock->mDataSize != 0 )
        {
            if ( sPool->mOp->mAllocBlock( sPool, &sLink->mPendingBlock ) == ACI_SUCCESS )
            {
                ACE_ASSERT( cmbBlockMove(sLink->mPendingBlock, sBlock, 0 ) == ACI_SUCCESS );
            }
            else
            {
                /* When an alloc error occurs, the error must be thrown to the upper module;
                 * therefore, ACI_PUSH() and ACI_POP() cannot be used.
                 */
            }
        }
        else
        {
            /* nothing to do */
        }

        if ( sPacketType == CMP_PACKET_TYPE_A5 )
        {
            *aBlock = sBlock;
        }
        else
        {
            /* nothing to do */
        }
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC cmnLinkPeerReqCompleteIB(cmnLinkPeer *aLink)
{
    ACP_UNUSED(aLink);

    return ACI_SUCCESS;
}

ACI_RC cmnLinkPeerResCompleteIB(cmnLinkPeer *aLink)
{
    ACP_UNUSED(aLink);

    return ACI_SUCCESS;
}

ACI_RC cmnLinkPeerSendIB(cmnLinkPeer *aLink, cmbBlock *aBlock)
{
    cmnLinkPeerIB *sLink = (cmnLinkPeerIB *)aLink;

    ACI_TEST(cmnSockSendIB(aBlock,
                           aLink,
                           &sLink->mDesc.mSock,
                           ACP_TIME_INFINITE) != ACI_SUCCESS);

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC cmnLinkPeerCheckIB(cmnLinkPeer *aLink, acp_bool_t *aIsClosed)
{
    cmnLinkPeerIB *sLink = (cmnLinkPeerIB *)aLink;

    return cmnSockCheckIB((cmnLink *)aLink, &sLink->mDesc.mSock, aIsClosed);
}

acp_bool_t cmnLinkPeerHasPendingRequestIB(cmnLinkPeer *aLink)
{
    cmnLinkPeerIB *sLink = (cmnLinkPeerIB *)aLink;

    return (sLink->mPendingBlock != NULL) ? ACP_TRUE : ACP_FALSE;
}

ACI_RC cmnLinkPeerAllocBlockIB(cmnLinkPeer *aLink, cmbBlock **aBlock)
{
    cmbBlock *sBlock;

    ACI_TEST(aLink->mPool->mOp->mAllocBlock(aLink->mPool, &sBlock) != ACI_SUCCESS);

    sBlock->mDataSize = CMP_HEADER_SIZE;
    sBlock->mCursor   = CMP_HEADER_SIZE;

    *aBlock = sBlock;

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC cmnLinkPeerFreeBlockIB(cmnLinkPeer *aLink, cmbBlock *aBlock)
{
    ACI_TEST(aLink->mPool->mOp->mFreeBlock(aLink->mPool, aBlock) != ACI_SUCCESS);

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

struct cmnLinkOP gCmnLinkPeerOpIBClient =
{
    "IB-PEER",

    cmnLinkPeerInitializeIB,
    cmnLinkPeerFinalizeIB,

    cmnLinkPeerCloseIB,

    cmnLinkPeerGetSockIB,

    cmnLinkPeerGetDispatchInfoIB,
    cmnLinkPeerSetDispatchInfoIB
};

struct cmnLinkPeerOP gCmnLinkPeerPeerOpIBClient =
{
    cmnLinkPeerSetBlockingModeIB,

    cmnLinkPeerGetInfoIB,
    cmnLinkPeerGetDescIB,

    cmnLinkPeerConnectIB,
    cmnLinkPeerSetOptionsIB,

    cmnLinkPeerAllocChannelIB,
    cmnLinkPeerHandshakeIB,

    cmnLinkPeerShutdownIB,

    cmnLinkPeerRecvIB,
    cmnLinkPeerSendIB,

    cmnLinkPeerReqCompleteIB,
    cmnLinkPeerResCompleteIB,

    cmnLinkPeerCheckIB,
    cmnLinkPeerHasPendingRequestIB,

    cmnLinkPeerAllocBlockIB,
    cmnLinkPeerFreeBlockIB,

    /* TASK-5894 Permit sysdba via IPC */
    NULL,

    /* PROJ-2474 SSL/TLS */
    NULL,
    NULL,
    NULL,

    /* PROJ-2625 Semi-async Prefetch, Prefetch Auto-tuning */
    cmnLinkPeerGetSndBufSizeIB,
    cmnLinkPeerSetSndBufSizeIB,
    cmnLinkPeerGetRcvBufSizeIB,
    cmnLinkPeerSetRcvBufSizeIB
};

ACI_RC cmnLinkPeerMapIB(cmnLink *aLink)
{
    cmnLinkPeer *sLink = (cmnLinkPeer *)aLink;

    ACE_ASSERT((aLink->mType == CMN_LINK_TYPE_PEER_SERVER) ||
               (aLink->mType == CMN_LINK_TYPE_PEER_CLIENT));
    ACE_ASSERT(aLink->mImpl == CMN_LINK_IMPL_IB);

    ACI_TEST(cmbPoolGetSharedPool(&sLink->mPool, CMB_POOL_IMPL_LOCAL) != ACI_SUCCESS);

    aLink->mOp     = &gCmnLinkPeerOpIBClient;
    sLink->mPeerOp = &gCmnLinkPeerPeerOpIBClient;

    sLink->mUserPtr = NULL;

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

acp_uint32_t cmnLinkPeerSizeIB(void)
{
    return ACI_SIZEOF(cmnLinkPeerIB);
}

