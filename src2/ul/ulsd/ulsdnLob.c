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
#include <ulsdnLob.h>

static ACI_RC ulsdnLobWriteEmpty(ulnFnContext *aFnContext,
                                 ulnPtContext *aPtContext,
                                 ulnLob       *aLob)
{
    ulnDbc       *sDbc;
    cmiProtocol          sPacket;
    cmiProtocolContext  *sCtx     = &(aPtContext->mCmiPtContext);
    acp_uint64_t         sLobLocatorVal;
    acp_uint32_t         sSize = 0;
    acp_uint16_t         sOrgWriteCursor = CMI_GET_CURSOR(sCtx);

    ULN_FNCONTEXT_GET_DBC(aFnContext, sDbc);

    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_TEST_RAISE(sDbc == NULL, InvalidHandleException);

    ACI_TEST_RAISE(aLob->mState != ULN_LOB_ST_OPENED, LABEL_LOB_NOT_OPENED);

    ULN_GET_LOB_LOCATOR_VALUE(&sLobLocatorVal,&(aLob->mLocatorID));
    sPacket.mOpID = CMP_OP_DB_LobPut;

    CMI_WRITE_CHECK(sCtx, 13 + sSize);

    CMI_WOP(sCtx, CMP_OP_DB_LobPut);
    CMI_WR8(sCtx, &sLobLocatorVal);
    CMI_WR4(sCtx, &sSize);          // size

    ACI_TEST(ulnWriteProtocol(aFnContext, aPtContext, &sPacket) != ACI_SUCCESS);

    return ACI_SUCCESS;

    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_EXCEPTION(InvalidHandleException)
    {
        ULN_FNCONTEXT_SET_RC(aFnContext, SQL_INVALID_HANDLE);
    }
    ACI_EXCEPTION(LABEL_LOB_NOT_OPENED)
    {
        ulnError(aFnContext, ulERR_FATAL_LOB_NOT_OPENED, "LobWriteEmpty");
    }
    ACI_EXCEPTION_END;

    CMI_SET_CURSOR(sCtx, sOrgWriteCursor);

    return ACI_FAILURE;
}

SQLRETURN ulsdnLobPrepare4Write(acp_sint16_t  aHandleType,
                                ulnObject    *aObject,
                                acp_sint16_t  aLocatorCType,
                                acp_uint64_t  aLocator,
                                acp_uint32_t  aStartOffset,
                                acp_uint32_t  aSize)
{
    ULN_FLAG(sNeedExit);

    ulnFnContext  sFnContext;

    ulnDbc       *sDbc;
    ulnLob        sLob;
    ulnMTypeID    sMTYPE;

    ULN_INIT_FUNCTION_CONTEXT(sFnContext, ULN_FID_PUTLOB, aObject, aHandleType);

    /*
     * Enter
     */

    ACI_TEST(ulnEnter(&sFnContext, NULL) != ACI_SUCCESS);

    ULN_FNCONTEXT_GET_DBC(&sFnContext, sDbc);
    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_TEST_RAISE(sDbc == NULL, InvalidHandleException);

    ULN_FLAG_UP(sNeedExit);

    switch (aLocatorCType)
    {
        case SQL_C_CLOB_LOCATOR:
            sMTYPE = ULN_MTYPE_CLOB;
            break;

        case SQL_C_BLOB_LOCATOR:
            sMTYPE = ULN_MTYPE_BLOB;
            break;

        default:
            ACI_RAISE(LABEL_INVALID_LOCATOR_TYPE);
    }

    /*
     * ulnLob 구조체 초기화
     */

    ulnLobInitialize(&sLob, sMTYPE);                        /* ULN_LOB_ST_INITIALIZED */
    sLob.mOp->mSetLocator(&sFnContext, &sLob, aLocator);    /* ULN_LOB_ST_LOCATOR */
    sLob.mState = ULN_LOB_ST_OPENED;                        /* ULN_LOB_ST_OPENED */

    /*
     * LOB 데이터 전송
     */
    // fix BUG-17722
    ACI_TEST(ulnInitializeProtocolContext(&sFnContext,
                                          &(sDbc->mPtContext),
                                          &(sDbc->mSession)) != ACI_SUCCESS);

    /*
     * UPDATE BEGIN
     */
    ACI_TEST(sLob.mOp->mPrepare4Write(&sFnContext,
                               &(sDbc->mPtContext),
                               &sLob,
                               aStartOffset,
                               aSize) != ACI_SUCCESS);

    /*
     * Exit
     */

    ULN_FLAG_DOWN(sNeedExit);

    ACI_TEST(ulnExit(&sFnContext) != ACI_SUCCESS);

    return ULN_FNCONTEXT_GET_RC(&sFnContext);

    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_EXCEPTION(InvalidHandleException)
    {
        ULN_FNCONTEXT_SET_RC(&sFnContext, SQL_INVALID_HANDLE);
    }
    ACI_EXCEPTION(LABEL_INVALID_LOCATOR_TYPE)
    {
        ulnError(&sFnContext, ulERR_ABORT_INVALID_APP_BUFFER_TYPE, aLocatorCType);
    }
    ACI_EXCEPTION_END;

    ULN_IS_FLAG_UP(sNeedExit)
    {
        ulnExit(&sFnContext);
    }

    return ULN_FNCONTEXT_GET_RC(&sFnContext);
}

