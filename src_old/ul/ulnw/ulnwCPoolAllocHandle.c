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

static acp_char_t *gFuncName = "SQLCPoolAllocHandle";

static void ulnwCPoolStatInit(ulnwCPoolStat *aStat)
{
    ACI_TEST(aStat == NULL);

    aStat->mTotal       = 0;
    aStat->mConnected   = 0;
    aStat->mInUse       = 0;
    aStat->mIdleTimeoutSet  = 0;
    return;

    ACI_EXCEPTION_END;
    return;
}

static void ulnwCPoolCfgInit(ulnwCPoolCfg *aCfg)
{
    ACI_TEST(aCfg == NULL);

    aCfg->mMinPool                  = ULNW_DEFAULT_MINPOOL;
    aCfg->mMaxPool                  = ULNW_DEFAULT_MAXPOOL;
    aCfg->mIdleTimeout              = ULNW_DEFAULT_IDLETIMEOUT;
    aCfg->mConnectionTimeout        = ULNW_DEFAULT_CONNECTIONTIMEOUT;

    aCfg->mTraceLog                 = ULNW_DEFAULT_TRACELOG;
    aCfg->mValidateQueryResult      = ULNW_DEFAULT_VALIDATEQUERYRESULT;
    aCfg->mSetValidateQueryResult   = ACP_FALSE;

    aCfg->mDataSourceLen            = 0;
    aCfg->mUserIdLen                = 0;
    aCfg->mPasswordLen              = 0;

    acpMemSet(&aCfg->mDataSource,   0, ACI_SIZEOF(aCfg->mDataSource));
    acpMemSet(&aCfg->mUserId,       0, ACI_SIZEOF(aCfg->mUserId));
    acpMemSet(&aCfg->mPassword,     0, ACI_SIZEOF(aCfg->mPassword));

    if (acpSnprintf((acp_char_t *)aCfg->mValidateQueryText,
                    ULNW_MAX_STRING_LENGTH,
                    "%s",
                    ULNW_DEfAULT_VALIDATEQUERYTEXT) == ACP_RC_SUCCESS)
    {
        aCfg->mValidateQueryLen = acpCStrLen((acp_char_t *)aCfg->mValidateQueryText,
                                             ACI_SIZEOF(aCfg->mValidateQueryText));
        aCfg->mSetValidateQueryResult = ACP_TRUE;
    }
    else
    {
        aCfg->mValidateQueryLen = 0;
        acpMemSet(&aCfg->mValidateQueryText, 0, ACI_SIZEOF(aCfg->mValidateQueryText));
        aCfg->mSetValidateQueryResult   = ACP_FALSE;
    }

    return;

    ACI_EXCEPTION_END;
    return;
}

