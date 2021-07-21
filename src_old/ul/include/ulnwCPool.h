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

#include <acp.h>
#include <uln.h>

#ifndef _ULNW_CPOOL_H_
#define _ULNW_CPOOL_H_

#define ULNW_DEBUG_MSG_RETURNSUCCESS        "return SUCCESS"
#define ULNW_DEBUG_MSG_RETURNERROR          "return ERROR"

#define ULNW_ERROR_MSG_SUCCESS              "success"
#define ULNW_ERROR_MSG_INVALIDHANDLE        "invalid SQLHDBCP handle"
#define ULNW_ERROR_MSG_INVALIDARG           "invalid argument(s)"
#define ULNW_ERROR_MSG_IMPOSSIBLE           "operation was not possible"
#define ULNW_ERROR_MSG_INVALIDATTRIBUTE     "invalid attribute argument"
#define ULNW_ERROR_MSG_OUTOFMEMORY          "out of memory"
#define ULNW_ERROR_MSG_ALREADYCONNECTED     "connection pool is already connected"
#define ULNW_ERROR_MSG_NOTCONNECTED         "connection pool is not connected yet"
#define ULNW_ERROR_MSG_NOAVAILABLECONNECTION "all connections in connection pool are in-use"

// MAX VALUES
#define ULNW_MAX_STRING_LENGTH              (1024)
#define ULNW_MAX_NAME_LENGTH                (64)

// DEFAULT VALUES
#define ULNW_DEFAULT_MINPOOL                (0)
#define ULNW_DEFAULT_MAXPOOL                (0)
#define ULNW_DEFAULT_IDLETIMEOUT            (0)
#define ULNW_DEFAULT_CONNECTIONTIMEOUT      (5)

#define ULNW_DEFAULT_DSN                    "127.0.0.1;PORT_NO=20300"
#define ULNW_DEfAULT_UID                    "sys"
#define ULNW_DEfAULT_PWD                    "manager"

#define ULNW_DEfAULT_VALIDATEQUERYTEXT      "SELECT 1+1 FROM DUAL"
#define ULNW_DEFAULT_VALIDATEQUERYRESULT    (2)

#define ULNW_DEFAULT_TRACELOG               (ALTIBASE_TRACELOG_NONE)

enum
{
    ALTIBASE_TRACELOG_DEBUG = 5
};

// ATTRIBUTES
// check sqlcli.h

// TYPES
typedef struct
{
    acp_sint32_t    mMinPool;
    acp_sint32_t    mMaxPool;
    acp_sint32_t    mIdleTimeout;
    acp_sint32_t    mConnectionTimeout;
    acp_sint32_t    mTraceLog;
    acp_sint32_t    mValidateQueryResult;

    acp_sint16_t    mDataSourceLen;
    acp_sint16_t    mUserIdLen;
    acp_sint16_t    mPasswordLen;
    acp_sint16_t    mValidateQueryLen;

    acp_char_t      mUserId[ULNW_MAX_NAME_LENGTH];
    acp_char_t      mPassword[ULNW_MAX_NAME_LENGTH];
    acp_char_t      mValidateQueryText[ULNW_MAX_STRING_LENGTH];
    acp_char_t      mDataSource[ULNW_MAX_STRING_LENGTH];

    acp_bool_t      mSetValidateQueryResult;
} ulnwCPoolCfg;

typedef struct
{
    acp_sint32_t    mTotal;     // MAXPOOL
    acp_sint32_t    mConnected; // CONNSTAT_OPEN + CONNSTAT_INUSE
    acp_sint32_t    mInUse;     // CONNSTAT_INUSE
    acp_sint32_t    mIdleTimeoutSet;
    acp_time_t      mLastTimestamp;
} ulnwCPoolStat;

enum
{
    ULNW_CPOOL_CONNSTAT_CLOSED = 0,
    ULNW_CPOOL_CONNSTAT_OPEN,
    ULNW_CPOOL_CONNSTAT_INUSE
};

