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

void ulnwCPoolUpdateLastTimestamp(ulnwCPool *aCPool)
{
    ACI_TEST(aCPool == NULL);
    ACI_TEST(aCPool->mStat.mTotal == 0);
    ACI_TEST(aCPool->mCfg.mIdleTimeout == 0);

    acpAtomicSet64(&aCPool->mStat.mLastTimestamp, acpTimeNow());

    return;

    ACI_EXCEPTION_END;

    return;
}

void ulnwCPoolCheckLastTimestamp(ulnwCPool *aCPool)
{
    acp_time_t      sNow;
    acp_time_t      sDiff;

    ACI_TEST(aCPool == NULL);
    ACI_TEST(aCPool->mStat.mTotal == 0);
    ACI_TEST(aCPool->mCfg.mIdleTimeout == 0);

    sNow = acpTimeNow();
    sDiff = sNow - aCPool->mStat.mLastTimestamp;
    if (sDiff > (aCPool->mCfg.mIdleTimeout * ACP_SINT64_LITERAL(1000000)))
    {
        ulnwCPoolManageConnections(aCPool);
    }
    acpAtomicSet64(&aCPool->mStat.mLastTimestamp, sNow);
    
    return;

    ACI_EXCEPTION_END;

    return;
}

void ulnwCPoolChangeConnectionStatusToClosed(
            ulnwCPool           *aCPool,
            acp_sint32_t        aConnectIndex)
{
    ulnwCPoolCStatus    *sCStatus;
    acp_char_t          *sFuncName = "CPoolChangeConnectionStatusToClosed";

    ACI_TEST(aCPool == NULL);
    ACI_TEST(aCPool->mDbcStatus == NULL);
    ACI_TEST((aConnectIndex < 0) || (aConnectIndex >= aCPool->mCfg.mMaxPool));

    sCStatus    = &aCPool->mDbcStatus[aConnectIndex];
    ACI_TEST(sCStatus->mStatus == ULNW_CPOOL_CONNSTAT_CLOSED); // SKIP

    if (sCStatus->mStatus == ULNW_CPOOL_CONNSTAT_OPEN)
    {
        ulnwDebug(sFuncName, "%d(%p) open -> closed", aConnectIndex, aCPool->mDbc[aConnectIndex]);
    }
    else // ULNW_CPOOL_CONNSTAT_INUSE
    {
        ulnwDebug(sFuncName, "%d(%p) in-use -> closed", aConnectIndex, aCPool->mDbc[aConnectIndex]);
        acpAtomicDec32(&aCPool->mStat.mInUse);
        acpAtomicSet32(&sCStatus->mStatus, 0);
    }

    acpAtomicDec32(&aCPool->mStat.mConnected);

    acpAtomicSet32(&sCStatus->mStatus, ULNW_CPOOL_CONNSTAT_CLOSED);

    return;

    ACI_EXCEPTION_END;

    return;
}

void ulnwCPoolChangeConnectionStatusToOpen(
            ulnwCPool           *aCPool,
            acp_sint32_t        aConnectIndex)
{
    ulnwCPoolCStatus    *sCStatus;
    acp_char_t          *sFuncName = "CPoolChangeConnectionStatusToOpen";

    ACI_TEST(aCPool == NULL);
    ACI_TEST(aCPool->mDbcStatus == NULL);
    ACI_TEST((aConnectIndex < 0) || (aConnectIndex >= aCPool->mCfg.mMaxPool));

    sCStatus    = &aCPool->mDbcStatus[aConnectIndex];
    ACI_TEST(sCStatus->mStatus == ULNW_CPOOL_CONNSTAT_OPEN); // SKIP

    if (sCStatus->mStatus == ULNW_CPOOL_CONNSTAT_CLOSED)
    {
        ulnwDebug(sFuncName, "%d(%p) closed -> open", aConnectIndex, aCPool->mDbc[aConnectIndex]);
        acpAtomicInc32(&aCPool->mStat.mConnected);
        ulnwCPoolSetConnectionIdleTimeout(aCPool, aConnectIndex, 0);
    }
    else // ULNW_CPOOL_CONNSTAT_INUSE
    {
        ulnwDebug(sFuncName, "%d(%p) in-use -> open", aConnectIndex, aCPool->mDbc[aConnectIndex]);
        acpAtomicDec32(&aCPool->mStat.mInUse);
        acpAtomicSet32(&sCStatus->mLeaseTimestamp, 0);
    }

    acpAtomicSet32(&sCStatus->mStatus, ULNW_CPOOL_CONNSTAT_OPEN);

    return;

    ACI_EXCEPTION_END;

    return;
}