static acp_rc_t ulnwCPoolInit(ulnwCPool *aCPool)
{
    acp_rc_t        sRc         = ACP_RC_SUCCESS;
    acp_char_t      *sFuncName  = "CPoolInit";
    acp_thr_mutex_t *sMutex;

    acp_bool_t  sNeedDestroyMutexMemory = ACP_FALSE;
    acp_bool_t  sNeedUnlock             = ACP_FALSE;

    ulnwDebug(sFuncName, "CPool=%p", aCPool);
    ACI_TEST_RAISE(aCPool == NULL, LABEL_INVALIDARG);

    ulnwDebug(sFuncName, "allocating memory for lock CPool->Lock (size=%ul)", ACI_SIZEOF(acp_thr_mutex_t));
    ACI_TEST_RAISE(acpMemAlloc((void **)&sMutex, ACI_SIZEOF(acp_thr_mutex_t))
                   != ACP_RC_SUCCESS, LABEL_OUTOFMEMORY);
    aCPool->mLock = sMutex;
    sNeedDestroyMutexMemory = ACP_TRUE;
    ulnwDebug(sFuncName, "allocated CPool->Lock=%p", aCPool->mLock);

    ulnwDebug(sFuncName, "initializing lock CPool->Lock");
    sRc = acpThrMutexCreate(aCPool->mLock, ACP_THR_MUTEX_DEFAULT); 
    ACI_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRc), LABEL_CREATEMUTEXFAILED);

    ulnwDebug(sFuncName, "locking CPool->Lock");
    sRc = acpThrMutexLock(aCPool->mLock);
    ACI_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRc), LABEL_LOCKMUTEXFAILED);
    sNeedUnlock = ACP_TRUE;

    ulnwDebug(sFuncName, "initializing CPool(Cfg, Stat, TraceInfo)");
    ulnwCPoolCfgInit(&aCPool->mCfg);
    ulnwCPoolStatInit(&aCPool->mStat);

    ulnwDebug(sFuncName, "allocating CPool->Env with SQLAllocHandle");
    aCPool->mEnv = SQL_NULL_HANDLE;
    ACI_TEST_RAISE(ulnAllocHandle(SQL_HANDLE_ENV, NULL, (void **)&aCPool->mEnv) == SQL_ERROR, LABEL_OUTOFMEMORY);
    ulnwDebug(sFuncName, "allocated CPool->Env=%p", aCPool->mEnv);

    aCPool->mDbc        = NULL;
    aCPool->mDbcStatus  = NULL;
    
    ulnwDebug(sFuncName, "unlocking CPool->Lock");
    sRc = acpThrMutexUnlock(aCPool->mLock);

    ulnwDebug(sFuncName, "initialization completed");
    ulnwDebug(sFuncName, ULNW_DEBUG_MSG_RETURNSUCCESS);

    return sRc;

    ACI_EXCEPTION(LABEL_INVALIDARG)
    {
        ulnwDebug(sFuncName, "invalid SQLHDBCP handle");
        sRc = ACP_RC_EINVAL;
    }
    ACI_EXCEPTION(LABEL_CREATEMUTEXFAILED)
    {
        ulnwDebug(sFuncName, "creating mutex failed");
        // pass result of acpThrMutexCreate as-is
    }
    ACI_EXCEPTION(LABEL_LOCKMUTEXFAILED)
    {
        ulnwDebug(sFuncName, "lock mutex failed");
        // pass result of acpThrMutexLock as-is
    }
    ACI_EXCEPTION(LABEL_OUTOFMEMORY)
    {
        ulnwDebug(sFuncName, ULNW_ERROR_MSG_OUTOFMEMORY);
        sRc = ACP_RC_ENOMEM;
    }
    ACI_EXCEPTION_END;

    if (sNeedUnlock == ACP_TRUE)
    {
        ulnwDebug(sFuncName, "unlocking lock CPool->Lock");
        acpThrMutexUnlock(aCPool->mLock);
    }

    if (sNeedDestroyMutexMemory == ACP_TRUE)
    {
        ulnwDebug(sFuncName, "deallocating memory for lock CPool->Lock");
        acpThrMutexDestroy(aCPool->mLock);
        acpMemFree(aCPool->mLock);
    }

    return sRc;
}

SQLRETURN ulnwCPoolAllocHandle(ulnwCPool **aCPool)
{
    acp_bool_t  sNeedFreeMemory = ACP_FALSE;
    ulnwCPool   *sCPool;

    ulnwTrace(gFuncName, "CPool=%p *CPool=%p", aCPool, *aCPool);

    ACI_TEST_RAISE(aCPool == NULL, LABEL_INVALIDHANDLE);

    ulnwDebug(gFuncName, "allocating SQLHDBCP (size=%d)", sizeof(ulnwCPool));
    ACI_TEST_RAISE(acpMemAlloc((void **)&sCPool, sizeof(ulnwCPool))
                   != ACP_RC_SUCCESS, LABEL_OUTOFMEMORY);
    sNeedFreeMemory = ACP_TRUE;

    ulnwDebug(gFuncName, "SQLHDBCP allocated at %p", sCPool);

    ulnwDebug(gFuncName, "initializing CPool=%p", sCPool);
    ACI_TEST_RAISE(ulnwCPoolInit(sCPool) != ACP_RC_SUCCESS, LABEL_CPOOLINITFAILED);

    *aCPool = sCPool;

    ulnwDebug(gFuncName, ULNW_DEBUG_MSG_RETURNSUCCESS);
    
    return SQL_SUCCESS;

    ACI_EXCEPTION(LABEL_INVALIDHANDLE)
    {
        ulnwError(gFuncName, ULNW_ERROR_MSG_INVALIDHANDLE);
    }
    ACI_EXCEPTION(LABEL_OUTOFMEMORY)
    {
        ulnwError(gFuncName, ULNW_ERROR_MSG_OUTOFMEMORY);
    }
    ACI_EXCEPTION(LABEL_CPOOLINITFAILED)
    {
        ulnwError(gFuncName, "SQLHDBCP initialization failed");
    }
    ACI_EXCEPTION_END;

    if (sNeedFreeMemory == ACP_TRUE)
    {
        acpMemFree(sCPool);
    }

    ulnwDebug(gFuncName, ULNW_DEBUG_MSG_RETURNERROR);
    return SQL_ERROR;
}

