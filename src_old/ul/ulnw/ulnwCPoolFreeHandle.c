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

#include <ulnwCPool.h>

static acp_char_t *gFuncName = "SQLCPoolFreeHandle";

SQLRETURN ulnwCPoolFreeHandle(ulnwCPool *aCPool)
{
    ulnwTrace(gFuncName, "CPool=%p", aCPool);

    ACI_TEST_RAISE(aCPool == NULL, LABEL_INVALIDHANDLE);
    ACI_TEST_RAISE(aCPool->mStat.mTotal != 0, LABEL_FUNCTIONSEQUENCEERROR);

    ulnwDebug(gFuncName, "destroying CPool->Lock(%p)", aCPool->mLock);
    ACI_TEST_RAISE(acpThrMutexLock(aCPool->mLock) != ACP_RC_SUCCESS, LABEL_DESTROYMUTEXFAILED);
    ACI_TEST_RAISE(acpThrMutexUnlock(aCPool->mLock) != ACP_RC_SUCCESS, LABEL_DESTROYMUTEXFAILED);
    ACI_TEST_RAISE(acpThrMutexDestroy(aCPool->mLock) != ACP_RC_SUCCESS, LABEL_DESTROYMUTEXFAILED);

    ulnwDebug(gFuncName, "freeing CPool->Env with SQLFreeHandle");
    ulnFreeHandle(SQL_HANDLE_ENV, aCPool->mEnv);
    aCPool->mEnv = SQL_NULL_HANDLE;

    ulnwDebug(gFuncName, "freeing CPool->Lock(%p)", aCPool->mLock);
    acpMemFree(aCPool->mLock);
    aCPool->mLock = NULL;

    ulnwDebug(gFuncName, "freeing CPool(%p)", aCPool);
    acpMemFree(aCPool);

    ulnwDebug(gFuncName, ULNW_DEBUG_MSG_RETURNSUCCESS);

    return SQL_SUCCESS;

    ACI_EXCEPTION(LABEL_INVALIDHANDLE)
    {
        ulnwError(gFuncName, ULNW_ERROR_MSG_INVALIDHANDLE);
    }
    ACI_EXCEPTION(LABEL_FUNCTIONSEQUENCEERROR)
    {
        ulnwDebug(gFuncName, "%d connections are still in use", aCPool->mStat.mInUse);
        ulnwDebug(gFuncName, "%d connections still exist(s)", aCPool->mStat.mConnected);
        if (aCPool->mStat.mInUse > 0)
        {
            ulnwDebug(gFuncName, "function sequence error (%d connections are still in use)", aCPool->mStat.mInUse);
        }
        else // aCPool->mStat.mTotal > 0
        {
            ulnwDebug(gFuncName, "function sequence error (call SQLCPoolDisconnect before freeing SQLHDBCP)");
        }
        ulnwError(gFuncName, ULNW_ERROR_MSG_IMPOSSIBLE);
    }
    ACI_EXCEPTION(LABEL_DESTROYMUTEXFAILED)
    {
        ulnwError(gFuncName, "SQLHDBCP still in use");
    }
    ACI_EXCEPTION_END;

    ulnwDebug(gFuncName, ULNW_DEBUG_MSG_RETURNERROR);
    return SQL_ERROR;
}


