/**
 *  Copyright (c) 1999~2017, Altibase Corp. and/or its affiliates. All rights reserved.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Affero General Public License, version 3,
 *  as published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU Affero General Public License for more details.
 *
 *  You should have received a copy of the GNU Affero General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _O_MMC_SESSION_H_
#define _O_MMC_SESSION_H_ 1

#include <idl.h>
#include <ide.h>
#include <idu.h>
#include <cm.h>
#include <smiTrans.h>
#include <qci.h>
#include <dki.h>
#include <sdi.h>
#include <sdiFailoverTypeStorage.h>
#include <mmErrorCode.h>
#include <mmcConvNumeric.h>
#include <mmcDef.h>
#include <mmcLob.h>
#include <mmcTrans.h>
#include <mmcStatement.h>
#include <mmcStatementManager.h>
#include <mmcMutexPool.h>
#include <mmcEventManager.h>
#include <mmqQueueInfo.h>
#include <mmtSessionManager.h>
#include <mmdDef.h>
#include <mmdXid.h>
#include <mtlTerritory.h>

#define MMC_CLIENTINFO_MAX_LEN              40
#define MMC_APPINFO_MAX_LEN                 128
#define MMC_MODULE_MAX_LEN                  128
#define MMC_ACTION_MAX_LEN                  128
#define MMC_NLSUSE_MAX_LEN                  QC_MAX_NAME_LEN
#define MMC_SSLINFO_MAX_LEN                 256 /* PROJ-2474 */
#define MMC_DATEFORMAT_MAX_LEN              (MTC_TO_CHAR_MAX_PRECISION)
/* 최소한 다음보다는 커야한다: prefix (4) + IDL_IP_ADDR_MAX_LEN (64) +  delim (1) + IDP_MAX_PROP_DBNAME_LEN (127)
 * 바꿀때는 다음도 함께 바꿀 것: (cli) ULN_MAX_FAILOVER_SOURCE_LEN, (jdbc) MAX_FAILOVER_SOURCE_LENGTH */
#define MMC_FAILOVER_SOURCE_MAX_LEN         256
//fix BUG-21311
#define MMC_SESSION_LOB_HASH_BUCKET_CNT     (5)
#define MMC_TIMEZONE_MAX_LEN                (MTC_TIMEZONE_NAME_LEN)
#define MMC_SHARD_PIN_STR_LEN               (SDI_MAX_SHARD_PIN_STR_LEN)
/*
 * 가장 이상적인 collection buffer의 크기는 통신 버퍼 사이즈(32K)와
 * 동일한 경우이다. 그러나 통신 헤더를 고려해야 하기 때문에 30K를
 * 기본값으로 설정한다.
 */
#define MMC_DEFAULT_COLLECTION_BUFFER_SIZE  (30*1024)

// BUG-34725
#define MMC_FETCH_PROTOCOL_TYPE_NORMAL  0
#define MMC_FETCH_PROTOCOL_TYPE_LIST    1

typedef struct mmcGCTxCommitInfo
{
    smSCN           mCoordSCN;
    smSCN           mPrepareSCN;
    smSCN           mGlobalCommitSCN;
    smSCN           mLastSystemSCN;
} mmcGCTxCommitInfo;

typedef struct mmcSessionInfo
{
    mmcSessID        mSessionID;
    /*
     * Runtime Task Info
     */

    mmcTask         *mTask;
    mmcTaskState    *mTaskState;

    /*
     * Client Info
     */

    SChar            mClientPackageVersion[MMC_CLIENTINFO_MAX_LEN + 1];
    SChar            mClientProtocolVersion[MMC_CLIENTINFO_MAX_LEN + 1];
    ULong            mClientPID;
    SChar            mClientType[MMC_CLIENTINFO_MAX_LEN + 1];
    SChar            mClientAppInfo[MMC_APPINFO_MAX_LEN + 1];
    SChar            mDatabaseName[QC_MAX_OBJECT_NAME_LEN + 1];
    /* BUG-41511 supporting to similar DBMS_APPLICATION_INFO */
    SChar            mModuleInfo[MMC_MODULE_MAX_LEN + 1];
    SChar            mActionInfo[MMC_ACTION_MAX_LEN + 1];
    SChar            mNlsUse[MMC_NLSUSE_MAX_LEN + 1];

    qciUserInfo      mUserInfo;

    // PROJ-2727
    SChar            mSessionPropValueStr[IDP_MAX_VALUE_LEN + 1];
    UInt             mSessionPropValueLen;
    UShort           mSessionPropID;

    UInt             mPropertyAttribute;
    
    // PROJ-1579 NCHAR
    UInt             mNlsNcharLiteralReplace;


    /*
     * Session Property
     */

    mmcCommitMode    mCommitMode;
    UChar            mExplainPlan;

    //------------------------------------
    // Transaction Begin 시에 필요한 정보
    // - mIsolationLevel     : isolation level
    // - mReplicationMode    : replication mode
    // - mTransactionMode    : read only transaction인지 아닌지에 대한 mode
    // - CommitWriteWaitMode : commit 시에, log가 disk에 내려갈때까지
    //                         기다릴지 말지에 대한 mode
    // ps )
    // mIsolationLevel에 Replication Mode, Transaction Mode 정보가
    // 모두 oring되어있는 것을 BUG-15396 수정하면서 모두 풀었음
    //------------------------------------
    UInt             mIsolationLevel;
    UInt             mReplicationMode;
    UInt             mTransactionMode;
    idBool           mCommitWriteWaitMode; // BUG-17878 : type 변경

    UInt             mOptimizerMode;
    UInt             mHeaderDisplayMode;
    UInt             mStackSize;
    UInt             mNormalFormMaximum;
    // BUG-23780 TEMP_TBS_MEMORY 힌트 적용여부를 property로 제공
    UInt             mOptimizerDefaultTempTbsType;  
    UInt             mIdleTimeout;
    UInt             mQueryTimeout;
    /* BUG-32885 Timeout for DDL must be distinct to query_timeout or utrans_timeout */
    UInt             mDdlTimeout;
    UInt             mFetchTimeout;
    UInt             mUTransTimeout;

    /* BUG-31144 The number of statements should be limited in a session */
    UInt             mNumberOfStatementsInSession;
    UInt             mMaxStatementsPerSession;

    /* TRX_UPDATE_MAX_LOGSIZE의 값을 Session Level에서 가지고 있다.
     *
     * BUG-19080: Update Version이 일정개수이상 발생하면 해당
     *            Transaction을 Abort하는 기능이 필요합니다. */
    ULong            mUpdateMaxLogSize;

    SChar            mDateFormat[MMC_DATEFORMAT_MAX_LEN + 1];
    UInt             mSTObjBufSize;

    // PROJ-1665 : Parallel DML Mode
    idBool           mParallelDmlMode;

    // PROJ-1579 NCHAR
    UInt             mNlsNcharConvExcp;

    /*
     * Session Property (Server Only)
     */

    mmcByteOrder     mClientHostByteOrder;
    mmcByteOrder     mNumericByteOrder;
    
    /*
     * PROJ-1752 LIST PROTOCOL
     * BUGBUG: 향후 JDBC에 LIST 프로토콜이 확장되었을 시점에는
     * 아래 변수는 삭제되어야 한다.
     */
    idBool           mHasClientListChannel;

    // BUG-34725
    UInt             mFetchProtocolType;

    // PROJ-2256
    UInt             mRemoveRedundantTransmission;

    /*
     * Runtime Session Info
     */

    mmcSessionState  mSessionState;

    ULong            mEventFlag;

    UInt             mConnectTime;
    UInt             mIdleStartTime;
    //fix BUG-24041 none-auto commit mode에서 select statement begin할때
    // mActivated를 on시키면안됨.
    
    // mActivated 정의를 다음과 같이 명확히 합니다(주로 none-auto commit mode에서 의미가 있음).
    //  session에서 사용하고 있는 트랜잭션이 begin후
    // select가 아닌 DML이 발생하여 table lock, record lock를 hold하였고,
    // SM log가 write된 상태를 Activated on으로 정의한다.
    
    // 하지만 select만 수행한 경우에 root statement종료시 table IS-lock이
    // release되고 MVCC때문에 record-lock을 대기하지 않고 SM log 로write
    //하지 않았기때문에   Actived off으로 정의한다.

    //  session에서 사용하고 있는 트랜잭션이 begin후 아무것도
    //  execute하지 않은경우 역시,Actived off으로 정의한다.
    idBool           mActivated;
    SInt             mOpenStmtCount;
    mmcStmtID        mCurrStmtID;

    /* PROJ-1381 Fetch Across Commits */
    SInt             mOpenHoldFetchCount;   /**< Holdable로 열린 Fetch 개수 */

    /*
     * XA
     */

    idBool           mXaSessionFlag;
    mmdXaAssocState  mXaAssocState;

    //BUG-21122
    UInt             mAutoRemoteExec;

    /* BUG-31390,43333 Failover info for v$session */
    SChar            mFailOverSource[MMC_FAILOVER_SOURCE_MAX_LEN + 1];

    // BUG-34830
    UInt             mTrclogDetailPredicate;

    // BUG-32101
    SInt             mOptimizerDiskIndexCostAdj;

    // BUG-43736
    SInt             mOptimizerMemoryIndexCostAdj;

    /* PROJ-2208 Multi Currency */
    SInt             mNlsTerritory;
    SInt             mNlsISOCurrency;
    SChar            mNlsISOCode[MTL_TERRITORY_ISO_LEN + 1];
    SChar            mNlsCurrency[MTL_TERRITORY_CURRENCY_LEN + 1];
    SChar            mNlsNumChar[MTL_TERRITORY_NUMERIC_CHAR_LEN + 1];
    
    /* PROJ-2209 DBTIMEZONE */
    SChar            mTimezoneString[MMC_TIMEZONE_MAX_LEN + 1];
    SLong            mTimezoneSecond;

    /* PROJ-2047 Strengthening LOB - LOBCACHE */
    UInt             mLobCacheThreshold;

    /* PROJ-1090 Function-based Index */
    UInt             mQueryRewriteEnable;

    /*
     * Session properties for database link
     */
    UInt             mGlobalTransactionLevel;
    UInt             mDblinkRemoteStatementAutoCommit;

    /*
     * BUG-38430 해당 session에서 마지막 실행한 쿼리로 인해 변경된 record 갯수
     */
    ULong            mLastProcessRow; 

    /* PROJ-2473 SNMP 지원 */
    UInt             mSessionFailureCount; /* 연속으로 실행이 실패한 수 */
    
    /* PROJ-2441 flashback */
    UInt             mRecyclebinEnable;

    // BUG-41398 use old sort
    UInt             mUseOldSort;

    // BUG-41944
    UInt             mArithmeticOpMode;
    
    /* PROJ-2462 Result Cache */
    UInt             mResultCacheEnable;
    UInt             mTopResultCacheMode;

    /* PROJ-2492 Dynamic sample selection */
    UInt             mOptimizerAutoStats;
    /* BUG-42134 Created transitivity predicate of join predicate must be reinforced. */
    UInt             mOptimizerTransitivityOldRule;

    // BUG-42464 dbms_alert package
    mmcEventManager  mEvent;

    /* BUG-42853 LOCK TABLE에 UNTIL NEXT DDL 기능 추가 */
    idBool           mLockTableUntilNextDDL;
    UInt             mTableIDOfLockTableUntilNextDDL;

    /* BUG-42639 Monitoring query */
    UInt             mOptimizerPerformanceView;

    /* PROJ-2626 Snapshot Export */
    mmcClientAppInfoType mClientAppInfoType;

    /* PROJ-2638 shard native linker */
    SChar            mShardNodeName[SDI_NODE_NAME_MAX_SIZE + 1];

    /* TASK-7219 Analyzer/Transformer/Executor 성능개선 */
    idBool           mCallByShardAnalyzeProtocol;

    /* BUG-44967 */
    mmcTransID       mTransID;

    /* PROJ-2660 */
    sdiShardPin      mShardPin;

    /* BUG-46090 Meta Node SMN 전파 */
    ULong            mShardMetaNumber;
    ULong            mToBeShardMetaNumber;
    ULong            mLastShardMetaNumber;
    ULong            mReceivedShardMetaNumber;

    /* PROJ-2677 */
    UInt             mReplicationDDLSync;
    UInt             mReplicationDDLSyncTimeout;
    UInt             mPrintOutEnable;

    /* PROJ-2735 DDL Transaction */    
    idBool           mTransactionalDDL;
    
    /* PROJ-2736 Global DDL */
    idBool           mGlobalDDL;

    /* BUG-45707 */
    UInt             mShardClient;
    sdiSessionType   mShardSessionType;
    /* BUG-45899 */
    UInt             mTrclogDetailShard;

    /* BUG-46092 */
    sdiFailoverTypeStorage mDataNodeFailoverType;

    /* PROJ-2632 */
    UInt             mSerialExecuteMode;
    UInt             mTrcLogDetailInformation;

    mmcMessageCallback mMessageCallback;  /* BUG-46019 MessageCallback 등록 여부 */

    /* BUG-47648  disk partition에서 사용되는 prepared memory 사용량 개선 */
    UInt             mReducePartPrepareMemory;

    ULong            mAllocTransRetryCount; // BUG-47655

    idBool           mShardInPSMEnable;
    sdiInternalOperation mShardInternalLocalOperation;

    /* PROJ-2733-DistTxInfo */
    mmcGCTxCommitInfo mGCTxCommitInfo;

    UInt             mShardStatementRetry;
    UInt             mIndoubtFetchTimeout;
    UInt             mIndoubtFetchMethod;

    idBool           mGCTxPermit;  /* GCTx 허용 여부, GCTx 미지원 클라이언트는 허용 안함. */

    /* BUG-48132 */
    UInt             mPlanHashOrSortMethod;

    /* BUG-48161 */
    UInt             mBucketCountMax;

    UInt             mShardDDLLockTimeout;
    UInt             mShardDDLLockTryCount;

    UInt             mDDLLockTimeout;

    /* BUG-48348 */
    UInt             mEliminateCommonSubexpression;

    UShort           mClientTouchNodeCount;  /* BUG-48384 */

    /* TASK-7219 Non-shard DML */
    UInt             mStmtExecSeqForShardTx;

} mmcSessionInfo;

