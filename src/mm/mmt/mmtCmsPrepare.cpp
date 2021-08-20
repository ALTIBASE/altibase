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
    UInt               sStatementID     = aStatement->getStmtID();
    UInt               sStatementType   = aStatement->getStmtType();
    UShort             sParamCount      = qci::getParameterCount(aStatement->getQciStmt());
    UShort             sResultSetCount  = aStatement->getResultSetCount();
    cmiWriteCheckState sWriteCheckState = CMI_WRITE_CHECK_DEACTIVATED;
    ULong              sValue8          = 0;

    switch (aProtocolContext->mProtocol.mOpID)
    {
        case CMP_OP_DB_PrepareV3:
        case CMP_OP_DB_PrepareByCIDV3:
            sWriteCheckState = CMI_WRITE_CHECK_ACTIVATED;
            /* BUG-44125 [mm-cli] IPCDA 모드 테스트 중 hang - iloader CLOB */
            CMI_WRITE_CHECK_WITH_IPCDA(aProtocolContext, 21, 21 + 1);
            sWriteCheckState = CMI_WRITE_CHECK_DEACTIVATED;

            CMI_WOP(aProtocolContext, CMP_OP_DB_PrepareV3Result);
            CMI_WR4(aProtocolContext, &sStatementID);
            CMI_WR4(aProtocolContext, &sStatementType);
            CMI_WR2(aProtocolContext, &sParamCount);
            CMI_WR2(aProtocolContext, &sResultSetCount);
            CMI_WR8(aProtocolContext, &sValue8);  /* BUG-48775 Reserved 8 bytes */
            break;

        case CMP_OP_DB_Prepare:
        case CMP_OP_DB_PrepareByCID:
            sWriteCheckState = CMI_WRITE_CHECK_ACTIVATED;
            /* BUG-44125 [mm-cli] IPCDA 모드 테스트 중 hang - iloader CLOB */
            CMI_WRITE_CHECK_WITH_IPCDA(aProtocolContext, 13, 13 + 1);
            sWriteCheckState = CMI_WRITE_CHECK_DEACTIVATED;

            CMI_WOP(aProtocolContext, CMP_OP_DB_PrepareResult);
            CMI_WR4(aProtocolContext, &sStatementID);
            CMI_WR4(aProtocolContext, &sStatementType);
            CMI_WR2(aProtocolContext, &sParamCount);
            CMI_WR2(aProtocolContext, &sResultSetCount);
            break;

        default:
            IDE_DASSERT(0);
            break;
    }

    if (cmiGetLinkImpl(aProtocolContext) == CMI_LINK_IMPL_IPCDA)
    {
        /*PROJ-2616*/
        CMI_WR1(aProtocolContext, aStatement->isSimpleQuery() == ID_TRUE ? 1 : 0);
    }
    else
    {
        /* do nothing. */
    }

    /* PROJ-2616 */
    MMT_IPCDA_INCREASE_DATA_COUNT(aProtocolContext);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    /* BUG-44124 ipcda 모드 사용 중 hang - iloader 컬럼이 많은 테이블 */
    if( (sWriteCheckState == CMI_WRITE_CHECK_ACTIVATED) && (cmiGetLinkImpl(aProtocolContext) == CMI_LINK_IMPL_IPCDA) )
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_IPCDA_MESSAGE_TOO_LONG, CMB_BLOCK_DEFAULT_SIZE));
    }

    return IDE_FAILURE;
}

