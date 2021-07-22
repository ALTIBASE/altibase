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

#if !defined(CM_DISABLE_IPC)

/*
 * IPC Channel Information
 */
extern acp_thr_mutex_t    gIpcMutex;
extern cmbShmInfo         gIpcShmInfo;
extern acp_sint32_t       gIpcClientCount;
extern acp_sint8_t       *gIpcShmBuffer;
extern acp_key_t          gIpcShmKey;
extern acp_sint32_t       gIpcShmID;
extern acp_key_t         *gIpcSemChannelKey;
extern acp_sint32_t      *gIpcSemChannelID;

/*
 * IPC Handshake Message
 */
#define IPC_HELLO_MSG    "IPC_HELLO"
#define IPC_READY_MSG    "IPC_READY"
#define IPC_PROTO_CNT    (9)

typedef struct cmnLinkPeerIPC
{
    cmnLinkPeer    mLinkPeer;
    cmnLinkDescIPC mDesc;
    acp_sint8_t*   mLastWriteBuffer; /* 클라이언트 비정상 종료시 서버가 write한 마지막 버퍼 주소 */
    acp_sint8_t*   mPrevWriteBuffer; /* 마지막으로 write한 shared memory 내의 버퍼주소 */
    acp_sint8_t*   mPrevReadBuffer;  /* 마지막으로 read한 shared memory 내의 버퍼주소 */
} cmnLinkPeerIPC;

ACI_RC cmnLinkPeerInitializeClientIPC(cmnLink *aLink)
{
    cmnLinkPeerIPC *sLink = (cmnLinkPeerIPC *)aLink;
    cmnLinkDescIPC *sDesc = &sLink->mDesc;

    /*
     * 멤버 초기화
     */

    sLink->mLastWriteBuffer = NULL;
    sLink->mPrevWriteBuffer = NULL;
    sLink->mPrevReadBuffer  = NULL;

    sDesc->mConnectFlag   = ACP_FALSE;
    sDesc->mHandShakeFlag = ACP_FALSE;

    sDesc->mSock.mHandle  = CMN_INVALID_SOCKET_HANDLE;
    sDesc->mChannelID     = -1;

    sDesc->mSemChannelKey = -1;
    sDesc->mSemChannelID  = -1;

    acpMemSet(&sDesc->mSemInfo, 0, ACI_SIZEOF(struct semid_ds));

    return ACI_SUCCESS;
}

