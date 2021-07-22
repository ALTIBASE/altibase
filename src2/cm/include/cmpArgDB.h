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

#ifndef _O_CMP_ARG_DB_H_
#define _O_CMP_ARG_DB_H_ 1

/**********************************************************
 * proj_2160 cm_type removal
 * structures below are used only for ALTIBASE 5
 * naming rule: cmpArgDBxxxxxA5
 **********************************************************/

typedef struct cmpArgDBMessageA5
{
    cmtVariable mMessage;
} cmpArgDBMessageA5;

typedef struct cmpArgDBErrorResultA5
{
    UChar       mOperationID;
    UShort      mErrorIndex;
    UInt        mErrorCode;
    cmtVariable mErrorMessage;
} cmpArgDBErrorResultA5;

typedef struct cmpArgDBErrorInfoA5
{
    UInt        mErrorCode;
} cmpArgDBErrorInfoA5;

typedef struct cmpArgDBErrorInfoResultA5
{
    UInt        mErrorCode;
    cmtVariable mErrorMessage;
} cmpArgDBErrorInfoResultA5;

typedef struct cmpArgDBConnectA5
{
    cmtVariable mDatabaseName;
    cmtVariable mUserName;
    cmtVariable mPassword;
    UShort      mMode;
} cmpArgDBConnectA5;

typedef struct cmpArgDBConnectResultA5
{
} cmpArgDBConnectResultA5;

typedef struct cmpArgDBDisconnectA5
{
    UChar       mOption;
} cmpArgDBDisconnectA5;

typedef struct cmpArgDBDisconnectResultA5
{
} cmpArgDBDisconnectResultA5;

typedef struct cmpArgDBPropertyGetA5
{
    UShort      mPropertyID;
} cmpArgDBPropertyGetA5;

typedef struct cmpArgDBPropertyGetResultA5
{
    UShort      mPropertyID;
    cmtAny      mValue;
} cmpArgDBPropertyGetResultA5;

typedef struct cmpArgDBPropertySetA5
{
    UShort      mPropertyID;
    cmtAny      mValue;
} cmpArgDBPropertySetA5;

typedef struct cmpArgDBPropertySetResultA5
{
} cmpArgDBPropertySetResultA5;

typedef struct cmpArgDBPrepareA5
{
    UInt        mStatementID;
    cmtVariable mStatementString;
    UChar       mMode;
} cmpArgDBPrepareA5;

typedef struct cmpArgDBPrepareResultA5
{
    UInt        mStatementID;
    UInt        mStatementType;
    UShort      mParamCount;
    UShort      mResultSetCount;
    // bug-25109 support ColAttribute baseTableName
    cmtVariable mBaseTableOwnerName;
    cmtVariable mBaseTableName;
    UShort      mBaseTableUpdatable;
    cmtVariable mTableName;   // unused
} cmpArgDBPrepareResultA5;

typedef struct cmpArgDBPlanGetA5
{
    UInt        mStatementID;
} cmpArgDBPlanGetA5;

typedef struct cmpArgDBPlanGetResultA5
{
    UInt        mStatementID;
    cmtVariable mPlan;
} cmpArgDBPlanGetResultA5;

typedef struct cmpArgDBColumnInfoGetA5
{
    UInt        mStatementID;
    UShort      mResultSetID;
    UShort      mColumnNumber;
} cmpArgDBColumnInfoGetA5;

typedef struct cmpArgDBColumnInfoGetResultA5
{
    UInt        mStatementID;
    UShort      mResultSetID;
    UShort      mColumnNumber;
    UInt        mDataType;
    UInt        mLanguage;
    UChar       mArguments;
    SInt        mPrecision;
    SInt        mScale;
    UChar       mNullableFlag;
    cmtVariable mDisplayName;
} cmpArgDBColumnInfoGetResultA5;

