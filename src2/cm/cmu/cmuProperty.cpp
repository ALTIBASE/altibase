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

#include <idl.h>
#include <idp.h>
#include <ide.h>
#include <cm.h>

UInt cmuProperty::mSockWriteTimeout;
UInt cmuProperty::mCmDispatcherSockPollType;

SChar *cmuProperty::mSslCa;
SChar *cmuProperty::mSslCaPath;
SChar *cmuProperty::mSslKey;
SChar *cmuProperty::mSslCert;
UInt   cmuProperty::mSslClientAuthentication;
UInt   cmuProperty::mSslVerifyPeerCert;
SChar *cmuProperty::mSslCipherList;

/*PROJ-2616*/
UInt   cmuProperty::mIPCDASimpleQueryDataBlockSize;
#if defined(ALTI_CFG_OS_LINUX)
idBool   cmuProperty::mIPCDAIsMessageQ;
size_t   cmuProperty::mIPCDAMessageQTimeout;
#endif

/* callback for Socket Write Timeout */
IDE_RC cmuProperty::callbackSockWriteTimeout(idvSQL * /*aStatistics*/,
                                             SChar * /*aName*/, 
                                             void  * /*aOldValue*/,
                                             void  *aNewValue, 
                                             void  * /*aArg*/)
{
    mSockWriteTimeout = *((UInt *)aNewValue);

    return IDE_SUCCESS;
}

void cmuProperty::initialize()
{
}

void cmuProperty::destroy()
{
}

IDE_RC cmuProperty::load()
{
    /* internal properties */
    IDE_ASSERT(idp::read("__SOCK_WRITE_TIMEOUT", &mSockWriteTimeout) == IDE_SUCCESS);

    /* Register Callback Function */
    idp::setupAfterUpdateCallback("__SOCK_WRITE_TIMEOUT",  callbackSockWriteTimeout);

    /* BUG-38951 Support to choice a type of CM dispatcher on run-time */
    IDE_ASSERT(idp::read("CM_DISPATCHER_SOCK_POLL_TYPE",
                         &mCmDispatcherSockPollType) == IDE_SUCCESS);

    /* PROJ-2474 SSL/TLS */
    IDE_ASSERT(idp::readPtr("SSL_CA", (void**)&mSslCa) == IDE_SUCCESS);
    IDE_ASSERT(idp::readPtr("SSL_CAPATH", (void**)&mSslCaPath) == IDE_SUCCESS);
    IDE_ASSERT(idp::readPtr("SSL_KEY", (void**)&mSslKey) == IDE_SUCCESS);
    IDE_ASSERT(idp::readPtr("SSL_CERT", (void**)&mSslCert) == IDE_SUCCESS);
    IDE_ASSERT(idp::readPtr("SSL_CIPHER_LIST", (void**)&mSslCipherList) == IDE_SUCCESS);

    IDE_ASSERT(idp::read("SSL_CLIENT_AUTHENTICATION", &mSslClientAuthentication) == IDE_SUCCESS);
    IDE_ASSERT(idp::read("__SSL_VERIFY_PEER_CERTIFICATE", &mSslVerifyPeerCert) == IDE_SUCCESS);

    /* PROJ-2616 IPCDA는 추가기능의 성격이 있으므로 ASSERT를 발생시키지 않음 */
    if (idp::read("IPCDA_DATABLOCK_SIZE",&mIPCDASimpleQueryDataBlockSize) == IDE_SUCCESS)
    {
        /* Do nothing. */
    }
    else
    {
        mIPCDASimpleQueryDataBlockSize = 20480;
    }

#if defined(ALTI_CFG_OS_LINUX)
    /* BUG-46390 잘못된 옵션 문자 수정 */
    if (idp::read("IPCDA_MESSAGEQ_MODE", &mIPCDAIsMessageQ) == IDE_SUCCESS)
    {
        /* Do nothing */
    }
    else
    {
        mIPCDAIsMessageQ = ID_FALSE;
    }

    if (idp::read("IPCDA_SERVER_MESSAGEQ_TIMEOUT", &mIPCDAMessageQTimeout) == IDE_SUCCESS)
    {
        /* Do nothing. */
    }
    else
    {
        mIPCDAMessageQTimeout = 100;
    }
#endif
    return IDE_SUCCESS;
}
