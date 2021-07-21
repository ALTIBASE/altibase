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
 * $Id: acpMemBarrier.h 9976 2010-02-10 06:02:39Z djin $
 ******************************************************************************/

#if !defined(_O_ACP_MEM_BARRIER_H_)
#define _O_ACP_MEM_BARRIER_H_

/**
 * @file
 * @ingroup CoreMem
 */

#include <acpTypes.h>

ACP_EXTERN_C_BEGIN

ACP_EXPORT void acpMemBarrier(void);
ACP_EXPORT void acpMemRBarrier(void);
ACP_EXPORT void acpMemWBarrier(void);
ACP_EXPORT void acpMemPrefetch(void* aPointer);
ACP_EXPORT void acpMemPrefetch0(void* aPointer);
ACP_EXPORT void acpMemPrefetch1(void* aPointer);
ACP_EXPORT void acpMemPrefetch2(void* aPointer);
ACP_EXPORT void acpMemPrefetchN(void* aPointer);


#if defined(ACP_CFG_DOXYGEN)

/**
 * Prefetches
 */
/* BUG-21281 Expansion of Read/Write Memory Barriers */
# define ACP_MEM_PREFETCH0(aPointer)
# define ACP_MEM_PREFETCH1(aPointer)
# define ACP_MEM_PREFETCH2(aPointer)
# define ACP_MEM_PREFETCHN(aPointer)
# define ACP_MEM_PREFETCH(aPointer)

/**
 * memory barriers
 */
# define ACP_MEM_BARRIER()
# define ACP_MEM_RBARRIER()
# define ACP_MEM_WBARRIER()

#else /* of DOXYGEN */

# if defined(ALTI_CFG_OS_WINDOWS)
#  if defined(ALTI_CFG_CPU_IA64)
/* Code for Itanium */
/* Memory Barrier */
#   define ACP_MEM_BARRIER() __mf()
#   define ACP_MEM_RBARRIER() ACP_MEM_BARRIER()
#   define ACP_MEM_WBARRIER() ACP_MEM_BARRIER()
/* Prefetch 
 * VC64 does not support inline assembly
 * I will add proper code */
#   define ACP_MEM_PREFETCH0(aPointer) ACP_UNUSED(aPointer)
#   define ACP_MEM_PREFETCH1(aPointer) ACP_UNUSED(aPointer)
#   define ACP_MEM_PREFETCH2(aPointer) ACP_UNUSED(aPointer)
#   define ACP_MEM_PREFETCHN(aPointer) ACP_UNUSED(aPointer)
#   define ACP_MEM_PREFETCH ACP_MEM_PREFETCH0

#  elif defined(ALTI_CFG_CPU_X86)
#   if defined(ACP_CFG_COMPILE_64BIT)
/* Code for 64bit Windows */
#    define ACP_MEM_BARRIER() __faststorefence()
#    define ACP_MEM_RBARRIER() ACP_MEM_BARRIER()
#    define ACP_MEM_WBARRIER() ACP_MEM_BARRIER()
/* Prefetch - Cannot find appropriate function */
#    define ACP_MEM_PREFETCH0(aPointer) ACP_UNUSED(aPointer)
#    define ACP_MEM_PREFETCH1(aPointer) ACP_UNUSED(aPointer)
#    define ACP_MEM_PREFETCH2(aPointer) ACP_UNUSED(aPointer)
#    define ACP_MEM_PREFETCHN(aPointer) ACP_UNUSED(aPointer)
#    define ACP_MEM_PREFETCH ACP_MEM_PREFETCH0
#   else
/* Code for 32bit Windows */
#    define ACP_MEM_BARRIER() __asm mfence
#    define ACP_MEM_RBARRIER() __asm lfence
#    define ACP_MEM_WBARRIER() __asm sfence
/* Prefetch - A hint for CPU */
#    define ACP_MEM_PREFETCH0(aPointer) __asm { prefetcht0  aPointer }
#    define ACP_MEM_PREFETCH1(aPointer) __asm { prefetcht1  aPointer }
#    define ACP_MEM_PREFETCH2(aPointer) __asm { prefetcht2  aPointer }
#    define ACP_MEM_PREFETCHN(aPointer) __asm { prefetchnta aPointer }
#    define ACP_MEM_PREFETCH ACP_MEM_PREFETCH0
#   endif
#  endif
/*
 * Not Windows, not gcc or NOINLINE
 */
