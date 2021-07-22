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


extern cmpArgFunction     gCmpArgInitializeFunctionDBClient[CMP_OP_DB_MAX_A5];
extern cmpArgFunction     gCmpArgFinalizeFunctionDBClient[CMP_OP_DB_MAX_A5];

extern cmpMarshalFunction gCmpReadFunctionDBClient[CMP_OP_DB_MAX_A5];
extern cmpMarshalFunction gCmpWriteFunctionDBClient[CMP_OP_DB_MAX_A5];


cmiCallbackFunction gCmpCallbackFunctionDBClient[CMP_OP_DB_MAX];


cmpModule gCmpModuleClientDB =
{
    "db",

    CMP_MODULE_DB,

    CM_PATCH_VERSION,  // proj_2160 cm_type removal
    CMP_OP_DB_MAX_A5,  /* BUG-43080 */
    CMP_OP_DB_MAX,

    gCmpArgInitializeFunctionDBClient,
    gCmpArgFinalizeFunctionDBClient,

    gCmpReadFunctionDBClient,
    gCmpWriteFunctionDBClient,

    gCmpCallbackFunctionDBClient, /* A7 */
    NULL                          /* A5 */
};
