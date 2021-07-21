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


ACI_RC cmpReadBASEError(cmbBlock *aBlock, cmpProtocol *aProtocol, cmpMarshalState *aMarshalState)
{
    cmpArgBASEError *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, BASE, Error);

    CMP_MARSHAL_BEGIN(2);
    CMP_MARSHAL_READ_UINT(sArg->mErrorCode);

    CMP_MARSHAL_STAGE(1);
    CMP_MARSHAL_READ_VARIABLE(sArg->mErrorMessage);

    CMP_MARSHAL_STAGE(0);

    return ACI_SUCCESS;
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

ACI_RC cmpWriteBASEError(cmbBlock *aBlock, cmpProtocol *aProtocol, cmpMarshalState *aMarshalState)
{
    cmpArgBASEError *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, BASE, Error);

    CMP_MARSHAL_BEGIN(2);
    CMP_MARSHAL_WRITE_UINT(sArg->mErrorCode);

    CMP_MARSHAL_STAGE(1);
    CMP_MARSHAL_WRITE_VARIABLE(sArg->mErrorMessage);

    CMP_MARSHAL_STAGE(0);

    return ACI_SUCCESS;
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

ACI_RC cmpReadBASEHandshake(cmbBlock *aBlock, cmpProtocol *aProtocol, cmpMarshalState *aMarshalState)
{
    cmpArgBASEHandshake *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, BASE, Handshake);

    CMP_MARSHAL_BEGIN(3);
    CMP_MARSHAL_READ_UCHAR(sArg->mBaseVersion);

    CMP_MARSHAL_STAGE(2);
    CMP_MARSHAL_READ_UCHAR(sArg->mModuleID);

    CMP_MARSHAL_STAGE(1);
    CMP_MARSHAL_READ_UCHAR(sArg->mModuleVersion);

    CMP_MARSHAL_STAGE(0);

    return ACI_SUCCESS;
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

ACI_RC cmpWriteBASEHandshake(cmbBlock *aBlock, cmpProtocol *aProtocol, cmpMarshalState *aMarshalState)
{
    cmpArgBASEHandshake *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, BASE, Handshake);

    CMP_MARSHAL_BEGIN(3);
    CMP_MARSHAL_WRITE_UCHAR(sArg->mBaseVersion);

    CMP_MARSHAL_STAGE(2);
    CMP_MARSHAL_WRITE_UCHAR(sArg->mModuleID);

    CMP_MARSHAL_STAGE(1);
    CMP_MARSHAL_WRITE_UCHAR(sArg->mModuleVersion);

    CMP_MARSHAL_STAGE(0);

    return ACI_SUCCESS;
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

cmpMarshalFunction gCmpReadFunctionBASEClient[CMP_OP_BASE_MAX] =
{
    cmpReadBASEError,
    cmpReadBASEHandshake,
};

cmpMarshalFunction gCmpWriteFunctionBASEClient[CMP_OP_BASE_MAX] =
{
    cmpWriteBASEError,
    cmpWriteBASEHandshake,
};