static IDE_RC answerPlanGetResult(cmiProtocolContext *aProtocolContext,
                                  mmcStmtID           aStmtID,
                                  iduVarString       *aPlanString)
{
    UInt               sLen;
    iduListNode       *sIterator;
    iduVarStringPiece *sPiece;
    UShort             sOrgWriteCursor  = CMI_GET_CURSOR(aProtocolContext);
    cmiWriteCheckState sWriteCheckState = CMI_WRITE_CHECK_DEACTIVATED;

    sLen = iduVarStringGetLength(aPlanString);

    sWriteCheckState = CMI_WRITE_CHECK_ACTIVATED;
    CMI_WRITE_CHECK(aProtocolContext, 9);
    sWriteCheckState = CMI_WRITE_CHECK_DEACTIVATED;

    CMI_WOP(aProtocolContext, CMP_OP_DB_PlanGetResult);
    CMI_WR4(aProtocolContext, &aStmtID);
    CMI_WR4(aProtocolContext, &sLen);

    IDU_LIST_ITERATE(&aPlanString->mPieceList, sIterator)
    {
        sPiece = (iduVarStringPiece *)sIterator->mObj;

        sWriteCheckState = CMI_WRITE_CHECK_ACTIVATED;
        CMI_WRITE_CHECK(aProtocolContext, sPiece->mLength);
        sWriteCheckState = CMI_WRITE_CHECK_DEACTIVATED;
        CMI_WCP(aProtocolContext, sPiece->mData, sPiece->mLength);
    }

    /* PROJ-2616 */
    MMT_IPCDA_INCREASE_DATA_COUNT(aProtocolContext);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    /* BUG-44124 ipcda 모드 사용 중 hang - iloader 컬럼이 많은 테이블 */
    if( (sWriteCheckState == CMI_WRITE_CHECK_ACTIVATED) && (cmiGetLinkImpl(aProtocolContext) == CMI_LINK_IMPL_IPCDA) )
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_IPCDA_MESSAGE_TOO_LONG, CMB_BLOCK_DEFAULT_SIZE));
    }

    CMI_SET_CURSOR(aProtocolContext, sOrgWriteCursor);

    return IDE_FAILURE;
}

static IDE_RC answerFreeResult(cmiProtocolContext *aProtocolContext)
{
    cmiWriteCheckState sWriteCheckState = CMI_WRITE_CHECK_DEACTIVATED;

    sWriteCheckState = CMI_WRITE_CHECK_ACTIVATED;
    CMI_WRITE_CHECK(aProtocolContext, 1);
    sWriteCheckState = CMI_WRITE_CHECK_DEACTIVATED;

    CMI_WOP(aProtocolContext, CMP_OP_DB_FreeResult);

    /* PROJ-2616 */
    MMT_IPCDA_INCREASE_DATA_COUNT(aProtocolContext);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    /* BUG-44124 ipcda 모드 사용 중 hang - iloader 컬럼이 많은 테이블 */
    if( (sWriteCheckState == CMI_WRITE_CHECK_ACTIVATED) && (cmiGetLinkImpl(aProtocolContext) == CMI_LINK_IMPL_IPCDA) )
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_IPCDA_MESSAGE_TOO_LONG, CMB_BLOCK_DEFAULT_SIZE));
    }

    return IDE_FAILURE;
}


