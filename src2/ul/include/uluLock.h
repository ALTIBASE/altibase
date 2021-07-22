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

#ifndef _O_ULU_LOCK_H_
#define _O_ULU_LOCK_H_

#include <acp.h>
#include <acl.h>
#include <aciTypes.h>

ACI_RC uluLockCreate(acp_thr_mutex_t **aLock);
ACI_RC uluLockDestroy(acp_thr_mutex_t *aLock);

#endif /* _O_ULU_LOCK_H_ */
