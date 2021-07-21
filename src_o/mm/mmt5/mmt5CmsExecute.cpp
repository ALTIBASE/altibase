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
#include <mmcConv.h>
#include <mmcSession.h>
#include <mmcStatement.h>
#include <mmcStatementManager.h>
#include <mmcTask.h>
#include <mmtServiceThread.h>
#include <mmqManager.h>
#include <mmdXa.h>
#include <mmtAuditManager.h>

typedef struct mmtCmsExecuteContext
{
    cmiProtocolContext *mProtocolContext;
    mmcStatement       *mStatement;
    SLong               mAffectedRowCount;
    SLong               mFetchedRowCount;
    idBool              mSuspended;
    UChar              *mCollectionData;
    UInt                mCursor; // bug-27621: pointer to UInt
} mmtCmsExecuteContext;


static IDE_RC answerExecuteResult(mmtCmsExecuteContext *aExecuteContext)
{
    cmiProtocol            sProtocol;
    cmpArgDBExecuteResultA5 *sArg;

    CMI_PROTOCOL_INITIALIZE(sProtocol, sArg, DB, ExecuteResult);

    sArg->mStatementID      = aExecuteContext->mStatement->getStmtID();
    sArg->mRowNumber        = aExecuteContext->mStatement->getRowNumber();

    sArg->mResultSetCount   = aExecuteContext->mStatement->getResultSetCount();
    sArg->mAffectedRowCount = (ULong)aExecuteContext->mAffectedRowCount;

    IDE_TEST(cmiWriteProtocol(aExecuteContext->mProtocolContext,
                              &sProtocol) != IDE_SUCCESS);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

static IDE_RC getBindParamCallback(idvSQL       * /*aStatistics*/,
                                   qciBindParam *aBindParam,
                                   void         *aData,
                                   void         *aExecuteContext)
{
    mmtCmsExecuteContext     *sExecuteContext = (mmtCmsExecuteContext *)aExecuteContext;
    mmcSession               *sSession        = sExecuteContext->mStatement->getSession();
    mmcStatement             *sStatement      = sExecuteContext->mStatement;
    cmiProtocol               sProtocol;
    cmpArgDBParamDataOutA5 *sArg;

    CMI_PROTOCOL_INITIALIZE(sProtocol, sArg, DB, ParamDataOut);

    IDE_TEST_RAISE(mmcConv::convertFromMT(sExecuteContext->mProtocolContext,
                                          &sArg->mData,
                                          aData,
                                          aBindParam->type,
                                          sSession)
                   != IDE_SUCCESS, ConversionFail);

    sArg->mStatementID = sExecuteContext->mStatement->getStmtID();
    sArg->mRowNumber   = sExecuteContext->mStatement->getRowNumber();
    sArg->mParamNumber = aBindParam->id + 1;

    // fix BUG-24881 Output Parameter도 Profiling 해야 합니다.
    if ((idvProfile::getProfFlag() & IDV_PROF_TYPE_BIND_FLAG) == IDV_PROF_TYPE_BIND_FLAG)
    {
        // bug-25312: prepare 이후에 autocommit을 off에서 on으로 변경하고
        // bind 하면 stmt->mTrans 가 null이어서 segv.
        // 변경: stmt->mTrans를 구하여 null이면 TransID로 0을 넘기도로 수정
        mmcTransObj *sTrans = sSession->getTransPtr(sStatement);
        smTID sTransID = (sTrans != NULL) ? mmcTrans::getTransID(sTrans) : 0;

        idvProfile::writeBindA5( (void *)&sArg->mData,
                               sStatement->getSessionID(),
                               sStatement->getStmtID(),
                               sTransID,
                               profWriteBindCallbackA5 );
    }

    IDE_TEST(cmiWriteProtocol(sExecuteContext->mProtocolContext, &sProtocol) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION(ConversionFail);
    {
        IDE_ASSERT(cmiFinalizeProtocol(&sProtocol) == IDE_SUCCESS);
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC getBindParamListCallbackA5(idvSQL       * /*aStatistics*/,
                                         qciBindParam *aBindParam,
                                         void         *aData,
                                         void         *aExecuteContext)
{
    mmtCmsExecuteContext *sExecuteContext = (mmtCmsExecuteContext *)aExecuteContext;
    mmcSession           *sSession        = sExecuteContext->mStatement->getSession();
    mmcStatement         *sStatement      = sExecuteContext->mStatement;
    cmtAny                sAny;

    IDE_TEST( cmtAnyInitialize( &sAny ) != IDE_SUCCESS );

    IDE_TEST(mmcConv::buildAnyFromMT(&sAny,
                                     aData,
                                     aBindParam->type,
                                     sSession)
             != IDE_SUCCESS);

    // fix BUG-24881 Output Parameter도 Profiling 해야 합니다.
    if ((idvProfile::getProfFlag() & IDV_PROF_TYPE_BIND_FLAG) == IDV_PROF_TYPE_BIND_FLAG)
    {
        // bug-25312: prepare 이후에 autocommit을 off에서 on으로 변경하고
        // bind 하면 stmt->mTrans 가 null이어서 segv.
        // 변경: stmt->mTrans를 구하여 null이면 TransID로 0을 넘기도로 수정
        mmcTransObj *sTrans = sSession->getTransPtr(sStatement);
        smTID sTransID = (sTrans != NULL) ? mmcTrans::getTransID(sTrans) : 0;

        idvProfile::writeBindA5( (void *)&sAny,
                               sStatement->getSessionID(),
                               sStatement->getStmtID(),
                               sTransID,
                               profWriteBindCallbackA5 );
    }

    // bug-27621: mCursor: UInt pointer -> UInt
    cmtCollectionWriteAny( sExecuteContext->mCollectionData,
                           &(sExecuteContext->mCursor),
                           &sAny );

    /*
     * CASE-13162
     * fetch해온 row를 collection buffer의 cursor위치부터 저장하고
     * cursor 위치를 증가시킴. 그러므로 cursor의 위치가 절대로
     * collection buffer의 크기를 넘을 수 없음.
     * 만약 cursor의 위치가 collection buffer의 크기를 넘어간다면
     * page가 깨지는 등의 abnormal한 상황임.
     */
    IDE_ASSERT( sExecuteContext->mCursor < sSession->getChunkSize() );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC answerParamDataOutListA5(mmtCmsExecuteContext *aExecuteContext)
{
    cmiProtocol                   sProtocol;
    cmpArgDBParamDataOutListA5 *sArg;

    CMI_PROTOCOL_INITIALIZE(sProtocol, sArg, DB, ParamDataOutList);

    sArg->mStatementID = aExecuteContext->mStatement->getStmtID();
    sArg->mRowNumber   = aExecuteContext->mStatement->getRowNumber();

    if( cmiCheckInVariable( aExecuteContext->mProtocolContext,
                            aExecuteContext->mCursor) == ID_TRUE )
    {
        cmtCollectionWriteInVariable( &sArg->mListData,
                                      aExecuteContext->mCollectionData,
                                      aExecuteContext->mCursor);
    }
    else
    {
        cmtCollectionWriteVariable( &sArg->mListData,
                                    aExecuteContext->mCollectionData,
                                    aExecuteContext->mCursor);
    }

    IDE_TEST(cmiWriteProtocol(aExecuteContext->mProtocolContext, &sProtocol) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC sendBindOutA5(mmcSession           *aSession,
                          mmtCmsExecuteContext *aExecuteContext)
{
    qciStatement *sQciStmt;
    qciBindParam  sBindParam;
    UShort        sParamCount;
    UInt          sOutParamSize;
    UInt          sOutParamCount;
    UShort        sParamIndex;
    UInt          sAllocSize;

    sQciStmt    = aExecuteContext->mStatement->getQciStmt();
    sParamCount = qci::getParameterCount(sQciStmt);

    IDE_TEST( qci::getOutBindParamSize( sQciStmt,
                                        &sOutParamSize,
                                        &sOutParamCount )
              != IDE_SUCCESS );
    
    IDE_TEST_CONT( sOutParamCount == 0, SKIP_SEND_BIND_OUT );
    
    if( aSession->getHasClientListChannel() == ID_TRUE )
    {
                    
        // out result에 필요한 메모리 공간을 estimation한다.
        // sOutParamSize는 QP에서의 공간을 의미하기 때문에,
        // 통신상 필요한 공간을 부가적으로 추가해야 한다.
        sAllocSize = sOutParamSize + (sOutParamCount * cmiGetMaxInTypeHeaderSize());
        IDE_TEST( aSession->allocChunk(IDL_MAX(sAllocSize, (MMC_DEFAULT_COLLECTION_BUFFER_SIZE)))
                        != IDE_SUCCESS );

        aExecuteContext->mCollectionData = aSession->getChunk();
        // bug-27621: mCursor: UInt pointer -> UInt
        // 지역변수 sCursor 주소지정 제거.
        aExecuteContext->mCursor = 0;
    }
    
    for (sParamIndex = 0; sParamIndex < sParamCount; sParamIndex++)
    {
        sBindParam.id = sParamIndex;

        IDE_TEST(qci::getBindParamInfo(sQciStmt, &sBindParam) != IDE_SUCCESS);

        if ((sBindParam.inoutType == CMP_DB_PARAM_INPUT_OUTPUT) ||
            (sBindParam.inoutType == CMP_DB_PARAM_OUTPUT))
        {
            if( aSession->getHasClientListChannel() == ID_TRUE )
            {
                IDE_TEST(qci::getBindParamData(sQciStmt,
                                               sParamIndex,
                                               getBindParamListCallbackA5,
                                               aExecuteContext)
                         != IDE_SUCCESS);
            }
            else
            {
                IDE_TEST(qci::getBindParamData(sQciStmt,
                                               sParamIndex,
                                               getBindParamCallback,
                                               aExecuteContext)
                         != IDE_SUCCESS);
            }
        }
    }

    if( aSession->getHasClientListChannel() == ID_TRUE )
    {
        IDE_TEST( answerParamDataOutListA5(aExecuteContext)
                  != IDE_SUCCESS );

        // bug-27571: klocwork warnings
        // 혹시나 해서 초기화 시켜준다.
        aExecuteContext->mCollectionData = NULL;
        aExecuteContext->mCursor = 0;
    }

    IDE_EXCEPTION_CONT( SKIP_SEND_BIND_OUT );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

static IDE_RC doRetry(mmcStatement *aStmt)
{
    IDE_TEST(qci::retry(aStmt->getQciStmt(), aStmt->getSmiStmt()) != IDE_SUCCESS);

    IDE_TEST(aStmt->endStmt(MMC_EXECUTION_FLAG_RETRY) != IDE_SUCCESS);

    IDE_TEST(aStmt->beginStmt() != IDE_SUCCESS);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

static IDE_RC doRebuild(mmcStatement *aStmt)
{
    IDE_TEST(qci::closeCursor(aStmt->getQciStmt(), aStmt->getSmiStmt()) != IDE_SUCCESS);

    IDE_TEST(aStmt->endStmt(MMC_EXECUTION_FLAG_REBUILD) != IDE_SUCCESS);

    // BUG_12177
    aStmt->setCursorFlag(SMI_STATEMENT_ALL_CURSOR);

    IDE_TEST(aStmt->beginStmt() != IDE_SUCCESS);

    IDE_TEST(aStmt->rebuild() != IDE_SUCCESS);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

// fix BUG-30891
static IDE_RC doEnd(mmtCmsExecuteContext *aExecuteContext, mmcExecutionFlag aExecutionFlag)
{
    mmcStatement *sStmt = aExecuteContext->mStatement;

    /* PROJ-2223 Altibase Auditing */
    mmtAuditManager::auditByAccess( sStmt, aExecutionFlag );
    
    // fix BUG-30990
    // QUEUE가 EMPTY일 경우에만 BIND_DATA 상태로 설정한다.
    switch(aExecutionFlag)
    {
        case MMC_EXECUTION_FLAG_SUCCESS :
            IDE_TEST(sStmt->clearStmt(MMC_STMT_BIND_NONE) != IDE_SUCCESS);
            break;
        case MMC_EXECUTION_FLAG_QUEUE_EMPTY :
            IDE_TEST(sStmt->clearStmt(MMC_STMT_BIND_DATA) != IDE_SUCCESS);
            break;
        default:
            IDE_TEST(sStmt->clearStmt(MMC_STMT_BIND_INFO) != IDE_SUCCESS);
            break;

    }

    IDE_TEST(sStmt->endStmt(aExecutionFlag) != IDE_SUCCESS);

    sStmt->setExecuteFlag(ID_FALSE);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC doExecuteA5(mmtCmsExecuteContext *aExecuteContext)
{
    mmcStatement   * sStatement = aExecuteContext->mStatement;
    mmcSession     * sSession   = sStatement->getSession();
    qciStatement   * sQciStmt   = sStatement->getQciStmt();
    smiStatement   * sSmiStmt   = NULL;
    idBool           sRetry;
    idBool           sRecordExist;
    UShort           sResultSetCount;
    mmcResultSet   * sResultSet = NULL;
    mmcExecutionFlag sExecutionFlag = MMC_EXECUTION_FLAG_FAILURE;
    idBool           sBeginFetchTime = ID_FALSE;
    UInt             sOldResultSetHWM = 0;

    // PROJ-2163
    IDE_TEST(sStatement->reprepare() != IDE_SUCCESS);

    IDE_TEST(sStatement->beginStmt() != IDE_SUCCESS);

    // PROJ-2446 BUG-40481
    // mmcStatement.mSmiStmtPtr is changed at mmcStatement::beginSP()
    sSmiStmt = sStatement->getSmiStmt();

    do
    {
        do
        {
            sRetry = ID_FALSE;

            if (sStatement->execute(&aExecuteContext->mAffectedRowCount,
                                    &aExecuteContext->mFetchedRowCount) != IDE_SUCCESS)
            {
                switch (ideGetErrorCode() & E_ACTION_MASK)
                {
                    case E_ACTION_IGNORE:
                        IDE_RAISE(ExecuteFailIgnore);
                        break;

                    case E_ACTION_RETRY:
                        IDE_TEST(doRetry(sStatement) != IDE_SUCCESS);

                        sRetry = ID_TRUE;
                        break;

                    case E_ACTION_REBUILD:
                        IDE_TEST(doRebuild(sStatement) != IDE_SUCCESS);

                        sRetry = ID_TRUE;
                        break;

                    case E_ACTION_ABORT:
                        IDE_RAISE(ExecuteFailAbort);
                        break;

                    case E_ACTION_FATAL:
                        IDE_CALLBACK_FATAL("fatal error returned from mmcStatement::execute()");
                        break;

                    default:
                        IDE_CALLBACK_FATAL("invalid error action returned from mmcStatement::execute()");
                        break;
                }
            }

        } while (sRetry == ID_TRUE);

        sStatement->setStmtState(MMC_STMT_STATE_EXECUTED);

        IDE_TEST( sendBindOutA5( sSession,
                               aExecuteContext )
                  != IDE_SUCCESS );

        // fix BUG-30913
        // execute의 성공 시점은 execute 및 sendBindOutA5 이후이다.
        sExecutionFlag = MMC_EXECUTION_FLAG_SUCCESS;

        sOldResultSetHWM = sStatement->getResultSetHWM();
        sResultSetCount = qci::getResultSetCount(sStatement->getQciStmt());
        sStatement->setResultSetCount(sResultSetCount);
        sStatement->setEnableResultSetCount(sResultSetCount);
        //fix BUG-27198 Code-Sonar  return value ignore하여, 결국 mResultSet가 
        // null pointer일때 이를 de-reference를 할수 있습니다.
        IDE_TEST_RAISE(sStatement->initializeResultSet(sResultSetCount) != IDE_SUCCESS,
                       RestoreResultSetValues);


        if (sResultSetCount > 0)
        {
            sStatement->setFetchFlag(MMC_FETCH_FLAG_PROCEED);

            sResultSet = sStatement->getResultSet( MMC_RESULTSET_FIRST );
            // bug-26977: codesonar: resultset null ref
            // null이 가능한지는 모르겠지만, 방어코드임.
            IDE_TEST(sResultSet == NULL);

            if( sResultSet->mInterResultSet == ID_TRUE )
            {
                sRecordExist = sResultSet->mRecordExist;

                if (sRecordExist == ID_TRUE)
                {
                    // fix BUG-31195
                    sStatement->setFetchStartTime(mmtSessionManager::getBaseTime());
                    /* BUG-19456 */
                    sStatement->setFetchEndTime(0);

                    IDV_SQL_OPTIME_BEGIN( sStatement->getStatistics(),
                                          IDV_OPTM_INDEX_QUERY_FETCH);

                    sBeginFetchTime = ID_TRUE;

                    IDE_TEST(sStatement->beginFetch(MMC_RESULTSET_FIRST) != IDE_SUCCESS);
                }
                else
                {
                    sStatement->setFetchFlag(MMC_FETCH_FLAG_CLOSE);
                }
            }
            else
            {
                // fix BUG-31195
                // qci::moveNextRecord() 시간을 Fetch Time에 추가
                sStatement->setFetchStartTime(mmtSessionManager::getBaseTime());
                /* BUG-19456 */
                sStatement->setFetchEndTime(0);

                IDV_SQL_OPTIME_BEGIN( sStatement->getStatistics(),
                                      IDV_OPTM_INDEX_QUERY_FETCH);

                sBeginFetchTime = ID_TRUE;

                if (qci::moveNextRecord(sQciStmt,
                                        sStatement->getSmiStmt(),
                                        &sRecordExist) != IDE_SUCCESS)
                {
                    // fix BUG-31195
                    sBeginFetchTime = ID_FALSE;

                    sStatement->setFetchStartTime(0);
                    /* BUG-19456 */
                    sStatement->setFetchEndTime(0);

                    IDV_SQL_OPTIME_END( sStatement->getStatistics(),
                                        IDV_OPTM_INDEX_QUERY_FETCH );

                    switch (ideGetErrorCode() & E_ACTION_MASK)
                    {
                        case E_ACTION_IGNORE:
                            IDE_RAISE(MoveNextRecordFailIgnore);
                            break;

                        case E_ACTION_RETRY:
                            IDE_TEST(doRetry(sStatement) != IDE_SUCCESS);

                            sRetry = ID_TRUE;
                            break;

                        case E_ACTION_REBUILD:
                            IDE_TEST(doRebuild(sStatement) != IDE_SUCCESS);

                            sRetry = ID_TRUE;
                            break;

                        case E_ACTION_ABORT:
                            IDE_RAISE(MoveNextRecordFailAbort);
                            break;

                        case E_ACTION_FATAL:
                            IDE_CALLBACK_FATAL("fatal error returned from qci::moveNextRecord()");
                            break;

                        default:
                            IDE_CALLBACK_FATAL("invalid error action returned from qci::moveNextRecord()");
                            break;
                    }
                }
                else
                {
                    if (sStatement->getStmtType() == QCI_STMT_DEQUEUE)
                    {
                        if (sRecordExist == ID_TRUE)
                        {
                            sSession->setExecutingStatement(NULL);
                            sSession->endQueueWait();

                            sStatement->beginFetch(MMC_RESULTSET_FIRST);
                        }
                        else
                        {
                            // fix BUG-31195
                            sBeginFetchTime = ID_FALSE;

                            sStatement->setFetchStartTime(0);
                            /* BUG-19456 */
                            sStatement->setFetchEndTime(0);

                            IDV_SQL_OPTIME_END( sStatement->getStatistics(),
                                                IDV_OPTM_INDEX_QUERY_FETCH );

                            if (sSession->isQueueTimedOut() != ID_TRUE)
                            {
                                sExecutionFlag = MMC_EXECUTION_FLAG_QUEUE_EMPTY;
                                //fix BUG-21361 queue drop의 동시성 문제로 서버 비정상 종료할수 있음.
                                sSession->beginQueueWait();
                                //fix BUG-19321
                                IDE_TEST(doEnd(aExecuteContext, sExecutionFlag) != IDE_SUCCESS);
                                sSession->setExecutingStatement(sStatement);
                                aExecuteContext->mSuspended = ID_TRUE;
                            }
                            else
                            {
                                sSession->setExecutingStatement(NULL);
                                sSession->endQueueWait();

                                IDE_TEST(doEnd(aExecuteContext, sExecutionFlag) != IDE_SUCCESS);
                            }
                        }
                    }
                    else
                    {
                        if (sRecordExist == ID_TRUE)
                        {
                            IDE_TEST(sStatement->beginFetch(MMC_RESULTSET_FIRST) != IDE_SUCCESS);

                            // PROJ-2256
                            sResultSet->mBaseRow.mIsFirstRow = ID_TRUE;
                        }
                        else
                        {
                            // fix BUG-31195
                            sBeginFetchTime = ID_FALSE;

                            sStatement->setFetchStartTime(0);
                            /* BUG-19456 */
                            sStatement->setFetchEndTime(0);

                            IDV_SQL_OPTIME_END( sStatement->getStatistics(),
                                                IDV_OPTM_INDEX_QUERY_FETCH );

                            if (sResultSetCount == 1)
                            {
                                sStatement->setFetchFlag(MMC_FETCH_FLAG_CLOSE);

                                mmcStatement::makePlanTreeBeforeCloseCursor( sStatement,

                                                                             sStatement );

                                IDE_TEST(doEnd(aExecuteContext, sExecutionFlag) != IDE_SUCCESS);
                            }
                        }
                    }
                }
            }
        }
        else
        {
            mmcStatement::makePlanTreeBeforeCloseCursor( sStatement,
                                                         sStatement );

            if (qci::closeCursor(sQciStmt, sSmiStmt) != IDE_SUCCESS)
            {
                IDE_TEST(ideIsRetry() != IDE_SUCCESS);

                IDE_TEST(doRetry(sStatement) != IDE_SUCCESS);

                sRetry = ID_TRUE;
            }
            else
            {
                IDE_TEST(doEnd(aExecuteContext, sExecutionFlag) != IDE_SUCCESS);
            }
        }

    } while (sRetry == ID_TRUE);

    return IDE_SUCCESS;

    IDE_EXCEPTION(ExecuteFailIgnore);
    {
        SChar sMsg[64];

        idlOS::snprintf(sMsg, ID_SIZEOF(sMsg), "mmcStatement::execute code=0x%x", ideGetErrorCode());

        IDE_SET(ideSetErrorCode(mmERR_ABORT_INTERNAL_SERVER_ERROR_ARG, sMsg));
    }
    IDE_EXCEPTION(ExecuteFailAbort);
    {
        // No Action
    }
    IDE_EXCEPTION(MoveNextRecordFailIgnore);
    {
        SChar sMsg[64];

        idlOS::snprintf(sMsg, ID_SIZEOF(sMsg), "qci::moveNextRecord code=0x%x", ideGetErrorCode());

        IDE_SET(ideSetErrorCode(mmERR_ABORT_INTERNAL_SERVER_ERROR_ARG, sMsg));
    }
    IDE_EXCEPTION(MoveNextRecordFailAbort);
    {
        mmcStatement::makePlanTreeBeforeCloseCursor( sStatement,
                                                     sStatement );

        qci::closeCursor(sQciStmt, sSmiStmt);
    }
    IDE_EXCEPTION( RestoreResultSetValues );
    {
        sStatement->setResultSetCount(sOldResultSetHWM);
        sStatement->setEnableResultSetCount(sOldResultSetHWM);
    }
    IDE_EXCEPTION_END;
    {
        IDE_PUSH();

        // fix BUG-31195
        if (sBeginFetchTime == ID_TRUE)
        {
            sStatement->setFetchStartTime(0);
            /* BUG-19456 */
            sStatement->setFetchEndTime(0);

            IDV_SQL_OPTIME_END( sStatement->getStatistics(),
                                IDV_OPTM_INDEX_QUERY_FETCH );

            sBeginFetchTime = ID_FALSE;
        }

        sSession->setExecutingStatement(NULL);
        sSession->endQueueWait();

        if (sStatement->isStmtBegin() == ID_TRUE)
        {
            /* PROJ-2337 Homogeneous database link
             *  Dequeue 후에 예외 처리를 수행하는 경우, Queue에서 얻었던 데이터를 반납해야 한다.
             *  TODO Dequeue 외에 다른 문제는 없는지 확인해야 한다.
             */
            if ( ( sStatement->getStmtType() == QCI_STMT_DEQUEUE ) &&
                 ( sExecutionFlag == MMC_EXECUTION_FLAG_SUCCESS ) )
            {
                sExecutionFlag = MMC_EXECUTION_FLAG_FAILURE;
            }
            else
            {
                /* Nothing to do */
            }

            /* PROJ-2223 Altibase Auditing */
            mmtAuditManager::auditByAccess( sStatement, sExecutionFlag );
            
            /*
             * [BUG-24187] Rollback될 statement는 Internal CloseCurosr를
             * 수행할 필요가 없습니다.
             */
            sStatement->setSkipCursorClose();
            sStatement->clearStmt(MMC_STMT_BIND_NONE);

            /* BUG-47650 BUG-38585 IDE_ASSERT remove */
            (void)sStatement->endStmt( sExecutionFlag );
        }

        sStatement->setExecuteFlag(ID_FALSE);
        
        /* BUG-29078
         * XA_END 수신전에 XA_ROLLBACK을 먼저 수신한 경우에 XA_END를 Heuristic하게 처리한다.
         */
        if ( (sSession->isXaSession() == ID_TRUE) && 
             (sSession->getXaAssocState() == MMD_XA_ASSOC_STATE_ASSOCIATED) &&
             (sSession->getLastXid() != NULL) )
        {
            mmdXa::heuristicEnd(sSession, sSession->getLastXid());
        }

        IDE_POP();
    }

    return IDE_FAILURE;
}


IDE_RC mmtServiceThread::executeA5(cmiProtocolContext *aProtocolContext,
                                 mmcStatement       *aStatement,
                                 idBool              aDoAnswer,  
                                 idBool             *aSuspended,
                                 UInt               *aResultSetCount,
                                 ULong              *aAffectedRowCount)
{
    mmtCmsExecuteContext sExecuteContext;

    sExecuteContext.mProtocolContext  = aProtocolContext;
    sExecuteContext.mStatement        = aStatement;
    sExecuteContext.mAffectedRowCount = 0;
    sExecuteContext.mSuspended        = ID_FALSE;
    sExecuteContext.mCollectionData   = NULL;
    sExecuteContext.mCursor           = 0;

    IDE_TEST(doExecuteA5(&sExecuteContext) != IDE_SUCCESS);

    if( (aDoAnswer == ID_TRUE) && (sExecuteContext.mSuspended != ID_TRUE) )
    {
        IDE_TEST(answerExecuteResult(&sExecuteContext) != IDE_SUCCESS);
    }

    *aSuspended = sExecuteContext.mSuspended;
    
    if( aAffectedRowCount != NULL )
    {
        *aAffectedRowCount = sExecuteContext.mAffectedRowCount;
    }

    if( aResultSetCount != NULL )
    {
        *aResultSetCount = aStatement->getResultSetCount();
    }

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC mmtServiceThread::executeProtocolA5(cmiProtocolContext *aProtocolContext,
                                         cmiProtocol        *aProtocol,
                                         void               *aSessionOwner,
                                         void               *aUserContext)
{
    cmpArgDBExecuteA5  *sArg           = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, Execute);
    mmcTask          *sTask          = (mmcTask *)aSessionOwner;
    mmtServiceThread *sThread        = (mmtServiceThread *)aUserContext;
    mmcSession       *sSession;
    mmcStatement     *sStatement;
    idBool            sSuspended     = ID_FALSE;

    IDE_CLEAR();

    IDE_TEST(findSession(sTask, &sSession, sThread) != IDE_SUCCESS);

    IDE_TEST(checkSessionState(sSession, MMC_SESSION_STATE_SERVICE) != IDE_SUCCESS);

    IDE_TEST(findStatement(&sStatement,
                           sSession,
                           &sArg->mStatementID,
                           sThread) != IDE_SUCCESS);

    IDE_TEST_RAISE(sStatement->getStmtState() != MMC_STMT_STATE_PREPARED,
                   InvalidStatementState);

    // PROJ-1518
    IDE_TEST_RAISE(sThread->atomicCheck(sStatement, &(sArg->mOption))
                                        != IDE_SUCCESS, SkipExecute);

    switch (sArg->mOption)
    {
        case CMP_DB_EXECUTE_NORMAL_EXECUTE:
            sStatement->setArray(ID_FALSE);
            sStatement->setRowNumber(sArg->mRowNumber);

            IDE_TEST(sThread->executeA5(aProtocolContext,
                                      sStatement,
                                      ID_TRUE,
                                      &sSuspended,
                                      NULL, NULL) != IDE_SUCCESS);

            break;

        case CMP_DB_EXECUTE_ARRAY_EXECUTE:
            sStatement->setArray(ID_TRUE);
            sStatement->setRowNumber(sArg->mRowNumber);

            IDE_TEST(sThread->executeA5(aProtocolContext,
                                      sStatement,
                                      ID_TRUE,
                                      &sSuspended,
                                      NULL, NULL) != IDE_SUCCESS);

            break;

        case CMP_DB_EXECUTE_ARRAY_BEGIN:
            sStatement->setArray(ID_TRUE);
            sStatement->setRowNumber(0);
            break;

        case CMP_DB_EXECUTE_ARRAY_END:
            sStatement->setArray(ID_FALSE);
            sStatement->setRowNumber(0);
            break;
        // PROJ-1518
        case CMP_DB_EXECUTE_ATOMIC_EXECUTE:
            sStatement->setRowNumber(sArg->mRowNumber);
            // Rebuild Error 를 처리하기 위해서는 Bind가 끝난 시점에서
            // atomicBegin 을 호출해야 한다.
            if( sArg->mRowNumber == 1)
            {
                IDE_TEST_RAISE((sThread->atomicBegin(sStatement) != IDE_SUCCESS), SkipExecute)
            }

            IDE_TEST_RAISE(sThread->atomicExecuteA5(sStatement, aProtocolContext) != IDE_SUCCESS, SkipExecute)
            break;

        case CMP_DB_EXECUTE_ATOMIC_BEGIN:
            sThread->atomicInit(sStatement);
            break;
        case CMP_DB_EXECUTE_ATOMIC_END:
            // 이 시점에서 sArg->mRowNumber 는 1이다.
            sStatement->setRowNumber(sArg->mRowNumber);
            IDE_TEST(sThread->atomicEndA5(sStatement, aProtocolContext) != IDE_SUCCESS );
            break;
        default:
            IDE_RAISE(InvalidExecuteOption);
            break;
    }

    IDE_EXCEPTION_CONT(SkipExecute);

    return (sSuspended == ID_TRUE) ? IDE_CM_STOP : IDE_SUCCESS;

    IDE_EXCEPTION(InvalidStatementState);
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_INVALID_STATEMENT_STATE_ERROR));
    }
    IDE_EXCEPTION(InvalidExecuteOption);
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_INVALID_EXECUTE_OPTION));
    }
    IDE_EXCEPTION_END;

    return sThread->answerErrorResultA5(aProtocolContext,
                                      CMI_PROTOCOL_OPERATION(DB, Execute),
                                      sArg->mRowNumber);
}

// proj_2160 cm_type removal
// 함수 내부에서 sendBindOutA5를 호출하기 때문에
// A5용 함수를 별도로 둔다.
IDE_RC mmtServiceThread::atomicExecuteA5(mmcStatement * aStatement, cmiProtocolContext *aProtocolContext)
{
    qciStatement          *sQciStmt    = aStatement->getQciStmt();
    mmtCmsExecuteContext   sExecuteContext;

    sExecuteContext.mProtocolContext  = aProtocolContext;
    sExecuteContext.mStatement        = aStatement;
    sExecuteContext.mAffectedRowCount = 0;
    sExecuteContext.mSuspended        = ID_FALSE;
    sExecuteContext.mCollectionData   = NULL;
    sExecuteContext.mCursor           = 0;

    IDE_TEST(qci::atomicInsert(sQciStmt) != IDE_SUCCESS);

    IDE_TEST( sendBindOutA5( aStatement->getSession(), &sExecuteContext ) != IDE_SUCCESS );

    // PROJ-2163
    // insert 후에 바로 데이타를 bind 해야하므로 EXEC_PREPARED 상태로 변경해야 한다.
    IDE_TEST( qci::atomicSetPrepareState( sQciStmt ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END
    {
        aStatement->setAtomicExecSuccess(ID_FALSE);
        // BUG-21489
        aStatement->setAtomicLastErrorCode(ideGetErrorCode());
    }

    return IDE_FAILURE;
}

// proj_2160 cm_type removal
// 함수 내부에서 A5용 static answerExecuteResult를 호출하기 때문에
// A5용 함수를 별도로 둔다
IDE_RC mmtServiceThread::atomicEndA5(mmcStatement * aStatement, cmiProtocolContext *aProtocolContext)
{
    mmcSession            *sSession    = aStatement->getSession();
    idvSQL                *sStatistics = aStatement->getStatistics();
    qciStatement          *sQciStmt    = aStatement->getQciStmt();
    mmcAtomicInfo         *sAtomicInfo = aStatement->getAtomicInfo();
    mmtCmsExecuteContext   sExecuteContext;

    sExecuteContext.mProtocolContext  = aProtocolContext;
    sExecuteContext.mStatement        = aStatement;
    sExecuteContext.mAffectedRowCount = 0;
    sExecuteContext.mSuspended        = ID_FALSE;
    sExecuteContext.mCollectionData   = NULL;
    sExecuteContext.mCursor           = 0;

    // List 형태의 프로토콜이 올때 1번째 row의 바인드가 실패시 Begin이 안할수 있다.
    IDE_TEST_RAISE( aStatement->isStmtBegin() != ID_TRUE, AtomicExecuteFail);

    IDE_TEST_RAISE( aStatement->getAtomicExecSuccess() != ID_TRUE, AtomicExecuteFail);

    sAtomicInfo->mIsCursorOpen = ID_FALSE;

    if ( qci::atomicEnd(sQciStmt) != IDE_SUCCESS)
    {
        switch (ideGetErrorCode() & E_ACTION_MASK)
        {
            case E_ACTION_IGNORE:
                IDE_RAISE(AtomicExecuteFail);
                break;

            // fix BUG-30449
            // RETRY 에러를 클라이언트에게 ABORT로 전달해
            // ATOMIC ARRAY INSERT 실패를 알려준다.
            case E_ACTION_RETRY:
            case E_ACTION_REBUILD:
                IDE_RAISE(ExecuteRetry);
                break;

            case E_ACTION_ABORT:
                IDE_RAISE(ExecuteFailAbort);
                break;

            case E_ACTION_FATAL:
                IDE_CALLBACK_FATAL("fatal error returned from mmcStatement::atomicEnd()");
                break;

            default:
                IDE_CALLBACK_FATAL("invalid error action returned from mmcStatement::atomicEnd()");
                break;
        }
    }

    qci::getRowCount(sQciStmt, &sExecuteContext.mAffectedRowCount, &sExecuteContext.mFetchedRowCount);

    IDV_SQL_OPTIME_END( sStatistics, IDV_OPTM_INDEX_QUERY_EXECUTE );

    IDV_SESS_ADD_DIRECT(sSession->getStatistics(),
                        IDV_STAT_INDEX_EXECUTE_SUCCESS_COUNT, 1);

    IDV_SQL_ADD_DIRECT(sStatistics, mExecuteSuccessCount, 1);
    IDV_SQL_ADD_DIRECT(sStatistics, mProcessRow, (ULong)(sExecuteContext.mAffectedRowCount));

    aStatement->setStmtState(MMC_STMT_STATE_EXECUTED);

    if (qci::closeCursor(aStatement->getQciStmt(), aStatement->getSmiStmt()) == IDE_SUCCESS)
    {
        IDE_TEST(doEnd(&sExecuteContext, MMC_EXECUTION_FLAG_SUCCESS) != IDE_SUCCESS);
    }

    IDE_TEST(answerExecuteResult(&sExecuteContext) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION(AtomicExecuteFail);
    {
        // BUG-21489
        IDE_SET(ideSetErrorCodeAndMsg(sAtomicInfo->mAtomicLastErrorCode,
                                      sAtomicInfo->mAtomicErrorMsg));

    }
    IDE_EXCEPTION(ExecuteRetry);
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_ATOMIC_EXECUTE_ERROR));
    }
    IDE_EXCEPTION(ExecuteFailAbort);
    {
        // No Action
    }
    IDE_EXCEPTION_END
    {
        IDE_PUSH();

        IDV_SQL_OPTIME_END( sStatistics,
                            IDV_OPTM_INDEX_QUERY_EXECUTE );

        IDV_SESS_ADD_DIRECT(sSession->getStatistics(),
                            IDV_STAT_INDEX_EXECUTE_FAILURE_COUNT, 1);
        IDV_SQL_ADD_DIRECT(sStatistics, mExecuteFailureCount, 1);

        aStatement->setExecuteFlag(ID_FALSE);

        if (aStatement->isStmtBegin() == ID_TRUE)
        {
            /* PROJ-2223 Altibase Auditing */
            mmtAuditManager::auditByAccess( aStatement, MMC_EXECUTION_FLAG_FAILURE );
            
            /*
             * [BUG-24187] Rollback될 statement는 Internal CloseCurosr를
             * 수행할 필요가 없습니다.
             */
            aStatement->setSkipCursorClose();
            aStatement->clearStmt(MMC_STMT_BIND_NONE);

            IDE_ASSERT(aStatement->endStmt(MMC_EXECUTION_FLAG_FAILURE) == IDE_SUCCESS);
        }

        IDE_POP();
    }
    return IDE_FAILURE;
}

