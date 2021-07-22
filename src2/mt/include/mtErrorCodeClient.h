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
 

/***********************************************************************
 * $Id: mtErrorCodeClient.h 26440 2008-06-10 04:02:48Z jdlee $
 **********************************************************************/

#ifndef _O_MT_ERROR_CODE_CLIENT_H_
#define _O_MT_ERROR_CODE_CLIENT_H_  1

typedef enum
{
# include "mtErrorCode.ih"
    MT_MAX_ERROR_CODE
} MT_ERR_CODE;

/* ------------------------------------------------
 *  Trace Code
 * ----------------------------------------------*/

extern acp_char_t * gMTTraceCode[];

#include "MT_TRC_CODE.ih"

#endif //

