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
 

/***********************************************************************
 * $Id: smErrorCodeClient.h 31937 2009-03-30 04:08:20Z kimmkeun $
 **********************************************************************/

/***********************************************************************
 *  File Name       :   smErrorCodeClient.h
 *
 *  Modify          :   Hee-Taek, Kim
 *
 *  Related Files   :
 *
 *  Description     :   SM 에러 코드 화일
 *
 *
 **********************************************************************/

#ifndef _O_SM_ERRORCODE_CLIENT_H_
#define _O_SM_ERRORCODE_CLIENT_H_  1

typedef enum
{
#include "smErrorCode.ih"

    SM_MAX_ERROR_CODE
} SM_ERR_CODE;

/* ------------------------------------------------
 *  Trace Code
 * ----------------------------------------------*/

extern const acp_char_t * gSMTraceCode[];

#include "SM_TRC_CODE.ih"


#endif //

