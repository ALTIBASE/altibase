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
 * $Id$
 **********************************************************************/

#include <uln.h>
#include <ulnPrivate.h>

#include <ulsd.h>
#include <ulsdExtension.h>

#define CLONE_STRING        ("CLONE")

SQLRETURN ulsdInitializeShardKeyContext( ulsdShardKeyContext ** aCtx,
                                         ulnDbc               * aDbc,
                                         acp_char_t           * aTableName,
                                         acp_sint32_t           aTableNameLen,
                                         acp_char_t           * aKeyColumnName,
                                         acp_sint32_t           aKeyColumnNameLen,
                                         acp_sint16_t           aKeyColumnValueType,
                                         void                 * aKeyColumnValueBuffer,
                                         ulvSLen                aKeyColumnValueBufferLen,
                                         ulvSLen              * aKeyColumnStrLenOrIndPtr,
                                         acp_char_t             aShardDistType[1] )
{
    ulnFnContext      sFnContext;
    SQLHSTMT          sStmt              = NULL;;
    SQLRETURN         sRet;
    SQLSMALLINT       sSqlDataType       = SQL_UNKNOWN_TYPE;
    SQLULEN           sParamSize         = 0;
    SQLSMALLINT       sDecimalDigits     = 0;
    acp_char_t      * sQueryStmt[]       = { "SELECT ", " FROM ", " WHERE ", " = ?" };
    acp_sint32_t      sQueryStmtLen[]    = {         7,        6,         7,      4 };
    SQLINTEGER        sQueryLen          = 0;

    acp_sint32_t      sIndex             = 0;
    acp_sint32_t      sTableNameLen      = 0;
    acp_sint32_t      sKeyColumnNameLen  = 0;
    SQLCHAR           sShardDistType     = ULSD_SHARD_DIST_TYPE_UNKNOWN;

    SQLCHAR             * sQuery         = NULL;
    ulsdStmtContext     * sSmtCtx        = NULL;
    ulsdShardKeyContext * sContext       = NULL;

    enum sQueryStmtIdx {
        STMT_SELECT = 0,
        STMT___FROM ,
        STMT__WHERE ,
        STMT__EQUAL ,
    };

    ULN_INIT_FUNCTION_CONTEXT( sFnContext, ULN_FID_NONE, aDbc, ULN_OBJ_TYPE_DBC );

    ACI_TEST_RAISE( aDbc == NULL,
                    LABEL_INVALID_HANDLE );
    ACI_TEST_RAISE( aCtx == NULL,
                    LABEL_CONTEXT_IS_INVALID );
    ACI_TEST_RAISE( aTableName == NULL,
                    LABEL_TABLE_NAME_INVALID );
    ACI_TEST_RAISE( aKeyColumnName == NULL,
                    LABEL_COLUMN_NAME_INVALID );
    ACI_TEST_RAISE( ulnDbcIsConnected( aDbc ) == ACP_FALSE,
                    LABEL_CONNECTION_NOT_FOUND );

    ACI_TEST_RAISE( acpMemAlloc( (void **) & sContext,
                                 ACI_SIZEOF(ulsdShardKeyContext) )
                    != ACP_RC_SUCCESS,
                    LABEL_NOT_ENOUGH_MEMORY );

    sContext->ConnectionHandle = NULL;
    sContext->StatementHandle  = NULL;
    sContext->TableName        = NULL;
    sContext->TableNameLen     = 0;
    sContext->KeyColumnName    = NULL;
    sContext->KeyColumnNameLen = 0;
    sContext->Query            = NULL;
    sContext->ShardDistType    = ULSD_SHARD_DIST_TYPE_UNKNOWN;

    /* Query buffer processing */

    if ( aTableNameLen == SQL_NTS )
    {
        sTableNameLen = acpCStrLen( aTableName, ACP_SINT32_MAX );
        ACI_TEST_RAISE( sTableNameLen == ACP_SINT32_MAX,
                        LABEL_TABLE_NAME_LENGTH_INVALID );
    }
    else
    {
        sTableNameLen = aTableNameLen;
    }

    if ( aKeyColumnNameLen == SQL_NTS )
    {
        sKeyColumnNameLen = acpCStrLen( aKeyColumnName, ACP_SINT32_MAX );
        ACI_TEST_RAISE( sKeyColumnNameLen == ACP_SINT32_MAX,
                        LABEL_COLUMN_NAME_LENGTH_INVALID );
    }
    else
    {
        sKeyColumnNameLen = aKeyColumnNameLen;
    }

    sQueryLen += sQueryStmtLen[STMT_SELECT];      /* SELECT */
    sQueryLen += sKeyColumnNameLen;               /* @KEY */
    sQueryLen += sQueryStmtLen[STMT___FROM];      /* FROM */
    sQueryLen += sTableNameLen;                   /* @TABLE */
    sQueryLen += sQueryStmtLen[STMT__WHERE];      /* WHERE */
    sQueryLen += sKeyColumnNameLen;               /* @KEY */
    sQueryLen += sQueryStmtLen[STMT__EQUAL];      /* = ? */


    ACI_TEST_RAISE( acpMemAlloc( (void **) & sQuery,
                                 ( sQueryLen + 1 ) )
                    != ACP_RC_SUCCESS,
                    LABEL_NOT_ENOUGH_MEMORY );

    acpMemCpy( sQuery + sIndex, sQueryStmt[STMT_SELECT],  sQueryStmtLen[STMT_SELECT]  ); sIndex += sQueryStmtLen[STMT_SELECT];
    acpMemCpy( sQuery + sIndex, aKeyColumnName,           sKeyColumnNameLen );           sIndex += sKeyColumnNameLen;
    acpMemCpy( sQuery + sIndex, sQueryStmt[STMT___FROM],  sQueryStmtLen[STMT___FROM]  ); sIndex += sQueryStmtLen[STMT___FROM];
    acpMemCpy( sQuery + sIndex, aTableName,               sTableNameLen     );           sIndex += sTableNameLen;
    acpMemCpy( sQuery + sIndex, sQueryStmt[STMT__WHERE],  sQueryStmtLen[STMT__WHERE]  ); sIndex += sQueryStmtLen[STMT__WHERE];
    acpMemCpy( sQuery + sIndex, aKeyColumnName,           sKeyColumnNameLen );           sIndex += sKeyColumnNameLen;
    acpMemCpy( sQuery + sIndex, sQueryStmt[STMT__EQUAL],  sQueryStmtLen[STMT__EQUAL]  ); sIndex += sQueryStmtLen[STMT__EQUAL];
    sQuery[sIndex] = '\0';

    ACI_TEST_RAISE( sQueryLen != sIndex,
                    LABEL_QUERY_LENGTH_INVALID );

    /* using shardcli */

    sRet = SQLAllocStmt( aDbc, &sStmt );
    ACI_TEST( !SQL_SUCCEEDED( sRet ) );

    sSmtCtx = &( ( ulnStmt *)sStmt )->mShardStmtCxt;

    sRet = SQLPrepare( sStmt,
                       sQuery,
                       sQueryLen );
    ACI_TEST_RAISE( !SQL_SUCCEEDED( sRet ),
                    LABEL_PREPARE_FAIL );

    ACI_TEST_RAISE( sSmtCtx->mShardCoordQuery == ACP_TRUE,
                    LABEL_NON_SHARD_QUERY1 );

    ACI_TEST_RAISE( sSmtCtx->mShardIsShardQuery == ACP_FALSE,
                    LABEL_NON_SHARD_QUERY2 );

    switch ( sSmtCtx->mShardSplitMethod )
    {
        case ULN_SHARD_SPLIT_HASH  :
            sShardDistType = ULSD_SHARD_DIST_TYPE_HASH;
            break;
        case ULN_SHARD_SPLIT_RANGE :
            sShardDistType = ULSD_SHARD_DIST_TYPE_RANGE;
            break;
        case ULN_SHARD_SPLIT_LIST  :
            sShardDistType = ULSD_SHARD_DIST_TYPE_LIST;
            break;
        case ULN_SHARD_SPLIT_CLONE :
            sShardDistType = ULSD_SHARD_DIST_TYPE_CLONE;
            break;
        case ULN_SHARD_SPLIT_SOLO  :
            sShardDistType = ULSD_SHARD_DIST_TYPE_SOLO;
            break;
        default:
            /* Nothing to do */
            break;
    }

    ACI_TEST_RAISE( sShardDistType == ULSD_SHARD_DIST_TYPE_UNKNOWN,
                    LABEL_NOT_SUPPORT_SHARD_DIST_TYPE );

    if ( sShardDistType != ULSD_SHARD_DIST_TYPE_CLONE )
    {
        ACI_TEST_RAISE( aKeyColumnValueBuffer == NULL,
                        LABEL_COLUMN_VALUE_INVALID );

        sRet = SQLDescribeParam( sStmt,
                                 1,
                                 &sSqlDataType,
                                 &sParamSize,
                                 &sDecimalDigits,
                                 NULL );
        ACI_TEST_RAISE( !SQL_SUCCEEDED( sRet ),
                        LABEL_DESC_PARAM_FAIL );

        sRet = SQLBindParameter( sStmt,
                                 1,
                                 SQL_PARAM_INPUT,
                                 aKeyColumnValueType,
                                 sSqlDataType,
                                 sParamSize,
                                 sDecimalDigits,
                                 aKeyColumnValueBuffer,
                                 aKeyColumnValueBufferLen,
                                 aKeyColumnStrLenOrIndPtr );
        ACI_TEST_RAISE( !SQL_SUCCEEDED( sRet ),
                        LABEL_BIND_PARAM_FAIL );
    }
    
    sContext->ConnectionHandle = aDbc;
    sContext->StatementHandle  = sStmt;
    sContext->TableName        = aTableName;
    sContext->TableNameLen     = sTableNameLen;
    sContext->KeyColumnName    = aKeyColumnName;
    sContext->KeyColumnNameLen = sKeyColumnNameLen;
    sContext->Query            = sQuery;
    sContext->ShardDistType    = sShardDistType;

    *aShardDistType = sShardDistType;
    *aCtx = sContext;

    return SQL_SUCCESS;

    ACI_EXCEPTION( LABEL_INVALID_HANDLE )
    {
        ULN_FNCONTEXT_SET_RC( &sFnContext, SQL_INVALID_HANDLE );
    }
    ACI_EXCEPTION( LABEL_CONTEXT_IS_INVALID )
    {
        ulnError(&sFnContext, ulERR_ABORT_SHARD_INTERNAL_ERROR, "Shard key context is invalid.");
    }
    ACI_EXCEPTION( LABEL_TABLE_NAME_INVALID )
    {
        ulnError(&sFnContext, ulERR_ABORT_SHARD_INTERNAL_ERROR, "Table name is invalid.");
    }
    ACI_EXCEPTION( LABEL_COLUMN_NAME_INVALID )
    {
        ulnError(&sFnContext, ulERR_ABORT_SHARD_INTERNAL_ERROR, "Column name is invalid.");
    }
    ACI_EXCEPTION( LABEL_CONNECTION_NOT_FOUND )
    {
        ulnError(&sFnContext, ulERR_ABORT_SHARD_INTERNAL_ERROR, "Connection is not found.");
    }
    ACI_EXCEPTION( LABEL_TABLE_NAME_LENGTH_INVALID )
    {
        ulnError(&sFnContext, ulERR_ABORT_SHARD_INTERNAL_ERROR, "Table name length is invalid.");
    }
    ACI_EXCEPTION( LABEL_COLUMN_NAME_LENGTH_INVALID )
    {
        ulnError(&sFnContext, ulERR_ABORT_SHARD_INTERNAL_ERROR, "Column name length is invalid.");
    }
    ACI_EXCEPTION( LABEL_COLUMN_VALUE_INVALID )
    {
        ulnError(&sFnContext, ulERR_ABORT_SHARD_INTERNAL_ERROR, "Column value is invalid.");
    }
    ACI_EXCEPTION( LABEL_QUERY_LENGTH_INVALID )
    {
        ulnError(&sFnContext, ulERR_ABORT_SHARD_INTERNAL_ERROR, "Query buffer length is invalid.");
    }
    ACI_EXCEPTION( LABEL_NOT_ENOUGH_MEMORY )
    {
        ulnError(&sFnContext, ulERR_FATAL_MEMORY_ALLOC_ERROR, "ulsdGetNodeByShardKeyValue");
    }
    ACI_EXCEPTION( LABEL_PREPARE_FAIL )
    {
        ulnDiagRecMoveAll( (ulnObject *)aDbc, (ulnObject *)sStmt );

        ULN_FNCONTEXT_SET_RC( &sFnContext,
                              sRet );
    }
    ACI_EXCEPTION( LABEL_BIND_PARAM_FAIL )
    {
        ulnDiagRecMoveAll( (ulnObject *)aDbc, (ulnObject *)sStmt );

        ULN_FNCONTEXT_SET_RC( &sFnContext,
                              sRet );
    }
    ACI_EXCEPTION( LABEL_NON_SHARD_QUERY1 )
    {
        ulnError(&sFnContext, ulERR_ABORT_SHARD_INTERNAL_ERROR, "Check the settings of shard table. (coordinator)");
    }
    ACI_EXCEPTION( LABEL_NON_SHARD_QUERY2 )
    {
        ulnError(&sFnContext, ulERR_ABORT_SHARD_INTERNAL_ERROR, "Check the settings of shard table. (non-shard query)");
    }
    ACI_EXCEPTION( LABEL_DESC_PARAM_FAIL )
    {
        ulnDiagRecMoveAll( (ulnObject *)aDbc, (ulnObject *)sStmt );

        ULN_FNCONTEXT_SET_RC( &sFnContext,
                              sRet );
    }
    ACI_EXCEPTION( LABEL_NOT_SUPPORT_SHARD_DIST_TYPE )
    {
        ulnError(&sFnContext, ulERR_ABORT_SHARD_INTERNAL_ERROR, "Not supported shard distribution type.");
    }
    ACI_EXCEPTION_END;

    if ( sStmt != NULL )
    {
        (void) SQLFreeStmt( sStmt, SQL_DROP );
    }

    if ( sQuery != NULL )
    {
        acpMemFree( sQuery );
    }

    if ( sContext != NULL )
    {
        acpMemFree( sContext );
    }

    return ULN_FNCONTEXT_GET_RC( &sFnContext );
}

