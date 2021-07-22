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

#ifndef _O_ULN_TYPES_H_
#define _O_ULN_TYPES_H_ 1

/*
 * 해당 PRECISION들은 최적의 상수를 찾기 위해  DB Type이 가질수 있는 최대
 * 크기로 부터 유추되었지만, DB Type과 상관없는 독립적인 상수이다.
 */
// BUG-24378 서버의 CHAR_MAX_PRECISION 이 32000으로 줄어들었음
// BUG-23061 nibble 의 최대길이가 254입니다. (MTD_NIBBLE_PRECISION_MAXIMUM)
#define ULN_DEFAULT_PRECISION         0
#define ULN_MAX_NUMERIC_PRECISION     38
#define ULN_MAX_NUMERIC_SCALE         20
#define ULN_MAX_CHAR_PRECISION        (32000)
#define ULN_MAX_GEOMETRY_PRECISION    (32000)
#define ULN_MAX_VARCHAR_PRECISION     (32000)
#define ULN_MAX_VARBIT_PRECISION      (32000)
#define ULN_MAX_NIBBLE_PRECISION      (254)
#define ULN_MAX_BYTE_PRECISION        (32000)
#define ULN_MAX_BINARY_PRECISION      (32000)

/*
 * SQL_C_XXXX 의 enumeration
 */
typedef enum ulnCTypeID
{
    ULN_CTYPE_NULL         =  0,
    ULN_CTYPE_DEFAULT      =  1,
    ULN_CTYPE_CHAR         =  2,
    ULN_CTYPE_NUMERIC      =  3,
    ULN_CTYPE_BIT          =  4,
    ULN_CTYPE_STINYINT     =  5,
    ULN_CTYPE_UTINYINT     =  6,
    ULN_CTYPE_SSHORT       =  7,
    ULN_CTYPE_USHORT       =  8,
    ULN_CTYPE_SLONG        =  9,
    ULN_CTYPE_ULONG        = 10,
    ULN_CTYPE_SBIGINT      = 11,
    ULN_CTYPE_UBIGINT      = 12,
    ULN_CTYPE_FLOAT        = 13,
    ULN_CTYPE_DOUBLE       = 14,
    ULN_CTYPE_BINARY       = 15,
    ULN_CTYPE_DATE         = 16,
    ULN_CTYPE_TIME         = 17,
    ULN_CTYPE_TIMESTAMP    = 18,
    ULN_CTYPE_INTERVAL     = 19,
    ULN_CTYPE_BLOB_LOCATOR = 20,
    ULN_CTYPE_CLOB_LOCATOR = 21,
    ULN_CTYPE_FILE         = 22,
    ULN_CTYPE_WCHAR        = 23,
    ULN_CTYPE_MAX          = 24
} ulnCTypeID;

/*
 * MTD_XXXX_ID 의 enumeration
 */
typedef enum ulnMTypeID
{
    ULN_MTYPE_NULL         =  0,
    ULN_MTYPE_CHAR         =  1,
    ULN_MTYPE_VARCHAR      =  2,
    ULN_MTYPE_NUMBER       =  3,
    ULN_MTYPE_NUMERIC      =  4,
    ULN_MTYPE_BIT          =  5,
    ULN_MTYPE_SMALLINT     =  6,
    ULN_MTYPE_INTEGER      =  7,
    ULN_MTYPE_BIGINT       =  8,
    ULN_MTYPE_REAL         =  9,
    ULN_MTYPE_FLOAT        = 10,
    ULN_MTYPE_DOUBLE       = 11,
    ULN_MTYPE_BINARY       = 12,
    ULN_MTYPE_VARBIT       = 13,
    ULN_MTYPE_NIBBLE       = 14,
    ULN_MTYPE_BYTE         = 15,
    ULN_MTYPE_VARBYTE      = 16,
    /* bind column info 에서 제대로 된 매핑을 하기 위해서 TIMESTAMP 를 가장 앞에 놓았다. */
    ULN_MTYPE_TIMESTAMP    = 17,
    /* 서버에서 올라오지 않음. fetch 시에는 전혀 필요없음. insert, update 시에만 필요함. */
    ULN_MTYPE_DATE         = 18,
    /* 서버에서 올라오지 않음. fetch 시에는 전혀 필요없음. insert, update 시에만 필요함. */
    ULN_MTYPE_TIME         = 19,
    ULN_MTYPE_INTERVAL     = 20,
    ULN_MTYPE_BLOB         = 21,
    ULN_MTYPE_CLOB         = 22,
    ULN_MTYPE_BLOB_LOCATOR = 23,
    ULN_MTYPE_CLOB_LOCATOR = 24,
    ULN_MTYPE_GEOMETRY     = 25,
    ULN_MTYPE_NCHAR        = 26,
    ULN_MTYPE_NVARCHAR     = 27,
    ULN_MTYPE_MAX          = 28
} ulnMTypeID;

