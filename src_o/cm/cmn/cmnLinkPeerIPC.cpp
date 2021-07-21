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

#if !defined(CM_DISABLE_IPC)

/*
 * IPC Channel Information
 */
extern PDL_thread_mutex_t gIpcMutex;
extern cmbShmInfo         gIpcShmInfo;
extern SInt               gIpcClientCount;
extern SChar *            gIpcShmBuffer;
extern idBool *           gIpcChannelList;
extern SInt               gIpcShmKey;
extern PDL_HANDLE         gIpcShmID;
extern SInt  *            gIpcSemChannelKey;
extern SInt  *            gIpcSemChannelID;

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
    SChar*         mLastWriteBuffer; // 클라이언트 비정상 종료시 서버가 write한 마지막 버퍼 주소
    SChar*         mPrevWriteBuffer; // 마지막으로 write한 shared memory 내의 버퍼주소
    SChar*         mPrevReadBuffer;  // 마지막으로 read한 shared memory 내의 버퍼주소
} cmnLinkPeerIPC;

IDE_RC cmnLinkPeerInitializeServerIPC(cmnLink *aLink)
{
    cmnLinkPeerIPC *sLink = (cmnLinkPeerIPC *)aLink;
    cmnLinkDescIPC *sDesc = &sLink->mDesc;

    /*
     * 멤버 초기화
     */

    sLink->mLastWriteBuffer = NULL;
    sLink->mPrevWriteBuffer = NULL;
    sLink->mPrevReadBuffer  = NULL;

    sDesc->mConnectFlag   = ID_TRUE;
    sDesc->mHandShakeFlag = ID_FALSE;

    sDesc->mHandle        = PDL_INVALID_SOCKET;
    sDesc->mChannelID     = -1;

    sDesc->mSemChannelKey = -1;
    sDesc->mSemChannelID  = PDL_INVALID_HANDLE;

    idlOS::memset(&(sDesc->mSemInfo), 0, ID_SIZEOF(semid_ds));

    return IDE_SUCCESS;
}

IDE_RC cmnLinkPeerInitializeClientIPC(cmnLink * /* aLink */)
{
    /* proj_2160 cm_type removal
     *  all codes in xxxClientIPC() of this file are removed.
     * cuz it is not used for client. */
    return IDE_SUCCESS;
}

IDE_RC cmnLinkPeerFinalizeIPC(cmnLink *aLink)
{
    IDE_TEST(aLink->mOp->mClose(aLink) != IDE_SUCCESS);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

/*
 * !!!!!!!!!!!!!!!!!!!! WARNING !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 * 클라이언트의 종료를 기다릴 때 *절대로* mutex를 잡아서는
 * 안된다. mutex를 잡고 종료를 기다릴 경우 시스템 전체가
 * 정지될 수 있다.
 * PR-4407
 */

IDE_RC cmnLinkPeerCloseServerIPC(cmnLink *aLink)
{
    cmnLinkPeerIPC    *sLink = (cmnLinkPeerIPC *)aLink;
    cmnLinkDescIPC    *sDesc = &sLink->mDesc;
    cmbShmChannelInfo *sChannelInfo;
    idBool             sLocked = ID_FALSE;

    IDE_TEST(sDesc->mConnectFlag != ID_TRUE);

    if (sDesc->mHandShakeFlag == ID_TRUE)
    {
        // To fix PR-2006
        IDE_TEST_RAISE(idlOS::thread_mutex_lock(&gIpcMutex) != 0, LockError);
        sLocked = ID_TRUE;

        if (gIpcChannelList[sDesc->mChannelID] == ID_TRUE)
        {
            // bug-27250 free Buf list can be crushed when client killed.
            // wake up client sending next packet of big data
            while (0 != idlOS::semop(gIpcSemChannelID[sDesc->mChannelID],
                                     sDesc->mOpSignSendMoreToSvr, 1))
            {
                IDE_TEST_RAISE(errno != EINTR, SemClientChannelReleaseError);
            }

            // wake up client wait
            // fix BUG-19265
            while (0 != idlOS::semop(gIpcSemChannelID[sDesc->mChannelID],
                                     sDesc->mOpSignSendToCli, 1))
            {
                IDE_TEST_RAISE(errno != EINTR, SemClientChannelReleaseError);
            }

            sLocked = ID_FALSE;
            IDE_TEST_RAISE(idlOS::thread_mutex_unlock(&gIpcMutex) != 0, UnlockError);

            // normal server mode : waiting for client to be killed
            while (0 != idlOS::semop(gIpcSemChannelID[sDesc->mChannelID],
                                     sDesc->mOpWaitCliExit,
                                     2))
            {
                IDE_TEST_RAISE(errno != EINTR, SemWaitClientDeadError);
            }

            IDE_TEST_RAISE(idlOS::thread_mutex_lock(&gIpcMutex) != 0, LockError);
            sLocked = ID_FALSE;

            /*
             * BUG-32398
             * 값을 변경해 여러 Client로의 혼선을 예방하자.
             */
            sChannelInfo = cmbShmGetChannelInfo(gIpcShmBuffer, sDesc->mChannelID);
            sChannelInfo->mTicketNum++;

            gIpcShmInfo.mUsedChannelCount--;
            gIpcChannelList[sDesc->mChannelID] = ID_FALSE;
        }

        // To fix PR-2006
        sLocked = ID_FALSE;
        IDE_TEST_RAISE(idlOS::thread_mutex_unlock(&gIpcMutex) != 0, UnlockError);
    }

    /*
     * socket이 열려있으면 닫음
     */

    if (sLink->mDesc.mHandle != PDL_INVALID_SOCKET)
    {
        idlOS::closesocket(sLink->mDesc.mHandle);
        sLink->mDesc.mHandle = PDL_INVALID_SOCKET;
    }

    sLink->mDesc.mConnectFlag = ID_FALSE;

    return IDE_SUCCESS;

    IDE_EXCEPTION(LockError);
    {
        IDE_SET(ideSetErrorCode(cmERR_FATAL_CMN_MUTEX_LOCK));
    }
    IDE_EXCEPTION(UnlockError);
    {
        IDE_SET(ideSetErrorCode(cmERR_FATAL_CMN_MUTEX_UNLOCK));
    }
    IDE_EXCEPTION(SemClientChannelReleaseError);
    {
        IDE_SET(ideSetErrorCode(cmERR_FATAL_CMN_SEM_OP));
    }
    IDE_EXCEPTION(SemWaitClientDeadError);
    {
        IDE_SET(ideSetErrorCode(cmERR_FATAL_CMN_SEM_OP));
    }

    IDE_EXCEPTION_END;

    if (sLocked == ID_TRUE)
    {
        (void)idlOS::thread_mutex_unlock(&gIpcMutex);
    }

    return IDE_FAILURE;
}

IDE_RC cmnLinkPeerCloseClientIPC(cmnLink * /* aLink */)
{
    return IDE_SUCCESS;
}

IDE_RC cmnLinkPeerGetHandleIPC(cmnLink */*aLink*/, void */*aHandle*/)
{
    return IDE_SUCCESS;
}

IDE_RC cmnLinkPeerGetDispatchInfoIPC(cmnLink * /*aLink*/, void * /*aDispatchInfo*/)
{
   return IDE_SUCCESS;
}

IDE_RC cmnLinkPeerSetDispatchInfoIPC(cmnLink * /*aLink*/, void * /*aDispatchInfo*/)
{
    return IDE_SUCCESS;
}

IDE_RC cmnLinkPeerSetBlockingModeIPC(cmnLinkPeer * /*aLink*/, idBool /*aBlockingMode*/)
{
    return IDE_SUCCESS;
}

IDE_RC cmnLinkPeerGetInfoIPC(cmnLinkPeer * /*aLink*/, SChar *aBuf, UInt aBufLen, cmiLinkInfoKey aKey)
{
    SInt sRet;

    switch (aKey)
    {
        case CMN_LINK_INFO_ALL:
        case CMN_LINK_INFO_IMPL:
        case CMN_LINK_INFO_IPC_KEY:
            sRet = idlOS::snprintf(aBuf, aBufLen, "IPC");

            IDE_TEST_RAISE(sRet < 0, StringOutputError);
            IDE_TEST_RAISE((UInt)sRet >= aBufLen, StringTruncated);

            break;

        default:
            IDE_RAISE(UnsupportedLinkInfoKey);
            break;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(StringOutputError);
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_STRING_OUTPUT_ERROR));
    }
    IDE_EXCEPTION(StringTruncated);
    {
        IDE_SET(ideSetErrorCode(cmERR_IGNORE_STRING_TRUNCATED));
    }
    IDE_EXCEPTION(UnsupportedLinkInfoKey);
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_UNSUPPORTED_LINK_INFO_KEY));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC cmnLinkPeerGetDescIPC(cmnLinkPeer *aLink, void *aDesc)
{
    cmnLinkPeerIPC *sLink = (cmnLinkPeerIPC *)aLink;

    /*
     * Desc를 돌려줌
     */
    *(cmnLinkDescIPC **)aDesc = &sLink->mDesc;

    return IDE_SUCCESS;
}