//--------------------
// X$SESSION 의 구조
//--------------------
typedef struct mmcSessionInfo4PerfV
{
    mmcSessID        mSessionID;      // ID
    //fix BUG-23656 session,xid ,transaction을 연계한 performance view를 제공하고,
    //그들간의 관계를 정확히 유지해야 함.
    mmcTransID       mTransID;
    mmcTaskState    *mTaskState;      // TASK_STATE
    ULong            mEventFlag;      // EVENTFLAG
    UChar            mCommName[IDL_IP_ADDR_MAX_LEN + 1]; // COMM_NAME
    /* PROJ-2474 SSL/TLS */
    UChar            mSslCipher[MMC_SSLINFO_MAX_LEN + 1];
    UChar            mSslPeerCertSubject[MMC_SSLINFO_MAX_LEN + 1];
    UChar            mSslPeerCertIssuer[MMC_SSLINFO_MAX_LEN + 1];
    idBool           mXaSessionFlag;  // XA_SESSION_FLAG
    mmdXaAssocState  mXaAssocState;   // XA_ASSOCIATE_FLAG
    UInt             mQueryTimeout;   // QUERY_TIME_LIMIT
    /* BUG-32885 Timeout for DDL must be distinct to query_timeout or utrans_timeout */
    UInt             mDdlTimeout;     // DDL_TIME_LIMIT
    UInt             mFetchTimeout;   // FETCH_TIME_LIMIT
    UInt             mUTransTimeout;  // UTRANS_TIME_LIMIT
    UInt             mIdleTimeout;    // IDLE_TIME_LIMIT
    UInt             mIdleStartTime;  // IDLE_START_TIME
    idBool           mActivated;      // ACTIVE_FLAG
    SInt             mOpenStmtCount;  // OPENED_STMT_COUNT
    SChar            mClientPackageVersion[MMC_CLIENTINFO_MAX_LEN + 1];  // CLIENT_PACKAGE_VERSION
    SChar            mClientProtocolVersion[MMC_CLIENTINFO_MAX_LEN + 1]; // CLIENT_PROTOCOL_VERSION
    ULong            mClientPID;      // CLIENT_PID
    SChar            mClientType[MMC_CLIENTINFO_MAX_LEN + 1]; // CLIENT_TYPE
    SChar            mClientAppInfo[MMC_APPINFO_MAX_LEN + 1]; // CLIENT_APP_INFO
    /* BUG-41511 supporting to similar DBMS_APPLICATION_INFO */
    SChar            mModuleInfo[MMC_MODULE_MAX_LEN + 1];
    SChar            mActionInfo[MMC_ACTION_MAX_LEN + 1];
    SChar            mNlsUse[MMC_NLSUSE_MAX_LEN + 1]; // CLIENT_NLS
    qciUserInfo      mUserInfo;       // DB_USERNAME, DB_USERID, DEFAULT_TBSID, DEFAULT_TEMP_TBSID, SYSDBA_FLAG
    mmcCommitMode    mCommitMode;          // AUTOCOMMIT_FLAG
    mmcSessionState  mSessionState;        // SESSION_STATE 
    UInt             mIsolationLevel;      // ISOLATION_LEVEL
    UInt             mReplicationMode;     // REPLICATION_MODE
    UInt             mTransactionMode;     // TRANSACTION_MODE 
    idBool           mCommitWriteWaitMode; // COMMIT_WRITE_WAIT_MODE
    ULong            mUpdateMaxLogSize;    // TRX_UPDATE_MAX_LOGSIZE
    UInt             mOptimizerMode;       // OPTIMIZER_MODE 
    UInt             mHeaderDisplayMode;   // HEADER_DISPLAY_MODE
    mmcStmtID        mCurrStmtID;          // CURRENT_STMT_ID
    UInt             mStackSize;           // STACK_SIZE
    SChar            mDateFormat[MMC_DATEFORMAT_MAX_LEN + 1]; // DEFAULT_DATE_FORMAT
    idBool           mParallelDmlMode;        // PARALLEL_DML_MODE
    UInt             mConnectTime;            // LOGIN_TIME
    UInt             mNlsNcharConvExcp;       // NLS_NCHAR_CONV_EXCP
    UInt             mNlsNcharLiteralReplace; // NLS_NCHAR_LITERAL_REPLACE
    UInt             mAutoRemoteExec;         // AUTO_REMOTE_EXEC
    SChar            mFailOverSource[MMC_FAILOVER_SOURCE_MAX_LEN + 1];   // BUG-31390,43333 Failover info for v$session
    SChar            mNlsTerritory[MMC_NLSUSE_MAX_LEN + 1];             // PROJ-2208
    SChar            mNlsISOCurrency[MMC_NLSUSE_MAX_LEN + 1];           // PROJ-2208
    SChar            mNlsCurrency[MTL_TERRITORY_CURRENCY_LEN + 1];   // PROJ-2208
    SChar            mNlsNumChar[MTL_TERRITORY_NUMERIC_CHAR_LEN + 1];// PROJ-2208
    SChar            mTimezoneString[MMC_TIMEZONE_MAX_LEN + 1];      /* PROJ-2209 DBTIMEZONE */
    UInt             mLobCacheThreshold;                             /* PROJ-2047 Strengthening LOB - LOBCACHE */
    UInt             mQueryRewriteEnable;                            /* PROJ-1090 Function-based Index */
    UInt             mDblinkGlobalTransactionLevel;     /* DBLINK_GLOBAL_TRANSACTION_LEVEL */
    UInt             mGlobalTransactionLevel;           /* GLOBAL_TRANSACTION_LEVEL */
    UInt             mDblinkRemoteStatementAutoCommit;  /* DBLINK_REMOTE_STATEMENT_COMMIT */

    /*
     * BUG-40120  MAX_STATEMENTS_PER_SESSION value has to be seen in v$session when using alter session.
     */
    UInt             mMaxStatementsPerSession;

    SChar            mShardPinStr[MMC_SHARD_PIN_STR_LEN + 1];

    /* BUG-46090 Meta Node SMN 전파 */
    ULong            mShardMetaNumber;          // SHARD_META_NUMBER
    ULong            mLastShardMetaNumber;
    ULong            mReceivedShardMetaNumber;

    /* PROJ-2677 */
    UInt             mReplicationDDLSync;
    UInt             mReplicationDDLSyncTimeout;

    /* BUG-45707 */
    UInt             mShardClient;
    UInt             mShardSessionType;

    mmcMessageCallback mMessageCallback;  /* BUG-46019 MessageCallback 등록 여부 */

    ULong            mAllocTransRetryCount;   // BUG-47655

    // PROJ-2727 add connect attr
    UInt             mNormalFormMaximum;
    UInt             mOptimizerDefaultTempTbsType;
    UInt             mSTObjBufSize;
    UInt             mTrclogDetailPredicate;
    SInt             mOptimizerDiskIndexCostAdj;
    SInt             mOptimizerMemoryIndexCostAdj;
    UInt             mRecyclebinEnable;
    UInt             mUseOldSort;
    UInt             mArithmeticOpMode;
    UInt             mResultCacheEnable;
    UInt             mTopResultCacheMode;
    UInt             mOptimizerAutoStats;
    UInt             mOptimizerTransitivityOldRule;
    UInt             mOptimizerPerformanceView;
    UInt             mPrintOutEnable;
    UInt             mTrclogDetailShard;
    UInt             mSerialExecuteMode;
    UInt             mTrcLogDetailInformation;
    UInt             mReducePartPrepareMemory;   
    idBool           mTransactionalDDL;     /* PROJ-2735 DDL Transaction */ 
    sdiInternalOperation mShardInternalLocalOperation;
    idBool           mGlobalDDL;            /* PROJ-2736 Global DDL */ 

    mmcGCTxCommitInfo mGCTxCommitInfo;

    UInt             mShardStatementRetry;
    UInt             mIndoubtFetchTimeout;
    UInt             mIndoubtFetchMethod;

    UInt             mShardDDLLockTimeout;
    UInt             mShardDDLLockTryCount;

    UInt             mDDLLockTimeout;


    UInt             mPlanHashOrSortMethod;            /* BUG-48132 */

    UInt             mBucketCountMax;                  /* BUG-48161 */

    UInt             mEliminateCommonSubexpression;    /* BUG-48348 */

    /* TASK-7219 Non-shard DML */
    UInt             mStmtExecSeqForShardTx;
} mmcSessionInfo4PerfV;

//--------------------
// PROJ-2451 Concurrent Execute Package
// X$INTERNAL_SESSION 의 구조
//--------------------
typedef struct mmcInternalSessionInfo4PerfV
{
    mmcSessID        mSessionID;      // ID
    //fix BUG-23656 session,xid ,transaction을 연계한 performance view를 제공하고,
    //그들간의 관계를 정확히 유지해야 함.
    mmcTransID       mTransID;
    UInt             mQueryTimeout;   // QUERY_TIME_LIMIT
    /* BUG-32885 Timeout for DDL must be distinct to query_timeout or utrans_timeout */
    UInt             mDdlTimeout;     // DDL_TIME_LIMIT
    UInt             mFetchTimeout;   // FETCH_TIME_LIMIT
    UInt             mUTransTimeout;  // UTRANS_TIME_LIMIT
    UInt             mIdleTimeout;    // IDLE_TIME_LIMIT
    UInt             mIdleStartTime;  // IDLE_START_TIME
    idBool           mActivated;      // ACTIVE_FLAG
    SInt             mOpenStmtCount;  // OPENED_STMT_COUNT
    SChar            mNlsUse[MMC_NLSUSE_MAX_LEN + 1]; // CLIENT_NLS
    qciUserInfo      mUserInfo;       // DB_USERNAME, DB_USERID, DEFAULT_TBSID, DEFAULT_TEMP_TBSID, SYSDBA_FLAG
    mmcCommitMode    mCommitMode;          // AUTOCOMMIT_FLAG
    mmcSessionState  mSessionState;        // SESSION_STATE 
    UInt             mIsolationLevel;      // ISOLATION_LEVEL
    UInt             mReplicationMode;     // REPLICATION_MODE
    UInt             mTransactionMode;     // TRANSACTION_MODE 
    idBool           mCommitWriteWaitMode; // COMMIT_WRITE_WAIT_MODE
    ULong            mUpdateMaxLogSize;    // TRX_UPDATE_MAX_LOGSIZE
    UInt             mOptimizerMode;       // OPTIMIZER_MODE 
    UInt             mHeaderDisplayMode;   // HEADER_DISPLAY_MODE
    mmcStmtID        mCurrStmtID;          // CURRENT_STMT_ID
    UInt             mStackSize;           // STACK_SIZE
    SChar            mDateFormat[MMC_DATEFORMAT_MAX_LEN + 1]; // DEFAULT_DATE_FORMAT
    idBool           mParallelDmlMode;        // PARALLEL_DML_MODE
    UInt             mConnectTime;            // LOGIN_TIME
    UInt             mNlsNcharConvExcp;       // NLS_NCHAR_CONV_EXCP
    UInt             mNlsNcharLiteralReplace; // NLS_NCHAR_LITERAL_REPLACE
    UInt             mAutoRemoteExec;         // AUTO_REMOTE_EXEC
    SChar            mFailOverSource[MMC_FAILOVER_SOURCE_MAX_LEN + 1];   // BUG-31390,BUG-43333 Failover info for v$session
    SChar            mNlsTerritory[MMC_NLSUSE_MAX_LEN + 1];             // PROJ-2208
    SChar            mNlsISOCurrency[MMC_NLSUSE_MAX_LEN + 1];           // PROJ-2208
    SChar            mNlsCurrency[MTL_TERRITORY_CURRENCY_LEN + 1];   // PROJ-2208
    SChar            mNlsNumChar[MTL_TERRITORY_NUMERIC_CHAR_LEN + 1];// PROJ-2208
    SChar            mTimezoneString[MMC_TIMEZONE_MAX_LEN + 1];      /* PROJ-2209 DBTIMEZONE */
    UInt             mLobCacheThreshold;                             /* PROJ-2047 Strengthening LOB - LOBCACHE */
    UInt             mQueryRewriteEnable;                            /* PROJ-1090 Function-based Index */
} mmcInternalSessionInfo4PerfV;


class mmcTask;
class mmdXid;

class mmcSession
{
public:
    static qciSessionCallback mCallback;
    static smiSessionCallback mSessionInfoCallback;

private:
    mmcSessionInfo  mInfo;

    mtlModule      *mLanguage;

    qciSession      mQciSession;

private:
    /*
     * Statement List
     */

    iduList         mStmtList;
    iduList         mFetchList;

    iduMutex        mStmtListMutex;

    /* PROJ-1381 Fetch Across Commits */
    iduList         mCommitedFetchList; /**< Commit된 Holdable Fetch List. */
    iduMutex        mFetchListMutex;    /**< FetchList, CommitedFetchList를 위한 lock. */

    mmcStatement   *mExecutingStatement;

private:
    /*
     * Transaction for Non-Autocommit Mode
     */

    mmcTransObj    *mTrans;
    friend class    mmcTrans;         /* access mTrans of mmcSession private member
                                       * to control share tx in mmcTrans class
                                       */
    mmcTxConcurrencyDump mTransDump;

    idBool          mTransAllocFlag;
    idBool          mSessionBegin;    /* non-autocommit의 session begin되었는지 여부 */
    idBool          mTransLazyBegin;  /* Query가 들어오면 Transaction을 시작 */
    idBool          mTransPrepared;   /* trans가 prepare되었는지 여부 */
    ID_XID          mTransXID;        /* trans가 prepare한 XID */
    sdiZKPendingJobFunc mZKPendingFunc;
private:
    /*
     * LOB
     */

    smuHashBase     mLobLocatorHash;

private:
    /*
     * PROJ-1629 2가지 통신 프로토콜 개선
     */

    UChar          *mChunk;
    UInt            mChunkSize;
    /* PROJ-2160 CM 타입제거
       위의 mChunk 는 Insert 시에 사용되며
       mOutChunk 는 outParam 시에 사용된다. */
    UChar          *mOutChunk;
    UInt            mOutChunkSize;

private:
    /*
     * Queue
     */

    mmqQueueInfo            *mQueueInfo;
    iduListNode             mQueueListNode;

    //PROJ-1677 DEQ
    smSCN                   mDeqViewSCN;
    vSLong                  mQueueEndTime;

    ULong                   mQueueWaitTime;

    smuHashBase             mEnqueueHash;
    //PROJ-1677 DEQUEUE
    smuHashBase             mDequeueHash4Rollback;
    mmcPartialRollbackFlag  mPartialRollbackFlag;

    idBool                  mNeedQueueWait;  /* BUG-46183 */
private:
    /*
     * XA
     */
    //fix BUG-21794
    iduList         mXidLst;
    //fix BUG-20850
    mmcCommitMode   mLocalCommitMode;
    mmcTransObj    *mLocalTrans;
    idBool          mLocalTransBegin;
    //fix BUG-21771
    idBool          mNeedLocalTxBegin;
    /* PROJ-2436 ADO.NET MSDTC */
    mmcTransEscalation mTransEscalation;

private:
    /* PROJ-2177 User Interface - Cancel */
    iduMemPool      mStmtIDPool;
    iduHash         mStmtIDMap;

private:
    /*
     * Database Link
     */
    dkiSession      mDatabaseLinkSession;

private:
    /*
     * Statistics
     */

    idvSession      mStatistics;
    idvSession      mOldStatistics; // for temporary stat-value keeping
    idvSQL          mStatSQL;

private:
    /* PROJ-2109 : Remove the bottleneck of alloc/free stmts. */
    /*
     * Mutextpool
     */
    mmcMutexPool    mMutexPool;

private:
    idBool          mIsNeedBlockCommit;

    idBool          mIsGTx;
    idBool          mIsGCTx;

public:
    IDE_RC initialize(mmcTask *aTask, mmcSessID aSessionID);
    IDE_RC finalize();
    void preFinalizeShardSession();

    IDE_RC findLanguage();

    /* BUG-47650 BUG-38585 IDE_ASSERT remove */
    IDE_RC disconnect(idBool aClearClientInfoFlag);
    void   changeSessionStateService();

    /* BUG-28866 : Logging을 위한 함수 추가 */
    void   loggingSession(SChar *aLogInOut, mmcSessionInfo *aInfo);
    IDE_RC beginSession();
    IDE_RC endSession();

    /* PROJ-1381 Fetch Across Commits */
    IDE_RC closeAllCursor(idBool aSuccess, mmcCloseMode aCursorCloseMode);
    IDE_RC closeAllCursorByFetchList(iduList *aFetchList, idBool aSuccess);

    inline void   getDeqViewSCN(smSCN * aDeqViewSCN);
    IDE_RC endPendingTrans( ID_XID *aXID,
                            idBool aIsCommit,
                            smSCN *aGlobalCommitSCN );
    IDE_RC endPendingSharedTxDelegateSession( mmcTransObj * aTransObj,
                                              ID_XID      * aXID,
                                              idBool        aIsCommit,
                                              smSCN       * aGlobalCommitSCN );
    IDE_RC prepareForShard( ID_XID * aXID,
                            idBool * aReadOnly,
                            smSCN  * aPrepareSCN );
    IDE_RC prepareForShardDelegateSession( mmcTransObj * aTransObj,
                                           ID_XID      * aXID,
                                           idBool      * aIsReadOnly,
                                           smSCN       * aPrepareSCN );
    IDE_RC closeCursorForShardDelegateSession( mmcTransObj * aTransObj );  /* BUG-48931 */

