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
 * $Id: acpAtomic-X86_WINDOWS.h 7869 2009-09-25 05:34:47Z djin $
 ******************************************************************************/

#if !defined(_O_ACP_ATOMIC_X86_WINDOWS_H_)
#define _O_ACP_ATOMIC_X86_WINDOWS_H_


#if defined(ACP_CFG_COMPILE_32BIT)
ACP_INLINE acp_sint32_t acpAtomicFetchAdd32(volatile void         *aAddr,
                                            volatile acp_sint32_t  aVal)
{
#pragma warning(push)
#pragma warning(disable: 4035)
    __asm
    {
        mov eax, aVal;
        mov ebx, aAddr;
        lock xadd [ebx], eax;
    }
#pragma warning(pop)
}

ACP_INLINE acp_sint16_t acpAtomicFetchAdd16(volatile void         *aAddr,
                                            volatile acp_sint16_t  aVal)
{
#pragma warning(push)
#pragma warning(disable: 4035)
    __asm
    {
        mov ax, aVal;
        mov ebx, aAddr;
        lock xadd [ebx], ax;
    }
#pragma warning(pop)
}
#endif  /* ACP_CFG_COMPILE_32BIT */

ACP_INLINE acp_sint16_t acpAtomicCas16(volatile void         *aAddr,
                                       volatile acp_sint16_t  aWith,
                                       volatile acp_sint16_t  aCmp)
{
#if defined(ACP_CFG_COMPILE_32BIT)
    __asm
    {
        mov ax, aCmp;
        mov ebx, aAddr;
        mov cx, aWith;
        lock cmpxchg [ebx], cx;
    }
#else
    return _InterlockedCompareExchange16(
            (acp_sint16_t volatile *)aAddr, aWith, aCmp);
#endif
}

ACP_INLINE acp_sint16_t acpAtomicSet16(volatile void         *aAddr,
                                       volatile acp_sint16_t  aVal)
{
#if defined(ACP_CFG_COMPILE_32BIT)
#pragma warning(push)
#pragma warning(disable: 4035)
    __asm
    {
        mov ax, aVal;
        mov ebx, aAddr;
        xchg [ebx], ax;
    }
#pragma warning(pop)
#else
    acp_sint16_t sPrev;
    acp_sint16_t sTemp;

    do
    {
        sPrev = *(volatile acp_sint16_t *)aAddr;
        sTemp = acpAtomicCas16(aAddr, aVal, sPrev);
    } while (sTemp != sPrev);

    return sPrev;
#endif
}

ACP_INLINE acp_sint16_t acpAtomicAdd16(volatile void         *aAddr,
                                       volatile acp_sint16_t  aVal)
{
#if defined(ACP_CFG_COMPILE_32BIT)
    return acpAtomicFetchAdd16(aAddr, aVal) + aVal;
#else
    acp_sint16_t sPrev;
    acp_sint16_t sTemp;

    do
    {
        sPrev = *(volatile acp_sint16_t *)aAddr;
        sTemp = acpAtomicCas16(aAddr, sPrev + aVal, sPrev);
    } while (sTemp != (sPrev));

    return sPrev + aVal;
#endif
}

ACP_INLINE acp_sint32_t acpAtomicCas32(volatile void         *aAddr,
                                       volatile acp_sint32_t  aWith,
                                       volatile acp_sint32_t  aCmp)
{
#if defined(ACP_CFG_COMPILE_32BIT)
#pragma warning(push)
#pragma warning(disable: 4035)
    __asm
    {
        mov eax, aCmp;
        mov ebx, aAddr;
        mov ecx, aWith;
        lock cmpxchg [ebx], ecx;
    }
#pragma warning(pop)
#else
    return InterlockedCompareExchange((LONG volatile *)aAddr,
                                      (LONG)aWith,
                                      (LONG)aCmp);
#endif
}

ACP_INLINE acp_sint64_t acpAtomicGet64(volatile void *aAddr)
{
#if defined(ACP_CFG_COMPILE_32BIT)
#pragma warning(push)
#pragma warning(disable: 4035)
    __asm
    {
        mov ecx, aAddr;
        mov edx, ecx;
        mov eax, ebx;
        lock cmpxchg8b [ecx];
    }
#pragma warning(pop)
#else
    ACP_MEM_BARRIER();

    return *(volatile acp_sint64_t *)aAddr;
#endif
}

ACP_INLINE acp_sint32_t acpAtomicSet32(volatile void         *aAddr,
                                       volatile acp_sint32_t  aVal)
{
#if defined(ACP_CFG_COMPILE_32BIT)
#pragma warning(push)
#pragma warning(disable: 4035)
    __asm
    {
        mov eax, aVal;
        mov ebx, aAddr;
        xchg [ebx], eax;
    }
#pragma warning(pop)
#else
    return InterlockedExchange((LONG volatile *)aAddr, (LONG)aVal);
#endif
}

ACP_INLINE acp_sint32_t acpAtomicAdd32(volatile void         *aAddr,
                                       volatile acp_sint32_t  aVal)
{
#if defined(ACP_CFG_COMPILE_32BIT)
    return acpAtomicFetchAdd32(aAddr, aVal) + aVal;
#else
    return InterlockedExchangeAdd((LONG volatile *)aAddr, (LONG)aVal) + aVal;
#endif
}

ACP_INLINE acp_sint64_t acpAtomicCas64(volatile void         *aAddr,
                                       volatile acp_sint64_t  aWith,
                                       volatile acp_sint64_t  aCmp)
{
#if defined(ACP_CFG_COMPILE_32BIT)
#pragma warning(push)
#pragma warning(disable: 4035)
    acp_uint32_t sWith0 = (acp_uint32_t)((acp_uint64_t)aWith & 0xffffffff);
    acp_uint32_t sWith1 = (acp_uint32_t)((acp_uint64_t)aWith >> 32);
    acp_uint32_t sCmp0  = (acp_uint32_t)((acp_uint64_t)aCmp & 0xffffffff);
    acp_uint32_t sCmp1  = (acp_uint32_t)((acp_uint64_t)aCmp >> 32);

    __asm
    {
        mov eax, [sCmp0];
        mov edx, [sCmp1];
        mov ebx, [sWith0];
        mov ecx, [sWith1];
        mov esi, [aAddr];
        lock cmpxchg8b [esi];
    }
#pragma warning(pop)
#else
    return (acp_sint64_t)InterlockedCompareExchangePointer(
        (PVOID volatile *)aAddr,
        (PVOID)aWith,
        (PVOID)aCmp);
#endif
}

ACP_INLINE acp_sint64_t acpAtomicSet64(volatile void         *aAddr,
                                       volatile acp_sint64_t  aVal)
{
#if defined(ACP_CFG_COMPILE_32BIT)
    acp_sint64_t sPrev;
    acp_sint64_t sTemp;

    do
    {
        sPrev = *(volatile acp_sint64_t *)aAddr;
        sTemp = acpAtomicCas64(aAddr, aVal, sPrev);
    } while (sTemp != sPrev);

    return sPrev;
#else
    return (acp_sint64_t)InterlockedExchangePointer((PVOID volatile *)aAddr,
                                                    (PVOID)aVal);
#endif
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

#endif
