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
 * $Id: acpAtomic-PPC.c 9884 2010-02-03 06:50:04Z gurugio $
 ******************************************************************************/

#include <acpAtomic.h>
#include <aceAssert.h>

#if !defined(__GNUC__) || defined(ACP_CFG_NOINLINE)
#error This file must be compiled with GNU C compiler and no NOINLINE macro!
#endif

#if !defined(ACP_CFG_COMPILE_64BIT)

#define ACP_ATOMIC_TEST_SIGNAL(aTest, aSignal) \
    do {                        \
        if((aTest))             \
        {                       \
            raise((aSignal));   \
        }                       \
        else                    \
        {                       \
            /* Do nothing */    \
        }                       \
    } while(ACP_FALSE)



typedef union acpAtomicLong
{
    acp_sint32_t mInt32[2];
    acp_sint64_t mInt64;
} acpAtomicLong;

ACP_EXPORT acp_sint64_t acpAtomicGet64(volatile void *aAddr)
{
    acpAtomicLong sPrev;
    ACP_ATOMIC_TEST_SIGNAL(((acp_ulong_t)aAddr & 0x00000007) != 0, SIGBUS);

    __asm__ __volatile__ ("eieio\n\t"
                          "ldarx  %1,0,%2\n\t"  /* load sPrev and reserve     */
                          "srdi   %0,%1,32\n\t" /* capture high word of sPrev */
                          "stdcx. %1,0,%2\n\t"  /* store conditional sPrev    */
                          "bne-   $-12\n\t"     /* retry if lost reservation  */
                          "isync"
                          : "=&r"(sPrev.mInt32[0]),
                            "=&r"(sPrev.mInt32[1])
                          : "b"(aAddr)
                          : "memory");
    return sPrev.mInt64;
}

ACP_EXPORT acp_sint64_t acpAtomicSet64(volatile void *aAddr, acp_sint64_t aVal)
{
    acpAtomicLong sPrev;
    acpAtomicLong sVal;
    acp_sint32_t  sTemp;
    ACP_ATOMIC_TEST_SIGNAL(((acp_ulong_t)aAddr & 0x00000007) != 0, SIGBUS);

    sVal.mInt64 = aVal;

    __asm__ __volatile__ ("eieio\n\t"
                          "ldarx  %1,0,%5\n\t"  /* load sPrev and reserve     */
                          "srdi   %0,%1,32\n\t" /* capture high word of sPrev */
                          "sldi   %2,%3,32\n\t" /* load high word of aVal     */
                          "or     %2,%2,%4\n\t" /* load low word of aVal      */
                          "stdcx. %2,0,%5\n\t"  /* store conditional aVal     */
                          "bne-   $-20\n\t"     /* retry if lost reservation  */
                          "isync"
                          : "=&r"(sPrev.mInt32[0]),
                            "=&r"(sPrev.mInt32[1]),
                            "=&r"(sTemp)
                          : "r"(sVal.mInt32[0]),
                            "r"(sVal.mInt32[1]),
                            "b"(aAddr)
                          : "memory");
    return sPrev.mInt64;
}

ACP_EXPORT acp_sint64_t acpAtomicAdd64(volatile void *aAddr, acp_sint64_t aVal)
{
    acpAtomicLong sResult;
    acpAtomicLong sVal;
    acp_sint32_t  sTemp;
    ACP_ATOMIC_TEST_SIGNAL(((acp_ulong_t)aAddr & 0x00000007) != 0, SIGBUS);

    sVal.mInt64 = aVal;

    __asm__ __volatile__ ("eieio\n\t"
                          "ldarx  %1,0,%5\n\t"  /* load prev and reserve      */
                          "srdi   %0,%1,32\n\t" /* capture high word of prev  */
                          "addc   %1,%1,%4\n\t" /* add low word of aVal       */
                          "adde   %0,%0,%3\n\t" /* add high word of aVal      */
                          "clrldi %1,%1,32\n\t" /* clear high word of result  */
                          "sldi   %2,%0,32\n\t" /* load high word of result   */
                          "or     %2,%2,%1\n\t" /* load low word of result    */
                          "stdcx. %2,0,%5\n\t"  /* store conditional result   */
                          "bne-   $-32\n\t"     /* retry if lost reservation  */
                          "isync"
                          : "=&r"(sResult.mInt32[0]),
                            "=&r"(sResult.mInt32[1]),
                            "=&r"(sTemp)
                          : "r"(sVal.mInt32[0]),
                            "r"(sVal.mInt32[1]),
                            "b"(aAddr)
                          : "memory");
    return sResult.mInt64;
}

ACP_EXPORT acp_sint64_t acpAtomicCas64(volatile void *aAddr,
                                       acp_sint64_t   aWith,
                                       acp_sint64_t   aCmp)
{
    acpAtomicLong sPrev;
    acpAtomicLong sWith;
    acpAtomicLong sCmp;
    acp_sint32_t  sTemp;
    ACP_ATOMIC_TEST_SIGNAL(((acp_ulong_t)aAddr & 0x00000007) != 0, SIGBUS);

    sWith.mInt64 = aWith;
    sCmp.mInt64  = aCmp;

    __asm__ __volatile__ ("eieio\n\t"
                          "ldarx  %1,0,%5\n\t"  /* load sPrev and reserve     */
                          "srdi   %0,%1,32\n\t" /* capture high word of sPrev */
                          "sldi   %2,%6,32\n\t" /* load high word of aCmp     */
                          "or     %2,%2,%7\n\t" /* load low word of aCmp      */
                          "cmpd   %1,%2\n\t"    /* compare sPrev with aCmp    */
                          "beq-   $+16\n\t"     /* try swap if equal          */
                          "stdcx. %1,0,%5\n\t"  /* store conditional sPrev    */
                          "bne-   $-28\n\t"     /* retry if lost reservation  */
                          "b      $+20\n\t"     /* finish                     */
                          "sldi   %2,%3,32\n\t" /* load high word of aWith    */
                          "or     %2,%2,%4\n\t" /* load low word of aWith     */
                          "stdcx. %2,0,%5\n\t"  /* store conditional aWith    */
                          "bne-   $-48\n\t"     /* retry if lost reservation  */
                          "isync"
                          : "=&r"(sPrev.mInt32[0]),
                            "=&r"(sPrev.mInt32[1]),
                            "=&r"(sTemp)
                          : "r"(sWith.mInt32[0]),
                            "r"(sWith.mInt32[1]),
                            "b"(aAddr),
                            "r"(sCmp.mInt32[0]),
                            "r"(sCmp.mInt32[1])
                          : "memory");
    return sPrev.mInt64;
}

#endif

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

#if defined(ACP_CFG_COMPILE_64BIT)

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

#endif