    IDE_RC blockDelegateSession( mmcTransObj * aTransObj, mmcSession ** aDelegatedSession );
    void   unblockDelegateSession( mmcTransObj * aTransObj );

    IDE_RC endPendingBySyntax( SChar * aXIDStr, 
                               UInt    aXIDStrSize,
                               idBool  aIsCommit );

    IDE_RC commit(idBool bInStoredProc = ID_FALSE);
    IDE_RC commitForceDatabaseLink(idBool bInStoredProc = ID_FALSE);
    IDE_RC rollback(const SChar* aSavePoint = NULL, idBool bInStoredProc = ID_FALSE);
    IDE_RC rollbackForceDatabaseLink( idBool bInStoredProc = ID_FALSE );
    IDE_RC savepoint(const SChar* aSavePoint, idBool bInStoredProc = ID_FALSE);

    IDE_RC commitInternal( void * aUserContext );
    
    void executeZookeeperPendingJob();

    /* BUG-46785 Shard statement partial rollback */
    IDE_RC shardStmtPartialRollback( void );

    IDE_RC shardNodeConnectionReport( UInt              aNodeId, 
                                      UChar             aDestination );
    IDE_RC shardNodeTransactionBrokenReport( void );

    //PROJ-1677 DEQUEUE
    inline void                     clearPartialRollbackFlag();
    inline mmcPartialRollbackFlag   getPartialRollbackFlag();
    inline void                     setPartialRollbackFlag();
public:
    /*
     * Accessor
     */

    mmcTask         *getTask();

    mtlModule       *getLanguage();

    qciSession      *getQciSession();

    mmcStatement    *getExecutingStatement();
    void             setExecutingStatement(mmcStatement *aStatement);

    iduList         *getStmtList();
    iduList         *getFetchList();

    void             lockForStmtList();
    void             unlockForStmtList();

    /* PROJ-2109 : Remove the bottleneck of alloc/free stmts. */
    mmcMutexPool    *getMutexPool();

    /* PROJ-1381 Fetch Across Commits */
    iduList*         getCommitedFetchList(void);
    void             lockForFetchList(void);
    void             unlockForFetchList(void);

    dkiSession     * getDatabaseLinkSession( void );
    
public:
    /*
     * Transaction Accessor
     */
    mmcTransObj     *allocTrans();
    mmcTransObj     *allocTrans(mmcStatement* aStmt);
    mmcTransObj     *getTransPtr();
    mmcTransObj     *getTransPtr(mmcStatement* aStmt);

    void             reallocTrans();
    void             setTrans(mmcTransObj* aTrans);

public:
    /*
     * Info Accessor
     */

    mmcSessionInfo  *getInfo();

    mmcSessID        getSessionID();
    ULong           *getEventFlag();

    qciUserInfo     *getUserInfo();
    void             setUserInfo(qciUserInfo *aUserInfo);
    idBool           isSysdba();

    mmcCommitMode    getCommitMode();
    IDE_RC           setCommitMode(mmcCommitMode aCommitMode);

    UChar            getExplainPlan();
    void             setExplainPlan(UChar aExplainPlan);

    UInt             getIsolationLevel();
    void             setIsolationLevel(UInt aIsolationLevel);

    // BUG-15396 수정 시, 추가되었음
    UInt             getReplicationMode();

    // BUG-15396 수정 시, 추가되었음
    UInt             getTransactionMode();

    // To Fix BUG-15396 : alter session set commit write 속성 변경 시 호출됨
    idBool           getCommitWriteWaitMode();
    void             setCommitWriteWaitMode(idBool aCommitWriteType);

    idBool           isReadOnlySession();

    void             setUpdateMaxLogSize( ULong aUpdateMaxLogSize );
    ULong            getUpdateMaxLogSize();

    idBool           isReadOnlyTransaction();

    UInt             getOptimizerMode();
    void             setOptimizerMode(UInt aMode);
    UInt             getHeaderDisplayMode();
    void             setHeaderDisplayMode(UInt aMode);

    UInt             getStackSize();
    IDE_RC           setStackSize(SInt aStackSize);

    /* BUG-41511 supporting to similar DBMS_APPLICATION_INFO */
    IDE_RC           setClientAppInfo( SChar *aClientAppInfo, UInt aLength );
    IDE_RC           setModuleInfo( SChar *aModuleInfo, UInt aLength );
    IDE_RC           setActionInfo( SChar *aActionInfo, UInt aLength );

    /* PROJ-2209 DBTIMEZONE */
    SLong            getTimezoneSecond();
    IDE_RC           setTimezoneSecond( SLong aTimezoneSecond );
    SChar           *getTimezoneString();
    IDE_RC           setTimezoneString( SChar *aTimezoneString, UInt aLength );

    UInt             getNormalFormMaximum();
    void             setNormalFormMaximum(UInt aNormalFormMaximum);

    // BUG-23780  TEMP_TBS_MEMORY 힌트 적용여부를 property로 제공
    UInt             getOptimizerDefaultTempTbsType();
    void             setOptimizerDefaultTempTbsType(UInt aValue);

    UInt             getQueryTimeout();
    void             setQueryTimeout(UInt aValue);

    /* BUG-32885 Timeout for DDL must be distinct to query_timeout or utrans_timeout */
    UInt             getDdlTimeout();
    void             setDdlTimeout(UInt aValue);

    UInt             getFetchTimeout();
    void             setFetchTimeout(UInt aValue);

    UInt             getUTransTimeout();
    void             setUTransTimeout(UInt aValue);

    UInt             getIdleTimeout();
    void             setIdleTimeout(UInt aValue);

    SChar           *getDateFormat();
    IDE_RC           setDateFormat(SChar *aDateFormat, UInt aLength);

    mmcByteOrder     getClientHostByteOrder();
    void             setClientHostByteOrder(mmcByteOrder aByteOrder);

    mmcByteOrder     getNumericByteOrder();
    void             setNumericByteOrder(mmcByteOrder aByteOrder);

    mmcSessionState  getSessionState();
    void             setSessionState(mmcSessionState aState);

    UInt             getIdleStartTime();
    void             setIdleStartTime(UInt aIdleStartTime);

    idBool           isActivated();
    void             setActivated(idBool aActivated);

    idBool           isAllStmtEnd();
    void             changeOpenStmt(SInt aCount);

    /* PROJ-1381 FAC : Holdable Fetch를 제외한 Stmt가 모두 닫혔는지 확인할 수 있어야 한다. */
    idBool           isAllStmtEndExceptHold(void);
    void             changeHoldFetch(SInt aCount);

    idBool           isXaSession();
    void             setXaSession(idBool aFlag);

    mmdXaAssocState  getXaAssocState();
    void             setXaAssocState(mmdXaAssocState aState);

    // BUG-15396 : transaction을 위한 session 정보 반환
    //             transaction begin 시에 필요함
    UInt             getSessionInfoFlagForTx();
    void             setSessionInfoFlagForTx(UInt   aIsolationLevel,
                                             UInt   aReplicationMode,
                                             UInt   aTransactionMode,
                                             idBool aCommitWriteWaitMode);
    // transaction의 isolation level을 반환
    UInt             getTxIsolationLevel(mmcTransObj * aTrans);

    // transaction의 transaction mode를 반환
    UInt             getTxTransactionMode(mmcTransObj * aTrans);

    // PROJ-1583 large geometry
    void             setSTObjBufSize(UInt aObjBufSize );
    UInt             getSTObjBufSize();

    // PROJ-1665 : alter session set parallel_dml_mode = 0/1 호출시 사용됨
    IDE_RC           setParallelDmlMode(idBool aParallelDmlMode);
    idBool           getParallelDmlMode();

    // PROJ-1579 NCHAR
    void             setNlsNcharConvExcp(UInt aValue);
    UInt             getNlsNcharConvExcp();

    // PROJ-1579 NCHAR
    void             setNlsNcharLiteralReplace(UInt aValue);
    UInt             getNlsNcharLiteralReplace();
    
    //BUG-21122
    UInt             getAutoRemoteExec();
    void             setAutoRemoteExec(UInt aValue);

    /* BUG-31144 */
    UInt getNumberOfStatementsInSession();
    void setNumberOfStatementsInSession(UInt aValue);

    UInt getMaxStatementsPerSession();
    void setMaxStatementsPerSession(UInt aValue);

    // PROJ-2256
    UInt             getRemoveRedundantTransmission();

    // BUG-34830
    UInt getTrclogDetailPredicate();
    void setTrclogDetailPredicate(UInt aTrclogDetailPredicate);

    // BUG-32101
    SInt getOptimizerDiskIndexCostAdj();
    void setOptimizerDiskIndexCostAdj(SInt aOptimizerDiskIndexCostAdj);

    // BUG-43736
    SInt getOptimizerMemoryIndexCostAdj();
    void setOptimizerMemoryIndexCostAdj(SInt aOptimizerMemoryIndexCostAdj);

    /* PROJ-2208 Multi Currency */
    const SChar * getNlsTerritory();
    IDE_RC  setNlsTerritory( SChar * aValue, UInt aLength );

    /* PROJ-2208 Multi Currency */
    SChar * getNlsISOCurrency();
    IDE_RC  setNlsISOCurrency( SChar * aValue, UInt aLength );

    /* PROJ-2208 Multi Currency */
    SChar * getNlsCurrency();
    IDE_RC  setNlsCurrency( SChar * aValue, UInt aLength );

    /* PROJ-2208 Multi Currency */
    SChar * getNlsNumChar();
    IDE_RC  setNlsNumChar( SChar * aValue, UInt aLength );

    /* PROJ-2047 Strengthening LOB - LOBCACHE */
    UInt getLobCacheThreshold();
    void setLobCacheThreshold(UInt aValue);

    /* PROJ-1090 Function-based Index */
    UInt getQueryRewriteEnable();
    void setQueryRewriteEnable(UInt aValue);
    
    /*
     * Database link session property
     */
    UInt   getGlobalTransactionLevel();
    IDE_RC setGlobalTransactionLevel( UInt aValue );
    IDE_RC checkGCTxPermit( UInt aValue );
    void   setDblinkRemoteStatementAutoCommit( UInt aValue );

    /* PROJ-2441 flashback */
    UInt getRecyclebinEnable();
    void setRecyclebinEnable(UInt aValue);

    /* BUG-42853 LOCK TABLE에 UNTIL NEXT DDL 기능 추가 */
    idBool getLockTableUntilNextDDL();
    void   setLockTableUntilNextDDL( idBool aValue );
    UInt   getTableIDOfLockTableUntilNextDDL();
    void   setTableIDOfLockTableUntilNextDDL( UInt aValue );

    // BUG-41398 use old sort
    UInt getUseOldSort();
    void setUseOldSort(UInt aValue);

    // BUG-41944
    UInt getArithmeticOpMode();
    void setArithmeticOpMode(UInt aValue);

    /* PROJ-2462 Result Cache */
    UInt getResultCacheEnable();
    void setResultCacheEnable( UInt aValue );
    /* PROJ-2462 Result Cache */
    UInt getTopResultCacheMode();
    void setTopResultCacheMode( UInt aValue );

    /* PROJ-2492 Dynamic sample selection */
    UInt getOptimizerAutoStats();
    void setOptimizerAutoStats( UInt aValue );
    idBool isAutoCommit();

    /* BUG-42134 Created transitivity predicate of join predicate must be reinforced. */
    UInt getOptimizerTransitivityOldRule();
    void setOptimizerTransitivityOldRule( UInt aValue );

    /* BUG-42639 Monitoring query */
    UInt getOptimizerPerformanceView();
    void setOptimizerPerformanceView( UInt aValue );

    /* PROJ-2626 Snapshot Export */
    mmcClientAppInfoType getClientAppInfoType( void );

    /* PROJ-2638 shard native linker */
    IDE_RC reloadShardMetaNumber( idBool aIsLocalOnly );
    IDE_RC touchShardNode(UInt aNodeId);

    /* PROJ-2660 */
    sdiShardPin getShardPIN();
    void        setShardPIN( sdiShardPin aShardPin );
    SChar      *getShardNodeName();
    idBool      isShardUserSession();
    idBool      isShardCoordinatorSession();
    /* TASK-7219 Analyzer/Transformer/Executor 성능개선 */
    idBool      getCallByShardAnalyzeProtocol();
    void        setCallByShardAnalyzeProtocol( idBool aCallByShardAnalyzeProtocol );

    idBool      isShardTrans();
    idBool      isShareableTrans();
    void        setNewSessionShardPin();

    /* BUG-46090 Meta Node SMN 전파 */
    idBool      isMetaNodeShardCli();
    ULong       getShardMetaNumber();
    void        setShardMetaNumber( ULong aSMN );

    IDE_RC setCallbackForReloadNewIncreasedDataSMN(smiTrans * aTrans);
    static void reloadDataShardMetaNumber( void * aSession );

    ULong       getLastShardMetaNumber();
    void        setLastShardMetaNumber(ULong aSMN);
    void        clearLastShardMetaNumber();
    idBool      isReshardOccurred();
    ULong       getReceivedShardMetaNumber();
    void        setReceivedShardMetaNumber(ULong aSMN);
    idBool      isNeedRebuildNoti();
    IDE_RC      applyShardMetaChange( smiTrans * aTrans,
                                      ULong    * aNewSMN );
    void        clearShardDataInfo();
    void        clearShardDataInfoForRebuild();

    /* BUG-46092 */
    void        freeRemoteStatement( UInt aNodeId, UChar aMode );
    UInt        getShardFailoverType( UInt aNodeId );

    /* BUG-46100 Session SMN Update */
    IDE_RC      checkSMNForDataNodeAndSetSMN( ULong         aSMNForSession,
                                              const SChar * aProtocolErrorMsg );

    /* BUG-45707 */
    void setShardClient( sdiShardClient aShardClient );
    /* BUG-46092 */
    UInt isShardClient();

    void setShardSessionType( sdiSessionType aSessionType );
    sdiSessionType getShardSessionType();
    inline idBool isShardLibrarySession();

    /* BUG-45899 */
    void setTrclogDetailShard( UInt aTrclogDetailShard );
    UInt getTrclogDetailShard();

    idBool getSessionBegin();
    idBool isTransAlloc( void );
    void   setSessionBegin(idBool aBegin);
    idBool getTransBegin();

    idBool getTransLazyBegin();
    void setTransLazyBegin( idBool aLazyBegin );

    void initTransStartMode();

    idBool getTransPrepared();
    void setTransPrepared(ID_XID * aXID);
    ID_XID* getTransPreparedXID();

    /* PROJ-2677 */
    UInt   getReplicationDDLSync();
    void   setReplicationDDLSync( UInt aValue );
    UInt   getReplicationDDLSyncTimeout();
    void   setReplicationDDLSyncTimeout( UInt aValue );

    /* PROJ-2735 DDL Transaction */
    idBool getTransactionalDDL();
    IDE_RC setTransactionalDDL( idBool aDDLTrans );

    /* PROJ-2736 Global DDL */
    idBool getGlobalDDL();
    IDE_RC setGlobalDDL( idBool aGlobalDDL );

    idBool isDDLAutoCommit();

    idBool globalDDLUserSession();

    void    setPrintOutEnable(UInt aValue);
    UInt    getPrintOutEnable();    

    /* PROJ-22632 */
    UInt getSerialExecuteMode();
    void setSerialExecuteMode( UInt aValue );

    UInt getTrcLogDetailInformation();
    void setTrcLogDetailInformation( UInt aValue );

    void setShardDDLLockTimeout( UInt aShardDDLLockTimeout );
    void setShardDDLLockTryCount( UInt aShardDDLLockTryCount );
    void setDDLLockTimeout( SInt aDDLLockTimeout );

    /* BUG-46019 */
    mmcMessageCallback getMessageCallback();
    void               setMessageCallback(mmcMessageCallback aValue);

