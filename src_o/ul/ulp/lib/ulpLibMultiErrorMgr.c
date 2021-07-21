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

#include <ulpLibMultiErrorMgr.h>

/* 
 * TASK-7218 Handling Multiple Errors
 *
 * Implementation:
 *     ulpSetErrorInfo4CLI()에서 에러를 셋팅할 때 Multi-Error가 있다면
 *     해당 ConnNode의 mMultiErrorMgr를 thread local 변수인 gMultiErrorMgr에
 *     assign해 두고 MultiError를 읽어갈 때 사용함.
 */
ACP_TLS(ulpMultiErrorMgr*, gMultiErrorMgr);

ulpMultiErrorMgr *ulpLibGetMultiErrorMgr( void )
{
    return gMultiErrorMgr;
}

void ulpLibSetMultiErrorMgr( ulpMultiErrorMgr *aMultiErrorMgr )
{
    gMultiErrorMgr = aMultiErrorMgr;
}

void ulpLibInitMultiErrorMgr( ulpMultiErrorMgr *aMultiErrorMgr )
{
    aMultiErrorMgr->mErrorNum = 0;
}

void ulpLibMultiErrorMgrSetDiagRec( SQLHSTMT            aHstmt,
                              ulpMultiErrorMgr   *aMultiErrorMgr,
                              acp_sint32_t        aRecordNo,
                              ulpSqlcode          aSqlcode,
                              acp_char_t         *aSqlstate,
                              acp_char_t         *aErrMsg )
{
    ulpMultiErrorStack *sErrorStack = NULL;

    // defense code
    ACI_TEST( aRecordNo > aMultiErrorMgr->mErrorNum ||
              aRecordNo > aMultiErrorMgr->mStackSize );

    sErrorStack = &(aMultiErrorMgr->mErrorStack[aRecordNo - 1]);

    sErrorStack->mSqlcode = -1 * aSqlcode;
    acpSnprintf(sErrorStack->mSqlstate, MAX_SQLSTATE_LEN, "%s", aSqlstate);
    acpSnprintf(sErrorStack->mMessage, MAX_ERRMSG_LEN, "%s", aErrMsg);

    if ( SQLGetDiagField( SQL_HANDLE_STMT,
                          aHstmt,
                          aRecordNo,
                          SQL_DIAG_ROW_NUMBER,
                          (void *) &(sErrorStack->mRowNum),
                          0,
                          0 ) != SQL_SUCCESS )
    {
        sErrorStack->mRowNum = 0;
    }
    if ( SQLGetDiagField( SQL_HANDLE_STMT,
                          aHstmt,
                          aRecordNo,
                          SQL_DIAG_COLUMN_NUMBER,
                          (void *) &(sErrorStack->mColumnNum),
                          0,
                          0 ) != SQL_SUCCESS )
    {
        sErrorStack->mColumnNum = 0;
    }

    return;
    ACI_EXCEPTION_END;
    return;
}