void ulnwCPoolChangeConnectionStatusToInUse(
            ulnwCPool           *aCPool,
            acp_sint32_t        aConnectIndex)
{
    ulnwCPoolCStatus    *sCStatus;
    acp_char_t          *sFuncName = "CPoolChangeConnectionStatusToInUse";

    ACI_TEST(aCPool == NULL);
    ACI_TEST(aCPool->mDbcStatus == NULL);
    ACI_TEST((aConnectIndex < 0) || (aConnectIndex >= aCPool->mCfg.mMaxPool));

    sCStatus    = &aCPool->mDbcStatus[aConnectIndex];
    ACI_TEST(sCStatus->mStatus == ULNW_CPOOL_CONNSTAT_INUSE); // SKIP

    if (sCStatus->mStatus == ULNW_CPOOL_CONNSTAT_CLOSED)
    {
        ulnwDebug(sFuncName, "%d(%p) closed -> in-use", aConnectIndex, aCPool->mDbc[aConnectIndex]);
        acpAtomicInc32(&aCPool->mStat.mConnected);
    }
    else // ULNW_CPOOL_CONNSTAT_OPEN
    {
        ulnwDebug(sFuncName, "%d(%p) open -> in-use", aConnectIndex, aCPool->mDbc[aConnectIndex]);
    }

    acpAtomicInc32(&aCPool->mStat.mInUse);
    acpAtomicSet32(&sCStatus->mLeaseTimestamp, acpTimeNow());

    acpAtomicSet32(&sCStatus->mStatus, ULNW_CPOOL_CONNSTAT_INUSE);

    ulnwCPoolSetConnectionIdleTimeout(aCPool, aConnectIndex, 0);

    return;

    ACI_EXCEPTION_END;

    return;
}

void ulnwCPoolUpdateConnectionStatus(
            ulnwCPool           *aCPool,
            acp_sint32_t        aConnectIndex)
{
    acp_char_t          *sFuncName  = "CPoolUpdateConnectionStatus";
    ulnwCPoolCStatus    *sCStatus;
    ulnDbc              *sDbc;
    SQLUINTEGER         sDead = SQL_FALSE;

    ulnwDebug(sFuncName, "CPool=%p ConnectIndex=%d", aCPool, aConnectIndex);

    ACI_TEST_RAISE(aCPool == NULL, LABEL_INVALIDHANDLE);
    ACI_TEST_RAISE(aCPool->mDbc == NULL, LABEL_INVALIDHANDLE);
    ACI_TEST_RAISE((aConnectIndex < 0) || (aConnectIndex >= aCPool->mCfg.mMaxPool), LABEL_INVALIDARG);
    
    sDbc        = aCPool->mDbc[aConnectIndex];
    sCStatus    = &aCPool->mDbcStatus[aConnectIndex];

    ACI_TEST_RAISE(sCStatus->mStatus == ULNW_CPOOL_CONNSTAT_INUSE, LABEL_SKIP);

    ACI_TEST_RAISE(ulnGetConnectAttr(sDbc, SQL_ATTR_CONNECTION_DEAD, &sDead, 0, NULL) == SQL_ERROR, LABEL_SQLERROR);
    ulnwDebug(sFuncName, "SQLGetConnectAttr(%p, CONNECTION_DEAD) DEAD=%s", sDbc, ((sDead == SQL_TRUE) ? "TRUE" : "FALSE"));

    if (sDead == SQL_TRUE)
    {
        ulnwCPoolDisconnectConnection(aCPool, aConnectIndex);
    }
    else // Dead == SQL_FALSE
    {
        ulnwCPoolChangeConnectionStatusToOpen(aCPool, aConnectIndex);
    }

    ACI_EXCEPTION_CONT(LABEL_SKIP);

    return;

    ACI_EXCEPTION(LABEL_INVALIDHANDLE)
    {
        ulnwDebug(sFuncName, ULNW_ERROR_MSG_INVALIDHANDLE);
    }
    ACI_EXCEPTION(LABEL_INVALIDARG)
    {
        ulnwDebug(sFuncName, "MaxPool=%d ConnectIndex=%d", aCPool->mCfg.mMaxPool, aConnectIndex);
        ulnwDebug(sFuncName, ULNW_ERROR_MSG_INVALIDARG);
    }
    ACI_EXCEPTION(LABEL_SQLERROR)
    {
        ulnwDebug(sFuncName, "SQLGetConnectAttr(SQL_ATTR_CONNECTION_DEAD) failed");
        ulnwTraceSqlError(sFuncName, "SQLGetConnectAttr", SQL_HANDLE_DBC, sDbc);
    }
    ACI_EXCEPTION_END;

    return;
}