IDE_RC cmnLinkPeerConnectIPC(cmnLinkPeer       *aLink,
                             cmnLinkConnectArg *aConnectArg,
                             PDL_Time_Value    *aTimeout,
                             SInt              /*aOption*/)
{
    cmnLinkPeerIPC    *sLink = (cmnLinkPeerIPC *)aLink;
    UInt               sPathLen;

    /*
     * socket 생성
     */

    sLink->mDesc.mHandle = idlOS::socket(AF_UNIX, SOCK_STREAM, 0);
    IDE_TEST_RAISE(sLink->mDesc.mHandle == PDL_INVALID_SOCKET, SocketError);

    /*
     * IPC 파일이름 길이 검사
     */

    sPathLen = idlOS::strlen(aConnectArg->mIPC.mFilePath);
    IDE_TEST_RAISE(ID_SIZEOF(sLink->mDesc.mAddr.sun_path) <= sPathLen, IpcPathTruncated);

    /*
     * IPC 주소 세팅
     */

    sLink->mDesc.mAddr.sun_family = AF_UNIX;
    idlOS::snprintf(sLink->mDesc.mAddr.sun_path,
                    ID_SIZEOF(sLink->mDesc.mAddr.sun_path),
                    "%s",
                    aConnectArg->mIPC.mFilePath);

    /*
     * connect
     */

    IDE_TEST_RAISE(idlVA::connect_timed_wait(sLink->mDesc.mHandle,
                                             (struct sockaddr *)(&sLink->mDesc.mAddr),
                                             ID_SIZEOF(sLink->mDesc.mAddr),
                                             aTimeout) < 0,
                   ConnectError);

    sLink->mDesc.mConnectFlag = ID_TRUE;

    return IDE_SUCCESS;

    IDE_EXCEPTION(SocketError);
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_SOCKET_OPEN_ERROR, errno));
    }
    IDE_EXCEPTION(IpcPathTruncated);
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_UNIX_PATH_TRUNCATED));
    }
    IDE_EXCEPTION(ConnectError);
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_CONNECT_ERROR, errno));
    }
    IDE_EXCEPTION_END;

    if (sLink->mDesc.mHandle != PDL_INVALID_SOCKET)
    {
        (void)idlOS::closesocket(sLink->mDesc.mHandle);
        sLink->mDesc.mHandle = PDL_INVALID_SOCKET;
    }

    return IDE_FAILURE;
}

IDE_RC cmnLinkPeerAllocChannelServerIPC(cmnLinkPeer *aLink, SInt *aChannelID)
{
    UInt   i;
    SInt   rc;
    SInt   sChannelID = -1;
    idBool sLocked    = ID_FALSE;

    cmnLinkPeerIPC *sLink = (cmnLinkPeerIPC *)aLink;
    cmnLinkDescIPC *sDesc = &sLink->mDesc;

    rc = idlOS::thread_mutex_lock(&gIpcMutex);
    IDE_TEST_RAISE( rc !=0, LockError);

    sLocked = ID_TRUE;

    IDE_TEST_RAISE( gIpcShmInfo.mUsedChannelCount >= gIpcShmInfo.mMaxChannelCount,
                    NoChannelError);

    for (i = 0; i < gIpcShmInfo.mMaxChannelCount; i++)
    {
        if (gIpcChannelList[i] != ID_TRUE)
        {
            sChannelID = i;
            gIpcChannelList[i] = ID_TRUE;
            break;
        }
    }

    IDE_TEST_RAISE( sChannelID == -1, NoChannelError);

    gIpcShmInfo.mUsedChannelCount++;

    // BUG-25420 [CodeSonar] Lock, Unlock 에러 핸들링 오류에 의한 Double Unlock
    // unlock 를 하기전에 세팅을해야 Double Unlock 을 막을수 있다.
    sLocked = ID_FALSE;
    rc = idlOS::thread_mutex_unlock(&gIpcMutex);
    IDE_TEST_RAISE( rc !=0, UnlockError);

    sDesc->mChannelID = sChannelID;

    *aChannelID = sChannelID;

    return IDE_SUCCESS;

    IDE_EXCEPTION(LockError);
    {
        IDE_SET(ideSetErrorCode(cmERR_FATAL_CMN_MUTEX_LOCK));
    }
    IDE_EXCEPTION(NoChannelError);
    {
        SChar             sProtoBuffer[20];
        cmnLinkDescIPCMsg sIPCMsg;

        sIPCMsg.mChannelID = -1;

        idlVA::recv_i(sDesc->mHandle, sProtoBuffer, IPC_PROTO_CNT);
        idlVA::send_i(sDesc->mHandle, &sIPCMsg, ID_SIZEOF(cmnLinkDescIPCMsg));

        IDE_SET(ideSetErrorCode(cmERR_ABORT_CMN_ERR_FULL_IPC_CHANNEL));
    }
    IDE_EXCEPTION(UnlockError);
    {
        IDE_SET(ideSetErrorCode(cmERR_FATAL_CMN_MUTEX_UNLOCK));
        gIpcShmInfo.mUsedChannelCount--;
    }

    IDE_EXCEPTION_END;

    *aChannelID       = -1;
    sDesc->mChannelID = -1;

    if (sChannelID != -1)
    {
        gIpcChannelList[sChannelID] = ID_FALSE;
    }
    if (sLocked == ID_TRUE)
    {
        (void)idlOS::thread_mutex_unlock(&gIpcMutex);
        sLocked = ID_FALSE;
    }

    return IDE_FAILURE;
}

IDE_RC cmnLinkPeerAllocChannelClientIPC(cmnLinkPeer */*aLink*/, SInt */*aChannelID*/)
{
    return IDE_FAILURE;
}

