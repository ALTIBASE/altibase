/**
 *  Copyright (c) 1999~2017, Altibase Corp. and/or its affiliates. All rights reserved.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License, version 3,
 *  as published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <cmAll.h>


static IDE_RC answerError(cmiProtocolContext *aProtocolContext)
{
    UInt             sErrorCode = ideGetErrorCode();
    SChar           *sErrorMsg  = ideGetErrorMsg(sErrorCode);
    cmiProtocol      sProtocol;
    cmpArgBASEError *sArg;

    CMI_PROTOCOL_INITIALIZE(sProtocol, sArg, BASE, Error);

    sArg->mErrorCode  = sErrorCode;

    if (sErrorMsg != NULL)
    {
        IDE_TEST(cmtVariableSetData(&sArg->mErrorMessage,
                                    (UChar *)sErrorMsg,
                                    idlOS::strlen(sErrorMsg)) != IDE_SUCCESS);
    }

    IDE_TEST(cmiWriteProtocol(aProtocolContext, &sProtocol) != IDE_SUCCESS);

    IDE_TEST(cmiFlushProtocol(aProtocolContext, ID_TRUE) != IDE_SUCCESS);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

static IDE_RC answerHandshake(cmiProtocolContext *aProtocolContext,
                              UChar               aBaseVersion,
                              UChar               aModuleID,
                              UChar               aModuleVersion)
{
    cmiProtocol          sProtocol;
    cmpArgBASEHandshake *sArg;

    CMI_PROTOCOL_INITIALIZE(sProtocol, sArg, BASE, Handshake);

    sArg->mBaseVersion   = aBaseVersion;
    sArg->mModuleID      = aModuleID;
    sArg->mModuleVersion = aModuleVersion;

    IDE_TEST(cmiWriteProtocol(aProtocolContext, &sProtocol) != IDE_SUCCESS);

    IDE_TEST(cmiFlushProtocol(aProtocolContext, ID_TRUE) != IDE_SUCCESS);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC cmpCallbackBASEError(cmiProtocolContext * /*aProtocolContext*/,
                            cmiProtocol        * /*aProtocol*/,
                            void               * /*aSessionOwner*/,
                            void               * /*aUserContext*/)
{
    /*
     * BUGBUG: 에러처리를 해야하나 일단 무시
     */

    IDE_ASSERT(0);

    return IDE_SUCCESS;
}

IDE_RC cmpCallbackBASEHandshake(cmiProtocolContext *aProtocolContext,
                                cmiProtocol        *aProtocol,
                                void               * /*aSessionOwner*/,
                                void               * /*aUserContext*/)
{
    cmpArgBASEHandshake *sArg     = CMI_PROTOCOL_GET_ARG(*aProtocol, BASE, Handshake);
    cmmSession          *sSession = aProtocolContext->mSession;
    UChar                sBaseVersion;
    UChar                sModuleVersion;

    
    /*
     * Module ID 검사
     */
    if (sSession != NULL)
    {
        IDE_TEST_RAISE(sSession->mModuleID != sArg->mModuleID, InvalidModule);
    }
    else
    {
        IDE_TEST_RAISE((sArg->mModuleID == CMP_MODULE_BASE) || (sArg->mModuleID >= CMP_MODULE_MAX),
                       InvalidModule);
    }

    /*
     * 사용할 Version 세팅
     */
    sBaseVersion   = IDL_MIN(sArg->mBaseVersion, gCmpModule[CMP_MODULE_BASE]->mVersionMax - 1);
    sModuleVersion = IDL_MIN(sArg->mModuleVersion, gCmpModule[sArg->mModuleID]->mVersionMax - 1);

    /*
     * Session에 Version 세팅
     */
    if (sSession != NULL)
    {
        sSession->mBaseVersion   = sBaseVersion;
        sSession->mModuleVersion = sModuleVersion;
    }

    return answerHandshake(aProtocolContext, sBaseVersion, sArg->mModuleID, sModuleVersion);

    IDE_EXCEPTION(InvalidModule);
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_INVALID_MODULE));
    }
    IDE_EXCEPTION_END;

    return answerError(aProtocolContext);
}

