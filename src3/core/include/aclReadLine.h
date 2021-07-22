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
 * $Id: aclReadLine.h 8570 2009-11-09 05:35:11Z gurugio $
 ******************************************************************************/

#if !defined(_O_ACL_READLINE_H_)
#define _O_ACL_READLINE_H_

/**
 * @file
 * @ingroup CoreChar
 *
 * ReadLine Library is wrapper of BSD editline library.
 */

#include <acp.h>


ACP_EXTERN_C_BEGIN

/**
 * type for completion function
 * @param text user input string
 * @param state count of function calling
 * @return pointer value of C-string to print. 
 *
 * Return value is a pointer of C-string. It must be
 * allocated dynamically and it will be freed out of the function.
 */
typedef acp_char_t *aclReadLineCallbackFunc_t(const acp_char_t *text,
                                                  acp_sint32_t state);

ACP_EXPORT void aclReadLineSetComplete(aclReadLineCallbackFunc_t *aFunc);

ACP_EXPORT acp_rc_t aclReadLineInit(acp_str_t *aAppName);

ACP_EXPORT acp_rc_t aclReadLineFinal(void);

ACP_EXPORT acp_rc_t aclReadLineLoadHistory(acp_str_t *aFileName);

ACP_EXPORT acp_rc_t aclReadLineSaveHistory(acp_str_t *aFileName);

ACP_EXPORT acp_rc_t aclReadLineAddHistory(acp_str_t *aStr);

ACP_EXPORT acp_bool_t aclReadLineIsInHistory(acp_str_t *aStr);

ACP_EXPORT acp_rc_t aclReadLineGetString(acp_char_t *aPrompt,
                                         acp_str_t  *aGetString);

ACP_EXTERN_C_END


#endif
