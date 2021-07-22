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

#ifndef _O_CMN_LINK_PRIVATE_CLIENT_H_
#define _O_CMN_LINK_PRIVATE_CLIENT_H_ 1

#if !defined(CM_DISABLE_SSL)
#include <cmnOpensslClient.h>
#endif

#if !defined(CM_DISABLE_TCP)

typedef struct cmnLinkDescTCP
{
    acp_sock_t               mSock;
    acp_sock_addr_storage_t  mAddr;
    acp_sock_len_t           mAddrLen; /* getnameinfo use this */
} cmnLinkDescTCP;


ACI_RC       cmnLinkListenMapTCP(cmnLink *aLink);
acp_uint32_t cmnLinkListenSizeTCP();

ACI_RC       cmnLinkPeerMapTCP(cmnLink *aLink);
acp_uint32_t cmnLinkPeerSizeTCP();

#endif


#if !defined(CM_DISABLE_UNIX)

typedef struct cmnLinkDescUNIX
{
    acp_sock_t         mSock;
    struct sockaddr_un mAddr;
} cmnLinkDescUNIX;


ACI_RC       cmnLinkListenMapUNIX(cmnLink *aLink);
acp_uint32_t cmnLinkListenSizeUNIX();

ACI_RC       cmnLinkPeerMapUNIX(cmnLink *aLink);
acp_uint32_t cmnLinkPeerSizeUNIX();

#endif

#if !defined(CM_DISABLE_SSL)

typedef struct cmnLinkDescSSL
{
    acp_sock_t               mSock;
    acp_sock_addr_storage_t  mAddr;
    acp_sock_len_t           mAddrLen; /* getnameinfo use this */
    SSL_CTX                 *mSslCtx; 
    SSL                     *mSslHandle;
} cmnLinkDescSSL;


ACI_RC       cmnLinkPeerMapSSL(cmnLink *aLink);
acp_uint32_t cmnLinkPeerSizeSSL();

#endif

/* PROJ-2681 */
typedef struct cmnLinkDescIB
{
    acp_sint32_t            mSock;
    acp_sock_addr_storage_t mAddr;
    acp_sock_len_t          mAddrLen;      /* getnameinfo needs this */
    acp_bool_t              mBlockingMode;
    acp_uint32_t            mLatency;      /* for RDMA_LATENCY rsocket option */
    acp_uint32_t            mConChkSpin;   /* for RDMA_CONCHKSPIN rsocket option */
} cmnLinkDescIB;

ACI_RC       cmnLinkListenMapIB(cmnLink *aLink);
acp_uint32_t cmnLinkListenSizeIB(void);

ACI_RC       cmnLinkPeerMapIB(cmnLink *aLink);
acp_uint32_t cmnLinkPeerSizeIB(void);

#if !defined(CM_DISABLE_IPC) && !defined(CM_DISABLE_IPCDA)

#define IPC_SEMCNT_PER_CHANNEL    (6)  /* semaphore count per ipc channel */

#define IPC_SEM_CHECK_SVR_EXIT    (0)  /* to detect server dead */
#define IPC_SEM_CHECK_CLI_EXIT    (1)  /* to detect client dead */
#define IPC_SEM_SEND_TO_SVR       (2)  /* to use channel in server-side */
#define IPC_SEM_SEND_TO_CLI       (3)  /* to use channel in client-side */
/*
 * bug-27250 free Buf list can be crushed when client killed
 */
#define IPC_SEM_SENDMORE_TO_SVR   (4)  /* client send more to server */
#define IPC_SEM_SENDMORE_TO_CLI   (5)  /* server send more to client */

#define IPC_SEMA_NUMBER( num ) ( IPC_SEMCNT_PER_CHANNEL * num )

/*PROJ-2616*/
#define IPCDA_SEM_CNT_PER_CHANNEL   (2)  /* semaphore count per ipc channel */
#define IPCDA_SEM_CHECK_SVR_EXIT    (0)  /* to detect server dead */
#define IPCDA_SEM_CHECK_CLI_EXIT    (1)  /* to detect client dead */

#define USE_DISCONN_CHECK

#define USE_NEW_UNDO_SEMA         (1)  /* 새로운 UNDO Semaphore operation 이용 : PR-2476*/
#define NEW_UNDO_SEMA_VALUE      (10)