    /* BUG-47648  disk partition에서 사용되는 prepared memory 사용량 개선 */
    UInt getReducePartPrepareMemory();
    void setReducePartPrepareMemory( UInt aValue );

    // PROJ-2727
    IDE_RC setSessionPropertyInfo( UShort   aSessionPropertyID,
                                   SChar  * aSessionPropValue,
                                   UInt     aSessionPropValueLen );
    
    IDE_RC setSessionPropertyInfo( UShort aSessionPropID,
                                   UInt   aSessionPropValue );
    
    void getSessionPropertyInfo( UShort *aSessionPropertyID,
                                 SChar  **aSessionPropValue,
                                 UInt   *aSessionPropValueLen );

    UInt getDblinkRemoteStatementAutoCommit();

    void setPropertyAttrbute( UInt aValue );
    UInt getPropertyAttrbute();

    // BUG-47773
    IDE_RC setShardSessionProperty();
    
    idBool getShardInPSMEnable();
    void setShardInPSMEnable( idBool aValue );

    /* for sharding HA management */
    static sdiInternalOperation getShardInternalLocalOperation(void * aSession);
    static IDE_RC setShardInternalLocalOperationCallback( void * aSession, sdiInternalOperation aValue );
    IDE_RC setShardInternalLocalOperation( sdiInternalOperation aValue );

    idBool getIsNeedBlockCommit();
    void   setIsNeedBlockCommit();

    IDE_RC blockForLibrarySession( mmcTransObj * aTrans,
                                   idBool      * aIsBlocked );
    void   unblockForLibrarySession( mmcTransObj * aTrans );

    /* PROJ-2733-DistTxInfo */
    UChar *getSessionTypeString();
    idBool isGTx();
    idBool isGCTx();
    sdiClientInfo * getShardClientInfo();
    void getCoordSCN( sdiClientInfo * aClientInfo,
                      smSCN         * aCoordSCN );
    void getCoordPrepareSCN( sdiClientInfo * aClientInfo,
                             smSCN         * aPrepareSCN );
    void setCoordGlobalCommitSCN( sdiClientInfo * aClientInfo,
                                  smSCN         * aGlobalCommitSCN );
    void setShardStatementRetry( UInt aValue );
    UInt getShardStatementRetry();
    void setIndoubtFetchTimeout( UInt aValue );
    void setIndoubtFetchMethod( UInt aValue );
    idBool getGCTxPermit();
    void   setGCTxPermit(idBool aValue);
    void clearToBeShardMetaNumber();

    /* BUG-48132 */
    UInt getPlanHashOrSortMethod();
    void setPlanHashOrSortMethod( UInt aValue );

    /* BUG-48161 */
    UInt getBucketCountMax();
    void setBucketCountMax( UInt aValue );

    /* BUG-48348 */
    UInt getEliminateCommonSubexpression();
    void setEliminateCommonSubexpression( UInt aValue );

    /* BUG-48384 */
    UShort getClientTouchNodeCount();
    void   setClientTouchNodeCount(UShort aValue);

    void setInternalTableSwap( smiTrans * aTrans );

    /* TASK-7219 Non-shard DML */
    void initStmtExecSeqForShardTx();
    void increaseStmtExecSeqForShardTx();
    void decreaseStmtExecSeqForShardTx();
    UInt getStmtExecSeqForShardTx();
    void setStmtExecSeqForShardTx( UInt aValue );
public:
    /*
     * Set
     */

    IDE_RC setReplicationMode(UInt aReplicationMode);
    IDE_RC setTX(UInt aType, UInt aValue, idBool aIsSession);
    IDE_RC set(SChar *aName, SChar *aValue);

    static IDE_RC setAger(mmcSession *aSession, SChar *aValue);

public:
    /*
     * LOB
     */

    IDE_RC allocLobLocator(mmcLobLocator **aLobLocator, UInt aStatementID, smLobLocator aLocatorID);
    IDE_RC freeLobLocator(smLobLocator aLocatorID, idBool *aFound);

    IDE_RC findLobLocator(mmcLobLocator **aLobLocator, smLobLocator aLocatorID);

    IDE_RC clearLobLocator();
    IDE_RC clearLobLocator(UInt aStatementID);

public:
    /*
     * PROJ-1629 2가지 통신 프로토콜 개선
     */

    UInt   getChunkSize();
    UChar *getChunk();
    IDE_RC allocChunk(UInt aAllocSize);
    IDE_RC allocChunk4Fetch(UInt aAllocSize);
    UInt   getFetchChunkLimit();
    idBool getHasClientListChannel();
    UInt   getFetchProtocolType();  // BUG-34725

    /* PROJ-2160 CM 타입제거 */
    UInt   getOutChunkSize();
    UChar *getOutChunk();
    IDE_RC allocOutChunk(UInt aAllocSize);

public:
    /*
     * Queue
     */

    mmqQueueInfo *getQueueInfo();
    void          setQueueInfo(mmqQueueInfo *aQueueInfo);

    iduListNode  *getQueueListNode();
    /* fix  BUG-27470 The scn and timestamp in the run time header of queue have duplicated objectives. */
    inline void   setQueueSCN(smSCN* aDeqViewSCN);

    void          setQueueWaitTime(ULong aWaitSec);
    ULong         getQueueWaitTime();

    idBool        isQueueReady();
    idBool        isQueueTimedOut();
    //fix BUG-19321
    void          beginQueueWait();
    void          endQueueWait();

    IDE_RC        bookEnqueue(UInt aTableID);
    //proj-1677   Dequeue
    IDE_RC        bookDequeue(UInt aTableID);
    IDE_RC        flushEnqueue(smSCN* aCommitSCN);
    //proj-1677   Dequeue
    IDE_RC        flushDequeue();
    IDE_RC        flushDequeue(smSCN* aCommitSCN);
    IDE_RC        clearEnqueue();
    //proj-1677   Dequeue
    IDE_RC        clearDequeue();
    
public:
    /*
     * XA
     */

    //fix BUG-21794,40772
    IDE_RC          addXid(ID_XID * aXID);
    void            removeXid(ID_XID * aXID);
    inline iduList* getXidList();
    inline ID_XID * getLastXid();
    //fix BUG-20850
    void          saveLocalCommitMode();
    void          restoreLocalCommitMode();
    void          setGlobalCommitMode(mmcCommitMode aCommitMode);
    
    void          saveLocalTrans();
    void          allocLocalTrans();
    void          restoreLocalTrans();
    //fix BUG-21771 
    inline void   setNeedLocalTxBegin(idBool aNeedTransBegin);
    inline idBool getNeedLocalTxBegin();

    /* PROJ-2436 ADO.NET MSDTC */
    inline void   setTransEscalation(mmcTransEscalation aTransEscalation);
    inline mmcTransEscalation getTransEscalation();

public:
    /* PROJ-2177 User Interface - Cancel */
    IDE_RC    putStmtIDMap(mmcStmtCID aStmtCID, mmcStmtID aStmtID);
    mmcStmtID getStmtIDFromMap(mmcStmtCID aStmtCID);
    mmcStmtID removeStmtIDFromMap(mmcStmtCID aStmtCID);

public:
    /*
     * Statistics
     */

    idvSession   *getStatistics();
    idvSQL       *getStatSQL();

    void          applyStatisticsToSystem();

public:
    // PROJ-2118 Bug Reporting
    void                   dumpSessionProperty( ideLogEntry &aLog, mmcSession *aSession );

public:
    /* Callback For SM */
    static ULong           getSessionUpdateMaxLogSizeCallback( idvSQL* aStatistics );
    static IDE_RC          getSessionSqlText( idvSQL * aStatistics,
                                              UChar  * aStrBuffer,
                                              UInt     aStrBufferSize);

    /*
     * Callback from QP
     */

    static const mtlModule *getLanguageCallback(void *aSession);
    static SChar           *getDateFormatCallback(void *aSession);
    static SChar           *getUserNameCallback(void *aSession);
    static SChar           *getUserPasswordCallback(void *aSession);
    static UInt             getUserIDCallback(void *aSession);
    static void             setUserIDCallback(void *aSession, UInt aUserID);
    static UInt             getSessionIDCallback(void *aSession);
    static SChar           *getSessionLoginIPCallback(void *aSession);
    static scSpaceID        getTableSpaceIDCallback(void *aSession);
    static scSpaceID        getTempSpaceIDCallback(void *aSession);
    static idBool           isSysdbaUserCallback(void *aSession);
    static idBool           isBigEndianClientCallback(void *aSession);
    static UInt             getStackSizeCallback(void *aSession);
    /* PROJ-2209 DBTIMEZONE */
    static SLong            getTimezoneSecondCallback( void *aSession );
    static SChar           *getTimezoneStringCallback( void *aSession );
    static UInt             getNormalFormMaximumCallback(void *aSession);
    // BUG-23780 TEMP_TBS_MEMORY 힌트 적용여부를 property로 제공
    static UInt             getOptimizerDefaultTempTbsTypeCallback(void *aSession);   
    static UInt             getOptimizerModeCallback(void *aSession);
    static UInt             getSelectHeaderDisplayCallback(void *aSession);
    static IDE_RC           savepointCallback(void *aSession, const SChar *aSavepoint, idBool aInStoredProc);
    static IDE_RC           commitCallback(void *aSession, idBool aInStoredProc);
    static IDE_RC           rollbackCallback(void *aSession, const SChar *aSavepoint, idBool aInStoredProc);
    static IDE_RC           setReplicationModeCallback(void *aSession, UInt aReplicationMode);
    static IDE_RC           setTXCallback(void *aSession, UInt aType, UInt aValue, idBool aIsSession);
    static IDE_RC           setStackSizeCallback(void *aSession, SInt aStackSize);
    static IDE_RC           setCallback(void *aSession, SChar *aName, SChar *aValue);
    static IDE_RC           setPropertyCallback(void *aSession, SChar *aPropName, UInt aPropNameLen, SChar *aPropValue, UInt aPropValueLen);
    static void             memoryCompactCallback();
    static IDE_RC           printToClientCallback(void *aSession, UChar *aMessage, UInt aMessageLen);
    static IDE_RC           getSvrDNCallback(void *aSession, SChar **aSvrDN, UInt *aSvrDNLen);
    static UInt             getSTObjBufSizeCallback(void *aSession);
    static idBool           isParallelDmlCallback(void *aSession);

    /* BUG-20856 */
    static IDE_RC           commitForceCallback(void * aSession,
                                                SChar * aXIDStr,
                                                UInt    aXIDStrSize);
    static IDE_RC           rollbackForceCallback(void * aSession,
                                                  SChar * aXIDStr,
                                                  UInt    aXIDStrSize);

    // PROJ-1579 NCHAR
    static UInt             getNlsNcharLiteralReplaceCallback(void *aSession);

    //BUG-21122
    static UInt             getAutoRemoteExecCallback(void *aSession);

    // BUG-25999
    static IDE_RC           removeHeuristicXidCallback(void * aSession,
                                              SChar * aXIDStr,
                                              UInt    aXIDStrSize);
                                              
    static UInt             getTrclogDetailPredicateCallback(void *aSession);
    static SInt             getOptimizerDiskIndexCostAdjCallback(void *aSession);
    static SInt             getOptimizerMemoryIndexCostAdjCallback(void *aSession);

    /* PROJ-2109 : Remove the bottleneck of alloc/free stmts. */
    /* Callback function to obtain a mutex from the mutex pool in mmcSession. */
    static IDE_RC          getMutexFromPoolCallback(void      *aSession,
                                                    iduMutex **aMutexObj,
                                                    SChar     *aMutexName);
    /* Callback function to free a mutex from the mutex pool in mmcSession. */
    static IDE_RC          freeMutexFromPoolCallback(void     *aSession,
                                                     iduMutex *aMutexObj );

    /* PROJ-2208 Multi Currency */
    static SChar         * getNlsISOCurrencyCallback( void * aSession );
    static SChar         * getNlsCurrencyCallback( void * aSession );
    static SChar         * getNlsNumCharCallback( void * aSession );

    /* PROJ-2047 Strengthening LOB - LOBCACHE */
    static UInt            getLobCacheThresholdCallback(void *aSession);

    /* PROJ-1090 Function-based Index */
    static UInt            getQueryRewriteEnableCallback(void *aSession);

    static void          * getDatabaseLinkSessionCallback( void * aSession );
    
    static IDE_RC commitForceDatabaseLinkCallback( void * aSession,
                                                   idBool aInStoredProc );
    static IDE_RC rollbackForceDatabaseLinkCallback( void * aSession,
                                                     idBool aInStoredProc );

    /* BUG-38430 */
    static ULong getSessionLastProcessRowCallback( void * aSession );

    /* BUG-38409 autonomous transaction */
    static IDE_RC swapTransaction( void * aUserContext , idBool aIsAT );

    /* PROJ-1812 ROLE */
    static UInt          * getRoleListCallback(void *aSession);
    
    /* PROJ-2441 flashback */
    static UInt getRecyclebinEnableCallback( void *aSession );
    
    /* BUG-42853 LOCK TABLE에 UNTIL NEXT DDL 기능 추가 */
    static idBool getLockTableUntilNextDDLCallback( void * aSession );
    static void   setLockTableUntilNextDDLCallback( void * aSession, idBool aValue );
    static UInt   getTableIDOfLockTableUntilNextDDLCallback( void * aSession );
    static void   setTableIDOfLockTableUntilNextDDLCallback( void * aSession, UInt aValue );

    /* BUG-41511 supporting to similar DBMS_APPLICATION_INFO */
    static IDE_RC setClientAppInfoCallback(void *aSession, SChar *aClientAppInfo, UInt aLength);
    static IDE_RC setModuleInfoCallback(void *aSession, SChar *aModuleInfo, UInt aLength);
    static IDE_RC setActionInfoCallback(void *aSession, SChar *aActionInfo, UInt aLength);

    /* PROJ-2451 Concurrent Execute Package */
    static IDE_RC          allocInternalSession( void ** aMmSession, void * aOrgMmSession );
    static IDE_RC allocInternalSessionWithUserInfo( void ** aMmSession, void * aUserInfo );

    static IDE_RC          freeInternalSession( void * aMmSession, idBool aIsSuccess );
    static smiTrans      * getSessionSmiTrans( void * aMmSession );
    static smiTrans      * getSessionSmiTransWithBegin( void * aMmSession );
    // PROJ-1904 Extend UDT
    static qciSession    * getQciSessionCallback( void * aMmSession );

    /* PROJ-2473 SNMP 지원 */
    inline UInt  getSessionFailureCount();
    inline UInt  addSessionFailureCount();
    inline void  resetSessionFailureCount();

    // BUG-41398 use old sort
    static UInt getUseOldSortCallback( void *aSession );

    // PROJ-2446
    static idvSQL * getStatisticsCallback(void *aSession);

    /* BUG-41452 Built-in functions for getting array binding info.*/
    static IDE_RC getArrayBindInfo( void * aUserContext );

    /* BUG-41561 */
    static UInt getLoginUserIDCallback( void * aMmSession );

    // BUG-41944
    static UInt getArithmeticOpModeCallback( void * aMmSession );

    /* PROJ-2462 Result Cache */
    static UInt getResultCacheEnableCallback( void * aSession );
    static UInt getTopResultCacheModeCallback( void * aSession );
    /* PROJ-2492 Dynamic sample selection */
    static UInt getOptimizerAutoStatsCallback( void * aSession );
    static idBool getIsAutoCommitCallback( void * aSession );

    /* BUG-42134 Created transitivity predicate of join predicate must be reinforced. */
    static UInt getOptimizerTransitivityOldRuleCallback( void * aSession );

    /* BUG-42639 Monitoring query */
    static UInt getOptimizerPerformanceViewCallback( void * aSession );

    /* PROJ-2701 Sharding online data rebuild */
    static idBool isShardUserSessionCallback( void *aSession );
    /* TASK-7219 Analyzer/Transformer/Executor 성능개선 */
    static idBool getCallByShardAnalyzeProtocolCallback( void *aSession );

