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

#include <cmAll.h>
#include <cmnOpenssl.h>

#if !defined(CM_DISABLE_SSL)

typedef struct cmnLinkPeerSSL
{
    cmnLinkPeer     mLinkPeer;

    cmnLinkDescSSL  mDesc;

    UInt            mDispatchInfo;

    cmbBlock       *mPendingBlock;
} cmnLinkPeerSSL;


static IDE_RC cmnLinkPeerInitializeSslCtx( SSL_CTX **aSslCtx )
{
    SSL_METHOD *sMethod = NULL;

    SChar      *sCa     = cmuProperty::getSslCa();
    SChar      *sCaPath = cmuProperty::getSslCaPath();
    SChar      *sCert   = cmuProperty::getSslCert();
    SChar      *sKey    = cmuProperty::getSslKey();
    UInt        sVerify = cmuProperty::getSslVerifyPeerCert();
    SSL_CTX    *sSslCtx = NULL;

    SInt sRet = 0;

    /* secure client using TLSv1 */
    sMethod = (SSL_METHOD *)cmnOpenssl::mFuncs.TLSv1_client_method(); 

    /* create a new context from the method */
    sSslCtx = cmnOpenssl::mFuncs.SSL_CTX_new(sMethod); 
    IDE_TEST_RAISE(sSslCtx == NULL, FailedToCreateSslCtx);

    if (sCert != NULL)
    {
        /* set the local certificate from CertFile */
        sRet = cmnOpenssl::mFuncs.SSL_CTX_use_certificate_file( sSslCtx, sCert, SSL_FILETYPE_PEM );
        IDE_TEST_RAISE(sRet == 0, FailedToSetCertificate);
    }
    else
    {
        /* the client does not have a certificate. */
    }

    if (sKey != NULL)
    {
        /* set the private key from KeyFile. */
        sRet = cmnOpenssl::mFuncs.SSL_CTX_use_PrivateKey_file(sSslCtx, sKey, SSL_FILETYPE_PEM);
        IDE_TEST_RAISE(sRet == 0, FailedToSetPrivateKey);
    }
    else
    {
        /* the client doesn not have a key */
    }

    /* verify the private key */
    sRet = cmnOpenssl::mFuncs.SSL_CTX_check_private_key(sSslCtx);
    IDE_TEST_RAISE(sRet == 0, FailedToVerifyPrivateKey);

    if ((sCa != NULL) || (sCaPath != NULL))
    {
        sRet = cmnOpenssl::mFuncs.SSL_CTX_load_verify_locations(sSslCtx, sCa, sCaPath);
        IDE_TEST_RAISE(sRet == 0, FailedToLoadVerifyLocations);
    }
    else
    {
        /* No location for trusted CA certificates has been specified. */

        /* load the default CA from the OS */
        sRet = cmnOpenssl::mFuncs.SSL_CTX_set_default_verify_paths(sSslCtx);
        IDE_TEST_RAISE(sRet == 0, FailedToLoadVerifyLocations);
    }

    if (sVerify == ID_TRUE)
    {
        cmnOpenssl::mFuncs.SSL_CTX_set_verify(sSslCtx, SSL_VERIFY_PEER, NULL);
    }
    else
    {
        cmnOpenssl::mFuncs.SSL_CTX_set_verify(sSslCtx, SSL_VERIFY_NONE, NULL);
    }

    cmnOpenssl::mFuncs.SSL_CTX_set_session_cache_mode(sSslCtx, SSL_SESS_CACHE_OFF);

    /* SSL_MODE_AUTO_RETRY
     * Never bother the application with retries if the transport is blocking. 
     * If a renegotiation take place during normal operation, a SSL_read() or SSL_write() would return with -1 
     * and indicate the need to retry with SSL_ERROR_WANT_READ. 
     * The flag SSL_MODE_AUTO_RETRY will cause read/write operations to only return 
     * after the handshake and successful completion. 
     *
     * The SSL_CTX_ctrl() returns the new mode bitmas after adding the argument. 
     * Therefore, the return value doesn't have to be checked at this point. */
    (void)cmnOpenssl::mFuncs.SSL_CTX_ctrl(sSslCtx, SSL_CTRL_MODE, SSL_MODE_AUTO_RETRY, NULL);

    *aSslCtx = sSslCtx;

    return IDE_SUCCESS;

    IDE_EXCEPTION( FailedToCreateSslCtx )
    {   
        IDE_SET(ideSetErrorCode(cmERR_ABORT_SSL_OPERATION, cmnOpenssl::getSslErrorMessage()));
    } 
    IDE_EXCEPTION( FailedToLoadVerifyLocations )
    {   
        IDE_SET(ideSetErrorCode(cmERR_ABORT_INVALID_VERIFY_LOCATION, cmnOpenssl::getSslErrorMessage()));
    }
    IDE_EXCEPTION( FailedToSetCertificate )
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_INVALID_CERTIFICATE, cmnOpenssl::getSslErrorMessage()));
    }
    IDE_EXCEPTION( FailedToSetPrivateKey )
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_INVALID_PRIVATE_KEY, cmnOpenssl::getSslErrorMessage()));
    }
    IDE_EXCEPTION( FailedToVerifyPrivateKey )
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_PRIVATE_KEY_VERIFICATION, cmnOpenssl::getSslErrorMessage()));
    }
    IDE_EXCEPTION_END;
    {
        if(sSslCtx != NULL) 
        {
            cmnOpenssl::mFuncs.SSL_CTX_free(sSslCtx);
        }
    }

    return IDE_FAILURE;
}

static void cmnLinkPeerDestroySslCtx(cmnLinkPeer *aLink)
{
    cmnLinkPeerSSL *sLink = (cmnLinkPeerSSL *)aLink;

    /* Free the allocated SSL_CTX object */
    if( sLink->mDesc.mSslCtx != NULL )
    {
        cmnOpenssl::mFuncs.SSL_CTX_free(sLink->mDesc.mSslCtx);
        sLink->mDesc.mSslCtx = NULL;
    }
    else
    {
        /* SSL_CTX is null */
    }
}

IDE_RC cmnLinkPeerInitializeSSL(cmnLink *aLink)
{
    cmnLinkPeerSSL *sLink = (cmnLinkPeerSSL *)aLink;

    /*
     * 멤버 초기화
     */
    sLink->mDesc.mHandle = PDL_INVALID_SOCKET;
    sLink->mDispatchInfo = 0;

    sLink->mPendingBlock = NULL;

    sLink->mDesc.mSslHandle = NULL;

    sLink->mDesc.mSslCtx = NULL;

    return IDE_SUCCESS;
}

