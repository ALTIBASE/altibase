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
 * $Id: acpByteOrder.h 9976 2010-02-10 06:02:39Z djin $
 ******************************************************************************/

#if !defined(_O_ACP_BYTE_ORDER_H_)
#define _O_ACP_BYTE_ORDER_H_

/**
 * @file
 * @ingroup CoreType
 */

#include <acpTypes.h>


ACP_EXTERN_C_BEGIN

#if defined(ALTI_CFG_CPU_X86) && !defined(ACP_CFG_DOXYGEN)
#if defined(ALTI_CFG_OS_WINDOWS)

#if defined(ACP_CFG_COMPILE_32BIT)
/* Windows and 32bit */

ACP_INLINE acp_uint16_t acpByteSwap16(acp_uint16_t aVal)
{
    __asm
    {
        mov ax, aVal;
        xchg ah, al;
    }
}

ACP_INLINE acp_uint32_t acpByteSwap32(acp_uint32_t aVal)
{
    __asm
    {
        mov eax, aVal;
        bswap eax;
    }
}

ACP_INLINE acp_uint64_t acpByteSwap64(acp_uint64_t aVal)
{
    __asm
    {
        mov edx, dword ptr [aVal];
        mov eax, dword ptr [aVal + 4];
        bswap edx;
        bswap eax;
    }
}

#define ACP_SWAP_BYTE2(aSrc, aDst) do { (aDst) = acpByteSwap16(aSrc); } while(0)
#define ACP_SWAP_BYTE4(aSrc, aDst) do { (aDst) = acpByteSwap32(aSrc); } while(0)
#define ACP_SWAP_BYTE8(aSrc, aDst) do { (aDst) = acpByteSwap64(aSrc); } while(0)

#else
/* Windows and 64bit */

#define ACP_SWAP_BYTE2(aSrc, aDst)                                      \
    do { (aDst) = _byteswap_ushort(aSrc); } while(0)

#define ACP_SWAP_BYTE4(aSrc, aDst)                                      \
    do { (aDst) = _byteswap_ulong(aSrc); } while(0)

#define ACP_SWAP_BYTE8(aSrc, aDst)                                      \
    do { (aDst) = _byteswap_uint64(aSrc); } while(0)

#endif

#else /* Not windows in X86 */

#if defined(__GNUC__) && !defined(ACP_CFG_NOINLINE)

ACP_INLINE acp_uint16_t acpByteSwap16(acp_uint16_t aVal)
{
    acp_uint16_t sRes;
    __asm__("xchgb %b0, %h0" : "=Q" (sRes) : "0" (aVal));
    return sRes;
}

ACP_INLINE acp_uint32_t acpByteSwap32(acp_uint32_t aVal)
{
    __asm__("bswap %0" : "=r" (aVal) : "0" (aVal));
    return aVal;
}

#if defined(ACP_CFG_COMPILE_32BIT)

ACP_INLINE acp_uint64_t acpByteSwap64(acp_uint64_t aVal)
{
    typedef union
    {
        acp_uint64_t sLL;
        acp_uint32_t sL[2];
    } acpByteSwap64Typpe;
    
    acpByteSwap64Typpe sVal;
    acpByteSwap64Typpe sRes;

    sVal.sLL = aVal;
    sRes.sL[0] = acpByteSwap32(sVal.sL[1]);
    sRes.sL[1] = acpByteSwap32(sVal.sL[0]);

    return sRes.sLL;
}

#else

ACP_INLINE acp_uint64_t acpByteSwap64(acp_uint64_t aVal)
{
    __asm__("bswap %0" : "=r" (aVal) : "0" (aVal));
    return aVal;
}

#endif

#define ACP_SWAP_BYTE2(aSrc, aDst) do { (aDst) = acpByteSwap16(aSrc); } while(0)
#define ACP_SWAP_BYTE4(aSrc, aDst) do { (aDst) = acpByteSwap32(aSrc); } while(0)
#define ACP_SWAP_BYTE8(aSrc, aDst) do { (aDst) = acpByteSwap64(aSrc); } while(0)

#else /* x86, not gcc or NOINLINE defined */