SQLRETURN ulsdnLobWrite(acp_sint16_t  aHandleType,
                        ulnObject    *aObject,
                        acp_sint16_t  aLocatorCType,
                        acp_uint64_t  aLocator,
                        acp_sint16_t  aSourceCType,
                        void         *aBuffer,
                        acp_uint32_t  aBufferSize)
{
    ULN_FLAG(sNeedExit);

    ulnFnContext  sFnContext;

    ulnDbc       *sDbc;
    ulnLob        sLob;
    ulnLobBuffer  sLobBuffer;
    ulnMTypeID    sMTYPE;
    ulnCTypeID    sCTYPE;

    ULN_INIT_FUNCTION_CONTEXT(sFnContext, ULN_FID_PUTLOB, aObject, aHandleType);

    /*
     * Enter
     */

    ACI_TEST(ulnEnter(&sFnContext, NULL) != ACI_SUCCESS);

    ULN_FNCONTEXT_GET_DBC(&sFnContext, sDbc);
    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_TEST_RAISE(sDbc == NULL, InvalidHandleException);

    ULN_FLAG_UP(sNeedExit);

    /*
     * BUGBUG : 저장된 LOB 데이터의 크기를 넘어선 offset 이 입력되면 에러를 낸다
     *          aFromPosition + aLength > ID_UINT_MAX --> ERROR!!!
     *          처리해 ... 줄까, 말까.. -_-;; 서버에서 처리하게 놔 둘까...
     */

    ACI_TEST_RAISE(aBuffer == NULL, LABEL_INVALID_NULL);

    switch (aLocatorCType)
    {
        case SQL_C_CLOB_LOCATOR:
            sMTYPE = ULN_MTYPE_CLOB;
            break;

        case SQL_C_BLOB_LOCATOR:
            sMTYPE = ULN_MTYPE_BLOB;
            break;

        default:
            ACI_RAISE(LABEL_INVALID_LOCATOR_TYPE);
    }

    switch (aSourceCType)
    {
        case SQL_C_CHAR:
            sCTYPE = ULN_CTYPE_CHAR;
            break;

        case SQL_C_BINARY:
            sCTYPE = ULN_CTYPE_BINARY;
            break;

        default:
            ACI_RAISE(LABEL_INVALID_BUFFER_TYPE);
    }

    /*
     * ulnLob 구조체 초기화
     */

    ulnLobInitialize(&sLob, sMTYPE);                        /* ULN_LOB_ST_INITIALIZED */
    sLob.mOp->mSetLocator(&sFnContext, &sLob, aLocator);    /* ULN_LOB_ST_LOCATOR */
    sLob.mState = ULN_LOB_ST_OPENED;                        /* ULN_LOB_ST_OPENED */

    if ( aBufferSize > 0 )
    {
        /*
         * ulnLobBuffer 초기화 및 준비
         */

        ulnLobBufferInitialize(&sLobBuffer,
                               sDbc,
                               sLob.mType,
                               sCTYPE,
                               (acp_uint8_t *)aBuffer,
                               aBufferSize);

        /*
         * LOB 데이터 전송
         */

        ACI_TEST(sLob.mOp->mWrite(&sFnContext,
                                  &(sDbc->mPtContext),
                                  &sLob,
                                  &sLobBuffer) != ACI_SUCCESS);

        /*
         * ulnLobBuffer 정리
         */

        sLobBuffer.mOp->mFinalize(&sFnContext, &sLobBuffer);
    }
    else
    {
        sLob.mBuffer = NULL;
        ACI_TEST( ulsdnLobWriteEmpty( &sFnContext,
                                      &(sDbc->mPtContext),
                                      &sLob )
                  != ACI_SUCCESS );
    }

    /*
     * Exit
     */

    ULN_FLAG_DOWN(sNeedExit);

    ACI_TEST(ulnExit(&sFnContext) != ACI_SUCCESS);

    return ULN_FNCONTEXT_GET_RC(&sFnContext);

    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_EXCEPTION(InvalidHandleException)
    {
        ULN_FNCONTEXT_SET_RC(&sFnContext, SQL_INVALID_HANDLE);
    }
    ACI_EXCEPTION(LABEL_INVALID_NULL)
    {
        /* HY009 : invalid use of null pointer */
        ulnError(&sFnContext, ulERR_ABORT_INVALID_USE_OF_NULL_POINTER);
    }

    ACI_EXCEPTION(LABEL_INVALID_BUFFER_TYPE)
    {
        ulnError(&sFnContext,
                 ulERR_ABORT_RESTRICTED_DATATYPE_VIOLATION,
                 aSourceCType,
                 (aLocatorCType == SQL_C_BLOB_LOCATOR) ? SQL_BLOB : SQL_CLOB);
    }
    ACI_EXCEPTION(LABEL_INVALID_LOCATOR_TYPE)
    {
        ulnError(&sFnContext, ulERR_ABORT_INVALID_APP_BUFFER_TYPE, aLocatorCType);
    }
    ACI_EXCEPTION_END;

    ULN_IS_FLAG_UP(sNeedExit)
    {
        ulnExit(&sFnContext);
    }

    return ULN_FNCONTEXT_GET_RC(&sFnContext);
}

