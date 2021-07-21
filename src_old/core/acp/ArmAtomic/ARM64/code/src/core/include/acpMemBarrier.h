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
 * $Id: acpMemBarrier.h 9976 2010-02-10 06:02:39Z djin $
 ******************************************************************************/

#if !defined(_O_ACP_MEM_BARRIER_H_)
#define _O_ACP_MEM_BARRIER_H_

/**
 * @file
 * @ingroup CoreMem
 */

#include <acpTypes.h>

ACP_EXTERN_C_BEGIN

ACP_EXPORT void acpMemBarrier(void);
ACP_EXPORT void acpMemRBarrier(void);
ACP_EXPORT void acpMemWBarrier(void);
ACP_EXPORT void acpMemPrefetch(void* aPointer);
ACP_EXPORT void acpMemPrefetch0(void* aPointer);
ACP_EXPORT void acpMemPrefetch1(void* aPointer);
ACP_EXPORT void acpMemPrefetch2(void* aPointer);
ACP_EXPORT void acpMemPrefetchN(void* aPointer);

/*
#  define ACP_MEM_BARRIER()           asm volatile ("dmb sy" : : : "memory")
#  define ACP_MEM_RBARRIER()          asm volatile ("dmb sy" : : : "memory")
#  define ACP_MEM_WBARRIER()          asm volatile ("dmb sy" : : : "memory")
*/
#  define ACP_MEM_BARRIER()           asm("dmb ish")
#  define ACP_MEM_RBARRIER()          asm("dmb ish")
#  define ACP_MEM_WBARRIER()          asm("dmb ish")


#  define ACP_MEM_PREFETCH0(aPointer) ACP_UNUSED(aPointer)
#  define ACP_MEM_PREFETCH1(aPointer) ACP_UNUSED(aPointer)
#  define ACP_MEM_PREFETCH2(aPointer) ACP_UNUSED(aPointer)
#  define ACP_MEM_PREFETCHN(aPointer) ACP_UNUSED(aPointer)
#  define ACP_MEM_PREFETCH ACP_MEM_PREFETCH0




ACP_EXTERN_C_END

#endif