    /* PROJ-2638 shard native linker */
    static ULong getShardPINCallback( void *aSession );
    static void setShardPINCallback( void *aSession );

    static ULong getShardMetaNumberCallback( void *aSession );
    static SChar * getShardNodeNameCallback( void *aSession );
    static IDE_RC reloadShardMetaNumberCallback( void   *aSession,
                                                 idBool  aIsLocalOnly );

    static sdiSessionType getShardSessionTypeCallback( void *aSession );

    /* BUG-45899 */
    static UInt getTrclogDetailShardCallback( void *aSession );

    static UChar getExplainPlanCallback( void *aSession );

    /* BUG-45844 (Server-Side) (Autocommit Mode) Multi-Transaction을 지원해야 합니다. */
    static UInt getGTXLevelCallback( void * aSession );

    /* PROJ-2677 DDL synchronization */
    static UInt  getReplicationDDLSyncCallback( void *aSession );

    static idBool getTransactionalDDLCallback( void *aSession );
    static idBool getGlobalDDLCallback( void *aSession );

    static UInt  getPrintOutEnableCallback( void *aSession );

    /* BUG-46092 */
    static UInt  isShardCliCallback( void * aSession );
    static void * getShardStmtCallback( void  * aUserContext );
    static void  freeShardStmtCallback( void  * aSession, 
                                        UInt    aNodeId, 
                                        UChar   aMode );
    static UInt   getShardFailoverTypeCallback( void *aSession, UInt aNodeId );

    /* PROJ-2632 */
    static UInt getSerialExecuteModeCallback( void * aSession );
    static UInt getTrcLogDetailInformationCallback( void *aSession );

    /* BUG-47648  disk partition에서 사용되는 prepared memory 사용량 개선 */
    static UInt getReducePartPrepareMemoryCallback( void * aSession );
    
    // PROJ-2727
    static void getSessionPropertyInfoCallback( void   * aSession,
                                                UShort * aSessionPropID,
                                                SChar  **aSessionPropValue,
                                                UInt   * aSessionPropValueLen );

    static UInt    getCommitWriteWaitModeCallback( void *aSession );
    static UInt    getDblinkRemoteStatementAutoCommitCallback( void *aSession );
    static UInt    getDdlTimeoutCallback( void *aSession );
    static UInt    getFetchTimeoutCallback( void *aSession );
    static UInt    getIdleTimeoutCallback( void *aSession );        
    static UInt    getMaxStatementsPerSessionCallback( void *aSession );
    static UInt    getNlsNcharConvExcpCallback( void *aSession );                
    static void    getNlsTerritoryCallback( void *aSession, SChar * aBuffer );
    static UInt    getQueryTimeoutCallback( void *aSession );                
    static UInt    getReplicationDDLSyncTimeoutCallback( void *aSession );                
    static ULong   getUpdateMaxLogSizeCallback( void *aSession );
    static UInt    getUTransTimeoutCallback( void *aSession );
    static UInt    getPropertyAttributeCallback( void *aSession );
    static void    setPropertyAttributeCallback( void *aSession, UInt aValue );    

    /* BUG-48132 */
    static UInt getPlanHashOrSortMethodCallback( void * aSession );

    /* BUG-48161 */
    static UInt getBucketCountMaxCallback( void * aSession );

    /* BUG-48348 */
    static UInt getEliminateCommonSubexpressionCallback( void * aSession );

    // BUG-42464 dbms_alert package
    static IDE_RC           registerCallback( void  * aSession,
                                              SChar * aName,
                                              UShort  aNameSize );     

    static IDE_RC           removeCallback( void * aSession,
                                            SChar * aName,
                                            UShort  aNameSize );     

    static IDE_RC           removeAllCallback( void * aSession );

    static IDE_RC           setDefaultsCallback( void  * aSession,
                                                 SInt    aPollingInterval );

    static IDE_RC           signalCallback( void  * aSession,
                                            SChar * aName,
                                            UShort  aNameSize,
                                            SChar * aMessage,
                                            UShort  aMessageSize );     

    static IDE_RC           waitAnyCallback( void   * aSession,
                                             idvSQL * aStatistics,
                                             SChar  * aName,
                                             UShort * aNameSize,
                                             SChar  * aMessage,  
                                             UShort * aMessageSize,  
                                             SInt   * aStatus,
                                             SInt     aTimeout );

    static IDE_RC           waitOneCallback( void   * aSession,
                                             idvSQL * aStatistics,
                                             SChar  * aName,
                                             UShort * aNameSize,
                                             SChar  * aMessage,
                                             UShort * aMessageSize,
                                             SInt   * aStatus,
                                             SInt     aTimeout );

    /* PROJ-2624 [기능성] MM - 유연한 access_list 관리방법 제공 */
    static IDE_RC           loadAccessListCallback();

    /* BUG-47655 TRANSACTION_TABLE_SIZE 부족 오류 메시지 수정 */
    static void setAllocTransRetryCountCallback( void  * aSession,
                                                 ULong   aRetryCount );
    static void   setShardInPSMEnableCallback( void * aSession, idBool aValue );
    static idBool getShardInPSMEnableCallback( void * aSession );

    /* PROJ-2728 Sharding LOB */
    static UInt   getStmtIdCallback( void * aUserContext );

    static void * findShardStmtCallback( void * aSession,
                                         UInt   aStmtId );

    // BUG-47861 INVOKE_USER_ID, INVOKE_USER_NAME function
    static SChar *getInvokeUserNameCallback(void *aSession);
    static void   setInvokeUserNameCallback(void *aSession, SChar * aInvokeUserName);

    static IDE_RC setInvokeUserPropertyInternalCallback( void  * aSession,
                                                         SChar * aPropName,
                                                         UInt    aPropNameLen,
                                                         SChar * aPropValue,
                                                         UInt    aPropValueLen );
    /* TASK-7219 */
    static void * getPlanStringCallback( void * aUserContext );

    /* PROJ-2733-DistTxInfo */
    static void  getStatementRequestSCNCallback(void *aMmStatement, smSCN *aSCN);
    static void  setStatementRequestSCNCallback(void *aMmStatement, smSCN *aSCN);
    static void  getStatementTxFirstStmtSCNCallback(void *aMmStatement, smSCN *aTxFirstStmtSCN);
    static ULong getStatementTxFirstStmtTimeCallback(void *aMmStatement);
    static sdiDistLevel getStatementDistLevelCallback(void *aMmStatement);
    static idBool isGTxCallback(void *aSession);
    static idBool isGCTxCallback(void *aSession);
    static UChar *getSessionTypeStringCallback(void *aSession);
    static UInt   getShardStatementRetryCallback(void *aSession);
    static UInt   getIndoubtFetchTimeoutCallback(void *aSession);
    static UInt   getIndoubtFetchMethodCallback(void *aSession);

    void getLastSystemSCN( UChar aOpID, smSCN * aLastSystemSCN );

    void setGlobalTransactionLevelFlag();

    static IDE_RC commitInternalCallback( void  * aSession,
                                          void  * aUserContext );

    static void setShardMetaNumberCallback( void  * aSession,
                                            ULong   aSMN );

    static void pauseShareTransFixCallback( void * aMmSession );
    static void resumShareTransFixCallback( void * aMmSession );
    static UInt getShardDDLLockTimeout(void *aSession);
    static UInt getShardDDLLockTryCount(void *aSession);
    static SInt getDDLLockTimeout(void *aSession);

    static void getUserInfoCallback( void * aSession, void * aUserInfo );

    static ULong getLastShardMetaNumberCallback( void * aMmSession );

    static idBool detectShardMetaChangeCallback( void * aMmSession );

    idBool detectShardMetaChange();
    IDE_RC rebuildShardSession( ULong         aTargetSMN,
                                mmcTransObj * aTrans );
    void   cleanupShardRebuildSession();
    IDE_RC propagateRebuildShardMetaNumber();
    IDE_RC propagateShardMetaNumber();
    IDE_RC processShardRetryError( mmcStatement * aStatement,
                                   UInt         * aStmtRetryMax,
                                   UInt         * aRebuildRetryMax );
    void transBeginForGTxEndTran();

    inline IDE_RC rebuildShardSessionBeforeEndTran( mmcTransObj * aTrans );
    inline void   rebuildShardSessionAfterEndTran();

    static UShort getClientTouchNodeCountCallback(void *aSession);  /* BUG-48384 */

    /* TASK-7219 Non-shard DML */
    static void increaseStmtExecSeqForShardTxCallback( void *aSession );
    static void decreaseStmtExecSeqForShardTxCallback( void *aSession );
    static UInt getStmtExecSeqForShardTxCallback( void *aSession );
    static sdiShardPartialExecType getStatementShardPartialExecTypeCallback( void *aMmStatement );

    /* BUG-48770 */
    static UInt checkSessionCountCallback();
private:
    IDE_RC makeShardSession( ULong                   aTargetSMN,
                             ULong                   aLastSessionSMN,
                             smiTrans              * aSmiTrans,
                             idBool                  aIsShardMetaChanged,
                             sdiRebuildPropaOption   aRebuildPropaOpt );

    IDE_RC makeShardSessionWithoutSession( ULong      aTargetSMN,
                                           ULong      aLastSessionSMN,
                                           smiTrans * aSmiTrans,
                                           idBool     aIsShardMetaChanged );
};


inline mmcTask *mmcSession::getTask()
{
    return mInfo.mTask;
}

inline mtlModule *mmcSession::getLanguage()
{
    return mLanguage;
}

inline qciSession *mmcSession::getQciSession()
{
    return &mQciSession;
}

inline mmcStatement *mmcSession::getExecutingStatement()
{
    return mExecutingStatement;
}

inline void mmcSession::setExecutingStatement(mmcStatement *aStatement)
{
    mExecutingStatement = aStatement;
}

inline iduList *mmcSession::getStmtList()
{
    return &mStmtList;
}

inline iduList *mmcSession::getFetchList()
{
    return &mFetchList;
}

inline void mmcSession::lockForStmtList()
{
    // do TASK-3873 code-sonar
    IDE_ASSERT( mStmtListMutex.lock(NULL /* idvSQL* */) == IDE_SUCCESS);
    
}

inline void mmcSession::unlockForStmtList()
{
    // do TASK-3873 code-sonar
    IDE_ASSERT( mStmtListMutex.unlock() == IDE_SUCCESS);
}

/* Transaction accessor functions renewal, by PROJ-2701 */
inline mmcTransObj *mmcSession::allocTrans()
{
    if ( mTrans == NULL )
    {
        IDE_ASSERT(mmcTrans::alloc(this, &mTrans) == IDE_SUCCESS);
        mTransAllocFlag = ID_TRUE;
    }

    return mTrans;
}

inline mmcTransObj *mmcSession::allocTrans(mmcStatement *aStmt)
{
    mmcTransObj *sTrans = NULL;
    if ( getCommitMode() == MMC_COMMITMODE_NONAUTOCOMMIT )
    {
        sTrans = allocTrans();
    }
    else
    {
        sTrans = aStmt->allocTrans();
    }

    return sTrans;
}

inline mmcTransObj *mmcSession::getTransPtr()
{
    return mTrans;
}

inline mmcTransObj *mmcSession::getTransPtr(mmcStatement *aStmt)
{
    mmcTransObj *sTrans = NULL;

    if ( getCommitMode() == MMC_COMMITMODE_NONAUTOCOMMIT )
    {
        sTrans = mTrans;
    }
    else
    {
        /* BUG-46713 codesonar */
        if ( aStmt != NULL )
        {
            sTrans = aStmt->getTransPtr();
        }
        else
        {
            sTrans = NULL;
        }
    }

    return sTrans;
}

inline void mmcSession::setTrans(mmcTransObj *aTrans)
{
    if ((mTrans != NULL) && (mTransAllocFlag == ID_TRUE))
    {
        IDE_ASSERT(mmcTrans::free(this, mTrans) == IDE_SUCCESS);

        mTransAllocFlag = ID_FALSE;
    }

    mTrans = aTrans;

    mTransAllocFlag = ID_TRUE;
}

/*
 * commit 모드 변경시 
 * 기존에 사용하던 공유 가능 트랜잭션 혹은 공유 불가 트랜잭션을 free하고, 
 * 현재 모드에 맞는 신규 트랜잭션을 할당 받는다.
 *
 * 신규 트랜잭션은 기존에 사용하던 트랜잭션과 다른 mempool에서 
 * 할당 받고 다른 형태로 초기화 되어야 한다.
 */
inline void mmcSession::reallocTrans()
{
    if ( mTrans != NULL )
    {
        IDE_DASSERT(mTransAllocFlag == ID_TRUE);
        (void)mmcTrans::free(this, mTrans);
        mTransAllocFlag = ID_FALSE;
        mTrans = NULL;
    }

    IDE_ASSERT(mmcTrans::alloc(this, &mTrans) == IDE_SUCCESS);
    mTransAllocFlag = ID_TRUE;
}

inline mmcSessionInfo *mmcSession::getInfo()
{
    return &mInfo;
}

inline mmcSessID mmcSession::getSessionID()
{
    return mInfo.mSessionID;
}

inline ULong *mmcSession::getEventFlag()
{
    return &mInfo.mEventFlag;
}

inline qciUserInfo *mmcSession::getUserInfo()
{
    return &mInfo.mUserInfo;
}

inline void mmcSession::setUserInfo(qciUserInfo *aUserInfo)
{
    mInfo.mUserInfo = *aUserInfo;
    mInfo.mUserInfo.invokeUserNamePtr = (SChar*)(&mInfo.mUserInfo.loginID);

    (void)dkiSessionSetUserId( &mDatabaseLinkSession, aUserInfo->userID );
}

inline idBool mmcSession::isSysdba()
{
    return mInfo.mUserInfo.mIsSysdba;
}

inline mmcCommitMode mmcSession::getCommitMode()
{
    return mInfo.mCommitMode;
}

inline UChar mmcSession::getExplainPlan()
{
    return mInfo.mExplainPlan;
}

inline void mmcSession::setExplainPlan(UChar aExplainPlan)
{
    mInfo.mExplainPlan = aExplainPlan;

    /* PROJ-2638 shard native linker */
    sdi::setExplainPlanAttr( &mQciSession, aExplainPlan );
}

// BUG-15396 수정 시, 추가되었음
inline UInt mmcSession::getReplicationMode()
{
    return mInfo.mReplicationMode;
}

// BUG-15396 수정 시, 추가되었음
inline UInt mmcSession::getTransactionMode()
{
    return mInfo.mTransactionMode;
}

inline SChar * mmcSession::getNlsCurrency()
{
    return mInfo.mNlsCurrency;
}

inline SChar * mmcSession::getNlsNumChar()
{
    return mInfo.mNlsNumChar;
}

/**********************************************************************
    BUG-15396
    alter session commit write wait/nowait;
    commit 시에 log가 disk에 기록될때까지 기다릴지, 바로 반환할지에
    대한 정보
**********************************************************************/

inline idBool mmcSession::getCommitWriteWaitMode()
{
    return mInfo.mCommitWriteWaitMode;
}

inline void mmcSession::setCommitWriteWaitMode( idBool aCommitWriteWaitMode )
{
    mInfo.mCommitWriteWaitMode = aCommitWriteWaitMode;
}

inline UInt mmcSession::getIsolationLevel()
{
    return mInfo.mIsolationLevel;
}

inline void mmcSession::setIsolationLevel(UInt aIsolationLevel)
{
    mInfo.mIsolationLevel = aIsolationLevel;
}

inline idBool mmcSession::isReadOnlySession()
{
    return (getTransactionMode() & SMI_TRANSACTION_UNTOUCHABLE) != 0 ? ID_TRUE : ID_FALSE;
}

inline void mmcSession::setUpdateMaxLogSize( ULong aUpdateMaxLogSize )
{
    mInfo.mUpdateMaxLogSize = aUpdateMaxLogSize;
}

