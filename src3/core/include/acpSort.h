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
 * $Id: 
 ******************************************************************************/
#if !defined(ACP_SORT_H)
#define ACP_SORT_H

#include <acp.h>

/**
 * @file
 * @ingroup CoreSort
 */

ACP_EXTERN_C_BEGIN

/**
 * Quick Sort
 * @param aBase Pointer to elements to sort
 * @param aElements Number of elements to sort
 * @param aSize Size of each element
 * @param aCompar Comparision function.
 * Must return <0 when aE1 < aE2, =0 when aE1 == aE2, >0 when aE1 > aE2
 */
ACP_INLINE void acpSortQuickSort(
    void *aBase, acp_size_t aElements, size_t aSize,
    acp_sint32_t(*aCompar)(const void *, const void *))
{
    qsort(aBase, aElements, aSize, aCompar);
}

ACP_EXTERN_C_END

#endif /*ACP_SORT_H*/
