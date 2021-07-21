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

#include <uln.h>
#include <ulnPrivate.h>
#include <ulnTypes.h>

/*
 * ULN_MTYPE 과 MTD_TYPE_ID 그리고, SQL_TYPE 간에 서로 변환할 수 있는 테이블.
 *
 * 또한 각 타입의 column size 와 decimal digits 목록을 나열해 두었으며,
 *
 * 아울러서 사용자가 SQL_C_DEFAULT 로 바인드 하였을 때
 * 사용자가 바인드한 버퍼의 타입을 추정할 수 있는 ULN_CTYPE 의 목록도 가지고 있는 테이블.
 *
 * Note : SQL_TINYINT 는 columnsize 가 3 인데, Altibase 에서는 지원하지 않는다. 그러므로 어차피
 *        나오지도 않는다. 모두 MTD_SMALLINT_ID 로 변환된다.
 */
typedef struct
{
    ulnMTypeID        mMTYPE;
    acp_uint32_t      mMTD_TYPE;
    acp_sint16_t      mSQL_TYPE;
    acp_sint16_t      mSQL_C_DEFAULT;
    acp_uint32_t      mColumnSize;
    acp_uint16_t      mDecimalDigits;
    acp_sint16_t      mSearchable;
    const acp_char_t *mLiteralPrefix;
    const acp_char_t *mLiteralSuffix;
    const acp_char_t *mTypeName;
} ulnMtypeFamily;

#define mSCALm (ACP_UINT16_MAX - 1) // column size 를 구할 때 해당 컬럼의 scale 을 리턴해야 한다.
#define mPRECm (ACP_UINT16_MAX - 2) // column size 를 구할 때 해당 컬럼의 precision 을 줘야 한다.
#define mSIZEm (ACP_UINT16_MAX - 3) // column size 를 구할 때 해당 컬럼의 size 를 줘야 한다.
#define mNOTOT (ACP_UINT16_MAX - 4) // column size 를 알 수 없을 때 SQL_NO_TOTAL 을 리턴해야 한다.

/*
 * BUGBUG : mt 타입의 속성 중 searchable 을 아래 표에 다시 적어야 한다.
 * BUGBUG : 서버로 타입 메타를 조회해서 가져온 값으로 세팅해야 한다. v
 *          첫번째 descriptor 에 데이터타입에 대한 조회가 들어오면 서버에서 v$datatype 을
 *          조회해서 타입들에 대한 정보를 가져와서 표로 구성한 후 사용자에게 넘겨 줘야 한다.
 */
