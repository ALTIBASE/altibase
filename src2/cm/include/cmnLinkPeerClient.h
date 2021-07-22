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

#ifndef _O_CMN_LINK_PEER_CLIENT_H_
#define _O_CMN_LINK_PEER_CLIENT_H_ 1

/* PROJ-2616 */
#if defined(ALTI_CFG_OS_LINUX)
#include <mqueue.h>
#endif

typedef struct cmnLinkPeer
{
    struct cmnLink        mLink;

    struct cmbPool       *mPool;

    void                 *mUserPtr;

    struct cmnLinkPeerOP *mPeerOp;
} cmnLinkPeer;

/* PROJ-2616 */
#if defined(ALTI_CFG_OS_LINUX)

#define CMN_IPCDA_MESSAGEQ_NAME_SIZE 64

typedef struct cmnIPCDAMessageQ
{
    acp_char_t     mName[CMN_IPCDA_MESSAGEQ_NAME_SIZE]; /* message queue name */
    struct mq_attr mAttr;                               /* message queue attribute */
    mqd_t          mMessageQID;                         /* message queue id */
    acp_bool_t     mUseMessageQ;                        /* message queue 사용여부 */
    acp_uint32_t   mMessageQTimeout;                    /* message queue timeout */
} cmnIPCDAMessageQ;
#endif

/* PROJ-2616 */
typedef struct cmnLinkPeerIPCDA
{
    cmnLinkPeer       mLinkPeer;
    cmnLinkDescIPCDA  mDesc;
    acp_uint32_t      mClientPID;       // IPCDA로 접속한 client의 process ID

#if defined(ALTI_CFG_OS_LINUX)
    cmnIPCDAMessageQ  mMessageQ;
#endif
} cmnLinkPeerIPCDA;

/*
 * bug-28277 ipc: server stop failed when idle clis exist
 * cmnLinkPeerOP mShutdown 함수에서 3번째 인자로 추가하여 사용됨.
 */
typedef enum
{
    CMN_SHUTDOWN_MODE_NORMAL = 0,
    CMN_SHUTDOWN_MODE_FORCE
} cmnShutdownMode;

struct cmnLinkPeerOP
{
    ACI_RC (*mSetBlockingMode)(cmnLinkPeer *aLink, acp_bool_t aBlockingMode);

    ACI_RC (*mGetInfo)(cmnLinkPeer *aLink, acp_char_t *aBuf, acp_uint32_t aBufLen, cmnLinkInfoKey aKey);
    ACI_RC (*mGetDesc)(cmnLinkPeer *aLink, void *aDesc);

    ACI_RC (*mConnect)(cmnLinkPeer *aLink, cmnLinkConnectArg *aConnectArg, acp_time_t aTimeout, acp_sint32_t aOption);
    ACI_RC (*mSetOptions)(cmnLinkPeer *aLink, acp_sint32_t aOption);

    ACI_RC (*mAllocChannel)(cmnLinkPeer *aLink, acp_sint32_t *aChannelID);
    ACI_RC (*mHandshake)(cmnLinkPeer *aLink);

    /*
     * bug-28277 ipc: server stop failed when idle clis exist
     */
    ACI_RC (*mShutdown)(cmnLinkPeer *aLink, cmnDirection aDirection,
                        cmnShutdownMode aMode);

    ACI_RC (*mRecv)(cmnLinkPeer *aLink, cmbBlock **aBlock, cmpHeader *aHeader, acp_time_t aTimeout);
    ACI_RC (*mSend)(cmnLinkPeer *aLink, cmbBlock *aBlock);

    ACI_RC (*mReqComplete)(cmnLinkPeer *aLink);
    ACI_RC (*mResComplete)(cmnLinkPeer *aLink);

    ACI_RC (*mCheck)(cmnLinkPeer *aLink, acp_bool_t *aIsClosed);

    acp_bool_t (*mHasPendingRequest)(cmnLinkPeer *aLink);

    ACI_RC (*mAllocBlock)(cmnLinkPeer *aLink, cmbBlock **aBlock); /* Alloc Block before Write */
    ACI_RC (*mFreeBlock)(cmnLinkPeer *aLink, cmbBlock *aBlock);   /* Free Block after Read */

    /* TASK-5894 Permit sysdba via IPC */
    ACI_RC (*mPermitConnection)(cmnLinkPeer *aLink,
                                acp_bool_t   aHasSysdba,
                                acp_bool_t   aIsSysdba,
                                acp_bool_t  *aPermit);

    /* PROJ-2474 SSL/TLS */
    ACI_RC (*mGetSslCipher)(cmnLinkPeer *aLink, acp_char_t *aBuf, acp_uint32_t aBufLen);
    ACI_RC (*mGetSslPeerCertSubject)(cmnLinkPeer *aLink, acp_char_t *aBuf, acp_uint32_t aBufLen);
    ACI_RC (*mGetSslPeerCertIssuer)(cmnLinkPeer *aLink, acp_char_t *aBuf, acp_uint32_t aBufLen);

    /* PROJ-2625 Semi-async Prefetch, Prefetch Auto-tuning */
    ACI_RC (*mGetSndBufSize)(cmnLinkPeer *aLink, acp_sint32_t *aSndBufSize);
    ACI_RC (*mSetSndBufSize)(cmnLinkPeer *aLink, acp_sint32_t  aSndBufSize);
    ACI_RC (*mGetRcvBufSize)(cmnLinkPeer *aLink, acp_sint32_t *aRcvBufSize);
    ACI_RC (*mSetRcvBufSize)(cmnLinkPeer *aLink, acp_sint32_t  aRcvBufSize);
};


void  *cmnLinkPeerGetUserPtr(cmnLinkPeer *aLink);
void   cmnLinkPeerSetUserPtr(cmnLinkPeer *aLink, void *aUserPtr);
/* BUG-44530 SSL에서 ALTIBASE_SOCK_BIND_ADDR 지원 */
ACI_RC cmnGetAddrInfo(acp_inet_addr_info_t **aAddr,
                      acp_bool_t            *aIsIPAddr,
                      const acp_char_t      *aServer,
                      acp_sint32_t           aPort);

#endif
