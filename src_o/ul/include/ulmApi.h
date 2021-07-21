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

#ifndef _ALTIBASE_MONITOR_H_
#define _ALTIBASE_MONITOR_H_

/*******************************
 *     API type definition     *
 *******************************/

/* V$SESSION - This view displays internally generated information about client sessions. */
typedef struct
{
    int       mID;                          /* The session identifier */
    long long mTransID;                     /* The identifier of the transaction currently being executed in the session */
    char      mTaskState[11+1];             /* The task status */
    char      mCommName[64+1];              /* Connection information */
    int       mXASessionFlag;               /* The XA session flag */
    int       mXAAssociateFlag;             /* The XA associate flag */
    int       mQueryTimeLimit;              /* This is the query timeout value for the current session. */
    int       mDdlTimeLimit;                /* This is the DDL timeout value for the current session. */
    int       mFetchTimeLimit;              /* This is the fetch timeout value for the current session. */
    int       mUTransTimeLimit;             /* This is the update transaction timeout value for the current session. */
    int       mIdleTimeLimit;               /* This is the idle timeout value for the current session. */
    int       mIdleStartTime;               /* This shows the time at which the session entered an Idle state. */
    int       mActiveFlag;                  /* The active transaction flag */
    int       mOpenedStmtCount;             /* The number of opened statements */
    char      mClientPackageVersion[40+1];  /* The client package version */
    char      mClientProtocolVersion[40+1]; /* The client protocol version */
    long long mClientPID;                   /* The client process identifier */
    char      mClientType[40+1];            /* The type of the client */
    char      mClientAppInfo[128+1];        /* The type of the client application */
    char      mClientNls[40+1];             /* The client character set */
    char      mDBUserName[40+1];            /* The database user name */
    int       mDBUserID;                    /* The database user identifier */
    long long mDefaultTbsID;                /* The user¡¯s default tablespace identifier */
    long long mDefaultTempTbsID;            /* The user¡¯s default temporary tablespaceidentifier */
    int       mSysDbaFlag;                  /* Indicates whether the connection was made as sysdba */
    int       mAutoCommitFlag;              /* The autocommit flag */
    char      mSessionState[13+1];          /* The status of the session */
    int       mIsolationLevel;              /* The isolation level */
    int       mReplicationMode;             /* The replication mode */
    int       mTransactionMode;             /* The transaction mode */
    int       mCommitWriteWaitMode;
    int       mOptimizerMode;               /* The optimization mode */
    int       mHeaderDisplayMode;           /*
                                               Indicates whether only the column names are output,
                                               or whether the table names are output along with the column names
                                               when the results of a SELECT query are output.
                                                   
                                               0 - The table names are displayed along with the column names.
                                               1 - Only the column names are output.
                                            */
    int       mCurrentStmtID;               /* The identifier of the current statement */
    int       mStackSize;                   /* The size of the stack */
    char      mDefaultDateFormat[64+1];     /*
                                               The default date format
                                               e.g.) DD-MON-RRRR
                                            */
    long long mTrxUpdateMaxLogSize;         /* The maximum size of the DML Log */
    int       mParallelDmlMode;             /* Whether to execute DML statements in parallel */
    int       mLoginTime;                   /* The amount of time the client has been logged in */
    char      mFailoverSource[255+1];
} ABIVSession;

/* V$SYSSTAT - This view shows the status of the system. */
typedef struct
{
    long long mValue;                       /* The value of the status */
} ABIVSysstat;

/* V$SESSTAT - This view shows statistics for all currently connected sessions. */
typedef struct
{
    int       mSID;                         /* The identifier of the session. */
    long long mValue;                       /* The value returned for the statistic */
} ABIVSesstat;

typedef struct
{
    int       mSeqNum;                      /* The serial number of the status */
    char      mName[128+1];                 /* The name of the status */
} ABIStatName;

/* V$SYSTEM_EVENT - This view shows cumulative statistical information about waits classified according to wait event. */
typedef struct
{
    long long mTotalWaits;                  /* The total number of waits for this event */
    long long mTotalTimeOuts;               /* The number of failures to gain access to the requested resource within the specified time */
    long long mTimeWaited;                  /* The total time spent waiting for this wait event by all processes (in milliseconds) */
    long long mAverageWait;                 /* The average length of a wait for this event (in milliseconds) */
    long long mTimeWaitedMicro;             /* The total time spent waiting for this wait event by all processes (in microseconds) */
} ABIVSystemEvent;

/* V$SESSION_EVENT - This view shows cumulative statistical wait information about all wait events for each session. */
typedef struct
{
    int       mSID;                         /* The identifier of the session */
    long long mTotalWaits;                  /* The total number of waits related to the wait event */
    long long mTotalTimeOuts;               /* The total number of times that a resource could not be accessed after the specified time */
    long long mTimeWaited;                  /* The total amount of time spent waiting for the wait event (in milliseconds) */
    long long mAverageWait;                 /* The average amount of time spent waiting for the wait event (in milliseconds) */
    long long mMaxWait;                     /* The maximum time spent waiting for the wait event (in milliseconds) */
    long long mTimeWaitedMicro;             /* The total amount of time spent waiting for the wait event (in microseconds) */
} ABIVSessionEvent;

