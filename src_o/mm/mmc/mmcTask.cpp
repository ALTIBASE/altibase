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

#include <idsPassword.h>
#include <idp.h>
#include <mmErrorCode.h>
#include <mmcTask.h>
#include <mmcSession.h>
#include <mmtServiceThread.h>
#include <mmtSessionManager.h>
#include <mmuProperty.h>
#include <sdi.h>
#include <sduProperty.h>

static const SChar *gMmcTaskStateName[MMC_TASK_STATE_MAX] =
{
    "Waiting",
    "Ready",
    "Executing",
    "Queue Waiting",
    "Queue Ready",
};

static const SChar *gMmcSessionStateName[MMC_SESSION_STATE_MAX] =
{
    "Initialized",
    "Authenticated",
    "Service Ready",
    "Service",
    "End",
    "Rollback",
};

static const SChar *gMmcCommitModeName[MMC_COMMITMODE_MAX] =
{
    "Non-Autocommit",
    "Autocommit"
};

static const SChar *gMmcStmtExecModeName[MMC_STMT_EXEC_MODE_MAX] =
{
    "Prepare-Execute",
    "Direct-Execute",
};

static const SChar *gMmcStmtStateName[MMC_STMT_STATE_MAX] =
{
    "Allocated",
    "Prepared",
    "Executed",
};



IDE_RC mmcTask::initialize()
{
    IDU_LIST_INIT_OBJ(&mAllocListNode, this);
    IDU_LIST_INIT_OBJ(&mThreadListNode, this);

    mTaskState               = MMC_TASK_STATE_WAITING;

    mConnectTime             = mmtSessionManager::getBaseTime();
    mLinkCheckTime           = 0;

    mIsShutdownTask          = ID_FALSE;
    mIsLoginTimeoutTask      = ID_FALSE;
    mLink                    = NULL;
    /*fix BUG-29717 When a task is terminated or migrated ,
      a busy degree of service thread which the task was assigned
      to should be decreased.*/
    mBusyExprienceCnt        = 0;
    
    mSession                 = NULL;
    mThread                  = NULL;

    // proj_2160 cm_type removal: set cmbBlock-ptr NULL
    cmiMakeCmBlockNull(&mProtocolContext);

    return IDE_SUCCESS;
}

IDE_RC mmcTask::finalize()
{
    IDE_RC sRet = IDE_SUCCESS;

    sRet = mmtSessionManager::freeSession(this);

    if (isShutdownTask() != ID_TRUE)
    {
        //cmiSession * sSession = getCmiSession();
        //(void) idsGPKIFinalize(&(sSession->mGPKICtx));

        // fix BUG-28267 [codesonar] Ignored Return Value
        if (getLink() != NULL)
        {
            (void)cmiFreeCmBlock(&(mProtocolContext)); // proj_2160 cm_type
            (void)cmiFreeLink(getLink());
        }
    }

    return sRet;
}

