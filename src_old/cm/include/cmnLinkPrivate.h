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

#ifndef _O_CMN_LINK_PRIVATE_H_
#define _O_CMN_LINK_PRIVATE_H_ 1

#if !defined(CM_DISABLE_SSL)
#include <openssl/ssl.h>
#endif 



IDE_RC cmnLinkPeerServerMapDUMMY(cmnLink *aLink);
UInt   cmnLinkPeerServerSizeDUMMY();



#if !defined(CM_DISABLE_TCP)

typedef struct cmnLinkDescTCP
{
    PDL_SOCKET              mHandle;
    struct sockaddr_storage mAddr;
    SInt                    mAddrLen; // getnameinfo needs this
} cmnLinkDescTCP;


IDE_RC cmnLinkListenMapTCP(cmnLink *aLink);
UInt   cmnLinkListenSizeTCP();

IDE_RC cmnLinkPeerMapTCP(cmnLink *aLink);
UInt   cmnLinkPeerSizeTCP();

#endif

#if !defined(CM_DISABLE_SSL)

typedef struct cmnLinkDescSSL
{
    PDL_SOCKET              mHandle;
    struct sockaddr_storage mAddr;
    SInt                    mAddrLen; /* getnameinfo needs this */
    SSL_CTX                *mSslCtx; 
    SSL                    *mSslHandle;
} cmnLinkDescSSL;


IDE_RC cmnLinkListenMapSSL(cmnLink *aLink);
UInt   cmnLinkListenSizeSSL();

IDE_RC cmnLinkPeerMapSSL(cmnLink *aLink);
UInt   cmnLinkPeerSizeSSL();

#endif

#if !defined(CM_DISABLE_UNIX)

typedef struct cmnLinkDescUNIX
{
    PDL_SOCKET         mHandle;
    struct sockaddr_un mAddr;
} cmnLinkDescUNIX;


IDE_RC cmnLinkListenMapUNIX(cmnLink *aLink);
UInt   cmnLinkListenSizeUNIX();

IDE_RC cmnLinkPeerMapUNIX(cmnLink *aLink);
UInt   cmnLinkPeerSizeUNIX();

#endif

/* PROJ-2681 */
typedef struct cmnLinkDescIB
{
    PDL_SOCKET              mHandle;
    struct sockaddr_storage mAddr;
    SInt                    mAddrLen; /* getnameinfo needs this */
    UInt                    mLatency; /* for RDMA_LATENCY rsocket option */
    UInt                    mConChkSpin; /* for RDMA_CONCHKSPIN rsocket option */
} cmnLinkDescIB;

IDE_RC cmnLinkListenMapIB(cmnLink *aLink);
UInt   cmnLinkListenSizeIB(void);

IDE_RC cmnLinkPeerMapIB(cmnLink *aLink);
UInt   cmnLinkPeerSizeIB(void);

/*PROJ-2616*/
#if !defined(CM_DISABLE_IPC) && !defined(CM_DISABLE_IPCDA)

#define IPC_SEMCNT_PER_CHANNEL (6)  // semaphore count per ipc channel

#define IPC_SEM_CHECK_SVR_EXIT    (0)  // to detect server dead
#define IPC_SEM_CHECK_CLI_EXIT    (1)  // to detect client dead
#define IPC_SEM_SEND_TO_SVR       (2)  // to use channel in server-side
#define IPC_SEM_SEND_TO_CLI       (3)  // to use channel in client-side
// bug-27250 free Buf list can be crushed when client killed
#define IPC_SEM_SENDMORE_TO_SVR   (4)  // client send more to server
#define IPC_SEM_SENDMORE_TO_CLI   (5)  // server send more to client

#define IPC_SEMA_NUMBER( num ) ( IPC_SEMCNT_PER_CHANNEL * num )

/*PROJ-2616*/
#define IPCDA_SEM_CNT_PER_CHANNEL   (6)  // semaphore count per ipc channel
#define IPCDA_SEM_CHECK_SVR_EXIT    (0)  // to detect server dead
#define IPCDA_SEM_CHECK_CLI_EXIT    (1)  // to detect client dead

#define USE_DISCONN_CHECK

#define USE_NEW_UNDO_SEMA    1   /* 새로운 UNDO Semaphore operation 이용 : PR-2476*/
#define NEW_UNDO_SEMA_VALUE 10

