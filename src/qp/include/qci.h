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
 

#ifndef _O_QCI_H_
#define _O_QCI_H_ 1

#include <iduFixedTable.h>
#include <smi.h>
#include <qc.h>
#include <qmnDef.h>
#include <qcmCreate.h>
#include <qcmPartition.h>
#include <qriParseTree.h>
#include <qsf.h>
#include <qmcInsertCursor.h>
#include <qmx.h>
#include <qmxSimple.h>
#include <qcmDatabaseLinks.h>
#include <qcuSqlSourceInfo.h>
#include <idsPassword.h>

/*****************************************************************************
 *
 * qci의 설계 원칙
 *
 * qci는 상위 레이어(MM)을 위해 만들어진 QP 인터페이스이다.
 * MM에서는 qci를 통해서 QP를 컨트롤 해야한다.
 * QP에서는 qci를 사용해서는 안된다.
 * qci는 QP의 최 상위 인터페이스이기 때문에
 * coupling을 줄이기 위해서이다.
 *
 * qci에는 qci class 뿐만 아니라 qciSession, qciStatement등의 자료구조,
 * qciMisc class도 포함한다.
 *
 *****************************************************************************/

#define QCI_EMPTY_USER_ID   QC_EMPTY_USER_ID  
#define QCI_PUBLIC_USER_ID  QC_PUBLIC_USER_ID  

/* PROJ-1789 Updatable Scrollable Cursor */

#define QCI_BIND_FLAGS_NULLABLE_MASK    QMS_TARGET_IS_NULLABLE_MASK
#define QCI_BIND_FLAGS_NULLABLE_TRUE    QMS_TARGET_IS_NULLABLE_TRUE
#define QCI_BIND_FLAGS_NULLABLE_FALSE   QMS_TARGET_IS_NULLABLE_FALSE

#define QCI_BIND_FLAGS_UPDATABLE_MASK   QMS_TARGET_IS_UPDATABLE_MASK
#define QCI_BIND_FLAGS_UPDATABLE_TRUE   QMS_TARGET_IS_UPDATABLE_TRUE
#define QCI_BIND_FLAGS_UPDATABLE_FALSE  QMS_TARGET_IS_UPDATABLE_FALSE

// iSQLSessionKind와 일치해야 한다.
#define QCI_EXPLAIN_PLAN_OFF          (0)
#define QCI_EXPLAIN_PLAN_ON           (1)
#define QCI_EXPLAIN_PLAN_ONLY         (2)

#define QCI_TEMPLATE_MAX_STACK_COUNT  QC_TEMPLATE_MAX_STACK_COUNT
#define QCI_SYS_USER_ID               QC_SYS_USER_ID
#define QCI_SYSTEM_USER_ID            QC_SYSTEM_USER_ID
/* BUG-39990 SM_MAX_NAME_LEN (128) --> QC_MAX_OBJECT_NAME_LEN */ 
#define QCI_MAX_NAME_LEN              QC_MAX_NAME_LEN
#define QCI_MAX_NAME_LEN_STR          QC_MAX_NAME_LEN_STR
#define QCI_MAX_OBJECT_NAME_LEN       QC_MAX_OBJECT_NAME_LEN
#define QCI_MAX_KEY_COLUMN_COUNT      QC_MAX_KEY_COLUMN_COUNT
#define QCI_MAX_COLUMN_COUNT          QC_MAX_COLUMN_COUNT
#define QCI_MAX_IP_LEN                QC_MAX_IP_LEN
#define QCI_SMI_STMT( _QcStmt_ )      QC_SMI_STMT( ( (qcStatement*)( _QcStmt_ ) ) )
#define QCI_PARSETREE( _QcStmt_ )     QC_PARSETREE( ( (qcStatement*)( _QcStmt_ ) ) )
#define QCI_STATISTIC( _QcStmt_ )     QC_STATISTICS( ( (qcStatement*)( _QcStmt_ ) ) )
#define QCI_QMX_MEM( _QcStmt_ )       QC_QMX_MEM( ( (qcStatement*)( _QcStmt_ ) ) )
#define QCI_QMP_MEM( _QcStmt_ )       QC_QMP_MEM( ( (qcStatement*)( _QcStmt_ ) ) )
#define QCI_QME_MEM( _QcStmt_ )       QC_QME_MEM( ( (qcStatement*)( _QcStmt_ ) ) )
#define QCI_STR_COPY( _charBuf_, _namePos_ )    QC_STR_COPY( _charBuf_, _namePos_ )

#define QCI_STMTTEXT( _QcStmt_ )      QC_STMTTEXT( ( (qcStatement*)( _QcStmt_ ) ) )
#define QCI_STMTTEXTLEN( _QcStmt_ )   QC_STMTTEXTLEN( ( (qcStatement*)( _QcStmt_ ) ) )

// qciStatement.flag
// fix BUG-12452
// rebuild시 실패한 statement에 대해,
// 다음 execute 수행시, rebuild를 수행하도록 한다.
// mmcStatement::execute()함수내에서 이를 검사해서 rebuild를 올려보낸다.
#define QCI_STMT_REBUILD_EXEC_MASK    (0x00000001)
#define QCI_STMT_REBUILD_EXEC_SUCCESS (0x00000000)
#define QCI_STMT_REBUILD_EXEC_FAILURE (0x00000001)

// PROJ-2163
// reprepare시 실패한 statement에 대해,
// 다음 execute 수행시, reprepare를 수행하도록 한다.
// mmcStatement::reprepare()함수내에서 이를 검사해서 수행한다.
#define QCI_STMT_REHARDPREPARE_EXEC_MASK    (0x00000004)
#define QCI_STMT_REHARDPREPARE_EXEC_SUCCESS (0x00000000)
#define QCI_STMT_REHARDPREPARE_EXEC_FAILURE (0x00000004)

// PROJ-2223 audit
#define QCI_STMT_AUDIT_MASK    (0x00000008)
#define QCI_STMT_AUDIT_FALSE   (0x00000000)
#define QCI_STMT_AUDIT_TRUE    (0x00000008)

#define QCI_STMT_SHARD_RETRY_REBUILD_MASK    (0x00000010)
#define QCI_STMT_SHARD_RETRY_REBUILD_FALSE   (0x00000000)
#define QCI_STMT_SHARD_RETRY_REBUILD_TRUE    (0x00000010)

#define QCI_SESSION_INTERNAL_DDL_SYNC_MASK  QC_SESSION_INTERNAL_DDL_SYNC_MASK
#define QCI_SESSION_INTERNAL_DDL_SYNC_FALSE QC_SESSION_INTERNAL_DDL_SYNC_FALSE
#define QCI_SESSION_INTERNAL_DDL_SYNC_TRUE  QC_SESSION_INTERNAL_DDL_SYNC_TRUE

#define QCI_SESSION_INTERNAL_DDL_MASK       QC_SESSION_INTERNAL_DDL_MASK
#define QCI_SESSION_INTERNAL_DDL_FALSE      QC_SESSION_INTERNAL_DDL_FALSE
#define QCI_SESSION_INTERNAL_DDL_TRUE       QC_SESSION_INTERNAL_DDL_TRUE

/* PROJ-2240 */
extern const void * gQcmReplications;
extern const void * gQcmReplicationsIndex [ QCM_MAX_META_INDICES ];
extern const void * gQcmReplReceiver;
extern const void * gQcmReplReceiverIndex [ QCM_MAX_META_INDICES ];
extern const void * gQcmReplHosts;
extern const void * gQcmReplHostsIndex [ QCM_MAX_META_INDICES ];
extern const void * gQcmReplItems;
extern const void * gQcmReplItemsIndex [ QCM_MAX_META_INDICES ];
extern const void * gQcmReplRecoveryInfos;

/* PROJ-1442 Replication Online 중 DDL 허용 */
extern const void * gQcmReplOldItems;
extern const void * gQcmReplOldItemsIndex  [QCM_MAX_META_INDICES];
extern const void * gQcmReplOldCols;
extern const void * gQcmReplOldColsIndex   [QCM_MAX_META_INDICES];
extern const void * gQcmReplOldIdxs;
extern const void * gQcmReplOldIdxsIndex   [QCM_MAX_META_INDICES];
extern const void * gQcmReplOldIdxCols;
extern const void * gQcmReplOldIdxColsIndex[QCM_MAX_META_INDICES];

/* PROJ-2642 */
extern const void * gQcmReplOldChecks;
extern const void * gQcmReplOldChecksIndex [QCM_MAX_META_INDICES];

/* PROJ-1915 Off-line replicator */
extern const void * gQcmReplOfflineDirs;
extern const void * gQcmReplOfflineDirsIndex [QCM_MAX_META_INDICES];

extern const void * gQcmReplItemReplaceHistory;
extern const void * gQcmReplItemReplaceHistoryIndex [ QCM_MAX_META_INDICES ];

// condition length
#define QCI_CONDITION_LEN                QC_CONDITION_LEN

// PROJ-1436
// 한번에 free할 최대 prepared private template의 수
#define QCI_MAX_FREE_PREP_TMPLATE_COUNT  (32)

/* PROJ-2598 Shard pilot(shard Analyze) */
typedef struct sdiAnalyzeInfo  qciShardAnalyzeInfo;

typedef struct sdiNodeInfo     qciShardNodeInfo;

typedef qcSession         qciSession;

typedef qcStmtInfo        qciStmtInfo;

// PROJ-1518
typedef qmcInsertCursor   qciInsertCursor;

typedef qcPlanProperty    qciPlanProperty;

// session으로 공유해야 하는 정보이지만,
// qp에서만 사용하는 정보들의 집합.
typedef qcSessionSpecific qciSessionSpecific;

// session정보를 얻기 위한 mmcSession의 callback 함수들의 집합.
typedef qcSessionCallback qciSessionCallback;

/*
 * PROJ-1832 New database link.
 */

/* Database Link callback */
typedef struct qcDatabaseLinkCallback   qciDatabaseLinkCallback;

/* Database Link callback for remote table */
typedef struct qcRemoteTableColumnInfo  qciRemoteTableColumnInfo;
typedef struct qcRemoteTableCallback    qciRemoteTableCallback;

// PROJ-2163
// Plan cache 등록을 위한 호스트 변수 정보
// qciSQLPlanCacheContext 에 qcPlanBindInfo 를 추가
typedef qcPlanBindInfo      qciPlanBindInfo;

/* PROJ-2240 */
typedef qcNameCharBuffer    qriNameCharBuffer;
typedef qtcMetaRangeColumn  qriMetaRangeColumn;

typedef qcNameCharBuffer    qdrNameCharBuffer;

/* Validate */
typedef struct qciValidateReplicationCallback
{

    IDE_RC    ( * mValidateCreate )           ( void        * aQcStatement );
    IDE_RC    ( * mValidateOneReplItem )      ( void        * aQcStatement,
                                                qriReplItem * aReplItem,
                                                SInt          aRole,
                                                idBool        aIsRecoveryOpt,
                                                SInt          aReplMode );
    IDE_RC    ( * mValidateAlterAddTbl )      ( void        * aQcStatement );
    IDE_RC    ( * mValidateAlterDropTbl )     ( void        * aQcStatement );
    IDE_RC    ( * mValidateAlterAddHost )     ( void        * aQcStatement );
    IDE_RC    ( * mValidateAlterDropHost )    ( void        * aQcStatement );
    IDE_RC    ( * mValidateAlterSetHost )     ( void        * aQcStatement );
    IDE_RC    ( * mValidateAlterSetMode )     ( void        * aQcStatement );
    IDE_RC    ( * mValidateDrop )             ( void        * aQcStatement );
    IDE_RC    ( * mValidateStart )            ( void        * aQcStatement );
    IDE_RC    ( * mValidateOfflineStart )     ( void        * aQcStatement );
    IDE_RC    ( * mValidateQuickStart )       ( void        * aQcStatement );
    IDE_RC    ( * mValidateSync )             ( void        * aQcStatement );
    IDE_RC    ( * mValidateSyncTbl )          ( void        * aQcStatement );
    IDE_RC    ( * mValidateTempSync )         ( void        * aQcStatement );
    IDE_RC    ( * mValidateReset )            ( void        * aQcStatement );
    IDE_RC    ( * mValidateAlterSetRecovery ) ( void        * aQcStatement );
    IDE_RC    ( * mValidateAlterSetOffline )  ( void        * aQcStatement );
    idBool    ( * mIsValidIPFormat )          ( SChar * aIP );
    IDE_RC    ( * mValidateAlterSetGapless )  ( void        * aQcStatement );
    IDE_RC    ( * mValidateAlterSetParallel ) ( void        * aQcStatement );
    IDE_RC    ( * mValidateAlterSetGrouping ) ( void        * aQcStatement );
    IDE_RC    ( * mValidateAlterSetDDLReplicate ) ( void    * aQcStatement );
    IDE_RC    ( * mValidateAlterPartition )   ( void        * aQcStatement,
                                                qcmTableInfo* aPartInfo );
    IDE_RC    ( * mValidateDeleteItemReplaceHistory ) ( void        * aQcStatement );
    IDE_RC    ( * mValidateFailback )          ( void        * aQcStatement ); 
    IDE_RC    ( * mValidateFailover )         ( void        * aQcStatement );
} qciValidateReplicationCallback;

