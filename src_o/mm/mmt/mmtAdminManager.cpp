/**
 *  Copyright (c) 1999~2017, Altibase Corp. and/or its affiliates. All rights reserved.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Affero General Public License, version 3,
 *  as published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU Affero General Public License for more details.
 *
 *  You should have received a copy of the GNU Affero General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <mmErrorCode.h>
#include <mmtAdminManager.h>
#include <mmtServiceThread.h>

mmcTask  *mmtAdminManager::mTask;
iduMutex  mmtAdminManager::mMutex;
// bug-24366: sendMsgService mutex invalid
idBool    mmtAdminManager::mMutexEnable;


IDE_RC mmtAdminManager::initialize()
{
    mTask = NULL;

    IDE_TEST(mMutex.initialize((SChar*)"ADMIN_THREAD_CONTROL_MUTEX",
                               IDU_MUTEX_KIND_POSIX,
                               IDV_WAIT_INDEX_NULL) != IDE_SUCCESS);
    // bug-24366: sendMsgService mutex invalid
    // mutex가 초기화 되었으므로 사용가능한 상태로 설정.
    // flag 사용처: sendMsgService
    mMutexEnable = ID_TRUE;

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC mmtAdminManager::finalize()
{
    cmiProtocolContext *sCtx;
    UInt                sMsgLen = 0;

    if (mTask != NULL)
    {
        // proj_2160 cm_type removal
        // server shutdown 할때, 빈 패킷 전송을 해주어야함
        // 메시지 프로토콜 종료 플래그 전송하여 cmiRecv에서 loop 탈출
        // 안그러면, isql에서 link failure로 반환됨.
        // cf) A5인 경우 mmcTask::finalize()안에서 처리됨
        sCtx = mTask->getProtocolContext();
        if (cmiGetPacketType(sCtx) != CMP_PACKET_TYPE_A5)
        {
            if ((sCtx->mWriteHeader.mA7.mCmSeqNo != 0) &&
                (CMP_HEADER_PROTO_END_IS_SET(&(sCtx->mWriteHeader)) == ID_FALSE))
            {
                CMI_WRITE_CHECK(sCtx, 1 + 4 + sMsgLen);
                CMI_WOP(sCtx, CMP_OP_DB_Message);
                CMI_WR4(sCtx, &sMsgLen);
                
                if (cmiGetLinkImpl(sCtx) == CMI_LINK_IMPL_IPCDA)
                {
                    MMT_IPCDA_INCREASE_DATA_COUNT(sCtx);
                }
                else
                {
                    (void)cmiSend(sCtx, ID_TRUE);
                }
            }
        }

        mTask->setShutdownTask(ID_FALSE);
        IDE_TEST(mmtSessionManager::freeTask(mTask) != IDE_SUCCESS);
    }

    IDE_TEST(mMutex.destroy() != IDE_SUCCESS);

    // bug-24366: sendMsgService mutex invalid
    // mutex가 destroy 되었으므로 사용 불가능한 상태로 설정.
    // flag 사용처: sendMsgService
    mMutexEnable = ID_FALSE;

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC mmtAdminManager::refreshUserInfo()
{
    mmcSession *sSession;

    IDE_ASSERT(mMutex.lock(NULL /* idvSQL* */) == IDE_SUCCESS);

    if (mTask != NULL)
    {
        sSession = mTask->getSession();

        if (sSession != NULL)
        {
            IDU_FIT_POINT("mmtAdminManager::refreshUserInfo::lock::getUserInfo");
            IDE_TEST( mmtServiceThread::getUserInfoFromDB( sSession->getStatSQL(), /* PROJ-2446 */ 
                                                           sSession->getUserInfo() ) 
                      != IDE_SUCCESS ); 
        }
    }

    IDE_ASSERT(mMutex.unlock() == IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    {
        IDE_ASSERT(mMutex.unlock() == IDE_SUCCESS);
    }

    return IDE_FAILURE;
}

