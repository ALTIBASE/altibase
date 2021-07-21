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
 * $Id: acpSearch.c 11144 2010-05-28 01:45:51Z gurugio $
 ******************************************************************************/


#include <acp.h>


ACP_EXPORT acp_rc_t acpSearchBinary(
    void *aKey, void *aBase,
    acp_size_t aElements, size_t aSize,
    acp_sint32_t(*aCompar)(const void *, const void *),
    void **aResult)
{
#if defined(ALTI_CFG_OS_WINDOWS)
    acp_sint32_t sMidElement;
    acp_rc_t sFound;
    acp_sint32_t sComparison;
    acp_sint32_t sLowBound = 0;
    acp_sint32_t sHighBound = aElements - 1;
    acp_char_t *sCompElement;
    
    while (ACP_TRUE)
    {

        ACP_TEST_RAISE(sLowBound > sHighBound, NOT_FOUND);

        /* This must not happen, otherwise this function has critical
         * bug. */
        ACP_TEST_RAISE(sHighBound >= (acp_sint32_t)aElements ||
                       sLowBound < 0, CRITICAL_FAULT);

        sMidElement = (sLowBound + sHighBound) / 2;
        sCompElement = (acp_char_t *)aBase + aSize * sMidElement;
        sComparison = aCompar((void *)sCompElement, aKey);

        if (sComparison < 0)
        {
            sLowBound = sMidElement + 1;
        }
        else if (sComparison > 0)
        {
            sHighBound = sMidElement - 1;
        }
        else
        {
            *aResult = sCompElement;
            sFound = ACP_RC_SUCCESS;
            break;
        }
    }
    return ACP_RC_SUCCESS;

    ACP_EXCEPTION(NOT_FOUND)
    {
        sFound = ACP_RC_ENOENT;
        *aResult = NULL;
    }

    ACP_EXCEPTION(CRITICAL_FAULT)
    {
        sFound = ACP_RC_EFAULT;
        *aResult = NULL;
    }

    ACP_EXCEPTION_END;
    return sFound;
#else
    *aResult = bsearch(aKey, aBase, aElements, aSize, aCompar);

    ACP_TEST(*aResult == NULL);

    return ACP_RC_SUCCESS;

    ACP_EXCEPTION_END;
    return ACP_RC_ENOENT;
#endif
}

