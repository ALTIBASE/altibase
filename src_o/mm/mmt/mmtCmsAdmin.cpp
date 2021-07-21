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

#include <idl.h>
#include <smi.h>
#include <qci.h>
#include <dki.h>
#include <mmm.h>
#include <mmErrorCode.h>
#include <mmcTask.h>
#include <mmdManager.h>
#include <mmtServiceThread.h>
#include <mmtSessionManager.h>
#include <mmtAdminManager.h>
#include <mmuProperty.h>

#define ADM_SEND_MSG(msg)  IDE_CALLBACK_SEND_MSG_NOLOG((SChar *)(msg));
#define ADM_SEND_SYM(msg)  IDE_CALLBACK_SEND_SYM_NOLOG((SChar *)(msg));


IDE_RC mmtServiceThread::startupDatabase(idvSQL */*aStatistics*/, void *aArg)
{
    qciArgStartup *sArg = (qciArgStartup *)aArg;
    mmmPhase       sReqPhase = MMM_STARTUP_MAX;

    IDE_TEST_RAISE(qci::isSysdba(sArg->mQcStatement) != ID_TRUE, InsufficientPriv);

    switch (sArg->mPhase)
    {
        case QCI_STARTUP_PROCESS:
            sReqPhase = MMM_STARTUP_PROCESS;
            break;
        case QCI_STARTUP_CONTROL:
            sReqPhase = MMM_STARTUP_CONTROL;
            break;
        case QCI_STARTUP_META:
            sReqPhase = MMM_STARTUP_META;
            break;
        case QCI_STARTUP_SERVICE:
            sReqPhase = MMM_STARTUP_SERVICE;
            break;
        case QCI_META_DOWNGRADE:
            // PROJ-2689
            sReqPhase = MMM_STARTUP_DOWNGRADE;
            break;
        default:
            IDE_CALLBACK_FATAL("invalid startup phase");
            break;
    }

    IDE_TEST_RAISE(mmm::execute(sReqPhase, sArg->mStartupFlag) != IDE_SUCCESS, StartupError);

    return IDE_SUCCESS;

    IDE_EXCEPTION(InsufficientPriv);
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_INSUFFICIENT_PRIV));
    }
    IDE_EXCEPTION(StartupError);
    {
        if ( ideGetErrorCode() != mmERR_ABORT_STARTUP_PHASE_ERROR )
        {
            ADM_SEND_MSG("Startup Failed....");
            idlOS::exit(-1);
        }
        else
        {
            /* nothing to do */
        }
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mmtServiceThread::shutdownDatabase(idvSQL */*aStatistics*/, void *aArg)
{
    idBool          sExist = ID_FALSE;
    qciArgShutdown *sArg   = (qciArgShutdown *)aArg;

    /* TASK-5894 Permit sysdba via IPC */
    mmcTask        *sTask     = mmtAdminManager::getTask();
    cmnLinkImpl     sLinkImpl = CMN_LINK_IMPL_INVALID;

    if (sTask != NULL)
    {
        sLinkImpl = cmiGetLinkImpl(sTask->getLink());
    }
    else
    {
        /* Nothing */
    }

    IDE_TEST_RAISE(qci::isSysdba(sArg->mQcStatement) != ID_TRUE, InsufficientPriv);

    switch (sArg->mPhase)
    {
        case QCI_SHUTDOWN_NORMAL: /* normal */
            IDE_TEST_RAISE(mmm::getCurrentPhase() != MMM_STARTUP_SERVICE, LevelError);

            ideLog::log(IDE_SERVER_0, MM_TRC_SHUTDOWN_NORMAL);

            /* TASK-5894 Permit sysdba via IPC */
            IDE_TEST_RAISE(sLinkImpl == CMN_LINK_IMPL_IPC, NotSupportedViaIPC)

            /* BUG-20727 prepared transaction 이 존재하는 경우 transaction 을
               살려둬야 하기 때문에 abort 로 알티베이스를 종료한다. */
            (void)smiExistPreparedTrans(&sExist);
            if ( sExist == ID_TRUE )
            {
                ideLog::log(IDE_SERVER_0, MM_TRC_SHUTDOWN_EXIT);
                ADM_SEND_MSG("Hmm..There are Prepared Transaction Branches....\n");
                ADM_SEND_MSG("Shutdown Abort Altibase.");
                idlOS::exit(0);
            }

            /* BUG-47863
               Notifier에 등록되어있는 Global TX가 존재한다면 실패처리 */
            IDE_TEST_RAISE( ( dkiGetDtxInfoCnt() > 0 ), GlobalTransactionsAreOpen );

            IDU_FIT_POINT( "mmtServiceThread::shutdownDatabase::dkiGetDtxInfoCnt::normal_sleep" );

            mmm::prepareShutdown(ALTIBASE_STATUS_SHUTDOWN_NORMAL, ID_TRUE);

            break;

        case QCI_SHUTDOWN_IMMEDIATE:  /* immediate */
            IDE_TEST_RAISE(mmm::getCurrentPhase() != MMM_STARTUP_SERVICE, LevelError);

            ideLog::log(IDE_SERVER_0, MM_TRC_SHUTDOWN_IMMEDIATE);

            /* TASK-5894 Permit sysdba via IPC */
            IDE_TEST_RAISE(sLinkImpl == CMN_LINK_IMPL_IPC, NotSupportedViaIPC)

            /* BUG-20727 */
            (void)smiExistPreparedTrans(&sExist);
            if ( sExist == ID_TRUE )
            {
                ideLog::log(IDE_SERVER_0, MM_TRC_SHUTDOWN_EXIT);
                ADM_SEND_MSG("Hmm..There are Prepared Transaction Branches....\n");
                ADM_SEND_MSG("Shutdown Abort Altibase.");
                idlOS::exit(0);
            }

            /* BUG-47863
               Notifier에 등록되어있는 Global TX가 존재한다면 실패처리 */
            IDE_TEST_RAISE( ( dkiGetDtxInfoCnt() > 0 ), GlobalTransactionsAreOpen );

            IDU_FIT_POINT( "mmtServiceThread::shutdownDatabase::dkiGetDtxInfoCnt::immediate_sleep" );

            mmm::prepareShutdown(ALTIBASE_STATUS_SHUTDOWN_IMMEDIATE, ID_TRUE);

            break;

        case QCI_SHUTDOWN_EXIT: /* abort */
            ideLog::log(IDE_SERVER_0, MM_TRC_SHUTDOWN_EXIT);

            /* BUG-39946 */
            ideSetCallbackFatal(ideNullCallbackFuncForFatal);
            ideSetCallbackAssert(ideNullCallbackFuncForAssert);

            idlOS::exit(0);
            break;

        default:
            ADM_SEND_MSG("Not Support Shutdown Mode");
            break;
    }

    ADM_SEND_MSG("Ok..Shutdown Proceeding....");

    return IDE_SUCCESS;

    IDE_EXCEPTION(LevelError);
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_SHUTDOWN_MODE_ERROR));
    }
    IDE_EXCEPTION(InsufficientPriv);
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_INSUFFICIENT_PRIV));
    }
    IDE_EXCEPTION(NotSupportedViaIPC);
    {
        ideLog::log(IDE_SERVER_0, MM_TRC_ABORT_SHUTDOWN_NOT_SUPPORTED_VIA_IPC);
        IDE_SET(ideSetErrorCode(mmERR_ABORT_SHUTDOWN_NOT_SUPPORTED_VIA_IPC));
    }
    IDE_EXCEPTION(GlobalTransactionsAreOpen);
    {
        ideLog::log( IDE_SERVER_0, "[ERR] Shutdown failed. Global TX is still open." );

        (void)dkiPrintNotifierInfo();

        IDE_SET(ideSetErrorCode(mmERR_ABORT_GLOBAL_TRANSACTIONS_ARE_OPEN));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mmtServiceThread::commitDTX(idvSQL */*aStatistics*/, void *aArg)
{
    qciArgCommitDTX *sArg = (qciArgCommitDTX *)aArg;

    IDE_ASSERT(aArg != NULL);

    IDE_TEST(mmdManager::commitTransaction(sArg->mSmTID) != IDE_SUCCESS);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;

}

IDE_RC mmtServiceThread::rollbackDTX(idvSQL */*aStatistics*/, void *aArg)
{
    qciArgRollbackDTX *sArg = (qciArgRollbackDTX *)aArg;

    IDE_ASSERT(aArg != NULL);

    IDE_TEST(mmdManager::rollbackTransaction(sArg->mSmTID) != IDE_SUCCESS);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;

}

IDE_RC mmtServiceThread::closeSession(idvSQL */*aStatistics*/, void *aArg)
{
    qciArgCloseSession *sArg      = (qciArgCloseSession *)aArg;
    mmcSession         *sSession  = (mmcSession *)sArg->mMmSession;
    SInt                sResultCount;
    SChar               sMessage[30];

    IDE_ASSERT(aArg != NULL);

    // To Fix BUG-15361
    // validate DB Name
    IDE_TEST( validateDBName( sArg->mDBName, sArg->mDBNameLen )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( sSession->getSessionID() == sArg->mSessionID,
                    SELF_TERMINATION_ERROR );
    
    IDE_TEST( mmtSessionManager::terminate( sSession,
                                            sArg->mSessionID,
                                            sArg->mUserName,
                                            sArg->mUserNameLen,
                                            sArg->mCloseAll,
                                            &sResultCount )
              != IDE_SUCCESS );

    if( mmcSession::getShardInternalLocalOperation( sSession ) == SDI_INTERNAL_OP_NOT )
    {
        IDE_TEST_RAISE( sResultCount == 0, SESSION_NOT_FOUND_ERROR );

        idlOS::snprintf( sMessage, ID_SIZEOF(sMessage), MM_MSG_SESSION_CLOSED, sResultCount );

        mmcSession::printToClientCallback( sSession, (UChar *)sMessage, idlOS::strlen(sMessage) );
    }
    else
    {
        /* shard internal local operation이 켜 있는 경우 shard drop node에서
         * 내부적으로 호출된 것이므로 성공/실패 메시지를 넘기지 않도록 한다. */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( SELF_TERMINATION_ERROR )
    {
        IDE_SET( ideSetErrorCode(mmERR_ABORT_CANNOT_CLOSE_SELF_SESSION) );
    }
    IDE_EXCEPTION( SESSION_NOT_FOUND_ERROR )
    {
        IDE_SET( ideSetErrorCode(mmERR_ABORT_NO_SESSION_TO_CLOSE) );
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

/***********************************************************************
 *
 * Description :
 *
 *    To Fix BUG-15361
 *    DB 제어문에서 사용되는 database name의 이름을 검증한다.
 *
 *       Ex) ALTER DATABASE mydb
 *
 * Implementation :
 *
 *    DB Name은 다음 두 가지가 동일한 이름으로 존재한다.
 *       - altibase.properties의 DB_NAME
 *       - Database에 저장된 이름
 *
 *    DATABASE에 저장된 이름을 사용하여 검사한다.
 *
 **********************************************************************/

IDE_RC
mmtServiceThread::validateDBName( SChar * aDBName, UInt aDBNameLen )
{
    SChar * sBaseName;
    UInt    sBaseLength;

    //---------------------------------
    // Parameter Validation
    //---------------------------------

    IDE_DASSERT( aDBName != NULL );
    
    //---------------------------------
    // Get Internal DB Name
    //---------------------------------

    sBaseName = smiGetDBName();
    sBaseLength = idlOS::strlen( sBaseName );

    //---------------------------------
    // Validate DB Name
    //---------------------------------

    IDE_TEST_RAISE( idlOS::strCaselessMatch( aDBName,
                                             aDBNameLen,
                                             sBaseName,
                                             sBaseLength )
                    != 0, ERR_INVALID_DB_NAME );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_DB_NAME );
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_DATABASE_NAME_ERROR));
    }
    
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

    
