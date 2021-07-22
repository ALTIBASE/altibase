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
 * $Id: acpAtomic.h 9976 2010-02-10 06:02:39Z djin $
 ******************************************************************************/

#if !defined(_O_ACP_ATOMIC_H_)
#define _O_ACP_ATOMIC_H_

/**
 * @file
 * @ingroup CoreAtomic
 */

#include <acpMemBarrier.h>


ACP_EXTERN_C_BEGIN


#if defined(ACP_CFG_DOXYGEN) || defined(ALINT)

/**
 * reads and returns the value atomically
 * @param aAddr pointer to a variable to access atomically.
 * Must be aligned with 2bytes.
 * @return value
 */
ACP_INLINE acp_sint16_t acpAtomicGet16(volatile void *aAddr);

/**
 * writes a value and returns the old value atomically
 * @param aAddr pointer to a variable to access atomically
 * Must be aligned with 2bytes.
 * @param aVal value
 * @return old value
 */
ACP_INLINE acp_sint16_t acpAtomicSet16(volatile void         *aAddr,
                                       volatile acp_sint16_t  aVal);

/**
 * increases by 1 and returns the new value atomically
 * @param aAddr pointer to the atomic data sturcture
 * Must be aligned with 2bytes.
 * @return new value
 */
ACP_INLINE acp_sint16_t acpAtomicInc16(volatile void *aAddr);

/**
 * decreases by 1 and returns the new value atomically
 * @param aAddr pointer to a variable to access atomically
 * Must be aligned with 2bytes.
 * @return new value
 */
ACP_INLINE acp_sint16_t acpAtomicDec16(volatile void *aAddr);

/**
 * adds a value and returns the new value atomically
 * @param aAddr pointer to a variable to access atomically
 * Must be aligned with 2bytes.
 * @param aVal value to add
 * @return new value
 */
ACP_INLINE acp_sint16_t acpAtomicAdd16(volatile void         *aAddr,
                                       volatile acp_sint16_t  aVal);

/**
 * subtracts a value and returns the new value atomically
 * @param aAddr pointer to a variable to access atomically
 * Must be aligned with 2bytes.
 * @param aVal value to subtract
 * @return new value
 */
ACP_INLINE acp_sint16_t acpAtomicSub16(volatile void         *aAddr,
                                       volatile acp_sint16_t  aVal);

/**
 * compares and swaps a value and returns the old value atomically
 * @param aAddr pointer to a variable to access atomically
 * Must be aligned with 2bytes.
 * @param aWith what to swap it with
 * @param aCmp the value to compare it to
 * @return old value
 */
ACP_INLINE acp_sint32_t acpAtomicCas16(volatile void         *aAddr,
                                       volatile acp_sint16_t  aWith,
                                       volatile acp_sint16_t  aCmp);

/**
 * reads and returns the value atomically
 * @param aAddr pointer to a variable to access atomically
 * Must be aligned with 4bytes.
 * @return value
 */
ACP_INLINE acp_sint32_t acpAtomicGet32(volatile void *aAddr);

/**
 * writes a value and returns the old value atomically
 * @param aAddr pointer to a variable to access atomically
 * Must be aligned with 4bytes.
 * @param aVal value
 * @return old value
 */
ACP_INLINE acp_sint32_t acpAtomicSet32(volatile void         *aAddr,
                                       volatile acp_sint32_t  aVal);

/**
 * increases by 1 and returns the new value atomically
 * @param aAddr pointer to the atomic data sturcture
 * Must be aligned with 4bytes.
 * @return new value
 */
ACP_INLINE acp_sint32_t acpAtomicInc32(volatile void *aAddr);

/**
 * decreases by 1 and returns the new value atomically
 * @param aAddr pointer to a variable to access atomically
 * Must be aligned with 4bytes.
 * @return new value
 */
ACP_INLINE acp_sint32_t acpAtomicDec32(volatile void *aAddr);

