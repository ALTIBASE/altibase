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
 
/*******************************************************************************
 * $Id: acpAtomic-OTHER_USE_LOCKS.c 5204 2009-04-10 01:17:46Z sjkim $
 ******************************************************************************/

#include <acpAtomic.h>
#include <aceAssert.h>
#include <acpThr.h>
#include <acpThrMutex.h>

#define ACP_ATOMIC_HASH_PRIME 7

static acp_thr_mutex_t gAcpAtomicMutex[ACP_ATOMIC_HASH_PRIME] =
{
    ACP_THR_MUTEX_INITIALIZER,
    ACP_THR_MUTEX_INITIALIZER,
    ACP_THR_MUTEX_INITIALIZER,
    ACP_THR_MUTEX_INITIALIZER,
    ACP_THR_MUTEX_INITIALIZER,
    ACP_THR_MUTEX_INITIALIZER,
    ACP_THR_MUTEX_INITIALIZER
};

/* for emulation of acpAtomic() */
ACP_EXPORT void acpAtomicAddrLock(volatile void *aAddr)
{
    (void)acpThrMutexLock(
        &gAcpAtomicMutex[((acp_ulong_t)aAddr >> 2) % ACP_ATOMIC_HASH_PRIME]
        );
}

/* for emulation of acpAtomic() */
ACP_EXPORT void acpAtomicAddrUnlock(volatile void *aAddr)
{
    (void)acpThrMutexUnlock(
        &gAcpAtomicMutex[((acp_ulong_t)aAddr >> 2) % ACP_ATOMIC_HASH_PRIME]
        );
}