typedef struct
{
    int       mEventID;                     /* The identifier of the wait event */
    char      mEvent[128+1];                /* The name of the wait event */
    int       mWaitClassID;                 /* The identifier of the wait class */
    char      mWaitClass[128+1];            /* The name of the wait class */
} ABIEventName;

/* V$SESSION_WAIT - This view displays information about wait events for all currently connected sessions. */
typedef struct
{
    int       mSID;                         /* The identifier of the session */
    int       mSeqNum;                      /* The identifier of the wait event */
    long long mP1;                          /* Parameter 1 of the wait event */
    long long mP2;                          /* Parameter 2 of the wait event */
    long long mP3;                          /* Parameter 3 of the wait event */
    int       mWaitClassID;                 /* The identifier of the wait class */
    long long mWaitTime;                    /* The time spent waiting (in milliseconds) */
    long long mSecondInTime;                /* The time spent waiting (in seconds) */
} ABIVSessionWait;

typedef struct
{
    int       mSessID;
    int       mStmtID;
    char      mSqlText[16384+1];
    int       mTextLength;
    int       mQueryStartTime;
    int       mExecuteFlag;
    char      mSqlCacheTextID[64+1];
    long long mParseTime;
    long long mSoftPrepareTime;
    int       mLastQueryStartTime;
    long long mExecuteTime;
    long long mFetchTime;
    int       mFetchStartTime;
    long long mTotalTime;
    long long mValidateTime;
    long long mOptimizeTime;
} ABISqlText;

typedef struct
{
    int       mWaiterSID;
    int       mHolderSID;
    char      mLockDesc[32+1];
} ABILockPair;

typedef struct
{
    char      mDBName[128+1];
    char      mDBVersion[128+1];
} ABIDBInfo;

typedef struct
{
    int       mLogicalReadCount;
    int       mPhysicalReadCount;
} ABIReadCount;

typedef struct
{
    char      mRepName[40+1];               /* The name of the replication object */
    long long mRepGap;                      /* The interval between the log sequence numbers of REP_LAST_SN and REP_SN  */
} ABIRepGap;

typedef struct
{
    char      mRepName[40+1];               /* The name of the replication object */
    char      mTableName[128+1];
    int       mInsertLogCount;
    int       mDeleteLogCount;
    int       mUpdateLogCount;
} ABIRepSentLogCount;

typedef enum
{
    ABI_USER,
    ABI_PASSWD,
    ABI_LOGFILE
} ABIPropType;


/************************************
 *     API function definition.     *
 ************************************/

int  ABISetProperty( ABIPropType aPropType, const char *aPropValue );

int  ABIInitialize( void );
int  ABIFinalize( void );

/*
   IF the return value is
   -1 - The connection is terminated.
   0  - The connection is still active.
 */
int  ABICheckConnection( void );

/*
   IF the aExecutingOnly value sets to
   0 - Return all sessions.
   1 - Return executing sessions only.
 */
int  ABIGetVSession( ABIVSession **aHandle, unsigned int aExecutingOnly );
int  ABIGetVSessionBySID( ABIVSession **aHandle, int aSessionID );

int  ABIGetVSysstat( ABIVSysstat **aHandle );
int  ABIGetVSesstat( ABIVSesstat **aHandle, unsigned int aExecutingOnly );
int  ABIGetVSesstatBySID( ABIVSesstat **aHandle, int aSessionID );
int  ABIGetStatName( ABIStatName **aHandle );

int  ABIGetVSystemEvent( ABIVSystemEvent **aHandle );
int  ABIGetVSessionEvent( ABIVSessionEvent **aHandle );
int  ABIGetVSessionEventBySID( ABIVSessionEvent **aHandle, int aSessionID );
int  ABIGetEventName( ABIEventName **aHandle );

int  ABIGetVSessionWait( ABIVSessionWait **aHandle );
int  ABIGetVSessionWaitBySID( ABIVSessionWait **aHandle, int aSessionID );

int  ABIGetSqlText( ABISqlText **aHandle, int aStmtID );
int  ABIGetLockPairBetweenSessions( ABILockPair **aHandle );
int  ABIGetDBInfo( ABIDBInfo **aHandle );
int  ABIGetReadCount( ABIReadCount **aHandle );

/*
   IF the aExecutingOnly value sets to
   0 - Return all sessions.
   1 - Return executing sessions only.
 */
int  ABIGetSessionCount( unsigned int aExecutingOnly );
int  ABIGetMaxClientCount( void );
int  ABIGetLockWaitSessionCount( void );

int  ABIGetRepGap( ABIRepGap **aHandle );
int  ABIGetRepSentLogCount( ABIRepSentLogCount **aHandle );

void ABIGetErrorMessage( int aErrCode, const char **aErrMsg );

#endif /* _ALTIBASE_MONITOR_H_ */
