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
 * $Id: acpSearch.h 11144 2010-05-28 01:45:51Z gurugio $
 ******************************************************************************/
#if !defined(ACP_SEARCH_H)
#define ACP_SEARCH_H

#include <acp.h>

/**
 * @file
 * @ingroup CoreSearch
 */

ACP_EXTERN_C_BEGIN


/**
 * Binary Search
 * @param aKey target
 * @param aBase initial member of the object array
 * @param aElements amount of objects
 * @param aSize the size of each member of the array
 * @param aCompar comparison function
 * @param aResult pointer to a searched member of the array
 * 
 */
ACP_EXPORT acp_rc_t acpSearchBinary(
    void *aKey, void *aBase,
    acp_size_t aElements, size_t aSize,
    acp_sint32_t(*aCompar)(const void *, const void *),
    void *aResult);

ACP_EXTERN_C_END

#endif /*ACP_SEARCH_H*/
