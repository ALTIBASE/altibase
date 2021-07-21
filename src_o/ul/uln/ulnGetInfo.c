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
#include <ulnGetInfo.h>

/*
 * ULN_SFID_78
 * SQLGetInfo(), DBC, C2
 *
 *	    -- [1]
 *	    (08003) [2]
 *
 *	where
 *	    [1] The InfoType argument was SQL_ODBC_VER.
 *	    [2] The InfoType argument was not SQL_ODBC_VER.
 */
ACI_RC ulnSFID_78(ulnFnContext *aFnContext)
{
    acp_uint16_t aInfoType;

    if (aFnContext->mWhere == ULN_STATE_ENTRY_POINT)
    {
        aInfoType = *(acp_uint16_t *)(aFnContext->mArgs);

        // Invalid in Statemachine.but..!!
        ACI_TEST_RAISE ( (aInfoType != SQL_ODBC_VER) && (aInfoType != SQL_DRIVER_ODBC_VER), LABEL_ABORT_NO_CONNECTION ); 
    }

    return ACI_SUCCESS;
    
    ACI_EXCEPTION(LABEL_ABORT_NO_CONNECTION);
    {
        /* [2] : 08003 */
        ulnError(aFnContext, ulERR_ABORT_NO_CONNECTION, "");
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

// BUG-23976 SQLGetInfo 에서 인자값이 NULL 이 올경우 결과의 길이를 리턴해야 합니다.
// NULL 처리와 길이 계산을 위해서 Info 타입의 형식에 맞추어 3가지 함수로 나누어서 처리합니다.
// String : ulnGetInfoText
// acp_uint32_t   : ulnGetInfoacp_uint32_t
// acp_uint16_t : ulnGetInfoUShort
static acp_sint32_t ulnGetInfoText(ulnFnContext     *aFnContext,
                                   acp_char_t       *aBuffer,
                                   acp_sint16_t      aBufferSize,
                                   const acp_char_t *aValue)
{
    acp_sint32_t sSize      = 0;
    acp_rc_t     sRet;

    sSize = acpCStrLen(aValue, ACP_SINT32_MAX);

    if ((aBuffer != NULL) && (aBufferSize > 0))
    {
        sRet = acpSnprintf(aBuffer, aBufferSize, "%s", aValue);
        if (ACP_RC_IS_ETRUNC(sRet))
        {
            ulnError(aFnContext, ulERR_IGNORE_RIGHT_TRUNCATED);
        }
    }
    else
    {
        ulnError(aFnContext, ulERR_IGNORE_RIGHT_TRUNCATED);
    }

    return sSize;
}

static acp_sint32_t ulnGetInfoUInt(acp_uint32_t      *aBuffer,
                                   acp_sint16_t       aBufferSize,
                                   const acp_uint32_t aValue)
{
    acp_sint32_t sSize;

    ACP_UNUSED(aBufferSize);
    sSize = ACI_SIZEOF(acp_uint32_t);

    if(aBuffer != NULL)
    {
        *aBuffer = aValue;
    }

    return sSize;
}

static acp_sint32_t ulnGetInfoUShort(acp_uint16_t      *aBuffer,
                                     acp_sint16_t       aBufferSize,
                                     const acp_uint16_t aValue)
{
    acp_sint32_t sSize;

    ACP_UNUSED(aBufferSize);
    sSize = ACI_SIZEOF(acp_uint16_t);

    if(aBuffer != NULL)
    {
        *aBuffer = aValue;
    }

    return sSize;
}

// BUG-20711
static ACI_RC ulnGetInfoCheckArgs(ulnFnContext *aFnContext,
                                  acp_sint16_t  aLength)
{
    ACI_TEST_RAISE( aLength < 0, LABEL_INVALID_BUFFER_LEN );

    return ACI_SUCCESS;

    ACI_EXCEPTION(LABEL_INVALID_BUFFER_LEN)
    {
        /*
         * HY090 : Invalid string of buffer length
         */
        ulnError(aFnContext, ulERR_ABORT_INVALID_BUFFER_LEN, aLength);
    }
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

/*
 * BUGBUG: 에러 체크도 안하고 Cli2 를 그대로 가져다 넣었다.
 *         나중에 실행해 보면서 체크한다.
 *
 * Note : 64bit odbc 관련한 주의 사항 :
 *        When the InfoType parameter has one of the following values,
 *        a 64-bit value is returned in *InfoValuePtr: (어차피 포인터이므로 신경쓰지 않아도 됨)
 *              SQL_DRIVER_HENV
 *              SQL_DRIVER_HDBC
 *              SQL_DRIVER_HLIB
 *
 *        When InfoType has either of the following 2 values *InfoValuePtr is 64-bits
 *        on both input and ouput:
 *              SQL_DRIVER_HSTMT
 *              SQL_DRIVER_HDESC
 */

SQLRETURN ulnGetInfo(ulnDbc       *aDbc,
                     acp_uint16_t  aInfoType,
                     void         *aInfoValuePtr,
                     acp_sint16_t  aBufferLength,
                     acp_sint16_t *aStrLenPtr)
{
    acp_char_t   sProtoVerStr[MAX_VERSTR_LEN];
    acp_bool_t   sNeedExit = ACP_FALSE;
    acp_sint16_t sSize = 0;
    ulnFnContext sFnContext;

    ULN_INIT_FUNCTION_CONTEXT(sFnContext, ULN_FID_GETINFO, aDbc, ULN_OBJ_TYPE_DBC);

    ACI_TEST(ulnEnter(&sFnContext, (void *)(&aInfoType)) != ACI_SUCCESS);
    sNeedExit = ACP_TRUE;

    /*
     * Check if arguments are valid
     */
    ACI_TEST(ulnGetInfoCheckArgs(&sFnContext, aBufferLength) != ACI_SUCCESS);

    switch (aInfoType)
    {
        case SQL_ACCESSIBLE_PROCEDURES: /* ODBC 1.0 */
            sSize = ulnGetInfoText(&sFnContext, (acp_char_t *)aInfoValuePtr, aBufferLength, "Y");
            break;

        case SQL_ACCESSIBLE_TABLES: /* ODBC 1.0 */
            sSize = ulnGetInfoText(&sFnContext, (acp_char_t *)aInfoValuePtr, aBufferLength, "Y");
            break;

        case SQL_ACTIVE_ENVIRONMENTS: /* ODBC 3.0 */
            sSize = ulnGetInfoUShort((acp_uint16_t *)(acp_uint16_t *)aInfoValuePtr, aBufferLength, 1);
            break;

        case SQL_AGGREGATE_FUNCTIONS: /* ODBC 3.0 */
            sSize = ulnGetInfoUInt((acp_uint32_t *)aInfoValuePtr,
                                   aBufferLength,
                                   SQL_AF_ALL |
                                   SQL_AF_AVG |
                                   SQL_AF_COUNT |
                                   SQL_AF_DISTINCT |
                                   SQL_AF_MAX |
                                   SQL_AF_MIN |
                                   SQL_AF_SUM );
            break;

        case SQL_ALTER_DOMAIN: /* ODBC 3.0 */
            sSize = ulnGetInfoUInt((acp_uint32_t *)aInfoValuePtr,
                                   aBufferLength,
                                   0);
            break;

        case SQL_ALTER_TABLE: /* ODBC 2.0 */
            sSize = ulnGetInfoUInt((acp_uint32_t *)aInfoValuePtr,
                                   aBufferLength,
                                   SQL_AT_ADD_COLUMN_DEFAULT |
                                   SQL_AT_ADD_COLUMN_SINGLE |
                                   SQL_AT_ADD_TABLE_CONSTRAINT |
                                   SQL_AT_CONSTRAINT_NAME_DEFINITION |
                                   SQL_AT_DROP_COLUMN_DEFAULT |
                                   SQL_AT_SET_COLUMN_DEFAULT);
            break;

        case SQL_ASYNC_MODE: /* ODBC 3.0 */
            sSize = ulnGetInfoUInt((acp_uint32_t *)aInfoValuePtr,
                                   aBufferLength,
                                   SQL_AM_NONE);
            break;

        case SQL_BATCH_ROW_COUNT: /* ODBC 3.0 */
            sSize = ulnGetInfoUInt((acp_uint32_t *)aInfoValuePtr,
                                   aBufferLength,
                                   0); // SQL_BRC_EXPLICIT;
            break;

        case SQL_BATCH_SUPPORT: /* ODBC 3.0 */
            sSize = ulnGetInfoUInt((acp_uint32_t *)aInfoValuePtr,
                                   aBufferLength,
                                   0);
            break;

        case SQL_BOOKMARK_PERSISTENCE: /* ODBC 2.0 */
            sSize = ulnGetInfoUInt((acp_uint32_t *)aInfoValuePtr,
                                   aBufferLength,
                                   0);
            break;

        case SQL_CATALOG_LOCATION: /* ODBC 2.0 */
            sSize = ulnGetInfoUShort((acp_uint16_t *)aInfoValuePtr,
                                     aBufferLength,
                                     0);
            break;

        case SQL_CATALOG_NAME: /* ODBC 3.0 */
            sSize = ulnGetInfoText(&sFnContext, (acp_char_t *)aInfoValuePtr, aBufferLength, "N");
            break;

        case SQL_CATALOG_NAME_SEPARATOR: /* ODBC 3.0 */
            // BUG-24974 SQLGetInfo 에서 SQL_CATALOG_NAME_SEPARATOR 의 값을 세팅하지 말아야 합니다.
            // 어차피 CATALOG 기능을 지원하지 않는다 따라서 절대 제대로 된 값을 넘겨서는 안된다.
            sSize = ulnGetInfoText(&sFnContext, (acp_char_t *)aInfoValuePtr, aBufferLength, "");
            break;

        case SQL_CATALOG_TERM: /* ODBC 1.0 */
            sSize = ulnGetInfoText(&sFnContext, (acp_char_t *)aInfoValuePtr, aBufferLength, "");
            break;

        case SQL_CATALOG_USAGE: /* ODBC 2.0 */
            sSize = ulnGetInfoUInt((acp_uint32_t *)aInfoValuePtr,
                                   aBufferLength,
                                   0);
            break;

        case SQL_COLLATION_SEQ: /* ODBC 3.0 */
            sSize = ulnGetInfoText(&sFnContext, (acp_char_t *)aInfoValuePtr, aBufferLength, "");
            break;

        case SQL_COLUMN_ALIAS: /* ODBC 3.0 */
            sSize = ulnGetInfoText(&sFnContext, (acp_char_t *)aInfoValuePtr, aBufferLength, "Y");
            break;

        case SQL_CONCAT_NULL_BEHAVIOR: /* ODBC 3.0 */
            sSize = ulnGetInfoUShort((acp_uint16_t *)aInfoValuePtr,
                                     aBufferLength,
                                     SQL_CB_NON_NULL);
            break;

        case SQL_CONVERT_BIGINT:                /* ODBC 1.0 */
        case SQL_CONVERT_BINARY:                /* ODBC 1.0 */
        case SQL_CONVERT_BIT:                   /* ODBC 1.0 */
        case SQL_CONVERT_CHAR:                  /* ODBC 1.0 */
        case SQL_CONVERT_DATE:                  /* ODBC 1.0 */
        case SQL_CONVERT_DECIMAL:               /* ODBC 1.0 */
        case SQL_CONVERT_DOUBLE:                /* ODBC 1.0 */
        case SQL_CONVERT_FLOAT:                 /* ODBC 1.0 */
        case SQL_CONVERT_INTEGER:               /* ODBC 1.0 */
        case SQL_CONVERT_INTERVAL_DAY_TIME:     /* ODBC 3.0 */
        case SQL_CONVERT_INTERVAL_YEAR_MONTH:   /* ODBC 3.0 */
        case SQL_CONVERT_LONGVARBINARY:         /* ODBC 1.0 */
        case SQL_CONVERT_LONGVARCHAR:           /* ODBC 1.0 */
        case SQL_CONVERT_NUMERIC:               /* ODBC 1.0 */
        case SQL_CONVERT_REAL:                  /* ODBC 1.0 */
        case SQL_CONVERT_SMALLINT:              /* ODBC 1.0 */
        case SQL_CONVERT_TIME:                  /* ODBC 1.0 */
        case SQL_CONVERT_TIMESTAMP:             /* ODBC 1.0 */
        case SQL_CONVERT_TINYINT:               /* ODBC 1.0 */
        case SQL_CONVERT_VARBINARY:             /* ODBC 1.0 */
        case SQL_CONVERT_VARCHAR:               /* ODBC 1.0 */
        case SQL_CONVERT_WCHAR:                 /* ODBC 1.0 */
        case SQL_CONVERT_WLONGVARCHAR:          /* ODBC 1.0 */
        case SQL_CONVERT_WVARCHAR:              /* ODBC 1.0 */
            sSize = ulnGetInfoUInt((acp_uint32_t *)aInfoValuePtr,
                                   aBufferLength,
                                   SQL_CVT_BIGINT |
                                   SQL_CVT_BINARY |
                                   SQL_CVT_BIT |
                                   SQL_CVT_CHAR |
                                   SQL_CVT_DATE |
                                   SQL_CVT_DECIMAL |
                                   SQL_CVT_DOUBLE |
                                   SQL_CVT_FLOAT |
                                   SQL_CVT_INTEGER |
                                   SQL_CVT_INTERVAL_YEAR_MONTH |
                                   SQL_CVT_INTERVAL_DAY_TIME |
                                   SQL_CVT_LONGVARBINARY |
                                   SQL_CVT_LONGVARCHAR |
                                   SQL_CVT_NUMERIC |
                                   SQL_CVT_REAL |
                                   SQL_CVT_SMALLINT |
                                   SQL_CVT_TIME |
                                   SQL_CVT_TIMESTAMP |
                                   SQL_CVT_TINYINT |
                                   SQL_CVT_VARBINARY |
                                   SQL_CVT_VARCHAR |
                                   SQL_CVT_WCHAR |
                                   SQL_CVT_WVARCHAR);
            break;

        case SQL_CONVERT_FUNCTIONS: /* ODBC 3.0 */
            sSize = ulnGetInfoUInt((acp_uint32_t *)aInfoValuePtr,
                                   aBufferLength,
                                   0); /* SQL_FN_CVT_CAST |
                                          SQL_FN_CVT_CONVERT; */
            break;

        case SQL_CORRELATION_NAME: /* ODBC 1.0 */
            sSize = ulnGetInfoUShort((acp_uint16_t *)aInfoValuePtr,
                                     aBufferLength,
                                     SQL_CN_DIFFERENT); //SQL_CN_ANY; /* BUGBUG May not be correct */
            break;

        case SQL_CREATE_ASSERTION: /* ODBC 3.0 */
            sSize = ulnGetInfoUInt((acp_uint32_t *)aInfoValuePtr,
                                   aBufferLength,
                                   0);
            break;

        case SQL_CREATE_CHARACTER_SET: /* ODBC 3.0 */
            sSize = ulnGetInfoUInt((acp_uint32_t *)aInfoValuePtr,
                                   aBufferLength,
                                   0);
            break;

        case SQL_CREATE_COLLATION: /* ODBC 3.0 */
            sSize = ulnGetInfoUInt((acp_uint32_t *)aInfoValuePtr,
                                   aBufferLength,
                                   0);
            break;

        case SQL_CREATE_DOMAIN: /* ODBC 3.0 */
            sSize = ulnGetInfoUInt((acp_uint32_t *)aInfoValuePtr,
                                   aBufferLength,
                                   0);
            break;

        case SQL_CREATE_SCHEMA: /* ODBC 3.0 */
            sSize = ulnGetInfoUInt((acp_uint32_t *)aInfoValuePtr,
                                   aBufferLength,
                                   SQL_CS_CREATE_SCHEMA |
                                   SQL_CS_AUTHORIZATION); /* lie for supporting SQL92 Entry Level */
            break;

        case SQL_CREATE_TABLE: /* ODBC 3.0 */
            sSize = ulnGetInfoUInt((acp_uint32_t *)aInfoValuePtr,
                                   aBufferLength,
                                   SQL_CT_CREATE_TABLE |
                                   SQL_CT_TABLE_CONSTRAINT |
                                   SQL_CT_CONSTRAINT_NAME_DEFINITION |
                                   SQL_CT_COLUMN_CONSTRAINT |
                                   SQL_CT_COLUMN_DEFAULT);
            break;

        case SQL_CREATE_TRANSLATION: /* ODBC 3.0 */
            sSize = ulnGetInfoUInt((acp_uint32_t *)aInfoValuePtr,
                                   aBufferLength,
                                   0);
            break;

        case SQL_CREATE_VIEW: /* ODBC 3.0 */
            sSize = ulnGetInfoUInt((acp_uint32_t *)aInfoValuePtr,
                                   aBufferLength,
                                   0);
            break;

        /* BUG-28758: SQLGetInfo(SQL_CURSOR_COMMIT_BEHAVIOR)이 잘못된 값 반환 */
        case SQL_CURSOR_COMMIT_BEHAVIOR: /* ODBC 1.0 */
        case SQL_CURSOR_ROLLBACK_BEHAVIOR: /* ODBC 1.0 */
            sSize = ulnGetInfoUShort((acp_uint16_t *)aInfoValuePtr, aBufferLength, SQL_CB_CLOSE);
            break;

        case SQL_CURSOR_SENSITIVITY: /* ODBC 3.0 */
            /*****
                  BUGBUG
                  An SQL92 Entry level conformant driver
                  will always return
                  the SQL_UNSPECIFIED option as supported.
            *****/
            sSize = ulnGetInfoUInt((acp_uint32_t *)aInfoValuePtr,
                                   aBufferLength,
                                   SQL_INSENSITIVE);
            break;

        case SQL_DATA_SOURCE_NAME: /* ODBC 1.0 */
            sSize = ulnGetInfoText(&sFnContext, (acp_char_t *)aInfoValuePtr, aBufferLength, aDbc->mDsnString);
            break;

        case SQL_DATA_SOURCE_READ_ONLY: /* ODBC 1.0 */
            sSize = ulnGetInfoText(&sFnContext, (acp_char_t *)aInfoValuePtr, aBufferLength, "N"); /* BUGBUG May not be correct */
            break;

        case SQL_DATABASE_NAME: /* ODBC 1.0 */
            sSize = ulnGetInfoText(&sFnContext, (acp_char_t *)aInfoValuePtr, aBufferLength, "database");
            break;

        case SQL_DATETIME_LITERALS: /* ODBC 3.0 */
            sSize = ulnGetInfoUInt((acp_uint32_t *)aInfoValuePtr,
                                   aBufferLength,
                                   0); /* BUGBUG have no idea */
            break;

        case SQL_DBMS_NAME: /* ODBC 1.0 */
            sSize = ulnGetInfoText(&sFnContext, (acp_char_t *)aInfoValuePtr, aBufferLength, "Altibase");
            break;

        case SQL_DBMS_VER: /* ODBC 1.0 */
            // fix BUG-18971
            sSize = ulnGetInfoText(&sFnContext, (acp_char_t *)aInfoValuePtr, aBufferLength, aDbc->mServerPackageVer);
            break;

        /* BUG-30232 */
        /* PROJ-2063: 프로토콜 버전을 얻을 수 있도록 추가 */
        case ALTIBASE_PROTO_VER:
            (void) acpSnprintf(sProtoVerStr, ACI_SIZEOF(sProtoVerStr), "%d.%d.%d",
                               CM_MAJOR_VERSION, CM_MINOR_VERSION, CM_PATCH_VERSION);
            sSize = ulnGetInfoText(&sFnContext, aInfoValuePtr, aBufferLength, sProtoVerStr);
            break; 

#ifdef COMPILE_SHARDCLI
        case ALTIBASE_SHARD_VER:
            (void) acpSnprintf(sProtoVerStr, ACI_SIZEOF(sProtoVerStr), "%d.%d.%d",
                               SHARD_MAJOR_VERSION, SHARD_MINOR_VERSION, SHARD_PATCH_VERSION);
            sSize = ulnGetInfoText(&sFnContext, aInfoValuePtr, aBufferLength, sProtoVerStr);
            break;
#endif /* COMPILE_SHARDCLI */

        case SQL_DDL_INDEX: /* ODBC 3.0 */
            sSize = ulnGetInfoUInt((acp_uint32_t *)aInfoValuePtr,
                                   aBufferLength,
                                   SQL_DI_CREATE_INDEX |
                                   SQL_DI_DROP_INDEX);
            break;

        case SQL_DEFAULT_TXN_ISOLATION: /* ODBC 1.0 */
            sSize = ulnGetInfoUInt((acp_uint32_t *)aInfoValuePtr,
                                   aBufferLength,
                                   SQL_TXN_READ_COMMITTED);
            break;

        case SQL_DESCRIBE_PARAMETER: /* ODBC 3.0 */
            sSize = ulnGetInfoText(&sFnContext, (acp_char_t *)aInfoValuePtr, aBufferLength, "N");
            break;

        case SQL_DM_VER: /* ODBC 3.0 */
            // BUGBUG SQL_SPEC_MAJOR SQL_SPEC_MINOR 매크로를 이용해야 한다.
            // 변경될일이 없기 때문에 일단 간단하게 처리한다.
            sSize = ulnGetInfoText(&sFnContext,
                                   (acp_char_t *)aInfoValuePtr,
                                   aBufferLength,
                                   "03.52.0000.0000");
            break;

        case SQL_DRIVER_NAME: /* ODBC 1.0 */
            sSize = ulnGetInfoText(&sFnContext, (acp_char_t *)aInfoValuePtr, aBufferLength, ALTIBASE_ODBC_NAME);
            break;

        case SQL_DRIVER_ODBC_VER: /* ODBC 2.0 */
            sSize = ulnGetInfoText(&sFnContext, (acp_char_t *)aInfoValuePtr, aBufferLength, ALTIBASE_ODBC_VER);
            break;

        case SQL_DRIVER_VER: /* ODBC 1.0 */
            sSize = ulnGetInfoText(&sFnContext, (acp_char_t *)aInfoValuePtr, aBufferLength, ALTIBASE_DLL_VER);
            break;

        case SQL_DROP_ASSERTION: /* ODBC 3.0 */
            sSize = ulnGetInfoUInt((acp_uint32_t *)aInfoValuePtr,
                                   aBufferLength,
                                   0);
            break;

        case SQL_DROP_CHARACTER_SET: /* ODBC 3.0 */
            sSize = ulnGetInfoUInt((acp_uint32_t *)aInfoValuePtr,
                                   aBufferLength,
                                   0);
            break;

        case SQL_DROP_COLLATION: /* ODBC 3.0 */
            sSize = ulnGetInfoUInt((acp_uint32_t *)aInfoValuePtr,
                                   aBufferLength,
                                   0);
            break;

        case SQL_DROP_DOMAIN: /* ODBC 3.0 */
            sSize = ulnGetInfoUInt((acp_uint32_t *)aInfoValuePtr,
                                   aBufferLength,
                                   0);
            break;

        case SQL_DROP_SCHEMA: /* ODBC 3.0 */
            sSize = ulnGetInfoUInt((acp_uint32_t *)aInfoValuePtr,
                                   aBufferLength,
                                   SQL_DS_DROP_SCHEMA |
                                   SQL_DS_CASCADE |
                                   SQL_DS_RESTRICT); /* lie for supporting SQL92 Entry Level */
            break;

        case SQL_DROP_TABLE: /* ODBC 3.0 */
            sSize = ulnGetInfoUInt((acp_uint32_t *)aInfoValuePtr,
                                   aBufferLength,
                                   SQL_DT_DROP_TABLE);
            break;

        case SQL_DROP_TRANSLATION: /* ODBC 3.0 */
            sSize = ulnGetInfoUInt((acp_uint32_t *)aInfoValuePtr,
                                   aBufferLength,
                                   0);
            break;

        case SQL_DROP_VIEW: /* ODBC 3.0 */
            sSize = ulnGetInfoUInt((acp_uint32_t *)aInfoValuePtr,
                                   aBufferLength,
                                   0);
            break;

        case SQL_DYNAMIC_CURSOR_ATTRIBUTES1: /* ODBC 3.0 */
            sSize = ulnGetInfoUInt((acp_uint32_t *)aInfoValuePtr,
                                   aBufferLength,
                                   0 /* not supported */ );
            break;

        case SQL_DYNAMIC_CURSOR_ATTRIBUTES2: /* ODBC 3.0 */
            sSize = ulnGetInfoUInt((acp_uint32_t *)aInfoValuePtr,
                                   aBufferLength,
                                   0 /* not supported */ );
            break;

        case SQL_EXPRESSIONS_IN_ORDERBY: /* ODBC 1.0 */
            sSize = ulnGetInfoText(&sFnContext, (acp_char_t *)aInfoValuePtr, aBufferLength, "Y");
            break;

        case SQL_FILE_USAGE: /* ODBC 2.0 */
            sSize = ulnGetInfoUShort((acp_uint16_t *)aInfoValuePtr,
                                     aBufferLength,
                                     SQL_FILE_NOT_SUPPORTED);
            break;

        case SQL_FORWARD_ONLY_CURSOR_ATTRIBUTES1: /* ODBC 3.0 */
            sSize = ulnGetInfoUInt((acp_uint32_t *)aInfoValuePtr,
                                   aBufferLength,
                                   SQL_CA1_NEXT);
            break;

        case SQL_FORWARD_ONLY_CURSOR_ATTRIBUTES2: /* ODBC 3.0 */
            sSize = ulnGetInfoUInt((acp_uint32_t *)aInfoValuePtr,
                                   aBufferLength,
                                   SQL_CA2_READ_ONLY_CONCURRENCY);
            break;

        case SQL_FETCH_DIRECTION: /* ODBC 1.0 : Deprecated ODBC 3.x */
            sSize = ulnGetInfoUInt((acp_uint32_t *)aInfoValuePtr,
                                   aBufferLength,
                                   SQL_FD_FETCH_NEXT |
                                   SQL_FD_FETCH_FIRST |
                                   SQL_FD_FETCH_LAST |
                                   SQL_FD_FETCH_PRIOR |
                                   SQL_FD_FETCH_ABSOLUTE |
                                   SQL_FD_FETCH_RELATIVE |
                                   SQL_FD_FETCH_BOOKMARK);
            break;

        case SQL_GETDATA_EXTENSIONS: /* ODBC 2.0 */
            sSize = ulnGetInfoUInt((acp_uint32_t *)aInfoValuePtr,
                                   aBufferLength,
                                   SQL_GD_ANY_COLUMN |
                                   SQL_GD_ANY_ORDER |
                                   SQL_GD_BOUND);
            break;

        case SQL_GROUP_BY: /* ODBC 2.0 */
            sSize = ulnGetInfoUShort((acp_uint16_t *)aInfoValuePtr,
                                     aBufferLength,
                                     SQL_GB_GROUP_BY_EQUALS_SELECT);
            break;

        case SQL_IDENTIFIER_CASE: /* ODBC 1.0 */
            sSize = ulnGetInfoUShort((acp_uint16_t *)aInfoValuePtr,
                                     aBufferLength,
                                     SQL_IC_UPPER);
            break;

        case SQL_IDENTIFIER_QUOTE_CHAR: /* ODBC 1.0 */
            sSize = ulnGetInfoText(&sFnContext, (acp_char_t *)aInfoValuePtr, aBufferLength, " ");
            break;

        case SQL_INDEX_KEYWORDS: /* ODBC 3.0 */
            sSize = ulnGetInfoUInt((acp_uint32_t *)aInfoValuePtr,
                                   aBufferLength,
                                   SQL_IK_ALL);
            break;

        case SQL_INFO_SCHEMA_VIEWS: /* ODBC 3.0 */
            sSize = ulnGetInfoUInt((acp_uint32_t *)aInfoValuePtr,
                                   aBufferLength,
                                   0);
            break;

        case SQL_INSERT_STATEMENT: /* ODBC 3.0 */
            sSize = ulnGetInfoUInt((acp_uint32_t *)aInfoValuePtr,
                                   aBufferLength,
                                   SQL_IS_INSERT_LITERALS |
                                   SQL_IS_INSERT_SEARCHED |
                                   SQL_IS_SELECT_INTO);
            break;

        case SQL_INTEGRITY: /* ODBC 1.0 */
            sSize = ulnGetInfoText(&sFnContext, (acp_char_t *)aInfoValuePtr, aBufferLength, "Y"); /** BUGBUG May not be correct **/
            break;

        case SQL_KEYSET_CURSOR_ATTRIBUTES1: /* ODBC 3.0 */
            sSize = ulnGetInfoUInt((acp_uint32_t *)aInfoValuePtr,
                                   aBufferLength,
                                   SQL_CA1_NEXT |
                                   SQL_CA1_ABSOLUTE |
                                   SQL_CA1_RELATIVE |
                                   SQL_CA1_BOOKMARK |
                                   SQL_CA1_LOCK_NO_CHANGE |
                                   SQL_CA1_POS_POSITION |
                                   SQL_CA1_POS_UPDATE |
                                   SQL_CA1_POS_DELETE |
                                   SQL_CA1_POS_REFRESH |
                                   SQL_CA1_BULK_ADD |
                                   SQL_CA1_BULK_UPDATE_BY_BOOKMARK |
                                   SQL_CA1_BULK_DELETE_BY_BOOKMARK |
                                   SQL_CA1_BULK_FETCH_BY_BOOKMARK );
            break;

        case SQL_KEYSET_CURSOR_ATTRIBUTES2: /* ODBC 3.0 */
            sSize = ulnGetInfoUInt((acp_uint32_t *)aInfoValuePtr,
                                   aBufferLength,
                                   SQL_CA2_READ_ONLY_CONCURRENCY |
                                   SQL_CA2_OPT_ROWVER_CONCURRENCY  |
                                   SQL_CA2_SENSITIVITY_DELETIONS  |
                                   SQL_CA2_SENSITIVITY_UPDATES );
            break;

        case SQL_KEYWORDS: /* ODBC 3.0 */
            sSize = ulnGetInfoText(&sFnContext, (acp_char_t *)aInfoValuePtr, aBufferLength,
                                   "ABSOLUTE, "
                                   "ADD, "
                                   "ALL, "
                                   "ALTER, "
                                   "AND, "
                                   "ANY, "
                                   "AS, "
                                   "ASC, "
                                   "AVG, "
                                   "BETWEEN, "
                                   "BIGINT, "
                                   "BLOB, "
                                   "BOTH, "
                                   "CHAR, "
                                   "CHECK, "
                                   "COLUMN, "
                                   "COMMIT, "
                                   "CONNECT, "
                                   "CONSTRAINT, "
                                   "CONSTRAINTS, "
                                   "COUNT, "
                                   "CREATE, "
                                   "DATE, "
                                   "DEFAULT, "
                                   "DELETE, "
                                   "DESC, "
                                   "DISCONNECT, "
                                   "DISTINCT, "
                                   "DOUBLE, "
                                   "DROP, "
                                   "END, "
                                   "ESCAPE, "
                                   "EXCEPT, "
                                   "EXISTS, "
                                   "FOREIGN, "
                                   "FROM, "
                                   "FULL, "
                                   "HAVING, "
                                   "IN, "
                                   "INDEX, "
                                   "INNER, "
                                   "INSERT, "
                                   "INTERSECT, "
                                   "INTO, "
                                   "JOIN, "
                                   "KEY, "
                                   "KEYS, "
                                   "LEADING, "
                                   "LIKE, "
                                   "LIMIT, "
                                   "LONGBLOB, "
                                   "MAX, "
                                   "MEDIMUMBLOB, "
                                   "MEDIUMINT, "
                                   "MEDIUMTEXT, "
                                   "MIN, "
                                   "MINUTE, "
                                   "MONTH, "
                                   "NCHAR, "
                                   "NOT, "
                                   "NULL, "
                                   "NVARCHAR, "
                                   "OFF, "
                                   "ON, "
                                   "ONLY, "
                                   "OR, "
                                   "ORDER, "
                                   "OUTER, "
                                   "PRECISION, "
                                   "PRIMARY, "
                                   "PROCEDURE, "
                                   "REFERENCES, "
                                   "REGEXP, "
                                   "RESTRICT, "
                                   "RLIKE, "
                                   "ROLLBACK, "
                                   "ROWS, "
                                   "SELECT, "
                                   "SET, "
                                   "SHOW, "
                                   "SOME, "
                                   "SQL, "
                                   "SQLCA, "
                                   "SQLCODE, "
                                   "SQLERROR, "
                                   "SQLSTATE, "
                                   "SUM, "
                                   "TABLE, "
                                   "TABLES, "
                                   "TINTTEXT, "
                                   "TINYBLOB, "
                                   "UNION, "
                                   "UNIQUE, "
                                   "UNSIGNED, "
                                   "UPDATE, "
                                   "USER, "
                                   "USING, "
                                   "VARCHAR, "
                                   "WHERE, "
                                   "WITH, "
                                   "YEAR, "
                                   "ZEROFILL");
            break;

        case SQL_LIKE_ESCAPE_CLAUSE: /* ODBC 2.0 */
            sSize = ulnGetInfoText(&sFnContext, (acp_char_t *)aInfoValuePtr, aBufferLength, "N");
            break;

        case SQL_LOCK_TYPES: /* ODBC 2.0 */
            sSize = ulnGetInfoUInt((acp_uint32_t *)aInfoValuePtr,
                                   aBufferLength,
                                   0);
            break;

        case SQL_MAX_ASYNC_CONCURRENT_STATEMENTS: /* ODBC 3.0 */
            sSize = ulnGetInfoUInt((acp_uint32_t *)aInfoValuePtr,
                                   aBufferLength,
                                   0); /* BUGBUG May not be correct */
            break;

        case SQL_MAX_BINARY_LITERAL_LEN: /* ODBC 2.0 */
            sSize = ulnGetInfoUInt((acp_uint32_t *)aInfoValuePtr,
                                   aBufferLength,
                                   ULN_MAX_BINARY_PRECISION * 2);
            break;

        case SQL_MAX_CATALOG_NAME_LEN: /* ODBC 1.0 */
            sSize = ulnGetInfoUShort((acp_uint16_t *)aInfoValuePtr,
                                     aBufferLength,
                                     0);
            break;

        case SQL_MAX_CHAR_LITERAL_LEN: /* ODBC 2.0 */
            sSize = ulnGetInfoUInt((acp_uint32_t *)aInfoValuePtr,
                                   aBufferLength,
                                   ULN_MAX_CHAR_PRECISION);
            break;

        case SQL_MAX_COLUMN_NAME_LEN: /* ODBC 1.0 */
            sSize = ulnGetInfoUShort((acp_uint16_t *)aInfoValuePtr,
                                     aBufferLength,
                                     MAX_NAME_LEN);
            break;

        case SQL_MAX_COLUMNS_IN_GROUP_BY: /* ODBC 2.0 */
            sSize = ulnGetInfoUShort((acp_uint16_t *)aInfoValuePtr,
                                     aBufferLength,
                                     0);
            break;

        case SQL_MAX_COLUMNS_IN_INDEX: /* ODBC 2.0 */
            sSize = ulnGetInfoUShort((acp_uint16_t *)aInfoValuePtr,
                                     aBufferLength,
                                     0);
            break;

        case SQL_MAX_COLUMNS_IN_ORDER_BY: /* ODBC 2.0 */
            sSize = ulnGetInfoUShort((acp_uint16_t *)aInfoValuePtr,
                                     aBufferLength,
                                     0);
            break;

        case SQL_MAX_COLUMNS_IN_SELECT: /* ODBC 2.0 */
            sSize = ulnGetInfoUShort((acp_uint16_t *)aInfoValuePtr,
                                     aBufferLength,
                                     0);
            break;

        case SQL_MAX_COLUMNS_IN_TABLE: /* ODBC 2.0 */
            sSize = ulnGetInfoUShort((acp_uint16_t *)aInfoValuePtr,
                                     aBufferLength,
                                     1024);
            break;

            /* bug-31196 SQLGetInfo(SQL_MAX_CONCURRENT_ACTIVITIES)
             * should return 0 for executing PSM in delphi.
             * cf) oracle return 0 */
        case SQL_MAX_CONCURRENT_ACTIVITIES: /* ODBC 1.0 */
            sSize = ulnGetInfoUShort((acp_uint16_t *)aInfoValuePtr,
                                     aBufferLength,
                                     0);
            break;

        case SQL_MAX_CURSOR_NAME_LEN: /* ODBC 1.0 */
            sSize = ulnGetInfoUShort((acp_uint16_t *)aInfoValuePtr,
                                     aBufferLength,
                                     MAX_NAME_LEN);
            break;

        case SQL_MAX_DRIVER_CONNECTIONS: /* ODBC 1.0 */
            sSize = ulnGetInfoUShort((acp_uint16_t *)aInfoValuePtr,
                                     aBufferLength,
                                     0); /* BUGBUG lie */
            break;

        case SQL_MAX_IDENTIFIER_LEN: /* ODBC 3.0 */
            sSize = ulnGetInfoUShort((acp_uint16_t *)aInfoValuePtr,
                                     aBufferLength,
                                     MAX_NAME_LEN);
            break;

        case SQL_MAX_INDEX_SIZE: /* ODBC 2.0 */
            sSize = ulnGetInfoUInt((acp_uint32_t *)aInfoValuePtr,
                                   aBufferLength,
                                   0); /* unkown */
            break;

        case SQL_MAX_PROCEDURE_NAME_LEN: /* ODBC 1.0 */
            sSize = ulnGetInfoUShort((acp_uint16_t *)aInfoValuePtr,
                                     aBufferLength,
                                     MAX_NAME_LEN);
            break;

        case SQL_MAX_ROW_SIZE: /* ODBC 2.0 */
            sSize = ulnGetInfoUInt((acp_uint32_t *)aInfoValuePtr,
                                   aBufferLength,
                                   0);
            break;

        case SQL_MAX_ROW_SIZE_INCLUDES_LONG: /* ODBC 3.0 */
            sSize = ulnGetInfoText(&sFnContext, (acp_char_t *)aInfoValuePtr, aBufferLength, "N");
            break;

        case SQL_MAX_SCHEMA_NAME_LEN: /* ODBC 1.0 */
            sSize = ulnGetInfoUShort((acp_uint16_t *)aInfoValuePtr,
                                     aBufferLength,
                                     MAX_NAME_LEN);
            break;

        case SQL_MAX_STATEMENT_LEN: /* ODBC 2.0 */
            sSize = ulnGetInfoUInt((acp_uint32_t *)aInfoValuePtr,
                                   aBufferLength,
                                   0);
            break;

        case SQL_MAX_TABLE_NAME_LEN: /* ODBC 1.0 */
            sSize = ulnGetInfoUShort((acp_uint16_t *)aInfoValuePtr,
                                     aBufferLength,
                                     MAX_NAME_LEN);
            break;

        case SQL_MAX_TABLES_IN_SELECT: /* ODBC 2.0 */
            sSize = ulnGetInfoUShort((acp_uint16_t *)aInfoValuePtr,
                                     aBufferLength,
                                     0);
            break;

        case SQL_MAX_USER_NAME_LEN: /* ODBC 2.0 */
            sSize = ulnGetInfoUShort((acp_uint16_t *)aInfoValuePtr,
                                     aBufferLength,
                                     MAX_NAME_LEN);
            break;

        case SQL_MULT_RESULT_SETS: /* ODBC 1.0 */
            sSize = ulnGetInfoText(&sFnContext, (acp_char_t *)aInfoValuePtr, aBufferLength, "N");
            break;

        case SQL_MULTIPLE_ACTIVE_TXN: /* ODBC 1.0 */
            sSize = ulnGetInfoText(&sFnContext, (acp_char_t *)aInfoValuePtr, aBufferLength, "N"); /** BUGBUG May not be correct **/
            break;

        case SQL_NEED_LONG_DATA_LEN: /* ODBC 2.0 */
            sSize = ulnGetInfoText(&sFnContext, (acp_char_t *)aInfoValuePtr, aBufferLength, "Y");
            break;

        case SQL_NON_NULLABLE_COLUMNS: /* ODBC 1.0 */
            sSize = ulnGetInfoUShort((acp_uint16_t *)aInfoValuePtr,
                                     aBufferLength,
                                     SQL_NNC_NON_NULL);
            break;

        case SQL_NULL_COLLATION: /* ODBC 2.0 */
            sSize = ulnGetInfoUShort((acp_uint16_t *)aInfoValuePtr,
                                     aBufferLength,
                                     SQL_NC_END);
            break;

        case SQL_NUMERIC_FUNCTIONS: /* ODBC 1.0 */
            sSize = ulnGetInfoUInt((acp_uint32_t *)aInfoValuePtr,
                                   aBufferLength,
                                   SQL_FN_NUM_ABS |
                                   SQL_FN_NUM_CEILING |
                                   SQL_FN_NUM_FLOOR |
                                   SQL_FN_NUM_MOD |
                                   SQL_FN_NUM_ROUND |
                                   SQL_FN_NUM_SIGN |
                                   SQL_FN_NUM_TRUNCATE);
            break;

        case SQL_ODBC_API_CONFORMANCE: /* ODBC 1.0 */
            sSize = ulnGetInfoUShort((acp_uint16_t *)aInfoValuePtr,
                                     aBufferLength,
                                     SQL_OAC_LEVEL1); /* BUGBUG lie */
            break;

        case SQL_ODBC_INTERFACE_CONFORMANCE: /* ODBC 3.0 */
            sSize = ulnGetInfoUInt((acp_uint32_t *)aInfoValuePtr,
                                   aBufferLength,
                                   SQL_OIC_CORE); /* BUGBUG lie */
            break;

        case SQL_ODBC_SQL_CONFORMANCE: /* ODBC 1.0 */
            sSize = ulnGetInfoUShort((acp_uint16_t *)aInfoValuePtr,
                                     aBufferLength,
                                     SQL_OSC_CORE); /* SQL_OSC_MINIMUM, SQL_OSC_CORE, SQL_OSC_EXTENDED */
            break;

        case SQL_ODBC_VER: /* ODBC 1.0 */
            sSize = ulnGetInfoText(&sFnContext, (acp_char_t *)aInfoValuePtr, aBufferLength, ALTIBASE_ODBC_VER);
            break;

        case SQL_OUTER_JOINS:
            sSize = ulnGetInfoText(&sFnContext, (acp_char_t *)aInfoValuePtr, aBufferLength, "N");
            break;

        case SQL_OJ_CAPABILITIES: /* ODBC 2.01 */
            sSize = ulnGetInfoUInt((acp_uint32_t *)aInfoValuePtr,
                                   aBufferLength,
                                   0);
            break;

        case SQL_ORDER_BY_COLUMNS_IN_SELECT: /* ODBC 2.0 */
            sSize = ulnGetInfoText(&sFnContext, (acp_char_t *)aInfoValuePtr, aBufferLength, "N");
            break;

        case SQL_PARAM_ARRAY_ROW_COUNTS: /* ODBC 3.0 */
            sSize = ulnGetInfoUInt((acp_uint32_t *)aInfoValuePtr,
                                   aBufferLength,
                                   SQL_PARC_BATCH);
            break;

        case SQL_PARAM_ARRAY_SELECTS: /* ODBC 3.0 */
            sSize = ulnGetInfoUInt((acp_uint32_t *)aInfoValuePtr,
                                   aBufferLength,
                                   SQL_PAS_NO_SELECT);
            break;

        case SQL_POSITIONED_STATEMENTS: /* ODBC 2.0 */
            sSize = ulnGetInfoUInt((acp_uint32_t *)aInfoValuePtr,
                                   aBufferLength,
                                   0);
            break;

        case SQL_POS_OPERATIONS: /* ODBC 2.0 */
            sSize = ulnGetInfoUInt((acp_uint32_t *)aInfoValuePtr,
                                   aBufferLength,
                                   SQL_POS_POSITION); /* BUGBUG lie */
            break;

        case SQL_PROCEDURE_TERM: /* ODBC 1.0 */
            sSize = ulnGetInfoText(&sFnContext, (acp_char_t *)aInfoValuePtr, aBufferLength, "procedure");
            break;

        case SQL_PROCEDURES: /* ODBC 1.0 */
            sSize = ulnGetInfoText(&sFnContext, (acp_char_t *)aInfoValuePtr, aBufferLength, "Y");
            break;

        case SQL_QUOTED_IDENTIFIER_CASE: /* ODBC 2.0 */
            sSize = ulnGetInfoUShort((acp_uint16_t *)aInfoValuePtr,
                                     aBufferLength,
                                     SQL_IC_SENSITIVE); /* BUGBUG lie */
            break;

        case SQL_ROW_UPDATES: /* ODBC 1.0 */
            sSize = ulnGetInfoText(&sFnContext, (acp_char_t *)aInfoValuePtr, aBufferLength, "N");
            break;

        case SQL_SCHEMA_TERM: /* ODBC 1.0 */
            sSize = ulnGetInfoText(&sFnContext, (acp_char_t *)aInfoValuePtr, aBufferLength, "schema"); /** BUGBUG lie **/
            break;

        case SQL_SCHEMA_USAGE: /* ODBC 2.0 */
            sSize = ulnGetInfoUInt((acp_uint32_t *)aInfoValuePtr,
                                   aBufferLength,
                                   SQL_SU_DML_STATEMENTS |
                                   SQL_SU_TABLE_DEFINITION |
                                   SQL_SU_PRIVILEGE_DEFINITION); /* BUGBUG lie */
            break;

        case SQL_SCROLL_CONCURRENCY: /* ODBC 1.0 */
            sSize = ulnGetInfoUInt((acp_uint32_t *)aInfoValuePtr,
                                   aBufferLength,
                                   SQL_SCCO_READ_ONLY);
            break;

        case SQL_SCROLL_OPTIONS: /* ODBC 1.0 */
            sSize = ulnGetInfoUInt((acp_uint32_t *)aInfoValuePtr,
                                   aBufferLength,
                                   SQL_SO_FORWARD_ONLY |
                                   SQL_SO_STATIC); /* SQL_SO_STATIC */
            break;

        case SQL_SEARCH_PATTERN_ESCAPE: /* ODBC 1.0 */
            sSize = ulnGetInfoText(&sFnContext, (acp_char_t *)aInfoValuePtr, aBufferLength, "");
            break;

        case SQL_SERVER_NAME: /* ODBC 1.0 */
            sSize = ulnGetInfoText(&sFnContext, (acp_char_t *)aInfoValuePtr, aBufferLength, "");
            break;

        case SQL_SPECIAL_CHARACTERS: /* ODBC 2.0 */
            sSize = ulnGetInfoText(&sFnContext, (acp_char_t *)aInfoValuePtr, aBufferLength, "_");
            break;

        case SQL_SQL_CONFORMANCE: /* ODBC 3.0 */
            sSize = ulnGetInfoUInt((acp_uint32_t *)aInfoValuePtr,
                                   aBufferLength,
                                   SQL_SC_SQL92_ENTRY);
            break;

        case SQL_SQL92_DATETIME_FUNCTIONS: /* ODBC 3.0 */
            sSize = ulnGetInfoUInt((acp_uint32_t *)aInfoValuePtr,
                                   aBufferLength,
                                   SQL_SDF_CURRENT_DATE);
            break;

        case SQL_SQL92_FOREIGN_KEY_DELETE_RULE: /* ODBC 3.0 */
            sSize = ulnGetInfoUInt((acp_uint32_t *)aInfoValuePtr,
                                   aBufferLength,
                                   SQL_SFKD_NO_ACTION);
            break;

        case SQL_SQL92_FOREIGN_KEY_UPDATE_RULE: /* ODBC 3.0 */
            sSize = ulnGetInfoUInt((acp_uint32_t *)aInfoValuePtr,
                                   aBufferLength,
                                   SQL_SFKU_NO_ACTION);
            break;

        case SQL_SQL92_GRANT: /* ODBC 3.0 */
            sSize = ulnGetInfoUInt((acp_uint32_t *)aInfoValuePtr,
                                   aBufferLength,
                                   0);
            break;

        case SQL_SQL92_NUMERIC_VALUE_FUNCTIONS: /* ODBC 3.0 */
            sSize = ulnGetInfoUInt((acp_uint32_t *)aInfoValuePtr,
                                   aBufferLength,
                                   0); /*  SQL_SNVF_BIT_LENGTH
                                           SQL_SNVF_CHAR_LENGTH
                                           SQL_SNVF_CHARACTER_LENGTH
                                           SQL_SNVF_EXTRACT
                                           SQL_SNVF_OCTET_LENGTH
                                           SQL_SNVF_POSITION  */
            break;

        case SQL_SQL92_PREDICATES: /* ODBC 3.0 */
            sSize = ulnGetInfoUInt((acp_uint32_t *)aInfoValuePtr,
                                   aBufferLength,
                                   SQL_SP_BETWEEN |
                                   SQL_SP_EXISTS |
                                   SQL_SP_IN |
                                   SQL_SP_ISNOTNULL |
                                   SQL_SP_ISNULL |
                                   SQL_SP_LIKE |
                                   SQL_SP_QUANTIFIED_COMPARISON |
                                   SQL_SP_UNIQUE);
            break;

        case SQL_SQL92_RELATIONAL_JOIN_OPERATORS: /* ODBC 3.0 */
            sSize = ulnGetInfoUInt((acp_uint32_t *)aInfoValuePtr,
                                   aBufferLength,
                                   SQL_SRJO_CORRESPONDING_CLAUSE |
                                   SQL_SRJO_EXCEPT_JOIN |
                                   SQL_SRJO_FULL_OUTER_JOIN |
                                   SQL_SRJO_INNER_JOIN |
                                   SQL_SRJO_INTERSECT_JOIN); /* BUGBUG May not be correct */
            break;

        case SQL_SQL92_REVOKE: /* ODBC 3.0 */
            sSize = ulnGetInfoUInt((acp_uint32_t *)aInfoValuePtr,
                                   aBufferLength,
                                   0);
            break;

        case SQL_SQL92_ROW_VALUE_CONSTRUCTOR: /* ODBC 3.0 */
            sSize = ulnGetInfoUInt((acp_uint32_t *)aInfoValuePtr,
                                   aBufferLength,
                                   SQL_SRVC_VALUE_EXPRESSION |
                                   SQL_SRVC_NULL |
                                   SQL_SRVC_DEFAULT |
                                   SQL_SRVC_ROW_SUBQUERY);
            break;

        case SQL_SQL92_STRING_FUNCTIONS: /* ODBC 3.0 */
            sSize = ulnGetInfoUInt((acp_uint32_t *)aInfoValuePtr,
                                   aBufferLength,
                                   SQL_SSF_UPPER |
                                   SQL_SSF_SUBSTRING |
                                   SQL_SSF_TRIM_BOTH |
                                   SQL_SSF_TRIM_LEADING |
                                   SQL_SSF_TRIM_TRAILING);
            break;

        case SQL_SQL92_VALUE_EXPRESSIONS: /* ODBC 3.0 */
            sSize = ulnGetInfoUInt((acp_uint32_t *)aInfoValuePtr,
                                   aBufferLength,
                                   0);
            break;

        case SQL_STANDARD_CLI_CONFORMANCE: /* ODBC 3.0 */
            sSize = ulnGetInfoUInt((acp_uint32_t *)aInfoValuePtr,
                                   aBufferLength,
                                   SQL_SCC_XOPEN_CLI_VERSION1 |
                                   SQL_SCC_ISO92_CLI);
            break;

        case SQL_STATIC_CURSOR_ATTRIBUTES1: /* ODBC 3.0 */
            sSize = ulnGetInfoUInt((acp_uint32_t *)aInfoValuePtr,
                                   aBufferLength,
                                   SQL_CA1_NEXT |
                                   SQL_CA1_ABSOLUTE |
                                   SQL_CA1_RELATIVE |
                                   SQL_CA1_BOOKMARK |
                                   SQL_CA1_POS_POSITION |
                                   SQL_CA1_POS_REFRESH |
                                   SQL_CA1_BULK_FETCH_BY_BOOKMARK );
            break;

        case SQL_STATIC_CURSOR_ATTRIBUTES2: /* ODBC 3.0 */
            sSize = ulnGetInfoUInt((acp_uint32_t *)aInfoValuePtr,
                                   aBufferLength,
                                   SQL_CA2_READ_ONLY_CONCURRENCY);
            break;

        case SQL_STATIC_SENSITIVITY: /* ODBC 2.0 */
            sSize = ulnGetInfoUInt((acp_uint32_t *)aInfoValuePtr,
                                   aBufferLength,
                                   0);
            break;

        case SQL_STRING_FUNCTIONS: /* ODBC 1.0 */
            sSize = ulnGetInfoUInt((acp_uint32_t *)aInfoValuePtr,
                                   aBufferLength,
                                   SQL_FN_STR_CHAR_LENGTH |
                                   SQL_FN_STR_CHARACTER_LENGTH |
                                   SQL_FN_STR_LCASE |
                                   SQL_FN_STR_LENGTH |
                                   SQL_FN_STR_LTRIM |
                                   SQL_FN_STR_OCTET_LENGTH |
                                   SQL_FN_STR_POSITION |
                                   SQL_FN_STR_RTRIM |
                                   SQL_FN_STR_SUBSTRING |
                                   SQL_FN_STR_UCASE);
            break;

        case SQL_SUBQUERIES: /* ODBC 2.0 */
            sSize = ulnGetInfoUInt((acp_uint32_t *)aInfoValuePtr,
                                   aBufferLength,
                                   SQL_SQ_CORRELATED_SUBQUERIES);
            break;

        case SQL_SYSTEM_FUNCTIONS: /* ODBC 1.0 */
            sSize = ulnGetInfoUInt((acp_uint32_t *)aInfoValuePtr,
                                   aBufferLength,
                                   0);
            break;

        case SQL_TABLE_TERM: /* ODBC 1.0 */
            sSize = ulnGetInfoText(&sFnContext, (acp_char_t *)aInfoValuePtr, aBufferLength, "table");
            break;

        case SQL_TIMEDATE_ADD_INTERVALS: /* ODBC 2.0 */
            sSize = ulnGetInfoUInt((acp_uint32_t *)aInfoValuePtr,
                                   aBufferLength,
                                   0);
            break;

        case SQL_TIMEDATE_DIFF_INTERVALS: /* ODBC 2.0 */
            sSize = ulnGetInfoUInt((acp_uint32_t *)aInfoValuePtr,
                                   aBufferLength,
                                   0);
            break;

        case SQL_TIMEDATE_FUNCTIONS: /* ODBC 1.0 */
            sSize = ulnGetInfoUInt((acp_uint32_t *)aInfoValuePtr,
                                   aBufferLength,
                                   SQL_FN_TD_EXTRACT);
            break;

        case SQL_TRANSACTION_CAPABLE: /* ODBC 1.0 */
            sSize = ulnGetInfoUShort((acp_uint16_t *)aInfoValuePtr,
                                     aBufferLength,
                                     SQL_TC_DDL_COMMIT);
            break;

        case SQL_TRANSACTION_ISOLATION_OPTION: /* ODBC 1.0 */
            sSize = ulnGetInfoUInt((acp_uint32_t *)aInfoValuePtr,
                                   aBufferLength,
                                   SQL_TXN_READ_COMMITTED |
                                   SQL_TXN_REPEATABLE_READ |
                                   SQL_TXN_SERIALIZABLE);
            break;

        case SQL_UNION: /* ODBC 2.0 */
            sSize = ulnGetInfoUInt((acp_uint32_t *)aInfoValuePtr,
                                   aBufferLength,
                                   SQL_U_UNION |
                                   SQL_U_UNION_ALL);
            break;

        case SQL_USER_NAME: /* ODBC 1.0 */
            sSize = ulnGetInfoText(&sFnContext, (acp_char_t *)aInfoValuePtr, aBufferLength, aDbc->mUserName);
            break;

        default:
            ACI_RAISE(LABEL_ABORT_INVALID_TYPE);
            break;
    }

    if(aStrLenPtr)
    {
        *aStrLenPtr = sSize;
    }

    sNeedExit = ACP_FALSE;
    ACI_TEST(ulnExit(&sFnContext) != ACI_SUCCESS);

    ULN_TRACE_LOG(&sFnContext, ULN_TRACELOG_MID, aInfoValuePtr, sSize,
            "%-18s| [%2"ACI_UINT32_FMT"] %2"ACI_UINT32_FMT":",
            "ulnGetInfo", aInfoType, sSize);

    return ULN_FNCONTEXT_GET_RC(&sFnContext);

    ACI_EXCEPTION(LABEL_ABORT_INVALID_TYPE)
    {
        /*
         * HY096: Information type out of range
         */    
        ulnError(&sFnContext, ulERR_ABORT_INVALID_INFO_TYPE, aInfoType);
    }
    ACI_EXCEPTION_END;

    if (sNeedExit == ACP_TRUE)
    {
        ulnExit(&sFnContext);
    }

    ULN_TRACE_LOG(&sFnContext, ULN_TRACELOG_LOW, aInfoValuePtr, sSize,
            "%-18s| [%3"ACI_UINT32_FMT"] fail", "ulnGetInfo", aInfoType);

    return ULN_FNCONTEXT_GET_RC(&sFnContext);
}


