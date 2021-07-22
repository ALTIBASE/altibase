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

static acp_char_t *gFuncName = "SQLCPoolGetConnection";

#define GET_CONNECTION                                      \
    *aDbc = (ulnDbc *)aCPool->mDbc[sInd];                   \
    ulnwCPoolChangeConnectionStatusToInUse(aCPool, sInd);   \
    sFoundConnection = ACP_TRUE;                            \
    ulnwTrace(gFuncName, "found open Dbc %d Dbc=%p", sInd, *aDbc);


SQLRETURN ulnwCPoolGetConnection(ulnwCPool *aCPool, ulnDbc **aDbc)
{
    ulnwCPoolStat       *sStat;
    ulnwCPoolCStatus    *sCStatus;

    acp_sint32_t        sTotal;
    acp_sint32_t        sInd        = 0;
    acp_bool_t          sNeedUnlock = ACP_FALSE;
    acp_bool_t          sFoundConnection    = ACP_FALSE;

    ulnwTrace(gFuncName, "CPool=%p Dbc=%p", aCPool, aDbc);

    ACI_TEST_RAISE(aCPool == NULL, LABEL_INVALIDHANDLE);
    ACI_TEST_RAISE(aDbc == NULL, LABEL_INVALIDARG);

    ulnwDebug(gFuncName, "locking CPool->Lock");
    acpThrMutexLock(aCPool->mLock);
    sNeedUnlock = ACP_TRUE;

    *aDbc   = SQL_NULL_HANDLE;
    sStat   = &aCPool->mStat;
    sTotal  = sStat->mTotal;

    ulnwDebug(gFuncName, "total=%d connected=%d in-use=%d ", sStat->mTotal, sStat->mConnected, sStat->mInUse);

    ACI_TEST_RAISE(sStat->mTotal == 0, LABEL_NOTCONNECTED);
    ACI_TEST_RAISE(sStat->mInUse == sStat->mTotal, LABEL_NOAVAILABLECONNECTION);

    // if InUse < Connected, find open connection
    if (sStat->mConnected > sStat->mInUse)
    {
        for (sInd = 0; sInd < sTotal; sInd++)
        {
            sCStatus = &aCPool->mDbcStatus[sInd];
            if (sCStatus->mStatus == ULNW_CPOOL_CONNSTAT_OPEN)
            {
                GET_CONNECTION;
                break;
            }
        }
    }
    // if (InUse == Connected) or (InUse < Connected, but no open connection), then open up new connection
    if (sFoundConnection == ACP_FALSE)
    {
        for (sInd = 0; sInd < sTotal; sInd++)
        {
            sCStatus = &aCPool->mDbcStatus[sInd];
            if (sCStatus->mStatus == ULNW_CPOOL_CONNSTAT_INUSE)
            {
                continue;
            }
            if (sCStatus->mStatus == ULNW_CPOOL_CONNSTAT_OPEN)
            {
                GET_CONNECTION;
                break;
            }
            else    // ULNW_CPOOL_CONNSTAT_CLOSED
            {
                if (ulnwCPoolConnectConnection(aCPool, sInd) == SQL_SUCCESS)
                {
                    GET_CONNECTION;
                    break;
                }
            }
        }
    }

    ACI_TEST_RAISE(sFoundConnection == ACP_FALSE, LABEL_NOAVAILABLECONNECTION);

    ulnwCPoolManageConnections(aCPool);

    ulnwDebug(gFuncName, "unlocking CPool->Lock");
    acpThrMutexUnlock(aCPool->mLock);
    sNeedUnlock = ACP_FALSE;

    ulnwDebug(gFuncName, ULNW_DEBUG_MSG_RETURNSUCCESS);

    return SQL_SUCCESS;

    ACI_EXCEPTION(LABEL_INVALIDHANDLE)
    {
        ulnwError(gFuncName, ULNW_ERROR_MSG_INVALIDHANDLE);
    }
    ACI_EXCEPTION(LABEL_INVALIDARG)
    {
        ulnwError(gFuncName, ULNW_ERROR_MSG_INVALIDARG);
    }
    ACI_EXCEPTION(LABEL_NOTCONNECTED)
    {
        ulnwDebug(gFuncName, "total connection=%d in-use connection=%d", sStat->mTotal, sStat->mInUse);
        ulnwError(gFuncName, ULNW_ERROR_MSG_NOTCONNECTED);
    }
    ACI_EXCEPTION(LABEL_NOAVAILABLECONNECTION)
    {
        ulnwError(gFuncName, ULNW_ERROR_MSG_NOAVAILABLECONNECTION);
    }
    ACI_EXCEPTION_END;

    if (sNeedUnlock == ACP_TRUE)
    {
        acpThrMutexUnlock(aCPool->mLock);
    }

    ulnwDebug(gFuncName, ULNW_DEBUG_MSG_RETURNERROR);
    return SQL_ERROR;
}