SQLRETURN ulsdnLobFinishWrite(acp_sint16_t  aHandleType,
                              ulnObject    *aObject,
                              acp_sint16_t  aLocatorCType,
                              acp_uint64_t  aLocator)
{
    ULN_FLAG(sNeedExit);
    ULN_FLAG(sNeedFinPtContext);

    ulnFnContext  sFnContext;

    ulnDbc       *sDbc;
    ulnLob        sLob;
    ulnMTypeID    sMTYPE;

    ULN_INIT_FUNCTION_CONTEXT(sFnContext, ULN_FID_PUTLOB, aObject, aHandleType);

    /*
     * Enter
     */

    ACI_TEST(ulnEnter(&sFnContext, NULL) != ACI_SUCCESS);

    ULN_FNCONTEXT_GET_DBC(&sFnContext, sDbc);
    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_TEST_RAISE(sDbc == NULL, InvalidHandleException);

    ULN_FLAG_UP(sNeedExit);
    ULN_FLAG_UP(sNeedFinPtContext);

    /*
     * BUGBUG : 저장된 LOB 데이터의 크기를 넘어선 offset 이 입력되면 에러를 낸다
     *          aFromPosition + aLength > ID_UINT_MAX --> ERROR!!!
     *          처리해 ... 줄까, 말까.. -_-;; 서버에서 처리하게 놔 둘까...
     */

    switch (aLocatorCType)
    {
        case SQL_C_CLOB_LOCATOR:
            sMTYPE = ULN_MTYPE_CLOB;
            break;

        case SQL_C_BLOB_LOCATOR:
            sMTYPE = ULN_MTYPE_BLOB;
            break;

        default:
            ACI_RAISE(LABEL_INVALID_LOCATOR_TYPE);
    }

    /*
     * ulnLob 구조체 초기화
     */

    ulnLobInitialize(&sLob, sMTYPE);                        /* ULN_LOB_ST_INITIALIZED */
    sLob.mOp->mSetLocator(&sFnContext, &sLob, aLocator);    /* ULN_LOB_ST_LOCATOR */
    sLob.mState = ULN_LOB_ST_OPENED;                        /* ULN_LOB_ST_OPENED */


    ACI_TEST(sLob.mOp->mFinishWrite(&sFnContext,
                                    &(sDbc->mPtContext),
                                    &sLob) != ACI_SUCCESS);

    ULN_FLAG_DOWN(sNeedFinPtContext);

    // fix BUG-17722
    ACI_TEST(ulnFinalizeProtocolContext(&sFnContext,
                        &(sDbc->mPtContext)) != ACI_SUCCESS);

    /*
     * Exit
     */

    ULN_FLAG_DOWN(sNeedExit);

    ACI_TEST(ulnExit(&sFnContext) != ACI_SUCCESS);

    return ULN_FNCONTEXT_GET_RC(&sFnContext);

    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_EXCEPTION(InvalidHandleException)
    {
        ULN_FNCONTEXT_SET_RC(&sFnContext, SQL_INVALID_HANDLE);
    }
    ACI_EXCEPTION(LABEL_INVALID_LOCATOR_TYPE)
    {
        ulnError(&sFnContext, ulERR_ABORT_INVALID_APP_BUFFER_TYPE, aLocatorCType);
    }
    ACI_EXCEPTION_END;

    ULN_IS_FLAG_UP(sNeedFinPtContext)
    {
        ulnFinalizeProtocolContext(&sFnContext,&(sDbc->mPtContext));
    }

    ULN_IS_FLAG_UP(sNeedExit)
    {
        ulnExit(&sFnContext);
    }

    return ULN_FNCONTEXT_GET_RC(&sFnContext);
}

