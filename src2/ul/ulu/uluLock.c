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

#include <acp.h>
#include <acl.h>
#include <ace.h>
#include <aciErrorMgr.h>
#include <ulu.h>
#include <uluLock.h>

ACI_RC uluLockCreate(acp_thr_mutex_t **aLock)
{
    acp_thr_mutex_t *sLock;

    ACI_TEST(acpMemAlloc((void**)&sLock, ACI_SIZEOF(acp_thr_mutex_t)) != ACP_RC_SUCCESS);

    *aLock = sLock;

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC uluLockDestroy(acp_thr_mutex_t *aLock)
{
    acpMemFree(aLock);

    return ACI_SUCCESS;
}
