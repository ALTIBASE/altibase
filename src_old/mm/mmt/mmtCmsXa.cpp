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
#include <mmcTask.h>
#include <mmdDef.h>
#include <mmdXa.h>
#include <mmtServiceThread.h>


typedef void (*mmdXaConnectionFunc)(mmdXaContext *aXaContext);
/* BUG-18981 */
typedef void (*mmdXaTransactionFunc)(mmdXaContext *aXaContext, ID_XID *aXid);

static IDE_RC answerXaResult(cmiProtocolContext *aCtx, mmdXaContext *aXaContext)
{
    CMI_WRITE_CHECK(aCtx, 6);

    CMI_WOP(aCtx, CMP_OP_DB_XaResult);
    CMI_WR1(aCtx, aXaContext->mOperation);
    CMI_WR4(aCtx, (UInt*)&(aXaContext->mReturnValue));

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

static IDE_RC xaOpConnection(cmiProtocolContext  *aCtx,
                             mmdXaContext        *aXaContext,
                             mmdXaConnectionFunc  aXaFunc)
{
    aXaFunc(aXaContext);

    return answerXaResult(aCtx, aXaContext);
}

static IDE_RC xaOpTransaction(cmiProtocolContext   *aCtx,
                              mmdXaContext         *aXaContext,
                              mmdXaTransactionFunc  aXaFunc )
{
    /* BUG-18981 */
    ID_XID      sXid;
    SLong       sFormatID;
    SLong       sGTRIDLength;
    SLong       sBQUALLength;

    /* PROJ-2160 CM 타입제거
       모두 읽은 다음에 프로토콜을 처리해야 한다. */
    // XID 구조멤버가  가변길이 long형이어서 고정길이 버퍼가 필요
    CMI_RD8(aCtx, (ULong*)&(sFormatID));
    CMI_RD8(aCtx, (ULong*)&(sGTRIDLength));
    CMI_RD8(aCtx, (ULong*)&(sBQUALLength));

    sXid.formatID     = sFormatID;
    sXid.gtrid_length = sGTRIDLength;
    sXid.bqual_length = sBQUALLength;

    CMI_RCP(aCtx, sXid.data, ID_MAXXIDDATASIZE);

    /* bug-36037: invalid xid
       invalid xid 관련 서버쪽 검사코드 추가 */
    if ((sXid.gtrid_length <= (vSLong) 0)               ||
        (sXid.gtrid_length >  (vSLong) ID_MAXGTRIDSIZE) ||
        (sXid.bqual_length <= (vSLong) 0)               ||
        (sXid.bqual_length >  (vSLong) ID_MAXBQUALSIZE))
    {
        IDE_RAISE(InvalidXid);
    }

    /* BUG-20726 */
    idlOS::memset(&(sXid.data[sXid.gtrid_length+sXid.bqual_length]),
                  0x00,
                  ID_MAXXIDDATASIZE-sXid.gtrid_length-sXid.bqual_length);

    aXaFunc(aXaContext, &sXid);

    return answerXaResult(aCtx, aXaContext);

    /* bug-36037: invalid xid */
    IDE_EXCEPTION(InvalidXid);
    {
        IDE_SET(ideSetErrorCode( mmERR_ABORT_INVALID_XID ));
        aXaContext->mReturnValue = XAER_NOTA;
    }
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

static IDE_RC xaOpRecover(cmiProtocolContext *aCtx,
                          mmdXaContext       *aXaContext,
                          vSLong              /*aCount*/)
{
    /* BUG-18981 */
    ID_XID  *sPreparedXids     = NULL;
    ID_XID  *sHeuristicXids    = NULL;
    SInt     i;
    SInt     sPreparedXidsCnt  = 0;
    SInt     sHeuristicXidsCnt = 0;
    SLong    sFormatID;
    SLong    sGTRIDLength;
    SLong    sBQUALLength;
    UShort   sOrgWriteCursor   = CMI_GET_CURSOR(aCtx);

    mmdXa::recover(aXaContext, &sPreparedXids, &sPreparedXidsCnt,
                   &sHeuristicXids, &sHeuristicXidsCnt);

    /* BUG-19070 */
    IDE_TEST(answerXaResult(aCtx, aXaContext) != IDE_SUCCESS);

    for (i = 0; i < sPreparedXidsCnt; i++)
    {
        // XID 구조멤버가  가변길이 long형이어서 고정길이 버퍼가 필요
        sFormatID    = sPreparedXids[i].formatID;
        sGTRIDLength = sPreparedXids[i].gtrid_length;
        sBQUALLength = sPreparedXids[i].bqual_length;

        CMI_WRITE_CHECK(aCtx, 1 + 24 + ID_MAXXIDDATASIZE);

        CMI_WOP(aCtx, CMP_OP_DB_XaXid);
        CMI_WR8(aCtx, (ULong*)&(sFormatID));
        CMI_WR8(aCtx, (ULong*)&(sGTRIDLength));
        CMI_WR8(aCtx, (ULong*)&(sBQUALLength));
        CMI_WCP(aCtx, sPreparedXids[i].data, ID_MAXXIDDATASIZE);
    }

    for (i = 0; i < sHeuristicXidsCnt; i++)
    {
        sFormatID    = sHeuristicXids[i].formatID;
        sGTRIDLength = sHeuristicXids[i].gtrid_length;
        sBQUALLength = sHeuristicXids[i].bqual_length;

        CMI_WRITE_CHECK(aCtx, 1 + 24 + ID_MAXXIDDATASIZE);

        CMI_WOP(aCtx, CMP_OP_DB_XaXid);
        CMI_WR8(aCtx, (ULong*)&(sFormatID));
        CMI_WR8(aCtx, (ULong*)&(sGTRIDLength));
        CMI_WR8(aCtx, (ULong*)&(sBQUALLength));
        CMI_WCP(aCtx, sHeuristicXids[i].data, ID_MAXXIDDATASIZE);
    }

    if (sPreparedXids != NULL)
    {
        // fix BUG-28267 [codesonar] Ignored Return Value
        IDE_ASSERT(iduMemMgr::free(sPreparedXids) == IDE_SUCCESS);
    }
    if (sHeuristicXids != NULL)
    {
        // fix BUG-28267 [codesonar] Ignored Return Value
        IDE_ASSERT(iduMemMgr::free(sHeuristicXids) == IDE_SUCCESS);
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if (sPreparedXids != NULL)
    {
        // fix BUG-28267 [codesonar] Ignored Return Value
        IDE_ASSERT(iduMemMgr::free(sPreparedXids) == IDE_SUCCESS);
    }
    if (sHeuristicXids != NULL)
    {
        // fix BUG-28267 [codesonar] Ignored Return Value
        IDE_ASSERT(iduMemMgr::free(sHeuristicXids) == IDE_SUCCESS);
    }

    CMI_SET_CURSOR(aCtx, sOrgWriteCursor);

    return IDE_FAILURE;
}


IDE_RC mmtServiceThread::xaOperationProtocol(cmiProtocolContext *aCtx,
                                             cmiProtocol        *,
                                             void               *aSessionOwner,
                                             void               *aUserContext)
{
    mmcTask             *sTask   = (mmcTask *)aSessionOwner;
    mmtServiceThread    *sThread = (mmtServiceThread *)aUserContext;
    mmcSession          *sSession;
    mmdXaContext         sXaContext;

    UChar       sOperation;
    SInt        sRmID;
    SLong       sFlag;
    SLong       sArgument;

    /* PROJ-2160 CM 타입제거
       모두 읽은 다음에 프로토콜을 처리해야 한다. */
    CMI_RD1(aCtx, sOperation);
    CMI_RD4(aCtx, (UInt*)&sRmID);
    CMI_RD8(aCtx, (ULong*)&sFlag);
    CMI_RD8(aCtx, (ULong*)&sArgument);

    IDE_CLEAR();

    IDE_TEST(findSession(sTask, &sSession, sThread) != IDE_SUCCESS);

    IDE_TEST(checkSessionState(sSession, MMC_SESSION_STATE_READY) != IDE_SUCCESS);

    sXaContext.mSession     = sSession;
    sXaContext.mOperation   = sOperation;
    sXaContext.mReturnValue = 0;
    sXaContext.mRmID        = sRmID;
    sXaContext.mFlag        = (vSLong)sFlag;

    switch (sXaContext.mOperation)
    {
        case CMP_DB_XA_OPEN:
            /* every xa session should be non-autocommit */
            /* BUG-20850
            IDE_TEST(sSession->setCommitMode(MMC_COMMITMODE_NONAUTOCOMMIT)
                     != IDE_SUCCESS);
            */
            IDE_TEST(xaOpConnection(aCtx, &sXaContext, mmdXa::open) != IDE_SUCCESS);
            if (sXaContext.mReturnValue != XA_OK )
            {
                sThread->answerErrorResult(aCtx,
                                      CMI_PROTOCOL_OPERATION(DB, XaOperation),
                                      0);
            }
            break;

        case CMP_DB_XA_CLOSE:
            IDE_TEST(xaOpConnection(aCtx, &sXaContext, mmdXa::close) != IDE_SUCCESS);
            if (sXaContext.mReturnValue != XA_OK )
            {
                sThread->answerErrorResult(aCtx,
                                      CMI_PROTOCOL_OPERATION(DB, XaOperation),
                                      0);
            }
            break;

        case CMP_DB_XA_RECOVER:
            IDE_TEST(xaOpRecover(aCtx, &sXaContext, sArgument) != IDE_SUCCESS);
            if (sXaContext.mReturnValue < XA_OK )
            {
                sThread->answerErrorResult(aCtx,
                                      CMI_PROTOCOL_OPERATION(DB, XaOperation),
                                      0);
            }
            break;

        default:
            IDE_RAISE(InvalidXaOperation);
            break;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(InvalidXaOperation);
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_INVALID_XA_OPERATION));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC mmtServiceThread::xaTransactionProtocol(cmiProtocolContext *aCtx,
                                               cmiProtocol        *,
                                               void               *aSessionOwner,
                                               void               *aUserContext)
{
    mmcTask             *sTask   = (mmcTask *)aSessionOwner;
    mmtServiceThread    *sThread = (mmtServiceThread *)aUserContext;
    mmcSession          *sSession;
    mmdXaContext         sXaContext;

    UChar       sOperation;
    SInt        sRmID;
    SLong       sFlag;
    SLong       sArgument;

    /* PROJ-2160 CM 타입제거
       모두 읽은 다음에 프로토콜을 처리해야 한다. */
    CMI_RD1(aCtx, sOperation);
    CMI_RD4(aCtx, (UInt*)&sRmID);
    CMI_RD8(aCtx, (ULong*)&sFlag);
    CMI_RD8(aCtx, (ULong*)&sArgument);

    IDE_CLEAR();

    sXaContext.mReturnValue = XA_OK;
    IDE_TEST(findSession(sTask, &sSession, sThread) != IDE_SUCCESS);

    IDE_TEST(checkSessionState(sSession, MMC_SESSION_STATE_READY) != IDE_SUCCESS);

    sXaContext.mSession     = sSession;
    sXaContext.mOperation   = sOperation;
    sXaContext.mRmID        = sRmID;
    sXaContext.mFlag        = (vSLong)sFlag;

    switch (sXaContext.mOperation)
    {

        case CMP_DB_XA_START:
            IDE_TEST(xaOpTransaction(aCtx,
                                     &sXaContext,
                                     mmdXa::start ) != IDE_SUCCESS);

            if (sXaContext.mReturnValue != XA_OK )
            {
                sThread->answerErrorResult(aCtx,
                                      CMI_PROTOCOL_OPERATION(DB, XaTransaction),
                                      0);
            }
            break;

        case CMP_DB_XA_END:
            IDE_TEST(xaOpTransaction(aCtx,
                                     &sXaContext,
                                     mmdXa::end ) != IDE_SUCCESS);

            if (sXaContext.mReturnValue != XA_OK )
            {
                sThread->answerErrorResult(aCtx,
                                      CMI_PROTOCOL_OPERATION(DB, XaTransaction),
                                      0);
            }
            break;

        case CMP_DB_XA_PREPARE:
            IDE_TEST(xaOpTransaction(aCtx,
                                     &sXaContext,
                                     mmdXa::prepare ) != IDE_SUCCESS);

            /* BUG-42723 */
            if ((sXaContext.mReturnValue != XA_OK) && (sXaContext.mReturnValue != XA_RDONLY))
            {
                sThread->answerErrorResult(aCtx,
                                      CMI_PROTOCOL_OPERATION(DB, XaTransaction),
                                      0);
            }
            else
            {
                /* do nothing */
            }
            break;

        case CMP_DB_XA_COMMIT:
            IDE_TEST(xaOpTransaction(aCtx,
                                     &sXaContext,
                                     mmdXa::commit ) != IDE_SUCCESS);

            if (sXaContext.mReturnValue != XA_OK )
            {
                sThread->answerErrorResult(aCtx,
                                      CMI_PROTOCOL_OPERATION(DB, XaTransaction),
                                      0);
            }
            break;

        case CMP_DB_XA_ROLLBACK:
            IDE_TEST(xaOpTransaction(aCtx,
                                     &sXaContext,
                                     mmdXa::rollback ) != IDE_SUCCESS);

            if (sXaContext.mReturnValue != XA_OK )
            {
                sThread->answerErrorResult(aCtx,
                                      CMI_PROTOCOL_OPERATION(DB, XaTransaction),
                                      0);
            }
            break;

        case CMP_DB_XA_FORGET:
            IDE_TEST(xaOpTransaction(aCtx,
                                     &sXaContext,
                                     mmdXa::forget ) != IDE_SUCCESS);

            if (sXaContext.mReturnValue != XA_OK )
            {
                sThread->answerErrorResult(aCtx,
                                      CMI_PROTOCOL_OPERATION(DB, XaTransaction),
                                      0);
            }
            break;

        case CMP_DB_XA_HEURISTIC_COMPLETED:
            IDE_TEST(xaOpTransaction(aCtx,
                                     &sXaContext,
                                     mmdXa::heuristicCompleted)
                    != IDE_SUCCESS);

            if (sXaContext.mReturnValue != XA_OK)
            {
                (void)sThread->answerErrorResult(
                        aCtx,
                        CMI_PROTOCOL_OPERATION(DB, XaTransaction),
                        0);
            }
            else
            {
                /* nothing to do */
            }
            break;

        default:
            IDE_RAISE(InvalidXaOperation);
            break;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(InvalidXaOperation);
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_INVALID_XA_OPERATION));
    }
    IDE_EXCEPTION_END;

    /* bug-36037: invalid xid
       invalid xid의 경우 client로 에러응답을 먼저주고 끊는다 */
    if (sXaContext.mReturnValue != XA_OK )
    {
        sThread->answerErrorResult(aCtx,
                CMI_PROTOCOL_OPERATION(DB, XaTransaction),
                0);
    }

    return IDE_FAILURE;
}
