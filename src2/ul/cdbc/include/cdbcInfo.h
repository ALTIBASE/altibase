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

#ifndef CDBC_INFO_H
#define CDBC_INFO_H 1

ACP_EXTERN_C_BEGIN



#define ALTIBASE_MAX_HOSTINFO_LEN       255



typedef enum cdbcABQueryType
{
    ALTIBASE_QUERY_NONE     = 0,
    ALTIBASE_QUERY_DDL,
    ALTIBASE_QUERY_SELECT,
    ALTIBASE_QUERY_INSERT,
    ALTIBASE_QUERY_UPDATE,
    ALTIBASE_QUERY_DELETE,
    ALTIBASE_QUERY_UNKNOWN
} cdbcABQueryType;

#define QUERY_IS_SELECTABLE(type)   (((type) == ALTIBASE_QUERY_SELECT ) || \
                                     ((type) == ALTIBASE_QUERY_UNKNOWN))
#define QUERY_NOT_SELECTABLE(type)  (! QUERY_IS_SELECTABLE(type))



CDBC_INTERNAL cdbcABQueryType altibase_query_type (const acp_char_t *aQstr);
CDBC_INTERNAL ALTIBASE_FIELD_TYPE altibase_typeconv_sql2alti (SQLSMALLINT aSqlType);
CDBC_INTERNAL SQLSMALLINT altibase_typeconv_alti2sql (ALTIBASE_FIELD_TYPE aAbType);



ACP_EXTERN_C_END

#endif /* CDBC_INFO_H */