typedef struct cmpCollectionDBColumnInfoGetResultA5
{
    UInt          mDataType;
    UInt          mLanguage;
    UChar         mArguments;
    SInt          mPrecision;
    SInt          mScale;
    UChar         mNullableFlag;
    cmtAny        mDisplayName;
} cmpCollectionDBColumnInfoGetResultA5;

typedef struct cmpArgDBColumnInfoGetListResultA5
{
    UInt          mStatementID;
    UShort        mResultSetID;
    UShort        mColumnCount;
    cmtCollection mListData;
} cmpArgDBColumnInfoGetListResultA5;

typedef struct cmpArgDBColumnInfoSetA5
{
    UInt        mStatementID;
    UShort      mResultSetID;
    UShort      mColumnNumber;
    UInt        mDataType;
    UInt        mLanguage;
    UChar       mArguments;
    SInt        mPrecision;
    SInt        mScale;
} cmpArgDBColumnInfoSetA5;

typedef struct cmpArgDBColumnInfoSetResultA5
{
} cmpArgDBColumnInfoSetResultA5;

typedef struct cmpArgDBParamInfoGetA5
{
    UInt        mStatementID;
    UShort      mParamNumber;
} cmpArgDBParamInfoGetA5;

typedef struct cmpArgDBParamInfoGetResultA5
{
    UInt        mStatementID;
    UShort      mParamNumber;
    UInt        mDataType;
    UInt        mLanguage;
    UChar       mArguments;
    SInt        mPrecision;
    SInt        mScale;
    UChar       mInOutType;
    UChar       mNullableFlag;
} cmpArgDBParamInfoGetResultA5;

typedef struct cmpArgDBParamInfoSetA5
{
    UInt        mStatementID;
    UShort      mParamNumber;
    UInt        mDataType;
    UInt        mLanguage;
    UChar       mArguments;
    SInt        mPrecision;
    SInt        mScale;
    UChar       mInOutType;
} cmpArgDBParamInfoSetA5;

typedef struct cmpArgDBParamInfoSetResultA5
{
} cmpArgDBParamInfoSetResultA5;

typedef struct cmpArgDBParamInfoSetListA5
{
    UInt          mStatementID;
    UShort        mParamCount;
    cmtCollection mListData;
} cmpArgDBParamInfoSetListA5;

typedef struct cmpCollectionDBParamInfoSetA5
{
    UShort      mParamNumber;
    UInt        mDataType;
    UInt        mLanguage;
    UChar       mArguments;
    SInt        mPrecision;
    SInt        mScale;
    UChar       mInOutType;
} cmpCollectionDBParamInfoSetA5;

typedef struct cmpArgDBParamInfoSetListResultA5
{
} cmpArgDBParamInfoSetListResultA5;

typedef struct cmpArgDBParamDataInA5
{
    UInt        mStatementID;
    UShort      mParamNumber;
    cmtAny      mData;
} cmpArgDBParamDataInA5;

// bug-28259: ipc needs paramDataInResult
typedef struct cmpArgDBParamDataInResultA5
{
} cmpArgDBParamDataInResultA5;

typedef struct cmpArgDBParamDataOutA5
{
    UInt        mStatementID;
    UShort      mRowNumber;
    UShort      mParamNumber;
    cmtAny      mData;
} cmpArgDBParamDataOutA5;

typedef struct cmpArgDBParamDataOutListA5
{
    UInt          mStatementID;
    UShort        mRowNumber;
    cmtCollection mListData;
} cmpArgDBParamDataOutListA5;

typedef struct cmpArgDBParamDataInListA5
{
    UInt          mStatementID;
    UShort        mFromRowNumber;
    UShort        mToRowNumber;
    UChar         mExecuteOption;
    cmtCollection mListData;
} cmpArgDBParamDataInListA5;

typedef struct cmpArgDBParamDataInListResultA5
{
    UInt        mStatementID;
    UShort      mFromRowNumber;
    UShort      mToRowNumber;
    UShort      mResultSetCount;
    ULong       mAffectedRowCount;
} cmpArgDBParamDataInListResultA5;

