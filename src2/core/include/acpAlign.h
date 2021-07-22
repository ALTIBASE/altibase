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
 * $Id: acpAlign.h 9884 2010-02-03 06:50:04Z gurugio $
 ******************************************************************************/

#if !defined(_O_ACP_ALIGN_H_)
#define _O_ACP_ALIGN_H_

/**
 * @file
 * @ingroup CoreType
 * @ingroup CoreMem
 */

#include <acpTypes.h>


ACP_EXTERN_C_BEGIN


/**
 * @param aValue value to be aligned
 * @return 4-byte aligned value
 */
#define ACP_ALIGN4(aValue)            (((aValue) + 3) & ~3)

/**
 * @param aValue value to be aligned
 * @return 8-byte aligned value
 */
#define ACP_ALIGN8(aValue)            (((aValue) + 7) & ~7)

#if defined(ACP_CFG_DOXYGEN)

/**
 * @param aValue value to be aligned
 * @return 4-byte aligned value in 32-bit system,
 * 8-byte aligned value in 64-bit system
 */
#define ACP_ALIGN(aValue)

#elif defined(ACP_CFG_COMPILE_64BIT)
#define ACP_ALIGN(aValue)             ACP_ALIGN8(aValue)
#else
#define ACP_ALIGN(aValue)             ACP_ALIGN4(aValue)
#endif

/**
 * @param aValue value to be aligned
 * @param aAlign align size
 * @return aligned value with specified align size
 */
#define ACP_ALIGN_ALL(aValue, aAlign)                   \
    ((((aValue) + (aAlign) - 1) / (aAlign)) * (aAlign))

/**
 * @param aPtr pointer to be aligned
 * @return 4-byte aligned pointer
 */
#define ACP_ALIGN4_PTR(aPtr)       ((void *)ACP_ALIGN4((acp_ulong_t)(aPtr)))

/**
 * @param aPtr pointer to be aligned
 * @return 8-byte aligned pointer
 */
#define ACP_ALIGN8_PTR(aPtr)       ((void *)ACP_ALIGN8((acp_ulong_t)(aPtr)))

/**
 * @param aPtr pointer to be aligned
 * @return 4-byte aligned pointer in 32-bit system,
 * 8-byte aligned pointer in 64-byt system
 */
#define ACP_ALIGN_PTR(aPtr)        ((void *)ACP_ALIGN((acp_ulong_t)(aPtr)))

/**
 * @param aPtr pointer to be aligned
 * @param aAlign align size
 * @return aligned pointer with specified align size
 */
#define ACP_ALIGN_ALL_PTR(aPtr, aAlign)                 \
    ((void *)ACP_ALIGN_ALL((acp_ulong_t)(aPtr), (aAlign)))


ACP_EXTERN_C_END


#endif
