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

static acp_char_t *gFuncName = "SQLCPoolGetAttr";

// read/write
static SQLRETURN ulnwGetMinPool(ulnwCPool *aCPool, SQLPOINTER aPtrMinPool, SQLINTEGER aZeroLen, SQLINTEGER *aNullLen);
static SQLRETURN ulnwGetMaxPool(ulnwCPool *aCPool, SQLPOINTER aPtrMaxPool, SQLINTEGER aZeroLen, SQLINTEGER *aNullLen);
static SQLRETURN ulnwGetIdleTimeout(ulnwCPool *aCPool, SQLPOINTER aPtrIdleTimeout, SQLINTEGER aZeroLen, SQLINTEGER *aNullLen);
static SQLRETURN ulnwGetConnectionTimeout(ulnwCPool *aCPool, SQLPOINTER aPtrConnectionTimeout, SQLINTEGER aZeroLen, SQLINTEGER *aNullLen);

static SQLRETURN ulnwGetDsn(ulnwCPool *aCPool, SQLPOINTER aPtrDsnStr, SQLINTEGER aDsnStrSize, SQLINTEGER *aDsnStrLen);
static SQLRETURN ulnwGetUid(ulnwCPool *aCPool, SQLPOINTER aPtrUidStr, SQLINTEGER aUidStrSize, SQLINTEGER *aUidStrLen);
static SQLRETURN ulnwGetPwd(ulnwCPool *aCPool, SQLPOINTER aPtrPwdStr, SQLINTEGER aPwdStrSize, SQLINTEGER *aPwdStrLen);

static SQLRETURN ulnwGetTraceLog(ulnwCPool *aCPool, SQLPOINTER aPtrLogLevel, SQLINTEGER aZeroLen  , SQLINTEGER *aNullLen);
static SQLRETURN ulnwGetValidateQueryText(ulnwCPool *aCPool, SQLPOINTER aPtrValidateQueryText, SQLINTEGER aValidateQuerySize, SQLINTEGER *aValidateQueryLen);
static SQLRETURN ulnwGetValidateQueryResult(ulnwCPool *aCPool, SQLPOINTER aPtrValidateQueryResult, SQLINTEGER aZeroLen, SQLINTEGER *aNullLen);

// readonly
static SQLRETURN ulnwGetTotalConnection(ulnwCPool *aCPool, SQLPOINTER aPtrTotalConn, SQLINTEGER aZeroLen, SQLINTEGER *aNullLen);
static SQLRETURN ulnwGetConnectedConnection(ulnwCPool *aCPool, SQLPOINTER aPtrConnectedConn, SQLINTEGER aZeroLen, SQLINTEGER *aNullLen);
static SQLRETURN ulnwGetInUseConnection(ulnwCPool *aCPool, SQLPOINTER aPtrInUseConn, SQLINTEGER aZeroLen, SQLINTEGER *aNullLen);

// attribute-handler table structure
typedef struct ulnwGetAttrCallback
{
    SQLINTEGER      mAttrId;
    SQLRETURN       (*mGetAttrCallback)(ulnwCPool *, SQLPOINTER, SQLINTEGER, SQLINTEGER *);
} ulnwGetAttrCallback;