IDE_RC cmnLinkPeerFinalizeSSL(cmnLink *aLink)
{
    cmnLinkPeerSSL *sLink = (cmnLinkPeerSSL *)aLink;
    cmbPool              *sPool = sLink->mLinkPeer.mPool;

    /* socket이 열려있으면 닫음 */
    IDE_TEST(aLink->mOp->mClose(aLink) != IDE_SUCCESS);

    /* Pending Block이 할당되어 있으면 해제 */
    if (sLink->mPendingBlock != NULL)
    {
        IDE_TEST(sPool->mOp->mFreeBlock(sPool, sLink->mPendingBlock) != IDE_SUCCESS);
    }
    else
    {
        /* do nothing */
    }

    if (sLink->mDesc.mSslCtx != NULL)
    {
        /* Clean up the thread's local error queue */
        cmnOpenssl::mFuncs.ERR_remove_state(0);
    }
    else
    {
        /* do nothing */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC cmnLinkPeerCloseSSL(cmnLink *aLink)
{
    cmnLinkPeerSSL *sLink = (cmnLinkPeerSSL *)aLink;

    /* Close the SSL socket */
    if (sLink->mDesc.mSslHandle != NULL )
    {
        /* BUG-41985 
           SSL_set_quiet_shutdown will set the internal flags to SSL_SENT_SHUTDOWN.
           The session is thus considered to be shutdown whitout sending a close alert to the peer. */

        cmnOpenssl::mFuncs.SSL_set_quiet_shutdown(sLink->mDesc.mSslHandle, 1);

        cmnOpenssl::mFuncs.SSL_free(sLink->mDesc.mSslHandle);
        sLink->mDesc.mSslHandle = NULL;
    }
    else
    {
        /* SSL handle is null */
    }

    cmnLinkPeerDestroySslCtx((cmnLinkPeer *)aLink);

    /* Close the TCP socket */
    if (sLink->mDesc.mHandle != PDL_INVALID_SOCKET)
    {
        idlOS::closesocket(sLink->mDesc.mHandle);

        sLink->mDesc.mHandle = PDL_INVALID_SOCKET;
    }
    else
    {
        /* do nothing */
    }

    return IDE_SUCCESS;
}

IDE_RC cmnLinkPeerGetHandleSSL(cmnLink *aLink, void *aHandle)
{
    cmnLinkPeerSSL *sLink = (cmnLinkPeerSSL *)aLink;

    /*
     * socket을 돌려줌
     */
    *(PDL_SOCKET *)aHandle = sLink->mDesc.mHandle;

    return IDE_SUCCESS;
}

IDE_RC cmnLinkPeerGetDispatchInfoSSL(cmnLink *aLink, void *aDispatchInfo)
{
    cmnLinkPeerSSL *sLink = (cmnLinkPeerSSL *)aLink;

    /*
     * DispatcherInfo를 돌려줌
     */
    *(UInt *)aDispatchInfo = sLink->mDispatchInfo;

    return IDE_SUCCESS;
}

IDE_RC cmnLinkPeerSetDispatchInfoSSL(cmnLink *aLink, void *aDispatchInfo)
{
    cmnLinkPeerSSL *sLink = (cmnLinkPeerSSL *)aLink;

    /*
     * DispatcherInfo를 세팅
     */
    sLink->mDispatchInfo = *(UInt *)aDispatchInfo;

    return IDE_SUCCESS;
}

IDE_RC cmnLinkPeerSetBlockingModeSSL(cmnLinkPeer *aLink, idBool aBlockingMode)
{
    cmnLinkPeerSSL *sLink = (cmnLinkPeerSSL *)aLink;

    /*
     * Set Blocking Mode
     */
    IDE_TEST(cmnSockSetBlockingMode(sLink->mDesc.mHandle, aBlockingMode) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    {
        ideLog::log(IDE_SERVER_2, CM_TRC_SOCKET_SET_BLOCKING_MODE_FAIL, errno);
    }

    return IDE_FAILURE;
}

IDE_RC cmnLinkPeerGetPeerCipherInfo(cmnLinkPeer *aLink, 
                                    SChar *aBuf, 
                                    UInt aBufLen)
{
    cmnLinkPeerSSL     *sLink = (cmnLinkPeerSSL *)aLink;
    SInt                sRet = 0;

    sRet = idlOS::snprintf(aBuf, aBufLen, "%s", 
                           cmnOpenssl::getSslCipherName(sLink->mDesc.mSslHandle));

    IDE_TEST_RAISE(sRet < 0, StringOutputError);
    IDE_TEST_RAISE((UInt)sRet >= aBufLen, StringTruncated);

    return IDE_SUCCESS;

    IDE_EXCEPTION(StringOutputError)
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_STRING_OUTPUT_ERROR));
    }
    IDE_EXCEPTION(StringTruncated)
    {
        IDE_SET(ideSetErrorCode(cmERR_IGNORE_STRING_TRUNCATED));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC cmnLinkPeerGetPeerCertSubject(cmnLinkPeer *aLink, 
                                     SChar *aBuf, 
                                     UInt aBufLen)
{
    cmnLinkPeerSSL     *sLink = (cmnLinkPeerSSL *)aLink;
    X509               *sPeerCert = NULL; /* Peer certificateion */
    SChar               sBuf[512]; /* Temporal buffer for X509 messages */
    SInt                sRet = 0;

    sPeerCert = cmnOpenssl::mFuncs.SSL_get_peer_certificate(sLink->mDesc.mSslHandle);
    if (sPeerCert != NULL)
    {   
        /* The peer has a certificate */
        cmnOpenssl::mFuncs.X509_NAME_oneline(cmnOpenssl::mFuncs.X509_get_subject_name(sPeerCert), sBuf, sizeof(sBuf));

        sRet = idlOS::snprintf(aBuf, aBufLen, "%s", sBuf);

        cmnOpenssl::mFuncs.X509_free(sPeerCert);
    }   
    else
    {
        aBuf[0] = '\0';
        aBufLen = 0;
    }   

    IDE_TEST_RAISE(sRet < 0, StringOutputError);
    IDE_TEST_RAISE((UInt)sRet >= aBufLen, StringTruncated);

    return IDE_SUCCESS;

    IDE_EXCEPTION(StringOutputError)
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_STRING_OUTPUT_ERROR));
    }
    IDE_EXCEPTION(StringTruncated)
    {
        IDE_SET(ideSetErrorCode(cmERR_IGNORE_STRING_TRUNCATED));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC cmnLinkPeerGetPeerCertIssuer(cmnLinkPeer *aLink, 
                                    SChar *aBuf, 
                                    UInt aBufLen)
{
    cmnLinkPeerSSL     *sLink = (cmnLinkPeerSSL *)aLink;
    X509               *sPeerCert = NULL; /* Peer certificateion */
    SChar               sBuf[512]; /* Temporal buffer for X509 messages */
    SInt                sRet = 0;

    sPeerCert = cmnOpenssl::mFuncs.SSL_get_peer_certificate(sLink->mDesc.mSslHandle);
    if (sPeerCert != NULL)
    {   
        /* The peer has a certificate */
        cmnOpenssl::mFuncs.X509_NAME_oneline(cmnOpenssl::mFuncs.X509_get_issuer_name(sPeerCert), sBuf, sizeof(sBuf));

        sRet = idlOS::snprintf(aBuf, aBufLen, "%s", sBuf);

        cmnOpenssl::mFuncs.X509_free(sPeerCert);
    }   
    else
    {
        aBuf[0] = '\0';
        aBufLen = 0;
    }   

    IDE_TEST_RAISE(sRet < 0, StringOutputError);
    IDE_TEST_RAISE((UInt)sRet >= aBufLen, StringTruncated);

    return IDE_SUCCESS;

    IDE_EXCEPTION(StringOutputError)
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_STRING_OUTPUT_ERROR));
    }
    IDE_EXCEPTION(StringTruncated)
    {
        IDE_SET(ideSetErrorCode(cmERR_IGNORE_STRING_TRUNCATED));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC cmnLinkPeerGetInfoSSL(cmnLinkPeer *aLink, 
                             SChar *aBuf, 
                             UInt aBufLen, 
                             cmnLinkInfoKey aKey)
{
    cmnLinkPeerSSL     *sLink = (cmnLinkPeerSSL *)aLink;
    struct sockaddr_storage  sAddr;
    SInt                sAddrLen = 0;
    SInt                sRet = 0;
    SChar               sErrMsg[256];

    SChar               sAddrStr[IDL_IP_ADDR_MAX_LEN];
    SChar               sPortStr[IDL_IP_PORT_MAX_LEN];

    sErrMsg[0] = '\0';

    /* proj-1538 ipv6 */
    switch (aKey)
    {
        case CMN_LINK_INFO_ALL:
        case CMN_LINK_INFO_REMOTE_ADDRESS:
        case CMN_LINK_INFO_REMOTE_IP_ADDRESS:
        case CMN_LINK_INFO_REMOTE_PORT:

            IDE_TEST_RAISE(
                idlOS::getnameinfo((struct sockaddr *)&sLink->mDesc.mAddr,
                                   sLink->mDesc.mAddrLen,
                                   sAddrStr, IDL_IP_ADDR_MAX_LEN,
                                   sPortStr, IDL_IP_PORT_MAX_LEN,
                                   NI_NUMERICHOST | NI_NUMERICSERV) != 0,
                GetNameInfoError);
            break;

        case CMN_LINK_INFO_LOCAL_ADDRESS:
        case CMN_LINK_INFO_LOCAL_IP_ADDRESS:
        case CMN_LINK_INFO_LOCAL_PORT:

            sAddrLen = ID_SIZEOF(sAddr);
            IDE_TEST_RAISE(idlOS::getsockname(sLink->mDesc.mHandle,
                                              (struct sockaddr *)&sAddr,
                                              &sAddrLen) != 0,
                           GetSockNameError);

            IDE_TEST_RAISE(
                idlOS::getnameinfo((struct sockaddr *)&sAddr,
                                   sAddrLen,
                                   sAddrStr, IDL_IP_ADDR_MAX_LEN,
                                   sPortStr, IDL_IP_PORT_MAX_LEN,
                                   NI_NUMERICHOST | NI_NUMERICSERV) != 0,
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
            IDE_RAISE(UnsupportedLinkInfoKey);
            break;
    }

    switch (aKey)
    {
        case CMN_LINK_INFO_ALL:
            sRet = idlOS::snprintf(aBuf, aBufLen, "SSL %s:%s",
                                   sAddrStr, sPortStr);
            break;

        case CMN_LINK_INFO_IMPL:
            sRet = idlOS::snprintf(aBuf, aBufLen, "SSL");
            break;

        case CMN_LINK_INFO_LOCAL_ADDRESS:
        case CMN_LINK_INFO_REMOTE_ADDRESS:
            sRet = idlOS::snprintf(aBuf, aBufLen, "%s:%s",
                                   sAddrStr, sPortStr);
            break;

        case CMN_LINK_INFO_LOCAL_IP_ADDRESS:
        case CMN_LINK_INFO_REMOTE_IP_ADDRESS:
            sRet = idlOS::snprintf(aBuf, aBufLen, "%s", sAddrStr);
            break;

        case CMN_LINK_INFO_LOCAL_PORT:
        case CMN_LINK_INFO_REMOTE_PORT:
            sRet = idlOS::snprintf(aBuf, aBufLen, "%s", sPortStr);
            break;

        case CMN_LINK_INFO_REMOTE_SOCKADDR:
            IDE_TEST_RAISE(aBufLen < (UInt)(sLink->mDesc.mAddrLen),
                           StringTruncated);
            idlOS::memcpy(aBuf, &sLink->mDesc.mAddr, sLink->mDesc.mAddrLen);
            break;

#if defined(TCP_INFO)
        /* PROJ-2625 Semi-async Prefetch, Prefetch Auto-tuning */
        case CMN_LINK_INFO_TCP_KERNEL_STAT:
            IDE_TEST_RAISE(idlOS::getsockopt(sLink->mDesc.mHandle,
                                             SOL_TCP,
                                             TCP_INFO,
                                             aBuf,
                                             (SInt *)&aBufLen) < 0, GetSockOptError);
            break;
#endif /* TCP_INFO */

        default:
            IDE_RAISE(UnsupportedLinkInfoKey);
            break;
    }

    IDE_TEST_RAISE(sRet < 0, StringOutputError);
    IDE_TEST_RAISE((UInt)sRet >= aBufLen, StringTruncated);

    return IDE_SUCCESS;

    IDE_EXCEPTION(StringOutputError)
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_STRING_OUTPUT_ERROR));
    }
    IDE_EXCEPTION(StringTruncated)
    {
        IDE_SET(ideSetErrorCode(cmERR_IGNORE_STRING_TRUNCATED));
    }
    IDE_EXCEPTION(GetSockNameError)
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_GETSOCKNAME_ERROR));
    }
    IDE_EXCEPTION(GetNameInfoError)
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_GETNAMEINFO_ERROR));
    }
    IDE_EXCEPTION(UnsupportedLinkInfoKey)
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_UNSUPPORTED_LINK_INFO_KEY));
    }
