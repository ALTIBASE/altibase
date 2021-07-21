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


extern cmpArgFunction     gCmpArgInitializeFunctionRPClient[CMP_OP_RP_MAX];
extern cmpArgFunction     gCmpArgFinalizeFunctionRPClient[CMP_OP_RP_MAX];

extern cmpMarshalFunction gCmpReadFunctionRPClient[CMP_OP_RP_MAX];
extern cmpMarshalFunction gCmpWriteFunctionRPClient[CMP_OP_RP_MAX];


cmiCallbackFunction gCmpCallbackFunctionRPClient[CMP_OP_RP_MAX];


cmpModule gCmpModuleClientRP =
{
    "rp",

    CMP_MODULE_RP,

    CMP_VER_RP_MAX,
    CMP_OP_RP_MAX,  /* BUG-43080 */
    CMP_OP_RP_MAX,

    gCmpArgInitializeFunctionRPClient,
    gCmpArgFinalizeFunctionRPClient,

    gCmpReadFunctionRPClient,
    gCmpWriteFunctionRPClient,

    gCmpCallbackFunctionRPClient,
    NULL
};