// fix BUG-20526
ulnMtypeFamily ulnTypeTableMTYPEs[ULN_MTYPE_MAX] =
{
/*    ULN_MTYPE               MTD_TYPE_ID          SQL_TYPE            SQL_C_DEFAULT   ColumnSize Decimal Searchable          literal literal
 *                                                                                                Digits                      prefix  suffix */

    { ULN_MTYPE_NULL,         MTD_NULL_ID,         SQL_UNKNOWN_TYPE,   SQL_C_CHAR,         0,      0,      SQL_PRED_NONE,       "", "",     ""          },

    { ULN_MTYPE_CHAR,         MTD_CHAR_ID,         SQL_CHAR,           SQL_C_CHAR,         mSIZEm, 0,      SQL_PRED_SEARCHABLE,      "\'", "\'", "CHAR"      },
    { ULN_MTYPE_VARCHAR,      MTD_VARCHAR_ID,      SQL_VARCHAR,        SQL_C_CHAR,         mSIZEm, 0,      SQL_PRED_SEARCHABLE, "\'", "\'", "VARCHAR"   },

    // BUGBUG : MTD_NUMBER_ID 는 쓰지 마라!
    { ULN_MTYPE_NUMBER,       MTD_NUMBER_ID,       SQL_NUMERIC,        SQL_C_NUMERIC,      mPRECm, 0,      SQL_PRED_BASIC,      "", "",     "NUMBER"    },
    { ULN_MTYPE_NUMERIC,      MTD_NUMERIC_ID,      SQL_NUMERIC,        SQL_C_NUMERIC,      mPRECm, mSCALm, SQL_PRED_BASIC,      "", "",     "NUMERIC"   },

    { ULN_MTYPE_BIT,          MTD_BIT_ID,          SQL_BIT,            SQL_C_BIT,          mSIZEm,      0,      SQL_PRED_SEARCHABLE,      "\'", "\'", "BIT"       },

    { ULN_MTYPE_SMALLINT,     MTD_SMALLINT_ID,     SQL_SMALLINT,       SQL_C_SSHORT,       5,      0,      SQL_PRED_BASIC,      "", "",     "SMALLINT"  },
    { ULN_MTYPE_INTEGER,      MTD_INTEGER_ID,      SQL_INTEGER,        SQL_C_SLONG,        10,     0,      SQL_PRED_BASIC,      "", "",     "INTEGER"   },
    { ULN_MTYPE_BIGINT,       MTD_BIGINT_ID,       SQL_BIGINT,         SQL_C_SBIGINT,      19,     0,      SQL_PRED_BASIC,      "", "",     "BIGINT"    },

    { ULN_MTYPE_REAL,         MTD_REAL_ID,         SQL_REAL,           SQL_C_FLOAT,        7,      0,      SQL_PRED_BASIC,      "", "",     "REAL"      },

    { ULN_MTYPE_FLOAT,        MTD_FLOAT_ID,        SQL_FLOAT,          SQL_C_DOUBLE,       38,     0,      SQL_PRED_BASIC,      "", "",     "FLOAT"     },
    { ULN_MTYPE_DOUBLE,       MTD_DOUBLE_ID,       SQL_DOUBLE,         SQL_C_DOUBLE,       15,     0,      SQL_PRED_BASIC,      "", "",     "DOUBLE"    },

    { ULN_MTYPE_BINARY,       MTD_BINARY_ID,       SQL_BINARY,         SQL_C_BINARY,       mSIZEm, 0,      SQL_PRED_BASIC,      "\'", "\'", "BINARY"    },
    { ULN_MTYPE_VARBIT,       MTD_VARBIT_ID,       SQL_VARBIT,         SQL_C_BINARY,       mSIZEm, 0,      SQL_PRED_SEARCHABLE,      "\'", "\'", "VARBIT"    },
    { ULN_MTYPE_NIBBLE,       MTD_NIBBLE_ID,       SQL_NIBBLE,         SQL_C_BINARY,       mSIZEm, 0,      SQL_PRED_SEARCHABLE,      "\'", "\'", "NIBBLE"    },
    { ULN_MTYPE_BYTE,         MTD_BYTE_ID,         SQL_BYTE,           SQL_C_BINARY,       mSIZEm, 0,      SQL_PRED_SEARCHABLE,      "\'", "\'", "BYTE"      },
    { ULN_MTYPE_VARBYTE,      MTD_VARBYTE_ID,      SQL_VARBYTE,        SQL_C_BINARY,       mSIZEm, 0,      SQL_PRED_SEARCHABLE,      "\'", "\'", "VARBYTE"   },

    // MTD_DATE_ID 는 SQL_TYPE 에서 말하는 타임스탬프와 가장 가까운 타입이다.
    // BUGBUG : column size
    { ULN_MTYPE_TIMESTAMP,    MTD_DATE_ID,         SQL_TYPE_TIMESTAMP, SQL_C_TIMESTAMP,    30,     mSCALm, SQL_PRED_BASIC,  "{ts\'", "\'}", "DATE"      },
    { ULN_MTYPE_DATE,         MTD_DATE_ID,         SQL_TYPE_TIMESTAMP, SQL_C_TIMESTAMP,    30,     mSCALm, SQL_PRED_BASIC,  "{ts\'", "\'}", "DATE"      },
    { ULN_MTYPE_TIME,         MTD_DATE_ID,         SQL_TYPE_TIMESTAMP, SQL_C_TIMESTAMP,    30,     mSCALm, SQL_PRED_BASIC,  "{ts\'", "\'}", "DATE"      },

    // MTD_INTERVAL_ID 는 도저히 concise type 으로 맞출 수 없어서 임시로 SQL_INTERVAL_DAY_TO_SECOND 로 했다.
    // BUGBUG : column size
    { ULN_MTYPE_INTERVAL, MTD_INTERVAL_ID, SQL_INTERVAL_DAY_TO_SECOND, SQL_C_DATE,         10,     mSCALm, SQL_PRED_BASIC,      "", "",     "INTERVAL"  },

    { ULN_MTYPE_BLOB,         MTD_BLOB_LOCATOR_ID, SQL_BLOB,           SQL_C_BINARY,       mNOTOT, 0,      SQL_PRED_NONE,       "\'", "\'", "BLOB"      },
    { ULN_MTYPE_CLOB,         MTD_CLOB_LOCATOR_ID, SQL_CLOB,           SQL_C_CHAR,         mNOTOT, 0,      SQL_PRED_NONE,       "\'", "\'", "CLOB"      },
    { ULN_MTYPE_BLOB_LOCATOR, MTD_BLOB_LOCATOR_ID, SQL_BLOB,           SQL_C_BLOB_LOCATOR, mNOTOT, 0,      SQL_PRED_NONE,       "\'", "\'", "BLOB"      },
    { ULN_MTYPE_CLOB_LOCATOR, MTD_CLOB_LOCATOR_ID, SQL_CLOB,           SQL_C_CLOB_LOCATOR, mNOTOT, 0,      SQL_PRED_NONE,       "\'", "\'", "CLOB"      },

    { ULN_MTYPE_GEOMETRY,     MTD_GEOMETRY_ID,     SQL_GEOMETRY,       SQL_C_BINARY,       mSIZEm, 0,      SQL_PRED_BASIC,      "", "",     "GEOMETRY"  },

    { ULN_MTYPE_NCHAR,        MTD_NCHAR_ID,        SQL_WCHAR,          SQL_C_WCHAR,        mSIZEm, 0,      SQL_PRED_SEARCHABLE, "\'", "\'", "NCHAR"     },
    { ULN_MTYPE_NVARCHAR,     MTD_NVARCHAR_ID,     SQL_WVARCHAR,       SQL_C_WCHAR,        mSIZEm, 0,      SQL_PRED_SEARCHABLE, "\'", "\'", "NVARCHAR"  }

};

ulnMTypeID ulnTypeMap_MTD_MTYPE(acp_uint32_t aMTD_TYPE)
{
    acp_uint32_t i;

    for(i = 0; i < ULN_MTYPE_MAX; i++)
    {
        /*
         * BUGBUG : 서버에서 넘어오는 정보가 col info 와 param info 가 다르다.
         *          통일해 달라고 요청하고 아래 두개의 if 문을 없애야 한다.
         */
        if(aMTD_TYPE == MTD_BLOB_ID) return ULN_MTYPE_BLOB;
        if(aMTD_TYPE == MTD_CLOB_ID) return ULN_MTYPE_CLOB;

        if(ulnTypeTableMTYPEs[i].mMTD_TYPE == aMTD_TYPE)
        {
            return ulnTypeTableMTYPEs[i].mMTYPE;
        }
    }

    acpFprintf(ACP_STD_ERR,
               "ulnTypeMap_MTD_MTYPE : unhandled mtd type. mtd type id = %d\n",
               aMTD_TYPE);
    acpStdFlush(ACP_STD_ERR);

    return ULN_MTYPE_MAX;
}