#define ACP_SWAP_BYTE2(aSrc, aDst)                                           \
    do                                                                       \
    {                                                                        \
        acp_uint16_t sByTeSwApSrc_MACRO_LOCAL_VAR = (aSrc);                 \
        ((acp_char_t *)(&(aDst)))[0] =                                      \
            ((acp_char_t *)(&sByTeSwApSrc_MACRO_LOCAL_VAR))[1];             \
        ((acp_char_t *)(&(aDst)))[1] =                                      \
            ((acp_char_t *)(&sByTeSwApSrc_MACRO_LOCAL_VAR))[0];             \
    } while (0)

#define ACP_SWAP_BYTE4(aSrc, aDst)                                          \
    do                                                                      \
    {                                                                       \
        acp_uint32_t sByTeSwApSrc_MACRO_LOCAL_VAR = (aSrc);                 \
        ((acp_char_t *)(&(aDst)))[0] =                                      \
            ((acp_char_t *)(&sByTeSwApSrc_MACRO_LOCAL_VAR))[3];             \
        ((acp_char_t *)(&(aDst)))[1] =                                      \
            ((acp_char_t *)(&sByTeSwApSrc_MACRO_LOCAL_VAR))[2];             \
        ((acp_char_t *)(&(aDst)))[2] =                                      \
            ((acp_char_t *)(&sByTeSwApSrc_MACRO_LOCAL_VAR))[1];             \
        ((acp_char_t *)(&(aDst)))[3] =                                      \
            ((acp_char_t *)(&sByTeSwApSrc_MACRO_LOCAL_VAR))[0];             \
    } while (0)

#define ACP_SWAP_BYTE8(aSrc, aDst)                              \
    do                                                          \
    {                                                           \
        acp_uint64_t sByTeSwApSrc_MACRO_LOCAL_VAR = (aSrc);     \
        ((acp_char_t *)(&(aDst)))[0] =                          \
            ((acp_char_t *)(&sByTeSwApSrc_MACRO_LOCAL_VAR))[7]; \
        ((acp_char_t *)(&(aDst)))[1] =                          \
            ((acp_char_t *)(&sByTeSwApSrc_MACRO_LOCAL_VAR))[6]; \
        ((acp_char_t *)(&(aDst)))[2] =                          \
            ((acp_char_t *)(&sByTeSwApSrc_MACRO_LOCAL_VAR))[5]; \
        ((acp_char_t *)(&(aDst)))[3] =                          \
            ((acp_char_t *)(&sByTeSwApSrc_MACRO_LOCAL_VAR))[4]; \
        ((acp_char_t *)(&(aDst)))[4] =                          \
            ((acp_char_t *)(&sByTeSwApSrc_MACRO_LOCAL_VAR))[3]; \
        ((acp_char_t *)(&(aDst)))[5] =                          \
            ((acp_char_t *)(&sByTeSwApSrc_MACRO_LOCAL_VAR))[2]; \
        ((acp_char_t *)(&(aDst)))[6] =                          \
            ((acp_char_t *)(&sByTeSwApSrc_MACRO_LOCAL_VAR))[1]; \
        ((acp_char_t *)(&(aDst)))[7] =                          \
            ((acp_char_t *)(&sByTeSwApSrc_MACRO_LOCAL_VAR))[0]; \
    } while (0)

#endif

#endif

#else

/**
 * swap and store 2-byte value
 * @param aSrc source value
 * @param aDst variable to be stored
 */
#define ACP_SWAP_BYTE2(aSrc, aDst)                                          \
    do                                                                      \
    {                                                                       \
        acp_uint16_t sByTeSwApSrc_MACRO_LOCAL_VAR = (aSrc);                 \
        ((acp_char_t *)(&(aDst)))[0] =                                      \
            ((acp_char_t *)(&sByTeSwApSrc_MACRO_LOCAL_VAR))[1];             \
        ((acp_char_t *)(&(aDst)))[1] =                                      \
            ((acp_char_t *)(&sByTeSwApSrc_MACRO_LOCAL_VAR))[0];             \
    } while (0)

/**
 * swap and store 4-byte value
 * @param aSrc source value
 * @param aDst variable to be stored
 */
#define ACP_SWAP_BYTE4(aSrc, aDst)                                          \
    do                                                                      \
    {                                                                       \
        acp_uint32_t sByTeSwApSrc_MACRO_LOCAL_VAR = (aSrc);                 \
        ((acp_char_t *)(&(aDst)))[0] =                                      \
            ((acp_char_t *)(&sByTeSwApSrc_MACRO_LOCAL_VAR))[3];             \
        ((acp_char_t *)(&(aDst)))[1] =                                      \
            ((acp_char_t *)(&sByTeSwApSrc_MACRO_LOCAL_VAR))[2];             \
        ((acp_char_t *)(&(aDst)))[2] =                                      \
            ((acp_char_t *)(&sByTeSwApSrc_MACRO_LOCAL_VAR))[1];             \
        ((acp_char_t *)(&(aDst)))[3] =                                      \
            ((acp_char_t *)(&sByTeSwApSrc_MACRO_LOCAL_VAR))[0];             \
    } while (0)