SQLRETURN ulnwCPoolSetConnectionIdleTimeout(
            ulnwCPool           *aCPool, 
            acp_sint32_t        aConnectIndex, 
            acp_uint32_t        aIdleTimeout)
{
    acp_char_t          *sFuncName  = "CPoolSetConnectionIdleTimeout";
    ulnwCPoolCStatus    *sCStatus;
    ulnDbc              *sDbc;

    ulnwDebug(sFuncName, "CPool=%p ConnectIndex=%d idle timeout=%d", aCPool, aConnectIndex, aIdleTimeout);

    ACI_TEST_RAISE(aCPool == NULL, LABEL_INVALIDHANDLE);
    ACI_TEST_RAISE(aCPool->mDbc == NULL, LABEL_INVALIDHANDLE);
    ACI_TEST_RAISE((aConnectIndex < 0) || (aConnectIndex >= aCPool->mCfg.mMaxPool), LABEL_INVALIDARG);

    sDbc    = aCPool->mDbc[aConnectIndex];
    sCStatus  = &aCPool->mDbcStatus[aConnectIndex];

    ACI_TEST_RAISE(sCStatus->mIdleTimeout == aIdleTimeout, LABEL_SKIP);

    ulnwDebug(sFuncName, "SQLSetConnectAttr(%p, ALTIBASE_IDLE_TIMEOUT)...", sDbc);
    ACI_TEST_RAISE(ulnSetConnectAttr(sDbc, ALTIBASE_IDLE_TIMEOUT, (SQLPOINTER)(acp_slong_t)aIdleTimeout, 0) != SQL_SUCCESS, LABEL_SQLERROR);
    acpAtomicSet32(&sCStatus->mIdleTimeout, aIdleTimeout);

    if (aIdleTimeout > 0)
    {
        acpAtomicInc32(&aCPool->mStat.mIdleTimeoutSet);
    }
    else // aIdleTimeout == 0
    {
        acpAtomicDec32(&aCPool->mStat.mIdleTimeoutSet);
    }
    ulnwDebug(sFuncName, "new idle timeout=%d", aCPool->mStat.mIdleTimeoutSet);

    ACI_EXCEPTION_CONT(LABEL_SKIP);

    return SQL_SUCCESS;

    ACI_EXCEPTION(LABEL_INVALIDHANDLE)
    {
        if (aCPool == NULL)
        {
            ulnwDebug(sFuncName, "CPool=NULL");
        }
        else
        {
            ulnwDebug(sFuncName, "CPool->Dbc = NULL");
        }
        ulnwDebug(sFuncName, ULNW_ERROR_MSG_INVALIDHANDLE);
    }
    ACI_EXCEPTION(LABEL_INVALIDARG)
    {
        ulnwDebug(sFuncName, "MaxPool=%d ConnectIndex=%d", aCPool->mCfg.mMaxPool, aConnectIndex);
        ulnwDebug(sFuncName, ULNW_ERROR_MSG_INVALIDARG);
    }
    ACI_EXCEPTION(LABEL_SQLERROR)
    {
        ulnwDebug(sFuncName, "SQLSetConnectAttr(DBC=%p, ALTIBASE_IDLE_TIMEOUT, %d, 0) failed", sDbc, aIdleTimeout);
        ulnwTraceSqlError(sFuncName, "SQLGetConnectAttr", SQL_HANDLE_DBC, sDbc);
    }
    ACI_EXCEPTION_END;

    return SQL_ERROR;
}

