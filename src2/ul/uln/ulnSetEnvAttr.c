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
#include <ulnSetEnvAttr.h>

/*
 * ULN_SFID_96 : ENV
 * SQLSetEnvAttr(), ENV, E1
 *      -- [1]
 *      (HY010) [2]
 * where
 *      [1] The SQL_ATTR_ODBC_VERSION environment attribute had been set on the environment.
 *      [2] The Attribute argument was not SQL_ATTR_ODBC_VERSION, and the SQL_ATTR_ODBC_VERSION
 *          environment attribute had not been set on the environment.
 */
ACI_RC ulnSFID_96(ulnFnContext *aContext)
{
    acp_sint32_t sAttribute;

    if(aContext->mWhere == ULN_STATE_ENTRY_POINT)
    {
        sAttribute = *(acp_sint32_t *)(aContext->mArgs);
        if(sAttribute != SQL_ATTR_ODBC_VERSION)
        {
            if(ulnEnvGetOdbcVersion(aContext->mHandle.mEnv) == 0)
            {
                /*
                 * ODBC version's not been set
                 */
                ACI_RAISE(LABEL_FUNC_SEQ);
            }
        }
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

/**
 * ulnSetEnvAttr.
 *
 * SQLSetEnvAttr 에 정확하게 1:1 로 매치되는 함수.
 *
 * 주의: aValuePtr 은 상황에 따라서 32비트 정수형 /값/으로도 해석할 수 있고,
 *       /포인터/로도 해석할 수 있다.
 *       따라서 이 값이 NULL 인지를 체크하는 것은 의미가 없다. -_-;;;
 */
SQLRETURN ulnSetEnvAttr(ulnEnv *aEnv, acp_sint32_t aAttribute, void *aValuePtr, acp_sint32_t aStrLen)
{
    ULN_FLAG(sNeedExit);

    acp_uint32_t sValue;
    ulnFnContext sFnContext;

    ACP_UNUSED(aStrLen);

    ULN_INIT_FUNCTION_CONTEXT(sFnContext, ULN_FID_SETENVATTR, aEnv, ULN_OBJ_TYPE_ENV);

    sValue = (acp_uint32_t)(((acp_ulong_t)aValuePtr) & ACP_UINT32_MAX);

    /*
     * Entering into the function
     */
    ACI_TEST(ulnEnter(&sFnContext, (void *)(&aAttribute)) != ACI_SUCCESS);
    ULN_FLAG_UP(sNeedExit);

    /*
     * You don't need to check if arguments are valid. It's pointless.
     */

    switch(aAttribute)
    {
        case SQL_ATTR_ODBC_VERSION:
            ACI_TEST_RAISE( ulnEnvSetOdbcVersion(aEnv, sValue) != ACI_SUCCESS, LABEL_HY092);
            break;

        case SQL_ATTR_OUTPUT_NTS:
            // NOT implemented
            // MSDN ODBC 스펙 :
            // A call to SQLSetEnvAttr to set it to SQL_FALSE returns SQL_ERROR
            // and SQLSTATE HYC00 (Optional feature not implemented).
            ACI_TEST_RAISE(sValue == SQL_FALSE, LABEL_HYC00);
            ACI_TEST_RAISE(sValue != SQL_TRUE,  LABEL_HY024);
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
     * Exiting
     */
    ULN_FLAG_DOWN(sNeedExit);
    ACI_TEST(ulnExit(&sFnContext) != ACI_SUCCESS);

    return ULN_FNCONTEXT_GET_RC(&sFnContext);

    ACI_EXCEPTION(LABEL_HY092)
    {
        ulnError(&sFnContext, ulERR_ABORT_INVALID_ATTR_OPTION, aAttribute);
    }

    ACI_EXCEPTION(LABEL_HY024)
    {
        /*
         * Invalid Attribute Value.
         * SQL_ATTR_OUTPUT_NTS 에는 SQL_TRUE / SQL_FALSE 만 올 수 있따.
         */
        ulnError(&sFnContext, ulERR_ABORT_INVALID_ATTRIBUTE_VALUE);
    }

    ACI_EXCEPTION(LABEL_HYC00)
    {
        ulnError(&sFnContext, ulERR_ABORT_OPTIONAL_FEATURE_NOT_IMPLEMENTED);
    }

    ACI_EXCEPTION_END;

    ULN_IS_FLAG_UP(sNeedExit)
    {
        ulnExit(&sFnContext);
    }

    return ULN_FNCONTEXT_GET_RC(&sFnContext);
}

