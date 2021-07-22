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

#ifndef _O_CM_ERROR_CODE_H_
#define _O_CM_ERROR_CODE_H_ 1

#include <idTypes.h>
#include <ideErrorMgr.h>
#include <idErrorCode.h>


typedef enum
{
#include "cmErrorCode.ih"
    CM_MAX_ERROR_CODE
} CM_ERR_CODE;


extern const SChar *gCMTraceCode[];

#include "CM_TRC_CODE.ih"


#endif