// attribute-handler table
static struct ulnwGetAttrCallback gGetAttrCallback[] =
{
    {ALTIBASE_CPOOL_ATTR_MIN_POOL,                      ulnwGetMinPool},
    {ALTIBASE_CPOOL_ATTR_MAX_POOL,                      ulnwGetMaxPool},
    {ALTIBASE_CPOOL_ATTR_IDLE_TIMEOUT,                  ulnwGetIdleTimeout},
    {ALTIBASE_CPOOL_ATTR_CONNECTION_TIMEOUT,            ulnwGetConnectionTimeout},
    {ALTIBASE_CPOOL_ATTR_DSN,                           ulnwGetDsn},
    {ALTIBASE_CPOOL_ATTR_UID,                           ulnwGetUid},
    {ALTIBASE_CPOOL_ATTR_PWD,                           ulnwGetPwd},
    {ALTIBASE_CPOOL_ATTR_TRACELOG,                      ulnwGetTraceLog},
    {ALTIBASE_CPOOL_ATTR_VALIDATE_QUERY_TEXT,           ulnwGetValidateQueryText},
    {ALTIBASE_CPOOL_ATTR_VALIDATE_QUERY_RESULT,         ulnwGetValidateQueryResult},
    {ALTIBASE_CPOOL_ATTR_TOTAL_CONNECTION_COUNT,        ulnwGetTotalConnection},
    {ALTIBASE_CPOOL_ATTR_CONNECTED_CONNECTION_COUNT,    ulnwGetConnectedConnection},
    {ALTIBASE_CPOOL_ATTR_INUSE_CONNECTION_COUNT,        ulnwGetInUseConnection},
};
static acp_sint32_t gGetAttrCallbackCount = (acp_sint32_t)(sizeof(gGetAttrCallback)/sizeof(ulnwGetAttrCallback));

// handler implementation
static SQLRETURN ulnwGetMinPool(ulnwCPool *aCPool, SQLPOINTER aPtrMinPool, SQLINTEGER aZeroLen, SQLINTEGER *aNullLen)
{
    ACP_UNUSED(aZeroLen);
    ACP_UNUSED(aNullLen);

    *(SQLINTEGER *)aPtrMinPool = aCPool->mCfg.mMinPool;

    return SQL_SUCCESS;
}

static SQLRETURN ulnwGetMaxPool(ulnwCPool *aCPool, SQLPOINTER aPtrMaxPool, SQLINTEGER aZeroLen, SQLINTEGER *aNullLen)
{
    ACP_UNUSED(aZeroLen);
    ACP_UNUSED(aNullLen);

    *(SQLINTEGER *)aPtrMaxPool = aCPool->mCfg.mMaxPool;

    return SQL_SUCCESS;
}

static SQLRETURN ulnwGetIdleTimeout(ulnwCPool *aCPool, SQLPOINTER aPtrIdleTimeout, SQLINTEGER aZeroLen, SQLINTEGER *aNullLen)
{
    ACP_UNUSED(aZeroLen);
    ACP_UNUSED(aNullLen);

    *(SQLINTEGER *)aPtrIdleTimeout = aCPool->mCfg.mIdleTimeout;

    return SQL_SUCCESS;
}

static SQLRETURN ulnwGetConnectionTimeout(ulnwCPool *aCPool, SQLPOINTER aPtrConnectionTimeout, SQLINTEGER aZeroLen, SQLINTEGER *aNullLen)
{
    ACP_UNUSED(aZeroLen);
    ACP_UNUSED(aNullLen);

    *(SQLINTEGER *)aPtrConnectionTimeout = aCPool->mCfg.mConnectionTimeout;

    return SQL_SUCCESS;
}

