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


IDE_RC cmpCallbackBASEError(cmiProtocolContext *, cmiProtocol *, void *, void *);
IDE_RC cmpCallbackBASEHandshake(cmiProtocolContext *, cmiProtocol *, void *, void *);


extern cmpArgFunction     gCmpArgInitializeFunctionBASE[CMP_OP_BASE_MAX];
extern cmpArgFunction     gCmpArgFinalizeFunctionBASE[CMP_OP_BASE_MAX];

extern cmpMarshalFunction gCmpReadFunctionBASE[CMP_OP_BASE_MAX];
extern cmpMarshalFunction gCmpWriteFunctionBASE[CMP_OP_BASE_MAX];


cmiCallbackFunction gCmpCallbackFunctionBASE[CMP_OP_BASE_MAX] =
{
    cmpCallbackBASEError,                /* CMP_OP_BASE_Error */
    cmpCallbackBASEHandshake,            /* CMP_OP_BASE_Handshake */
    NULL,                                /* CMP_OP_BASE_SecureVersion */
    NULL,                                /* CMP_OP_BASE_SecureVersionResult */
    NULL,                                /* CMP_OP_BASE_Envelope */
    NULL,                                /* CMP_OP_BASE_EnvelopeResult */
};


cmpModule gCmpModuleBASE =
{
    "base",

    CMP_MODULE_BASE,

    CMP_VER_BASE_MAX,
    CMP_OP_BASE_MAX,  /* BUG-43080 */
    CMP_OP_BASE_MAX,

    gCmpArgInitializeFunctionBASE,
    gCmpArgFinalizeFunctionBASE,

    gCmpReadFunctionBASE,
    gCmpWriteFunctionBASE,

    gCmpCallbackFunctionBASE,
    NULL
};
