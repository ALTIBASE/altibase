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

#define READONLY_PROPERTY_BODY                              \
    ACP_UNUSED(aCPool);                                     \
    ACP_UNUSED(aReadonlyAttr);                              \
    ACP_UNUSED(aZeroLen);                                   \
                                                            \
    ulnwDebug(gFuncName, "this property is readonly");      \
    return SQL_ERROR;

static acp_char_t *gFuncName = "SQLCPoolSetAttr";

static SQLRETURN ulnwSetMinPool(ulnwCPool *aCPool, SQLPOINTER aPtrMinPool, SQLINTEGER aZeroLen);
static SQLRETURN ulnwSetMaxPool(ulnwCPool *aCPool, SQLPOINTER aPtrMaxPool, SQLINTEGER aZeroLen);
static SQLRETURN ulnwSetIdleTimeout(ulnwCPool *aCPool, SQLPOINTER aPtrIdleTimeout, SQLINTEGER aZeroLen);
static SQLRETURN ulnwSetConnectionTimeout(ulnwCPool *aCPool, SQLPOINTER aPtrConnectionTimeout, SQLINTEGER aZeroLen);

static SQLRETURN ulnwSetDsn(ulnwCPool *aCPool, SQLPOINTER aPtrDsn, SQLINTEGER aDsnLen);
static SQLRETURN ulnwSetUid(ulnwCPool *aCPool, SQLPOINTER aPtrUid, SQLINTEGER aUidLen);
static SQLRETURN ulnwSetPwd(ulnwCPool *aCPool, SQLPOINTER aPtrPwd, SQLINTEGER aPwdLen);

static SQLRETURN ulnwSetTraceLog(ulnwCPool *aCPool, SQLPOINTER aPtrTraceLog, SQLINTEGER aZeroLen);
static SQLRETURN ulnwSetValidateQueryText(ulnwCPool *aCPool, SQLPOINTER aPtrValidateQueryText, SQLINTEGER aValidateQueryLen);
static SQLRETURN ulnwSetValidateQueryResult(ulnwCPool *aCPool, SQLPOINTER aPtrValidateQueryResult, SQLINTEGER aZeroLen);

static SQLRETURN ulnwSetTotalConnection(ulnwCPool *aCPool, SQLPOINTER aPtrTotalConn, SQLINTEGER aZeroLen);
static SQLRETURN ulnwSetConnectedConnection(ulnwCPool *aCPool, SQLPOINTER aPtrConnectedConn, SQLINTEGER aZeroLen);
static SQLRETURN ulnwSetInUseConnection(ulnwCPool *aCPool, SQLPOINTER aPtrInUseConn, SQLINTEGER aZeroLen);

typedef struct ulnwSetAttrCallback
{
    SQLINTEGER      mAttrId;
    SQLRETURN       (*mSetAttrCallback)(ulnwCPool *, SQLPOINTER, SQLINTEGER);
} ulnwSetAttrCallback;

static struct ulnwSetAttrCallback gSetAttrCallback[] =
{
    {ALTIBASE_CPOOL_ATTR_MIN_POOL,                      ulnwSetMinPool},
    {ALTIBASE_CPOOL_ATTR_MAX_POOL,                      ulnwSetMaxPool},
    {ALTIBASE_CPOOL_ATTR_IDLE_TIMEOUT,                  ulnwSetIdleTimeout},
    {ALTIBASE_CPOOL_ATTR_CONNECTION_TIMEOUT,            ulnwSetConnectionTimeout},
    {ALTIBASE_CPOOL_ATTR_DSN,                           ulnwSetDsn},
    {ALTIBASE_CPOOL_ATTR_UID,                           ulnwSetUid},
    {ALTIBASE_CPOOL_ATTR_PWD,                           ulnwSetPwd},
    {ALTIBASE_CPOOL_ATTR_TRACELOG,                      ulnwSetTraceLog},
    {ALTIBASE_CPOOL_ATTR_VALIDATE_QUERY_TEXT,           ulnwSetValidateQueryText},
    {ALTIBASE_CPOOL_ATTR_VALIDATE_QUERY_RESULT,         ulnwSetValidateQueryResult},

