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

static acp_char_t *gFuncName = "SQLCPoolReturnConnection";

SQLRETURN ulnwCPoolReturnConnection(ulnwCPool *aCPool, ulnDbc *aDbc)
{
    ulnwCPoolStat       *sStat;
    ulnwCPoolCfg        *sCfg;

    acp_sint32_t        sTotal;
    acp_sint32_t        sInd;
    acp_bool_t          sNeedUnlock = ACP_FALSE;
    acp_bool_t          sFoundDbc   = ACP_FALSE;

    ulnwTrace(gFuncName, "CPool=%p Dbc=%p", aCPool, aDbc);

    ACI_TEST_RAISE(aCPool == NULL, LABEL_INVALIDHANDLE);
    ACI_TEST_RAISE(aDbc == NULL, LABEL_INVALIDARG);

    // locking
    ulnwDebug(gFuncName, "locking CPool->Lock");
    acpThrMutexLock(aCPool->mLock);
    sNeedUnlock = ACP_TRUE;

    sStat   = &aCPool->mStat;
    sTotal  = sStat->mTotal;
    sCfg    = &aCPool->mCfg;

    ulnwDebug(gFuncName, "total=%d connected=%d in-use=%d", sStat->mTotal, sStat->mConnected, sStat->mInUse);

    ACI_TEST_RAISE(sTotal == 0, LABEL_NOTCONNECTED);

    // find the aDbc in the pool
    for (sInd = 0; sInd < sTotal; sInd++)
    {
        if (aCPool->mDbc[sInd] == aDbc)
        {
            ulnwTrace(gFuncName, "returning connection index=%d (%p)", sInd, aDbc);

            ulnwCPoolChangeConnectionStatusToOpen(aCPool, sInd);
            ulnwCPoolUpdateConnectionStatus(aCPool, sInd);

            sFoundDbc = ACP_TRUE;

            break;
        }
    }
    if (sFoundDbc == ACP_FALSE)
    {
        ulnwTrace(gFuncName, "could not find Dbc=%p", aDbc);
        ACI_RAISE(LABEL_INVALIDARG);
    }

    // if Connected > MinPool, then set idle_timeout
    if (sStat->mConnected > sCfg->mMinPool)
    {
        ulnwCPoolSetConnectionIdleTimeout(aCPool, sInd, sCfg->mIdleTimeout);
    }

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
        ulnwError(gFuncName, ULNW_ERROR_MSG_NOTCONNECTED);
    }
    ACI_EXCEPTION_END;

    if (sNeedUnlock == ACP_TRUE)
    {
        acpThrMutexUnlock(aCPool->mLock);
    }

    ulnwDebug(gFuncName, ULNW_DEBUG_MSG_RETURNERROR);
    return SQL_ERROR;
}