typedef struct cmnLinkDescIPC
{
    acp_sock_t         mSock;
    struct sockaddr_un mAddr;
    acp_key_t          mShmKey;
    acp_sint32_t       mShmID;
    acp_key_t          mSemChannelKey;
    acp_sint32_t       mSemChannelID;
    acp_key_t          mSemBufferKey;
    acp_sint32_t       mSemBufferID;
    acp_sint32_t       mChannelID;
    struct semid_ds    mSemInfo;

    acp_uint32_t       mTicketNum;  // BUG-32398 타임스탬프에서 티켓번호로 변경

    /*
     * BUG-12909
     * close시 mHandle에 PDL_INVALID_SOCKET을 세팅하는 대신
     * 이 플래그를 사용한다.
     */
    acp_bool_t         mConnectFlag;

    /*
     * HandShake가 실패할 경우 그냥 close()하면 에러 발생하기에
     * 상태를 저장하고, I/O를 수행할 때 필요조치를 함.
     */
    acp_bool_t         mHandShakeFlag;

    /*
     * bug-28340 rename semop name for readability
     * IPC_SEM_CHECK_SVR_EXIT (0) : controlled by server
     */
    struct sembuf      mOpInitSvrExit[1];    /* in Server only */
    struct sembuf      mOpCheckSvrExit[2];   /* in Client only */
    struct sembuf      mOpSignSvrExit[1];    /* in Client only */

    /*
     * IPC_SEM_CHECK_CLI_EXIT (1) : controlled by client
     */
    struct sembuf      mOpInitCliExit[1];    /* in Client only */
    struct sembuf      mOpCheckCliExit[2];   /* in Server only */
    struct sembuf      mOpWaitCliExit[2];    /* in Server only */
    struct sembuf      mOpSignCliExit[1];    /* in Server only */

    /*
     * IPC_SEM_SEND_TO_SVR (2) : controlled by client
     */
    struct sembuf      mOpInitSendToSvr[1];  /* in Client only */
    struct sembuf      mOpWaitSendToSvr[1];  /* in Server only */
    struct sembuf      mOpSignSendToSvr[1];  /* in Client only */

    /*
     * IPC_SEM_SEND_TO_CLI (3) : controlled by server
     */
    struct sembuf      mOpInitSendToCli[1];  /* in Server only */
    struct sembuf      mOpWaitSendToCli[1];  /* in Client only */
    struct sembuf      mOpSignSendToCli[1];  /* in Server only */

    /*
     * bug-27250 free Buf list can be crushed when client killed
     * 큰 프로토콜에 대한 다음 패킷 송신 신호 제어용 semaphore
     * IPC_SEM_SENDMORE_TO_SVR (4) : controlled by server
     */
    struct sembuf      mOpInitSendMoreToSvr[1];  /* in Server */
    struct sembuf      mOpCheckSendMoreToSvr[1]; /* in Client */
    struct sembuf      mOpWaitSendMoreToSvr[2];  /* in Client */
    struct sembuf      mOpSignSendMoreToSvr[1];  /* in Server */

    /*
     * IPC_SEM_SENDMORE_TO_CLI (5) : controlled by client
     */
    struct sembuf      mOpInitSendMoreToCli[1];  /* in Client */
    struct sembuf      mOpCheckSendMoreToCli[1]; /* in Server */
    struct sembuf      mOpWaitSendMoreToCli[2];  /* in Server */
    struct sembuf      mOpSignSendMoreToCli[1];  /* in Client */

} cmnLinkDescIPC;

typedef struct cmnLinkDescIPCMsg
{
    acp_uint32_t       mMaxChannelCount;
    acp_uint32_t       mMaxBufferCount;
    acp_key_t          mShmKey;
    acp_key_t          mSemChannelKey;
    acp_key_t          mSemBufferKey;
    acp_sint32_t       mChannelID;
} cmnLinkDescIPCMsg;

/*PROJ-2616*/
typedef struct cmnLinkDescIPCDA
{
    acp_sock_t         mSock;
    struct sockaddr_un mAddr;
    acp_key_t          mShmKey;
    acp_sint32_t       mShmID;
    acp_key_t          mSemChannelKey;
    acp_sint32_t       mSemChannelID;
    acp_key_t          mSemBufferKey;
    acp_sint32_t       mSemBufferID;
    acp_sint32_t       mChannelID;
    struct semid_ds    mSemInfo;

    acp_uint32_t       mTicketNum;  /* BUG-32398 타임스탬프에서 티켓번호로 변경 */

    /*
     * BUG-12909
     * close시 mHandle에 PDL_INVALID_SOCKET을 세팅하는 대신
     * 이 플래그를 사용한다.
     */
    acp_bool_t         mConnectFlag;

    /*
     * HandShake가 실패할 경우 그냥 close()하면 에러 발생하기에
     * 상태를 저장하고, I/O를 수행할 때 필요조치를 함.
     */
    acp_bool_t         mHandShakeFlag;

    /*
     * bug-28340 rename semop name for readability
     * IPC_SEM_CHECK_SVR_EXIT (0) : controlled by server
     */
    struct sembuf      mOpInitSvrExit[1];    /* in Server only */
    struct sembuf      mOpCheckSvrExit[2];   /* in Client only */
    struct sembuf      mOpSignSvrExit[1];    /* in Client only */

    /*
     * IPC_SEM_CHECK_CLI_EXIT (1) : controlled by client
     */
    struct sembuf      mOpInitCliExit[1];    /* in Client only */
    struct sembuf      mOpCheckCliExit[2];   /* in Server only */
    struct sembuf      mOpWaitCliExit[2];    /* in Server only */
    struct sembuf      mOpSignCliExit[1];    /* in Server only */

} cmnLinkDescIPCDA;

typedef struct cmnLinkDescIPCDAMsg
{
    acp_uint32_t mMaxChannelCount;
    acp_uint32_t mMaxBufferCount;
    acp_key_t    mShmKey;
    acp_key_t    mShmKeyForSimpleQuery;
    acp_key_t    mSemChannelKey;
    acp_key_t    mSemBufferKey;
    acp_sint32_t mChannelID;
    acp_uint32_t mDataBlockSize;
    acp_uint32_t mUseMessageQ;
} cmnLinkDescIPCDAMsg;

ACI_RC       cmnLinkPeerClientMapIPC(cmnLink *aLink);
acp_uint32_t cmnLinkPeerClientSizeIPC();

/*PROJ-2616*/
ACI_RC       cmnLinkPeerClientMapIPCDA(cmnLink *aLink);
acp_uint32_t cmnLinkPeerClientSizeIPCDA();

/*
 * bug-27250 free Buf list can be crushed when client killed
 */
ACI_RC cmnLinkPeerWaitSendClientIPC(cmnLink* aLink);
ACI_RC cmnLinkPeerWaitSendClientIPCDA(cmnLink* aLink);/*PROJ-2616*/

#endif

#endif
