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
 * $Id: acpAtomic-X86_LINUX.c 11386 2010-06-28 09:15:31Z djin $
 ******************************************************************************/

/*******************************************************************************
 * This file is added to compile core with native C compiler.
 * Linux has only gcc, so it may not be needed.
 * But to test, this file was made.
 ******************************************************************************/

#include <acpAtomic.h>

/*******************************************************************************
 * NOINLINE must not be used to compile this source
 ******************************************************************************/
#if !defined(__GNUC__) || defined(ACP_CFG_NOINLINE)
#error This file must be compiled with GNU C compiler and no NOINLINE macro!
#endif

ACP_EXPORT acp_sint16_t acpAtomicCas16EXPORT(volatile void         *aAddr,
                                             volatile acp_sint16_t  aWith,
                                             volatile acp_sint16_t  aCmp)
{
    return acpAtomicCas16(aAddr, aWith, aCmp);
}

ACP_EXPORT acp_sint16_t acpAtomicSet16EXPORT(volatile void         *aAddr,
                                             volatile acp_sint16_t  aVal)
{
    return acpAtomicSet16(aAddr, aVal);
}

ACP_EXPORT acp_sint16_t acpAtomicAdd16EXPORT(volatile void         *aAddr,
                                             volatile acp_sint16_t  aVal)
{
    return acpAtomicAdd16(aAddr, aVal);
}

ACP_EXPORT acp_sint32_t acpAtomicCas32EXPORT(volatile void         *aAddr,
                                             volatile acp_sint32_t  aWith,
                                             volatile acp_sint32_t  aCmp)
{
    return acpAtomicCas32(aAddr, aWith, aCmp);
}

ACP_EXPORT acp_sint32_t acpAtomicSet32EXPORT(volatile void         *aAddr,
                                             volatile acp_sint32_t  aVal)
{
    return acpAtomicSet32(aAddr, aVal);
}

ACP_EXPORT acp_sint32_t acpAtomicAdd32EXPORT(volatile void         *aAddr,
                                             volatile acp_sint32_t  aVal)
{
    return acpAtomicAdd32(aAddr, aVal);
}

ACP_EXPORT acp_sint64_t acpAtomicGet64EXPORT(volatile void *aAddr)
{
    return acpAtomicGet64(aAddr);
}

ACP_EXPORT acp_sint64_t acpAtomicSet64EXPORT(volatile void         *aAddr,
                                             volatile acp_sint64_t  aVal)
{
    return acpAtomicSet64(aAddr, aVal);
}

ACP_EXPORT acp_sint64_t acpAtomicAdd64EXPORT(volatile void         *aAddr,
                                             volatile acp_sint64_t  aVal)
{
    return acpAtomicAdd64(aAddr, aVal);
}

#if defined(ACP_CFG_COMPILE_64BIT)

ACP_EXPORT acp_sint64_t acpAtomicCas64EXPORT(volatile void         *aAddr,
                                             volatile acp_sint64_t  aWith,
                                             volatile acp_sint64_t  aCmp)
{
    return acpAtomicCas64(aAddr, aWith, aCmp);
}

#else

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
     *
     * Another problem ;
     *  MacOSX gcc also has same problem even when static compile
     */
#   if defined(ACP_CFG_PIC) || defined(ALTI_CFG_OS_DARWIN)
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

#endif  /* ACP_CFG_COMPILE_64BIT */