/* Execute */
typedef struct qciExecuteReplicationCallback
{
    /*------------------- DDL -------------------*/
    IDE_RC    ( * mExecuteCreate )                 ( void        * aQcStatement );
    IDE_RC    ( * mExecuteAlterAddTbl )            ( void        * aQcStatement );
    IDE_RC    ( * mExecuteAlterDropTbl )           ( void        * aQcStatement );
    IDE_RC    ( * mExecuteAlterAddHost )           ( void        * aQcStatement );
    IDE_RC    ( * mExecuteAlterDropHost )          ( void        * aQcStatement );
    IDE_RC    ( * mExecuteAlterSetHost )           ( void        * aQcStatement );
    IDE_RC    ( * mExecuteAlterSetMode )           ( void        * aQcStatement );
    IDE_RC    ( * mExecuteDrop )                   ( void        * aQcStatement );
    IDE_RC    ( * mExecuteStart )                  ( void        * aQcStatement );
    IDE_RC    ( * mExecuteQuickStart )             ( void        * aQcStatement );
    IDE_RC    ( * mExecuteSync )                   ( void        * aQcStatement );
    IDE_RC    ( * mExecuteSyncCondition )          ( void        * aQcStatement );
    IDE_RC    ( * mExecuteTempSync )               ( void        * aQcStatement );
    IDE_RC    ( * mExecuteReset )                  ( void        * aQcStatement );
    IDE_RC    ( * mExecuteAlterSetRecovery )       ( void        * aQcStatement );
    IDE_RC    ( * mExecuteAlterSetOfflineEnable )  ( void        * aQcStatement );
    IDE_RC    ( * mExecuteAlterSetOfflineDisable ) ( void        * aQcStatement );
    IDE_RC    ( * mExecuteAlterSetGapless )        ( void        * aQcStatement );
    IDE_RC    ( * mExecuteAlterSetParallel )       ( void        * aQcStatement );
    IDE_RC    ( * mExecuteAlterSetGrouping )       ( void        * aQcStatement );
    IDE_RC    ( * mExecuteAlterSetDDLReplicate )   ( void        * aQcStatement );
    IDE_RC    ( * mExecuteAlterSplitPartition )    ( void        * aQcStatement,
                                                     qcmTableInfo   * aTableInfo,
                                                     qcmTableInfo   * aSrcPartInfo,
                                                     qcmTableInfo   * aDstPartInfo1,
                                                     qcmTableInfo   * aDstPartInfo2 );
    IDE_RC    ( * mExecuteAlterMergePartition )    ( void         * aQcStatement,
                                                     qcmTableInfo * aTableInfo,
                                                     qcmTableInfo * aSrcPartInfo1,
                                                     qcmTableInfo * aSrcPartInfo2,
                                                     qcmTableInfo * aDstPartInfo );
    IDE_RC    ( * mExecuteAlterDropPartition )    ( void           * aQcStatement,
                                                    qcmTableInfo   * aTableInfo,
                                                    qcmTableInfo   * aSrcPartInfo );
    
    IDE_RC    ( * mExecuteFailover )              ( void        * aQcStatement );
    /*------------------- DCL -------------------*/
    IDE_RC    ( * mExecuteStop )                   ( void         * aQcStatement,
                                                     SChar        * aReplName );                                                    
    IDE_RC    ( * mExecuteFlush )                  ( smiStatement  * aSmiStmt,
                                                     SChar         * aReplName,
                                                     rpFlushOption * aFlushOption,
                                                     idvSQL        * aStatistics );
    IDE_RC    ( * mExecuteFailback )               ( void        * aQcStatement );
    IDE_RC    ( * mExecuteDeleteItemReplaceHistory )      ( void        * aQcStatement );
} qciExecuteReplicationCallback;

/* Catalog */
typedef struct qciCatalogReplicationCallback
{
    IDE_RC    ( * mUpdateReplItemsTableOIDArray ) ( void         * aQcStatement,
                                                    smOID        * aBeforeTableOIDArray,
                                                    smOID        * aAfterTableOIDArray,
                                                    UInt           aTableOIDCount );
    IDE_RC    ( * mCheckReplicationExistByName ) ( void            * aQcStatement,
                                                   qciNamePosition   aReplName,
                                                   idBool          * aIsTrue );
    IDE_RC    ( * mIsConsistentReplication ) ( void            * aQcStatement,
                                               qciNamePosition   aReplName,
                                               idBool          * aIsTrue );
} qciCatalogReplicationCallback;

typedef struct qciManageReplicationCallback
{
    IDE_RC   ( * mIsDDLEnableOnReplicatedTable ) ( UInt               aRequireLevel,
                                                   qcmTableInfo     * aTableInfo );
    IDE_RC   ( * mStopReceiverThreads ) ( smiStatement * aSmiStmt,
                                          idvSQL       * aStatistics,
                                          smOID        * aTableOIDArray,
                                          UInt           aTableOIDCount );

    IDE_RC   ( *mIsRunningEagerSenderByTableOID) ( smiStatement * aSmiStmt,
                                                   idvSQL        * aStatistics,
                                                   smOID         * aTableOIDArray,
                                                   UInt            aTableOIDCount,
                                                   idBool        * aIsExist );

    IDE_RC   ( * mIsRunningEagerReceiverByTableOID ) ( smiStatement * aSmiStmt,
                                                       idvSQL        * aStatistics,
                                                       smOID         * aTableOIDArray,
                                                       UInt            aTableOIDCount,
                                                       idBool        * aIsExist );

    IDE_RC   ( * mWriteTableMetaLog ) ( void        * aQcStatement,
                                        smOID         aOldTableOID,
                                        smOID         aNewTableOID );

    IDE_RC   ( * mIsDDLAsycReplOption ) ( void         * aQcStatement,
                                          qcmTableInfo * aSrcPartInfo,
                                          idBool       * aIsDDLReplOption );

} qciManageReplicationCallback;

typedef qcmAccessOption qciAccessOption;

#define QCI_ACCESS_OPTION_NONE        QCM_ACCESS_OPTION_NONE
#define QCI_ACCESS_OPTION_READ_ONLY   QCM_ACCESS_OPTION_READ_ONLY
#define QCI_ACCESS_OPTION_READ_WRITE  QCM_ACCESS_OPTION_READ_WRITE
#define QCI_ACCESS_OPTION_READ_APPEND QCM_ACCESS_OPTION_READ_APPEND

#define QCI_NO_PARTITION_ORDER        QDB_NO_PARTITION_ORDER

typedef qcmTableInfo qciTableInfo;
typedef qcmPartitionInfoList  qciPartitionInfoList;
// PROJ-1624 non-partitioned index
typedef qmsIndexTableRef qciIndexTableRef;

// statement 상태주기
typedef enum
{
    QCI_STMT_STATE_ALLOCED          = 0,
    QCI_STMT_STATE_INITIALIZED      = 1,
    QCI_STMT_STATE_PARSED           = 2,
    // PROJ-2163 순서변경
    // QP 의 type binding(호스트 변수의 타입 결정) 이 prepare 과정 중
    // 수행되도록 변경되었다.
    // 따라서 QCI_STMT_STATE_PREPARED 와 QCI_STMT_STATE_PARAM_INFO_BOUND 의
    // 순서가 서로 바뀌었다.
    // QCI_STMT_STATE_PREPARED         = 3,
    // QCI_STMT_STATE_PARAM_INFO_BOUND = 4,
    QCI_STMT_STATE_PARAM_INFO_BOUND = 3,  // host 변수 정보
    QCI_STMT_STATE_PREPARED         = 4,
    // BUG-
    // PROJ-1558 LOB지원을 위한 MM+CM 확장에서
    // mm내부적으로 direct execute는 없어지고,
    // prepare/execute로만 수행됨.
    // 따라서, direct execute를 위한 아래 상태전이는 제거함.
    // proj-1535
    // direct execute의 경우 spvEnv->procPlanList에 대해
    // unlatch하지 않은 상태가 필요함
    // QCI_STMT_STATE_PREPARED_DIRECT  = 4,
    QCI_STMT_STATE_PARAM_DATA_BOUND = 5,  // host변수 데이타
    QCI_STMT_STATE_EXECUTED         = 6,
    QCI_STMT_STATE_MAX
} qciStmtState;

// proj-1535
// statement상태전이시, statement와 PSM에 대해 수행할 작업들
typedef enum
{
    STATE_OK                = 0x00,
    STATE_PARAM_DATA_CLEAR  = 0x01,  // fix BUG-16482
    STATE_CLEAR             = 0x02,  // clearStatement
    STATE_CLOSE             = 0x04,  // closeStatement (binding memory)
    STATE_PSM_LATCH         = 0x08,  // latchObject & checkObject
    STATE_PSM_UNLATCH       = 0x10,  // unlatchObject
    STATE_ERR               = 0x20
} qciStateMode;

typedef struct
{
    UInt StateBit;
} qciStateInfo;

typedef enum
{
    EXEC_INITIALIZE       = 0,
    EXEC_PARSE            = 1,
    EXEC_BIND_PARAM_INFO  = 2,  // PROJ-2163 바인딩 재정립으로 순서 변경
    EXEC_PREPARE          = 3,
    EXEC_BIND_PARAM_DATA  = 4,
    EXEC_EXECUTE          = 5,
    EXEC_FINALIZE         = 6,
    EXEC_REBUILD          = 7,
    EXEC_RETRY            = 8,
    EXEC_CLEAR            = 9,
    EXEC_FUNC_MAX
} qciExecFunc;

//PROJECT PROJ-1436 SQL-Plan Cache.
typedef enum
{
    QCI_SQL_PLAN_CACHE_IN_OFF = 0,
    QCI_SQL_PLAN_CACHE_IN_ON
} qciSqlPlanCacheInMode;

typedef enum
{
    QCI_SQL_PLAN_CACHE_IN_NORMAL = 0,   // SUCCESS이거나 cache 비대상인 경우
    QCI_SQL_PLAN_CACHE_IN_FAILURE
} qciSqlPlanCacheInResult;

// Plan Display Option에 대한 정의로
// ALTER SESSION SET EXPLAIN PLAN = ON 과
// ALTER SESSION SET EXPLAIN PLAN = ONLY를 구별하기 위한 상수이다.
typedef enum
{
    QCI_DISPLAY_ALL  = QMN_DISPLAY_ALL, // 수행 결과를 모두 Display 함
    QCI_DISPLAY_CODE      // Code 영역의 정보만을 Display 함
} qciPlanDisplayMode;

/* PROJ-2207 Password policy support */
typedef enum
{
    QCI_ACCOUNT_OPEN = 0,
    QCI_ACCOUNT_OPEN_TO_LOCKED,
    QCI_ACCOUNT_LOCKED_TO_OPEN
} qciAccLockStatus;

typedef struct qciAccLimitOpts
{
    UInt             mCurrentDate;     /* 현재 일수*/
    UInt             mPasswReuseDate;  /* PASSWORD_REUSE_DATE */
    UInt             mUserFailedCnt;   /* LOGIN FAIL 횟수 */
    qciAccLockStatus mAccLockStatus;   /* LOCK 상태 */
    
    UInt    mAccountLock;         /* ACCOUNT_LOCK */
    UInt    mPasswLimitFlag;      /* PASSWORD_LIMIT_FLAG */
    UInt    mLockDate;            /* LOCK_DATE */
    UInt    mPasswExpiryDate;     /* PASSWORD_EXPIRY_DATE */
    UInt    mFailedCount;         /* FAILED_COUNT */
    UInt    mReuseCount;          /* REUSE_COUNT */
    UInt    mFailedLoginAttempts; /* FAILED_LOGIN_ATTEMPTS */
    UInt    mPasswLifeTime;       /* PASSWORD_LIFE_TIME */
    UInt    mPasswReuseTime;      /* PASSWORD_REUSE_TIME */
    UInt    mPasswReuseMax;       /* PASSWORD_REUSE_MAX */
    UInt    mPasswLockTime;       /* PASSWORD_LOCK_TIME */
    UInt    mPasswGraceTime;      /* PASSWORD_GRACE_TIME */
    SChar   mPasswVerifyFunc[QC_PASSWORD_OPT_LEN + 1];
} qciAccLimitOpts;

/* PROJ-2474 SSL/TLS Support */
typedef enum qciDisableTCP
{
    QCI_DISABLE_TCP_NULL = 0,
    QCI_DISABLE_TCP_FALSE,
    QCI_DISABLE_TCP_TRUE
} qciDisableTCP;

/* PROJ-2705 fast simple memory Partition table */
typedef enum qciSimpleLevel
{
    QCI_SIMPLE_LEVEL_NONE = 0,
    QCI_SIMPLE_LEVEL_TABLE,
    QCI_SIMPLE_LEVEL_PARTITION
} qciSimpleLevel;

/* PROJ-2474 SSL/TLS Support */
typedef enum qciConnectType
{
    QCI_CONNECT_DUMMY   = 0,                   // CMI_LINK_IMPL_DUMMY
    QCI_CONNECT_TCP,                           // CMI_LINK_IMPL_TCP
    QCI_CONNECT_UNIX,                          // CMI_LINK_IMPL_UNIX
    QCI_CONNECT_IPC,                           // CMI_LINK_IMPL_IPC
    QCI_CONNECT_SSL,                           // CMI_LINK_IMPL_SSL
    QCI_CONNECT_IPCDA,                         // CMI_LINK_IMPL_IPCDA
    QCI_CONNECT_IB,                            // CMI_LINK_IMPL_IB
    QCI_CONNECT_INVALID,                       // CMI_LINK_IMPL_INVALID
    QCI_CONNECT_BASE    = QCI_CONNECT_TCP,     // CMI_LINK_IMPL_BASE
    QCI_CONNECT_MAX     = QCI_CONNECT_INVALID  // CMI_LINK_IMPL_MAX
} qciConnectType;

typedef struct qciUserInfo
{
    SChar      loginID[QC_MAX_OBJECT_NAME_LEN + 1];
    SChar      loginOrgPassword[IDS_MAX_PASSWORD_LEN + 1];      // PROJ-2638
    SChar      loginPassword[IDS_MAX_PASSWORD_BUFFER_LEN + 1];  // BUG-38565
    SChar      loginIP[QC_MAX_IP_LEN + 1];
    UInt       loginUserID;   /* BUG-41561 */
    UInt       userID;
    idBool     invokeUserPropertyEnable;
    SChar    * invokeUserNamePtr;
    SChar      invokeUserName[QC_MAX_OBJECT_NAME_LEN + 1];
    SChar      userPassword[IDS_MAX_PASSWORD_BUFFER_LEN + 1];
    SChar    * mUsrDN; // pointer of idsGPKICtx.mUsrDN
    SChar    * mSvrDN; // pointer of idsGPKICtx.mUsrDN
    idBool     mCheckPassword; // ID_FALSE for idsGPKICtx
    scSpaceID  tablespaceID;
    scSpaceID  tempTablespaceID;
    idBool     mIsSysdba;
    /* PROJ-2207 Password policy support */
    qciAccLimitOpts  mAccLimitOpts;
    /* PROJ-1812 ROLE */
    UInt       mRoleList[QDD_USER_TO_ROLES_MAX_COUNT + 1];
    /* PROJ-2474 TSL/SSL Support */
    qciDisableTCP    mDisableTCP;
    qciConnectType   mConnectType;
} qciUserInfo;

