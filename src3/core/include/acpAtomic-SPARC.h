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
 * $Id: acpAtomic-SPARC.h 9125 2009-12-10 05:43:15Z djin $
 ******************************************************************************/

#if !defined(_O_ACP_ATOMIC_SPARC_H_)
#define _O_ACP_ATOMIC_SPARC_H_

/*
 * sparc cas instruction is supported from v8plus architecture
 * gcc needs -mcpu=v9 flag for 32-bit compile
 */

#if (ALTI_CFG_OS_MAJOR >= 2) && \
    (ALTI_CFG_OS_MINOR >= 10)
/*******************************************************************************
 * SunOS 5.10 (Solaris 2.10) and higher
 ******************************************************************************/

ACP_INLINE acp_sint32_t acpAtomicCas32(volatile void         *aAddr,
                                       volatile acp_sint32_t  aWith,
                                       volatile acp_sint32_t  aCmp)
{
    return (acp_sint32_t)
        atomic_cas_32((acp_uint32_t*)aAddr,
                      (acp_uint32_t)aCmp,
                      (acp_uint32_t)aWith);
}

ACP_INLINE acp_sint16_t acpAtomicCas16(volatile void         *aAddr,
                                       volatile acp_sint16_t  aWith,
                                       volatile acp_sint16_t  aCmp)
{
    return (acp_sint16_t)
        atomic_cas_16((volatile acp_uint16_t*)aAddr,
                      (acp_uint16_t)aCmp,
                      (acp_uint16_t)aWith);
}

ACP_INLINE acp_sint16_t acpAtomicSet16(volatile void         *aAddr,
                                       volatile acp_sint16_t  aVal)
{
    return (acp_sint16_t)
        atomic_swap_16((volatile acp_uint16_t*)aAddr,
                       (acp_uint16_t)aVal);
}

ACP_INLINE acp_sint16_t acpAtomicAdd16(volatile void         *aAddr,
                                       volatile acp_sint16_t  aVal)
{
    return (acp_sint16_t)
        atomic_add_16_nv((volatile acp_uint16_t*)aAddr,
                         (acp_uint16_t)aVal);
}

ACP_INLINE acp_sint32_t acpAtomicSet32(volatile void         *aAddr,
                                       volatile acp_sint32_t  aVal)
{
    return (acp_sint32_t)
        atomic_swap_32((volatile acp_uint32_t*)aAddr,
                       (acp_uint32_t)aVal);
}

ACP_INLINE acp_sint32_t acpAtomicAdd32(volatile void         *aAddr,
                                       volatile acp_sint32_t  aVal)
{
    return (acp_sint32_t)
        atomic_add_32_nv((volatile acp_uint32_t*)aAddr,
                         (acp_uint32_t)aVal);
}

ACP_INLINE acp_sint64_t acpAtomicGet64(volatile void *aAddr)
{
    return (acp_sint64_t)
        atomic_cas_64((volatile acp_uint64_t*)aAddr,
                      ACP_UINT64_LITERAL(0),
                      ACP_UINT64_LITERAL(0));
}

ACP_INLINE acp_sint64_t acpAtomicCas64(volatile void         *aAddr,
                                       volatile acp_sint64_t  aWith,
                                       volatile acp_sint64_t  aCmp)
{
    return (acp_sint64_t)
        atomic_cas_64((volatile acp_uint64_t*)aAddr,
                      (acp_uint64_t)aCmp,
                      (acp_uint64_t)aWith);
}

ACP_INLINE acp_sint64_t acpAtomicSet64(volatile void         *aAddr,
                                       volatile acp_sint64_t  aVal)
{
    return (acp_sint64_t)
        atomic_swap_64((volatile acp_uint64_t*)aAddr,
                       (acp_uint64_t)aVal);
}

ACP_INLINE acp_sint64_t acpAtomicAdd64(volatile void         *aAddr,
                                       volatile acp_sint64_t  aVal)
{
    return (acp_sint64_t)
        atomic_add_64_nv((volatile acp_uint64_t*)aAddr,
                         (acp_uint64_t)aVal);
}

#else
/*******************************************************************************
 * SunOS 5.9 (Solaris 2.9) and lower
 ******************************************************************************/

ACP_EXPORT acp_sint32_t acpAtomicCas32EXPORT(volatile void         *aAddr,
                                             volatile acp_sint32_t  aWith,
                                             volatile acp_sint32_t  aCmp);
#if defined(ACP_CFG_COMPILE_64BIT)
ACP_EXPORT acp_sint64_t acpAtomicCas64EXPORT(volatile void         *aAddr,
                                             volatile acp_sint64_t  aWith,
                                             volatile acp_sint64_t  aCmp);
#endif

#if defined(__GNUC__) && !defined(ACP_CFG_NOINLINE)

