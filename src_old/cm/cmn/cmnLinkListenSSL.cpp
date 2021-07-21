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

typedef struct cmnLinkListenSSL
{
    cmnLinkListen mLinkListen;

    PDL_SOCKET    mHandle;
    UInt          mDispatchInfo;
    SSL_CTX      *mSslCtx;
} cmnLinkListenSSL;

#if (DEBUG)  /* BUG-47037 */
/* This callback function shows state strings, and information
 * about alerts being handled and error messages */
static void cmnSslHandshakeInfoCallback( const SSL *aSSL, SInt aWhere, SInt aRet )
{
    const SChar *sStr;
    SInt   sWhere;

    sWhere = aWhere & ~SSL_ST_MASK;

    if (sWhere & SSL_ST_CONNECT)
    {
        sStr = "SSL_connect";
    }
    else if (sWhere & SSL_ST_ACCEPT)
    {
        sStr = "SSL_accept";
    }
    else
    {
        sStr = "Undefined";
    }

    if (aWhere & SSL_CB_LOOP)
    {
        ideLog::log(IDE_SERVER_0, "%s: %s\n", 
                    sStr, 
                    cmnOpenssl::mFuncs.SSL_state_string_long(aSSL));
    }
    else if (aWhere & SSL_CB_ALERT)
    {
        sStr = (aWhere & SSL_CB_READ) ? "read" : "write";
        ideLog::log(IDE_SERVER_0, "SSL alert %s: %s: %s\n", 
                    sStr, 
                    cmnOpenssl::mFuncs.SSL_alert_type_string_long(aRet), 
                    cmnOpenssl::mFuncs.SSL_alert_desc_string_long(aRet));
    }
    else if (aWhere & SSL_CB_EXIT)
    {
        if (aRet == 0)
        {
            ideLog::log(IDE_SERVER_0, "%s: failed in %s\n", 
                        sStr, 
                        cmnOpenssl::mFuncs.SSL_state_string_long(aSSL));
        }
        else if (aRet < 0)
        {
            ideLog::log(IDE_SERVER_0, "%s: error in %s\n", 
                        sStr, 
                        cmnOpenssl::mFuncs.SSL_state_string_long(aSSL));
        }
        else
        {
            /* do nothing */
        }
    }
    else
    {
        /* do nothing */
    }
}
#endif