#define QCI_COPY_USER_INFO( dstPtr, srcPtr )                                         \
{                                                                                    \
    idlOS::strncpy( dstPtr->loginID , srcPtr->loginID, QC_MAX_OBJECT_NAME_LEN + 1 ); \
    idlOS::strncpy( dstPtr->loginOrgPassword , srcPtr->loginOrgPassword, IDS_MAX_PASSWORD_LEN + 1 ); \
    idlOS::strncpy( dstPtr->loginPassword , srcPtr->loginPassword, IDS_MAX_PASSWORD_BUFFER_LEN + 1 ); \
    idlOS::strncpy( dstPtr->loginIP , srcPtr->loginIP, QC_MAX_IP_LEN + 1 ); \
    dstPtr->loginUserID = srcPtr->loginUserID; \
    dstPtr->userID = srcPtr->userID; \
    dstPtr->invokeUserPropertyEnable = srcPtr->invokeUserPropertyEnable ; \
    dstPtr->invokeUserNamePtr = NULL; \
    idlOS::strncpy( dstPtr->invokeUserName , srcPtr->invokeUserName, QC_MAX_OBJECT_NAME_LEN + 1 ); \
    idlOS::strncpy( dstPtr->userPassword , srcPtr->userPassword, IDS_MAX_PASSWORD_BUFFER_LEN + 1 ); \
    dstPtr->mUsrDN = NULL; \
    dstPtr->mSvrDN = NULL; \
    dstPtr->mCheckPassword = srcPtr->mCheckPassword; \
    dstPtr->tablespaceID = srcPtr->tablespaceID; \
    dstPtr->tempTablespaceID = srcPtr->tempTablespaceID; \
    dstPtr->mIsSysdba = srcPtr->mIsSysdba; \
    dstPtr->mAccLimitOpts = srcPtr->mAccLimitOpts;\
    idlOS::memcpy( dstPtr->mRoleList, srcPtr->mRoleList, ( ID_SIZEOF(UInt)* ( QDD_USER_TO_ROLES_MAX_COUNT + 1 ) ) );\
    dstPtr->mDisableTCP = srcPtr->mDisableTCP;\
    dstPtr->mConnectType = srcPtr->mConnectType;\
}

typedef struct qciSyncItems
{
    SChar          syncUserName[QC_MAX_OBJECT_NAME_LEN + 1];
    SChar          syncTableName[QC_MAX_OBJECT_NAME_LEN + 1];
    SChar          syncPartitionName[QC_MAX_OBJECT_NAME_LEN + 1];
    qciSyncItems * next;
} qciSyncItems;

// PROJ-2223 audit
// for audit operation
#define QCI_AUDIT_LOGGING_SESSION_SUCCESS_MASK   (0x00000001)
#define QCI_AUDIT_LOGGING_SESSION_SUCCESS_FALSE  (0x00000000)
#define QCI_AUDIT_LOGGING_SESSION_SUCCESS_TRUE   (0x00000001)

#define QCI_AUDIT_LOGGING_SESSION_FAILURE_MASK   (0x00000002)
#define QCI_AUDIT_LOGGING_SESSION_FAILURE_FALSE  (0x00000000)
#define QCI_AUDIT_LOGGING_SESSION_FAILURE_TRUE   (0x00000002)

#define QCI_AUDIT_LOGGING_ACCESS_SUCCESS_MASK    (0x00000004)
#define QCI_AUDIT_LOGGING_ACCESS_SUCCESS_FALSE   (0x00000000)
#define QCI_AUDIT_LOGGING_ACCESS_SUCCESS_TRUE    (0x00000004)

#define QCI_AUDIT_LOGGING_ACCESS_FAILURE_MASK    (0x00000008)
#define QCI_AUDIT_LOGGING_ACCESS_FAILURE_FALSE   (0x00000000)
#define QCI_AUDIT_LOGGING_ACCESS_FAILURE_TRUE    (0x00000008)

typedef enum qciAuditStatus
{
    QCI_AUDIT_OFF = 0,  /* To disable auditing */
    QCI_AUDIT_ON,       /* To enable auditing */
    QCI_AUDIT_STATUS,   /* To show current auditing status */
    QCI_AUDIT_RELOAD    /* To reload auditing options */
} qciAuditStatus;

typedef enum qciAuditOperIdx
{
    // DML, SP
    QCI_AUDIT_OPER_DML    = 0,
    QCI_AUDIT_OPER_SELECT = QCI_AUDIT_OPER_DML,
    QCI_AUDIT_OPER_INSERT,
    QCI_AUDIT_OPER_UPDATE,
    QCI_AUDIT_OPER_DELETE,
    QCI_AUDIT_OPER_MOVE, 
    QCI_AUDIT_OPER_MERGE, 
    QCI_AUDIT_OPER_ENQUEUE, 
    QCI_AUDIT_OPER_DEQUEUE, 
    QCI_AUDIT_OPER_LOCK,
    QCI_AUDIT_OPER_EXECUTE,

    // DCL
    QCI_AUDIT_OPER_DCL,
    QCI_AUDIT_OPER_COMMIT = QCI_AUDIT_OPER_DCL,
    QCI_AUDIT_OPER_ROLLBACK,
    QCI_AUDIT_OPER_SAVEPOINT,
    QCI_AUDIT_OPER_CONNECT,
    QCI_AUDIT_OPER_DISCONNECT,
    QCI_AUDIT_OPER_ALTER_SESSION,
    QCI_AUDIT_OPER_ALTER_SYSTEM,

    // DDL
    QCI_AUDIT_OPER_DDL,
    
    QCI_AUDIT_OPER_MAX
    
} qciAuditOperIdx;

typedef struct qciAuditOperation
{
    UInt     userID;
    SChar    objectName[QC_MAX_OBJECT_NAME_LEN + 1];
    UInt     operation[QCI_AUDIT_OPER_MAX];
} qciAuditOperation;

typedef enum qciAuditObjectType
{
    QCI_AUDIT_OBJECT_TABLE,  // table, view, queue
    QCI_AUDIT_OBJECT_SEQ,    // sequence
    QCI_AUDIT_OBJECT_PROC,   // procedure, function
    QCI_AUDIT_OBJECT_PKG     // BUG-36973 package  
} qciAuditObjectType;

// sql에서 참조하는 audit objects
typedef struct qciAuditRefObject
{
    UInt                 userID;
    SChar                objectName[QC_MAX_OBJECT_NAME_LEN + 1];
    qciAuditObjectType   objectType;
} qciAuditRefObject;

// sql의 audit operation과 참조하는 audit objects
typedef struct qciAuditInfo
{
    qciAuditRefObject   * refObjects;
    UInt                  refObjectCount;
    qciAuditOperIdx       operation;
} qciAuditInfo;

typedef struct qciAuditManagerCallback
{
    void ( *mReloadAuditCond ) ( qciAuditOperation *aObjectOptions,
                                 UInt               aObjectCount,
                                 qciAuditOperation *aUserOptions,
                                 UInt               aUserOptionCount );
    void ( *mStopAudit ) ( void );
} qciAuditManagerCallback;

/* PROJ-2626 Snapshot Export */
typedef struct qciSnapshotCallback
{
    IDE_RC ( *mSnapshotBeginEnd )( idBool aIsBegin );
} qciSnapshotCallback;

typedef struct qciStatement
{
    qcStatement           statement;
    qciStmtState          state;

    UInt                  flag;

    // mm이 관리하는 root smiStatement.
    // rebuild시 root smiStatement가 필요.
    // rebuild시 smiStatement end & begin을 수행해야 하므로,
    // qp에서 따로 관리를 할 필요없이
    // mm으로부터 smiStatement를 받아서 처리함.
    // smiStatement    * parentSmiStmtForPrepare;

} qciStatement;

typedef enum
{
    QCI_STARTUP_INIT         = IDU_STARTUP_INIT,
    QCI_STARTUP_PRE_PROCESS  = IDU_STARTUP_PRE_PROCESS,
    QCI_STARTUP_PROCESS      = IDU_STARTUP_PROCESS,
    QCI_STARTUP_CONTROL      = IDU_STARTUP_CONTROL,
    QCI_STARTUP_META         = IDU_STARTUP_META,
    QCI_STARTUP_SERVICE      = IDU_STARTUP_SERVICE,
    QCI_STARTUP_SHUTDOWN     = IDU_STARTUP_SHUTDOWN,

    QCI_SHUTDOWN_NORMAL      = 11,
    QCI_SHUTDOWN_IMMEDIATE   = 12,
    QCI_SHUTDOWN_EXIT        = 13,

    QCI_SESSION_CLOSE        = 21,

    QCI_DTX_COMMIT           = 31,
    QCI_DTX_ROLLBACK         = 32,

    QCI_META_DOWNGRADE = 98, /* PROJ-2689 */
    QCI_META_UPGRADE   = 99,

    QCI_STARTUP_MAX
} qciStartupPhase;

typedef struct qciArgCreateDB
{
    SChar       * mDBName ;
    UInt          mDBNameLen;
    SChar       * mOwnerDN;
    UInt          mOwnerDNLen;
    scPageID      mUserCreatePageCount ;
    idBool        mArchiveLog;

    // PROJ-1579 NCHAR
    SChar       * mDBCharSet;
    SChar       * mNationalCharSet;
} qciArgCreateDB;

typedef struct qciArgDropDB
{
    SChar       * mDBName ;
    SInt          mDBNameLen;
} qciArgDropDB;

typedef struct qciArgStartup
{
    qciStatement    * mQcStatement;
    qciStartupPhase   mPhase;
    UInt              mStartupFlag;
} qciArgStartup;

typedef struct qciArgShutdown
{
    qciStatement    * mQcStatement;
    qciStartupPhase   mPhase;
    UInt              mShutdownFlag;
} qciArgShutdown;

typedef struct qciArgCloseSession
{
    void        * mMmSession;
    SChar       * mDBName;
    UInt          mDBNameLen;
    UInt          mSessionID;
    SChar       * mUserName;
    UInt          mUserNameLen;
    idBool        mCloseAll;
} qciArgCloseSession;

typedef struct qciArgCommitDTX
{
    smTID         mSmTID;
} qciArgCommitDTX;

typedef struct qciArgRollbackDTX
{
    smTID         mSmTID;
} qciArgRollbackDTX;

typedef struct qciArgCreateQueue
{
    UInt          mTableID;
} qciArgCreateQueue;

typedef struct qciArgDropQueue
{
    void       * mMmSession;
    UInt         mTableID;
} qciArgDropQueue;

typedef struct qciArgEnqueue
{
    void       * mMmSession;
    UInt         mTableID;
} qciArgEnqueue;

typedef struct qciArgDequeue
{
    void       * mMmSession;
    UInt         mTableID;   // dequeue를 수행할 큐 테이블의 ID
    smSCN        mViewSCN;   // PROJ-1677 DEQ dequeue 수행시 statement SCN.
    ULong        mWaitSec;   // dequeue시 대기 시간
} qciArgDequeue;


//-------------------------------------
// BIND
//-------------------------------------
typedef struct qciBindColumn
{
    UShort   mId;           // 0 부터 사용함.
    UInt     mType;
    UInt     mLanguage;
    UInt     mArguments;
    SInt     mPrecision;
    SInt     mScale;
    UInt     mMaxByteSize;  // PROJ-2256
    UInt     mFlag;         // nullable, updatable

    /* PROJ-1789 Updatable Scrollable Cursor */
    SChar  * mDisplayName;
    UInt     mDisplayNameSize;
    SChar  * mColumnName;
    UInt     mColumnNameSize;
    SChar  * mBaseColumnName;
    UInt     mBaseColumnNameSize;
    SChar  * mTableName;
    UInt     mTableNameSize;
    SChar  * mBaseTableName;
    UInt     mBaseTableNameSize;
    SChar  * mSchemaName;
    UInt     mSchemaNameSize;
    SChar  * mCatalogName;
    UInt     mCatalogNameSize;

} qciBindColumn;

typedef struct qciBindParam
{
    UShort          id;      // 0 부터 사용함.
    SChar         * name;
    UInt            type;
    UInt            language;
    UInt            arguments;
    SInt            precision;      // 사용자가 바인드한 precision
    SInt            scale;          // 사용자가 바인드한 scale
    UInt            inoutType;
    void          * data;           // 데이타를 가리키는 포인터
    UInt            dataSize;       // PROJ-2163
    SShort          ctype;          // PROJ-2256
    SShort          sqlctype;       // PROJ-2616
    
} qciBindParam;

typedef struct qciBindColumnInfo
{
    struct qmsTarget * target;
    qciBindColumn      column;
} qciBindColumnInfo;

typedef struct qciBindParamInfo
{
    qciBindParam    param;
    idBool          isParamInfoBound;
    idBool          isParamDataBound; // fix BUG-16482
    mtvConvert    * convert;
    void          * canonBuf;
} qciBindParamInfo;

/* PROJ-2160 CM 타입제거
   fetch 시 사용되는 정보 */
typedef struct qciFetchColumnInfo
{
    UChar *value;
    SInt   dataTypeId;
} qciFetchColumnInfo;

// PROJ-1386 Dynamic SQL   -- [BEGIN]

typedef struct qciBindData
{
    UShort         id;
    SChar        * name;
    mtcColumn    * column;
    void         * data;
    UInt           size;
    qciBindData  * next;
} qciBindData;

typedef struct qciSQLPrepareContext
{
    void                * mmStatement;
    idBool                execMode; // ID_FALSE : prepared, ID_TRUE : direct
    SChar               * sqlString;
    UInt                  sqlStringLen;
    qciStmtType           stmtType; // OUT
} qciSQLPrepareContext;

typedef struct qciSQLParamInfoContext
{
    void                * mmStatement;
    qciBindParam          bindParam;
} qciSQLParamInfoContext;

typedef struct qciSQLParamDataContext
{
    void                * mmStatement;
    qciBindData           bindParamData;
} qciSQLParamDataContext;

