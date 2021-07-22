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
#include <mmcSession.h>
#include <mmcStatement.h>
#include <mmcStatementManager.h>
#include <mmtServiceThread.h>


static IDE_RC answerCancelResult(cmiProtocolContext *aProtocolContext)
{
    cmiWriteCheckState sWriteCheckState = CMI_WRITE_CHECK_DEACTIVATED;

    sWriteCheckState = CMI_WRITE_CHECK_ACTIVATED;
    CMI_WRITE_CHECK(aProtocolContext, 1);
    sWriteCheckState = CMI_WRITE_CHECK_DEACTIVATED;

    CMI_WOP(aProtocolContext, CMP_OP_DB_CancelResult);

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

static IDE_RC answerTransactionResult(cmiProtocolContext *aProtocolContext)
{
    cmiWriteCheckState sWriteCheckState = CMI_WRITE_CHECK_DEACTIVATED;

    sWriteCheckState = CMI_WRITE_CHECK_ACTIVATED;
    CMI_WRITE_CHECK(aProtocolContext, 1);
    sWriteCheckState = CMI_WRITE_CHECK_DEACTIVATED;

    CMI_WOP(aProtocolContext, CMP_OP_DB_TransactionResult);

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

/* BUG-46785 Shard statement partial rollback */
static IDE_RC answerSetSavepointResult( cmiProtocolContext *aProtocolContext )
{
    cmiWriteCheckState sWriteCheckState = CMI_WRITE_CHECK_DEACTIVATED;

    sWriteCheckState = CMI_WRITE_CHECK_ACTIVATED;
    CMI_WRITE_CHECK( aProtocolContext, 1 );
    sWriteCheckState = CMI_WRITE_CHECK_DEACTIVATED;

    CMI_WOP( aProtocolContext, CMP_OP_DB_SetSavepointResult );

    /* PROJ-2616 */
    MMT_IPCDA_INCREASE_DATA_COUNT( aProtocolContext );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    /* BUG-44124 ipcda 모드 사용 중 hang - iloader 컬럼이 많은 테이블 */
    if( ( sWriteCheckState == CMI_WRITE_CHECK_ACTIVATED ) &&
        ( cmiGetLinkImpl( aProtocolContext ) == CMI_LINK_IMPL_IPCDA ) )
    {
        IDE_SET( ideSetErrorCode( mmERR_ABORT_IPCDA_MESSAGE_TOO_LONG, CMB_BLOCK_DEFAULT_SIZE ) );
    }

    return IDE_FAILURE;
}

/* BUG-46785 Shard statement partial rollback */
static IDE_RC answerRollbackToSavepointResult( cmiProtocolContext *aProtocolContext )
{
    cmiWriteCheckState sWriteCheckState = CMI_WRITE_CHECK_DEACTIVATED;

    sWriteCheckState = CMI_WRITE_CHECK_ACTIVATED;
    CMI_WRITE_CHECK( aProtocolContext, 1 );
    sWriteCheckState = CMI_WRITE_CHECK_DEACTIVATED;

    CMI_WOP( aProtocolContext, CMP_OP_DB_RollbackToSavepointResult );

    /* PROJ-2616 */
    MMT_IPCDA_INCREASE_DATA_COUNT( aProtocolContext );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    /* BUG-44124 ipcda 모드 사용 중 hang - iloader 컬럼이 많은 테이블 */
    if( ( sWriteCheckState == CMI_WRITE_CHECK_ACTIVATED ) &&
        ( cmiGetLinkImpl( aProtocolContext ) == CMI_LINK_IMPL_IPCDA ) )
    {
        IDE_SET( ideSetErrorCode( mmERR_ABORT_IPCDA_MESSAGE_TOO_LONG, CMB_BLOCK_DEFAULT_SIZE ) );
    }

    return IDE_FAILURE;
}

/* BUG-46785 Shard statement partial rollback */
static IDE_RC answerShardNodeReportResult( cmiProtocolContext  * aProtocolContext )
{
    cmiWriteCheckState sWriteCheckState = CMI_WRITE_CHECK_DEACTIVATED;

    sWriteCheckState = CMI_WRITE_CHECK_ACTIVATED;
    CMI_WRITE_CHECK( aProtocolContext, 1 );
    sWriteCheckState = CMI_WRITE_CHECK_DEACTIVATED;

    CMI_WOP( aProtocolContext, CMP_OP_DB_ShardNodeReportResult );

    /* PROJ-2616 */
    MMT_IPCDA_INCREASE_DATA_COUNT( aProtocolContext );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    /* BUG-44124 ipcda 모드 사용 중 hang - iloader 컬럼이 많은 테이블 */
    if( ( sWriteCheckState == CMI_WRITE_CHECK_ACTIVATED ) &&
        ( cmiGetLinkImpl(aProtocolContext) == CMI_LINK_IMPL_IPCDA ) )
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_IPCDA_MESSAGE_TOO_LONG, CMB_BLOCK_DEFAULT_SIZE));
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_FAILURE;
}

