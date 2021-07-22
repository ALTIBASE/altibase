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
 * $Id: actDump.h 9884 2010-02-03 06:50:04Z gurugio $
 ******************************************************************************/

#if !defined(_O_ACT_DUMP_H_)
#define _O_ACT_DUMP_H_

/**
 * @file
 * @ingroup CoreUnit
 */

#include <acpPrintf.h>


ACP_EXTERN_C_BEGIN


ACP_EXPORT void actDump(void *aBuffer, acp_size_t aLen);


/**
 * dumps a memory region
 * @param aBuffer memory pointer to dump
 * @param aLen memory size to dump
 */
#define ACT_DUMP(aBuffer, aLen)                                 \
    do                                                          \
    {                                                           \
        actTestError(__FILE__, __LINE__);                       \
        (void)acpPrintf("dump %zu byte(s) of " #aBuffer "\n",   \
                        (acp_size_t)(aLen));                    \
        actDump((aBuffer), (aLen));                             \
    } while (0)

/**
 * dumps a memory region
 * @param aBuffer memory pointer to dump
 * @param aLen memory size to dump
 * @param aMsg message to print;
 * this should be parenthesized because it actually calls 'acpPrintf @a aMsg'
 */
#define ACT_DUMP_DESC(aBuffer, aLen, aMsg)                      \
    do                                                          \
    {                                                           \
        actTestError(__FILE__, __LINE__);                       \
        (void)acpPrintf aMsg ;                                  \
        (void)acpPrintf("\n");                                  \
        actDump((aBuffer), (aLen));                             \
    } while (0)


ACP_EXTERN_C_END


#endif