/**
 * swap and store 8-byte value
 * @param aSrc source value
 * @param aDst variable to be stored
 */
#define ACP_SWAP_BYTE8(aSrc, aDst)                                          \
    do                                                                      \
    {                                                                       \
        acp_uint64_t sByTeSwApSrc_MACRO_LOCAL_VAR = (aSrc);                 \
        ((acp_char_t *)(&(aDst)))[0] =                                      \
            ((acp_char_t *)(&sByTeSwApSrc_MACRO_LOCAL_VAR))[7];             \
        ((acp_char_t *)(&(aDst)))[1] =                                      \
            ((acp_char_t *)(&sByTeSwApSrc_MACRO_LOCAL_VAR))[6];             \
        ((acp_char_t *)(&(aDst)))[2] =                                      \
            ((acp_char_t *)(&sByTeSwApSrc_MACRO_LOCAL_VAR))[5];             \
        ((acp_char_t *)(&(aDst)))[3] =                                      \
            ((acp_char_t *)(&sByTeSwApSrc_MACRO_LOCAL_VAR))[4];             \
        ((acp_char_t *)(&(aDst)))[4] =                                      \
            ((acp_char_t *)(&sByTeSwApSrc_MACRO_LOCAL_VAR))[3];             \
        ((acp_char_t *)(&(aDst)))[5] =                                      \
            ((acp_char_t *)(&sByTeSwApSrc_MACRO_LOCAL_VAR))[2];             \
        ((acp_char_t *)(&(aDst)))[6] =                                      \
            ((acp_char_t *)(&sByTeSwApSrc_MACRO_LOCAL_VAR))[1];             \
        ((acp_char_t *)(&(aDst)))[7] =                                      \
            ((acp_char_t *)(&sByTeSwApSrc_MACRO_LOCAL_VAR))[0];             \
    } while (0)

#endif

#if defined(ACP_CFG_DOXYGEN)

/**
 * convert 2-byte value to host byte order
 * @param aSrc source value
 * @param aDst variable to be stored
 */
#define ACP_TOH_BYTE2(aSrc, aDst)

/**
 * convert 4-byte value to host byte order
 * @param aSrc source value
 * @param aDst variable to be stored
 */
#define ACP_TOH_BYTE4(aSrc, aDst)

/**
 * convert 8-byte value to host byte order
 * @param aSrc source value
 * @param aDst variable to be stored
 */
#define ACP_TOH_BYTE8(aSrc, aDst)

/**
 * convert 2-byte value to network byte order
 * @param aSrc source value
 * @param aDst variable to be stored
 */
#define ACP_TON_BYTE2(aSrc, aDst)

/**
 * convert 4-byte value to network byte order
 * @param aSrc source value
 * @param aDst variable to be stored
 */
#define ACP_TON_BYTE4(aSrc, aDst)

/**
 * convert 8-byte value to network byte order
 * @param aSrc source value
 * @param aDst variable to be stored
 */
#define ACP_TON_BYTE8(aSrc, aDst)

#elif defined(ACP_CFG_BIG_ENDIAN)
#define ACP_TOH_BYTE2(aSrc, aDst) do { (aDst) = (aSrc); } while(0)
#define ACP_TOH_BYTE4(aSrc, aDst) do { (aDst) = (aSrc); } while(0)
#define ACP_TOH_BYTE8(aSrc, aDst) do { (aDst) = (aSrc); } while(0)
#define ACP_TON_BYTE2(aSrc, aDst) do { (aDst) = (aSrc); } while(0)
#define ACP_TON_BYTE4(aSrc, aDst) do { (aDst) = (aSrc); } while(0)
#define ACP_TON_BYTE8(aSrc, aDst) do { (aDst) = (aSrc); } while(0)
#else
#define ACP_TOH_BYTE2(aSrc, aDst) ACP_SWAP_BYTE2(aSrc, aDst)
#define ACP_TOH_BYTE4(aSrc, aDst) ACP_SWAP_BYTE4(aSrc, aDst)
#define ACP_TOH_BYTE8(aSrc, aDst) ACP_SWAP_BYTE8(aSrc, aDst)
#define ACP_TON_BYTE2(aSrc, aDst) ACP_SWAP_BYTE2(aSrc, aDst)
#define ACP_TON_BYTE4(aSrc, aDst) ACP_SWAP_BYTE4(aSrc, aDst)
#define ACP_TON_BYTE8(aSrc, aDst) ACP_SWAP_BYTE8(aSrc, aDst)
#endif