IDE_RC cmnLinkListenInitializeSSL(cmnLink *aLink)
{
    cmnLinkListenSSL *sLink = (cmnLinkListenSSL *)aLink;

    SSL_METHOD *sMethod = NULL;
    SSL_CTX    *sSslCtx = NULL;
    SInt        sRet = 0;
    SChar      *sServerKeyFile = NULL;
    SChar      *sCAFile = NULL;
    SChar      *sCAPath = NULL;
    SChar      *sServerCertFile = NULL;
    SChar      *sCipherList = NULL;
    STACK_OF(X509_NAME) *sCAList = NULL;

    /*
     * Handle 초기화
     */
    sLink->mHandle = PDL_INVALID_SOCKET;

    /* Get properties relating to SSL/TLS */
    sCAFile = cmuProperty::getSslCa();
    if (sCAFile[0] == '\0')
    {
        sCAFile = NULL;
    }
    else
    {
        /* do nothing */
    }

    sCAPath = cmuProperty::getSslCaPath();
    if (sCAPath[0] == '\0')
    {
        sCAPath = NULL;
    }
    else
    {
        /* do nothing */
    }

    sCipherList = cmuProperty::getSslCipherList();
    if (sCipherList[0] == '\0')
    {
        sCipherList = NULL;
    }
    else
    {
        /* do nothing */
    }

    sServerKeyFile = cmuProperty::getSslKey();
    sServerCertFile = cmuProperty::getSslCert();

    /* Create and set up the SSL Context structure(SSL_CTX) */
    sMethod = (SSL_METHOD*)cmnOpenssl::mFuncs.TLSv1_server_method(); /* tLSv1 hello messages. */
    sSslCtx = cmnOpenssl::mFuncs.SSL_CTX_new(sMethod); /* create new context from method */
    IDE_TEST_RAISE(sSslCtx == NULL, FailedToCreateSslCtx);

#if (DEBUG)  /* BUG-47037 */
    /* set the callback function, that can be used to obtain state infomation for SSL objects */
    if ( cmnOpenssl::mFuncs.SSL_CTX_set_info_callback != NULL )
    {
        cmnOpenssl::mFuncs.SSL_CTX_set_info_callback(sSslCtx, cmnSslHandshakeInfoCallback);
    }
    else
    {
        /* No callback function is available since the pointer is null. */
    }
#endif

    /* set the certificate */
    sRet = cmnOpenssl::mFuncs.SSL_CTX_use_certificate_file(sSslCtx, sServerCertFile, SSL_FILETYPE_PEM);
    IDE_TEST_RAISE(sRet == 0, FailedToSetCertificate);

    /* set the private key from KeyFile. */
    sRet = cmnOpenssl::mFuncs.SSL_CTX_use_PrivateKey_file(sSslCtx, sServerKeyFile, SSL_FILETYPE_PEM);
    IDE_TEST_RAISE(sRet == 0, FailedToSetPrivateKey);

    /* verify the private key */
    sRet = cmnOpenssl::mFuncs.SSL_CTX_check_private_key(sSslCtx);
    IDE_TEST_RAISE(sRet == 0, FailedToVerifyPrivateKey);

    /* set cipher list */
    if (sCipherList != NULL) 
    {
        sRet = cmnOpenssl::mFuncs.SSL_CTX_set_cipher_list(sSslCtx, sCipherList);
        IDE_TEST_RAISE(sRet == 0, SslOperationFail);
    }
    else
    {
        /* do nothing */
    }

    /* load the certificates of the CA that are trusted by the application */
    if ((sCAFile != NULL) || (sCAPath != NULL))
    {
        sRet = cmnOpenssl::mFuncs.SSL_CTX_load_verify_locations(sSslCtx, sCAFile, sCAPath);
        IDE_TEST_RAISE(sRet == 0, FailedToLoadVerifyLocations);
    }
    else
    {
        ideLog::log(IDE_SERVER_0, "No location for trusted CA certificates is specified.\n");

        /* load the default CA from OS */
        sRet = cmnOpenssl::mFuncs.SSL_CTX_set_default_verify_paths(sSslCtx);
        IDE_TEST_RAISE(sRet == 0, FailedToLoadVerifyLocations);
    }

    /* Disable session caching not to reuse session information. 
     * BUGBUG: Need to check whether the SSL_SESS_CACHE_OFF option does not affect performance */
    cmnOpenssl::mFuncs.SSL_CTX_set_session_cache_mode(sSslCtx, SSL_SESS_CACHE_OFF);

    if (cmuProperty::getSslClientAuthentication() == ID_TRUE)
    {
        sCAList = cmnOpenssl::mFuncs.SSL_load_client_CA_file(sCAFile);
        IDE_TEST_RAISE( sCAList == NULL, FailedToLoadClientCAFile );

        cmnOpenssl::mFuncs.SSL_CTX_set_client_CA_list(sSslCtx, sCAList);

        /* set the verfication flags for ctx.
        * SSL_VERIFY_NONE: The server will not send a client certificate request to the client, 
        *                  so the client will not send a certificate. 
        * SSL_VERIFY_PEER: The server sends a client certificate request to the client. 
        * SSL_VERIFY_FAIL_IF_NO_PEER_CERT: If the client did not return a certificate, 
        *                  the TLS/SSL handshake is immediately terminated with an alert message.
        * SSL_VERIFY_CLIENT_ONCE: Only request a client certificate on the initial SSL/TLS handshake. 
        *                  Do not ask for a client certification again in case of a renegotiation. */
        cmnOpenssl::mFuncs.SSL_CTX_set_verify(sSslCtx, 
                                              SSL_VERIFY_PEER | 
                                              SSL_VERIFY_CLIENT_ONCE | 
                                              SSL_VERIFY_FAIL_IF_NO_PEER_CERT, 
                                              NULL); 
    }
    else
    {
        cmnOpenssl::mFuncs.SSL_CTX_set_verify(sSslCtx, SSL_VERIFY_NONE, NULL);
    }

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

    sLink->mSslCtx = sSslCtx;

    return IDE_SUCCESS;

    IDE_EXCEPTION( SslOperationFail )
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_SSL_OPERATION, cmnOpenssl::getSslErrorMessage()));
    }
    IDE_EXCEPTION( FailedToCreateSslCtx )
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_SSL_OPERATION, cmnOpenssl::getSslErrorMessage()));
    }
    IDE_EXCEPTION( FailedToLoadClientCAFile )
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_INVALID_CA_LIST_FILE, cmnOpenssl::getSslErrorMessage()));
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

    return IDE_FAILURE;
}