IDE_RC mmcTask::setLink(cmiLink *aLink)
{
    IDE_TEST(aLink == NULL);

    cmiSetLinkUserPtr(aLink, this);
    // proj_2160 cm_type removal
    IDE_TEST(cmiAllocCmBlock(&mProtocolContext,
                             CMP_MODULE_DB,
                             aLink,
                             this) != IDE_SUCCESS);
    mLink = aLink;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC mmcTask::authenticate(qciUserInfo *aUserInfo)
{
    /* PROJ-2446 */
    mmcSession     *sSession    = NULL;
    idvSQL         *sStatistics = NULL;

    idBool          sIsRemoteIP = ID_FALSE;
    UInt            sPasswordLen;

    /* PROJ-2474 SSL/TLS Support */
    if ( qci::checkDisableTCPOption( aUserInfo ) != IDE_SUCCESS )
    {
        ideClearError();
        IDE_RAISE( DISABLED_TCP_CONNECTION );
    }
    else
    {
        /* Nothing to do */
    }

    if (aUserInfo->mIsSysdba == ID_TRUE)
    {
        // bug-19279 remote sysdba enable + sys can kill session
        // REMOTE_SYSDBA_ENABLE 속성이 off(0) 이면
        // 원격 sysdba 접속을 막는다.
        // link 방식이 tcp이고 IP주소가 127.0.0.1이 아닌 경우
        // 원격 sysdba로 판단하고 막는다
        if (mmuProperty::getRemoteSysdbaEnable() == REMOTE_SYSDBA_ENABLE_OFF)
        {
            (void) cmiCheckRemoteAccess(getLink(), &sIsRemoteIP);
            IDE_TEST_RAISE(sIsRemoteIP == ID_TRUE, RemoteSysdbaEnableOff);
        }

        IDE_TEST_RAISE((cmiGetLinkFeature(getLink()) & CMI_LINK_FEATURE_SYSDBA) == 0,
                       SysdbaNotAcceptable);

        IDE_TEST_RAISE(aUserInfo->userID != QC_SYS_USER_ID, InvalidAccessMode);
    }

    if( aUserInfo->mCheckPassword == ID_TRUE )
    {
        if (*aUserInfo->loginPassword != 0)
        {
            sPasswordLen = idlOS::strlen( aUserInfo->loginPassword );
            IDE_TEST_RAISE( sPasswordLen > IDS_MAX_PASSWORD_LEN, TooLongPasswordLength );

            /* PROJ-2638 shard native linker */
            idlOS::memcpy( aUserInfo->loginOrgPassword,
                           aUserInfo->loginPassword,
                           sPasswordLen );
            aUserInfo->loginOrgPassword[sPasswordLen] = '\0';

            /* simple scramble */
            sdi::charXOR( aUserInfo->loginOrgPassword, sPasswordLen );

            idsPassword::crypt( aUserInfo->loginPassword,
                                aUserInfo->loginPassword,
                                idlOS::strlen(aUserInfo->loginPassword),
                                aUserInfo->userPassword );
        }

        /* set failed login count
         * FAILED_LOGIN_ATTEMPTS 값이 설정 여부와 상관 없이
         * login 실패한 count를 설정 count 설정 유무로
         * passwPolicyAuthenticate()에서 password invaild 구분 하여
         * error 처리 한다. */
        if ( idlOS::strcmp(aUserInfo->loginPassword,
                           aUserInfo->userPassword) != 0 )
        {
            aUserInfo->mAccLimitOpts.mUserFailedCnt =
                aUserInfo->mAccLimitOpts.mFailedCount + 1;
        }
        else
        {
            aUserInfo->mAccLimitOpts.mUserFailedCnt = 0;
        }
    }

    /* PROJ-2446 */
    sSession = getSession();

    if( sSession != NULL )
    {
        sStatistics = sSession->getStatSQL();
    }
    else
    {
        /* do nothing */    
    }
    
    /* password policy check */
    if( qci::passwPolicyCheck( sStatistics, aUserInfo ) != IDE_SUCCESS )
    {
        if( ideGetErrorCode() == qpERR_ABORT_QCI_INVALID_PASSWORD_ERROR )
        {
            ideClearError();
            IDE_RAISE(IncorrectPassword);
        }
        else
        {
            IDE_TEST( ID_TRUE );
        }
    }
    else
    {
        // Nothing To Do
    }

    // BUG-47817
    if ( SDU_SHARD_ENABLE == 1 )
    {
        if ( SDU_SHARD_ADMIN_MODE == 1 )
        {
            IDE_TEST_RAISE((aUserInfo->userID != QC_SYS_USER_ID) &&
                           (aUserInfo->userID != QC_SYSTEM_USER_ID),
                           shardAdminModeError);
        }
        else if (SDU_SHARD_ADMIN_MODE == 0)
        {
            IDE_TEST_RAISE(aUserInfo->userID == QC_SYSTEM_USER_ID, InvalidAccessMode);
        }
    }
    else
    {
        // nothing to do
    }
    
    if (mmuProperty::getAdminMode() == 1)
    {
        IDE_TEST_RAISE((aUserInfo->userID != QC_SYS_USER_ID) &&
                       (aUserInfo->userID != QC_SYSTEM_USER_ID),
                       AdminModeError);
        IDE_TEST_RAISE(aUserInfo->mIsSysdba != ID_TRUE, AdminModeError);
    }
    else if (mmuProperty::getAdminMode() == 0)
    {
        IDE_TEST_RAISE(aUserInfo->userID == QC_SYSTEM_USER_ID, InvalidAccessMode);
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION(RemoteSysdbaEnableOff);
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_REMOTE_SYSDBA_NOT_ALLOWED));
    }
    IDE_EXCEPTION(SysdbaNotAcceptable);
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_ADMIN_NOT_ACCEPTED));
    }
    IDE_EXCEPTION(IncorrectPassword);
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_INVALID_PASSWORD_ERROR));
    }
    IDE_EXCEPTION(AdminModeError);
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_ADMIN_MODE_ERROR));
    }
    IDE_EXCEPTION(InvalidAccessMode);
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_MMC_ACCESS_MODE));
    }
    /* BUG-33207 Limitation for the length of input password should be shortened */
    IDE_EXCEPTION(TooLongPasswordLength);
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_TOO_LONG_PASSWORD_ERROR));
    }
    IDE_EXCEPTION( DISABLED_TCP_CONNECTION );
    {
        IDE_SET( ideSetErrorCode( mmERR_ABORT_DISABLED_TCP_USER ) );
    }
    IDE_EXCEPTION(shardAdminModeError);
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_SHARD_ADMIN_MODE_ERROR));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mmcTask::log(ideLogEntry &aLog)
{
    SChar         sBuffer[IDE_BUFFER_SIZE];
    mmcSession   *sSession;
    mmcStatement *sStmt;
    iduListNode  *sIterator;
    
    sSession = getSession();

    IDE_TEST(sSession == NULL);

    sBuffer[0] = 0;

    IDE_TEST(logGeneralInfo(sBuffer, ID_SIZEOF(sBuffer)) != IDE_SUCCESS);

    if (sSession->getCommitMode() == MMC_COMMITMODE_NONAUTOCOMMIT)
    {
        logTransactionInfo(mmcTrans::getSmiTrans(sSession->getTransPtr()),
                           sBuffer,
                           ID_SIZEOF(sBuffer),
                           "        Transaction ID ");
    }

    idlVA::appendFormat(sBuffer, ID_SIZEOF(sBuffer), "\n");

    sSession->lockForStmtList();

    IDU_LIST_ITERATE(sSession->getStmtList(), sIterator)
    {
        sStmt = (mmcStatement *)sIterator->mObj;

        logStatementInfo(sStmt, sBuffer, ID_SIZEOF(sBuffer));
    }

    sSession->unlockForStmtList();

    aLog.append(sBuffer);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC mmcTask::logGeneralInfo(SChar *aBuffer, UInt aBufferLen)
{
    SChar           sConnection[IDL_IP_ADDR_MAX_LEN];
    mmcSession     *sSession = getSession();
    mmcSessionInfo *sInfo    = sSession->getInfo();

    IDE_TEST(getLink() == NULL);

    IDE_TEST(cmiGetLinkInfo(getLink(),
                            sConnection,
                            ID_SIZEOF(sConnection),
                            CMI_LINK_INFO_ALL) != IDE_SUCCESS);

    /* BUG-39098 Thread ID 출력포맷을 ULong으로 변경  */
    idlVA::appendFormat(aBuffer,
                        aBufferLen,
                        "Session [%" ID_UINT32_FMT "]\n"
                        "        Connection      : %s\n"
                        "        Client Package  : %s\n"
                        "        Client Protocol : %s\n"
                        "        Client PID      : %" ID_UINT64_FMT "\n"
                        "        Client Type     : %s\n"
                        "        Client AppInfo  : %s\n"
                        "        Thread ID       : %" ID_UINT64_FMT "\n"
                        "        Task State      : %s\n"
                        "        Session State   : %s (%s)\n"
                        "        Commit Mode     : %s\n"
                        "        Transaction     : %s\n"
                        "        Isolation Level : %" ID_UINT32_FMT "\n"
                        "        Open Stmt Count : %" ID_UINT32_FMT "\n"
                        "        Current Stmt ID : %" ID_UINT32_FMT "\n",
                        sSession->getSessionID(),
                        sConnection,
                        sInfo->mClientPackageVersion,
                        sInfo->mClientProtocolVersion,
                        sInfo->mClientPID,
                        sInfo->mClientType,
                        sInfo->mClientAppInfo,
                        getThread() ? idlOS::getThreadID(getThread()->getTid()) : (ULong)-1,
                        gMmcTaskStateName[getTaskState()],
                        gMmcSessionStateName[sSession->getSessionState()],
                        IDU_SESSION_CHK_CLOSED(*sSession->getEventFlag()) ? "Terminating..." : "Opened",
                        gMmcCommitModeName[sSession->getCommitMode()],
                        sSession->isActivated() ? "Activated" : "Inactivated",
                        sInfo->mIsolationLevel,
                        sInfo->mOpenStmtCount,
                        sInfo->mCurrStmtID);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC mmcTask::logTransactionInfo(smiTrans *aTrans, SChar *aBuffer, UInt aBufferLen, const SChar *aTitle)
{
    IDE_TEST(aBuffer == NULL || aTitle == NULL);

    if ((aTrans != NULL) && (aTrans->getTransID() != 0))
    {
        idlVA::appendFormat(aBuffer,
                            aBufferLen,
                            "%s : %" ID_UINT32_FMT "\n",
                            aTitle,
                            aTrans->getTransID());
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mmcTask::logStatementInfo(mmcStatement *aStmt, SChar *aBuffer, UInt aBufferLen)
{
    mmcTransObj *sTrans;
    smSCN     sSCN;
    ULong     sOutSCN;
    UInt      sQueryTimeGap;
    UInt      sFetchTimeGap;
    UInt      sUTransTimeGap;

    SM_INIT_SCN(&sSCN);

    sTrans = getSession()->getTransPtr(aStmt);

    if (aStmt->isStmtBegin() == ID_TRUE)
    {
        sSCN = aStmt->getSmiStmt()->getSCN();
    }

#ifdef COMPILE_64BIT
    sOutSCN = sSCN;
#else
    sOutSCN = ((ULong)((((ULong)sSCN.mHigh) << 32) | (ULong)sSCN.mLow));
#endif

    sQueryTimeGap  = aStmt->getQueryStartTime();
    sFetchTimeGap  = aStmt->getFetchStartTime();
    sUTransTimeGap = (sTrans != NULL) ? mmcTrans::getFirstUpdateTime(sTrans) : 0;

    sQueryTimeGap  = (sQueryTimeGap > 0)  ? (mmtSessionManager::getBaseTime() - sQueryTimeGap)  : 0;
    sFetchTimeGap  = (sFetchTimeGap > 0)  ? (mmtSessionManager::getBaseTime() - sFetchTimeGap)  : 0;
    sUTransTimeGap = (sUTransTimeGap > 0) ? (mmtSessionManager::getBaseTime() - sUTransTimeGap) : 0;

    idlVA::appendFormat(aBuffer,
                        aBufferLen,
                        "        Statement [%" ID_UINT32_FMT "]\n"
                        "                Mode  : %s%s\n"
                        "                Row   : %"ID_UINT32_FMT"\n"
                        "                State : %s\n"
                        "                SCN   : %" ID_UINT64_FMT "\n"
                        "                Time  : U=>%" ID_UINT32_FMT ", Q=>%" ID_UINT32_FMT ", F=>%" ID_UINT32_FMT "\n",
                        aStmt->getStmtID(),
                        gMmcStmtExecModeName[aStmt->getStmtExecMode()],
                        aStmt->isArray() ? " (Array)" : "",
                        aStmt->getRowNumber(),
                        gMmcStmtStateName[aStmt->getStmtState()],
                        sOutSCN,
                        sUTransTimeGap,
                        sQueryTimeGap,
                        sFetchTimeGap);

    if ( ( getSession()->getCommitMode() == MMC_COMMITMODE_AUTOCOMMIT ) &&
         ( aStmt->isRootStmt() == ID_TRUE ) )
    {
        sTrans = aStmt->getTransPtr();
        logTransactionInfo(mmcTrans::getSmiTrans(sTrans), aBuffer, aBufferLen, "                Tx ID");
    }

    aStmt->lockQuery();

    if (aStmt->getQueryString() != NULL)
    {
        idlVA::appendFormat(aBuffer,
                            aBufferLen,
                            "                SQL   : \"%s\"\n",
                            aStmt->getQueryString());
    }

    aStmt->unlockQuery();

    return IDE_SUCCESS;
}