typedef struct qciSQLExecuteContext
{
    void                * mmStatement;
    qciBindData         * outBindParamDataList;
    SLong                 affectedRowCount;
    SLong                 fetchedRowCount;
    idBool                recordExist;
    UShort                resultSetCount;
    idBool                isFirst;
} qciSQLExecuteContext;

typedef struct qciSQLFetchContext
{
    iduMemory           * memory;
    void                * mmStatement;
    qciBindData         * bindColumnDataList;
    idBool                nextRecordExist;
} qciSQLFetchContext;

typedef struct qciSQLAllocStmtContext
{
    void                * mmStatement; // OUT parameter
    void                * mmParentStatement;
    void                * mmSession;
    idBool                dedicatedMode; // session의 current stmt로 설정여부
} qciSQLAllocStmtContext;

typedef struct qciSQLFreeStmtContext
{
    void                * mmStatement; // OUT parameter
    idBool                freeMode; // ID_FALSE : FREE_CLOSE, ID_TRUE : FREE_DROP
} qciSQLFreeStmtContext;

typedef struct qciSQLCheckBindContext
{
    void                * mmStatement;
    UShort                bindCount;
} qciSQLCheckBindContext;

/* PROJ-2197 PSM Renewal */
typedef struct qciSQLGetQciStmtContext
{
    void                * mmStatement;
    qciStatement        * mQciStatement;
} qciSQLGetQciStmtContext;

//PROJ-1436 SQL-Plan Cache.
typedef struct qciSQLPlanCacheContext
{
    qciSqlPlanCacheInMode   mPlanCacheInMode;
    UInt                    mSmiStmtCursorFlag;
    qciStmtType             mStmtType;
    UInt                    mSharedPlanSize;
    void                  * mSharedPlanMemory;
    UInt                    mPrepPrivateTemplateSize;
    void                  * mPrepPrivateTemplate;
    qciPlanProperty       * mPlanEnv;
    qciPlanBindInfo         mPlanBindInfo;             // PROJ-2163
    idBool                  mPlanCacheKeep; // BUG-46137
} qciSQLPlanCacheContext;

// BUG-36203 PSM Optimize
typedef struct qciSQLFetchEndContext
{
    void * mmStatement;
} qciSQLFetchEndContext;

/* BUG-38509 autonomous transaction */
typedef struct qciSwapTransContext
{
    void     * mmSession;
    void     * mmStatement;
    smiTrans * mNewSmiTrans;
    void     * mNewMmcTrans;
    void     * mOriMmcTrans;
    void     * mOriSmiStmt;
    idBool     mIsExecSuccess;
} qciSwapTransactionContext;

/* BUG-41452 Built-in functions for getting array binding info. */
typedef struct qciArrayBindContext
{
    void     * mMmStatement;
    idBool     mIsArrayBound;
    UInt       mCurrRowCnt;
    UInt       mTotalRowCnt;
} qciArrayBindContext;

typedef struct qciInternalSQLCallback
{
    IDE_RC (*mAllocStmt)(void * aUserContext);
    IDE_RC (*mPrepare)(void * aUserContext);
    IDE_RC (*mBindParamInfo)(void * aUserContext);
    IDE_RC (*mBindParamData)(void * aUserContext);
    IDE_RC (*mExecute)(void * aUserContext);
    IDE_RC (*mFetch)(void * aUserContext);
    IDE_RC (*mFreeStmt)(void * aUserContext);
    IDE_RC (*mCheckBindParamCount)(void * aUserContext);
    IDE_RC (*mCheckBindColumnCount)(void * aUserContext);
    IDE_RC (*mGetQciStmt)(void * aUserContext);
    IDE_RC (*mEndFetch)(void * aUserContext);
} qciInternalSQLCallback;

typedef enum qciDDLTargetType
{
    QCI_DDL_TARGET_NONE,
    QCI_DDL_TARGET_TABLE,
    QCI_DDL_TARGET_PARTITION
} qciDDLTargetType;


// PROJ-1386 Dynamic SQL   -- [END]

/* PROJ-1832 New database link. */
typedef qcmDatabaseLinksItem qciDatabaseLinksItem;

typedef IDE_RC (*qciDatabaseCallback)( idvSQL * /* aStatistics */, void * aArg );
typedef IDE_RC (*qciQueueCallback)( void * aArg );
//PROJ-1677 DEQ
typedef IDE_RC (*qciDeQueueCallback)( void * aArg,idBool aBookDeq);
typedef IDE_RC (*qciOutBindLobCallback)( void         * aMmSession,
                                         smLobLocator * aOutLocator,
                                         smLobLocator * aOutFirstLocator,
                                         UShort         aOutCount );
typedef IDE_RC (*qciCloseOutBindLobCallback)( void         * aMmSession,
                                              smLobLocator * aOutFirstLocator,
                                              UShort         aOutCount );
typedef IDE_RC (*qciFetchColumnCallback)( idvSQL        * aStatistics,
                                          qciBindColumn * aBindColumn,
                                          void          * aData,
                                          void          * aContext );
typedef IDE_RC (*qciGetBindParamDataCallback)( idvSQL       * aStatistics,
                                               qciBindParam * aBindParam,
                                               void         * aData,
                                               void         * aContext );
typedef IDE_RC (*qciReadBindParamInfoCallback)( void         * aBindContext,
                                                qciBindParam * aBindParam);
typedef IDE_RC (*qciSetParamDataCallback)( idvSQL    * aStatistics,
                                           void      * aBindParam,
                                           void      * aTarget,
                                           UInt        aTargetSize,
                                           void      * aTemplate,
                                           void      * aBindContext );
typedef IDE_RC (*qciSetParamData4RebuildCallback)( idvSQL    * /* aStatistics */,
                                                   void      * aBindParam,
                                                   void      * aTarget,
                                                   UInt        aTargetSize,
                                                   void      * aTemplate,
                                                   void      * aSession4Rebuild,
                                                   void      * aBindData );
typedef IDE_RC (*qciReadBindParamDataCallback)( void *  aBindContext );

typedef IDE_RC (*qciCopyBindParamDataCallback)( qciBindParam *aBindParam,
                                                UChar        *aSource,
                                                UInt         *aStructSize );
//PROJ-1436 SQL-PLAN CACHE
typedef IDE_RC (*qciGetSmiStatement4PrepareCallback)( void          * aGetSmiStmtContext,
                                                      smiStatement ** aSmiStatement );

/* PROJ-2240 */
typedef IDE_RC (*qciSetReplicationCallback)( qciValidateReplicationCallback aValidateCallback,
                                             qciExecuteReplicationCallback  aExecuteCallback,
                                             qciCatalogReplicationCallback  aCatalogaCallback,
                                             qciManageReplicationCallback   aManageCallback );

/*****************************************************************************
 *
 * qci class에는 크게 qciSession 관련 함수와
 * qciStatement 관련 함수로 나뉜다.
 *
 * qciSession 관련 함수는 initializeSession, finalizeSession 함수가 있으며,
 * qciStatement 관련 함수는 모두 qciStatement * 인자를 받는다.
 *
 * + qciStatement의 상태 주기
 *
 *   - alloced
 *     : qciStatement 지역 변수를 선언했을 때의 상태
 *
 *   - initialized
 *     : 초기화가 된 상태. 내부적으로 각종 memory가 alloc된다.
 *       그리고 qciSession이 세팅된다.
 *
 *   - parsed
 *     : 파싱 과정을 마친 상태
 *       rebuild가 발생하면 이 상태부터 다시 출발한다.
 *
 *   - prepared
 *     : validation, optimization 과정을 마친 상태
 *       SQL cache의 대상이 된다.
 *
 *   - bindParamInfo
 *     : 호스트 변수에 대한 컬럼 정보가 바인딩된 상태
 *
 *   - bindParamData
 *     : 호스트 변수에 데이터가 바인딩된 상태
 *
 *   - executed
 *     : 실행이 된 상태
 *       최상위 plan의 init이 호출된 상태이다.
 *
 *****************************************************************************/
class qci
{
private:

    static qciStartupPhase mStartupPhase;

    static idBool          mInplaceUpdateDisable;
    // proj-1535
    // statement 상태전이에 관계된 함수 호출시,
    // 현 상태에서 호출될 수 있는 함수인지 판단하고,
    // 호출가능한 함수이면,
    // 함수수행에 맞게 psm lock or qcStatement 상태 정리.
    static IDE_RC checkExecuteFuncAndSetEnv( qciStatement * aStatement,
                                             qciExecFunc  aExecFunc );

    // prepare 직후에 bind column 배열을 생성함
    static IDE_RC makeBindColumnArray( qciStatement * aStatement );

    // prepare 직후에 bind param 배열을 생성함
    static IDE_RC makeBindParamArray( qciStatement * aStatement );

    // execute전 bind param info 정보를 구축한다.
    static IDE_RC buildBindParamInfo( qciStatement * aStatement );

    // PROJ-1436 shared plan cache를 생성한다.
    static IDE_RC makePlanCacheInfo( qciStatement           * aStatement,
                                     qciSQLPlanCacheContext * aPlanCacheContext );

    // PROJ-1436 prepared template을 생성한다.
    static IDE_RC allocPrepTemplate( qcStatement     * aStatement,
                                     qcPrepTemplate ** aPrepTemplate );

    static IDE_RC checkBindInfo( qciStatement *aStatement );

    static IDE_RC validatePlanOrg(
        qciGetSmiStatement4PrepareCallback   aGetSmiStmt4PrepareCallback,
        void                               * aGetSmiStmt4PrepareContext, 
        qciStatement                       * aStatement,
        void                               * aSharedPlan,
        idBool                             * aIsValidPlan );

    static IDE_RC validatePlanMode(
        qciGetSmiStatement4PrepareCallback   aGetSmiStmt4PrepareCallback,
        void                               * aGetSmiStmt4PrepareContext, 
        qciStatement                       * aStatement,
        void                               * aSharedPlan,
        idBool                             * aIsValidPlan );
public:
    // 해당 과정을 성공적으로 수행했을 경우,
    // 그에 맞는 상태전이로 이동.
    static IDE_RC  changeStmtState( qciStatement * aStatement,
                                    qciExecFunc    aExecFunc );

    static qciSessionCallback               	mSessionCallback;
    static qciOutBindLobCallback            	mOutBindLobCallback;
    static qciCloseOutBindLobCallback       	mCloseOutBindLobCallback;
    static qciInternalSQLCallback           	mInternalSQLCallback;
    static qciSetParamData4RebuildCallback  	mSetParamData4RebuildCallback;

    /* PROJ-2240 */
    static qciValidateReplicationCallback 	mValidateReplicationCallback;
    static qciExecuteReplicationCallback 	mExecuteReplicationCallback;
    static qciCatalogReplicationCallback 	mCatalogReplicationCallback;
    static qciManageReplicationCallback     mManageReplicationCallback;

    /* PROJ-2223 */
    static qciAuditManagerCallback   mAuditManagerCallback;

    /* PROJ-2626 Snapshot Export */
    static qciSnapshotCallback       mSnapshotCallback;

    static IDE_RC initializeStmtInfo( qciStmtInfo * aStmtInfo,
                                      void        * aMmStatement );

    /*************************************************************************
     *
     * qciSession 관련 함수들
     *
     *************************************************************************/

    // 세션을 초기화한다.
    // QP 내부적으로 사용되는 데이터를 위한
    // 공간 할당 및 각 데이터에 대한 초기화가 수행된다.
    // 세션이 시작될 때 호출되어야 한다.
    static IDE_RC initializeSession( qciSession *aSession,
                                     void       *aMmSession );

    // QP 내부적으로 사용된 데이터를 위한 공간을 해제한다.
    // 세션이 종료될 때 호출되어야 한다.
    static IDE_RC finalizeSession( qciSession *aSession,
                                   void       *aMmSession );

    // PROJ-1407 Temporary Table
    // session에서 end-transaction이 수행되는 경우 commit이후에 수행된다.
    static void endTransForSession( qciSession * aSession,
                                    idBool       aCommit,
                                    smSCN      * aCommitSCN,
                                    ULong        aNewSMN,
                                    sdiZKPendingJobFunc * aOutPendingJobFunc );

    // PROJ-1407 Temporary Table
    // session에서 end-session이 수행되는 경우 commit이후에 수행된다.
    static void endSession( qciSession * aSession );
    
    /*************************************************************************
     *
     * qciStatement 상태 주기 함수들
     *
     *************************************************************************/

    // initializeStatement()
    // statement를 초기화한다.
    // 내부적으로 공간을 할당받고 각종 멤버들을 초기화한다.
    // session은 읽는 용도로만 사용한다.
    // 이 함수가 호출되면 statement는 initialized 상태가 된다.
    //
    //   @ aStatement : 대상 statement
    //
    //   @ aSession   : statement처리시 참조할 세션 정보
    //                  같은 세션안의 모든 statement들은
    //                  session 정보를 공유한다.
    //
    static IDE_RC initializeStatement( qciStatement *aStatement,
                                       qciSession   *aSession,
                                       qciStmtInfo  *aStmtInfo,
                                       idvSQL       *aStatistics );

    // finalizeStatement()
    // initialize할 때 할당 받았던 메모리들을 모두 해제한다.
    // 이 함수가 호출되면 alloced 상태가 된다.
    //
    //   @ aStatement : 대상 statement
    //
    static IDE_RC finalizeStatement( qciStatement *aStatement );

    // clearStatement()
    // statement의 상태를 뒤로 돌린다.
    // parse, prepare, bind, execute 시에 저장된 정보들을 삭제한다.
    //
    //   @ aStatement : 대상 statement
    //
    //   @ aTargetState : 되돌릴 상태
    //       - INITIALIZED : 모든 정보들을 삭제하고 초기화한다.
    //       - PARAM_DATA_BOUND : execution 정보를 날리고
    //                            bind된 후의 상태로 돌린다.
    //       - 그외        : 상태전이 에러 발생
    //
    static IDE_RC clearStatement( qciStatement *aStatement,
                                  smiStatement *aSmiStmt,
                                  qciStmtState  aTargetState );

    // getCurrentState()
    // statement의 현재 상태를 돌려준다.
    //
    //   @ aStatement : 대상 statement
    //
    //   @ aState     : 대상 statement의 상태 (Output)
    //
    static IDE_RC getCurrentState( qciStatement *aStatement,
                                   qciStmtState *aState );