IDE_RC mmtServiceThread::prepareProtocol(cmiProtocolContext *aProtocolContext,
                                         cmiProtocol        *aProtocol,
                                         void               *aSessionOwner,
                                         void               *aUserContext)
{
    mmcTask          *sTask   = (mmcTask *)aSessionOwner;
    mmtServiceThread *sThread = (mmtServiceThread *)aUserContext;
    mmcSession       *sSession;
    //fix BUG-18284.
    mmcStatement     *sStatement = NULL;
    SChar            *sQuery;
    IDE_RC            sRet;

    UInt              sStatementID;
    UInt              sStatementStringLen;
    UInt              sRowSize;
    UChar             sMode;

    /* PROJ-2160 CM 타입제거
       모두 읽은 다음에 프로토콜을 처리해야 한다. */
    switch (aProtocol->mOpID)
    {
        case CMP_OP_DB_PrepareV3:
            CMI_RD4(aProtocolContext, &sStatementID);
            CMI_RD1(aProtocolContext, sMode);
            CMI_SKIP_READ_BLOCK(aProtocolContext, 17);  /* BUG-48775 Reserved 17 bytes */
            CMI_RD4(aProtocolContext, &sStatementStringLen);
            break;

        case CMP_OP_DB_Prepare:
            CMI_RD4(aProtocolContext, &sStatementID);
            CMI_RD1(aProtocolContext, sMode);
            CMI_RD4(aProtocolContext, &sStatementStringLen);
            break;

        default:
            IDE_DASSERT(0);
            break;
    }

    sRowSize   = sStatementStringLen;

    IDE_CLEAR();

    IDE_TEST(findSession(sTask, &sSession, sThread) != IDE_SUCCESS);

    IDE_TEST(checkSessionState(sSession, MMC_SESSION_STATE_SERVICE) != IDE_SUCCESS);

    if (sStatementID == 0)
    {
        IDE_TEST(mmcStatementManager::allocStatement(&sStatement, sSession, NULL) != IDE_SUCCESS);

        sThread->setStatement(sStatement);

        /* PROJ-2177 User Interface - Cancel */
        sSession->getInfo()->mCurrStmtID = sStatement->getStmtID();
        IDU_SESSION_CLR_CANCELED(*sSession->getEventFlag());

        /* BUG-38472 Query timeout applies to one statement. */
        IDU_SESSION_CLR_TIMEOUT( *sSession->getEventFlag() );
    }
    else
    {
        IDE_TEST(findStatement(&sStatement,
                               sSession,
                               &sStatementID,
                               sThread) != IDE_SUCCESS);

        /* PROJ-2223 Altibase Auditing */
        mmtAuditManager::auditBySession( sStatement );

        IDE_TEST(sStatement->clearPlanTreeText() != IDE_SUCCESS);

        IDE_TEST_RAISE(sStatement->getStmtState() >= MMC_STMT_STATE_EXECUTED,
                       InvalidStatementState);


        IDE_TEST(qci::clearStatement(sStatement->getQciStmt(),
                                     sStatement->getSmiStmt(),
                                     QCI_STMT_STATE_INITIALIZED) != IDE_SUCCESS);

        sStatement->clearGCTxStmtInfo();
        sStatement->setStmtState(MMC_STMT_STATE_ALLOC);
    }

    IDE_TEST_RAISE(sStatementStringLen == 0, NullQuery);

    IDU_FIT_POINT( "mmtServiceThread::prepareProtocol::malloc::Query" );

    IDE_TEST(iduMemMgr::malloc(IDU_MEM_MMC,
                               sStatementStringLen + 1,
                               (void **)&sQuery,
                               IDU_MEM_IMMEDIATE) != IDE_SUCCESS);

    if (cmiGetLinkImpl(aProtocolContext) == CMI_LINK_IMPL_IPCDA)
    {
        IDE_TEST_RAISE( cmiSplitReadIPCDA( aProtocolContext,
                                           sRowSize,
                                           (UChar**)&sQuery,
                                           (UChar*)sQuery)
                        != IDE_SUCCESS, cm_error );
    }
    else
    {
        IDE_TEST_RAISE( cmiSplitRead( aProtocolContext,
                                      sRowSize,
                                      (UChar*)sQuery,
                                      NULL )
                        != IDE_SUCCESS, cm_error );
    }
    sRowSize = 0;

    sQuery[sStatementStringLen] = 0;

    /* PROJ-1381 FAC : Mode 설정 */
    if ( (sMode & CMP_DB_PREPARE_MODE_EXEC_MASK) == CMP_DB_PREPARE_MODE_EXEC_PREPARE )
    {
        sStatement->setStmtExecMode(MMC_STMT_EXEC_PREPARED);
    }
    else
    {
        sStatement->setStmtExecMode(MMC_STMT_EXEC_DIRECT);
    }

    if ( ((sMode & CMP_DB_PREPARE_MODE_HOLD_MASK) == CMP_DB_PREPARE_MODE_HOLD_ON)
     &&  (sSession->getCommitMode() == MMC_COMMITMODE_NONAUTOCOMMIT) )
    {
        /* BUG-49075 샤딩에서는 FAC가 지원되지 않는다. */
        IDE_TEST_RAISE(sdi::isShardEnable() == ID_TRUE, NotSupportCursorHold);
        sStatement->setCursorHold(MMC_STMT_CURSOR_HOLD_ON);
    }
    else
    {
        sStatement->setCursorHold(MMC_STMT_CURSOR_HOLD_OFF);
    }

    if ( (sMode & CMP_DB_PREPARE_MODE_KEYSET_MASK) == CMP_DB_PREPARE_MODE_KEYSET_ON )
    {
        /* BUG-47050 샤딩에서는 KEYSET이 지원되지 않는다. */
        IDE_TEST_RAISE(sdi::isShardEnable() == ID_TRUE, NotSupportKeysetOn);
        sStatement->setKeysetMode(MMC_STMT_KEYSETMODE_ON);
    }
    else
    {
        sStatement->setKeysetMode(MMC_STMT_KEYSETMODE_OFF);
    }

    /* TASK-7219 Non-shard DML */
    if ( (sMode & CMP_DB_PREPARE_MODE_SHARD_PARTIAL_EXEC_MASK) == CMP_DB_PREPARE_MODE_SHARD_PARTIAL_EXEC_COORD )
    {
        sStatement->setShardPartialExecType( SDI_SHARD_PARTIAL_EXEC_TYPE_COORD );
    }
    else if ( (sMode & CMP_DB_PREPARE_MODE_SHARD_PARTIAL_EXEC_MASK) == CMP_DB_PREPARE_MODE_SHARD_PARTIAL_EXEC_QUERY )
    {
        sStatement->setShardPartialExecType( SDI_SHARD_PARTIAL_EXEC_TYPE_QUERY );
    }
    else
    {
        sStatement->setShardPartialExecType( SDI_SHARD_PARTIAL_EXEC_TYPE_NONE );
    }

    IDE_TEST(sStatement->prepare(sQuery, sStatementStringLen) != IDE_SUCCESS);

    IDE_TEST( answerPrepareResult(aProtocolContext, sStatement) != IDE_SUCCESS );

    if ( sSession->isNeedRebuildNoti() == ID_TRUE )
    {
        IDE_TEST( sendShardRebuildNoti( aProtocolContext )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(InvalidStatementState)
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_INVALID_STATEMENT_STATE_ERROR));
    }
    IDE_EXCEPTION(NullQuery)
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_INSUFFICIENT_QUERY_ERROR));
    }
    IDE_EXCEPTION(cm_error)
    {
        return IDE_FAILURE;
    }
    IDE_EXCEPTION(NotSupportCursorHold)
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDA_NOT_SUPPORTED_SQLTEXT_FOR_SHARD,
                                  "CURSOR HOLD ON restrictions",
                                  "" ) );
    }
    IDE_EXCEPTION(NotSupportKeysetOn)
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDA_NOT_SUPPORTED_SQLTEXT_FOR_SHARD,
                                  "KEYSET ON restrictions",
                                  "" ) );
    }
    IDE_EXCEPTION_END;

    if( sRowSize != 0 )
    {
        IDE_TEST_RAISE( cmiSplitSkipRead( aProtocolContext,
                                          sRowSize,
                                          NULL )
                        != IDE_SUCCESS, cm_error );
    }

    sRet = sThread->answerErrorResult(aProtocolContext,
                                      aProtocol->mOpID,
                                      0,
                                      sSession);

    if (sRet == IDE_SUCCESS)
    {
        sThread->mErrorFlag = ID_TRUE;
    }

    //fix BUG-18284.
    // do exactly same as CMP_DB_FREE_DROP
    if (sStatementID == 0)
    {
        if(sStatement != NULL)
        {
            sThread->setStatement(NULL);

            IDE_ASSERT(  sStatement->closeCursor(ID_TRUE) == IDE_SUCCESS );
            IDE_ASSERT( mmcStatementManager::freeStatement(sStatement) == IDE_SUCCESS );
        }//if
    }//if
    
    return sRet;
}

