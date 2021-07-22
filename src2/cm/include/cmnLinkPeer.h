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

#ifndef _O_CMN_LINK_PEER_H_
#define _O_CMN_LINK_PEER_H_ 1

#include <mqueue.h>

typedef struct cmnLinkPeer
{
    struct cmnLink        mLink;

    struct cmbPool       *mPool;

    idvSession           *mStatistics;
    void                 *mUserPtr;

    struct cmnLinkPeerOP *mPeerOp;
} cmnLinkPeer;

/* PROJ-2616 */
#if defined(ALTI_CFG_OS_LINUX)

#define CMN_IPCDA_MESSAGEQ_NAME_SIZE 64

typedef struct cmnIPCDAMessageQ
{
    SChar          mName[CMN_IPCDA_MESSAGEQ_NAME_SIZE];      /* message queue name */
    struct mq_attr mAttr;                                    /* message queue attribute */
    mqd_t          mId;                                      /* message queue id */
    idBool         mNeedToNotify;
} cmnIPCDAMessageQ;
#endif

/* PROJ-2616 */
typedef struct cmnLinkPeerIPCDA
{
    struct cmnLinkPeer       mLinkPeer;
    struct cmnLinkDescIPCDA  mDesc;
    UInt                     mClientPID;       // IPCDA로 접속한 client의 process idle
#if defined(ALTI_CFG_OS_LINUX)
    cmnIPCDAMessageQ         mMessageQ;
#endif
} cmnLinkPeerIPCDA;

// bug-28277 ipc: server stop failed when idle clis exist
// cmnLinkPeerOP mShutdown 함수에서 3번째 인자로 추가하여 사용됨.
typedef enum
{
    CMN_SHUTDOWN_MODE_NORMAL = 0,
    CMN_SHUTDOWN_MODE_FORCE
} cmnShutdownMode;

struct cmnLinkPeerOP
{
    IDE_RC (*mSetBlockingMode)(cmnLinkPeer *aLink, idBool aBlockingMode);

    IDE_RC (*mGetInfo)(cmnLinkPeer *aLink, SChar *aBuf, UInt aBufLen, cmnLinkInfoKey aKey);
    IDE_RC (*mGetDesc)(cmnLinkPeer *aLink, void *aDesc);

    IDE_RC (*mConnect)(cmnLinkPeer *aLink, cmnLinkConnectArg *aConnectArg, PDL_Time_Value *aTimeout, SInt aOption);
    IDE_RC (*mSetOptions)(cmnLinkPeer *aLink, SInt aOption);

    IDE_RC (*mAllocChannel)(cmnLinkPeer *aLink, SInt *aChannelID);
    IDE_RC (*mHandshake)(cmnLinkPeer *aLink);

    // bug-28277 ipc: server stop failed when idle clis exist
    IDE_RC (*mShutdown)(cmnLinkPeer *aLink, cmnDirection aDirection,
                        cmnShutdownMode aMode);

    IDE_RC (*mRecv)(cmnLinkPeer *aLink, cmbBlock **aBlock, cmpHeader *aHeader, PDL_Time_Value *aTimeout);
    IDE_RC (*mSend)(cmnLinkPeer *aLink, cmbBlock *aBlock);

    IDE_RC (*mReqComplete)(cmnLinkPeer *aLink);
    IDE_RC (*mResComplete)(cmnLinkPeer *aLink);

    IDE_RC (*mCheck)(cmnLinkPeer *aLink, idBool *aIsClosed);

    idBool (*mHasPendingRequest)(cmnLinkPeer *aLink);

    IDE_RC (*mAllocBlock)(cmnLinkPeer *aLink, cmbBlock **aBlock); /* Alloc Block before Write */
    IDE_RC (*mFreeBlock)(cmnLinkPeer *aLink, cmbBlock *aBlock);   /* Free Block after Read */

    /* TASK-5894 Permit sysdba via IPC */
    IDE_RC (*mPermitConnection)(cmnLinkPeer *aLink,
                                idBool       aHasSysdbaViaIPC,
                                idBool       aIsSysdba);

    /* PROJ-2474 SSL/TLS */
    IDE_RC (*mGetSslCipher)(cmnLinkPeer *aLink, SChar *aBuf, UInt aBufLen);
    IDE_RC (*mGetSslPeerCertSubject)(cmnLinkPeer *aLink, SChar *aBuf, UInt aBufLen);
    IDE_RC (*mGetSslPeerCertIssuer)(cmnLinkPeer *aLink, SChar *aBuf, UInt aBufLen);

    /* PROJ-2625 Semi-async Prefetch, Prefetch Auto-tuning */
    IDE_RC (*mGetSndBufSize)(cmnLinkPeer *aLink, SInt *aSndBufSize);
    IDE_RC (*mSetSndBufSize)(cmnLinkPeer *aLink, SInt  aSndBufSize);
    IDE_RC (*mGetRcvBufSize)(cmnLinkPeer *aLink, SInt *aRcvBufSize);
    IDE_RC (*mSetRcvBufSize)(cmnLinkPeer *aLink, SInt  aRcvBufSize);
};


idvSession *cmnLinkPeerGetStatistics(cmnLinkPeer *aLink);
void        cmnLinkPeerSetStatistics(cmnLinkPeer *aLink, idvSession *aStatistics);

void       *cmnLinkPeerGetUserPtr(cmnLinkPeer *aLink);
void        cmnLinkPeerSetUserPtr(cmnLinkPeer *aLink, void *aUserPtr);

/*PROJ-2616*/
IDE_RC      cmnLinkPeerInitSvrWriteIPCDA(void *aCtx);
IDE_RC      cmnLinkPeerInitIPCDA(void *aCtx,UInt aSessionID);

#endif