inline ULong mmcSession::getUpdateMaxLogSize()
{
    return mInfo.mUpdateMaxLogSize;
}

// non auto commit 일 경우에만 호출됨
inline idBool mmcSession::isReadOnlyTransaction()
{
    return (getTxTransactionMode(mTrans) & SMI_TRANSACTION_UNTOUCHABLE) != 0 ? ID_TRUE : ID_FALSE;
}

inline UInt mmcSession::getOptimizerMode()
{
    return mInfo.mOptimizerMode;
}

inline void mmcSession::setOptimizerMode(UInt aMode)
{
    mInfo.mOptimizerMode = (aMode == 0) ? 0 : 1;
}

inline UInt mmcSession::getHeaderDisplayMode()
{
    return mInfo.mHeaderDisplayMode;
}

inline void mmcSession::setHeaderDisplayMode(UInt aMode)
{
    mInfo.mHeaderDisplayMode = (aMode == 0) ? 0 : 1;
}

inline UInt mmcSession::getStackSize()
{
    return mInfo.mStackSize;
}

/* PROJ-2209 DBTIMEZONE */
inline SLong mmcSession::getTimezoneSecond()
{
    return mInfo.mTimezoneSecond;
}

inline SChar *mmcSession::getTimezoneString()
{
    return mInfo.mTimezoneString;
}

inline UInt mmcSession::getNormalFormMaximum()
{
    return mInfo.mNormalFormMaximum;
}

inline void mmcSession::setNormalFormMaximum(UInt aMaximum)
{
    mInfo.mNormalFormMaximum = aMaximum;
}

// BUG-23780 TEMP_TBS_MEMORY 힌트 적용여부를 property로 제공
inline UInt mmcSession::getOptimizerDefaultTempTbsType()
{
    return mInfo.mOptimizerDefaultTempTbsType;    
}

inline void mmcSession::setOptimizerDefaultTempTbsType(UInt aValue)
{
    mInfo.mOptimizerDefaultTempTbsType = aValue;    
}

inline UInt mmcSession::getIdleTimeout()
{
    return mInfo.mIdleTimeout;
}

inline void mmcSession::setIdleTimeout(UInt aValue)
{
    mInfo.mIdleTimeout = aValue;
}

inline UInt mmcSession::getQueryTimeout()
{
    return mInfo.mQueryTimeout;
}

inline void mmcSession::setQueryTimeout(UInt aValue)
{
    mInfo.mQueryTimeout = aValue;
}

/* BUG-32885 Timeout for DDL must be distinct to query_timeout or utrans_timeout */
inline UInt mmcSession::getDdlTimeout()
{
    return mInfo.mDdlTimeout;
}

inline void mmcSession::setDdlTimeout(UInt aValue)
{
    mInfo.mDdlTimeout = aValue;
}

inline UInt mmcSession::getFetchTimeout()
{
    return mInfo.mFetchTimeout;
}

inline void mmcSession::setFetchTimeout(UInt aValue)
{
    mInfo.mFetchTimeout = aValue;
}

inline UInt mmcSession::getUTransTimeout()
{
    return mInfo.mUTransTimeout;
}

inline void mmcSession::setUTransTimeout(UInt aValue)
{
    mInfo.mUTransTimeout = aValue;
}

inline SChar *mmcSession::getDateFormat()
{
    return mInfo.mDateFormat;
}

inline mmcByteOrder mmcSession::getClientHostByteOrder()
{
    return mInfo.mClientHostByteOrder;
}

inline void mmcSession::setClientHostByteOrder(mmcByteOrder aByteOrder)
{
    mInfo.mClientHostByteOrder = aByteOrder;
}

inline mmcByteOrder mmcSession::getNumericByteOrder()
{
    return mInfo.mNumericByteOrder;
}

inline void mmcSession::setNumericByteOrder(mmcByteOrder aByteOrder)
{
    mInfo.mNumericByteOrder = aByteOrder;
}

inline mmcSessionState mmcSession::getSessionState()
{
    return mInfo.mSessionState;
}

inline void mmcSession::setSessionState(mmcSessionState aState)
{
    mInfo.mSessionState = aState;
}

inline UInt mmcSession::getIdleStartTime()
{
    return mInfo.mIdleStartTime;
}

inline void mmcSession::setIdleStartTime(UInt aIdleStartTime)
{
    mInfo.mIdleStartTime = aIdleStartTime;
}

inline idBool mmcSession::isActivated()
{
    return mInfo.mActivated;
}

inline void mmcSession::setActivated(idBool aActivated)
{
    mInfo.mActivated = aActivated;
}

inline idBool mmcSession::isAllStmtEnd()
{
    return (mInfo.mOpenStmtCount == 0 ? ID_TRUE : ID_FALSE);
}

inline void mmcSession::changeOpenStmt(SInt aCount)
{
    mInfo.mOpenStmtCount += aCount;
}

inline idBool mmcSession::isXaSession()
{
    return mInfo.mXaSessionFlag;
}

inline void mmcSession::setXaSession(idBool aFlag)
{
    mInfo.mXaSessionFlag = aFlag;
}

inline mmdXaAssocState mmcSession::getXaAssocState()
{
    return mInfo.mXaAssocState;
}

inline void mmcSession::setXaAssocState(mmdXaAssocState aState)
{
    mInfo.mXaAssocState = aState;
}

inline mmqQueueInfo *mmcSession::getQueueInfo()
{
    return mQueueInfo;
}

inline void mmcSession::setQueueInfo(mmqQueueInfo *aQueueInfo)
{
    mQueueInfo = aQueueInfo;
}

inline iduListNode *mmcSession::getQueueListNode()
{
    return &mQueueListNode;
}


//PROJ-1677 DEQ
/* fix  BUG-27470 The scn and timestamp in the run time header of queue have duplicated objectives. */
inline void mmcSession::setQueueSCN(smSCN * aDeqViewSCN)
{
    SM_SET_SCN(&mDeqViewSCN,aDeqViewSCN)
}

inline void mmcSession::setQueueWaitTime(ULong aWaitMicroSec)
{
    PDL_Time_Value sWaitTime;

    mQueueWaitTime = aWaitMicroSec;

    if (aWaitMicroSec == 0)
    {
        mQueueEndTime = 0;
    }
    else if (aWaitMicroSec == ID_ULONG_MAX)
    {
        mQueueEndTime = -1;
    }
    else
    {
        /* BUG-46183 */
        sWaitTime.initialize(0, aWaitMicroSec);
        sWaitTime += idlOS::gettimeofday();
        mNeedQueueWait = ID_TRUE;

        mQueueEndTime = sWaitTime.microsec();
    }
}

//BUG-21122
inline void mmcSession::setAutoRemoteExec(UInt aValue)
{
    mInfo.mAutoRemoteExec = aValue;
}

inline UInt mmcSession::getAutoRemoteExec()
{
    return mInfo.mAutoRemoteExec;
}

inline ULong mmcSession::getQueueWaitTime()
{
    return mQueueWaitTime;
}

inline idBool mmcSession::isQueueReady()
{
    if (mQueueInfo != NULL)
    {
        //PROJ-1677
        /* fix  BUG-27470 The scn and timestamp in the run time header of queue have duplicated objectives. */
        return mQueueInfo->isQueueReady4Session(&mDeqViewSCN);
    }
    else
    {
        return ID_FALSE;
    }
}

inline idBool mmcSession::isQueueTimedOut()
{
    PDL_Time_Value sCurTime;

    if (mQueueInfo != NULL)
    {
        if (mQueueEndTime == 0)
        {
            return ID_TRUE;
        }
        else if (mQueueEndTime == -1)
        {
            return ID_FALSE;
        }
        else
        {
            sCurTime = idlOS::gettimeofday();

            /* BUG-46183 u 단위의 짧은 시간은 Queue 구조상 대기하지 않아 mNeedQueueWait flag를 둔다. */
            if (mQueueEndTime <= sCurTime.microsec())
            {
                if (mNeedQueueWait == ID_TRUE)
                {
                    mNeedQueueWait = ID_FALSE;
                    return ID_FALSE;
                }
                else
                {
                    return ID_TRUE;
                }
            }
            else
            {
                mNeedQueueWait = ID_FALSE;
                return ID_FALSE;
            }
        }
    }

    return ID_FALSE;
}

//fix BUG-20850
inline void mmcSession::saveLocalCommitMode()
{
    mLocalCommitMode = getCommitMode();
}

inline void mmcSession::restoreLocalCommitMode()
{
    mInfo.mCommitMode =  mLocalCommitMode;
}

inline void mmcSession::setGlobalCommitMode(mmcCommitMode aCommitMode)
{
    mInfo.mCommitMode = aCommitMode;
}

inline void mmcSession::saveLocalTrans()
{
    mLocalTrans = getTransPtr();
    mLocalTransBegin = getSessionBegin();
    mTransAllocFlag = ID_FALSE;
    setSessionBegin( ID_FALSE );
    mTrans = NULL;
}

inline void mmcSession::allocLocalTrans()
{
    if (mLocalTrans == NULL)
    {
        /* currently not used: need remove BUGBUG */
        IDE_ASSERT(mmcTrans::alloc( this, &mLocalTrans ) == IDE_SUCCESS);
    }
    else
    {
        /* already allocated */
    }
}

inline void mmcSession::restoreLocalTrans()
{
    
    mTrans = mLocalTrans;
    setSessionBegin( mLocalTransBegin );
    /* fix BUG-31002, Remove unnecessary code dependency from the inline function of mm module. */
    if(mLocalTrans != NULL)
    {
        mTransAllocFlag = ID_TRUE;
    }
    mLocalTrans = NULL;
    mLocalTransBegin = ID_FALSE;
}

inline idvSession *mmcSession::getStatistics()
{
    return &mStatistics;
}

inline idvSQL *mmcSession::getStatSQL()
{
    return &mStatSQL;
}

inline void mmcSession::applyStatisticsToSystem()
{
    idvManager::applyStatisticsToSystem(&mStatistics, &mOldStatistics);
}


/*******************************************************************
 BUG-15396

 Description : transaction시에 필요한 session 정보를
               smiTrans.flag 값으로 설정하여 반환

 Implementaion : Transaction 시작 시에 넘겨줘야 할 flag 정보를
                 Oring 하여 반환
********************************************************************/
inline UInt mmcSession::getSessionInfoFlagForTx()
{
    UInt sFlag = 0;

    // isolation level, replication mode, transaction mode 모두
    // smiTrans.flag 값과 session의 값과 동일
    sFlag = getIsolationLevel() | getReplicationMode() | getTransactionMode();

    // BUG-17878 : commit write wait mode를 bool type으로 변경
    // smiTrans.flag값과 session의 commit write wait mode 값이 다름
    if ( getCommitWriteWaitMode() == ID_TRUE )
    {
        sFlag &= ~SMI_COMMIT_WRITE_MASK;
        sFlag |= SMI_COMMIT_WRITE_WAIT;
    }
    else
    {
        sFlag &= ~SMI_COMMIT_WRITE_MASK;
        sFlag |= SMI_COMMIT_WRITE_NOWAIT;
    }

    /* PROJ-2733-DistTxInfo Set GLOBAL_TRANSACTION_LEVEL */
    if ( isGCTx() == ID_TRUE )
    {
        sFlag &= ~SMI_TRANS_GCTX_MASK;
        sFlag |= SMI_TRANS_GCTX_ON;
    }
    else
    {
        sFlag &= ~SMI_TRANS_GCTX_MASK;
        sFlag |= SMI_TRANS_GCTX_OFF;
    }

    return sFlag;
}

/*******************************************************************
 BUG-15396
 Description : transaction의 Isolation Level을 반환
 Implementaion : Transaction 으로부터 isolation level을 받아
                 이를 반환
********************************************************************/
inline UInt mmcSession::getTxIsolationLevel(mmcTransObj * aTrans)
{
    IDE_ASSERT( aTrans != NULL );
    return mmcTrans::getSmiTrans(aTrans)->getIsolationLevel();
}

/*******************************************************************
 BUG-15396
 Description : transaction의 transaction mode를 반환
 Implementaion : Transaction 으로부터 transaction mode를 받아
                 이를 반환
********************************************************************/
inline UInt mmcSession::getTxTransactionMode(mmcTransObj * aTrans)
{
    IDE_ASSERT( aTrans != NULL );

    return mmcTrans::getSmiTrans(aTrans)->getTransactionMode();
}
//PROJ-1677 DEQUEUE
inline void  mmcSession::clearPartialRollbackFlag()
{
    mPartialRollbackFlag = MMC_DID_NOT_PARTIAL_ROLLBACK;
}
//PROJ-1677 DEQUEUE
inline void  mmcSession::setPartialRollbackFlag()
{
    mPartialRollbackFlag = MMC_DID_PARTIAL_ROLLBACK;
}
//PROJ-1677 DEQUEUE
inline mmcPartialRollbackFlag   mmcSession::getPartialRollbackFlag()
{
     return mPartialRollbackFlag;
}
//PROJ-1677 DEQUEUE
inline void   mmcSession::getDeqViewSCN(smSCN * aDeqViewSCN)
{
    SM_SET_SCN(aDeqViewSCN,&mDeqViewSCN);
}

/*******************************************************************
 PROJ-1665
 Description : Parallel DML Mode 반환
********************************************************************/
inline idBool mmcSession::getParallelDmlMode()
{
    return mInfo.mParallelDmlMode;
}

/*******************************************************************
 PROJ-1579 NCHAR
 Description : NLS_NCHAR_CONV_EXCP 프로퍼티
********************************************************************/
inline UInt   mmcSession::getNlsNcharConvExcp()
{
    return mInfo.mNlsNcharConvExcp;
}
inline void mmcSession::setNlsNcharConvExcp(UInt aConvExcp)
{
    mInfo.mNlsNcharConvExcp = (aConvExcp == 0) ? 0 : 1;
}

/*******************************************************************
 PROJ-1579 NCHAR
 Description : NLS_NCHAR_LITERAL_REPLACE 프로퍼티
********************************************************************/
inline UInt   mmcSession::getNlsNcharLiteralReplace()
{
    return mInfo.mNlsNcharLiteralReplace;
}

inline UChar *mmcSession::getChunk()
{
    return mChunk;
}

