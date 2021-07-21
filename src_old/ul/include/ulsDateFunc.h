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
 *
 * Spatio-Temporal Date Á¶ÀÛ ÇÔ¼ö
 *
 ***********************************************************************/

#ifndef _O_ULS_DATE_FUNC_H_
#define _O_ULS_DATE_FUNC_H_    (1)

#include <acp.h>
#include <aciTypes.h>
#include <aciErrorMgr.h>

#include <mtcdTypes.h>

#include <ulsEnvHandle.h>
#include <ulsTypes.h>

/*----------------------------------------------------------------*
 *  External Interfaces
 *----------------------------------------------------------------*/

/*----------------------------------------------------------------*
 *  Date(acsDateType) Manipulation Interfaces                     
 *----------------------------------------------------------------*/

/* DATE °ª È¹µæ*/
ACSRETURN ulsGetDate( ulsHandle           * aHandle,
                      mtdDateType         * aDateValue,
                      acp_sint32_t        * aYear,
                      acp_sint32_t        * aMonth,
                      acp_sint32_t        * aDay,
                      acp_sint32_t        * aHour,
                      acp_sint32_t        * aMin,
                      acp_sint32_t        * aSec,
                      acp_sint32_t        * aMicSec );

/* DATE °ª ¼³Á¤*/
ACSRETURN ulsSetDate( ulsHandle           * aHandle,
                      mtdDateType         * aDateValue,
                      acp_sint32_t          aYear,
                      acp_sint32_t          aMonth,
                      acp_sint32_t          aDay,
                      acp_sint32_t          aHour,
                      acp_sint32_t          aMin,
                      acp_sint32_t          aSec,
                      acp_sint32_t          aMicSec );

/*----------------------------------------------------------------*
 *  Internal Interfaces
 *----------------------------------------------------------------*/

#endif /* _O_ULS_DATE_FUNC_H_ */