ACI_RC cmnLinkPeerFinalizeIPC(cmnLink *aLink)
{
    ACI_TEST(aLink->mOp->mClose(aLink) != ACI_SUCCESS);

    return ACI_SUCCESS;
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

/*
 * !!!!!!!!!!!!!!!!!!!! WARNING !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 * 클라이언트의 종료를 기다릴 때 *절대로* mutex를 잡아서는
 * 안된다. mutex를 잡고 종료를 기다릴 경우 시스템 전체가
 * 정지될 수 있다.
 * PR-4407
 */

ACI_RC cmnLinkPeerCloseClientIPC(cmnLink *aLink)
{
    cmnLinkPeerIPC    *sLink = (cmnLinkPeerIPC *)aLink;
    cmnLinkDescIPC    *sDesc = &sLink->mDesc;
    cmbShmChannelInfo *sChannelInfo;
    acp_sint32_t       rc;
    struct sembuf      sOp;

    /*
     * BUG-34140
     * A IPC channels may be hanged when try to the IPC connecting the server which is starting.
     */
    sOp.sem_num = IPC_SEM_SEND_TO_SVR;
    sOp.sem_op  = 100;
    sOp.sem_flg = 0;

    if(sDesc->mConnectFlag == ACP_TRUE)
    {
        if (sDesc->mHandShakeFlag == ACP_TRUE)
        {
            sChannelInfo = cmbShmGetChannelInfo(gIpcShmBuffer, sDesc->mChannelID);

            /* bug-29324 channel not closed when a client alive after disconn
             * before: timestamp를 비교하여 같은 경우만 종료처리를 수행함.
             * after :
             * 1. 종료처리는 꼭 필요한 과정이므로 timestamp 비교를 제거함.
             * 2. 종료신호 송신 시점(mOpSignCliExit)이 제일 마지막이 되도록 원복
             */

            /* BUG-32398 타임스탬프(티켓번호) 비교부분 복원 */
            if (sDesc->mTicketNum == sChannelInfo->mTicketNum)
            {
                /*
                 * bug-27250 free Buf list can be crushed.
                 * wake up server sending next packet of big data
                 */
                while(1)
                {
                    rc = semop(sDesc->mSemChannelID, sDesc->mOpSignSendMoreToCli, 1);

                    if (rc != 0)
                    {
                        ACI_TEST_RAISE(errno != EINTR, SemOpError);
                    }
                    else
                    {
                        break;
                    }
                }

                while(1)
                {
                    /*
                     * BUG-34140
                     * A IPC channels may be hanged when try to the IPC connecting the server which is starting.
                     */
                    // rc = semop(sDesc->mSemChannelID, sDesc->mOpSignSendToSvr, 1);
                    rc = semop( sDesc->mSemChannelID, &sOp, 1 );

                    if (rc != 0)
                    {
                        ACI_TEST_RAISE(errno != EINTR, SemOpError);
                    }
                    else
                    {
                        break;
                    }
                }

                while(1)
                {
                    rc = semop(sDesc->mSemChannelID, sDesc->mOpSignCliExit, 1);

                    if (rc != 0)
                    {
                        ACI_TEST_RAISE(errno != EINTR, SemOpError);
                    }
                    else
                    {
                        break;
                    }
                }
            }  /* if (sDesc->mTicketNum == sChannelInfo->mTicketNum) */
        }
    }

    /*
     * socket이 열려있으면 닫음
     */

    if (sLink->mDesc.mSock.mHandle != CMN_INVALID_SOCKET_HANDLE)
    {
        ACI_TEST(acpSockClose(&sLink->mDesc.mSock) != ACP_RC_SUCCESS);
        sLink->mDesc.mSock.mHandle = CMN_INVALID_SOCKET_HANDLE;
    }

    sLink->mDesc.mConnectFlag = ACP_FALSE;

    return ACI_SUCCESS;

    ACI_EXCEPTION(SemOpError);
    {
        ACI_SET(aciSetErrorCode(cmERR_FATAL_CMN_SEM_OP));
    }

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC cmnLinkPeerGetSockIPC(cmnLink *aLink, void **aHandle)
{
    ACP_UNUSED(aLink);
    ACP_UNUSED(aHandle);

    return ACI_SUCCESS;
}

ACI_RC cmnLinkPeerGetDispatchInfoIPC(cmnLink * aLink, void * aDispatchInfo)
{
    ACP_UNUSED(aLink);
    ACP_UNUSED(aDispatchInfo);

    return ACI_SUCCESS;
}

ACI_RC cmnLinkPeerSetDispatchInfoIPC(cmnLink * aLink, void * aDispatchInfo)
{
    ACP_UNUSED(aLink);
    ACP_UNUSED(aDispatchInfo);

    return ACI_SUCCESS;
}

ACI_RC cmnLinkPeerSetBlockingModeIPC(cmnLinkPeer * aLink, acp_bool_t aBlockingMode)
{
    ACP_UNUSED(aLink);
    ACP_UNUSED(aBlockingMode);

    return ACI_SUCCESS;
}

ACI_RC cmnLinkPeerGetInfoIPC(cmnLinkPeer * aLink, acp_char_t *aBuf, acp_uint32_t aBufLen, cmiLinkInfoKey aKey)
{
    acp_sint32_t sRet;

    ACP_UNUSED(aLink);

    switch (aKey)
    {
        case CMN_LINK_INFO_ALL:
        case CMN_LINK_INFO_IMPL:
        case CMN_LINK_INFO_IPC_KEY:
            sRet = acpSnprintf(aBuf, aBufLen, "IPC");

            ACI_TEST_RAISE(ACP_RC_IS_ETRUNC(sRet), StringTruncated);
            ACI_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRet), StringOutputError);

            break;

        default:
            ACI_RAISE(UnsupportedLinkInfoKey);
            break;
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION(StringOutputError);
    {
        ACI_SET(aciSetErrorCode(cmERR_ABORT_STRING_OUTPUT_ERROR));
    }
    ACI_EXCEPTION(StringTruncated);
    {
        ACI_SET(aciSetErrorCode(cmERR_IGNORE_STRING_TRUNCATED));
    }
    ACI_EXCEPTION(UnsupportedLinkInfoKey);
    {
        ACI_SET(aciSetErrorCode(cmERR_ABORT_UNSUPPORTED_LINK_INFO_KEY));
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC cmnLinkPeerGetDescIPC(cmnLinkPeer *aLink, void *aDesc)
{
    cmnLinkPeerIPC *sLink = (cmnLinkPeerIPC *)aLink;

    /*
     *  Desc를 돌려줌
     */
    *(cmnLinkDescIPC **)aDesc = &sLink->mDesc;

    return ACI_SUCCESS;
}

ACI_RC cmnLinkPeerConnectIPC(cmnLinkPeer       *aLink,
                             cmnLinkConnectArg *aConnectArg,
                             acp_time_t         aTimeout,
                             acp_sint32_t       aOption)
{
    cmnLinkPeerIPC    *sLink = (cmnLinkPeerIPC *)aLink;
    acp_rc_t           sRet;

    ACP_UNUSED(aOption);

    /*
     * socket 생성
     */

    sRet = acpSockOpen(&sLink->mDesc.mSock,
                       ACP_AF_UNIX,
                       ACP_SOCK_STREAM,
                       0);
    ACI_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRet), SocketError);

    /*
     * IPC 주소 세팅
     */

    sLink->mDesc.mAddr.sun_family = AF_UNIX;
    sRet = acpSnprintf(sLink->mDesc.mAddr.sun_path,
                       ACI_SIZEOF(sLink->mDesc.mAddr.sun_path),
                       "%s",
                       aConnectArg->mIPC.mFilePath);

    /*
     *  IPC 파일이름 길이 검사
     */

    ACI_TEST_RAISE(ACP_RC_IS_ETRUNC(sRet), IpcPathTruncated);
    ACI_TEST(ACP_RC_NOT_SUCCESS(sRet));

    /*
     * connect
     */

    sRet = acpSockTimedConnect(&sLink->mDesc.mSock,
                               (acp_sock_addr_t *)(&sLink->mDesc.mAddr),
                               ACI_SIZEOF(sLink->mDesc.mAddr),
                               aTimeout);
    ACI_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRet), ConnectError);

    sLink->mDesc.mConnectFlag = ACP_TRUE;

    return ACI_SUCCESS;

    ACI_EXCEPTION(SocketError);
    {
        ACI_SET(aciSetErrorCode(cmERR_ABORT_SOCKET_OPEN_ERROR, sRet));
    }
    ACI_EXCEPTION(IpcPathTruncated);
    {
        ACI_SET(aciSetErrorCode(cmERR_ABORT_UNIX_PATH_TRUNCATED));
    }
    ACI_EXCEPTION(ConnectError);
    {
        ACI_SET(aciSetErrorCode(cmERR_ABORT_CONNECT_ERROR, sRet));
    }
    ACI_EXCEPTION_END;

    if (sLink->mDesc.mSock.mHandle != CMN_INVALID_SOCKET_HANDLE)
    {
        (void)acpSockClose(&sLink->mDesc.mSock);
        sLink->mDesc.mSock.mHandle = CMN_INVALID_SOCKET_HANDLE;
    }

    return ACI_FAILURE;
}

ACI_RC cmnLinkPeerAllocChannelClientIPC(cmnLinkPeer *aLink, acp_sint32_t *aChannelID)
{
    ACP_UNUSED(aLink);
    ACP_UNUSED(aChannelID);

    return ACI_FAILURE;
}