#if defined(TCP_INFO)
    IDE_EXCEPTION(GetSockOptError)
    {
        CMN_SET_ERRMSG_SOCK_OPT(sErrMsg, ID_SIZEOF(sErrMsg), "TCP_INFO", errno);

        IDE_SET(ideSetErrorCode(cmERR_ABORT_GETSOCKOPT_ERROR, sErrMsg));
    }
#endif /* TCP_INFO */
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC cmnLinkPeerGetDescSSL(cmnLinkPeer *aLink, void *aDesc)
{
    cmnLinkPeerSSL *sLink = (cmnLinkPeerSSL *)aLink;

    /*
     * Desc를 돌려줌
     */
    *(cmnLinkDescSSL **)aDesc = &sLink->mDesc;

    return IDE_SUCCESS;
}

/* PROJ-2625 Semi-async Prefetch, Prefetch Auto-tuning */
IDE_RC cmnLinkPeerGetSndBufSizeSSL(cmnLinkPeer *aLink, SInt *aSndBufSize)
{
    cmnLinkPeerSSL *sLink = (cmnLinkPeerSSL *)aLink;

    IDE_TEST(cmnSockGetSndBufSize(sLink->mDesc.mHandle, aSndBufSize) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC cmnLinkPeerSetSndBufSizeSSL(cmnLinkPeer *aLink, SInt aSndBufSize)
{
    cmnLinkPeerSSL *sLink = (cmnLinkPeerSSL *)aLink;

    IDE_TEST(cmnSockSetSndBufSize(sLink->mDesc.mHandle, aSndBufSize) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC cmnLinkPeerGetRcvBufSizeSSL(cmnLinkPeer *aLink, SInt *aRcvBufSize)
{
    cmnLinkPeerSSL *sLink = (cmnLinkPeerSSL *)aLink;

    IDE_TEST(cmnSockGetRcvBufSize(sLink->mDesc.mHandle, aRcvBufSize) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC cmnLinkPeerSetRcvBufSizeSSL(cmnLinkPeer *aLink, SInt aRcvBufSize)
{
    cmnLinkPeerSSL *sLink = (cmnLinkPeerSSL *)aLink;

    IDE_TEST(cmnSockSetRcvBufSize(sLink->mDesc.mHandle, aRcvBufSize) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC cmnLinkPeerConnByIP(cmnLinkPeer       *aLink,
                                  cmnLinkConnectArg *aConnectArg,
                                  PDL_Time_Value    *aTimeout,
                                  SInt               aOption,
                                  struct addrinfo   *aAddr);

static IDE_RC cmnLinkPeerConnByName(cmnLinkPeer       *aLink,
                                    cmnLinkConnectArg *aConnectArg,
                                    PDL_Time_Value    *aTimeout,
                                    SInt               aOption,
                                    struct addrinfo   *aAddrFirst);

/* BUGBUG: This function must be tested */
IDE_RC cmnLinkPeerConnectSSL(cmnLinkPeer       *aLink,
                             cmnLinkConnectArg *aConnectArg,
                             PDL_Time_Value    *aTimeout,
                             SInt aOption)
{
    cmnLinkPeerSSL *sLink = (cmnLinkPeerSSL *)aLink;
    UChar                 sTempAddr[ID_SIZEOF(struct in6_addr)];
    idBool                sHasIPAddr;
    struct addrinfo       sHints;
    SChar                 sPortStr[IDL_IP_PORT_MAX_LEN];
    SInt                  sRet = 0;
    const SChar          *sErrStr = NULL;
    SChar                 sErrMsg[256];
    struct addrinfo      *sAddrFirst = NULL;

    SSL_CTX              *sSslCtx = NULL;
    SSL                  *sSsl = NULL;
    X509                 *sPeer = NULL;
    SLong                 sSslRet = 0;
    SChar                 sPeerCN[256];

    sRet = cmnLinkPeerInitializeSslCtx(&sSslCtx);
    IDE_TEST_RAISE(sRet == IDE_FAILURE, FailedToSetupSslCtx);

    /* proj-1538 ipv6 */
    idlOS::memset(&sHints, 0x00, ID_SIZEOF(struct addrinfo));
    sHints.ai_socktype = SOCK_STREAM;
#if defined(AI_NUMERICSERV)
    sHints.ai_flags    = AI_NUMERICSERV; /* port no */
#endif

    /* inet_pton() returns a positive value on success */
    /* check to see if ipv4 address. ex) ddd.ddd.ddd.ddd */
    if (idlOS::inet_pton(AF_INET, aConnectArg->mSSL.mAddr,
                         sTempAddr) > 0)
    {
        sHasIPAddr = ID_TRUE;
        sHints.ai_family   = AF_INET;
        sHints.ai_flags    |= AI_NUMERICHOST;
    }
    /* check to see if ipv6 address. ex) h:h:h:h:h:h:h:h */
    /* bug-30835: support link-local address with zone index.
     *  before: inet_pton(inet6)
     *  after : strchr(':')
     *  inet_pton does not support zone index form. */
    else if (idlOS::memchr(aConnectArg->mSSL.mAddr, ':', strlen(aConnectArg->mSSL.mAddr)) != NULL)
    {
        sHasIPAddr = ID_TRUE;
        sHints.ai_family   = AF_INET6;
        sHints.ai_flags    |= AI_NUMERICHOST;
    }
    /* hostname format. ex) db.server.com */
    else
    {
        sHasIPAddr = ID_FALSE;
        sHints.ai_family   = AF_UNSPEC;
    }

    idlOS::sprintf(sPortStr, "%"ID_UINT32_FMT"", aConnectArg->mSSL.mPort);
    sRet = idlOS::getaddrinfo(aConnectArg->mSSL.mAddr, sPortStr,
                              &sHints, &sAddrFirst);
    if ((sRet != 0) || (sAddrFirst == NULL))
    {
        IDE_RAISE(GetAddrInfoError);
    }
    else
    {
        /* successfully got the address info */
    }

    /* in case that a user inputs the IP address directly */
    if (sHasIPAddr == ID_TRUE)
    {
        IDE_TEST(cmnLinkPeerConnByIP((cmnLinkPeer *)sLink, aConnectArg, aTimeout,
                                     aOption, sAddrFirst)
                 != IDE_SUCCESS);
    }
    /* in case that a user inputs the hostname instead of IP */
    else
    {
        IDE_TEST(cmnLinkPeerConnByName((cmnLinkPeer *)sLink, aConnectArg, aTimeout,
                                      aOption, sAddrFirst)
                 != IDE_SUCCESS);
    }

    if (sAddrFirst != NULL)
    {
        idlOS::freeaddrinfo(sAddrFirst);
        sAddrFirst = NULL;
    }
    else
    {
        /* do nothing */
    }

    if (cmuProperty::getSslVerifyPeerCert() == ID_TRUE)
    {
        /* ssl connection */
        sSsl = cmnOpenssl::mFuncs.SSL_new(sSslCtx);
        IDE_TEST_RAISE(sSsl == NULL, SslOperationFail);

        sRet = cmnOpenssl::mFuncs.SSL_connect(sSsl);
        IDE_TEST_RAISE(sRet <= 0, FailedToSSLConnect);

        sSslRet = cmnOpenssl::mFuncs.SSL_get_verify_result(sSsl);
        IDE_TEST_RAISE(sSslRet != X509_V_OK, FailedToVerifyPeerCertificate);

        sPeer = cmnOpenssl::mFuncs.SSL_get_peer_certificate(sSsl);
        cmnOpenssl::mFuncs.X509_NAME_get_text_by_NID(cmnOpenssl::mFuncs.X509_get_subject_name(sPeer),
                                                     NID_commonName, 
                                                     sPeerCN, 
                                                     sizeof(sPeerCN));

        sRet = idlOS::strcasecmp(sPeerCN, aConnectArg->mSSL.mAddr);
        IDE_TEST_RAISE(sRet != 0, FailedToVerifyPeerCertificate);
    }
    else
    {
        ideLog::log(IDE_SERVER_0, "Skip ceritificate verification of host[%s]\n", aConnectArg->mSSL.mAddr);
    }

    sLink->mDesc.mSslCtx = sSslCtx;
    sLink->mDesc.mSslHandle = sSsl;

    return IDE_SUCCESS;

    IDE_EXCEPTION(SslOperationFail)
    {   
        IDE_SET(ideSetErrorCode(cmERR_ABORT_SSL_OPERATION, cmnOpenssl::getSslErrorMessage()));
    }  
    IDE_EXCEPTION(FailedToSSLConnect)
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_SSL_CONNECT, cmnOpenssl::getSslErrorMessage()));
    }
    IDE_EXCEPTION( FailedToVerifyPeerCertificate )
    {   
        IDE_SET(ideSetErrorCode(cmERR_ABORT_VERIFY_PEER_CERITIFICATE, cmnOpenssl::getSslErrorMessage()));
    } 
    IDE_EXCEPTION(FailedToSetupSslCtx)
    {
        ideLog::log(IDE_SERVER_2, "SSL_CTX setup failed.\n");
    }
    IDE_EXCEPTION(GetAddrInfoError)
    {
        sErrStr = idlOS::gai_strerror(sRet);
        sErrMsg[0] = '\0';

        if (sErrStr == NULL)
        {
            idlOS::sprintf(sErrMsg, "%"ID_INT32_FMT, sRet);
        }
        else
        {
            idlOS::sprintf(sErrMsg, "%s", sErrStr);
        }

        IDE_SET(ideSetErrorCode(cmERR_ABORT_GETADDRINFO_ERROR, sErrMsg));
    }
    IDE_EXCEPTION_END;

    if (sAddrFirst != NULL)
    {
        idlOS::freeaddrinfo(sAddrFirst);
        sAddrFirst = NULL;
    }
    else
    {
        /* sAddrFirst is null */
    }

    if (sSsl != NULL)
    {
        cmnOpenssl::mFuncs.SSL_free(sSsl);
    }
    else
    {
        /* sSsl is null */
    }

    if (sSslCtx != NULL)
    {
        cmnOpenssl::mFuncs.SSL_CTX_free(sSslCtx);
    }
    else
    {
        /* sSslCtx is null */
    }

    return IDE_FAILURE;
}

/* proj-1538 ipv6 */
/* in case that a user inputs the IP address directly */
static IDE_RC cmnLinkPeerConnByIP(cmnLinkPeer*       aLink,
                                  cmnLinkConnectArg* /* aConnectArg */,
                                  PDL_Time_Value*    aTimeout,
                                  SInt               aOption,
                                  struct addrinfo*   aAddr)
{
    cmnLinkPeerSSL *sLink = (cmnLinkPeerSSL *)aLink;
    PDL_SOCKET            sHandle = PDL_INVALID_SOCKET;
    SInt                  sRet = 0;

    sHandle = idlOS::socket(aAddr->ai_family, SOCK_STREAM, 0);
    IDE_TEST_RAISE(sHandle == PDL_INVALID_SOCKET, SocketError);

    sRet = idlVA::connect_timed_wait(sHandle,
                                     aAddr->ai_addr,
                                     aAddr->ai_addrlen,
                                     aTimeout);

    /* bug-30835: support link-local address with zone index.
     * On linux, link-local addr without zone index makes EINVAL.
     *  .EINVAL   : Invalid argument to connect()
     *  .others   : Client unable to establish connection. */
    if (sRet != 0)
    {
        if (errno == EINVAL)
        {
            IDE_RAISE(ConnInvalidArg);
        }
        else
        {
            IDE_TEST_RAISE( errno == ETIMEDOUT, TimedOut );
            IDE_RAISE(ConnectError);
        }
    }
    else
    {
        /* do nothing */
    }

    /* save fd and IP address into link info */
    sLink->mDesc.mHandle = sHandle;
    idlOS::memcpy(&sLink->mDesc.mAddr, aAddr->ai_addr, aAddr->ai_addrlen);
    sLink->mDesc.mAddrLen = aAddr->ai_addrlen;

    /* socket 초기화 */
    IDE_TEST(aLink->mPeerOp->mSetOptions(aLink, aOption) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION(SocketError)
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_SOCKET_OPEN_ERROR, errno));
    }
    IDE_EXCEPTION(ConnectError)
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_CONNECT_ERROR, errno));
    }
    /* bug-30835: support link-local address with zone index. */
    IDE_EXCEPTION(ConnInvalidArg)
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_CONNECT_INVALIDARG));
    }
    IDE_EXCEPTION(TimedOut)
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_TIMED_OUT));
    }
    IDE_EXCEPTION_END;

    // BUG-24170 [CM] cmiConnect 실패 시, cmiConnect 내에서 close
    if (sHandle != PDL_INVALID_SOCKET)
    {
        (void)idlOS::closesocket(sHandle);
    }
    else
    {
        /* sHandle is a PDL_INVALID_SOCKET */
    }

    sLink->mDesc.mHandle = PDL_INVALID_SOCKET;

    return IDE_FAILURE;
}

