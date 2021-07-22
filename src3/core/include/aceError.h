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
 * $Id: aceError.h 4378 2009-01-30 07:51:52Z sjkim $
 ******************************************************************************/

#if !defined(_O_ACE_ERROR_H_)
#define _O_ACE_ERROR_H_

/**
 * @file
 * @ingroup CoreError
 */

#include <acpTypes.h>


ACP_EXTERN_C_BEGIN


/**
 * user defined message id type
 */
typedef acp_uint64_t ace_msgid_t;


/**
 * function result code type
 */
typedef enum ace_rc_t
{
    ACE_RC_SUCCESS = 0, /**< success */
    ACE_RC_FAILURE = -1 /**< failure */
} ace_rc_t;


#define ACE_LEVEL_OFFSET   28
#define ACE_PRODUCT_OFFSET 20
#define ACE_INDEX_OFFSET   32

#define ACE_LEVEL_MASK     ACP_UINT64_LITERAL(0x00000000F0000000)
#define ACE_PRODUCT_MASK   ACP_UINT64_LITERAL(0x000000000FF00000)
#define ACE_SUBCODE_MASK   ACP_UINT64_LITERAL(0x00000000000FFFFF)
#define ACE_INDEX_MASK     ACP_UINT64_LITERAL(0x0000FFFF00000000)
#define ACE_CODE_MASK      (ACE_PRODUCT_MASK | ACE_SUBCODE_MASK)

#define ACE_LEVEL_MIN      0
#define ACE_LEVEL_MAX      0xf

#define ACE_PRODUCT_MIN    0
#define ACE_PRODUCT_MAX    0xff

#define ACE_SUBCODE_MIN    1
#define ACE_SUBCODE_MAX    0xfffff

#define ACE_HIDDEN_ARG1    ((aclContext *)aContext)
#define ACE_HIDDEN_ARG2    ((aclContext *)aContext)->mException
#define ACE_HIDDEN_ARGS    ACE_HIDDEN_ARG1, ACE_HIDDEN_ARG2

#define ACE_LEVEL_0        ACE_HIDDEN_ARGS, 0
#define ACE_LEVEL_1        ACE_HIDDEN_ARGS, 1
#define ACE_LEVEL_2        ACE_HIDDEN_ARGS, 2
#define ACE_LEVEL_3        ACE_HIDDEN_ARGS, 3
#define ACE_LEVEL_4        ACE_HIDDEN_ARGS, 4
#define ACE_LEVEL_5        ACE_HIDDEN_ARGS, 5
#define ACE_LEVEL_6        ACE_HIDDEN_ARGS, 6
#define ACE_LEVEL_7        ACE_HIDDEN_ARGS, 7
#define ACE_LEVEL_8        ACE_HIDDEN_ARGS, 8
#define ACE_LEVEL_9        ACE_HIDDEN_ARGS, 9
#define ACE_LEVEL_10       ACE_HIDDEN_ARGS, 10
#define ACE_LEVEL_11       ACE_HIDDEN_ARGS, 11
#define ACE_LEVEL_12       ACE_HIDDEN_ARGS, 12
#define ACE_LEVEL_13       ACE_HIDDEN_ARGS, 13
#define ACE_LEVEL_14       ACE_HIDDEN_ARGS, 14
#define ACE_LEVEL_15       ACE_HIDDEN_ARGS, 15

/**
 * gets error level
 * @param e error code
 * @result error level
 */
#define ACE_ERROR_LEVEL(e)   (acp_sint32_t)\
                             (((e) & ACE_LEVEL_MASK) >> ACE_LEVEL_OFFSET)
/**
 * gets error code (without error level)
 * @param e error code
 * @result error code
 */
#define ACE_ERROR_CODE(e)    (acp_sint32_t)((e) & ACE_CODE_MASK)
/**
 * gets error code product code
 * @param e error code
 * @result product code
 */
#define ACE_ERROR_PRODUCT(e) (acp_sint32_t)\
                             (((e) & ACE_PRODUCT_MASK) >> ACE_PRODUCT_OFFSET)
/**
 * gets sub error code (without error level and product code)
 * @param e error code
 * @result sub error code
 */
#define ACE_ERROR_SUBCODE(e) (acp_sint32_t)((e) & ACE_SUBCODE_MASK)

/**
 * gets index code (internal number)
 * @param e error code
 * @result index  code
 */
#define ACE_ERROR_INDEX(e) (acp_sint32_t)\
                           (((e) & ACE_INDEX_MASK) >> ACE_INDEX_OFFSET)


ACP_EXTERN_C_END


#endif
