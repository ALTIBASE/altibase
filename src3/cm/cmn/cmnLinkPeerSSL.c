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

#if !defined(CM_DISABLE_SSL)

extern cmnOpenssl *gOpenssl; /* BUG-45235 */

extern acp_uint32_t     gSslLibErrorCode; 
extern acp_char_t       gSslLibErrorMsg[];

typedef struct cmnLinkPeerSSL
{
    cmnLinkPeer     mLinkPeer;

    cmnLinkDescSSL  mDesc;

    acp_uint32_t    mDispatchInfo;

    cmbBlock       *mPendingBlock;
} cmnLinkPeerSSL;

static ACI_RC cmnLinkPeerDestroySslCtx(cmnLinkPeer *aLink)
{
    cmnLinkPeerSSL *sLink = (cmnLinkPeerSSL *)aLink;

    /* BUG-44547 */
    ACI_TEST_RAISE(gOpenssl == NULL, NoSslLibraryLoaded);

    /* Free the allocated SSL_CTX object */
    if( sLink->mDesc.mSslCtx != NULL )
    {
        gOpenssl->mFuncs.SSL_CTX_free( sLink->mDesc.mSslCtx );
        sLink->mDesc.mSslCtx = NULL;
    }
    else
    {
        /* SSL_CTX is null */
    }

    ACI_EXCEPTION_CONT(NoSslLibraryLoaded);

    return ACI_SUCCESS;
}

static ACI_RC cmnLinkPeerInitializeSslCtx( cmnLinkPeer *aLink,
                                           cmnLinkConnectArg *aConnectArg )
{
    cmnLinkPeerSSL *sLink = (cmnLinkPeerSSL *)aLink;

    SSL_METHOD *sMethod = NULL;
    SSL_CTX    *sSslCtx = NULL;
    acp_char_t *sCa     = aConnectArg->mSSL.mCa;
    acp_char_t *sCaPath = aConnectArg->mSSL.mCaPath;
    acp_char_t *sCert   = aConnectArg->mSSL.mCert;
    acp_char_t *sKey    = aConnectArg->mSSL.mKey;
    acp_char_t *sCipher = aConnectArg->mSSL.mCipher;
    acp_bool_t  sVerify = aConnectArg->mSSL.mVerify;

    acp_rc_t     sRet = ACI_SUCCESS;
    acp_sint32_t sSslRet = 0;

    ACI_TEST_RAISE(gOpenssl == NULL, NoSslLibrary);

    /* secure client using TLSv1 */
    sMethod = (SSL_METHOD *)gOpenssl->mFuncs.TLSv1_client_method();

    /* create new context from the method */
    sSslCtx = gOpenssl->mFuncs.SSL_CTX_new(sMethod);
    ACI_TEST_RAISE(sSslCtx == NULL, SslOperationFail );

    /* set the certificate */
    if (sCert != NULL)
    {
        sSslRet = gOpenssl->mFuncs.SSL_CTX_use_certificate_file(sSslCtx, sCert, SSL_FILETYPE_PEM);
        ACI_TEST_RAISE(sSslRet == 0, FailedToSetCertificate);
    }
    else
    {
        /* the client does not have a certificate. */
    }

    /* set the private key */
    if (sKey != NULL)
    {
        sSslRet = gOpenssl->mFuncs.SSL_CTX_use_PrivateKey_file(sSslCtx, sKey, SSL_FILETYPE_PEM);
        ACI_TEST_RAISE(sSslRet == 0, FailedToSetCertificate);
    }
    else
    {
        /* the client does not have a key */
    }

    /* verify the key */
    if ((sCert != NULL) && (sKey != NULL))
    {
        sSslRet = gOpenssl->mFuncs.SSL_CTX_check_private_key(sSslCtx);
        ACI_TEST_RAISE(sSslRet == 0, FailedToVerifyPrivateKey);
    }
    else
    {
        /* skip the key verification with the certificate */
    }

    /* set the cipher list */
    if (sCipher != NULL)
    {
        sSslRet = gOpenssl->mFuncs.SSL_CTX_set_cipher_list(sSslCtx, sCipher);
        ACI_TEST_RAISE(sSslRet == 0, SslOperationFail);
    }
    else
    {
        /* no cipher algorithm has been selected. */
    }

    /* load the trust store */
    if ((sCa != NULL) || (sCaPath != NULL))
    {
        sRet = gOpenssl->mFuncs.SSL_CTX_load_verify_locations(sSslCtx, sCa, sCaPath);
        ACI_TEST_RAISE(sRet == 0, FailedToLoadVerifyLocations);
    }
    else
    {
        /* No location for trusted CA certificates has been specified. */

        /* load the default CA from the OS */
        sRet = gOpenssl->mFuncs.SSL_CTX_set_default_verify_paths(sSslCtx);
        ACI_TEST_RAISE(sRet == 0, FailedToLoadVerifyLocations);
    }

    if (sVerify == ACP_TRUE)
    {
        gOpenssl->mFuncs.SSL_CTX_set_verify(sSslCtx, SSL_VERIFY_PEER, NULL);
    }
    else
    {
        gOpenssl->mFuncs.SSL_CTX_set_verify(sSslCtx, SSL_VERIFY_NONE, NULL);
    }

    gOpenssl->mFuncs.SSL_CTX_set_session_cache_mode(sSslCtx, SSL_SESS_CACHE_OFF);

    /* SSL_MODE_AUTO_RETRY
     * Never bother the application with retries if the transport is blocking. 
     * If a renegotiation take place during normal operation, a SSL_read() or SSL_write() would return with -1 
     * and indicate the need to retry with SSL_ERROR_WANT_READ. 
     * The flag SSL_MODE_AUTO_RETRY will cause read/write operations to only return 
     * after the handshake and successful completion. 
     *
     * The SSL_CTX_ctrl() returns the new mode bitmas after adding the argument. 
     * Therefore, the return value doesn't have to be checked at this point. */
    (void)gOpenssl->mFuncs.SSL_CTX_ctrl(sSslCtx, SSL_CTRL_MODE, SSL_MODE_AUTO_RETRY, NULL);

    sLink->mDesc.mSslCtx = sSslCtx; 

    return ACI_SUCCESS;

    ACI_EXCEPTION( FailedToVerifyPrivateKey )
    {
        ACI_SET(aciSetErrorCode(cmERR_ABORT_PRIVATE_KEY_VERIFICATION, cmnOpensslErrorMessage(gOpenssl)));
    }
    ACI_EXCEPTION( FailedToSetCertificate )
    {
        ACI_SET(aciSetErrorCode(cmERR_ABORT_INVALID_CERTIFICATE, cmnOpensslErrorMessage(gOpenssl)));
    }
    ACI_EXCEPTION( SslOperationFail )
    {    
        ACI_SET(aciSetErrorCode(cmERR_ABORT_SSL_OPERATION, cmnOpensslErrorMessage(gOpenssl)));
    }   
    ACI_EXCEPTION( FailedToLoadVerifyLocations )
    {
        ACI_SET(aciSetErrorCode(cmERR_ABORT_INVALID_VERIFY_LOCATION, cmnOpensslErrorMessage(gOpenssl)));
    }
    ACI_EXCEPTION( NoSslLibrary )
    {
        aciSetErrorCodeAndMsg(gSslLibErrorCode, gSslLibErrorMsg);
    }
    ACI_EXCEPTION_END;

    if( sSslCtx != NULL )
    {
        gOpenssl->mFuncs.SSL_CTX_free( sSslCtx );
    }

    return ACI_FAILURE;
}

