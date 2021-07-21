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

#ifndef _O_CMP_ARG_DB_CLIENT_H_
#define _O_CMP_ARG_DB_CLIENT_H_ 1

/**********************************************************
 * proj_2160 cm_type removal
 * structures below are used only for ALTIBASE 5
 * naming rule: cmpArgDBxxxxxA5
 **********************************************************/

typedef struct cmpArgDBMessageA5
{
    cmtVariable         mMessage;
} cmpArgDBMessageA5;

typedef struct cmpArgDBErrorResultA5
{
    acp_uint8_t         mOperationID;
    acp_uint16_t        mErrorIndex;
    acp_uint32_t        mErrorCode;
    cmtVariable         mErrorMessage;
} cmpArgDBErrorResultA5;

typedef struct cmpArgDBErrorInfoA5
{
    acp_uint32_t        mErrorCode;
} cmpArgDBErrorInfoA5;

typedef struct cmpArgDBErrorInfoResultA5
{
    acp_uint32_t        mErrorCode;
    cmtVariable         mErrorMessage;
} cmpArgDBErrorInfoResultA5;

typedef struct cmpArgDBConnectA5
{
    cmtVariable         mDatabaseName;
    cmtVariable         mUserName;
    cmtVariable         mPassword;
    acp_uint16_t        mMode;
} cmpArgDBConnectA5;

typedef struct cmpArgDBDisconnectA5
{
    acp_uint8_t         mOption;
} cmpArgDBDisconnectA5;

typedef struct cmpArgDBPropertyGetA5
{
    acp_uint16_t        mPropertyID;
} cmpArgDBPropertyGetA5;

typedef struct cmpArgDBPropertyGetResultA5
{
    acp_uint16_t        mPropertyID;
    cmtAny              mValue;
} cmpArgDBPropertyGetResultA5;

typedef struct cmpArgDBPropertySetA5
{
    acp_uint16_t        mPropertyID;
    cmtAny              mValue;
} cmpArgDBPropertySetA5;

typedef struct cmpArgDBPrepareA5
{
    acp_uint32_t        mStatementID;
    cmtVariable         mStatementString;
    acp_uint8_t         mMode;
} cmpArgDBPrepareA5;

typedef struct cmpArgDBPrepareResultA5
{
    acp_uint32_t        mStatementID;
    acp_uint32_t        mStatementType;
    acp_uint16_t        mParamCount;
    acp_uint16_t        mResultSetCount;
    /*
     * bug-25109 support ColAttribute baseTableName
     */
    cmtVariable         mBaseTableOwnerName;
    cmtVariable         mBaseTableName;
    acp_uint16_t        mBaseTableUpdatable;
    cmtVariable         mTableName;   /* unused */
} cmpArgDBPrepareResultA5;

typedef struct cmpArgDBPlanGetA5
{
    acp_uint32_t        mStatementID;
} cmpArgDBPlanGetA5;

typedef struct cmpArgDBPlanGetResultA5
{
    acp_uint32_t        mStatementID;
    cmtVariable         mPlan;
} cmpArgDBPlanGetResultA5;

typedef struct cmpArgDBColumnInfoGetA5
{
    acp_uint32_t        mStatementID;
    acp_uint16_t        mResultSetID;
    acp_uint16_t        mColumnNumber;
} cmpArgDBColumnInfoGetA5;

typedef struct cmpArgDBColumnInfoGetResultA5
{
    acp_uint32_t        mStatementID;
    acp_uint16_t        mResultSetID;
    acp_uint16_t        mColumnNumber;
    acp_uint32_t        mDataType;
    acp_uint32_t        mLanguage;
    acp_uint8_t         mArguments;
    acp_sint32_t        mPrecision;
    acp_sint32_t        mScale;
    acp_uint8_t         mNullableFlag;
    cmtVariable         mDisplayName;
} cmpArgDBColumnInfoGetResultA5;

