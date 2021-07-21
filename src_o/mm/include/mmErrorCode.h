/**
 *  Copyright (c) 1999~2017, Altibase Corp. and/or its affiliates. All rights reserved.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Affero General Public License, version 3,
 *  as published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU Affero General Public License for more details.
 *
 *  You should have received a copy of the GNU Affero General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _O_MAIN_ERROR_H_
# define _O_MAIN_ERROR_H_ 1

#include <idl.h>
#include <idTypes.h>
#include <ideErrorMgr.h>
#include <idErrorCode.h>

typedef enum
{
#include "mmErrorCode.ih"

    MM_MAX_ERROR_CODE

} MM_ERR_CODE;

extern const SChar *gMMTraceCode[];

#include "MM_TRC_CODE.ih"

#endif /* _O_MAINERROR_H_ */
