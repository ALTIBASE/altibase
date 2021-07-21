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

static IDE_RC answerXaResultA5(cmiProtocolContext *aProtocolContext, mmdXaContext *aXaContext)
{
    cmiProtocol       sProtocol;
    cmpArgDBXaResultA5 *sArg;

    CMI_PROTOCOL_INITIALIZE(sProtocol, sArg, DB, XaResult);

    sArg->mOperation   = aXaContext->mOperation;
    sArg->mReturnValue = aXaContext->mReturnValue;

    IDE_TEST(cmiWriteProtocol(aProtocolContext, &sProtocol) != IDE_SUCCESS);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

static IDE_RC xaOpConnectionA5(cmiProtocolContext  *aProtocolContext,
                             mmdXaContext        *aXaContext,
                             mmdXaConnectionFunc  aXaFunc)
{
    aXaFunc(aXaContext);

    return answerXaResultA5(aProtocolContext, aXaContext);
}

static IDE_RC xaOpTransactionA5(cmiProtocolContext   *aProtocolContext,
                              mmdXaContext         *aXaContext,
                              mmdXaTransactionFunc  aXaFunc,
                              cmpArgDBXaTransactionA5 * aArg)
{
    /* BUG-18981 */
    ID_XID      sXid;
    UInt        sDataLen;
    
    sXid.formatID     = aArg->mFormatID;
    sXid.gtrid_length = aArg->mGTRIDLength;
    sXid.bqual_length = aArg->mBQUALLength;

    /* bug-36037: invalid xid
       invalid xid 관련 서버쪽 검사코드 추가 */
    if ((sXid.gtrid_length <= (vSLong) 0)               ||
        (sXid.gtrid_length >  (vSLong) ID_MAXGTRIDSIZE) ||
        (sXid.bqual_length <= (vSLong) 0)               ||
        (sXid.bqual_length >  (vSLong) ID_MAXBQUALSIZE))
    {
        IDE_RAISE(InvalidXid);
    }

    sDataLen = cmtVariableGetSize(&(aArg->mData));
    IDE_TEST_RAISE(sDataLen > ID_MAXXIDDATASIZE, InvalidXidDataSize);
    
    IDE_TEST(cmtVariableGetData(&(aArg->mData), (UChar *)&(sXid.data), sDataLen)
             != IDE_SUCCESS);
    /* BUG-20726 */
    idlOS::memset(&(sXid.data[sXid.gtrid_length+sXid.bqual_length]),
                  0x00,
                  ID_MAXXIDDATASIZE-sXid.gtrid_length-sXid.bqual_length);

    aXaFunc(aXaContext, &sXid);

    return answerXaResultA5(aProtocolContext, aXaContext);

    /* bug-36037: invalid xid */
    IDE_EXCEPTION(InvalidXid);
    {
        IDE_SET(ideSetErrorCode( mmERR_ABORT_INVALID_XID ));
        aXaContext->mReturnValue = XAER_NOTA;
    }
    IDE_EXCEPTION(InvalidXidDataSize);
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_INVALID_XA_XID_DATA_SIZE));
        aXaContext->mReturnValue = XAER_NOTA;
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC xaOpRecoverA5(cmiProtocolContext *aProtocolContext,
                          mmdXaContext       *aXaContext,
                          vSLong              /*aCount*/)
{
    /* BUG-18981 */
    ID_XID  *sPreparedXids = NULL;
    ID_XID  *sHeuristicXids = NULL;
    SInt     i;
    SInt     sPreparedXidsCnt = 0;
    SInt     sHeuristicXidsCnt = 0;

    mmdXa::recover(aXaContext, &sPreparedXids, &sPreparedXidsCnt,
                   &sHeuristicXids, &sHeuristicXidsCnt);

    /* BUG-19070 */    
    IDE_TEST(answerXaResultA5(aProtocolContext, aXaContext) != IDE_SUCCESS);

    for (i = 0; i < sPreparedXidsCnt; i++)
    {
        IDE_TEST(cmxXidWrite(aProtocolContext, &sPreparedXids[i]) != IDE_SUCCESS);
    }
    for (i = 0; i < sHeuristicXidsCnt; i++)
    {
        IDE_TEST(cmxXidWrite(aProtocolContext, &sHeuristicXids[i]) != IDE_SUCCESS);
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
    {
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
    }

    return IDE_FAILURE;
}


IDE_RC mmtServiceThread::xaOperationProtocolA5(cmiProtocolContext *aProtocolContext,
                                             cmiProtocol        *aProtocol,
                                             void               *aSessionOwner,
                                             void               *aUserContext)
{
    cmpArgDBXaOperationA5 *sArg    = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, XaOperation);
    mmcTask             *sTask   = (mmcTask *)aSessionOwner;
    mmtServiceThread    *sThread = (mmtServiceThread *)aUserContext;
    mmcSession          *sSession;
    mmdXaContext         sXaContext;

    IDE_CLEAR();

    IDE_TEST(findSession(sTask, &sSession, sThread) != IDE_SUCCESS);

    IDE_TEST(checkSessionState(sSession, MMC_SESSION_STATE_READY) != IDE_SUCCESS);

    sXaContext.mSession     = sSession;
    sXaContext.mOperation   = sArg->mOperation;
    sXaContext.mReturnValue = 0;
    sXaContext.mRmID        = sArg->mRmID;
    sXaContext.mFlag        = (vSLong)sArg->mFlag;

    switch (sXaContext.mOperation)
    {
        case CMP_DB_XA_OPEN:
            /* every xa session should be non-autocommit */
            /* BUG-20850
            IDE_TEST(sSession->setCommitMode(MMC_COMMITMODE_NONAUTOCOMMIT)
                     != IDE_SUCCESS);
            */
            IDE_TEST(xaOpConnectionA5(aProtocolContext, &sXaContext, mmdXa::open) != IDE_SUCCESS);
            if (sXaContext.mReturnValue != XA_OK )
            {
                sThread->answerErrorResultA5(aProtocolContext,
                                      CMI_PROTOCOL_OPERATION(DB, XaOperation),
                                      0);
            }
            break;

        case CMP_DB_XA_CLOSE:
            IDE_TEST(xaOpConnectionA5(aProtocolContext, &sXaContext, mmdXa::close) != IDE_SUCCESS);
            if (sXaContext.mReturnValue != XA_OK )
            {
                sThread->answerErrorResultA5(aProtocolContext,
                                      CMI_PROTOCOL_OPERATION(DB, XaOperation),
                                      0);
            }
            break;

        case CMP_DB_XA_RECOVER:
            IDE_TEST(xaOpRecoverA5(aProtocolContext, &sXaContext, sArg->mArgument) != IDE_SUCCESS);
            if (sXaContext.mReturnValue < XA_OK )
            {
                sThread->answerErrorResultA5(aProtocolContext,
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


IDE_RC mmtServiceThread::xaTransactionProtocolA5(cmiProtocolContext *aProtocolContext,
                                             cmiProtocol        *aProtocol,
                                             void               *aSessionOwner,
                                             void               *aUserContext)
{
    cmpArgDBXaTransactionA5 *sArg    = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, XaTransaction);
    mmcTask             *sTask   = (mmcTask *)aSessionOwner;
    mmtServiceThread    *sThread = (mmtServiceThread *)aUserContext;
    mmcSession          *sSession;
    mmdXaContext         sXaContext;

    IDE_CLEAR();

    sXaContext.mReturnValue = XA_OK;
    IDE_TEST(findSession(sTask, &sSession, sThread) != IDE_SUCCESS);

    IDE_TEST(checkSessionState(sSession, MMC_SESSION_STATE_READY) != IDE_SUCCESS);
    
    sXaContext.mSession     = sSession;
    sXaContext.mOperation   = sArg->mOperation;
    sXaContext.mRmID        = sArg->mRmID;
    sXaContext.mFlag        = (vSLong)sArg->mFlag;

    switch (sXaContext.mOperation)
    {

        case CMP_DB_XA_START:
            IDE_TEST(xaOpTransactionA5(aProtocolContext, &sXaContext,
                                     mmdXa::start, sArg) != IDE_SUCCESS);
            if (sXaContext.mReturnValue != XA_OK )
            {
                sThread->answerErrorResultA5(aProtocolContext,
                                      CMI_PROTOCOL_OPERATION(DB, XaTransaction),
                                      0);
            }
            break;
            
        case CMP_DB_XA_END:
            IDE_TEST(xaOpTransactionA5(aProtocolContext, &sXaContext,
                                     mmdXa::end, sArg) != IDE_SUCCESS);
            if (sXaContext.mReturnValue != XA_OK )
            {
                sThread->answerErrorResultA5(aProtocolContext,
                                      CMI_PROTOCOL_OPERATION(DB, XaTransaction),
                                      0);
            }
            break;
            
        case CMP_DB_XA_PREPARE:
            IDE_TEST(xaOpTransactionA5(aProtocolContext, &sXaContext,
                                     mmdXa::prepare, sArg) != IDE_SUCCESS);

            /* BUG-42723 */
            if ((sXaContext.mReturnValue != XA_OK) && (sXaContext.mReturnValue != XA_RDONLY))
            {
                sThread->answerErrorResultA5(aProtocolContext,
                                      CMI_PROTOCOL_OPERATION(DB, XaTransaction),
                                      0);
            }
            else
            {
                /* do nothing */
            }
            break;
            
        case CMP_DB_XA_COMMIT:
            IDE_TEST(xaOpTransactionA5(aProtocolContext, &sXaContext,
                                     mmdXa::commit, sArg) != IDE_SUCCESS);
            if (sXaContext.mReturnValue != XA_OK )
            {
                sThread->answerErrorResultA5(aProtocolContext,
                                      CMI_PROTOCOL_OPERATION(DB, XaTransaction),
                                      0);
            }
            break;
            
        case CMP_DB_XA_ROLLBACK:
            IDE_TEST(xaOpTransactionA5(aProtocolContext, &sXaContext,
                                     mmdXa::rollback, sArg) != IDE_SUCCESS);
            if (sXaContext.mReturnValue != XA_OK )
            {
                sThread->answerErrorResultA5(aProtocolContext,
                                      CMI_PROTOCOL_OPERATION(DB, XaTransaction),
                                      0);
            }
            break;
            
        case CMP_DB_XA_FORGET:
            IDE_TEST(xaOpTransactionA5(aProtocolContext, &sXaContext,
                                     mmdXa::forget, sArg) != IDE_SUCCESS);
            if (sXaContext.mReturnValue != XA_OK )
            {
                sThread->answerErrorResultA5(aProtocolContext,
                                      CMI_PROTOCOL_OPERATION(DB, XaTransaction),
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

    /* bug-36037: invalid xid
       invalid xid의 경우 client로 에러응답을 먼저주고 끊는다 */
    if (sXaContext.mReturnValue != XA_OK )
    {
        sThread->answerErrorResultA5(aProtocolContext,
                CMI_PROTOCOL_OPERATION(DB, XaTransaction),
                0);
    }

    return IDE_FAILURE;
}