    /*************************************************************************
     *
     * Query 처리 함수들
     * parsing -> prepare -> bindParamInfo -> bindParamData
     *         -> execute -> fetch의 과정을 거친다.
     *
     *************************************************************************/

    // parse()
    // initialized 상태의 statement에 대해 parsing 작업을 수행한다.
    // 입력받은 query string의 주소를 저장한다.
    // queryString은 mm에서 관리한다.
    //
    //   @ aStatement   : 대상 statement
    //
    //   @ aQueryString : parsing할 SQL text가 저장된 공간의 주소
    //
    //   @ aQueryLen    : SQL text의 길이
    //
    static IDE_RC parse( qciStatement *aStatement,
                         SChar        *aQueryString,
                         UInt          aQueryLen );

    // prepare()
    // validation, optimization 작업을 수행한다.
    // stored procedure를 처리하기 위해 build 작업을 수행한다.
    // 내부적으로 parse tree가 완성되고, execution plan이 생성된다.
    // 그리고 prepared된 statement에 대해 cursor flag를 얻는다.
    // 이 flag를 통해서
    // memory table에 접근하는지 disk table에 접근하는지
    // 정보를 알 수 있다.
    //
    //   @ aStatement     : 대상 statement
    //
    //   @ aParentSmiStmt : 최상위 smiStatement, 즉 dummy statement.
    //                      prepare에 필요한 smiStatement는
    //                      QP 내부에서 생성한다.
    //
    //   @ aSmiStmtCursorFlag : prepared된 statement가 생성한
    //                          smi statement에 대한
    //                          cursor flag를 얻는다.
    //   @ aDirectFlag : direct execute의 경우
    //                   prepare를 수행하고
    //                   QCI_STMT_STATE_PREPARED_DIRECT 상태가 된다.
    // PROJ-1436 SQL-Plan Cache.
    static IDE_RC hardPrepare( qciStatement           *aSatement,
                               smiStatement           *aParentSmiStmt,
                               qciSQLPlanCacheContext *aPlanCacheContext,
                               smiDistTxInfo          *aDistTxInfo = NULL );

    //-------------------------------------
    // BIND COLUMN ( target column에 대한 정보 )
    //-------------------------------------
    static IDE_RC setBindColumnInfo( qciStatement  * aStatement,
                                     qciBindColumn * aBindColumn );

    static IDE_RC getBindColumnInfo( qciStatement  * aStatement,
                                     qciBindColumn * aBindColumn );

    /* PROJ-2160 CM Type remove */
    static void getFetchColumnInfo( qciStatement       * aStatement,
                                    UShort               aBindId,
                                    qciFetchColumnInfo * aFetchColumnInfo);

    // executed 상태의 select 구문에 대해 fetch 작업을 수행한다.
    static IDE_RC fetchColumn( qciStatement           * aStatement,
                               UShort                   aBindId,
                               qciFetchColumnCallback   aFetchColumnCallback,
                               void                   * aFetchColumnContext );

    static IDE_RC fetchColumn( iduMemory     * aMemory,
                               qciStatement  * aStatement,
                               UShort          aBindId,
                               mtcColumn     * aColumn,
                               void          * aData );

    static IDE_RC checkBindColumnCount( qciStatement  * aStatement,
                                        UShort          aBindColumnCount );

    //-------------------------------------
    // BIND PARAMETER ( host 변수에 대한 정보 )
    //-------------------------------------

    static IDE_RC setBindParamInfo( qciStatement  * aStatement,
                                    qciBindParam  * aBindParam );

    static IDE_RC getBindParamInfo( qciStatement  * aStatement,
                                    qciBindParam  * aBindParam );
    /* prj-1697 */
    static IDE_RC setBindParamInfoSet( qciStatement                 * aStatement,
                                       void                         * aBindContext,
                                       qciReadBindParamInfoCallback   aReadBindParamInfoCallback);

    static IDE_RC setBindParamDataOld( qciStatement                * aStatement,
                                       UShort                        aBindId,
                                       qciSetParamDataCallback       aSetParamDataCallback,
                                       void                        * aSetBindParamDataContext);

    static IDE_RC setBindParamData( qciStatement  * aStatement,
                                    UShort          aBindId,
                                    void          * aData,
                                    UInt            aDataSize );

    static IDE_RC setBindParamDataSetOld( qciStatement                * aStatement,
                                          void                        * aBindContext,
                                          qciSetParamDataCallback       aSetParamDataCallback,
                                          qciReadBindParamDataCallback  aReadBindParamDataCallback );

    /* PROJ-2160 CM Type remove */
    static IDE_RC setBindParamData( qciStatement                 *aStatement,
                                    UShort                        aBindId,
                                    UChar                        *aParamData,
                                    qciCopyBindParamDataCallback  aCopyBindParamDataCallback );

    /* PROJ-2160 CM Type remove */
    static IDE_RC setBindParamDataSet( qciStatement                 *aStatement,
                                       UChar                        *aParamData,
                                       UInt                         *aOffset,
                                       qciCopyBindParamDataCallback  aCopyBindParamDataCallback );

    static IDE_RC getBindParamData( qciStatement                * aStatement,
                                    UShort                        aBindId,
                                    qciGetBindParamDataCallback   aGetBindParamDataCallback,
                                    void                        * aGetBindParamDataContext );

    static IDE_RC getBindParamData( qciStatement  * aStatement,
                                    UShort          aBindId,
                                    void          * aData );

    static IDE_RC checkBindParamCount( qciStatement  * aStatement,
                                       UShort          aBindParamCount );

    /* PROJ-2160 CM Type remove */
    static UInt getBindParamType( qciStatement * aStatement,
                                  UInt           aBindID );

    static UInt getBindParamInOutType( qciStatement  * aStatement,
                                       UInt            aBindID );

    static IDE_RC getOutBindParamSize( qciStatement * aStatement,
                                       UInt         * aOutBindParamSize,
                                       UInt         * aOutBindParamCount );
    
    //--------------------------------------
    // parameter의 info나 data에 대한 상태전이 함수.
    // (1) 쿼리수행시
    //     initialized->parse->(direct)prepared
    //     ->bindParamInfo->bindParamData->execute
    // (2) execute 중
    //     parameter의 info나 data 정보가 변경되었을 경우
    // 상태전이를 위한 함수.
    //--------------------------------------
    static IDE_RC bindParamInfo( qciStatement           * aStatement,
                                 qciSQLPlanCacheContext * aPlanCacheContext );

    static IDE_RC setParamDataState( qciStatement * aStatement );

    static IDE_RC setBindTuple( qciStatement * aStatement );


    // execute()
    // execution plan을 시작한다.
    // 이 함수가 수행되기 위해서는 statement가 반드시
    // bound 상태이어야 한다.
    // 이외의 경우는 error가 발생한다.
    // execution에 필요한 smiStatement를 직접 인자로 전달해주어야 한다.
    // 이 smiStatement는 statement의 타입(DML, DDL, DCL 등)에 따라
    // 또는 세션이 auto commit/non auto commit 모드에 따라 다르기 때문에
    // 세션 레벨에서 결정해서 넘겨줘야 한다.
    //
    // execution도중에 rebuild error가 발생할 수 있다.
    //   - execution이 시작된 시점에서 stored procedure에 대한
    //     DDL 작업이 있었으면 rebuild error를 발생시킨다.
    //   - execution plan을 실행하는 도중 plan이 유효하지 않다고
    //     판단되면 rebuild error를 발생시킨다.
    // MM에서는 rebuild 에러를 받게 되면 qci::rebuild를 통해서
    // rebuild 작업을 수행할 수 있다.
    //
    //   @ aStatement     : 대상 statement
    //
    //   @ aSmiStmt       : execution을 수행할 smiStatement
    //                      statement의 타입에 따라 또는 auto commit
    //                      모드에 따라 다른 smiStatement를 넘겨줘야 한다.
    //                      prepare때는 dummy statement를 넘겨줬지만
    //                      execute시에는 실제 statement를 넘긴다.
    //
    static IDE_RC execute( qciStatement * aStatement,
                           smiStatement * aSmiStmt );

    // fetch시 fetch할 레코드 유무 반환.
    static IDE_RC moveNextRecord( qciStatement * aStatement,
                                  smiStatement * aSmiStmt,
                                  idBool       * aRecordExist );


    // hardRebuild()
    // execute를 호출했을 때 rebuild error를 받으면 이 함수를 통해서
    // rebuild 작업을 수행한다.
    // 내부적으로 parse, prepare, bindParamInfo, bindParamData 작업을 한다.
    // 따라서 prepare시에 넘겨받던 aSmiStmtCursorFlag를 여기서도
    // 넘겨받을 수 있다.
    // prepare시에 필요한 parent smi statement는 mm으로부터 다시 받아야 함.
    // ( rebuild시 smiStatement end & begin을 수행해야 하므로,
    //   qp에서 따로 관리를 할 수 없으므로,
    //   mm으로부터 smiStatement를 받아서 처리해야 함. )
    //
    //   @ aStatement     : 대상 statement
    //   @ aParentSmiStmt : root statement
    //   @ aSmiStmtCursorFlag : prepared된 statement가 생성한
    //                          smi statement에 대한
    //                          cursor flag를 얻는다.
    //
    static IDE_RC hardRebuild( qciStatement            * aStatement,
                               smiStatement            * aSmiStmt,
                               smiStatement            * aParentSmiStmt,
                               qciSQLPlanCacheContext  * aPlanCacheContext,
                               SChar                   * aQueryString,
                               UInt                      aQueryLen,
                               smiDistTxInfo           * aDistTxInfo = NULL );

    // retry()
    // execute를 호출했을때 retry error를 받으면,
    // 이 함수를 통해서,
    // retry 수행을 위한 상태전이 및 관련작업을 수행한다.
    // 이 함수내에서는 execute를 수행할 수 있는 직전 상태로 되돌린다.
    static IDE_RC retry( qciStatement * aStatement,
                         smiStatement * aSmiStmt );

    /*************************************************************************
     *
     * PARSED 상태부터 호출할 수 있는 함수들
     *
     *************************************************************************/

    // getStmtType()
    // parsed 상태의 statement에 대해 parsing된 구문의 종류를 얻어온다.
    //
    //   @ aStatement   : 대상 statement
    //
    //   @ aType        : 대상 statement의 유형
    //                    (DDL, DML, DCL, INSERT, UPDATE, DELETE, SELECT,...)
    //
    static IDE_RC getStmtType( qciStatement  *aStatement,
                               qciStmtType   *aType );

    // PROJ-1386 Dynamic-SQL
    static IDE_RC checkInternalSqlType( qciStmtType aType );

    static IDE_RC checkInternalProcCall( qciStatement * aStatement );

    // hasFixedTableView()
    // parsed 상태의 statement에 대해 fixed table이나 performance view를
    // 참조하는지에 대한 여부를 구한다.
    //
    //   @ aStatement : 대상 statement
    //
    //   @ aHas       : parsed된 statement가 fixed table이나 performance view
    //                 를 참조하면 ID_TRUE가, 아니면 ID_FALSE를
    //                 반환받는다.
    //
    static IDE_RC hasFixedTableView( qciStatement *aStatement,
                                     idBool       *aHas );

    static IDE_RC getShardAnalyzeInfo( qciStatement         * aStatement,
                                       qciShardAnalyzeInfo ** aAnalyzeInfo );

    /*************************************************************************
     *
     * DCL 실행 함수들
     *
     *************************************************************************/

    // DCL류의 statement를 execute한다.
    // 이 함수를 호출하기 위해서는 bound상태이어야 하며
    // 호출 후에는 executed 상태가 된다.
    //
    //   @ aStatement  : 대상 statement
    //
    static IDE_RC executeDCL( qciStatement *aStatement,
                              smiStatement *aSmiStmt,
                              smiTrans     *aSmiTrans );

    /*************************************************************************
     * PROJ-2551 simple query 최적화
     * fast execute 함수들
     *************************************************************************/

    // simple query인가?
    static idBool isSimpleQuery( qciStatement * aStatement );
    
    // fast execute와 fetch를 수행한다.
    static IDE_RC fastExecute( smiTrans      * aSmiTrans,
                               qciStatement  * aStatement,
                               UShort        * aBindColInfo,
                               UChar         * aBindBuffer,
                               UInt            aShmSize,
                               UChar         * aShmResult,
                               UInt          * aRowCount );

    /*************************************************************************
     *
     *
     *
     *************************************************************************/

    // connect Protocol 로 처리되는 곳에서 호출됨.
    // 이 부분에서는 parsetree정보, session정보가 없음.
    // ( mmtCmsConnection::connectProtocol() )
    // 이 함수가 기존에 mm에서 수행하던 아래 작업을 일괄 수행한다.
    // (1) qcmUserID함수 인자로 넘기기 위한
    //     userName으로 qcNamePosition정보 구성작업
    // (2) qci2::setSmiStmt()
    // (3) qci2::setStmtText()
    // (4) user정보 구성.
    static IDE_RC getUserInfo( qciStatement  *aStatement,
                               smiStatement  *aSmiStmt,
                               qciUserInfo   *aResult );

    /* PROJ-2207 Password policy support */
    static IDE_RC passwPolicyCheck( idvSQL      *aStatistics, /* PROJ-2446 */
                                    qciUserInfo *aUserInfo );

    static IDE_RC updatePasswPolicy( idvSQL         *aStatistics, /* PROJ-2446 */
                                     qciUserInfo    *aUserInfo );
    
    // QP에 callback 함수들을 넘긴다.
    // 반드시 서버 구동시에 호출되어야 한다.
    //
    // 파라미터 설명은 생략함
    //
    static IDE_RC setDatabaseCallback(
        qciDatabaseCallback        aCreatedbFuncPtr,
        qciDatabaseCallback        aDropdbFuncPtr,
        qciDatabaseCallback        aCloseSession,
        qciDatabaseCallback        aCommitDTX,
        qciDatabaseCallback        aRollbackDTX,
        qciDatabaseCallback        aStartupFuncPtr,
        qciDatabaseCallback        aShutdownFuncPtr );