    {ALTIBASE_CPOOL_ATTR_TOTAL_CONNECTION_COUNT,        ulnwSetTotalConnection},
    {ALTIBASE_CPOOL_ATTR_CONNECTED_CONNECTION_COUNT,    ulnwSetConnectedConnection},
    {ALTIBASE_CPOOL_ATTR_INUSE_CONNECTION_COUNT,        ulnwSetInUseConnection},
};
static acp_sint32_t gSetAttrCallbackCount = (acp_sint32_t)(sizeof(gSetAttrCallback)/sizeof(ulnwSetAttrCallback));

static SQLRETURN ulnwSetMinPool(ulnwCPool *aCPool, SQLPOINTER aPtrMinPool, SQLINTEGER aZeroLen)
{
    acp_slong_t     sValue = (acp_slong_t)aPtrMinPool;
    acp_char_t      *sFuncName = "CPoolSetMinPool";

    ACP_UNUSED(aZeroLen);

    ACI_TEST_RAISE(sValue > ACP_SINT32_MAX, LABEL_INVALIDARG);
    ACI_TEST_RAISE(aCPool->mStat.mTotal > 0, LABEL_UNABLETOCHANGE);

    aCPool->mCfg.mMinPool = (acp_sint32_t)sValue;

    return SQL_SUCCESS;

    ACI_EXCEPTION(LABEL_INVALIDARG)
    {
        ulnwError(gFuncName, ULNW_ERROR_MSG_INVALIDARG);
    }
    ACI_EXCEPTION(LABEL_UNABLETOCHANGE)
    {
        ulnwDebug(sFuncName, "unable to change MinPool because CPool has been connected");
        ulnwError(gFuncName, ULNW_ERROR_MSG_ALREADYCONNECTED);
    }
    ACI_EXCEPTION_END;

    return SQL_ERROR;
}

static SQLRETURN ulnwSetMaxPool(ulnwCPool *aCPool, SQLPOINTER aPtrMaxPool, SQLINTEGER aZeroLen)
{
    acp_slong_t     sValue = (acp_slong_t)aPtrMaxPool;
    acp_char_t      *sFuncName = "CPoolSetMaxPool";

    ACP_UNUSED(aZeroLen);

    ACI_TEST_RAISE(sValue > ACP_SINT32_MAX, LABEL_INVALIDARG);
    ACI_TEST_RAISE(aCPool->mStat.mTotal > 0, LABEL_UNABLETOCHANGE);

    aCPool->mCfg.mMaxPool = sValue;

    return SQL_SUCCESS;

    ACI_EXCEPTION(LABEL_INVALIDARG)
    {
        ulnwError(gFuncName, ULNW_ERROR_MSG_INVALIDARG);
    }
    ACI_EXCEPTION(LABEL_UNABLETOCHANGE)
    {
        ulnwDebug(sFuncName, "unable to change MaxPool because CPool has been connected");
        ulnwError(gFuncName, ULNW_ERROR_MSG_ALREADYCONNECTED);
    }
    ACI_EXCEPTION_END;

    return SQL_ERROR;
}

static SQLRETURN ulnwSetIdleTimeout(ulnwCPool *aCPool, SQLPOINTER aPtrIdleTimeout, SQLINTEGER aZeroLen)
{
    acp_slong_t     sValue = (acp_slong_t)aPtrIdleTimeout;

    ACP_UNUSED(aZeroLen);

    ACI_TEST_RAISE(sValue > ACP_SINT32_MAX, LABEL_INVALIDARG);

    aCPool->mCfg.mIdleTimeout = (acp_sint32_t)sValue;

    return SQL_SUCCESS;

    ACI_EXCEPTION(LABEL_INVALIDARG)
    {
        ulnwError(gFuncName, ULNW_ERROR_MSG_INVALIDARG);
    }
    ACI_EXCEPTION_END;

    return SQL_ERROR;
}

static SQLRETURN ulnwSetConnectionTimeout(ulnwCPool *aCPool, SQLPOINTER aPtrConnectionTimeout, SQLINTEGER aZeroLen)
{
    acp_slong_t     sValue = (acp_slong_t)aPtrConnectionTimeout;

    ACP_UNUSED(aZeroLen);

    ACI_TEST_RAISE(sValue > ACP_SINT32_MAX, LABEL_INVALIDARG);

    aCPool->mCfg.mConnectionTimeout = (acp_sint32_t)sValue;

    return SQL_SUCCESS;

    ACI_EXCEPTION(LABEL_INVALIDARG)
    {
        ulnwError(gFuncName, ULNW_ERROR_MSG_INVALIDARG);
    }
    ACI_EXCEPTION_END;

    return SQL_ERROR;
}

