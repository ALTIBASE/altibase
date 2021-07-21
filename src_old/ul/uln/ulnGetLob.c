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
#include <ulnLob.h>

SQLRETURN ulnGetLob(acp_sint16_t  aHandleType,
                    ulnObject    *aObject,
                    acp_sint16_t  aLocatorCType,
                    acp_uint64_t  aSrcLocator,
                    acp_uint32_t  aFromPosition,
                    acp_uint32_t  aForLength,
                    acp_sint16_t  aTargetCType,
                    void         *aBuffer,
                    acp_uint32_t  aBufferSize,
                    acp_uint32_t *aLengthWritten)
{
    ULN_FLAG(sNeedExit);
    ULN_FLAG(sNeedFinPtContext);

    ulnFnContext  sFnContext;
    ulnPtContext *sPtContext = NULL;
    ulnDbc       *sDbc;
    ulnLob        sLob;
    ulnLobBuffer  sLobBuffer;
    ulnMTypeID    sMTYPE;
    ulnCTypeID    sCTYPE;

    ULN_INIT_FUNCTION_CONTEXT(sFnContext, ULN_FID_GETLOB, aObject, aHandleType);

    /*
     * Enter
     */

    ACI_TEST(ulnEnter(&sFnContext, NULL) != ACI_SUCCESS);

    ULN_FNCONTEXT_GET_DBC(&sFnContext, sDbc);        //BUG-28749

    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_TEST_RAISE(sDbc == NULL, InvalidHandleException);

    ULN_FLAG_UP(sNeedExit);

    sPtContext = &(sDbc->mPtContext);
    /* BUG-44125 [mm-cli] IPCDA 모드 테스트 중 hang - iloader CLOB */
    ACI_TEST_RAISE(cmiGetLinkImpl(&sPtContext->mCmiPtContext) == CMI_LINK_IMPL_IPCDA,
                   IPCDANotSupport);
    /*
     * check arguments
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

    switch (aTargetCType)
    {
        case SQL_C_CHAR:
            sCTYPE = ULN_CTYPE_CHAR;
            break;

        case SQL_C_WCHAR:
            sCTYPE = ULN_CTYPE_WCHAR;
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
    sLob.mOp->mSetLocator(&sFnContext, &sLob, aSrcLocator); /* ULN_LOB_ST_LOCATOR */
    sLob.mState = ULN_LOB_ST_OPENED;                        /* ULN_LOB_ST_OPENED */

    /*
     * ulnLobBuffer 초기화 및 준비
     */

    ulnLobBufferInitialize(&sLobBuffer,
                           sDbc,
                           sLob.mType,
                           sCTYPE,
                           (acp_uint8_t *)aBuffer,
                           aBufferSize);

    sLobBuffer.mOp->mPrepare(&sFnContext, &sLobBuffer);

    /*
     * 서버에서 데이터 가져와서 사용자 버퍼에 쓰기
     */
    //fix BUG-17722
    ACI_TEST(ulnInitializeProtocolContext(&sFnContext,
                                          &(sDbc->mPtContext),
                                          &(sDbc->mSession)) != ACI_SUCCESS);

    ULN_FLAG_UP(sNeedFinPtContext);
    //fix BUG-17722
    ACI_TEST(sLob.mOp->mGetData(&sFnContext,
                                &(sDbc->mPtContext),
                                &sLob,
                                &sLobBuffer,
                                aFromPosition,
                                aForLength) != ACI_SUCCESS);

    ULN_FLAG_DOWN(sNeedFinPtContext);
    //fix BUG-17722
    ACI_TEST(ulnFinalizeProtocolContext(&sFnContext,
                                        &(sDbc->mPtContext))
                != ACI_SUCCESS);

    /*
     * 가져온 데이터의 길이 반환
     */

    if (aLengthWritten != NULL)
    {
        /* PROJ-2047 Strengthening LOB - Partial Converting */
        switch (sMTYPE)
        {
            case ULN_MTYPE_BLOB:
                *aLengthWritten = sLob.mSizeRetrieved;
                break;

            case ULN_MTYPE_CLOB:
                if (sCTYPE == ULN_CTYPE_CHAR && 
                    sDbc->mCharsetLangModule == sDbc->mClientCharsetLangModule)
                {
                    *aLengthWritten = sLob.mSizeRetrieved;
                }
                else if (sCTYPE != ULN_CTYPE_BINARY)
                {
                    *aLengthWritten = ulnCharSetGetCopiedDestLen(&sLobBuffer.mCharSet);
                }
                else
                {
                    *aLengthWritten = sLob.mSizeRetrieved;
                }
                break;

            default:
                break;
        }
    }

    /*
     * ulnLobBuffer 정리
     */

    sLobBuffer.mOp->mFinalize(&sFnContext, &sLobBuffer);

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
        /* HY009 */
        ulnError(&sFnContext, ulERR_ABORT_INVALID_USE_OF_NULL_POINTER);
    }

    ACI_EXCEPTION(LABEL_INVALID_BUFFER_TYPE)
    {
        /* 07006 */
        ulnError(&sFnContext,
                 ulERR_ABORT_RESTRICTED_DATATYPE_VIOLATION,
                 aTargetCType,
                 (aLocatorCType == SQL_C_BLOB_LOCATOR) ? SQL_BLOB : SQL_CLOB);
    }

    ACI_EXCEPTION(LABEL_INVALID_LOCATOR_TYPE)
    {
        /* HY003 */
        ulnError(&sFnContext, ulERR_ABORT_INVALID_APP_BUFFER_TYPE, aLocatorCType);
    }
    ACI_EXCEPTION(IPCDANotSupport)
    {
        ulnError(&sFnContext, ulERR_ABORT_IPCDA_UNSUPPORTED_FUNCTION);
    }
    ACI_EXCEPTION_END;

    ULN_IS_FLAG_UP(sNeedFinPtContext)
    {
        //fix BUG-17722
        ulnFinalizeProtocolContext(&sFnContext,&(sDbc->mPtContext) );
    }

    ULN_IS_FLAG_UP(sNeedExit)
    {
        ulnExit(&sFnContext);
    }

    return ULN_FNCONTEXT_GET_RC(&sFnContext);
}