/*
 * 각종 타입 매칭, 변환, 기본값 등을 얻는 함수
 */
acp_uint32_t ulnTypeMap_MTYPE_MTD(ulnMTypeID aMTYPE);
ulnMTypeID   ulnTypeMap_MTD_MTYPE(acp_uint32_t aMTD_TYPE);

acp_sint16_t ulnTypeMap_MTYPE_SQL(ulnMTypeID aMTYPE);
ulnMTypeID   ulnTypeMap_SQL_MTYPE(acp_sint16_t aSQL_TYPE);

acp_sint16_t ulnTypeMap_CTYPE_SQLC(ulnCTypeID aCTYPE);
ulnCTypeID   ulnTypeMap_SQLC_CTYPE(acp_sint16_t aSQL_C_TYPE);

acp_sint16_t ulnTypeMap_LOB_SQLTYPE(acp_sint16_t aSQLTYPE, acp_bool_t aLongDataCompat);

acp_uint32_t ulnTypeGetColumnSizeOfType(ulnMTypeID aMTYPE, ulnMeta *aMeta);
acp_sint16_t ulnTypeGetDecimalDigitsOfType(ulnMTypeID aMTYPE, ulnMeta *aMeta);

acp_sint16_t ulnTypeGetDefault_SQL_C_TYPE(ulnMTypeID aMTYPE);

acp_sint16_t ulnTypeGetOdbcDatetimeIntCode(acp_sint16_t aType);
acp_sint16_t ulnTypeGetOdbcVerboseType(acp_sint16_t aType);

ACP_INLINE acp_bool_t ulnTypeIsMemBoundLob(ulnMTypeID aMTYPE, ulnCTypeID aCTYPE)
{
    if (aMTYPE == ULN_MTYPE_CLOB || aMTYPE == ULN_MTYPE_BLOB)
    {
        /*
         * 사용자가 lob locator 로 바인딩하지 않았으면
         *      --> SQL_C_FILE, SQL_C_BINARY, SQL_C_CHAR --> memory bound lob
         */
        if (aCTYPE != ULN_CTYPE_BLOB_LOCATOR && aCTYPE != ULN_CTYPE_CLOB_LOCATOR)
        {
            return ACP_TRUE;
        }
    }

    return ACP_FALSE;
}

acp_bool_t ulnTypeIsOdbcConciseType(acp_sint16_t aType);
acp_bool_t ulnTypeIsVariableLength(ulnCTypeID aCTYPE);

acp_sint32_t ulnTypeGetDisplaySize(ulnMTypeID aMTYPE, ulnMeta *aMeta);

acp_sint16_t  ulnTypeGetInfoSearchable(ulnMTypeID aMTYPE);
acp_char_t   *ulnTypeGetInfoLiteralPrefix(ulnMTypeID aMTYPE);
acp_char_t   *ulnTypeGetInfoLiteralSuffix(ulnMTypeID aMTYPE);
acp_char_t   *ulnTypeGetInfoName(ulnMTypeID aMTYPE);

acp_sint32_t ulnTypeGetSizeOfFixedType(ulnMTypeID aMTYPE);
acp_bool_t   ulnTypeIsFixedMType(ulnMTypeID aMTYPE);

/* PROJ-2160 CM 타입제거
   서버타입인 MT 데이타의 최대 사이즈를 리턴한다. */
/* BUG-35016 */
acp_uint32_t ulnTypeGetMaxMtSize(ulnDbc *aDbc, ulnMeta *aMeta);

#endif /* _O_ULN_TYPES_H_ */