# elif !defined(__GNUC__) || defined(ACP_CFG_NOINLINE)
#  define ACP_MEM_BARRIER()             acpMemBarrier()
#  define ACP_MEM_RBARRIER()            acpMemRBarrier()
#  define ACP_MEM_WBARRIER()            acpMemWBarrier()
#  define ACP_MEM_PREFETCH0(aPointer)   acpMemPrefetch0(aPointer)
#  define ACP_MEM_PREFETCH1(aPointer)   acpMemPrefetch1(aPointer)
#  define ACP_MEM_PREFETCH2(aPointer)   acpMemPrefetch2(aPointer)
#  define ACP_MEM_PREFETCHN(aPointer)   acpMemPrefetchN(aPointer)
#  define ACP_MEM_PREFETCH(aPointer)    acpMemPrefetch(aPointer)

/*
 * Not Windows, gcc, NOINLINE
 */
# elif defined(ALTI_CFG_CPU_ALPHA)
/* Code for Alpha CPU */
#  define ACP_MEM_BARRIER() __asm__ __volatile__ ("mb":::"memory")
#  define ACP_MEM_RBARRIER() ACP_MEM_BARRIER()
#  define ACP_MEM_WBARRIER() __asm__ __volatile__("wmb":::"memory")
/* Prefetch Instruction still not found */
#  define ACP_MEM_PREFETCH0(aPointer) ACP_UNUSED(aPointer)
#  define ACP_MEM_PREFETCH1(aPointer) ACP_UNUSED(aPointer)
#  define ACP_MEM_PREFETCH2(aPointer) ACP_UNUSED(aPointer)
#  define ACP_MEM_PREFETCHN(aPointer) ACP_UNUSED(aPointer)
#  define ACP_MEM_PREFETCH ACP_MEM_PREFETCH0

# elif defined(ALTI_CFG_CPU_IA64)
/* Code for Itanium */
#  define ACP_MEM_BARRIER() __asm__ __volatile__ ("mf":::"memory")
#  define ACP_MEM_RBARRIER() ACP_MEM_BARRIER()
#  define ACP_MEM_WBARRIER() ACP_MEM_BARRIER()
/* Prefetch Instructions */
#  define ACP_MEM_PREFETCH0(aPointer) __asm__ __volatile__ \
    ("lfetch [%0]" : : "r"(aPointer))
#  define ACP_MEM_PREFETCH1(aPointer) __asm__ __volatile__ \
    ("lfetch.nt1 [%0]" : : "r"(aPointer))
#  define ACP_MEM_PREFETCH2(aPointer) __asm__ __volatile__ \
    ("lfetch.nt2 [%0]" : : "r"(aPointer))
#  define ACP_MEM_PREFETCHN(aPointer) __asm__ __volatile__ \
    ("lfetch.nta [%0]" : : "r"(aPointer))
#  define ACP_MEM_PREFETCH ACP_MEM_PREFETCH0

# elif defined(ALTI_CFG_CPU_PARISC)
/* Code for Pa-Risc */
#  define ACP_MEM_BARRIER() __asm__ __volatile__ ("":::"memory")
#  define ACP_MEM_RBARRIER() ACP_MEM_BARRIER()
#  define ACP_MEM_WBARRIER() ACP_MEM_BARRIER()
/* Prefetch Instruction still not found */
#  define ACP_MEM_PREFETCH(aPointer) __asm__ __volatile__ \
    ("ldw 0(%0), %%r0\n\t" : : "r" (aPointer))
