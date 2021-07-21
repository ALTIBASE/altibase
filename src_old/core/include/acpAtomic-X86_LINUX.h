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
 * $Id: acpAtomic-X86_LINUX.h 8779 2009-11-20 11:06:58Z djin $
 ******************************************************************************/

#if !defined(_O_ACP_ATOMIC_X86_LINUX_H_)
#define _O_ACP_ATOMIC_X86_LINUX_H_

ACP_EXPORT acp_sint16_t acpAtomicCas16EXPORT(volatile void         *aAddr,
                                             volatile acp_sint16_t  aWith,
                                             volatile acp_sint16_t  aCmp);
ACP_EXPORT acp_sint16_t acpAtomicSet16EXPORT(volatile void         *aAddr,
                                             volatile acp_sint16_t  aVal);
ACP_EXPORT acp_sint16_t acpAtomicAdd16EXPORT(volatile void         *aAddr,
                                             volatile acp_sint16_t  aVal);
ACP_EXPORT acp_sint32_t acpAtomicCas32EXPORT(volatile void         *aAddr,
                                             volatile acp_sint32_t  aWith,
                                             volatile acp_sint32_t  aCmp);
ACP_EXPORT acp_sint32_t acpAtomicSet32EXPORT(volatile void         *aAddr,
                                             volatile acp_sint32_t  aVal);
ACP_EXPORT acp_sint32_t acpAtomicAdd32EXPORT(volatile void         *aAddr,
                                             volatile acp_sint32_t  aVal);
#if defined(ACP_CFG_COMPILE_64BIT)
ACP_EXPORT acp_sint64_t acpAtomicCas64EXPORT(volatile void         *aAddr,
                                             volatile acp_sint64_t  aWith,
                                             volatile acp_sint64_t  aCmp);
#endif
ACP_EXPORT acp_sint64_t acpAtomicGet64EXPORT(volatile void *aAddr);
ACP_EXPORT acp_sint64_t acpAtomicSet64EXPORT(volatile void         *aAddr,
                                             volatile acp_sint64_t  aVal);
ACP_EXPORT acp_sint64_t acpAtomicAdd64EXPORT(volatile void         *aAddr,
                                             volatile acp_sint64_t  aVal);

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

#else

/*
 * Not GNUC or NOINLINE given.
 */

ACP_INLINE acp_sint16_t acpAtomicCas16(volatile void         *aAddr,
                                       volatile acp_sint16_t  aWith,
                                       volatile acp_sint16_t  aCmp)
{
    return acpAtomicCas16EXPORT(aAddr, aWith, aCmp);
}

ACP_INLINE acp_sint16_t acpAtomicSet16(volatile void         *aAddr,
                                       volatile acp_sint16_t  aVal)
{
    return acpAtomicSet16EXPORT(aAddr, aVal);
}

ACP_INLINE acp_sint16_t acpAtomicAdd16(volatile void         *aAddr,
                                       volatile acp_sint16_t  aVal)
{
    return acpAtomicAdd16EXPORT(aAddr, aVal);
}

ACP_INLINE acp_sint32_t acpAtomicCas32(volatile void         *aAddr,
                                       volatile acp_sint32_t  aWith,
                                       volatile acp_sint32_t  aCmp)
{
    return acpAtomicCas32EXPORT(aAddr, aWith, aCmp);
}

ACP_INLINE acp_sint32_t acpAtomicSet32(volatile void         *aAddr,
                                       volatile acp_sint32_t  aVal)
{
    return acpAtomicSet32EXPORT(aAddr, aVal);
}

ACP_INLINE acp_sint32_t acpAtomicAdd32(volatile void         *aAddr,
                                       volatile acp_sint32_t  aVal)
{
    return acpAtomicAdd32EXPORT(aAddr, aVal);
}

#if defined(ACP_CFG_COMPILE_64BIT)

ACP_INLINE acp_sint64_t acpAtomicCas64(volatile void         *aAddr,
                                       volatile acp_sint64_t  aWith,
                                       volatile acp_sint64_t  aCmp)
{
    return acpAtomicCas64EXPORT(aAddr, aWith, aCmp);
}

#else

ACP_EXPORT acp_sint64_t acpAtomicCas64(volatile void         *aAddr,
                                       volatile acp_sint64_t  aWith,
                                       volatile acp_sint64_t  aCmp);

#endif

ACP_INLINE acp_sint64_t acpAtomicGet64(volatile void *aAddr)
{
    return acpAtomicGet64EXPORT(aAddr);
}

ACP_INLINE acp_sint64_t acpAtomicSet64(volatile void         *aAddr,
                                       volatile acp_sint64_t  aVal)
{
    return acpAtomicSet64EXPORT(aAddr, aVal);
}

ACP_INLINE acp_sint64_t acpAtomicAdd64(volatile void         *aAddr,
                                       volatile acp_sint64_t  aVal)
{
    return acpAtomicAdd64EXPORT(aAddr, aVal);
}

#endif



#endif