IDE_RC cmnLinkPeerSetOperation(cmnLinkDescIPC *aDesc)
{
    //=====================================================
    // bug-28340 rename semop name for readability
    // 가독성을 위해 semaphore op 변수명을 다음과 같이 변경
    //=====================================================
    // IPC_SEM_SERVER_DETECT  > IPC_SEM_CHECK_SVR_EXIT (0)
    // server_detect_init     > InitSvrExit  : 서버가 잡고 시작
    // server_detect_try      > CheckSvrExit : 서버가 죽었는지 확인
    // server_detect_release  > SignSvrExit  : 서버가 종료신호 보냄
    //=====================================================
    // IPC_SEM_CLIENT_DETECT  > IPC_SEM_CHECK_CLI_EXIT (1)
    // client_detect_init     > InitCliExit  : cli가 잡고 시작
    // client_detect_try      > CheckCliExit : cli가 죽었는지 확인
    // client_detect_hold     > WaitCliExit  : cli 종료때까지 대기
    // client_detect_release  > SignCliExit  : cli가 종료신호 보냄
    //=====================================================
    // IPC_SEM_SERVER_CHANNEL > IPC_SEM_SEND_TO_SVR (2)
    // server_channel_init    > InitSendToSvr :cli가 잡고 시작
    // server_channel_hold    > WaitSendToSvr :서버가 수신대기
    // server_channel_release > SignSendToSvr :cli가 서버수신허락
    //=====================================================
    // IPC_SEM_CLIENT_CHANNLE > IPC_SEM_SEND_TO_CLI (3)
    // client_channel_init    > InitSendToCli :서버가 잡고 시작
    // client_channel_hold    > WaitSendToCli :cli가 수신대기
    // client_channel_release > SignSendToCli :서버가 cli수신허락
    //=====================================================
    // IPC_SEM_CLI_SENDMORE  > IPC_SEM_SENDMORE_TO_SVR (4)
    // cli_sendmore_init     > InitSendMoreToSvr  :서버가 잡고 시작
    // cli_sendmore_try      > CheckSendMoreToSvr :cli가 송신시도
    // cli_sendmore_hold     > WaitSendMoreToSvr  :cli가 송신대기
    // cli_sendmore_release  > SignSendMoreToSvr  :서버가 송신허락
    //=====================================================
    // IPC_SEM_SVR_SENDMORE  > IPC_SEM_SENDMORE_TO_CLI (5)
    // svr_sendmore_init     > InitSendMoreToCli  :cli가 잡고 시작
    // svr_sendmore_try      > CheckSendMoreToCli :서버가 송신시도
    // svr_sendmore_hold     > WaitSendMoreToCli  :서버가 송신대기
    // svr_sendmore_release  > SignSendMoreToCli  :cli가 송신허락
    //=====================================================

    //=====================================================
    // IPC_SEM_CHECK_SVR_EXIT (0)
    aDesc->mOpInitSvrExit[0].sem_num = IPC_SEM_CHECK_SVR_EXIT;
    aDesc->mOpInitSvrExit[0].sem_op  = -1;
    aDesc->mOpInitSvrExit[0].sem_flg = SEM_UNDO | IPC_NOWAIT;

    aDesc->mOpCheckSvrExit[0].sem_num = IPC_SEM_CHECK_SVR_EXIT;
    aDesc->mOpCheckSvrExit[0].sem_op  = -1;
    aDesc->mOpCheckSvrExit[0].sem_flg = IPC_NOWAIT;
    aDesc->mOpCheckSvrExit[1].sem_num = IPC_SEM_CHECK_SVR_EXIT;
    aDesc->mOpCheckSvrExit[1].sem_op  = 1;
    aDesc->mOpCheckSvrExit[1].sem_flg = 0;

    aDesc->mOpSignSvrExit[0].sem_num = IPC_SEM_CHECK_SVR_EXIT;
    aDesc->mOpSignSvrExit[0].sem_op  = 1;
    aDesc->mOpSignSvrExit[0].sem_flg = 0;

    //=====================================================
    // IPC_SEM_CHECK_CLI_EXIT (1)
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

    //=====================================================
    // IPC_SEM_SEND_TO_SVR (2)
    aDesc->mOpInitSendToSvr[0].sem_num = IPC_SEM_SEND_TO_SVR;
    aDesc->mOpInitSendToSvr[0].sem_op  = (-1 * gIpcShmInfo.mMaxBufferCount);
    aDesc->mOpInitSendToSvr[0].sem_flg = SEM_UNDO;

    aDesc->mOpWaitSendToSvr[0].sem_num = IPC_SEM_SEND_TO_SVR;
    aDesc->mOpWaitSendToSvr[0].sem_op  = -1;
    aDesc->mOpWaitSendToSvr[0].sem_flg = 0;

    aDesc->mOpSignSendToSvr[0].sem_num = IPC_SEM_SEND_TO_SVR;
    aDesc->mOpSignSendToSvr[0].sem_op  = 1;
    aDesc->mOpSignSendToSvr[0].sem_flg = 0;

    //=====================================================
    // IPC_SEM_SEND_TO_CLI (3)
    aDesc->mOpInitSendToCli[0].sem_num = IPC_SEM_SEND_TO_CLI;
    aDesc->mOpInitSendToCli[0].sem_op  = (-1 * gIpcShmInfo.mMaxBufferCount);
    aDesc->mOpInitSendToCli[0].sem_flg = SEM_UNDO | IPC_NOWAIT;

    aDesc->mOpWaitSendToCli[0].sem_num = IPC_SEM_SEND_TO_CLI;
    aDesc->mOpWaitSendToCli[0].sem_op  = -1;
    aDesc->mOpWaitSendToCli[0].sem_flg = 0;

    aDesc->mOpSignSendToCli[0].sem_num = IPC_SEM_SEND_TO_CLI;
    aDesc->mOpSignSendToCli[0].sem_op  = 1;
    aDesc->mOpSignSendToCli[0].sem_flg = 0;

    // bug-27250 free Buf list can be crushed when client killed
    // 큰 프로토콜에 대한 다음 패킷 송신 신호 제어용 semaphore
    //=====================================================
    // IPC_SEM_SENDMORE_TO_SVR (4)
    aDesc->mOpInitSendMoreToSvr[0].sem_num = IPC_SEM_SENDMORE_TO_SVR;
    aDesc->mOpInitSendMoreToSvr[0].sem_op  = (-1 * gIpcShmInfo.mMaxBufferCount);
    aDesc->mOpInitSendMoreToSvr[0].sem_flg = SEM_UNDO | IPC_NOWAIT;

    aDesc->mOpCheckSendMoreToSvr[0].sem_num = IPC_SEM_SENDMORE_TO_SVR;
    aDesc->mOpCheckSendMoreToSvr[0].sem_op  = -1;
    aDesc->mOpCheckSendMoreToSvr[0].sem_flg = IPC_NOWAIT;

    // cmnDispatcherWaitLink 에서 무한대기 하기위해 사용.
    aDesc->mOpWaitSendMoreToSvr[0].sem_num = IPC_SEM_SENDMORE_TO_SVR;
    aDesc->mOpWaitSendMoreToSvr[0].sem_op  = -1;
    aDesc->mOpWaitSendMoreToSvr[0].sem_flg = 0;
    aDesc->mOpWaitSendMoreToSvr[1].sem_num = IPC_SEM_SENDMORE_TO_SVR;
    aDesc->mOpWaitSendMoreToSvr[1].sem_op  = 1;
    aDesc->mOpWaitSendMoreToSvr[1].sem_flg = 0;

    aDesc->mOpSignSendMoreToSvr[0].sem_num = IPC_SEM_SENDMORE_TO_SVR;
    aDesc->mOpSignSendMoreToSvr[0].sem_op  = 1;
    aDesc->mOpSignSendMoreToSvr[0].sem_flg = 0;

    //=====================================================
    // IPC_SEM_SENDMORE_TO_CLI (5)
    aDesc->mOpInitSendMoreToCli[0].sem_num = IPC_SEM_SENDMORE_TO_CLI;
    aDesc->mOpInitSendMoreToCli[0].sem_op  = (-1 * gIpcShmInfo.mMaxBufferCount);
    aDesc->mOpInitSendMoreToCli[0].sem_flg = SEM_UNDO;

    aDesc->mOpCheckSendMoreToCli[0].sem_num = IPC_SEM_SENDMORE_TO_CLI;
    aDesc->mOpCheckSendMoreToCli[0].sem_op  = -1;
    aDesc->mOpCheckSendMoreToCli[0].sem_flg = IPC_NOWAIT;

    // cmnDispatcherWaitLink 에서 무한대기 하기위해 사용.
    aDesc->mOpWaitSendMoreToCli[0].sem_num = IPC_SEM_SENDMORE_TO_CLI;
    aDesc->mOpWaitSendMoreToCli[0].sem_op  = -1;
    aDesc->mOpWaitSendMoreToCli[0].sem_flg = 0;
    aDesc->mOpWaitSendMoreToCli[1].sem_num = IPC_SEM_SENDMORE_TO_CLI;
    aDesc->mOpWaitSendMoreToCli[1].sem_op  = 1;
    aDesc->mOpWaitSendMoreToCli[1].sem_flg = 0;

    aDesc->mOpSignSendMoreToCli[0].sem_num = IPC_SEM_SENDMORE_TO_CLI;
    aDesc->mOpSignSendMoreToCli[0].sem_op  = 1;
    aDesc->mOpSignSendMoreToCli[0].sem_flg = 0;

    return IDE_SUCCESS;
}

