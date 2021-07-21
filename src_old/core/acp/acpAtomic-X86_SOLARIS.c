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
 * $Id: acpAtomic-X86_LINUX.c 4529 2009-02-13 01:45:30Z jykim $
 ******************************************************************************/

#include <acpAtomic.h>

/*******************************************************************************
 * NOINLINE must not be used to compile this source
 ******************************************************************************/
#if defined(__GNUC__) && defined(ACP_CFG_COMPILE_32BIT)

/*
 * 32bit code
 */
ACP_EXPORT acp_sint64_t acpAtomicCas64(volatile void *aAddr,
                                       acp_sint64_t   aWith,
                                       acp_sint64_t   aCmp)
{
    acp_sint64_t sPrev;

    /*
     * GCC uses ebx as the PIC register for globals in shared libraries.
     * GCC won't temporarily spill it if it's used as input, output, or clobber.
     * Instead, it complains with a "can't find a register in class `BREG' while
     * reloading `asm'" error. This is probably a compiler bug, but as the
     * cmpxchg8b instruction requires ebx. So, I work around this here by using
     * esi as input and exchange before and after the cmpxchg8b instruction.
     */
#   if defined(ACP_CFG_PIC)
    __asm__ __volatile__ ("xchgl %%esi,%%ebx\n\t"
                          "lock; cmpxchg8b %3\n\t"
                          "xchgl %%ebx,%%esi"
                          : "=A"(sPrev)
                          : "S"((acp_uint32_t)((acp_uint64_t)aWith)),
                            "c"((acp_uint32_t)((acp_uint64_t)aWith >> 32)),
                            "m"(*(volatile acp_sint64_t *)aAddr),
                            "0"(aCmp)
                          : "memory");
#   else
    __asm__ __volatile__ ("lock; cmpxchg8b %3"
                          : "=A"(sPrev)
                          : "b"((acp_uint32_t)((acp_uint64_t)aWith)),
                            "c"((acp_uint32_t)((acp_uint64_t)aWith >> 32)),
                            "m"(*(volatile acp_sint64_t *)aAddr),
                            "0"(aCmp)
                          : "memory");
#   endif

    return sPrev;
}

#endif /* ONLY GNUC */
