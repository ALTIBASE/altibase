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
 * $Id: idErrorCodeClient.h 26440 2008-06-10 04:02:48Z jdlee $
 **********************************************************************/

/***********************************************************************
 *	File Name 		:	idErrorCodeClient.h
 *
 *  Modify			:	Hee-Taek, Kim
 *
 *	Related Files	:
 *
 *	Description		:	ID 에러 코드 화일
 *
 *
 **********************************************************************/

#ifndef ID_ERROR_CODE_CLIENT_H
#define ID_ERROR_CODE_CLIENT_H  1

#ifndef BUILD_FOR_UTIL
typedef enum _id_err_code
{
#include "idErrorCode.ih"

    ID_MAX_ERROR_CODE


} ID_ERR_CODE;


/* ------------------------------------------------
 *  Trace Code
 * ----------------------------------------------*/

ACP_EXTERN_C const acp_char_t *gIDTraceCode[];

#include "ID_TRC_CODE.ih"

#endif

#endif

