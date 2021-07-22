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
 * $Id: acpAtomic-ALPHA.h 8779 2009-11-20 11:06:58Z djin $
 ******************************************************************************/

#if !defined(_O_ACP_ATOMIC_ALPHA_H_)
#define _O_ACP_ATOMIC_ALPHA_H_

/*
 * only for DEC TRU64 COFF format binary
 */

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
ACP_EXPORT acp_sint64_t acpAtomicCas64EXPORT(volatile void         *aAddr,
                                             volatile acp_sint64_t  aWith,
                                             volatile acp_sint64_t  aCmp);
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
    acp_sint16_t sCmp;
    acp_sint16_t sTmp;
    acp_sint16_t sNew;
    acp_ulong_t  sAddrTmp;

    __asm__ __volatile__ ("andnot %[__sAddr16],7,%[sAddrTmp]\n\t"
                          "inswl %[__aWith],%[__sAddr16],%[sNew]\n\t"
                          "1: ldq_l %[sTmp],0(%[sAddrTmp])\n\t"
                          "extwl %[sTmp],%[__sAddr16],%[sPrev]\n\t"
                          "cmpeq %[sPrev],%[__aCmp],%[sCmp]\n\t"
                          "beq %[sCmp],2f\n\t"
                          "mskwl %[sTmp],%[__sAddr16],%[sTmp]\n\t"
                          "or %[sNew],%[sTmp],%[sTmp]\n\t"
                          "stq_c %[sTmp],0(%[sAddrTmp])\n\t"
                          "beq %[sTmp],1b\n\t"
                          "mb\n\t"
                          "2:"
                          : [sPrev] "=&r" (sPrev),
                            [sNew] "=&r" (sNew),
                            [sTmp] "=&r" (sTmp),
                            [sCmp] "=&r" (sCmp),
                            [sAddrTmp] "=&r" (sAddrTmp)
                          : [__sAddr16] "r" (aAddr),
                            [__aCmp] "Ir" (aCmp),
                            [__aWith] "r" (aWith)
                          : "memory");

    return sPrev;
}