SQLRETURN ulsdGetNodeByShardKeyValue( ulsdShardKeyContext * aCtx,
                                      const acp_char_t   ** aDstNodeName )
{
    ulnFnContext      sFnContext;
    SQLHSTMT          sStmt;
    ulsdStmtContext * sSmtCtx;
    ulsdDbc         * sShard;
    SQLRETURN         sRet;
    acp_uint16_t      sNodeDbcIndex;

    ACI_TEST_RAISE( aCtx == NULL,
                    LABEL_CONTEXT_IS_INVALID );

    ULN_INIT_FUNCTION_CONTEXT( sFnContext, ULN_FID_NONE, aCtx->ConnectionHandle, ULN_OBJ_TYPE_DBC );

    ACI_TEST_RAISE( aCtx->ConnectionHandle == NULL,
                    LABEL_INVALID_HANDLE );
    ACI_TEST_RAISE( aDstNodeName == NULL,
                    LABEL_DST_NODE_NAME_INVALID );

    if ( aCtx->ShardDistType == ULSD_SHARD_DIST_TYPE_CLONE )
    {
        *aDstNodeName = CLONE_STRING;
    }
    else
    {
        sStmt   = aCtx->StatementHandle;
        sSmtCtx = &( ( ulnStmt *)sStmt )->mShardStmtCxt;

        ulsdGetShardFromDbc( aCtx->ConnectionHandle, &sShard );

        sRet = ulsdNodeDecideStmt( sStmt, ULN_FID_EXECUTE );
        ACI_TEST_RAISE( !SQL_SUCCEEDED( sRet ),
                        LABEL_NODE_DECIDE_FAIL );

        ACI_TEST_RAISE( sSmtCtx->mNodeDbcIndexCount != 1,
                        LABEL_MULTI_NODE_IS_NOT_VALID );

        sNodeDbcIndex = sSmtCtx->mNodeDbcIndexArr[0];

        *aDstNodeName = sShard->mNodeInfo[sNodeDbcIndex]->mNodeName;
    }

    return SQL_SUCCESS;

    ACI_EXCEPTION( LABEL_CONTEXT_IS_INVALID )
    {
        ULN_INIT_FUNCTION_CONTEXT( sFnContext, ULN_FID_NONE, NULL, ULN_OBJ_TYPE_DBC );

        ULN_FNCONTEXT_SET_RC( &sFnContext, SQL_INVALID_HANDLE );
    }
    ACI_EXCEPTION( LABEL_INVALID_HANDLE )
    {
        ULN_FNCONTEXT_SET_RC( &sFnContext, SQL_INVALID_HANDLE );
    }
    ACI_EXCEPTION( LABEL_DST_NODE_NAME_INVALID )
    {
        ulnError(&sFnContext, ulERR_ABORT_SHARD_INTERNAL_ERROR, "Destination node name is invalid.");
    }
    ACI_EXCEPTION( LABEL_NODE_DECIDE_FAIL )
    {
        ulnError(&sFnContext, ulERR_ABORT_SHARD_INTERNAL_ERROR, "To Decide Destination node fails.");
    }
    ACI_EXCEPTION( LABEL_MULTI_NODE_IS_NOT_VALID )
    {
        ulnError(&sFnContext, ulERR_ABORT_SHARD_INTERNAL_ERROR, "Destination node is not single.");
    }
    ACI_EXCEPTION_END;

    return ULN_FNCONTEXT_GET_RC( &sFnContext );
}