inline IDE_RC mmcSession::allocChunk(UInt aAllocSize)
{
    if( aAllocSize > mChunkSize )
    {
        if( mChunk != NULL )
        {
            IDE_DASSERT( mChunkSize != 0 );
            IDE_TEST( iduMemMgr::free( mChunk ) != IDE_SUCCESS );
        }

        IDU_FIT_POINT_RAISE( "mmcSession::allocChunk::malloc::Chunk",
                              InsufficientMemory );

        IDE_TEST_RAISE( iduMemMgr::malloc( IDU_MEM_MMC,
                                           aAllocSize,
                                           (void**)&mChunk )
                        != IDE_SUCCESS, InsufficientMemory );
        mChunkSize = aAllocSize;
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION(InsufficientMemory);
    {
        /* BUG-42755 Prevent dangling pointer */
        mChunk     = NULL;
        mChunkSize = 0;

        IDE_SET(ideSetErrorCode(idERR_ABORT_InsufficientMemory));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

inline IDE_RC mmcSession::allocChunk4Fetch(UInt aAllocSize)
{
    IDE_TEST( allocChunk( aAllocSize * 2 ) != IDE_SUCCESS );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

inline idBool mmcSession::getHasClientListChannel()
{
    return mInfo.mHasClientListChannel;
}

// BUG-34725 
inline UInt mmcSession::getFetchProtocolType()
{
    return mInfo.mFetchProtocolType;
}

inline UInt mmcSession::getChunkSize()
{
    return mChunkSize;
}

inline UInt mmcSession::getFetchChunkLimit()
{
    return mChunkSize / 2;
}

/*******************************************************************
 PROJ-2160 CM 타입제거
 Description : outParam 시에 사용된다.
********************************************************************/
inline UChar *mmcSession::getOutChunk()
{
    return mOutChunk;
}

inline IDE_RC mmcSession::allocOutChunk(UInt aAllocSize)
{
    if( aAllocSize > mOutChunkSize )
    {
        if( mOutChunk != NULL )
        {
            IDE_DASSERT( mOutChunkSize != 0 );
            IDE_TEST( iduMemMgr::free( mOutChunk ) != IDE_SUCCESS );
        }

        IDU_FIT_POINT_RAISE( "mmcSession::allocOutChunk::malloc::OutChunk",
                              InsufficientMemory );

        IDE_TEST_RAISE( iduMemMgr::malloc( IDU_MEM_MMC,
                                           aAllocSize,
                                           (void**)&mOutChunk )
                        != IDE_SUCCESS, InsufficientMemory );

        mOutChunkSize = aAllocSize;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(InsufficientMemory);
    {
        /* BUG-42755 Prevent dangling pointer */
        mOutChunk     = NULL;
        mOutChunkSize = 0;

        IDE_SET(ideSetErrorCode(idERR_ABORT_InsufficientMemory));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

inline UInt mmcSession::getOutChunkSize()
{
    return mOutChunkSize;
}

inline void mmcSession::setNeedLocalTxBegin(idBool aNeedLocalTxBegin)
{
    mNeedLocalTxBegin    = aNeedLocalTxBegin;
}

inline idBool mmcSession::getNeedLocalTxBegin()
{
    return mNeedLocalTxBegin;
}

inline void mmcSession::setTransEscalation(mmcTransEscalation aTransEscalation)
{
    mTransEscalation = aTransEscalation;
}

inline mmcTransEscalation mmcSession::getTransEscalation()
{
    return mTransEscalation;
}

//fix BUG-21794
inline iduList* mmcSession::getXidList()
{
    return &mXidLst;    
}

inline ID_XID * mmcSession::getLastXid()
{
    mmdIdXidNode     *sLastXidNode;
    iduListNode     *sIterator;
    
    sIterator = IDU_LIST_GET_LAST(&mXidLst);
    sLastXidNode = (mmdIdXidNode*) sIterator->mObj;
    return  &(sLastXidNode->mXID);
}

/* BUG-31144 */
inline UInt mmcSession::getNumberOfStatementsInSession()
{
        return mInfo.mNumberOfStatementsInSession;
}

inline void mmcSession::setNumberOfStatementsInSession(UInt aValue)
{
        mInfo.mNumberOfStatementsInSession = aValue;
}

inline UInt mmcSession::getMaxStatementsPerSession()
{
        return mInfo.mMaxStatementsPerSession;
}

inline void mmcSession::setMaxStatementsPerSession(UInt aValue)
{
        mInfo.mMaxStatementsPerSession = aValue;
}

inline UInt mmcSession::getTrclogDetailPredicate()
{
    return mInfo.mTrclogDetailPredicate;
}

inline void mmcSession::setTrclogDetailPredicate(UInt aTrclogDetailPredicate)
{
        mInfo.mTrclogDetailPredicate = aTrclogDetailPredicate;
}

inline SInt mmcSession::getOptimizerDiskIndexCostAdj()
{
        return mInfo.mOptimizerDiskIndexCostAdj;
}

inline void mmcSession::setOptimizerDiskIndexCostAdj(SInt aOptimizerDiskIndexCostAdj)
{
        mInfo.mOptimizerDiskIndexCostAdj = aOptimizerDiskIndexCostAdj;
}

// BUG-43736
inline SInt mmcSession::getOptimizerMemoryIndexCostAdj()
{
        return mInfo.mOptimizerMemoryIndexCostAdj;
}

inline void mmcSession::setOptimizerMemoryIndexCostAdj(SInt aOptimizerMemoryIndexCostAdj)
{
        mInfo.mOptimizerMemoryIndexCostAdj = aOptimizerMemoryIndexCostAdj;
}

// PROJ-2256 Communication protocol for efficient query result transmission
inline UInt mmcSession::getRemoveRedundantTransmission()
{
    return mInfo.mRemoveRedundantTransmission;
}

/* PROJ-2109 : Remove the bottleneck of alloc/free stmts. */
inline mmcMutexPool *mmcSession::getMutexPool()
{
    return &mMutexPool;
}

/* PROJ-2047 Strengthening LOB - LOBCACHE */
inline UInt mmcSession::getLobCacheThreshold()
{
    return mInfo.mLobCacheThreshold;
}

inline void mmcSession::setLobCacheThreshold(UInt aValue)
{
    mInfo.mLobCacheThreshold = aValue;
}

/* PROJ-1090 Function-based Index */
inline UInt mmcSession::getQueryRewriteEnable()
{
    return mInfo.mQueryRewriteEnable;
}

inline void mmcSession::setQueryRewriteEnable(UInt aValue)
{
    mInfo.mQueryRewriteEnable = aValue;
}

/* PROJ-2441 flashback */
inline UInt mmcSession::getRecyclebinEnable()
{
    return mInfo.mRecyclebinEnable;
}

inline void mmcSession::setRecyclebinEnable(UInt aValue)
{
    mInfo.mRecyclebinEnable = aValue;
}

/* BUG-42853 LOCK TABLE에 UNTIL NEXT DDL 기능 추가 */
inline idBool mmcSession::getLockTableUntilNextDDL()
{
    return mInfo.mLockTableUntilNextDDL;
}

inline void mmcSession::setLockTableUntilNextDDL( idBool aValue )
{
    mInfo.mLockTableUntilNextDDL = aValue;
}

inline UInt mmcSession::getTableIDOfLockTableUntilNextDDL()
{
    return mInfo.mTableIDOfLockTableUntilNextDDL;
}

inline void mmcSession::setTableIDOfLockTableUntilNextDDL( UInt aValue )
{
    mInfo.mTableIDOfLockTableUntilNextDDL = aValue;
}

// BUG-41398 use old sort
inline UInt mmcSession::getUseOldSort()
{
    return mInfo.mUseOldSort;
}

inline void mmcSession::setUseOldSort(UInt aValue)
{
    mInfo.mUseOldSort = aValue;
}

// BUG-41944
inline UInt mmcSession::getArithmeticOpMode()
{
    return mInfo.mArithmeticOpMode;
}

inline void mmcSession::setArithmeticOpMode(UInt aValue)
{
    mInfo.mArithmeticOpMode = aValue;
}

/* PROJ-2462 Result Cache */
inline UInt mmcSession::getResultCacheEnable()
{
    return mInfo.mResultCacheEnable;
}
inline void mmcSession::setResultCacheEnable(UInt aValue)
{
    mInfo.mResultCacheEnable = aValue;
}

/* PROJ-2462 Result Cache */
inline UInt mmcSession::getTopResultCacheMode()
{
    return mInfo.mTopResultCacheMode;
}
inline void mmcSession::setTopResultCacheMode(UInt aValue)
{
    mInfo.mTopResultCacheMode = aValue;
}

/* PROJ-2492 Dynamic sample selection */
inline UInt mmcSession::getOptimizerAutoStats()
{
    return mInfo.mOptimizerAutoStats;
}
inline void mmcSession::setOptimizerAutoStats(UInt aValue)
{
    mInfo.mOptimizerAutoStats = aValue;
}

/* PROJ-2462 Result Cache */
inline idBool mmcSession::isAutoCommit()
{
    idBool sIsAutoCommit = ID_FALSE;

    if ( mInfo.mCommitMode == MMC_COMMITMODE_NONAUTOCOMMIT )
    {
        sIsAutoCommit = ID_FALSE;
    }
    else
    {
        sIsAutoCommit = ID_TRUE;
    }

    return sIsAutoCommit;
}

/* BUG-42134 Created transitivity predicate of join predicate must be reinforced. */
inline UInt mmcSession::getOptimizerTransitivityOldRule()
{
    return mInfo.mOptimizerTransitivityOldRule;
}

/* BUG-42134 Created transitivity predicate of join predicate must be reinforced. */
inline void mmcSession::setOptimizerTransitivityOldRule(UInt aValue)
{
    mInfo.mOptimizerTransitivityOldRule = aValue;
}

/* BUG-42639 Monitoring query */
inline UInt mmcSession::getOptimizerPerformanceView()
{
    return mInfo.mOptimizerPerformanceView;
}

/* BUG-42639 Monitoring query */
inline void mmcSession::setOptimizerPerformanceView(UInt aValue)
{
    mInfo.mOptimizerPerformanceView = aValue;
}

/*******************************************************************
  PROJ-2118 Bug Reporting
  Description : Dump Session Properties
 *******************************************************************/
inline void mmcSession::dumpSessionProperty( ideLogEntry &aLog, mmcSession *aSession )
{
    mmcSessionInfo  *sInfo;

    sInfo = aSession->getInfo();

    aLog.append( "*----------------------------------------*\n" );
    aLog.appendFormat( "*        Session[%.4u] properties        *\n", sInfo->mSessionID );
    aLog.append( "*----------------------------------------*\n" );
    aLog.appendFormat( "%-22s : %u\n", "OPTIMIZER_MODE", sInfo->mOptimizerMode );
    aLog.appendFormat( "%-22s : %llu\n", "TRX_UPDATE_MAX_LOGSIZE", sInfo->mUpdateMaxLogSize );
    aLog.appendFormat( "%-22s : %u\n", "NLS_NCHAR_CONV_EXCP", sInfo->mNlsNcharConvExcp );
    aLog.appendFormat( "%-22s : %u\n", "FETCH_TIMEOUT", sInfo->mFetchTimeout );
    aLog.appendFormat( "%-22s : %u\n", "IDLE_TIMEOUT", sInfo->mIdleTimeout );
    aLog.appendFormat( "%-22s : %u\n", "QUERY_TIMEOUT", sInfo->mQueryTimeout );
    aLog.appendFormat( "%-22s : %u\n", "DDL_TIMEOUT", sInfo->mDdlTimeout );
    aLog.appendFormat( "%-22s : %u\n", "UTRANS_TIMEOUT", sInfo->mUTransTimeout );
    aLog.appendFormat( "%-22s : %u\n", "AUTO_COMMIT", sInfo->mCommitMode );
    aLog.appendFormat( "%-22s : %u\n\n\n", "COMMIT_WRITE_WAIT_MODE", sInfo->mCommitWriteWaitMode );
}

/* PROJ-1381 Fetch Across Commits */

/**
 * Commit된 FetchList를 얻는다.
 *
 * @return Commit된 FetchList
 */
inline iduList* mmcSession::getCommitedFetchList(void)
{
    return &mCommitedFetchList;
}

/**
 * FetchList, CommitedFetchList 변경을 위해 lock을 잡는다.
 */
inline void mmcSession::lockForFetchList(void)
{
    IDE_ASSERT( mFetchListMutex.lock(NULL /* idvSQL* */) == IDE_SUCCESS);
}

/**
 * FetchList, CommitedFetchList 변경을 위해 잡은 lock 푼다.
 */
inline void mmcSession::unlockForFetchList(void)
{
    IDE_ASSERT( mFetchListMutex.unlock() == IDE_SUCCESS);
}

/**
 * Holdable Fetch로 start된 Stmt 수를 조정한다.
 *
 * @param aCount[IN] 조정할 값. 수를 늘리려면 양수, 줄이려면 음수 사용.
 */
inline void mmcSession::changeHoldFetch(SInt aCount)
{
    mInfo.mOpenHoldFetchCount += aCount;
}

/**
 * Holdable Fetch를 제외한 Stmt가 모두 end 됐는지 확인한다.
 *
 * @return Holdable Fetch를 제외한 Stmt가 모두 end 됐으면 ID_TRUE, 아니면 ID_FALSE
 */
inline idBool mmcSession::isAllStmtEndExceptHold(void)
{
    return ((mInfo.mOpenStmtCount - mInfo.mOpenHoldFetchCount) == 0 ? ID_TRUE : ID_FALSE);
}

/* PROJ-2473 SNMP 지원 */
inline UInt mmcSession::getSessionFailureCount()
{
    return mInfo.mSessionFailureCount;
}

inline UInt mmcSession::addSessionFailureCount()
{
    mInfo.mSessionFailureCount += 1;

    return mInfo.mSessionFailureCount;
}

inline void mmcSession::resetSessionFailureCount()
{
    mInfo.mSessionFailureCount = 0;
}

/* PROJ-2677 DDL synchronization */
inline UInt mmcSession::getReplicationDDLSync()
{
    return mInfo.mReplicationDDLSync;
}

inline void mmcSession::setReplicationDDLSync( UInt aValue )
{
    mInfo.mReplicationDDLSync = aValue;
}

inline UInt mmcSession::getReplicationDDLSyncTimeout()
{
    return mInfo.mReplicationDDLSyncTimeout;
}

inline void mmcSession::setReplicationDDLSyncTimeout( UInt aValue )
{
    mInfo.mReplicationDDLSyncTimeout = aValue;
}

inline idBool mmcSession::getTransactionalDDL()
{
    return mInfo.mTransactionalDDL;
}

inline idBool mmcSession::getGlobalDDL()
{
    return mInfo.mGlobalDDL;
}

inline idBool mmcSession::isDDLAutoCommit()
{
    idBool sDDLAutoCommit = ID_TRUE;

    if ( getCommitMode() == MMC_COMMITMODE_NONAUTOCOMMIT )
    {    
        if (( getTransactionalDDL() == ID_TRUE ) &&
            (( mQciSession.mQPSpecific.mFlag & QC_SESSION_SHARD_DDL_MASK ) !=
             QC_SESSION_SHARD_DDL_TRUE ) &&
            ( globalDDLUserSession() != ID_TRUE ))
        {
            sDDLAutoCommit = ID_FALSE;
        }
    }

    return sDDLAutoCommit;
}

inline idBool mmcSession::globalDDLUserSession()
{
    idBool sDDLUserSession = ID_FALSE;

    if ( ( SDU_SHARD_ENABLE == 1 ) &&
         ( getGlobalDDL() == ID_TRUE ) &&
         ( isShardUserSession() == ID_TRUE ) )
    {
        sDDLUserSession = ID_TRUE;
    }

    return sDDLUserSession;
}

/**
 * PROJ-2626 Snapshot Export
 * 현재 세션의 ClientAppInfoType을 반환한다.
 */
inline mmcClientAppInfoType mmcSession::getClientAppInfoType( void )
{
    return mInfo.mClientAppInfoType;
}

inline sdiShardPin mmcSession::getShardPIN()
{
    return mInfo.mShardPin;
}

inline void mmcSession::setShardPIN( sdiShardPin aShardPin )
{
    mInfo.mShardPin = aShardPin;
}

inline idBool mmcSession::isMetaNodeShardCli()
{
    return ( ( isShardUserSession() == ID_TRUE )
             && ( isShardClient() == ID_TRUE ) )
           ? ID_TRUE : ID_FALSE;
}

inline ULong mmcSession::getShardMetaNumber()
{
    return mInfo.mShardMetaNumber;
}

inline void mmcSession::setShardMetaNumber( ULong aSMN )
{
    mInfo.mShardMetaNumber = aSMN;
}

inline ULong mmcSession::getLastShardMetaNumber()
{
    return mInfo.mLastShardMetaNumber;
}

inline void mmcSession::setLastShardMetaNumber(ULong aSMN)
{
    if ( ( mInfo.mLastShardMetaNumber == SDI_NULL_SMN ) ||
         ( mInfo.mLastShardMetaNumber == mInfo.mShardMetaNumber ) )
    {
        mInfo.mLastShardMetaNumber = aSMN;
    }
    else
    {
        /* Nothing to do */
    }
}

inline void mmcSession::clearLastShardMetaNumber()
{
    mInfo.mLastShardMetaNumber = mInfo.mShardMetaNumber;
}

inline idBool mmcSession::isReshardOccurred()
{
    idBool sRet = ID_FALSE;

    sRet = sdi::isReshardOccurred( mInfo.mShardMetaNumber, mInfo.mLastShardMetaNumber );

    return sRet;
}

inline ULong mmcSession::getReceivedShardMetaNumber()
{
    return mInfo.mReceivedShardMetaNumber;
}

inline void mmcSession::setReceivedShardMetaNumber( ULong aSMN )
{
    mInfo.mReceivedShardMetaNumber = aSMN;
}

inline idBool mmcSession::isNeedRebuildNoti()
{
    idBool sRet = ID_FALSE;

    if ( isShardClient() == SDI_SHARD_CLIENT_TRUE )
    {
        if ( mInfo.mShardMetaNumber > mInfo.mReceivedShardMetaNumber )
        {
            sRet = ID_TRUE;
        }
        else
        {
            IDE_DASSERT( mInfo.mShardMetaNumber == mInfo.mReceivedShardMetaNumber );
        }
    }

    return sRet;
}

inline SChar *mmcSession::getShardNodeName()
{
    return mInfo.mShardNodeName;
}

inline idBool mmcSession::isShardUserSession()
{
    idBool sResult = ID_FALSE;

    if ( mInfo.mShardSessionType == SDI_SESSION_TYPE_USER )
    {
        sResult = ID_TRUE;
    }
    return sResult;
}

inline idBool mmcSession::isShardCoordinatorSession()
{
    idBool sResult = ID_FALSE;

    if ( mInfo.mShardSessionType == SDI_SESSION_TYPE_COORD )
    {
        sResult = ID_TRUE;
    }
    return sResult;
}

/* TASK-7219 Analyzer/Transformer/Executor 성능개선 */
inline idBool mmcSession::getCallByShardAnalyzeProtocol()
{
    return mInfo.mCallByShardAnalyzeProtocol;
}

inline void mmcSession::setCallByShardAnalyzeProtocol( idBool aCallByShardAnalyzeProtocol )
{
    mInfo.mCallByShardAnalyzeProtocol = aCallByShardAnalyzeProtocol;
}

/*
 * PROJ-2660 hybrid sharding
 * (변경전)data node에서 shard pin이 같은 세션은 tx를 공유할 수 있다.
 *
 * PROJ-2701 Sharding online data rebuild
 * (변경후)모든 node의 meta,data connection들은 shard pin이 같은 세션끼리 tx를 공유할 수 있다.
 */
inline idBool mmcSession::isShardTrans()
{
    return (mInfo.mShardPin != SDI_SHARD_PIN_INVALID) ? ID_TRUE : ID_FALSE;

}

/* PROJ-2701 online data rebuild */
inline idBool mmcSession::isShareableTrans()
{
    return ( ( mInfo.mShardPin != SDI_SHARD_PIN_INVALID ) && 
             ( getCommitMode() == MMC_COMMITMODE_NONAUTOCOMMIT )
             ? ID_TRUE : ID_FALSE );
}

inline void mmcSession::setNewSessionShardPin()
{
    setShardPIN( sdi::makeShardPin() );
}

inline idBool mmcSession::getSessionBegin()
{
    return mSessionBegin;
}

inline idBool mmcSession::isTransAlloc( void )
{
    return mTransAllocFlag;
}

inline idBool mmcSession::getTransBegin()
{
    idBool sIsBegin = ID_FALSE;

    if ( mmcTrans::isShareableTrans( mTrans ) == ID_TRUE )
    {
        if ( mSessionBegin == ID_TRUE )
        {
            sIsBegin = mmcTrans::isSharableTransBegin( mTrans );
        }
        else
        {
            sIsBegin = ID_FALSE;
        }
    }
    else
    {
        sIsBegin = mSessionBegin;
    }
    return sIsBegin;
}

inline idBool mmcSession::getTransLazyBegin()
{
    return mTransLazyBegin;
}

inline void mmcSession::setTransLazyBegin( idBool aLazyBegin )
{
    mTransLazyBegin = aLazyBegin;
}

inline idBool mmcSession::getTransPrepared()
{
    return mTransPrepared;
}

inline ID_XID* mmcSession::getTransPreparedXID()
{
    return &mTransXID;
}

inline void mmcSession::setPrintOutEnable(UInt aValue)
{
    mInfo.mPrintOutEnable = aValue;
}

inline UInt mmcSession::getPrintOutEnable()
{
    return mInfo.mPrintOutEnable;
}

/* BUG-45707 */
inline void mmcSession::setShardClient( sdiShardClient aShardClient )
{
    mInfo.mShardClient = (UInt)aShardClient;
}

inline UInt mmcSession::isShardClient()
{
    return mInfo.mShardClient;
}

inline void mmcSession::setShardSessionType( sdiSessionType aSessionType )
{
    mInfo.mShardSessionType = aSessionType;
}

inline sdiSessionType mmcSession::getShardSessionType()
{
    return mInfo.mShardSessionType;
}

/* PROJ-2701 online data rebuild */
inline idBool mmcSession::isShardLibrarySession()
{
    idBool sResult = ID_FALSE;

    // BUG-47324
    if ( mInfo.mShardSessionType == SDI_SESSION_TYPE_LIB )
    {
        sResult = ID_TRUE;
    }
    return sResult;
}

/* BUG-45899 */
inline UInt mmcSession::getTrclogDetailShard()
{
    return mInfo.mTrclogDetailShard;
}

inline void mmcSession::setTrclogDetailShard( UInt aTrclogDetailShard )
{
    mInfo.mTrclogDetailShard = aTrclogDetailShard;
}

/* PROJ-2632 */
inline UInt mmcSession::getSerialExecuteMode()
{
    return mInfo.mSerialExecuteMode;
}

inline void mmcSession::setSerialExecuteMode( UInt aValue )
{
    mInfo.mSerialExecuteMode = aValue;
}

inline UInt mmcSession::getTrcLogDetailInformation()
{
    return mInfo.mTrcLogDetailInformation;
}

inline void mmcSession::setTrcLogDetailInformation( UInt aValue )
{
    mInfo.mTrcLogDetailInformation = aValue;
}

inline void mmcSession::setShardDDLLockTimeout( UInt aShardDDLLockTimeout )
{
    mInfo.mShardDDLLockTimeout = aShardDDLLockTimeout;
}

inline void mmcSession::setShardDDLLockTryCount( UInt aShardDDLLockTryCount )
{
    mInfo.mShardDDLLockTryCount = aShardDDLLockTryCount;
}

inline void mmcSession::setDDLLockTimeout( SInt aDDLLockTimeout )
{
    mInfo.mDDLLockTimeout = aDDLLockTimeout;
}

/* BUG-46019 */
inline mmcMessageCallback mmcSession::getMessageCallback()
{
    return mInfo.mMessageCallback;
}

inline void mmcSession::setMessageCallback(mmcMessageCallback aValue)
{
    mInfo.mMessageCallback = aValue;
}

/* BUG-47648  disk partition에서 사용되는 prepared memory 사용량 개선 */
inline UInt mmcSession::getReducePartPrepareMemory()
{
    return mInfo.mReducePartPrepareMemory;
}

inline void mmcSession::setReducePartPrepareMemory( UInt aValue )
{
    mInfo.mReducePartPrepareMemory = aValue;
}

inline void mmcSession::getSessionPropertyInfo( UShort *aSessionPropID,
                                                SChar  **aSessionPropValue,
                                                UInt   *aSessionPropValueLen )
{
    *aSessionPropValue    = mInfo.mSessionPropValueStr;
    *aSessionPropValueLen = mInfo.mSessionPropValueLen;
    *aSessionPropID       = mInfo.mSessionPropID;
}

inline UInt mmcSession::getDblinkRemoteStatementAutoCommit()
{
    return mInfo.mDblinkRemoteStatementAutoCommit;
}

inline UInt mmcSession::getPropertyAttrbute()
{
    return mInfo.mPropertyAttribute;
}

inline void mmcSession::setPropertyAttrbute( UInt aValue )
{
    mInfo.mPropertyAttribute = aValue;
}

inline idBool mmcSession::getShardInPSMEnable()
{
    return mInfo.mShardInPSMEnable;
}

inline void mmcSession::setShardInPSMEnable( idBool aValue )
{
    mInfo.mShardInPSMEnable = aValue;
}

inline void mmcSession::initStmtExecSeqForShardTx()
{
    mInfo.mStmtExecSeqForShardTx = SDI_STMT_EXEC_SEQ_INIT;
}

inline void mmcSession::increaseStmtExecSeqForShardTx()
{
    mInfo.mStmtExecSeqForShardTx++;
}

inline void mmcSession::decreaseStmtExecSeqForShardTx()
{
    mInfo.mStmtExecSeqForShardTx--;
}

inline UInt mmcSession::getStmtExecSeqForShardTx()
{
    return mInfo.mStmtExecSeqForShardTx;
}

inline void mmcSession::setStmtExecSeqForShardTx( UInt aValue )
{
    mInfo.mStmtExecSeqForShardTx = aValue;
}

inline idBool mmcSession::getIsNeedBlockCommit()
{
    return mIsNeedBlockCommit;
}

inline void mmcSession::setIsNeedBlockCommit()
{
    if ( ( isGTx() == ID_TRUE ) &&
         ( isShareableTrans() == ID_TRUE ) &&
         ( isShardLibrarySession() ) )
    {
        mIsNeedBlockCommit = ID_TRUE;
    }
    else
    {
        mIsNeedBlockCommit = ID_FALSE;
    }
}

inline IDE_RC mmcSession::blockForLibrarySession( mmcTransObj * aTrans,
                                                  idBool      * aIsBlocked )
{
    mmcSession * sDelegatedSession = NULL;

    *aIsBlocked = ID_FALSE;

    if ( ( mIsNeedBlockCommit == ID_TRUE ) &&
         ( getQciSession()->mQPSpecific.mClientInfo != NULL ) )
    {
        mmcTrans::fixSharedTrans( aTrans, getSessionID() );
        IDE_TEST( blockDelegateSession( aTrans, &sDelegatedSession ) != IDE_SUCCESS );
        mmcTrans::unfixSharedTrans( aTrans, getSessionID() );

        if ( sDelegatedSession != NULL )
        {
            IDE_DASSERT( isShardLibrarySession() == ID_TRUE );

            *aIsBlocked = ID_TRUE;
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

inline void mmcSession::unblockForLibrarySession( mmcTransObj * aTrans )
{
    mmcTrans::fixSharedTrans( aTrans, getSessionID() );
    unblockDelegateSession( aTrans );
    mmcTrans::unfixSharedTrans( aTrans, getSessionID() );
}

/* PROJ-2733-DistTxInfo */
inline UChar *mmcSession::getSessionTypeString()
{
    switch (getShardSessionType())
    {
        case SDI_SESSION_TYPE_COORD:
            return (UChar *)"COORD";

        case SDI_SESSION_TYPE_LIB:
            return (UChar *)"LIB";

        case SDI_SESSION_TYPE_USER:
            if (this->isShardClient() == SDI_SHARD_CLIENT_TRUE)
            {
                return (UChar *)"USER-SHARDCLI";
            }
            else
            {
                return (UChar *)"USER-CLI";
            }

        default:  /* Non-reachable */
            return (UChar *)"UNKNOWN";
    }
}

inline UInt mmcSession::getGlobalTransactionLevel()
{
    return mInfo.mGlobalTransactionLevel;
}

inline idBool mmcSession::isGTx()
{
    return mIsGTx;
}

inline idBool mmcSession::isGCTx()
{
    return mIsGCTx;
}

inline sdiClientInfo * mmcSession::getShardClientInfo()
{
    return getQciSession()->mQPSpecific.mClientInfo;
}

inline void mmcSession::getCoordSCN( sdiClientInfo * aClientInfo,
                                     smSCN         * aCoordSCN )
{
    if ( aClientInfo != NULL )
    {
        SM_GET_SCN( aCoordSCN,
                    &aClientInfo->mGCTxInfo.mCoordSCN );
    }
    else
    {
        SMI_INIT_SCN( aCoordSCN );
    }
}

inline void mmcSession::getCoordPrepareSCN( sdiClientInfo * aClientInfo,
                                            smSCN         * aPrepareSCN )
{
    if ( aClientInfo != NULL )
    {
        SM_GET_SCN( aPrepareSCN,
                    &aClientInfo->mGCTxInfo.mPrepareSCN );
    }
    else
    {
        SMI_INIT_SCN( aPrepareSCN );
    }
}

inline void mmcSession::setCoordGlobalCommitSCN( sdiClientInfo * aClientInfo,
                                                 smSCN         * aGlobalCommitSCN )
{
    if ( aClientInfo != NULL )
    {
        SM_SET_SCN( &aClientInfo->mGCTxInfo.mGlobalCommitSCN,
                    aGlobalCommitSCN );
    }
}

inline void mmcSession::setShardStatementRetry( UInt aValue )
{
    mInfo.mShardStatementRetry = aValue;
}

inline UInt mmcSession::getShardStatementRetry()
{
    return mInfo.mShardStatementRetry;
}

inline void mmcSession::setIndoubtFetchTimeout( UInt aValue )
{
    mInfo.mIndoubtFetchTimeout = aValue;
}

inline void mmcSession::setIndoubtFetchMethod( UInt aValue )
{
    mInfo.mIndoubtFetchMethod = aValue;
}

inline idBool mmcSession::getGCTxPermit()
{
    return mInfo.mGCTxPermit;
}

inline void mmcSession::setGCTxPermit(idBool aValue)
{
    mInfo.mGCTxPermit = aValue;
}

/* BUG-48132 */
inline UInt mmcSession::getPlanHashOrSortMethod()
{
    return mInfo.mPlanHashOrSortMethod;
}

inline void mmcSession::setPlanHashOrSortMethod( UInt aValue )
{
    mInfo.mPlanHashOrSortMethod = aValue;
}

/* BUG-48161 */
inline UInt mmcSession::getBucketCountMax()
{
    return mInfo.mBucketCountMax;
}

inline void mmcSession::setBucketCountMax( UInt aValue)
{
    mInfo.mBucketCountMax = aValue;
}

/* BUG-48348 */
inline UInt mmcSession::getEliminateCommonSubexpression()
{
    return mInfo.mEliminateCommonSubexpression;
}

inline void mmcSession::setEliminateCommonSubexpression( UInt aValue )
{
    mInfo.mEliminateCommonSubexpression = aValue;
}

inline void mmcSession::clearToBeShardMetaNumber()
{
    mInfo.mToBeShardMetaNumber = SDI_NULL_SMN;
}

inline IDE_RC mmcSession::rebuildShardSessionBeforeEndTran( mmcTransObj * aTrans )
{
    ULong sSMNForDataNode = SDI_NULL_SMN;

    if ( ( isShardUserSession() == ID_TRUE ) ||
         ( isShardLibrarySession() == ID_TRUE ) )
    {
        if ( ( mQciSession.mQPSpecific.mFlag & QC_SESSION_SHARD_META_TOUCH_MASK ) ==
             QC_SESSION_SHARD_META_TOUCH_FALSE )
        {
            sSMNForDataNode = sdi::getSMNForDataNode();

            if ( getShardMetaNumber() < sSMNForDataNode )
            {
                IDE_TEST( rebuildShardSession( sSMNForDataNode,
                                               aTrans )
                          != IDE_SUCCESS );
            }
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

inline void mmcSession::rebuildShardSessionAfterEndTran()
{
    if ( ( isShardUserSession() == ID_TRUE ) ||
         ( isShardLibrarySession() == ID_TRUE ) )
    {
        if ( isReshardOccurred() == ID_TRUE )
        {
            cleanupShardRebuildSession();
        }
    }
}

/* BUG-48384 */
inline UShort mmcSession::getClientTouchNodeCount()
{
    return mInfo.mClientTouchNodeCount;
}

inline void mmcSession::setClientTouchNodeCount(UShort aValue)
{
    mInfo.mClientTouchNodeCount = aValue;
}
#endif

