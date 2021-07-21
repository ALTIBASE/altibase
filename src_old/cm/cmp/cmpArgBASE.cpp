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

cmpOpMap gCmpOpBaseMap[] =
{
    {"CMP_OP_BASE_Error"               } ,
    {"CMP_OP_BASE_Handshake"           } ,
    {"CMP_OP_BASE_SecureVersion"       } ,
    {"CMP_OP_BASE_SecureVersionResult" } ,
    {"CMP_OP_BASE_SecureEnvelope"      } ,
    {"CMP_OP_BASE_SecureEnvelopeResult"} ,
    {"CMP_OP_BASE_MAX_VER1"            }
};

IDE_RC cmpArgInitializeBASEError(cmpProtocol *aProtocol)
{
    cmpArgBASEError *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, BASE, Error);

    IDE_TEST(cmtVariableInitialize(&sArg->mErrorMessage) != IDE_SUCCESS);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC cmpArgFinalizeBASEError(cmpProtocol *aProtocol)
{
    cmpArgBASEError *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, BASE, Error);

    IDE_TEST(cmtVariableFinalize(&sArg->mErrorMessage) != IDE_SUCCESS);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC cmpArgInitializeBASESecureVersion(cmpProtocol *aProtocol)
{
    cmpArgBASESecureVersion *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, BASE, SecureVersion);

    IDE_TEST(cmtVariableInitialize(&sArg->mSecureVersion) != IDE_SUCCESS);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC cmpArgFinalizeBASESecureVersion(cmpProtocol *aProtocol)
{
    cmpArgBASESecureVersion *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, BASE, SecureVersion);

    IDE_TEST(cmtVariableFinalize(&sArg->mSecureVersion) != IDE_SUCCESS);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC cmpArgInitializeBASESecureVersionResult(cmpProtocol *aProtocol)
{
    cmpArgBASESecureVersionResult *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, BASE, SecureVersionResult);

    IDE_TEST(cmtVariableInitialize(&sArg->mRandomValue) != IDE_SUCCESS);
    IDE_TEST(cmtVariableInitialize(&sArg->mSvrCertificate) != IDE_SUCCESS);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC cmpArgFinalizeBASESecureVersionResult(cmpProtocol *aProtocol)
{
    cmpArgBASESecureVersionResult *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, BASE, SecureVersionResult);

    IDE_TEST(cmtVariableFinalize(&sArg->mRandomValue) != IDE_SUCCESS);
    IDE_TEST(cmtVariableFinalize(&sArg->mSvrCertificate) != IDE_SUCCESS);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC cmpArgInitializeBASESecureEnvelope(cmpProtocol *aProtocol)
{
    cmpArgBASESecureEnvelope *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, BASE, SecureEnvelope);

    IDE_TEST(cmtVariableInitialize(&sArg->mEnvelope) != IDE_SUCCESS);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC cmpArgFinalizeBASESecureEnvelope(cmpProtocol *aProtocol)
{
    cmpArgBASESecureEnvelope *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, BASE, SecureEnvelope);

    IDE_TEST(cmtVariableFinalize(&sArg->mEnvelope) != IDE_SUCCESS);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC cmpArgInitializeBASESecureEnvelopeResult(cmpProtocol *aProtocol)
{
    cmpArgBASESecureEnvelopeResult *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, BASE, SecureEnvelopeResult);

    IDE_TEST(cmtVariableInitialize(&sArg->mValue) != IDE_SUCCESS);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC cmpArgFinalizeBASESecureEnvelopeResult(cmpProtocol *aProtocol)
{
    cmpArgBASESecureEnvelopeResult *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, BASE, SecureEnvelopeResult);

    IDE_TEST(cmtVariableFinalize(&sArg->mValue) != IDE_SUCCESS);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

cmpArgFunction gCmpArgInitializeFunctionBASE[CMP_OP_BASE_MAX] =
{
    cmpArgInitializeBASEError,                /* CMP_OP_BASE_Error */
    cmpArgNULL,                               /* CMP_OP_BASE_Handshake */
    cmpArgInitializeBASESecureVersion,        /* CMP_OP_BASE_SecureVersion */
    cmpArgInitializeBASESecureVersionResult,  /* CMP_OP_BASE_SecureVersionResult */
    cmpArgInitializeBASESecureEnvelope,       /* CMP_OP_BASE_Envelope */
    cmpArgInitializeBASESecureEnvelopeResult, /* CMP_OP_BASE_EnvelopeResult */
};

cmpArgFunction gCmpArgFinalizeFunctionBASE[CMP_OP_BASE_MAX] =
{
    cmpArgFinalizeBASEError,                  /* CMP_OP_BASE_Error */
    cmpArgNULL,                               /* CMP_OP_BASE_Handshake */
    cmpArgFinalizeBASESecureVersion,          /* CMP_OP_BASE_SecureVersion */
    cmpArgFinalizeBASESecureVersionResult,    /* CMP_OP_BASE_SecureVersionResult */
    cmpArgFinalizeBASESecureEnvelope,         /* CMP_OP_BASE_Envelope */
    cmpArgFinalizeBASESecureEnvelopeResult,   /* CMP_OP_BASE_EnvelopeResult */
};