typedef struct cmpCollectionDBColumnInfoGetResultA5
{
    acp_uint32_t        mDataType;
    acp_uint32_t        mLanguage;
    acp_uint8_t         mArguments;
    acp_sint32_t        mPrecision;
    acp_sint32_t        mScale;
    acp_uint8_t         mNullableFlag;
    cmtAny              mDisplayName;
} cmpCollectionDBColumnInfoGetResultA5;

typedef struct cmpArgDBColumnInfoGetListResultA5
{
    acp_uint32_t        mStatementID;
    acp_uint16_t        mResultSetID;
    acp_uint16_t        mColumnCount;
    cmtCollection       mListData;
} cmpArgDBColumnInfoGetListResultA5;

typedef struct cmpArgDBColumnInfoSetA5
{
    acp_uint32_t        mStatementID;
    acp_uint16_t        mResultSetID;
    acp_uint16_t        mColumnNumber;
    acp_uint32_t        mDataType;
    acp_uint32_t        mLanguage;
    acp_uint8_t         mArguments;
    acp_sint32_t        mPrecision;
    acp_sint32_t        mScale;
} cmpArgDBColumnInfoSetA5;

typedef struct cmpArgDBParamInfoGetA5
{
    acp_uint32_t        mStatementID;
    acp_uint16_t        mParamNumber;
} cmpArgDBParamInfoGetA5;

typedef struct cmpArgDBParamInfoGetResultA5
{
    acp_uint32_t        mStatementID;
    acp_uint16_t        mParamNumber;
    acp_uint32_t        mDataType;
    acp_uint32_t        mLanguage;
    acp_uint8_t         mArguments;
    acp_sint32_t        mPrecision;
    acp_sint32_t        mScale;
    acp_uint8_t         mInOutType;
    acp_uint8_t         mNullableFlag;
} cmpArgDBParamInfoGetResultA5;

typedef struct cmpArgDBParamInfoSetA5
{
    acp_uint32_t        mStatementID;
    acp_uint16_t        mParamNumber;
    acp_uint32_t        mDataType;
    acp_uint32_t        mLanguage;
    acp_uint8_t         mArguments;
    acp_sint32_t        mPrecision;
    acp_sint32_t        mScale;
    acp_uint8_t         mInOutType;
} cmpArgDBParamInfoSetA5;

typedef struct cmpArgDBParamInfoSetListA5
{
    acp_uint32_t        mStatementID;
    acp_uint16_t        mParamCount;
    cmtCollection       mListData;
} cmpArgDBParamInfoSetListA5;

typedef struct cmpCollectionDBParamInfoSetA5
{
    acp_uint16_t        mParamNumber;
    acp_uint32_t        mDataType;
    acp_uint32_t        mLanguage;
    acp_uint8_t         mArguments;
    acp_sint32_t        mPrecision;
    acp_sint32_t        mScale;
    acp_uint8_t         mInOutType;
} cmpCollectionDBParamInfoSetA5;

typedef struct cmpArgDBParamDataInA5
{
    acp_uint32_t        mStatementID;
    acp_uint16_t        mParamNumber;
    cmtAny              mData;
} cmpArgDBParamDataInA5;

typedef struct cmpArgDBParamDataOutA5
{
    acp_uint32_t        mStatementID;
    acp_uint16_t        mRowNumber;
    acp_uint16_t        mParamNumber;
    cmtAny              mData;
} cmpArgDBParamDataOutA5;

typedef struct cmpArgDBParamDataOutListA5
{
    acp_uint32_t        mStatementID;
    acp_uint16_t        mRowNumber;
    cmtCollection       mListData;
} cmpArgDBParamDataOutListA5;

typedef struct cmpArgDBParamDataInListA5
{
    acp_uint32_t        mStatementID;
    acp_uint16_t        mFromRowNumber;
    acp_uint16_t        mToRowNumber;
    acp_uint8_t         mExecuteOption;
    cmtCollection       mListData;
} cmpArgDBParamDataInListA5;

