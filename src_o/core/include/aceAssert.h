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
 * $Id: aceAssert.h 8998 2009-12-03 07:02:04Z gurugio $
 ******************************************************************************/

#if !defined(_O_ACE_ASSERT_H_)
#define _O_ACE_ASSERT_H_

/**
 * @file
 * @ingroup CoreError
 */

#include <acpTypes.h>


ACP_EXTERN_C_BEGIN

/**
 * type of assert callback function
 */
typedef void aceAssertCallback(const acp_char_t *aExpr,
                               const acp_char_t *aFile,
                               acp_sint32_t      aLine);


ACP_EXPORT void aceSetAssertCallback(aceAssertCallback *aCallback);

ACP_EXPORT void aceAssert(const acp_char_t *aExpr,
                          const acp_char_t *aFile,
                          acp_sint32_t      aLine);


/**
 * asserts an expression
 * @param aExpr an expression
 */
#define ACE_ASSERT(aExpr)                               \
    do                                                  \
    {                                                   \
        if (aExpr)                                      \
        {                                               \
        }                                               \
        else                                            \
        {                                               \
            aceAssert(#aExpr, ACP_FILE_NAME, ACP_LINE_NUM);  \
        }                                               \
    } while (0)


#if defined(ACP_CFG_DEBUG) || \
    defined(ACP_CFG_DOXYGEN) || \
    defined(__STATIC_ANALYSIS_DOING__)

/**
 * asserts an expression only in debug build mode
 * @param aExpr an expression
 *
 * it will expand nothing in release build mode
 */
#define ACE_DASSERT(aExpr) ACE_ASSERT(aExpr)

#else

#define ACE_DASSERT(aExpr)

#endif


ACP_EXTERN_C_END


#endif
