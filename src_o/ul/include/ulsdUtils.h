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
 * $Id: ulsdUtils.h 90038 2021-02-22 02:03:47Z donlet $
 **********************************************************************/

#ifndef _O_ULSD_UTILS_H_
#define _O_ULSD_UTILS_H_ 1

#include <uln.h>
#include <ulnPrivate.h>
#include <ulsd.h>

/* BUG-48384 */
ACI_RC ulsdBuildClientTouchNodeArr(ulnFnContext *aFnContext, ulsdDbc *aShard, acp_uint32_t aStmtType);
void ulsdPropagateClientTouchNodeArrToNode(ulnDbc *aNodeDbc, ulnDbc *aMetaDbc);

#endif  /* _O_ULSD_UTILS_H_ */