SQLRETURN ulnwCPoolConnectConnection(
            ulnwCPool           *aCPool,
            acp_sint32_t        aConnectIndex)
{
    acp_char_t          *sFuncName = "CPoolConnectConnection";
    ulnwCPoolCfg        *sCfg;
    ulnDbc              *sDbc;
    acp_char_t          sConnStr[ULNW_MAX_STRING_LENGTH];
    acp_char_t          sUIDPWD[ULNW_MAX_STRING_LENGTH];
    
    ulnwDebug(sFuncName, "CPool=%p ConnectIndex=%d", aCPool, aConnectIndex);

    ACI_TEST_RAISE(aCPool == NULL, LABEL_INVALIDHANDLE);
    ACI_TEST_RAISE(aCPool->mDbc == NULL, LABEL_INVALIDHANDLE);
    ACI_TEST_RAISE((aConnectIndex < 0) || (aConnectIndex >= aCPool->mCfg.mMaxPool), LABEL_INVALIDARG);
    
    sCfg    = &aCPool->mCfg;
    sDbc    = aCPool->mDbc[aConnectIndex];

    ulnwCPoolUpdateConnectionStatus(aCPool, aConnectIndex);
    ACI_TEST_RAISE(aCPool->mDbcStatus[aConnectIndex].mStatus != ULNW_CPOOL_CONNSTAT_CLOSED, LABEL_SKIP);

    // ULNW_CPOOL_CONNSTAT_CLOSED now
    sUIDPWD[0] = '\0';

    if (sCfg->mUserIdLen > 0)
    {
        if (sCfg->mPasswordLen > 0)
        {
            acpSnprintf(sUIDPWD, ULNW_MAX_STRING_LENGTH, "UID=%s;PWD=%s", sCfg->mUserId, sCfg->mPassword);
        }
        else
        {
            acpSnprintf(sUIDPWD, ULNW_MAX_STRING_LENGTH, "UID=%s", sCfg->mUserId);
        }
    }
    ulnwDebug(sFuncName, "UIDPWD=\"%s\"", sUIDPWD);

    sConnStr[0] = '\0';
    if (sUIDPWD[0] != '\0')
    {
        ACI_TEST_RAISE(acpSnprintf(sConnStr,
                                   ULNW_MAX_STRING_LENGTH,
                                   "DSN=%s;%s",
                                   sCfg->mDataSource,
                                   sUIDPWD) != ACP_RC_SUCCESS, 
                       LABEL_GENCONNECTSTRINGFAILED);
    }
    else
    {
        ACI_TEST_RAISE(acpSnprintf(sConnStr,
                                   ULNW_MAX_STRING_LENGTH,
                                   "DSN=%s",
                                   sCfg->mDataSource) != ACP_RC_SUCCESS, 
                       LABEL_GENCONNECTSTRINGFAILED);
    }

    ulnwDebug(sFuncName, "connection string=\"%s\"", sConnStr);

    ulnwDebug(sFuncName, "SQLDriverConnect(%p, connstr, %d,...)...", sDbc, acpCStrLen(sConnStr, ULNW_MAX_STRING_LENGTH));
    ACI_TEST_RAISE(ulnDriverConnect(sDbc, sConnStr, acpCStrLen(sConnStr, ULNW_MAX_STRING_LENGTH), NULL, 0, NULL) != SQL_SUCCESS,
                    LABEL_SQLERROR);

    ulnwCPoolChangeConnectionStatusToOpen(aCPool, aConnectIndex);

    ACI_EXCEPTION_CONT(LABEL_SKIP);

    return SQL_SUCCESS;

    ACI_EXCEPTION(LABEL_INVALIDHANDLE)
    {
        if (aCPool == NULL)
        {
            ulnwDebug(sFuncName, "CPool=NULL");
        }
        else
        {
            ulnwDebug(sFuncName, "CPool->Dbc = NULL");
        }
        ulnwDebug(sFuncName, ULNW_ERROR_MSG_INVALIDHANDLE);
    }
    ACI_EXCEPTION(LABEL_INVALIDARG)
    {
        ulnwDebug(sFuncName, "MaxPool=%d ConnectIndex=%d", aCPool->mCfg.mMaxPool, aConnectIndex);
        ulnwDebug(sFuncName, ULNW_ERROR_MSG_INVALIDARG);
    }
    ACI_EXCEPTION(LABEL_GENCONNECTSTRINGFAILED)
    {
        ulnwDebug(sFuncName, "failed to generate connection string");
    }
    ACI_EXCEPTION(LABEL_SQLERROR)
    {
        ulnwDebug(sFuncName, "SQLDriverConnect failed");
        ulnwTraceSqlError(sFuncName, "SQLDriverConnect", SQL_HANDLE_DBC, sDbc);
    }
    ACI_EXCEPTION_END;

    return SQL_ERROR;
}