acp_uint32_t ulnTypeMap_MTYPE_MTD(ulnMTypeID aMTYPE)
{
    ACE_ASSERT(aMTYPE < ULN_MTYPE_MAX);

    return ulnTypeTableMTYPEs[aMTYPE].mMTD_TYPE;
}

/*
 * BUGBUG : odbc 스펙의 concise 와 verbose type 을 고려해야 한다.
 *          ulnTypeTableMTYPEs[].mSQL_TYPE 은 concise type 이다.
 */
acp_sint16_t ulnTypeMap_MTYPE_SQL(ulnMTypeID aMTYPE)
{
    ACE_ASSERT(aMTYPE < ULN_MTYPE_MAX);

    return ulnTypeTableMTYPEs[aMTYPE].mSQL_TYPE;
}

/*
 * ColumnSize, DecimalDigits 를 리턴한다. 이는 SQLDescribeCol() 함수를 호출했을 때에만 사용된다.
 */
acp_uint32_t ulnTypeGetColumnSizeOfType(ulnMTypeID aMTYPE, ulnMeta *aMeta)
{
    switch (ulnTypeTableMTYPEs[aMTYPE].mColumnSize)
    {
        case mPRECm: return ulnMetaGetPrecision(aMeta);
        case mSCALm: return ulnMetaGetScale(aMeta);
        case mSIZEm: return (acp_uint32_t)ulnMetaGetOdbcLength(aMeta);

        /*
         * msdn SQLDescribeCol() 의 설명에 의하면, 길이를 모를 때에는 0 을 리턴해야 한다고 한다.
         * 이는 SQLColAttribute() 의 display size 와 완전히 다른 것이다.
         *
         * BUGBUG : LOB 타입일 때 이렇게 SQL_NO_TOTAL 을 주어도 문제가 없는지 검증 필요하다.
         */

        // fix BUG-18987
        // fix BUG-19411
        case mNOTOT: return (acp_uint32_t)SQL_NO_TOTAL;
        default:     return ulnTypeTableMTYPEs[aMTYPE].mColumnSize;
    }
}

acp_sint16_t ulnTypeGetDecimalDigitsOfType(ulnMTypeID aMTYPE, ulnMeta *aMeta)
{
    switch (ulnTypeTableMTYPEs[aMTYPE].mDecimalDigits)
    {
        case mPRECm: return ulnMetaGetPrecision(aMeta);
        case mSCALm: return ulnMetaGetScale(aMeta);
        case mSIZEm: return ulnMetaGetOdbcLength(aMeta);
        case mNOTOT: return SQL_NO_TOTAL;
        default:     return ulnTypeTableMTYPEs[aMTYPE].mDecimalDigits;
    }
}

acp_sint16_t ulnTypeGetInfoSearchable(ulnMTypeID aMTYPE)
{
    ACE_ASSERT(aMTYPE < ULN_MTYPE_MAX);

    return ulnTypeTableMTYPEs[aMTYPE].mSearchable;
}

acp_char_t *ulnTypeGetInfoLiteralPrefix(ulnMTypeID aMTYPE)
{
    ACE_ASSERT(aMTYPE < ULN_MTYPE_MAX);

    return (acp_char_t *)ulnTypeTableMTYPEs[aMTYPE].mLiteralPrefix;
}

acp_char_t *ulnTypeGetInfoLiteralSuffix(ulnMTypeID aMTYPE)
{
    ACE_ASSERT(aMTYPE < ULN_MTYPE_MAX);

    return (acp_char_t *)ulnTypeTableMTYPEs[aMTYPE].mLiteralSuffix;
}

acp_char_t *ulnTypeGetInfoName(ulnMTypeID aMTYPE)
{
    ACE_ASSERT(aMTYPE < ULN_MTYPE_MAX);

    return (acp_char_t *)ulnTypeTableMTYPEs[aMTYPE].mTypeName;
}

/*
 * Display size 는 타입을 화면에 출력하기 위해 필요한 문자의 갯수를 일컫는다.
 */
