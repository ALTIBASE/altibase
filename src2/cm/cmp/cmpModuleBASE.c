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


struct cmiProtocolContext;

ACI_RC cmpCallbackBASEError(struct cmiProtocolContext *,
                            struct cmpProtocol *,
                            void *,
                            void *);
ACI_RC cmpCallbackBASEHandshake(struct cmiProtocolContext *,
                                struct cmpProtocol *,
                                void *,
                                void *);


extern cmpArgFunction     gCmpArgInitializeFunctionBASEClient[CMP_OP_BASE_MAX];
extern cmpArgFunction     gCmpArgFinalizeFunctionBASEClient[CMP_OP_BASE_MAX];

extern cmpMarshalFunction gCmpReadFunctionBASEClient[CMP_OP_BASE_MAX];
extern cmpMarshalFunction gCmpWriteFunctionBASEClient[CMP_OP_BASE_MAX];


cmiCallbackFunction gCmpCallbackFunctionBASEClient[CMP_OP_BASE_MAX] =
{
    cmpCallbackBASEError,                /* CMP_OP_BASE_Error */
    cmpCallbackBASEHandshake,            /* CMP_OP_BASE_Handshake */
};


cmpModule gCmpModuleClientBASE =
{
    "base",

    CMP_MODULE_BASE,

    CMP_VER_BASE_MAX,
    CMP_OP_BASE_MAX,  /* BUG-43080 */
    CMP_OP_BASE_MAX,

    gCmpArgInitializeFunctionBASEClient,
    gCmpArgFinalizeFunctionBASEClient,

    gCmpReadFunctionBASEClient,
    gCmpWriteFunctionBASEClient,

    gCmpCallbackFunctionBASEClient,
    NULL
};