typedef struct cmpArgDBExecuteA5
{
    UInt        mStatementID;
    UShort      mRowNumber;
    UChar       mOption;
} cmpArgDBExecuteA5;

typedef struct cmpArgDBExecuteResultA5
{
    UInt        mStatementID;
    UShort      mRowNumber;
    UShort      mResultSetCount;
    ULong       mAffectedRowCount;
} cmpArgDBExecuteResultA5;

typedef struct cmpArgDBFetchMoveA5
{
    UInt        mStatementID;
    UShort      mResultSetID;
    UChar       mWhence;
    SLong       mOffset;
} cmpArgDBFetchMoveA5;

typedef struct cmpArgDBFetchMoveResultA5
{
} cmpArgDBFetchMoveResultA5;

typedef struct cmpArgDBFetchA5
{
    UInt        mStatementID;
    UShort      mResultSetID;
    UShort      mRecordCount;
    UShort      mColumnFrom;
    UShort      mColumnTo;
} cmpArgDBFetchA5;

typedef struct cmpArgDBFetchBeginResultA5
{
    UInt        mStatementID;
    UShort      mResultSetID;
} cmpArgDBFetchBeginResultA5;

typedef struct cmpArgDBFetchResultA5
{
    cmtAny      mData;
} cmpArgDBFetchResultA5;

typedef struct cmpArgDBFetchListResultA5
{
    UShort        mRecordCount;
    UInt          mRecordSize;
    cmtCollection mListData;
} cmpArgDBFetchListResultA5;

typedef struct cmpArgDBFetchEndResultA5
{
    UInt        mStatementID;
    UShort      mResultSetID;
} cmpArgDBFetchEndResultA5;

typedef struct cmpArgDBFreeA5
{
    UInt        mStatementID;
    UShort      mResultSetID;
    UChar       mMode;
} cmpArgDBFreeA5;

typedef struct cmpArgDBFreeResultA5
{
} cmpArgDBFreeResultA5;

typedef struct cmpArgDBCancelA5
{
    UInt        mStatementID;
} cmpArgDBCancelA5;

typedef struct cmpArgDBCancelResultA5
{
} cmpArgDBCancelResultA5;

typedef struct cmpArgDBTransactionA5
{
    UChar       mOperation;
} cmpArgDBTransactionA5;

typedef struct cmpArgDBTransactionResultA5
{
} cmpArgDBTransactionResultA5;

typedef struct cmpArgDBLobGetSizeA5
{
    ULong       mLocatorID;
} cmpArgDBLobGetSizeA5;

typedef struct cmpArgDBLobGetSizeResultA5
{
    ULong       mLocatorID;
    UInt        mSize;
} cmpArgDBLobGetSizeResultA5;

typedef struct cmpArgDBLobGetA5
{
    ULong       mLocatorID;
    UInt        mOffset;
    UInt        mSize;
} cmpArgDBLobGetA5;

typedef struct cmpArgDBLobGetBytePosCharLenA5
{
    ULong       mLocatorID;
    UInt        mOffset;
    UInt        mSize;
} cmpArgDBLobGetBytePosCharLenA5;

typedef struct cmpArgDBLobGetCharPosCharLenA5
{
    ULong       mLocatorID;
    UInt        mOffset;
    UInt        mSize;
} cmpArgDBLobGetCharPosCharLenA5;

typedef struct cmpArgDBLobBytePosA5
{
    ULong       mLocatorID;
    UInt        mCharOffset;
} cmpArgDBLobBytePosA5;

typedef struct cmpArgDBLobGetResultA5
{
    ULong       mLocatorID;
    UInt        mOffset;
    cmtVariable mData;
} cmpArgDBLobGetResultA5;

