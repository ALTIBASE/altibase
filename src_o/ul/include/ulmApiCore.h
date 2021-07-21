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

#ifndef _ULM_API_CORE_H_
#define _ULM_API_CORE_H_

#include <sqlcli.h>
#include <aciErrorMgr.h>
#include <ulmApi.h>

#define ULM_SIZE_64     64
#define ULM_SIZE_1024   1024
#define ULM_SIZE_2048   2048
#define ULM_SIZE_4096   4096
#define ULM_EXTENT_SIZE 20

#define ULM_SET_ERR_CODE( aErrCode );           \
        if( aciGetSystemErrno() == EPIPE )      \
        {                                       \
            gErrCode = ULM_ERR_RECEIVE_SIGPIPE; \
            errno = 0;                          \
        }                                       \
        else                                    \
        {                                       \
            gErrCode = aErrCode;                \
        }


typedef enum
{
    ULM_FLAG_FREE = 0,
    ULM_FLAG_SET
} ulmFlag;

typedef enum
{
    ULM_V_SESSION,
    ULM_V_SESSION_EXECUTING_ONLY,
    ULM_V_SESSION_BY_SID,
    ULM_V_SYSSTAT,
    ULM_V_SESSTAT,
    ULM_V_SESSTAT_EXECUTING_ONLY, // BUG-34728, 새로 추가
    ULM_V_SESSTAT_BY_SID,
    ULM_STAT_NAME,
    ULM_V_SYSTEM_EVENT,
    ULM_V_SESSION_EVENT,
    ULM_V_SESSION_EVENT_BY_SID,
    ULM_EVENT_NAME,
    ULM_V_SESSION_WAIT,
    ULM_V_SESSION_WAIT_BY_SID,
    ULM_SQL_TEXT,
    ULM_SQL_TEXT_BY_STMT_ID,      // BUG-34728, 새로 추가
    ULM_LOCK_PAIR_BETWEEN_SESSIONS,
    ULM_DB_INFO,
    ULM_READ_COUNT,
    ULM_SESSION_COUNT,
    ULM_SESSION_COUNT_EXECUTING_ONLY,
    ULM_MAX_CLIENT_COUNT,
    ULM_LOCK_WAIT_SESSION_COUNT,
    ULM_REP_GAP,                  // BUG-40397
    ULM_REP_SENT_LOG_COUNT,       // BUG-40397
    ULM_MAX_QUERY_COUNT
} ulmQueryType;

typedef enum
{
    ULM_INIT_ERR_CODE = -21,
    ULM_ERR_RECEIVE_SIGPIPE,
    ULM_ERR_SET_TOO_LONG_PROPERTY_VALUE,
    /* Begin ABIInitialize */
    ULM_ERR_OPEN_LOGFILE,
    ULM_ERR_ALLOC_HANDLE_ENV,
    ULM_ERR_ALLOC_HANDLE_DBC,
    ULM_ERR_CONNECT_DB,
    /* End */
    /* Begin ulmPrepare */
    ULM_ERR_ALLOC_HANDLE_STMT,
    ULM_ERR_SET_STMT_ATTR,
    ULM_ERR_PREPARE_STMT,
    ULM_ERR_ALLOC_MEM,
    /* End */
    ULM_ERR_BIND_PARAMETER,
    /* Begin ulmExecute */
    ULM_ERR_EXECUTE_STMT,
    ULM_ERR_BIND_COLUMN,
    ULM_ERR_FETCH_RESULT,
    ULM_ERR_CLOSE_CURSOR,
    /* End */
    /* Begin ABIFinalize */
    ULM_ERR_FREE_HANDLE_STMT,
    ULM_ERR_DISCONNECT_DB,
    ULM_ERR_FREE_HANDLE_DBC,
    ULM_ERR_FREE_HANDLE_ENV,
    ULM_ERR_CLOSE_LOGFILE
    /* End */
} ulmErrCode;

