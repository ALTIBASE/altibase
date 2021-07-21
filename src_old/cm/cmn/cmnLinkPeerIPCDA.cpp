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
#include <stdlib.h>

#if !defined(CM_DISABLE_IPCDA)

/* IPC DA Channel Information */
extern cmbShmIPCDAInfo    gIPCDAShmInfo;
extern iduMutex           gIPCDALock;
extern PDL_thread_mutex_t gIPCDAMutex;

/* IPCDA Handshake Message */
#define IPCDA_HELLO_MSG          "IPCDA_HELLO"
#define IPCDA_READY_MSG          "IPCDA_READY"
#define IPCDA_MSG_PREFIX_CNT     (4)
#define IPCDA_MSG_CNT            (11)
#define IPCDA_PID_PREFIX_CNT     (5)
#define IPCDA_PID_CNT            (5)
#define IPCDA_PROTO_CNT          IPCDA_MSG_PREFIX_CNT + IPCDA_MSG_CNT + IPCDA_PID_PREFIX_CNT + IPCDA_PID_CNT


UChar * cmnLinkPeerGetReadBlock(UInt aChannelID)
{
    return cmbShmIPCDAGetDefaultClientBuffer(gIPCDAShmInfo.mShmBuffer,
                                             gIPCDAShmInfo.mMaxChannelCount,
                                             aChannelID);
}

UChar * cmnLinkPeerGetWriteBlock(UInt aChannelID)
{
    return cmbShmIPCDAGetDefaultServerBuffer(gIPCDAShmInfo.mShmBuffer,
                                             gIPCDAShmInfo.mMaxChannelCount,
                                             aChannelID);
}

UChar * cmnLinkPeerGetReadBlockForSimpleQuery(UInt aChannelID)
{
    return cmbShmIPCDAGetServerReadDataBlock(gIPCDAShmInfo.mShmBufferForSimpleQuery,
                                             gIPCDAShmInfo.mMaxChannelCount,
                                             aChannelID);
}

UChar * cmnLinkPeerGetWriteBlockForSimpleQuery(UInt aChannelID)
{
    return cmbShmIPCDAGetServerWriteDataBlock(gIPCDAShmInfo.mShmBufferForSimpleQuery,
                                              gIPCDAShmInfo.mMaxChannelCount,
                                              aChannelID);
}

IDE_RC cmnLinkPeerInitIPCDA(void *aCtx, UInt aChannelID )
{
    cmiProtocolContext *sCtx = (cmiProtocolContext*)aCtx;

    sCtx->mReadBlock = (cmbBlock*)cmnLinkPeerGetReadBlock(aChannelID );

    sCtx->mWriteBlock = (cmbBlock*)cmnLinkPeerGetWriteBlock(aChannelID );

    sCtx->mSimpleQueryFetchIPCDAWriteBlock.mData =
                    (UChar *)cmnLinkPeerGetWriteBlockForSimpleQuery(aChannelID);

    return IDE_SUCCESS;
}

