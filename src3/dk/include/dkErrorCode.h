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
 * $Id$
 **********************************************************************/

#ifndef _O_DK_ERROR_CODE_H_
#define _O_DK_ERROR_CODE_H_  1

typedef enum
{
#include "dkErrorCode.ih"
    
    DK_MAX_ERROR_CODE

} DK_ERR_CODE;

/* ------------------------------------------------
 *  Trace Code 
 * ----------------------------------------------*/

extern const SChar * gDKTraceCode[];

#include "DK_TRC_CODE.ih"

#endif /* _O_DK_ERROR_CODE_H_ */