/* proj-1538 ipv6 */
/* in case that a user inputs the hostname instead of IP */
static IDE_RC cmnLinkPeerConnByName(cmnLinkPeer*       aLink,
                                    cmnLinkConnectArg* aConnectArg,
                                    PDL_Time_Value*    aTimeout,
                                    SInt               aOption,
                                    struct addrinfo*   aAddrFirst)
{
    cmnLinkPeerSSL *sLink = (cmnLinkPeerSSL *)aLink;
    struct addrinfo      *sAddr = NULL;

#define CM_HOST_IP_MAX_COUNT 10
    struct addrinfo     *sAddrListV4[CM_HOST_IP_MAX_COUNT];
    struct addrinfo     *sAddrListV6[CM_HOST_IP_MAX_COUNT];
    struct addrinfo    **sAddrListFinal = NULL;

    SInt                 sAddrCntV4 = 0;
    SInt                 sAddrCntV6 = 0;
    SInt                 sAddrCntFinal = 0;

    SInt                 sIdx    = 0;
    SInt                 sTryCnt = 0;
    PDL_SOCKET           sHandle = PDL_INVALID_SOCKET;
    SInt                 sRet    = 0;


    idlOS::memset(sAddrListV4, 0x00, ID_SIZEOF(sAddrListV4));
    idlOS::memset(sAddrListV6, 0x00, ID_SIZEOF(sAddrListV6));

