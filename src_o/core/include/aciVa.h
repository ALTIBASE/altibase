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
 
/***********************************************************************
 * $Id: aciVa.h 11319 2010-06-23 02:39:42Z djin $
 **********************************************************************/

#ifndef _O_ACIVA_H_
# define _O_ACIVA_H_ 1

#include <acpTypes.h>

ACP_EXTERN_C_BEGIN

#if defined(ALTI_CFG_OS_WINDOWS)
# define ACI_DIRECTORY_SEPARATOR_STR_A "\\"
# define ACI_DIRECTORY_SEPARATOR_CHAR_A '\\'
#else
# define ACI_DIRECTORY_SEPARATOR_STR_A "/"
# define ACI_DIRECTORY_SEPARATOR_CHAR_A '/'
#endif

ACP_EXPORT const acp_char_t *aciVaBasename (const acp_char_t *pathname);
ACP_EXPORT acp_rc_t aciVaAppendFormat(acp_char_t *aBuffer,
                                      acp_size_t aBufferSize,
                                      const acp_char_t *aFormat,
                                      ...);
ACP_EXPORT acp_bool_t aciVaFdeof(acp_file_t *aFile);

ACP_EXTERN_C_END

#endif /* ACIVA_H */