IDE_RC cmnLinkPeerInitSvrWriteIPCDA(void *aCtx)
{
    cmiProtocolContext *sCtx        = (cmiProtocolContext *)aCtx;
    cmbBlockIPCDA      *sWriteBlock = NULL;
    cmnLinkPeerIPCDA   *sLink       = NULL;
    cmnLinkDescIPCDA   *sDesc       = NULL;

    IDE_TEST(sCtx->mIsDisconnect == ID_TRUE);

    sWriteBlock = (cmbBlockIPCDA *)sCtx->mWriteBlock;

    /* BUG-46390 WriteBlock 초기화 작업이므로 WFlag는 무조건 DEACTIVATED 이어야 함 
     * DEACTIVATED가 아닐경우 cmiLinkPeerFinalizeSvrForIPCDA가 호출되지 않았다는 의미,
     * 따라서 connection을 종료한다. */
    IDE_TEST_RAISE(acpAtomicGet32(&sWriteBlock->mWFlag) != CMB_IPCDA_SHM_DEACTIVATED, lockFlagError);

    /* BUG-46502 atomic 함수 적용, atomic에 mem_barrier가 포함되어 있다.*/
    acpAtomicSet32(&sWriteBlock->mWFlag, CMB_IPCDA_SHM_ACTIVATED);

    sWriteBlock->mBlock.mData      = &sWriteBlock->mData;
    sWriteBlock->mBlock.mBlockSize = CMB_BLOCK_DEFAULT_SIZE - sizeof(cmbBlockIPCDA);
    sWriteBlock->mBlock.mCursor    = CMP_HEADER_SIZE;

    acpAtomicSet32(&sWriteBlock->mRFlag, CMB_IPCDA_SHM_ACTIVATED);

    return IDE_SUCCESS;

    IDE_EXCEPTION(lockFlagError);
    {
        sLink = (cmnLinkPeerIPCDA*)sCtx->mLink;
        sDesc = &sLink->mDesc;
        ideLog::log(IDE_SERVER_0, "[IPCDA Recv] WriteBuffer LockFlag Error! "
                                  "chnlID: %"ID_INT32_FMT" "
                                  "RFlag: %"ID_INT32_FMT" "
                                  "WFlag: %"ID_INT32_FMT" "
                                  "OperationCount: %"ID_INT32_FMT" ",
                                  sDesc->mChannelID, sWriteBlock->mRFlag, sWriteBlock->mWFlag, sWriteBlock->mOperationCount );

    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC cmnLinkPeerInitializeServerIPCDA(cmnLink *aLink)
{
    cmnLinkPeerIPCDA *sLink = (cmnLinkPeerIPCDA *)aLink;
    cmnLinkDescIPCDA *sDesc = &sLink->mDesc;

    /* 멤버 초기화 */
    sDesc->mConnectFlag   = ID_TRUE;
    sDesc->mHandShakeFlag = ID_FALSE;

    sDesc->mHandle        = PDL_INVALID_SOCKET;
    sDesc->mChannelID     = -1;

    sDesc->mSemChannelKey = -1;
    sDesc->mSemChannelID  = PDL_INVALID_HANDLE;

    idlOS::memset(&(sDesc->mSemInfo), 0, ID_SIZEOF(semid_ds));

    return IDE_SUCCESS;
}

IDE_RC cmnLinkPeerInitializeClientIPCDA(cmnLink * /*aLink*/)
{
    return IDE_SUCCESS;
}

IDE_RC cmnLinkPeerFinalizeIPCDA(cmnLink *aLink)
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
IDE_RC cmnLinkPeerCloseServerIPCDA(cmnLink *aLink)
{
    cmnLinkPeerIPCDA       *sLink        = (cmnLinkPeerIPCDA *)aLink;
    cmnLinkDescIPCDA       *sDesc        = &sLink->mDesc;
    cmbShmIPCDAChannelInfo *sChannelInfo = NULL;
    idBool                  sLocked      = ID_FALSE;

#if defined(ALTI_CFG_OS_LINUX)
        if( cmuProperty::isIPCDAMessageQ() == ID_TRUE )
        {
            mq_unlink( sLink->mMessageQ.mName );
        }
#endif

    IDE_TEST(sDesc->mConnectFlag != ID_TRUE);

    if (sDesc->mHandShakeFlag == ID_TRUE)
    {
        // To fix PR-2006
        /* BUG-44468 [cm] codesonar warning in CM */
        IDE_ASSERT( gIPCDALock.lock(NULL) == IDE_SUCCESS ); 
        sLocked = ID_TRUE;
        
        if (gIPCDAShmInfo.mChannelList[sDesc->mChannelID] == ID_TRUE)
        {
            /* BUG-46390 normal server mode : waiting for client to be killed */
            while (0 != idlOS::semop(gIPCDAShmInfo.mSemChannelID[sDesc->mChannelID],
                                     sDesc->mOpWaitCliExit,
                                     2))
            {
                IDE_TEST_RAISE(errno != EINTR, SemWaitClientDeadError);
            }

            /*
             * BUG-32398
             * 값을 변경해 여러 Client로의 혼선을 예방하자.
             */
            sChannelInfo = cmbShmIPCDAGetChannelInfo( gIPCDAShmInfo.mShmBuffer, sDesc->mChannelID );
            sChannelInfo->mTicketNum++;

            gIPCDAShmInfo.mUsedChannelCount--;
            gIPCDAShmInfo.mChannelList[sDesc->mChannelID] = ID_FALSE;
            sDesc->mConnectFlag = ID_FALSE;
        }
        // To fix PR-2006
        sLocked = ID_FALSE;
        /* BUG-44468 [cm] codesonar warning in CM */
        IDE_ASSERT( gIPCDALock.unlock() == IDE_SUCCESS );
    }

    /* socket이 열려있으면 닫음 */
    if (sLink->mDesc.mHandle != PDL_INVALID_SOCKET)
    {
        idlOS::closesocket(sLink->mDesc.mHandle);
        sLink->mDesc.mHandle = PDL_INVALID_SOCKET;
    }

    sLink->mDesc.mConnectFlag = ID_FALSE;

    return IDE_SUCCESS;

    IDE_EXCEPTION(SemWaitClientDeadError);
    {
        IDE_SET(ideSetErrorCode(cmERR_FATAL_CMN_SEM_OP));
    }
    IDE_EXCEPTION_END;

    if (sLocked == ID_TRUE)
    {
        /* BUG-44468 [cm] codesonar warning in CM */
        IDE_ASSERT( gIPCDALock.unlock() == IDE_SUCCESS );
        sLocked = ID_FALSE;
    }

    return IDE_FAILURE;
}

IDE_RC cmnLinkPeerCloseClientIPCDA(cmnLink * /*aLink*/)
{
    /* not used */
    return IDE_SUCCESS;
}

IDE_RC cmnLinkPeerGetHandleIPCDA(cmnLink */*aLink*/, void */*aHandle*/)
{
    /* not used */
    return IDE_SUCCESS;
}

IDE_RC cmnLinkPeerGetDispatchInfoIPCDA(cmnLink * /*aLink*/, void * /*aDispatchInfo*/)
{
    /* not used */
   return IDE_SUCCESS;
}

IDE_RC cmnLinkPeerSetDispatchInfoIPCDA(cmnLink * /*aLink*/, void * /*aDispatchInfo*/)
{
    /* not used */
    return IDE_SUCCESS;
}

IDE_RC cmnLinkPeerSetBlockingModeIPCDA(cmnLinkPeer * /*aLink*/, idBool /*aBlockingMode*/)
{
    /* not used */
    return IDE_SUCCESS;
}

IDE_RC cmnLinkPeerGetInfoIPCDA(cmnLinkPeer * /*aLink*/, SChar *aBuf, UInt aBufLen, cmiLinkInfoKey aKey)
{
    SInt sRet;

    switch (aKey)
    {
        case CMN_LINK_INFO_ALL:
        case CMN_LINK_INFO_IMPL:
        case CMN_LINK_INFO_IPCDA_KEY:
            sRet = idlOS::snprintf(aBuf, aBufLen, "IPCDA");

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

IDE_RC cmnLinkPeerGetDescIPCDA(cmnLinkPeer *aLink, void *aDesc)
{
    cmnLinkPeerIPCDA *sLink = (cmnLinkPeerIPCDA *)aLink;

    /* Desc를 돌려줌 */
    *(cmnLinkDescIPCDA **)aDesc = &sLink->mDesc;

    return IDE_SUCCESS;
}

IDE_RC cmnLinkPeerConnectIPCDA(cmnLinkPeer       *aLink,
                             cmnLinkConnectArg *aConnectArg,
                             PDL_Time_Value    *aTimeout,
                             SInt              /*aOption*/)
{
    cmnLinkPeerIPCDA    *sLink = (cmnLinkPeerIPCDA *)aLink;
    UInt               sPathLen;

    /* socket 생성 */
    sLink->mDesc.mHandle = idlOS::socket(AF_UNIX, SOCK_STREAM, 0);
    IDE_TEST_RAISE(sLink->mDesc.mHandle == PDL_INVALID_SOCKET, SocketError);

    /* IPCDA 파일이름 길이 검사 */
    sPathLen = idlOS::strlen(aConnectArg->mIPCDA.mFilePath);
    IDE_TEST_RAISE(ID_SIZEOF(sLink->mDesc.mAddr.sun_path) <= sPathLen, IpcPathTruncated);

    /* IPC 주소 세팅 */
    sLink->mDesc.mAddr.sun_family = AF_UNIX;
    idlOS::snprintf(sLink->mDesc.mAddr.sun_path,
                    ID_SIZEOF(sLink->mDesc.mAddr.sun_path),
                    "%s",
                    aConnectArg->mIPCDA.mFilePath);

    /* connect */
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

IDE_RC cmnLinkPeerAllocChannelServerIPCDA(cmnLinkPeer *aLink, SInt *aChannelID)
{
    UInt   i;
    SInt   sChannelID = -1;
    idBool sLocked    = ID_FALSE;

    cmnLinkPeerIPCDA *sLink = (cmnLinkPeerIPCDA *)aLink;
    cmnLinkDescIPCDA *sDesc = &sLink->mDesc;

    /* BUG-44468 [cm] codesonar warning in CM */
    IDE_ASSERT( gIPCDALock.lock(NULL) == IDE_SUCCESS );

    sLocked = ID_TRUE;

    IDE_TEST_RAISE( gIPCDAShmInfo.mUsedChannelCount >= gIPCDAShmInfo.mMaxChannelCount,
                    NoChannelError);

    for (i = 0; i < gIPCDAShmInfo.mMaxChannelCount; i++)
    {
        if (gIPCDAShmInfo.mChannelList[i] != ID_TRUE)
        {
            sChannelID = i;
            gIPCDAShmInfo.mChannelList[i] = ID_TRUE;
            break;
        }
    }

    IDE_TEST_RAISE( sChannelID == -1, NoChannelError);

    gIPCDAShmInfo.mUsedChannelCount++;

    // BUG-25420 [CodeSonar] Lock, Unlock 에러 핸들링 오류에 의한 Double Unlock
    // unlock 를 하기전에 세팅을해야 Double Unlock 을 막을수 있다.
    sLocked = ID_FALSE;
    /* BUG-44468 [cm] codesonar warning in CM */
    IDE_ASSERT( gIPCDALock.unlock() == IDE_SUCCESS );

    sDesc->mChannelID = sChannelID;

    *aChannelID = sChannelID;

    return IDE_SUCCESS;

    IDE_EXCEPTION(NoChannelError);
    {
        SChar               sProtoBuffer[32];  /* BUG-46054 */
        cmnLinkDescIPCDAMsg sIPCMsg;

        sIPCMsg.mChannelID = -1;

        idlVA::recv_i(sDesc->mHandle, sProtoBuffer, IPCDA_PROTO_CNT);
        idlVA::send_i(sDesc->mHandle, &sIPCMsg, ID_SIZEOF(cmnLinkDescIPCDAMsg));

        IDE_SET(ideSetErrorCode(cmERR_ABORT_CMN_ERR_FULL_IPC_CHANNEL,
                                gIPCDAShmInfo.mMaxChannelCount,
                                gIPCDAShmInfo.mUsedChannelCount,
                                CMB_BLOCK_DEFAULT_SIZE));

    }
    IDE_EXCEPTION_END;

    *aChannelID       = -1;
    sDesc->mChannelID = -1;

    if (sChannelID != -1)
    {
        gIPCDAShmInfo.mChannelList[sChannelID] = ID_FALSE;
    }
    if (sLocked == ID_TRUE)
    {
        /* BUG-44468 [cm] codesonar warning in CM */
        IDE_ASSERT( gIPCDALock.unlock() == IDE_SUCCESS );
        sLocked = ID_FALSE;
    }

    return IDE_FAILURE;
}

IDE_RC cmnLinkPeerAllocChannelClientIPCDA(cmnLinkPeer */*aLink*/, SInt */*aChannelID*/)
{
    return IDE_SUCCESS;
}

IDE_RC cmnLinkPeerSetOperation(cmnLinkDescIPCDA *aDesc)
{
    //=====================================================
    // bug-28340 rename semop name for readability
    // 가독성을 위해 semaphore op 변수명을 다음과 같이 변경
    //=====================================================
    // IPCDA_SEM_SERVER_DETECT  > IPCDA_SEM_CHECK_SVR_EXIT (0)
    // server_detect_init     > InitSvrExit  : 서버가 잡고 시작
    // server_detect_try      > CheckSvrExit : 서버가 죽었는지 확인
    // server_detect_release  > SignSvrExit  : 서버가 종료신호 보냄
    //=====================================================
    // IPCDA_SEM_CLIENT_DETECT  > IPCDA_SEM_CHECK_CLI_EXIT (1)
    // client_detect_init     > InitCliExit  : cli가 잡고 시작
    // client_detect_try      > CheckCliExit : cli가 죽었는지 확인
    // client_detect_hold     > WaitCliExit  : cli 종료때까지 대기
    // client_detect_release  > SignCliExit  : cli가 종료신호 보냄
    //=====================================================

    //=====================================================
    // IPCDA_SEM_CHECK_SVR_EXIT (0)
    aDesc->mOpInitSvrExit[0].sem_num = IPCDA_SEM_CHECK_SVR_EXIT;
    aDesc->mOpInitSvrExit[0].sem_op  = -1;
    aDesc->mOpInitSvrExit[0].sem_flg = SEM_UNDO | IPC_NOWAIT;

    aDesc->mOpCheckSvrExit[0].sem_num = IPCDA_SEM_CHECK_SVR_EXIT;
    aDesc->mOpCheckSvrExit[0].sem_op  = -1;
    aDesc->mOpCheckSvrExit[0].sem_flg = IPC_NOWAIT;
    aDesc->mOpCheckSvrExit[1].sem_num = IPCDA_SEM_CHECK_SVR_EXIT;
    aDesc->mOpCheckSvrExit[1].sem_op  = 1;
    aDesc->mOpCheckSvrExit[1].sem_flg = 0;

    aDesc->mOpSignSvrExit[0].sem_num = IPCDA_SEM_CHECK_SVR_EXIT;
    aDesc->mOpSignSvrExit[0].sem_op  = 1;
    aDesc->mOpSignSvrExit[0].sem_flg = 0;

    //=====================================================
    // IPCDA_SEM_CHECK_CLI_EXIT (1)
    aDesc->mOpInitCliExit[0].sem_num = IPCDA_SEM_CHECK_CLI_EXIT;
    aDesc->mOpInitCliExit[0].sem_op  = -1;
    aDesc->mOpInitCliExit[0].sem_flg = SEM_UNDO;

    aDesc->mOpCheckCliExit[0].sem_num = IPCDA_SEM_CHECK_CLI_EXIT;
    aDesc->mOpCheckCliExit[0].sem_op  = -1;
    aDesc->mOpCheckCliExit[0].sem_flg = IPC_NOWAIT;
    aDesc->mOpCheckCliExit[1].sem_num = IPCDA_SEM_CHECK_CLI_EXIT;
    aDesc->mOpCheckCliExit[1].sem_op  =  1;
    aDesc->mOpCheckCliExit[1].sem_flg = 0;

    aDesc->mOpWaitCliExit[0].sem_num = IPCDA_SEM_CHECK_CLI_EXIT;
    aDesc->mOpWaitCliExit[0].sem_op  = -1;
    aDesc->mOpWaitCliExit[0].sem_flg = 0;
    aDesc->mOpWaitCliExit[1].sem_num = IPCDA_SEM_CHECK_CLI_EXIT;
    aDesc->mOpWaitCliExit[1].sem_op  =  1;
    aDesc->mOpWaitCliExit[1].sem_flg = 0;

    aDesc->mOpSignCliExit[0].sem_num = IPCDA_SEM_CHECK_CLI_EXIT;
    aDesc->mOpSignCliExit[0].sem_op  = 1;
    aDesc->mOpSignCliExit[0].sem_flg = 0;

    return IDE_SUCCESS;
}

/* bug-31572: IPC dispatcher hang might occur on AIX.
 * print some sema info for debugging.
 * this function called only in cmnLinkPeerHandshakeServer, below */
void cmnLinkPeerPrintSemaIPCDA(cmnLinkDescIPCDA* aDesc, SInt aSemID, SInt aSemNum)
{
    union semun         sDummyArg;
    SInt                sSemVal;
    SInt                sSemPid;

    idlOS::memset(&sDummyArg, 0x00, ID_SIZEOF(sDummyArg));
    sSemVal = idlOS::semctl(aSemID, aSemNum, GETVAL, sDummyArg);
    sSemPid = idlOS::semctl(aSemID, aSemNum, GETPID, sDummyArg);
    ideLog::log(IDE_SERVER_2, "[IPC dispatcher] re-init "
                "chnlID: %"ID_INT32_FMT" "
                "sem id: %"ID_INT32_FMT" "
                "semnum: %"ID_INT32_FMT" "
                "semval: %"ID_INT32_FMT" "
                "sempid: %"ID_INT32_FMT,
                aDesc->mChannelID,
                aSemID,
                aSemNum,
                sSemVal,
                sSemPid);
}

IDE_RC cmnLinkPeerHandshakeServerIPCDA(cmnLinkPeer *aLink)
{
    cmnLinkPeerIPCDA *sLink = (cmnLinkPeerIPCDA *)aLink;
    cmnLinkDescIPCDA *sDesc = &sLink->mDesc;

    SInt                     rc                  = 0;
    SInt                     sCount              = 0;
    SChar                    sProtoBuffer[32];
    union semun              sArg;
    cmnLinkDescIPCDAMsg      sIPCMsg;
    cmbShmIPCDAChannelInfo  *sChannelInfo        = NULL;
    cmbBlockIPCDA           *sBlockIPCDAForWrite = NULL;
    cmbBlockIPCDA           *sBlockIPCDAForRead  = NULL;
    SChar                   *sPIDPtr             = NULL;

    // fix BUG-21947
    sDesc->mHandShakeFlag = ID_TRUE;

    cmnLinkPeerSetOperation(sDesc);

    // recv hello message
    idlOS::memset(sProtoBuffer, 0x00, 32);
    sCount = idlVA::recv_i(sDesc->mHandle, sProtoBuffer, 32);
    IDE_TEST_RAISE(sCount <= (IPCDA_MSG_PREFIX_CNT + IPCDA_MSG_CNT + IPCDA_PID_PREFIX_CNT), InetSocketError);

    IDV_SESS_ADD(aLink->mStatistics, IDV_STAT_INDEX_RECV_UNIX_BYTE, (ULong)sCount);
    IDV_SESS_ADD(aLink->mStatistics, IDV_STAT_INDEX_RECV_SOCKET_COUNT, 1 );

    rc = idlOS::memcmp(sProtoBuffer + IPCDA_MSG_PREFIX_CNT, 
                       IPCDA_HELLO_MSG, 
                       IPCDA_MSG_CNT);
    IDE_TEST_RAISE(rc != 0, InetSocketError);
    
    sPIDPtr = idlOS::strstr(sProtoBuffer, ";PID=");
    if (sPIDPtr != NULL)
    {
        sLink->mClientPID = (UInt)idlOS::atoi((SChar*)(sPIDPtr + IPCDA_PID_PREFIX_CNT));
    }
    else
    {
        /* do nothing. */
    }

    IDE_ASSERT(sDesc->mChannelID >= 0);

    // init semaphore value
    sArg.val = 1;
    IDE_TEST_RAISE(idlOS::semctl(gIPCDAShmInfo.mSemChannelID[sDesc->mChannelID],
                                 0,
                                 SETVAL,
                                 sArg) != 0,
                   SemServerInitError);
    sArg.val = 1;
    IDE_TEST_RAISE(idlOS::semctl(gIPCDAShmInfo.mSemChannelID[sDesc->mChannelID],
                                 1,
                                 SETVAL,
                                 sArg) != 0,
                   SemServerInitError);

    sArg.val = gIPCDAShmInfo.mMaxBufferCount;
    IDE_TEST_RAISE(idlOS::semctl(gIPCDAShmInfo.mSemChannelID[sDesc->mChannelID],
                                 2,
                                 SETVAL,
                                 sArg) != 0,
                   SemServerInitError);

    sArg.val = gIPCDAShmInfo.mMaxBufferCount;
    IDE_TEST_RAISE(idlOS::semctl(gIPCDAShmInfo.mSemChannelID[sDesc->mChannelID],
                                 3,
                                 SETVAL,
                                 sArg) != 0,
                   SemServerInitError);

    sArg.val = gIPCDAShmInfo.mMaxBufferCount;
    IDE_TEST_RAISE(idlOS::semctl(gIPCDAShmInfo.mSemChannelID[sDesc->mChannelID],
                                 4,
                                 SETVAL,
                                 sArg) != 0,
                   SemServerInitError);
    sArg.val = gIPCDAShmInfo.mMaxBufferCount;
    IDE_TEST_RAISE(idlOS::semctl(gIPCDAShmInfo.mSemChannelID[sDesc->mChannelID],
                                 5,
                                 SETVAL,
                                 sArg) != 0,
                   SemServerInitError);


    sChannelInfo = cmbShmIPCDAGetChannelInfo( gIPCDAShmInfo.mShmBuffer, sDesc->mChannelID );
    /*
     * BUG-32398
     * 타임스탬프 대신 티켓번호을 사용한다.
     * 초단위라서 같은 값을 가지는 클라이언트가 존재할 가능성이 있다.
     */
    sDesc->mTicketNum = sChannelInfo->mTicketNum;

    // set server semaphore to detect server dead
    // fix BUG-19265
    while (0 != idlOS::semop(gIPCDAShmInfo.mSemChannelID[sDesc->mChannelID],
                             sDesc->mOpInitSvrExit, 1))
    {
        /* bug-31572: IPC dispatcher hang might occur on AIX. */
        if (errno == EAGAIN)
        {
            /* print some info for debugging */
            cmnLinkPeerPrintSemaIPCDA(sDesc,
                                      gIPCDAShmInfo.mSemChannelID[sDesc->mChannelID],
                                      IPCDA_SEM_CHECK_SVR_EXIT);

            /* init semval again */
            sArg.val = 1;
            IDE_TEST_RAISE(
                idlOS::semctl(gIPCDAShmInfo.mSemChannelID[sDesc->mChannelID],
                              IPCDA_SEM_CHECK_SVR_EXIT,
                              SETVAL, sArg)
                != 0, SemServerInitError);
        }
        else
        {
            IDE_TEST_RAISE(errno != EINTR, SemServerInitError);
        }
    }

    // send IPCDA information to client
    sIPCMsg.mMaxChannelCount      = gIPCDAShmInfo.mMaxChannelCount;
    sIPCMsg.mMaxBufferCount       = gIPCDAShmInfo.mMaxBufferCount;
    sIPCMsg.mShmKey               = gIPCDAShmInfo.mShmKey;
    sIPCMsg.mShmKeyForSimpleQuery = gIPCDAShmInfo.mShmKeyForSimpleQuery;
    sIPCMsg.mSemChannelKey        = gIPCDAShmInfo.mSemChannelKey[sDesc->mChannelID];
    // bug-26921: del gIpcSemBufferKey => we don't use anymore
    sIPCMsg.mSemBufferKey         = 0;
    sIPCMsg.mChannelID            = sDesc->mChannelID;
    sIPCMsg.mDataBlockSize        = cmuProperty::getIPCDASimpleQueryDataBlockSize();
#if defined(ALTI_CFG_OS_LINUX)
    sIPCMsg.mUseMessageQ           = cmuProperty::isIPCDAMessageQ();
#endif
    sBlockIPCDAForRead = (cmbBlockIPCDA *)cmbShmIPCDAGetDefaultClientBuffer(gIPCDAShmInfo.mShmBuffer,
                                                                            gIPCDAShmInfo.mMaxChannelCount,
                                                                            sDesc->mChannelID);
    sBlockIPCDAForRead->mWFlag                  = CMB_IPCDA_SHM_DEACTIVATED;
    sBlockIPCDAForRead->mRFlag                  = CMB_IPCDA_SHM_DEACTIVATED;
    sBlockIPCDAForRead->mOperationCount         = 0;

    sBlockIPCDAForWrite = (cmbBlockIPCDA *)cmbShmIPCDAGetDefaultServerBuffer(gIPCDAShmInfo.mShmBuffer,
                                                                             gIPCDAShmInfo.mMaxChannelCount,
                                                                             sDesc->mChannelID);
    sBlockIPCDAForWrite->mWFlag                  = CMB_IPCDA_SHM_DEACTIVATED;
    sBlockIPCDAForWrite->mRFlag                  = CMB_IPCDA_SHM_DEACTIVATED;
    sBlockIPCDAForWrite->mOperationCount         = 0;

    sCount = idlVA::send_i(sDesc->mHandle, &sIPCMsg, ID_SIZEOF(cmnLinkDescIPCDAMsg));
    IDE_TEST_RAISE(sCount != ID_SIZEOF(cmnLinkDescIPCDAMsg), InetSocketError);
    IDV_SESS_ADD(aLink->mStatistics, IDV_STAT_INDEX_SEND_UNIX_BYTE, (ULong)sCount);
    IDV_SESS_ADD(aLink->mStatistics, IDV_STAT_INDEX_SEND_SOCKET_COUNT, 1 );

    // recv client ready msg
    sCount = idlVA::recv_i(sDesc->mHandle, sProtoBuffer, IPCDA_MSG_CNT);
    IDE_TEST_RAISE(sCount != IPCDA_MSG_CNT, InetSocketError);
    IDV_SESS_ADD(aLink->mStatistics, IDV_STAT_INDEX_RECV_UNIX_BYTE, (ULong)sCount);
    IDV_SESS_ADD(aLink->mStatistics, IDV_STAT_INDEX_RECV_SOCKET_COUNT, 1 );

    rc = idlOS::memcmp(sProtoBuffer, IPCDA_READY_MSG, IPCDA_MSG_CNT);
    IDE_TEST_RAISE(rc != 0, InetSocketError);

    // fix BUG-18833
    if (sDesc->mHandle != PDL_INVALID_SOCKET)
    {
        idlOS::closesocket(sLink->mDesc.mHandle);
        sDesc->mHandle = PDL_INVALID_SOCKET;
    }

#if defined(ALTI_CFG_OS_LINUX)
    if ( cmuProperty::isIPCDAMessageQ() == ID_TRUE )
    {
        sLink->mMessageQ.mAttr.mq_maxmsg  = CMI_IPCDA_MESSAGEQ_MAX_MESSAGE;
        sLink->mMessageQ.mAttr.mq_msgsize = CMI_IPCDA_MESSAGEQ_MESSAGE_SIZE;

        idlOS::memset( sLink->mMessageQ.mName, 0, CMN_IPCDA_MESSAGEQ_NAME_SIZE);
        idlOS::snprintf( sLink->mMessageQ.mName, ID_SIZEOF(sLink->mMessageQ.mName),
                         "/temp.%d", sDesc->mChannelID+1);

        sLink->mMessageQ.mId = mq_open( sLink->mMessageQ.mName,
                                           O_WRONLY | O_CREAT,
                                           0666,
                                           &(sLink->mMessageQ.mAttr) );
        IDE_TEST( sLink->mMessageQ.mId == -1 );
    }
#endif

    return IDE_SUCCESS;

    IDE_EXCEPTION(SemServerInitError);
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_CMN_SEM_INIT_OP));
    }
    IDE_EXCEPTION(InetSocketError);
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_CMN_SOCKET_CLOSED));
        IDE_ERRLOG(IDE_SERVER_0);
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC cmnLinkPeerHandshakeClientIPCDA(cmnLinkPeer * /*aLink*/)
{
    /* not used */
    return IDE_SUCCESS;
}

IDE_RC cmnLinkPeerShutdownServerIPCDA(cmnLinkPeer    *aLink,
                                      cmnDirection    aDirection,
                                      cmnShutdownMode aMode)
{
    cmnLinkPeerIPCDA *sLink   = (cmnLinkPeerIPCDA *)aLink;
    cmnLinkDescIPCDA *sDesc   = &sLink->mDesc;
    idBool            sLocked = ID_FALSE;

    IDE_TEST(sDesc->mConnectFlag != ID_TRUE);

    if (sDesc->mHandShakeFlag == ID_TRUE)
    {
        /* BUG-44468 [cm] codesonar warning in CM */
        IDE_ASSERT( gIPCDALock.lock(NULL) == IDE_SUCCESS );
        sLocked = ID_TRUE;

        if (gIPCDAShmInfo.mChannelList[sDesc->mChannelID] == ID_TRUE)
        {
            /*
             * BUG-32398
             *
             * Shutdown시 기존처럼 모드에 따라 세마포어를 풀어준다.
             */
            if (aMode == CMN_SHUTDOWN_MODE_FORCE)
            {
                // client가 종료했다고 표시.
                while (0 != idlOS::semop(gIPCDAShmInfo.mSemChannelID[sDesc->mChannelID],
                                         sDesc->mOpSignCliExit,
                                         1))
                {
                    IDE_TEST_RAISE(errno != EINTR, SemOpError);
                }
            }

            // fix BUG-19265
            // server close flag = ON
            while (0 != idlOS::semop(gIPCDAShmInfo.mSemChannelID[sDesc->mChannelID],
                                     sDesc->mOpSignSvrExit,
                                     1))
            {
                IDE_TEST_RAISE(errno != EINTR, SemServerDetectRelaseError);
            }
        }

        sLocked = ID_FALSE;
        /* BUG-44468 [cm] codesonar warning in CM */
        IDE_ASSERT( gIPCDALock.unlock() == IDE_SUCCESS );
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

    IDE_EXCEPTION(SemServerDetectRelaseError);
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
        /* BUG-44468 [cm] codesonar warning in CM */
        IDE_ASSERT( gIPCDALock.unlock() == IDE_SUCCESS );
        sLocked = ID_FALSE;
    }

    if (gIPCDAShmInfo.mSemChannelID[sDesc->mChannelID] == -1)
    {
        return IDE_SUCCESS;
    }

    return IDE_FAILURE;
}

IDE_RC cmnLinkPeerSetOptionsIPCDA(cmnLinkPeer* /*aLink*/, SInt /*aOption*/)
{
    /* not used */
    return IDE_SUCCESS;
}

IDE_RC cmnLinkPeerShutdownClientIPCDA(cmnLinkPeer*    /*aLink*/,
                                      cmnDirection    /*aDirection*/,
                                      cmnShutdownMode /*aMode*/)
{
    /* not used */
    return IDE_SUCCESS;
}

IDE_RC cmnLinkPeerAllocBlockServerIPCDA(cmnLinkPeer* /*aLink*/, cmbBlock** /*aBlock*/)
{
    /* not used */
    return IDE_SUCCESS;
}

IDE_RC cmnLinkPeerAllocBlockClientIPCDA(cmnLinkPeer* /*aLink*/, cmbBlock** /*aBlock*/)
{
    /* not used */
    return IDE_SUCCESS;
}

IDE_RC cmnLinkPeerFreeBlockServerIPCDA(cmnLinkPeer* /*aLink*/, cmbBlock* /*aBlock*/)
{
    /* not used */
    return IDE_SUCCESS;
}

IDE_RC cmnLinkPeerFreeBlockClientIPCDA(cmnLinkPeer* /*aLink*/, cmbBlock* /*aBlock*/)
{
    /* not used */
    return IDE_SUCCESS;
}

IDE_RC cmnLinkPeerRecvServerIPCDA(cmnLinkPeer    * /*aLink*/,
                                  cmbBlock      ** /*aBlock*/,
                                  cmpHeader      * /*aHeader*/,
                                  PDL_Time_Value * /*aTimeout*/)
{
    /* This will be not used in ipc_da. */
    return IDE_SUCCESS;
}

IDE_RC cmnLinkPeerRecvClientIPCDA(cmnLinkPeer    * /*aLink*/,
                                  cmbBlock      ** /*aBlock*/,
                                  cmpHeader      * /*aHeader*/,
                                  PDL_Time_Value * /*aTimeout*/)
{
    /* This is client's part. */
    return IDE_SUCCESS;
}

IDE_RC cmnLinkPeerReqCompleteIPCDA(cmnLinkPeer * aLink)
{
    ACP_UNUSED(aLink);
    return IDE_SUCCESS;
}

IDE_RC cmnLinkPeerResCompleteIPCDA(cmnLinkPeer * aLink)
{
    ACP_UNUSED(aLink);
    return IDE_SUCCESS;
}

IDE_RC cmnLinkPeerCheckServerIPCDA(cmnLinkPeer * aLink, idBool * aIsClosed)
{
    cmnLinkPeerIPCDA *sLink = (cmnLinkPeerIPCDA *)aLink;
    cmnLinkDescIPCDA *sDesc = &sLink->mDesc;

    SInt rc;

    /* BUG-45976 */
    while (1)
    {
        rc = idlOS::semop(gIPCDAShmInfo.mSemChannelID[sDesc->mChannelID],
                          sDesc->mOpCheckCliExit, 2);
        if (rc == 0)
        {
            IDE_RAISE(ConnectionClosed);
        }
        else if ((rc == -1) && (errno == EAGAIN))
        {
            break;
        }
        else
        {
            IDE_TEST_RAISE(errno == EIDRM, ConnectionClosed);
            IDE_TEST_RAISE(errno != EINTR, SemOpError);
        }
    }

    *aIsClosed = ID_FALSE;

    return IDE_SUCCESS;

    IDE_EXCEPTION(ConnectionClosed)
    {
        (void)idlOS::semop(gIPCDAShmInfo.mSemChannelID[sDesc->mChannelID],
                           sDesc->mOpSignCliExit, 1);

        IDE_SET(ideSetErrorCode(cmERR_ABORT_CONNECTION_CLOSED));
    }
    IDE_EXCEPTION(SemOpError)
    {
        IDE_SET(ideSetErrorCode(cmERR_FATAL_CMN_SEM_OP));
    }
    IDE_EXCEPTION_END;

    *aIsClosed = ID_TRUE;

    return IDE_FAILURE;
}

IDE_RC cmnLinkPeerCheckClientIPCDA(cmnLinkPeer * /*aLink*/, idBool * /*aIsClosed*/)
{
    /* not used */
    return IDE_SUCCESS;
}

idBool cmnLinkPeerHasPendingRequestIPCDA(cmnLinkPeer * /*aLink*/)
{
    /* not used */
    return ID_FALSE;
}

IDE_RC cmnLinkPeerSendServerIPCDA(cmnLinkPeer* /*aLink*/, cmbBlock* /*aBlock*/)
{
    /* do nothing. */
    return IDE_SUCCESS;
}

IDE_RC cmnLinkPeerSendClientIPCDA(cmnLinkPeer* /*aLink*/, cmbBlock* /*aBlock*/)
{
    /* This is client's part. */
    return IDE_SUCCESS;
}

struct cmnLinkOP gCmnLinkPeerServerOpIPCDA =
{
    "IPCDA-PEER-SERVER",

    cmnLinkPeerInitializeServerIPCDA,
    cmnLinkPeerFinalizeIPCDA,

    cmnLinkPeerCloseServerIPCDA,

    cmnLinkPeerGetHandleIPCDA,

    cmnLinkPeerGetDispatchInfoIPCDA,
    cmnLinkPeerSetDispatchInfoIPCDA
};

struct cmnLinkPeerOP gCmnLinkPeerPeerServerOpIPCDA =
{
    cmnLinkPeerSetBlockingModeIPCDA,

    cmnLinkPeerGetInfoIPCDA,
    cmnLinkPeerGetDescIPCDA,

    cmnLinkPeerConnectIPCDA,
    cmnLinkPeerSetOptionsIPCDA,

    cmnLinkPeerAllocChannelServerIPCDA,
    cmnLinkPeerHandshakeServerIPCDA,

    cmnLinkPeerShutdownServerIPCDA,

    cmnLinkPeerRecvServerIPCDA,
    cmnLinkPeerSendServerIPCDA,

    cmnLinkPeerReqCompleteIPCDA,
    cmnLinkPeerResCompleteIPCDA,

    cmnLinkPeerCheckServerIPCDA,
    cmnLinkPeerHasPendingRequestIPCDA,

    cmnLinkPeerAllocBlockServerIPCDA,
    cmnLinkPeerFreeBlockServerIPCDA,

    NULL,

    NULL,
    NULL,
    NULL,

    NULL,
    NULL,
    NULL,
    NULL
};


IDE_RC cmnLinkPeerServerMapIPCDA(cmnLink *aLink)
{
    cmnLinkPeer *sLink = (cmnLinkPeer *)aLink;

    /* Link 검사 */
    IDE_ASSERT(aLink->mType == CMN_LINK_TYPE_PEER_SERVER);
    IDE_ASSERT(aLink->mImpl == CMN_LINK_IMPL_IPCDA);

    /* 함수 포인터 세팅 */
    aLink->mOp     = &gCmnLinkPeerServerOpIPCDA;
    sLink->mPeerOp = &gCmnLinkPeerPeerServerOpIPCDA;

    /* 멤버 초기화 */
    sLink->mStatistics = NULL;
    sLink->mUserPtr    = NULL;

    return IDE_SUCCESS;
}

UInt cmnLinkPeerServerSizeIPCDA()
{
    return ID_SIZEOF(cmnLinkPeerIPCDA);
}

struct cmnLinkOP gCmnLinkPeerClientOpIPCDA =
{
    "IPCDA-PEER-CLIENT",

    cmnLinkPeerInitializeClientIPCDA,
    cmnLinkPeerFinalizeIPCDA,

    cmnLinkPeerCloseClientIPCDA,

    cmnLinkPeerGetHandleIPCDA,

    cmnLinkPeerGetDispatchInfoIPCDA,
    cmnLinkPeerSetDispatchInfoIPCDA
};

struct cmnLinkPeerOP gCmnLinkPeerPeerClientOpIPCDA =
{
    cmnLinkPeerSetBlockingModeIPCDA,

    cmnLinkPeerGetInfoIPCDA,
    cmnLinkPeerGetDescIPCDA,

    cmnLinkPeerConnectIPCDA,
    cmnLinkPeerSetOptionsIPCDA,

    cmnLinkPeerAllocChannelClientIPCDA,
    cmnLinkPeerHandshakeClientIPCDA,

    cmnLinkPeerShutdownClientIPCDA,

    cmnLinkPeerRecvClientIPCDA,
    cmnLinkPeerSendClientIPCDA,

    cmnLinkPeerReqCompleteIPCDA,
    cmnLinkPeerResCompleteIPCDA,

    cmnLinkPeerCheckClientIPCDA,
    cmnLinkPeerHasPendingRequestIPCDA,

    cmnLinkPeerAllocBlockClientIPCDA,
    cmnLinkPeerFreeBlockClientIPCDA,

    NULL,

    NULL,
    NULL,
    NULL,

    NULL,
    NULL,
    NULL,
    NULL
};

IDE_RC cmnLinkPeerClientMapIPCDA(cmnLink* /*aLink*/)
{
    return IDE_SUCCESS;
}

UInt cmnLinkPeerClientSizeIPCDA()
{
    return ID_SIZEOF(cmnLinkPeerIPCDA);
}

IDE_RC cmnLinkPeerWaitSendServerIPCDA(cmnLink* /*aLink*/)
{
    /* not used */
    return IDE_SUCCESS;
}

#endif