typedef struct
{
    acp_sint32_t  mID;
    acp_sint64_t  mTransID;
    acp_char_t    mTaskState[11+1];
    acp_char_t    mCommName[64+1];
    acp_sint32_t  mXASessionFlag;
    acp_sint32_t  mXAAssociateFlag;
    acp_sint32_t  mQueryTimeLimit;
    acp_sint32_t  mDdlTimeLimit;
    acp_sint32_t  mFetchTimeLimit;
    acp_sint32_t  mUTransTimeLimit;
    acp_sint32_t  mIdleTimeLimit;
    acp_sint32_t  mIdleStartTime;
    acp_sint32_t  mActiveFlag;
    acp_sint32_t  mOpenedStmtCount;
    acp_char_t    mClientPackageVersion[40+1];
    acp_char_t    mClientProtocolVersion[40+1];
    acp_sint64_t  mClientPID;
    acp_char_t    mClientType[40+1];
    acp_char_t    mClientAppInfo[128+1];
    acp_char_t    mClientNls[40+1];
    acp_char_t    mDBUserName[40+1];
    acp_sint32_t  mDBUserID;
    acp_sint64_t  mDefaultTbsID;
    acp_sint64_t  mDefaultTempTbsID;
    acp_sint32_t  mSysDbaFlag;
    acp_sint32_t  mAutoCommitFlag;
    acp_char_t    mSessionState[13+1];
    acp_sint32_t  mIsolationLevel;
    acp_sint32_t  mReplicationMode;
    acp_sint32_t  mTransactionMode;
    acp_sint32_t  mCommitWriteWaitMode;
    acp_sint32_t  mOptimizerMode;
    acp_sint32_t  mHeaderDisplayMode;
    acp_sint32_t  mCurrentStmtID;
    acp_sint32_t  mStackSize;
    acp_char_t    mDefaultDateFormat[64+1];
    acp_sint64_t  mTrxUpdateMaxLogSize;
    acp_sint32_t  mParallelDmlMode;
    acp_sint32_t  mLoginTime;
    acp_char_t    mFailoverSource[255+1];
    SQLLEN        mLengthOrInd[40];
} ulmVSession;

typedef struct
{
    acp_sint64_t  mValue;
    SQLLEN        mLengthOrInd;
} ulmVSysstat;

typedef struct
{
    acp_sint32_t  mSID;
    acp_sint64_t  mValue;
    SQLLEN        mLengthOrInd[2];
} ulmVSesstat;

typedef struct
{
    acp_sint32_t  mSeqNum;
    acp_char_t    mName[128+1];
    SQLLEN        mLengthOrInd[2];
} ulmStatName;

typedef struct
{
    acp_sint64_t  mTotalWaits;
    acp_sint64_t  mTotalTimeOuts;
    acp_sint64_t  mTimeWaited;
    acp_sint64_t  mAverageWait;
    acp_sint64_t  mTimeWaitedMicro;
    SQLLEN        mLengthOrInd[5];
} ulmVSystemEvent;

typedef struct
{
    acp_sint32_t  mSID;
    acp_sint64_t  mTotalWaits;
    acp_sint64_t  mTotalTimeOuts;
    acp_sint64_t  mTimeWaited;
    acp_sint64_t  mAverageWait;
    acp_sint64_t  mMaxWait;
    acp_sint64_t  mTimeWaitedMicro;
    SQLLEN        mLengthOrInd[7];
} ulmVSessionEvent;

typedef struct
{
    acp_sint32_t  mEventID;
    acp_char_t    mEvent[128+1];
    acp_sint32_t  mWaitClassID;
    acp_char_t    mWaitClass[128+1];
    SQLLEN        mLengthOrInd[4];
} ulmEventName;

typedef struct
{
    acp_sint32_t  mSID;
    acp_sint32_t  mSeqNum;
    acp_sint64_t  mP1;
    acp_sint64_t  mP2;
    acp_sint64_t  mP3;
    acp_sint32_t  mWaitClassID;
    acp_sint64_t  mWaitTime;
    acp_sint64_t  mSecondInTime;
    SQLLEN        mLengthOrInd[8];
} ulmVSessionWait;

typedef struct
{
    acp_sint32_t  mSessID;           // BUG-34728, 새로 추가
    acp_sint32_t  mStmtID;           // BUG-34728, 새로 추가
    // BUG-34728: cpu is too high
    // v$sqltext 조회에서  x$statement query 컬럼 조회로 변경
    // 이유: 구현이 쉬워짐
    acp_char_t    mSqlText[16384+1];
    SQLLEN        mSqlTextLength;    /* BUG-41825 */
    acp_sint32_t  mQueryStartTime;
    acp_sint32_t  mExecuteFlag;
    acp_char_t    mSqlCacheTextID[64+1];  /* BUG-45924 */
    SQLLEN        mLengthOrInd[1];
    /* BUG-46436 */
    acp_sint64_t  mParseTime;
    acp_sint64_t  mSoftPrepareTime;
    acp_sint32_t  mLastQueryStartTime;
    acp_sint64_t  mExecuteTime;
    acp_sint64_t  mFetchTime;
    acp_sint32_t  mFetchStartTime;
    acp_sint64_t  mTotalTime;
    acp_sint64_t  mValidateTime;
    acp_sint64_t  mOptimizeTime;
} ulmSqlText;