IDE_RC cmnLinkListenFinalizeSSL(cmnLink *aLink)
{
    /*
     * socket이 열려있으면 닫음
     */
    IDE_TEST(aLink->mOp->mClose(aLink) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC cmnLinkListenCloseSSL(cmnLink *aLink)
{
    cmnLinkListenSSL *sLink = (cmnLinkListenSSL *)aLink;

    /*
     * socket이 열려있으면 닫음
     */
    if (sLink->mHandle != PDL_INVALID_SOCKET)
    {
        idlOS::closesocket(sLink->mHandle);

        sLink->mHandle = PDL_INVALID_SOCKET;

        cmnOpenssl::mFuncs.SSL_CTX_free(sLink->mSslCtx);
    }
    else
    {
        /* do nothing */
    }

    return IDE_SUCCESS;
}

IDE_RC cmnLinkListenGetHandleSSL(cmnLink *aLink, void *aHandle)
{
    cmnLinkListenSSL *sLink = (cmnLinkListenSSL *)aLink;

    /*
     * socket 을 돌려줌
     */
    *(PDL_SOCKET *)aHandle = sLink->mHandle;

    return IDE_SUCCESS;
}

IDE_RC cmnLinkListenGetDispatchInfoSSL(cmnLink *aLink, void *aDispatchInfo)
{
    cmnLinkListenSSL *sLink = (cmnLinkListenSSL *)aLink;

    /*
     * DispatcherInfo를 돌려줌
     */
    *(UInt *)aDispatchInfo = sLink->mDispatchInfo;

    return IDE_SUCCESS;
}

IDE_RC cmnLinkListenSetDispatchInfoSSL(cmnLink *aLink, void *aDispatchInfo)
{
    cmnLinkListenSSL *sLink = (cmnLinkListenSSL *)aLink;

    /*
     * DispatcherInfo를 세팅
     */
    sLink->mDispatchInfo = *(UInt *)aDispatchInfo;

    return IDE_SUCCESS;
}

IDE_RC cmnLinkListenListenSSL(cmnLinkListen *aLink, cmnLinkListenArg *aListenArg)
{
    cmnLinkListenSSL    *sLink = (cmnLinkListenSSL *)aLink;
    SInt                 sOption = 0;

    struct addrinfo      sHints;
    struct addrinfo     *sAddr = NULL;
    SInt                 sRet = 0;
    SChar                sPortStr[IDL_IP_PORT_MAX_LEN];
    const SChar          *sErrStr = NULL;
    SChar                sErrMsg[256];

    /*
     * socket이 이미 열려있는지 검사
     */
    IDE_TEST_RAISE(sLink->mHandle != PDL_INVALID_SOCKET, SocketAlreadyOpened);

    /* proj-1538 ipv6 */
    idlOS::memset(&sHints, 0x00, ID_SIZEOF(struct addrinfo));
    if (aListenArg->mSSL.mIPv6 == NET_CONN_IP_STACK_V4_ONLY)
    {
        sHints.ai_family   = AF_INET;
    }
    else
    {
        sHints.ai_family   = AF_INET6;
    }
    sHints.ai_socktype = SOCK_STREAM;
    sHints.ai_flags    = AI_PASSIVE;

    idlOS::sprintf(sPortStr, "%"ID_UINT32_FMT"", aListenArg->mSSL.mPort);
    sRet = idlOS::getaddrinfo(NULL, sPortStr, &sHints, &sAddr);
    if ((sRet != 0) || (sAddr == NULL))
    {
        sErrStr = idlOS::gai_strerror(sRet);
        if (sErrStr == NULL)
        {
            idlOS::sprintf(sErrMsg, "%"ID_INT32_FMT, sRet);
        }
        else
        {
            idlOS::sprintf(sErrMsg, "%s", sErrStr);
        }
        IDE_RAISE(GetAddrInfoError);
    }
    else
    {
        /* do nothing */
    }
    
    /* create socket */
    sLink->mHandle = idlOS::socket(sAddr->ai_family,
                                   sAddr->ai_socktype,
                                   sAddr->ai_protocol);
    IDE_TEST_RAISE(sLink->mHandle == PDL_INVALID_SOCKET, SocketError);

    /* SO_REUSEADDR */
    sOption = 1;

    if (idlOS::setsockopt(sLink->mHandle,
                          SOL_SOCKET,
                          SO_REUSEADDR,
                          (SChar *)&sOption,
                          ID_SIZEOF(sOption)) < 0)
    {
        ideLog::log(IDE_SERVER_0, CM_TRC_SOCKET_REUSEADDR_FAIL, errno);
    }
    else
    {
        /* do nothing */
    }

#if defined(IPV6_V6ONLY)
    if ((sAddr->ai_family == AF_INET6) &&
        (aListenArg->mSSL.mIPv6 == NET_CONN_IP_STACK_V6_ONLY))
    {
        sOption = 1;
        if (idlOS::setsockopt(sLink->mHandle, IPPROTO_IPV6, IPV6_V6ONLY,
                              (SChar *)&sOption, ID_SIZEOF(sOption)) < 0)
        {
            idlOS::sprintf(sErrMsg, "IPV6_V6ONLY: %"ID_INT32_FMT, errno);
            IDE_RAISE(SetSockOptError);
        }
        else
        {
            /* do nothing */
        }
    }
    else
    {
        /* do nothing */
    }
#endif

    /* bind */
    if (idlOS::bind(sLink->mHandle,
                    (struct sockaddr *)sAddr->ai_addr,
                    sAddr->ai_addrlen) < 0)
    {
        // bug-34487: print port no
        ideLog::log(IDE_SERVER_0, CM_TRC_TCP_BIND_FAIL, sPortStr, errno);

        IDE_RAISE(BindError);
    }
    else
    {
        /* do nothing */
    }

    /* listen */
    IDE_TEST_RAISE(idlOS::listen(sLink->mHandle, aListenArg->mSSL.mMaxListen) < 0, ListenError);

    if (sAddr != NULL)
    {
        (void)idlOS::freeaddrinfo(sAddr);
        sAddr = NULL;
    }
    else
    {
        /* do nothing */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(SocketAlreadyOpened)
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_SOCKET_ALREADY_OPENED));
    }
    IDE_EXCEPTION(SocketError)
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_SOCKET_OPEN_ERROR, errno));
    }
    IDE_EXCEPTION(BindError)
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_BIND_ERROR, errno));
    }
    IDE_EXCEPTION(ListenError)
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_LISTEN_ERROR));
    }
    IDE_EXCEPTION(GetAddrInfoError)
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_GETADDRINFO_ERROR, sErrMsg));
    }
    IDE_EXCEPTION(SetSockOptError)
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_SETSOCKOPT_ERROR, sErrMsg));
    }
    IDE_EXCEPTION_END;

    if (sAddr != NULL)
    {
        (void)idlOS::freeaddrinfo(sAddr);
        sAddr = NULL;
    }
    else
    {
        /* do nothing */
    }

    return IDE_FAILURE;
}

