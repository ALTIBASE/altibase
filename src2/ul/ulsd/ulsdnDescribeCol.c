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
 

/**********************************************************************
 * $Id: ulsdnDescribeCol.c 00000 2017-02-27 10:26:00Z swhors $
 **********************************************************************/

#include <uln.h>
#include <ulnPrivate.h>
#include <mtcc.h>

#include <ulsd.h>

static ACI_RC ulsdDescribeColCheckArgs(ulnFnContext *aFnContext,
                                       acp_uint16_t  aColumnNumber,
                                       acp_sint16_t  aBufferLength)
{
    ulnStmt      *sStmt = aFnContext->mHandle.mStmt;
    acp_sint16_t  sNumOfResultColumns;

    /*
     * HY090 : Invalid string or buffer length
     */
    ACI_TEST_RAISE(aBufferLength < 0, LABEL_INVALID_BUFF_LEN);

    sNumOfResultColumns = ulnStmtGetColumnCount(sStmt);
    ACI_TEST_RAISE(sNumOfResultColumns == 0, LABEL_NO_RESULT_SET);

    /*
     * 07009 : Invalid Descriptor index
     */
    ACI_TEST_RAISE((aColumnNumber == 0) &&
                   (ulnStmtGetAttrUseBookMarks(sStmt) == SQL_UB_OFF),
                   LABEL_INVALID_DESC_INDEX);

    ACI_TEST_RAISE(aColumnNumber > sNumOfResultColumns, LABEL_INVALID_DESC_INDEX);

    return ACI_SUCCESS;

    ACI_EXCEPTION(LABEL_INVALID_DESC_INDEX)
    {
        /*
         * 07009 : BOOKMARK 지원안하는데 column number 에 0 을 준다거나
         *         result column 보다 큰 index 를 줬을 때 발생
         */
        ulnError(aFnContext, ulERR_ABORT_INVALID_DESCRIPTOR_INDEX, aColumnNumber);
    }

    ACI_EXCEPTION(LABEL_NO_RESULT_SET)
    {
        /*
         * 07005 : result set 을 생성 안하는 statement 가 실행되어서
         *         result set 이 없는데 SQLDescribeCol() 을 호출하였다.
         */
        ulnError(aFnContext, ulERR_ABORT_STMT_HAVE_NO_RESULT_SET);
    }

    ACI_EXCEPTION(LABEL_INVALID_BUFF_LEN)
    {
        ulnError(aFnContext, ulERR_ABORT_INVALID_BUFFER_LEN, aBufferLength);
    }

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

/* PROJ-2638 shard native linker */
SQLRETURN ulsdDescribeCol( ulnStmt      *aStmt,
                           acp_uint16_t  aColumnNumber,
                           acp_char_t   *aColumnName,
                           acp_sint32_t  aBufferLength,
                           acp_uint32_t *aNameLengthPtr,
                           acp_uint32_t *aDataMtTypePtr,
                           acp_sint32_t *aPrecisionPtr,
                           acp_sint16_t *aScalePtr,
                           acp_sint16_t *aNullablePtr )
{
    ULN_FLAG(sNeedExit);

    ulnDescRec   *sIrdRecord;
    ulnFnContext  sFnContext;
    ulnMeta      *sMeta     = NULL;
    acp_char_t   *sColName  = NULL;

    ULN_INIT_FUNCTION_CONTEXT( sFnContext,
                               ULN_FID_DESCRIBECOL,
                               aStmt,
                               ULN_OBJ_TYPE_STMT);

    ACI_TEST( ulnEnter( &sFnContext, NULL ) != ACI_SUCCESS );
    ULN_FLAG_UP( sNeedExit );

    /* PROJ-1891 Deferred Prepare
     * If the Defer Prepares is enabled, send the deferred prepare first */
    if ( aStmt->mAttrDeferredPrepare == ULN_CONN_DEFERRED_PREPARE_ON )
    {
        ACI_TEST( ulnFinalizeProtocolContext(&sFnContext,
                                             &aStmt->mParentDbc->mPtContext )
                  != ACI_SUCCESS);

        ulnUpdateDeferredState(&sFnContext, aStmt);
    }

    /*
     * ===========================================
     * Function BEGIN
     * ===========================================
     */

    ACI_TEST( ulsdDescribeColCheckArgs( &sFnContext,
                                       aColumnNumber,
                                       aBufferLength) != ACI_SUCCESS );

    ACI_TEST_RAISE( aColumnNumber == 0, LABEL_MEM_MAN_ERR );

    sIrdRecord = ulnStmtGetIrdRec( aStmt, aColumnNumber );
    ACI_TEST_RAISE( sIrdRecord == NULL, LABEL_MEM_MAN_ERR );

    if ( aColumnName != NULL )
    {
        sColName = ulnDescRecGetColumnName( sIrdRecord );

        if ( sColName == NULL )
        {
            *aColumnName = 0;
            if ( aNameLengthPtr != NULL )
            {
                *aNameLengthPtr = 0;
            }
        }
        else
        {
            acpSnprintf( aColumnName, aBufferLength, "%s", sColName );
            if ( aNameLengthPtr != NULL )
            {
                *aNameLengthPtr = acpCStrLen( aColumnName, aBufferLength );
            }
        }
    }

    sMeta = &sIrdRecord->mMeta;

    if ( aDataMtTypePtr != NULL )
    {
        *aDataMtTypePtr = ulnTypeMap_MTYPE_MTD( sMeta->mMTYPE );
    }

    if ( aPrecisionPtr != NULL )
    {
        *aPrecisionPtr = (acp_sint32_t)ulnMetaGetPrecision( sMeta );

        if ( (*aPrecisionPtr == 0) &&
             (ulsdIsFixedPrecision( sMeta->mMTYPE ) == ACP_FALSE) )
        {
            *aPrecisionPtr = ulnMetaGetOdbcLength( sMeta );
        }
    }

    if ( aScalePtr != NULL )
    {
        *aScalePtr = ulnMetaGetScale( sMeta );
    }

    if ( aNullablePtr != NULL )
    {
        *aNullablePtr = sMeta->mNullable;
    }

    /*
     * ===========================================
     * Function END
     * ===========================================
     */

    ULN_FLAG_DOWN( sNeedExit );
    ACI_TEST( ulnExit( &sFnContext ) != ACI_SUCCESS );

    ULN_TRACE_LOG( &sFnContext, ULN_TRACELOG_MID, NULL, 0,
            "%-18s| [%2"ACI_UINT32_FMT" name: %-20s"
            " stypePtr: %p]", "ulnDescribeColEx",
            aColumnNumber, aColumnName, aDataMtTypePtr );

    return ULN_FNCONTEXT_GET_RC( &sFnContext );

    ACI_EXCEPTION( LABEL_MEM_MAN_ERR )
    {
        ulnError( &sFnContext,
                  ulERR_FATAL_MEMORY_MANAGEMENT_ERROR,
                  "ulnDescribeColEx : IRD not found");
    }

    ACI_EXCEPTION_END;

    ULN_IS_FLAG_UP( sNeedExit )
    {
        ulnExit( &sFnContext );
    }

    ULN_TRACE_LOG( &sFnContext, ULN_TRACELOG_LOW, NULL, 0,
            "%-18s| [%2"ACI_UINT32_FMT" name: %-20s"
            " stypePtr: %p] fail", "ulnDescribeColEx",
            aColumnNumber, aColumnName, aDataMtTypePtr );

    return ULN_FNCONTEXT_GET_RC( &sFnContext );
}
