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

/* 
 * PROJ-2047 Strengthening LOB - Added Interfaces
 */
SQLRETURN ulnTrimLob(acp_sint16_t   aHandleType,
                     ulnObject     *aObject,
                     acp_sint16_t   aLocatorCType,
                     acp_uint64_t   aLocator,
                     acp_uint32_t   aStartOffset)
{
    ULN_FLAG(sNeedExit);
    ULN_FLAG(sNeedFinPtContext);

    ulnFnContext  sFnContext;
    ulnPtContext *sPtContext = NULL;
    ulnDbc       *sDbc;
    ulnLob        sLob;
    ulnMTypeID    sMTYPE;

    ULN_INIT_FUNCTION_CONTEXT(sFnContext, ULN_FID_TRIMLOB, aObject, aHandleType);

    /*
     * Enter
     */
    ACI_TEST(ulnEnter(&sFnContext, NULL) != ACI_SUCCESS);

    ULN_FNCONTEXT_GET_DBC(&sFnContext, sDbc);

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

    ACI_TEST(ulnInitializeProtocolContext(&sFnContext,
                                          &(sDbc->mPtContext),
                                          &(sDbc->mSession)) != ACI_SUCCESS);

    ULN_FLAG_UP(sNeedFinPtContext);

    ACI_TEST(sLob.mOp->mTrim(&sFnContext,
                             &(sDbc->mPtContext),
                             &sLob,
                             aStartOffset) != ACI_SUCCESS);

    ULN_FLAG_DOWN(sNeedFinPtContext);

    ACI_TEST(ulnFinalizeProtocolContext(&sFnContext,
                                        &(sDbc->mPtContext))
                != ACI_SUCCESS);

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
        ulnFinalizeProtocolContext(&sFnContext,&(sDbc->mPtContext) );
    }

    ULN_IS_FLAG_UP(sNeedExit)
    {
        ulnExit(&sFnContext);
    }

    return ULN_FNCONTEXT_GET_RC(&sFnContext);
}