    // mmcSession의 정보를 참조하기 위한 callback 함수 설정.
    static IDE_RC setSessionCallback( qciSessionCallback *aCallback );
    //PROJ-1677
    static IDE_RC setQueueCallback( qciQueueCallback aQueueCreateFuncPtr,
                                    qciQueueCallback aQueueDropFuncPtr,
                                    qciQueueCallback aQueueEnqueueFuncPtr,
                                    qciDeQueueCallback aQueueDequeueFuncPtr,
                                    qciQueueCallback   aSetQueueStampFuncPtr);

    static IDE_RC setParamData4RebuildCallback( qciSetParamData4RebuildCallback    aCallback );

    static IDE_RC setOutBindLobCallback(
        qciOutBindLobCallback      aOutBindLobCallback,
        qciCloseOutBindLobCallback aCloseOutBindLobCallback );

    static IDE_RC setInternalSQLCallback( qciInternalSQLCallback * aCallback );

    /* PROJ-1832 New database link */
    static IDE_RC setDatabaseLinkCallback( qciDatabaseLinkCallback * aCallback );
    static IDE_RC setRemoteTableCallback( qciRemoteTableCallback * aCallback );
    
    /* PROJ-2223 Altibase Auditing */
    static IDE_RC setAuditManagerCallback( qciAuditManagerCallback *aCallback );

    /* PROJ-2626 Snapshot Export */
    static IDE_RC setSnapshotCallback( qciSnapshotCallback * aCallback );

    static IDE_RC startup( idvSQL         * aStatistics,
                           qciStartupPhase  aStartupPhase );

    static qciStartupPhase getStartupPhase();

    static idBool isSysdba( qciStatement * aStatement );

    // DML 수행후, 영향을 받은 레코드 수 반환
    // BUG-44536 Affected row count와 fetched row count를 분리
    //   1. affected row count에 일단 row count를 담고
    //   2. SELECT, SELECT FOR UPDATE이면 fetched row count로 옮긴 후
    //      affected row count를 0으로 만든다.
    inline static void getRowCount( qciStatement * aStatement,
                                    SLong        * aAffectedRowCount,
                                    SLong        * aFetchedRowCount )
    {
        qcStatement * sStatement = &aStatement->statement;
        qcTemplate  * sTemplate;
        qciStmtType   sStmtType = QCI_STMT_MASK_MAX;

        // PROJ-2551 simple query 최적화
        if ( ( ( sStatement->mFlag & QC_STMT_FAST_EXEC_MASK )
               == QC_STMT_FAST_EXEC_TRUE ) &&
             ( ( sStatement->mFlag & QC_STMT_FAST_BIND_MASK )
               == QC_STMT_FAST_BIND_TRUE ) )
        {
            sStmtType = sStatement->myPlan->parseTree->stmtKind;
            *aAffectedRowCount = sStatement->simpleInfo.numRows;
        }
        else
        {
            sTemplate = QC_PRIVATE_TMPLATE(sStatement);
            
            if ( sTemplate != NULL )
            {
                if ( (sStatement->myPlan->parseTree->stmtKind & QCI_STMT_MASK_SP) == QCI_STMT_MASK_SP )
                {
                    sStmtType = sTemplate->stmtType;
                    *aAffectedRowCount = sTemplate->numRows;
                }
                else
                {
                    sStmtType = sStatement->myPlan->parseTree->stmtKind;
                    *aAffectedRowCount = sTemplate->numRows;
                }
            }
            else
            {
                *aAffectedRowCount = -1;
            }
        }

        if ( (sStmtType == QCI_STMT_SELECT) ||
             (sStmtType == QCI_STMT_SELECT_FOR_UPDATE) )
        {
            *aFetchedRowCount = *aAffectedRowCount;
            *aAffectedRowCount = -1;
        }
        else
        {
            *aFetchedRowCount = -1;
        }
    }

    // graph와 plan tree의 text정보를 반환
    static IDE_RC getPlanTreeText( qciStatement * aStatement,
                                   iduVarString * aString,
                                   idBool         aIsCodeOnly );

    static IDE_RC getPlanTreeTextForFixedTable( qciStatement * aStatement,
                                                iduVarString * aString,
                                                idBool         aIsCodeOnly );

    static SInt   getLineNo( SChar * aStmtText, SInt aOffset );

    static IDE_RC makePlanTreeText( qciStatement * aStatement,
                                    iduVarString * aString,
                                    idBool         aIsCodeOnly,
                                    idBool         aIsNoGraph );

    static IDE_RC printPlanTreeText( qcStatement  * aStatement,
                                     qcTemplate   * aTemplate,
                                     qmgGraph     * aGraph,
                                     qmnPlan      * aPlan,
                                     qmnDisplay     aDisplay,
                                     iduVarString * aString,
                                     idBool         aIsNoGraph );

    inline static UShort getColumnCount( qciStatement * aStatement )
    {
        return aStatement->statement.myPlan->sBindColumnCount;
    }

    inline static UShort getParameterCount( qciStatement * aStatement )
    {
        return aStatement->statement.pBindParamCount;
    }

    static idBool isLastParamData( qciStatement * aStatement,
                                   UShort         aBindParamId );

    // execute시의 cursor를 닫고, temp table을 제거한다.
    static IDE_RC closeCursor( qciStatement * aStatement,
                               smiStatement * aSmiStmt );

    static IDE_RC refineStackSize( qciStatement * aStatement );

    static IDE_RC checkRebuild( qciStatement * aStatement );

    static UShort getResultSetCount( qciStatement * aStatement );

    static IDE_RC getResultSet( qciStatement * aStatement,
                                UShort         aResultSetID,
                                void        ** aResultSetStmt,
                                idBool       * aInterResultSet,
                                idBool       * aRecordExist );

    /* PROJ-2160 CM Type remove */
    static ULong getRowActualSize( qciStatement * aStatement );

    // fix BUG-17715
    static IDE_RC getRowSize( qciStatement * aStatement,
                              UInt         * aSize );

    // BUG-25109
    // simple query에 사용된 base table name을 반환한다.
    static IDE_RC getBaseTableInfo( qciStatement * aStatement,
                                    SChar        * aTableOwnerName,
                                    SChar        * aTableName,
                                    idBool       * aIsUpdatable );

    //PROJ-1436 SQL-Plan Cache.
    static IDE_RC isMatchedEnvironment( qciStatement    * aStatement,
                                        qciPlanProperty * aPlanProperty,
                                        qciPlanBindInfo * aPlanBindInfo, // PROJ-2163
                                        idBool          * aIsMatched );

    //rebuild시 변경되는 environment를 기록한다.
    static IDE_RC rebuildEnvironment( qciStatement    * aStatement,
                                      qciPlanProperty * aEnv );

    //soft-prepare과정에서 plan에 대한 validation을 수행한다.
    static IDE_RC validatePlan(
        qciGetSmiStatement4PrepareCallback   aGetSmiStmt4PrepareCallback,
        void                               * aGetSmiStmt4PrepareContext, 
        qciStatement                       * aStatement,
        void                               * aSharedPlan,
        idBool                             * aIsValidPlan );

    //soft-prepare과정에서 plan에 대한 권한을 검사한다.
    static IDE_RC checkPrivilege(
        qciGetSmiStatement4PrepareCallback   aGetSmiStmt4PrepareCallback,
        void                               * aGetSmiStmt4PrepareContext, 
        qciStatement                       * aStatement,
        void                               * aSharedPlan );

    // shared template을 private template으로 설정한다.
    static IDE_RC setPrivateTemplate( qciStatement            * aStatement,
                                      void                    * aSharedPlan,
                                      qciSqlPlanCacheInResult   aInResult );

    // shared template을 복사하여 private template을 생성한다.
    static IDE_RC clonePrivateTemplate( qciStatement            * aStatement,
                                        void                    * aSharedPlan,
                                        void                    * aPrepPrivateTemplate,
                                        qciSQLPlanCacheContext  * aPlanCacheContext );

    // shared plan memory를 해제한다.
    static IDE_RC freeSharedPlan( void * aSharedPlan );

    // prepared private template을 해제한다.
    static IDE_RC freePrepPrivateTemplate( void * aPrepPrivateTemplate );
    
    // PROJ-1518 Atomic Array Insert
    static IDE_RC atomicBegin( qciStatement  * aStatement,
                               smiStatement  * aSmiStmt );

    static IDE_RC atomicInsert( qciStatement  * aStatement );

    // PROJ-2163
    static IDE_RC atomicSetPrepareState( qciStatement * aStatement );

    static IDE_RC atomicEnd( qciStatement  * aStatement );

    static void atomicFinalize( qciStatement  * aStatement,
                                idBool        * aIsCursorOpen,
                                smiStatement  * aSmiStmt );
    
    // PROJ-1436 prepared template을 삭제한다.
    static IDE_RC freePrepTemplate( qcPrepTemplate * aPrepTemplate );

    // PROJ-2163
    // Plan 의 호스트 변수 타입과 사용자 바인드 타입 비교
    static idBool isBindChanged( qciStatement * aStatement );

    // D$ 테이블 또는 NO_PLAN_CACHE 힌트 사용 여부 판단
    static idBool isCacheAbleStatement( qciStatement * aStatement );

    // 바인드 메모리를 유지하고 statement 를 clear
    static IDE_RC clearStatement4Reprepare( qciStatement  * aStatement,
                                            smiStatement  * aSmiStmt );

    // qcg::setPrivateArea 함수의 wrapper
    static IDE_RC setPrivateArea( qciStatement * aStatement );


    /* PROJ-2240 */
    static IDE_RC setReplicationCallback(
            qciValidateReplicationCallback    aValidateCallback,
            qciExecuteReplicationCallback     aExecuteCallback,
            qciCatalogReplicationCallback     aCatalogCallback,
            qciManageReplicationCallback      aManageCallback );

    // PROJ-2223 audit
    static void getAllRefObjects( qciStatement       * aStatement,
                                  qciAuditRefObject ** aRefObjects,
                                  UInt               * aRefObjectCount );

    // PROJ-2223 audit
    static void getAuditOperation( qciStatement    * aStatement,
                                   qciAuditOperIdx  * aOperIndex,
                                   const SChar     ** aOperString );
    /* BUG-41986 */
    static void getAuditOperationByOperID( qciAuditOperIdx  aOperIndex, 
                                           const SChar    **aOperString );

    /* BUG-36205 Plan Cache On/Off property for PSM */
    static idBool isCalledByPSM( qciStatement * aStatement );

    /* BUG-38496 Notify users when their password expiry date is approaching */
    static UInt getRemainingGracePeriod( qciUserInfo * aUserInfo );

    /* BUG-39441
     * need a interface which returns whether DML on replication table or not */
    static idBool hasReplicationTable(qciStatement* aStatement);

    /* PROJ-2474 SSL/TLS Support */
    static IDE_RC checkDisableTCPOption( qciUserInfo *aUserInfo );

    // BUG-41248 DBMS_SQL package
    static IDE_RC setBindParamInfoByName( qciStatement  * aStatement,
                                          qciBindParam  * aBindParam );

    static IDE_RC setBindParamInfoByNameInternal( qciStatement  * aStatement,
                                                  qciBindParam  * aBindParam );

    static idBool isBindParamEnd( qciStatement * aStatement );

    static IDE_RC setBindParamDataByName( qciStatement  * aStatement,
                                          SChar         * aBindName,
                                          void          * aData,
                                          UInt            aDataSize );

    static IDE_RC setBindParamDataByNameInternal( qciStatement  * aStatement,
                                                  UShort          aBindId,
                                                  void          * aData,
                                                  UInt            aDataSize );

    static idBool isBindDataEnd( qciStatement * aStatement );

    static IDE_RC checkBindParamByName( qciStatement  * aStatement,
                                        SChar         * aBindName );

    // BUG-43158 Enhance statement list caching in PSM
    static IDE_RC initializeStmtListInfo( qcStmtListInfo * aStmtListInfo );

    // BUG-43158 Enhance statement list caching in PSM
    static IDE_RC finalizeStmtListInfo( qcStmtListInfo * aStmtListInfo );

    /* PROJ-2626 Snapshot Export */
    static void setInplaceUpdateDisable( idBool aTrue );
    static idBool getInplaceUpdateDisable( void );

    /* BUG-46090 Meta Node SMN 전파 */
    static void clearShardDataInfo( qciStatement * aStatement );

    static void clearShardDataInfoForRebuild( qciStatement * aStatement );

    /* PROJ-2701 Sharding online data rebuild */
    static IDE_RC checkShardPlanRebuild( qcStatement * aStatement );

    static qcStatement * getSelectStmtOfDDL( qcStatement * aStatement );

    // BUG-47790
    static IDE_RC reloadShardMetaNumber( qciStatement * aStatement,
                                         void         * aSession );
    
    static IDE_RC setPropertyForShardMeta( qciStatement * aStatement );
    
    static IDE_RC revertPropertyForShardMeta( qciStatement *aStatement );

    static idBool isShardDbmsPkg( qciStatement * aStatement );

    /* TASK-7219 Non-shard DML */
    static IDE_RC setShardPartialExecType( qciStatement            * aStatement,
                                           sdiShardPartialExecType   aShardPartialExecType );
};

/*****************************************************************************
 *
 * qciMisc class에는 qciStatement를 인자로 받지 않는
 * 함수들로 구성된다.
 *
 *****************************************************************************/
class qciMisc {
public:
    /*************************************************************************
     * 입력된 사용자 이름으로 사용자 정보를 얻어온다.
     *
     * - aUserName     : (input) 사용자 이름 문자열
     * - aUserNameLen  : (input) 사용자 이름 문자열 길이
     * - aUserID       : (output) 사용자 아이디
     * - aUserPassword : (output) 사용자 암호; authorization에 사용된다.
     * - aTableID      : (output) 사용자의 tablespace ID
     * - aTempID       : (output) 사용자의 temp tablespace ID
     *************************************************************************/

    static UInt getQueryStackSize( );
    // BUG-26017 [PSM] server restart시 수행되는 psm load과정에서
    // 관련프로퍼티를 참조하지 못하는 경우 있음.
    static UInt getOptimizerMode();
    static UInt getAutoRemoteExec();    
    // BUG-23780 TEMP_TBS_MEMORY 힌트 적용여부를 property로 제공
    static UInt getOptimizerDefaultTempTbsType();    