typedef struct cmnLinkDescIPC
{
    PDL_SOCKET         mHandle;
    struct sockaddr_un mAddr;
    SInt               mShmKey;
    PDL_HANDLE         mShmID;
    SInt               mSemChannelKey;
    PDL_HANDLE         mSemChannelID;
    SInt               mSemBufferKey;
    PDL_HANDLE         mSemBufferID;
    SInt               mChannelID;
    struct semid_ds    mSemInfo;

    UInt               mTicketNum;  // BUG-32398 타임스탬프에서 티켓번호로 변경

    /*
     * BUG-12909
     * close시 mHandle에 PDL_INVALID_SOCKET을 세팅하는 대신
     * 이 플래그를 사용한다.
     */
    idBool             mConnectFlag;

    /*
     * HandShake가 실패할 경우 그냥 close()하면 에러 발생하기에
     * 상태를 저장하고, I/O를 수행할 때 필요조치를 함.
     */
    idBool             mHandShakeFlag;

    // bug-28340 rename semop name for readability
    // IPC_SEM_CHECK_SVR_EXIT (0) : controlled by server
    struct sembuf      mOpInitSvrExit[1];    // in Server only
    struct sembuf      mOpCheckSvrExit[2];   // in Client only
    struct sembuf      mOpSignSvrExit[1];    // in Client only

    // IPC_SEM_CHECK_CLI_EXIT (1) : controlled by client
    struct sembuf      mOpInitCliExit[1];    // in Client only
    struct sembuf      mOpCheckCliExit[2];   // in Server only
    struct sembuf      mOpWaitCliExit[2];    // in Server only
    struct sembuf      mOpSignCliExit[1];    // in Server only

    // IPC_SEM_SEND_TO_SVR (2) : controlled by client
    struct sembuf      mOpInitSendToSvr[1];  // in Client only
    struct sembuf      mOpWaitSendToSvr[1];  // in Server only
    struct sembuf      mOpSignSendToSvr[1];  // in Client only

    // IPC_SEM_SEND_TO_CLI (3) : controlled by server
    struct sembuf      mOpInitSendToCli[1];  // in Server only
    struct sembuf      mOpWaitSendToCli[1];  // in Client only
    struct sembuf      mOpSignSendToCli[1];  // in Server only

    // bug-27250 free Buf list can be crushed when client killed
    // 큰 프로토콜에 대한 다음 패킷 송신 신호 제어용 semaphore
    // IPC_SEM_SENDMORE_TO_SVR (4) : controlled by server
    struct sembuf      mOpInitSendMoreToSvr[1];  // in Server
    struct sembuf      mOpCheckSendMoreToSvr[1]; // in Client
    struct sembuf      mOpWaitSendMoreToSvr[2];  // in Client
    struct sembuf      mOpSignSendMoreToSvr[1];  // in Server

    // IPC_SEM_SENDMORE_TO_CLI (5) : controlled by client
    struct sembuf      mOpInitSendMoreToCli[1];  // in Client
    struct sembuf      mOpCheckSendMoreToCli[1]; // in Server
    struct sembuf      mOpWaitSendMoreToCli[2];  // in Server
    struct sembuf      mOpSignSendMoreToCli[1];  // in Client

} cmnLinkDescIPC;

typedef struct cmnLinkDescIPCMsg
{
    UInt               mMaxChannelCount;
    UInt               mMaxBufferCount;
    SInt               mShmKey;
    SInt               mSemChannelKey;
    SInt               mSemBufferKey;
    SInt               mChannelID;
} cmnLinkDescIPCMsg;

/*PROJ-2616*/
typedef struct cmnLinkDescIPCDA
{
    PDL_SOCKET         mHandle;
    struct sockaddr_un mAddr;
    SInt               mShmKey;
    PDL_HANDLE         mShmID;
    SInt               mSemChannelKey;
    PDL_HANDLE         mSemChannelID;
    SInt               mSemBufferKey;
    PDL_HANDLE         mSemBufferID;
    SInt               mChannelID;
    struct semid_ds    mSemInfo;

    UInt               mTicketNum;  /* BUG-32398 타임스탬프에서 티켓번호로 변경 */

    /*
     * BUG-12909
     * close시 mHandle에 PDL_INVALID_SOCKET을 세팅하는 대신
     * 이 플래그를 사용한다.
     */
    idBool             mConnectFlag;

    /*
     * HandShake가 실패할 경우 그냥 close()하면 에러 발생하기에
     * 상태를 저장하고, I/O를 수행할 때 필요조치를 함.
     */
    idBool             mHandShakeFlag;

    /* bug-28340 rename semop name for readability */
    /* IPC_SEM_CHECK_SVR_EXIT (0) : controlled by server */
    struct sembuf      mOpInitSvrExit[1];    /* in Server only */
    struct sembuf      mOpCheckSvrExit[2];   /* in Client only */
    struct sembuf      mOpSignSvrExit[1];    /* in Client only */

    /* IPC_SEM_CHECK_CLI_EXIT (1) : controlled by client */
    struct sembuf      mOpInitCliExit[1];    /* in Client only */
    struct sembuf      mOpCheckCliExit[2];   /* in Server only */
    struct sembuf      mOpWaitCliExit[2];    /* in Server only */
    struct sembuf      mOpSignCliExit[1];    /* in Server only */

} cmnLinkDescIPCDA;

typedef struct cmnLinkDescIPCDAMsg
{
    UInt mMaxChannelCount;
    UInt mMaxBufferCount;
    SInt mShmKey;
    SInt mShmKeyForSimpleQuery;
    SInt mSemChannelKey;
    SInt mSemBufferKey;
    SInt mChannelID;
    UInt mDataBlockSize;
    UInt mUseMessageQ;
} cmnLinkDescIPCDAMsg;

IDE_RC cmnLinkListenMapIPC(cmnLink *aLink);
UInt   cmnLinkListenSizeIPC();

IDE_RC cmnLinkPeerServerMapIPC(cmnLink *aLink);
UInt   cmnLinkPeerServerSizeIPC();

IDE_RC cmnLinkPeerClientMapIPC(cmnLink *aLink);
UInt   cmnLinkPeerClientSizeIPC();

// bug-27250 free Buf list can be crushed when client killed
IDE_RC cmnLinkPeerWaitSendServerIPC(cmnLink* aLink);
IDE_RC cmnLinkPeerWaitSendClientIPC(cmnLink* aLink);

/*PROJ-2616*/
IDE_RC cmnLinkListenMapIPCDA(cmnLink *aLink);
UInt   cmnLinkListenSizeIPCDA();
IDE_RC cmnLinkPeerServerMapIPCDA(cmnLink *aLink);
UInt   cmnLinkPeerServerSizeIPCDA();
IDE_RC cmnLinkPeerClientMapIPCDA(cmnLink *aLink);
UInt   cmnLinkPeerClientSizeIPCDA();
IDE_RC cmnLinkPeerWaitSendServerIPCDA(cmnLink* aLink);
IDE_RC cmnLinkPeerWaitSendClientIPCDA(cmnLink* aLink);

#endif

#endif
