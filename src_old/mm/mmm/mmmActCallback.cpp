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
#include <sti.h>
#include <rpi.h>
#include <mmm.h>
#include <mmi.h>
#include <mmErrorCode.h>
#include <mmcSession.h>
#include <mmtThreadManager.h>
#include <mmtServiceThread.h>
#include <mmqManager.h>
#include <mmtInternalSql.h>

idBool assertSystemError(SChar *aFile, SInt aLine, idBool aAcceptFaultTolerance)
{
    idBool      sIsFaultTolerate = ID_FALSE; /* PROJ-2617 */
    ideLogEntry sLog(IDE_ERR_0);

    sLog.appendFormat(MM_TRC_ASSERT, aFile, aLine);
    /* fix BUG-28226 ,
       IDE_ASSERT에서 last altibase error code, error message를 찍었으면 좋겠습니다. */
    ideLog::logErrorMsgInternal(sLog);

    sLog.write();

    iduFatalCallback::doCallback();

    ideLog::flushAllModuleLogs();

    if (aAcceptFaultTolerance == ID_TRUE)
    {
        if (mmm::getCurrentPhase() == MMM_STARTUP_SERVICE)
        {
            sIsFaultTolerate = ideCanFaultTolerate();
        }
        else
        {
            /* nothing to do */
        }
    }
    else
    {
        /* nothing to do */
    }

    return (sIsFaultTolerate == ID_TRUE) ? ID_FALSE : ID_TRUE;
}

void urgentSystemError(SChar *aFile, SInt aLine, SChar *aMsg)
{
    ideLogEntry sLog(IDE_ERR_0);
    sLog.appendFormat(MM_TRC_FATAL, aFile, aLine, aMsg);

    sLog.append(MM_TRC_CURRENT_IDE_STACK);
    ideLog::logErrorMsgInternal(sLog);
    sLog.append(MM_TRC_FATAL_TERMINATED);

    if(mmtThreadManager::logCurrentSessionInfo(sLog) != ID_TRUE)
    {
        sLog.append(MM_TRC_SERVER_NOT_ABORTED_BY_SERVICE_SESSION);
    }
    sLog.write();

    ideLogEntry sLog2(IDE_DUMP_0);
    idp::dumpProperty( sLog2 );
    sLog2.write();

    iduFatalCallback::doCallback();

    ideLog::flushAllModuleLogs();

    // script
#if defined(INTEL_LINUX) || defined(IA64_SUSE_LINUX) || defined(IA64_LINUX) || defined(ALPHA_LINUX) \
    || defined(POWERPC_LINUX) || defined(POWERPC64_LINUX) || defined(AMD64_LINUX) || defined(XEON_LINUX) || defined(X86_64_LINUX)
    /* PR-3951 workaround for LINUX-THREAD
     * Don't call idlOS::abort() on linux. I don't know why. (maybe signal problem)
     * anyway, exit(-1) call works well now.
     */
    idlOS::exit(-1);
#else
    idlOS::abort();
#endif
}

void logCurState(SChar *aFile, SInt aLine, SChar *aMsg)
{
    ideLogEntry sLog(IDE_SERVER_0);
    sLog.appendFormat(MM_TRC_WARNING, ideGetSystemErrno(), aFile, aLine, aMsg);

    ideLog::logErrorMsgInternal(sLog);
    sLog.write();
}

void mmiErrorLog(SChar *aFile, SInt aLine)
{
    ideLogEntry sLog(IDE_SERVER_0);
    ideLog::logErrorMsgInternal(sLog);
    sLog.appendFormat("[%s:%d]\n", aFile, aLine);
    sLog.write();
}

IDE_RC mmmLinkCheckCallback(void *aLink)
{
    idBool sIsClosed;

    if (cmiCheckLink((cmiLink *)aLink, &sIsClosed) == IDE_SUCCESS)
    {
        return sIsClosed ? IDE_FAILURE : IDE_SUCCESS;
    }
    else
    {
        return IDE_FAILURE;
    }
}

//fix PROJ-1749
idBool mmmGetDumpStack()
{
    return ID_TRUE;
}


/* =======================================================
 * Action Function
 * => Setup All Callback Functions
 * =====================================================*/

static IDE_RC mmmPhaseActionCallback(mmmPhase         /*aPhase*/,
                                     UInt             /*aOptionflag*/,
                                     mmmPhaseAction * /*aAction*/)
{
    mmm::mStartupTime = idlOS::gettimeofday();

    ideSetCallbackFatal(urgentSystemError);
    ideSetCallbackWarning(logCurState);
    ideSetCallbackErrlog(mmiErrorLog);
    ideSetCallbackAssert(assertSystemError);

    //fix PROJ-1749
    if((mmi::getServerOption() & MMI_SIGNAL_MASK) == MMI_SIGNAL_TRUE)
    {
        ideSetCallbackDumpStack(mmmGetDumpStack);
    }

    idvManager::setLinkCheckCallback(mmmLinkCheckCallback);
    idvManager::setBaseTimeCallback(mmtSessionManager::getBaseTime);

    qci::setDatabaseCallback(mmtServiceThread::createDatabase,
                             mmtServiceThread::dropDatabase,
                             mmtServiceThread::closeSession,
                             mmtServiceThread::commitDTX,
                             mmtServiceThread::rollbackDTX,
                             mmtServiceThread::startupDatabase,
                             mmtServiceThread::shutdownDatabase);

    IDE_TEST( sti::addExtQP_Callback() != IDE_SUCCESS );

    smiSetSessionCallback(&mmcSession::mSessionInfoCallback); 
 
    /* PROJ-2735 DDL Transaction */   
    smiSetTransactionalDDLCallback( &qciMisc::mTransactionalDDLCallback );
    
    qci::setSessionCallback(&mmcSession::mCallback);

    qci::setQueueCallback(mmqManager::createQueue,
                          mmqManager::dropQueue,
                          mmqManager::enqueue,
                          mmqManager::dequeue,
                          mmqManager::setQueueStamp);

    qci::setOutBindLobCallback(mmtServiceThread::outBindLobCallback,
                               mmtServiceThread::closeOutBindLobCallback);

    qci::setInternalSQLCallback( &mmtInternalSql::mCallback );
    
    qci::setParamData4RebuildCallback( mmtServiceThread::setParamData4RebuildCallback );

    qci::setReplicationCallback( rpi::getReplicationValidateCallback( ),
                                 rpi::getReplicationExecuteCallback( ),
                                 rpi::getReplicationCatalogCallback( ),
                                 rpi::getReplicationManageCallback( ) );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* =======================================================
 *  Action Func Object for Action Descriptor
 * =====================================================*/

mmmPhaseAction gMmmActCallback =
{
    (SChar *)"Setup Callback..",
    MMM_ACTION_NO_LOG,
    mmmPhaseActionCallback
};
