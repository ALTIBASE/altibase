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
#include <ulnCloseCursor.h>

/*
 * ULN_SFID_05
 * SQLCloseCursor(), STMT, S5-S7
 *      S1 [np]
 *      S3 [p]
 *  where
 *      [np] Not prepared. The statement was not prepared.
 *      [p]  Prepared. The statement was prepared.
 */
ACI_RC ulnSFID_05(ulnFnContext *aFnContext)
{
    if (aFnContext->mWhere == ULN_STATE_EXIT_POINT)
    {
        if (ulnStmtIsPrepared(aFnContext->mHandle.mStmt) == ACP_TRUE)
        {
            /* [p] */
            ULN_OBJ_SET_STATE(aFnContext->mHandle.mStmt, ULN_S_S3);
        }
        else
        {
            /* [np] */
            ULN_OBJ_SET_STATE(aFnContext->mHandle.mStmt, ULN_S_S1);
        }
    }

    /*
     * BUGBUG : DBC 상태전이함수도 호출해야 하는데, 상위 핸들의 상태 전이를
     *          어떻게 하는지 지금은 기억이 안난다.
     */

    return ACI_SUCCESS;
}

SQLRETURN ulnCloseCursor(ulnStmt *aStmt)
{
    ULN_FLAG(sNeedExit);
    ulnFnContext  sFnContext;

    ULN_INIT_FUNCTION_CONTEXT(sFnContext, ULN_FID_CLOSECURSOR, aStmt, ULN_OBJ_TYPE_STMT);

    ACI_TEST(ulnEnter(&sFnContext, NULL) != ACI_SUCCESS);
    ULN_FLAG_UP(sNeedExit);

    ACI_TEST(ulnCursorClose(&sFnContext, &aStmt->mCursor) != ACI_SUCCESS);

    ulnStmtFreeAllResult(aStmt);

    ULN_FLAG_DOWN(sNeedExit);
    ACI_TEST(ulnExit(&sFnContext) != ACI_SUCCESS);

    return ULN_FNCONTEXT_GET_RC(&sFnContext);

    ACI_EXCEPTION_END;

    ULN_IS_FLAG_UP(sNeedExit)
    {
        ulnExit(&sFnContext);
    }

    return ULN_FNCONTEXT_GET_RC(&sFnContext);
}