typedef struct cmpArgDBParamDataInListResultA5
{
    acp_uint32_t        mStatementID;
    acp_uint16_t        mFromRowNumber;
    acp_uint16_t        mToRowNumber;
    acp_uint16_t        mResultSetCount;
    acp_uint64_t        mAffectedRowCount;
} cmpArgDBParamDataInListResultA5;

typedef struct cmpArgDBExecuteA5
{
    acp_uint32_t        mStatementID;
    acp_uint16_t        mRowNumber;
    acp_uint8_t         mOption;
} cmpArgDBExecuteA5;

typedef struct cmpArgDBExecuteResultA5
{
    acp_uint32_t        mStatementID;
    acp_uint16_t        mRowNumber;
    acp_uint16_t        mResultSetCount;
    acp_uint64_t        mAffectedRowCount;
} cmpArgDBExecuteResultA5;

typedef struct cmpArgDBFetchMoveA5
{
    acp_uint32_t        mStatementID;
    acp_uint16_t        mResultSetID;
    acp_uint8_t         mWhence;
    acp_sint64_t        mOffset;
} cmpArgDBFetchMoveA5;

typedef struct cmpArgDBFetchA5
{
    acp_uint32_t        mStatementID;
    acp_uint16_t        mResultSetID;
    acp_uint16_t        mRecordCount;
    acp_uint16_t        mColumnFrom;
    acp_uint16_t        mColumnTo;
} cmpArgDBFetchA5;

typedef struct cmpArgDBFetchBeginResultA5
{
    acp_uint32_t        mStatementID;
    acp_uint16_t        mResultSetID;
} cmpArgDBFetchBeginResultA5;

typedef struct cmpArgDBFetchResultA5
{
    cmtAny              mData;
} cmpArgDBFetchResultA5;

typedef struct cmpArgDBFetchListResultA5
{
    acp_uint16_t          mRecordCount;
    acp_uint32_t          mRecordSize;
    cmtCollection         mListData;
} cmpArgDBFetchListResultA5;

typedef struct cmpArgDBFetchEndResultA5
{
    acp_uint32_t        mStatementID;
    acp_uint16_t        mResultSetID;
} cmpArgDBFetchEndResultA5;

typedef struct cmpArgDBFreeA5
{
    acp_uint32_t        mStatementID;
    acp_uint16_t        mResultSetID;
    acp_uint8_t         mMode;
} cmpArgDBFreeA5;

typedef struct cmpArgDBCancelA5
{
    acp_uint32_t        mStatementID;
} cmpArgDBCancelA5;

typedef struct cmpArgDBTransactionA5
{
    acp_uint8_t         mOperation;
} cmpArgDBTransactionA5;

typedef struct cmpArgDBLobGetSizeA5
{
    acp_uint64_t        mLocatorID;
} cmpArgDBLobGetSizeA5;

typedef struct cmpArgDBLobGetSizeResultA5
{
    acp_uint64_t        mLocatorID;
    acp_uint32_t        mSize;
} cmpArgDBLobGetSizeResultA5;

typedef struct cmpArgDBLobGetA5
{
    acp_uint64_t        mLocatorID;
    acp_uint32_t        mOffset;
    acp_uint32_t        mSize;
} cmpArgDBLobGetA5;

typedef struct cmpArgDBLobGetBytePosCharLenA5
{
    acp_uint64_t        mLocatorID;
    acp_uint32_t        mOffset;
    acp_uint32_t        mSize;
} cmpArgDBLobGetBytePosCharLenA5;

typedef struct cmpArgDBLobGetCharPosCharLenA5
{
    acp_uint64_t        mLocatorID;
    acp_uint32_t        mOffset;
    acp_uint32_t        mSize;
} cmpArgDBLobGetCharPosCharLenA5;

typedef struct cmpArgDBLobBytePosA5
{
    acp_uint64_t        mLocatorID;
    acp_uint32_t        mCharOffset;
} cmpArgDBLobBytePosA5;

