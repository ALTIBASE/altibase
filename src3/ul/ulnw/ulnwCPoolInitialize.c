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

static acp_char_t *gFuncName = "SQLCPoolInitialize";

SQLRETURN ulnwCPoolInitialize(ulnwCPool *aCPool)
{
    acp_sint32_t        sInd    = 0;
    acp_sint32_t        sTotal  = 0;
    ulnwCPoolCfg        *sCfg;
    ulnwCPoolCStatus    *sCStatus;
    ulnwCPoolStat       *sStat;


    acp_bool_t          sNeedUnlock         = ACP_FALSE;
    acp_bool_t          sNeedFreeDbcStatus  = ACP_FALSE;
    acp_bool_t          sNeedFreeDbcArray   = ACP_FALSE;
    acp_bool_t          sNeedFreeDbcHandles = ACP_FALSE;

    ulnwTrace(gFuncName, "CPool=%p", aCPool);

    // checking arguments
    ACI_TEST_RAISE(aCPool == NULL, LABEL_INVALIDHANDLE);

    // status check
    ACI_TEST_RAISE(aCPool->mStat.mTotal > 0, LABEL_ALREADYCONNECTED);

    ACI_TEST_RAISE((aCPool->mCfg.mMinPool > aCPool->mCfg.mMaxPool) || (aCPool->mCfg.mMaxPool == 0),
                LABEL_INVALIDCONFIG);

    // locking
    ulnwDebug(gFuncName, "locking CPool->Lock");
    ACI_TEST_RAISE(acpThrMutexLock(aCPool->mLock) != ACP_RC_SUCCESS, LABEL_LOCKINGFAILED);
    sNeedUnlock = ACP_TRUE;

    // get Total number of connection
    sCfg = &aCPool->mCfg;
    sTotal = sCfg->mMaxPool;

    // allocating DBC status
    ulnwDebug(gFuncName, "allocating memory for DbcStatus");
    ACI_TEST_RAISE(acpMemAlloc((void **)&aCPool->mDbcStatus,
                               ACI_SIZEOF(ulnwCPoolCStatus) * sTotal)
                   != ACP_RC_SUCCESS, LABEL_OUTOFMEMORY);
    sNeedFreeDbcStatus = ACP_TRUE;
    
    // initializing DBC status
    for (sInd = 0; sInd < sTotal; sInd++)
    {
        sCStatus                    = &aCPool->mDbcStatus[sInd];
        sCStatus->mStatus           = ULNW_CPOOL_CONNSTAT_CLOSED;
        sCStatus->mIdleTimeout      = 0;
        sCStatus->mLeaseTimestamp   = 0;
    }

    // allocating memory for DBC handles
    ulnwDebug(gFuncName, "allocating memory for Dbc (MaxPool=%d)", sTotal);
    ACI_TEST_RAISE(acpMemAlloc((void **)&aCPool->mDbc,
                               ACI_SIZEOF(ulnDbc *) * sTotal)
                   != ACP_RC_SUCCESS, LABEL_OUTOFMEMORY);
    sNeedFreeDbcArray = ACP_TRUE;

    // allocating DBC handles
    ulnwDebug(gFuncName, "allocating DBC handles");
    for (sInd = 0; sInd < sTotal; sInd++)
    {
        aCPool->mDbc[sInd] = SQL_NULL_HANDLE;
    }
    for (sInd = 0; sInd < sTotal; sInd++)
    {
        ulnwDebug(gFuncName, "%02d allocating", sInd);
        if (ulnAllocHandle(SQL_HANDLE_DBC, (void *)aCPool->mEnv, (void **)&aCPool->mDbc[sInd]) == SQL_ERROR)
        {
            ulnwDebug(gFuncName, "%d SQLAllocHandle failed", sInd);
            ulnwTraceSqlError(gFuncName, "SQLAllocHandle", SQL_HANDLE_ENV, aCPool->mEnv);
            ACI_RAISE(LABEL_OUTOFMEMORY);
        }
    }
    sNeedFreeDbcHandles = ACP_TRUE;

    // setting properties for DBC handles
    ulnwDebug(gFuncName, "setting properties for DBC handles (idle_timeout=%d, connection_timeout=%d tracelog=%d)",
            sCfg->mIdleTimeout, sCfg->mConnectionTimeout, sCfg->mTraceLog);
    for (sInd = 0; sInd < sTotal; sInd++)
    {
        // IDLE_TIMEOUT 0
        if (ulnSetConnectAttr(aCPool->mDbc[sInd], ALTIBASE_IDLE_TIMEOUT, (SQLPOINTER)0, 0) != SQL_SUCCESS)
        {
            ulnwDebug(gFuncName, "%02d SQLSetConnectAttr(%p, ALTIBASE_IDLE_TIMEOUT, 0) failed", 
                    sInd, aCPool->mDbc[sInd]);
            ulnwTraceSqlError(gFuncName, "SQLSetConnectAttr", SQL_HANDLE_DBC, aCPool->mDbc[sInd]);
        }

        // TRACELOG sCfg->mTraceLog
        if (ulnSetConnectAttr(aCPool->mDbc[sInd], ALTIBASE_TRACELOG, (SQLPOINTER)
                    (acp_slong_t)((sCfg->mTraceLog == ALTIBASE_TRACELOG_DEBUG) ? ALTIBASE_TRACELOG_FULL : sCfg->mTraceLog), 0) 
                != SQL_SUCCESS)
        {
            ulnwDebug(gFuncName, "%02d SQLSetConnectAttr(%p, ALTIBASE_TRACELOG, %d) failed", 
                    sInd, aCPool->mDbc[sInd], sCfg->mTraceLog);
            ulnwTraceSqlError(gFuncName, "SQLSetConnectAttr", SQL_HANDLE_DBC, aCPool->mDbc[sInd]);
        }
        ulnwDebug(gFuncName, "%02d setting idle_timeout, tracelog finished", sInd);
    }

    // initializing statistics
    ulnwDebug(gFuncName, "initializing statistics");

    sStat = &aCPool->mStat;
    sStat->mTotal            = sTotal;
    sStat->mConnected        = 0;
    sStat->mInUse            = 0;
    sStat->mIdleTimeoutSet   = 0;

    ulnwCPoolUpdateLastTimestamp(aCPool);

    // making connections 
    ulnwDebug(gFuncName, "making %d connections", sCfg->mMinPool);
    for (sInd = 0; (sInd < sTotal) && (sStat->mConnected < sCfg->mMinPool); sInd++)
    {
        ulnwDebug(gFuncName, "%02d connecting", sInd);
        if (ulnwCPoolConnectConnection(aCPool, sInd) == SQL_SUCCESS)
        {
            ACI_TEST_RAISE(ulnwCPoolValidateConnection(aCPool, sInd) != SQL_SUCCESS, LABEL_VALIDATIONFAILED);
            ulnwCPoolChangeConnectionStatusToOpen(aCPool, sInd);
        }
        else
        {
            ACI_RAISE(LABEL_CONNECTIONFAILED);
        }
        ulnwDebug(gFuncName, "%02d total:%d connected:%d in-use:%d idletimeout:%d",
                sInd, sStat->mTotal, sStat->mConnected, sStat->mInUse, sStat->mIdleTimeoutSet);
    }

    ulnwDebug(gFuncName, "statistics: total=%d connected=%d in-use=%d idle_timeout_set=%d min_pool=%d max_pool=%d",
            sStat->mTotal, sStat->mConnected, sStat->mInUse, sStat->mIdleTimeoutSet, 
            sCfg->mMinPool, sCfg->mMaxPool);


    ulnwDebug(gFuncName, "unlocking CPool->Lock");
    ACI_TEST_RAISE(acpThrMutexUnlock(aCPool->mLock) != ACP_RC_SUCCESS, LABEL_UNLOCKINGFAILED);
    sNeedUnlock = ACP_FALSE;

    ulnwDebug(gFuncName, ULNW_DEBUG_MSG_RETURNSUCCESS);

    return SQL_SUCCESS;

    ACI_EXCEPTION(LABEL_INVALIDHANDLE)
    {
        ulnwError(gFuncName, ULNW_ERROR_MSG_INVALIDHANDLE);
    }
    ACI_EXCEPTION(LABEL_INVALIDCONFIG)
    {
        ulnwError(gFuncName, "the configuration for connection pool has conflicts (MIN_POOL=%d MAX_POOL=%d)", 
                aCPool->mCfg.mMinPool, aCPool->mCfg.mMaxPool);
        ulnwError(gFuncName, ULNW_ERROR_MSG_IMPOSSIBLE);
    }
    ACI_EXCEPTION(LABEL_ALREADYCONNECTED)
    {
        ulnwError(gFuncName, ULNW_ERROR_MSG_ALREADYCONNECTED);
    }
    ACI_EXCEPTION(LABEL_OUTOFMEMORY)
    {
        ulnwError(gFuncName, ULNW_ERROR_MSG_OUTOFMEMORY);
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
    ACI_EXCEPTION(LABEL_CONNECTIONFAILED)
    {
        ulnwError(gFuncName, "opening the minimun number of connections (%d) for connection pool failed (failed at %d). "
                "please check ALTIBASE properties file at $ALTIBASE_HOME/conf/altibase.properties.", 
                aCPool->mCfg.mMinPool, sInd);
        ulnwError(gFuncName, ULNW_ERROR_MSG_IMPOSSIBLE);
    }
    ACI_EXCEPTION(LABEL_VALIDATIONFAILED)
    {
        ulnwError(gFuncName, "validation test failed");
    }
    ACI_EXCEPTION_END;

    if (sNeedFreeDbcStatus == ACP_TRUE)
    {
        acpMemFree(aCPool->mDbcStatus);
    }
    
    if (sNeedFreeDbcHandles == ACP_TRUE)
    {
        for (sInd = 0; sInd < sTotal; sInd++)
        {
            if (aCPool->mDbc[sInd] != SQL_NULL_HANDLE)
            {
                ulnFreeHandle(SQL_HANDLE_DBC, aCPool->mDbc[sInd]);
            }
        }
    }
    
    if (sNeedFreeDbcArray == ACP_TRUE)
    {
        acpMemFree(aCPool->mDbc);
    }

    if (sNeedUnlock == ACP_TRUE)
    {
        acpThrMutexUnlock(aCPool->mLock);
    }

    ulnwDebug(gFuncName, ULNW_DEBUG_MSG_RETURNERROR);
    return SQL_ERROR;
}

