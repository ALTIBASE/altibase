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

#ifndef _O_ULN_BIND_PARAM_DATA_IN_H_
#define _O_ULN_BIND_PARAM_DATA_IN_H_ 1

#include <ulnBindInfo.h>
#include <ulnCharSet.h>

#define ULN_BIND_PARAM_DATA_IN_BUILD_ANY_FUNC(aFunctionName)  \
    ACI_RC aFunctionName (ulnFnContext *aFnContext,           \
                          ulnDescRec   *aDescRecApd,          \
                          ulnDescRec   *aDescRecIpd,          \
                          void         *aUserDataPtr,         \
                          acp_sint32_t  aUserOctetLength,     \
                          acp_uint32_t  aRowNo0Based,         \
                          acp_uint8_t  *aConversionBuffer,    \
                          ulnCharSet   *aCharSet)

/*
 * BUGBUG : aDescRecIpd 가 여기 들어가 있는 이유는,
 *          old 함수에서 SQL_C_DEFAULT 를 체크하고 그에 따른 값을 얻기 위해 사용하는
 *          단 한가지 이유밖에 없다.
 *          이것을 잘 처리해서 없애도록 해야 한다.
 */

typedef ACI_RC ulnParamDataInBuildAnyFunc(ulnFnContext *aFnContext,
                                              ulnDescRec   *aDescRecApd,
                                              ulnDescRec   *aDescRecIpd,
                                              void         *aUserDataPtr,
                                              acp_sint32_t aUserOctetLength,
                                              acp_uint32_t aRowNo0Based,
                                              acp_uint8_t  *aConversionBuffer,
                                              ulnCharSet   *aCharSet);

/*
 * ======================================================================
 * SQL_C_CHAR 를 SQL_TYPE 에 알맞게 precision 을 보아가면서 전송하는 함수
 * ======================================================================
 */

ULN_BIND_PARAM_DATA_IN_BUILD_ANY_FUNC(ulnParamDataInBuildAny_CHAR_CHAR);
ULN_BIND_PARAM_DATA_IN_BUILD_ANY_FUNC(ulnParamDataInBuildAny_CHAR_SMALLINT);
ULN_BIND_PARAM_DATA_IN_BUILD_ANY_FUNC(ulnParamDataInBuildAny_CHAR_INTEGER);
ULN_BIND_PARAM_DATA_IN_BUILD_ANY_FUNC(ulnParamDataInBuildAny_CHAR_BIGINT);
ULN_BIND_PARAM_DATA_IN_BUILD_ANY_FUNC(ulnParamDataInBuildAny_CHAR_NUMERIC);
ULN_BIND_PARAM_DATA_IN_BUILD_ANY_FUNC(ulnParamDataInBuildAny_CHAR_BIT);
ULN_BIND_PARAM_DATA_IN_BUILD_ANY_FUNC(ulnParamDataInBuildAny_CHAR_REAL);
ULN_BIND_PARAM_DATA_IN_BUILD_ANY_FUNC(ulnParamDataInBuildAny_CHAR_FLOAT);
ULN_BIND_PARAM_DATA_IN_BUILD_ANY_FUNC(ulnParamDataInBuildAny_CHAR_DOUBLE);
ULN_BIND_PARAM_DATA_IN_BUILD_ANY_FUNC(ulnParamDataInBuildAny_CHAR_BINARY);
ULN_BIND_PARAM_DATA_IN_BUILD_ANY_FUNC(ulnParamDataInBuildAny_CHAR_NIBBLE);
ULN_BIND_PARAM_DATA_IN_BUILD_ANY_FUNC(ulnParamDataInBuildAny_CHAR_BYTE);
ULN_BIND_PARAM_DATA_IN_BUILD_ANY_FUNC(ulnParamDataInBuildAny_CHAR_TIMESTAMP);
ULN_BIND_PARAM_DATA_IN_BUILD_ANY_FUNC(ulnParamDataInBuildAny_CHAR_INTERVAL);
ULN_BIND_PARAM_DATA_IN_BUILD_ANY_FUNC(ulnParamDataInBuildAny_CHAR_NCHAR);
 
ULN_BIND_PARAM_DATA_IN_BUILD_ANY_FUNC(ulnParamDataInBuildAny_NUMERIC_NUMERIC);

/*
 * =======================================
 * SQL_C_BIT --> SQL_TYPES
 * =======================================
 */

ULN_BIND_PARAM_DATA_IN_BUILD_ANY_FUNC(ulnParamDataInBuildAny_BIT_CHAR);
ULN_BIND_PARAM_DATA_IN_BUILD_ANY_FUNC(ulnParamDataInBuildAny_BIT_NCHAR);
ULN_BIND_PARAM_DATA_IN_BUILD_ANY_FUNC(ulnParamDataInBuildAny_BIT_NUMERIC);
ULN_BIND_PARAM_DATA_IN_BUILD_ANY_FUNC(ulnParamDataInBuildAny_BIT_BIT);
ULN_BIND_PARAM_DATA_IN_BUILD_ANY_FUNC(ulnParamDataInBuildAny_BIT_SMALLINT);
ULN_BIND_PARAM_DATA_IN_BUILD_ANY_FUNC(ulnParamDataInBuildAny_BIT_INTEGER);
ULN_BIND_PARAM_DATA_IN_BUILD_ANY_FUNC(ulnParamDataInBuildAny_BIT_BIGINT);
ULN_BIND_PARAM_DATA_IN_BUILD_ANY_FUNC(ulnParamDataInBuildAny_BIT_REAL);
ULN_BIND_PARAM_DATA_IN_BUILD_ANY_FUNC(ulnParamDataInBuildAny_BIT_DOUBLE);

