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

cmpOpMap gCmpOpBaseMapClient[] =
{
    {"CMP_OP_BASE_Error"               } ,
    {"CMP_OP_BASE_Handshake"           } ,
    {"CMP_OP_BASE_MAX_VER1"            }
};

ACI_RC cmpArgInitializeBASEError(cmpProtocol *aProtocol)
{
    cmpArgBASEError *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, BASE, Error);

    ACI_TEST(cmtVariableInitialize(&sArg->mErrorMessage) != ACI_SUCCESS);

    return ACI_SUCCESS;
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

ACI_RC cmpArgFinalizeBASEError(cmpProtocol *aProtocol)
{
    cmpArgBASEError *sArg = CMI_PROTOCOL_GET_ARG(*aProtocol, BASE, Error);

    ACI_TEST(cmtVariableFinalize(&sArg->mErrorMessage) != ACI_SUCCESS);

    return ACI_SUCCESS;
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

cmpArgFunction gCmpArgInitializeFunctionBASEClient[CMP_OP_BASE_MAX] =
{
    cmpArgInitializeBASEError,                /* CMP_OP_BASE_Error */
    cmpArgNULL,                               /* CMP_OP_BASE_Handshake */
};

cmpArgFunction gCmpArgFinalizeFunctionBASEClient[CMP_OP_BASE_MAX] =
{
    cmpArgFinalizeBASEError,                  /* CMP_OP_BASE_Error */
    cmpArgNULL,                               /* CMP_OP_BASE_Handshake */
};
