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

#ifndef _O_CMU_PROPERTY_H_
#define _O_CMU_PROPERTY_H_ 1

class cmuProperty
{
public:
    static UInt mSockWriteTimeout;
    static UInt mCmDispatcherSockPollType;

    /* PROJ-2474 SSL/TLS */
    static SChar *mSslCa;   /* the Certificate Authority */
    static SChar *mSslCaPath;
    static SChar *mSslKey;  /* server private key */
    static SChar *mSslCert; /* server public key */
    static UInt   mSslClientAuthentication; /* Use Mutual Authentication */
    static UInt   mSslVerifyPeerCert; /* Verify Peer Ceritificate */
    static SChar *mSslCipherList; /* SSL cipher list */
    
    /*PROJ-2616*/
    static UInt   mIPCDASimpleQueryDataBlockSize;
#if defined(ALTI_CFG_OS_LINUX)
    static idBool   mIPCDAIsMessageQ;
    static size_t   mIPCDAMessageQTimeout;
#endif

public:
    static void   initialize();
    static void   destroy();
    static IDE_RC load();

    /* getter and callback functions for each property */

    /* __SOCK_WRITE_TIMEOUT */
    static UInt   getSockWriteTimeout() { return mSockWriteTimeout; }
    static IDE_RC callbackSockWriteTimeout(idvSQL * /*aStatistics*/,
                                           SChar *, void *, void *, void*);

    /* BUG-38951 Support to choice a type of CM dispatcher on run-time */
    static UInt   getCmDispatcherSockPollType()
    {
        return mCmDispatcherSockPollType;
    }

    /* PROJ-2474 SSL/TLS */
    static SChar *getSslCa()
    {   
        return mSslCa;
    }   

    static SChar *getSslCaPath()
    {   
        return mSslCaPath;
    }   

    static SChar *getSslKey()
    {   
        return mSslKey;
    }   

    static SChar *getSslCert()
    {   
        return mSslCert;
    }   

    static UInt getSslClientAuthentication()
    {
        return mSslClientAuthentication;
    }

    static UInt getSslVerifyPeerCert()
    {
        return mSslVerifyPeerCert;
    }

    static SChar *getSslCipherList()
    {
        return mSslCipherList;
    }

    /* PROJ-2616 */
    static UInt getIPCDASimpleQueryDataBlockSize()
    {
        return mIPCDASimpleQueryDataBlockSize;
    }

#if defined(ALTI_CFG_OS_LINUX)
    /* PROJ-2616 */
    /* ipcda messageQ */
    static UInt isIPCDAMessageQ()
    {
        return mIPCDAIsMessageQ;
    }

    static UInt getIPCDAMessageQTimeout()
    {
        return mIPCDAMessageQTimeout;
    }
#endif
};

#endif