static SQLRETURN ulnwGetDsn(ulnwCPool *aCPool, SQLPOINTER aPtrDsnStr, SQLINTEGER aDsnStrSize, SQLINTEGER *aDsnStrLen)
{
    acp_char_t  *sFuncName = "CPoolGetDsn";

    ACI_TEST_RAISE(aPtrDsnStr == NULL, LABEL_INVALIDARG);
    ACI_TEST_RAISE(aCPool->mCfg.mDataSourceLen == 0, LABEL_SKIP);
    ACI_TEST_RAISE(aCPool->mCfg.mDataSourceLen > aDsnStrSize, LABEL_NOTSUFFICIENTBUFFER);

    ACI_TEST_RAISE(acpSnprintf(aPtrDsnStr,
                               aDsnStrSize,
                               "%s",
                               aCPool->mCfg.mDataSource) != ACP_RC_SUCCESS,
                   LABEL_NOTSUFFICIENTBUFFER);

    ACI_EXCEPTION_CONT(LABEL_SKIP);
    if (aDsnStrLen != NULL)
    {
        *aDsnStrLen = aCPool->mCfg.mDataSourceLen;
    }

    return SQL_SUCCESS;

    ACI_EXCEPTION(LABEL_INVALIDARG)
    {
        ulnwError(gFuncName, ULNW_ERROR_MSG_INVALIDARG);
    }
    ACI_EXCEPTION(LABEL_NOTSUFFICIENTBUFFER)
    {
        ulnwDebug(sFuncName, "datasource length=%d give buffer size=%d", 
                aCPool->mCfg.mDataSourceLen, aDsnStrSize);
        ulnwDebug(sFuncName, "not sufficient buffer");
        ulnwError(gFuncName, ULNW_ERROR_MSG_INVALIDARG);
    }
    ACI_EXCEPTION_END;

    return SQL_ERROR;
}

static SQLRETURN ulnwGetUid(ulnwCPool *aCPool, SQLPOINTER aPtrUidStr, SQLINTEGER aUidStrSize, SQLINTEGER *aUidStrLen)
{
    acp_char_t  *sFuncName = "CPoolGetUid";

    ACI_TEST_RAISE(aPtrUidStr == NULL, LABEL_INVALIDARG);
    ACI_TEST_RAISE(aCPool->mCfg.mUserIdLen == 0, LABEL_SKIP);
    ACI_TEST_RAISE(aCPool->mCfg.mUserIdLen > aUidStrSize, LABEL_NOTSUFFICIENTBUFFER);

    ACI_TEST_RAISE(acpSnprintf(aPtrUidStr,
                               aUidStrSize,
                               "%s",
                               aCPool->mCfg.mUserId) != ACP_RC_SUCCESS,
                   LABEL_NOTSUFFICIENTBUFFER);

    ACI_EXCEPTION_CONT(LABEL_SKIP);
    if (aUidStrLen != NULL)
    {
        *aUidStrLen = aCPool->mCfg.mUserIdLen;
    }

    return SQL_SUCCESS;

    ACI_EXCEPTION(LABEL_INVALIDARG)
    {
        ulnwError(gFuncName, ULNW_ERROR_MSG_INVALIDARG);
    }
    ACI_EXCEPTION(LABEL_NOTSUFFICIENTBUFFER)
    {
        ulnwDebug(sFuncName, "userid length=%d give buffer size=%d", 
                aCPool->mCfg.mUserIdLen, aUidStrSize);
        ulnwDebug(sFuncName, "not sufficient buffer");
        ulnwError(gFuncName, ULNW_ERROR_MSG_INVALIDARG);
    }
    ACI_EXCEPTION_END;

    return SQL_ERROR;
}

static SQLRETURN ulnwGetPwd(ulnwCPool *aCPool, SQLPOINTER aPtrPwdStr, SQLINTEGER aPwdStrSize, SQLINTEGER *aPwdStrLen)
{
    acp_char_t  *sFuncName = "CPoolGetPwd";

    ACI_TEST_RAISE(aPtrPwdStr == NULL, LABEL_INVALIDARG);
    ACI_TEST_RAISE(aCPool->mCfg.mPasswordLen == 0, LABEL_SKIP);
    ACI_TEST_RAISE(aCPool->mCfg.mPasswordLen > aPwdStrSize, LABEL_NOTSUFFICIENTBUFFER);

    ACI_TEST_RAISE(acpSnprintf(aPtrPwdStr,
                               aPwdStrSize,
                               "%s",
                               aCPool->mCfg.mPassword) != ACP_RC_SUCCESS,
                   LABEL_NOTSUFFICIENTBUFFER);

    ACI_EXCEPTION_CONT(LABEL_SKIP);
    if (aPwdStrLen != NULL)
    {
        *aPwdStrLen = aCPool->mCfg.mPasswordLen;
    }

    return SQL_SUCCESS;

    ACI_EXCEPTION(LABEL_INVALIDARG)
    {
        ulnwError(gFuncName, ULNW_ERROR_MSG_INVALIDARG);
    }
    ACI_EXCEPTION(LABEL_NOTSUFFICIENTBUFFER)
    {
        ulnwDebug(sFuncName, "password length=%d give buffer size=%d", 
                aCPool->mCfg.mPasswordLen, aPwdStrSize);
        ulnwDebug(sFuncName, "not sufficient buffer");
        ulnwError(gFuncName, ULNW_ERROR_MSG_INVALIDARG);
    }
    ACI_EXCEPTION_END;

    return SQL_ERROR;
}