    static idBool isStmtDDL( qciStmtType aStmtType );
    static idBool isStmtDML( qciStmtType aStmtType );
    static idBool isStmtDCL( qciStmtType aStmtType );
    static idBool isStmtSP( qciStmtType aStmtType );
    static idBool isStmtDB( qciStmtType aStmtType );

    static UInt getStmtType( qciStmtType aStmtType );

    inline static UInt getStmtTypeNumber( qciStmtType aStmtType )
    {
        return ( getStmtType(aStmtType) >> 16 );
    }


    static IDE_RC buildPerformanceView( idvSQL * aStatistics );

    // PROJ-1726
    static IDE_RC initializePerformanceViewManager();
    static IDE_RC finalizePerformanceViewManager();
    static IDE_RC addPerformanceView( SChar* aPerfViewStr );

    static IDE_RC createDB( idvSQL * aStatistics,
                            SChar  * aDBName,
                            SChar  * aOwnerDN,
                            UInt     aOwnerDNLen );

    static IDE_RC getLanguage( SChar * aLanguage, mtlModule ** aModule );

    
    static IDE_RC getPartitionInfoList( void                  * aQcStatement,
                                        smiStatement          * aSmiStmt,
                                        iduVarMemList         * aMem,
                                        UInt                    aTableID,
                                        qcmPartitionInfoList ** aPartInfoList );

    static IDE_RC getPartitionInfoList( void                  * aQcStatement,
                                        smiStatement          * aSmiStmt,
                                        iduMemory             * aMem,
                                        UInt                    aTableID,
                                        qcmPartitionInfoList ** aPartInfoList );

    static IDE_RC getPartMinMaxValue( smiStatement * aSmiStmt,
                                      UInt           aPartitionID,
                                      SChar        * aMinValue,
                                      SChar        * aMaxValue );

    static IDE_RC getPartitionOrder( smiStatement * aSmiStmt,
                                     UInt           aTableID,
                                     UChar        * aPartitionName,
                                     SInt           aPartitionNameSize,
                                     UInt         * aPartOrder );

    // Proj-2059 DB Upgrade
    // TableInfo를 구하는 부분과 CondValue를 검사하는 함수를
    // Wrapping 함수 형태로 분리
    static IDE_RC comparePartCondValues( idvSQL           * aStatistics,
                                         qcmTableInfo     * aTableInfo,
                                         SChar            * aValue1,
                                         SChar            * aValue2,
                                         SInt             * aResult );

    static IDE_RC comparePartCondValues( idvSQL           * aStatistics,
                                         SChar            * aTableName,
                                         SChar            * aUserName,
                                         SChar            * aValue1,
                                         SChar            * aValue2,
                                         SInt             * aResult );

    // 하나의 transaction 으로 처리함
    // qcStatement 를 변수로 이용
    static IDE_RC comparePartCondValues( void             * aQcStatement,
                                         qcmTableInfo     * aTableInfo,
                                         SChar            * aValue1,
                                         SChar            * aValue2,
                                         SInt             * aResult );

    static IDE_RC comparePartCondValues( smiStatement     * aSmiStatement,
                                         SChar            * aTableName,
                                         SChar            * aUserName,
                                         SChar            * aValue1,
                                         SChar            * aValue2,
                                         ULong              aTimeout,
                                         SInt             * aResult );

    /* BUG-41986 */
    static IDE_RC getUserIdByName( SChar             * aUserName,
                                   UInt              * aUserID );

    /* BUG-37480 Check Constraint Semantic Comparision */
    static IDE_RC isEquivalentExpressionString( SChar  * aExprString1,
                                                SChar  * aExprString2,
                                                idBool * aIsEquivalent );

    //-----------------------------------
    // PROJ-1362
    // SM LOB 함수의 wrapper
    //-----------------------------------

    static IDE_RC lobRead( idvSQL       * aStatistics,
                           smLobLocator   aLocator,
                           UInt           aOffset,
                           UInt           aMount,
                           UChar        * aPiece );

    static IDE_RC clobRead( idvSQL       * aStatistics,
                            smLobLocator   aLocator,
                            UInt           aOffset,
                            UInt           aByteLength,
                            UInt           aCharLength,
                            UChar        * aBuffer,
                            mtlModule    * aLanguage,
                            UInt         * aReadByteLength,
                            UInt         * aReadCharLength );

    /* PROJ-2047 Strengthening LOB - Removed aOldSize */
    static IDE_RC lobPrepare4Write( idvSQL     * aStatistics,
                                    smLobLocator aLocator,
                                    UInt         aOffset,
                                    UInt         aSize);

    /* PROJ-2047 Strengthening LOB - Removed aOffset */
    static IDE_RC lobWrite( idvSQL      *  aStatistics,
                            smLobLocator   aLocator,
                            UInt           aPieceLen,
                            UChar        * aPiece );

    static IDE_RC lobFinishWrite( idvSQL * aStatistics,
                                  smLobLocator aLocator );

    static IDE_RC lobCopy( idvSQL       * aStatistics,
                           smLobLocator aDestLocator,
                           smLobLocator aSrcLocator );

    static idBool lobIsOpen( smLobLocator aLocator );

    static IDE_RC lobGetLength( idvSQL       * aStatistics,
                                smLobLocator   aLocator,
                                UInt         * aLength,
                                idBool       * aIsNullLob = NULL );

    /* PROJ-2047 Strengthening LOB - Added Interfaces */
    static IDE_RC lobTrim(idvSQL       *aStatistics,
                          smLobLocator  aLocator,
                          UInt          aOffset);

    static IDE_RC lobFinalize( idvSQL       * aStatistics,
                               smLobLocator   aLocator );

    //-----------------------------------
    // PROJ-2002 Column Security
    // startup, shutdown security module
    //-----------------------------------

    static IDE_RC startSecurity( idvSQL * aStatistics );

    static IDE_RC stopSecurity( void );
    
    //-----------------------------------
    // PROJ-2002 Column Security
    // verify seucity module
    //-----------------------------------
    
    static IDE_RC getECCPolicyName( SChar  * aECCPolicyName );
    
    static IDE_RC getECCPolicyCode( UChar   * aECCPolicyCode,
                                    UShort  * aECCPolicyCodeSize );
    
    static IDE_RC verifyECCPolicyCode( UChar   * aECCPolicyCode,
                                       UShort    aECCPolicyCodeSize,
                                       idBool  * aIsValid );
    
    static IDE_RC getPolicyCode( SChar   * aPolicyName,
                                 UChar   * aPolicyCode,
                                 UShort  * aPolicyCodeLength );
    
    static IDE_RC verifyPolicyCode( SChar  * aPolicyName,
                                    UChar  * aPolicyCode,
                                    UShort   aPolicyCodeLength,
                                    idBool * aIsValid );
    
    // PROJ-2223 audit
    static IDE_RC getAuditMetaInfo( idBool              * aIsStarted,
                                    qciAuditOperation  ** aObjectOptions,
                                    UInt                * aObjectOptionCount,
                                    qciAuditOperation  ** aUserOptions,
                                    UInt                * aUserOptionCount );
    
public:

    static IDE_RC addExtFuncModule();

    static IDE_RC addExtRangeFunc();

    static IDE_RC addExtCompareFunc();
    
    static IDE_RC addExternModuleCallback( qcmExternModule * aExternModule );

    static IDE_RC touchTable( smiStatement * aSmiStmt,
                              UInt           aTableID );

    static IDE_RC getTableInfoByID( smiStatement    * aSmiStmt,
                                    UInt              aTableID,
                                    qcmTableInfo   ** aTableInfo,
                                    smSCN           * aSCN,
                                    void           ** aTableHandle );

    static IDE_RC validateAndLockTable( void             * aQcStatement,
                                        void             * aTableHandle,
                                        smSCN              aSCN,
                                        smiTableLockMode   aLockMode );

    static IDE_RC getUserName(void        * aQcStatement,
                              UInt          aUserID,
                              SChar       * aUserName);

    static IDE_RC makeAndSetQcmTableInfo(
        smiStatement * aSmiStmt,
        UInt           aTableID,
        smOID          aTableOID,
        const void   * aTableRow = NULL );

    static IDE_RC makeAndSetQcmPartitionInfo(
        smiStatement * aSmiStmt,
        UInt           aTableID,
        smOID          aTableOID,
        qciTableInfo * aTableInfo );

    static void destroyQcmTableInfo( qcmTableInfo *aInfo );

    static void destroyQcmPartitionInfo( qcmTableInfo *aInfo );

    static IDE_RC getDiskRowSize( void * aTableHandle,
                                  UInt * aRowSize );

    static IDE_RC copyMtcColumns( void         * aTableHandle,
                                  mtcColumn    * aColumns );

    /* PROJ-1594 Volatile TBS */
    /* SM에서 callback으로 호출되는 함수로서,
       null row를 생성한다. */
    static IDE_RC makeNullRow(idvSQL        *aStatistics,   /* PROJ-2446 */ 
                              smiColumnList *aColumnList,
                              smiValue      *aNullRow,
                              SChar         *aValueBuffer);

    // PROJ-1723
    static IDE_RC writeDDLStmtTextLog( qcStatement    * aStatement,
                                       UInt             aUID,
                                       SChar          * aTableName );

    // BUG-21895
    static IDE_RC smiCallbackCheckNeedUndoRecord( smiStatement * aSmiStmt,
                                                  void         * aTableHandle,
                                                  idBool       * aIsNeed );

    // PROJ-1705
    static IDE_RC storingSize( mtcColumn  * aStoringColumn,
                               mtcColumn  * aValueColumn,
                               void       * aValue,
                               UInt       * aOutStoringSize );

    // PROJ-1705
    static IDE_RC mtdValue2StoringValue( mtcColumn  * aStoringColumn,
                                         mtcColumn  * aValueColumn,
                                         void       * aValue,
                                         void      ** aOutStoringValue );

    /* PROJ */
    static IDE_RC checkDDLReplicationPriv( void   * aQcStatement );

    static IDE_RC getUserID( void             * aQcStatement,
                             qciNamePosition    aUserName,
                             UInt             * aUserID );

    static IDE_RC getUserID( void              *aQcStatement,
                             SChar             *aUserName,
                             UInt               aUserNameSize,
                             UInt              *aUserID );

    static IDE_RC getUserID( idvSQL       * aStatistics,
                             smiStatement * aSmiStatement,
                             SChar        * aUserName,
                             UInt           aUserNameSize,
                             UInt         * aUserID );

    static IDE_RC getTableInfo( void             *aQcStatement,
                                UInt              aUserID,
                                qciNamePosition   aTableName,
                                qcmTableInfo    **aTableInfo,
                                smSCN            *aSCN,
                                void            **aTableHandle );

    static IDE_RC getTableInfo( void             *aQcStatement,
                                UInt             aUserID,
                                UChar           *aTableName,
                                SInt             aTableNameSize,
                                qcmTableInfo   **aTableInfo,
                                smSCN           *aSCN,
                                void           **aTableHandle );
    static IDE_RC getTableInfoAndLock( void            *aQcStatement,
                                       SChar           *aUserName,
                                       SChar           *aTableName,
                                       smiTableLockMode aLockMode,
                                       ULong            aTimeout,
                                       qcmTableInfo   **aOutTableInfo,
                                       void           **aOutTableHandle );

    static IDE_RC lockTableForDDLValidation( void      * aQcStatement,
                                             void      * aTableHandle,
                                             smSCN       aSCN );

    static IDE_RC isOperatableReplication( void        * aQcStatement,
                                           qriReplItem * aReplItem,
                                           UInt          aOperatableFlag );

    static idBool isTemporaryTable( qcmTableInfo * aTableInfo );

    static IDE_RC getPolicyInfo( SChar   * aPolicyName,
                                 idBool  * aIsExist,
                                 idBool  * aIsSalt,
                                 idBool  * aIsEncodeECC );

    static void setVarcharValue( mtdCharType * aValue,
                                 mtcColumn   * ,
                                 SChar       * aString,
                                 SInt          aLength );

    static void makeMetaRangeSingleColumn( qtcMetaRangeColumn  * aRangeColumn,
                                           const mtcColumn     * aColumnDesc,
                                           const void          * aValue,
                                           smiRange            * aRange );

    static IDE_RC runDMLforDDL( smiStatement * aSmiStmt,
                                SChar        * aSqlStr,
                                vSLong       * aRowCnt );

    static IDE_RC runDMLforInternal( smiStatement * aSmiStmt,
                                     SChar        * aSqlStr,
                                     vSLong       * aRowCnt );
    
    /* PROJ-2701 Sharding online data rebuild */
    static IDE_RC runSQLforShardMeta( smiStatement * aSmiStmt,
                                      SChar        * aSqlStr,
                                      vSLong       * aRowCnt );

    static IDE_RC selectCount( smiStatement        * aSmiStmt,
                               const void          * aTable,
                               vSLong              * aSelectedRowCount,  /* OUT */
                               smiCallBack         * aCallback = NULL,
                               smiRange            * aRange = NULL,
                               const void          * aIndex = NULL );

    static IDE_RC sysdate( mtdDateType* aDate );

    static IDE_RC getTableHandleByID( smiStatement  * aSmiStmt,
                                      UInt            aTableID,
                                      void         ** aTableHandle,
                                      smSCN         * aSCN );

    static IDE_RC getTableHandleByName( smiStatement  * aSmiStmt,
                                        UInt            aUserID,
                                        UChar         * aTableName,
                                        SInt            aTableNameSize,
                                        void         ** aTableHandle,
                                        smSCN         * aSCN );

    static void makeMetaRangeDoubleColumn( qtcMetaRangeColumn  * aFirstRangeColumn,
                                           qtcMetaRangeColumn  * aSecondRangeColumn,
                                           const mtcColumn     * aFirstColumnDesc,
                                           const void          * aFirstColValue,
                                           const mtcColumn     * aSecondColumnDesc,
                                           const void          * aSecondColValue,
                                           smiRange            * aRange);