/* ------------------------------------------------
 *  Pointer Based Swap 
 * ----------------------------------------------*/
#define ACP_SWAP_BYTE2_PTR(aSrc, aDst)                                  \
    do                                                                  \
    {                                                                   \
        acp_uint8_t sByTeSwApSrc_MACRO_LOCAL_VAR[2];                    \
        sByTeSwApSrc_MACRO_LOCAL_VAR[0] = ((acp_char_t *)(aSrc))[1];    \
        sByTeSwApSrc_MACRO_LOCAL_VAR[1] = ((acp_char_t *)(aSrc))[0];    \
        ((acp_char_t *)(aDst))[0] = sByTeSwApSrc_MACRO_LOCAL_VAR[0];    \
        ((acp_char_t *)(aDst))[1] = sByTeSwApSrc_MACRO_LOCAL_VAR[1];    \
    } while (0)

#define ACP_SWAP_BYTE4_PTR(aSrc, aDst)                                      \
    do                                                                      \
    {                                                                       \
        acp_uint8_t sByTeSwApSrc_MACRO_LOCAL_VAR[4];                        \
        sByTeSwApSrc_MACRO_LOCAL_VAR[0] = ((acp_char_t *)(aSrc))[3];        \
        sByTeSwApSrc_MACRO_LOCAL_VAR[1] = ((acp_char_t *)(aSrc))[2];        \
        sByTeSwApSrc_MACRO_LOCAL_VAR[2] = ((acp_char_t *)(aSrc))[1];        \
        sByTeSwApSrc_MACRO_LOCAL_VAR[3] = ((acp_char_t *)(aSrc))[0];        \
        ((acp_char_t *)(aDst))[0] = sByTeSwApSrc_MACRO_LOCAL_VAR[0];        \
        ((acp_char_t *)(aDst))[1] = sByTeSwApSrc_MACRO_LOCAL_VAR[1];        \
        ((acp_char_t *)(aDst))[2] = sByTeSwApSrc_MACRO_LOCAL_VAR[2];        \
        ((acp_char_t *)(aDst))[3] = sByTeSwApSrc_MACRO_LOCAL_VAR[3];        \
    } while (0)

#define ACP_SWAP_BYTE8_PTR(aSrc, aDst)                                      \
    do                                                                      \
    {                                                                       \
        acp_uint8_t sByTeSwApSrc_MACRO_LOCAL_VAR[8];                        \
        sByTeSwApSrc_MACRO_LOCAL_VAR[0] = ((acp_char_t *)(aSrc))[7];        \
        sByTeSwApSrc_MACRO_LOCAL_VAR[1] = ((acp_char_t *)(aSrc))[6];        \
        sByTeSwApSrc_MACRO_LOCAL_VAR[2] = ((acp_char_t *)(aSrc))[5];        \
        sByTeSwApSrc_MACRO_LOCAL_VAR[3] = ((acp_char_t *)(aSrc))[4];        \
        sByTeSwApSrc_MACRO_LOCAL_VAR[4] = ((acp_char_t *)(aSrc))[3];        \
        sByTeSwApSrc_MACRO_LOCAL_VAR[5] = ((acp_char_t *)(aSrc))[2];        \
        sByTeSwApSrc_MACRO_LOCAL_VAR[6] = ((acp_char_t *)(aSrc))[1];        \
        sByTeSwApSrc_MACRO_LOCAL_VAR[7] = ((acp_char_t *)(aSrc))[0];        \
        ((acp_char_t *)(aDst))[0] = sByTeSwApSrc_MACRO_LOCAL_VAR[0];        \
        ((acp_char_t *)(aDst))[1] = sByTeSwApSrc_MACRO_LOCAL_VAR[1];        \
        ((acp_char_t *)(aDst))[2] = sByTeSwApSrc_MACRO_LOCAL_VAR[2];        \
        ((acp_char_t *)(aDst))[3] = sByTeSwApSrc_MACRO_LOCAL_VAR[3];        \
        ((acp_char_t *)(aDst))[4] = sByTeSwApSrc_MACRO_LOCAL_VAR[4];        \
        ((acp_char_t *)(aDst))[5] = sByTeSwApSrc_MACRO_LOCAL_VAR[5];        \
        ((acp_char_t *)(aDst))[6] = sByTeSwApSrc_MACRO_LOCAL_VAR[6];        \
        ((acp_char_t *)(aDst))[7] = sByTeSwApSrc_MACRO_LOCAL_VAR[7];        \
    } while (0)