static SQLRETURN ulnwGetTraceLog(ulnwCPool *aCPool, SQLPOINTER aPtrLogLevel, SQLINTEGER aZeroLen  , SQLINTEGER *aNullLen)
{
    ACP_UNUSED(aZeroLen);
    ACP_UNUSED(aNullLen);

    *(SQLINTEGER *)aPtrLogLevel = aCPool->mCfg.mTraceLog;

    return SQL_SUCCESS;
}

static SQLRETURN ulnwGetValidateQueryText(ulnwCPool *aCPool, SQLPOINTER aPtrValidateQueryText, SQLINTEGER aValidateQuerySize, SQLINTEGER *aValidateQueryLen)
{
    acp_char_t  *sFuncName = "CPoolGetValidateQueryText";

    ACI_TEST_RAISE(aPtrValidateQueryText == NULL, LABEL_INVALIDARG);
    ACI_TEST_RAISE(aCPool->mCfg.mValidateQueryLen > aValidateQuerySize, LABEL_NOTSUFFICIENTBUFFER);

    if (acpSnprintf(aPtrValidateQueryText,
                    aValidateQuerySize,
                    "%s",
                    aCPool->mCfg.mValidateQueryText) == ACP_RC_SUCCESS)
    {
        if (aValidateQueryLen != NULL)
        {
            *aValidateQueryLen = acpCStrLen(aPtrValidateQueryText, aValidateQuerySize);
        }
    }
    else
    {
        if (aValidateQueryLen != NULL)
        {
            *aValidateQueryLen = 0;
        }
        ACI_RAISE(LABEL_NOTSUFFICIENTBUFFER);
    }

    return SQL_SUCCESS;

    ACI_EXCEPTION(LABEL_INVALIDARG)
    {
        ulnwError(gFuncName, ULNW_ERROR_MSG_INVALIDARG);
    }
    ACI_EXCEPTION(LABEL_NOTSUFFICIENTBUFFER)
    {
        ulnwDebug(sFuncName, "validation query text length=%d give buffer size=%d", 
                aCPool->mCfg.mValidateQueryLen, aValidateQuerySize);
        ulnwDebug(sFuncName, "not sufficient buffer");
        ulnwError(gFuncName, ULNW_ERROR_MSG_INVALIDARG);
    }
    ACI_EXCEPTION_END;

    return SQL_ERROR;
}

static SQLRETURN ulnwGetValidateQueryResult(ulnwCPool *aCPool, SQLPOINTER aPtrValidateQueryResult, SQLINTEGER aZeroLen, SQLINTEGER *aNullLen)
{
    acp_char_t  *sFuncName = "CPoolGetValidateQueryResult";

    ACP_UNUSED(aZeroLen);
    ACP_UNUSED(aNullLen);

    ACI_TEST(aCPool->mCfg.mSetValidateQueryResult == ACP_FALSE)
    *(SQLINTEGER *)aPtrValidateQueryResult = aCPool->mCfg.mValidateQueryResult;

    return SQL_SUCCESS;

    ACI_EXCEPTION_END;

    ulnwDebug(sFuncName, "validation query result has not been set");

    return SQL_ERROR;
}

