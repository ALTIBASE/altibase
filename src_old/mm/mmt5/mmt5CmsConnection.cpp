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

#include <idsCrypt.h>
#include <idv.h>
#include <mtl.h>
#include <qcm.h>
#include <qcmUser.h>
#include <mmm.h>
#include <mmErrorCode.h>
#include <mmcSession.h>
#include <mmcTask.h>
#include <mmtSessionManager.h>
#include <mmtServiceThread.h>
#include <smi.h>
#include <cmi.h>
#include <idl.h>
#include <mmtAdminManager.h>
#include <mmtAuditManager.h>


static IDE_RC answerConnectResult(cmiProtocolContext *aProtocolContext)
{
    cmiProtocol            sProtocol;
    cmpArgDBConnectResultA5 *sArg;

    CMI_PROTOCOL_INITIALIZE(sProtocol, sArg, DB, ConnectResult);

    ACP_UNUSED(sArg);

    IDE_TEST(cmiWriteProtocol(aProtocolContext, &sProtocol) != IDE_SUCCESS);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

static IDE_RC answerDisconnectResult(cmiProtocolContext *aProtocolContext)
{
    cmiProtocol               sProtocol;
    cmpArgDBDisconnectResultA5 *sArg;

    CMI_PROTOCOL_INITIALIZE(sProtocol, sArg, DB, DisconnectResult);

    ACP_UNUSED(sArg);

    IDE_TEST(cmiWriteProtocol(aProtocolContext, &sProtocol) != IDE_SUCCESS);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

static IDE_RC answerPropertyGetResult(cmiProtocolContext *aProtocolContext,
                                      mmcSession         *aSession,
                                      UShort              aPropertyID,
                                      idBool             *aIsUnsupportedProperty)
{
    cmiProtocol                sProtocol;
    cmpArgDBPropertyGetResultA5 *sArg;
    mmcSessionInfo            *sInfo = aSession->getInfo();
    SChar                     *sDBCharSet = NULL;
    SChar                     *sNationalCharSet = NULL;

    CMI_PROTOCOL_INITIALIZE(sProtocol, sArg, DB, PropertyGetResult);

    sArg->mPropertyID = aPropertyID;

    switch (sArg->mPropertyID)
    {
        case CMP_DB_PROPERTY_CLIENT_PACKAGE_VERSION:
        case CMP_DB_PROPERTY_CLIENT_PROTOCOL_VERSION:
        case CMP_DB_PROPERTY_CLIENT_PID:
        case CMP_DB_PROPERTY_CLIENT_TYPE:
            IDE_TEST(cmtAnySetNull(&sArg->mValue) != IDE_SUCCESS);
            break;

        case CMP_DB_PROPERTY_APP_INFO:
            IDE_TEST(cmtAnyWriteVariable(&sArg->mValue,
                                         (UChar *)sInfo->mClientAppInfo,
                                         idlOS::strlen(sInfo->mClientAppInfo)) != IDE_SUCCESS);
            break;

        case CMP_DB_PROPERTY_NLS:
            IDE_TEST(cmtAnyWriteVariable(&sArg->mValue,
                                         (UChar *)sInfo->mNlsUse,
                                         idlOS::strlen(sInfo->mNlsUse)) != IDE_SUCCESS);
            break;

        case CMP_DB_PROPERTY_AUTOCOMMIT:
            IDE_TEST(cmtAnyWriteUChar(&sArg->mValue,
                                      (sInfo->mCommitMode == MMC_COMMITMODE_AUTOCOMMIT) ? 1 : 0)
                     != IDE_SUCCESS);
            break;

        case CMP_DB_PROPERTY_EXPLAIN_PLAN:
            IDE_TEST(cmtAnyWriteUChar(&sArg->mValue, sInfo->mExplainPlan) != IDE_SUCCESS);
            break;

        case CMP_DB_PROPERTY_OPTIMIZER_MODE:
            IDE_TEST(cmtAnyWriteUInt(&sArg->mValue, sInfo->mOptimizerMode) != IDE_SUCCESS);
            break;

        case CMP_DB_PROPERTY_HEADER_DISPLAY_MODE:
            IDE_TEST(cmtAnyWriteUInt(&sArg->mValue, sInfo->mHeaderDisplayMode) != IDE_SUCCESS);
            break;

        case CMP_DB_PROPERTY_STACK_SIZE:
            IDE_TEST(cmtAnyWriteUInt(&sArg->mValue, sInfo->mStackSize) != IDE_SUCCESS);
            break;

        case CMP_DB_PROPERTY_IDLE_TIMEOUT:
            IDE_TEST(cmtAnyWriteUInt(&sArg->mValue, sInfo->mIdleTimeout) != IDE_SUCCESS);
            break;

        case CMP_DB_PROPERTY_QUERY_TIMEOUT:
            IDE_TEST(cmtAnyWriteUInt(&sArg->mValue, sInfo->mQueryTimeout) != IDE_SUCCESS);
            break;

        /* BUG-35123 */
        /* BUG-32885 Timeout for DDL must be distinct to query_timeout or utrans_timeout */
        case CMP_DB_PROPERTY_DDL_TIMEOUT:
            IDE_TEST(cmtAnyWriteUInt(&sArg->mValue, sInfo->mDdlTimeout) != IDE_SUCCESS);
            break;

        case CMP_DB_PROPERTY_FETCH_TIMEOUT:
            IDE_TEST(cmtAnyWriteUInt(&sArg->mValue, sInfo->mFetchTimeout) != IDE_SUCCESS);
            break;

        case CMP_DB_PROPERTY_UTRANS_TIMEOUT:
            IDE_TEST(cmtAnyWriteUInt(&sArg->mValue, sInfo->mUTransTimeout) != IDE_SUCCESS);
            break;

        case CMP_DB_PROPERTY_DATE_FORMAT:
            IDE_TEST(cmtAnyWriteVariable(&sArg->mValue,
                                         (UChar *)sInfo->mDateFormat,
                                         idlOS::strlen(sInfo->mDateFormat)) != IDE_SUCCESS);
            break;

        case CMP_DB_PROPERTY_NORMALFORM_MAXIMUM:
            IDE_TEST(cmtAnyWriteUInt(&sArg->mValue, sInfo->mNormalFormMaximum) != IDE_SUCCESS);
            break;

        // fix BUG-18971
        case CMP_DB_PROPERTY_SERVER_PACKAGE_VERSION:
            IDE_TEST(cmtAnyWriteVariable(&sArg->mValue,
                                         (UChar *)iduVersionString,
                                         idlOS::strlen(iduVersionString)) != IDE_SUCCESS);
            break;

        // PROJ-1579 NCHAR
        case CMP_DB_PROPERTY_NLS_NCHAR_LITERAL_REPLACE:
            IDE_TEST(cmtAnyWriteUInt(&sArg->mValue, sInfo->mNlsNcharLiteralReplace) != IDE_SUCCESS);
            break;

        // PROJ-1579 NCHAR
        case CMP_DB_PROPERTY_NLS_CHARACTERSET:
            sDBCharSet = smiGetDBCharSet();

            IDE_TEST(cmtAnyWriteVariable(&sArg->mValue,
                                         (UChar *)sDBCharSet,
                                         idlOS::strlen(sDBCharSet)) != IDE_SUCCESS);
            break;

        // PROJ-1579 NCHAR
        case CMP_DB_PROPERTY_NLS_NCHAR_CHARACTERSET:
            sNationalCharSet = smiGetNationalCharSet();

            IDE_TEST(cmtAnyWriteVariable(&sArg->mValue,
                                         (UChar *)sNationalCharSet,
                                         idlOS::strlen(sNationalCharSet)) != IDE_SUCCESS);
            break;

        // PROJ-1579 NCHAR
        case CMP_DB_PROPERTY_ENDIAN:
            IDE_TEST(cmtAnyWriteUChar(&sArg->mValue,
                                      (iduBigEndian == ID_TRUE) ? 1 : 0)
                     != IDE_SUCCESS);
            break;

        /* BUG-36759, PROJ-2257 */
        case CMP_DB_PROPERTY_REMOVE_REDUNDANT_TRANSMISSION:
            IDE_TEST( cmtAnyWriteUInt( &sArg->mValue,
                                       sInfo->mRemoveRedundantTransmission ) != IDE_SUCCESS );
            break;

        default:
            /* BUG-36256 Improve property's communication */
            IDE_RAISE(UnsupportedProperty);
            break;
    }

    IDE_TEST(cmiWriteProtocol(aProtocolContext, &sProtocol) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION(UnsupportedProperty)
    {
        *aIsUnsupportedProperty = ID_TRUE;
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC answerPropertySetResult(cmiProtocolContext *aProtocolContext)
{
    cmiProtocol                sProtocol;
    cmpArgDBPropertySetResultA5 *sArg;

    CMI_PROTOCOL_INITIALIZE(sProtocol, sArg, DB, PropertySetResult);

    ACP_UNUSED(sArg);

    IDE_TEST(cmiWriteProtocol(aProtocolContext, &sProtocol) != IDE_SUCCESS);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}


IDE_RC mmtServiceThread::connectProtocolA5(cmiProtocolContext *aProtocolContext,
                                         cmiProtocol        *aProtocol,
                                         void               *aSessionOwner,
                                         void               *aUserContext)
{
    cmpArgDBConnectA5  *sArg    = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, Connect);
    mmcTask          *sTask   = (mmcTask *)aSessionOwner;
    mmtServiceThread *sThread = (mmtServiceThread *)aUserContext;
    mmcSession       *sSession;
    qciUserInfo       sUserInfo;
    SChar             sUserName[QC_MAX_OBJECT_NAME_LEN + 1];
    SChar             sPassword[QC_MAX_NAME_LEN + 1];
    UInt              sUserNameLen;
    UInt              sPasswordLen;
    IDE_RC            sRet;
    cmiLink         * sLink = NULL;

    /* TASK-5894 Permit sysdba via IPC */
    idBool            sHasSysdbaViaIPC = ID_FALSE;
    idBool            sIsSysdba        = ID_FALSE;
    SChar             sErrMsg[MAX_ERROR_MSG_LEN] = {'\0', };

    /* PROJ-2446 */
    idvSQL           *sStatistics = NULL;

    /* BUG-41986 */
    idvAuditTrail     sAuditTrail;
    UInt              sResultCode = 0;

    IDE_CLEAR();

    IDE_TEST_RAISE(sTask == NULL, NoTask);

    idlOS::memset(&sUserInfo, 0, ID_SIZEOF(sUserInfo));

    /*
     * 프로토콜로부터 UserName, Password 획득
     */

    sUserNameLen = cmtVariableGetSize(&sArg->mUserName);
    sPasswordLen = cmtVariableGetSize(&sArg->mPassword);

    IDE_TEST_RAISE(sUserNameLen > QC_MAX_OBJECT_NAME_LEN, UserNameTooLong);
    IDE_TEST_RAISE(sPasswordLen > QC_MAX_NAME_LEN, PasswordTooLong);

    IDE_TEST(cmtVariableGetData(&sArg->mUserName,
                                (UChar *)sUserName,
                                sUserNameLen) != IDE_SUCCESS);
    IDE_TEST(cmtVariableGetData(&sArg->mPassword,
                                (UChar *)sPassword,
                                sPasswordLen) != IDE_SUCCESS);

    /* TASK-5894 Permit sysdba via IPC */
    sIsSysdba = (sArg->mMode == CMP_DB_CONNECT_MODE_SYSDBA) ? ID_TRUE : ID_FALSE;
    /* BUG-43654 */
    sHasSysdbaViaIPC = mmtAdminManager::isConnectedViaIPC();

    IDE_TEST_RAISE(cmiPermitConnection(sTask->getLink(),
                                       sHasSysdbaViaIPC,
                                       sIsSysdba)
                   != IDE_SUCCESS, ConnectionNotPermitted);

    // To Fix BUG-17430
    // 무조건 대문자로 변경하면 안됨.
    // idlOS::strUpper(sUserInfo.loginID, sUserNameLen);
    sUserName[sUserNameLen] = '\0';
    mtl::makeNameInSQL( sUserInfo.loginID, sUserName, sUserNameLen );

    // To fix BUG-21137
    // password에도 double quotation이 올 수 있다.
    // 이를 makeNameInSQL함수로 제거한다.
    sPassword[sPasswordLen] = '\0';
    mtl::makePasswordInSQL( sUserInfo.loginPassword, sPassword, sPasswordLen );

    // PROJ-2002 Column Security
    // login IP(session login IP)는 모든 레코드의 컬럼마다
    // 호출하여 호출 횟수가 많아 IP를 별도로 저장한다.
    if( cmiGetLinkInfo( sTask->getLink(),
                        sUserInfo.loginIP,
                        QCI_MAX_IP_LEN,
                        CMI_LINK_INFO_REMOTE_IP_ADDRESS )
        == IDE_SUCCESS )
    {
        sUserInfo.loginIP[QCI_MAX_IP_LEN] = '\0';
    }
    else
    {
        idlOS::snprintf( sUserInfo.loginIP,
                         QCI_MAX_IP_LEN,
                         "127.0.0.1" );
        sUserInfo.loginIP[QCI_MAX_IP_LEN] = '\0';
    }

    sUserInfo.mUsrDN         = NULL;
    sUserInfo.mSvrDN         = NULL;
    sUserInfo.mCheckPassword = ID_TRUE;

    sUserInfo.mIsSysdba = sIsSysdba;

    if (mmm::getCurrentPhase() == MMM_STARTUP_SERVICE)
    {
        /* >> PROJ-2446 */
        sSession = sTask->getSession();

        if (sSession != NULL)
        {
            sStatistics = sSession->getStatSQL();
        }
        else
        {
            /* do nothing */
        }
            
        IDE_TEST(getUserInfoFromDB(sStatistics, &sUserInfo) != IDE_SUCCESS);
        /* << PROJ-2446 */
    }
    else
    {
        IDE_TEST_RAISE(sUserInfo.mIsSysdba != ID_TRUE, OnlyAdminAcceptable);

        IDE_TEST(getUserInfoFromFile(&sUserInfo) != IDE_SUCCESS);
    }

    /* PROJ-2474 SSL/TLS */
    IDE_TEST(cmiGetLinkForProtocolContext(aProtocolContext, &sLink));
    sUserInfo.mConnectType = (qciConnectType)sLink->mImpl;

    IDE_TEST(sTask->authenticate(&sUserInfo) != IDE_SUCCESS);

    /*
     * Session 생성
     */

    IDE_TEST(mmtSessionManager::allocSession(sTask, sUserInfo.mIsSysdba) != IDE_SUCCESS);

    /*
     * Session 상태 확인
     */

    sSession = sTask->getSession();
    

    IDE_TEST_RAISE(sSession->getSessionState() >= MMC_SESSION_STATE_AUTH, AlreadyConnectedError);

    /*
     * Session에 로그인정보 저장
     */

    sTask->getSession()->setUserInfo(&sUserInfo);

    sSession->setSessionState(MMC_SESSION_STATE_AUTH);

    IDV_SESS_ADD(sTask->getSession()->getStatistics(), IDV_STAT_INDEX_LOGON_CUMUL, 1);
    IDV_SESS_ADD(sTask->getSession()->getStatistics(), IDV_STAT_INDEX_LOGON_CURR, 1);

    sRet = answerConnectResult(aProtocolContext);

    /* BUG-41986 */
    IDE_TEST_CONT( mmtAuditManager::isAuditStarted() != ID_TRUE, AUDIT_NOT_STARTED );

    sResultCode = (sRet == IDE_SUCCESS) ? 0 : E_ERROR_CODE(ideGetErrorCode());

    mmtAuditManager::initAuditConnInfo( sSession, 
                                        &sAuditTrail, 
                                        &sUserInfo, 
                                        sResultCode, 
                                        QCI_AUDIT_OPER_CONNECT );

    mmtAuditManager::auditConnectInfo( &sAuditTrail );

    IDE_EXCEPTION_CONT( AUDIT_NOT_STARTED );

    return sRet;

    IDE_EXCEPTION(NoTask);
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_SESSION_NOT_SPECIFIED));
    }
    IDE_EXCEPTION(UserNameTooLong);
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_TOO_LONG_IDENTIFIER_NAME));
    }
    IDE_EXCEPTION(PasswordTooLong);
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_TOO_LONG_IDENTIFIER_NAME));
    }
    IDE_EXCEPTION(OnlyAdminAcceptable);
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_ADMIN_MODE_ONLY));
    }
    IDE_EXCEPTION(AlreadyConnectedError);
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_ALREADY_CONNECT_ERROR));
    }
    IDE_EXCEPTION( ConnectionNotPermitted )
    {
        idlOS::snprintf( sErrMsg, ID_SIZEOF(sErrMsg), ideGetErrorMsg() );
        IDE_SET( ideSetErrorCode(mmERR_ABORT_FAIL_TO_ESTABLISH_CONNECTION, sErrMsg) );
    }
    IDE_EXCEPTION_END;
    {
        /* BUG-41986 */
        IDE_TEST_CONT( mmtAuditManager::isAuditStarted() != ID_TRUE, AUDIT_NOT_STARTED_FOR_ERR_RESULT );

        mmtAuditManager::initAuditConnInfo( sSession, 
                                            &sAuditTrail, 
                                            &sUserInfo, 
                                            E_ERROR_CODE(ideGetErrorCode()),
                                            QCI_AUDIT_OPER_DISCONNECT );

        mmtAuditManager::auditConnectInfo( &sAuditTrail );

        IDE_EXCEPTION_CONT( AUDIT_NOT_STARTED_FOR_ERR_RESULT );

        sRet = sThread->answerErrorResultA5(aProtocolContext,
                                          CMI_PROTOCOL_OPERATION(DB, Connect),
                                          0);

        if (sRet == IDE_SUCCESS)
        {
            sThread->mErrorFlag = ID_TRUE;
        }
    }

    return sRet;
}