acp_sint32_t ulnTypeGetDisplaySize(ulnMTypeID aMTYPE, ulnMeta *aMeta)
{
    switch (aMTYPE)
    {
        case ULN_MTYPE_NULL:
            return 0;

        case ULN_MTYPE_CHAR:
        case ULN_MTYPE_VARCHAR:
            return ulnMetaGetOdbcLength(aMeta);

        case ULN_MTYPE_VARBIT:
        case ULN_MTYPE_NIBBLE:
            return ulnMetaGetOdbcLength(aMeta);

        case ULN_MTYPE_FLOAT:
            // 유효 범위가 -1E+120 ~ 1E+120이므로
            // 부호와 소수점, 그리고 소수점앞의 0을 고려하면 123개 문자로 표현될 수 있다. (1 + 120 + 1 + 1)
            return 123;
        case ULN_MTYPE_NUMBER:
        case ULN_MTYPE_NUMERIC:
            // precision이 38, scale이 128까지 가능하므로
            // 부호와 소수점, 그리고 소수점앞의 0을 고려하면 최대 길이는 169다. (38 + 128 + 1 + 1 + 1)
            // (precision은 소수점 이하를 포함하므로, 실제로는 이보다 작겠지만 그냥 넉넉하게 잡는다.)
            return 169;

        case ULN_MTYPE_BIT:
            return 1;
        case ULN_MTYPE_SMALLINT:
            return 6;
        case ULN_MTYPE_INTEGER:
            return 11;
        case ULN_MTYPE_BIGINT:
            return 20;
        case ULN_MTYPE_REAL:
            return 14;
        case ULN_MTYPE_DOUBLE:
            return 24;

        case ULN_MTYPE_BINARY:
        case ULN_MTYPE_BYTE:
        case ULN_MTYPE_VARBYTE:
            return ulnMetaGetOdbcLength(aMeta) * 2;

        case ULN_MTYPE_DATE:
        case ULN_MTYPE_TIME:
        case ULN_MTYPE_TIMESTAMP:
            return 30;  // yyyy-mon-dd hh:mm:ss 123000000

        case ULN_MTYPE_INTERVAL:
            return 40;  // BUGBUG : 계산하기 힘들어서 그냥 40으로 함 -_-;;

        case ULN_MTYPE_NCHAR:
        case ULN_MTYPE_NVARCHAR:
            return ulnMetaGetOctetLength(aMeta);
        case ULN_MTYPE_BLOB:
        case ULN_MTYPE_CLOB:
        case ULN_MTYPE_GEOMETRY:
        case ULN_MTYPE_MAX:
            /*
             * 아래의 두 타입은 오로지 사용자가 lob locator 로 바인드 했을 경우의
             * out binding 을 위해서만 존재한다.
             */
        case ULN_MTYPE_CLOB_LOCATOR:
        case ULN_MTYPE_BLOB_LOCATOR:
            return SQL_NO_TOTAL;
    }

    return SQL_NO_TOTAL;
}

/*
 * SQL_C_DEFAULT 로 바인딩시 어떤 타입을 가정해야 하는지 결정하는 함수.
 */
acp_sint16_t ulnTypeGetDefault_SQL_C_TYPE(ulnMTypeID aMTYPE)
{
    return ulnTypeTableMTYPEs[aMTYPE].mSQL_C_DEFAULT;
}

/*
 * 이 함수는 단 한곳, out param 을 위한 임시 버퍼를 할당할 때에만 쓰인다.
 */
acp_sint32_t ulnTypeGetSizeOfFixedType(ulnMTypeID aMTYPE)
{
    switch (aMTYPE)
    {
        case ULN_MTYPE_NULL:        return 8;   // just for the hell of it
        case ULN_MTYPE_FLOAT:
        case ULN_MTYPE_NUMBER:
        case ULN_MTYPE_NUMERIC:     return ACI_SIZEOF(cmtNumeric);
        case ULN_MTYPE_SMALLINT:    return ACI_SIZEOF(acp_sint16_t);
        case ULN_MTYPE_INTEGER:     return ACI_SIZEOF(acp_sint32_t);
        case ULN_MTYPE_BIGINT:      return ACI_SIZEOF(acp_sint64_t);
        case ULN_MTYPE_REAL:        return ACI_SIZEOF(acp_float_t);
        case ULN_MTYPE_DOUBLE:      return ACI_SIZEOF(acp_double_t);
        case ULN_MTYPE_DATE:
        case ULN_MTYPE_TIME:
        case ULN_MTYPE_TIMESTAMP:   return ACI_SIZEOF(mtdDateType);
        case ULN_MTYPE_INTERVAL:    return ACI_SIZEOF(cmtInterval);
        case ULN_MTYPE_BLOB_LOCATOR:
        case ULN_MTYPE_CLOB_LOCATOR:return ACI_SIZEOF(acp_uint64_t);

        case ULN_MTYPE_CHAR:
        case ULN_MTYPE_VARCHAR:
        case ULN_MTYPE_NCHAR:
        case ULN_MTYPE_NVARCHAR:
        case ULN_MTYPE_BIT:
        case ULN_MTYPE_VARBIT:
        case ULN_MTYPE_NIBBLE:
        case ULN_MTYPE_BYTE:
        case ULN_MTYPE_VARBYTE:
        case ULN_MTYPE_BINARY:
        case ULN_MTYPE_GEOMETRY:
        case ULN_MTYPE_BLOB:
        case ULN_MTYPE_CLOB:
        case ULN_MTYPE_MAX:
        default:
            ACE_ASSERT(0);
            return 0;
    }
}

acp_bool_t ulnTypeIsFixedMType(ulnMTypeID aMTYPE)
{
    switch (aMTYPE)
    {
        case ULN_MTYPE_NULL:
        case ULN_MTYPE_NUMBER:
        case ULN_MTYPE_NUMERIC:
        case ULN_MTYPE_SMALLINT:
        case ULN_MTYPE_INTEGER:
        case ULN_MTYPE_BIGINT:
        case ULN_MTYPE_REAL:
        case ULN_MTYPE_FLOAT:
        case ULN_MTYPE_DOUBLE:
        case ULN_MTYPE_DATE:
        case ULN_MTYPE_TIME:
        case ULN_MTYPE_TIMESTAMP:
        case ULN_MTYPE_INTERVAL:
        case ULN_MTYPE_BLOB_LOCATOR:
        case ULN_MTYPE_CLOB_LOCATOR:
            return ACP_TRUE;

        case ULN_MTYPE_CHAR:
        case ULN_MTYPE_VARCHAR:
        case ULN_MTYPE_NCHAR:
        case ULN_MTYPE_NVARCHAR:
        case ULN_MTYPE_BIT:
        case ULN_MTYPE_VARBIT:
        case ULN_MTYPE_NIBBLE:
        case ULN_MTYPE_BYTE:
        case ULN_MTYPE_VARBYTE:
        case ULN_MTYPE_BINARY:
        case ULN_MTYPE_GEOMETRY:
        case ULN_MTYPE_BLOB:
        case ULN_MTYPE_CLOB:
            return ACP_FALSE;
        default:
        case ULN_MTYPE_MAX:
            ACE_ASSERT(0);
            return ACP_FALSE;
    }
}