static SQLRETURN ulnwGetTotalConnection(ulnwCPool *aCPool, SQLPOINTER aPtrTotalConn, SQLINTEGER aZeroLen, SQLINTEGER *aNullLen)
{
    ACP_UNUSED(aZeroLen);
    ACP_UNUSED(aNullLen);

    ulnwCPoolCheckLastTimestamp(aCPool);
    *(SQLINTEGER *)aPtrTotalConn = aCPool->mStat.mTotal;

    return SQL_SUCCESS;
}

static SQLRETURN ulnwGetConnectedConnection(ulnwCPool *aCPool, SQLPOINTER aPtrConnectedConn, SQLINTEGER aZeroLen, SQLINTEGER *aNullLen)
{
    ACP_UNUSED(aZeroLen);
    ACP_UNUSED(aNullLen);

    ulnwCPoolCheckLastTimestamp(aCPool);
    *(SQLINTEGER *)aPtrConnectedConn = aCPool->mStat.mConnected;

    return SQL_SUCCESS;
}

static SQLRETURN ulnwGetInUseConnection(ulnwCPool *aCPool, SQLPOINTER aPtrInUseConn, SQLINTEGER aZeroLen, SQLINTEGER *aNullLen)
{
    ACP_UNUSED(aZeroLen);
    ACP_UNUSED(aNullLen);

    ulnwCPoolCheckLastTimestamp(aCPool);
    *(SQLINTEGER *)aPtrInUseConn = aCPool->mStat.mInUse;

    return SQL_SUCCESS;
}


SQLRETURN ulnwCPoolGetAttr(ulnwCPool *aCPool,
                        SQLINTEGER  aAttribute,
                        SQLPOINTER  aValue,
                        SQLINTEGER  aBufferLength,
                        SQLINTEGER  *aStringLength)
{
    SQLRETURN       sReturn = SQL_SUCCESS;
    acp_sint32_t    sInd;

    ulnwTrace(gFuncName, "CPool=%p Attribute=%d Value=%p(%d) BufferLength=%d",
                    aCPool, aAttribute, aValue, aValue, aBufferLength);

    ACI_TEST_RAISE(aCPool == NULL, LABEL_INVALIDHANDLE);
    ACI_TEST_RAISE(aValue == NULL, LABEL_INVALIDARG);

    for (sInd = 0; sInd < gGetAttrCallbackCount; sInd++)
    {
        if (gGetAttrCallback[sInd].mAttrId == aAttribute)
        {
            ACI_TEST_RAISE(acpThrMutexLock(aCPool->mLock) != ACP_RC_SUCCESS, LABEL_LOCKINGFAILED);
            sReturn = gGetAttrCallback[sInd].mGetAttrCallback(aCPool, aValue, aBufferLength, aStringLength);
            ACI_TEST_RAISE(acpThrMutexUnlock(aCPool->mLock) != ACP_RC_SUCCESS, LABEL_UNLOCKINGFAILED);
            break;
        }
    }
    ACI_TEST_RAISE(sInd == gGetAttrCallbackCount, LABEL_INVALIDATTRIBUTE);
    ACI_TEST(sReturn != SQL_SUCCESS);

    ulnwDebug(gFuncName, ULNW_DEBUG_MSG_RETURNSUCCESS);

    return sReturn;

    ACI_EXCEPTION(LABEL_INVALIDHANDLE)
    {
        ulnwError(gFuncName, ULNW_ERROR_MSG_INVALIDHANDLE);
    }
    ACI_EXCEPTION(LABEL_INVALIDARG)
    {
        ulnwError(gFuncName, ULNW_ERROR_MSG_INVALIDARG);
    }
    ACI_EXCEPTION(LABEL_INVALIDATTRIBUTE)
    {
        ulnwError(gFuncName, ULNW_ERROR_MSG_INVALIDATTRIBUTE);
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

