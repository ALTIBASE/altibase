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

/**
 * 개별 바인드 정보가 바꼈을때만 바인드하게 하려면 USE_CHECK_EACH_PARAM_CHANGED,
 * altibase_get_autocommit()을 사용하려면 SUPPORT_GET_AUTOCOMMIT,
 * 디버그용 로그 기능을 사용하려면 USE_CDBCLOG를 define 한다.
 */

#ifndef CDBC_DEF_H
#define CDBC_DEF_H 1

ACP_EXTERN_C_BEGIN



#define CDBC_INLINE                 ACP_INLINE
#define CDBC_INTERNAL               
#define CDBC_EXPORT                 

#define CDBC_NULLTERM_SIZE          2       /**< NULL-term의 최대 크기. UTF16 고려 */

#define CDBC_DEFAULT_ARRAY_SIZE     1000    /**< store 할 때 사용할 array size */

#define CDBC_CLI_SUCCEEDED(aRC)     SQL_SUCCEEDED(aRC)
#define CDBC_CLI_NOT_SUCCEEDED(aRC) (! CDBC_CLI_SUCCEEDED(aRC))

/* DEBUG로 빌드 할 때는 항상 USE_CDBCLOG를 켠다 */
#if defined(DEBUG) && !defined(USE_CDBCLOG)
    #define USE_CDBCLOG             ACP_TRUE
#endif

CDBC_INLINE
acp_char_t * altibase_rc_string (ALTIBASE_RC aRC)
{
    #define CASE_RETURN_STR(ID) case ID : return # ID

    switch (aRC)
    {
        CASE_RETURN_STR( ALTIBASE_FAILOVER_SUCCESS );
        CASE_RETURN_STR( ALTIBASE_SUCCESS );
        CASE_RETURN_STR( ALTIBASE_SUCCESS_WITH_INFO );
        CASE_RETURN_STR( ALTIBASE_NO_DATA );
        CASE_RETURN_STR( ALTIBASE_ERROR );
        CASE_RETURN_STR( ALTIBASE_INVALID_HANDLE );

        default: return "ALTIBASE_UNKNOWN_RC";
    }

    #undef CASE_RETURN_STR
}

CDBC_INLINE
acp_char_t * altibase_bind_type_string (ALTIBASE_BIND_TYPE aBindType)
{
    #define CASE_RETURN_STR(ID) case ID : return # ID

    switch (aBindType)
    {
        CASE_RETURN_STR( ALTIBASE_BIND_NULL     );
        CASE_RETURN_STR( ALTIBASE_BIND_BINARY   );
        CASE_RETURN_STR( ALTIBASE_BIND_STRING   );
        CASE_RETURN_STR( ALTIBASE_BIND_WSTRING  );
        CASE_RETURN_STR( ALTIBASE_BIND_SMALLINT );
        CASE_RETURN_STR( ALTIBASE_BIND_INTEGER  );
        CASE_RETURN_STR( ALTIBASE_BIND_BIGINT   );
        CASE_RETURN_STR( ALTIBASE_BIND_REAL     );
        CASE_RETURN_STR( ALTIBASE_BIND_DOUBLE   );
        CASE_RETURN_STR( ALTIBASE_BIND_NUMERIC  );
        CASE_RETURN_STR( ALTIBASE_BIND_DATE     );

        default: return "ALTIBASE_BIND_TYPE_UNKNOWN";
    }

    #undef CASE_RETURN_STR
}

CDBC_INLINE
acp_char_t * cli_sql_type_string (SQLSMALLINT aSqlType)
{
    #define CASE_RETURN_STR(ID) case ID : return # ID

    switch (aSqlType)
    {
        CASE_RETURN_STR( SQL_CHAR );
        CASE_RETURN_STR( SQL_VARCHAR );
        CASE_RETURN_STR( SQL_WCHAR );
        CASE_RETURN_STR( SQL_WVARCHAR );
        CASE_RETURN_STR( SQL_DECIMAL );
        CASE_RETURN_STR( SQL_NUMERIC );
        CASE_RETURN_STR( SQL_SMALLINT );
        CASE_RETURN_STR( SQL_INTEGER );
        CASE_RETURN_STR( SQL_REAL );
        CASE_RETURN_STR( SQL_FLOAT );
        CASE_RETURN_STR( SQL_DOUBLE );
        CASE_RETURN_STR( SQL_BIT );
        CASE_RETURN_STR( SQL_VARBIT );
        CASE_RETURN_STR( SQL_TINYINT );
        CASE_RETURN_STR( SQL_BIGINT );
        CASE_RETURN_STR( SQL_BINARY );
        CASE_RETURN_STR( SQL_VARBINARY );
        CASE_RETURN_STR( SQL_DATE );
        CASE_RETURN_STR( SQL_TIME );
        CASE_RETURN_STR( SQL_TIMESTAMP );
        CASE_RETURN_STR( SQL_INTERVAL_MONTH );
        CASE_RETURN_STR( SQL_INTERVAL_YEAR );
        CASE_RETURN_STR( SQL_INTERVAL_YEAR_TO_MONTH );
        CASE_RETURN_STR( SQL_INTERVAL_DAY );
        CASE_RETURN_STR( SQL_INTERVAL_HOUR );
        CASE_RETURN_STR( SQL_INTERVAL_MINUTE );
        CASE_RETURN_STR( SQL_INTERVAL_SECOND );
        CASE_RETURN_STR( SQL_INTERVAL_DAY_TO_HOUR );
        CASE_RETURN_STR( SQL_INTERVAL_DAY_TO_MINUTE );
        CASE_RETURN_STR( SQL_INTERVAL_DAY_TO_SECOND );
        CASE_RETURN_STR( SQL_INTERVAL_HOUR_TO_MINUTE );
        CASE_RETURN_STR( SQL_INTERVAL_HOUR_TO_SECOND );
        CASE_RETURN_STR( SQL_INTERVAL_MINUTE_TO_SECOND );
        CASE_RETURN_STR( SQL_BYTE );
        CASE_RETURN_STR( SQL_VARBYTE );
        CASE_RETURN_STR( SQL_NIBBLE );
        CASE_RETURN_STR( SQL_LONGVARBINARY );
        CASE_RETURN_STR( SQL_BLOB );
        CASE_RETURN_STR( SQL_BLOB_LOCATOR );
        CASE_RETURN_STR( SQL_LONGVARCHAR );
        CASE_RETURN_STR( SQL_CLOB );
        CASE_RETURN_STR( SQL_CLOB_LOCATOR );
        CASE_RETURN_STR( SQL_GUID );

        default: return "UNKNOWN";
    }

    #undef CASE_RETURN_STR
}

CDBC_INLINE
acp_char_t * cli_parameter_type_string (SQLSMALLINT aParamType)
{
    #define CASE_RETURN_STR(ID) case ID : return # ID

    switch (aParamType)
    {
		CASE_RETURN_STR( SQL_PARAM_TYPE_UNKNOWN );
		CASE_RETURN_STR( SQL_PARAM_INPUT );
		CASE_RETURN_STR( SQL_PARAM_INPUT_OUTPUT );
		CASE_RETURN_STR( SQL_RESULT_COL );
		CASE_RETURN_STR( SQL_PARAM_OUTPUT );
		CASE_RETURN_STR( SQL_RETURN_VALUE );

        default: return "UNKNOWN";
    }

    #undef CASE_RETURN_STR
}



ACP_EXTERN_C_END

#endif /* CDBC_DEF_H */

