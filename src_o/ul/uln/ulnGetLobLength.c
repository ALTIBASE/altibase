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

static ACI_RC ulnGetLobLengthCheckArgs(ulnFnContext *aFnContext,
                                       acp_sint16_t  aLocatorType,
                                       acp_uint32_t *aLengthPtr)
{
    ACI_TEST_RAISE(aLocatorType != SQL_C_BLOB_LOCATOR && aLocatorType != SQL_C_CLOB_LOCATOR,
                   LABEL_INVALID_LOCATOR_TYPE);

    ACI_TEST_RAISE(aLengthPtr == NULL, LABEL_INVALID_NULL);

    return ACI_SUCCESS;

    ACI_EXCEPTION(LABEL_INVALID_LOCATOR_TYPE)
    {
        /* HY003 */
        ulnError(aFnContext, ulERR_ABORT_INVALID_APP_BUFFER_TYPE, aLocatorType);
    }

    ACI_EXCEPTION(LABEL_INVALID_NULL)
    {
        /* HY009 */
        ulnError(aFnContext, ulERR_ABORT_INVALID_USE_OF_NULL_POINTER);
    }

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

SQLRETURN ulnGetLobLength(acp_sint16_t  aHandleType,
                          ulnObject    *aObject,
                          acp_uint64_t  aLocator,
                          acp_sint16_t  aLocatorType,
                          acp_uint32_t *aLengthPtr,
                          acp_uint16_t *aIsNull)
{
    ULN_FLAG(sNeedFinPtContext);
    ULN_FLAG(sNeedExit);

    ulnFnContext  sFnContext;
    ulnDbc       *sDbc;

    ULN_INIT_FUNCTION_CONTEXT(sFnContext, ULN_FID_GETLOBLENGTH, aObject, aHandleType);

    ACI_TEST(ulnEnter(&sFnContext, NULL) != ACI_SUCCESS);
    ULN_FLAG_UP(sNeedExit);

    ULN_FNCONTEXT_GET_DBC(&sFnContext, sDbc); // PROJ-2728

    /*
     * ===========================================
     * Function BEGIN
     * ===========================================
     */

    ACI_TEST(ulnGetLobLengthCheckArgs(&sFnContext, aLocatorType, aLengthPtr) != ACI_SUCCESS);
    //fix BUG-17722
    ACI_TEST(ulnInitializeProtocolContext(&sFnContext,
                                          &(sDbc->mPtContext),
                                          &(sDbc->mSession)) != ACI_SUCCESS);
    ULN_FLAG_UP(sNeedFinPtContext);

    /*
     * -----------------------------------------
     * Protocol Context
     */

    ACI_TEST(ulnLobGetSize(&sFnContext, &(sDbc->mPtContext),
                 aLocator, aLengthPtr, aIsNull) != ACI_SUCCESS);

    /*
     * Protocol Context
     * -----------------------------------------
     */

    ULN_FLAG_DOWN(sNeedFinPtContext);
    //fix BUG-17722
    ACI_TEST(ulnFinalizeProtocolContext(&sFnContext,
                          &(sDbc->mPtContext)) != ACI_SUCCESS);

    /*
     * ===========================================
     * Function END
     * ===========================================
     */

    ULN_FLAG_DOWN(sNeedExit);
    ACI_TEST(ulnExit(&sFnContext) != ACI_SUCCESS);

    return ULN_FNCONTEXT_GET_RC(&sFnContext);

    ACI_EXCEPTION_END;

    if (sNeedFinPtContext == ACP_TRUE)
    {
        //fix BUG-17722
        ulnFinalizeProtocolContext(&sFnContext, &(sDbc->mPtContext));
    }

    if (sNeedExit == ACP_TRUE)
    {
        ulnExit(&sFnContext);
    }

    return ULN_FNCONTEXT_GET_RC(&sFnContext);
}

