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
#include <mmErrorCode.h>
#include <mmcConv.h>
#include <mmcConvFmMT.h>
#include <mmcStatement.h>
#include <mmcStatementManager.h>
#include <mmcTask.h>
#include <mmtServiceThread.h>
#include <mmtAuditManager.h>

typedef struct mmtCmsFetchContext
{
    cmiProtocolContext *mProtocolContext;
    mmcStatement       *mStatement;
    UShort              mResultSetID;
    UShort              mColumnFrom;
    UShort              mColumnTo;
    UShort              mRecordNumber;
    UChar              *mCollectionData;
    UInt                mCollectionSize;
    UInt                mCursor;         // bug-27621: pointer to UInt
    UInt                mCurRowSize;     // 현재 누적되고 있는 레코드의 크기
    UInt                mPrvRowSize;     // 이전 레코드의 실제 크기
    UInt                mMaxRowSize;     // 최대 레코드 크기 (estimation한 값)
    UInt                mLastRecordPos;  // CollectionData 내에서 마지막 완료된
                                         // 레코드의 위치 ( Fetch 도중 실패한 경우
                                         // 이전까지 완료한 레코드는 전송되어야 한다 )
    mmcBaseRow         *mBaseRow;        // PROJ-2256
} mmtCmsFetchContext;