typedef struct cmpArgDBLobGetResultA5
{
    acp_uint64_t        mLocatorID;
    acp_uint32_t        mOffset;
    cmtVariable         mData;
} cmpArgDBLobGetResultA5;

typedef struct cmpArgDBLobGetBytePosCharLenResultA5
{
    acp_uint64_t        mLocatorID;
    acp_uint32_t        mOffset;
    acp_uint32_t        mCharLength;
    cmtVariable         mData;
} cmpArgDBLobGetBytePosCharLenResultA5;

typedef struct cmpArgDBLobBytePosResultA5
{
    acp_uint64_t        mLocatorID;
    acp_uint32_t        mByteOffset;
} cmpArgDBLobBytePosResultA5;

typedef struct cmpArgDBLobCharLengthA5
{
    acp_uint64_t        mLocatorID;
} cmpArgDBLobCharLengthA5;

typedef struct cmpArgDBLobCharLengthResultA5
{
    acp_uint64_t        mLocatorID;
    acp_uint32_t        mLength;
} cmpArgDBLobCharLengthResultA5;

/* PROJ-2047 Strengthening LOB - Removed mOldSize */
typedef struct cmpArgDBLobPutBeginA5
{
    acp_uint64_t        mLocatorID;
    acp_uint32_t        mOffset;
    acp_uint32_t        mOldSize; /* Not Used */
    acp_uint32_t        mNewSize;
} cmpArgDBLobPutBeginA5;

/* PROJ-2047 Strengthening LOB - Removed mOffset */
typedef struct cmpArgDBLobPutA5
{
    acp_uint64_t        mLocatorID;
    acp_uint32_t        mOffset; /* Not Used */
    cmtVariable         mData;
} cmpArgDBLobPutA5;

typedef struct cmpArgDBLobPutEndA5
{
    acp_uint64_t        mLocatorID;
} cmpArgDBLobPutEndA5;

typedef struct cmpArgDBLobFreeA5
{
    acp_uint64_t        mLocatorID;
} cmpArgDBLobFreeA5;

typedef struct cmpArgDBLobFreeAllA5
{
    acp_uint64_t        mLocatorCount;
    cmtCollection       mListData;
} cmpArgDBLobFreeAllA5;

/* PROJ-1573 XA */
typedef struct cmpArgDBXaOperationA5
{
    acp_uint8_t         mOperation;
    acp_sint32_t        mRmID;
    acp_sint64_t        mFlag;
    acp_sint64_t        mArgument;
} cmpArgDBXaOperationA5;

typedef struct cmpArgDBXaXidA5
{
    acp_sint64_t        mFormatID;
    acp_sint64_t        mGTRIDLength;
    acp_sint64_t        mBQUALLength;
    cmtVariable         mData;
} cmpArgDBXaXidA5;

typedef struct cmpArgDBXaResultA5
{
    acp_uint8_t         mOperation;
    acp_sint32_t        mReturnValue;
} cmpArgDBXaResultA5;

typedef struct cmpArgDBXaTransactionA5
{
    acp_uint8_t         mOperation;
    acp_sint32_t        mRmID;
    acp_sint64_t        mFlag;
    acp_sint64_t        mArgument;
    acp_sint64_t        mFormatID;
    acp_sint64_t        mGTRIDLength;
    acp_sint64_t        mBQUALLength;
    cmtVariable         mData;
} cmpArgDBXaTransactionA5;

/* PROJ-2177 User Interface - Cancel */

typedef struct cmpArgDBPrepareByCID
{
    acp_uint32_t        mStmtCID;
    cmtVariable         mStatementString;
    acp_uint8_t         mMode;
} cmpArgDBPrepareByCID;

typedef struct cmpArgDBCancelByCID
{
    acp_uint32_t        mStmtCID;
} cmpArgDBCancelByCID;

