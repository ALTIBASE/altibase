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

/***********************************************************************
 * $Id: ulsdnDistTxInfo.c 88046 2020-07-14 07:12:07Z donlet $
 **********************************************************************/

#include <ulnPrivate.h>
#include <uln.h>
#include <ulsdnDistTxInfo.h>

/**
 *  Interface for Shard Native Linker
 */

/* PROJ-2733-DistTxInfo */
SQLRETURN SQL_API ulsdnGetSCN(ulnDbc       *aDbc,
                              acp_uint64_t *aSCN)
{
    ulnFnContext sFnContext;

    ULN_INIT_FUNCTION_CONTEXT(sFnContext, ULN_FID_NONE, aDbc, ULN_OBJ_TYPE_DBC);

    ACI_TEST_RAISE(aDbc == NULL, LABEL_INVALID_HANDLE);
    ACI_TEST_RAISE(aSCN == NULL, LABEL_INVALID_USE_OF_NULL);

    *aSCN = aDbc->mSCN;

    return ULN_FNCONTEXT_GET_RC(&sFnContext);

    ACI_EXCEPTION(LABEL_INVALID_HANDLE)
    {
        ulnError(&sFnContext, ulERR_ABORT_INVALID_HANDLE);
    }
    ACI_EXCEPTION(LABEL_INVALID_USE_OF_NULL)
    {
        ulnError(&sFnContext, ulERR_ABORT_INVALID_USE_OF_NULL_POINTER);
    }
    ACI_EXCEPTION_END;

    return ULN_FNCONTEXT_GET_RC(&sFnContext);
}

SQLRETURN SQL_API ulsdnSetSCN(ulnDbc       *aDbc,
                              acp_uint64_t *aSCN)
{
    ulnFnContext sFnContext;

    ULN_INIT_FUNCTION_CONTEXT(sFnContext, ULN_FID_NONE, aDbc, ULN_OBJ_TYPE_DBC);

    ACI_TEST_RAISE(aDbc == NULL, LABEL_INVALID_HANDLE);
    ACI_TEST_RAISE(aSCN == NULL, LABEL_INVALID_USE_OF_NULL);

    aDbc->mSCN = *aSCN;

    return ULN_FNCONTEXT_GET_RC(&sFnContext);

    ACI_EXCEPTION(LABEL_INVALID_HANDLE)
    {
        ulnError(&sFnContext, ulERR_ABORT_INVALID_HANDLE);
    }
    ACI_EXCEPTION(LABEL_INVALID_USE_OF_NULL)
    {
        ulnError(&sFnContext, ulERR_ABORT_INVALID_USE_OF_NULL_POINTER);
    }
    ACI_EXCEPTION_END;

    return ULN_FNCONTEXT_GET_RC(&sFnContext);
}

SQLRETURN SQL_API ulsdnSetTxFirstStmtSCN(ulnDbc       *aDbc,
                                         acp_uint64_t *aTxFirstStmtSCN)
{
    ulnFnContext sFnContext;

    ULN_INIT_FUNCTION_CONTEXT(sFnContext, ULN_FID_NONE, aDbc, ULN_OBJ_TYPE_DBC);

    ACI_TEST_RAISE(aDbc == NULL, LABEL_INVALID_HANDLE);
    ACI_TEST_RAISE(aTxFirstStmtSCN == NULL, LABEL_INVALID_USE_OF_NULL);

    aDbc->mTxFirstStmtSCN = *aTxFirstStmtSCN;

    return ULN_FNCONTEXT_GET_RC(&sFnContext);

    ACI_EXCEPTION(LABEL_INVALID_HANDLE)
    {
        ulnError(&sFnContext, ulERR_ABORT_INVALID_HANDLE);
    }
    ACI_EXCEPTION(LABEL_INVALID_USE_OF_NULL)
    {
        ulnError(&sFnContext, ulERR_ABORT_INVALID_USE_OF_NULL_POINTER);
    }
    ACI_EXCEPTION_END;

    return ULN_FNCONTEXT_GET_RC(&sFnContext);
}

SQLRETURN SQL_API ulsdnSetTxFirstStmtTime(ulnDbc       *aDbc,
                                          acp_sint64_t  aTxFirstStmtTime)
{
    ulnFnContext sFnContext;

    ULN_INIT_FUNCTION_CONTEXT(sFnContext, ULN_FID_NONE, aDbc, ULN_OBJ_TYPE_DBC);

    ACI_TEST_RAISE(aDbc == NULL, LABEL_INVALID_HANDLE);

    aDbc->mTxFirstStmtTime = aTxFirstStmtTime;

    return ULN_FNCONTEXT_GET_RC(&sFnContext);

    ACI_EXCEPTION(LABEL_INVALID_HANDLE)
    {
        ulnError(&sFnContext, ulERR_ABORT_INVALID_HANDLE);
    }
    ACI_EXCEPTION_END;

    return ULN_FNCONTEXT_GET_RC(&sFnContext);
}

SQLRETURN SQL_API ulsdnSetDistLevel(ulnDbc        *aDbc,
                                    ulsdDistLevel  aDistLevel)
{
    ulnFnContext sFnContext;

    ULN_INIT_FUNCTION_CONTEXT(sFnContext, ULN_FID_NONE, aDbc, ULN_OBJ_TYPE_DBC);

    ACI_TEST_RAISE(aDbc == NULL, LABEL_INVALID_HANDLE);

    aDbc->mDistLevel = aDistLevel;

    return ULN_FNCONTEXT_GET_RC(&sFnContext);

    ACI_EXCEPTION(LABEL_INVALID_HANDLE)
    {
        ulnError(&sFnContext, ulERR_ABORT_INVALID_HANDLE);
    }
    ACI_EXCEPTION_END;

    return ULN_FNCONTEXT_GET_RC(&sFnContext);
}
