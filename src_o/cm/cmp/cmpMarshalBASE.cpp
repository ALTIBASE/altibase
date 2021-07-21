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


IDE_RC cmpReadBASEError(cmbBlock *aBlock, cmpProtocol *aProtocol, cmpMarshalState *aMarshalState)
{
    cmpArgBASEError *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, BASE, Error);

    CMP_MARSHAL_BEGIN(2);
    CMP_MARSHAL_READ_UINT(sArg->mErrorCode);

    CMP_MARSHAL_STAGE(1);
    CMP_MARSHAL_READ_VARIABLE(sArg->mErrorMessage);

    CMP_MARSHAL_STAGE(0);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC cmpWriteBASEError(cmbBlock *aBlock, cmpProtocol *aProtocol, cmpMarshalState *aMarshalState)
{
    cmpArgBASEError *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, BASE, Error);

    CMP_MARSHAL_BEGIN(2);
    CMP_MARSHAL_WRITE_UINT(sArg->mErrorCode);

    CMP_MARSHAL_STAGE(1);
    CMP_MARSHAL_WRITE_VARIABLE(sArg->mErrorMessage);

    CMP_MARSHAL_STAGE(0);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC cmpReadBASEHandshake(cmbBlock *aBlock, cmpProtocol *aProtocol, cmpMarshalState *aMarshalState)
{
    cmpArgBASEHandshake *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, BASE, Handshake);

    CMP_MARSHAL_BEGIN(3);
    CMP_MARSHAL_READ_UCHAR(sArg->mBaseVersion);

    CMP_MARSHAL_STAGE(2);
    CMP_MARSHAL_READ_UCHAR(sArg->mModuleID);

    CMP_MARSHAL_STAGE(1);
    CMP_MARSHAL_READ_UCHAR(sArg->mModuleVersion);

    CMP_MARSHAL_STAGE(0);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC cmpWriteBASEHandshake(cmbBlock *aBlock, cmpProtocol *aProtocol, cmpMarshalState *aMarshalState)
{
    cmpArgBASEHandshake *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, BASE, Handshake);

    CMP_MARSHAL_BEGIN(3);
    CMP_MARSHAL_WRITE_UCHAR(sArg->mBaseVersion);

    CMP_MARSHAL_STAGE(2);
    CMP_MARSHAL_WRITE_UCHAR(sArg->mModuleID);

    CMP_MARSHAL_STAGE(1);
    CMP_MARSHAL_WRITE_UCHAR(sArg->mModuleVersion);

    CMP_MARSHAL_STAGE(0);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC cmpReadBASESecureVersion(cmbBlock *aBlock, cmpProtocol *aProtocol, cmpMarshalState *aMarshalState)
{
    cmpArgBASESecureVersion *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, BASE, SecureVersion);

    CMP_MARSHAL_BEGIN(1);
    CMP_MARSHAL_READ_VARIABLE(sArg->mSecureVersion);

    CMP_MARSHAL_STAGE(0);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC cmpWriteBASESecureVersion(cmbBlock *aBlock, cmpProtocol *aProtocol, cmpMarshalState *aMarshalState)
{
    cmpArgBASESecureVersion *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, BASE, SecureVersion);

    CMP_MARSHAL_BEGIN(1);
    CMP_MARSHAL_WRITE_VARIABLE(sArg->mSecureVersion);

    CMP_MARSHAL_STAGE(0);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC cmpReadBASESecureVersionResult(cmbBlock *aBlock, cmpProtocol *aProtocol, cmpMarshalState *aMarshalState)
{
    cmpArgBASESecureVersionResult *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, BASE, SecureVersionResult);

    CMP_MARSHAL_BEGIN(2);
    CMP_MARSHAL_READ_VARIABLE(sArg->mRandomValue);

    CMP_MARSHAL_STAGE(1);
    CMP_MARSHAL_READ_VARIABLE(sArg->mSvrCertificate);

    CMP_MARSHAL_STAGE(0);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC cmpWriteBASESecureVersionResult(cmbBlock *aBlock, cmpProtocol *aProtocol, cmpMarshalState *aMarshalState)
{
    cmpArgBASESecureVersionResult *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, BASE, SecureVersionResult);

    CMP_MARSHAL_BEGIN(2);
    CMP_MARSHAL_WRITE_VARIABLE(sArg->mRandomValue);

    CMP_MARSHAL_STAGE(1);
    CMP_MARSHAL_WRITE_VARIABLE(sArg->mSvrCertificate);

    CMP_MARSHAL_STAGE(0);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC cmpReadBASESecureEnvelope(cmbBlock *aBlock, cmpProtocol *aProtocol, cmpMarshalState *aMarshalState)
{
    cmpArgBASESecureEnvelope *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, BASE, SecureEnvelope);

    CMP_MARSHAL_BEGIN(1);
    CMP_MARSHAL_READ_VARIABLE(sArg->mEnvelope);

    CMP_MARSHAL_STAGE(0);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC cmpWriteBASESecureEnvelope(cmbBlock *aBlock, cmpProtocol *aProtocol, cmpMarshalState *aMarshalState)
{
    cmpArgBASESecureEnvelope *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, BASE, SecureEnvelope);

    CMP_MARSHAL_BEGIN(1);
    CMP_MARSHAL_WRITE_VARIABLE(sArg->mEnvelope);

    CMP_MARSHAL_STAGE(0);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC cmpReadBASESecureEnvelopeResult(cmbBlock *aBlock, cmpProtocol *aProtocol, cmpMarshalState *aMarshalState)
{
    cmpArgBASESecureEnvelopeResult *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, BASE, SecureEnvelopeResult);

    CMP_MARSHAL_BEGIN(1);
    CMP_MARSHAL_READ_VARIABLE(sArg->mValue);

    CMP_MARSHAL_STAGE(0);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC cmpWriteBASESecureEnvelopeResult(cmbBlock *aBlock, cmpProtocol *aProtocol, cmpMarshalState *aMarshalState)
{
    cmpArgBASESecureEnvelopeResult *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, BASE, SecureEnvelopeResult);

    CMP_MARSHAL_BEGIN(1);
    CMP_MARSHAL_WRITE_VARIABLE(sArg->mValue);

    CMP_MARSHAL_STAGE(0);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

cmpMarshalFunction gCmpReadFunctionBASE[CMP_OP_BASE_MAX] =
{
    cmpReadBASEError,
    cmpReadBASEHandshake,
    cmpReadBASESecureVersion,
    cmpReadBASESecureVersionResult,
    cmpReadBASESecureEnvelope,
    cmpReadBASESecureEnvelopeResult,
};

cmpMarshalFunction gCmpWriteFunctionBASE[CMP_OP_BASE_MAX] =
{
    cmpWriteBASEError,
    cmpWriteBASEHandshake,
    cmpWriteBASESecureVersion,
    cmpWriteBASESecureVersionResult,
    cmpWriteBASESecureEnvelope,
    cmpWriteBASESecureEnvelopeResult,
};
