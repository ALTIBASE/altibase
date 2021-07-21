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
#include <ulnGetEnvAttr.h>

/*
 * ULN_SFID_91 : ENV
 * SQLGetEnvAttr(), SQLDataSource(), ENV, E1, E2
 *      -- [1]
 *      (HY010) [2]
 * where
 *      [1] The SQL_ATTR_ODBC_VERSION environment attribute had been set on the environment.
 *      [2] The SQL_ATTR_ODBC_VERSION environment attribute had not been set on the environment.
 */
ACI_RC ulnSFID_91(ulnFnContext *aContext)
{
    acp_uint16_t sOdbcVersion;

    if(aContext->mWhere == ULN_STATE_ENTRY_POINT)
    {
        sOdbcVersion = ulnEnvGetOdbcVersion(aContext->mHandle.mEnv);

        /*
         * ODBC version's not been set
         */
        ACI_TEST_RAISE(sOdbcVersion == 0, LABEL_FUNC_SEQ);
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION(LABEL_FUNC_SEQ)
    {
        /*
         * HY010
         */
        ulnError(aContext, ulERR_ABORT_FUNCTION_SEQUENCE_ERR);
    }

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC ulnGetEnvAttrCheckArgs(ulnFnContext *aContext,
                              void         *aValuePtr,
                              acp_sint32_t *aStringLengthPtr)
{
    /*
     * HY009 : Invalide Use of Null Pointer
     */
    ACI_TEST_RAISE(aValuePtr == NULL || aStringLengthPtr == NULL, LABEL_INVALID_NULL);

    return ACI_SUCCESS;

    ACI_EXCEPTION(LABEL_INVALID_NULL)
    {
        ulnError(aContext, ulERR_ABORT_INVALID_USE_OF_NULL_POINTER);
    }

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

SQLRETURN ulnGetEnvAttr(ulnEnv       *aEnv,
                        acp_sint32_t  aAttribute,
                        void         *aValuePtr,
                        acp_sint32_t  aBufferLength,
                        acp_sint32_t *aStringLengthPtr)
{
    ULN_FLAG(sNeedExit);

    ulnFnContext sFnContext;

    ACP_UNUSED(aBufferLength);

    ULN_INIT_FUNCTION_CONTEXT(sFnContext, ULN_FID_GETENVATTR, aEnv, ULN_OBJ_TYPE_ENV);

    /*
     * Entering to ulnGetEnvAttr function.
     *
     * Checking arguments might need to add diagnostic records to the object.
     * We need to lock the object.
     * Hence, *CheckArgs() function should be preceeded by ulnEnter.
     */
    ACI_TEST(ulnEnter(&sFnContext, NULL) != ACI_SUCCESS);
    ULN_FLAG_UP(sNeedExit);

    /*
     * Check if arguments are valid
     */
    ACI_TEST(ulnGetEnvAttrCheckArgs(&sFnContext, aValuePtr, aStringLengthPtr) != ACI_SUCCESS);

    /*
     * Function's main body
     */
    switch(aAttribute)
    {
        case SQL_ATTR_ODBC_VERSION:
            *(acp_uint16_t *)aValuePtr = ulnEnvGetOdbcVersion(aEnv);
            break;

        case SQL_ATTR_OUTPUT_NTS:
            ulnEnvGetOutputNts(aEnv, (acp_sint32_t *)aValuePtr);
            break;

        case SQL_ATTR_CONNECTION_POOLING:
        case SQL_ATTR_CP_MATCH:
            /*
             * HYC00 Optional featuere not implemented
             */
            ACI_RAISE(LABEL_HYC00);
            break;

        default:
            /*
             * HY092 : Invalid attribute/option identifier
             */
            ACI_RAISE(LABEL_HY092);
            break;
    }

    /*
     * Exiting from ulnGetEnvAttr function.
     */
    ULN_FLAG_DOWN(sNeedExit);
    ACI_TEST(ulnExit(&sFnContext) != ACI_SUCCESS);

    return ULN_FNCONTEXT_GET_RC(&sFnContext);

    ACI_EXCEPTION(LABEL_HYC00)
    {
        ulnError(&sFnContext, ulERR_ABORT_OPTIONAL_FEATURE_NOT_IMPLEMENTED);
    }

    ACI_EXCEPTION(LABEL_HY092)
    {
        ulnError(&sFnContext, ulERR_ABORT_INVALID_ATTR_OPTION, aAttribute);
    }

    ACI_EXCEPTION_END;

    ULN_IS_FLAG_UP(sNeedExit)
    {
        ulnExit(&sFnContext);
    }

    return ULN_FNCONTEXT_GET_RC(&sFnContext);
}