    for (sAddr = aAddrFirst;
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
        
        sHandle = idlOS::socket(sAddr->ai_family, SOCK_STREAM, 0);
        IDE_TEST_RAISE(sHandle == PDL_INVALID_SOCKET, SocketError);
        sRet = idlVA::connect_timed_wait(sHandle,
                                         sAddr->ai_addr,
                                         sAddr->ai_addrlen,
                                         aTimeout);
        if (sRet == 0)
        {
            break;
        }
        else
        {
            /* do nothing */
        }

        (void)idlOS::closesocket(sHandle);
        sHandle = PDL_INVALID_SOCKET;
    }

    /* bug-30835: support link-local address with zone index.
     * On linux, link-local addr without zone index makes EINVAL. */
    if ((sRet != 0) || (sTryCnt == 0))
    {
        if (errno == EINVAL)
        {
            IDE_RAISE(ConnInvalidArg);
        }
        else
        {
            IDE_TEST_RAISE( errno == ETIMEDOUT, TimedOut );
            IDE_RAISE(ConnectError);
        }
    }
    else
    {
        /* do nothing */
    }

    /* save fd and IP address into link info */
    sLink->mDesc.mHandle = sHandle;
    idlOS::memcpy(&sLink->mDesc.mAddr, sAddr->ai_addr, sAddr->ai_addrlen);
    sLink->mDesc.mAddrLen = sAddr->ai_addrlen;