    static void makeMetaRangeTripleColumn( qtcMetaRangeColumn  * aFirstRangeColumn,
                                           qtcMetaRangeColumn  * aSecondRangeColumn,
                                           qtcMetaRangeColumn  * aThirdRangeColumn,
                                           const mtcColumn     * aFirstColumnDesc,
                                           const void          * aFirstColValue,
                                           const mtcColumn     * aSecondColumnDesc,
                                           const void          * aSecondColValue,
                                           const mtcColumn     * aThirdColumnDesc,
                                           const void          * aThirdColValue,
                                           smiRange            * aRange);
    
    static IDE_RC getSequenceHandleByName( smiStatement     * aSmiStmt,
                                           UInt               aUserID,
                                           UChar            * aSequenceName,
                                           SInt               aSequenceNameSize,
                                           void            ** aSequenceHandle );

    static IDE_RC insertIndexTable4OneRow( smiStatement      * aSmiStmt,
                                           qciIndexTableRef  * aIndexTableRef,
                                           smiValue          * aInsertValue,
                                           smOID               aPartOID,
                                           scGRID              aRowGRID );
    
    static IDE_RC updateIndexTable4OneRow( smiStatement      * aSmiStmt,
                                           qciIndexTableRef  * aIndexTableRef,
                                           UInt                aUpdateColumnCount,
                                           UInt              * aUpdateColumnID,
                                           smiValue          * aUpdateValue,
                                           void              * aRow,
                                           scGRID              aRowGRID );
    
    static IDE_RC deleteIndexTable4OneRow( smiStatement      * aSmiStmt,
                                           qmsIndexTableRef  * aIndexTableRef,
                                           void              * aRow,
                                           scGRID              aRowGRID );

    /*
     * PROJ-1832 New database link.
     */
    
    static IDE_RC getQcStatement( mtcTemplate * aTemplate,
                                  void ** aQcStatement );

    static IDE_RC getDatabaseLinkSession( void * aQcStatement,
                                          void ** aSession );

    static IDE_RC getTempSpaceId( void * aQcStatement, scSpaceID * aSpaceId );

    static IDE_RC getSmiStatement( void * aQcStatement,
                                   smiStatement ** aStatement );

    static UInt getSessionUserID( void * aQcStatement );
        
    /*
     * for SYS_DATABASE_LINKS_ meta table.
     */ 
    static IDE_RC getNewDatabaseLinkId( void * aQcStatement,
                                        UInt * aDatabaseLinkId );
    
    static IDE_RC insertDatabaseLinkItem( void * aQcStatement,
                                          qciDatabaseLinksItem * aItem,
                                          idBool aPublicFlag );
    static IDE_RC deleteDatabaseLinkItem( void * aQcStatement,
                                          smOID aLinkOID );
    static IDE_RC deleteDatabaseLinkItemByUserId( void  * aQcStatement,
                                                  UInt    aUserId );
    
    static IDE_RC getDatabaseLinkFirstItem( idvSQL * aStatistics, qciDatabaseLinksItem ** aFirstItem );
    static IDE_RC getDatabaseLinkNextItem( qciDatabaseLinksItem * aCurrentItem,
                                           qciDatabaseLinksItem ** aNextItem );
    static IDE_RC freeDatabaseLinkItems( qciDatabaseLinksItem * aFirstItem );
                                           
    /*
     * privilege for Database link.
     */
    static IDE_RC checkCreateDatabaseLinkPrivilege( void * aQcStatement,
                                                    idBool aPublic );
    static IDE_RC checkDropDatabaseLinkPrivilege( void * aQcStatement,
                                                  idBool aPublic );

    static idBool isSysdba( void * aQcStatement );

    // PROJ-2397 Compressed Column Table Replication
    static IDE_RC makeDictValueForCompress( smiStatement  * aStatement,
                                            void          * aTableHandle,
                                            void          * aIndexHeader,
                                            smiValue      * aInsertedRow,
                                            void          * aOIDValue );

    /* PROJ-1438 Job Scheduler */
    static void getExecJobItems( SInt   * aIems,
                                 UInt   * aCount,
                                 UInt     aMaxCount );

    static void executeJobItem( UInt     aJobThreadIndex,
                                SInt     aJob,
                                void   * aSession );

    static IDE_RC executeTempSQL( void  * aMmSession,
                                  SChar * aSQL,
                                  idBool  aIsCommit );

    /* BUG-45783 */
    static void resetInitialJobState( void );

    static idBool isExecuteForNatc( void );

    static UInt getConcExecDegreeMax( void );

    /* PROJ-2446 ONE SOURCE XDB USE */
    static idvSQL* getStatistics( mtcTemplate * aTemplate );

    /* PROJ-2446 ONE SOURCE XDB smiGlobalCallBackList에서 사용 된다.
     * partition meta cache, procedure cache, trigger cache */
    static IDE_RC makeAndSetQcmTblInfo( smiStatement * aSmiStmt,
                                        UInt           aTableID,
                                        smOID          aTableOID );

    static IDE_RC createProcObjAndInfo( smiStatement  * aSmiStmt,
                                        smOID           aProcOID );

    /* PROJ-2446 ONE SOURCE FOR SUPPOTING PACKAGE */
    static IDE_RC createPkgObjAndInfo( smiStatement  * aSmiStmt,
                                       smOID           aPkgOID );

    static IDE_RC createTriggerCache( void    * aTriggerHandle,
                                      void   ** aTriggerCache );

    static idvSQL * getStatisticsFromQcStatement( void * aQcStatement );

    // BUG-41030 For called by PSM flag setting
    static void setPSMFlag( void * aQcStmt,
                            idBool aValue );

    // PROJ-2551 simple query 최적화
    static void initSimpleInfo( qcSimpleInfo * aInfo );
    
    static idBool isSimpleQuery( qcStatement * aStatement );

    static idBool isSimpleSelectQuery( qmnPlan     * aPlan,
                                       SChar       * aHostValues,
                                       UInt        * aHostValueCount );
    
    static idBool isSimpleBind( qcStatement * aStatement );

    static idBool isSimpleSelectBind( qcStatement * aStatement,
                                      qmnPlan     * aPlan );

    static void setSimpleFlag( qcStatement * aStatement );

    static void setSimpleBindFlag( qcStatement * aStatement );

    static void buildSimpleHostValues( SChar         * aHostValues,
                                       UInt          * aHostValueCount,
                                       qmnValueInfo  * aSimpleValues,
                                       UInt            aSimpleValueCount );

    static idBool checkSimpleBind( qciBindParamInfo * aBindParam,
                                   qmnValueInfo     * aSimpleValues,
                                   UInt               aSimpleValueCount );

    static idBool checkExecFast( qcStatement  * aStatement, qmnPlan * aPlan );

    /* BUG-41696 */
    static IDE_RC checkRunningEagerReplicationByTableOID( qcStatement  * aStatement,
                                                          smOID        * aReplicatedTableOIDArray,
                                                          UInt           aReplicatedTableOIDCount );

    /* BUG-43605 [mt] random함수의 seed 설정을 session 단위로 변경해야 합니다. */
    static void initRandomInfo( qcRandomInfo * aRandomInfo );

    /* PROJ-2626 Snapshot Export */
    static void getMemMaxAndUsedSize( ULong * aMaxSize, ULong * aUsedSize );
    static IDE_RC getDiskUndoMaxAndUsedSize( ULong * aMaxSize, ULong * aUsedSize );

    /* PROJ-2642 */
    static IDE_RC writeTableMetaLogForReplication( qcStatement    * aStatement,
                                                   smOID          * aOldTableOID,
                                                   smOID          * aNewTableOID,
                                                   UInt             aTableCount );
   
    /* PROJ-2677 DDL synchronization */
    static void     restoreTempInfoForPartition( qciTableInfo * aTableInfo,
                                                 qciTableInfo * aPartInfo );

    static void     restoreTempInfo( qciTableInfo         * aTableInfo,
                                     qciPartitionInfoList * aPartInfoList,
                                     qdIndexTableList     * aIndexTableList );

    static IDE_RC   checkRollbackAbleDDLEnable( smiTrans * aTrans, 
                                                smOID      aTableOID,
                                                idBool     aCallByRepl );

    static IDE_RC   validateAndLockPartitionInfoList( qciStatement         * aQciStatement,
                                                      qciPartitionInfoList * aPartInfoList,
                                                      smiTBSLockValidType    aTBSLvType,
                                                      smiTableLockMode       aLockMode,
                                                      ULong                  aLockWaitMicroSec );

    static IDE_RC   runDDLforInternal( idvSQL       * aStatistics,
                                       smiStatement * aSmiStmt,
                                       UInt           aUserID,
                                       UInt           aSessionFlag,
                                       SChar        * aSqlStr );

    static IDE_RC   runDDLforInternalWithMmSession( idvSQL       * aStatistics,
                                                    void         * aMmSession,
                                                    smiStatement * aSmiStmt,
                                                    UInt           aUserID,
                                                    UInt           aSessionFlag,
                                                    SChar        * aSqlStr );

    static IDE_RC   runRollbackableInternalDDL( qcStatement  * aStatement,
                                                smiStatement * aSmiStmt,
                                                UInt           aUserID,
                                                SChar        * aSqlStr );

    static IDE_RC   runDCLforInternal( qcStatement  * aStatement,
                                       SChar        * aSqlStr,
                                       void         * aSession );

    static IDE_RC runSelectOneRowforDDL( smiStatement * aSmiStmt,
                                         SChar        * aSqlStr,
                                         void         * aResult,
                                         idBool       * aRecordExist,
                                         idBool         aCalledPWVerifyFunc );
    
    static idBool   isReplicableDDL( qciStatement * aQciStatement );

    /* BUG-45948 */
    static void     getSmiStmt( qciStatement *aQciStatement, smiStatement ** aSmiStatement );
    static void     setSmiStmt( qciStatement *aQciStatement, smiStatement * aSmiStatement );

    static idBool   isDDLSync( qciStatement * aQciStatement );

    static idBool   isDDLSync( void * aQcStatement );

    static idBool   isInternalDDL( void * aQcStatement );

    /* PROJ-2736 Global DDL */
    static idBool   getGlobalDDL( void * aQcStatement );

    /* PROJ-2735 DDL Transaction */
    static idBool   getIsNeedDDLInfo( void * aQcStatement );

    static idBool   getTransactionalDDL( void * aQcStatement );

    static idBool   getIsRollbackableInternalDDL( void * aQcStatement );

    static void     setTransactionalDDLAvailable( void * aQcStatement, idBool aAvailable );

    static idBool   getTransactionalDDLAvailable( qciStatement * aQciStatement );

    static void     setDDLSrcInfo( void        * aQcStatement,
                                   idBool        aTransactionalDDLAvailable,
                                   UInt          aSrcTableOIDCount,
                                   smOID       * aSrcTableOIDArray,
                                   UInt          aSrcPartOIDCountPerTable,
                                   smOID       * aSrcPartOIDArray );

    static qciDDLTargetType getDDLTargetType( UInt aSrcPartOIDCount );
  
    static smOID  * getDDLSrcTableOIDArray( qciStatement * aQciStatement, UInt * aSrcTableOIDCount );
    static smOID  * getDDLSrcTableOIDArray( void * aQcStatement, UInt * aSrcTableOIDCount );

    static smOID  * getDDLSrcPartTableOIDArray( qciStatement * aQciStatement, UInt * aSrcPartOIDCount );
    static smOID  * getDDLSrcPartTableOIDArray( void * aQcStatement, UInt * aSrcPartOIDCount );

    static void     setDDLDestInfo( void        * aQcStatement,
                                    UInt          aDestTableOIDCount,
                                    smOID       * aDestTableOIDArray,
                                    UInt          aDestPartOIDCountPerTable,
                                   smOID        * aDestPartOIDArray );

    static smOID  * getDDLDestTableOIDArray( qciStatement * aQciStatement, UInt * aDestTableOIDCount );
    static smOID  * getDDLDestTableOIDArray( qcStatement * aQcStatement, UInt * aDestTableOIDCount );
    
    static smOID  * getDDLDestPartTableOIDArray( qciStatement * aQciStatement, UInt * aDestPartOIDCount );
    static smOID  * getDDLDestPartTableOIDArray( void * aQcStatement, UInt * aDestPartOIDCount );

    static smiTransactionalDDLCallback mTransactionalDDLCallback;

    static IDE_RC   backupDDLTargetOldTableInfo( smiTrans               * aTrans, 
                                                 smOID                    aTableOID,
                                                 UInt                     aPartOIDCount,
                                                 smOID                  * aPartOIDArray,
                                                 smiDDLTargetTableInfo ** aDDLTargetTableInfo );
    
    static IDE_RC   backupDDLTargetNewTableInfo( smiTrans               * aTrans, 
                                                 smOID                    aTableOID,
                                                 UInt                     aPartOIDCount,
                                                 smOID                  * aPartOIDArray,
                                                 smiDDLTargetTableInfo ** aDDLTargetTableInfo );
    
    static void     removeDDLTargetTableInfo( smiTrans * aTrans, smiDDLTargetTableInfo * aDDLTargetTableInfo );

    static void     restoreDDLTargetOldTableInfo( smiDDLTargetTableInfo * aDDLTargetTableInfo );

    static void     destroyDDLTargetNewTableInfo( smiDDLTargetTableInfo * aDDLTargetTableInfo );

    static IDE_RC   rebuildStatement( qciStatement * aQciStatement, 
                                      smiTrans     * aTrans,
                                      UInt           aFlag );

    static idBool   isLockTableUntillNextDDL( qciStatement * aQciStatement );

    static idBool   isLockTableUntillNextDDL( void * aQcStatement );

    static idBool  intersectColumn( UInt *aColumnIDList1,
                                    UInt aColumnIDCount1,
                                    UInt *aColumnIDList2,
                                    UInt aColumnIDCount2);

    // TASK-7244 PSM partial rollback in Sharding
    static void     setBeginSP( qcStatement * aQcStatement );
    static void     unsetBeginSP( qcStatement * aQcStatement );
    static idBool   isBeginSP( qcStatement * aQcStatement );

    // TASK-7244 Set shard split method to PSM info
    static IDE_RC   makeProcStatusInvalidAndSetShardSplitMethodByName( qcStatement * aQcStatement,
                                                                       qsOID         aProcOID,
                                                                       SChar       * aShardSplitMethodStr );
};

#endif /* _O_QCI_H_ */