IDE_RC cmnLinkPeerSyncServerIPC(cmnLinkDescIPC *aDesc)
{
    // 시작하기 위한 동기화
    // fix BUG-19265
    while (0 !=idlOS::semop(gIpcSemChannelID[aDesc->mChannelID],
                             aDesc->mOpSignSendToCli, 1))
    {
        IDE_TEST_RAISE(errno != EINTR, SemOpError);
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(SemOpError);
    {
        IDE_SET(ideSetErrorCode(cmERR_FATAL_CMN_SEM_OP));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC cmnLinkPeerSyncClientIPC(cmnLinkDescIPC * /* aDesc */)
{
    return IDE_FAILURE;
}

/* bug-31572: IPC dispatcher hang might occur on AIX.
 * print some sema info for debugging.
 * this function called only in cmnLinkPeerHandshakeServer, below */
void cmnLinkPeerPrintSemaIPC(cmnLinkDescIPC* aDesc, SInt aSemID, SInt aSemNum)
{
    union semun         sDummyArg;
    SInt                sSemVal;
    SInt                sSemPid;

    idlOS::memset(&sDummyArg, 0x00, ID_SIZEOF(sDummyArg));
    sSemVal = idlOS::semctl(aSemID, aSemNum, GETVAL, sDummyArg);
    sSemPid = idlOS::semctl(aSemID, aSemNum, GETPID, sDummyArg);
    ideLog::log(IDE_SERVER_2, "[IPC dispatcher] re-init "
                "chnlID: %d "
                "sem id: %d "
                "semnum: %d "
                "semval: %d "
                "sempid: %d",
                aDesc->mChannelID,
                aSemID,
                aSemNum,
                sSemVal,
                sSemPid);
}

IDE_RC cmnLinkPeerHandshakeServerIPC(cmnLinkPeer *aLink)
{
    cmnLinkPeerIPC *sLink = (cmnLinkPeerIPC *)aLink;
    cmnLinkDescIPC *sDesc = &sLink->mDesc;

    SInt                rc;
    SInt                sCount;
    SChar               sProtoBuffer[20];
    union semun         sArg;
    cmnLinkDescIPCMsg   sIPCMsg;
    cmbShmChannelInfo  *sChannelInfo;

    // fix BUG-21947
    sDesc->mHandShakeFlag = ID_TRUE;

    cmnLinkPeerSetOperation(sDesc);

    // recv hello message

    sCount = idlVA::recv_i(sDesc->mHandle, sProtoBuffer, IPC_PROTO_CNT);
    IDE_TEST_RAISE(sCount != IPC_PROTO_CNT, InetSocketError);
    IDV_SESS_ADD(aLink->mStatistics, IDV_STAT_INDEX_RECV_UNIX_BYTE, (ULong)sCount);
    IDV_SESS_ADD(aLink->mStatistics, IDV_STAT_INDEX_RECV_SOCKET_COUNT, 1 );

    rc = idlOS::memcmp(sProtoBuffer, IPC_HELLO_MSG, IPC_PROTO_CNT);
    IDE_TEST_RAISE(rc != 0, InetSocketError);

    IDE_ASSERT(sDesc->mChannelID >= 0);

    // init semaphore value

    sArg.val = 1;
    IDE_TEST_RAISE(idlOS::semctl(gIpcSemChannelID[sDesc->mChannelID],
                                  0, SETVAL, sArg)
                   != 0, ResetSemError);
    sArg.val = 1;
    IDE_TEST_RAISE(idlOS::semctl(gIpcSemChannelID[sDesc->mChannelID],
                                  1, SETVAL, sArg)
                   != 0, ResetSemError);

    sArg.val = gIpcShmInfo.mMaxBufferCount;
    IDE_TEST_RAISE(idlOS::semctl(gIpcSemChannelID[sDesc->mChannelID],
                                 2, SETVAL, sArg)
                   != 0, ResetSemError);

    sArg.val = gIpcShmInfo.mMaxBufferCount;
    IDE_TEST_RAISE(idlOS::semctl(gIpcSemChannelID[sDesc->mChannelID],
                                 3, SETVAL, sArg)
                   != 0, ResetSemError);

    // bug-27250 free Buf list can be crushed when client killed
    // init IPC_SEM_SENDMORE_TO_SVR, IPC_SEM_SENDMORE_TO_CLI
    sArg.val = gIpcShmInfo.mMaxBufferCount;
    IDE_TEST_RAISE(idlOS::semctl(gIpcSemChannelID[sDesc->mChannelID],
                                  4, SETVAL, sArg)
                   != 0, ResetSemError);
    sArg.val = gIpcShmInfo.mMaxBufferCount;
    IDE_TEST_RAISE(idlOS::semctl(gIpcSemChannelID[sDesc->mChannelID],
                                  5, SETVAL, sArg)
                   != 0, ResetSemError);


    sChannelInfo = cmbShmGetChannelInfo(gIpcShmBuffer, sDesc->mChannelID);
    /*
     * BUG-32398
     * 타임스탬프 대신 티켓번호을 사용한다.
     * 초단위라서 같은 값을 가지는 클라이언트가 존재할 가능성이 있다.
     */
    sDesc->mTicketNum = sChannelInfo->mTicketNum;

    // set server semaphore to detect server dead
    // fix BUG-19265
    while (0 != idlOS::semop(gIpcSemChannelID[sDesc->mChannelID],
                             sDesc->mOpInitSvrExit, 1))
    {
        /* bug-31572: IPC dispatcher hang might occur on AIX. */
        if (errno == EAGAIN)
        {
            /* print some info for debugging */
            cmnLinkPeerPrintSemaIPC(sDesc,
                                    gIpcSemChannelID[sDesc->mChannelID],
                                    IPC_SEM_CHECK_SVR_EXIT);

            /* init semval again */
            sArg.val = 1;
            IDE_TEST_RAISE(
                idlOS::semctl(gIpcSemChannelID[sDesc->mChannelID],
                              IPC_SEM_CHECK_SVR_EXIT,
                              SETVAL, sArg)
                != 0, ResetSemError);
        }
        else
        {
            IDE_TEST_RAISE(errno != EINTR, SemServerInitError);
        }
    }

    // hold clinet channel to prevent clinet's starting
    while (0 != idlOS::semop(gIpcSemChannelID[sDesc->mChannelID],
                             sDesc->mOpInitSendToCli, 1))
    {
        /* bug-31572: IPC dispatcher hang might occur on AIX.
         * as-is: wait forever until holding sema lock.
         * to-be: if holding lock is failed, try again. */
        if (errno == EAGAIN)
        {
            /* print some info for debugging */
            cmnLinkPeerPrintSemaIPC(sDesc,
                                    gIpcSemChannelID[sDesc->mChannelID],
                                    IPC_SEM_SEND_TO_CLI);

            /* init semval again */
            sArg.val = gIpcShmInfo.mMaxBufferCount;
            IDE_TEST_RAISE(
                idlOS::semctl(gIpcSemChannelID[sDesc->mChannelID],
                              IPC_SEM_SEND_TO_CLI,
                              SETVAL, sArg)
                != 0, ResetSemError);
        }
        else
        {
            IDE_TEST_RAISE(errno != EINTR, SemClientInitError);
        }
    }

    // bug-27250 free Buf list can be crushed when client killed
    // hold client send for big protocol
    while (0 != idlOS::semop(gIpcSemChannelID[sDesc->mChannelID],
                             sDesc->mOpInitSendMoreToSvr, 1))
    {
        /* bug-31572: IPC dispatcher hang might occur on AIX. */
        if (errno == EAGAIN)
        {
            /* print some info for debugging */
            cmnLinkPeerPrintSemaIPC(sDesc,
                                    gIpcSemChannelID[sDesc->mChannelID],
                                    IPC_SEM_SENDMORE_TO_SVR);

            /* init semval again */
            sArg.val = gIpcShmInfo.mMaxBufferCount;
            IDE_TEST_RAISE(
                idlOS::semctl(gIpcSemChannelID[sDesc->mChannelID],
                              IPC_SEM_SENDMORE_TO_SVR,
                              SETVAL, sArg)
                != 0, ResetSemError);
        }
        else
        {
            IDE_TEST_RAISE(errno != EINTR, SemClientInitError);
        }
    }

    // send ipc information to client

    sIPCMsg.mMaxChannelCount = gIpcShmInfo.mMaxChannelCount;
    sIPCMsg.mMaxBufferCount  = gIpcShmInfo.mMaxBufferCount;
    sIPCMsg.mShmKey          = gIpcShmKey;
    sIPCMsg.mSemChannelKey   = gIpcSemChannelKey[sDesc->mChannelID];
    // bug-26921: del gIpcSemBufferKey => we don't use anymore
    sIPCMsg.mSemBufferKey    = 0;
    sIPCMsg.mChannelID       = sDesc->mChannelID;

    sCount = idlVA::send_i(sDesc->mHandle, &sIPCMsg, ID_SIZEOF(cmnLinkDescIPCMsg));
    IDE_TEST_RAISE(sCount != ID_SIZEOF(cmnLinkDescIPCMsg), InetSocketError);
    IDV_SESS_ADD(aLink->mStatistics, IDV_STAT_INDEX_SEND_UNIX_BYTE, (ULong)sCount);
    IDV_SESS_ADD(aLink->mStatistics, IDV_STAT_INDEX_SEND_SOCKET_COUNT, 1 );

    // recv client ready msg

    sCount = idlVA::recv_i(sDesc->mHandle, sProtoBuffer, IPC_PROTO_CNT);
    IDE_TEST_RAISE(sCount != IPC_PROTO_CNT, InetSocketError);
    IDV_SESS_ADD(aLink->mStatistics, IDV_STAT_INDEX_RECV_UNIX_BYTE, (ULong)sCount);
    IDV_SESS_ADD(aLink->mStatistics, IDV_STAT_INDEX_RECV_SOCKET_COUNT, 1 );

    rc = idlOS::memcmp(sProtoBuffer, IPC_READY_MSG, IPC_PROTO_CNT);
    IDE_TEST_RAISE(rc != 0, InetSocketError);

    // Sync after Establish Connection

    IDE_TEST(cmnLinkPeerSyncServerIPC(sDesc) != IDE_SUCCESS);

    // fix BUG-18833
    if (sDesc->mHandle != PDL_INVALID_SOCKET)
    {
        idlOS::closesocket(sLink->mDesc.mHandle);
        sDesc->mHandle = PDL_INVALID_SOCKET;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ResetSemError);
    {
        ideLog::log(IDE_SERVER_2, CM_TRC_IPC_SEMRESET_ERROR, errno);
        IDE_SET(ideSetErrorCode(cmERR_ABORT_CMN_SEM_INIT_OP));
    }
    IDE_EXCEPTION(SemServerInitError);
    {
        ideLog::log(IDE_SERVER_2, CM_TRC_IPC_SEMINIT_ERROR, errno);
        IDE_SET(ideSetErrorCode(cmERR_ABORT_CMN_SEM_INIT_OP));
    }
    IDE_EXCEPTION(SemClientInitError);
    {
        ideLog::log(IDE_SERVER_2, CM_TRC_IPC_CLIENT_SEMINIT_ERROR, errno);
        IDE_SET(ideSetErrorCode(cmERR_ABORT_CMN_SEM_INIT_OP));
    }
    IDE_EXCEPTION(InetSocketError);
    {
        ideLog::log(IDE_SERVER_2, "[ERROR] on network operation between server/client\n");
        IDE_SET(ideSetErrorCode(cmERR_ABORT_CMN_SOCKET_CLOSED));
        IDE_ERRLOG(IDE_SERVER_0);
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC cmnLinkPeerHandshakeClientIPC(cmnLinkPeer * /* aLink */)
{
    return IDE_FAILURE;
}

IDE_RC cmnLinkPeerSetOptionsIPC(cmnLinkPeer * /*aLink*/, SInt /*aOption*/)
{
	return IDE_SUCCESS;
}

IDE_RC cmnLinkPeerShutdownServerIPC(cmnLinkPeer *aLink,
                                    cmnDirection aDirection,
                                    cmnShutdownMode aMode)
{
    cmnLinkPeerIPC    *sLink = (cmnLinkPeerIPC *)aLink;
    cmnLinkDescIPC    *sDesc = &sLink->mDesc;
    idBool             sLocked = ID_FALSE;

    IDE_TEST(sDesc->mConnectFlag != ID_TRUE);

    if (sDesc->mHandShakeFlag == ID_TRUE)
    {
        // To fix PR-2006
        IDE_TEST_RAISE(idlOS::thread_mutex_lock(&gIpcMutex) != 0, LockError);
        sLocked = ID_TRUE;

        if (gIpcChannelList[sDesc->mChannelID] == ID_TRUE)
        {
            /*
             * BUG-32398
             *
             * Shutdown시 기존처럼 모드에 따라 세마포어를 풀어준다.
             */
            if (aMode == CMN_SHUTDOWN_MODE_FORCE)
            {
                // client가 종료했다고 표시.
                while (0 != idlOS::semop(gIpcSemChannelID[sDesc->mChannelID],
                                            sDesc->mOpSignCliExit,
                                            1))
                {
                    IDE_TEST_RAISE(errno != EINTR, SemOpError);
                }

                // client로 송신대기하는 lock을 풀어줌.
                while (0 != idlOS::semop(gIpcSemChannelID[sDesc->mChannelID],
                                            sDesc->mOpSignSendMoreToCli,
                                            1))
                {
                    IDE_TEST_RAISE(errno != EINTR, SemOpError);
                }

            }

            /*
             * BUG-32398
             *
             * Client 수신대기 세마포어를 풀어주어 세션종료 과정시
             * 블럭되지 않도록 한다.
             *
             * 즉 Shutdown시 FORCE 모드를 제외하고는 서버쪽 세마포어만
             * 풀어주는데 예외적으로 Client 수신대기 세마포어도 풀어준다.
             */
            // client로부터 수신대기하는 lock을 풀어줌.
            while (0 != idlOS::semop(gIpcSemChannelID[sDesc->mChannelID],
                                        sDesc->mOpSignSendToSvr,
                                        1))
            {
                IDE_TEST_RAISE(errno != EINTR, SemOpError);
            }

            // fix BUG-19265
            // server close flag = ON
            while (0 != idlOS::semop(gIpcSemChannelID[sDesc->mChannelID],
                                     sDesc->mOpSignSvrExit,
                                     1))
            {
                IDE_TEST_RAISE(errno != EINTR, SemServerDetectRelaseError);
            }

            // bug-27250 free Buf list can be crushed when client killed.
            // wake up client sending next packet of big data
            while (0 != idlOS::semop(gIpcSemChannelID[sDesc->mChannelID],
                                     sDesc->mOpSignSendMoreToSvr,
                                     1))
            {
                IDE_TEST_RAISE(errno != EINTR, SemClientChannelelaseError);
            }

            // wake up client
            while (0 != idlOS::semop(gIpcSemChannelID[sDesc->mChannelID],
                                     sDesc->mOpSignSendToCli,
                                     1))
            {
                IDE_TEST_RAISE(errno != EINTR, SemClientChannelelaseError);
            }

        }

        sLocked = ID_FALSE;
        IDE_TEST_RAISE(idlOS::thread_mutex_unlock(&gIpcMutex) != 0, UnlockError);
    }

    /*
     * shutdown
     */
    // fix BUG-18833
    if (sLink->mDesc.mHandle != PDL_INVALID_SOCKET)
    {
        IDE_TEST_RAISE(idlOS::shutdown(sLink->mDesc.mHandle, aDirection) != 0, ShutdownError);
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(LockError);
    {
        IDE_SET(ideSetErrorCode(cmERR_FATAL_CMN_MUTEX_LOCK));
    }
    IDE_EXCEPTION(UnlockError);
    {
        IDE_SET(ideSetErrorCode(cmERR_FATAL_CMN_MUTEX_UNLOCK));
    }
    IDE_EXCEPTION(SemServerDetectRelaseError);
    {
        IDE_SET(ideSetErrorCode(cmERR_FATAL_CMN_SEM_OP));
    }
    IDE_EXCEPTION(SemClientChannelelaseError);
    {
        IDE_SET(ideSetErrorCode(cmERR_FATAL_CMN_SEM_OP));
    }
    IDE_EXCEPTION(SemOpError);
    {
        IDE_SET(ideSetErrorCode(cmERR_FATAL_CMN_SEM_OP));
    }
    IDE_EXCEPTION(ShutdownError);
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_SOCKET_SHUTDOWN_FAILED));
    }

    IDE_EXCEPTION_END;

    if (sLocked == ID_TRUE)
    {
        (void)idlOS::thread_mutex_unlock(&gIpcMutex);
        sLocked = ID_FALSE;
    }

    if (gIpcSemChannelID[sDesc->mChannelID] == -1)
    {
        return IDE_SUCCESS;
    }

    return IDE_FAILURE;
}

IDE_RC cmnLinkPeerShutdownClientIPC(cmnLinkPeer * /* aLink */,
                                    cmnDirection /*aDirection*/,
                                    cmnShutdownMode /*aMode*/)
{
    return IDE_SUCCESS;
}

IDE_RC cmnLinkPeerAllocBlockServerIPC(cmnLinkPeer *aLink, cmbBlock **aBlock)
{
    // bug-27250 free Buf list can be crushed when client killed
    // modify: to be same as tcp
    // 변경전: 공유 메모리 block을 할당
    // 변경후:  mAllocBlock에서 일반 block buffer를 할당
    cmbBlock *sBlock;

    IDE_TEST(aLink->mPool->mOp->mAllocBlock(aLink->mPool, &sBlock) != IDE_SUCCESS);

    /*
     * Write Block 초기화
     */
    sBlock->mDataSize    = CMP_HEADER_SIZE;
    sBlock->mCursor      = CMP_HEADER_SIZE;

    /*
     * Write Block을 돌려줌
     */
    *aBlock = sBlock;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC cmnLinkPeerAllocBlockClientIPC(cmnLinkPeer * /* aLink */,
                                      cmbBlock ** /* aBlock */)
{
    return IDE_SUCCESS;
}

IDE_RC cmnLinkPeerFreeBlockServerIPC(cmnLinkPeer *aLink, cmbBlock *aBlock)
{
    // bug-27250 free Buf list can be crushed when client killed
    // 공유 메모리 linked list 조정하는 부분 제거
    // Block 해제
    IDE_TEST(aLink->mPool->mOp->mFreeBlock(aLink->mPool, aBlock) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC cmnLinkPeerFreeBlockClientIPC(cmnLinkPeer * /* aLink */,
                                     cmbBlock * /* aBlock */)
{
    return IDE_SUCCESS;
}

IDE_RC cmnLinkPeerRecvServerIPC(cmnLinkPeer    *aLink,
                                cmbBlock      **aBlock,
                                cmpHeader      *aHeader,
                                PDL_Time_Value */*aTimeout*/)
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
    cmbPool        *sPool = aLink->mPool;
    cmnLinkDescIPC *sDesc = &sLink->mDesc;

    SInt            rc;
    cmbBlock       *sBlock = NULL;
    SChar          *sDataBlock;
    cmpHeader       sHeader;

    union semun     sArg;
    SInt            sSemVal = 0;
    cmpPacketType   sPacketType = aLink->mLink.mPacketType;

    IDE_TEST(sDesc->mConnectFlag != ID_TRUE || sDesc->mHandShakeFlag != ID_TRUE);

    IDV_SESS_ADD(aLink->mStatistics, IDV_STAT_INDEX_RECV_IPC_BLOCK_COUNT, (ULong)1);

    while(1)
    {
        rc = idlOS::semop(gIpcSemChannelID[sDesc->mChannelID],
                          sDesc->mOpWaitSendToSvr, 1);
        if (rc == 0)
        {
            break;
        }
        else
        {
            IDE_TEST_RAISE(EIDRM == errno, SemRemoved);
            IDE_TEST_RAISE(EINTR != errno, SemOpError);
        }
    }

    /* bug-31258: A IPC channel whose client has exited
       might not be released on AIX.
       On AIX, client_detect_try might be failed when the
       client is exited without SQLDisconnect(). so we do
       semctl(GETVAL) to check the client status.
       If a client has exited without SQLDisconnect(),
       semval would be 9, not 0. */
    idlOS::memset(&sArg, 0x00, ID_SIZEOF(sArg));
    sSemVal = idlOS::semctl(gIpcSemChannelID[sDesc->mChannelID],
                            IPC_SEM_SEND_TO_SVR, GETVAL, sArg);
    IDE_TEST_RAISE(sSemVal >= (CMB_SHM_SEMA_UNDO_VALUE-2),
                   ConnectionClosed);

    // disconnect check
    rc = idlOS::semop(gIpcSemChannelID[sDesc->mChannelID],
                      sDesc->mOpCheckCliExit, 2);
    IDE_TEST_RAISE(rc == 0, ConnectionClosed);

    /*
     * BUG-32398
     *
     * FetchTimeout등 서버세션이 먼저 종료한 경우를 체크해
     * 클라이언트와 상관없이 세션의 statement를 정리하자.
     *
     * 최종적인 Close 단계에서 클라이언트의 종료를 기다리기 때문에
     * 다른 클라이언트가 접속할 수 없다.
     */
    rc = idlOS::semop(gIpcSemChannelID[sDesc->mChannelID],
                      sDesc->mOpCheckSvrExit, 2);
    IDE_TEST_RAISE(rc == 0, ConnectionClosed);


    //======================================================
    // bug-27250 free Buf list can be crushed when client killed
    // 변경전: 고정 또는 여분의 block 얻어옴.
    // 변경후: 무조건 고정된 통신 block 얻어옴.

    sDataBlock = cmbShmGetDefaultServerBuffer(gIpcShmBuffer,
                                              gIpcShmInfo.mMaxChannelCount,
                                              sDesc->mChannelID);
    // proj_2160 cm_type removal
    // A5: block을 매 수신시마다 여기서 할당해야 한다
    if (sPacketType == CMP_PACKET_TYPE_A5)
    {
        IDE_TEST(sPool->mOp->mAllocBlock(sPool, &sBlock) != IDE_SUCCESS);
    }
    // A7 or CMP_PACKET_TYPE_UNKNOWN: block이 이미 할당돼 있다
    else
    {
        sBlock = *aBlock;
    }

    // 헤더를 먼저 읽어 data size를 구한 다음에 해당 size 만큼
    // buffer에 복사해 둔다. 다른 buffer로 복사하는 이유는,
    // 이후 marshal 단계에서 복사가 아닌 포인터를 사용하기 때문.
    // read header
    idlOS::memcpy(sBlock->mData, sDataBlock, CMP_HEADER_SIZE);
    sBlock->mDataSize = CMP_HEADER_SIZE;
    // Protocol Header 해석
    IDE_TEST(cmpHeaderRead(aLink, &sHeader, sBlock) != IDE_SUCCESS);

    // read payload
    idlOS::memcpy(sBlock->mData + CMP_HEADER_SIZE,
            sDataBlock + CMP_HEADER_SIZE, sHeader.mA7.mPayloadLength);

    sBlock->mDataSize += sHeader.mA7.mPayloadLength;

    //======================================================
    // bug-27250 free Buf list can be crushed when client killed.
    // 프로토콜 data가 커서 여러 패킷에 나뉘어 수신하는 경우,
    // 다음 패킷을 수신할 준비가 되어 있음을 서버에 알려주어
    // 동기화를 시킨다.(반드시 block 복사 이후에 풀어주어야 함)
    // need to recv next packet.
    // seq end가 아니라는 것은 수신할 다음 패킷이 더 있음을 의미
    if (CMP_HEADER_PROTO_END_IS_SET(&sHeader) == ID_FALSE)
    {
        // 큰 프로토콜 패킷 송신 신호 제어용 semaphore 사용
        while(1)
        {
            rc = idlOS::semop(gIpcSemChannelID[sDesc->mChannelID],
                    sDesc->mOpSignSendMoreToSvr, 1);
            if (rc == 0)
            {
                break;
            }
            else
            {
                IDE_TEST_RAISE(EINTR != errno, SemOpError);
            }
        }
    }

    /* proj_2160 cm_type removal
     *  Do not use mLink.mPacketType. instead, use sPacketType.
     * cuz, mLink's value could be changed in cmpHeaderRead().
     * and, this if-stmt shows explicitly that
     * it needs to return block ptr in only A5. */
    if (sPacketType == CMP_PACKET_TYPE_A5)
    {
        *aBlock  = sBlock;
    }
    *aHeader = sHeader;

    return IDE_SUCCESS;

    IDE_EXCEPTION(SemRemoved);
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_CONNECTION_CLOSED));
    }
    IDE_EXCEPTION(SemOpError);
    {
        IDE_SET(ideSetErrorCode(cmERR_FATAL_CMN_SEM_OP));
    }
    IDE_EXCEPTION(ConnectionClosed);
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_CONNECTION_CLOSED));
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC cmnLinkPeerRecvClientIPC(cmnLinkPeer    * /*aLink*/,
                                cmbBlock      ** /*aBlock*/,
                                cmpHeader      * /*aHeader*/,
                                PDL_Time_Value * /*aTimeout*/)
{
    return IDE_FAILURE;
}

IDE_RC cmnLinkPeerReqCompleteIPC(cmnLinkPeer * aLink)
{
    // write complete

    cmnLinkPeerIPC *sLink = (cmnLinkPeerIPC *)aLink;

    sLink->mPrevWriteBuffer = NULL;

    return IDE_SUCCESS;
}

IDE_RC cmnLinkPeerResCompleteIPC(cmnLinkPeer * aLink)
{
    // read complete

    cmnLinkPeerIPC *sLink = (cmnLinkPeerIPC *)aLink;

    sLink->mPrevReadBuffer = NULL;

    return IDE_SUCCESS;
}

IDE_RC cmnLinkPeerCheckServerIPC(cmnLinkPeer * aLink, idBool * aIsClosed)
{
    cmnLinkPeerIPC *sLink = (cmnLinkPeerIPC *)aLink;
    cmnLinkDescIPC *sDesc = &sLink->mDesc;

    SInt    rc;

    rc = idlOS::semop(gIpcSemChannelID[sDesc->mChannelID],
                      sDesc->mOpCheckCliExit, 2);
    IDE_TEST_RAISE(rc == 0, ConnectionClosed);

    *aIsClosed = ID_FALSE;

    return IDE_SUCCESS;

    IDE_EXCEPTION(ConnectionClosed);
    {
        (void) idlOS::semop(gIpcSemChannelID[sDesc->mChannelID],
                            sDesc->mOpSignCliExit, 1);

        IDE_SET(ideSetErrorCode(cmERR_ABORT_CONNECTION_CLOSED));
    }

    IDE_EXCEPTION_END;

    *aIsClosed = ID_TRUE;

    return IDE_FAILURE;
}

IDE_RC cmnLinkPeerCheckClientIPC(cmnLinkPeer * /* aLink */, idBool * /* aIsClosed */)
{
    return IDE_FAILURE;
}

idBool cmnLinkPeerHasPendingRequestIPC(cmnLinkPeer * /*aLink*/)
{
    return ID_FALSE;
}

IDE_RC cmnLinkPeerSendServerIPC(cmnLinkPeer *aLink, cmbBlock *aBlock)
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

    SInt            rc;

    cmpHeader       sHeader;
    SChar          *sDataBlock;

    IDE_TEST(sDesc->mConnectFlag != ID_TRUE || sDesc->mHandShakeFlag != ID_TRUE);

    // disconnect check
    rc = idlOS::semop(gIpcSemChannelID[sDesc->mChannelID],
                      sDesc->mOpCheckCliExit, 2);
    IDE_TEST_RAISE(rc == 0, ConnectionClosed);

    //=======================================================
    // bug-27250 free Buf list can be crushed when client killed
    // client가 다음 패킷을 수신할 준비 되었는지 확인(try)
    // 준비가 안됐다면 WirteBlockList에 추가되어 나중에 retry.
    if (sLink->mPrevWriteBuffer != NULL)
    {
        // 큰 프로토콜 패킷 송신 신호 제어용 semaphore 사용
        while(1)
        {
            rc = idlOS::semop(gIpcSemChannelID[sDesc->mChannelID],
                    sDesc->mOpCheckSendMoreToCli, 1);
            if (rc == 0)
            {
                break;
            }
            else
            {
                // lock try에 실패하면 goto retry
                IDE_TEST_RAISE(EAGAIN == errno, Retry);
                IDE_TEST_RAISE(EIDRM == errno, SemRemoved);
                IDE_TEST_RAISE(EINTR != errno, SemOpError);
            }
        }
    }

    // bug-27250 free Buf list can be crushed when client killed.
    // block 데이터를 공유 메모리 통신 버퍼로 복사한후 송신한다.
    // 헤더 정보는 datasize를 알기 위해 필요하다.
    // 수신측의 marshal 단계에서 공유 메모리를 직접 참조하는 것을
    // 막기 위해 복사를 한다.
    IDE_TEST(cmpHeaderRead(aLink, &sHeader, aBlock) != IDE_SUCCESS);
    aBlock->mCursor = 0;

    sDataBlock = cmbShmGetDefaultClientBuffer(gIpcShmBuffer,
            gIpcShmInfo.mMaxChannelCount,
            sDesc->mChannelID);
    idlOS::memcpy(sDataBlock, aBlock->mData, aBlock->mDataSize);
    aBlock->mCursor = aBlock->mDataSize;
    //======================================================

    IDV_SESS_ADD(aLink->mStatistics, IDV_STAT_INDEX_SEND_IPC_BLOCK_COUNT, (ULong)1);

    while(1)
    {
        rc = idlOS::semop(gIpcSemChannelID[sDesc->mChannelID],
                          sDesc->mOpSignSendToCli, 1);
        if (rc == 0)
        {
            break;
        }
        else
        {
            IDE_TEST_RAISE(EINTR != errno, SemOpError);
        }
    }

    //=======================================================
    // bug-27250 free Buf list can be crushed when client killed
    // 프로토콜 data가 커서 여러 패킷에 나뉘어 송신하는 경우,
    // 일단 mark만 해둔다.(다음 호출시 위쪽의 코드를 타게 됨)
    // seq end가 아니라는 것은 송신할 다음 패킷이 더 있음을 의미
    if (CMP_HEADER_PROTO_END_IS_SET(&sHeader) == ID_FALSE)
    {
        sLink->mPrevWriteBuffer = sDataBlock;
    }

    /* proj_2160 cm_type removal
     * A7 use static-block for a session */
    if (aLink->mLink.mPacketType == CMP_PACKET_TYPE_A5)
    {
        IDE_TEST(aLink->mPool->mOp->mFreeBlock(aLink->mPool, aBlock)
                 != IDE_SUCCESS);
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(SemOpError);
    {
        IDE_SET(ideSetErrorCode(cmERR_FATAL_CMN_SEM_OP));
    }
    IDE_EXCEPTION(ConnectionClosed);
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_CONNECTION_CLOSED));
    }
    // bug-27250 free Buf list can be crushed when client killed
    IDE_EXCEPTION(Retry);
    {
        IDE_SET(ideSetErrorCode(cmERR_RETRY_SOCKET_OPERATION_WOULD_BLOCK));
    }
    IDE_EXCEPTION(SemRemoved);
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_CONNECTION_CLOSED));
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC cmnLinkPeerSendClientIPC(cmnLinkPeer * /*aLink*/, cmbBlock * /*aBlock*/)
{
    return IDE_FAILURE;
}