IDE_RC mmtServiceThread::disconnectProtocolA5(cmiProtocolContext *aProtocolContext,
                                            cmiProtocol        * /*aProtocol*/,
                                            void               *aSessionOwner,
                                            void               *aUserContext)
{
    mmcTask          *sTask = (mmcTask *)aSessionOwner;
    mmtServiceThread *sThread = (mmtServiceThread *)aUserContext;
    mmcSession       *sSession;

    idvAuditTrail     sAuditTrail; /* BUG-41986 */

    IDE_CLEAR();

    IDE_TEST(findSession(sTask, &sSession, sThread) != IDE_SUCCESS);

    IDE_TEST(checkSessionState(sSession, MMC_SESSION_STATE_AUTH) != IDE_SUCCESS);

    sSession->setSessionState(MMC_SESSION_STATE_END);

    aProtocolContext->mIsDisconnect = ID_TRUE;

    /* BUG-41986 
     * Auditing for disconnection must be conducted 
     * before releasing the resources related to the session to get the session information
     */
    IDE_TEST_CONT( mmtAuditManager::isAuditStarted() != ID_TRUE, AUDIT_NOT_STARTED );

    mmtAuditManager::initAuditConnInfo( sSession, 
                                        &sAuditTrail, 
                                        sSession->getUserInfo(), 
                                        0, /* success */
                                        QCI_AUDIT_OPER_DISCONNECT );

    mmtAuditManager::auditConnectInfo( &sAuditTrail );

    IDE_EXCEPTION_CONT( AUDIT_NOT_STARTED );

    /* BUG-38585 IDE_ASSERT remove */
    IDU_FIT_POINT("mmtServiceThread::disconnectProtocolA5::EndSession");
    IDE_TEST(sSession->endSession() != IDE_SUCCESS);

    IDE_TEST(mmtSessionManager::freeSession(sTask) != IDE_SUCCESS);

    return answerDisconnectResult(aProtocolContext);

    IDE_EXCEPTION_END;

    /* BUG-41986 */
    IDE_TEST_CONT( mmtAuditManager::isAuditStarted() != ID_TRUE, AUDIT_NOT_STARTED_FOR_ERR_RESULT );

    mmtAuditManager::initAuditConnInfo( sSession, 
                                        &sAuditTrail, 
                                        sSession->getUserInfo(), 
                                        E_ERROR_CODE(ideGetErrorCode()),
                                        QCI_AUDIT_OPER_DISCONNECT );

    mmtAuditManager::auditConnectInfo( &sAuditTrail );

    IDE_EXCEPTION_CONT( AUDIT_NOT_STARTED_FOR_ERR_RESULT );

    return sThread->answerErrorResultA5(aProtocolContext,
                                      CMI_PROTOCOL_OPERATION(DB, Disconnect),
                                      0);
}

