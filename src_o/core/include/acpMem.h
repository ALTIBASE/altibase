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
 * $Id: acpMem.h 11316 2010-06-22 23:56:42Z djin $
 ******************************************************************************/

#if !defined(_O_ACP_MEM_H_)
#define _O_ACP_MEM_H_

/**
 * @file
 * @ingroup CoreMem
 */

#include <acpTypes.h>
#include <acpError.h>

ACP_EXTERN_C_BEGIN

ACP_EXPORT acp_rc_t acpMemAlloc(void **aAddr, acp_size_t aSize);
ACP_EXPORT acp_rc_t acpMemAllocAlign(void       **aAddr,
                                     acp_size_t   aSize,
                                     acp_size_t   aAlignSize);
ACP_EXPORT acp_rc_t acpMemCalloc(void       **aAddr,
                                 acp_size_t   aElements,
                                 acp_size_t   aSize);
ACP_EXPORT acp_rc_t acpMemRealloc(void **aAddr, acp_size_t aSize);
ACP_EXPORT void acpMemFree(void *aAddr);
ACP_EXPORT void acpMemFreeAlign(void *aAddr);

/**
 * fills the memory with @a aVal
 *
 * @param aAddr pointer to the memory
 * @param aVal the value to write
 * @param aSize size to fill
 */
ACP_INLINE void acpMemSet(void*       aAddr,
                          acp_uint8_t aVal,
                          acp_size_t  aSize)
{
    if (aSize > 0)
    {
#if defined(ALTI_CFG_OS_WINDOWS)
        FillMemory(aAddr, aSize, aVal);
#else
        /* 0xFF is useless but for Codesonar analysis */
        memset(aAddr, (acp_sint32_t)aVal & 0xFF, aSize);
#endif
    }
    else
    {
        /* do nothing */
    }
}

/**
 * compares bytes in memory
 *
 * @param aAddr1 pointer to the memory to compare
 * @param aAddr2 pointer to the memory to compare
 * @param aSize size of memory to compare
 * @return zero if the two memories are identical,
 * otherwise the difference between the first two different bytes
 */
ACP_INLINE acp_sint32_t acpMemCmp(const void *aAddr1,
                                  const void *aAddr2,
                                  acp_size_t  aSize)
{
    if (aSize > 0)
    {
#if defined(ALTI_CFG_OS_WINDOWS)
        acp_uint8_t* sAddr1 = (acp_uint8_t*)aAddr1;
        acp_uint8_t* sAddr2 = (acp_uint8_t*)aAddr2;
        acp_sint32_t sRet = 0;

        while((0 == sRet) && (0 != aSize))
        {
            sRet = (acp_sint32_t)(*sAddr1) - (acp_sint32_t)(*sAddr2);
            sAddr1++;
            sAddr2++;
            aSize--;
        }

        return sRet;
#else
        return memcmp(aAddr1, aAddr2, aSize);
#endif
    }
    else
    {
        return 0;
    }
}

/**
 * copies memory
 *
 * @param aDest pointer to the destination memory
 * @param aSrc pointer to the source memory
 * @param aSize size of memory to copy
 *
 * If the two memories overlap, behavior is undefined.
 * Applications in which the two memories might overlap
 * should use acpMemMove() instead.
 */
ACP_INLINE void acpMemCpy(void*       aDest,
                          const void* aSrc,
                          acp_size_t  aSize)
{
    if (aSize > 0)
    {
#if defined(ALTI_CFG_OS_WINDOWS)
        CopyMemory(aDest, aSrc, aSize);
#else
        memcpy(aDest, aSrc, aSize);
#endif
    }
    else
    {
        /* do nothing */
    }
}

/**
 * copies memory
 *
 * @param aDest pointer to the destination memory
 * @param aSrc pointer to the source memory
 * @param aSize size of memory to move
 *
 * The two memory may overlap;
 * the copy is always done in a non-destructive manner.
 */
ACP_INLINE void acpMemMove(void*       aDest,
                           const void* aSrc,
                           acp_size_t  aSize)
{
    if (aSize > 0)
    {
#if defined(ALTI_CFG_OS_WINDOWS)
        MoveMemory(aDest, aSrc, aSize);
#else
        memmove(aDest, aSrc, aSize);
#endif
    }
    else
    {
        /* do nothing */
    }
}

ACP_EXTERN_C_END


#endif
