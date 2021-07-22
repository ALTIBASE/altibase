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
 * $Id: acpAtomic-STAZ.h 4937 2009-03-23 04:54:51Z jykim $
 ******************************************************************************/

#if !defined(_O_ACP_ATOMIC_STAZ_H_)
#define _O_ACP_ATOMIC_STAZ_H_


/*
 * These address Lock/Unlock functions are only static analysis test of
 * Codesonar and actually not used in anywhere.
 */
ACP_INLINE void acpAtomicAddrLock(volatile void *aAddr)
{
    ACP_UNUSED(aAddr);
    return;
}

ACP_INLINE void acpAtomicAddrUnlock(volatile void *aAddr)
{
    ACP_UNUSED(aAddr);
    return;
}

ACP_INLINE acp_sint16_t acpAtomicCas16(volatile void         *aAddr,
                                       volatile acp_sint16_t  aWith,
                                       volatile acp_sint16_t  aCmp)
{
    acp_sint16_t sPrev;

    acpAtomicAddrLock(aAddr);

    sPrev = *(volatile acp_sint16_t *)aAddr;

    if (sPrev == aCmp)
    {
        *(volatile acp_sint16_t *)aAddr = aWith;
    }
    else
    {
        /* do nothing */
    }

    acpAtomicAddrUnlock(aAddr);

    return sPrev;
}

ACP_INLINE acp_sint16_t acpAtomicSet16(volatile void         *aAddr,
                                       volatile acp_sint16_t  aVal)
{
    acp_sint16_t sPrev;

    acpAtomicAddrLock(aAddr);

    sPrev  = *(volatile acp_sint16_t *)aAddr;
    *(volatile acp_sint16_t *)aAddr = aVal;

    acpAtomicAddrUnlock(aAddr);

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

ACP_INLINE acp_sint32_t acpAtomicCas32(volatile void         *aAddr,
                                       volatile acp_sint32_t  aWith,
                                       volatile acp_sint32_t  aCmp)
{
    acp_sint32_t sPrev;

    acpAtomicAddrLock(aAddr);

    sPrev = *(volatile acp_sint32_t *)aAddr;

    if (sPrev == aCmp)
    {
        *(volatile acp_sint32_t *)aAddr = aWith;
    }
    else
    {
        /* do nothing */
    }

    acpAtomicAddrUnlock(aAddr);

    return sPrev;
}

ACP_INLINE acp_sint32_t acpAtomicSet32(volatile void         *aAddr,
                                       volatile acp_sint32_t  aVal)
{
    acp_sint32_t sPrev;

    acpAtomicAddrLock(aAddr);

    sPrev  = *(volatile acp_sint32_t *)aAddr;
    *(volatile acp_sint32_t *)aAddr = aVal;

    acpAtomicAddrUnlock(aAddr);

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

    acpAtomicAddrLock(aAddr);

    sPrev = *(volatile acp_sint64_t *)aAddr;

    if (sPrev == aCmp)
    {
        *(volatile acp_sint64_t *)aAddr = aWith;
    }
    else
    {
        /* do nothing */
    }

    acpAtomicAddrUnlock(aAddr);

    return sPrev;
}

ACP_INLINE acp_sint64_t acpAtomicSet64(volatile void         *aAddr,
                                       volatile acp_sint64_t  aVal)
{
    acp_sint64_t sPrev;

    acpAtomicAddrLock(aAddr);

    sPrev  = *(volatile acp_sint64_t *)aAddr;
    *(volatile acp_sint64_t *)aAddr = aVal;

    acpAtomicAddrUnlock(aAddr);

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

#endif
