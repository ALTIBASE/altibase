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
 * $Id: acpBit.h 6940 2009-08-13 13:07:15Z djin $
 ******************************************************************************/

#if !defined(_O_ACP_BIT_H_)
#define _O_ACP_BIT_H_

/**
 * @file
 * @ingroup CoreMath
 */

#include <acpTypes.h>


ACP_EXTERN_C_BEGIN

#if defined(__STATIC_ANALYSIS_DOING__) && !defined(ALINT)

ACP_INLINE acp_sint32_t acpBitFfs32(acp_uint32_t aValue)
{
    ACP_UNUSED(aValue);
    return 0;
}

ACP_INLINE acp_sint32_t acpBitFfs64(acp_uint32_t aValue)
{
    ACP_UNUSED(aValue);
    return 0;
}

ACP_INLINE acp_sint32_t acpBitFls32(acp_uint32_t aValue)
{
    ACP_UNUSED(aValue);
    return 0;
}

ACP_INLINE acp_sint32_t acpBitFls64(acp_uint32_t aValue)
{
    ACP_UNUSED(aValue);
    return 0;
}

ACP_INLINE acp_sint32_t acpBitCountSet32(acp_uint32_t aValue)
{
    ACP_UNUSED(aValue);
    return 0;
}
ACP_INLINE acp_sint32_t acpBitCountSet64(acp_uint32_t aValue)
{
    ACP_UNUSED(aValue);
    return 0;
}

#else 

ACP_INLINE acp_sint32_t acpBitCountSetGeneric(acp_uint32_t aValue)
{
    acp_sint32_t sCnt = 0;

    while( aValue > 0 )
    {
        sCnt += aValue & 1;
        aValue >>= 1;
    }
    return sCnt;
}

#if !defined(ACP_CFG_DOXYGEN) && defined(__GNUC__) &&                   \
    ((__GNUC__ > 3) || ((__GNUC__ == 3) && (__GNUC_MINOR__ >= 4)))

ACP_INLINE acp_sint32_t acpBitFfs32(acp_uint32_t aValue)
{
    return __builtin_ffs(aValue) - 1;
}

ACP_INLINE acp_sint32_t acpBitFls32(acp_uint32_t aValue)
{
    return (aValue == 0) ? (-1) : (31 - __builtin_clz(aValue));
}

ACP_INLINE acp_sint32_t acpBitCountSet32(acp_uint32_t aValue)
{
#if defined(ALTI_CFG_CPU_X86)
    return __builtin_popcount(aValue);
#else
    return acpBitCountSetGeneric(aValue);
#endif
}

#define ACP_BIT_HAS_NATIVE_64BIT

/* Macros also causes problem with $(CC) -O2 in 32bit release mode. *
#if defined(ACP_CFG_COMPILE_64BIT)
#define acpBitFfs64(aValue) (__builtin_ffsl(aValue) - 1)
#else
#define acpBitFfs64(aValue) (__builtin_ffsll(aValue) - 1)
#endif
* Never uncomment these lines!!! */

ACP_INLINE acp_sint32_t acpBitFfs64(acp_uint64_t aValue)
{
#if defined(ACP_CFG_COMPILE_64BIT)
    return __builtin_ffsl(aValue) - 1;
#else

#if !defined(ACP_CFG_DOXYGEN) && defined(__GNUC__) && \
((__GNUC__ == 4) && (__GNUC_MINOR__ == 2))
    if (aValue == 0)
    {
        return -1;
    }
    else
    {
        if ((aValue & ACP_UINT64_LITERAL(0xffffffff)) != 0)
        {
            return acpBitFfs32((acp_uint32_t)(aValue &
                                              ACP_UINT64_LITERAL(0xffffffff)));
        }
        else
        {
            return acpBitFfs32((acp_uint32_t)(aValue >> 32)) + 32;
        }
    }
#else
    /* Error with gcc 4.2.9 -> Waiting for updated version */
    return (__builtin_ffsll(aValue) - 1);
#endif /* of GNU C version check */
#endif /* of 64bit compilation */
}

ACP_INLINE acp_sint32_t acpBitFls64(acp_uint64_t aValue)
{
#if defined(ACP_CFG_COMPILE_64BIT)
    return (aValue == 0) ? (-1) : (63 - __builtin_clzl(aValue));
#else
    return (aValue == 0) ? (-1) : (63 - __builtin_clzll(aValue));
#endif
}

ACP_INLINE acp_sint32_t acpBitCountSet64(acp_uint64_t aValue)
{
#if defined(ALTI_CFG_CPU_X86)

#if defined(ACP_CFG_COMPILE_64BIT)
    return __builtin_popcountl(aValue);
#else
    return __builtin_popcountll(aValue);
#endif

#else
    acp_sint32_t sCnt = 0;

    if (aValue != 0)
    {
        if ((aValue & ACP_UINT64_LITERAL(0xffffffff)) != 0 )
        {
            sCnt = acpBitCountSet32((acp_uint32_t)(aValue &
                                          ACP_UINT64_LITERAL(0xffffffff)));
        }
        if ((aValue & ACP_UINT64_LITERAL(0xffffffff00000000)) != 0)
        {
            sCnt += acpBitCountSet32((acp_uint32_t)(aValue >> 32)); 
        }
    }
    return sCnt;
#endif
}



#elif !defined(ACP_CFG_DOXYGEN) && defined(_MSC_VER) && (_MSC_VER >= 1400)

