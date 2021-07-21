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

#include <cm.h>
#include <qci.h>
#include <mmErrorCode.h>
#include <mmcSession.h>
#include <mmcStatement.h>
#include <mmcStatementManager.h>
#include <mmcTask.h>
#include <mmtServiceThread.h>
#include <mmtAuditManager.h>

static IDE_RC answerPrepareResult(cmiProtocolContext *aProtocolContext, mmcStatement *aStatement)
{
    cmiProtocol            sProtocol;
    cmpArgDBPrepareResultA5 *sArg;

    // bug-25109 support ColAttribute baseTableName
    SChar  sBaseTableOwnerName[QC_MAX_OBJECT_NAME_LEN+1];
    SChar  sBaseTableName[QC_MAX_OBJECT_NAME_LEN+1];
    UInt   sBaseTableOwnerNameLen  = 0;
    UInt   sBaseTableNameLen       = 0;

    idBool sBaseTableUpdatable = ID_FALSE;

    qciStatement* sQciStmt = NULL;


    sQciStmt = aStatement->getQciStmt();
    IDE_TEST(sQciStmt == NULL);

    CMI_PROTOCOL_INITIALIZE(sProtocol, sArg, DB, PrepareResult);

    sArg->mStatementID    = aStatement->getStmtID();
    sArg->mStatementType  = aStatement->getStmtType();
    sArg->mParamCount     = qci::getParameterCount(sQciStmt);
    sArg->mResultSetCount = aStatement->getResultSetCount();


    // bug-25109 support ColAttribute baseTableName
    sBaseTableOwnerName[0] = '\0';
    sBaseTableName[0]      = '\0';

    // cf) view인 경우 QP에서 table 명이 아닌 view 명을 반환.
    // 하지만, view인 경우 사용안할 것이므로 상관없음.
    IDE_TEST(qci::getBaseTableInfo(sQciStmt,
                                   sBaseTableOwnerName,
                                   sBaseTableName,
                                   &sBaseTableUpdatable) != IDE_SUCCESS);

    // fix BUG-28267 [codesonar] Ignored Return Value
    sBaseTableOwnerNameLen = idlOS::strlen(sBaseTableOwnerName);
    IDE_TEST(sBaseTableOwnerNameLen > QCI_MAX_OBJECT_NAME_LEN);

    sBaseTableNameLen      = idlOS::strlen(sBaseTableName);
    IDE_TEST(sBaseTableNameLen > QCI_MAX_OBJECT_NAME_LEN);

    IDE_TEST(cmtVariableSetData(&(sArg->mBaseTableOwnerName),
                       (UChar*)sBaseTableOwnerName, 
                       sBaseTableOwnerNameLen) != IDE_SUCCESS);

    IDE_TEST(cmtVariableSetData(&(sArg->mBaseTableName),
                       (UChar*)sBaseTableName, 
                       sBaseTableNameLen) != IDE_SUCCESS);

    // table: ID_TRUE, view: ID_FALSE
    sArg->mBaseTableUpdatable = (UShort)sBaseTableUpdatable;

    IDE_TEST(cmiWriteProtocol(aProtocolContext, &sProtocol) != IDE_SUCCESS);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

static IDE_RC answerPlanGetResult(cmiProtocolContext *aProtocolContext,
                                  mmcStmtID           aStmtID,
                                  iduVarString       *aPlanString)
{
    cmiProtocol            sProtocol;
    cmpArgDBPlanGetResultA5 *sArg;

    CMI_PROTOCOL_INITIALIZE(sProtocol, sArg, DB, PlanGetResult);

    sArg->mStatementID = aStmtID;

    IDE_TEST(cmtVariableSetVarString(&sArg->mPlan, aPlanString) != IDE_SUCCESS);

    IDE_TEST(cmiWriteProtocol(aProtocolContext, &sProtocol) != IDE_SUCCESS);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

static IDE_RC answerFreeResult(cmiProtocolContext *aProtocolContext)
{
    cmiProtocol         sProtocol;
    cmpArgDBFreeResultA5 *sArg;

    CMI_PROTOCOL_INITIALIZE(sProtocol, sArg, DB, FreeResult);

    ACP_UNUSED(sArg);

    IDE_TEST(cmiWriteProtocol(aProtocolContext, &sProtocol) != IDE_SUCCESS);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}


IDE_RC mmtServiceThread::prepareProtocolA5(cmiProtocolContext *aProtocolContext,
                                         cmiProtocol        *aProtocol,
                                         void               *aSessionOwner,
                                         void               *aUserContext)
{
    cmpArgDBPrepareA5  *sArg    = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, Prepare);
    mmcTask          *sTask   = (mmcTask *)aSessionOwner;
    mmtServiceThread *sThread = (mmtServiceThread *)aUserContext;
    mmcSession       *sSession;
    //fix BUG-18284.
    mmcStatement     *sStatement = NULL;
    SChar            *sQuery;
    UInt              sQueryLen;
    IDE_RC            sRet;

    IDE_CLEAR();

    IDE_TEST(findSession(sTask, &sSession, sThread) != IDE_SUCCESS);

    IDE_TEST(checkSessionState(sSession, MMC_SESSION_STATE_SERVICE) != IDE_SUCCESS);

    if (sArg->mStatementID == 0)
    {
        IDE_TEST(mmcStatementManager::allocStatement(&sStatement, sSession, NULL) != IDE_SUCCESS);

        sThread->setStatement(sStatement);
    }
    else
    {
        IDE_TEST(findStatement(&sStatement,
                               sSession,
                               &sArg->mStatementID,
                               sThread) != IDE_SUCCESS);

        /* PROJ-2223 Altibase Auditing */
        mmtAuditManager::auditBySession( sStatement );

        IDE_TEST(sStatement->clearPlanTreeText() != IDE_SUCCESS);

        IDE_TEST_RAISE(sStatement->getStmtState() >= MMC_STMT_STATE_EXECUTED,
                       InvalidStatementState);

        IDE_TEST(qci::clearStatement(sStatement->getQciStmt(),
                                     sStatement->getSmiStmt(),
                                     QCI_STMT_STATE_INITIALIZED) != IDE_SUCCESS);

        sStatement->setStmtState(MMC_STMT_STATE_ALLOC);
    }
    // bug-34746: query_timeout flag is cleared by other stmt
    // query_timeout 확인시 mCurrStmtID로 비교한다
    sSession->getInfo()->mCurrStmtID = sStatement->getStmtID();

    /* BUG-38472 Query timeout applies to one statement. */
    IDU_SESSION_CLR_TIMEOUT( *sSession->getEventFlag() );

    sQueryLen = cmtVariableGetSize(&sArg->mStatementString);

    IDE_TEST_RAISE(sQueryLen == 0, NullQuery);

    IDU_FIT_POINT("mmtServiceThread::prepareProtocolA5::malloc::Query");
    IDE_TEST(iduMemMgr::malloc(IDU_MEM_MMC,
                               sQueryLen + 1,
                               (void **)&sQuery,
                               IDU_MEM_IMMEDIATE) != IDE_SUCCESS);

    IDE_TEST(cmtVariableGetData(&sArg->mStatementString,
                                (UChar *)sQuery,
                                sQueryLen) != IDE_SUCCESS);

    sQuery[sQueryLen] = 0;

    sStatement->setStmtExecMode(sArg->mMode ? MMC_STMT_EXEC_DIRECT : MMC_STMT_EXEC_PREPARED);

    IDE_TEST(sStatement->prepare(sQuery, sQueryLen) != IDE_SUCCESS);

    return answerPrepareResult(aProtocolContext, sStatement);

    IDE_EXCEPTION(InvalidStatementState);
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_INVALID_STATEMENT_STATE_ERROR));
    }
    IDE_EXCEPTION(NullQuery);
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_INSUFFICIENT_QUERY_ERROR));
    }
    IDE_EXCEPTION_END;
    {
        sRet = sThread->answerErrorResultA5(aProtocolContext,
                                          CMI_PROTOCOL_OPERATION(DB, Prepare),
                                          0);

        if (sRet == IDE_SUCCESS)
        {
            sThread->mErrorFlag = ID_TRUE;
        }
        //fix BUG-18284.
        // do exactly same as CMP_DB_FREE_DROP
        if (sArg->mStatementID == 0)
        {
            if(sStatement != NULL)
            {    
                sThread->setStatement(NULL);

                IDE_ASSERT(sStatement->closeCursor(ID_TRUE) == IDE_SUCCESS);
                IDE_ASSERT(mmcStatementManager::freeStatement(sStatement) == IDE_SUCCESS);

            }//if
        }//if
    }
    
    return sRet;
}