/* 
 * TASK-5894 Permit sysdba via IPC
 *
 * IPC에서 SYSDBA 접속을 허용하기 위해 채널 1개는 항상 비워둔다.
 * 일반 사용자가 접속 했을 때 남은 채널수를 확인해 허용 여부를 결정한다.
 */
IDE_RC cmnLinkPeerPermitConnectionServerIPC(cmnLinkPeer* /*aLink*/,
                                            idBool         aHasSysdbaViaIPC,
                                            idBool         aIsSysdba)
{
    idBool sNeedUnlock = ID_FALSE;

    IDE_TEST_RAISE( idlOS::thread_mutex_lock(&gIpcMutex) != 0, LockError );
    sNeedUnlock = ID_TRUE;

    /* IPC Channel이 꽉 찬 경우,
     * Sysdba가 접속되어 있지 않고 접속시도중인 클라이언트가 
     * Sysdba가 아니면 접속을 허용하지 않아야 한다. */
    IDE_TEST_RAISE( (gIpcShmInfo.mUsedChannelCount >= gIpcShmInfo.mMaxChannelCount) &&
                    (aHasSysdbaViaIPC == ID_FALSE) &&
                    (aIsSysdba == ID_FALSE),
                    NoChannelError );

    sNeedUnlock = ID_FALSE;
    IDE_TEST_RAISE( idlOS::thread_mutex_unlock(&gIpcMutex) != 0, UnlockError );

    return IDE_SUCCESS;

    IDE_EXCEPTION(LockError);
    {
        IDE_SET(ideSetErrorCode(cmERR_FATAL_CMN_MUTEX_LOCK));
    }
    IDE_EXCEPTION(UnlockError);
    {
        IDE_SET(ideSetErrorCode(cmERR_FATAL_CMN_MUTEX_UNLOCK));
    }
    IDE_EXCEPTION( NoChannelError )
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_CMN_ERR_FULL_IPC_CHANNEL));
    }
    IDE_EXCEPTION_END;

    if (sNeedUnlock == ID_TRUE)
    {
        (void) idlOS::thread_mutex_unlock(&gIpcMutex);
    }

    return IDE_FAILURE;
}

