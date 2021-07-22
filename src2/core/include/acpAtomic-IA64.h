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
 * $Id: acpAtomic-IA64.h 8779 2009-11-20 11:06:58Z djin $
 ******************************************************************************/

#if !defined(_O_ACP_ATOMIC_IA64_H_)
#define _O_ACP_ATOMIC_IA64_H_


ACP_EXPORT acp_sint16_t acpAtomicSet16EXPORT(volatile void         *aAddr,
                                             volatile acp_sint16_t  aVal);
ACP_EXPORT acp_sint16_t acpAtomicAdd16EXPORT(volatile void         *aAddr,
                                             volatile acp_sint16_t  aVal);

#if !defined(ACP_CFG_COMPILE_32BIT)
ACP_EXPORT acp_sint32_t acpAtomicCas32EXPORT(volatile void         *aAddr,
                                             volatile acp_sint32_t  aWith,
                                             volatile acp_sint32_t  aCmp);
#endif

ACP_EXPORT acp_sint32_t acpAtomicSet32EXPORT(volatile void         *aAddr,
                                             volatile acp_sint32_t  aVal);
ACP_EXPORT acp_sint32_t acpAtomicAdd32EXPORT(volatile void         *aAddr,
                                             volatile acp_sint32_t  aVal);
ACP_EXPORT acp_sint64_t acpAtomicCas64EXPORT(volatile void         *aAddr,
                                             volatile acp_sint64_t  aWith,
                                             volatile acp_sint64_t  aCmp);
ACP_EXPORT acp_sint64_t acpAtomicGet64EXPORT(volatile void *aAddr);
ACP_EXPORT acp_sint64_t acpAtomicSet64EXPORT(volatile void         *aAddr,
                                             volatile acp_sint64_t  aVal);
ACP_EXPORT acp_sint64_t acpAtomicAdd64EXPORT(volatile void         *aAddr,
                                             volatile acp_sint64_t  aVal);


#if defined(__GNUC__) && defined(ALTI_CFG_OS_LINUX)
ACP_INLINE acp_sint16_t acpAtomicCas16(volatile void         *aAddr,
                                       volatile acp_sint16_t  aWith,
                                       volatile acp_sint16_t  aCmp)
{
	return __sync_val_compare_and_swap((acp_sint16_t*)aAddr, aCmp, aWith);
}
#else
ACP_EXPORT acp_sint16_t acpAtomicCas16(volatile void         *aAddr,
                                       volatile acp_sint16_t  aWith,
                                       volatile acp_sint16_t  aCmp);
#endif

#if defined(__GNUC__) && !defined(ACP_CFG_NOINLINE)

ACP_INLINE acp_sint16_t acpAtomicSet16(volatile void         *aAddr,
                                       volatile acp_sint16_t  aVal)
{
    acp_sint16_t sPrev;

    __asm__ __volatile__ ("xchg2 %0=%1,%2"
                          : "=r"(sPrev), "=m"(*(volatile acp_sint32_t *)aAddr)
                          : "r"(aVal)
                          : "memory");
    return sPrev;
}

ACP_INLINE acp_sint16_t acpAtomicAdd16(volatile void         *aAddr,
                                       volatile acp_sint16_t  aVal)
{
    acp_sint16_t sPrev;
    acp_sint16_t sTemp;

    do
    {
        sPrev = *(volatile acp_sint16_t *)aAddr;
        sTemp = acpAtomicCas16(aAddr, sPrev + aVal, sPrev);
    } while (sTemp != sPrev);

    return sPrev + aVal;
}

#if defined(ACP_CFG_COMPILE_32BIT)
ACP_EXPORT acp_sint32_t acpAtomicCas32(volatile void         *aAddr,
                                       volatile acp_sint32_t  aWith,
                                       volatile acp_sint32_t  aCmp);