SQLRETURN ulsdFinalizeShardKeyContext( ulsdShardKeyContext * aCtx )
{
    ulnFnContext      sFnContext;

    ACI_TEST_RAISE( aCtx == NULL,
                    LABEL_CONTEXT_IS_INVALID );

    ULN_INIT_FUNCTION_CONTEXT( sFnContext, ULN_FID_NONE, aCtx->ConnectionHandle, ULN_OBJ_TYPE_DBC );

    ACI_TEST_RAISE( aCtx->ConnectionHandle == NULL,
                    LABEL_INVALID_HANDLE );

    if ( aCtx->Query != NULL )
    {
        acpMemFree( aCtx->Query );
        aCtx->Query = NULL;
    }

    if ( aCtx->StatementHandle != NULL )
    {
        (void) SQLFreeStmt( aCtx->StatementHandle, SQL_DROP );
        aCtx->StatementHandle = NULL;
    }

    acpMemFree( aCtx );
    aCtx = NULL;

    return SQL_SUCCESS;

    ACI_EXCEPTION( LABEL_CONTEXT_IS_INVALID )
    {
        ULN_INIT_FUNCTION_CONTEXT( sFnContext, ULN_FID_NONE, NULL, ULN_OBJ_TYPE_DBC );

        ULN_FNCONTEXT_SET_RC( &sFnContext, SQL_INVALID_HANDLE );
    }
    ACI_EXCEPTION( LABEL_INVALID_HANDLE )
    {
        ULN_FNCONTEXT_SET_RC( &sFnContext, SQL_INVALID_HANDLE );
    }
    ACI_EXCEPTION_END;

    return ULN_FNCONTEXT_GET_RC( &sFnContext );
}