/* PROJ-2177 */
IDE_RC mmtServiceThread::prepareByCIDProtocol(cmiProtocolContext *aProtocolContext,
                                              cmiProtocol        *aProtocol,
                                              void               *aSessionOwner,
                                              void               *aUserContext)
{
    mmcTask                 *sTask   = (mmcTask *)aSessionOwner;
    mmtServiceThread        *sThread = (mmtServiceThread *)aUserContext;
    mmcSession              *sSession;
    mmcStatement            *sStatement = NULL;
    mmcStmtID               sStmtID;
    SChar                   *sQuery = NULL;
    IDE_RC                  sRet;

    UInt                    sStmtCID;
    UInt                    sStatementStringLen;
    UInt                    sRowSize;
    UChar                   sMode;

    /* PROJ-2160 CM 타입제거
       모두 읽은 다음에 프로토콜을 처리해야 한다. */
    switch (aProtocol->mOpID)
    {
        case CMP_OP_DB_PrepareByCIDV3:
            CMI_RD4(aProtocolContext, &sStmtCID);
            CMI_RD1(aProtocolContext, sMode);
            CMI_SKIP_READ_BLOCK(aProtocolContext, 17);  /* BUG-48775 Reserved 17 bytes */
            CMI_RD4(aProtocolContext, &sStatementStringLen);
            break;

        case CMP_OP_DB_PrepareByCID:
            CMI_RD4(aProtocolContext, &sStmtCID);
            CMI_RD1(aProtocolContext, sMode);
            CMI_RD4(aProtocolContext, &sStatementStringLen);
            break;

        default:
            IDE_DASSERT(0);
            break;
    }

    sRowSize   = sStatementStringLen;

    IDE_CLEAR();

    IDE_TEST(findSession(sTask, &sSession, sThread) != IDE_SUCCESS);

    IDE_TEST(checkSessionState(sSession, MMC_SESSION_STATE_SERVICE) != IDE_SUCCESS);

    IDE_TEST_RAISE(sStmtCID == MMC_STMT_CID_NONE, StmtNotFoundException);

    sStmtID = sSession->getStmtIDFromMap(sStmtCID);
    if (sStmtID == MMC_STMT_ID_NONE)
    {
        IDE_TEST(mmcStatementManager::allocStatement(&sStatement, sSession, NULL) != IDE_SUCCESS);

        sStatement->setStmtCID(sStmtCID);
        IDE_TEST(sSession->putStmtIDMap(sStmtCID, sStatement->getStmtID()) != IDE_SUCCESS);

        sThread->setStatement(sStatement);

        /* PROJ-2177 User Interface - Cancel */
        sSession->getInfo()->mCurrStmtID = sStatement->getStmtID();
        IDU_SESSION_CLR_CANCELED(*sSession->getEventFlag());

        /* BUG-38472 Query timeout applies to one statement. */
        IDU_SESSION_CLR_TIMEOUT( *sSession->getEventFlag() );
    }
    else
    {
        IDE_TEST(findStatement(&sStatement,
                               sSession,
                               &sStmtID,
                               sThread) != IDE_SUCCESS);

        IDE_TEST(sStatement->clearPlanTreeText() != IDE_SUCCESS);

        IDE_TEST_RAISE(sStatement->getStmtState() >= MMC_STMT_STATE_EXECUTED,
                       InvalidStatementState);

        IDE_TEST(qci::clearStatement(sStatement->getQciStmt(),
                                     sStatement->getSmiStmt(),
                                     QCI_STMT_STATE_INITIALIZED) != IDE_SUCCESS);

        sStatement->clearGCTxStmtInfo();
        sStatement->setStmtState(MMC_STMT_STATE_ALLOC);
    }

    IDE_TEST_RAISE(sStatementStringLen == 0, NullQuery);

    IDU_FIT_POINT( "mmtServiceThread::prepareByCIDProtocol::malloc::Query" );

    IDE_TEST(iduMemMgr::malloc(IDU_MEM_MMC,
                               sStatementStringLen + 1,
                               (void **)&sQuery,
                               IDU_MEM_IMMEDIATE) != IDE_SUCCESS);

    if (cmiGetLinkImpl(aProtocolContext) == CMI_LINK_IMPL_IPCDA)
    {
        IDE_TEST_RAISE( cmiSplitReadIPCDA( aProtocolContext,
                                           sRowSize,
                                           (UChar**)&sQuery,
                                           (UChar*)sQuery)
                        != IDE_SUCCESS, cm_error );
    }
    else
    {
        IDE_TEST_RAISE( cmiSplitRead( aProtocolContext,
                                      sRowSize,
                                      (UChar*)sQuery,
                                      NULL )
                        != IDE_SUCCESS, cm_error );
    }
    sRowSize = 0;

    sQuery[sStatementStringLen] = '\0';

    /* PROJ-1381 FAC : Mode 설정 */
    if ( (sMode & CMP_DB_PREPARE_MODE_EXEC_MASK) == CMP_DB_PREPARE_MODE_EXEC_PREPARE )
    {
        sStatement->setStmtExecMode(MMC_STMT_EXEC_PREPARED);
    }
    else
    {
        sStatement->setStmtExecMode(MMC_STMT_EXEC_DIRECT);
    }

    if ( ((sMode & CMP_DB_PREPARE_MODE_HOLD_MASK) == CMP_DB_PREPARE_MODE_HOLD_ON)
     &&  (sSession->getCommitMode() == MMC_COMMITMODE_NONAUTOCOMMIT) )
    {
        /* BUG-49075 샤딩에서는 FAC가 지원되지 않는다. */
        IDE_TEST_RAISE(sdi::isShardEnable() == ID_TRUE, NotSupportCursorHold);
        sStatement->setCursorHold(MMC_STMT_CURSOR_HOLD_ON);
    }
    else
    {
        sStatement->setCursorHold(MMC_STMT_CURSOR_HOLD_OFF);
    }

    if ( (sMode & CMP_DB_PREPARE_MODE_KEYSET_MASK) == CMP_DB_PREPARE_MODE_KEYSET_ON )
    {
        /* BUG-47050 샤딩에서는 KEYSET이 지원되지 않는다. */
        IDE_TEST_RAISE(sdi::isShardEnable() == ID_TRUE, NotSupportKeysetOn);
        sStatement->setKeysetMode(MMC_STMT_KEYSETMODE_ON);
    }
    else
    {
        sStatement->setKeysetMode(MMC_STMT_KEYSETMODE_OFF);
    }

    IDE_TEST(sStatement->prepare(sQuery, sStatementStringLen) != IDE_SUCCESS);

    IDE_TEST( answerPrepareResult(aProtocolContext, sStatement) != IDE_SUCCESS );

    if ( sSession->isNeedRebuildNoti() == ID_TRUE )
    {
        IDE_TEST( sendShardRebuildNoti( aProtocolContext )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(StmtNotFoundException)
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_STATEMENT_NOT_FOUND));
    }
    IDE_EXCEPTION(InvalidStatementState)
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_INVALID_STATEMENT_STATE_ERROR));
    }
    IDE_EXCEPTION(NullQuery)
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_INSUFFICIENT_QUERY_ERROR));
    }
    IDE_EXCEPTION(cm_error)
    {
        return IDE_FAILURE;
    }
    IDE_EXCEPTION(NotSupportCursorHold)
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDA_NOT_SUPPORTED_SQLTEXT_FOR_SHARD,
                                  "CURSOR HOLD ON restrictions",
                                  "" ) );
    }
    IDE_EXCEPTION(NotSupportKeysetOn)
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDA_NOT_SUPPORTED_SQLTEXT_FOR_SHARD,
                                  "KEYSET ON restrictions",
                                  "" ) );
    }
    IDE_EXCEPTION_END;

    if( sRowSize != 0 )
    {
        IDE_TEST_RAISE( cmiSplitSkipRead( aProtocolContext,
                                          sRowSize,
                                          NULL )
                        != IDE_SUCCESS, cm_error );
    }
    sRet = sThread->answerErrorResult(aProtocolContext,
                                      aProtocol->mOpID,
                                      0);
    if (sRet == IDE_SUCCESS)
    {
        sThread->mErrorFlag = ID_TRUE;
    }

    if(sStatement != NULL)
    {
        sThread->setStatement(NULL);
        IDE_ASSERT( sStatement->closeCursor(ID_TRUE) == IDE_SUCCESS );
        IDE_ASSERT( mmcStatementManager::freeStatement(sStatement) == IDE_SUCCESS );
    }
    return sRet;
}

