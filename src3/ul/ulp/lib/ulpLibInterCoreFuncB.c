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

#include <ulpLibInterCoreFuncB.h>

ACI_RC ulpSetStmtAttrParamCore( ulpLibConnNode *aConnNode,
                                ulpLibStmtNode *aStmtNode,
                                SQLHSTMT       *aHstmt,
                                ulpSqlstmt     *aSqlstmt,
                                ulpSqlca       *aSqlca,
                                ulpSqlcode     *aSqlcode,
                                ulpSqlstate    *aSqlstate)
{
/***********************************************************************
 *
 * Description :
 *    row / column wise binding을 위한 statement 속성 설정.
 *
 * Implementation :
 *
 ***********************************************************************/

    SQLRETURN sSqlRes;

    if ( aSqlstmt->isarr == 1 )
    {
        if ( aSqlstmt->isstruct == 1 )
        {
            sSqlRes = SQLSetStmtAttr( *aHstmt,
                                      SQL_ATTR_PARAM_BIND_TYPE,
                                      (SQLPOINTER)(acp_slong_t)aSqlstmt->structsize,
                                      0 );

            ACI_TEST_RAISE( ( sSqlRes == SQL_ERROR ) || ( sSqlRes == SQL_INVALID_HANDLE )
                            , ERR_CLI_SETSTMT );
        }
        else
        {
            sSqlRes = SQLSetStmtAttr( *aHstmt,
                                      SQL_ATTR_PARAM_BIND_TYPE,
                                      (SQLPOINTER)SQL_BIND_BY_COLUMN,
                                      0 );

            ACI_TEST_RAISE( ( sSqlRes == SQL_ERROR ) || ( sSqlRes == SQL_INVALID_HANDLE )
                            , ERR_CLI_SETSTMT );
        }

        /* BUG-45779 PSM Array Type을 사용할때는 SQL_ATTR_PARAMSET_SIZE 값이 1을 초과 해서는 안된다 */
        if( (aStmtNode != NULL) &&
            (aSqlstmt->stmttype == S_DirectPSM) &&
            (aStmtNode->mUlpPSMArrInfo.mIsPSMArray == ACP_TRUE) )
        {
            sSqlRes = SQLSetStmtAttr( *aHstmt,
                                       SQL_ATTR_PARAMSET_SIZE,
                                       (SQLPOINTER)1,
                                       0 );
        }
        else
        {
            sSqlRes = SQLSetStmtAttr( *aHstmt,
                                       SQL_ATTR_PARAMSET_SIZE,
                                       (SQLPOINTER)(acp_slong_t)aSqlstmt->arrsize,
                                       0 );
        }

        ACI_TEST_RAISE( ( sSqlRes == SQL_ERROR ) || ( sSqlRes == SQL_INVALID_HANDLE )
                        , ERR_CLI_SETSTMT );
    }
    else
    {
        sSqlRes = SQLSetStmtAttr( *aHstmt,
                                  SQL_ATTR_PARAM_BIND_TYPE,
                                  (SQLPOINTER)SQL_BIND_BY_COLUMN,
                                  0 );

        ACI_TEST_RAISE( ( sSqlRes == SQL_ERROR ) || ( sSqlRes == SQL_INVALID_HANDLE )
                        , ERR_CLI_SETSTMT );

        sSqlRes = SQLSetStmtAttr( *aHstmt,
                                  SQL_ATTR_PARAMSET_SIZE,
                                  (SQLPOINTER)1,
                                  0 );

        ACI_TEST_RAISE( ( sSqlRes == SQL_ERROR ) || ( sSqlRes == SQL_INVALID_HANDLE )
                        , ERR_CLI_SETSTMT );
    }

    if( aSqlstmt->isatomic == 1 /*TRUE*/ )
    {
        sSqlRes = SQLSetStmtAttr( *aHstmt,
                                  ALTIBASE_STMT_ATTR_ATOMIC_ARRAY,
                                  (SQLPOINTER) SQL_TRUE,
                                  0 );

        ACI_TEST_RAISE( ( sSqlRes == SQL_ERROR ) || ( sSqlRes == SQL_INVALID_HANDLE )
                        , ERR_CLI_SETSTMT );
    }
    else
    {
        sSqlRes = SQLSetStmtAttr( *aHstmt,
                                  ALTIBASE_STMT_ATTR_ATOMIC_ARRAY,
                                  (SQLPOINTER) SQL_FALSE,
                                  0 );

        ACI_TEST_RAISE( ( sSqlRes == SQL_ERROR ) || ( sSqlRes == SQL_INVALID_HANDLE )
                        , ERR_CLI_SETSTMT );
    }
/*
    sSqlRes = SQLSetStmtAttr( *aHstmt,
                              SQL_ATTR_PARAM_STATUS_PTR,
                              (SQLPOINTER) aSqlstmt -> statusptr,
                              0 );

    ACI_TEST_RAISE( ( sSqlRes == SQL_ERROR ) || ( sSqlRes == SQL_INVALID_HANDLE )
                    , ERR_CLI_SETSTMT );
*/
    /* stmt Node에 stmt속성 정보 set*/
    if ( aStmtNode != NULL )
    {
        aStmtNode -> mBindInfo.mIsArray    = (acp_bool_t) aSqlstmt -> isarr ;
        aStmtNode -> mBindInfo.mArraySize  =
                (aSqlstmt -> isarr == 1)?aSqlstmt -> arrsize : 0;
        aStmtNode -> mBindInfo.mIsStruct   = (acp_bool_t) aSqlstmt -> isstruct ;
        aStmtNode -> mBindInfo.mStructSize =
                (aSqlstmt -> isstruct == 1)?aSqlstmt -> structsize : 0;
        aStmtNode -> mBindInfo.mIsAtomic   = (acp_bool_t) aSqlstmt -> isatomic ;
        /*aStmtNode -> mStatusPtr            = (void*) aSqlstmt -> statusptr ;*/
        /* Statement 상태를 SETSTMTATTR로 변경함.*/
        aStmtNode -> mStmtState = S_SETSTMTATTR;
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION ( ERR_CLI_SETSTMT );
    {
        ulpSetErrorInfo4CLI( aConnNode,
                             *aHstmt,
                             SQL_ERROR,
                             aSqlca,
                             aSqlcode,
                             aSqlstate,
                             ERR_TYPE2 );

    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;

}


ACI_RC ulpSetStmtAttrRowCore(   ulpLibConnNode *aConnNode,
                                ulpLibStmtNode *aStmtNode,
                                SQLHSTMT    *aHstmt,
                                ulpSqlstmt  *aSqlstmt,
                                ulpSqlca    *aSqlca,
                                ulpSqlcode  *aSqlcode,
                                ulpSqlstate *aSqlstate )
{
/***********************************************************************
 *
 * Description :
 *  array fetch를 위한 statement 속성 설정.
 *
 * Implementation :
 *
 ***********************************************************************/

    SQLRETURN sSqlRes;

    if ( aSqlstmt->isarr == 1 )
    {
        if ( aSqlstmt->isstruct == 1 )
        {
            sSqlRes = SQLSetStmtAttr( *aHstmt,
                                      SQL_ATTR_ROW_BIND_TYPE,
                                      (SQLPOINTER)(acp_slong_t)aSqlstmt->structsize,
                                      0 );

            ACI_TEST_RAISE( ( sSqlRes == SQL_ERROR ) || ( sSqlRes == SQL_INVALID_HANDLE )
                            , ERR_CLI_SETSTMT );
        }
        else
        {
            sSqlRes = SQLSetStmtAttr( *aHstmt,
                                      SQL_ATTR_ROW_BIND_TYPE,
                                      (SQLPOINTER)SQL_BIND_BY_COLUMN,
                                      0 );

            ACI_TEST_RAISE( ( sSqlRes == SQL_ERROR ) || ( sSqlRes == SQL_INVALID_HANDLE )
                            , ERR_CLI_SETSTMT );
        }

        sSqlRes = SQLSetStmtAttr( *aHstmt,
                                  SQL_ATTR_ROW_ARRAY_SIZE,
                                  (SQLPOINTER)(acp_slong_t)aSqlstmt->arrsize,
                                  0 );

        ACI_TEST_RAISE( ( sSqlRes == SQL_ERROR ) || ( sSqlRes == SQL_INVALID_HANDLE )
                        , ERR_CLI_SETSTMT );

    }
    else
    {
        sSqlRes = SQLSetStmtAttr( *aHstmt,
                                  SQL_ATTR_ROW_BIND_TYPE,
                                  (SQLPOINTER)SQL_BIND_BY_COLUMN,
                                  0 );

        ACI_TEST_RAISE( ( sSqlRes == SQL_ERROR ) || ( sSqlRes == SQL_INVALID_HANDLE )
                        , ERR_CLI_SETSTMT );

        sSqlRes = SQLSetStmtAttr( *aHstmt,
                                  SQL_ATTR_ROW_ARRAY_SIZE,
                                  (SQLPOINTER)1,
                                  0 );

        ACI_TEST_RAISE( ( sSqlRes == SQL_ERROR ) || ( sSqlRes == SQL_INVALID_HANDLE )
                        , ERR_CLI_SETSTMT );
    }

    sSqlRes = SQLSetStmtAttr( *aHstmt,
                               SQL_ATTR_ROW_STATUS_PTR,
                               NULL,
                               0 );

    ACI_TEST_RAISE( ( sSqlRes == SQL_ERROR ) || ( sSqlRes == SQL_INVALID_HANDLE )
                    , ERR_CLI_SETSTMT );

    if ( aStmtNode != NULL )
    {
        aStmtNode -> mBindInfo.mIsArray    = (acp_bool_t) aSqlstmt -> isarr ;
        aStmtNode -> mBindInfo.mArraySize  =
                                 (aSqlstmt -> isarr == 1)?aSqlstmt -> arrsize : 0;
        aStmtNode -> mBindInfo.mIsStruct   = (acp_bool_t) aSqlstmt -> isstruct ;
        aStmtNode -> mBindInfo.mStructSize =
                                 (aSqlstmt -> isstruct == 1)?aSqlstmt -> structsize : 0;
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION ( ERR_CLI_SETSTMT );
    {
        ulpSetErrorInfo4CLI( aConnNode,
                             *aHstmt,
                             SQL_ERROR,
                             aSqlca,
                             aSqlcode,
                             aSqlstate,
                             ERR_TYPE2 );

    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;

}

/**************************************************************************
 * Description : dynamic sql bind를 위한 statement 설정
 *               array bind은 따로 설정한다.
 *
 **************************************************************************/
ACI_RC ulpSetStmtAttrDynamicParamCore( ulpLibConnNode *aConnNode,
                                       ulpLibStmtNode *aStmtNode,
                                       SQLHSTMT       *aHstmt,
                                       ulpSqlca       *aSqlca,
                                       ulpSqlcode     *aSqlcode,
                                       ulpSqlstate    *aSqlstate )
{

    SQLRETURN sSqlRes;

    sSqlRes = SQLSetStmtAttr( *aHstmt,
                              SQL_ATTR_ROW_BIND_TYPE,
                              (SQLPOINTER) SQL_BIND_BY_COLUMN,
                              0 );
    ACI_TEST_RAISE( (sSqlRes == SQL_ERROR) || (sSqlRes == SQL_INVALID_HANDLE),
                    ERR_CLI_SETSTMT );
    
    if ( aStmtNode != NULL )
    {
        aStmtNode->mBindInfo.mIsStruct = ACP_FALSE;
        aStmtNode->mStmtState = S_SETSTMTATTR;
    }
    else
    {
        /* do nothing */
    }
    
    return ACI_SUCCESS;

    ACI_EXCEPTION( ERR_CLI_SETSTMT )
    {
        (void) ulpSetErrorInfo4CLI( aConnNode,
                                    *aHstmt,
                                    (SQLRETURN)SQL_ERROR,
                                    aSqlca,
                                    aSqlcode,
                                    aSqlstate,
                                    ERR_TYPE2 );
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

/**************************************************************************
 * Description : dynamic sql fetch를 위한 statement 설정
 *
 **************************************************************************/
ACI_RC ulpSetStmtAttrDynamicRowCore( ulpLibConnNode *aConnNode,
                                     ulpLibStmtNode *aStmtNode,
                                     SQLHSTMT       *aHstmt,
                                     ulpSqlca       *aSqlca,
                                     ulpSqlcode     *aSqlcode,
                                     ulpSqlstate    *aSqlstate )
{
    SQLRETURN sSqlRes;

    sSqlRes = SQLSetStmtAttr( *aHstmt,
                              SQL_ATTR_ROW_BIND_TYPE,
                              (SQLPOINTER) SQL_BIND_BY_COLUMN,
                              0 );
    ACI_TEST_RAISE( (sSqlRes == SQL_ERROR) || (sSqlRes == SQL_INVALID_HANDLE),
                    ERR_CLI_SETSTMT );
    
    sSqlRes = SQLSetStmtAttr( *aHstmt,
                              SQL_ATTR_ROW_ARRAY_SIZE,
                              (SQLPOINTER) 1,
                              0 );
    ACI_TEST_RAISE( (sSqlRes == SQL_ERROR) || (sSqlRes == SQL_INVALID_HANDLE),
                    ERR_CLI_SETSTMT );

    sSqlRes = SQLSetStmtAttr( *aHstmt,
                              SQL_ATTR_ROW_STATUS_PTR,
                              NULL,
                              0 );
    ACI_TEST_RAISE( (sSqlRes == SQL_ERROR) || (sSqlRes == SQL_INVALID_HANDLE),
                    ERR_CLI_SETSTMT );

    if ( aStmtNode != NULL )
    {
        aStmtNode->mBindInfo.mIsArray = ACP_FALSE;
        aStmtNode->mBindInfo.mArraySize = 0;
        aStmtNode->mBindInfo.mIsStruct = ACP_FALSE;
    }
    else
    {
        /* do nothing */
    }
    
    return ACI_SUCCESS;

    ACI_EXCEPTION( ERR_CLI_SETSTMT )
    {
        (void) ulpSetErrorInfo4CLI( aConnNode,
                                    *aHstmt,
                                    (SQLRETURN)SQL_ERROR,
                                    aSqlca,
                                    aSqlcode,
                                    aSqlstate,
                                    ERR_TYPE2 );
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}


ACI_RC ulpBindParamCore( ulpLibConnNode *aConnNode,
                         ulpLibStmtNode *aStmtNode,
                         SQLHSTMT       *aHstmt,
                         ulpSqlstmt     *aSqlstmt,
                         ulpHostVar     *aHostVar,
                         acp_sint16_t    aIndex,
                         ulpSqlca       *aSqlca,
                         ulpSqlcode     *aSqlcode,
                         ulpSqlstate    *aSqlstate,
                         SQLSMALLINT     aInOut )
{
/***********************************************************************
 *
 * Description :
 *    in/out type host 변수에 대한 bindparam을 처리한다.
 *
 * Implementation :
 *
 ***********************************************************************/

    SQLRETURN   sSqlRes;
    SQLSMALLINT sCType;
    SQLSMALLINT sSqlType;
    SQLULEN     sPrecision;
    acp_bool_t  sFixedInd;

    /* BUG-45779 */
    ulpPSMArrDataInfo   *sUlpPSMArrDataInfo     = NULL;
    acp_list_node_t     *sUlpPSMArrDataInfoNode = NULL;
    SQLULEN              sPSMArraySize          = 0;

    acp_uint32_t         sMaxlen                = 0;

    sPrecision = 0;
    sFixedInd  = ACP_FALSE;
    sCType     = sSqlType = SQL_UNKNOWN_TYPE;
    sMaxlen    = aHostVar->mMaxlen;

    if ( aHostVar->mType == H_BLOB_FILE )
    {
        sSqlRes = SQLBindFileToParam( *aHstmt,
                                      (SQLSMALLINT)aIndex,
                                      SQL_BLOB,
                                      (SQLCHAR *)aHostVar->mHostVar,
                                      NULL,
                                      aHostVar->mFileopt,
                                      sMaxlen,
                                      aHostVar->mHostInd );

        ACI_TEST_RAISE( (sSqlRes == SQL_ERROR) || (sSqlRes == SQL_INVALID_HANDLE)
                        , ERR_CLI_BINDFILEPARAM );
    }
    else if ( aHostVar->mType == H_CLOB_FILE )
    {

        sSqlRes = SQLBindFileToParam( *aHstmt,
                                      (SQLSMALLINT)aIndex,
                                      SQL_CLOB,
                                      (SQLCHAR *)aHostVar->mHostVar,
                                      NULL,
                                      aHostVar->mFileopt,
                                      sMaxlen,
                                      aHostVar->mHostInd );

        ACI_TEST_RAISE( (sSqlRes == SQL_ERROR) || (sSqlRes == SQL_INVALID_HANDLE)
                        , ERR_CLI_BINDFILEPARAM );
    }
    else if( aStmtNode != NULL &&
             aHostVar->isarr == 1 &&                              /* host 변수가 array이며 */
             aStmtNode->mUlpPSMArrInfo.mIsPSMArray == ACP_TRUE )  /* psm이 array type의 인자로 정의되어 있으며 */
    {
        ACI_TEST( acpMemAlloc((void**)&sUlpPSMArrDataInfo, sizeof(ulpPSMArrDataInfo)) != ACP_RC_SUCCESS);

        sPSMArraySize = sizeof(psmArrayMetaInfo) + (aHostVar->arrsize * aHostVar->mSizeof);
        ACI_TEST( acpMemAlloc((void**)&sUlpPSMArrDataInfo->mData, sPSMArraySize) != ACP_RC_SUCCESS);

        /* BUG-45779 PSM Array Bind */
        sSqlRes = SQLBindParameter( *aHstmt,
                                    (SQLUSMALLINT)aIndex,
                                     aInOut,
                                     SQL_C_BINARY,
                                     SQL_BINARY,
                                     sPSMArraySize, /* META + host variable size */
                                     0,
                                     sUlpPSMArrDataInfo->mData,
                                     sPSMArraySize, /* META + host variable size */
                                     NULL );

        ACI_TEST_RAISE( (sSqlRes == SQL_ERROR) || (sSqlRes == SQL_INVALID_HANDLE), ERR_CLI_BINDPARAM );

        /* BUG-45779 Bind 성공한 HostVar정보를 저장 */
        sUlpPSMArrDataInfo->mUlpHostVar = aHostVar;
        ACI_TEST( acpMemAlloc((void**)&sUlpPSMArrDataInfoNode, sizeof(acp_list_node_t)) != ACP_RC_SUCCESS);

        acpListInitObj(sUlpPSMArrDataInfoNode, sUlpPSMArrDataInfo);
        acpListAppendNode(&aStmtNode->mUlpPSMArrInfo.mPSMArrDataInfoList, sUlpPSMArrDataInfoNode);

    }
    else
    {
        switch ( aHostVar->mType )
        {
            case H_NUMERIC :
                sSqlType = SQL_NUMERIC;
                sCType   = SQL_C_CHAR;
                if ( aHostVar->mLen > 0 )
                {
                    sPrecision = (SQLULEN)( aHostVar->mLen - 1 );
                }
                else
                {
                    sPrecision = 1;
                }
                break;
            /* BUG-45933 */
            case H_NUMERIC_STRUCT :
                sSqlType = SQL_NUMERIC;
                sCType   = SQL_C_BINARY;
                sPrecision = 0;
                sMaxlen = sizeof(SQL_NUMERIC_STRUCT);
                break;
            case H_INTEGER :
                sSqlType = SQL_INTEGER;
                sCType   = SQL_C_SLONG;
                sPrecision = 0;
                break;
            case H_INT :
                if ( aHostVar->mUnsign == 1 )
                {
                    sSqlType = SQL_BIGINT;
                    if (aHostVar->mSizeof == 2)
                    {
                        sCType = SQL_C_USHORT;
                    }
                    else if (aHostVar->mSizeof == 4)
                    {
                        sCType = SQL_C_ULONG;
                    }
                    else if (aHostVar->mSizeof == 8) 
                    {
                        sCType = SQL_C_UBIGINT;
                    }
                }
                else
                {
                    sSqlType = SQL_INTEGER;
                    if (aHostVar->mSizeof == 2)
                    {
                        sCType = SQL_C_SSHORT;
                    }
                    else if (aHostVar->mSizeof == 4) 
                    {
                        sCType = SQL_C_SLONG;
                    }
                    else if (aHostVar->mSizeof == 8) 
                    {
                        sCType = SQL_C_SBIGINT;
                    }
                }
                sPrecision = 0;
                break;
            case H_LONG  :
            case H_LONGLONG:
                sSqlType = SQL_BIGINT;
                if ( aHostVar->mUnsign == 1 )
                {
                    if (aHostVar->mSizeof == 2)
                    {
                        sCType = SQL_C_USHORT;
                    }
                    else if (aHostVar->mSizeof == 4)
                    {
                        sCType = SQL_C_ULONG;
                    }
                    else if (aHostVar->mSizeof == 8)
                    {
                        sCType = SQL_C_UBIGINT;
                    }
                }
                else
                {
                    if (aHostVar->mSizeof == 2)
                    {
                        sCType = SQL_C_SSHORT;
                    }
                    else if (aHostVar->mSizeof == 4)
                    {
                        sCType = SQL_C_SLONG;
                    }
                    else if (aHostVar->mSizeof == 8)
                    {
                        sCType = SQL_C_SBIGINT;
                    }
                }
                sPrecision = 0;
                break;
            case H_SHORT :
                if (aHostVar->mUnsign == 1)
                {
                    sSqlType = SQL_INTEGER;
                    if (aHostVar->mSizeof == 2)
                    {
                        sCType = SQL_C_USHORT;
                    }
                    else if (aHostVar->mSizeof == 4)
                    {
                        sCType = SQL_C_ULONG;
                    }
                    else if (aHostVar->mSizeof == 8)
                    {
                        sCType = SQL_C_UBIGINT;
                    }
                }
                else
                {
                    sSqlType = SQL_SMALLINT;
                    if (aHostVar->mSizeof == 2)
                    {
                        sCType = SQL_C_SSHORT;
                    }
                    else if (aHostVar->mSizeof == 4)
                    {
                        sCType = SQL_C_SLONG;
                    }
                    else if (aHostVar->mSizeof == 8)
                    {
                        sCType = SQL_C_SBIGINT;
                    }
                }
                sPrecision = 0;
                break;
            case H_CHAR    :
                sCType   = SQL_C_CHAR;
                sSqlType = SQL_CHAR;
                if (aHostVar->mLen == 0 && !(aHostVar->mIsDynAlloc))
                {
                    sPrecision = 1;
                }
                else if ( ulpGetStmtOption( aSqlstmt,
                                            ULP_OPT_NOT_NULL_PAD ) == ACP_TRUE )
                {
                    sCType     = SQL_C_BINARY;
                    sFixedInd  = ACP_TRUE;
                    sPrecision = (SQLULEN)aHostVar->mLen;
                }
                else
                {
                    sPrecision = (SQLULEN)(aHostVar->mLen - 1);
                }
                break;
            case H_VARCHAR :
                sCType   = SQL_C_CHAR;
                sSqlType = SQL_VARCHAR;
                if ( (aHostVar->mLen == 0) && !( aHostVar->mIsDynAlloc ) )
                {
                    sPrecision = 1;
                }
                else if ( ulpGetStmtOption( aSqlstmt,
                                            ULP_OPT_NOT_NULL_PAD ) == ACP_TRUE )
                {
                    sCType     = SQL_C_BINARY;
                    sFixedInd  = ACP_TRUE;
                    sPrecision = (SQLULEN)aHostVar->mLen;
                }
                else
                {
                    sPrecision = (SQLULEN)(aHostVar->mLen - 1);
                }
                break;
            case H_NCHAR:
            case H_NVARCHAR:
                sSqlType =
                        (aHostVar->mType == H_NCHAR)? SQL_WCHAR: SQL_WVARCHAR;

                /* utf8은 지원안하고 utf16만 지원한다.(nchar_utf16 옵션을 준 경우)*/
                if ( gUlpLibOption.mIsNCharUTF16 == ACP_TRUE )
                {
                    /* parameter binding시에는 반드시 indicator가 존재해야 한다*/
                    ACI_TEST_RAISE(aHostVar->mHostInd == NULL, ERR_NO_IND_4NCHAR);
                    sCType     = SQL_C_WCHAR;
                    sPrecision = (SQLULEN)(aHostVar->mLen);
                }
                /* default (nchar_utf16 옵션을 주지 않은 경우)*/
                else
                {
                    sCType     = SQL_C_CHAR;
                    sPrecision = (SQLULEN)(aHostVar->mLen - 1);
                }
                /*======================================*/
                /* binding시 서버 mtdEstimate()에서 검사하는*/
                /* national charset 최대 precision (문자수를 의미)*/
                /* utf16: 32767, utf8: 16383.*/
                /* qtc.cpp:fixAfterBinding()에서 cralloc을 하는데*/
                /* ncs가 utf16이면 precision *2, utf8이면 *4한 만큼 할당*/
                /* 우리는 utf16/utf8인지 모르므로 일단 2로 나누어 둔다*/
                /* 단, pointer(nchar only)타입인 경우 length가 SES_MAX_CHAR*/
                /* (65001)로 세팅되는데, nchar limit를 넘지않기 위해*/
                /* 4로 나눈 16250 을 precision으로 세팅*/
                /* cf) nchar utf8에서 buffer size가 32767 이상이면*/
                /*     서버에서 error를 발생시킬 것이다*/
                /*======================================*/
                if (aHostVar->mIsDynAlloc != 0)
                {
                    sPrecision /= 4;
                }
                else
                {
                    sPrecision /= 2;
                }

                if ((aHostVar->mLen == 0) && !(aHostVar->mIsDynAlloc))
                {
                    sPrecision = 1;
                }

                break;
            case H_BINARY :
                sSqlType = SQL_BLOB;
                sCType   = SQL_C_BINARY;
                sPrecision = ACP_SINT32_MAX;
                break;
            case H_BINARY2 :  /* BUG-46418 */
                sSqlType = SQL_BINARY;
                sCType   = SQL_C_BINARY;
                if (aHostVar->mLen == 0 && !(aHostVar->mIsDynAlloc))
                {
                    sPrecision = 1;
                }
                else
                {
                    sPrecision = (SQLULEN)(aHostVar->mLen);
                }
                break;
            case H_BIT :
                sSqlType = SQL_VARBIT;
                sCType   = SQL_C_BINARY;
                if (aHostVar->mLen > 4)
                {
                    sPrecision = (SQLULEN)((aHostVar->mLen - 4) * 8);
                }
                else
                {
                    sPrecision = 0;
                }
                break;
            case H_BYTES :
                sSqlType = SQL_BYTES;
                sCType   = SQL_C_BINARY;
                sPrecision = (SQLULEN)aHostVar->mLen;
                break;
            case H_VARBYTE :
                sSqlType = SQL_VARBYTE;
                sCType   = SQL_C_BINARY;
                sPrecision = (SQLULEN)aHostVar->mLen;
                break;
            case H_NIBBLE :
                sSqlType = SQL_NIBBLE;
                sCType   = SQL_C_BINARY;
                if (aHostVar->mLen > 1)
                {
                    sPrecision = (SQLULEN)((aHostVar->mLen - 1) * 2);
                }
                else
                {
                    sPrecision = 0;
                }
                break;
            case H_FLOAT :
                sSqlType = SQL_REAL;
                sCType   = SQL_C_FLOAT;
                sPrecision = 0;
                break;
            case H_DOUBLE :
                sSqlType = SQL_DOUBLE;
                sCType   = SQL_C_DOUBLE;
                sPrecision = 0;
                break;
            case H_DATE :
                sSqlType = SQL_TYPE_DATE;
                sCType   = SQL_C_TYPE_DATE;
                sPrecision = 0;
                break;
            case H_TIME :
                sSqlType = SQL_TYPE_TIME;
                sCType   = SQL_C_TYPE_TIME;
                sPrecision = 0;
                break;
            case H_TIMESTAMP :
                sSqlType = SQL_TYPE_TIMESTAMP;
                sCType   = SQL_C_TYPE_TIMESTAMP;
                sPrecision = 0;
                break;
            case H_BLOB :
                sSqlType = SQL_BLOB;
                sCType   = SQL_C_BINARY;
                sPrecision = ACP_SINT32_MAX;
                break;
            case H_CLOB :
                sSqlType = SQL_CLOB;
                sCType   = SQL_C_CHAR;
                sPrecision = ACP_SINT32_MAX;
                break;
            case H_BLOBLOCATOR :
                sSqlType = SQL_BLOB_LOCATOR;
                sCType   = SQL_C_BLOB_LOCATOR;
                sPrecision = 0;
                break;
            case H_CLOBLOCATOR :
                sSqlType = SQL_CLOB_LOCATOR;
                sCType   = SQL_C_CLOB_LOCATOR;
                sPrecision = 0;
                break;
        }

        sSqlRes = SQLBindParameter( *aHstmt,
                                    (SQLUSMALLINT)aIndex,
                                    aInOut,
                                    sCType,
                                    sSqlType,
                                    sPrecision,
                                    0,
                                    (SQLPOINTER)aHostVar->mHostVar,
                                    sMaxlen,
                                    aHostVar->mHostInd );

        ACI_TEST_RAISE( (sSqlRes == SQL_ERROR) || (sSqlRes == SQL_INVALID_HANDLE)
                        , ERR_CLI_BINDPARAM );
    }

    if ( sFixedInd == ACP_TRUE )
    {
        sSqlRes = SQLSetStmtAttr( *aHstmt,
                                  SQL_ATTR_INPUT_NTS,
                                  (SQLPOINTER)SQL_FALSE,
                                  0);

        ACI_TEST_RAISE( (sSqlRes == SQL_ERROR) || (sSqlRes == SQL_INVALID_HANDLE)
                        , ERR_CLI_SETSTMT );
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION ( ERR_CLI_BINDFILEPARAM );
    {
        ulpSetErrorInfo4CLI( aConnNode,
                             *aHstmt,
                             SQL_ERROR,
                             aSqlca,
                             aSqlcode,
                             aSqlstate,
                             ERR_TYPE2 );

    }
    ACI_EXCEPTION ( ERR_CLI_BINDPARAM );
    {
        ulpSetErrorInfo4CLI( aConnNode,
                             *aHstmt,
                             SQL_ERROR,
                             aSqlca,
                             aSqlcode,
                             aSqlstate,
                             ERR_TYPE2 );

    }
    ACI_EXCEPTION ( ERR_CLI_SETSTMT );
    {
        ulpSetErrorInfo4CLI( aConnNode,
                             *aHstmt,
                             SQL_ERROR,
                             aSqlca,
                             aSqlcode,
                             aSqlstate,
                             ERR_TYPE2 );

    }
    ACI_EXCEPTION ( ERR_NO_IND_4NCHAR );
    {
        ulpErrorMgr sErrorMgr;
        ulpSetErrorCode( &sErrorMgr,
                         ulpERR_ABORT_NCHAR_No_Indicator_Error );

        ulpSetErrorInfo4PCOMP( aSqlca,
                               aSqlcode,
                               aSqlstate,
                               ulpGetErrorMSG(&sErrorMgr),
                               SQL_ERROR/* sqlcode */,
                               NULL );

    }
    ACI_EXCEPTION_END;

    /* BUG-45779 error 발생시 자원 정리 */
    if ( sUlpPSMArrDataInfoNode != NULL )
    {
        (void)acpMemFree(sUlpPSMArrDataInfoNode);
    }
    if ( sUlpPSMArrDataInfo != NULL )
    {
        (void)acpMemFree(sUlpPSMArrDataInfo);
    }

    return ACI_FAILURE;
}

ACI_RC ulpBindColCore( ulpLibConnNode *aConnNode,
                       SQLHSTMT       *aHstmt,
                       ulpSqlstmt     *aSqlstmt,
                       ulpHostVar     *aHostVar,
                       acp_sint16_t    aIndex,
                       ulpSqlca       *aSqlca,
                       ulpSqlcode     *aSqlcode,
                       ulpSqlstate    *aSqlstate )
{
/***********************************************************************
 *
 * Description :
 *    out type host 변수에대한 bindcol을 처리한다.
 *    
 *
 * Implementation :
 *
 ***********************************************************************/

    acp_sint32_t sLen;
    SQLRETURN    sSqlRes;
    SQLSMALLINT  sCType = SQL_UNKNOWN_TYPE;

    sLen = aHostVar->mLen;

    if ( (aHostVar->mType == H_BLOB_FILE) ||
         (aHostVar->mType == H_CLOB_FILE) )
    {
        sSqlRes = SQLBindFileToCol( *aHstmt,
                                    (SQLSMALLINT)aIndex,
                                    (SQLCHAR *)aHostVar->mHostVar,
                                    NULL,
                                    aHostVar->mFileopt,
                                    (SQLLEN)sLen,
                                    aHostVar->mHostInd );

        ACI_TEST_RAISE( (sSqlRes == SQL_ERROR) || (sSqlRes == SQL_INVALID_HANDLE)
                        , ERR_CLI_BINDFILECOL );
    }
    else
    {
        switch ( aHostVar->mType )
        {
            case H_INTEGER :
                sCType = SQL_C_SLONG;
                break;
            case H_INT   :
            case H_LONG  :
            case H_LONGLONG:
            case H_SHORT :
                if ( aHostVar->mUnsign == 1 )
                {
                    if (aHostVar->mSizeof == 2)
                    {
                        sCType = SQL_C_USHORT;
                    }
                    else if (aHostVar->mSizeof == 4)
                    {
                        sCType = SQL_C_ULONG;
                    }
                    else if (aHostVar->mSizeof == 8)
                    {
                        sCType = SQL_C_UBIGINT;
                    }
                }
                else
                {
                    if (aHostVar->mSizeof == 2)
                    {
                        sCType = SQL_C_SSHORT;
                    }
                    else if (aHostVar->mSizeof == 4)
                    {
                        sCType = SQL_C_SLONG;
                    }
                    else if (aHostVar->mSizeof == 8)
                    {
                        sCType = SQL_C_SBIGINT;
                    }
                }
                break;
            case H_CHAR    :
            case H_VARCHAR :
                if ( aHostVar->mLen == 0 && !(aHostVar->mIsDynAlloc) )
                {
                    sCType = SQL_C_BINARY;
                    sLen   = 1;
                }
                else if ( ulpGetStmtOption( aSqlstmt,
                                            ULP_OPT_NOT_NULL_PAD ) == ACP_TRUE )
                {
                    sCType = SQL_C_BINARY;
                }
                else
                {
                    sCType = SQL_C_CHAR;
                }
                break;
            case H_NCHAR:
            case H_NVARCHAR:
                if ( aHostVar->mLen == 0 && !(aHostVar->mIsDynAlloc) )
                {
                    sCType = SQL_C_BINARY;
                    sLen   = 1;
                }
                else
                {
                    if ( gUlpLibOption.mIsNCharUTF16 != ACP_TRUE )
                    {
                        sCType = SQL_C_CHAR;
                    }
                    else
                    {
                        sCType = SQL_C_WCHAR;
                    }
                }
                break;
            case H_FLOAT  :
                sCType = SQL_C_FLOAT;
                break;
            case H_DOUBLE :
                sCType = SQL_C_DOUBLE;
                break;
            case H_DATE   :
                sCType = SQL_C_TYPE_DATE;
                break;
            case H_TIME   :
                sCType = SQL_C_TYPE_TIME;
                break;
            case H_TIMESTAMP :
                sCType = SQL_C_TYPE_TIMESTAMP;
                break;
            case H_NIBBLE :
            case H_BYTES  :
            case H_VARBYTE:
            case H_BIT    :
                sCType = SQL_C_BINARY;
                break;
            case H_BLOB   :
            case H_BINARY :
                sCType = SQL_C_BINARY;
                if ( aHostVar->mIsDynAlloc == 1 )
                {
                    if ( aHostVar->mHostInd != NULL )
                    {
                        if ( *(aHostVar->mHostInd) > 0 )
                        {
                            sLen = *(aHostVar->mHostInd);
                        }
                        else
                        {
                            sLen = ACP_SINT32_MAX;
                        }
                    }
                    else
                    {
                        sLen = ACP_SINT32_MAX;
                    }
                }
                break;
            case H_BINARY2:  /* BUG-46418 */
                sCType = SQL_C_BINARY;
                if ( aHostVar->mLen == 0 && !(aHostVar->mIsDynAlloc) )
                {
                    sCType = SQL_C_BINARY;
                    sLen   = 1;
                }
                break;
            case H_CLOB   :
                sCType = SQL_C_CHAR;
                if ( aHostVar->mIsDynAlloc == 1 )
                {
                    if ( aHostVar->mHostInd != NULL )
                    {
                        if ( *(aHostVar->mHostInd) > 0 )
                        {
                            sLen = *(aHostVar->mHostInd);
                        }
                        else
                        {
                            sLen = ACP_SINT32_MAX;
                        }
                    }
                    else
                    {
                        sLen = ACP_SINT32_MAX;
                    }
                }
                break;
            case H_BLOBLOCATOR :
                sCType = SQL_C_BLOB_LOCATOR;
                break;
            case H_CLOBLOCATOR :
                sCType = SQL_C_CLOB_LOCATOR;
                break;
            /* BUG-45933 */
            case H_NUMERIC_STRUCT :
                sCType = SQL_C_BINARY;
                sLen = sizeof(SQL_NUMERIC_STRUCT);
                break;
        }

        sSqlRes = SQLBindCol( *aHstmt,
                              (SQLUSMALLINT)aIndex,
                              sCType,
                              (SQLPOINTER)aHostVar->mHostVar,
                              (SQLLEN)sLen,
                              aHostVar->mHostInd);

        ACI_TEST_RAISE( (sSqlRes == SQL_ERROR) || (sSqlRes == SQL_INVALID_HANDLE)
                        , ERR_CLI_BINDCOL );
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION ( ERR_CLI_BINDFILECOL );
    {
        ulpSetErrorInfo4CLI( aConnNode,
                             *aHstmt,
                             SQL_ERROR,
                             aSqlca,
                             aSqlcode,
                             aSqlstate,
                             ERR_TYPE2 );

    }
    ACI_EXCEPTION ( ERR_CLI_BINDCOL );
    {
        ulpSetErrorInfo4CLI( aConnNode,
                             *aHstmt,
                             SQL_ERROR,
                             aSqlca,
                             aSqlcode,
                             aSqlstate,
                             ERR_TYPE2 );

    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC ulpSetDateFmtCore( ulpLibConnNode *aConnNode )
{
/***********************************************************************
 *
 * Description :
 *
 *      DBC의 ALTIBASE_DATE_FORMAT을 설정해준다.
 *      환경변수 ALTIBASE_DATE_FORMAT이 설정되어있으면 환경변수로 설정하고,
 *      그렇지 않으면 기본값을 사용한다.
 *
 * Implementation :
 *
 ***********************************************************************/
    acp_char_t *sDateFmt;

    /* BUG-41471 Maintain one APRE source code for HDB and HDB-DA */
    if ( acpEnvGet( (acp_char_t *)ENV_ALTIBASE_DATE_FORMAT, &sDateFmt ) == ACP_RC_SUCCESS )
    {
        ACI_TEST( SQLSetConnectAttr( aConnNode->mHdbc,
                                     ALTIBASE_DATE_FORMAT,
                                     (SQLPOINTER)sDateFmt, SQL_NTS ) );
    }
    else
    {
        /* do nothing */
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}


void ulpSetColRowSizeCore( ulpSqlstmt *aSqlstmt )
{
/***********************************************************************
 *
 * Description :
 *      ulpSqlstmt 구조체의 host변수들에 대한 isarr/isstruct 정보를 읽어 array/rowwise
 *      binding여부를 결정하여 ulpSqlstmt.isarr 와 ulpSqlstmt.arrsize 필드를 세팅함.
 *
 * Implementation :
 *      다른 type의 호스트 변수가 올때까지 검색하여 arrsize를 최소값으로 설정함.
 ***********************************************************************/
    acp_sint32_t   sI;
    acp_uint32_t   sSize;

    sSize  = 0;
    aSqlstmt->isarr   = 0;
    aSqlstmt->arrsize = 0;

    aSqlstmt->isstruct = 0;
    aSqlstmt->structsize = 0;

    if ( aSqlstmt->numofhostvar > 0 )
    {
        /* BUG-32952 */
        /* set array size */
        for ( sI = 0 ;
              aSqlstmt->numofhostvar > sI;
              sI++ )
        {
            if ( aSqlstmt->hostvalue[sI].isarr != 0 )
            {
                if ( (sSize == 0) || (sSize > aSqlstmt->hostvalue[sI].arrsize) )
                {
                    sSize = aSqlstmt->hostvalue[sI].arrsize;
                    aSqlstmt->isarr = 1;
                    aSqlstmt->arrsize = sSize;
                }
            }
        }

        /* set struct size */
        for ( sI = 0 ;
              aSqlstmt->numofhostvar > sI;
              sI++ )
        {
            if ( aSqlstmt->hostvalue[sI].isstruct != 0 )
            {
                sSize = aSqlstmt->hostvalue[sI].structsize;
                aSqlstmt->isstruct = 1;
                aSqlstmt->structsize = sSize;
                break;
            }
        }
    }
}


/* BUG-31405 : Failover성공후 Failure of finding statement 에러 발생. */
void ulpSetFailoverFlag( ulpLibConnNode *aConnNode )
{
/***********************************************************************
 *
 * Description :
 *      FailOver에서 성공한뒤 apre에서는 해당 커넥션에 포함된 stmt들을 다시 prepare한다.
 *      나중에 해당구문이 재수행될때 reprepare되도록 mNeedReprepare를 true로 쎄팅해준다.
 *
 * Implementation :
 *
 ***********************************************************************/

    acp_sint32_t    i;
    ulpLibStmtNode *sStmtNode;

    sStmtNode = NULL;

    /***************************
    * Reprepare statements    *
    ***************************/

    /* 1.1 statements in the unnamedStmtCacheList */
    if ( aConnNode->mUnnamedStmtCacheList.mNumNode > 0 )
    {
        sStmtNode = aConnNode->mUnnamedStmtCacheList.mList;
        for ( ; sStmtNode != NULL ; sStmtNode = sStmtNode->mNextStmt )
        {
            if ( (sStmtNode->mStmtState   != S_INITIAL) &&
                  (sStmtNode->mQueryStr    != NULL) )
            {
                sStmtNode->mNeedReprepare = ACP_TRUE;
            }
        }
    }

    /* 1.2 statements in the statement hash table */
    if ( aConnNode->mStmtHashT.mNumNode > 0 )
    {
        for( i = 0 ; i < MAX_NUM_STMT_HASH_TAB_ROW ; i++ )
        {
            sStmtNode = aConnNode->mStmtHashT.mTable[i].mList;
            for ( ; sStmtNode != NULL ; sStmtNode = sStmtNode->mNextStmt )
            {
                if ( (sStmtNode->mStmtState   != S_INITIAL) &&
                      (sStmtNode->mQueryStr    != NULL) )
                {
                    if ( sStmtNode->mCursorName[0] != '\0' )
                    {   /* cursor일 경우 Freestmt함. */
                        SQLFreeStmt( sStmtNode->mHstmt, SQL_CLOSE );
                    }
                    sStmtNode->mNeedReprepare = ACP_TRUE;
                }
            }
        }
    }

    /* 1.3 statements in the cursor hash table */
    if ( aConnNode->mCursorHashT.mNumNode > 0 )
    {
        for( i = 0 ; i < MAX_NUM_STMT_HASH_TAB_ROW ; i++ )
        {
            sStmtNode = aConnNode->mCursorHashT.mTable[i].mList;
            for ( ; sStmtNode != NULL ; sStmtNode = sStmtNode->mNextStmt )
            {
                /* 위에서 stmt hash table을 검색하면서 reprepare을 이미 했기때문에,
                   mStmtName이 없는 경우만 고려하면 된다. */
                if( (sStmtNode->mStmtName[0] == '\0')      &&
                     (sStmtNode->mCurState    != C_INITIAL) &&
                     (sStmtNode->mQueryStr    != NULL) )
                {
                    SQLFreeStmt( sStmtNode->mHstmt, SQL_CLOSE );
                    sStmtNode->mNeedReprepare = ACP_TRUE;
                }
            }
        }
    }
}

/* BUG-25643 : apre 에서 ONERR 구문이 잘못 동작합니다. */
ACI_RC ulpGetOnerrErrCodeCore( ulpLibConnNode *aConnNode,
                               ulpSqlstmt     *aSqlstmt,
                               SQLHSTMT       *aHstmt,
                               acp_sint32_t   *aErrCode )
{
/***********************************************************************
 *
 * Description :
 *      ONERR 구문이 사용되었을 경우 사용자가 지정한 배열에 각 row에 대한
 *      native error code를 저장해준다.
 *
 * @param[out] aErrCode
 *      사용자 지정 error code 배열 pointer.
 ***********************************************************************/

    acp_sint32_t i;
    SQLRETURN    sSQLRC;
    acp_sint32_t sNDiagRec;
    SQLLEN       sRowNo;
    acp_sint16_t sStrLen;
    acp_sint32_t sSQLCODE;

    /* Diag rec. 개수 알아냄. */
    sSQLRC = SQLGetDiagField(SQL_HANDLE_STMT,
                             (SQLHANDLE)*aHstmt,
                             0,
                             SQL_DIAG_NUMBER,
                             (SQLPOINTER)&sNDiagRec,
                             SQL_IS_POINTER,
                             (SQLSMALLINT *)&sStrLen);

    ACI_TEST_RAISE( (sSQLRC == SQL_INVALID_HANDLE)||
                    (sSQLRC == SQL_ERROR), ERR_CLI_GETDIAGFIELD );

    for( i = 1 ; i <= sNDiagRec ; i++ )
    {
        /* 행 번호 알아냄. */
        sSQLRC = SQLGetDiagField(SQL_HANDLE_STMT,
                                 (SQLHANDLE)*aHstmt,
                                 (SQLSMALLINT)i,
                                 SQL_DIAG_ROW_NUMBER,
                                 (SQLPOINTER)&sRowNo,
                                 SQL_IS_POINTER,
                                 (SQLSMALLINT *)&sStrLen);

        ACI_TEST_RAISE( (sSQLRC == SQL_INVALID_HANDLE)||
                        (sSQLRC == SQL_ERROR), ERR_CLI_GETDIAGFIELD );

        /* native error code 알아냄. */
        sSQLRC = SQLGetDiagField(SQL_HANDLE_STMT,
                                 (SQLHANDLE)*aHstmt,
                                 (SQLSMALLINT)i,
                                 SQL_DIAG_NATIVE,
                                 (SQLPOINTER)&sSQLCODE,
                                 SQL_IS_POINTER,
                                 (SQLSMALLINT *)&sStrLen);

        ACI_TEST_RAISE( (sSQLRC == SQL_INVALID_HANDLE)||
                        (sSQLRC == SQL_ERROR), ERR_CLI_GETDIAGFIELD );

        if ( (sSQLRC == SQL_SUCCESS) && (sRowNo > 0) )
        {
            aErrCode[(sRowNo -1)] = sSQLCODE;
        }
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION( ERR_CLI_GETDIAGFIELD );
    {
        ulpSetErrorInfo4CLI( aConnNode,
                             *aHstmt,
                             sSQLRC,
                             aSqlstmt->sqlcaerr,
                             aSqlstmt->sqlcodeerr,
                             aSqlstmt->sqlstateerr,
                             ERR_TYPE1 );
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

/* BUG-45448 FOR구문이 포함되었을경우 arrsize 보정하는 로직을 함수로 정의 */
ACI_RC ulpAdjustArraySize(ulpSqlstmt *aSqlstmt)
{
/***********************************************************************
 *
 * Description :
 *      사용자가 FOR구문을 사용하였을경우 FOR에 사용된 값을 우선적으로 
 *      arrsize에 할당한다.
 *
 * @param aSqlstmt
 *      ulpSqlStmt
 ***********************************************************************/
 
    /* FOR절 처리*/
    if ( aSqlstmt->iters == 0 )
    {
        ACI_TEST_RAISE( aSqlstmt->isatomic == 1, ERR_ATOMIC_FOR_ZERO );
    }
    else
    {
        /* BUG-28788 : FOR절을 이용하여 struct pointer type의 array insert가 안됨  */
        if ( aSqlstmt->isarr == 0 )
        {
            /* FOR 절이와서 iter가 0이 아니라면 array type이 아니더라도*/
            /* 무조건 iter 만큼 array insert해라 라고 간주된다.*/
            aSqlstmt->isarr   = 1;
            aSqlstmt->arrsize = aSqlstmt->iters;
        }
        else
        {
            aSqlstmt->arrsize = ACP_MIN( aSqlstmt->iters, aSqlstmt->arrsize );
        }
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION ( ERR_ATOMIC_FOR_ZERO );
    {
        ulpErrorMgr sErrorMgr;
        ulpSetErrorCode( &sErrorMgr,
                         ulpERR_ABORT_Atomic_For_Value_Error );

        ulpSetErrorInfo4PCOMP( aSqlstmt->sqlcaerr,
                               aSqlstmt->sqlcodeerr,
                               aSqlstmt->sqlstateerr,
                               ulpGetErrorMSG(&sErrorMgr),
                               ulpGetErrorCODE(&sErrorMgr),
                               ulpGetErrorSTATE(&sErrorMgr) );
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC ulpPSMArrayWrite(ulpSqlstmt *aSqlstmt, ulpHostVar *aHostVar, void* aBuffer)
{
/***********************************************************************
 * BUG-45779
 *
 * Description :
 *      Server로 전달할 PSM Array Bind를 위한 Meta정보와 Host variable 데이터를 copy한다.
 *
 * @param aSqlstmt
 *        aHostVar        host variable infomation
 *        aBuffer         meta + host varivable 
 ***********************************************************************/
    psmArrayMetaInfo sPsmArrayMetaInfo;

    sPsmArrayMetaInfo.mUlpVersion = APRE_PSM_ARRAY_META_VERSION;

    switch ( aHostVar->mType )
    {
        case H_SHORT:
            sPsmArrayMetaInfo.mUlpSqltype = SQL_C_SHORT;
            break;
        case H_INT:
            sPsmArrayMetaInfo.mUlpSqltype = SQL_C_LONG;
            break;
        case H_LONG:
        case H_LONGLONG:
            sPsmArrayMetaInfo.mUlpSqltype = SQL_BIGINT;
            break;
        case H_FLOAT:
            sPsmArrayMetaInfo.mUlpSqltype = SQL_C_FLOAT;
            break;
        case H_DOUBLE:
            sPsmArrayMetaInfo.mUlpSqltype = SQL_C_DOUBLE;
            break;
        default:
            /* BUG-45779 지원하지 않는 Type */
            ACI_RAISE(UnsupportCType);
    }

    sPsmArrayMetaInfo.mUlpElemCount = aHostVar->arrsize;
    sPsmArrayMetaInfo.mUlpReturnElemCount = 0;
    sPsmArrayMetaInfo.mUlpHasNull = 0;

    /* BUG-45779 in, inout type일 경우에만 data를 copy한다. */
    if ( aHostVar->mInOut == H_IN ||
         aHostVar->mInOut == H_INOUT )
    {
        (void)acpMemCpy( (acp_char_t*)aBuffer,
                         &sPsmArrayMetaInfo,
                         sizeof(psmArrayMetaInfo));

        (void)acpMemCpy( (acp_char_t*)aBuffer + sizeof(psmArrayMetaInfo),
                         (acp_char_t*)aHostVar->mHostVar,
                         aHostVar->arrsize * aHostVar->mSizeof );
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION ( UnsupportCType )
    {
        ulpErrorMgr sErrorMgr;
        ulpSetErrorCode( &sErrorMgr, ulpERR_ABORT_Not_Supported_Host_Var_Type );
        ulpSetErrorInfo4PCOMP( aSqlstmt->sqlcaerr,
                               aSqlstmt->sqlcodeerr,
                               aSqlstmt->sqlstateerr,
                               ulpGetErrorMSG(&sErrorMgr),
                               ulpGetErrorCODE(&sErrorMgr),
                               ulpGetErrorSTATE(&sErrorMgr) );
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}


ACI_RC ulpPSMArrayRead(ulpSqlstmt *aSqlstmt, ulpHostVar *aHostVar, void* aBuffer)
{
/***********************************************************************
 * BUG-45779
 *
 * Description :
 *      Server로 부터 전달받은 Meta정보와 Host variable 데이터를 copy한다.
 *
 * @param aSqlstmt
 *        aHostVar        host variable infomation
 *        aBuffer         meta + host varivable 
 ***********************************************************************/
    psmArrayMetaInfo *sPsmArrayMetaInfo;

    sPsmArrayMetaInfo = (psmArrayMetaInfo*)aBuffer;

    ACI_TEST_RAISE( sPsmArrayMetaInfo->mUlpVersion != APRE_PSM_ARRAY_META_VERSION, Invalid_PSM_Array_Version );

    /* BUG-45779 out parameter일 경우 psm array type과 apre 타입을 비교한다. */
    switch(aHostVar->mType)
    {
        case H_SHORT:
            ACI_TEST_RAISE( sPsmArrayMetaInfo->mUlpSqltype != SQL_C_SHORT, InvalidPSMArrayType);
            break;
        case H_INT:
            ACI_TEST_RAISE( sPsmArrayMetaInfo->mUlpSqltype != SQL_C_LONG, InvalidPSMArrayType);
            break;
        case H_LONG:
        case H_LONGLONG:
            ACI_TEST_RAISE( sPsmArrayMetaInfo->mUlpSqltype != SQL_BIGINT, InvalidPSMArrayType);
            break;
        case H_FLOAT:
            ACI_TEST_RAISE( sPsmArrayMetaInfo->mUlpSqltype != SQL_C_FLOAT, InvalidPSMArrayType);
            break;
        case H_DOUBLE:
            ACI_TEST_RAISE( sPsmArrayMetaInfo->mUlpSqltype != SQL_C_DOUBLE, InvalidPSMArrayType);
            break;
        default:
            /* BUG-45779 지원하지 않는 Type */
            ACI_RAISE(UnsupportCType);
    }

    /* BUG-45779 out array bind에 처리된 값을 indicator에 셋팅한다. */
    if ( aHostVar->mHostInd != NULL )
    {
        *(aHostVar->mHostInd) = (SQLLEN)sPsmArrayMetaInfo->mUlpReturnElemCount;
    }

    (void)acpMemCpy((acp_char_t*)aHostVar->mHostVar,
                    ((acp_char_t*)aBuffer + sizeof(psmArrayMetaInfo)),
                    aHostVar->arrsize * aHostVar->mSizeof );

    return ACI_SUCCESS;

    ACI_EXCEPTION ( Invalid_PSM_Array_Version )
    {
        ulpErrorMgr sErrorMgr;
        ulpSetErrorCode( &sErrorMgr, ulpERR_ABORT_Invalid_PSM_Array_Version );
        ulpSetErrorInfo4PCOMP( aSqlstmt->sqlcaerr,
                               aSqlstmt->sqlcodeerr,
                               aSqlstmt->sqlstateerr,
                               ulpGetErrorMSG(&sErrorMgr),
                               ulpGetErrorCODE(&sErrorMgr),
                               ulpGetErrorSTATE(&sErrorMgr) );
    }
    ACI_EXCEPTION ( UnsupportCType )
    {
        ulpErrorMgr sErrorMgr;
        ulpSetErrorCode( &sErrorMgr, ulpERR_ABORT_Not_Supported_Host_Var_Type );
        ulpSetErrorInfo4PCOMP( aSqlstmt->sqlcaerr,
                               aSqlstmt->sqlcodeerr,
                               aSqlstmt->sqlstateerr,
                               ulpGetErrorMSG(&sErrorMgr),
                               ulpGetErrorCODE(&sErrorMgr),
                               ulpGetErrorSTATE(&sErrorMgr) );
    }
    ACI_EXCEPTION ( InvalidPSMArrayType )
    {
        ulpErrorMgr sErrorMgr;
        ulpSetErrorCode( &sErrorMgr, ulpERR_ABORT_Invalid_PSM_Array_Type );
        ulpSetErrorInfo4PCOMP( aSqlstmt->sqlcaerr,
                               aSqlstmt->sqlcodeerr,
                               aSqlstmt->sqlstateerr,
                               ulpGetErrorMSG(&sErrorMgr),
                               ulpGetErrorCODE(&sErrorMgr),
                               ulpGetErrorSTATE(&sErrorMgr) );
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC ulpPSMArrayHasNullCheck(ulpSqlstmt *aSqlstmt, void* aBuffer)
{
    psmArrayMetaInfo *sPsmArrayMetaInfo;

    sPsmArrayMetaInfo = (psmArrayMetaInfo*)aBuffer;

    if( sPsmArrayMetaInfo->mUlpHasNull == 1 )
    {
        /* BUG-45779 out parameter에 null값이 포함되어 있을경우, 데이터저장후 error로 리턴한다. */
        ACI_RAISE( HasNull );
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION ( HasNull )
    {
        ulpErrorMgr sErrorMgr;
        ulpSetErrorCode( &sErrorMgr, ulpERR_ABORT_Column_Value_Is_Null );
        ulpSetErrorInfo4PCOMP( aSqlstmt->sqlcaerr,
                               aSqlstmt->sqlcodeerr,
                               aSqlstmt->sqlstateerr,
                               ulpGetErrorMSG(&sErrorMgr),
                               ulpGetErrorCODE(&sErrorMgr),
                               ulpGetErrorSTATE(&sErrorMgr) );
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

 /* BUG-45779 stmtNode에 할당된 PSM Array 메모리를 해제한다. */
 void ulpPSMArrayMetaFree(ulpPSMArrInfo  *aUlpPSMArrInfo)
 {
 /***********************************************************************
 * BUG-45779
 *
 * Description :
 *      Bind된 PSM Array Buffer를 메모리에서 해제한다.
 *
 * @param aUlpPSMArrInfo    psm array infomation
 ***********************************************************************/
     acp_list_node_t *sPSMMetaIterator;
     acp_list_node_t *sPSMMetaNodeNext;

     ulpPSMArrDataInfo   *sUlpPSMArrDataInfo;

     ACP_LIST_ITERATE_SAFE(&aUlpPSMArrInfo->mPSMArrDataInfoList, sPSMMetaIterator, sPSMMetaNodeNext)
     {
         sUlpPSMArrDataInfo = (ulpPSMArrDataInfo *)sPSMMetaIterator->mObj;

         if( sUlpPSMArrDataInfo->mData != NULL)
         {
             (void)acpMemFree( sUlpPSMArrDataInfo->mData );
             sUlpPSMArrDataInfo->mData = NULL;
         }
         if( sUlpPSMArrDataInfo->mUlpHostVar != NULL)
         {
             /* User 메모리이므로 해제하지 않음 */
             sUlpPSMArrDataInfo->mUlpHostVar = NULL;
         }

         acpListDeleteNode(sPSMMetaIterator);
         (void)acpMemFree(sPSMMetaIterator);
     }
}