static SQLRETURN ulnwSetDsn(ulnwCPool *aCPool, SQLPOINTER aPtrDsn, SQLINTEGER aDsnLen)
{
    acp_char_t      *sFuncName  = "CPoolSetDsn";
    acp_size_t      sLen        = 0;

    ACI_TEST_RAISE(aPtrDsn == NULL, LABEL_INVALIDARG);

    sLen = acpCStrLen(aPtrDsn, aDsnLen);
    ACI_TEST_RAISE(sLen > ACI_SIZEOF(aCPool->mCfg.mDataSource), LABEL_TOOLONG);

    ulnwDebug(sFuncName, "DSN=%s (%d)", aPtrDsn, aDsnLen);

    ACI_TEST_RAISE(acpSnprintf(aCPool->mCfg.mDataSource,
                               ACI_SIZEOF(aCPool->mCfg.mDataSource),
                               "%s",
                               aPtrDsn) != ACP_RC_SUCCESS,
                   LABEL_COPYFAILED);

    aCPool->mCfg.mDataSourceLen = sLen;

    return SQL_SUCCESS;

    ACI_EXCEPTION(LABEL_INVALIDARG)
    {
        ulnwError(gFuncName, ULNW_ERROR_MSG_INVALIDARG);
    }
    ACI_EXCEPTION(LABEL_TOOLONG)
    {
        ulnwDebug(sFuncName, "DSN is too long (given:%d, strlen:%d)", aDsnLen, sLen);
        ulnwError(gFuncName, ULNW_ERROR_MSG_INVALIDARG);
    }
    ACI_EXCEPTION(LABEL_COPYFAILED)
    {
        ulnwDebug(sFuncName, "failed to copy DSN");
        ulnwError(gFuncName, ULNW_ERROR_MSG_INVALIDARG);

        aCPool->mCfg.mDataSource[0] = '\0';
        aCPool->mCfg.mDataSourceLen = 0;
    }
    ACI_EXCEPTION_END;

    return SQL_ERROR;
}

static SQLRETURN ulnwSetUid(ulnwCPool *aCPool, SQLPOINTER aPtrUid, SQLINTEGER aUidLen)
{
    acp_char_t      *sFuncName  = "CPoolSetUid";
    acp_size_t      sLen        = 0;

    ACI_TEST_RAISE(aPtrUid == NULL, LABEL_INVALIDARG);

    sLen = acpCStrLen(aPtrUid, aUidLen);
    ACI_TEST_RAISE(sLen > ACI_SIZEOF(aCPool->mCfg.mUserId), LABEL_TOOLONG);

    ulnwDebug(sFuncName, "UID=%s (%d)", aPtrUid, aUidLen);

    ACI_TEST_RAISE(acpSnprintf(aCPool->mCfg.mUserId,
                               ACI_SIZEOF(aCPool->mCfg.mUserId),
                               "%s",
                               aPtrUid) != ACP_RC_SUCCESS,
                   LABEL_COPYFAILED);

    aCPool->mCfg.mUserIdLen = sLen;

    return SQL_SUCCESS;

    ACI_EXCEPTION(LABEL_INVALIDARG)
    {
        ulnwError(gFuncName, ULNW_ERROR_MSG_INVALIDARG);
    }
    ACI_EXCEPTION(LABEL_TOOLONG)
    {
        ulnwDebug(sFuncName, "UID is too long (given:%d, strlen:%d)", aUidLen, sLen);
        ulnwError(gFuncName, ULNW_ERROR_MSG_INVALIDARG);
    }
    ACI_EXCEPTION(LABEL_COPYFAILED)
    {
        ulnwDebug(sFuncName, "failed to copy Uid");
        ulnwError(gFuncName, ULNW_ERROR_MSG_INVALIDARG);

        aCPool->mCfg.mUserId[0] = '\0';
        aCPool->mCfg.mUserIdLen = 0;
    }
    ACI_EXCEPTION_END;

    return SQL_ERROR;
}