typedef union cmpArgDB
{
    cmpArgDBMessageA5                     mMessage;
    cmpArgDBErrorResultA5                 mErrorResult;
    cmpArgDBErrorInfoA5                   mErrorInfo;
    cmpArgDBErrorInfoResultA5             mErrorInfoResult;
    cmpArgDBConnectA5                     mConnect;
    cmpArgDBDisconnectA5                  mDisconnect;
    cmpArgDBPropertyGetA5                 mPropertyGet;
    cmpArgDBPropertyGetResultA5           mPropertyGetResult;
    cmpArgDBPropertySetA5                 mPropertySet;
    cmpArgDBPrepareA5                     mPrepare;
    cmpArgDBPrepareResultA5               mPrepareResult;
    cmpArgDBPlanGetA5                     mPlanGet;
    cmpArgDBPlanGetResultA5               mPlanGetResult;
    cmpArgDBColumnInfoGetA5               mColumnInfoGet;
    cmpArgDBColumnInfoGetResultA5         mColumnInfoGetResult;
    cmpArgDBColumnInfoGetListResultA5     mColumnInfoGetListResult;
    cmpArgDBColumnInfoSetA5               mColumnInfoSet;
    cmpArgDBParamInfoGetA5                mParamInfoGet;
    cmpArgDBParamInfoGetResultA5          mParamInfoGetResult;
    cmpArgDBParamInfoSetA5                mParamInfoSet;
    cmpArgDBParamInfoSetListA5            mParamInfoSetList;
    cmpArgDBParamDataInA5                 mParamDataIn;
    cmpArgDBParamDataOutA5                mParamDataOut;
    cmpArgDBParamDataOutListA5            mParamDataOutList;
    cmpArgDBParamDataInListA5             mParamDataInList;
    cmpArgDBParamDataInListResultA5       mParamDataInListResult;
    cmpArgDBExecuteA5                     mExecute;
    cmpArgDBExecuteResultA5               mExecuteResult;
    cmpArgDBFetchMoveA5                   mFetchMove;
    cmpArgDBFetchA5                       mFetch;
    cmpArgDBFetchBeginResultA5            mFetchBeginResult;
    cmpArgDBFetchResultA5                 mFetchResult;
    cmpArgDBFetchListResultA5             mFetchListResult;
    cmpArgDBFetchEndResultA5              mFetchEndResult;
    cmpArgDBFreeA5                        mFree;
    cmpArgDBCancelA5                      mCancel;
    cmpArgDBTransactionA5                 mTransaction;
    cmpArgDBLobGetSizeA5                  mLobGetSize;
    cmpArgDBLobGetSizeResultA5            mLobGetSizeResult;
    cmpArgDBLobGetA5                      mLobGet;
    cmpArgDBLobGetResultA5                mLobGetResult;
    cmpArgDBLobPutBeginA5                 mLobPutBegin;
    cmpArgDBLobPutA5                      mLobPut;
    cmpArgDBLobPutEndA5                   mLobPutEnd;
    cmpArgDBLobFreeA5                     mLobFree;
    cmpArgDBLobFreeAllA5                  mLobFreeAll;
    cmpArgDBXaOperationA5                 mXaOperation;
    cmpArgDBXaXidA5                       mXaXid;
    cmpArgDBXaResultA5                    mXaResult;
    cmpArgDBXaTransactionA5               mXaTransaction;
    cmpArgDBLobGetBytePosCharLenA5        mLobGetBytePosCharLen;
    cmpArgDBLobGetBytePosCharLenResultA5  mLobGetBytePosCharLenResult;
    cmpArgDBLobGetCharPosCharLenA5        mLobGetCharPosCharLen;
    cmpArgDBLobBytePosA5                  mLobBytePos;
    cmpArgDBLobBytePosResultA5            mLobBytePosResult;
    cmpArgDBLobCharLengthA5               mLobCharLength;
    cmpArgDBLobCharLengthResultA5         mLobCharLengthResult;
} cmpArgDB;

#endif
