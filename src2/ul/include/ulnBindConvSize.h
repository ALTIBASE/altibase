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

#ifndef _O_ULN_BIND_CONV_SIZE_H_
#define _O_ULN_BIND_CONV_SIZE_H_ 1

#include <ulnBindInfo.h>

#define ULN_BIND_CONV_SIZE_FUNC(aFunctionName)          \
    acp_sint32_t aFunctionName (ulnDbc       *aDbc,     \
                                acp_sint32_t  aUserOctetLength)

typedef acp_sint32_t ulnBindConvSizeFunc(ulnDbc       *aDbc,
                                         acp_sint32_t  aUserOctetLength);

ULN_BIND_CONV_SIZE_FUNC(ulnBindConvSize_NONE);
ULN_BIND_CONV_SIZE_FUNC(ulnBindConvSize_DB_CHARACTER);
ULN_BIND_CONV_SIZE_FUNC(ulnBindConvSize_NATIONAL_CHARACTER);

#undef ULN_BIND_PARAM_SIZE_FUNC

#endif /* _O_ULN_BIND_CONV_SIZE_H_ */