typedef struct cmpArgDBLobGetBytePosCharLenResultA5
{
    ULong       mLocatorID;
    UInt        mOffset;
    UInt        mCharLength;
    cmtVariable mData;
} cmpArgDBLobGetBytePosCharLenResultA5;

typedef struct cmpArgDBLobBytePosResultA5
{
    ULong       mLocatorID;
    UInt        mByteOffset;
} cmpArgDBLobBytePosResultA5;

typedef struct cmpArgDBLobCharLengthA5
{
    ULong       mLocatorID;
} cmpArgDBLobCharLengthA5;

typedef struct cmpArgDBLobCharLengthResultA5
{
    ULong       mLocatorID;
    UInt        mLength;
} cmpArgDBLobCharLengthResultA5;

/* PROJ-2047 Strengthening LOB - Removed mOldSize */
typedef struct cmpArgDBLobPutBeginA5
{
    ULong       mLocatorID;
    UInt        mOffset;
    UInt        mOldSize; /* Not Used */
    UInt        mNewSize;
} cmpArgDBLobPutBeginA5;

typedef struct cmpArgDBLobPutBeginResultA5
{
} cmpArgDBLobPutBeginReseultA5;

/* PROJ-2047 Strengthening LOB - Removed mOffset */
typedef struct cmpArgDBLobPutA5
{
    ULong       mLocatorID;
    UInt        mOffset; /* Not Used */
    cmtVariable mData;
} cmpArgDBLobPutA5;

typedef struct cmpArgDBLobPutEndA5
{
    ULong       mLocatorID;
} cmpArgDBLobPutEndA5;

typedef struct cmpArgDBLobPutEndResultA5
{
} cmpArgDBLobPutEndResultA5;

typedef struct cmpArgDBLobFreeA5
{
    ULong       mLocatorID;
} cmpArgDBLobFreeA5;

typedef struct cmpArgDBLobFreeResultA5
{
} cmpArgDBLobFreeResultA5;

typedef struct cmpArgDBLobFreeAllA5
{
    ULong          mLocatorCount;
    cmtCollection  mListData;
} cmpArgDBLobFreeAllA5;

typedef struct cmpArgDBLobFreeAllResultA5
{
} cmpArgDBLobFreeAllResultA5;

/* PROJ-1573 XA */
typedef struct cmpArgDBXaOperationA5
{
    UChar       mOperation;
    SInt        mRmID;
    SLong       mFlag;
    SLong       mArgument;
} cmpArgDBXaOperationA5;

typedef struct cmpArgDBXaXidA5
{
    SLong       mFormatID;
    SLong       mGTRIDLength;
    SLong       mBQUALLength;
    cmtVariable mData;
} cmpArgDBXaXidA5;

typedef struct cmpArgDBXaResultA5
{
    UChar       mOperation;
    SInt        mReturnValue;
} cmpArgDBXaResultA5;

typedef struct cmpArgDBXaTransactionA5
{
    UChar       mOperation;
    SInt        mRmID;
    SLong       mFlag;
    SLong       mArgument;
    SLong       mFormatID;
    SLong       mGTRIDLength;
    SLong       mBQUALLength;
    cmtVariable mData;
} cmpArgDBXaTransactionA5;

/* PROJ-2177 User Interface - Cancel */

typedef struct cmpArgDBPrepareByCID
{
    UInt        mStmtCID;
    cmtVariable mStatementString;
    UChar       mMode;
} cmpArgDBPrepareByCID;

typedef struct cmpArgDBCancelByCID
{
    UInt        mStmtCID;
} cmpArgDBCancelByCID;

