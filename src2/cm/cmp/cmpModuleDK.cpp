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

extern cmpArgFunction     gCmpArgInitializeFunctionDK[CMP_OP_DK_MAX];
extern cmpArgFunction     gCmpArgFinalizeFunctionDK[CMP_OP_DK_MAX];

extern cmpMarshalFunction gCmpReadFunctionDK[CMP_OP_DK_MAX];
extern cmpMarshalFunction gCmpWriteFunctionDK[CMP_OP_DK_MAX];

cmiCallbackFunction gCmpCallbackFunctionDK[CMP_OP_DK_MAX];

cmpModule gCmpModuleDK =
{
    "dk",

    CMP_MODULE_DK,

    CMP_VER_DK_MAX,
    CMP_OP_DK_MAX,  /* BUG-43080 */
    CMP_OP_DK_MAX,

    gCmpArgInitializeFunctionDK,
    gCmpArgFinalizeFunctionDK,

    gCmpReadFunctionDK,
    gCmpWriteFunctionDK,

    gCmpCallbackFunctionDK,
    NULL
};