IDE_RC mmtServiceThread::planGetProtocolA5(cmiProtocolContext *aProtocolContext,
                                         cmiProtocol        *aProtocol,
                                         void               *aSessionOwner,
                                         void               *aUserContext)
{
    cmpArgDBPlanGetA5  *sArg    = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, PlanGet);
    mmcTask          *sTask   = (mmcTask *)aSessionOwner;
    mmtServiceThread *sThread = (mmtServiceThread *)aUserContext;
    mmcSession       *sSession;
    mmcStatement     *sStatement;

    IDE_CLEAR();

    IDE_TEST(findSession(sTask, &sSession, sThread) != IDE_SUCCESS);

    IDE_TEST(checkSessionState(sSession, MMC_SESSION_STATE_SERVICE) != IDE_SUCCESS);

    IDE_TEST(findStatement(&sStatement,
                           sSession,
                           &sArg->mStatementID,
                           sThread) != IDE_SUCCESS);

    if (sStatement->isPlanPrinted() != ID_TRUE)
    {
        IDE_TEST(checkStatementState(sStatement, MMC_STMT_STATE_PREPARED) != IDE_SUCCESS);

        IDE_TEST(sStatement->makePlanTreeText(ID_TRUE) != IDE_SUCCESS);
    }

    return answerPlanGetResult(aProtocolContext,
                               sStatement->getStmtID(),
                               sStatement->getPlanString());

    IDE_EXCEPTION_END;

    return sThread->answerErrorResultA5(aProtocolContext,
                                      CMI_PROTOCOL_OPERATION(DB, PlanGet),
                                      0);
}