IDE_RC mmtServiceThread::cancelProtocol(cmiProtocolContext *aProtocolContext,
                                        cmiProtocol        *,
                                        void               */* aSessionOwner */,
                                        void               *aUserContext)
{
    mmtServiceThread *sThread = (mmtServiceThread *)aUserContext;

    UInt          sStatementID;

    /* PROJ-2160 CM 타입제거
       모두 읽은 다음에 프로토콜을 처리해야 한다. */
    CMI_RD4(aProtocolContext, &sStatementID);

    IDE_CLEAR();

    /* PROJ-2177 User Interface - Cancel */
    IDE_TEST(mmtSessionManager::setCancelEvent(sStatementID) != IDE_SUCCESS);

    return answerCancelResult(aProtocolContext);

    IDE_EXCEPTION_END;

    return sThread->answerErrorResult(aProtocolContext,
                                      CMI_PROTOCOL_OPERATION(DB, Cancel),
                                      0);
}

/* PROJ-2177 User Interface - Cancel */
IDE_RC mmtServiceThread::cancelByCIDProtocol(cmiProtocolContext *aProtocolContext,
                                             cmiProtocol        */* aProtocol */,
                                             void               */* aSessionOwner */,
                                             void               *aUserContext)
{
    mmtServiceThread    *sThread = (mmtServiceThread *)aUserContext;
    mmcSessID            sSessionID;
    mmcSession          *sSession = NULL;
    UInt                 sStmtCID;
    mmcStmtID            sStmtID;

    IDE_CLEAR();

    /* PROJ-2160 CM 타입제거
       모두 읽은 다음에 프로토콜을 처리해야 한다. */
    CMI_RD4(aProtocolContext, &sStmtCID);

    sSessionID = MMC_STMT_CID_SESSION(sStmtCID);
    IDE_TEST(mmtSessionManager::findSession(&sSession, sSessionID) != IDE_SUCCESS);

    /* StmtCID로 Cancel을 시도할 때는, 아직 Stmt가 없을 수도 있다.
     * 이때는 조용히 넘어가며, SUCCESS를 반환한다. (ERROR가 아니다.)
     * 예) Alloc 하자마자 Cancel을 시도한 경우 */
    sStmtID = sSession->getStmtIDFromMap(sStmtCID);
    if (sStmtID != MMC_STMT_ID_NONE)
    {
        IDE_TEST(mmtSessionManager::setCancelEvent(sStmtID) != IDE_SUCCESS);
    }

    return answerCancelResult(aProtocolContext);

    IDE_EXCEPTION_END;

    return sThread->answerErrorResult(aProtocolContext,
                                      CMI_PROTOCOL_OPERATION(DB, CancelByCID),
                                      0);
}