static SQLRETURN ulnwSetPwd(ulnwCPool *aCPool, SQLPOINTER aPtrPwd, SQLINTEGER aPwdLen)
{
    acp_char_t      *sFuncName  = "CPoolSetPwd";
    acp_size_t      sLen        = 0;

    ACI_TEST_RAISE(aPtrPwd == NULL, LABEL_INVALIDARG);

    sLen = acpCStrLen(aPtrPwd, aPwdLen);
    ACI_TEST_RAISE(sLen > ACI_SIZEOF(aCPool->mCfg.mPassword), LABEL_TOOLONG);

    ulnwDebug(sFuncName, "PWD=%s (%d)", aPtrPwd, aPwdLen);

    ACI_TEST_RAISE(acpSnprintf(aCPool->mCfg.mPassword,
                               ACI_SIZEOF(aCPool->mCfg.mPassword),
                               "%s",
                               aPtrPwd) != ACP_RC_SUCCESS,
                   LABEL_COPYFAILED);

    aCPool->mCfg.mPasswordLen = sLen;

    return SQL_SUCCESS;

    ACI_EXCEPTION(LABEL_INVALIDARG)
    {
        ulnwError(gFuncName, ULNW_ERROR_MSG_INVALIDARG);
    }
    ACI_EXCEPTION(LABEL_TOOLONG)
    {
        ulnwDebug(sFuncName, "PWD is too long (given:%d, strlen:%d)", aPwdLen, sLen);
        ulnwError(gFuncName, ULNW_ERROR_MSG_INVALIDARG);
    }
    ACI_EXCEPTION(LABEL_COPYFAILED)
    {
        ulnwDebug(sFuncName, "failed to copy PWD");
        ulnwError(gFuncName, ULNW_ERROR_MSG_INVALIDARG);

        aCPool->mCfg.mPassword[0] = '\0';
        aCPool->mCfg.mPasswordLen = 0;
    }
    ACI_EXCEPTION_END;

    return SQL_ERROR;
}


static SQLRETURN ulnwSetTraceLog(ulnwCPool *aCPool, SQLPOINTER aPtrTraceLog, SQLINTEGER aZeroLen  )
{
    acp_slong_t     sValue      = (acp_slong_t)aPtrTraceLog;

    ACP_UNUSED(aZeroLen);

    ACI_TEST_RAISE(
            ((acp_sint32_t)sValue != ALTIBASE_TRACELOG_NONE) &&
            ((acp_sint32_t)sValue != ALTIBASE_TRACELOG_ERROR) &&
            ((acp_sint32_t)sValue != ALTIBASE_TRACELOG_FULL) &&
            ((acp_sint32_t)sValue != ALTIBASE_TRACELOG_DEBUG)
            , LABEL_INVALIDARG);

    aCPool->mCfg.mTraceLog = (acp_sint32_t)sValue;

    ulnwTraceSet(aCPool->mCfg.mTraceLog);

    return SQL_SUCCESS;

    ACI_EXCEPTION(LABEL_INVALIDARG)
    {
        ulnwError(gFuncName, ULNW_ERROR_MSG_INVALIDARG);
    }
    ACI_EXCEPTION_END;

    return SQL_ERROR;
}


