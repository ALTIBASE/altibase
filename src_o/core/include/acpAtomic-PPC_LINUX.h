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
 * $Id: acpAtomic-PPC.h 8779 2009-11-20 11:06:58Z djin $
 ******************************************************************************/

#if !defined(_O_ACP_ATOMIC_PPC_H_)
#define _O_ACP_ATOMIC_PPC_H_

/*
 * AIX assembler does not support local labels, so we must not use them
 * AIX assembler needs -many flag for 32-bit compile
 */

ACP_EXPORT acp_sint16_t acpAtomicCas16(volatile void         *aAddr,
                                       volatile acp_sint16_t  aWith,
                                       volatile acp_sint16_t  aCmp);

ACP_EXPORT acp_sint16_t acpAtomicSet16(volatile void         *aAddr,
                                       volatile acp_sint16_t  aVal);

ACP_EXPORT acp_sint16_t acpAtomicAdd16(volatile void         *aAddr,
                                       volatile acp_sint16_t  aVal);

#if defined(__GNUC__) && !defined(ACP_CFG_NOINLINE)
ACP_INLINE acp_sint32_t acpAtomicCas32(volatile void         *aAddr,
                                       volatile acp_sint32_t  aWith,
                                       volatile acp_sint32_t  aCmp)
{
    acp_sint32_t sPrev;

    __asm__ __volatile__ ("lwsync\n\t"
                          "lwarx %0,0,%1\n\t"
                          "cmpw %0,%3\n\t"
                          "bne- $+12\n\t"
                          "stwcx. %2,0,%1\n\t"
                          "bne- $-16\n\t"
                          "isync"
                          : "=&r"(sPrev)
                          : "b"(aAddr), "r"(aWith), "r"(aCmp)
                          : "memory");
    return sPrev;
}

ACP_INLINE acp_sint32_t acpAtomicSet32(volatile void         *aAddr,
                                       volatile acp_sint32_t  aVal)
{
    acp_sint32_t sPrev;

    __asm__ __volatile__ ("lwsync\n\t"
                          "lwarx %0,0,%2\n\t"
                          "stwcx. %3,0,%2\n\t"
                          "bne- $-8\n\t"
                          "isync"
                          : "=&r"(sPrev), "+m"(*(volatile acp_sint32_t *)aAddr)
                          : "r"(aAddr), "r"(aVal)
                          : "memory");
    return sPrev;
}

ACP_INLINE acp_sint32_t acpAtomicAdd32(volatile void         *aAddr,
                                       volatile acp_sint32_t  aVal)
{
    acp_sint32_t sPrev;
    acp_sint32_t sResult;

    __asm__ __volatile__ ("lwsync\n\t"
                          "lwarx %0,0,%2\n\t"
                          "add %1,%0,%3\n\t"
                          "stwcx. %1,0,%2\n\t"
                          "bne- $-12\n\t"
                          "isync"
                          : "=&r"(sPrev), "=&r"(sResult)
                          : "b"(aAddr), "r"(aVal)
                          : "memory");
    return sResult;
}

#if defined(ACP_CFG_COMPILE_64BIT)

ACP_INLINE acp_sint64_t acpAtomicGet64(volatile void *aAddr)
{
    ACP_MEM_BARRIER();

    return *(volatile acp_sint64_t *)aAddr;
}

ACP_INLINE acp_sint64_t acpAtomicCas64(volatile void         *aAddr,
                                       volatile acp_sint64_t  aWith,
                                       volatile acp_sint64_t  aCmp)
{
    acp_sint64_t sPrev;

    __asm__ __volatile__ ("lwsync\n\t"
                          "ldarx %0,0,%1\n\t"
                          "cmpd %0,%3\n\t"
                          "bne- $+12\n\t"
                          "stdcx. %2,0,%1\n\t"
                          "bne- $-16\n\t"
                          "isync"
                          : "=&r"(sPrev)
                          : "b"(aAddr), "r"(aWith), "r"(aCmp)
                          : "memory");
    return sPrev;
}

ACP_INLINE acp_sint64_t acpAtomicSet64(volatile void         *aAddr,
                                       volatile acp_sint64_t  aVal)
{
    acp_sint64_t sPrev;

    __asm__ __volatile__ ("lwsync\n\t"
                          "ldarx %0,0,%2\n\t"
                          "stdcx. %3,0,%2\n\t"
                          "bne- $-8\n\t"
                          "isync"
                          : "=&r"(sPrev), "+m"(*(volatile acp_sint64_t *)aAddr)
                          : "r"(aAddr), "r"(aVal)
                          : "memory");
    return sPrev;
}

ACP_INLINE acp_sint64_t acpAtomicAdd64(volatile void         *aAddr,
                                       volatile acp_sint64_t  aVal)
{
    acp_sint64_t sPrev;
    acp_sint64_t sResult;

    __asm__ __volatile__ ("lwsync\n\t"
                          "ldarx %0,0,%2\n\t"
                          "add %1,%0,%3\n\t"
                          "stdcx. %1,0,%2\n\t"
                          "bne- $-12\n\t"
                          "isync"
                          : "=&r"(sPrev), "=&r"(sResult)
                          : "b"(aAddr), "r"(aVal)
                          : "memory");
    return sResult;
}

#else

ACP_EXPORT acp_sint64_t acpAtomicCas64(volatile void         *aAddr,
                                       volatile acp_sint64_t  aWith,
                                       volatile acp_sint64_t  aCmp);

ACP_EXPORT acp_sint64_t acpAtomicGet64(volatile void *aAddr);

ACP_EXPORT acp_sint64_t acpAtomicSet64(volatile void         *aAddr,
                                       volatile acp_sint64_t  aVal);

ACP_EXPORT acp_sint64_t acpAtomicAdd64(volatile void         *aAddr,
                                       volatile acp_sint64_t  aVal);

#endif

#else
/* Native C compiler */

ACP_EXPORT acp_sint32_t acpAtomicCas32(volatile void         *aAddr,
                                       volatile acp_sint32_t  aWith,
                                       volatile acp_sint32_t  aCmp);

ACP_EXPORT acp_sint32_t acpAtomicAdd32(volatile void         *aAddr,
                                       volatile acp_sint32_t  aVal);

ACP_EXPORT acp_sint64_t acpAtomicCas64(volatile void         *aAddr,
                                       volatile acp_sint64_t  aWith,
                                       volatile acp_sint64_t  aCmp);

ACP_EXPORT acp_sint64_t acpAtomicAdd64(volatile void         *aAddr,
                                       volatile acp_sint64_t  aVal);

ACP_INLINE acp_sint64_t acpAtomicGet64(volatile void         *aAddr)
{
    return acpAtomicCas64(aAddr, 0, 0);
}

ACP_INLINE acp_sint32_t acpAtomicSet32(volatile void         *aAddr,
                                       volatile acp_sint32_t  aVal)
{
    acp_sint32_t sPrev;
    acp_sint32_t sTemp;

    do
    {
        sPrev = *(volatile acp_sint32_t *)aAddr;
        sTemp = acpAtomicCas32(aAddr, aVal, sPrev);
    } while (sTemp != sPrev);

    return sPrev;
}

ACP_INLINE acp_sint64_t acpAtomicSet64(volatile void         *aAddr,
                                       volatile acp_sint64_t  aVal)
{
    acp_sint64_t sPrev;
    acp_sint64_t sTemp;

    do
    {
        sPrev = *(volatile acp_sint64_t *)aAddr;
        sTemp = acpAtomicCas64(aAddr, aVal, sPrev);
    } while (sTemp != sPrev);

    return sPrev;
}

#endif

#endif