IDE_RC mmtServiceThread::transactionProtocol(cmiProtocolContext *aProtocolContext,
                                             cmiProtocol        *,
                                             void               *aSessionOwner,
                                             void               *aUserContext)
{
    mmcTask             *sTask = (mmcTask *)aSessionOwner;
    mmtServiceThread    *sThread = (mmtServiceThread *)aUserContext;
    mmcSession          *sSession;

    UChar       sOperation;

    /* PROJ-2160 CM 타입제거
       모두 읽은 다음에 프로토콜을 처리해야 한다. */
    CMI_RD1(aProtocolContext, sOperation);

    IDE_CLEAR();

    IDE_TEST(findSession(sTask, &sSession, sThread) != IDE_SUCCESS);

    IDE_TEST(checkSessionState(sSession, MMC_SESSION_STATE_SERVICE) != IDE_SUCCESS);

    /* BUG-20832 */
    IDE_TEST_RAISE(sSession->getXaAssocState() != MMD_XA_ASSOC_STATE_NOTASSOCIATED,
                   DCLNotAllowedError);

    switch (sOperation)
    {
        case CMP_DB_TRANSACTION_COMMIT:
            IDE_TEST(sSession->commit() != IDE_SUCCESS);
            break;

        case CMP_DB_TRANSACTION_ROLLBACK:
            IDE_TEST(sSession->rollback() != IDE_SUCCESS);
            break;

        default:
            IDE_RAISE(InvalidOperation);
            break;
    }

    IDE_TEST( answerTransactionResult(aProtocolContext) != IDE_SUCCESS )

    if ( sSession->isNeedRebuildNoti() == ID_TRUE )
    {
        IDE_TEST( sendShardRebuildNoti( aProtocolContext )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(DCLNotAllowedError);
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_NOT_ALLOWED_DCL));
    }
    IDE_EXCEPTION(InvalidOperation);
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_INVALID_ERROR));
    }
    IDE_EXCEPTION_END;

    return sThread->answerErrorResult(aProtocolContext,
                                      CMI_PROTOCOL_OPERATION(DB, Transaction),
                                      0);
}

/* BUG-46785 Shard statement partial rollback */
IDE_RC mmtServiceThread::setSavepointProtocol( cmiProtocolContext   *aProtocolContext,
                                               cmiProtocol          *,
                                               void                 *aSessionOwner,
                                               void                 *aUserContext )
{

    mmcTask             *sTask = (mmcTask *)aSessionOwner;
    mmtServiceThread    *sThread = (mmtServiceThread *)aUserContext;
    mmcSession          *sSession;
    SChar                sSavepointName[SMI_MAX_SVPNAME_SIZE] = { 0, };
    UInt                 sSavepointNameLength = 0;

    CMI_RD4( aProtocolContext, &sSavepointNameLength );

    CMI_RCP( aProtocolContext, sSavepointName, sSavepointNameLength );

    IDE_CLEAR();

    IDE_TEST( findSession(sTask, &sSession, sThread) != IDE_SUCCESS );

    IDE_TEST( checkSessionState(sSession, MMC_SESSION_STATE_SERVICE) != IDE_SUCCESS );

    /* BUG-20832 */
    IDE_TEST_RAISE( sSession->getXaAssocState() != MMD_XA_ASSOC_STATE_NOTASSOCIATED,
                    DCLNotAllowedError );

    IDE_TEST( sSession->savepoint( sSavepointName, ID_FALSE ) != IDE_SUCCESS );

    return answerSetSavepointResult( aProtocolContext );

    IDE_EXCEPTION(DCLNotAllowedError);
    {
        IDE_SET( ideSetErrorCode( mmERR_ABORT_NOT_ALLOWED_DCL ) );
    }
    IDE_EXCEPTION_END;

    return sThread->answerErrorResult( aProtocolContext,
                                       CMI_PROTOCOL_OPERATION( DB, SetSavepoint ),
                                       0 );
}