/* ------------------------------------------------
 *  Pointer Based Assign 
 * ----------------------------------------------*/


#define ACP_ASGN_BYTE2_PTR(aSrc, aDst)                                  \
    do                                                                  \
    {                                                                   \
        ((acp_char_t *)(aDst))[0] = ((acp_char_t *)(aSrc))[0];          \
        ((acp_char_t *)(aDst))[1] = ((acp_char_t *)(aSrc))[1];          \
    } while (0)

#define ACP_ASGN_BYTE4_PTR(aSrc, aDst)                                  \
    do                                                                  \
    {                                                                   \
        ((acp_char_t *)(aDst))[0] = ((acp_char_t *)(aSrc))[0];        \
        ((acp_char_t *)(aDst))[1] = ((acp_char_t *)(aSrc))[1];        \
        ((acp_char_t *)(aDst))[2] = ((acp_char_t *)(aSrc))[2];        \
        ((acp_char_t *)(aDst))[3] = ((acp_char_t *)(aSrc))[3];        \
    } while (0)

#define ACP_ASGN_BYTE8_PTR(aSrc, aDst)                                  \
    do                                                                  \
    {                                                                   \
        ((acp_char_t *)(aDst))[0] = ((acp_char_t *)(aSrc))[0];        \
        ((acp_char_t *)(aDst))[1] = ((acp_char_t *)(aSrc))[1];        \
        ((acp_char_t *)(aDst))[2] = ((acp_char_t *)(aSrc))[2];        \
        ((acp_char_t *)(aDst))[3] = ((acp_char_t *)(aSrc))[3];        \
        ((acp_char_t *)(aDst))[4] = ((acp_char_t *)(aSrc))[4];        \
        ((acp_char_t *)(aDst))[5] = ((acp_char_t *)(aSrc))[5];        \
        ((acp_char_t *)(aDst))[6] = ((acp_char_t *)(aSrc))[6];        \
        ((acp_char_t *)(aDst))[7] = ((acp_char_t *)(aSrc))[7];        \
    } while (0)


#if defined(ACP_CFG_DOXYGEN)

/**
 * convert 2-byte value to host byte order
 * @param aSrc source pointer
 * @param aDst pointer to be stored
 */
#define ACP_TOH_BYTE2_PTR(aSrc, aDst)

/**
 * convert 4-byte value to host byte order
 * @param aSrc source pointer
 * @param aDst pointer to be stored
 */
#define ACP_TOH_BYTE4_PTR(aSrc, aDst)

/**
 * convert 8-byte value to host byte order
 * @param aSrc source pointer
 * @param aDst pointer to be stored
 */
#define ACP_TOH_BYTE8_PTR(aSrc, aDst)

/**
 * convert 2-byte value to network byte order
 * @param aSrc source pointer
 * @param aDst pointer to be stored
 */
#define ACP_TON_BYTE2_PTR(aSrc, aDst)

/**
 * convert 4-byte value to network byte order
 * @param aSrc source pointer
 * @param aDst pointer to be stored
 */
#define ACP_TON_BYTE4_PTR(aSrc, aDst)

/**
 * convert 8-byte value to network byte order
 * @param aSrc source pointer
 * @param aDst pointer to be stored
 */
#define ACP_TON_BYTE8_PTR(aSrc, aDst)