/**
 * adds a value and returns the new value atomically
 * @param aAddr pointer to a variable to access atomically
 * Must be aligned with 4bytes.
 * @param aVal value to add
 * @return new value
 */
ACP_INLINE acp_sint32_t acpAtomicAdd32(volatile void         *aAddr,
                                       volatile acp_sint32_t  aVal);

/**
 * subtracts a value and returns the new value atomically
 * @param aAddr pointer to a variable to access atomically
 * Must be aligned with 4bytes.
 * @param aVal value to subtract
 * @return new value
 */
ACP_INLINE acp_sint32_t acpAtomicSub32(volatile void         *aAddr,
                                       volatile acp_sint32_t  aVal);

/**
 * compares and swaps a value and returns the old value atomically
 * @param aAddr pointer to a variable to access atomically
 * Must be aligned with 4bytes.
 * @param aWith what to swap it with
 * @param aCmp the value to compare it to
 * @return old value
 */
ACP_INLINE acp_sint32_t acpAtomicCas32(volatile void         *aAddr,
                                       volatile acp_sint32_t  aWith,
                                       volatile acp_sint32_t  aCmp);

/**
 * reads and returns the value atomically
 * @param aAddr pointer to a variable to access atomically
 * Must be aligned with 8bytes.
 * @return value
 */
ACP_INLINE acp_sint64_t acpAtomicGet64(volatile void *aAddr);

/**
 * writes a value and returns the old value atomically
 * @param aAddr pointer to a variable to access atomically
 * Must be aligned with 8bytes.
 * @param aVal value
 * @return old value
 */
ACP_INLINE acp_sint64_t acpAtomicSet64(volatile void         *aAddr,
                                       volatile acp_sint64_t  aVal);

/**
 * increases by 1 and returns the new value atomically
 * @param aAddr pointer to the atomic data sturcture
 * Must be aligned with 8bytes.
 * @return new value
 */
ACP_INLINE acp_sint64_t acpAtomicInc64(volatile void *aAddr);

/**
 * decreases by 1 and returns the new value atomically
 * @param aAddr pointer to a variable to access atomically
 * Must be aligned with 8bytes.
 * @return new value
 */
ACP_INLINE acp_sint64_t acpAtomicDec64(volatile void *aAddr);

/**
 * adds a value and returns the new value atomically
 * @param aAddr pointer to a variable to access atomically
 * Must be aligned with 8bytes.
 * @param aVal value to add
 * @return new value
 */
ACP_INLINE acp_sint64_t acpAtomicAdd64(volatile void         *aAddr,
                                       volatile acp_sint64_t  aVal);

/**
 * subtracts a value and returns the new value atomically
 * @param aAddr pointer to a variable to access atomically
 * Must be aligned with 8bytes.
 * @param aVal value to subtract
 * @return new value
 */
ACP_INLINE acp_sint64_t acpAtomicSub64(volatile void         *aAddr,
                                       volatile acp_sint64_t  aVal);

/**
 * compares and swaps a value and returns the old value atomically
 * @param aAddr pointer to a variable to access atomically
 * Must be aligned with 8bytes.
 * @param aWith what to swap it with
 * @param aCmp the value to compare it to
 * @return old value
 */
ACP_INLINE acp_sint64_t acpAtomicCas64(volatile void *aAddr,
                                       volatile acp_sint64_t   aWith,
                                       volatile acp_sint64_t   aCmp);

/**
 * reads and returns the value atomically
 * @param aAddr pointer to a variable to access atomically
 * @return value
 */
ACP_INLINE acp_slong_t acpAtomicGet(volatile void *aAddr);

/**
 * writes a value and returns the old value atomically
 * @param aAddr pointer to a variable to access atomically
 * @param aVal value
 * @return old value
 */
ACP_INLINE acp_slong_t acpAtomicSet(volatile void *aAddr, acp_slong_t aVal);