/*
 * sparc cas instruction is supported from v8plus architecture
 * gcc needs -mcpu=v9 flag for 32-bit compile
 */

ACP_INLINE acp_sint32_t acpAtomicCas32(volatile void         *aAddr,
                                       volatile acp_sint32_t  aWith,
                                       volatile acp_sint32_t  aCmp)
{
    __asm__ __volatile__ ("membar #StoreLoad | #LoadLoad\n\t"
                          "cas [%2],%3,%0\n\t"
                          "membar #StoreLoad | #StoreStore"
                          : "=&r"(aWith)
                          : "0"(aWith), "r"(aAddr), "r"(aCmp)
                          : "memory");
    return aWith;
}


#if defined(ACP_CFG_COMPILE_64BIT)

ACP_INLINE acp_sint64_t acpAtomicCas64(volatile void         *aAddr,
                                       volatile acp_sint64_t  aWith,
                                       volatile acp_sint64_t  aCmp)
{
    __asm__ __volatile__ ("membar #StoreLoad | #LoadLoad\n\t"
                          "casx [%2],%3,%0\n\t"
                          "membar #StoreLoad | #StoreStore"
                          : "=&r"(aWith)
                          : "0"(aWith), "r"(aAddr), "r"(aCmp)
                          : "memory");
    return aWith;
}

#else

ACP_EXPORT acp_sint64_t acpAtomicCas64(volatile void         *aAddr,
                                       volatile acp_sint64_t  aWith,
                                       volatile acp_sint64_t  aCmp);

#endif  /* ACP_CFG_COMPILE_64BIT */

#else
/* No GNUC or NOINLINE defined */

ACP_EXPORT acp_sint32_t acpAtomicCas32(volatile void         *aAddr,
                                       volatile acp_sint32_t  aWith,
                                       volatile acp_sint32_t  aCmp);

ACP_EXPORT acp_sint64_t acpAtomicCas64(volatile void         *aAddr,
                                       volatile acp_sint64_t  aWith,
                                       volatile acp_sint64_t  aCmp);

#endif

/* Common to GCC and native */
ACP_INLINE acp_sint16_t acpAtomicCas16(volatile void         *aAddr,
                                       volatile acp_sint16_t  aWith,
                                       volatile acp_sint16_t  aCmp)
{
    acp_uint32_t sRet;
    acp_uint32_t sVal;

    /* alinged */
    if (((acp_ulong_t)aAddr & 0x03) == 0)
    {
        do
        {
            sVal = *((acp_uint32_t*)aAddr);
            sRet = acpAtomicCas32(aAddr,
                                  ((acp_sint32_t)aWith << 16) | (sVal & 0xFFFF),
                                  ((acp_sint32_t)aCmp << 16)  | (sVal & 0xFFFF));
        } while((sVal & 0xFFFF) != (sRet & 0xFFFF));

        sRet = (sRet & 0xFFFF0000) >> 16;
    }
    /* not aligned */
    else
    {
        aAddr = ((acp_uint16_t*)aAddr) - 1;

        do
        {
            sVal = *((acp_uint32_t*)((acp_uint16_t*)aAddr));
            /* BUG-30077 vsebelev aWith and aCmp should be cast to uint16
               in other case it's automaticaly casts to sint32 and corrupt data. */
            sRet = acpAtomicCas32(((acp_sint16_t *)aAddr),
                                  (sVal & 0xFFFF0000) | (acp_uint16_t)aWith,
                                  (sVal & 0xFFFF0000) | (acp_uint16_t)aCmp);
        } while((sVal & 0xFFFF0000) != (sRet & 0xFFFF0000));

        sRet = (sRet & 0xFFFF);
    }

    return (acp_sint16_t)sRet;
}

ACP_INLINE acp_sint16_t acpAtomicSet16(volatile void         *aAddr,
                                       volatile acp_sint16_t  aVal)
{
    acp_sint16_t sPrev;
    acp_sint16_t sTemp;

    do
    {
        sPrev = *(volatile acp_sint16_t *)aAddr;
        sTemp = acpAtomicCas16(aAddr, aVal, sPrev);
    } while (sTemp != sPrev);

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

#  if defined(ACP_CFG_COMPILE_64BIT)

ACP_INLINE acp_sint64_t acpAtomicGet64(volatile void *aAddr)
{
    ACP_MEM_BARRIER();
    return *(volatile acp_sint64_t *)aAddr;
}

#  else

ACP_INLINE acp_sint64_t acpAtomicGet64(volatile void *aAddr)
{
    acp_sint64_t sPrev = *(volatile acp_sint64_t *)aAddr;

    return acpAtomicCas64(aAddr, sPrev, sPrev);
}

#  endif


# endif

#endif