/* BUG-46785 Shard statement partial rollback */
IDE_RC mmtServiceThread::rollbackToSavepointProtocol( cmiProtocolContext   *aProtocolContext,
                                                      cmiProtocol          *,
                                                      void                 *aSessionOwner,
                                                      void                 *aUserContext )
{

    mmcTask             *sTask = (mmcTask *)aSessionOwner;
    mmtServiceThread    *sThread = (mmtServiceThread *)aUserContext;
    mmcSession          *sSession;
    SChar                sSavepointName[1024] = { 0, };
    UInt                 sSavepointNameLength = 0;

    CMI_RD4( aProtocolContext, &sSavepointNameLength );

    CMI_RCP( aProtocolContext, sSavepointName, sSavepointNameLength );

    IDE_CLEAR();

    IDE_TEST( findSession(sTask, &sSession, sThread) != IDE_SUCCESS );

    IDE_TEST( checkSessionState(sSession, MMC_SESSION_STATE_SERVICE) != IDE_SUCCESS );

    /* BUG-20832 */
    IDE_TEST_RAISE( sSession->getXaAssocState() != MMD_XA_ASSOC_STATE_NOTASSOCIATED,
                    DCLNotAllowedError );

    IDE_TEST( sSession->rollback( sSavepointName, ID_FALSE ) != IDE_SUCCESS );

    return answerRollbackToSavepointResult( aProtocolContext );

    IDE_EXCEPTION( DCLNotAllowedError );
    {
        IDE_SET( ideSetErrorCode( mmERR_ABORT_NOT_ALLOWED_DCL ) );
    }
    IDE_EXCEPTION_END;

    return sThread->answerErrorResult( aProtocolContext,
                                       CMI_PROTOCOL_OPERATION( DB, RollbackToSavepoint ),
                                       0 );
}

/* BUG-46785 Shard statement partial rollback */
IDE_RC mmtServiceThread::shardNodeReport( cmiProtocolContext *aProtocolContext,
                                          cmiProtocol        *,
                                          void               *aSessionOwner,
                                          void               *aUserContext )
{
    mmcTask             *sTask = (mmcTask *)aSessionOwner;
    mmtServiceThread    *sThread = (mmtServiceThread *)aUserContext;
    mmcSession          *sSession;
    UInt                 sType;
    UInt                 sNodeId;
    UChar                sDestination;

    CMI_RD4( aProtocolContext, &sType );

    switch( sType )
    {
        case CMP_DB_SHARD_NODE_CONNECTION_REPORT:
            CMI_RD4( aProtocolContext, &sNodeId );
            CMI_RD1( aProtocolContext, sDestination );
            break;

        case CMP_DB_SHARD_NODE_TRANSACTION_BROKEN_REPORT:
        default:
            break;
    }

    IDE_TEST( findSession( sTask, &sSession, sThread ) != IDE_SUCCESS );

    IDE_TEST( checkSessionState( sSession, MMC_SESSION_STATE_SERVICE ) != IDE_SUCCESS) ;

    IDU_FIT_POINT_RAISE( "mmtServiceThread::shardNodeReport::InvalidType",
                         InvalidType );

    switch( sType )
    {
        case CMP_DB_SHARD_NODE_CONNECTION_REPORT:
            IDE_TEST( sSession->shardNodeConnectionReport( sNodeId,
                                                           sDestination )
                      != IDE_SUCCESS );
            break;

        case CMP_DB_SHARD_NODE_TRANSACTION_BROKEN_REPORT:
            IDE_TEST( sSession->shardNodeTransactionBrokenReport() != IDE_SUCCESS );
            break;

        default:
            IDE_RAISE( InvalidType );
            break;
    }

    return answerShardNodeReportResult( aProtocolContext );

    IDE_EXCEPTION( InvalidType )
    {
        IDE_SET( ideSetErrorCode( mmERR_ABORT_INTERNAL_SERVER_ERROR_ARG,
                                  "Report type is invalid" ) );
    }
    IDE_EXCEPTION_END;

    return sThread->answerErrorResult( aProtocolContext,
                                       CMI_PROTOCOL_OPERATION( DB, ShardNodeReport ),
                                       0 );
}