ACP_INLINE acp_sint16_t acpAtomicSet16(volatile void         *aAddr,
                                       volatile acp_sint16_t  aVal)
{
    acp_sint16_t sRet;
    acp_sint16_t sTmp;
    acp_sint16_t sVal;
    acp_ulong_t  sAddrTmp;

    __asm__ __volatile__ ("andnot %[__sAddr16],7,%[sAddrTmp]\n\t"
                          "inswl %[__aVal],%[__sAddr16],%[sVal]\n\t"
                          "1: ldq_l %[sTmp],0(%[sAddrTmp])\n\t"
                          "extwl %[sTmp],%[__sAddr16],%[sRet]\n\t"
                          "mskwl %[sTmp],%[__sAddr16],%[sTmp]\n\t"
                          "or %[sVal],%[sTmp],%[sTmp]\n\t"
                          "stq_c %[sTmp],0(%[sAddrTmp])\n\t"
                          "beq %[sTmp],1b\n\t"
                          "mb\n\t"
                          : [sRet] "=&r" (sRet),
                            [sVal] "=&r" (sVal),
                            [sTmp] "=&r" (sTmp),
                            [sAddrTmp] "=&r" (sAddrTmp)
                          : [__sAddr16] "r" (aAddr),
                            [__aVal] "r" (aVal)
                          : "memory");

    return sRet;
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

ACP_INLINE acp_sint32_t acpAtomicCas32(volatile void         *aAddr,
                                       volatile acp_sint32_t  aWith,
                                       volatile acp_sint32_t  aCmp)
{
    acp_sint32_t sPrev;
    acp_sint32_t sTemp;

    __asm__ __volatile__ ("1:\n\t"
                          "ldl_l %0,%5\n\t"
                          "cmpeq %0,%3,%1\n\t"
                          "beq %1,2f\n\t"
                          "mov %4,%1\n\t"
                          "stl_c %1,%2\n\t"
                          "beq %1,1b\n\t"
                          "mb\n\t"
                          "2:\n\t"
                          : "=&r"(sPrev),
                            "=&r"(sTemp),
                            "=m"(*(volatile acp_sint32_t *)aAddr)
                          : "r"((acp_slong_t)aCmp),
                            "r"(aWith),
                            "m"(*(volatile acp_sint32_t *)aAddr)
                          : "memory");
    return sPrev;
}

ACP_INLINE acp_sint32_t acpAtomicSet32(volatile void         *aAddr,
                                       volatile acp_sint32_t  aVal)
{
    acp_sint32_t sTemp;

    __asm__ __volatile__ ("1:\n\t"
                          "ldl_l %0,%4\n\t"
                          "bis $31,%3,%1\n\t"
                          "stl_c %1,%2\n\t"
                          "beq %1,1b\n"
                          "mb\n\t"
                          : "=&r"(aVal),
                            "=&r"(sTemp),
                            "=m"(*(volatile acp_sint32_t *)aAddr)
                          : "rI"(aVal), "m"(*(volatile acp_sint32_t *)aAddr)
                          : "memory");
    return aVal;
}

ACP_INLINE acp_sint32_t acpAtomicAdd32(volatile void         *aAddr,
                                       volatile acp_sint32_t  aVal)
{
    acp_sint32_t sResult;
    acp_sint32_t sTemp;

    __asm__ __volatile__ ("1:\n\t"
                          "ldl_l %0,%1\n\t"
                          "addl %0,%3,%2\n\t"
                          "addl %0,%3,%0\n\t"
                          "stl_c %0,%1\n\t"
                          "beq %0,1b\n\t"
                          "mb\n\t"
                          : "=&r"(sTemp),
                            "=m"(*(volatile acp_sint32_t *)aAddr),
                            "=&r"(sResult)
                          : "Ir"(aVal), "m"(*(volatile acp_sint32_t *)aAddr)
                          : "memory");
    return sResult;
}

ACP_INLINE acp_sint64_t acpAtomicCas64(volatile void         *aAddr,
                                       volatile acp_sint64_t  aWith,
                                       volatile acp_sint64_t  aCmp)
{
    acp_sint64_t sPrev;
    acp_sint64_t sTemp;

    __asm__ __volatile__ ("1:\n\t"
                          "ldq_l %0,%5\n\t"
                          "cmpeq %0,%3,%1\n\t"
                          "beq %1,2f\n\t"
                          "mov %4,%1\n\t"
                          "stq_c %1,%2\n\t"
                          "beq %1,1b\n\t"
                          "mb\n\t"
                          "2:\n\t"
                          : "=&r"(sPrev),
                            "=&r"(sTemp),
                            "=m"(*(volatile acp_sint64_t *)aAddr)
                          : "r"((acp_slong_t)aCmp),
                            "r"(aWith),
                            "m"(*(volatile acp_sint64_t *)aAddr)
                          : "memory");
    return sPrev;
}

ACP_INLINE acp_sint64_t acpAtomicSet64(volatile void         *aAddr,
                                       volatile acp_sint64_t  aVal)
{
    acp_sint64_t sTemp;

    __asm__ __volatile__ ("1:\n\t"
                          "ldq_l %0,%4\n\t"
                          "bis $31,%3,%1\n\t"
                          "stq_c %1,%2\n\t"
                          "beq %1,1b\n"
                          "mb\n\t"
                          : "=&r"(aVal),
                            "=&r"(sTemp),
                            "=m"(*(volatile acp_sint64_t *)aAddr)
                          : "rI"(aVal), "m"(*(volatile acp_sint64_t *)aAddr)
                          : "memory");
    return aVal;
}

ACP_INLINE acp_sint64_t acpAtomicAdd64(volatile void         *aAddr,
                                       volatile acp_sint64_t  aVal)
{
    acp_sint64_t sResult;
    acp_sint64_t sTemp;

    __asm__ __volatile__ ("1:\n\t"
                          "ldq_l %0,%1\n\t"
                          "addq %0,%3,%2\n\t"
                          "addq %0,%3,%0\n\t"
                          "stq_c %0,%1\n\t"
                          "beq %0,1b\n\t"
                          "mb\n\t"
                          : "=&r"(sTemp),
                            "=m"(*(volatile acp_sint64_t *)aAddr),
                            "=&r"(sResult)
                          : "Ir"(aVal), "m"(*(volatile acp_sint64_t *)aAddr)
                          : "memory");
    return sResult;
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
