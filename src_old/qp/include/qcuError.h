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
 * $Id: qcuError.h 82075 2018-01-17 06:39:52Z jina.kim $
 **********************************************************************/

#ifndef _O_QCUERROR_H_
# define _O_QCUERROR_H_

#include <ideErrorMgr.h>
#include <idErrorCode.h>

/* QP용 에러 코드 */
enum QPERR_CODE
{
#include <qcuErrorCode.ih>
  QPERR_MAX_ERRORCODE
};

/* ------------------------------------------------
 *  Trace Code
 * ----------------------------------------------*/

extern const SChar * gQPTraceCode[];

#include "QP_TRC_CODE.ih"

#endif /* _O_QCUERROR_H_ */