    /* socket 초기화 */
    IDE_TEST(aLink->mPeerOp->mSetOptions(aLink, aOption) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION(SocketError)
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_SOCKET_OPEN_ERROR, errno));
    }
    IDE_EXCEPTION(ConnectError)
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_CONNECT_ERROR, errno));
    }
    /* bug-30835: support link-local address with zone index. */
    IDE_EXCEPTION(ConnInvalidArg)
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_CONNECT_INVALIDARG));
    }
    IDE_EXCEPTION(TimedOut)
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_TIMED_OUT));
    }
    IDE_EXCEPTION_END;

    // BUG-24170 [CM] cmiConnect 실패 시, cmiConnect 내에서 close
    if (sHandle != PDL_INVALID_SOCKET)
    {
        (void)idlOS::closesocket(sHandle);
    }
    else
    {
        /* do nothing */
    }
    sLink->mDesc.mHandle = PDL_INVALID_SOCKET;

    return IDE_FAILURE;
}

IDE_RC cmnLinkPeerAllocChannelSSL(cmnLinkPeer */*aLink*/, SInt */*aChannelID*/)
{
    return IDE_SUCCESS;
}

IDE_RC cmnLinkPeerHandshakeSSL(cmnLinkPeer */*aLink*/)
{
    return IDE_SUCCESS;
}

IDE_RC cmnLinkPeerSetOptionsSSL(cmnLinkPeer *aLink, SInt aOption)
{
    cmnLinkPeerSSL *sLink = (cmnLinkPeerSSL *)aLink;
    SInt                  sOption = 0;
    linger                sLingerOption;

    /*
     * SO_KEEPALIVE 세팅
     */
    sOption = 1;

    if (idlOS::setsockopt(sLink->mDesc.mHandle,
                          SOL_SOCKET,
                          SO_KEEPALIVE,
                          (SChar *)&sOption,
                          ID_SIZEOF(sOption)) < 0)
    {
        ideLog::log(IDE_SERVER_0, CM_TRC_SOCKET_KEEPALIVE_FAIL, errno);
    }
    else
    {
        /* do nothing */
    }

    // BUG-26484: 추가로 설정할 소켓 옵션을 지정
    if (aOption == SO_LINGER)
    {
        // 연속으로 연결했다 끊기를 반복하면 더 이상 연결할 수 없게된다.
        // 일반적으로 소켓은 close해도 TIME_WAIT 상태로 일정시간 대기하기 때문이다.
        // SO_LINGER 옵션 추가. (SO_REUSEADDR 옵션으로는 잘 안될 수도 있다;)
        /*
        * SO_LINGER 세팅
        */
        sLingerOption.l_onoff  = 1;
        sLingerOption.l_linger = 0;

        if (idlOS::setsockopt(sLink->mDesc.mHandle,
                              SOL_SOCKET,
                              SO_LINGER,
                              (SChar *)&sLingerOption,
                              ID_SIZEOF(sLingerOption)) < 0)
        {
            ideLog::log(IDE_SERVER_0, CM_TRC_SOCKET_SET_OPTION_FAIL, "SO_LINGER", errno);
        }
        else
        {
            /* no error */
        }
    }
    else if (aOption == SO_REUSEADDR)
    {
        /*
         * SO_REUSEADDR 세팅
         */
        sOption = 1;

        if (idlOS::setsockopt(sLink->mDesc.mHandle,
                              SOL_SOCKET,
                              SO_REUSEADDR,
                              (SChar *)&sOption,
                              ID_SIZEOF(sOption)) < 0)
        {
            ideLog::log(IDE_SERVER_0, CM_TRC_SOCKET_SET_OPTION_FAIL, "SO_REUSEADDR", errno);
        }
        else
        {
            /* no error */
        }
    }
    else
    {
        /* do nothing */
    }

    /*
     * TCP_NODELAY 세팅
     */
    sOption = 1;

    if (idlOS::setsockopt(sLink->mDesc.mHandle,
                          IPPROTO_TCP,
                          TCP_NODELAY,
                          (char *)&sOption,
                          ID_SIZEOF(sOption)) < 0)
    {
        ideLog::log(IDE_SERVER_2, CM_TRC_TCP_NODELAY_FAIL, errno);
    }
    else
    {
        /* no error */
    }

    /* BUG-22028
     * SO_SNDBUF 세팅 (대역폭 * 지연율) * 2
     */
    sOption = CMB_BLOCK_DEFAULT_SIZE * 2;

    if (idlOS::setsockopt(sLink->mDesc.mHandle,
                          SOL_SOCKET,
                          SO_SNDBUF,
                          (SChar *)&sOption,
                          ID_SIZEOF(sOption)) < 0)
    {
        ideLog::log(IDE_SERVER_0, CM_TRC_SOCKET_SET_OPTION_FAIL, "SO_SNDBUF", errno);
    }
    else
    {
        /* no error */
    }

    /* BUG-22028
     * SO_RCVBUF 세팅 (대역폭 * 지연율) * 2
     */
    sOption = CMB_BLOCK_DEFAULT_SIZE * 2;

    if (idlOS::setsockopt(sLink->mDesc.mHandle,
                          SOL_SOCKET,
                          SO_RCVBUF,
                          (SChar *)&sOption,
                          ID_SIZEOF(sOption)) < 0)
    {
        ideLog::log(IDE_SERVER_0, CM_TRC_SOCKET_SET_OPTION_FAIL, "SO_RCVBUF", errno);
    }
    else
    {
        /* no error */
    }

    return IDE_SUCCESS;
}

