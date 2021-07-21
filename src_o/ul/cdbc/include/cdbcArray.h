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

#ifndef CDBC_ARRAY_H
#define CDBC_ARRAY_H 1

ACP_EXTERN_C_BEGIN



CDBC_INTERNAL ALTIBASE_RC altibase_result_set_array_bind (cdbcABRes *aABRes, acp_sint32_t aArraySize);
CDBC_INTERNAL ALTIBASE_RC altibase_result_set_array_fetch (cdbcABRes *aABRes, acp_sint32_t aArraySize);



ACP_EXTERN_C_END

#endif /* CDBC_ARRAY_H */

