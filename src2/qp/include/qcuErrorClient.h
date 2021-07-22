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
 * $Id: qcuErrorClient.h 26440 2008-06-10 04:02:48Z jdlee $
 **********************************************************************/

#ifndef _O_QCUERROR_CLIENT_H_
#define _O_QCUERROR_CLIENT_H_ 1

/* QP용 에러 코드 */
enum QPERR_CODE
{
#include <qcuErrorCode.ih>
  QPERR_MAX_ERRORCODE
};

/* ------------------------------------------------
 *  Trace Code
 * ----------------------------------------------*/

extern const acp_char_t * gQPTraceCode[];

#include "QP_TRC_CODE.ih"

#endif /* _O_QCUERROR_H_ */