IDE_RC mmtServiceThread::propertyGetProtocolA5(cmiProtocolContext *aProtocolContext,
                                             cmiProtocol        *aProtocol,
                                             void               *aSessionOwner,
                                             void               *aUserContext)
{
    cmpArgDBPropertyGetA5 *sArg  = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, PropertyGet);
    mmcTask             *sTask = (mmcTask *)aSessionOwner;
    mmtServiceThread    *sThread = (mmtServiceThread *)aUserContext;
    mmcSession          *sSession;
    idBool               sIsUnsupportedProperty = ID_FALSE;
    IDE_RC               sRC = IDE_FAILURE;

    IDE_CLEAR();

    IDE_TEST(findSession(sTask, &sSession, sThread) != IDE_SUCCESS);

    IDE_TEST(checkSessionState(sSession, MMC_SESSION_STATE_AUTH) != IDE_SUCCESS);

    IDE_TEST(answerPropertyGetResult(aProtocolContext,
                                     sSession,
                                     sArg->mPropertyID,
                                     &sIsUnsupportedProperty) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if (sIsUnsupportedProperty == ID_TRUE)
    {
        /*
         * BUG-36256 Improve property's communication
         *
         * ulnCallbackDBPropertySetResult 함수를 이용할 수 없기에 Get()도
         * 통일성을 위해 answerErrorResult를 이용해 에러를 발생시키지 않고
         * Client에게 응답을 준다.
         */
        ideLog::log(IDE_MM_0,
                    MM_TRC_GET_UNSUPPORTED_PROPERTY,
                    sArg->mPropertyID);

        IDE_SET(ideSetErrorCode(mmERR_IGNORE_UNSUPPORTED_PROPERTY, sArg->mPropertyID));

        sRC = sThread->answerErrorResultA5(aProtocolContext,
                                           CMI_PROTOCOL_OPERATION(DB, PropertyGet),
                                           sArg->mPropertyID);
    }
    else
    {
        sRC = sThread->answerErrorResultA5(aProtocolContext,
                                           CMI_PROTOCOL_OPERATION(DB, PropertyGet),
                                           0);
    }

    return sRC;
}