IDE_RC cmnLinkListenAcceptSSL(cmnLinkListen *aLink, cmnLinkPeer **aLinkPeer)
{
    cmnLinkListenSSL  *sLink     = (cmnLinkListenSSL *)aLink;
    cmnLinkPeer       *sLinkPeer = NULL;
    cmnLinkDescSSL    *sDesc = NULL;
    cmnLinkDescSSL     sTmpDesc;

    SInt                     sRet = 0;
    
    SSL_CTX                 *sSslCtx = sLink->mSslCtx; /* SSL context */
#ifdef DEBUG
    X509                    *sPeerCert = NULL; /* Peer certificateion */
    SChar                    sBuf[512]; /* Temporal buffer for X509 messages */
#endif

    /*
     * 새로운 Link를 할당
     */
    /* BUG-29957
     * cmnLinkAlloc 실패시 Connect를 요청한 Socket을 임시로 accept 해줘야 한다.
     */
    IDE_TEST_RAISE(cmnLinkAlloc((cmnLink **)&sLinkPeer, CMN_LINK_TYPE_PEER_SERVER, CMN_LINK_IMPL_SSL) 
                   != IDE_SUCCESS, LinkError);

    /*
     * Desc 획득
     */
    IDE_TEST_RAISE(sLinkPeer->mPeerOp->mGetDesc(sLinkPeer, &sDesc) != IDE_SUCCESS, LinkError);

    /* TASK-3873 5.3.3 Release Static Analysis Code-sonar */
    /* Code-Sonar가 Function  Pointer를 follow하지 못해서..
       assert를 넣었습니다 */
    IDE_ASSERT( sDesc != NULL);
    
    /*
     * accept
     */
    sDesc->mAddrLen = ID_SIZEOF(sDesc->mAddr);

    sDesc->mHandle = idlOS::accept(sLink->mHandle,
                                   (struct sockaddr *)&(sDesc->mAddr),
                                   &sDesc->mAddrLen);

    IDE_TEST_RAISE(sDesc->mHandle == PDL_INVALID_SOCKET, AcceptError);

    /*
     * Link를 돌려줌
     */
    *aLinkPeer = sLinkPeer;

    /*
     * socket 초기화
     */
    IDE_TEST((*aLinkPeer)->mPeerOp->mSetOptions(*aLinkPeer, SO_NONE) != IDE_SUCCESS);

    /* set SSL */
    sDesc->mSslHandle = cmnOpenssl::mFuncs.SSL_new(sSslCtx);
    IDE_TEST_RAISE(sDesc->mSslHandle == NULL, SslOperationFail);

    sRet = cmnOpenssl::mFuncs.SSL_set_fd(sDesc->mSslHandle, sDesc->mHandle);
    IDE_TEST_RAISE(sRet != 1, SslOperationFail); /* 0: failed, 1: succeeded */
    

    /* SSL handshake */
    sRet = cmnOpenssl::mFuncs.SSL_accept(sDesc->mSslHandle);
    IDE_TEST_RAISE(sRet <= 0, SslHandshakeFail);

#ifdef DEBUG
    ideLog::log(IDE_SERVER_0, "SSL connection using %s\n", 
                cmnOpenssl::mFuncs.SSL_CIPHER_get_name(cmnOpenssl::mFuncs.SSL_get_current_cipher(sDesc->mSslHandle)));


    /* check client's certificate */
    sPeerCert = cmnOpenssl::mFuncs.SSL_get_peer_certificate(sDesc->mSslHandle);
    if (sPeerCert != NULL)
    {   
        /* The peer has a certificate */
        ideLog::log(IDE_SERVER_0, "Peer Certificate\n");

        cmnOpenssl::mFuncs.X509_NAME_oneline(cmnOpenssl::mFuncs.X509_get_subject_name(sPeerCert), sBuf, sizeof(sBuf));
        ideLog::log(IDE_SERVER_0, "\t subject: '%s'", sBuf);

        cmnOpenssl::mFuncs.X509_NAME_oneline(cmnOpenssl::mFuncs.X509_get_issuer_name(sPeerCert), sBuf, sizeof(sBuf));
        ideLog::log(IDE_SERVER_0, "\t issuer: '%s'", sBuf);

        cmnOpenssl::mFuncs.X509_free(sPeerCert);
    }
    else
    {
        /* The peer does not have a certificate */
        ideLog::log(IDE_SERVER_0, "Peer does not have a certificate.");
    }
#endif

    return IDE_SUCCESS;
    
    IDE_EXCEPTION(LinkError)
    {
        /* BUG-29957 */
        // bug-33934: codesonar: 3번째인자 NULL 대신 길이 넘기도록 수정
        sTmpDesc.mAddrLen = ID_SIZEOF(sTmpDesc.mAddr);
        sTmpDesc.mHandle = idlOS::accept(sLink->mHandle,
                                         (struct sockaddr *)&(sTmpDesc.mAddr),
                                         &sTmpDesc.mAddrLen);

        if (sTmpDesc.mHandle != PDL_INVALID_SOCKET)
        {
            idlOS::closesocket(sTmpDesc.mHandle);
        }
        else
        {
            /* do nothing */
        }
    }
    IDE_EXCEPTION(AcceptError)
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_ACCEPT_ERROR));
    }
    IDE_EXCEPTION(SslOperationFail)
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_SSL_OPERATION, cmnOpenssl::getSslErrorMessage()));
    }
    IDE_EXCEPTION(SslHandshakeFail)
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_SSL_HANDSHAKE, cmnOpenssl::getSslErrorMessage()));
    }
    IDE_EXCEPTION_END;
    {
        if (sLinkPeer != NULL)
        {
            IDE_ASSERT(cmnLinkFree((cmnLink *)sLinkPeer) == IDE_SUCCESS);
        }
        else
        {
            /* do nothing */
        }

        *aLinkPeer = NULL;
    }

    return IDE_FAILURE;
}