/**
 * increases by 1 and returns the new value atomically
 * @param aAddr pointer to the atomic data sturcture
 * @return new value
 */
ACP_INLINE acp_slong_t acpAtomicInc(volatile void *aAddr);

/**
 * decreases by 1 and returns the new value atomically
 * @param aAddr pointer to a variable to access atomically
 * @return new value
 */
ACP_INLINE acp_slong_t acpAtomicDec(volatile void *aAddr);

/**
 * adds a value and returns the new value atomically
 * @param aAddr pointer to a variable to access atomically
 * @param aVal value to add
 * @return new value
 */
ACP_INLINE acp_slong_t acpAtomicAdd(volatile void *aAddr, acp_slong_t aVal);

/**
 * subtracts a value and returns the new value atomically
 * @param aAddr pointer to a variable to access atomically
 * @param aVal value to subtract
 * @return new value
 */
ACP_INLINE acp_slong_t acpAtomicSub(volatile void *aAddr, acp_slong_t aVal);

/**
 * compares and swaps a value and returns the old value atomically
 * @param aAddr pointer to a variable to access atomically
 * @param aWith what to swap it with
 * @param aCmp the value to compare it to
 * @return old value
 */
ACP_INLINE acp_slong_t acpAtomicCas(volatile void        *aAddr,
                                    volatile acp_slong_t  aWith,
                                    volatile acp_slong_t  aCmp);


/**
 * compares and swaps a 16-bit value and returns boolean value
 * @param aAddr pointer to a variable to access atomically
 * Must be aligned with 2bytes.
 * @param aWith what to swap it with
 * @param aCmp the value to compare it to
 * @return boolean value(if a new value is assigned, it returns true)
 */
ACP_INLINE acp_bool_t acpAtomicCas16Bool(volatile void *aAddr,
                                         volatile acp_sint16_t   aWith,
                                         volatile acp_sint16_t   aCmp);

/**
 * compares and swaps a 32-bit value and returns boolean value
 * @param aAddr pointer to a variable to access atomically
 * Must be aligned with 4bytes.
 * @param aWith what to swap it with
 * @param aCmp the value to compare it to
 * @return boolean value(if a new value is assigned, it returns true)
 */
ACP_INLINE acp_bool_t acpAtomicCas32Bool(volatile void *aAddr,
                                         volatile acp_sint32_t   aWith,
                                         volatile acp_sint32_t   aCmp);

/**
 * compares and swaps a 64bit-type value and returns boolean value
 * @param aAddr pointer to a variable to access atomically
 * Must be aligned with 8bytes.
 * @param aWith what to swap it with
 * @param aCmp the value to compare it to
 * @return boolean value(if a new value is assigned, it returns true)
 */
ACP_INLINE acp_bool_t acpAtomicCas64Bool(volatile void *aAddr,
                                         volatile acp_sint64_t   aWith,
                                         volatile acp_sint64_t   aCmp);

/**
 * compares and swaps a long-type value and returns boolean value
 * @param aAddr pointer to a variable to access atomically
 * @param aWith what to swap it with
 * @param aCmp the value to compare it to
 * @return boolean value(if a new value is assigned, it returns true)
 */
ACP_INLINE acp_bool_t acpAtomicCasBool(volatile void        *aAddr,
                                       volatile acp_slong_t  aWith,
                                       volatile acp_slong_t  aCmp);

#else

/* Implementation of
 *     acpAtomicSet16,
 *     acpAtomicSet32,
 *     acpAtomicSet64,
 *     acpAtomicAdd16,
 *     acpAtomicAdd32,
 *     acpAtomicAdd64,
 *     acpAtomicCas16,
 *     acpAtomicCas32,
 *     acpAtomicCas64
 *
 * GET, INC, DEC, SUB are covered by the previous operations.
 *
 * (WARNING)
 *  : acpAtomic-X86_LINUX.h and acpAtomic-X86_SOLARIS.h
 *    are PERFECTLY IDENTICAL sources.
 */