ACI_RC cmnLinkPeerSetOperation(cmnLinkDescIPC *aDesc)
{
    /*
     * =====================================================
     * bug-28340 rename semop name for readability
     * 가독성을 위해 semaphore op 변수명을 다음과 같이 변경
     * =====================================================
     * IPC_SEM_SERVER_DETECT  > IPC_SEM_CHECK_SVR_EXIT (0)
     * server_detect_init     > InitSvrExit  : 서버가 잡고 시작
     * server_detect_try      > CheckSvrExit : 서버가 죽었는지 확인
     * server_detect_release  > SignSvrExit  : 서버가 종료신호 보냄
     * =====================================================
     * IPC_SEM_CLIENT_DETECT  > IPC_SEM_CHECK_CLI_EXIT (1)
     * client_detect_init     > InitCliExit  : cli가 잡고 시작
     * client_detect_try      > CheckCliExit : cli가 죽었는지 확인
     * client_detect_hold     > WaitCliExit  : cli 종료때까지 대기
     * client_detect_release  > SignCliExit  : cli가 종료신호 보냄
     * =====================================================
     * IPC_SEM_SERVER_CHANNEL > IPC_SEM_SEND_TO_SVR (2)
     * server_channel_init    > InitSendToSvr :cli가 잡고 시작
     * server_channel_hold    > WaitSendToSvr :서버가 수신대기
     * server_channel_release > SignSendToSvr :cli가 서버수신허락
     * =====================================================
     * IPC_SEM_CLIENT_CHANNLE > IPC_SEM_SEND_TO_CLI (3)
     * client_channel_init    > InitSendToCli :서버가 잡고 시작
     * client_channel_hold    > WaitSendToCli :cli가 수신대기
     * client_channel_release > SignSendToCli :서버가 cli수신허락
     * =====================================================
     * IPC_SEM_CLI_SENDMORE  > IPC_SEM_SENDMORE_TO_SVR (4)
     * cli_sendmore_init     > InitSendMoreToSvr  :서버가 잡고 시작
     * cli_sendmore_try      > CheckSendMoreToSvr :cli가 송신시도
     * cli_sendmore_hold     > WaitSendMoreToSvr  :cli가 송신대기
     * cli_sendmore_release  > SignSendMoreToSvr  :서버가 송신허락
     * =====================================================
     * IPC_SEM_SVR_SENDMORE  > IPC_SEM_SENDMORE_TO_CLI (5)
     * svr_sendmore_init     > InitSendMoreToCli  :cli가 잡고 시작
     * svr_sendmore_try      > CheckSendMoreToCli :서버가 송신시도
     * svr_sendmore_hold     > WaitSendMoreToCli  :서버가 송신대기
     * svr_sendmore_release  > SignSendMoreToCli  :cli가 송신허락
     * =====================================================
     */

    /*
     * =====================================================
     * IPC_SEM_CHECK_SVR_EXIT (0)
     */
    aDesc->mOpInitSvrExit[0].sem_num = IPC_SEM_CHECK_SVR_EXIT;
    aDesc->mOpInitSvrExit[0].sem_op  = -1;
    aDesc->mOpInitSvrExit[0].sem_flg = SEM_UNDO;

    aDesc->mOpCheckSvrExit[0].sem_num = IPC_SEM_CHECK_SVR_EXIT;
    aDesc->mOpCheckSvrExit[0].sem_op  = -1;
    aDesc->mOpCheckSvrExit[0].sem_flg = IPC_NOWAIT;
    aDesc->mOpCheckSvrExit[1].sem_num = IPC_SEM_CHECK_SVR_EXIT;
    aDesc->mOpCheckSvrExit[1].sem_op  = 1;
    aDesc->mOpCheckSvrExit[1].sem_flg = 0;

    aDesc->mOpSignSvrExit[0].sem_num = IPC_SEM_CHECK_SVR_EXIT;
    aDesc->mOpSignSvrExit[0].sem_op  = 1;
    aDesc->mOpSignSvrExit[0].sem_flg = 0;

    /*
     * =====================================================
     * IPC_SEM_CHECK_CLI_EXIT (1)
     */
    aDesc->mOpInitCliExit[0].sem_num = IPC_SEM_CHECK_CLI_EXIT;
    aDesc->mOpInitCliExit[0].sem_op  = -1;
    aDesc->mOpInitCliExit[0].sem_flg = SEM_UNDO;

    aDesc->mOpCheckCliExit[0].sem_num = IPC_SEM_CHECK_CLI_EXIT;
    aDesc->mOpCheckCliExit[0].sem_op  = -1;
    aDesc->mOpCheckCliExit[0].sem_flg = IPC_NOWAIT;
    aDesc->mOpCheckCliExit[1].sem_num = IPC_SEM_CHECK_CLI_EXIT;
    aDesc->mOpCheckCliExit[1].sem_op  =  1;
    aDesc->mOpCheckCliExit[1].sem_flg = 0;

    aDesc->mOpWaitCliExit[0].sem_num = IPC_SEM_CHECK_CLI_EXIT;
    aDesc->mOpWaitCliExit[0].sem_op  = -1;
    aDesc->mOpWaitCliExit[0].sem_flg = 0;
    aDesc->mOpWaitCliExit[1].sem_num = IPC_SEM_CHECK_CLI_EXIT;
    aDesc->mOpWaitCliExit[1].sem_op  =  1;
    aDesc->mOpWaitCliExit[1].sem_flg = 0;

    aDesc->mOpSignCliExit[0].sem_num = IPC_SEM_CHECK_CLI_EXIT;
    aDesc->mOpSignCliExit[0].sem_op  = 1;
    aDesc->mOpSignCliExit[0].sem_flg = 0;

    /* =====================================================
     * IPC_SEM_SEND_TO_SVR (2)
     */
    aDesc->mOpInitSendToSvr[0].sem_num = IPC_SEM_SEND_TO_SVR;
    aDesc->mOpInitSendToSvr[0].sem_op  = (-1 * gIpcShmInfo.mMaxBufferCount);
    aDesc->mOpInitSendToSvr[0].sem_flg = SEM_UNDO;

    aDesc->mOpWaitSendToSvr[0].sem_num = IPC_SEM_SEND_TO_SVR;
    aDesc->mOpWaitSendToSvr[0].sem_op  = -1;
    aDesc->mOpWaitSendToSvr[0].sem_flg = 0;

    aDesc->mOpSignSendToSvr[0].sem_num = IPC_SEM_SEND_TO_SVR;
    aDesc->mOpSignSendToSvr[0].sem_op  = 1;
    aDesc->mOpSignSendToSvr[0].sem_flg = 0;

    /* =====================================================
     * IPC_SEM_SEND_TO_CLI (3)
     */
    aDesc->mOpInitSendToCli[0].sem_num = IPC_SEM_SEND_TO_CLI;
    aDesc->mOpInitSendToCli[0].sem_op  = (-1 * gIpcShmInfo.mMaxBufferCount);
    aDesc->mOpInitSendToCli[0].sem_flg = SEM_UNDO;

    aDesc->mOpWaitSendToCli[0].sem_num = IPC_SEM_SEND_TO_CLI;
    aDesc->mOpWaitSendToCli[0].sem_op  = -1;
    aDesc->mOpWaitSendToCli[0].sem_flg = 0;

    aDesc->mOpSignSendToCli[0].sem_num = IPC_SEM_SEND_TO_CLI;
    aDesc->mOpSignSendToCli[0].sem_op  = 1;
    aDesc->mOpSignSendToCli[0].sem_flg = 0;

    /*
     * bug-27250 free Buf list can be crushed when client killed
     * 큰 프로토콜에 대한 다음 패킷 송신 신호 제어용 semaphore
     * =====================================================
     * IPC_SEM_SENDMORE_TO_SVR (4)
     */
    aDesc->mOpInitSendMoreToSvr[0].sem_num = IPC_SEM_SENDMORE_TO_SVR;
    aDesc->mOpInitSendMoreToSvr[0].sem_op  = (-1 * gIpcShmInfo.mMaxBufferCount);
    aDesc->mOpInitSendMoreToSvr[0].sem_flg = SEM_UNDO;

    aDesc->mOpCheckSendMoreToSvr[0].sem_num = IPC_SEM_SENDMORE_TO_SVR;
    aDesc->mOpCheckSendMoreToSvr[0].sem_op  = -1;
    aDesc->mOpCheckSendMoreToSvr[0].sem_flg = IPC_NOWAIT;

    /*
     * cmnDispatcherWaitLink 에서 무한대기 하기위해 사용.
     */
    aDesc->mOpWaitSendMoreToSvr[0].sem_num = IPC_SEM_SENDMORE_TO_SVR;
    aDesc->mOpWaitSendMoreToSvr[0].sem_op  = -1;
    aDesc->mOpWaitSendMoreToSvr[0].sem_flg = 0;
    aDesc->mOpWaitSendMoreToSvr[1].sem_num = IPC_SEM_SENDMORE_TO_SVR;
    aDesc->mOpWaitSendMoreToSvr[1].sem_op  = 1;
    aDesc->mOpWaitSendMoreToSvr[1].sem_flg = 0;

    aDesc->mOpSignSendMoreToSvr[0].sem_num = IPC_SEM_SENDMORE_TO_SVR;
    aDesc->mOpSignSendMoreToSvr[0].sem_op  = 1;
    aDesc->mOpSignSendMoreToSvr[0].sem_flg = 0;

    /*
     * =====================================================
     * IPC_SEM_SENDMORE_TO_CLI (5)
     */
    aDesc->mOpInitSendMoreToCli[0].sem_num = IPC_SEM_SENDMORE_TO_CLI;
    aDesc->mOpInitSendMoreToCli[0].sem_op  = (-1 * gIpcShmInfo.mMaxBufferCount);
    aDesc->mOpInitSendMoreToCli[0].sem_flg = SEM_UNDO;

    aDesc->mOpCheckSendMoreToCli[0].sem_num = IPC_SEM_SENDMORE_TO_CLI;
    aDesc->mOpCheckSendMoreToCli[0].sem_op  = -1;
    aDesc->mOpCheckSendMoreToCli[0].sem_flg = IPC_NOWAIT;

    /*
     * cmnDispatcherWaitLink 에서 무한대기 하기위해 사용.
     */
    aDesc->mOpWaitSendMoreToCli[0].sem_num = IPC_SEM_SENDMORE_TO_CLI;
    aDesc->mOpWaitSendMoreToCli[0].sem_op  = -1;
    aDesc->mOpWaitSendMoreToCli[0].sem_flg = 0;
    aDesc->mOpWaitSendMoreToCli[1].sem_num = IPC_SEM_SENDMORE_TO_CLI;
    aDesc->mOpWaitSendMoreToCli[1].sem_op  = 1;
    aDesc->mOpWaitSendMoreToCli[1].sem_flg = 0;

    aDesc->mOpSignSendMoreToCli[0].sem_num = IPC_SEM_SENDMORE_TO_CLI;
    aDesc->mOpSignSendMoreToCli[0].sem_op  = 1;
    aDesc->mOpSignSendMoreToCli[0].sem_flg = 0;

    return ACI_SUCCESS;
}