#else
ACP_INLINE acp_sint32_t acpAtomicCas32(volatile void         *aAddr,
                                       volatile acp_sint32_t  aWith,
                                       volatile acp_sint32_t  aCmp)
{
    acp_slong_t sPrev;

    __asm__ __volatile__ ("mov ar.ccv=%3;; cmpxchg4.acq %0=[%1],%2,ar.ccv"
                          : "=r"(sPrev)
                          : "r"(aAddr), "r"(aWith), "r"(aCmp)
                          : "memory");

    return sPrev;
}
#endif

ACP_INLINE acp_sint32_t acpAtomicSet32(volatile void         *aAddr,
                                       volatile acp_sint32_t  aVal)
{
    acp_slong_t sPrev;

    __asm__ __volatile__ ("xchg4 %0=%1,%2"
                          : "=r"(sPrev), "=m"(*(volatile acp_sint32_t *)aAddr)
                          : "r"(aVal)
                          : "memory");
    return sPrev;
}

ACP_INLINE acp_sint32_t acpAtomicAdd32(volatile void         *aAddr,
                                       volatile acp_sint32_t  aVal)
{
    acp_sint32_t sPrev;
    acp_sint32_t sTemp;

    do
    {
        sPrev = *(volatile acp_sint32_t *)aAddr;
        sTemp = acpAtomicCas32(aAddr, sPrev + aVal, sPrev);
    } while (sTemp != sPrev);

    return sPrev + aVal;
}

ACP_INLINE acp_sint64_t acpAtomicCas64(volatile void         *aAddr,
                                       volatile acp_sint64_t  aWith,
                                       volatile acp_sint64_t  aCmp)
{
    acp_sint64_t sPrev;

#if defined(ACP_CFG_COMPILE_32BIT)
    __asm__ __volatile__ ("addp4 %0=0,%1\n\t"
                          "mov ar.ccv=%3;; cmpxchg8.acq %0=[%0],%2,ar.ccv"
                          : "=&r"(sPrev)
                          : "r"(aAddr), "r"(aWith), "r"(aCmp)
                          : "memory");
#else
    __asm__ __volatile__ ("mov ar.ccv=%3;; cmpxchg8.acq %0=[%1],%2,ar.ccv"
                          : "=r"(sPrev)
                          : "r"(aAddr), "r"(aWith), "r"(aCmp)
                          : "memory");
#endif

    return sPrev;
}

ACP_INLINE acp_sint64_t acpAtomicSet64(volatile void         *aAddr,
                                       volatile acp_sint64_t  aVal)
{
    acp_sint64_t sPrev;

    __asm__ __volatile__ ("xchg8 %0=%1,%2"
                          : "=r"(sPrev), "=m"(*(volatile acp_sint64_t *)aAddr)
                          : "r"(aVal)
                          : "memory");
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

ACP_INLINE acp_sint64_t acpAtomicGet64(volatile void *aAddr)
{
#if defined(ACP_CFG_COMPILE_64BIT)
    ACP_MEM_BARRIER();

    return *(volatile acp_sint64_t *)aAddr;
#else
    acp_sint64_t sPrev = *(volatile acp_sint64_t *)aAddr;

    return acpAtomicCas64(aAddr, sPrev, sPrev);
#endif  /* ACP_CFG_COMPILE_64BIT */
}

#else
/*
 * Code for non-gcc or with NOINLINE directive
 */

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

#if defined(ACP_CFG_COMPILE_32BIT)
ACP_EXPORT acp_sint32_t acpAtomicCas32(volatile void         *aAddr,
                                       volatile acp_sint32_t  aWith,
                                       volatile acp_sint32_t  aCmp);
#else
ACP_INLINE acp_sint32_t acpAtomicCas32(volatile void         *aAddr,
                                       volatile acp_sint32_t  aWith,
                                       volatile acp_sint32_t  aCmp)
{
    return acpAtomicCas32EXPORT(aAddr, aWith, aCmp);
}
#endif

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

ACP_INLINE acp_sint64_t acpAtomicCas64(volatile void         *aAddr,
                                       volatile acp_sint64_t  aWith,
                                       volatile acp_sint64_t  aCmp)
{
    return acpAtomicCas64EXPORT(aAddr, aWith, aCmp);
}

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
