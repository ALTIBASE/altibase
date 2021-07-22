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
#include <mmtServiceThread.h>
#include <mmuProperty.h>


static IDE_RC answerErrorInfoResult(cmiProtocolContext *aProtocolContext, UInt aErrorCode)
{
    cmiProtocol              sProtocol;
    cmpArgDBErrorInfoResultA5 *sArg;
    SChar                   *sErrorMsg = ideGetErrorMsg(aErrorCode);

    CMI_PROTOCOL_INITIALIZE(sProtocol, sArg, DB, ErrorInfoResult);

    sArg->mErrorCode = aErrorCode;

    if (sErrorMsg != NULL)
    {
        IDE_TEST(cmtVariableSetData(&sArg->mErrorMessage,
                                    (UChar *)sErrorMsg,
                                    idlOS::strlen(sErrorMsg)) != IDE_SUCCESS);
    }

    IDE_TEST(cmiWriteProtocol(aProtocolContext, &sProtocol) != IDE_SUCCESS);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}


IDE_RC mmtServiceThread::answerErrorResultA5(cmiProtocolContext *aProtocolContext,
                                           UChar               aOperationID,
                                           UShort              aErrorIndex)
{
    UInt                 sErrorCode;
    ideErrorMgr         *sErrorMgr  = ideGetErrorMgr();
    SChar               *sErrorMsg;
    cmiProtocol          sProtocol;
    cmpArgDBErrorResultA5 *sErrorArg;
    SChar                sMessage[4096];

    IDE_TEST_RAISE(mErrorFlag == ID_TRUE, ErrorAlreadySent);
    //fix [BUG-27123 : mm/NEW] [5.3.3 release Code-Sonar] mm Ignore return values series 2
    sErrorCode = ideGetErrorCode();
    if ((sErrorCode & E_ACTION_MASK) != E_ACTION_IGNORE)
    {
        if ((iduProperty::getSourceInfo() & IDE_SOURCE_INFO_CLIENT) == 0)
        {
            sErrorMsg = ideGetErrorMsg(sErrorCode);
        }
        else
        {
            idlOS::snprintf(sMessage,
                            ID_SIZEOF(sMessage),
                            "(%s:%" ID_INT32_FMT ") %s",
                            sErrorMgr->Stack.LastErrorFile,
                            sErrorMgr->Stack.LastErrorLine,
                            sErrorMgr->Stack.LastErrorMsg);

            sErrorMsg = sMessage;
        }
    }
    else
    {
        sErrorMsg = NULL;
    }

    CMI_PROTOCOL_INITIALIZE(sProtocol, sErrorArg, DB, ErrorResult);

    sErrorArg->mOperationID = aOperationID;
    sErrorArg->mErrorIndex  = aErrorIndex;
    sErrorArg->mErrorCode   = sErrorCode;

    if (sErrorMsg != NULL)
    {
        IDE_TEST(cmtVariableSetData(&sErrorArg->mErrorMessage,
                                    (UChar *)sErrorMsg,
                                    idlOS::strlen(sErrorMsg)) != IDE_SUCCESS);
    }

    IDE_TEST(cmiWriteProtocol(aProtocolContext, &sProtocol) != IDE_SUCCESS);

    switch (sErrorCode)
    {
        case idERR_ABORT_Session_Closed:
        case idERR_ABORT_Session_Disconnected:
            IDE_RAISE(SessionClosed);
            break;

        default:
            break;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ErrorAlreadySent);

    return IDE_SUCCESS;

    IDE_EXCEPTION(SessionClosed);

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mmtServiceThread::errorInfoProtocolA5(cmiProtocolContext *aProtocolContext,
                                           cmiProtocol        *aProtocol,
                                           void               * /*aSessionOwner*/,
                                           void               * /*aUserContext*/)
{
    cmpArgDBErrorInfoA5 *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, DB, ErrorInfo);

    return answerErrorInfoResult(aProtocolContext, sArg->mErrorCode);
}

IDE_RC mmtServiceThread::invalidProtocolA5(cmiProtocolContext *aProtocolContext,
                                         cmiProtocol        *aProtocol,
                                         void               * /*aSessionOwner*/,
                                         void               *aUserContext)
{
    mmtServiceThread *sThread = (mmtServiceThread *)aUserContext;

    IDE_SET(ideSetErrorCode(mmERR_ABORT_INVALID_ERROR));

    return sThread->answerErrorResultA5(aProtocolContext, aProtocol->mOpID, 0);
}