/*
 * ====================================================
 * SQL_C_TYPE 으로부터 ULN_CTYPE 을 얻는 함수
 * SQL_TYPE 으로부터 ULN_MTYPE 을 얻는 함수.
 *
 * 즉, 외부의 타입에서 내부의 타입으로 매핑하는 함수.
 * ====================================================
 */

/*
 * BUGBUG
 *      SQL_WLONGVARCHAR:
 * 상기 타입에 대한 경우를 처리하고 있지 않다.
 *
 * BUGBUG: ODBC 3.0 에는 있는데, unix odbc 헤더파일에는 없다.
 *      SQL_TYPE_UTCDATETIME:
 *      SQL_TYPE_UTCTIME:
 */

ulnMTypeID ulnTypeMap_SQL_MTYPE(acp_sint16_t aSQL_TYPE)
{
    switch (aSQL_TYPE)
    {
        case SQL_CHAR:                      return ULN_MTYPE_CHAR;
        case SQL_VARCHAR:                   return ULN_MTYPE_VARCHAR;
        case SQL_WCHAR:                     return ULN_MTYPE_NCHAR;
        case SQL_WVARCHAR:                  return ULN_MTYPE_NVARCHAR;
        case SQL_DECIMAL:                   return ULN_MTYPE_NUMERIC;
        case SQL_NUMERIC:                   return ULN_MTYPE_NUMERIC;
        case SQL_SMALLINT:                  return ULN_MTYPE_SMALLINT;
        case SQL_INTEGER:                   return ULN_MTYPE_INTEGER;
        case SQL_REAL:                      return ULN_MTYPE_REAL;
        case SQL_FLOAT:                     return ULN_MTYPE_FLOAT;
        case SQL_DOUBLE:                    return ULN_MTYPE_DOUBLE;
        case SQL_BIT:                       return ULN_MTYPE_BIT;
        case SQL_VARBIT:                    return ULN_MTYPE_VARBIT;
        case SQL_TINYINT:                   return ULN_MTYPE_SMALLINT;
        case SQL_BIGINT:                    return ULN_MTYPE_BIGINT;

        case SQL_BINARY:
        case SQL_VARBINARY:                 return ULN_MTYPE_BINARY;

        // verbose type
        case SQL_DATETIME:                  return ULN_MTYPE_TIMESTAMP;

        // concise types
        case SQL_TYPE_DATE:                 return ULN_MTYPE_DATE;
        case SQL_TYPE_TIME:                 return ULN_MTYPE_TIME;
        case SQL_TYPE_TIMESTAMP:

        // ODBC 2.x
        // case SQL_DATE:   == SQL_DATETIME
        // case SQL_TIME:   // BUGBUG : == SQL_INTERVAL
        case SQL_TIMESTAMP:                 return ULN_MTYPE_TIMESTAMP;

        // verbose type
        case SQL_INTERVAL:

        // concise types
        case SQL_INTERVAL_MONTH:
        case SQL_INTERVAL_YEAR:
        case SQL_INTERVAL_YEAR_TO_MONTH:
        case SQL_INTERVAL_DAY:
        case SQL_INTERVAL_HOUR:
        case SQL_INTERVAL_MINUTE:
        case SQL_INTERVAL_SECOND:
        case SQL_INTERVAL_DAY_TO_HOUR:
        case SQL_INTERVAL_DAY_TO_MINUTE:
        case SQL_INTERVAL_DAY_TO_SECOND:
        case SQL_INTERVAL_HOUR_TO_MINUTE:
        case SQL_INTERVAL_HOUR_TO_SECOND:
        case SQL_INTERVAL_MINUTE_TO_SECOND: return ULN_MTYPE_INTERVAL;

        case SQL_BYTE:                      return ULN_MTYPE_BYTE;
        case SQL_VARBYTE:                   return ULN_MTYPE_VARBYTE;
        case SQL_NIBBLE:                    return ULN_MTYPE_NIBBLE;

        // BUG-21570 SQL_LONGVARBINARY 는 의미적으로 BLOB이다.
        case SQL_LONGVARBINARY:
        case SQL_BLOB:
        case SQL_BLOB_LOCATOR:              return ULN_MTYPE_BLOB;

        // BUG-21570 SQL_LONGVARCHAR 는 의미적으로 CLOB이다.
        case SQL_LONGVARCHAR:
        case SQL_CLOB:
        case SQL_CLOB_LOCATOR:              return ULN_MTYPE_CLOB;

        case SQL_GUID:

        default:                            return ULN_MTYPE_MAX;
    }
}

/*
 * Note : SQL_C_SHORT, SQL_C_LONG, and SQL_C_TINYINT have been replaced in ODBC
 *        by signed and unsigned types:
 *              SQL_C_SSHORT and SQL_C_USHORT,
 *              SQL_C_SLONG and SQL_C_ULONG,
 *              SQL_C_STINYINT and SQL_C_UTINYINT.
 *        An ODBC 3.x driver that should work with ODBC 2.x apps S/H/O/U/L/D/ S/U/P/P/O/R/T/
 *              SQL_C_SHORT, SQL_C_LONG, SQL_C_TINYINT,
 *        because when they are called, the Driver Manager passes them through to the driver.
 */

