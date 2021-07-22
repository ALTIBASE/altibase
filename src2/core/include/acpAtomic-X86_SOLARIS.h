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

#if !defined(_O_ACP_ATOMIC_X86_SOLARIS_H_)
#define _O_ACP_ATOMIC_X86_SOLARIS_H_

#if defined(__GNUC__) && !defined(ACP_CFG_NOINLINE)

ACP_INLINE acp_sint16_t acpAtomicCas16(volatile void         *aAddr,
                                       volatile acp_sint16_t  aWith,
                                       volatile acp_sint16_t  aCmp)
{
    acp_sint16_t sPrev;

    __asm__ __volatile__ ("lock; cmpxchgw %1,%2"
                          : "=a"(sPrev)
                          : "q"(aWith),
                            "m"(*(volatile acp_sint16_t *)aAddr),
                            "0"(aCmp)
                          : "memory");

    return sPrev;
}

ACP_INLINE acp_sint16_t acpAtomicSet16(volatile void         *aAddr,
                                       volatile acp_sint16_t  aVal)
{
    /* xchg instructions always implies lock anyway
     * we dont need to use again it. */

    __asm__ __volatile__ ("xchgw %0,%1"
                          : "=r"(aVal)
                          : "m"(*(volatile acp_sint16_t *)aAddr), "0"(aVal)
                          : "memory");
    return aVal;
}

ACP_INLINE acp_sint16_t acpAtomicAdd16(volatile void         *aAddr,
                                       volatile acp_sint16_t  aVal)
{
    acp_sint16_t sTemp = aVal;

    __asm__ __volatile__ ("lock; xaddw %0,%1"
                          : "+r"(sTemp), "+m"(*(volatile acp_sint16_t *)aAddr)
                          :
                          : "memory");
    return sTemp + aVal;
}

ACP_INLINE acp_sint32_t acpAtomicCas32(volatile void         *aAddr,
                                       volatile acp_sint32_t  aWith,
                                       volatile acp_sint32_t  aCmp)
{
    acp_sint32_t sPrev;

    __asm__ __volatile__ ("lock; cmpxchgl %1,%2"
                          : "=a"(sPrev)
                          : "r"(aWith),
                            "m"(*(volatile acp_sint32_t *)aAddr),
                            "0"(aCmp)
                          : "memory");
    return sPrev;
}

ACP_INLINE acp_sint32_t acpAtomicSet32(volatile void         *aAddr,
                                       volatile acp_sint32_t  aVal)
{
    __asm__ __volatile__ ("xchgl %0,%1"
                          : "=r"(aVal)
                          : "m"(*(volatile acp_sint32_t *)aAddr), "0"(aVal)
                          : "memory");
    return aVal;
}

ACP_INLINE acp_sint32_t acpAtomicAdd32(volatile void         *aAddr,
                                       volatile acp_sint32_t  aVal)
{
    acp_sint32_t sTemp = aVal;

    __asm__ __volatile__ ("lock; xaddl %0,%1"
                          : "+r"(sTemp), "+m"(*(volatile acp_sint32_t *)aAddr)
                          :
                          : "memory");
    return sTemp + aVal;
}

#if defined(ACP_CFG_COMPILE_64BIT)

ACP_INLINE acp_sint64_t acpAtomicCas64(volatile void         *aAddr,
                                       volatile acp_sint64_t  aWith,
                                       volatile acp_sint64_t  aCmp)
{
    acp_sint64_t sPrev;

    __asm__ __volatile__ ("lock; cmpxchgq %1,%2"
                          : "=a"(sPrev)
                          : "r"(aWith),
                            "m"(*(volatile acp_sint64_t *)aAddr),
                            "0"(aCmp)
                          : "memory");
    return sPrev;
}

ACP_INLINE acp_sint64_t acpAtomicGet64(volatile void *aAddr)
{
    ACP_MEM_BARRIER();

    return *(volatile acp_sint64_t *)aAddr;
}

ACP_INLINE acp_sint64_t acpAtomicSet64(volatile void         *aAddr,
                                       volatile acp_sint64_t  aVal)
{
    __asm__ __volatile__ ("xchgq %0,%1"
                          : "=r"(aVal)
                          : "m"(*(volatile acp_sint64_t *)aAddr), "0"(aVal)
                          : "memory");
    return aVal;
}

ACP_INLINE acp_sint64_t acpAtomicAdd64(volatile void         *aAddr,
                                       volatile acp_sint64_t  aVal)
{
    acp_sint64_t sPrev = aVal;

    __asm__ __volatile__ ("lock; xaddq %0,%1"
                          : "+r"(sPrev), "+m"(*(volatile acp_sint64_t *)aAddr)
                          :
                          : "memory");
    return sPrev + aVal;
}

#else

ACP_EXPORT acp_sint64_t acpAtomicCas64(volatile void         *aAddr,
                                       volatile acp_sint64_t  aWith,
                                       volatile acp_sint64_t  aCmp);

ACP_INLINE acp_sint64_t acpAtomicGet64(volatile void *aAddr)
{
    acp_sint64_t sVal;

    __asm__ __volatile__ ("mov %%ebx,%%eax\n\t"
                          "mov %%ecx,%%edx\n\t"
                          "lock; cmpxchg8b %1"
                          : "=&A"(sVal)
                          : "m"(*(volatile acp_sint64_t *)aAddr)
                          : "cc");
    return sVal;
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

ACP_INLINE acp_sint64_t acpAtomicAdd64(volatile void         *aAddr,
                                       volatile acp_sint64_t  aVal)
{
    acp_sint64_t sPrev;
    acp_sint64_t sTemp;

    do
    {
        sPrev = *(volatile acp_sint64_t *)aAddr;
        sTemp = acpAtomicCas64(aAddr, sPrev + aVal, sPrev);
    } while (sTemp != sPrev);

    return sPrev + aVal;
}

#endif  /* ACP_CFG_COMPILE_64BIT */
 
/*
 * Not GNUC or NOINLINE given.
 */
#else

ACP_EXPORT acp_sint16_t acpAtomicCas16(volatile void         *aAddr,
                                       volatile acp_sint16_t  aWith,
                                       volatile acp_sint16_t  aCmp);

ACP_EXPORT acp_sint16_t acpAtomicSet16(volatile void         *aAddr,
                                       volatile acp_sint16_t  aVal);

ACP_EXPORT acp_sint16_t acpAtomicAdd16(volatile void         *aAddr,
                                       volatile acp_sint16_t  aVal);

ACP_EXPORT acp_sint32_t acpAtomicCas32(volatile void         *aAddr,
                                       volatile acp_sint32_t  aWith,
                                       volatile acp_sint32_t  aCmp);

ACP_EXPORT acp_sint32_t acpAtomicSet32(volatile void         *aAddr,
                                       volatile acp_sint32_t  aVal);

ACP_EXPORT acp_sint32_t acpAtomicAdd32(volatile void         *aAddr,
                                       volatile acp_sint32_t  aVal);

ACP_EXPORT acp_sint64_t acpAtomicCas64(volatile void         *aAddr,
                                       volatile acp_sint64_t  aWith,
                                       volatile acp_sint64_t  aCmp);

ACP_EXPORT acp_sint64_t acpAtomicGet64(volatile void *aAddr);

ACP_EXPORT acp_sint64_t acpAtomicSet64(volatile void         *aAddr,
                                       volatile acp_sint64_t  aVal);

ACP_EXPORT acp_sint64_t acpAtomicAdd64(volatile void         *aAddr,
                                       volatile acp_sint64_t  aVal);

#endif

#endif
