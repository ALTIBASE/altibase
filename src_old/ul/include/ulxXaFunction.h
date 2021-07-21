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

#ifndef _O_ULX_XA_FUNCTION_H_
#define _O_ULX_XA_FUNCTION_H_ 1

#include <uln.h>

ACP_EXTERN_C_BEGIN

int ulxXaOpen(char *aXa_info, int aRmid, long aFlags);

int ulxXaClose(char *aXa_info, int aRmid, long aFlags);

int ulxXaFree(char *aXa_info, int aRmid, long aFlags);

int ulxXaCommit(XID *aXid, int aRmid, long aFlags);

int ulxXaComplete(int *aHandle, int *retval, int rmid, long aFlags);

int ulxXaEnd(XID *aXid, int aRmid, long aFlags);

int ulxXaForget(XID *aXid, int aRmid, long aFlags);

int ulxXaPrepare(XID *aXid, int aRmid, long aFlags);

int ulxXaRecover(XID *aXid, long aCount, int aRmid, long aFlags);

int ulxXaRollback(XID *aXid, int aRmid, long aFlags);

int ulxXaStart(XID *aXid, int aRmid, long aFlags);

ACP_EXTERN_C_END

#endif /* _O_ULX_XA_FUNCTION_H_ */