acp_sint16_t ulnTypeMap_CTYPE_SQLC(ulnCTypeID aCTYPE)
{
    switch (aCTYPE)
    {
        case ULN_CTYPE_NULL:            return 0;

        case ULN_CTYPE_DEFAULT:         return SQL_C_DEFAULT;

        case ULN_CTYPE_CHAR:            return SQL_C_CHAR;
        case ULN_CTYPE_WCHAR:           return SQL_C_WCHAR;

        case ULN_CTYPE_BINARY:          return SQL_C_BINARY;

                                        /* BUGBUG : SQL_C_SHORT 는 어떡할꺼나 */
        case ULN_CTYPE_SSHORT:          return SQL_C_SSHORT;
        case ULN_CTYPE_USHORT:          return SQL_C_USHORT;

        case ULN_CTYPE_SLONG:           return SQL_C_SLONG;
        case ULN_CTYPE_ULONG:           return SQL_C_ULONG;

        case ULN_CTYPE_FLOAT:           return SQL_C_FLOAT;
        case ULN_CTYPE_DOUBLE:          return SQL_C_DOUBLE;

                                        // BUGBUG : SQL_C_TINYINT 도 있는데..
        case ULN_CTYPE_STINYINT:        return SQL_C_STINYINT;
        case ULN_CTYPE_UTINYINT:        return SQL_C_UTINYINT;

        case ULN_CTYPE_SBIGINT:         return SQL_C_SBIGINT;
        case ULN_CTYPE_UBIGINT:         return SQL_C_UBIGINT;

        case ULN_CTYPE_DATE:
        case ULN_CTYPE_TIME:
        case ULN_CTYPE_TIMESTAMP:       return SQL_C_TYPE_TIMESTAMP;
        case ULN_CTYPE_INTERVAL:        return SQL_INTERVAL;

        case ULN_CTYPE_NUMERIC:         return SQL_C_NUMERIC;

        case ULN_CTYPE_BLOB_LOCATOR:    return SQL_C_BLOB_LOCATOR;
        case ULN_CTYPE_CLOB_LOCATOR:    return SQL_C_CLOB_LOCATOR;
        case ULN_CTYPE_FILE:            return SQL_C_FILE;

        case ULN_CTYPE_BIT:             return SQL_C_BIT;

        case ULN_CTYPE_MAX:
        default:
            return 0;
    }
}

ulnCTypeID ulnTypeMap_SQLC_CTYPE(acp_sint16_t aSQL_C_TYPE)
{
    switch (aSQL_C_TYPE)
    {
        case SQL_C_DEFAULT:        return ULN_CTYPE_DEFAULT;

        case SQL_C_CHAR:           return ULN_CTYPE_CHAR;

        case SQL_C_WCHAR:          return ULN_CTYPE_WCHAR;

        case SQL_C_SHORT:
        case SQL_C_SSHORT:         return ULN_CTYPE_SSHORT;
        case SQL_C_USHORT:         return ULN_CTYPE_USHORT;

        case SQL_C_LONG:
        case SQL_C_SLONG:          return ULN_CTYPE_SLONG;
        case SQL_C_ULONG:          return ULN_CTYPE_ULONG;

        case SQL_C_TINYINT:
        case SQL_C_STINYINT:       return ULN_CTYPE_STINYINT;
        case SQL_C_UTINYINT:       return ULN_CTYPE_UTINYINT;

        case SQL_C_SBIGINT:        return ULN_CTYPE_SBIGINT;
        case SQL_C_UBIGINT:        return ULN_CTYPE_UBIGINT;

        case SQL_C_FLOAT:          return ULN_CTYPE_FLOAT;
        case SQL_C_DOUBLE:         return ULN_CTYPE_DOUBLE;
        case SQL_C_BIT:            return ULN_CTYPE_BIT;

        case SQL_C_BINARY:         return ULN_CTYPE_BINARY;
        case SQL_C_TYPE_DATE:      return ULN_CTYPE_DATE;
        case SQL_C_TYPE_TIME:      return ULN_CTYPE_TIME;
        case SQL_C_TYPE_TIMESTAMP: return ULN_CTYPE_TIMESTAMP;
        case SQL_C_NUMERIC:        return ULN_CTYPE_NUMERIC;

        case SQL_C_BLOB_LOCATOR:   return ULN_CTYPE_BLOB_LOCATOR;
        case SQL_C_CLOB_LOCATOR:   return ULN_CTYPE_CLOB_LOCATOR;

        case SQL_C_FILE:           return ULN_CTYPE_FILE;

        case SQL_C_INTERVAL_YEAR:
        case SQL_C_INTERVAL_MONTH:
        case SQL_C_INTERVAL_DAY:
        case SQL_C_INTERVAL_HOUR:
        case SQL_C_INTERVAL_MINUTE:
        case SQL_C_INTERVAL_SECOND:
        case SQL_C_INTERVAL_YEAR_TO_MONTH:
        case SQL_C_INTERVAL_DAY_TO_HOUR:
        case SQL_C_INTERVAL_DAY_TO_MINUTE:
        case SQL_C_INTERVAL_DAY_TO_SECOND:
        case SQL_C_INTERVAL_HOUR_TO_MINUTE:
        case SQL_C_INTERVAL_HOUR_TO_SECOND:
        case SQL_C_INTERVAL_MINUTE_TO_SECOND:   return ULN_CTYPE_INTERVAL;

        // in ODBC 2.x, the C date, time, and timestamp data types are
        // following three values.
        //
        // case SQL_C_VARBOOKMARK:  // SQL_C_BINARY 와 같은 값.
        // case SQL_C_BOOKMARK:     // SQL_C_UBIGINT 와 같은 값.
        case SQL_C_GUID:

        default:                                return ULN_CTYPE_MAX;
    }
}

