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
 * $Id:
 ******************************************************************************/

#include <acpMemBarrier.h>

#if !defined(ALTI_CFG_OS_WINDOWS) && \
    (!defined(__GNUC__) || defined(ACP_CFG_NOINLINE))

/* Inline assembly porting for non-gcc compiler */

/* All functions are included at object file, not source file. */

/*
 * 1. generate object files with gcc.
 * For example, followings are building acpMemBarrier at 32bit solaris-SPARC.
 * ex)
 * $ gcc -c -mcpu=v8 -o objects/acpMemBarrier-SPARC_32MODE.o acpMemBarrier.c
 * $ gcc -c -fPIC -mcpu=v8 -o
 *   objects/acpMemBarrier-SPARC_32MODE_pic.o acpMemBarrier.c
 */
 
/*
 * 2. fix core.mk to link building object files. For example,
 * ex)
 *   ifeq ($(ALTI_CFG_CPU),SPARC)
 *    ifeq ($(compile_bit),32)
 *      CORE_OBJS   += $(CORE_DIR)/src/acp/objects/acpMemBarrier-SPARC_32MODE.o
 *      CORE_SHOBJS += 
 *          $(CORE_DIR)/src/acp/objects/acpMemBarrier-SPARC_32MODE_pic.o
 *    else
 *      CORE_OBJS   += $(CORE_DIR)/src/acp/objects/acpMemBarrier-SPARC_64MODE.o
 *      CORE_SHOBJS += 
 *          $(CORE_DIR)/src/acp/objects/acpMemBarrier-SPARC_64MODE_pic.o
 *    endif
 *   endif
 */

# if defined(ALTI_CFG_CPU_POWERPC)

ACP_EXPORT void acpMemRBarrier()
{
    acpMemBarrier();
}

ACP_EXPORT void acpMemWBarrier()
{
    acpMemBarrier();
}

ACP_EXPORT void acpMemPrefetch0(void* aPointer) { }

ACP_EXPORT void acpMemPrefetch1(void* aPointer) { }

ACP_EXPORT void acpMemPrefetch2(void* aPointer) { }

ACP_EXPORT void acpMemPrefetchN(void* aPointer) { }

ACP_EXPORT void acpMemPrefetch(void* aPointer) { }

# endif

#else

ACP_EXPORT void acpMemBarrier()
{
    ACP_MEM_BARRIER();
}

ACP_EXPORT void acpMemRBarrier()
{
    ACP_MEM_RBARRIER();
}

ACP_EXPORT void acpMemWBarrier()
{
    ACP_MEM_WBARRIER();
}

ACP_EXPORT void acpMemPrefetch0(void* aPointer)
{
    ACP_MEM_PREFETCH0(aPointer);
}

ACP_EXPORT void acpMemPrefetch1(void* aPointer)
{
    ACP_MEM_PREFETCH1(aPointer);
}

ACP_EXPORT void acpMemPrefetch2(void* aPointer)
{
    ACP_MEM_PREFETCH2(aPointer);
}

ACP_EXPORT void acpMemPrefetchN(void* aPointer)
{
    ACP_MEM_PREFETCHN(aPointer);
}

ACP_EXPORT void acpMemPrefetch(void* aPointer)
{
    ACP_MEM_PREFETCH(aPointer);
}

#endif