struct cmnLinkOP gCmnLinkPeerServerOpIPC =
{
    "IPC-PEER-SERVER",

    cmnLinkPeerInitializeServerIPC,
    cmnLinkPeerFinalizeIPC,

    cmnLinkPeerCloseServerIPC,

    cmnLinkPeerGetHandleIPC,

    cmnLinkPeerGetDispatchInfoIPC,
    cmnLinkPeerSetDispatchInfoIPC
};

struct cmnLinkPeerOP gCmnLinkPeerPeerServerOpIPC =
{
    cmnLinkPeerSetBlockingModeIPC,

    cmnLinkPeerGetInfoIPC,
    cmnLinkPeerGetDescIPC,

    cmnLinkPeerConnectIPC,
    cmnLinkPeerSetOptionsIPC,

    cmnLinkPeerAllocChannelServerIPC,
    cmnLinkPeerHandshakeServerIPC,

    cmnLinkPeerShutdownServerIPC,

    cmnLinkPeerRecvServerIPC,
    cmnLinkPeerSendServerIPC,

    cmnLinkPeerReqCompleteIPC,
    cmnLinkPeerResCompleteIPC,

    cmnLinkPeerCheckServerIPC,
    cmnLinkPeerHasPendingRequestIPC,

    cmnLinkPeerAllocBlockServerIPC,
    cmnLinkPeerFreeBlockServerIPC,

