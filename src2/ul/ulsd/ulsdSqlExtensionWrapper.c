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
#include <ulnConfigFile.h>
#include <ulnPrivate.h>
#include <ulsdExtension.h>
#include <sqlcli.h>

#ifndef SQL_API
#define SQL_API
#endif

/*
 * PROJ-2739 Client-side Sharding LOB
 *
 * ======================
 * LOB 과 관련된 함수들.
 * ======================
 */

SQLRETURN SQL_API SQLBindFileToCol(SQLHSTMT     aHandle,
                                   SQLSMALLINT  aColumnNumber,
                                   SQLCHAR     *aFileNameArray,
                                   SQLLEN      *aFileNameLengthArray,
                                   SQLUINTEGER *aFileOptionsArray,
                                   SQLLEN       aMaxFileNameLength,
                                   SQLLEN      *aLenOrIndPtr)
{
    ULN_TRACE(SQLBindFileToCol);

    return ulsdNodeBindCol( (ulnStmt *)aHandle,
                            (acp_sint16_t  )aColumnNumber,
                            (acp_sint16_t  )SQL_C_FILE,
                            (acp_char_t   *)aFileNameArray,
                            (ulvSLen  )aMaxFileNameLength,
                            (ulvSLen *)aLenOrIndPtr,
                            (ulvSLen      *)aFileNameLengthArray,
                            (acp_uint32_t *)aFileOptionsArray );
}

SQLRETURN SQL_API SQLBindFileToParam(SQLHSTMT     aHandle,
                                     SQLSMALLINT  aParameterNumber,
                                     SQLSMALLINT  aSqlType,
                                     SQLCHAR     *aFileNameArray,
                                     SQLLEN      *aFileNameLengthArray,
                                     SQLUINTEGER *aFileOptionsArray,
                                     SQLLEN       aMaxFileNameLength,
                                     SQLLEN      *aLenOrIndPtr)
{
    ULN_TRACE(SQLBindFileToParam);

    return ulsdNodeBindParameter((ulnStmt      *)aHandle,
                                 (acp_uint16_t  )aParameterNumber,
                                 (acp_char_t   *)NULL, // ParamName
                                 (acp_sint16_t  )SQL_PARAM_INPUT,
                                 (acp_sint16_t  )SQL_C_FILE,
                                 (acp_sint16_t  )aSqlType,
                                 (ulvULen       )0, // ColumnSize
                                 (acp_sint16_t  )0, // DecimalDigits
                                 (acp_char_t   *)aFileNameArray,
                                 (ulvSLen       )aMaxFileNameLength,
                                 (ulvSLen      *)aLenOrIndPtr,
                                 (ulvSLen      *)aFileNameLengthArray,
                                 (acp_uint32_t *)aFileOptionsArray);
}

SQLRETURN SQL_API SQLGetLobLength(SQLHSTMT     aHandle,
                                  SQLUBIGINT   aLocator,
                                  SQLSMALLINT  aLocatorCType,
                                  SQLUINTEGER *aLobLengthPtr)
{
    ULN_TRACE(SQLGetLobLength);

    return ulsdGetLobLength((ulnStmt      *)aHandle,
                            (acp_uint64_t  )aLocator,
                            (acp_sint16_t  )aLocatorCType,
                            (acp_uint32_t *)aLobLengthPtr);
}

SQLRETURN SQL_API SQLPutLob(SQLHSTMT    aHandle,
                            SQLSMALLINT aLocatorCType,
                            SQLUBIGINT  aLocator,
                            SQLUINTEGER aStartOffset,
                            SQLUINTEGER aSizeToBeUpdated,
                            SQLSMALLINT aSourceCType,
                            SQLPOINTER  aDataToPut,
                            SQLUINTEGER aSizeDataToPut)
{
    ULN_TRACE(SQLPutLob);

    return ulsdPutLob((ulnStmt    *)aHandle,
                      (acp_sint16_t)aLocatorCType,
                      (acp_uint64_t)aLocator,
                      (acp_uint32_t)aStartOffset,
                      (acp_uint32_t)aSizeToBeUpdated,
                      (acp_sint16_t)aSourceCType,
                      (void       *)aDataToPut,
                      (acp_uint32_t)aSizeDataToPut);
}