typedef struct
{
    acp_sint32_t  mWaiterSID;
    acp_sint32_t  mHolderSID;
    acp_char_t    mLockDesc[32+1];
    SQLLEN        mLengthOrInd[3];
} ulmLockPair;

typedef struct
{
    acp_char_t    mDBName[128+1];
    acp_char_t    mDBVersion[128+1];
    SQLLEN        mLengthOrInd[2];
} ulmDBInfo;

typedef struct
{
    acp_sint32_t  mLogicalReadCount;
    acp_sint32_t  mPhysicalReadCount;
    SQLLEN        mLengthOrInd[2];
} ulmReadCount;

typedef struct
{
    SQLHANDLE     mHStmt;
    void         *mBindArray;
    void         *mResultArray;
    acp_uint32_t  mResultArraySize;
    acp_char_t   *mQuery;
    acp_rc_t     (*mBindCol)( void );
    void         (*mCopyBindToResult)( acp_uint32_t );
} ulmResourceManager;

// BUG-33946 The maxgauge library should provide the API that sets the user and the password.
typedef struct
{
    acp_char_t    mPropValue[ULM_SIZE_2048];
} ulmProperties;

/* BUG-40397 The API for replication should be implemented at altiMonitor */
typedef struct
{
    acp_char_t    mRepName[40+1];
    acp_sint64_t  mRepGap;
    SQLLEN        mLengthOrInd[2];
} ulmRepGap;

typedef struct
{
    acp_char_t    mRepName[40+1];
    acp_char_t    mTableName[128+1];
    acp_sint32_t  mInsertLogCount;
    acp_sint32_t  mDeleteLogCount;
    acp_sint32_t  mUpdateLogCount;
    SQLLEN        mLengthOrInd[6];
} ulmRepSentLogCount;


acp_rc_t ulmPrepare( acp_size_t aBindRowSize, acp_size_t aResultRowSize, acp_uint32_t aCount );
acp_rc_t ulmExecute( acp_size_t aResultRowSize, acp_uint32_t *aRowCount );

acp_rc_t ulmBindColOfVSession( void );
acp_rc_t ulmBindColOfVSysstat( void );
acp_rc_t ulmBindColOfVSesstat( void );
acp_rc_t ulmBindColOfStatName( void );
acp_rc_t ulmBindColOfVSystemEvent( void );
acp_rc_t ulmBindColOfVSessionEvent( void );
acp_rc_t ulmBindColOfEventName( void );
acp_rc_t ulmBindColOfVSessionWait( void );
acp_rc_t ulmBindColOfSqlText( void );
acp_rc_t ulmBindColOfLockPair( void );
acp_rc_t ulmBindColOfDBInfo( void );
acp_rc_t ulmBindColOfReadCount( void );
/* BUG-40397 The API for replication should be implemented at altiMonitor */
acp_rc_t ulmBindColOfRepGap( void );
acp_rc_t ulmBindColOfRepSentLogCount( void );

void     ulmCopyOfVSession( acp_uint32_t aPos );
void     ulmCopyOfVSysstat( acp_uint32_t aPos );
void     ulmCopyOfVSesstat( acp_uint32_t aPos );
void     ulmCopyOfStatName( acp_uint32_t aPos );
void     ulmCopyOfVSystemEvent( acp_uint32_t aPos );
void     ulmCopyOfVSessionEvent( acp_uint32_t aPos );
void     ulmCopyOfEventName( acp_uint32_t aPos );
void     ulmCopyOfVSessionWait( acp_uint32_t aPos );
void     ulmCopyOfSqlText( acp_uint32_t aPos );
void     ulmCopyOfLockPair( acp_uint32_t aPos );
void     ulmCopyOfDBInfo( acp_uint32_t aPos );
void     ulmCopyOfReadCount( acp_uint32_t aPos );
/* BUG-40397 The API for replication should be implemented at altiMonitor */
void     ulmCopyOfRepGap( acp_uint32_t aPos );
void     ulmCopyOfRepSentLogCount( acp_uint32_t aPos );

acp_rc_t ulmCleanUpResource( void );

void     ulmRecordErrorLog( const acp_char_t *aFunctionName );

#endif /* _ULM_API_CORE_H_ */