#elif defined(ACP_CFG_BIG_ENDIAN)
#define ACP_TOH_BYTE2_PTR(aSrc, aDst) ACP_ASGN_BYTE2_PTR((aSrc), (aDst))
#define ACP_TOH_BYTE4_PTR(aSrc, aDst) ACP_ASGN_BYTE4_PTR((aSrc), (aDst))
#define ACP_TOH_BYTE8_PTR(aSrc, aDst) ACP_ASGN_BYTE8_PTR((aSrc), (aDst))
#define ACP_TON_BYTE2_PTR(aSrc, aDst) ACP_ASGN_BYTE2_PTR((aSrc), (aDst))
#define ACP_TON_BYTE4_PTR(aSrc, aDst) ACP_ASGN_BYTE4_PTR((aSrc), (aDst))
#define ACP_TON_BYTE8_PTR(aSrc, aDst) ACP_ASGN_BYTE8_PTR((aSrc), (aDst))
#else
#define ACP_TOH_BYTE2_PTR(aSrc, aDst) ACP_SWAP_BYTE2_PTR((aSrc), (aDst))
#define ACP_TOH_BYTE4_PTR(aSrc, aDst) ACP_SWAP_BYTE4_PTR((aSrc), (aDst))
#define ACP_TOH_BYTE8_PTR(aSrc, aDst) ACP_SWAP_BYTE8_PTR((aSrc), (aDst))
#define ACP_TON_BYTE2_PTR(aSrc, aDst) ACP_SWAP_BYTE2_PTR((aSrc), (aDst))
#define ACP_TON_BYTE4_PTR(aSrc, aDst) ACP_SWAP_BYTE4_PTR((aSrc), (aDst))
#define ACP_TON_BYTE8_PTR(aSrc, aDst) ACP_SWAP_BYTE8_PTR((aSrc), (aDst))
#endif

/* ------------------------------------------------
 *  Single Argument Byte Ordering API
 * ----------------------------------------------*/

ACP_INLINE acp_uint16_t acpByteOrderTOH2(acp_uint16_t aValue)
{
    acp_uint16_t sDest;
    
    ACP_TOH_BYTE2(aValue, sDest);

    return sDest;
}

ACP_INLINE acp_uint16_t acpByteOrderTON2(acp_uint16_t aValue)
{
    acp_uint16_t sDest;
    
    ACP_TON_BYTE2(aValue, sDest);

    return sDest;
}


ACP_INLINE acp_uint32_t acpByteOrderTOH4(acp_uint32_t aValue)
{
    acp_uint32_t sDest;
    
    ACP_TOH_BYTE4(aValue, sDest);

    return sDest;
}

ACP_INLINE acp_uint32_t acpByteOrderTON4(acp_uint32_t aValue)
{
    acp_uint32_t sDest;
    
    ACP_TON_BYTE4(aValue, sDest);

    return sDest;
}


ACP_INLINE acp_uint64_t acpByteOrderTOH8(acp_uint64_t aValue)
{
    acp_uint64_t sDest;
    
    ACP_TOH_BYTE8(aValue, sDest);

    return sDest;
}

ACP_INLINE acp_uint64_t acpByteOrderTON8(acp_uint64_t aValue)
{
    acp_uint64_t sDest;
    
    ACP_TON_BYTE8(aValue, sDest);

    return sDest;
}


/**
 * convert 2-byte value to host byte order
 * @param aValue source value
 */
#define ACP_TOH_BYTE2_ARG(aValue)  \
                       ((acp_uint16_t)acpByteOrderTOH2((acp_uint16_t)(aValue)))

/**
 * convert 4-byte value to host byte order
 * @param aValue source value
 */
#define ACP_TOH_BYTE4_ARG(aValue)  \
                       ((acp_uint32_t)acpByteOrderTOH4((acp_uint32_t)(aValue)))

/**
 * convert 4-byte value to host byte order
 * @param aValue source value
 */
#define ACP_TOH_BYTE8_ARG(aValue)  \
                       ((acp_uint64_t)acpByteOrderTOH8((acp_uint64_t)(aValue)))

/**
 * convert 2-byte value to network byte order
 * @param aValue source value
 */
#define ACP_TON_BYTE2_ARG(aValue)  \
                       ((acp_uint32_t)acpByteOrderTON2((acp_uint32_t)(aValue)))

/**
 * convert 4-byte value to network byte order
 * @param aValue source value
 */
#define ACP_TON_BYTE4_ARG(aValue)  \
                       ((acp_uint32_t)acpByteOrderTON4((acp_uint32_t)(aValue)))

/**
 * convert 8-byte value to network byte order
 * @param aValue source value
 */
#define ACP_TON_BYTE8_ARG(aValue) \
                       ((acp_uint64_t)acpByteOrderTON8((acp_uint64_t)(aValue)))


ACP_EXTERN_C_END


#endif