/*
 * BUGBUG : 현재는 사용하지 않지만, descriptor field 세팅하는 부분을 손질할 때 쓰일것 같다.
 */

acp_sint16_t ulnTypeGetOdbcDatetimeIntCode(acp_sint16_t aType)
{
    switch (aType)
    {
        case SQL_TYPE_DATE:                 return SQL_CODE_DATE;
        case SQL_TYPE_TIME:                 return SQL_CODE_TIME;
        case SQL_TYPE_TIMESTAMP:            return SQL_CODE_TIMESTAMP;
        case SQL_INTERVAL_MONTH:            return SQL_CODE_MONTH;
        case SQL_INTERVAL_YEAR:             return SQL_CODE_YEAR;
        case SQL_INTERVAL_YEAR_TO_MONTH:    return SQL_CODE_YEAR_TO_MONTH;
        case SQL_INTERVAL_DAY:              return SQL_CODE_DAY;
        case SQL_INTERVAL_HOUR:             return SQL_CODE_HOUR;
        case SQL_INTERVAL_MINUTE:           return SQL_CODE_MINUTE;
        case SQL_INTERVAL_SECOND:           return SQL_CODE_SECOND;
        case SQL_INTERVAL_DAY_TO_HOUR:      return SQL_CODE_DAY_TO_HOUR;
        case SQL_INTERVAL_DAY_TO_MINUTE:    return SQL_CODE_DAY_TO_MINUTE;
        case SQL_INTERVAL_DAY_TO_SECOND:    return SQL_CODE_DAY_TO_SECOND;
        case SQL_INTERVAL_HOUR_TO_MINUTE:   return SQL_CODE_HOUR_TO_MINUTE;
        case SQL_INTERVAL_HOUR_TO_SECOND:   return SQL_CODE_HOUR_TO_SECOND;
        case SQL_INTERVAL_MINUTE_TO_SECOND: return SQL_CODE_MINUTE_TO_SECOND;

        /*
         * Note: ODBC3.0 SQLSetDescField() 함수 설명 :
         *       SQL_DESC_CONCISE_TYPE 이 datetime 이거나 interval 이 아니면 SQL_DESC_TYPE 필드는
         *       동일한 값으로 세팅하고, SQL_DESC_DATETIME_INTERVAL_CODE 는 0 으로 세팅한다.
         */
        default:                            return 0;
    }
}

acp_sint16_t ulnTypeGetOdbcVerboseType(acp_sint16_t aType)
{
    switch (aType)
    {
        /*
         * Concise SQL types and C types : 상수값이 동일하다.
         */
        case SQL_TYPE_DATE:                     // SQL_C_TYPE_DATE
        case SQL_TYPE_TIME:                     // SQL_C_TYPE_TIME
        case SQL_TYPE_TIMESTAMP:                // SQL_C_TYPE_TIMESTAMP
            return SQL_DATETIME;

        case SQL_INTERVAL_MONTH:                // SQL_C_INTERVAL_MONTH
        case SQL_INTERVAL_YEAR:                 // SQL_C_INTERVAL_YEAR
        case SQL_INTERVAL_YEAR_TO_MONTH:        // SQL_C_INTERVAL_YEAR_TO_MONTH
        case SQL_INTERVAL_DAY:                  // SQL_C_INTERVAL_DAY
        case SQL_INTERVAL_HOUR:                 // SQL_C_INTERVAL_HOUR
        case SQL_INTERVAL_MINUTE:               // SQL_C_INTERVAL_MINUTE
        case SQL_INTERVAL_SECOND:               // SQL_C_INTERVAL_SECOND
        case SQL_INTERVAL_DAY_TO_HOUR:          // SQL_C_INTERVAL_DAY_TO_HOUR
        case SQL_INTERVAL_DAY_TO_MINUTE:        // SQL_C_INTERVAL_DAY_TO_MINUTE
        case SQL_INTERVAL_DAY_TO_SECOND:        // SQL_C_INTERVAL_DAY_TO_SECOND
        case SQL_INTERVAL_HOUR_TO_MINUTE:       // SQL_C_INTERVAL_HOUR_TO_MINUTE
        case SQL_INTERVAL_HOUR_TO_SECOND:       // SQL_C_INTERVAL_HOUR_TO_SECOND
        case SQL_INTERVAL_MINUTE_TO_SECOND:     // SQL_C_INTERVAL_MINUTE_TO_SECOND
            return SQL_INTERVAL;

        default:
            return aType;
    }
}

/*
 * 타입의 특성을 알아보는 함수
 */