IDE_RC mmtServiceThread::propertySetProtocolA5(cmiProtocolContext *aProtocolContext,
                                             cmiProtocol        *aProtocol,
                                             void               *aSessionOwner,
                                             void               *aUserContext)
{
    cmpArgDBPropertySetA5 *sArg    = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, PropertySet);
    mmcTask             *sTask   = (mmcTask *)aSessionOwner;
    mmtServiceThread    *sThread = (mmtServiceThread *)aUserContext;
    mmcSession          *sSession;
    mmcSessionInfo      *sInfo;
    cmtVariable         *sVariable;
    UInt                 sLen;
    UChar                sBool;
    ULong                sValue;

    IDE_CLEAR();

    IDE_TEST(findSession(sTask, &sSession, sThread) != IDE_SUCCESS);

    IDE_TEST(checkSessionState(sSession, MMC_SESSION_STATE_AUTH) != IDE_SUCCESS);

    sInfo = sSession->getInfo();

    switch (sArg->mPropertyID)
    {
        case CMP_DB_PROPERTY_CLIENT_PACKAGE_VERSION:
            IDE_TEST_RAISE(sSession->getSessionState() != MMC_SESSION_STATE_AUTH,
                           InvalidSessionState);

            IDE_TEST(cmtAnyReadVariable(&sArg->mValue, &sVariable) != IDE_SUCCESS);

            sLen = IDL_MIN(cmtVariableGetSize(sVariable),
                           ID_SIZEOF(sInfo->mClientPackageVersion) - 1);

            IDE_TEST(cmtVariableGetData(sVariable,
                                        (UChar *)sInfo->mClientPackageVersion,
                                        sLen) != IDE_SUCCESS);

            sInfo->mClientPackageVersion[sLen] = 0;

            sSession->changeSessionStateService();

            break;

        case CMP_DB_PROPERTY_CLIENT_PROTOCOL_VERSION:
            IDE_TEST_RAISE(sSession->getSessionState() != MMC_SESSION_STATE_AUTH,
                           InvalidSessionState);

            IDE_TEST(cmtAnyReadULong(&sArg->mValue, &sValue) != IDE_SUCCESS);

            idlOS::snprintf(sInfo->mClientProtocolVersion,
                            ID_SIZEOF(sInfo->mClientProtocolVersion),
                            "%" ID_UINT32_FMT ".%" ID_UINT32_FMT ".%" ID_UINT32_FMT,
                            CM_GET_MAJOR_VERSION(sValue),
                            CM_GET_MINOR_VERSION(sValue),
                            CM_GET_PATCH_VERSION(sValue));

            sSession->changeSessionStateService();

            break;

        case CMP_DB_PROPERTY_CLIENT_PID:
            IDE_TEST_RAISE(sSession->getSessionState() != MMC_SESSION_STATE_AUTH,
                           InvalidSessionState);

            IDE_TEST(cmtAnyReadULong(&sArg->mValue, &sInfo->mClientPID) != IDE_SUCCESS);

            sSession->changeSessionStateService();

            break;

        case CMP_DB_PROPERTY_CLIENT_TYPE:
            IDE_TEST_RAISE(sSession->getSessionState() != MMC_SESSION_STATE_AUTH,
                           InvalidSessionState);

            IDE_TEST(cmtAnyReadVariable(&sArg->mValue, &sVariable) != IDE_SUCCESS);

            sLen = IDL_MIN(cmtVariableGetSize(sVariable), ID_SIZEOF(sInfo->mClientType) - 1);

            IDE_TEST(cmtVariableGetData(sVariable, (UChar *)sInfo->mClientType, sLen) != IDE_SUCCESS);

            sInfo->mClientType[sLen] = 0;

            sSession->changeSessionStateService();

            break;

        case CMP_DB_PROPERTY_APP_INFO:
            IDE_TEST(cmtAnyReadVariable(&sArg->mValue, &sVariable) != IDE_SUCCESS);

            sLen = IDL_MIN(cmtVariableGetSize(sVariable), ID_SIZEOF(sInfo->mClientAppInfo) - 1);

            IDE_TEST(cmtVariableGetData(sVariable,
                                        (UChar *)sInfo->mClientAppInfo,
                                        sLen) != IDE_SUCCESS);

            sInfo->mClientAppInfo[sLen] = 0;

            /* PROJ-2626 Snapshot Export
             * iloader 인지 아닌지를 구분해야 될 경우 매번 string compare를 하기
             * 보다 미리 값을 정해 놓는다.
             */
            if ( ( sLen == 7 ) &&
                 ( idlOS::strncmp( sInfo->mClientAppInfo, "iloader", sLen ) == 0 ) )
            {
                sInfo->mClientAppInfoType = MMC_CLIENT_APP_INFO_TYPE_ILOADER;
            }
            else
            {
                /* Nothing to do */
            }
            break;

        case CMP_DB_PROPERTY_NLS:
            IDE_TEST_RAISE(sSession->getSessionState() != MMC_SESSION_STATE_AUTH,
                           InvalidSessionState);

            IDE_TEST(cmtAnyReadVariable(&sArg->mValue, &sVariable) != IDE_SUCCESS);

            sLen = IDL_MIN(cmtVariableGetSize(sVariable), ID_SIZEOF(sInfo->mNlsUse) - 1);

            IDE_TEST(cmtVariableGetData(sVariable, (UChar *)sInfo->mNlsUse, sLen) != IDE_SUCCESS);

            sInfo->mNlsUse[sLen] = 0;

            IDE_TEST(sSession->findLanguage() != IDE_SUCCESS);

            sSession->changeSessionStateService();

            break;

        case CMP_DB_PROPERTY_AUTOCOMMIT:
            IDE_TEST(cmtAnyReadUChar(&sArg->mValue, &sBool) != IDE_SUCCESS);

            /* BUG-21230 */
            IDE_TEST_RAISE(sSession->getXaAssocState() != MMD_XA_ASSOC_STATE_NOTASSOCIATED,
                           DCLNotAllowedError);
                  
            IDE_TEST(sSession->setCommitMode(sBool ?
                                             MMC_COMMITMODE_AUTOCOMMIT :
                                             MMC_COMMITMODE_NONAUTOCOMMIT) != IDE_SUCCESS);

            break;

        case CMP_DB_PROPERTY_EXPLAIN_PLAN:
            IDE_TEST(cmtAnyReadUChar(&sArg->mValue, &sBool) != IDE_SUCCESS);

            sSession->setExplainPlan(sBool);
            break;

        case CMP_DB_PROPERTY_OPTIMIZER_MODE:
            IDE_TEST(cmtAnyReadUInt(&sArg->mValue, &sInfo->mOptimizerMode) != IDE_SUCCESS);
            break;

        case CMP_DB_PROPERTY_HEADER_DISPLAY_MODE:
            IDE_TEST(cmtAnyReadUInt(&sArg->mValue, &sInfo->mHeaderDisplayMode) != IDE_SUCCESS);
            break;

        case CMP_DB_PROPERTY_STACK_SIZE:
            IDE_TEST(cmtAnyReadUInt(&sArg->mValue, &sInfo->mStackSize) != IDE_SUCCESS);
            break;

        case CMP_DB_PROPERTY_IDLE_TIMEOUT:
            IDE_TEST(cmtAnyReadUInt(&sArg->mValue, &sInfo->mIdleTimeout) != IDE_SUCCESS);
            break;

        case CMP_DB_PROPERTY_QUERY_TIMEOUT:
            IDE_TEST(cmtAnyReadUInt(&sArg->mValue, &sInfo->mQueryTimeout) != IDE_SUCCESS);
            break;

        /* BUG-35123 */
        /* BUG-32885 Timeout for DDL must be distinct to query_timeout or utrans_timeout */
        case CMP_DB_PROPERTY_DDL_TIMEOUT:
            IDE_TEST(cmtAnyReadUInt(&sArg->mValue, &sInfo->mDdlTimeout) != IDE_SUCCESS);
            break;

        case CMP_DB_PROPERTY_FETCH_TIMEOUT:
            IDE_TEST(cmtAnyReadUInt(&sArg->mValue, &sInfo->mFetchTimeout) != IDE_SUCCESS);
            break;

        case CMP_DB_PROPERTY_UTRANS_TIMEOUT:
            IDE_TEST(cmtAnyReadUInt(&sArg->mValue, &sInfo->mUTransTimeout) != IDE_SUCCESS);
            break;

        case CMP_DB_PROPERTY_DATE_FORMAT:
            IDE_TEST(cmtAnyReadVariable(&sArg->mValue, &sVariable) != IDE_SUCCESS);

            sLen = cmtVariableGetSize(sVariable);

            IDE_TEST_RAISE(sLen >= MMC_DATEFORMAT_MAX_LEN, DateFormatTooLong);

            IDE_TEST(cmtVariableGetData(sVariable, (UChar *)sInfo->mDateFormat, sLen)
                     != IDE_SUCCESS);

            sInfo->mDateFormat[sLen] = 0;

            break;

        // PROJ-1579 NCHAR
        case CMP_DB_PROPERTY_NLS_NCHAR_LITERAL_REPLACE:
            IDE_TEST(cmtAnyReadUInt(&sArg->mValue, &sInfo->mNlsNcharLiteralReplace) != IDE_SUCCESS);
            break;

        /* BUG-31144 */
        case CMP_DB_PROPERTY_MAX_STATEMENTS_PER_SESSION:
            IDE_TEST_RAISE(((UInt)sArg->mValue.mValue.mUInt32 < sSession->getNumberOfStatementsInSession()), StatementNumberExceedsInputValue);
            IDE_TEST(cmtAnyReadUInt(&sArg->mValue, &sInfo->mMaxStatementsPerSession) != IDE_SUCCESS);
            break;
        /* BUG-31390 Failover info for v$session */
        case CMP_DB_PROPERTY_FAILOVER_SOURCE:
            IDE_TEST(cmtAnyReadVariable(&sArg->mValue, &sVariable) != IDE_SUCCESS);
            sLen = IDL_MIN(cmtVariableGetSize(sVariable), ID_SIZEOF(sInfo->mFailOverSource) - 1);
            IDE_TEST(cmtVariableGetData(sVariable,
                                        (UChar *)sInfo->mFailOverSource,
                                        sLen) != IDE_SUCCESS);
            sInfo->mFailOverSource[sLen] = 0;
            break;

        // BUG-34725
        case CMP_DB_PROPERTY_FETCH_PROTOCOL_TYPE:
            IDE_TEST( cmtAnyReadUInt( &sArg->mValue, &sInfo->mFetchProtocolType ) != IDE_SUCCESS );
            break;

        // PROJ-2256
        case CMP_DB_PROPERTY_REMOVE_REDUNDANT_TRANSMISSION:
            IDE_TEST( cmtAnyReadUInt( &sArg->mValue,
                                      &sInfo->mRemoveRedundantTransmission ) != IDE_SUCCESS );
            break;
            
        default:
            /* BUG-36256 Improve property's communication */
            IDE_RAISE(UnsupportedProperty);
            break;
    }

    return answerPropertySetResult(aProtocolContext);

    IDE_EXCEPTION(DCLNotAllowedError);
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_NOT_ALLOWED_DCL));
    }
    IDE_EXCEPTION(InvalidSessionState);
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_INVALID_SESSION_STATE));
    }
    IDE_EXCEPTION(DateFormatTooLong);
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_MMC_DATE_FORMAT_LENGTH_EXCEED,
                                MMC_DATEFORMAT_MAX_LEN));
    }

    /* BUG-31144 */
    IDE_EXCEPTION(StatementNumberExceedsInputValue);
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_STATEMENT_NUMBER_EXCEEDS_INPUT_VALUE));
    }

    IDE_EXCEPTION(UnsupportedProperty)
    {
        /*
         * BUG-36256 Improve property's communication
         *
         * CMP_OP_DB_ProprtySetResult는 OP(2)만 보내기 때문에 하위 호환성을
         * 유지하기 위해서는 ulnCallbackDBPropertySetResult 함수를 이용할 수 없다.
         * answerErrorResult를 이용해 에러를 발생시키지 않고 Client에게 응답을 준다.
         * 초기 설계가 아쉬운 부분이다.
         */
        ideLog::log(IDE_MM_0,
                    MM_TRC_SET_UNSUPPORTED_PROPERTY,
                    sArg->mPropertyID);

        IDE_SET(ideSetErrorCode(mmERR_IGNORE_UNSUPPORTED_PROPERTY, sArg->mPropertyID));

        return sThread->answerErrorResultA5(aProtocolContext,
                                            CMI_PROTOCOL_OPERATION(DB, PropertySet),
                                            sArg->mPropertyID);
    }

    IDE_EXCEPTION_END;

    return sThread->answerErrorResultA5(aProtocolContext,
                                      CMI_PROTOCOL_OPERATION(DB, PropertySet),
                                      0);
}