IDE_RC cmnLinkPeerShutdownSSL( cmnLinkPeer    *aLink,
                               cmnDirection    aDirection,
                               cmnShutdownMode /*aMode*/ )
{
    cmnLinkPeerSSL *sLink = (cmnLinkPeerSSL *)aLink;
    SInt sRet = 0;

    if( sLink->mDesc.mSslHandle != NULL )
    {
        /* SSL shutdown */
        sRet = cmnOpenssl::mFuncs.SSL_shutdown( sLink->mDesc.mSslHandle );
        if( sRet == 0 )
        {
            /* BUG-39922 */
            cmnOpenssl::mFuncs.SSL_set_quiet_shutdown( sLink->mDesc.mSslHandle, 1 );
            
            sRet = cmnOpenssl::mFuncs.SSL_shutdown( sLink->mDesc.mSslHandle );
        }
        else
        {
            /* The SSL connection has been shut down successfully. */
        }

        IDE_TEST_RAISE( sRet != 1, SSLShutdownError );
    }

    /* TCP socket shutdown */
    IDE_TEST_RAISE(idlOS::shutdown(sLink->mDesc.mHandle, aDirection) != 0, ShutdownError);

    return IDE_SUCCESS;

    IDE_EXCEPTION(SSLShutdownError)
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_SSL_SHUTDOWN, cmnOpenssl::getSslErrorMessage()));
    }
    IDE_EXCEPTION(ShutdownError)
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_SOCKET_SHUTDOWN_FAILED));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC cmnSslSockRecv(SSL            *aSslHandle,
                             cmbBlock       *aBlock,
                             cmnLinkPeer    *aLink,
                             UShort          aSize,
                             PDL_Time_Value *aTimeout,
                             idvStatIndex    aStatIndex)
{
    ssize_t sSize = 0;
    SInt    sSslError = SSL_ERROR_NONE;

    IDE_TEST(aBlock == NULL);

    /*  
     * aSize 이상 aBlock으로 데이터 읽음
     */
    while (aBlock->mDataSize < aSize)
    {   
        /*
         * Dispatcher를 이용하여 Timeout 만큼 대기
         */
        if (aTimeout != NULL)
        {
            IDE_TEST(cmnDispatcherWaitLink((cmnLink *)aLink,
                                           CMN_DIRECTION_RD,
                                           aTimeout) != IDE_SUCCESS);
        }
        else
        {
            /* aTimeout is null */
        }

        sSize = cmnOpenssl::mFuncs.SSL_read(aSslHandle, 
                                            aBlock->mData + aBlock->mDataSize, 
                                            aBlock->mBlockSize - aBlock->mDataSize);

        if (sSize <= 0)
        {
            sSslError = cmnOpenssl::mFuncs.SSL_get_error(aSslHandle, sSize);

            switch (sSslError)
            {
                /* As at any time a re-negotiation is possible, 
                 * a call to SSL_read() can also cause write operations as well as  read operations. 
                 * The calling process must repeat the call 
                 * after taking appropriate action to satisfy the needs of SSL_read() */

                case SSL_ERROR_WANT_READ:
                    IDE_TEST(cmnDispatcherWaitLink((cmnLink *)aLink,
                                                   CMN_DIRECTION_RD, 
                                                   aTimeout) != IDE_SUCCESS);
                    break;

                case SSL_ERROR_WANT_WRITE:
                    IDE_TEST(cmnDispatcherWaitLink((cmnLink *)aLink,
                                                   CMN_DIRECTION_WR, 
                                                   aTimeout) != IDE_SUCCESS);
                    break;

                case SSL_ERROR_SYSCALL:
                    if (sSize == 0)
                    {
                        IDE_RAISE(ConnectionClosed);
                    }
                    else
                    {
                        IDE_RAISE(SslReadError);
                    }
                    break;

                default:
                    IDE_RAISE(SslReadError);
                    break;
            }

            // To Fix BUG-21008
            IDV_SESS_ADD(aLink->mStatistics, IDV_STAT_INDEX_RECV_SOCKET_COUNT, 1 );
        }
        else
        {
            aBlock->mDataSize += sSize;
            IDV_SESS_ADD(aLink->mStatistics, aStatIndex, (ULong)sSize);
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ConnectionClosed)
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_CONNECTION_CLOSED));
    }
    IDE_EXCEPTION(SslReadError)
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_SSL_READ, cmnOpenssl::getSslErrorMessage()));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


