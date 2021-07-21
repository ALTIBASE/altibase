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

static IDE_RC answerTransactionResult(cmiProtocolContext *aProtocolContext)
{
    cmiProtocol                sProtocol;
    cmpArgDBTransactionResultA5 *sArg;

    CMI_PROTOCOL_INITIALIZE(sProtocol, sArg, DB, TransactionResult);

    ACP_UNUSED(sArg);

    IDE_TEST(cmiWriteProtocol(aProtocolContext, &sProtocol) != IDE_SUCCESS);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC mmtServiceThread::transactionProtocolA5(cmiProtocolContext *aProtocolContext,
                                             cmiProtocol        *aProtocol,
                                             void               *aSessionOwner,
                                             void               *aUserContext)
{
    cmpArgDBTransactionA5 *sArg  = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, Transaction);
    mmcTask             *sTask = (mmcTask *)aSessionOwner;
    mmtServiceThread    *sThread = (mmtServiceThread *)aUserContext;
    mmcSession          *sSession;

    IDE_CLEAR();

    IDE_TEST(findSession(sTask, &sSession, sThread) != IDE_SUCCESS);

    IDE_TEST(checkSessionState(sSession, MMC_SESSION_STATE_SERVICE) != IDE_SUCCESS);

    /* BUG-20832 */
     IDE_TEST_RAISE(sSession->getXaAssocState() != MMD_XA_ASSOC_STATE_NOTASSOCIATED,
                   DCLNotAllowedError);

    switch (sArg->mOperation)
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

    return answerTransactionResult(aProtocolContext);

    IDE_EXCEPTION(DCLNotAllowedError);
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_NOT_ALLOWED_DCL));
    }
    IDE_EXCEPTION(InvalidOperation);
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_INVALID_ERROR));
    }
    IDE_EXCEPTION_END;

    return sThread->answerErrorResultA5(aProtocolContext,
                                      CMI_PROTOCOL_OPERATION(DB, Transaction),
                                      0);
}