IDE_RC mmtAdminManager::sendMsgService(const SChar *aMessage, SInt aCRFlag, idBool aLogMsg)
{
    cmiProtocolContext *sProtocolContext;
    cmiProtocol         sProtocol;
    cmpArgDBMessageA5  *sArg;
    SInt                sFlag           = 0;
    UInt                sMessageLen     = idlOS::strlen(aMessage);
    UInt                sStrMessageLen  = 0;
    SChar               sStr[10]        = "\n";

    // bug-24366: sendMsgService mutex invalid.
    // 에러 시나리오:
    // db shutdown 명령을 받게 되면 shutdown 과정을 쭉 수행하고,
    // mmPhaseActionShutdownCM 에서 mmtAdminManager::finalize를 호출하여
    // mutex를 destroy하고, 이후에 cmiFinalize 등에서
    // 에러가 발생(ex)semaphore del err)하면 mmm::executeInternal의
    // 예외처리 부분에서 sendMsgService(IDE_CALLBACK_SEND_MSG())를 호출한다.
    // sendMsgService를 호출하는 이유는 에러메시지를 iSQL로도 송신하기 위함.
    // 그러면 본 함수에서 이미 destroy 된 mutex에 lock을 걸려고 하여 assert.
    // 참고로 위의 mmtAdminManager::finalize에서 이미 link도 해제되기 때문에
    // 사실상 cm을 통해 isql로 에러메시지를 송신할 수가 없다.
    // 변경전: mutex 상태와 관계없이 무조건 mutex.lock
    // 변경후: mutex가 이미 destroy 되었다면, iSQL로 메시지는 송신하지 못하고
    // 서버 메시지 로그만 남기도록 함.
    // cf) mmtAdminManager에서 mutex lock을 사용하는데, 필요없어 보인다.
    //   (sysdba 모드는 전체 db에서 단일 session만 가능하므로 필요없지 않을까?)
    if (mMutexEnable == ID_TRUE)
    {
        IDE_ASSERT(mMutex.lock(NULL /* idvSQL* */) == IDE_SUCCESS);

        sFlag = 1; // err시에 unlock 이 필요함을 표시
        if (mTask != NULL)
        {
            sProtocolContext = mTask->getProtocolContext();

            sFlag = 2; // err시에 cm 관련 처리가 필요함을 표시

            if (cmiGetPacketType(sProtocolContext) != CMP_PACKET_TYPE_A5)
            {
                /* BUG-44125 [mm-cli] IPCDA 모드 테스트 중 hang - iloader CLOB */
                if( aCRFlag == 1)
                {
                    sStrMessageLen += idlOS::strlen(sStr);
                    CMI_WRITE_CHECK(sProtocolContext, (1 + 4 + sMessageLen) + (1 + 4 + sStrMessageLen));

                    CMI_WOP(sProtocolContext, CMP_OP_DB_Message);
                    CMI_WR4(sProtocolContext, &sMessageLen);
                    CMI_WCP(sProtocolContext, aMessage, sMessageLen);

                    CMI_WOP(sProtocolContext, CMP_OP_DB_Message);
                    CMI_WR4(sProtocolContext, &sStrMessageLen);
                    CMI_WCP(sProtocolContext, sStr, sStrMessageLen);
                }
                else
                {
                    CMI_WRITE_CHECK(sProtocolContext, 1 + 4 + sMessageLen);

                    CMI_WOP(sProtocolContext, CMP_OP_DB_Message);
                    CMI_WR4(sProtocolContext, &sMessageLen);
                    CMI_WCP(sProtocolContext, aMessage, sMessageLen);
                }

                IDU_FIT_POINT("mmtAdminManager::sendMsgService::lock::cmiSend");
                if (cmiGetLinkImpl(sProtocolContext) == CMI_LINK_IMPL_IPCDA)
                {
                    MMT_IPCDA_INCREASE_DATA_COUNT(sProtocolContext);
                }
                else
                {
                    IDE_TEST(cmiSend(sProtocolContext, ID_FALSE) != IDE_SUCCESS);
                }
            }
            else
            {
                CMI_PROTOCOL_INITIALIZE(sProtocol, sArg, DB, Message);

                IDE_TEST(cmtVariableSetData(&sArg->mMessage,
                                            (UChar *)aMessage,
                                            idlOS::strlen(aMessage)) != IDE_SUCCESS);

                IDE_TEST(cmiWriteProtocol(sProtocolContext, &sProtocol) != IDE_SUCCESS);

                if (aCRFlag == 1)
                {
                    CMI_PROTOCOL_INITIALIZE(sProtocol, sArg, DB, Message);

                    IDE_TEST(cmtVariableSetData(&sArg->mMessage, (UChar *)"\n", 1)
                             != IDE_SUCCESS);

                    IDE_TEST(cmiWriteProtocol(sProtocolContext, &sProtocol)
                             != IDE_SUCCESS);
                }

                IDE_TEST(cmiFlushProtocol(sProtocolContext, ID_FALSE) != IDE_SUCCESS);
            }
        }
        sFlag = 0; // unlock을 안해도 됨을 표시
        IDE_ASSERT(mMutex.unlock() == IDE_SUCCESS);
    }

    if (aLogMsg == ID_TRUE)
    {
        if ((aMessage[0] != '.' || aMessage[1] != 0) && aMessage[0] != '#')
        {
            ideLog::log(IDE_SERVER_0, "%s", aMessage);
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    {
        // bug-24366: sendMsgService mutex invalid.
        // flag가 1 이상이면 lock 상태이므로 unlock을 해야 한다.
        if (sFlag != 0)
        {
            IDE_ASSERT(mMutex.unlock() == IDE_SUCCESS);

            if ((sFlag == 2) &&
                (cmiGetPacketType(sProtocolContext) == CMP_PACKET_TYPE_A5))
            {
                IDE_PUSH();
                // bug-26983: codesonar: return value ignored
                // void 나 assert 로 처리해야 하는데 메모리 해제 및
                // 초기화가 실패하면 안되므로 assert로 처리하기로 함.
                IDE_ASSERT(cmiFinalizeProtocol(&sProtocol) == IDE_SUCCESS);
                IDE_POP();
            }
        }

        ideLog::log(IDE_SERVER_0, MM_TRC_SYSDBA_DISCONNECTED);
    }

    return IDE_FAILURE;
}

IDE_RC mmtAdminManager::sendMsgPreProcess(const SChar *aMessage, SInt /*aCRFlag*/, idBool aLogMsg)
{
    if (aLogMsg == ID_TRUE)
    {
        if ((aMessage[0] != '.' || aMessage[1] != 0) && aMessage[0] != '#')
        {
            ideLog::log(IDE_SERVER_0, "%s", aMessage);
        }
    }

    return IDE_SUCCESS;
}

IDE_RC mmtAdminManager::sendMsgConsole(const SChar *aMessage, SInt aCRFlag, idBool aLogMsg)
{
    idlOS::fprintf(stdout, "%s", aMessage);
    idlOS::fflush(stdout);

    if (aCRFlag == 1)
    {
   	idlOS::printf("\n");
	idlOS::fflush(stdout);
    }

    if (aLogMsg == ID_TRUE)
    {
        if ((aMessage[0] != '.' || aMessage[1] != 0) && aMessage[0] != '#')
        {
            ideLog::log(IDE_SERVER_0, "%s", aMessage);
        }
    }

    return IDE_SUCCESS;
}

IDE_RC mmtAdminManager::sendNChar()
{
    cmiProtocolContext          *sProtocolContext;
    cmiProtocol                  sProtocol;
    cmpArgDBPropertyGetResultA5 *sArg;
    UShort                       sPropertyID;
    SChar                       *sCharSet;
    SChar                       *sNCharSet;
    UInt                         sLen;
    UInt                         sNLen;

    IDE_ASSERT(mMutex.lock(NULL /* idvSQL* */) == IDE_SUCCESS);

    if (mTask != NULL)
    {
        sProtocolContext = mTask->getProtocolContext();

        if (cmiGetPacketType(sProtocolContext) != CMP_PACKET_TYPE_A5)
        {
            //BUGBUG : 매번 전송하지 말고 NCHAR 값이 유효할때만 전송하도록 변경할 것

            // CMP_DB_PROPERTY_NLS_CHARACTERSET
            sPropertyID = CMP_DB_PROPERTY_NLS_CHARACTERSET;
            sCharSet    = smiGetDBCharSet();
            sLen        = idlOS::strlen(sCharSet);

            /* BUG-44125 [mm-cli] IPCDA 모드 테스트 중 hang - iloader CLOB */
            sNCharSet    = smiGetNationalCharSet();
            sNLen        = idlOS::strlen(sNCharSet);

            CMI_WRITE_CHECK(sProtocolContext, (1 + 2 + 4 + sLen) + (1 + 2 + 4 + sNLen));

            CMI_WOP(sProtocolContext, CMP_OP_DB_PropertyGetResult);
            CMI_WR2(sProtocolContext, &sPropertyID);
            CMI_WR4(sProtocolContext, &sLen);
            CMI_WCP(sProtocolContext, sCharSet, sLen);

            // CMP_DB_PROPERTY_NLS_NCHAR_CHARACTERSET
            sPropertyID = CMP_DB_PROPERTY_NLS_NCHAR_CHARACTERSET;

            CMI_WOP(sProtocolContext, CMP_OP_DB_PropertyGetResult);
            CMI_WR2(sProtocolContext, &sPropertyID);
            CMI_WR4(sProtocolContext, &sNLen);
            CMI_WCP(sProtocolContext, sNCharSet, sNLen);

            IDU_FIT_POINT("mmtAdminManager::sendNChar::lock::cmiSend");
            if (cmiGetLinkImpl(sProtocolContext) == CMI_LINK_IMPL_IPCDA)
            {
                MMT_IPCDA_INCREASE_DATA_COUNT(sProtocolContext);
            }
            else
            {
                IDE_TEST(cmiSend(sProtocolContext, ID_FALSE) != IDE_SUCCESS);
            }

        }
        else
        {
            CMI_PROTOCOL_INITIALIZE(sProtocol, sArg, DB, PropertyGetResult);

            // CMP_DB_PROPERTY_NLS_CHARACTERSET
            sArg->mPropertyID = CMP_DB_PROPERTY_NLS_CHARACTERSET;
            sCharSet = smiGetDBCharSet();
            IDE_TEST(cmtAnyWriteVariable(&sArg->mValue,
                                         (UChar *)sCharSet,
                                         idlOS::strlen(sCharSet)) != IDE_SUCCESS);
            IDE_TEST(cmiWriteProtocol(sProtocolContext, &sProtocol) != IDE_SUCCESS);

            // CMP_DB_PROPERTY_NLS_NCHAR_CHARACTERSET
            sArg->mPropertyID = CMP_DB_PROPERTY_NLS_NCHAR_CHARACTERSET;
            sCharSet = smiGetNationalCharSet();
            IDE_TEST(cmtAnyWriteVariable(&sArg->mValue,
                                         (UChar *)sCharSet,
                                         idlOS::strlen(sCharSet)) != IDE_SUCCESS);
            IDE_TEST(cmiWriteProtocol(sProtocolContext, &sProtocol) != IDE_SUCCESS);

            IDE_TEST(cmiFlushProtocol(sProtocolContext, ID_FALSE) != IDE_SUCCESS);
        }
    }

    IDE_ASSERT(mMutex.unlock() == IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    {
        IDE_ASSERT(mMutex.unlock() == IDE_SUCCESS);

        if (cmiGetPacketType(sProtocolContext) == CMP_PACKET_TYPE_A5)
        {
            IDE_PUSH();
            // bug-26983: codesonar: return value ignored
            // void 나 assert 로 처리해야 하는데 메모리 해제 및
            // 초기화가 실패하면 안되므로 assert로 처리하기로 함.
            IDE_ASSERT(cmiFinalizeProtocol(&sProtocol) == IDE_SUCCESS);
            IDE_POP();
        }

        ideLog::log(IDE_SERVER_0, MM_TRC_SYSDBA_DISCONNECTED);
    }

    return IDE_FAILURE;
}

void mmtAdminManager::waitForAdminTaskEnd()
{
    idBool sIsConnected;

    while (1)
    {
        IDE_ASSERT(mMutex.lock(NULL /* idvSQL* */) == IDE_SUCCESS);

        sIsConnected = isConnected();

        IDE_ASSERT(mMutex.unlock() == IDE_SUCCESS);

        if (sIsConnected == ID_FALSE)
        {
            break;
        }

        idlOS::sleep(1);
    }
}

IDE_RC mmtAdminManager::setTask(mmcTask *aTask)
{
    IDE_ASSERT(mMutex.lock(NULL /* idvSQL* */) == IDE_SUCCESS);


    IDU_FIT_POINT("mmtAdminManager::setTask::lock::Task");
    IDE_TEST(mTask != NULL);

    mTask = aTask;

    IDE_ASSERT(mMutex.unlock() == IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_ADMIN_ALREADY_RUNNING));

        IDE_ASSERT(mMutex.unlock() == IDE_SUCCESS);
    }

    return IDE_FAILURE;
}

IDE_RC mmtAdminManager::unsetTask(mmcTask *aTask)
{
    IDE_ASSERT(mMutex.lock(NULL /* idvSQL* */) == IDE_SUCCESS);

    if (mTask == aTask)
    {
        mTask = NULL;
    }

    IDE_ASSERT(mMutex.unlock() == IDE_SUCCESS);

    return IDE_SUCCESS;
}
