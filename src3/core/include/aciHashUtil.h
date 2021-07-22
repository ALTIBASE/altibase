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
 * $Id: 
 *
 **********************************************************************/

#ifndef ACI_HASHUTIL_H
#define ACI_HASHUTIL_H 1

#include <acp.h>

ACP_EXTERN_C_BEGIN

ACP_EXPORT acp_uint32_t aciHashHashString(acp_uint32_t aHash,
                                          const acp_uint8_t *aValue, 
                                          acp_uint32_t aLength);

ACP_EXTERN_C_END

#endif /*  ACI_HASHUTIL_H */

