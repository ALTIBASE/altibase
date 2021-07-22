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
 * $Id: acpAtomic-ARM64_LINUX.h 8779 2009-11-20 11:06:58Z djin $
 ******************************************************************************/

#if !defined(_O_ACP_ATOMIC_ARM64_LINUX_H_)
#define _O_ACP_ATOMIC_ARM64_LINUX_H_

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



// Inline Assembly package for ARMv8-aarch64 RISC
// 07/13/2018, Joonho Park
ACP_INLINE acp_sint16_t acpAtomicCas16(volatile void         *aAddr,
                                       volatile acp_sint16_t  aWith,
                                       volatile acp_sint16_t  aCmp)
{
    acp_sint16_t sPrev, sRes;

    do {
            __asm__ __volatile__ (
                            "ldxrh %w[prev], [%3]\n"
                            "mov   %w[result], #0\n"
                            "cmp   %w[prev], %w[cmp]\n"
                            "bne   1f\n"
                            "stxrh %w[result], %w[with], [%3]\n"
                            "1:"
                            : [result]"=&r"(sRes), [prev]"=&r"(sPrev), "+Qo"(*(acp_sint16_t *)aAddr)
                            : "r"(aAddr), [cmp]"Ir"(aCmp), [with]"r"(aWith)
                            : "cc");
    } while (__builtin_expect(sRes != 0, 0));

    return sPrev;
}

ACP_INLINE acp_sint16_t acpAtomicSet16(volatile void         *aAddr,
                                       volatile acp_sint16_t  aVal)
{
    acp_sint16_t sPrev, sRes;

    do {
            __asm__ __volatile__ (
                            "ldxrh %w[prev], [%3]\n"
                            "mov   %w[result], #0\n"
                            "stxrh %w[result], %w[with], [%3]\n"
                            : [result]"=&r"(sRes), [prev]"=&r"(sPrev), "+Qo"(*(acp_sint16_t *)aAddr)
                            : "r"(aAddr), [with]"r"(aVal)
                            : "cc");
    } while (__builtin_expect(sRes != 0, 0));

    return aVal;
}

ACP_INLINE acp_sint16_t acpAtomicAdd16(volatile void         *aAddr,
                                       volatile acp_sint16_t  aVal)
{
    acp_sint16_t sTemp, sRes;

    do {
            __asm__ __volatile__ (
                            "ldxrh %w[temp], [%3]\n"
                            "add   %w[temp], %w[temp], %w[inc]\n"
                            "mov   %w[result], #0\n"
                            "stxrh %w[result], %w[temp], [%3]\n"
                            : [result]"=&r"(sRes), [temp]"=&r"(sTemp), "+Qo"(*(acp_sint16_t *)aAddr)
                            : "r"(aAddr), [inc]"r"(aVal)
                            : "cc");
    } while (__builtin_expect(sRes != 0, 0));

    return sTemp;
}

ACP_INLINE acp_sint32_t acpAtomicCas32(volatile void         *aAddr,
                                       volatile acp_sint32_t  aWith,
                                       volatile acp_sint32_t  aCmp)
{
    acp_sint32_t sPrev, sRes;

    do {
            __asm__ __volatile__ (
                            "ldxr  %w[prev], [%3]\n"
                            "mov   %w[result], #0\n"
                            "cmp   %w[prev], %w[cmp]\n"
                            "bne   1f\n"
                            "stxr  %w[result], %w[with], [%3]\n"
                            "1:"
                            : [result]"=&r"(sRes), [prev]"=&r"(sPrev), "+Qo"(*(acp_sint32_t *)aAddr)
                            : "r"(aAddr), [cmp]"Ir"(aCmp), [with]"r"(aWith)
                            : "cc");
    } while (__builtin_expect(sRes != 0, 0));

    return sPrev;
}

ACP_INLINE acp_sint32_t acpAtomicSet32(volatile void         *aAddr,
                                       volatile acp_sint32_t  aVal)
{
    acp_sint32_t sPrev, sRes;

    do {
            __asm__ __volatile__ (
                            "ldxr  %w[prev], [%3]\n"
                            "mov   %w[result], #0\n"
                            "stxr  %w[result], %w[with], [%3]\n"
                            : [result]"=&r"(sRes), [prev]"=&r"(sPrev), "+Qo"(*(acp_sint32_t *)aAddr)
                            : "r"(aAddr), [with]"r"(aVal)
                            : "cc");
    } while (__builtin_expect(sRes != 0, 0));

    return aVal;
}

ACP_INLINE acp_sint32_t acpAtomicAdd32(volatile void         *aAddr,
                                       volatile acp_sint32_t  aVal)
{
    acp_sint32_t sTemp, sRes;

    do {
            __asm__ __volatile__ (
                            "ldxr  %w[temp], [%3]\n"
                            "add   %w[temp], %w[temp], %w[inc]\n"
                            "mov   %w[result], #0\n"
                            "stxr  %w[result], %w[temp], [%3]\n"
                            : [result]"=&r"(sRes), [temp]"=&r"(sTemp), "+Qo"(*(acp_sint32_t *)aAddr)
                            : "r"(aAddr), [inc]"r"(aVal)
                            : "cc");
    } while (__builtin_expect(sRes != 0, 0));

    return sTemp;
}

ACP_INLINE acp_sint64_t acpAtomicCas64(volatile void         *aAddr,
                                       volatile acp_sint64_t  aWith,
                                       volatile acp_sint64_t  aCmp)
{
    acp_sint64_t sPrev, sRes;

    do {
            __asm__ __volatile__ (
                            "ldxr  %[prev], [%3]\n"
                            "mov   %[result], #0\n"
                            "cmp   %[prev], %[cmp]\n"
                            "bne   1f\n"
                            "stxr  %w[result], %[with], [%3]\n"
                            "1:"
                            : [result]"=&r"(sRes), [prev]"=&r"(sPrev), "+Qo"(*(acp_sint64_t *)aAddr)
                            : "r"(aAddr), [cmp]"Ir"(aCmp), [with]"r"(aWith)
                            : "cc");
    } while (__builtin_expect(sRes != 0, 0));

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
    acp_sint64_t sPrev, sRes;

    do {
            __asm__ __volatile__ (
                            "ldxr  %[prev], [%3]\n"
                            "mov   %[result], #0\n"
                            "stxr  %w[result], %[with], [%3]\n"
                            : [result]"=&r"(sRes), [prev]"=&r"(sPrev), "+Qo"(*(acp_sint64_t *)aAddr)
                            : "r"(aAddr), [with]"r"(aVal)
                            : "cc");
    } while (__builtin_expect(sRes != 0, 0));

    return aVal;
}

ACP_INLINE acp_sint64_t acpAtomicAdd64(volatile void         *aAddr,
                                       volatile acp_sint64_t  aVal)
{
    acp_sint64_t sTemp, sRes;

    do {
            __asm__ __volatile__ (
                            "ldxr  %[temp], [%3]\n"
                            "add   %[temp], %[temp], %[inc]\n"
                            "mov   %[result], #0\n"
                            "stxr  %w[result], %[temp], [%3]\n"
                            : [result]"=&r"(sRes), [temp]"=&r"(sTemp), "+Qo"(*(acp_sint64_t *)aAddr)
                            : "r"(aAddr), [inc]"r"(aVal)
                            : "cc");
    } while (__builtin_expect(sRes != 0, 0));

    return sTemp;
}
// end of Inline Assembly package

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