ACP_INLINE acp_sint32_t acpBitFfs32(acp_uint32_t aValue)
{
    unsigned long sIndex;
    return (_BitScanForward(&sIndex, aValue) == 0) ? -1 : sIndex;
}

ACP_INLINE acp_sint32_t acpBitFls32(acp_uint32_t aValue)
{
    unsigned long sIndex;
    return (_BitScanReverse(&sIndex, aValue) == 0) ? -1 : sIndex;
}

#if defined(ACP_CFG_COMPILE_64BIT)

#define ACP_BIT_HAS_NATIVE_64BIT

ACP_INLINE acp_sint32_t acpBitFfs64(acp_uint64_t aValue)
{
    unsigned long sIndex;
    return (_BitScanForward64(&sIndex, aValue) == 0) ? -1 : sIndex;
}

ACP_INLINE acp_sint32_t acpBitFls64(acp_uint64_t aValue)
{
    unsigned long sIndex;
    return (_BitScanReverse64(&sIndex, aValue) == 0) ? -1 : sIndex;
}

#endif

#else

ACP_INLINE acp_sint32_t acpBitFlsGeneric(acp_uint32_t aValue)
{
    acp_sint32_t sBit;

    if (aValue == 0)
    {
        return 0;
    }
    else
    {
        sBit = 32;
    }

    if ((aValue & 0xffff0000) == 0)
    {
        aValue <<= 16;
        sBit    -= 16;
    }
    else
    {
        /* continue */
    }

    if ((aValue & 0xff000000) == 0)
    {
        aValue <<= 8;
        sBit    -= 8;
    }
    else
    {
        /* continue */
    }

    if ((aValue & 0xf0000000) == 0)
    {
        aValue <<= 4;
        sBit    -= 4;
    }
    else
    {
        /* continue */
    }

    if ((aValue & 0xc0000000) == 0)
    {
        aValue <<= 2;
        sBit    -= 2;
    }
    else
    {
        /* continue */
    }

    if ((aValue & 0x80000000) == 0)
    {
        aValue <<= 1;
        sBit    -= 1;
    }
    else
    {
        /* continue */
    }

    return sBit;
}

/**
 * finds first bit set of 32bit value
 * @param aValue a 32bit integer value
 * @return the index of first bit set
 */
ACP_INLINE acp_sint32_t acpBitFfs32(acp_uint32_t aValue)
{
    return acpBitFlsGeneric(aValue & (~aValue + 1)) - 1;
}

/**
 * finds last bit set of 32bit value
 * @param aValue a 32bit integer value
 * @return the index of last bit set
 */
ACP_INLINE acp_sint32_t acpBitFls32(acp_uint32_t aValue)
{
    return acpBitFlsGeneric(aValue) - 1;
}

/**
 * Counts the number of one bits in a 32bit value
 * @param aValue a 32bit integer value
 * @return the number of 1-bit in aValue 
 */
ACP_INLINE acp_sint32_t acpBitCountSet32(acp_uint32_t aValue)
{
    return acpBitCountSetGeneric(aValue);
}
#endif

#if !defined(ACP_BIT_HAS_NATIVE_64BIT) || defined(ACP_CFG_DOXYGEN)

/**
 * finds first bit set of 64bit value
 * @param aValue a 64bit integer value
 * @return the index of first bit set
 */
ACP_INLINE acp_sint32_t acpBitFfs64(acp_uint64_t aValue)
{
    if (aValue == 0)
    {
        return -1;
    }
    else
    {
        if ((aValue & ACP_UINT64_LITERAL(0xffffffff)) != 0)
        {
            return acpBitFfs32((acp_uint32_t)(aValue &
                                              ACP_UINT64_LITERAL(0xffffffff)));
        }
        else
        {
            return acpBitFfs32((acp_uint32_t)(aValue >> 32)) + 32;
        }
    }
}

/**
 * finds last bit set of 64bit value
 * @param aValue a 64bit integer value
 * @return the index of last bit set
 */
ACP_INLINE acp_sint32_t acpBitFls64(acp_uint64_t aValue)
{
    if ((aValue & ACP_UINT64_LITERAL(0xffffffff00000000)) != 0)
    {
        return acpBitFls32((acp_uint32_t)(aValue >> 32)) + 32;
    }
    else
    {
        return acpBitFls32((acp_uint32_t)(aValue &
                                          ACP_UINT64_LITERAL(0xffffffff)));
    }
}

/**
 * Counts the number of one bits in a 64bit value
 * @param aValue a 64bit integer value
 * @return the number of 1-bit in aValue 
 */
ACP_INLINE acp_sint32_t acpBitCountSet64(acp_uint64_t aValue)
{
    acp_sint32_t sCnt = 0;

    if (aValue != 0)
    {
        if ((aValue & ACP_UINT64_LITERAL(0xffffffff)) != 0 )
        {
            sCnt = acpBitCountSet32((acp_uint32_t)(aValue &
                                          ACP_UINT64_LITERAL(0xffffffff)));
        }
        if ((aValue & ACP_UINT64_LITERAL(0xffffffff00000000)) != 0)
        {
            sCnt += acpBitCountSet32((acp_uint32_t)(aValue >> 32)); 
        }
    }
    return sCnt;
}
#endif

#endif /* of static analysis */

ACP_EXTERN_C_END


#endif