IDE_RC mmtServiceThread::freeProtocolA5(cmiProtocolContext *aProtocolContext,
                                      cmiProtocol        *aProtocol,
                                      void               *aSessionOwner,
                                      void               *aUserContext)
{
    cmpArgDBFreeA5     *sArg    = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, Free);
    mmcTask          *sTask   = (mmcTask *)aSessionOwner;
    mmtServiceThread *sThread = (mmtServiceThread *)aUserContext;
    mmcSession       *sSession;
    mmcStatement     *sStatement;

    IDE_CLEAR();

    IDE_TEST(findSession(sTask, &sSession, sThread) != IDE_SUCCESS);

    IDE_TEST(checkSessionState(sSession, MMC_SESSION_STATE_SERVICE) != IDE_SUCCESS);

    IDE_TEST(findStatement(&sStatement,
                           sSession,
                           &sArg->mStatementID,
                           sThread) != IDE_SUCCESS);

    switch (sArg->mMode)
    {
        case CMP_DB_FREE_CLOSE:
            if ((sStatement->getStmtState() >= MMC_STMT_STATE_EXECUTED) &&
                (sSession->getExplainPlan() == QCI_EXPLAIN_PLAN_ON))
            {
                IDE_TEST(sStatement->makePlanTreeText(ID_FALSE) != IDE_SUCCESS);
            }

            IDE_TEST(sStatement->closeCursor(ID_TRUE) != IDE_SUCCESS);

            break;

        case CMP_DB_FREE_DROP:
            sThread->setStatement(NULL);

            IDE_TEST(sStatement->closeCursor(ID_TRUE) != IDE_SUCCESS);

            IDE_TEST(mmcStatementManager::freeStatement(sStatement) != IDE_SUCCESS);

            break;

        default:
            IDE_RAISE(InvalidFreeMode);
            break;
    }

    return answerFreeResult(aProtocolContext);

    IDE_EXCEPTION(InvalidFreeMode);
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_INVALID_STATEMENT_FREE_MODE));
    }
    IDE_EXCEPTION_END;

    return sThread->answerErrorResultA5(aProtocolContext,
                                      CMI_PROTOCOL_OPERATION(DB, Free),
                                      0);
}