SQLRETURN ulnwCPoolDisconnectConnection(
            ulnwCPool           *aCPool,
            acp_sint32_t        aConnectIndex)
{
    acp_char_t          *sFuncName = "CPoolDisconnectConnection";
    ulnDbc              *sDbc;

    ulnwDebug(sFuncName, "CPool=%p ConnectIndex=%d", aCPool, aConnectIndex);

    ACI_TEST_RAISE(aCPool == NULL, LABEL_INVALIDHANDLE);
    ACI_TEST_RAISE(aCPool->mDbc == NULL, LABEL_INVALIDHANDLE);
    ACI_TEST_RAISE((aConnectIndex < 0) && (aConnectIndex >= aCPool->mCfg.mMaxPool), LABEL_INVALIDARG);

    sDbc    = aCPool->mDbc[aConnectIndex];

    ulnwDebug(sFuncName, "SQLDisconnect(%p)...", sDbc);
    ACI_TEST_RAISE(ulnDisconnect(sDbc) != SQL_SUCCESS, LABEL_SQLERROR);

    ulnwCPoolChangeConnectionStatusToClosed(aCPool, aConnectIndex);

    return SQL_SUCCESS;

    ACI_EXCEPTION(LABEL_INVALIDHANDLE)
    {
        ulnwDebug(sFuncName, ULNW_ERROR_MSG_INVALIDHANDLE);
    }
    ACI_EXCEPTION(LABEL_INVALIDARG)
    {
        ulnwDebug(sFuncName, ULNW_ERROR_MSG_INVALIDARG);
    }
    ACI_EXCEPTION(LABEL_SQLERROR)
    {
        ulnwDebug(sFuncName, "SQLDisconnect failed");
        ulnwTraceSqlError(sFuncName, "SQLDisconnect", SQL_HANDLE_DBC, sDbc);
    }
    ACI_EXCEPTION_END;

    return SQL_ERROR;
}