ACI_RC ulpLibMultiErrorMgrSetNumber( SQLHSTMT          aHstmt,
                                  ulpMultiErrorMgr *aMultiErrMgr )
{ 
    SQLRETURN       sSqlres;
    acp_sint32_t    sNumRecs = 0;

    sSqlres = SQLGetDiagField( SQL_HANDLE_STMT,
                               aHstmt,
                               0,
                               SQL_DIAG_NUMBER,
                               &sNumRecs,
                               0,
                               0 );

    if ( sSqlres == SQL_SUCCESS &&
         sNumRecs > aMultiErrMgr->mStackSize )
    {
        // realloc
        ACI_TEST_RAISE( acpMemRealloc( (void **) &(aMultiErrMgr->mErrorStack),
                                       sNumRecs * ACI_SIZEOF(ulpMultiErrorStack) )
                        != ACP_RC_SUCCESS, ERR_MEMORY_ALLOC );

        aMultiErrMgr->mStackSize = sNumRecs;
    }

    aMultiErrMgr->mErrorNum  = sNumRecs;

    return ACI_SUCCESS;

    ACI_EXCEPTION (ERR_MEMORY_ALLOC);
    {
        ulpErrorMgr sErrorMgr;
        ulpSetErrorCode( &sErrorMgr,
                         ulpERR_ABORT_Memory_Alloc_Error );
        ulpPrintfErrorCode( ACP_STD_ERR,
                            &sErrorMgr);
        ACE_ASSERT(0);
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

acp_sint32_t ulpLibMultiErrorMgrGetNumber( ulpMultiErrorMgr *aMultiErrorMgr )
{
    /* multi-error가 셋팅되어 있지 않으면 sqlca 이용, 따라서 1 반환 */
    if ( aMultiErrorMgr == NULL )
    {
        return 1;
    }
    else
    {
        return aMultiErrorMgr->mErrorNum;
    }
}

acp_sint32_t ulpLibMultiErrorMgrGetSqlcode( ulpMultiErrorStack *aError )
{
    /* multi-error가 셋팅되어 있지 않으면 SQLCODE 반환 */
    if (aError == NULL)
    {
        return SQLCODE;
    }
    else
    {
        return aError->mSqlcode;
    }
}

void ulpLibMultiErrorMgrGetSqlstate( ulpMultiErrorStack *aError,
                                  acp_char_t         *aBuffer,
                                  acp_sint16_t        aBufferSize,
                                  ulvSLen            *aActualLength )
{
    acp_char_t   *sSqlstate = NULL;
    acp_sint16_t  sActualLength;

    /* multi-error가 셋팅되어 있지 않으면 SQLSTATE 반환 */
    if ( aError == NULL )
    {
        sSqlstate = SQLSTATE;
    }
    else
    {
        sSqlstate = aError->mSqlstate;
    }

    if (aActualLength != NULL)
    {
        sActualLength = acpCStrLen(sSqlstate, ACP_SINT32_MAX);
        *aActualLength = sActualLength;
    }

    if ((aBuffer != NULL) && (aBufferSize > 0))
    {
        acpSnprintf(aBuffer, aBufferSize, "%s", sSqlstate);
    }
}

void ulpLibMultiErrorMgrGetMessageText( ulpMultiErrorStack *aError,
                                     acp_char_t         *aBuffer,
                                     acp_sint16_t        aBufferSize,
                                     ulvSLen            *aActualLength )
{
    acp_char_t   *sMessageText = NULL;
    acp_sint16_t  sActualLength;

    /* multi-error가 셋팅되어 있지 않으면 sqlca 이용 */
    if ( aError == NULL )
    {
        sMessageText = sqlca.sqlerrm.sqlerrmc;
    }
    else
    {
        sMessageText = aError->mMessage;
    }

    if (aActualLength != NULL)
    {
        sActualLength = acpCStrLen(sMessageText, ACP_SINT32_MAX);
        *aActualLength = sActualLength;
    }

    if ((aBuffer != NULL) && (aBufferSize > 0))
    {
        acpSnprintf(aBuffer, aBufferSize, "%s", sMessageText);
    }
}

acp_sint32_t ulpLibMultiErrorMgrGetRowNumber( ulpMultiErrorStack *aError )
{
    /* multi-error가 셋팅되어 있지 않으면 unknown */
    if ( aError == NULL )
    {
        return SQL_ROW_NUMBER_UNKNOWN;
    }
    else
    {
        return aError->mRowNum;
    }
}

acp_sint32_t ulpLibMultiErrorMgrGetColumnNumber( ulpMultiErrorStack *aError )
{
    /* multi-error가 셋팅되어 있지 않으면 unknown */
    if ( aError == NULL )
    {
        return SQL_COLUMN_NUMBER_UNKNOWN;
    }
    else
    {
        return aError->mColumnNum;
    }
}