IDE_RC mmtServiceThread::planGetProtocol(cmiProtocolContext *aProtocolContext,
                                         cmiProtocol        *,
                                         void               *aSessionOwner,
                                         void               *aUserContext)
{
    mmcTask          *sTask   = (mmcTask *)aSessionOwner;
    mmtServiceThread *sThread = (mmtServiceThread *)aUserContext;
    mmcSession       *sSession;
    mmcStatement     *sStatement;

    UInt              sStatementID;

    /* PROJ-2160 CM 타입제거
       모두 읽은 다음에 프로토콜을 처리해야 한다. */
    CMI_RD4(aProtocolContext, &sStatementID);

    IDE_CLEAR();

    IDE_TEST(findSession(sTask, &sSession, sThread) != IDE_SUCCESS);

    IDE_TEST(checkSessionState(sSession, MMC_SESSION_STATE_SERVICE) != IDE_SUCCESS);

    /* BUG-32902 Explain Plan OFF 일 때는 에러를 내는게 낫다. */
    IDE_TEST_RAISE(sSession->getExplainPlan() == QCI_EXPLAIN_PLAN_OFF,
                   InvalidSessionStateException);

    IDE_TEST(findStatement(&sStatement,
                           sSession,
                           &sStatementID,
                           sThread) != IDE_SUCCESS);

    if (sStatement->isPlanPrinted() != ID_TRUE)
    {
        IDE_TEST(checkStatementState(sStatement, MMC_STMT_STATE_PREPARED) != IDE_SUCCESS);

        IDE_TEST(sStatement->makePlanTreeText(ID_TRUE) != IDE_SUCCESS);
    }

    return answerPlanGetResult(aProtocolContext,
                               sStatement->getStmtID(),
                               sStatement->getPlanString());

    IDE_EXCEPTION(InvalidSessionStateException);
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_INVALID_SESSION_STATE));
    }
    IDE_EXCEPTION_END;

    return sThread->answerErrorResult(aProtocolContext,
                                      CMI_PROTOCOL_OPERATION(DB, PlanGet),
                                      0);
}