static IDE_RC answerFetchMoveResult(cmiProtocolContext *aProtocolContext)
{
    cmiProtocol              sProtocol;
    cmpArgDBFetchMoveResultA5 *sArg;

    CMI_PROTOCOL_INITIALIZE(sProtocol, sArg, DB, FetchMoveResult);

    ACP_UNUSED(sArg);

    IDE_TEST(cmiWriteProtocol(aProtocolContext, &sProtocol) != IDE_SUCCESS);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

static IDE_RC answerFetchBeginResult(cmiProtocolContext *aProtocolContext, mmcStatement *aStatement, UShort aResultSetID)
{
    cmiProtocol               sProtocol;
    cmpArgDBFetchBeginResultA5 *sArg;

    CMI_PROTOCOL_INITIALIZE(sProtocol, sArg, DB, FetchBeginResult);

    sArg->mStatementID = aStatement->getStmtID();
    sArg->mResultSetID = aResultSetID;

    IDE_TEST(cmiWriteProtocol(aProtocolContext, &sProtocol) != IDE_SUCCESS);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

static IDE_RC answerFetchEndResult(cmiProtocolContext *aProtocolContext, mmcStatement *aStatement, UShort aResultSetID)
{
    cmiProtocol             sProtocol;
    cmpArgDBFetchEndResultA5 *sArg;

    CMI_PROTOCOL_INITIALIZE(sProtocol, sArg, DB, FetchEndResult);

    sArg->mStatementID = aStatement->getStmtID();
    sArg->mResultSetID = aResultSetID;

    IDE_TEST(cmiWriteProtocol(aProtocolContext, &sProtocol) != IDE_SUCCESS);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

static IDE_RC answerFetchListResult(cmiProtocolContext *aProtocolContext,
                                    UInt                aRowCount,
                                    UInt                aRowSize,
                                    UChar              *aCollectionData,
                                    UInt                aCollectionSize)
{
    cmiProtocol             sProtocol;
    cmpArgDBFetchListResultA5 *sArg;

    CMI_PROTOCOL_INITIALIZE(sProtocol, sArg, DB, FetchListResult);

    sArg->mRecordCount = aRowCount;
    sArg->mRecordSize  = aRowSize;
    
    if( cmiCheckInVariable( aProtocolContext, aCollectionSize ) == ID_TRUE )
    {
        cmtCollectionWriteInVariable( &sArg->mListData,
                                      aCollectionData,
                                      aCollectionSize );
    }
    else
    {
        cmtCollectionWriteVariable( &sArg->mListData,
                                    aCollectionData,
                                    aCollectionSize );
    }
    
    IDE_TEST(cmiWriteProtocol(aProtocolContext, &sProtocol) != IDE_SUCCESS);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

static IDE_RC fetchColumnCallback(idvSQL        * /*aStatistics*/,
                                  qciBindColumn *aBindColumn,
                                  void          *aData,
                                  void          *aFetchContext)
{
    mmtCmsFetchContext  *sFetchContext = (mmtCmsFetchContext *)aFetchContext;
    cmiProtocol          sProtocol;
    cmpArgDBFetchResultA5 *sArg;

    CMI_PROTOCOL_INITIALIZE(sProtocol, sArg, DB, FetchResult);

    IDE_TEST_RAISE(mmcConv::convertFromMT(sFetchContext->mProtocolContext,
                                          &sArg->mData,
                                          aData,
                                          aBindColumn->mType,
                                          sFetchContext->mStatement->getSession())
                   != IDE_SUCCESS, ConversionFail);


    IDE_TEST(cmiWriteProtocol(sFetchContext->mProtocolContext,
                              &sProtocol) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION(ConversionFail);
    {
        IDE_ASSERT(cmiFinalizeProtocol(&sProtocol) == IDE_SUCCESS);
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC fetchColumnListCallback(idvSQL        * /*aStatistics*/,
                                      qciBindColumn *aBindColumn,
                                      void          *aData,
                                      void          *aFetchContext)
{
    mmtCmsFetchContext *sFetchContext = (mmtCmsFetchContext *)aFetchContext;
    mmcSession         *sSession      = sFetchContext->mStatement->getSession();
    cmtAny              sAny;
    UInt                sCursor       = 0; 

    if( aBindColumn->mId == 0 )
    {
        // To fix BUG-20474
        // 이전 레코드의 사이즈를 이용해서 향후 저장될 사이즈를 estimation한다.
        // 잘못된 estimation은 fetch chunk의 사이즈를 초과 할수 있다.
        // 따라서, fetch chunk는 요구한 사이즈의 2배가 할당되어야 한다.
        if( (sFetchContext->mCursor + sFetchContext->mPrvRowSize)
            > sSession->getFetchChunkLimit() )
        {
            answerFetchListResult( sFetchContext->mProtocolContext,
                                   sFetchContext->mRecordNumber,
                                   sFetchContext->mPrvRowSize,
                                   sFetchContext->mCollectionData,
                                   sFetchContext->mCursor);
        
            IDE_TEST( cmiFlushProtocol( sFetchContext->mProtocolContext,
                                        ID_FALSE )
                      != IDE_SUCCESS );

            sFetchContext->mCursor       = 0;
            sFetchContext->mRecordNumber = 0;
        }
        sFetchContext->mCurRowSize = 0;
    }

    // bug-27621: mCursor: UInt pointer -> UInt
    sCursor = sFetchContext->mCursor;
    
    IDE_TEST( cmtAnyInitialize( &sAny ) != IDE_SUCCESS );

    // PROJ-2256
    if ( sFetchContext->mBaseRow != NULL )
    {
        IDE_TEST( mmcConvFromMT::checkDataRedundancy( sFetchContext->mBaseRow,
                                                      aBindColumn,
                                                      aData,
                                                      &sAny ) != IDE_SUCCESS );
    }

    /*
     * PROJ-2256 Communication protocol for efficient query result transmission
     *
     * Description :
     * REMOVE_REDUNDANT_TRANSMISSION (중복 전송 제거) 세션 프로퍼티가 off 상태이거나
     * 중복 검사 후 중복이 발생하지 않은 경우에 기존 루틴을 통해 CM 타입으로 변경하도록 한다.
     */
    if ( sAny.mType != CMT_ID_REDUNDANCY )
    {
        IDE_TEST( mmcConv::buildAnyFromMT( &sAny,
                                           aData,
                                           aBindColumn->mType,
                                           sSession ) != IDE_SUCCESS );
    }

    cmtCollectionWriteAny( sFetchContext->mCollectionData,
                           &sCursor,
                           &sAny );

    /*
     * CASE-13162
     * fetch해온 row를 collection buffer의 cursor위치부터 저장하고
     * cursor 위치를 증가시킴. 그러므로 cursor의 위치가 절대로
     * collection buffer의 크기를 넘을 수 없음.
     * 만약 cursor의 위치가 collection buffer의 크기를 넘어간다면
     * page가 깨지는 등의 abnormal한 상황임.
     */
    IDE_ASSERT( sCursor < sSession->getChunkSize() );

    sFetchContext->mCurRowSize += (sCursor - sFetchContext->mCursor);
    sFetchContext->mCursor = sCursor;

    if( aBindColumn->mId == sFetchContext->mColumnTo )
    {
        sFetchContext->mLastRecordPos = sFetchContext->mCursor;
        if( sFetchContext->mRecordNumber > 0 )
        {
            // 이전 레코드의 사이즈와 현재 레코드의 사이즈가 다르다면
            // 클라이언트에게 전송되는 레코드들은 고정길이가 아닌
            // 가변 길이를 갖는다고 명시적으로 알려줌.
            if( sFetchContext->mPrvRowSize != sFetchContext->mCurRowSize )
            {
                sFetchContext->mPrvRowSize = CMP_DB_FETCHLIST_VARIABLE_RECORD;
            }
        }
        else
        {
            sFetchContext->mPrvRowSize = sFetchContext->mCurRowSize;
        }
        sFetchContext->mRecordNumber++;
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
static IDE_RC doFetchA5( mmtCmsFetchContext *aFetchContext,
                         idBool              aHasClientListChannel,
                         UInt                aFetchProtocolType )   // BUG-34725
{
    UShort        sColumnIndex;
    mmcResultSet *sResultSet;
    mmcStatement *sResultSetStmt;
    
    sResultSet = aFetchContext->mStatement->getResultSet(aFetchContext->mResultSetID);
    // bug-26977: codesonar: resultset null ref
    // null이 가능한지는 모르겠지만, 방어코드임.
    IDE_TEST(sResultSet == NULL);

    sResultSetStmt = (mmcStatement*)sResultSet->mResultSetStmt;
    // bug-26977: codesonar: resultset null ref
    // null이 가능한지는 모르겠지만, 방어코드추가.
    IDE_TEST(sResultSetStmt == NULL);
 
    for (sColumnIndex  = aFetchContext->mColumnFrom;
         sColumnIndex <= aFetchContext->mColumnTo;
         sColumnIndex++)
    {
        if ( aHasClientListChannel == ID_TRUE
             || aFetchProtocolType == MMC_FETCH_PROTOCOL_TYPE_LIST )    // BUG-34725
        {
            IDE_TEST(qci::fetchColumn(sResultSetStmt->getQciStmt(),
                                      sColumnIndex,
                                      fetchColumnListCallback,
                                      aFetchContext) != IDE_SUCCESS);
        }
        else
        {
            IDE_TEST(qci::fetchColumn(sResultSetStmt->getQciStmt(),
                                      sColumnIndex,
                                      fetchColumnCallback,
                                      aFetchContext) != IDE_SUCCESS);
        }
    }

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

static IDE_RC fetchEnd(cmiProtocolContext *aProtocolContext,
                       mmcStatement       *aStatement,
                       UShort              aResultSetID)
{
    IDE_RC         sRet;
    UShort         sEnableResultSetCount;
    mmcResultSet * sResultSet;
    mmcStatement * sResultSetStmt;
    
    sResultSet = aStatement->getResultSet( aResultSetID );
    // bug-26977: codesonar: resultset null ref
    // null이 가능한지는 모르겠지만, 방어코드임.
    IDE_TEST(sResultSet == NULL);

    sResultSetStmt = (mmcStatement*)sResultSet->mResultSetStmt;
    
    sEnableResultSetCount = aStatement->getEnableResultSetCount();

    if (aStatement->getResultSetState(aResultSetID) != MMC_RESULTSET_STATE_FETCH_CLOSE)
    {
        // bug-26977: codesonar: resultset null ref
        // null이 가능한지는 모르겠지만, 방어코드추가.
        IDE_TEST(sResultSetStmt == NULL);

        mmcStatement::makePlanTreeBeforeCloseCursor( aStatement,
                                                     sResultSetStmt );
        
        IDE_TEST_RAISE(qci::closeCursor(sResultSetStmt->getQciStmt(),
                                        sResultSetStmt->getSmiStmt())
                                        != IDE_SUCCESS, CloseCursorFailure);

        answerFetchEndResult(aProtocolContext, aStatement, aResultSetID);

        // Fetch 가능한 Result Set을 하나 감소
        sEnableResultSetCount--;
        aStatement->setEnableResultSetCount(sEnableResultSetCount);
    }

    IDE_TEST(aStatement->endFetch(aResultSetID) != IDE_SUCCESS);

    if (sEnableResultSetCount <= 0)
    {
        /* PROJ-2223 Altibase Auditing */
        mmtAuditManager::auditByAccess( aStatement, MMC_EXECUTION_FLAG_SUCCESS );

        IDE_TEST_RAISE(qci::closeCursor(aStatement->getQciStmt(),
                                        aStatement->getSmiStmt())
                                        != IDE_SUCCESS, CloseCursorFailure);

        sRet = aStatement->clearStmt(MMC_STMT_BIND_NONE);

        IDE_TEST(aStatement->endStmt(MMC_EXECUTION_FLAG_SUCCESS) != IDE_SUCCESS);

        aStatement->setExecuteFlag(ID_FALSE);

        IDE_TEST(sRet != IDE_SUCCESS);
    }

    return IDE_SUCCESS;

    /* 
     * BUG-30053 
     * Even though qci::closeCursor failed, 
     * The statement should be cleared.
     */
    IDE_EXCEPTION(CloseCursorFailure);
    {
        aStatement->clearStmt(MMC_STMT_BIND_INFO);
        aStatement->endStmt(MMC_EXECUTION_FLAG_FAILURE);
        aStatement->setExecuteFlag(ID_FALSE);
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC mmtServiceThread::fetchA5(cmiProtocolContext *aProtocolContext,
                               mmcSession         *aSession,
                               mmcStatement       *aStatement,
                               UShort              aResultSetID,
                               UShort              aColumnFrom,
                               UShort              aColumnTo,
                               UShort              aRecordCount)
{
    mmtCmsFetchContext  sFetchContext;
    idBool              sRecordExist;
    UInt                sSize;
    mmcResultSet       *sResultSet;
    mmcStatement       *sResultSetStmt;
    mmcBaseRow         *sBaseRow = NULL;    // PROJ-2256
    
    sFetchContext.mProtocolContext = aProtocolContext;
    sFetchContext.mStatement       = aStatement;
    sFetchContext.mResultSetID     = aResultSetID;
    sFetchContext.mColumnFrom      = aColumnFrom;
    sFetchContext.mColumnTo        = aColumnTo;
    sFetchContext.mRecordNumber    = 0;
    sFetchContext.mLastRecordPos   = 0;
    sFetchContext.mBaseRow         = NULL;

    sResultSet = aStatement->getResultSet( aResultSetID );
    // bug-26977: codesonar: resultset null ref
    // null이 가능한지는 모르겠지만, 방어코드임.
    IDE_TEST(sResultSet == NULL);

    sResultSetStmt = (mmcStatement*)sResultSet->mResultSetStmt;
    // bug-26977: codesonar: resultset null ref
    // null이 가능한지는 모르겠지만, 방어코드추가.
    IDE_TEST(sResultSetStmt == NULL);
 
    IDE_TEST( qci::getRowSize( sResultSetStmt->getQciStmt(), &sSize )
              != IDE_SUCCESS );
    
    if ( aSession->getHasClientListChannel() == ID_TRUE
         || aSession->getFetchProtocolType() == MMC_FETCH_PROTOCOL_TYPE_LIST )  // BUG-34725
    {
        // 통신상 필요한 공간을 estimation한다.
        sFetchContext.mMaxRowSize =
            qci::getColumnCount(sResultSetStmt->getQciStmt()) * cmiGetMaxInTypeHeaderSize();
        sFetchContext.mMaxRowSize += sSize;

        // 적어도 하나의 레코드는 전송되어야 한다.
        // BUG-29810 Fetch시 Memeory 부족상황에서 에러코드를 보여주지 않습니다.
        IDE_TEST_RAISE(aSession->allocChunk4Fetch(
                                 IDL_MAX(sFetchContext.mMaxRowSize,
                                         (MMC_DEFAULT_COLLECTION_BUFFER_SIZE)) )
                       != IDE_SUCCESS, FetchError);

        sFetchContext.mCollectionData  = aSession->getChunk();
        // bug-27621: mCursor: UInt pointer -> UInt
        // 지역변수 sCursor의 주소지정을 제거.
        sFetchContext.mCursor          = 0;
        sFetchContext.mCurRowSize      = 0;
        sFetchContext.mPrvRowSize      = 0;

        // PROJ-2256
        if ( aSession->getRemoveRedundantTransmission() == 1
             && sResultSetStmt->getStmtType() != QCI_STMT_DEQUEUE )
        {
            sBaseRow               = &( sResultSet->mBaseRow );
            sFetchContext.mBaseRow = sBaseRow;
            IDE_TEST( aStatement->allocBaseRow( sBaseRow, sSize )
                      != IDE_SUCCESS );
        }
    }

    if (aRecordCount != 0)
    {
        do
        {
            IDE_TEST_RAISE( doFetchA5( &sFetchContext,
                                       aSession->getHasClientListChannel(),
                                       aSession->getFetchProtocolType() )   // BUG-34725
                            != IDE_SUCCESS, FetchError );

            if (aStatement->getStmtType() == QCI_STMT_DEQUEUE)
            {
                sRecordExist = ID_FALSE;
            }
            else
            {
                IDE_TEST_RAISE(qci::moveNextRecord(sResultSetStmt->getQciStmt(),
                                                   sResultSetStmt->getSmiStmt(),
                                                   &sRecordExist) != IDE_SUCCESS,
                               FetchError);

                // PROJ-2256
                if ( sBaseRow != NULL )
                {
                    sBaseRow->mBaseColumnPos = 0;

                    if ( sBaseRow->mIsFirstRow == ID_TRUE )
                    {
                        sBaseRow->mIsFirstRow = ID_FALSE;
                    }
                }
            }

            if (sRecordExist != ID_TRUE)
            {
                aStatement->setFetchFlag(MMC_FETCH_FLAG_CLOSE);
                break;
            }

            aRecordCount--;

        } while (aRecordCount > 0);
    }
    else
    {
        // fix BUG-17715
        // 클라이언트가 FETCH할 레코드 갯수를 명시하지 않았을 경우
        // 통신버퍼에 가능한 모든 레코드 갯수를 저장한다.
        do
        {
            IDE_TEST_RAISE( doFetchA5( &sFetchContext,
                                       aSession->getHasClientListChannel(),
                                       aSession->getFetchProtocolType() )   // BUG-34725
                            != IDE_SUCCESS, FetchError );

            if (aStatement->getStmtType() == QCI_STMT_DEQUEUE)
            {
                sRecordExist = ID_FALSE;
            }
            else
            {
                IDE_TEST_RAISE(qci::moveNextRecord(sResultSetStmt->getQciStmt(),
                                                   sResultSetStmt->getSmiStmt(),
                                                   &sRecordExist) != IDE_SUCCESS,
                               FetchError);

                // PROJ-2256
                if ( sBaseRow != NULL )
                {
                    sBaseRow->mBaseColumnPos = 0;

                    if ( sBaseRow->mIsFirstRow == ID_TRUE )
                    {
                        sBaseRow->mIsFirstRow = ID_FALSE;
                    }
                }
            }

            if (sRecordExist != ID_TRUE)
            {
                aStatement->setFetchFlag(MMC_FETCH_FLAG_CLOSE);
                break;
            }

            if ( aSession->getHasClientListChannel() == ID_TRUE
                 || aSession->getFetchProtocolType() == MMC_FETCH_PROTOCOL_TYPE_LIST )  // BUG-34725
            {
                // 이전 사이즈를 이용하여 향후 필요한 사이즈를 estimation 한다.
                if( (sFetchContext.mCursor + sFetchContext.mPrvRowSize)
                    > aSession->getFetchChunkLimit() )
                {
                    break;
                }
            }
            else
            {
                if( cmiCheckFetch(aProtocolContext, sSize) != IDE_SUCCESS )
                {
                    break;
                }
            }
        } while (1);
    }

    if ( ( aSession->getHasClientListChannel() == ID_TRUE
           || aSession->getFetchProtocolType() == MMC_FETCH_PROTOCOL_TYPE_LIST )    // BUG-34725
         && sFetchContext.mRecordNumber > 0 )
    {
        answerFetchListResult( aProtocolContext,
                               sFetchContext.mRecordNumber,
                               sFetchContext.mPrvRowSize,
                               sFetchContext.mCollectionData,
                               sFetchContext.mCursor);
    }
    
    if (sRecordExist != ID_TRUE)
    {
        IDE_TEST(fetchEnd(aProtocolContext,
                          aStatement,
                          aResultSetID)
                 != IDE_SUCCESS);
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION(FetchError);
    {
        IDE_PUSH();

        if ( ( aSession->getHasClientListChannel() == ID_TRUE
               || aSession->getFetchProtocolType() == MMC_FETCH_PROTOCOL_TYPE_LIST )    // BUG-34725
             && sFetchContext.mRecordNumber > 0 )
        {
            answerFetchListResult( aProtocolContext,
                                   sFetchContext.mRecordNumber,
                                   sFetchContext.mPrvRowSize,
                                   sFetchContext.mCollectionData,
                                   sFetchContext.mLastRecordPos );
        }
        
        fetchEnd(aProtocolContext, aStatement, aResultSetID);

        IDE_POP();
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mmtServiceThread::fetchMoveProtocolA5(cmiProtocolContext *aProtocolContext,
                                           cmiProtocol        *aProtocol,
                                           void               *aSessionOwner,
                                           void               *aUserContext)
{
    cmpArgDBFetchMoveA5 *sArg    = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, FetchMove);
    mmcTask           *sTask   = (mmcTask *)aSessionOwner;
    mmtServiceThread  *sThread = (mmtServiceThread *)aUserContext;
    mmcSession        *sSession;
    mmcStatement      *sStatement;
    idBool             sRecordExist = ID_FALSE;
    SLong              i;
    mmcResultSet     * sResultSet;
    mmcStatement     * sResultSetStmt;
    
    IDE_CLEAR();

    IDE_TEST(findSession(sTask, &sSession, sThread) != IDE_SUCCESS);

    IDE_TEST(checkSessionState(sSession, MMC_SESSION_STATE_SERVICE) != IDE_SUCCESS);

    IDE_TEST(findStatement(&sStatement,
                           sSession,
                           &sArg->mStatementID,
                           sThread) != IDE_SUCCESS);
    //fix BUG-17689.
    IDE_TEST_RAISE(checkStatementState(sStatement, MMC_STMT_STATE_EXECUTED) != IDE_SUCCESS,
                   NoRows);

    IDE_TEST_RAISE(sArg->mWhence != CMP_DB_FETCHMOVE_CUR, UnsupportedFetchMove);
    IDE_TEST_RAISE(sArg->mOffset <= 0, UnsupportedFetchMove);

    sResultSet = sStatement->getResultSet(sArg->mResultSetID);
    // bug-26977: codesonar: resultset null ref
    // null이 가능한지는 모르겠지만, 방어코드임.
    IDE_TEST(sResultSet == NULL);

    sResultSetStmt = (mmcStatement*)sResultSet->mResultSetStmt;

    sStatement->setFetchEndTime(0); /* BUG-19456 */
    sStatement->setFetchStartTime(mmtSessionManager::getBaseTime());

    if (sStatement->getStmtType() == QCI_STMT_DEQUEUE)
    {
        if (sArg->mOffset > 1)
        {
            IDE_TEST(fetchEnd(aProtocolContext, sStatement, sArg->mResultSetID) != IDE_SUCCESS);
        }
        else
        {
            IDE_TEST(answerFetchMoveResult(aProtocolContext) != IDE_SUCCESS);
        }
    }
    else
    {
        sArg->mOffset--;

        // bug-26977: codesonar: resultset null ref
        // null이 가능한지는 모르겠지만, 방어코드추가.
        IDE_TEST(sResultSetStmt == NULL);

        for (i = 0; i < sArg->mOffset; i++)
        {
            IDE_TEST(qci::moveNextRecord(sResultSetStmt->getQciStmt(),
                                         sResultSetStmt->getSmiStmt(),
                                         &sRecordExist) != IDE_SUCCESS);

            if (sRecordExist != ID_TRUE)
            {
                sStatement->setFetchFlag(MMC_FETCH_FLAG_CLOSE);
                IDE_TEST(fetchEnd(aProtocolContext, sStatement, sArg->mResultSetID) != IDE_SUCCESS);

                break;
            }
        }

        if (sRecordExist == ID_TRUE)
        {
            IDE_TEST(answerFetchMoveResult(aProtocolContext) != IDE_SUCCESS);
        }
    }

    /* BUG-19456 */
    sStatement->setFetchEndTime(mmtSessionManager::getBaseTime());

    return IDE_SUCCESS;

    IDE_EXCEPTION(NoRows);
    {
        //fix BUG-17689.
        answerFetchEndResult(aProtocolContext,
                             sStatement,
                             sArg->mResultSetID);
        return IDE_SUCCESS;
    }
    IDE_EXCEPTION(UnsupportedFetchMove);
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_UNSUPPORTED_FETCHMOVE));
    }
    IDE_EXCEPTION_END;

    return sThread->answerErrorResultA5(aProtocolContext,
                                      CMI_PROTOCOL_OPERATION(DB, FetchMove),
                                      0);
}

IDE_RC mmtServiceThread::fetchProtocolA5(cmiProtocolContext *aProtocolContext,
                                       cmiProtocol        *aProtocol,
                                       void               *aSessionOwner,
                                       void               *aUserContext)
{
    cmpArgDBFetchA5    *sArg    = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, Fetch);
    mmcTask          *sTask   = (mmcTask *)aSessionOwner;
    mmtServiceThread *sThread = (mmtServiceThread *)aUserContext;
    mmcSession       *sSession;
    mmcStatement     *sStatement;
    UShort            sColumnCount;
    UShort            sResultSetID;
    mmcResultSet    * sResultSet;
    mmcStatement    * sResultSetStmt;
    
    IDE_CLEAR();

    IDE_TEST(findSession(sTask, &sSession, sThread) != IDE_SUCCESS);

    IDE_TEST(checkSessionState(sSession, MMC_SESSION_STATE_SERVICE) != IDE_SUCCESS);

    IDE_TEST(findStatement(&sStatement,
                           sSession,
                           &sArg->mStatementID,
                           sThread) != IDE_SUCCESS);

    //fix BUG-17689.
    sResultSetID = sArg->mResultSetID;
    IDE_TEST_RAISE(checkStatementState(sStatement, MMC_STMT_STATE_EXECUTED) != IDE_SUCCESS,
                   NoRows);

    sResultSet = sStatement->getResultSet( sResultSetID );
    // bug-26977: codesonar: resultset null ref
    // null이 가능한지는 모르겠지만, 방어코드임.
    IDE_TEST(sResultSet == NULL);

    sResultSetStmt = (mmcStatement*)sResultSet->mResultSetStmt;

    switch (sStatement->getResultSetState(sResultSetID))
    {
        case MMC_RESULTSET_STATE_INITIALIZE:
            
            // Result Set의 moveNextReocrd()는 Execute시에 호출 되었다.
            // Result Set이 여러개인 경우 처음 execute시에 record가 없으면
            // MMC_RESULTSET_STATE_INITIALIZE상태이며, 이는 바로 fetchEnd
            sStatement->setFetchFlag(MMC_FETCH_FLAG_CLOSE);
            IDE_RAISE(FetchEnd);
            break;
            
        case MMC_RESULTSET_STATE_FETCH_READY:
            sStatement->setFetchFlag(MMC_FETCH_FLAG_PROCEED);
            IDE_TEST(answerFetchBeginResult(aProtocolContext, sStatement, sResultSetID) != IDE_SUCCESS);

            IDE_TEST(sStatement->setResultSetState(sResultSetID, MMC_RESULTSET_STATE_FETCH_PROCEED) != IDE_SUCCESS);
            break;

        case MMC_RESULTSET_STATE_FETCH_PROCEED:
            break;

        default:
            IDE_RAISE(NoCursor);
            break;
    }

    sStatement->setFetchEndTime(0); /* BUG-19456 */
    sStatement->setFetchStartTime(mmtSessionManager::getBaseTime());

    // bug-26977: codesonar: resultset null ref
    // null이 가능한지는 모르겠지만, 방어코드추가.
    IDE_TEST(sResultSetStmt == NULL);
    sColumnCount = qci::getColumnCount(sResultSetStmt->getQciStmt());

    if (sArg->mColumnFrom == 0)
    {
        sArg->mColumnFrom = 1;
    }

    if (sArg->mColumnTo == 0)
    {
        sArg->mColumnTo = sColumnCount;
    }
    else
    {
        if (sArg->mColumnTo > sColumnCount)
        {
            sArg->mColumnTo = sColumnCount;
        }
    }

    IDE_TEST(sThread->fetchA5(aProtocolContext,
                            sSession,
                            sStatement,
                            sArg->mResultSetID,
                            sArg->mColumnFrom - 1,
                            sArg->mColumnTo - 1,
                            sArg->mRecordCount) != IDE_SUCCESS);

    /* BUG-19456 */
    sStatement->setFetchEndTime(mmtSessionManager::getBaseTime());

    return IDE_SUCCESS;

    IDE_EXCEPTION(NoRows);
    {
        //fix BUG-17689.
        answerFetchEndResult(aProtocolContext,
                             sStatement,
                             sResultSetID);
        return IDE_SUCCESS;
    }
    IDE_EXCEPTION(FetchEnd);
    {
        IDE_PUSH();

        fetchEnd(aProtocolContext,
                 sStatement,
                 sResultSetID);
        
        IDE_POP();

        return IDE_SUCCESS;
    }
    
    IDE_EXCEPTION(NoCursor);
    {
        IDE_SET(ideSetErrorCode(mmERR_IGNORE_NO_CURSOR));

    }
    IDE_EXCEPTION_END;

    return sThread->answerErrorResultA5(aProtocolContext,
                                      CMI_PROTOCOL_OPERATION(DB, Fetch),
                                      0);
}
