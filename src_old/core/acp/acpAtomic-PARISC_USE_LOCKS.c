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
 * $Id: acpAtomic-PARISC_USE_LOCKS.c 6162 2009-06-25 06:47:40Z djin $
 ******************************************************************************/

#include <acpAtomic.h>
#include <acpSpinLock.h>

#define ACP_ATOMIC_HASH_PRIME 7

static acp_spin_lock_t gAcpAtomicLock[ACP_ATOMIC_HASH_PRIME] =
{
    ACP_SPIN_LOCK_INITIALIZER(-1),
    ACP_SPIN_LOCK_INITIALIZER(-1),
    ACP_SPIN_LOCK_INITIALIZER(-1),
    ACP_SPIN_LOCK_INITIALIZER(-1),
    ACP_SPIN_LOCK_INITIALIZER(-1),
    ACP_SPIN_LOCK_INITIALIZER(-1),
    ACP_SPIN_LOCK_INITIALIZER(-1)
};

ACP_EXPORT void acpAtomicAddrLock(volatile void *aAddr)
{
    acp_sint32_t sHash = ((acp_uint32_t)aAddr >> 2) % ACP_ATOMIC_HASH_PRIME;

    acpSpinLockLock(&gAcpAtomicLock[sHash]);
}

ACP_EXPORT void acpAtomicAddrUnlock(volatile void *aAddr)
{
    acp_sint32_t sHash = ((acp_uint32_t)aAddr >> 2) % ACP_ATOMIC_HASH_PRIME;

    acpSpinLockUnlock(&gAcpAtomicLock[sHash]);
}