SQLRETURN SQL_API SQLGetLob(SQLHSTMT     aHandle,
                            SQLSMALLINT  aLocatorCType,
                            SQLUBIGINT   aLocator,
                            SQLUINTEGER  aStartOffset,
                            SQLUINTEGER  aSizeToGet,
                            SQLSMALLINT  aTargetCType,
                            SQLPOINTER   aBufferToStoreData,
                            SQLUINTEGER  aSizeBuffer,
                            SQLUINTEGER *aSizeReadPtr)
{
    ULN_TRACE(SQLGetLob);

    return ulsdGetLob((ulnStmt      *)aHandle,
                      (acp_sint16_t  )aLocatorCType,
                      (acp_uint64_t  )aLocator,
                      (acp_uint32_t  )aStartOffset,
                      (acp_uint32_t  )aSizeToGet,
                      (acp_sint16_t  )aTargetCType,
                      (void         *)aBufferToStoreData,
                      (acp_uint32_t  )aSizeBuffer,
                      (acp_uint32_t *)aSizeReadPtr);
}

SQLRETURN SQL_API SQLFreeLob(SQLHSTMT aHandle, SQLUBIGINT aLocator)
{
    ULN_TRACE(SQLFreeLob);

    return ulsdFreeLob((ulnStmt *)aHandle, aLocator);
}

SQLRETURN SQL_API SQLTrimLob(SQLHSTMT     aHandle,
                             SQLSMALLINT  aLocatorCType,
                             SQLUBIGINT   aLocator,
                             SQLUINTEGER  aStartOffset)
{
    ULN_TRACE(SQLTrimLob);

    return ulsdTrimLob((ulnStmt    *)aHandle,
                       (acp_sint16_t)aLocatorCType,
                       (acp_uint64_t)aLocator,
                       (acp_uint32_t)aStartOffset);
}


/*
 * ===============================
 * 비표준이지만, 필요한 함수들
 * ===============================
 */

SQLRETURN SQL_API SQLGetPlan(SQLHSTMT aStmt, SQLCHAR **aPlan)
{
    SQLRETURN    sRet = SQL_ERROR;
    ulnFnContext sFnContext;

    ULN_TRACE(SQLGetPlan);

    ULN_INIT_FUNCTION_CONTEXT(sFnContext, ULN_FID_GETPLAN, aStmt, ULN_OBJ_TYPE_STMT);

    ACI_TEST_RAISE(aStmt == SQL_NULL_HANDLE, InvalidHandle);

    if ( ( ((ulnStmt *)aStmt)->mShardStmtCxt.mShardCoordQuery ) == ACP_TRUE )
    {
        sRet = ulnGetPlan((ulnStmt *)aStmt, (acp_char_t **)aPlan);
    }
    else
    {
        ACI_TEST(ulnClearDiagnosticInfoFromObject((ulnObject*)aStmt) != ACI_SUCCESS);
        ACI_RAISE(NotSupported);
    }
    return sRet;

    ACI_EXCEPTION(InvalidHandle)
    {
        ulnError(&sFnContext, ulERR_ABORT_INVALID_HANDLE);
        sRet = SQL_INVALID_HANDLE;
    }
    ACI_EXCEPTION(NotSupported)
    {
        ulnError(&sFnContext, ulERR_ABORT_SHARD_UNSUPPORTED_FUNCTION, "SQLGetPlan of shard-query");
    }
    ACI_EXCEPTION_END;

    return sRet;
}

//PROJ-1645 UL-FailOver.
void  SQL_API   SQLDumpDataSources()
{
    ulnConfigFileDump();
}

/* BUG-28793 */
void  SQL_API   SQLDumpDataSourceFromName(SQLCHAR *aDataSourceName)
{
    ulnConfigFileDumpFromName((acp_char_t*)aDataSourceName);
}


/* 
 * PROJ-1721 Name-based Binding
 *
 * 함수 이름에 Beta를 붙인 건 추후 CLI에서 아래 함수가 정식으로 추가될
 * 가능성을 염두했기 때문이며 SQLBindParameterByName은 예약해 둔다.
 * 현재 ADO.net 드라이버에서만 호출된다.
 *
 * @ParameterName : NULL이 아닌 경우 '\0' 문자로 반드시 끝나야 한다.
 */