SQLRETURN ulnwCPoolValidateConnection(ulnwCPool *aCPool, acp_sint32_t aConnectIndex)
{
    acp_char_t      *sFuncName  = "CPoolValidateConnection";
    acp_char_t      sColName[ULNW_MAX_NAME_LENGTH];
    ulnwCPoolCfg    *sCfg = NULL;
    ulnDbc          *sDbc = NULL;
    ulnStmt         *sStmt = NULL;
    acp_sint16_t    sColCount   = 0;
    acp_sint16_t    sColNameLen = 0;
    acp_sint16_t    sColType = 0;
    acp_sint64_t    sColSize    = 0;

    acp_sint32_t    sValidateQueryResult;

    acp_bool_t      sNeedFreeStmtHandle = ACP_FALSE;

    ulnwDebug(sFuncName, "CPool=%p ConnectIndex=%d", aCPool, aConnectIndex);

    ACI_TEST_RAISE(aCPool == NULL, LABEL_INVALIDHANDLE);
    ACI_TEST_RAISE(aCPool->mDbc == NULL, LABEL_INVALIDHANDLE);
    sCfg = &aCPool->mCfg;
    ACI_TEST_RAISE((aConnectIndex < 0) && (aConnectIndex >= sCfg->mMaxPool), LABEL_INVALIDARG);
    ACI_TEST_RAISE(sCfg->mValidateQueryLen == 0, LABEL_SKIP);
    ACI_TEST_RAISE(sCfg->mSetValidateQueryResult == ACP_FALSE, LABEL_SKIP);

    sDbc    = aCPool->mDbc[aConnectIndex];
    ulnwDebug(sFuncName, "Dbc=%p", sDbc);

    ulnwCPoolUpdateConnectionStatus(aCPool, aConnectIndex);
    ACI_TEST_RAISE(aCPool->mDbcStatus[aConnectIndex].mStatus == ULNW_CPOOL_CONNSTAT_CLOSED, LABEL_NOTCONNECTED);
    ulnwDebug(sFuncName, "connection is not closed");

    ulnwDebug(sFuncName, "SQLAllocHandle(SQL_HANDLE_STMT)");
    ACI_TEST_RAISE(ulnAllocHandle(SQL_HANDLE_STMT, (ulnDbc *)sDbc, (void **)&sStmt) != SQL_SUCCESS, LABEL_SQLERROR);
    sNeedFreeStmtHandle = ACP_TRUE;
    ulnwDebug(sFuncName, "Stmt=%p", sStmt);

    ulnwDebug(sFuncName, "SQLPrepare(%p, \"%s\" (%d))...", sStmt, sCfg->mValidateQueryText, sCfg->mValidateQueryLen);
    ACI_TEST_RAISE(ulnPrepare(sStmt, sCfg->mValidateQueryText, sCfg->mValidateQueryLen, NULL) != SQL_SUCCESS, LABEL_SQLERROR);

    ulnwDebug(sFuncName, "SQLExecute(%p)...", sStmt);
    ACI_TEST_RAISE(ulnExecute(sStmt) != SQL_SUCCESS, LABEL_SQLERROR);

    ACI_TEST_RAISE(ulnNumResultCols(sStmt, &sColCount) != SQL_SUCCESS, LABEL_SQLERROR);
    ulnwDebug(sFuncName, "SQLNumResultCols(%p) returns %d", sStmt, sColCount);

    ACI_TEST_RAISE(ulnDescribeCol(sStmt, 1, sColName, ACI_SIZEOF(sColName), &sColNameLen, &sColType, (ulvULen *)&sColSize, NULL, NULL) != SQL_SUCCESS, LABEL_SQLERROR);
    ulnwDebug(sFuncName, "SQLDescribeCol(Ord:1 Name:\"%s\"(%d) Type:%d Size:%d,...)", sColName, sColNameLen, sColType, sColSize);

    ulnwDebug(sFuncName, "SQLBindCol(Ord:1 Type:SQL_C_SLONG,...)");
    ACI_TEST_RAISE(ulnBindCol(sStmt, 1, SQL_C_SLONG, &sValidateQueryResult, 0, (ulvSLen *)&sColSize) != SQL_SUCCESS, LABEL_SQLERROR);

    ulnwDebug(sFuncName, "SQLFetch(%p)", sStmt);
    ACI_TEST_RAISE(ulnFetch(sStmt) != SQL_SUCCESS, LABEL_VALIDATIONFAILED);
    ACI_TEST_RAISE(sValidateQueryResult != sCfg->mValidateQueryResult, LABEL_VALIDATIONFAILED);

    ulnwDebug(sFuncName, "validation successful");

    ulnwDebug(sFuncName, "SQLFreeHandle(%p)", sStmt);

    ulnFreeHandle(SQL_HANDLE_STMT, sStmt);
    sNeedFreeStmtHandle = ACP_FALSE;

    ACI_EXCEPTION_CONT(LABEL_SKIP);

    return SQL_SUCCESS;

    ACI_EXCEPTION(LABEL_INVALIDHANDLE)
    {
        ulnwDebug(sFuncName, ULNW_ERROR_MSG_INVALIDHANDLE);
    }
    ACI_EXCEPTION(LABEL_INVALIDARG)
    {
        ulnwDebug(sFuncName, ULNW_ERROR_MSG_INVALIDARG);
    }
    ACI_EXCEPTION(LABEL_NOTCONNECTED)
    {
        ulnwDebug(sFuncName, "not connected connection");
    }
    ACI_EXCEPTION(LABEL_SQLERROR)
    {
        if (sNeedFreeStmtHandle == ACP_FALSE)
        {
            ulnwTraceSqlError(sFuncName, "SQL...", SQL_HANDLE_DBC, sDbc);
        }
        else
        {
            ulnwTraceSqlError(sFuncName, "SQL...", SQL_HANDLE_STMT, sStmt);
        }
    }
    ACI_EXCEPTION(LABEL_VALIDATIONFAILED)
    {
        ulnwDebug(sFuncName, "validation failed");
    }
    ACI_EXCEPTION_END;

    if (sNeedFreeStmtHandle == ACP_TRUE)
    {
        ulnwDebug(sFuncName, "SQLFreeHandle(STMT)");
        ulnFreeHandle(SQL_HANDLE_STMT, sStmt);
    }

    return SQL_ERROR;
}

