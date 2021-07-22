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

SQLRETURN ulnFreeLob(acp_sint16_t  aHandleType,
                     ulnObject    *aObject,
                     acp_uint64_t  aLocator)
{
    acp_bool_t    sNeedExit = ACP_FALSE;
    acp_bool_t    sNeedFinPtContext = ACP_FALSE;
    ulnDbc       *sDbc;

    ulnFnContext  sFnContext;

    ULN_INIT_FUNCTION_CONTEXT(sFnContext, ULN_FID_FREELOB, aObject, aHandleType);

    /*
     * Enter
     */
    ACI_TEST(ulnEnter(&sFnContext, NULL) != ACI_SUCCESS);

    sNeedExit = ACP_TRUE;

    ULN_FNCONTEXT_GET_DBC(&sFnContext, sDbc); // PROJ-2728

    /*
     * initialize protocol context
     */
    //fix BUG-17722
    ACI_TEST(ulnInitializeProtocolContext(&sFnContext,
                                          &(sDbc->mPtContext),
                                          &(sDbc->mSession))
             != ACI_SUCCESS);
    sNeedFinPtContext = ACP_TRUE;

    //fix BUG-17722
    ACI_TEST(ulnLobFreeLocator(&sFnContext,&(sDbc->mPtContext),
                               aLocator) != ACI_SUCCESS);

    if (sDbc->mConnType == ULN_CONNTYPE_IPC)
    {
        /*
         * finalize protocol context
         */
        ACI_TEST(ulnFinalizeProtocolContext(&sFnContext, &(sDbc->mPtContext)) != ACI_SUCCESS);
    }

    /*
     * Exit
     */
    sNeedExit = ACP_FALSE;

    ACI_TEST(ulnExit(&sFnContext) != ACI_SUCCESS);

    return ULN_FNCONTEXT_GET_RC(&sFnContext);

    ACI_EXCEPTION_END;

    if(sNeedFinPtContext == ACP_TRUE)
    {
        //fix BUG-17722
        ulnFinalizeProtocolContext(&sFnContext,&(sDbc->mPtContext));
    }

    if(sNeedExit == ACP_TRUE)
    {
        ulnExit(&sFnContext);
    }

    return ULN_FNCONTEXT_GET_RC(&sFnContext);
}

