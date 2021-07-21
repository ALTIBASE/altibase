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


extern cmpArgFunction     gCmpArgInitializeFunctionDB[CMP_OP_DB_MAX_A5];
extern cmpArgFunction     gCmpArgFinalizeFunctionDB[CMP_OP_DB_MAX_A5];

extern cmpMarshalFunction gCmpReadFunctionDB[CMP_OP_DB_MAX_A5];
extern cmpMarshalFunction gCmpWriteFunctionDB[CMP_OP_DB_MAX_A5];


cmiCallbackFunction gCmpCallbackFunctionDB[CMP_OP_DB_MAX];
cmiCallbackFunction gCmpCallbackFunctionDBA5[CMP_OP_DB_MAX_A5];


cmpModule gCmpModuleDB =
{
    "db",

    CMP_MODULE_DB,

    // proj_2160 cm_type removal
    // CMP_VER_DB_MAX를 더이상 사용하지 않기 때문에 
    // 대신 CM_PATCH_VERSION으로 변경.
    // 예전에도 별 의미 없었음(BaseHandshake 할때 DB 버전 확인용)
    CM_PATCH_VERSION,
    CMP_OP_DB_MAX_A5,  /* BUG-43080 */
    CMP_OP_DB_MAX,

    gCmpArgInitializeFunctionDB,
    gCmpArgFinalizeFunctionDB,

    gCmpReadFunctionDB,
    gCmpWriteFunctionDB,

    gCmpCallbackFunctionDB,
    gCmpCallbackFunctionDBA5
};