void ulnwCPoolManageConnections(ulnwCPool *aCPool)
{
    acp_char_t          *sFuncName = "CPoolManageConnections";
    ulnwCPoolStat       *sStat;
    ulnwCPoolCfg        *sCfg;
    acp_sint32_t        sInd;
    acp_sint32_t        sTotal;
    ulnwCPoolCStatus    *sCStatus;

    ACI_TEST(aCPool == NULL);
    ACI_TEST(aCPool->mStat.mTotal == 0);

    sStat   = &aCPool->mStat;
    sCfg    = &aCPool->mCfg;
    sTotal = sStat->mTotal;

    ulnwCPoolUpdateLastTimestamp(aCPool);

    ulnwDebug(sFuncName, "CPool=%p connected=%d in-use=%d min_pool=%d", 
            aCPool, sStat->mConnected, sStat->mInUse, sCfg->mMinPool);

    ACI_TEST((sStat->mIdleTimeoutSet == 0) && (sStat->mConnected == sCfg->mMinPool));
    if (sStat->mIdleTimeoutSet > 0)
    {
        for (sInd = 0; sInd < sTotal; sInd++)
        {
            sCStatus = &aCPool->mDbcStatus[sInd];
            if ((sCStatus->mStatus == ULNW_CPOOL_CONNSTAT_OPEN) &&
                (sCStatus->mIdleTimeout > 0))
            {
                ulnwCPoolUpdateConnectionStatus(aCPool, sInd);
            }
        }
    }
    ACI_TEST((sStat->mIdleTimeoutSet == 0) && (sStat->mConnected == sCfg->mMinPool));

    ulnwDebug(sFuncName, "BEGIN ==> total=%d connected=%d in-use=%d idletimeoutset=%d",
            sStat->mTotal, sStat->mConnected, sStat->mInUse, sStat->mIdleTimeoutSet);

    if (sStat->mConnected > sCfg->mMinPool)
    {
        // set idle timeout
        for (sInd = 0; (sInd < sTotal) && ((sStat->mConnected - sStat->mInUse) > sStat->mIdleTimeoutSet); sInd++)
        {
            sCStatus = &aCPool->mDbcStatus[sInd];
            if ((sCStatus->mStatus == ULNW_CPOOL_CONNSTAT_OPEN) && (sCStatus->mIdleTimeout == 0))
            {
                ulnwCPoolSetConnectionIdleTimeout(aCPool, sInd, sCfg->mIdleTimeout);
            }
        }
    }
    else // sStat->mConnected <= sStat->mMinPool
    {
        for (sInd = 0; (sInd < sTotal) && (sStat->mConnected < sCfg->mMinPool); sInd++) // make connection
        {
            sCStatus = &aCPool->mDbcStatus[sInd];
            if (sCStatus->mStatus == ULNW_CPOOL_CONNSTAT_CLOSED)
            {
                ulnwDebug(sFuncName, "connecting %d th connection", sInd);

                if (ulnwCPoolConnectConnection(aCPool, sInd) == SQL_SUCCESS)
                {
                    ulnwCPoolChangeConnectionStatusToOpen(aCPool, sInd);
                }
            }
        }
    }

    ulnwDebug(sFuncName, "END ==> total=%d connected=%d in-use=%d idletimeoutset=%d",
            sStat->mTotal, sStat->mConnected, sStat->mInUse, sStat->mIdleTimeoutSet);

    return;

    ACI_EXCEPTION_END;

    return;
}