static IDE_RC cmnSslSockSend(SSL            *aSSL,
                             cmbBlock       *aBlock,
                             cmnLinkPeer    *aLink,
                             PDL_Time_Value *aTimeout,
                             idvStatIndex    aStatIndex)
{
    ssize_t sSize = 0;
    SInt    sSslError = SSL_ERROR_NONE;

    IDE_TEST(aBlock == NULL);

    if (aBlock->mCursor == aBlock->mDataSize)
    {
        aBlock->mCursor = 0;
    }
    else
    {
        /* If the block is not empty, and has some data to send */
    }

    while (aBlock->mCursor < aBlock->mDataSize)
    {
        /*
         * Dispatcher를 이용하여 Timeout 만큼 대기
         */
        if (aTimeout != NULL)
        {
            IDE_TEST(cmnDispatcherWaitLink((cmnLink *)aLink,
                                           CMN_DIRECTION_WR,
                                           aTimeout) != IDE_SUCCESS);
        }
        else
        {
            /* aTimeout is null */
        }

        sSize = cmnOpenssl::mFuncs.SSL_write(aSSL,
                                             aBlock->mData + aBlock->mCursor,
                                             aBlock->mDataSize - aBlock->mCursor);

        // To Fix BUG-21008
        IDV_SESS_ADD(aLink->mStatistics, IDV_STAT_INDEX_SEND_SOCKET_COUNT, 1 );

        if (sSize <= 0)
        {
            sSslError = cmnOpenssl::mFuncs.SSL_get_error(aSSL, sSize);

            switch (sSslError)
            {
                /* As at any time a re-negotiation is possible, 
                 * a call to SSL_write() can also cause write operations as well as  read operations. 
                 * The calling process must repeat the call 
                 * after taking appropriate action to satisfy the needs of SSL_write() */

                case SSL_ERROR_WANT_WRITE:
                    IDE_RAISE(Retry);
                    break;

                case SSL_ERROR_WANT_READ:
                    IDE_RAISE(Retry);
                    break;

                default:
                    IDE_RAISE(SslWriteError);
                    break;
            }
        }
        else
        {
            aBlock->mCursor += sSize;

            IDV_SESS_ADD(aLink->mStatistics, aStatIndex, (ULong)sSize);
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(Retry)
    {
        IDE_SET(ideSetErrorCode(cmERR_RETRY_SOCKET_OPERATION_WOULD_BLOCK));
    }
    IDE_EXCEPTION(SslWriteError)
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_SSL_WRITE, cmnOpenssl::getSslErrorMessage()));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC cmnLinkPeerRecvSSL(cmnLinkPeer *aLink, 
                          cmbBlock **aBlock,
                          cmpHeader *aHeader, 
                          PDL_Time_Value *aTimeout)
{
    cmnLinkPeerSSL *sLink  = (cmnLinkPeerSSL *)aLink;
    cmbPool              *sPool  = aLink->mPool;
    cmbBlock             *sBlock = NULL;
    cmpHeader             sHeader;
    UShort                sPacketSize = 0;
    cmpPacketType         sPacketType = aLink->mLink.mPacketType;

    IDE_TEST_RAISE(sPacketType == CMP_PACKET_TYPE_A5, UnsupportedNetworkProtocol);

    /*
     * Pending Block있으면 사용 그렇지 않으면 Block 할당
     */
    /* proj_2160 cm_type removal */
    /* A7 or CMP_PACKET_TYPE_UNKNOWN: block already allocated. */

    sBlock = *aBlock;

    if (sLink->mPendingBlock != NULL)
    {
        idlOS::memcpy(sBlock->mData,
                      sLink->mPendingBlock->mData,
                      sLink->mPendingBlock->mDataSize);
        sBlock->mDataSize = sLink->mPendingBlock->mDataSize;

        IDE_TEST(sPool->mOp->mFreeBlock(sPool, sLink->mPendingBlock)
                 != IDE_SUCCESS);
        sLink->mPendingBlock = NULL;
    }
    else
    {
        /* no pending data */
    }

    /*
     * Protocol Header Size 크기 이상 읽음
     */
    IDE_TEST_RAISE(cmnSslSockRecv(sLink->mDesc.mSslHandle, 
                                  sBlock,
                                  aLink,
                                  CMP_HEADER_SIZE,
                                  aTimeout,
                                  IDV_STAT_INDEX_RECV_TCP_BYTE) != IDE_SUCCESS, SockRecvError);

    /*
     * Protocol Header 해석
     */
    IDE_TEST(cmpHeaderRead(aLink, &sHeader, sBlock) != IDE_SUCCESS);
    sPacketSize = sHeader.mA7.mPayloadLength + CMP_HEADER_SIZE;

    /*
     * 패킷 크기 이상 읽음
     */
    IDE_TEST_RAISE(cmnSslSockRecv(sLink->mDesc.mSslHandle,
                                  sBlock,
                                  aLink,
                                  sPacketSize,
                                  aTimeout,
                                  IDV_STAT_INDEX_RECV_TCP_BYTE) != IDE_SUCCESS, SockRecvError);

    /*
     * 패킷 크기 이상 읽혔으면 현재 패킷 이후의 데이터를 Pending Block으로 옮김
     */
    if (sBlock->mDataSize > sPacketSize)
    {
        IDE_TEST(sPool->mOp->mAllocBlock(sPool, &sLink->mPendingBlock) != IDE_SUCCESS);
        IDE_TEST(cmbBlockMove(sLink->mPendingBlock, sBlock, sPacketSize) != IDE_SUCCESS);
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
    *aHeader = sHeader;

    return IDE_SUCCESS;
    
    /* BUG-39127 If a network timeout occurs during replication sync 
     * then it fails by communication protocol error.
     */ 
    IDE_EXCEPTION( SockRecvError )
    {
        if (sBlock->mDataSize != 0)
        {
            if (sPool->mOp->mAllocBlock( sPool, &sLink->mPendingBlock ) == IDE_SUCCESS)
            {
                IDE_ASSERT( cmbBlockMove(sLink->mPendingBlock, sBlock, 0 ) == IDE_SUCCESS );
            }
            else
            {
                /* when the alloc error occured, the error must return to upper module
                 * therefore in this exception, do not use IDE_PUSH() and IDE_POP()
                 */
            }
        }
        else
        {
            /* nothing to do */
        }
    }
    IDE_EXCEPTION( UnsupportedNetworkProtocol )
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_UNSUPPORTED_NETWORK_PROTOCOL));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC cmnLinkPeerReqCompleteSSL(cmnLinkPeer */*aLink*/)
{
    return IDE_SUCCESS;
}

IDE_RC cmnLinkPeerResCompleteSSL(cmnLinkPeer */*aLink*/)
{
    return IDE_SUCCESS;
}

IDE_RC cmnLinkPeerSendSSL(cmnLinkPeer *aLink, cmbBlock *aBlock)
{
    cmnLinkPeerSSL *sLink = (cmnLinkPeerSSL *)aLink;

    /*
     * Block 전송
     */
    IDE_TEST(cmnSslSockSend(sLink->mDesc.mSslHandle,
                            aBlock,
                            aLink,
                            NULL,
                            IDV_STAT_INDEX_SEND_TCP_BYTE) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC cmnLinkPeerCheckSSL(cmnLinkPeer *aLink, idBool *aIsClosed)
{
    cmnLinkPeerSSL *sLink = (cmnLinkPeerSSL *)aLink;

    return cmnSockCheck((cmnLink *)aLink, sLink->mDesc.mHandle, aIsClosed);
}

idBool cmnLinkPeerHasPendingRequestSSL(cmnLinkPeer *aLink)
{
    cmnLinkPeerSSL *sLink = (cmnLinkPeerSSL *)aLink;

    return (sLink->mPendingBlock != NULL) ? ID_TRUE : ID_FALSE;
}

IDE_RC cmnLinkPeerAllocBlockSSL(cmnLinkPeer *aLink, cmbBlock **aBlock)
{
    cmbBlock *sBlock = NULL;

    IDE_TEST(aLink->mPool->mOp->mAllocBlock(aLink->mPool, &sBlock) != IDE_SUCCESS);

    /*
     * Write Block 초기화
     */
    sBlock->mDataSize = CMP_HEADER_SIZE;
    sBlock->mCursor   = CMP_HEADER_SIZE;

    /*
     * Write Block을 돌려줌
     */
    *aBlock = sBlock;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC cmnLinkPeerFreeBlockSSL(cmnLinkPeer *aLink, cmbBlock *aBlock)
{
    /*
     * Block 해제
     */
    IDE_TEST(aLink->mPool->mOp->mFreeBlock(aLink->mPool, aBlock) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


struct cmnLinkOP gCmnLinkPeerOpSSL =
{
    "SSL-PEER",

    cmnLinkPeerInitializeSSL,
    cmnLinkPeerFinalizeSSL,

    cmnLinkPeerCloseSSL,

    cmnLinkPeerGetHandleSSL,

    cmnLinkPeerGetDispatchInfoSSL,
    cmnLinkPeerSetDispatchInfoSSL
};

struct cmnLinkPeerOP gCmnLinkPeerPeerOpSSL =
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
    cmnLinkPeerGetPeerCipherInfo, 
    cmnLinkPeerGetPeerCertSubject, 
    cmnLinkPeerGetPeerCertIssuer,

    /* PROJ-2625 Semi-async Prefetch, Prefetch Auto-tuning */
    cmnLinkPeerGetSndBufSizeSSL,
    cmnLinkPeerSetSndBufSizeSSL,
    cmnLinkPeerGetRcvBufSizeSSL,
    cmnLinkPeerSetRcvBufSizeSSL
};


IDE_RC cmnLinkPeerMapSSL(cmnLink *aLink)
{
    cmnLinkPeer *sLink = (cmnLinkPeer *)aLink;

    /*
     * Link 검사
     */
    IDE_ASSERT((aLink->mType == CMN_LINK_TYPE_PEER_SERVER) ||
               (aLink->mType == CMN_LINK_TYPE_PEER_CLIENT));
    IDE_ASSERT(aLink->mImpl == CMN_LINK_IMPL_SSL);

    /*
     * Shared Pool 획득
     */
    IDE_TEST(cmbPoolGetSharedPool(&sLink->mPool, CMB_POOL_IMPL_LOCAL) != IDE_SUCCESS);

    /*
     * 함수 포인터 세팅
     */
    aLink->mOp     = &gCmnLinkPeerOpSSL;
    sLink->mPeerOp = &gCmnLinkPeerPeerOpSSL;

    /*
     * 멤버 초기화
     */
    sLink->mStatistics = NULL;
    sLink->mUserPtr    = NULL;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

UInt cmnLinkPeerSizeSSL()
{
    return ID_SIZEOF(cmnLinkPeerSSL);
}

#endif /* CM_DISABLE_SSL */