#  define ACP_MEM_PREFETCH0 ACP_MEM_PREFETCH
#  define ACP_MEM_PREFETCH1 ACP_MEM_PREFETCH
#  define ACP_MEM_PREFETCH2 ACP_MEM_PREFETCH
#  define ACP_MEM_PREFETCHN ACP_MEM_PREFETCH

# elif defined(ALTI_CFG_CPU_POWERPC)
/* Code for PowerPC */
#  define ACP_MEM_BARRIER() __asm__ __volatile__ ("sync":::"memory")
#  define ACP_MEM_RBARRIER() ACP_MEM_BARRIER()
#  define ACP_MEM_WBARRIER() ACP_MEM_BARRIER()
/* Prefetch Instructions */
#  define ACP_MEM_PREFETCH(aPointer) __asm__ __volatile__ \
    ("dcbt 0,%0\n\t" : : "r" (aPointer))
#  define ACP_MEM_PREFETCH0 ACP_MEM_PREFETCH
#  define ACP_MEM_PREFETCH1 ACP_MEM_PREFETCH
#  define ACP_MEM_PREFETCH2 ACP_MEM_PREFETCH
#  define ACP_MEM_PREFETCHN ACP_MEM_PREFETCH

# elif defined(ALTI_CFG_CPU_SPARC)
/* Code for Solaris */
#  define ACP_MEM_BARRIER() __asm__ __volatile__ ("ba,pt %%xcc,1f\n\t"   \
                                                "membar "               \
                                                "#LoadLoad |"           \
                                                "#LoadStore | "         \
                                                "#StoreStore | "        \
                                                "#StoreLoad\n\t"        \
                                                "1:"                    \
                                                :::"memory")
#  define ACP_MEM_RBARRIER() ACP_MEM_BARRIER()
#  define ACP_MEM_WBARRIER() ACP_MEM_BARRIER()
/* Prefetch Instruction still not found */
#  define ACP_MEM_PREFETCH0(aPointer) ACP_UNUSED(aPointer)
#  define ACP_MEM_PREFETCH1(aPointer) ACP_UNUSED(aPointer)
#  define ACP_MEM_PREFETCH2(aPointer) ACP_UNUSED(aPointer)
#  define ACP_MEM_PREFETCHN(aPointer) ACP_UNUSED(aPointer)
#  define ACP_MEM_PREFETCH ACP_MEM_PREFETCH0

# elif defined(ALTI_CFG_CPU_X86)
/*
 * BUG-45576	ACP_MEM_BARRIER() must be implemented in x86. 
 */

// borrowed from idl.
#  define ACP_MEM_BARRIER()           asm("mfence")
#  define ACP_MEM_RBARRIER()          asm("mfence")
#  define ACP_MEM_WBARRIER()          asm("mfence")
#  define ACP_MEM_PREFETCH0(aPointer) ACP_UNUSED(aPointer)
#  define ACP_MEM_PREFETCH1(aPointer) ACP_UNUSED(aPointer)
#  define ACP_MEM_PREFETCH2(aPointer) ACP_UNUSED(aPointer)
#  define ACP_MEM_PREFETCHN(aPointer) ACP_UNUSED(aPointer)
#  define ACP_MEM_PREFETCH ACP_MEM_PREFETCH0
# elif defined(ALTI_CFG_CPU_ARM64)
#  define ACP_MEM_BARRIER()           asm("dmb ish")
#  define ACP_MEM_RBARRIER()          asm("dmb ish")
#  define ACP_MEM_WBARRIER()          asm("dmb ish")
#  define ACP_MEM_PREFETCH0(aPointer) ACP_UNUSED(aPointer)
#  define ACP_MEM_PREFETCH1(aPointer) ACP_UNUSED(aPointer)
#  define ACP_MEM_PREFETCH2(aPointer) ACP_UNUSED(aPointer)
#  define ACP_MEM_PREFETCHN(aPointer) ACP_UNUSED(aPointer)
#  define ACP_MEM_PREFETCH ACP_MEM_PREFETCH0
# else
#  error acpMemBarrier not implemented
# endif /* Each CPUs */
#endif /* Else of doxygen */

ACP_EXTERN_C_END

#endif