typedef union cmpArgDB
{
    cmpArgDBMessageA5                     mMessage;
    cmpArgDBErrorResultA5                 mErrorResult;
    cmpArgDBErrorInfoA5                   mErrorInfo;
    cmpArgDBErrorInfoResultA5             mErrorInfoResult;
    cmpArgDBConnectA5                     mConnect;
    cmpArgDBConnectResultA5               mConnectResult;
    cmpArgDBDisconnectA5                  mDisconnect;
    cmpArgDBDisconnectResultA5            mDisconnectResult;
    cmpArgDBPropertyGetA5                 mPropertyGet;
    cmpArgDBPropertyGetResultA5           mPropertyGetResult;
    cmpArgDBPropertySetA5                 mPropertySet;
    cmpArgDBPropertySetResultA5           mPropertySetResult;
    cmpArgDBPrepareA5                     mPrepare;
    cmpArgDBPrepareResultA5               mPrepareResult;
    cmpArgDBPlanGetA5                     mPlanGet;
    cmpArgDBPlanGetResultA5               mPlanGetResult;
    cmpArgDBColumnInfoGetA5               mColumnInfoGet;
    cmpArgDBColumnInfoGetResultA5         mColumnInfoGetResult;
    cmpArgDBColumnInfoGetListResultA5     mColumnInfoGetListResult;
    cmpArgDBColumnInfoSetA5               mColumnInfoSet;
    cmpArgDBColumnInfoSetResultA5         mColumnInfoSetResult;
    cmpArgDBParamInfoGetA5                mParamInfoGet;
    cmpArgDBParamInfoGetResultA5          mParamInfoGetResult;
    cmpArgDBParamInfoSetA5                mParamInfoSet;
    cmpArgDBParamInfoSetResultA5          mParamInfoSetResult;
    cmpArgDBParamInfoSetListA5            mParamInfoSetList;
    cmpArgDBParamInfoSetListResultA5      mParamInfoSetListResult;
    cmpArgDBParamDataInA5                 mParamDataIn;
    cmpArgDBParamDataOutA5                mParamDataOut;
    cmpArgDBParamDataOutListA5            mParamDataOutList;
    cmpArgDBParamDataInListA5             mParamDataInList;
    cmpArgDBParamDataInListResultA5       mParamDataInListResult;
    cmpArgDBExecuteA5                     mExecute;
    cmpArgDBExecuteResultA5               mExecuteResult;
    cmpArgDBFetchMoveA5                   mFetchMove;
    cmpArgDBFetchMoveResultA5             mFetchMoveResult;
    cmpArgDBFetchA5                       mFetch;
    cmpArgDBFetchBeginResultA5            mFetchBeginResult;
    cmpArgDBFetchResultA5                 mFetchResult;
    cmpArgDBFetchListResultA5             mFetchListResult;
    cmpArgDBFetchEndResultA5              mFetchEndResult;
    cmpArgDBFreeA5                        mFree;
    cmpArgDBFreeResultA5                  mFreeResult;
    cmpArgDBCancelA5                      mCancel;
    cmpArgDBCancelResultA5                mCancelResult;
    cmpArgDBTransactionA5                 mTransaction;
    cmpArgDBTransactionResultA5           mTransactionResult;
    cmpArgDBLobGetSizeA5                  mLobGetSize;
    cmpArgDBLobGetSizeResultA5            mLobGetSizeResult;
    cmpArgDBLobGetA5                      mLobGet;
    cmpArgDBLobGetResultA5                mLobGetResult;
    cmpArgDBLobPutBeginA5                 mLobPutBegin;
    cmpArgDBLobPutBeginResultA5           mLobPutBeginResult;
    cmpArgDBLobPutA5                      mLobPut;
    cmpArgDBLobPutEndA5                   mLobPutEnd;
    cmpArgDBLobPutEndResultA5             mLobPutEndResult;
    cmpArgDBLobFreeA5                     mLobFree;
    cmpArgDBLobFreeResultA5               mLobFreeResult;
    cmpArgDBLobFreeAllA5                  mLobFreeAll;
    cmpArgDBLobFreeAllResultA5            mLobFreeAllResult;
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
    cmpArgDBParamDataInResultA5           mParamDataInResult; // bug-28259
} cmpArgDB;

#endif