# if defined(__STATIC_ANALYSIS_DOING__)
#  include <acpAtomic-STAZ.h>

# else
#   if defined(ALTI_CFG_CPU_ALPHA)
#    include <acpAtomic-ALPHA.h>

#   elif defined(ALTI_CFG_CPU_IA64)
#    include <acpAtomic-IA64.h>

#   elif defined(ALTI_CFG_CPU_POWERPC)
#    include <acpAtomic-PPC.h>

#   elif defined(ALTI_CFG_CPU_SPARC)
#    include <acpAtomic-SPARC.h>

#   elif defined(ALTI_CFG_CPU_PARISC)
#    include <acpAtomic-PARISC.h>

#   elif defined(ALTI_CFG_CPU_X86)
#    if defined(ALTI_CFG_OS_WINDOWS)
#     include <acpAtomic-X86_WINDOWS.h>

#    elif defined(ALTI_CFG_OS_LINUX)
#     include <acpAtomic-X86_LINUX.h>

#    elif defined(ALTI_CFG_OS_FREEBSD)
#     include <acpAtomic-X86_LINUX.h>

#    elif defined(ALTI_CFG_OS_DARWIN)
#     include <acpAtomic-X86_LINUX.h>

#    elif  defined(ALTI_CFG_OS_SOLARIS)
#     include <acpAtomic-X86_SOLARIS.h>
#    endif  /* ALTI_CFG_OS_WINDOWS */


#   else
#    warning Not supportable platform
#   endif  /* ALTI_CFG_CPU_X86 */
# endif   /* __STATIC_ANALYSIS_DOING__ */
#endif  /* ACP_CFG_DOXYGEN */

#if !defined(ACP_CFG_COMPILE_64BIT) || !defined(ALTI_CFG_CPU_POWERPC)
ACP_INLINE acp_sint16_t acpAtomicGet16(volatile void *aAddr)
{
    ACP_MEM_BARRIER();

    return *(volatile acp_sint16_t *)aAddr;
}
#endif

ACP_INLINE acp_sint16_t acpAtomicSub16(volatile void         *aAddr,
                                       volatile acp_sint16_t  aVal)
{
    return acpAtomicAdd16(aAddr, -aVal);
}

ACP_INLINE acp_sint16_t acpAtomicInc16(volatile void *aAddr)
{
    return acpAtomicAdd16(aAddr, 1);
}

ACP_INLINE acp_sint16_t acpAtomicDec16(volatile void *aAddr)
{
    return acpAtomicAdd16(aAddr, -1);
}

ACP_INLINE acp_sint32_t acpAtomicGet32(volatile void *aAddr)
{
    return acpAtomicCas32(aAddr, 0, 0);
}

ACP_INLINE acp_sint32_t acpAtomicSub32(volatile void         *aAddr,
                                       volatile acp_sint32_t  aVal)
{
    return acpAtomicAdd32(aAddr, -aVal);
}

ACP_INLINE acp_sint32_t acpAtomicInc32(volatile void *aAddr)
{
    return acpAtomicAdd32(aAddr, 1);
}

ACP_INLINE acp_sint32_t acpAtomicDec32(volatile void *aAddr)
{
    return acpAtomicAdd32(aAddr, -1);
}

ACP_INLINE acp_sint64_t acpAtomicSub64(volatile void         *aAddr,
                                       volatile acp_sint64_t  aVal)
{
    return acpAtomicAdd64(aAddr, -aVal);
}

ACP_INLINE acp_sint64_t acpAtomicInc64(volatile void *aAddr)
{
    return acpAtomicAdd64(aAddr, 1);
}

ACP_INLINE acp_sint64_t acpAtomicDec64(volatile void *aAddr)
{
    return acpAtomicAdd64(aAddr, -1);
}


/*
 * Atomic operations for long integer
 */