acp_bool_t ulnTypeIsOdbcConciseType(acp_sint16_t aType)
{
    switch (aType)
    {
        /*
         * Concise SQL types and C types : 상수값이 동일하다.
         */
        case SQL_TYPE_DATE:                     // SQL_C_TYPE_DATE
        case SQL_TYPE_TIME:                     // SQL_C_TYPE_TIME
        case SQL_TYPE_TIMESTAMP:                // SQL_C_TYPE_TIMESTAMP
        case SQL_INTERVAL_MONTH:                // SQL_C_INTERVAL_MONTH
        case SQL_INTERVAL_YEAR:                 // SQL_C_INTERVAL_YEAR
        case SQL_INTERVAL_YEAR_TO_MONTH:        // SQL_C_INTERVAL_YEAR_TO_MONTH
        case SQL_INTERVAL_DAY:                  // SQL_C_INTERVAL_DAY
        case SQL_INTERVAL_HOUR:                 // SQL_C_INTERVAL_HOUR
        case SQL_INTERVAL_MINUTE:               // SQL_C_INTERVAL_MINUTE
        case SQL_INTERVAL_SECOND:               // SQL_C_INTERVAL_SECOND
        case SQL_INTERVAL_DAY_TO_HOUR:          // SQL_C_INTERVAL_DAY_TO_HOUR
        case SQL_INTERVAL_DAY_TO_MINUTE:        // SQL_C_INTERVAL_DAY_TO_MINUTE
        case SQL_INTERVAL_DAY_TO_SECOND:        // SQL_C_INTERVAL_DAY_TO_SECOND
        case SQL_INTERVAL_HOUR_TO_MINUTE:       // SQL_C_INTERVAL_HOUR_TO_MINUTE
        case SQL_INTERVAL_HOUR_TO_SECOND:       // SQL_C_INTERVAL_HOUR_TO_SECOND
        case SQL_INTERVAL_MINUTE_TO_SECOND:     // SQL_C_INTERVAL_MINUTE_TO_SECOND
            return ACP_TRUE;

        default:
            return ACP_FALSE;
    }
}

acp_bool_t ulnTypeIsVariableLength(ulnCTypeID aCTYPE)
{
    /*
     * 이 함수는 SQLGetData() 시에만 호출되는 함수이다.
     */

    switch (aCTYPE)
    {
        case ULN_CTYPE_CHAR:
        case ULN_CTYPE_BINARY:
            return ACP_TRUE;

        default:
            return ACP_FALSE;
    }
}

acp_sint16_t ulnTypeMap_LOB_SQLTYPE(acp_sint16_t aSQLTYPE, acp_bool_t aLongDataCompat)
{
    /*
     * BUG-16253 에서 설명한 바와 같이
     * SQL_ATTR_LONGDATA_COMPAT 속성에 따라서 응용 프로그램이
     * 표준 타입으로 LOB 을 select 할 수 있도록 해 준다.
     */

    if (aLongDataCompat == ACP_TRUE)
    {
        if (aSQLTYPE == SQL_BLOB)
        {
            return SQL_LONGVARBINARY;
        }
        else if (aSQLTYPE == SQL_CLOB)
        {
            return SQL_LONGVARCHAR;
        }
    }

    return aSQLTYPE;
}

acp_uint32_t ulnTypeGetMaxMtSize(ulnDbc *aDbc, ulnMeta *aMeta)
{
    /* BUG-35016 */
    mtlModule *aServerCharSet = NULL;
    mtlModule *aClientCharSet = NULL;

    if (aDbc != NULL)
    {
        aServerCharSet = aDbc->mCharsetLangModule;
        aClientCharSet = aDbc->mClientCharsetLangModule;
    }
    else
    {
        /* Nothing */
    }

    switch (aMeta->mMTYPE)
    {
        case ULN_MTYPE_SMALLINT:
            return 2;
        case ULN_MTYPE_INTEGER:
            return 4;
        case ULN_MTYPE_BIGINT:
            return 8;
        case ULN_MTYPE_REAL:
            return 4;
        case ULN_MTYPE_DOUBLE:
            return 8;
        case ULN_MTYPE_NUMBER:
        case ULN_MTYPE_NUMERIC:
        case ULN_MTYPE_FLOAT:
            return 21;
        case ULN_MTYPE_DATE:
        case ULN_MTYPE_TIME:
        case ULN_MTYPE_TIMESTAMP:
            return 8;
        case ULN_MTYPE_INTERVAL:
            return 16;
        case ULN_MTYPE_BLOB:
        case ULN_MTYPE_CLOB:
        case ULN_MTYPE_BLOB_LOCATOR:
        case ULN_MTYPE_CLOB_LOCATOR:
            return 8;
        case ULN_MTYPE_CHAR:
        case ULN_MTYPE_VARCHAR:
            /*
             * BUG-35016
             *
             * The size of buffer using sending parameters is considered
             * to convert into the server's charset.
             */
            if (aServerCharSet != aClientCharSet)
            {
                return ulnMetaGetOdbcLength(aMeta) * aServerCharSet->maxPrecision(1) + 2;
            }
            else
            {
                return ulnMetaGetOdbcLength(aMeta) + 2;
            }
        case ULN_MTYPE_NCHAR:
        case ULN_MTYPE_NVARCHAR:
            return (ulnMetaGetOdbcLength(aMeta) * ULN_MAX_CONVERSION_RATIO) + 2;
        case ULN_MTYPE_BIT:
        case ULN_MTYPE_VARBIT:
            return (4 + BIT_TO_BYTE(ulnMetaGetOdbcLength(aMeta)));
        case ULN_MTYPE_NIBBLE:
            return (1 + (ulnMetaGetOdbcLength(aMeta) + 1) / 2);
        case ULN_MTYPE_BYTE:
        case ULN_MTYPE_VARBYTE:
            return ulnMetaGetOdbcLength(aMeta) + 2;
        case ULN_MTYPE_BINARY:
            return ulnMetaGetOdbcLength(aMeta) + 8;
        case ULN_MTYPE_NULL:
        case ULN_MTYPE_GEOMETRY:
        case ULN_MTYPE_MAX:
        default:
            ACE_ASSERT(0);
            return 0;
    }
}