struct cmnLinkOP gCmnLinkListenOpSSL =
{
    "SSL-LISTEN",

    cmnLinkListenInitializeSSL,
    cmnLinkListenFinalizeSSL,

    cmnLinkListenCloseSSL,

    cmnLinkListenGetHandleSSL,

    cmnLinkListenGetDispatchInfoSSL,
    cmnLinkListenSetDispatchInfoSSL
};

struct cmnLinkListenOP gCmnLinkListenListenOpSSL =
{
    cmnLinkListenListenSSL,
    cmnLinkListenAcceptSSL
};


IDE_RC cmnLinkListenMapSSL(cmnLink *aLink)
{
    cmnLinkListen *sLink = (cmnLinkListen *)aLink;

    /*
     * Link 검사
     */
    IDE_ASSERT(aLink->mType == CMN_LINK_TYPE_LISTEN);
    IDE_ASSERT(aLink->mImpl == CMN_LINK_IMPL_SSL);

    /*
     * 함수 포인터 세팅
     */
    aLink->mOp       = &gCmnLinkListenOpSSL;
    sLink->mListenOp = &gCmnLinkListenListenOpSSL;

    return IDE_SUCCESS;
}

UInt cmnLinkListenSizeSSL()
{
    return ID_SIZEOF(cmnLinkListenSSL);
}


#endif /* CM_DISABLE_SSL */