typedef struct
{
    acp_uint32_t    mStatus;
    acp_uint32_t    mIdleTimeout;
    acp_time_t      mLeaseTimestamp;
} ulnwCPoolCStatus;

typedef struct
{
    acp_thr_mutex_t  *mLock;
    ulnwCPoolCfg     mCfg;
    ulnwCPoolStat    mStat;

    ulnEnv           *mEnv;
    ulnDbc           **mDbc;

    ulnwCPoolCStatus *mDbcStatus;
} ulnwCPool;

// EXPORTING FUNCTION DECLARATIONS

SQLRETURN ulnwCPoolAllocHandle(ulnwCPool **aCPool);

SQLRETURN ulnwCPoolFreeHandle(ulnwCPool  *aCPool);

SQLRETURN ulnwCPoolGetAttr(
                    ulnwCPool       *aCPool, 
                    SQLINTEGER      aAttribute,
                    SQLPOINTER      aValue,
                    SQLINTEGER      aBufferLength,
                    SQLINTEGER      *aStringLength);

SQLRETURN ulnwCPoolSetAttr(
                    ulnwCPool       *aCPool, 
                    SQLINTEGER      aAttribute, 
                    SQLPOINTER      aValue, 
                    SQLINTEGER      aStringLength);

SQLRETURN ulnwCPoolInitialize(ulnwCPool *aCPool);

SQLRETURN ulnwCPoolConnectConnection(
                    ulnwCPool       *aCPool,
                    acp_sint32_t    aConnectIndex);

SQLRETURN ulnwCPoolDisconnectConnection(
                    ulnwCPool       *aCPool,
                    acp_sint32_t    aConnectIndex);

void ulnwCPoolUpdateConnectionStatus(
                    ulnwCPool       *aCPool,
                    acp_sint32_t    aConnectIndex);

void ulnwCPoolChangeConnectionStatusToClosed(
                    ulnwCPool       *aCPool,
                    acp_sint32_t    aConnectIndex);

void ulnwCPoolChangeConnectionStatusToInUse(
                    ulnwCPool       *aCPool,
                    acp_sint32_t    aConnectIndex);

void ulnwCPoolChangeConnectionStatusToOpen(
                    ulnwCPool       *aCPool,
                    acp_sint32_t    aConnectIndex);

SQLRETURN ulnwCPoolValidateConnection(
                    ulnwCPool       *aCPool,
                    acp_sint32_t    aConnectIndex);

SQLRETURN ulnwCPoolSetConnectionIdleTimeout(
                    ulnwCPool       *aCPool,
                    acp_sint32_t    aConnectIndex,
                    acp_uint32_t    aIdleTimeout);

SQLRETURN ulnwCPoolFinalize(ulnwCPool *aCPool);

SQLRETURN ulnwCPoolGetConnection(
                    ulnwCPool       *aCPool,
                    ulnDbc          **aUlnDbc);

SQLRETURN ulnwCPoolReturnConnection(
                    ulnwCPool       *aCPool,
                    ulnDbc          *aUlnDbc);

void ulnwCPoolCheckLastTimestamp(ulnwCPool *aCPool);
void ulnwCPoolUpdateLastTimestamp(ulnwCPool *aCPool);

void ulnwCPoolManageConnections(ulnwCPool *aCPool);

void ulnwTrace(const acp_char_t *aFuncName, const acp_char_t *aFormat, ...);

void ulnwDebug(const acp_char_t *aFuncName, const acp_char_t *aFormat, ...);

void ulnwError(const acp_char_t *aFuncName, const acp_char_t *aFormat, ...);

void ulnwTraceSqlError(const acp_char_t *aFuncName, const acp_char_t *aSqlFuncName, SQLSMALLINT aHandleType, SQLHANDLE aSqlHandle);

void ulnwTraceSet(acp_uint32_t aCurrentLogLevel);

#endif // _ULNW_CPOOL_H_