ACI_RC cmnLinkPeerInitializeSSL(cmnLink *aLink)
{
    cmnLinkPeerSSL *sLink = (cmnLinkPeerSSL *)aLink;

    /* Initialize member variables */
    sLink->mDesc.mSock.mHandle = CMN_INVALID_SOCKET_HANDLE;
    sLink->mDispatchInfo       = 0;
    sLink->mPendingBlock       = NULL;
    sLink->mDesc.mSslHandle    = NULL;
    sLink->mDesc.mSslCtx       = NULL;

    return ACI_SUCCESS;
}

ACI_RC cmnLinkPeerFinalizeSSL(cmnLink *aLink)
{
    cmnLinkPeerSSL *sLink = (cmnLinkPeerSSL *)aLink;
    cmbPool        *sPool = sLink->mLinkPeer.mPool;

    /* TCP socket이 열려있으면 닫음 */
    ACI_TEST( aLink->mOp->mClose(aLink) != ACI_SUCCESS );

    /* Pending Block이 할당되어 있으면 해제 */
    if( sLink->mPendingBlock != NULL )
    {
        ACI_TEST( sPool->mOp->mFreeBlock(sPool, sLink->mPendingBlock) != ACI_SUCCESS );
    }
    else
    {
        /* No pending block exists */
    }

    /* BUG-44547 */
    ACI_TEST_RAISE(gOpenssl == NULL, NoSslLibraryLoaded);

    /* Clean up the thread's local error queue */
    gOpenssl->mFuncs.ERR_remove_state(0); 

    ACI_EXCEPTION_CONT( NoSslLibraryLoaded );

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC cmnLinkPeerCloseSSL(cmnLink *aLink)
{
    cmnLinkPeerSSL *sLink = (cmnLinkPeerSSL *)aLink;

    /* This function does not check the ssl library validation 
     * that other cmn ssl functions do due to the following reasons. 
     * - The validation can be checked with mSslHandle at this point.
     * - In case of client connections, it can be possible that 
     *   the client failed to load the ssl library at cmiInitialize()
     *   and has not gotten noticed the failure up until this point. 
     * - However, despite the failure, this function must be conducted 
     *   to free the uln link already allocated at the upper layer. 
     * - See also BUG-44547  */

    if (sLink->mDesc.mSslHandle != NULL)
    {
        gOpenssl->mFuncs.SSL_set_quiet_shutdown(sLink->mDesc.mSslHandle, 1);

        /* remove the SSL structure and free up the allocated memory */
        gOpenssl->mFuncs.SSL_free(sLink->mDesc.mSslHandle);
        sLink->mDesc.mSslHandle = NULL;
    }
    else
    {
        /* do nothing */
    }

    ACI_TEST(cmnLinkPeerDestroySslCtx((cmnLinkPeer *)aLink) != ACI_SUCCESS);

    /* TCP socket이 열려있으면 닫음 */
    if (sLink->mDesc.mSock.mHandle != CMN_INVALID_SOCKET_HANDLE)
    {
        ACI_TEST(acpSockClose(&sLink->mDesc.mSock) != ACP_RC_SUCCESS);

        sLink->mDesc.mSock.mHandle = CMN_INVALID_SOCKET_HANDLE;
    }
    else
    {
        /* do nothing */
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC cmnLinkPeerGetSockSSL(cmnLink *aLink, void **aSock)
{
    cmnLinkPeerSSL *sLink = (cmnLinkPeerSSL *)aLink;

    /* Return socket */
    *(acp_sock_t **)aSock = &sLink->mDesc.mSock;

    return ACI_SUCCESS;
}

ACI_RC cmnLinkPeerGetDispatchInfoSSL(cmnLink *aLink, void *aDispatchInfo)
{
    cmnLinkPeerSSL *sLink = (cmnLinkPeerSSL *)aLink;

    /* Return DispatcherInfo */
    *(acp_uint32_t *)aDispatchInfo = sLink->mDispatchInfo;

    return ACI_SUCCESS;
}

ACI_RC cmnLinkPeerSetDispatchInfoSSL(cmnLink *aLink, void *aDispatchInfo)
{
    cmnLinkPeerSSL *sLink = (cmnLinkPeerSSL *)aLink;

    /* Set DispatcherInfo */
    sLink->mDispatchInfo = *(acp_uint32_t *)aDispatchInfo;

    return ACI_SUCCESS;
}

ACI_RC cmnLinkPeerSetBlockingModeSSL(cmnLinkPeer *aLink, acp_bool_t aBlockingMode)
{
    cmnLinkPeerSSL *sLink = (cmnLinkPeerSSL *)aLink;

    /* Set Blocking Mode */
    ACI_TEST(cmnSockSetBlockingMode(&sLink->mDesc.mSock, aBlockingMode) != ACI_SUCCESS);

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC cmnLinkPeerGetInfoSSL(cmnLinkPeer *aLink, 
                             acp_char_t *aBuf, 
                             acp_uint32_t aBufLen,
                             cmnLinkInfoKey aKey)
{
    cmnLinkPeerSSL     *sLink = (cmnLinkPeerSSL *)aLink;
    acp_sock_len_t      sAddrLen = 0;
    acp_rc_t            sRet = ACP_RC_SUCCESS;
    acp_char_t          sErrMsg[256];

    acp_sock_addr_storage_t  sAddr;
    acp_char_t               sAddrStr[ACP_INET_IP_ADDR_MAX_LEN];
    acp_char_t               sPortStr[ACP_INET_IP_PORT_MAX_LEN];

    sErrMsg[0] = '\0';

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
            ACI_TEST_RAISE(acpSockGetName(&sLink->mDesc.mSock,
                                          (acp_sock_addr_t *)&sAddr,
                                          &sAddrLen) != ACP_RC_SUCCESS,
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
                                   sPortStr,
                                   ACI_SIZEOF(sPortStr),
                                   ACP_INET_NI_NUMERICSERV) != ACP_RC_SUCCESS,
                GetNameInfoError);
            break;

        case CMN_LINK_INFO_IMPL:
        case CMN_LINK_INFO_REMOTE_SOCKADDR:
            break;

#if defined(TCP_INFO)
        case CMN_LINK_INFO_TCP_KERNEL_STAT: /* PROJ-2625 */
            break;
#endif /* TCP_INFO */

        default:
            ACI_RAISE(UnsupportedLinkInfoKey);
            break;
    }

    switch (aKey)
    {
        case CMN_LINK_INFO_ALL:
            sRet = acpSnprintf(aBuf, aBufLen, "SSL %s:%s", sAddrStr, sPortStr);
            break;

        case CMN_LINK_INFO_IMPL:
            sRet = acpSnprintf(aBuf, aBufLen, "SSL");
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
            /*ACI_TEST_RAISE(aBufLen < (acp_uint32_t)(sLink->mDesc.mAddrLen),*/
            ACI_TEST_RAISE(aBufLen < sLink->mDesc.mAddrLen, StringTruncated);
            acpMemCpy(aBuf, &sLink->mDesc.mAddr, sLink->mDesc.mAddrLen);
            break;

#if defined(TCP_INFO)
        /* PROJ-2625 Semi-async Prefetch, Prefetch Auto-tuning */
        case CMN_LINK_INFO_TCP_KERNEL_STAT:
            sRet = acpSockGetOpt(&sLink->mDesc.mSock,
                                 SOL_TCP,
                                 TCP_INFO,
                                 aBuf,
                                 (acp_sock_len_t *)&aBufLen);

            ACI_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRet), GetSockOptError);
            break;
#endif /* TCP_INFO */

        default:
            ACI_RAISE(UnsupportedLinkInfoKey);
            break;
    }

    ACI_TEST_RAISE(sRet < 0, StringOutputError);

    ACI_TEST_RAISE((acp_uint32_t)sRet >= aBufLen, StringTruncated);

    return ACI_SUCCESS;

    ACI_EXCEPTION(StringOutputError)
    {
        ACI_SET(aciSetErrorCode(cmERR_ABORT_STRING_OUTPUT_ERROR));
    }
    ACI_EXCEPTION(StringTruncated)
    {
        ACI_SET(aciSetErrorCode(cmERR_IGNORE_STRING_TRUNCATED));
    }
    ACI_EXCEPTION(GetSockNameError)
    {
        ACI_SET(aciSetErrorCode(cmERR_ABORT_GETSOCKNAME_ERROR));
    }
    ACI_EXCEPTION(GetNameInfoError)
    {
        ACI_SET(aciSetErrorCode(cmERR_ABORT_GETNAMEINFO_ERROR));
    }
    ACI_EXCEPTION(UnsupportedLinkInfoKey)
    {
        ACI_SET(aciSetErrorCode(cmERR_ABORT_UNSUPPORTED_LINK_INFO_KEY));
    }
#if defined(TCP_INFO)
    ACI_EXCEPTION(GetSockOptError)
    {
        CMN_SET_ERRMSG_SOCK_OPT(sErrMsg, ACI_SIZEOF(sErrMsg),
                                "TCP_INFO", ACP_RC_TO_SYS_ERROR(sRet));

        ACI_SET(aciSetErrorCode(cmERR_ABORT_GETSOCKOPT_ERROR, sErrMsg));
    }
#endif /* TCP_INFO */
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC cmnLinkPeerGetDescSSL(cmnLinkPeer *aLink, void *aDesc)
{
    cmnLinkPeerSSL *sLink = (cmnLinkPeerSSL *)aLink;

    /*
     * Desc를 돌려줌
     */
    *(cmnLinkDescSSL **)aDesc = &sLink->mDesc;

    return ACI_SUCCESS;
}

/* PROJ-2625 Semi-async Prefetch, Prefetch Auto-tuning */
ACI_RC cmnLinkPeerGetSndBufSizeSSL(cmnLinkPeer *aLink, acp_sint32_t *aSndBufSize)
{
    cmnLinkPeerSSL *sLink = (cmnLinkPeerSSL *)aLink;

    ACI_TEST(cmnSockGetSndBufSize(&sLink->mDesc.mSock, aSndBufSize) != ACI_SUCCESS);

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC cmnLinkPeerSetSndBufSizeSSL(cmnLinkPeer *aLink, acp_sint32_t aSndBufSize)
{
    cmnLinkPeerSSL *sLink = (cmnLinkPeerSSL *)aLink;

    ACI_TEST(cmnSockSetSndBufSize(&sLink->mDesc.mSock, aSndBufSize) != ACI_SUCCESS);

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC cmnLinkPeerGetRcvBufSizeSSL(cmnLinkPeer *aLink, acp_sint32_t *aRcvBufSize)
{
    cmnLinkPeerSSL *sLink = (cmnLinkPeerSSL *)aLink;

    ACI_TEST(cmnSockGetRcvBufSize(&sLink->mDesc.mSock, aRcvBufSize) != ACI_SUCCESS);

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC cmnLinkPeerSetRcvBufSizeSSL(cmnLinkPeer *aLink, acp_sint32_t aRcvBufSize)
{
    cmnLinkPeerSSL *sLink = (cmnLinkPeerSSL *)aLink;

    ACI_TEST(cmnSockSetRcvBufSize(&sLink->mDesc.mSock, aRcvBufSize) != ACI_SUCCESS);

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

static ACI_RC cmnLinkPeerConnByIP(cmnLinkPeer*          aLink,
                                  acp_time_t            aTimeout,
                                  acp_sint32_t          aOption,
                                  acp_inet_addr_info_t* aAddr,
                                  acp_inet_addr_info_t* aBindAddr);

static ACI_RC cmnLinkPeerConnByName(cmnLinkPeer*          aLink,
                                    cmnLinkConnectArg*    aConnectArg,
                                    acp_time_t            aTimeout,
                                    acp_sint32_t          aOption,
                                    acp_inet_addr_info_t* aAddr,
                                    acp_inet_addr_info_t* aBindAddr);

ACI_RC cmnLinkPeerConnectSSL(cmnLinkPeer       *aLink,
                             cmnLinkConnectArg *aConnectArg,
                             acp_time_t         aTimeout,
                             acp_sint32_t       aOption)
{
    cmnLinkPeerSSL        *sLink = (cmnLinkPeerSSL *)aLink;

    /* BUG-44530 SSL에서 ALTIBASE_SOCK_BIND_ADDR 지원 */
    acp_inet_addr_info_t  *sBindAddr   = NULL;
    acp_inet_addr_info_t  *sAddr       = NULL;
    acp_bool_t             sAddrIsIP   = ACP_FALSE;

    acp_rc_t               sRet    = ACP_RC_SUCCESS;
    acp_sint32_t           sSslRet = 0;

    ACI_TEST_RAISE(gOpenssl == NULL, NoSslLibrary);

    /* *********************************************************
     * proj-1538 ipv6: use getaddrinfo()
     * *********************************************************/
    /* BUG-44530 SSL에서 ALTIBASE_SOCK_BIND_ADDR 지원 */
    if (aConnectArg->mSSL.mBindAddr != NULL)
    {
        ACI_TEST(cmnGetAddrInfo(&sBindAddr,
                                NULL,
                                aConnectArg->mSSL.mBindAddr,
                                0) != ACI_SUCCESS);
    }

    ACI_TEST(cmnGetAddrInfo(&sAddr,
                            &sAddrIsIP,
                            aConnectArg->mSSL.mAddr,
                            aConnectArg->mSSL.mPort) != ACI_SUCCESS);

    /* in case that a user inputs the IP address directly */
    if (sAddrIsIP == ACP_TRUE)
    {
        ACI_TEST(cmnLinkPeerConnByIP(aLink,
                                     aTimeout,
                                     aOption,
                                     sAddr,
                                     sBindAddr) != ACI_SUCCESS);
    }
    /* in case that a user inputs the hostname instead of IP */
    else
    {
        ACI_TEST(cmnLinkPeerConnByName(aLink,
                                       aConnectArg,
                                       aTimeout,
                                       aOption,
                                       sAddr,
                                       sBindAddr) != ACI_SUCCESS);
    }

    /* SSL/TLS */
    sRet = cmnLinkPeerInitializeSslCtx(aLink, aConnectArg);
    ACI_TEST(sRet != ACI_SUCCESS);

    sLink->mDesc.mSslHandle = gOpenssl->mFuncs.SSL_new(sLink->mDesc.mSslCtx);
    ACI_TEST_RAISE(sLink->mDesc.mSslHandle == NULL, SslOperationFail);

    sSslRet = gOpenssl->mFuncs.SSL_set_fd(sLink->mDesc.mSslHandle, sLink->mDesc.mSock.mHandle);
    ACI_TEST_RAISE(sSslRet != 1, SslOperationFail); /* 0: failed, 1: succeeded */

    sSslRet = gOpenssl->mFuncs.SSL_connect(sLink->mDesc.mSslHandle);
    ACI_TEST_RAISE(sSslRet != 1, SslConnectionFail);

    if (sAddr != NULL)
    {
        acpInetFreeAddrInfo(sAddr);
        sAddr = NULL;
    }
    /* BUG-44530 SSL에서 ALTIBASE_SOCK_BIND_ADDR 지원 */
    if (sBindAddr != NULL)
    {
        acpInetFreeAddrInfo(sBindAddr);
        sBindAddr = NULL;
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION( SslConnectionFail )
    {
        ACI_SET(aciSetErrorCode(cmERR_ABORT_SSL_CONNECT, cmnOpensslErrorMessage(gOpenssl)));
    }
    ACI_EXCEPTION( SslOperationFail )
    {
        ACI_SET(aciSetErrorCode(cmERR_ABORT_SSL_OPERATION, cmnOpensslErrorMessage(gOpenssl)));
    }
    ACI_EXCEPTION( NoSslLibrary )
    {
        aciSetErrorCodeAndMsg(gSslLibErrorCode, gSslLibErrorMsg);
    }
    ACI_EXCEPTION_END;

    if (sAddr != NULL)
    {
        acpInetFreeAddrInfo(sAddr);
        sAddr = NULL;
    }
    /* BUG-44530 SSL에서 ALTIBASE_SOCK_BIND_ADDR 지원 */
    if (sBindAddr != NULL)
    {
        acpInetFreeAddrInfo(sBindAddr);
        sBindAddr = NULL;
    }

    return ACI_FAILURE;
}

/* proj-1538 ipv6 */
/* in case that a user inputs the IP address directly */
static ACI_RC cmnLinkPeerConnByIP(cmnLinkPeer*          aLink,
                                  acp_time_t            aTimeout,
                                  acp_sint32_t          aOption,
                                  acp_inet_addr_info_t* aAddr,
                                  acp_inet_addr_info_t* aBindAddr)
{
    cmnLinkPeerSSL      *sLink = (cmnLinkPeerSSL *)aLink;
    acp_rc_t             sRet  = ACP_RC_SUCCESS;


    sRet = acpSockOpen(&sLink->mDesc.mSock,
                       aAddr->ai_family,
                       ACP_SOCK_STREAM,
                       0);
    ACI_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRet), SocketError);

    /* BUG-44530 SSL에서 ALTIBASE_SOCK_BIND_ADDR 지원 */
    if (aBindAddr != NULL)
    {
        sRet = acpSockBind(&sLink->mDesc.mSock,
                           (acp_sock_addr_t*)aBindAddr->ai_addr,
                           aBindAddr->ai_addrlen,
                           ACP_TRUE);
        ACI_TEST_RAISE(sRet != ACP_RC_SUCCESS, BindError);
    }

    sRet = acpSockTimedConnect(&sLink->mDesc.mSock,
                               (acp_sock_addr_t *)aAddr->ai_addr,
                               aAddr->ai_addrlen,
                               aTimeout);

    ACI_TEST_RAISE(sRet != ACP_RC_SUCCESS, ConnectError);

    /* save fd and IP address into link info */
    acpMemCpy(&sLink->mDesc.mAddr, aAddr->ai_addr, aAddr->ai_addrlen);
    sLink->mDesc.mAddrLen = aAddr->ai_addrlen;

    /* socket 초기화 */
    ACI_TEST(aLink->mPeerOp->mSetOptions(aLink, aOption) != ACI_SUCCESS);

    return ACI_SUCCESS;

    ACI_EXCEPTION(SocketError)
    {
        ACI_SET(aciSetErrorCode(cmERR_ABORT_SOCKET_OPEN_ERROR, sRet));
    }
    ACI_EXCEPTION(BindError)
    {
        ACI_SET(aciSetErrorCode(cmERR_ABORT_BIND_ERROR, sRet));
    }
    ACI_EXCEPTION(ConnectError)
    {
        /* bug-30835: support link-local address with zone index.
         * On linux, link-local addr without zone index makes EINVAL.
         *  .EINVAL   : Invalid argument to connect()
         *  .others   : Client unable to establish connection.
         * caution: acpSockTimedConnect does not change errno.
         *  Instead, it returns errno directly. */
        if (ACP_RC_IS_EINVAL(sRet))
        {
            ACI_SET(aciSetErrorCode(cmERR_ABORT_CONNECT_INVALIDARG));
        }
        else
        {
            ACI_SET(aciSetErrorCode(cmERR_ABORT_CONNECT_ERROR, sRet));
        }
    }
    ACI_EXCEPTION_END;

    // BUG-24170 [CM] cmiConnect 실패 시, cmiConnect 내에서 close
    if (sLink->mDesc.mSock.mHandle != CMN_INVALID_SOCKET_HANDLE)
    {
        (void)acpSockClose(&sLink->mDesc.mSock);
        sLink->mDesc.mSock.mHandle = CMN_INVALID_SOCKET_HANDLE;
    }
    else
    {
        /* The socket handle is CMN_INVALID_SOCKET_HANDLE */
    }

    return ACI_FAILURE;
}

/* proj-1538 ipv6 */
/* in case that a user inputs the hostname instead of IP */
static ACI_RC cmnLinkPeerConnByName(cmnLinkPeer*          aLink,
                                    cmnLinkConnectArg*    aConnectArg,
                                    acp_time_t            aTimeout,
                                    acp_sint32_t          aOption,
                                    acp_inet_addr_info_t* aAddr,
                                    acp_inet_addr_info_t* aBindAddr)
{
    cmnLinkPeerSSL        *sLink = (cmnLinkPeerSSL *)aLink;
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
    acp_rc_t               sRet    = ACP_RC_SUCCESS;


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
        else
        {
            /* do nothing */
        }
    }

    /* if prefer IPv4, then order is v4 -> v6 */
    if (aConnectArg->mSSL.mPreferIPv6 == 0)
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

        sRet = acpSockOpen(&sLink->mDesc.mSock,
                           sAddr->ai_family,
                           ACP_SOCK_STREAM,
                           0);
        ACI_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRet), SocketError);

        /* BUG-44530 SSL에서 ALTIBASE_SOCK_BIND_ADDR 지원 */
        if (aBindAddr != NULL)
        {
            sRet = acpSockBind(&sLink->mDesc.mSock,
                               (acp_sock_addr_t*)aBindAddr->ai_addr,
                               aBindAddr->ai_addrlen,
                               ACP_TRUE);
            ACI_TEST_RAISE(sRet != ACP_RC_SUCCESS, BindError);
        }

        sRet = acpSockTimedConnect(&sLink->mDesc.mSock,
                                   (acp_sock_addr_t *)sAddr->ai_addr,
                                   sAddr->ai_addrlen,
                                   aTimeout);
        if (ACP_RC_IS_SUCCESS(sRet))
        {
            break;
        }
        else
        {   
            /* sRet is not ACP_SUCCESS */
        }

        if (sLink->mDesc.mSock.mHandle != CMN_INVALID_SOCKET_HANDLE)
        {
            (void)acpSockClose(&sLink->mDesc.mSock);
            sLink->mDesc.mSock.mHandle = CMN_INVALID_SOCKET_HANDLE;
        }
        else
        {
            /* The socket handle is invalid. */
        }
    }

    ACI_TEST_RAISE((sRet != ACP_RC_SUCCESS) || (sTryCnt == 0), ConnectError);

    /* save fd and IP address into link info */
    acpMemCpy(&sLink->mDesc.mAddr, sAddr->ai_addr, sAddr->ai_addrlen);
    sLink->mDesc.mAddrLen = sAddr->ai_addrlen;

    /* socket 초기화 */
    ACI_TEST(aLink->mPeerOp->mSetOptions(aLink, aOption) != ACI_SUCCESS);

    return ACI_SUCCESS;

    ACI_EXCEPTION(SocketError)
    {
        ACI_SET(aciSetErrorCode(cmERR_ABORT_SOCKET_OPEN_ERROR, sRet));
    }
    ACI_EXCEPTION(BindError)
    {
        ACI_SET(aciSetErrorCode(cmERR_ABORT_BIND_ERROR, sRet));
    }
    ACI_EXCEPTION(ConnectError)
    {
        /* bug-30835: support link-local address with zone index.
         * On linux, link-local addr without zone index makes EINVAL. */
        if (ACP_RC_IS_EINVAL(sRet))
        {
            ACI_SET(aciSetErrorCode(cmERR_ABORT_CONNECT_INVALIDARG));
        }
        else
        {
            ACI_SET(aciSetErrorCode(cmERR_ABORT_CONNECT_ERROR, sRet));
        }
    }
    ACI_EXCEPTION_END;

    /* BUG-24170 [CM] cmiConnect 실패 시, cmiConnect 내에서 close */
    if (sLink->mDesc.mSock.mHandle != CMN_INVALID_SOCKET_HANDLE)
    {
        (void)acpSockClose(&sLink->mDesc.mSock);
        sLink->mDesc.mSock.mHandle = CMN_INVALID_SOCKET_HANDLE;
    }
    else
    {
        /* The socket handle is invalid */
    }

    return ACI_FAILURE;
}