static SQLRETURN ulnwSetValidateQueryText(ulnwCPool *aCPool, SQLPOINTER aPtrValidateQueryText, SQLINTEGER aValidateQueryLen)
{
    acp_char_t      *sFuncName  = "CPoolSetValidateQueryText";
    acp_size_t      sLen        = 0;

    ACI_TEST_RAISE(aPtrValidateQueryText == NULL, LABEL_INVALIDARG);

    sLen = acpCStrLen(aPtrValidateQueryText, aValidateQueryLen);
    ACI_TEST_RAISE(sLen > ACI_SIZEOF(aCPool->mCfg.mValidateQueryText), LABEL_QUERYTOOLONG);

    ACI_TEST_RAISE(acpSnprintf(aCPool->mCfg.mValidateQueryText,
                               ACI_SIZEOF(aCPool->mCfg.mValidateQueryText), 
                               "%s",
                               aPtrValidateQueryText) != ACP_RC_SUCCESS, 
                   LABEL_COPYFAILED);

    aCPool->mCfg.mValidateQueryLen = acpCStrLen((acp_char_t *)aCPool->mCfg.mValidateQueryText, ULNW_MAX_STRING_LENGTH);

    return SQL_SUCCESS;

    ACI_EXCEPTION(LABEL_INVALIDARG)
    {
        ulnwError(gFuncName, ULNW_ERROR_MSG_INVALIDARG);
    }
    ACI_EXCEPTION(LABEL_QUERYTOOLONG)
    {
        ulnwDebug(sFuncName, "query is too long (%d)", aValidateQueryLen);
        ulnwError(gFuncName, ULNW_ERROR_MSG_INVALIDARG);
    }
    ACI_EXCEPTION(LABEL_COPYFAILED)
    {
        ulnwDebug(sFuncName, "failed to copy validation query text");
        ulnwError(gFuncName, ULNW_ERROR_MSG_INVALIDARG);

        aCPool->mCfg.mValidateQueryText[0] = '\0';
        aCPool->mCfg.mValidateQueryLen = 0;
    }
    ACI_EXCEPTION_END;


    return SQL_ERROR;
}

static SQLRETURN ulnwSetValidateQueryResult(ulnwCPool *aCPool, SQLPOINTER aPtrValidateQueryResult, SQLINTEGER aZeroLen)
{
    acp_slong_t     sValue = (acp_slong_t)aPtrValidateQueryResult;
    ACP_UNUSED(aZeroLen);

    aCPool->mCfg.mValidateQueryResult       = (acp_sint32_t)sValue;
    aCPool->mCfg.mSetValidateQueryResult    = ACP_TRUE;

    return SQL_SUCCESS;
}


static SQLRETURN ulnwSetTotalConnection(ulnwCPool *aCPool, SQLPOINTER aReadonlyAttr, SQLINTEGER aZeroLen)
{
    READONLY_PROPERTY_BODY;
}

static SQLRETURN ulnwSetConnectedConnection(ulnwCPool *aCPool, SQLPOINTER aReadonlyAttr, SQLINTEGER aZeroLen)
{
    READONLY_PROPERTY_BODY;
}

static SQLRETURN ulnwSetInUseConnection(ulnwCPool *aCPool, SQLPOINTER aReadonlyAttr, SQLINTEGER aZeroLen)
{
    READONLY_PROPERTY_BODY;
}


SQLRETURN ulnwCPoolSetAttr(ulnwCPool *aCPool,
                        SQLINTEGER  aAttribute,
                        SQLPOINTER  aValue,
                        SQLINTEGER  aBufferLength)
{
    SQLRETURN       sReturn = SQL_SUCCESS;
    acp_sint32_t    sInd;

    ulnwTrace(gFuncName, "CPool=%p Attribute=%d Value=%p(%d), BufferLength=%d",
                aCPool, aAttribute, aValue, aValue, aBufferLength);

    ACI_TEST_RAISE(aCPool == NULL, LABEL_INVALIDHANDLE);

    for (sInd = 0; sInd < gSetAttrCallbackCount; sInd++)
    {
        if (gSetAttrCallback[sInd].mAttrId == aAttribute)
        {
            acpThrMutexLock(aCPool->mLock);
            sReturn = gSetAttrCallback[sInd].mSetAttrCallback(aCPool, aValue, aBufferLength);
            acpThrMutexUnlock(aCPool->mLock);
            break;
        }
    }
    ACI_TEST_RAISE(sInd == gSetAttrCallbackCount, LABEL_INVALIDATTRIBUTE);
    ACI_TEST(sReturn != SQL_SUCCESS);

    ulnwDebug(gFuncName, ULNW_DEBUG_MSG_RETURNSUCCESS);

    return sReturn;

    ACI_EXCEPTION(LABEL_INVALIDHANDLE)
    {
        ulnwError(gFuncName, ULNW_ERROR_MSG_INVALIDHANDLE);
    }
    ACI_EXCEPTION(LABEL_INVALIDATTRIBUTE)
    {
        ulnwError(gFuncName, ULNW_ERROR_MSG_INVALIDATTRIBUTE);
    }
    ACI_EXCEPTION_END;

    ulnwDebug(gFuncName, ULNW_DEBUG_MSG_RETURNERROR);
    return SQL_ERROR;
}

