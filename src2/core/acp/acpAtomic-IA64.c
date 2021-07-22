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
 * $Id$
 ******************************************************************************/
#include <acpAtomic.h>

#if !defined(__GNUC__) || defined(ACP_CFG_NOINLINE)
#error This file must be compiled with GNU C compiler and no NOINLINE macro!
#endif

#if defined(ACP_CFG_COMPILE_32BIT)

/* if this function is implemented by inline, we cannot sure it's operation. */
ACP_EXPORT acp_sint32_t acpAtomicCas32(volatile void         *aAddr,
                                       volatile acp_sint32_t  aWith,
                                       volatile acp_sint32_t  aCmp)
{
    acp_slong_t sPrev;

    __asm__ __volatile__ ("addp4 %0=0,%1\n\t"
                          "mov ar.ccv=%3;; cmpxchg4.acq %0=[%0],%2,ar.ccv"
                          : "=&r"(sPrev)
                          : "r"(aAddr), "r"(aWith), "r"(aCmp)
                          : "memory");

    return sPrev;
}

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

#if !defined(ACP_CFG_COMPILE_32BIT)

ACP_EXPORT acp_sint32_t acpAtomicCas32EXPORT(volatile void         *aAddr,
                                             volatile acp_sint32_t  aWith,
                                             volatile acp_sint32_t  aCmp)
{
    return acpAtomicCas32(aAddr, aWith, aCmp);
}

#endif

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

ACP_EXPORT acp_sint64_t acpAtomicCas64EXPORT(volatile void         *aAddr,
                                             volatile acp_sint64_t  aWith,
                                             volatile acp_sint64_t  aCmp)
{
    return acpAtomicCas64(aAddr, aWith, aCmp);
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