#if (ODBCVER >= 0x0300)
SQLRETURN SQL_API SQLBindParameterByNameBeta(SQLHSTMT     StatementHandle,
                                             SQLCHAR     *ParameterName,
                                             SQLUSMALLINT ParameterNumber,
                                             SQLSMALLINT  InputOutputType,
                                             SQLSMALLINT  ValueType,
                                             SQLSMALLINT  ParameterType,
                                             SQLULEN      ColumnSize,
                                             SQLSMALLINT  DecimalDigits,
                                             SQLPOINTER   ParameterValuePtr,
                                             SQLLEN       BufferLength,
                                             SQLLEN      *StrLen_or_IndPtr)
{
    ULN_TRACE(SQLBindParameterByNameBeta);

    return ulnBindParameter((ulnStmt *)StatementHandle,
                            (acp_uint16_t  )ParameterNumber,
                            (acp_char_t   *)ParameterName,
                            (acp_sint16_t  )InputOutputType,
                            (acp_sint16_t  )ValueType,
                            (acp_sint16_t  )ParameterType,
                            (ulvULen  )ColumnSize,
                            (acp_sint16_t  )DecimalDigits,
                            (void    *)ParameterValuePtr,
                            (ulvSLen  )BufferLength,
                            (ulvSLen *)StrLen_or_IndPtr);
}
#endif

/* 
 * PROJ-1721 Name-based Binding
 *
 * CLI에서 Name-based Binding을 지원하면 이 함수는 불필요하다.
 * 현재 ADO.net 드라이버에서만 호출되며 ADO.net 드라이버 외에서
 * 호출된 경우 Statement를 불필요하게 분석하지 않기 위해 추가한다.
 *
 * @AnalyzeText : NULL이 아닌 경우 '\0' 문자로 반드시 끝나야 한다.
 *                NULL이면 StatementText를 분석하지 않는다.
 */
SQLRETURN SQL_API SQLPrepareByName(SQLHSTMT   StatementHandle,
                                   SQLCHAR   *StatementText,
                                   SQLINTEGER TextLength,
                                   SQLCHAR   *AnalyzeText)
{
    ULN_TRACE(SQLPrepareByName);
    return ulnPrepare((ulnStmt *)StatementHandle,
                      (acp_char_t *)StatementText,
                      (acp_sint32_t)TextLength,
                      (acp_char_t *)AnalyzeText);
}

SQLRETURN SQL_API SQLInitializeShardKeyContext( SQLShardKeyContext * ShardKeyContext,
                                                SQLHDBC              ConnectionHandle,
                                                SQLCHAR            * TableName,
                                                SQLINTEGER           TableNameLen,
                                                SQLCHAR            * KeyColumnName,
                                                SQLINTEGER           KeyColumnNameLen,
                                                SQLSMALLINT          KeyColumnValueType,
                                                SQLPOINTER           KeyColumnValueBuffer,
                                                SQLLEN               KeyColumnValueBufferLen,
                                                SQLLEN             * KeyColumnStrLenOrIndPtr,
                                                SQLCHAR              ShardDistType[1] )
{
    return ulsdInitializeShardKeyContext( (ulsdShardKeyContext **)ShardKeyContext,
                                          ConnectionHandle,
                                          (acp_char_t *)TableName,
                                          TableNameLen,
                                          (acp_char_t *)KeyColumnName,
                                          KeyColumnNameLen,
                                          KeyColumnValueType,
                                          (acp_char_t *)KeyColumnValueBuffer,
                                          KeyColumnValueBufferLen,
                                          KeyColumnStrLenOrIndPtr,
                                          (acp_char_t *)ShardDistType );
}

SQLRETURN SQL_API SQLGetNodeByShardKeyValue( SQLShardKeyContext     ShardKeyContext,
                                             const SQLCHAR       ** DstNodeName )
{
    return ulsdGetNodeByShardKeyValue( (ulsdShardKeyContext *)ShardKeyContext,
                                       (const acp_char_t **)DstNodeName );
}

SQLRETURN SQL_API SQLFinalizeShardKeyContext( SQLShardKeyContext ShardKeyContext )
{
    return ulsdFinalizeShardKeyContext( (ulsdShardKeyContext *)ShardKeyContext );
}

