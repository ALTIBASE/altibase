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
 * $Id: acpAtomic-X86_LINUX.c 4529 2009-02-13 01:45:30Z jykim $
 ******************************************************************************/

/*******************************************************************************
 * This file is added to compile core with native C compiler.
 *
 * To make objects;
 *
 * 1. 32bit. Static.
 *  $ gcc -c -o ./objects/acpAtomic-X86_SOLARIS_32MODE.o
 *    acpAtomic-X86_SOLARIS_Native.c -DACP_CFG_COMPILE_32BIT
 * 2. 32bit. Dynamic.
 *  $ gcc -c -fPIC -o ./objects/acpAtomic-X86_SOLARIS_32MODE_pic.o 
 *    acpAtomic-X86_SOLARIS_Native.c -DACP_CFG_COMPILE_32BIT -DACP_CFG_PIC
 * 3. 64bit. Static.
 *  $ gcc -c m64 -o ./objects/acpAtomic-X86_SOLARIS_64MODE.o
 *    acpAtomic-X86_SOLARIS_Native.c
 * 4. 64bit. Dynamic.
 *  $ gcc -c -o -m64 -fPIC *  ./objects/acpAtomic-X86_SOLARIS_64MODE_pic.o
 *    acpAtomic-X86_SOLARIS_Native.c -DACP_CFG_PIC
 *
 ******************************************************************************/

/*
 * Integer Type
 */
/**
 * 1-byte signed integer type
 */
typedef signed char        acp_sint8_t;
/**
 * 1-byte unsigned integer type
 */
typedef unsigned char      acp_uint8_t;

/**
 * 2-byte signed integer type
 */
typedef signed short       acp_sint16_t;
/**
 * 2-byte unsigned integer type
 */
typedef unsigned short     acp_uint16_t;

/**
 * 4-byte signed integer type
 */
typedef signed int         acp_sint32_t;
/**
 * 4-byte unsigned integer type
 */
typedef unsigned int       acp_uint32_t;

/**
 * 8-byte signed integer type
 */
typedef signed long long   acp_sint64_t;
/**
 * 8-byte unsigned integer type
 */
typedef unsigned long long acp_uint64_t;


acp_sint16_t acpAtomicCas16(volatile void         *aAddr,
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

acp_sint16_t acpAtomicSet16(volatile void         *aAddr,
                            volatile acp_sint16_t  aVal)
{
    __asm__ __volatile__ ("xchgw %0,%1"
                          : "=r"(aVal)
                          : "m"(*(volatile acp_sint16_t *)aAddr), "0"(aVal)
                          : "memory");
    return aVal;
}

acp_sint16_t acpAtomicAdd16(volatile void         *aAddr,
                            volatile acp_sint16_t  aVal)
{
    acp_sint16_t sTemp = aVal;

    __asm__ __volatile__ ("lock; xaddw %0,%1"
                          : "+r"(sTemp), "+m"(*(volatile acp_sint16_t *)aAddr)
                          :
                          : "memory");
    return sTemp + aVal;
}

acp_sint32_t acpAtomicCas32(volatile void         *aAddr,
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

acp_sint32_t acpAtomicSet32(volatile void         *aAddr,
                            volatile acp_sint32_t  aVal)
{
    __asm__ __volatile__ ("xchgl %0,%1"
                          : "=r"(aVal)
                          : "m"(*(volatile acp_sint32_t *)aAddr), "0"(aVal)
                          : "memory");
}

acp_sint32_t acpAtomicAdd32(volatile void         *aAddr,
                            volatile acp_sint32_t  aVal)
{
    acp_sint32_t sTemp = aVal;

    __asm__ __volatile__ ("lock; xaddl %0,%1"
                          : "+r"(sTemp), "+m"(*(volatile acp_sint32_t *)aAddr)
                          :
                          : "memory");
    return sTemp + aVal;
}


#if defined(ACP_CFG_COMPILE_32BIT)
/*
 * 32bit
 */
acp_sint64_t acpAtomicCas64(volatile void *aAddr,
                            acp_sint64_t   aWith,
                            acp_sint64_t   aCmp)
{
    acp_sint64_t sPrev;

    /*
     * GCC uses ebx as the PIC register for globals in shared libraries.
     * GCC won't temporarily spill it if it's used as input, output, or clobber.
     * Instead, it complains with a "can't find a register in class `BREG' while
     * reloading `asm'" error. This is probably a compiler bug, but as the
     * cmpxchg8b instruction requires ebx. So, I work around this here by using
     * esi as input and exchange before and after the cmpxchg8b instruction.
     */
#   if defined(ACP_CFG_PIC)
    __asm__ __volatile__ ("xchgl %%esi,%%ebx\n\t"
                          "lock; cmpxchg8b %3\n\t"
                          "xchgl %%ebx,%%esi"
                          : "=A"(sPrev)
                          : "S"((acp_uint32_t)((acp_uint64_t)aWith)),
                            "c"((acp_uint32_t)((acp_uint64_t)aWith >> 32)),
                            "m"(*(volatile acp_sint64_t *)aAddr),
                            "0"(aCmp)
                          : "memory");
#   else
    __asm__ __volatile__ ("lock; cmpxchg8b %3"
                          : "=A"(sPrev)
                          : "b"((acp_uint32_t)((acp_uint64_t)aWith)),
                            "c"((acp_uint32_t)((acp_uint64_t)aWith >> 32)),
                            "m"(*(volatile acp_sint64_t *)aAddr),
                            "0"(aCmp)
                          : "memory");
#   endif

    return sPrev;
}


acp_sint64_t acpAtomicGet64(volatile void *aAddr)
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

acp_sint64_t acpAtomicSet64(volatile void         *aAddr,
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

acp_sint64_t acpAtomicAdd64(volatile void         *aAddr,
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

#else

acp_sint64_t acpAtomicCas64(volatile void *aAddr,
                            acp_sint64_t   aWith,
                            acp_sint64_t   aCmp)
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

acp_sint64_t acpAtomicGet64(volatile void *aAddr)
{
    __asm__ __volatile__ ("mfence":::"memory");

    return *(volatile acp_sint64_t *)aAddr;
}

acp_sint64_t acpAtomicSet64(volatile void         *aAddr,
                            volatile acp_sint64_t  aVal)
{
    __asm__ __volatile__ ("xchgq %0,%1"
                          : "=r"(aVal)
                          : "m"(*(volatile acp_sint64_t *)aAddr), "0"(aVal)
                          : "memory");
    return aVal;
}

acp_sint64_t acpAtomicAdd64(volatile void         *aAddr,
                            volatile acp_sint64_t  aVal)
{
    acp_sint64_t sPrev = aVal;

    __asm__ __volatile__ ("lock; xaddq %0,%1"
                          : "+r"(sPrev), "+m"(*(volatile acp_sint64_t *)aAddr)
                          :
                          : "memory");
    return sPrev + aVal;
}

#endif
