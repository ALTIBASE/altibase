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

#if !defined(_O_ACP_TEST_H_)
#define _O_ACP_TEST_H_

#include <acpTypes.h>

/**
 * @file
 * @ingroup CoreUnit
 */

ACP_EXTERN_C_BEGIN

#if defined(ACP_CFG_TRACE)

ACP_EXPORT void acpTestTraceBase(char* aFormat, ...);

#  define ACP_TRACE                                                         \
    acpTestTraceBase("ACP_TRACE::[%s:%d] ",ACP_FILE_NAME, ACP_LINE_NUM),    \
    acpTestTraceBase
#else
#  define ACP_TRACE (void)
#endif

#define ACP_EXCEPTION_END                       \
    ACP_EXCEPTION_END_LABEL:                    \
    do                                          \
    {                                           \
    } while(0)

#define ACP_EXCEPTION(aLabel)                   \
    goto ACP_EXCEPTION_END_LABEL;               \
    aLabel:

#define ACP_TEST(aExpr)                         \
    do                                          \
    {                                           \
        if(aExpr)                               \
        {                                       \
            goto ACP_EXCEPTION_END_LABEL;       \
        }                                       \
        else                                    \
        {                                       \
        }                                       \
    } while(0)

#define ACP_TEST_RAISE(aExpr, aLabel)           \
    do                                          \
    {                                           \
        if(aExpr)                               \
        {                                       \
            goto aLabel;                        \
        }                                       \
        else                                    \
        {                                       \
        }                                       \
    } while(0)

#define ACP_RAISE(aLabel)                       \
    do                                          \
    {                                           \
        goto aLabel;                            \
    } while(0)

ACP_EXTERN_C_END

#endif