IDE_RC mmtServiceThread::freeProtocol(cmiProtocolContext *aProtocolContext,
                                      cmiProtocol        *,
                                      void               *aSessionOwner,
                                      void               *aUserContext)
{
    mmcTask          *sTask   = (mmcTask *)aSessionOwner;
    mmtServiceThread *sThread = (mmtServiceThread *)aUserContext;
    mmcSession       *sSession;
    mmcStatement     *sStatement;

    UInt              sStatementID;
    UShort            sResultSetID;
    UChar             sMode;

    /* PROJ-2160 CM 타입제거
       모두 읽은 다음에 프로토콜을 처리해야 한다. */
    CMI_RD4(aProtocolContext, &sStatementID);
    CMI_RD2(aProtocolContext, &sResultSetID);
    CMI_RD1(aProtocolContext, sMode);

    IDE_CLEAR();

    IDE_TEST(findSession(sTask, &sSession, sThread) != IDE_SUCCESS);

    IDE_TEST(checkSessionState(sSession, MMC_SESSION_STATE_SERVICE) != IDE_SUCCESS);

    IDE_TEST(findStatement(&sStatement,
                           sSession,
                           &sStatementID,
                           sThread) != IDE_SUCCESS);

    /* BUG-46092 */
    if ( sdi::isShardEnable() == ID_TRUE )
    {
        switch (sMode)
        {
            case CMP_DB_FREE_CLOSE:
            case CMP_DB_FREE_DROP:
                sStatement->freeAllRemoteStatement( sMode );
                break;

            default:
                break;
        }
    }

    switch (sMode)
    {
        case CMP_DB_FREE_CLOSE:
            /* PROJ-2616 simple query select일 경우 cursor close할 필요가 없다
             * qci::fastExecute()에서 close가 이미 수행됨 */
            if(sStatement->isSimpleQuerySelectExecuted() != ID_TRUE )
            {
                if (sStatement->getStmtState() >= MMC_STMT_STATE_EXECUTED  &&
                    sSession->getExplainPlan() == QCI_EXPLAIN_PLAN_ON )
                {
                    IDE_TEST(sStatement->makePlanTreeText(ID_FALSE) != IDE_SUCCESS);
                }
                IDE_TEST(sStatement->closeCursor(ID_TRUE) != IDE_SUCCESS);
            }
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

    return sThread->answerErrorResult(aProtocolContext,
                                      CMI_PROTOCOL_OPERATION(DB, Free),
                                      0,
                                      sSession);
}