ACI_RC cmnLinkPeerSyncClientIPC(cmnLinkDescIPC *aDesc)
{
    acp_sint32_t rc;

    /*
     * 시작하기 위한 동기화
     */
    while(1)
    {
        rc = semop(aDesc->mSemChannelID, aDesc->mOpWaitSendToCli, 1);
        if (rc == 0)
        {
            break;
        }
        else
        {
            ACI_TEST_RAISE(errno != EINTR, SemOpError);
        }
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION(SemOpError);
    {
        ACI_SET(aciSetErrorCode(cmERR_FATAL_CMN_SEM_OP));
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC cmnLinkPeerHandshakeClientIPC(cmnLinkPeer *aLink)
{
    cmnLinkPeerIPC *sLink = (cmnLinkPeerIPC *)aLink;
    cmnLinkDescIPC *sDesc = &sLink->mDesc;

    acp_sint32_t       rc;
    acp_size_t         sCount = 0;
    acp_sint32_t       sFlag = 0666;
    acp_sint32_t       sSemCount = 0;
    acp_sint8_t        sProtoBuffer[20];
    acp_bool_t         sLocked = ACP_FALSE;
    cmnLinkDescIPCMsg  sIpcMsg;
    cmbShmChannelInfo *sChannelInfo;
    // BUG-32625 32bit Client IPC Connection Error
    union semun {
        struct semid_ds *buf;
    } sArg;

    /*
     * send hello message
     */

    acpMemCpy(sProtoBuffer, IPC_HELLO_MSG, IPC_PROTO_CNT);

    ACI_TEST(acpSockSend(&sDesc->mSock,
                         sProtoBuffer,
                         IPC_PROTO_CNT,
                         &sCount,
                         0) != ACP_RC_SUCCESS);
    ACI_TEST( sCount != IPC_PROTO_CNT);

    /*
     * receive ipc protocol message
     */

    ACI_TEST(acpSockRecv(&sDesc->mSock,
                         &sIpcMsg,
                         ACI_SIZEOF(cmnLinkDescIPCMsg),
                         &sCount,
                         0) != ACP_RC_SUCCESS);

    ACI_TEST(sCount != ACI_SIZEOF(cmnLinkDescIPCMsg));

    ACI_TEST_RAISE(sIpcMsg.mChannelID < 0, NoChannelError);

    /*
     * CAUTION: don't move
     * if do, it causes network deadlock in MT_threaded Clinets
     */

    ACI_TEST(acpThrMutexLock(&gIpcMutex) != ACP_RC_SUCCESS);

    sLocked = ACP_TRUE;

    sDesc->mChannelID = sIpcMsg.mChannelID;

    if (gIpcShmInfo.mMaxChannelCount != sIpcMsg.mMaxChannelCount)
    {
        gIpcShmInfo.mMaxChannelCount = sIpcMsg.mMaxChannelCount;
    }

    if (gIpcShmInfo.mMaxBufferCount != sIpcMsg.mMaxBufferCount)
    {
        gIpcShmInfo.mMaxBufferCount = sIpcMsg.mMaxBufferCount;
    }

    if (gIpcShmKey != sIpcMsg.mShmKey)
    {
        gIpcShmKey           = sIpcMsg.mShmKey;
        sLink->mDesc.mShmKey = sIpcMsg.mShmKey;

        gIpcShmInfo.mMaxChannelCount  = sIpcMsg.mMaxChannelCount;
        gIpcShmInfo.mUsedChannelCount = 0;

        gIpcShmID = -1;
        ACI_TEST(cmbShmDetach() != ACI_SUCCESS);
    }

    /*
     * attach shared memory
     */
    ACI_TEST(cmbShmAttach() != ACI_SUCCESS);

    /*
     * attach channel semaphores
     */

    if (sDesc->mSemChannelKey != sIpcMsg.mSemChannelKey)
    {
        sDesc->mSemChannelKey = sIpcMsg.mSemChannelKey;
        sSemCount             = IPC_SEMCNT_PER_CHANNEL;
        sDesc->mSemChannelID  = -1;
    }

    if (sDesc->mSemChannelID == -1)
    {
        /*
         * BUGBUG
         */
        sDesc->mSemChannelID = semget(sDesc->mSemChannelKey, sSemCount, sFlag);
        ACI_TEST(sLink->mDesc.mSemChannelID < 0);
    }

    cmnLinkPeerSetOperation(sDesc);

    // BUG-32625 32bit Client IPC Connection Error
    sArg.buf = &sDesc->mSemInfo;
    ACI_TEST(semctl(sDesc->mSemChannelID, 0, IPC_STAT, sArg) != 0);

    /*
     * set client semaphore to detect client dead
     */

    while(1)
    {
        rc = semop(sDesc->mSemChannelID, sDesc->mOpInitCliExit, 1);
        if (rc == 0)
        {
            break;
        }
        else
        {
            ACI_TEST(errno != EINTR);
        }
    }

    /*
     * hold server channel to prevent server's starting
     */

    while(1)
    {
        rc = semop(sDesc->mSemChannelID, sDesc->mOpInitSendToSvr, 1);
        if (rc == 0)
        {
            break;
        }
        else
        {
            ACI_TEST(errno != EINTR);
        }
    }

    /*
     * bug-27250 free Buf list can be crushed when client killed
     * hold server send for big protocol
     */
    while(1)
    {
        rc = semop(sDesc->mSemChannelID, sDesc->mOpInitSendMoreToCli, 1);
        if (rc == 0)
        {
            break;
        }
        else
        {
            ACI_TEST(errno != EINTR);
        }
    }

    /*
     * send ready msg
     */

    acpMemCpy(sProtoBuffer, IPC_READY_MSG, IPC_PROTO_CNT);

    ACI_TEST(acpSockSend(&sDesc->mSock,
                         sProtoBuffer,
                         IPC_PROTO_CNT,
                         &sCount,
                         0) != ACP_RC_SUCCESS);

    ACI_TEST( sCount != IPC_PROTO_CNT);

    gIpcClientCount++;

    sChannelInfo = cmbShmGetChannelInfo(gIpcShmBuffer, sDesc->mChannelID);
    sDesc->mTicketNum = sChannelInfo->mTicketNum;

    /*
     * BUG-25420 [CodeSonar] Lock, Unlock 에러 핸들링 오류에 의한 Double Unlock
     * unlock 를 하기전에 세팅을해야 Double Unlock 을 막을수 있다.
     */
    sLocked = ACP_FALSE;
    rc = acpThrMutexUnlock(&gIpcMutex);
    if (ACP_RC_NOT_SUCCESS(rc))
    {
        gIpcClientCount--;
        ACI_RAISE(ACI_EXCEPTION_END_LABEL);
    }

    /*
     * Sync after Establish Connection
     */

    ACI_TEST(cmnLinkPeerSyncClientIPC(sDesc) != ACI_SUCCESS);

    sDesc->mHandShakeFlag = ACP_TRUE;

    /*
     * fix BUG-18833
     */
    if (sDesc->mSock.mHandle != CMN_INVALID_SOCKET_HANDLE)
    {
        ACI_TEST(acpSockClose(&sLink->mDesc.mSock) != ACP_RC_SUCCESS);
        sDesc->mSock.mHandle = CMN_INVALID_SOCKET_HANDLE;
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION(NoChannelError);
    {
        /* 
         * TASK-5894 Permit sysdba via IPC
         *
         * cm-ipc 소켓이 열려있지 않은 것과 IPC 채널이 없는 것을 구분한다.
         */
        ACI_SET(aciSetErrorCode(cmERR_ABORT_CMN_ERR_FULL_IPC_CHANNEL));
    }
    ACI_EXCEPTION_END;

    if (sLocked == ACP_TRUE)
    {
        ACE_ASSERT(acpThrMutexUnlock(&gIpcMutex) == ACP_RC_SUCCESS);
    }

    return ACI_FAILURE;
}

ACI_RC cmnLinkPeerSetOptionsIPC(cmnLinkPeer * aLink, acp_sint32_t aOption)
{
    ACP_UNUSED(aLink);
    ACP_UNUSED(aOption);

    return ACI_SUCCESS;
}

ACI_RC cmnLinkPeerShutdownClientIPC(cmnLinkPeer    *aLink,
                                    cmnDirection    aDirection,
                                    cmnShutdownMode aMode)
{
    cmnLinkPeerIPC    *sLink = (cmnLinkPeerIPC *)aLink;
    cmnLinkDescIPC    *sDesc = &sLink->mDesc;
    cmbShmChannelInfo *sChannelInfo;
    acp_sint32_t       rc;

    ACP_UNUSED(aDirection);
    ACP_UNUSED(aMode);

    if (sDesc->mHandShakeFlag == ACP_TRUE)
    {
        /*
         * clinet의 close flag를 on : 즉, detect sem = 1
         */

        sChannelInfo = cmbShmGetChannelInfo(gIpcShmBuffer, sDesc->mChannelID);

        /* BUG-32398 타임스탬프(티켓번호) 비교부분 복원 */
        if (sDesc->mTicketNum == sChannelInfo->mTicketNum)
        {
            /*
             * Client의 Connection이 유효할 경우에만 close 연산을 수행해라.
             * bug-27162: ipc server,client hang
             * 순서 변경
             * why: server read에서 client 종료 감지부분 있음
             * before:
             * release server read waiting -> mark client exited
             * after:
             * mark client exited -> release server read waiting
             */

            while(1)
            {
                rc = semop(sDesc->mSemChannelID,
                           sDesc->mOpSignCliExit,
                           1);

                if (rc != 0)
                {
                    ACI_TEST_RAISE(errno != EINTR, SemOpError);
                }
                else
                {
                    break;
                }
            }

            /*
             * bug-27250 free Buf list can be crushed when client killed.
             * wake up server sending next packet of big data
             */
            while(1)
            {
                rc = semop(sDesc->mSemChannelID,
                           sDesc->mOpSignSendMoreToCli,
                           1);
                if (rc != 0)
                {
                    ACI_TEST_RAISE(errno != EINTR, SemOpError);
                }
                else
                {
                    break;
                }
            }

            while(1)
            {
                rc = semop(sDesc->mSemChannelID,
                           sDesc->mOpSignSendToSvr,
                           1);

                if (rc != 0)
                {
                    ACI_TEST_RAISE(errno != EINTR, SemOpError);
                }
                else
                {
                    break;
                }
            }
        }
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION(SemOpError);
    {
        ACI_SET(aciSetErrorCode(cmERR_FATAL_CMN_SEM_OP));
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC cmnLinkPeerAllocBlockClientIPC(cmnLinkPeer *aLink, cmbBlock **aBlock)
{
    /*
     * bug-27250 free Buf list can be crushed when client killed
     * modify: to be same as tcp
     * 변경전: 공유 메모리 block을 할당
     * 변경후:  mAllocBlock에서 일반 block buffer를 할당
     */
    cmbBlock *sBlock;

    ACI_TEST(aLink->mPool->mOp->mAllocBlock(aLink->mPool, &sBlock) != ACI_SUCCESS);

    /*
     * Write Block 초기화
     */
    sBlock->mDataSize    = CMP_HEADER_SIZE;
    sBlock->mCursor      = CMP_HEADER_SIZE;

    /*
     *  Write Block을 돌려줌
     */
    *aBlock = sBlock;

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

ACI_RC cmnLinkPeerFreeBlockClientIPC(cmnLinkPeer *aLink, cmbBlock *aBlock)
{
    /*
     * bug-27250 free Buf list can be crushed when client killed
     * 공유 메모리 linked list 조정하는 부분 제거
     * Block 해제
     */
    ACI_TEST(aLink->mPool->mOp->mFreeBlock(aLink->mPool, aBlock) != ACI_SUCCESS);

    return ACI_SUCCESS;
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

ACI_RC cmnLinkPeerRecvClientIPC(cmnLinkPeer    *aLink,
                                cmbBlock      **aBlock,
                                cmpHeader      *aHeader,
                                acp_time_t      aTimeout)
{
    /*
     * semaphore는 mutex가 아니다!!!!
     *
     * lock을 획득, 반환하는 개념이 아니라
     * 사용할 수 있는 자원의 갯수로 판단한다.
     *
     * write : 무조건 쓰고 +1
     * read  : -1 후 읽기
     */

    cmnLinkPeerIPC *sLink = (cmnLinkPeerIPC *)aLink;
    cmnLinkDescIPC *sDesc = &sLink->mDesc;

    acp_sint32_t       rc;
    cmbBlock          *sBlock = *aBlock; /* proj_2160 cm_type removal */
    acp_sint8_t       *sDataBlock;
    cmpHeader          sHeader;
    cmbShmChannelInfo *sChannelInfo;

    ACP_UNUSED(aTimeout);

    ACI_TEST(sDesc->mConnectFlag != ACP_TRUE || sDesc->mHandShakeFlag != ACP_TRUE);

    /*
     * check ghost-connection in shutdown state() : PR-4096
     */
    sChannelInfo = cmbShmGetChannelInfo(gIpcShmBuffer, sDesc->mChannelID);
    ACI_TEST_RAISE(sDesc->mTicketNum != sChannelInfo->mTicketNum, GhostConnection);

    while(1)
    {
        rc = semop(sDesc->mSemChannelID, sDesc->mOpWaitSendToCli, 1);
        if (rc == 0)
        {
            break;
        }
        else
        {
            ACI_TEST_RAISE(EINTR != errno, SemOpError);
        }
    }

    /*
     * disconnect check
     */

    rc = semop(sDesc->mSemChannelID, sDesc->mOpCheckSvrExit, 2);
    ACI_TEST_RAISE(rc == 0, ConnectionClosed);


    /*
     * ======================================================
     * bug-27250 free Buf list can be crushed when client killed
     * 변경전: 고정 또는 여분의 block 얻어옴.
     * 변경후: 무조건 고정된 통신 block 얻어옴.
     */

    sDataBlock = cmbShmGetDefaultClientBuffer(gIpcShmBuffer,
                                              gIpcShmInfo.mMaxChannelCount,
                                              sDesc->mChannelID);

    /*
     * 헤더를 먼저 읽어 data size를 구한 다음에 해당 size 만큼
     * buffer에 복사해 둔다. 다른 buffer로 복사하는 이유는,
     * 이후 marshal 단계에서 복사가 아닌 포인터를 사용하기 때문.
     */

    /*
     * read header
     */
    acpMemCpy(sBlock->mData, sDataBlock, CMP_HEADER_SIZE);
    sBlock->mDataSize    = CMP_HEADER_SIZE;
    /*
     * Protocol Header 해석
     */
    ACI_TEST(cmpHeaderRead(aLink, &sHeader, sBlock) != ACI_SUCCESS);

    /*
     * read payload
     */
    acpMemCpy(sBlock->mData + CMP_HEADER_SIZE,
              sDataBlock + CMP_HEADER_SIZE,
              sHeader.mA7.mPayloadLength);
    sBlock->mDataSize = CMP_HEADER_SIZE + sHeader.mA7.mPayloadLength;

    /*
     * ======================================================
     * bug-27250 free Buf list can be crushed when client killed.
     * 프로토콜 data가 커서 여러 패킷에 나뉘어 수신하는 경우,
     * 다음 패킷을 수신할 준비가 되어 있음을 서버에 알려주어
     * 동기화를 시킨다.(반드시 block 복사 이후에 풀어주어야 함)
     * need to recv next packet.
     * seq end가 아니라는 것은 수신할 다음 패킷이 더 있음을 의미
     */
    if (CMP_HEADER_PROTO_END_IS_SET(&sHeader) == ACP_FALSE)
    {
        /*
         * 큰 프로토콜 패킷 송신 신호 제어용 semaphore 사용
         */
        while(1)
        {
            rc = semop(sDesc->mSemChannelID, sDesc->mOpSignSendMoreToCli, 1);
            if (rc == 0)
            {
                break;
            }
            else
            {
                ACI_TEST_RAISE(EINTR != errno, SemOpError);
            }
        }
    }

    *aHeader = sHeader;

    return ACI_SUCCESS;

    ACI_EXCEPTION(SemOpError);
    {
        ACI_SET(aciSetErrorCode(cmERR_FATAL_CMN_SEM_OP));
    }
    ACI_EXCEPTION(GhostConnection);
    {
        ACI_SET(aciSetErrorCode(cmERR_ABORT_CONNECTION_CLOSED));
    }
    ACI_EXCEPTION(ConnectionClosed);
    {
        (void)semop(sDesc->mSemChannelID, sDesc->mOpSignSvrExit, 1);
        ACI_SET(aciSetErrorCode(cmERR_ABORT_CONNECTION_CLOSED));
    }

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC cmnLinkPeerReqCompleteIPC(cmnLinkPeer * aLink)
{
    /*
     * write complete
     */

    cmnLinkPeerIPC *sLink = (cmnLinkPeerIPC *)aLink;

    sLink->mPrevWriteBuffer = NULL;

    return ACI_SUCCESS;
}

ACI_RC cmnLinkPeerResCompleteIPC(cmnLinkPeer * aLink)
{
    /*
     * read complete
     */

    cmnLinkPeerIPC *sLink = (cmnLinkPeerIPC *)aLink;

    sLink->mPrevReadBuffer = NULL;

    return ACI_SUCCESS;
}

ACI_RC cmnLinkPeerCheckClientIPC(cmnLinkPeer * aLink, acp_bool_t * aIsClosed)
{
    cmnLinkPeerIPC *sLink = (cmnLinkPeerIPC *)aLink;
    cmnLinkDescIPC *sDesc = &sLink->mDesc;

    acp_sint32_t    rc;

    rc = semop(sDesc->mSemChannelID, sDesc->mOpCheckSvrExit, 2);
    ACI_TEST_RAISE(rc == 0, ConnectionClosed);

    *aIsClosed = ACP_FALSE;

    return ACI_SUCCESS;

    ACI_EXCEPTION(ConnectionClosed);
    {
        ACI_SET(aciSetErrorCode(cmERR_ABORT_CONNECTION_CLOSED));
    }

    ACI_EXCEPTION_END;

    *aIsClosed = ACP_TRUE;

    return ACI_FAILURE;
}

acp_bool_t cmnLinkPeerHasPendingRequestIPC(cmnLinkPeer * aLink)
{
    ACP_UNUSED(aLink);

    return ACP_FALSE;
}

ACI_RC cmnLinkPeerSendClientIPC(cmnLinkPeer *aLink, cmbBlock *aBlock)
{
    /*
     * semaphore는 mutex가 아니다!!!!
     *
     * lock을 획득, 반환하는 개념이 아니라
     * 사용할 수 있는 자원의 갯수로 판단한다.
     *
     * write : 무조건 쓰고 +1
     * read  : -1 후 읽기
     */

    cmnLinkPeerIPC *sLink = (cmnLinkPeerIPC *)aLink;
    cmnLinkDescIPC *sDesc = &sLink->mDesc;

    acp_sint32_t       rc;
    cmbShmChannelInfo *sChannelInfo;

    cmpHeader          sHeader;
    acp_sint8_t       *sDataBlock;

    ACI_TEST((sDesc->mConnectFlag != ACP_TRUE) ||
             (sDesc->mHandShakeFlag != ACP_TRUE));

    /*
     * check ghost-connection in shutdown state() : PR-4096
     */
    sChannelInfo = cmbShmGetChannelInfo(gIpcShmBuffer, sDesc->mChannelID);
    ACI_TEST_RAISE(sDesc->mTicketNum != sChannelInfo->mTicketNum, GhostConnection);

    /*
     * =======================================================
     * bug-27250 free Buf list can be crushed when client killed
     * server가 다음 패킷을 수신할 준비 되었는지 확인(try)
     * 준비가 안됐다면 WirteBlockList에 추가되어 나중에 retry.
     */
    if (sLink->mPrevWriteBuffer != NULL)
    {
        /*
         * 큰 프로토콜 패킷 송신 신호 제어용 semaphore 사용
         */
        while(1)
        {
            rc = semop(sDesc->mSemChannelID,
                       sDesc->mOpCheckSendMoreToSvr, 1);
            if (rc == 0)
            {
                break;
            }
            else
            {
                // lock try에 실패하면 goto retry
                ACI_TEST_RAISE(EAGAIN == errno, Retry);
                ACI_TEST_RAISE(EINTR != errno, SemOpError);
            }
        }
    }

    /*
     * bug-27250 free Buf list can be crushed when client killed.
     * block 데이터를 공유 메모리 통신 버퍼로 복사한후 송신한다.
     * 헤더 정보는 datasize를 알기 위해 필요하다.
     * 수신측의 marshal 단계에서 공유 메모리를 직접 참조하는 것을
     * 막기 위해 복사를 한다.
     */

    ACI_TEST(cmpHeaderRead(aLink, &sHeader, aBlock) != ACI_SUCCESS);
    aBlock->mCursor = 0;

    sDataBlock = cmbShmGetDefaultServerBuffer(gIpcShmBuffer,
            gIpcShmInfo.mMaxChannelCount,
            sDesc->mChannelID);
    acpMemCpy(sDataBlock, aBlock->mData, aBlock->mDataSize);
    aBlock->mCursor = aBlock->mDataSize;
    /*
     * ======================================================
     */

    while(1)
    {
        rc = semop(sDesc->mSemChannelID, sDesc->mOpSignSendToSvr, 1);
        if (rc == 0)
        {
            break;
        }
        else
        {
            ACI_TEST_RAISE(EINTR != errno, SemOpError);
        }
    }

    /*
     * =======================================================
     * bug-27250 free Buf list can be crushed when client killed
     * 프로토콜 data가 커서 여러 패킷에 나뉘어 송신하는 경우,
     * 일단 mark만 해둔다.(다음 호출시 위쪽의 코드를 타게 됨)
     * seq end가 아니라는 것은 송신할 다음 패킷이 더 있음을 의미
     */
    if (CMP_HEADER_PROTO_END_IS_SET(&sHeader) == ACP_FALSE)
    {
        sLink->mPrevWriteBuffer = sDataBlock;
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION(GhostConnection);
    {
        ACI_SET(aciSetErrorCode(cmERR_ABORT_CONNECTION_CLOSED));
    }
    ACI_EXCEPTION(SemOpError);
    {
        ACI_SET(aciSetErrorCode(cmERR_FATAL_CMN_SEM_OP));
    }
    /*
     * bug-27250 free Buf list can be crushed when client killed
     */
    ACI_EXCEPTION(Retry);
    {
        ACI_SET(aciSetErrorCode(cmERR_RETRY_SOCKET_OPERATION_WOULD_BLOCK));
    }

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

struct cmnLinkOP gCmnLinkPeerClientOpIPC =
{
    "IPC-PEER-CLIENT",

    cmnLinkPeerInitializeClientIPC,
    cmnLinkPeerFinalizeIPC,

    cmnLinkPeerCloseClientIPC,

    cmnLinkPeerGetSockIPC,

    cmnLinkPeerGetDispatchInfoIPC,
    cmnLinkPeerSetDispatchInfoIPC
};

struct cmnLinkPeerOP gCmnLinkPeerPeerClientOpIPC =
{
    cmnLinkPeerSetBlockingModeIPC,

    cmnLinkPeerGetInfoIPC,
    cmnLinkPeerGetDescIPC,

    cmnLinkPeerConnectIPC,
    cmnLinkPeerSetOptionsIPC,

    cmnLinkPeerAllocChannelClientIPC,
    cmnLinkPeerHandshakeClientIPC,

    cmnLinkPeerShutdownClientIPC,

    cmnLinkPeerRecvClientIPC,
    cmnLinkPeerSendClientIPC,

    cmnLinkPeerReqCompleteIPC,
    cmnLinkPeerResCompleteIPC,

    cmnLinkPeerCheckClientIPC,
    cmnLinkPeerHasPendingRequestIPC,

    cmnLinkPeerAllocBlockClientIPC,
    cmnLinkPeerFreeBlockClientIPC,

    /* TASK-5894 Permit sysdba via IPC */
    NULL,

    /* PROJ-2474 SSL/TLS */
    NULL,
    NULL,
    NULL,

    /* PROJ-2625 Semi-async Prefetch, Prefetch Auto-tuning */
    NULL,
    NULL,
    NULL,
    NULL
};


ACI_RC cmnLinkPeerClientMapIPC(cmnLink *aLink)
{
    cmnLinkPeer *sLink = (cmnLinkPeer *)aLink;

    /*
     * Link 검사
     */
    ACE_ASSERT(aLink->mType == CMN_LINK_TYPE_PEER_CLIENT);
    ACE_ASSERT(aLink->mImpl == CMN_LINK_IMPL_IPC);

    /*
     *  BUGBUG: IPC Pool 획득
     */
    ACI_TEST(cmbPoolGetSharedPool(&sLink->mPool, CMB_POOL_IMPL_IPC) != ACI_SUCCESS);

    /*
     * 함수 포인터 세팅
     */
    aLink->mOp     = &gCmnLinkPeerClientOpIPC;
    sLink->mPeerOp = &gCmnLinkPeerPeerClientOpIPC;

    /*
     * 멤버 초기화
     */
    sLink->mUserPtr    = NULL;

    return ACI_SUCCESS;
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

acp_uint32_t cmnLinkPeerClientSizeIPC()
{
    return ACI_SIZEOF(cmnLinkPeerIPC);
}

/*
 * bug-27250 free Buf list can be crushed when client killed.
 * cmiWriteBlock에서 protocol end packet 송신시
 * pending block이 있는 경우 이 코드 수행
 */
ACI_RC cmnLinkPeerWaitSendClientIPC(cmnLink* aLink)
{

    cmnLinkPeerIPC *sLink = (cmnLinkPeerIPC *)aLink;
    cmnLinkDescIPC *sDesc = &sLink->mDesc;
    acp_sint32_t            rc;

    /*
     * receiver가 송신 허락 신호를 줄때까지 무한 대기
     */
    while(1)
    {
        rc = semop(sDesc->mSemChannelID,
                   sDesc->mOpWaitSendMoreToSvr, 2);
        if (rc == 0)
        {
            break;
        }
        else
        {
            ACI_TEST_RAISE(EIDRM == errno, SemRemoved);
            ACI_TEST_RAISE(EINTR != errno, SemOpError);
        }
    }

    /*
     * disconnect check
     */
    rc = semop(sDesc->mSemChannelID,
               sDesc->mOpCheckSvrExit, 2);
    ACI_TEST_RAISE(rc == 0, ConnectionClosed);

    return ACI_SUCCESS;

    ACI_EXCEPTION(SemRemoved);
    {
        ACI_SET(aciSetErrorCode(cmERR_ABORT_CONNECTION_CLOSED));
    }
    ACI_EXCEPTION(SemOpError);
    {
        ACI_SET(aciSetErrorCode(cmERR_FATAL_CMN_SEM_OP));
    }
    ACI_EXCEPTION(ConnectionClosed);
    {
        ACI_SET(aciSetErrorCode(cmERR_ABORT_CONNECTION_CLOSED));
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

#endif