ACP_INLINE acp_slong_t acpAtomicGet(volatile void *aAddr)
{
#if defined(ACP_CFG_COMPILE_32BIT)
    return acpAtomicGet32(aAddr);
#else
    return acpAtomicGet64(aAddr);
#endif
}

ACP_INLINE acp_slong_t acpAtomicSet(volatile void *aAddr, acp_slong_t aVal)
{
#if defined(ACP_CFG_COMPILE_32BIT)
    return acpAtomicSet32(aAddr, (acp_sint32_t)aVal);
#else
    return acpAtomicSet64(aAddr, (acp_sint64_t)aVal);
#endif
}

ACP_INLINE acp_slong_t acpAtomicInc(volatile void *aAddr)
{
#if defined(ACP_CFG_COMPILE_32BIT)
    return acpAtomicInc32(aAddr);
#else
    return acpAtomicInc64(aAddr);
#endif
}

ACP_INLINE acp_slong_t acpAtomicDec(volatile void *aAddr)
{
#if defined(ACP_CFG_COMPILE_32BIT)
    return acpAtomicDec32(aAddr);
#else
    return acpAtomicDec64(aAddr);
#endif
}

ACP_INLINE acp_slong_t acpAtomicAdd(volatile void        *aAddr,
                                    volatile acp_slong_t  aVal)
{
#if defined(ACP_CFG_COMPILE_32BIT)
    return acpAtomicAdd32(aAddr, (acp_sint32_t)aVal);
#else
    return acpAtomicAdd64(aAddr, (acp_sint64_t)aVal);
#endif
}

ACP_INLINE acp_slong_t acpAtomicSub(volatile void        *aAddr,
                                    volatile acp_slong_t  aVal)
{
#if defined(ACP_CFG_COMPILE_32BIT)
    return acpAtomicSub32(aAddr, (acp_sint32_t)aVal);
#else
    return acpAtomicSub64(aAddr, (acp_sint64_t)aVal);
#endif
}

ACP_INLINE acp_slong_t acpAtomicCas(volatile void        *aAddr,
                                    volatile acp_slong_t  aWith,
                                    volatile acp_slong_t  aCmp)
{
#if defined(ACP_CFG_COMPILE_32BIT)
    return acpAtomicCas32(aAddr, (acp_sint32_t)aWith, (acp_sint32_t)aCmp);
#else
    return acpAtomicCas64(aAddr, (acp_sint64_t)aWith, (acp_sint64_t)aCmp);
#endif
}


/*
 * check whether a new value is assigned or not for Compare and Swap functions.
 */
ACP_INLINE acp_bool_t acpAtomicCas16Bool(volatile void         *aAddr,
                                         volatile acp_sint16_t  aWith,
                                         volatile acp_sint16_t  aCmp)
{
    return (acp_bool_t)(aCmp == acpAtomicCas16(aAddr, aWith, aCmp));
}

ACP_INLINE acp_bool_t acpAtomicCas32Bool(volatile void         *aAddr,
                                         volatile acp_sint32_t  aWith,
                                         volatile acp_sint32_t  aCmp)
{
    return (acp_bool_t)(aCmp == acpAtomicCas32(aAddr, aWith, aCmp));
}

ACP_INLINE acp_bool_t acpAtomicCas64Bool(volatile void         *aAddr,
                                         volatile acp_sint64_t  aWith,
                                         volatile acp_sint64_t  aCmp)
{
    return (acp_bool_t)(aCmp == acpAtomicCas64(aAddr, aWith, aCmp));
}

ACP_INLINE acp_bool_t acpAtomicCasBool(volatile void        *aAddr,
                                       volatile acp_slong_t  aWith,
                                       volatile acp_slong_t  aCmp)
{
    return (acp_bool_t)(aCmp == acpAtomicCas(aAddr, aWith, aCmp));
}



ACP_EXTERN_C_END


#endif
