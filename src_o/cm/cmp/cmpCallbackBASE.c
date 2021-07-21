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

#include <cmAllClient.h>

static ACI_RC answerError(cmiProtocolContext *aProtocolContext)
{
    acp_uint32_t     sErrorCode = aciGetErrorCode();
    acp_char_t      *sErrorMsg = aciGetErrorMsg(sErrorCode);
    cmiProtocol      sProtocol;
    cmpArgBASEError *sArg;

    CMI_PROTOCOL_INITIALIZE(sProtocol, sArg, BASE, Error);

    sArg->mErrorCode  = sErrorCode;

    if (sErrorMsg != NULL)
    {
        ACI_TEST(cmtVariableSetData(&sArg->mErrorMessage,
                                    (acp_uint8_t *)sErrorMsg,
                                    acpCStrLen(sErrorMsg, 2048)) != ACP_RC_SUCCESS);
    }

    ACI_TEST(cmiWriteProtocol(aProtocolContext, &sProtocol) != ACP_RC_SUCCESS);

    ACI_TEST(cmiFlushProtocol(aProtocolContext, ACP_TRUE) != ACP_RC_SUCCESS);

    return ACI_SUCCESS;
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

static ACI_RC answerHandshake(cmiProtocolContext *aProtocolContext,
                              acp_uint8_t           aBaseVersion,
                              acp_uint8_t           aModuleID,
                              acp_uint8_t           aModuleVersion)
{
    cmiProtocol          sProtocol;
    cmpArgBASEHandshake *sArg;

    CMI_PROTOCOL_INITIALIZE(sProtocol, sArg, BASE, Handshake);

    sArg->mBaseVersion   = aBaseVersion;
    sArg->mModuleID      = aModuleID;
    sArg->mModuleVersion = aModuleVersion;

    ACI_TEST(cmiWriteProtocol(aProtocolContext, &sProtocol) != ACP_RC_SUCCESS);

    ACI_TEST(cmiFlushProtocol(aProtocolContext, ACP_TRUE) != ACP_RC_SUCCESS);

    return ACI_SUCCESS;
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

ACI_RC cmpCallbackBASEError(cmiProtocolContext *aProtocolContext,
                            cmiProtocol        *aProtocol,
                            void               *aSessionOwner,
                            void               *aUserContext)
{
    ACP_UNUSED(aProtocolContext);
    ACP_UNUSED(aProtocol);
    ACP_UNUSED(aSessionOwner);
    ACP_UNUSED(aUserContext);

    ACE_ASSERT(0);

    return ACI_SUCCESS;
}

ACI_RC cmpCallbackBASEHandshake(cmiProtocolContext *aProtocolContext,
                                cmiProtocol        *aProtocol,
                                void               *aSessionOwner,
                                void               *aUserContext)
{
    cmpArgBASEHandshake *sArg     = CMI_PROTOCOL_GET_ARG(*aProtocol, BASE, Handshake);
    cmmSession          *sSession = aProtocolContext->mSession;
    acp_uint8_t          sBaseVersion;
    acp_uint8_t          sModuleVersion;

    ACP_UNUSED(aSessionOwner);
    ACP_UNUSED(aUserContext);

    /*
     * Module ID 검사
     */
    if (sSession != NULL)
    {
        ACI_TEST_RAISE(sSession->mModuleID != sArg->mModuleID, InvalidModule);
    }
    else
    {
        ACI_TEST_RAISE((sArg->mModuleID == CMP_MODULE_BASE) ||
                       (sArg->mModuleID >= CMP_MODULE_MAX),
                       InvalidModule);
    }

    /*
     * 사용할 Version 세팅
     */
    sBaseVersion   = ACP_MIN(sArg->mBaseVersion, gCmpModuleClient[CMP_MODULE_BASE]->mVersionMax - 1);
    sModuleVersion = ACP_MIN(sArg->mModuleVersion, gCmpModuleClient[sArg->mModuleID]->mVersionMax - 1);

    /*
     * Session에 Version 세팅
     */
    if (sSession != NULL)
    {
        sSession->mBaseVersion   = sBaseVersion;
        sSession->mModuleVersion = sModuleVersion;
    }

    return answerHandshake(aProtocolContext, sBaseVersion, sArg->mModuleID, sModuleVersion);

    ACI_EXCEPTION(InvalidModule);
    {
        ACI_SET(aciSetErrorCode(cmERR_ABORT_INVALID_MODULE));
    }
    ACI_EXCEPTION_END;

    return answerError(aProtocolContext);
}