ACI_RC cmnLinkPeerAllocChannelSSL(cmnLinkPeer *aLink, acp_sint32_t *aChannelID)
{
    ACP_UNUSED(aLink);
    ACP_UNUSED(aChannelID);

    return ACI_SUCCESS;
}

ACI_RC cmnLinkPeerHandshakeSSL(cmnLinkPeer *aLink)
{
    ACP_UNUSED(aLink);

    return ACI_SUCCESS;
}

ACI_RC cmnLinkPeerSetOptionsSSL(cmnLinkPeer *aLink, acp_sint32_t aOption)
{
    cmnLinkPeerSSL *sLink = (cmnLinkPeerSSL *)aLink;
    acp_sint32_t    sOption  = 0;
    struct linger   sLingerOption;

    /*
     * SO_KEEPALIVE 세팅
     */
    sOption = 1;

    (void)acpSockSetOpt(&sLink->mDesc.mSock,
                        SOL_SOCKET,
                        SO_KEEPALIVE,
                        (void *)&sOption,
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

        (void)acpSockSetOpt(&sLink->mDesc.mSock,
                            SOL_SOCKET,
                            SO_LINGER,
                            (void *)&sLingerOption,
                            ACI_SIZEOF(sLingerOption));
    }
    else if (aOption == SO_REUSEADDR)
    {
        /*
         * SO_REUSEADDR 세팅
         */
        sOption = 1;

        (void)acpSockSetOpt(&sLink->mDesc.mSock,
                            SOL_SOCKET,
                            SO_REUSEADDR,
                            (void *)&sOption,
                            ACI_SIZEOF(sOption));
    }
    else
    {
        /* do nothing */
    }

    /*
     * TCP_NODELAY 세팅
     */
    sOption = 1;

    (void)acpSockSetOpt(&sLink->mDesc.mSock,
                        IPPROTO_TCP,
                        TCP_NODELAY,
                        (acp_sint8_t *)&sOption,
                        ACI_SIZEOF(sOption));

    /* BUG-22028
     * SO_SNDBUF 세팅 (대역폭 * 지연율) * 2
     */
    sOption = CMB_BLOCK_DEFAULT_SIZE * 2;

    (void)acpSockSetOpt(&sLink->mDesc.mSock,
                        SOL_SOCKET,
                        SO_SNDBUF,
                        (acp_sint8_t *)&sOption,
                        ACI_SIZEOF(sOption));

    /*
     * BUG-22028
     * SO_RCVBUF 세팅 (대역폭 * 지연율) * 2
     */
    sOption = CMB_BLOCK_DEFAULT_SIZE * 2;

    (void)acpSockSetOpt(&sLink->mDesc.mSock,
                        SOL_SOCKET,
                        SO_RCVBUF,
                        (acp_sint8_t *)&sOption,
                        ACI_SIZEOF(sOption));

    return ACI_SUCCESS;
}