/*
 * ======================================================================
 * SQL_C_XXX --> SQL_XXX
 * ======================================================================
 */

ULN_BIND_PARAM_DATA_IN_BUILD_ANY_FUNC(ulnParamDataInBuildAny_TO_SMALLINT);
ULN_BIND_PARAM_DATA_IN_BUILD_ANY_FUNC(ulnParamDataInBuildAny_TO_INTEGER);
ULN_BIND_PARAM_DATA_IN_BUILD_ANY_FUNC(ulnParamDataInBuildAny_TO_BIGINT);
ULN_BIND_PARAM_DATA_IN_BUILD_ANY_FUNC(ulnParamDataInBuildAny_FLOAT_REAL);
ULN_BIND_PARAM_DATA_IN_BUILD_ANY_FUNC(ulnParamDataInBuildAny_DOUBLE_DOUBLE);

/*
 * =======================================
 * SQL_C_BINARY --> SQL_TYPES
 * =======================================
 */

ULN_BIND_PARAM_DATA_IN_BUILD_ANY_FUNC(ulnParamDataInBuildAny_BINARY_CHAR);
ULN_BIND_PARAM_DATA_IN_BUILD_ANY_FUNC(ulnParamDataInBuildAny_BINARY_NCHAR);
ULN_BIND_PARAM_DATA_IN_BUILD_ANY_FUNC(ulnParamDataInBuildAny_BINARY_BINARY);
ULN_BIND_PARAM_DATA_IN_BUILD_ANY_FUNC(ulnParamDataInBuildAny_BINARY_NUMERIC);
ULN_BIND_PARAM_DATA_IN_BUILD_ANY_FUNC(ulnParamDataInBuildAny_BINARY_BIT);
ULN_BIND_PARAM_DATA_IN_BUILD_ANY_FUNC(ulnParamDataInBuildAny_BINARY_NIBBLE);
ULN_BIND_PARAM_DATA_IN_BUILD_ANY_FUNC(ulnParamDataInBuildAny_BINARY_BYTE);
ULN_BIND_PARAM_DATA_IN_BUILD_ANY_FUNC(ulnParamDataInBuildAny_BINARY_SMALLINT);
ULN_BIND_PARAM_DATA_IN_BUILD_ANY_FUNC(ulnParamDataInBuildAny_BINARY_INTEGER);
ULN_BIND_PARAM_DATA_IN_BUILD_ANY_FUNC(ulnParamDataInBuildAny_BINARY_BIGINT);
ULN_BIND_PARAM_DATA_IN_BUILD_ANY_FUNC(ulnParamDataInBuildAny_BINARY_DOUBLE);
ULN_BIND_PARAM_DATA_IN_BUILD_ANY_FUNC(ulnParamDataInBuildAny_BINARY_REAL);
ULN_BIND_PARAM_DATA_IN_BUILD_ANY_FUNC(ulnParamDataInBuildAny_BINARY_INTERVAL);
ULN_BIND_PARAM_DATA_IN_BUILD_ANY_FUNC(ulnParamDataInBuildAny_BINARY_TIMESTAMP);
ULN_BIND_PARAM_DATA_IN_BUILD_ANY_FUNC(ulnParamDataInBuildAny_BINARY_DATE);

/*
 * =======================================================
 * SQL_C_DATE, SQL_C_TIME, SQL_C_TIMESTAMP --> SQL_TYPES
 * =======================================================
 */

ULN_BIND_PARAM_DATA_IN_BUILD_ANY_FUNC(ulnParamDataInBuildAny_DATE_DATE);
ULN_BIND_PARAM_DATA_IN_BUILD_ANY_FUNC(ulnParamDataInBuildAny_TIME_TIME);

ULN_BIND_PARAM_DATA_IN_BUILD_ANY_FUNC(ulnParamDataInBuildAny_TIMESTAMP_DATE);
ULN_BIND_PARAM_DATA_IN_BUILD_ANY_FUNC(ulnParamDataInBuildAny_TIMESTAMP_TIME);
ULN_BIND_PARAM_DATA_IN_BUILD_ANY_FUNC(ulnParamDataInBuildAny_TIMESTAMP_TIMESTAMP);

/*
 * =============================================================
 * 기존에 사용하던 ulnWriteParamDataInREQ() 를 호출하는 함수
 * =============================================================
 */

ULN_BIND_PARAM_DATA_IN_BUILD_ANY_FUNC(ulnParamDataInBuildAny_OLD);

#undef ULN_BIND_PARAM_DATA_IN_FUNC

#endif /* _O_ULN_BIND_PARAM_DATA_IN_H_ */