    /* TASK-5894 Permit sysdba via IPC */
    cmnLinkPeerPermitConnectionServerIPC, 

    /* PROJ-2474 SSL/TSL */
    NULL, 
    NULL, 
    NULL,

    /* PROJ-2625 Semi-async Prefetch, Prefetch Auto-tuning */
    NULL,
    NULL,
    NULL,
    NULL
};


IDE_RC cmnLinkPeerServerMapIPC(cmnLink *aLink)
{
    cmnLinkPeer *sLink = (cmnLinkPeer *)aLink;

    /*
     * Link 검사
     */
    IDE_ASSERT(aLink->mType == CMN_LINK_TYPE_PEER_SERVER);
    IDE_ASSERT(aLink->mImpl == CMN_LINK_IMPL_IPC);

    /*
     * BUGBUG: IPC Pool 획득
     */
    IDE_TEST(cmbPoolGetSharedPool(&sLink->mPool, CMB_POOL_IMPL_IPC) != IDE_SUCCESS);

    /*
     * 함수 포인터 세팅
     */
    aLink->mOp     = &gCmnLinkPeerServerOpIPC;
    sLink->mPeerOp = &gCmnLinkPeerPeerServerOpIPC;

    /*
     * 멤버 초기화
     */
    sLink->mStatistics = NULL;
    sLink->mUserPtr    = NULL;

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

UInt cmnLinkPeerServerSizeIPC()
{
    return ID_SIZEOF(cmnLinkPeerIPC);
}

struct cmnLinkOP gCmnLinkPeerClientOpIPC =
{
    "IPC-PEER-CLIENT",

    cmnLinkPeerInitializeClientIPC,
    cmnLinkPeerFinalizeIPC,

    cmnLinkPeerCloseClientIPC,

    cmnLinkPeerGetHandleIPC,

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


IDE_RC cmnLinkPeerClientMapIPC(cmnLink *aLink)
{
    cmnLinkPeer *sLink = (cmnLinkPeer *)aLink;

    /*
     * Link 검사
     */
    IDE_ASSERT(aLink->mType == CMN_LINK_TYPE_PEER_CLIENT);
    IDE_ASSERT(aLink->mImpl == CMN_LINK_IMPL_IPC);

    /*
     * BUGBUG: IPC Pool 획득
     */
    IDE_TEST(cmbPoolGetSharedPool(&sLink->mPool, CMB_POOL_IMPL_IPC) != IDE_SUCCESS);

    /*
     * 함수 포인터 세팅
     */
    aLink->mOp     = &gCmnLinkPeerClientOpIPC;
    sLink->mPeerOp = &gCmnLinkPeerPeerClientOpIPC;

    /*
     * 멤버 초기화
     */
    sLink->mStatistics = NULL;
    sLink->mUserPtr    = NULL;

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

UInt cmnLinkPeerClientSizeIPC()
{
    return ID_SIZEOF(cmnLinkPeerIPC);
}


// bug-27250 free Buf list can be crushed when client killed.
// cmiWriteBlock에서 protocol end packet 송신시
// pending block이 있는 경우 이 코드 수행.
// 송신 대기 list에 큰 프로토콜 packet들이 대기하고 있는 경우,
// receiver가 수신 준비가 됐는지 확인하기 위해 다음 수행
IDE_RC cmnLinkPeerWaitSendServerIPC(cmnLink* aLink)
{

    cmnLinkPeerIPC *sLink = (cmnLinkPeerIPC *)aLink;
    cmnLinkDescIPC *sDesc = &sLink->mDesc;
    SInt            rc;

    // receiver가 송신 허락 신호를 줄때까지 무한 대기
    while(1)
    {
        rc = idlOS::semop(gIpcSemChannelID[sDesc->mChannelID],
                          sDesc->mOpWaitSendMoreToCli, 2);
        if (rc == 0)
        {
            break;
        }
        else
        {
            IDE_TEST_RAISE(EIDRM == errno, SemRemoved);
            IDE_TEST_RAISE(EINTR != errno, SemOpError);
        }
    }

    // disconnect check
    rc = idlOS::semop(gIpcSemChannelID[sDesc->mChannelID],
                      sDesc->mOpCheckCliExit, 2);
    IDE_TEST_RAISE(rc == 0, ConnectionClosed);

    return IDE_SUCCESS;

    IDE_EXCEPTION(SemRemoved);
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_CONNECTION_CLOSED));
    }
    IDE_EXCEPTION(SemOpError);
    {
        IDE_SET(ideSetErrorCode(cmERR_FATAL_CMN_SEM_OP));
    }
    IDE_EXCEPTION(ConnectionClosed);
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_CONNECTION_CLOSED));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

// bug-27250 free Buf list can be crushed when client killed.
// cmiWriteBlock에서 protocol end packet 송신시
// pending block이 있는 경우 이 코드 수행
IDE_RC cmnLinkPeerWaitSendClientIPC(cmnLink* /*aLink*/)
{
    return IDE_FAILURE;
}

#endif