ACI_RC cmnLinkPeerShutdownSSL(cmnLinkPeer    *aLink,
                              cmnDirection    aDirection,
                              cmnShutdownMode aMode)
{
    cmnLinkPeerSSL *sLink = (cmnLinkPeerSSL *)aLink;
    acp_rc_t        sRet = ACP_RC_SUCCESS;

    ACP_UNUSED(aMode);

    /* This function does not check the ssl library validation 
     * that other cmn ssl functions do due to the following reasons. 
     * - The validation can be checked with mSslHandle at this point.
     * - In case of client connections, it can be possible that 
     *   the client failed to load the ssl library at cmiInitialize()
     *   and has not gotten noticed the failure up until this point. 
     * - However, despite the failure, this function must be conducted 
     *   to free the uln link already allocated at the upper layer. 
     * - See also BUG-44547  */

    if(sLink->mDesc.mSslHandle != NULL)
    {
        /* SSL shutdown */

        /* SSL_shutdown() shuts down an active SSL/TLS connection.
         * It sends a 'close notify' shutdown alert to the peer. */
        sRet = gOpenssl->mFuncs.SSL_shutdown(sLink->mDesc.mSslHandle);
        if (sRet == 0) 
        {
            /* BUG-39922 */
            gOpenssl->mFuncs.SSL_set_quiet_shutdown(sLink->mDesc.mSslHandle, 1);
            /* When the 'quiet shutdown' is enabled, 
             * SSL_shutdown() will always succeed and return 1. */
            sRet = gOpenssl->mFuncs.SSL_shutdown(sLink->mDesc.mSslHandle);
        }
        else
        {
            /* The SSL connection has been shut down successfully. */
        }

        ACI_TEST_RAISE(sRet != 1, SSLShutdownError);
    }

    /* TCP socket shutdown */
    sRet = acpSockShutdown(&sLink->mDesc.mSock, aDirection);

    switch (sRet)
    {
        case ACP_RC_SUCCESS:
        case ACP_RC_ENOTCONN:
            break;
        default:
            ACI_RAISE(ShutdownError);
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION(SSLShutdownError)
    {
        ACI_SET(aciSetErrorCode(cmERR_ABORT_SSL_SHUTDOWN, cmnOpensslErrorMessage(gOpenssl)));
    }
    ACI_EXCEPTION(ShutdownError)
    {
        ACI_SET(aciSetErrorCode(cmERR_ABORT_SOCKET_SHUTDOWN_FAILED));
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}


static ACI_RC cmnSslSockRecv(cmnLinkPeer    *aLink,
                             cmbBlock       *aBlock,
                             acp_uint16_t    aSize,
                             acp_time_t      aTimeout)
{
    cmnLinkPeerSSL *sLink = (cmnLinkPeerSSL *)aLink;

    acp_ssize_t   sSize = 0;
    acp_sint32_t sSslError = SSL_ERROR_NONE;

    ACI_TEST_RAISE(gOpenssl == NULL, NoSslLibrary);

    /*  
     * aSize 이상 aBlock으로 데이터 읽음
     */
    while (aBlock->mDataSize < aSize)
    {
        /*
         * Dispatcher를 이용하여 Timeout 만큼 대기
         */
        if (aTimeout != ACP_TIME_INFINITE)
        {
            ACI_TEST(cmnDispatcherWaitLink((cmnLink *)sLink,
                                           CMN_DIRECTION_RD,
                                           aTimeout) != ACI_SUCCESS);
        }
        else
        {
            /* aTimeout is ACP_TIME_INFINITE */
        }

        /*
         * Socket으로부터 읽음
         */
        sSize = gOpenssl->mFuncs.SSL_read(sLink->mDesc.mSslHandle,
                                          aBlock->mData + aBlock->mDataSize,
                                          aBlock->mBlockSize - aBlock->mDataSize);

        if (sSize <= 0)
        {
            sSslError = gOpenssl->mFuncs.SSL_get_error(sLink->mDesc.mSslHandle, sSize);

            switch (sSslError)
            {
                case SSL_ERROR_WANT_READ:
                    ACI_TEST(cmnDispatcherWaitLink((cmnLink *)sLink,
                                                   CMN_DIRECTION_RD,
                                                   aTimeout) != ACI_SUCCESS);
                    break;

                case SSL_ERROR_WANT_WRITE:
                    ACI_TEST(cmnDispatcherWaitLink((cmnLink *)sLink,
                                                   CMN_DIRECTION_WR,
                                                   aTimeout) != ACI_SUCCESS);
                    break;

                case SSL_ERROR_SYSCALL:
                    if (sSize == 0)
                    {
                        ACI_RAISE(ConnectionClosed);
                    }
                    else
                    {
                        ACI_RAISE(SslReadError);
                    }
                    break;

                default:
                    ACI_RAISE(SslReadError);
                    break;
            }
        }
        else
        {
            aBlock->mDataSize += sSize;
        }

    }

    return ACI_SUCCESS;

    ACI_EXCEPTION(ConnectionClosed)
    {
        ACI_SET(aciSetErrorCode(cmERR_ABORT_CONNECTION_CLOSED));
    }
    ACI_EXCEPTION(SslReadError)
    {
        ACI_SET(aciSetErrorCode(cmERR_ABORT_SSL_READ, cmnOpensslErrorMessage(gOpenssl)));
    }
    ACI_EXCEPTION(NoSslLibrary)
    {
        aciSetErrorCodeAndMsg(gSslLibErrorCode, gSslLibErrorMsg);
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

static ACI_RC cmnSslSockSend(cmnLinkPeer    *aLink,
                             cmbBlock       *aBlock,
                             acp_time_t      aTimeout)
{
    cmnLinkPeerSSL *sLink = (cmnLinkPeerSSL *)aLink;

    acp_size_t   sSize = 0;
    acp_sint32_t sSslError = SSL_ERROR_NONE;

    ACI_TEST_RAISE(gOpenssl == NULL, NoSslLibrary);

    if (aBlock->mCursor == aBlock->mDataSize)
    {
        aBlock->mCursor = 0;
    }
    else
    {
        /* The block is not empty */
    }

    while (aBlock->mCursor < aBlock->mDataSize)
    {
        /*
         * Dispatcher를 이용하여 Timeout 만큼 대기
         */
        if (aTimeout != ACP_TIME_INFINITE)
        {
            ACI_TEST(cmnDispatcherWaitLink((cmnLink *)sLink,
                                           CMN_DIRECTION_WR,
                                           aTimeout) != ACI_SUCCESS);
        }
        else
        {
            /* aTimeout is ACP_TIME_INFINITE */
        }

        sSize = gOpenssl->mFuncs.SSL_write(sLink->mDesc.mSslHandle,
                                           aBlock->mData + aBlock->mCursor,
                                           aBlock->mDataSize - aBlock->mCursor);

        if (sSize <= 0)
        {
            sSslError = gOpenssl->mFuncs.SSL_get_error(sLink->mDesc.mSslHandle, sSize);

            switch (sSslError)
            {
                case SSL_ERROR_WANT_WRITE:
                    ACI_RAISE(Retry);
                    break;

                case SSL_ERROR_WANT_READ:
                    ACI_RAISE(Retry);
                    break;

                default:
                    ACI_RAISE(SslWriteError);
                    break;
            }
        }
        else
        {
            aBlock->mCursor += sSize;
        }

    }

    return ACI_SUCCESS;

    ACI_EXCEPTION(Retry)
    {
        ACI_SET(aciSetErrorCode(cmERR_RETRY_SOCKET_OPERATION_WOULD_BLOCK));
    }
    ACI_EXCEPTION(SslWriteError)
    {
        ACI_SET(aciSetErrorCode(cmERR_ABORT_SSL_WRITE, cmnOpensslErrorMessage(gOpenssl)));
    }
    ACI_EXCEPTION(NoSslLibrary)
    {
        aciSetErrorCodeAndMsg(gSslLibErrorCode, gSslLibErrorMsg);
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC cmnLinkPeerRecvSSL(cmnLinkPeer *aLink, 
                          cmbBlock **aBlock, 
                          cmpHeader *aHeader, 
                          acp_time_t aTimeout)
{
    cmnLinkPeerSSL *sLink  = (cmnLinkPeerSSL *)aLink;
    cmbPool        *sPool  = aLink->mPool;
    cmbBlock       *sBlock = NULL;
    cmpHeader       sHeader;
    acp_uint16_t    sPacketSize = 0;
    cmpPacketType   sPacketType = aLink->mLink.mPacketType;

    /*
     * Pending Block있으면 사용 그렇지 않으면 Block 할당
     */
    /* proj_2160 cm_type removal */
    /* A7 or CMP_PACKET_TYPE_UNKNOWN: block already allocated. */
    if (sPacketType != CMP_PACKET_TYPE_A5)
    {
        sBlock = *aBlock;
        if (sLink->mPendingBlock != NULL)
        {
            acpMemCpy(sBlock->mData,
                      sLink->mPendingBlock->mData,
                      sLink->mPendingBlock->mDataSize);
            sBlock->mDataSize = sLink->mPendingBlock->mDataSize;

            ACI_TEST(sPool->mOp->mFreeBlock(sPool, sLink->mPendingBlock)
                     != ACI_SUCCESS);
            sLink->mPendingBlock = NULL;
        }
        else
        {
            /* The packet type is A5 */
        }
    }
    // A5: block will be allocated here.
    else
    {
        if (sLink->mPendingBlock != NULL)
        {
            sBlock               = sLink->mPendingBlock;
            sLink->mPendingBlock = NULL;
        }
        else
        {
            ACI_TEST(sPool->mOp->mAllocBlock(sPool, &sBlock) != ACI_SUCCESS);
        }
    }

    /*
     * Protocol Header Size 크기 이상 읽음
     */
    ACI_TEST_RAISE(cmnSslSockRecv(aLink,
                                  sBlock,
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
    ACI_TEST_RAISE(cmnSslSockRecv(aLink,
                                  sBlock,
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
    else
    {
        /* do nothing */
    }

    /*
     * Block과 Header를 돌려줌
     */
    /* proj_2160 cm_type removal
     *  Do not use mLink.mPacketType. instead, use sPacketType.
     * cuz, mLink's value could be changed in cmpHeaderRead().
     * and, this if-stmt shows explicitly that
     * it needs to return block ptr in only A5. */
    if (sPacketType == CMP_PACKET_TYPE_A5)
    {
        *aBlock  = sBlock;
    }
    else
    {
        /* The packet type is not A5. */
    }

    *aHeader = sHeader;

    return ACI_SUCCESS;
    
    /* BUG-39127  If a network timeout occurs during replication sync 
     * then it fails by communication protocol error. */
    ACI_EXCEPTION( SockRecvError )
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

ACI_RC cmnLinkPeerReqCompleteSSL(cmnLinkPeer *aLink)
{
    ACP_UNUSED(aLink);

    return ACI_SUCCESS;
}

ACI_RC cmnLinkPeerResCompleteSSL(cmnLinkPeer *aLink)
{
    ACP_UNUSED(aLink);

    return ACI_SUCCESS;
}

ACI_RC cmnLinkPeerSendSSL(cmnLinkPeer *aLink, 
                          cmbBlock *aBlock)
{
    /*
     * Block 전송
     */
    ACI_TEST(cmnSslSockSend(aLink, 
                            aBlock,
                            ACP_TIME_INFINITE) != ACI_SUCCESS);

    /* proj_2160 cm_type removal
     * A7 use static-block for a session */
    if (aLink->mLink.mPacketType == CMP_PACKET_TYPE_A5)
    {
        ACI_TEST(aLink->mPool->mOp->mFreeBlock(aLink->mPool, aBlock) != ACI_SUCCESS);
    }
    else
    {
        /* The packet type is not A5. */
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC cmnLinkPeerCheckSSL(cmnLinkPeer *aLink, 
                           acp_bool_t *aIsClosed)
{
    cmnLinkPeerSSL *sLink = (cmnLinkPeerSSL *)aLink;

    return cmnSockCheck((cmnLink *)aLink, &sLink->mDesc.mSock, aIsClosed);
}

acp_bool_t cmnLinkPeerHasPendingRequestSSL(cmnLinkPeer *aLink)
{
    cmnLinkPeerSSL *sLink = (cmnLinkPeerSSL *)aLink;

    return (sLink->mPendingBlock != NULL) ? ACP_TRUE : ACP_FALSE;
}

ACI_RC cmnLinkPeerAllocBlockSSL(cmnLinkPeer *aLink, 
                                cmbBlock **aBlock)
{
    cmbBlock *sBlock = NULL;

    ACI_TEST(aLink->mPool->mOp->mAllocBlock(aLink->mPool, &sBlock) != ACI_SUCCESS);

    /*
     * Write Block 초기화
     */
    sBlock->mDataSize = CMP_HEADER_SIZE;
    sBlock->mCursor   = CMP_HEADER_SIZE;

    /*
     * Write Block을 돌려줌
     */
    *aBlock = sBlock;

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC cmnLinkPeerFreeBlockSSL(cmnLinkPeer *aLink, 
                               cmbBlock *aBlock)
{
    /*
     * Block 해제
     */
    ACI_TEST(aLink->mPool->mOp->mFreeBlock(aLink->mPool, aBlock) != ACI_SUCCESS);

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}


struct cmnLinkOP gCmnLinkPeerOpSSLClient =
{
    "SSL-PEER",

    cmnLinkPeerInitializeSSL,
    cmnLinkPeerFinalizeSSL,

    cmnLinkPeerCloseSSL,

    cmnLinkPeerGetSockSSL,

    cmnLinkPeerGetDispatchInfoSSL,
    cmnLinkPeerSetDispatchInfoSSL
};

struct cmnLinkPeerOP gCmnLinkPeerPeerOpSSLClient =
{
    cmnLinkPeerSetBlockingModeSSL,

    cmnLinkPeerGetInfoSSL,
    cmnLinkPeerGetDescSSL,

    cmnLinkPeerConnectSSL,
    cmnLinkPeerSetOptionsSSL,

    cmnLinkPeerAllocChannelSSL,
    cmnLinkPeerHandshakeSSL,

    cmnLinkPeerShutdownSSL,

    cmnLinkPeerRecvSSL,
    cmnLinkPeerSendSSL,

    cmnLinkPeerReqCompleteSSL,
    cmnLinkPeerResCompleteSSL,

    cmnLinkPeerCheckSSL,
    cmnLinkPeerHasPendingRequestSSL,

    cmnLinkPeerAllocBlockSSL,
    cmnLinkPeerFreeBlockSSL,

    /* TASK-5894 Permit sysdba via IPC */
    NULL,

    /* PROJ-2474 SSL/TLS */
    NULL,
    NULL,
    NULL,

    /* PROJ-2625 Semi-async Prefetch, Prefetch Auto-tuning */
    cmnLinkPeerGetSndBufSizeSSL,
    cmnLinkPeerSetSndBufSizeSSL,
    cmnLinkPeerGetRcvBufSizeSSL,
    cmnLinkPeerSetRcvBufSizeSSL
};


ACI_RC cmnLinkPeerMapSSL(cmnLink *aLink)
{
    cmnLinkPeer *sLink = (cmnLinkPeer *)aLink;

    /*
     * Link 검사
     */
    ACE_ASSERT(aLink->mType == CMN_LINK_TYPE_PEER_SERVER ||
               aLink->mType == CMN_LINK_TYPE_PEER_CLIENT);
    ACE_ASSERT(aLink->mImpl == CMN_LINK_IMPL_SSL);

    /*
     * Shared Pool 획득
     */
    ACI_TEST(cmbPoolGetSharedPool(&sLink->mPool, CMB_POOL_IMPL_LOCAL) != ACI_SUCCESS);

    /*
     * 함수 포인터 세팅
     */
    aLink->mOp     = &gCmnLinkPeerOpSSLClient;
    sLink->mPeerOp = &gCmnLinkPeerPeerOpSSLClient;

    /*
     * 멤버 초기화
     */
    sLink->mUserPtr    = NULL;

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

acp_uint32_t cmnLinkPeerSizeSSL()
{
    return ACI_SIZEOF(cmnLinkPeerSSL);
}

#endif /* CM_DISABLE_SSL */
