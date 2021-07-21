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

static acp_char_t   *gFuncName = "SQLCPoolFinalize";

SQLRETURN ulnwCPoolFinalize(ulnwCPool *aCPool)
{
    acp_sint32_t        sInd = 0;
    acp_sint32_t        sTotal = 0;
    ulnwCPoolCStatus    *sCStatus;

    ulnwTrace(gFuncName, "CPool=%p", aCPool);

    ACI_TEST_RAISE(aCPool == NULL, LABEL_INVALIDHANDLE);
    if (aCPool->mStat.mTotal == 0)
    {
        ulnwDebug(gFuncName, "connection pool is not connected yet");
        ACI_RAISE(LABEL_SKIP);
    }
    ACI_TEST_RAISE(aCPool->mStat.mInUse > 0, LABEL_STILLINUSE);

    // locking
    ulnwDebug(gFuncName, "locking CPool->Lock");
    ACI_TEST_RAISE(acpThrMutexLock(aCPool->mLock) != ACP_RC_SUCCESS,
                   LABEL_LOCKINGFAILED);

    sTotal = aCPool->mStat.mTotal;

    ulnwDebug(gFuncName, "number of total connections=%d, opened connection=%d", 
            aCPool->mStat.mTotal, aCPool->mStat.mConnected);

    // disconnect connections
    ulnwDebug(gFuncName, "disconnecting connections");
    for (sInd = 0; sInd < sTotal; sInd++)
    {
       sCStatus = &aCPool->mDbcStatus[sInd];
       ulnwCPoolUpdateConnectionStatus(aCPool, sInd);

       ulnwDebug(gFuncName, "%02d %s", sInd, 
               (sCStatus->mStatus == ULNW_CPOOL_CONNSTAT_OPEN) ? "OPEN" : "CLOSED");

       if (sCStatus->mStatus == ULNW_CPOOL_CONNSTAT_CLOSED)
       {
           continue;
       }
       ulnwCPoolDisconnectConnection(aCPool, sInd);
       ulnwCPoolChangeConnectionStatusToClosed(aCPool, sInd);
    }

    // free DBC handles
    ulnwDebug(gFuncName, "freeing Dbc handles");
    for (sInd = 0; sInd < sTotal; sInd++)
    {
        if (ulnFreeHandle(SQL_HANDLE_DBC, aCPool->mDbc[sInd]) != SQL_SUCCESS)
        {
            ulnwDebug(gFuncName, "%d SQLFreeHandle(%p) failed", sInd, aCPool->mDbc[sInd]);
            ulnwTraceSqlError(gFuncName, "SQLFreeHandle", SQL_HANDLE_DBC, aCPool->mDbc);
        }
    }

    aCPool->mStat.mTotal = 0;

    // free allocated memory
    ulnwDebug(gFuncName, "freeing Dbc (%p)", aCPool->mDbc);
    acpMemFree(aCPool->mDbc);
    aCPool->mDbc = NULL;
    ulnwDebug(gFuncName, "freeing Dbc status (%p)", aCPool->mDbcStatus);
    acpMemFree(aCPool->mDbcStatus);
    aCPool->mDbcStatus = NULL;

    // unlocking
    ulnwDebug(gFuncName, "unlocking CPool->Lock");
    ACI_TEST_RAISE(acpThrMutexUnlock(aCPool->mLock) != ACP_RC_SUCCESS,
                   LABEL_UNLOCKINGFAILED);

    ACI_EXCEPTION_CONT(LABEL_SKIP);

    ulnwDebug(gFuncName, ULNW_DEBUG_MSG_RETURNSUCCESS);

    return SQL_SUCCESS;

    ACI_EXCEPTION(LABEL_INVALIDHANDLE)
    {
        ulnwError(gFuncName, ULNW_ERROR_MSG_INVALIDHANDLE);
    }
    ACI_EXCEPTION(LABEL_STILLINUSE)
    {
        ulnwDebug(gFuncName, "%d connections are still in use", aCPool->mStat.mInUse);
        ulnwError(gFuncName, ULNW_ERROR_MSG_IMPOSSIBLE);
    }
    ACI_EXCEPTION(LABEL_LOCKINGFAILED)
    {
        ulnwDebug(gFuncName, "locking CPool->Lock failed");
        ulnwError(gFuncName, ULNW_ERROR_MSG_IMPOSSIBLE);
    }
    ACI_EXCEPTION(LABEL_UNLOCKINGFAILED)
    {
        ulnwDebug(gFuncName, "unlocking CPool->Lock failed");
        ulnwError(gFuncName, ULNW_ERROR_MSG_IMPOSSIBLE);
    }
    ACI_EXCEPTION_END;

    ulnwDebug(gFuncName, ULNW_DEBUG_MSG_RETURNERROR);
    return SQL_ERROR;
}
