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
 

/***********************************************************************
 * $Id$
 **********************************************************************/

#ifndef _O_SDI_H_
#define _O_SDI_H_ 1

#include <idl.h>
#include <idu.h>
#include <ide.h>
#include <sdiTypes.h>
#include <sduProperty.h>
#include <sde.h>
#include <qci.h>
#include <smi.h>

/* BUG-47459 */
#define SAVEPOINT_FOR_SHARD_STMT_PARTIAL_ROLLBACK        ("$$SHARD_PARTIAL_ROLLBACK")
#define SAVEPOINT_FOR_SHARD_CLONE_PROC_PARTIAL_ROLLBACK  ("$$SHARD_CLONE_PROC_PARTIAL_ROLLBACK")
// TASK-7244 PSM partial rollback in Sharding
#define SAVEPOINT_FOR_SHARD_GLOBAL_PROC_PARTIAL_ROLLBACK ("$$SHARD_GLOBAL_PROC_PARTIAL_ROLLBACK")
#define SAVEPOINT_FOR_SHARD_SHARD_PROC_PARTIAL_ROLLBACK  ("$$SHARD_SHARD_PROC_PARTIAL_ROLLBACK")

typedef enum
{
    SDI_XA_RECOVER_START = 0,
    SDI_XA_RECOVER_CONT  = 1,
    SDI_XA_RECOVER_END   = 2
} sdiXARecoverOption;

typedef ULong  sdiShardPin;
typedef UShort sdiShardNodeId;

typedef enum
{
    SDI_OFFSET_VERSION      = 56,   /* << 7 byte, sdmShardPinInfo.mVersion                  */
    SDI_OFFSET_RESERVED     = 48,   /* << 6 byte, sdmShardPinInfo.mReserved                 */
    SDI_OFFSET_SHARD_NODE_ID = 32,   /* << 4 byte, sdmShardPinInfo.mMetaNodeInfo.mMetaNodeId */
    SDI_OFFSET_SEQEUNCE     = 0,    /* << 0 byte, sdmShardPinInfo.mSeq                      */
} sdiShardPinFactorOffset;

/* TASK-7219 Non-shard DML */
typedef struct sdiOutRefBindInfo
{
    UShort mTable;
    UShort mColumn;
    sdiOutRefBindInfo * mNext;
} sdiOutRefBindInfo;

/*
 * (SChar*) "CREATE TABLE SYS_SHARD.LOCAL_META_INFO_ ( "
 * "SHARD_NODE_ID      INTEGER, "
 * "SHARDED_DB_NAME    VARCHAR(40), "
 * "NODE_NAME          VARCHAR(10), "
 * "HOST_IP            VARCHAR(64), "
 * "PORT_NO            INTEGER, "
 * "INTERNAL_HOST_IP   VARCHAR(64), "
 * "INTERNAL_PORT_NO   INTEGER, "
 *  "INTERNAL_REPLICATION_HOST_IP   VARCHAR(64), "
 *  "INTERNAL_REPLICATION_PORT_NO   INTEGER, "
 * "INTERNAL_CONN_TYPE INTEGER )"
 */
typedef struct sdiLocalMetaInfo
{
    sdiShardNodeId  mShardNodeId;
    SChar           mShardedDBName[SDI_SHARDED_DB_NAME_MAX_SIZE + 1];
    SChar           mNodeName[SDI_NODE_NAME_MAX_SIZE + 1];
    SChar           mHostIP[SDI_SERVER_IP_SIZE + 1];
    UShort          mPortNo;
    SChar           mInternalHostIP[SDI_SERVER_IP_SIZE + 1];
    UShort          mInternalPortNo;
    SChar           mInternalRPHostIP[SDI_SERVER_IP_SIZE + 1];
    UShort          mInternalRPPortNo;
    UInt            mInternalConnType;
    UInt            mKSafety;
    UInt            mReplicationMode;
    UInt            mParallelCount;
} sdiLocalMetaInfo;

typedef struct sdiGlobalMetaInfo
{
    ULong                   mShardMetaNumber;
} sdiGlobalMetaNodeInfo;

typedef enum
{
    SDI_SHARD_PIN_INVALID = 0,
} sdiShardPinStatus;

typedef enum
{
    SDI_NODE_CONNECT_TYPE_DEFAULT = 0,
    SDI_NODE_CONNECT_TYPE_TCP     = 1,  /* = ULN_CONNTYPE_TCP */
    SDI_NODE_CONNECT_TYPE_IB      = 8,  /* = ULN_CONNTYPE_IB  */
    /* else NOT SUPPORT */
} sdiInternalNodeConnectType;

typedef enum {
    SDI_FAILOVER_ACTIVE_ONLY    = 0,        /* = ULSD_CONN_TO_ACTIVE */
    SDI_FAILOVER_ALTERNATE_ONLY = 1,        /* = ULSD_CONN_TO_ALTERNATE */
    SDI_FAILOVER_ALL            = 2,
    SDI_FAILOVER_MAX            = 3,
    SDI_FAILOVER_UCHAR_MAX      = 255,
    SDI_FAILOVER_NOT_USED       = SDI_FAILOVER_UCHAR_MAX,
    /* Do not over 0xff(255). Values cast into UChar. */
} sdiFailOverTarget;

/* PROJ-2724 shard database preperation */
typedef struct sdiDatabaseInfo
{
    SChar* mDBName;       /* smiGetDBName() */
    UInt   mTransTblSize; /* smiGetTransTblSize */
    SChar* mDBCharSet;        /* mtc::getDBCharSet(); */
    SChar* mNationalCharSet;  /* mtc::getNationalCharSet(); */
    SChar* mDBVersion;        /* smVersionString */
    UChar  mMajorVersion;     /* Shard major ver of client SHARD_MAJOR_VERSION */
    UChar  mMinorVersion;     /* Shard minor ver of client SHARD_MINOR_VERSION */
    UChar  mPatchVersion;     /* Shard patch ver of client SHARD_PATCH_VERSION */
} sdiDatabaseInfo;

typedef struct sdiNode
{
    ULong           mSMN;
    UInt            mNodeId;
    SChar           mNodeName[SDI_NODE_NAME_MAX_SIZE + 1];
    SChar           mServerIP[SDI_SERVER_IP_SIZE];
    UShort          mPortNo;
    SChar           mAlternateServerIP[SDI_SERVER_IP_SIZE];
    UShort          mAlternatePortNo;
    UShort          mConnectType;
} sdiNode;

#define SDI_NODE_NULL_ID (ID_UINT_MAX)
#define SDI_NODE_NULL_IDX (SDI_NODE_MAX_COUNT)
#define SDI_IS_NULL_NAME( _name_ )                \
    ( (idlOS::strMatch(                           \
        _name_, idlOS::strlen(_name_),           \
        SDM_NA_STR, SDM_NA_STR_SIZE) == 0) ?      \
        ID_TRUE :                                 \
        ID_FALSE )

#define SDI_SET_NULL_NAME( _name_ )                \
    ( idlOS::strncpy((_name_), SDM_NA_STR, SDM_NA_STR_SIZE + 1) )

#define SDI_INIT_NODE( node )                            \
{                                                        \
    (node)->mSMN                     = 0; \
    (node)->mNodeId                  = SDI_NODE_NULL_ID; \
    SDI_SET_NULL_NAME((node)->mNodeName);                \
    (node)->mServerIP[0]             = '\0';             \
    (node)->mPortNo                  = 0;                \
    (node)->mAlternateServerIP[0]    = '\0';             \
    (node)->mAlternatePortNo         = 0;                \
    (node)->mConnectType             = SDI_NODE_CONNECT_TYPE_DEFAULT; \
}

#define SDI_COPY_NODE( dstnode, srcnode )                            \
{                                                        \
    (dstnode)->mNodeId                  = srcnode->mNodeId; \
    idlOS::strncpy(dstnode->mNodeName, srcnode->mNodeName, SDI_NODE_NAME_MAX_SIZE + 1); \
    idlOS::strncpy(dstnode->mServerIP, srcnode->mServerIP, SDI_SERVER_IP_SIZE); \
    (dstnode)->mPortNo                  = srcnode->mPortNo;                \
    idlOS::strncpy(dstnode->mAlternateServerIP, srcnode->mAlternateServerIP, SDI_SERVER_IP_SIZE); \
    (dstnode)->mAlternatePortNo         = srcnode->mAlternatePortNo;                \
    (dstnode)->mConnectType             = srcnode->mConnectType; \
}

typedef struct sdiNodeInfo
{
    UShort          mCount;
    sdiNode         mNodes[SDI_NODE_MAX_COUNT];
} sdiNodeInfo;

typedef struct sdiReplicaSet
{
    UInt            mReplicaSetId;
    SChar           mPrimaryNodeName[SDI_NODE_NAME_MAX_SIZE + 1];
    SChar           mFirstBackupNodeName[SDI_NODE_NAME_MAX_SIZE + 1];
    SChar           mSecondBackupNodeName[SDI_NODE_NAME_MAX_SIZE + 1];
    SChar           mStopFirstBackupNodeName[SDI_NODE_NAME_MAX_SIZE + 1];
    SChar           mStopSecondBackupNodeName[SDI_NODE_NAME_MAX_SIZE + 1];    
    SChar           mFirstReplName[SDI_REPLICATION_NAME_MAX_SIZE + 1];
    SChar           mFirstReplFromNodeName[SDI_NODE_NAME_MAX_SIZE + 1];
    SChar           mFirstReplToNodeName[SDI_NODE_NAME_MAX_SIZE + 1];
    SChar           mSecondReplName[SDI_REPLICATION_NAME_MAX_SIZE + 1];
    SChar           mSecondReplFromNodeName[SDI_NODE_NAME_MAX_SIZE + 1];
    SChar           mSecondReplToNodeName[SDI_NODE_NAME_MAX_SIZE + 1];
    ULong           mSMN;
} sdiReplicaSet;

#define SDI_REPLICASET_NULL_ID ( 0 )

#define SDI_INIT_REPLICA_SET( replicaset )                       \
{                                                                \
    (replicaset)->mReplicaSetId  = SDI_REPLICASET_NULL_ID;       \
    SDI_SET_NULL_NAME((replicaset)->mPrimaryNodeName);           \
    SDI_SET_NULL_NAME((replicaset)->mFirstBackupNodeName);       \
    SDI_SET_NULL_NAME((replicaset)->mSecondBackupNodeName);      \
    SDI_SET_NULL_NAME((replicaset)->mStopFirstBackupNodeName);   \
    SDI_SET_NULL_NAME((replicaset)->mStopSecondBackupNodeName);  \
    SDI_SET_NULL_NAME((replicaset)->mFirstReplName);             \
    SDI_SET_NULL_NAME((replicaset)->mFirstReplFromNodeName);     \
    SDI_SET_NULL_NAME((replicaset)->mSecondReplName);            \
    SDI_SET_NULL_NAME((replicaset)->mSecondReplToNodeName);      \
    (replicaset)->mSMN       = SDI_NULL_SMN;                     \
}

typedef struct sdiReplicaSetInfo
{
    UShort          mCount;
    sdiReplicaSet   mReplicaSets[SDI_NODE_MAX_COUNT];
} sdiReplicaSetInfo;



typedef enum
{
    SDI_BACKUP_1ST = 1,
    SDI_BACKUP_2ND = 2,
} sdiBackupOrder ;

/* TASK-7219 Non-shard DML */
typedef struct sdiTupleColumn
{
    UShort * mTuple;
    UShort * mColumn;

} sdiTupleColumn;

typedef enum
{
    SDI_FAILBACK_NONE = 0,
    SDI_FAILBACK_JOIN = 1,
    SDI_FAILBACK_JOIN_SMN_MODIFIED = 2,
    SDI_FAILBACK_FAILEDOVER = 3,
} sdiFailbackState;

typedef union sdiValue
{
    // hash shard인 경우
    UInt        mHashMax;

    // range shard인 경우
    UChar       mMax[1];      // 대표값
    SShort      mSmallintMax; // shard key가 smallint type인 경우
    SInt        mIntegerMax;  // shard key가 integer type인 경우
    SLong       mBigintMax;   // shard key가 bigint type인 경우
    mtdCharType mCharMax;     // shard key가 char/varchar type인 경우
    UShort      mCharMaxBuf[(SDI_RANGE_VARCHAR_MAX_SIZE + 1) / 2];  // 2byte align

    // bind parameter인 경우
    UShort      mBindParamId;

    // out ref column인 경우
    sdiTupleColumn mOutRefInfo;

} sdiValue;

typedef struct sdiRange
{
    UInt     mNodeId;
    SChar    mPartitionName[ QC_MAX_OBJECT_NAME_LEN + 1 ];
    sdiValue mValue;
    sdiValue mSubValue;
    UInt     mReplicaSetId;
} sdiRange;

typedef struct sdiRangeInfo
{
    UShort          mCount;
    sdiRange      * mRanges;
} sdiRangeInfo;

typedef struct sdiTableInfo
{
    UInt            mShardID;
    SChar           mUserName[QC_MAX_OBJECT_NAME_LEN + 1];
    SChar           mObjectName[QC_MAX_OBJECT_NAME_LEN + 1];
    SChar           mObjectType;
    SChar           mKeyColumnName[QC_MAX_OBJECT_NAME_LEN + 1];
    UInt            mKeyDataType;   // shard key의 mt type id
    UShort          mKeyColOrder;
    sdiSplitMethod  mSplitMethod;

    /* PROJ-2655 Composite shard key */
    idBool          mSubKeyExists;
    SChar           mSubKeyColumnName[QC_MAX_OBJECT_NAME_LEN + 1];
    UInt            mSubKeyDataType;   // shard key의 mt type id
    UShort          mSubKeyColOrder;
    sdiSplitMethod  mSubSplitMethod;

    UInt            mDefaultNodeId; // default node id
    SChar           mDefaultPartitionName[QC_MAX_OBJECT_NAME_LEN + 1]; // default partition name
    UInt            mDefaultPartitionReplicaSetId; // default ReplicaSetID
} sdiTableInfo;

#define SDI_INIT_TABLE_INFO( info )                 \
{                                                   \
    (info)->mShardID             = 0;               \
    (info)->mUserName[0]         = '\0';            \
    (info)->mObjectName[0]       = '\0';            \
    (info)->mObjectType          = 0;               \
    (info)->mKeyColumnName[0]    = '\0';            \
    (info)->mKeyDataType         = ID_UINT_MAX;     \
    (info)->mKeyColOrder         = 0;               \
    (info)->mSplitMethod         = SDI_SPLIT_NONE;  \
    (info)->mSubKeyExists        = ID_FALSE;        \
    (info)->mSubKeyColumnName[0] = '\0';            \
    (info)->mSubKeyDataType      = 0;               \
    (info)->mSubKeyColOrder      = 0;               \
    (info)->mSubSplitMethod      = SDI_SPLIT_NONE;  \
    (info)->mDefaultNodeId       = SDI_NODE_NULL_ID;     \
}

typedef struct sdiTableInfoList
{
    sdiTableInfo            * mTableInfo;
    struct sdiTableInfoList * mNext;
} sdiTableInfoList;

typedef struct sdiObjectInfo
{
    ULong           mSMN;
    sdiTableInfo    mTableInfo;
    sdiRangeInfo    mRangeInfo;

    /* PROJ-2701 Online data rebuild
     * SMN history별로 shard object information가 생성된다.
     * 현재는 sessionSMN, dataSMN 당 하나씩 총 2개가 생성될 수 있다.
     */
    struct sdiObjectInfo * mNext;

    // BUG-49070 mIsLocalForce를 변경할 때 mKeyFlags를 침범하므로 mIsLocalForce를 mKeyFlags 위로 옮깁니다.
    idBool          mIsLocalForce;
    // view인 경우 key가 여러개일 수 있어 컬럼 갯수만큼 할당하여 사용한다.
    UChar           mKeyFlags[1];
    // BUG-49070 mKeyFlags 아래에 새로운 member를 추가하면 안됩니다.

} sdiObjectInfo;

#define SDI_INIT_OBJECT_INFO( info )            \
{                                               \
    (info)->mSMN               = ID_ULONG(0);   \
    SDI_INIT_TABLE_INFO(&((info)->mTableInfo)); \
    (info)->mRangeInfo.mCount  = 0;             \
    (info)->mRangeInfo.mRanges = NULL;          \
    (info)->mNext              = NULL;          \
    (info)->mKeyFlags[0]       = '\0';          \
    (info)->mIsLocalForce      = ID_FALSE       \
}

typedef enum sdiValueInfoType
{
    SDI_VALUE_INFO_HOST_VAR     = 0,
    SDI_VALUE_INFO_CONST_VAL    = 1,
    SDI_VALUE_INFO_OUT_REF_COL  = 2
} sdiValueInfoType;

typedef struct sdiValueInfo
{
    UChar    mType;             // 0 : hostVariable, 1 : constant, 2 : out ref column to bind
    UInt     mDataValueType;
    sdiValue mValue;

} sdiValueInfo;

typedef struct sdiAnalyzeInfo
{
    UShort             mValuePtrCount;
    sdiValueInfo    ** mValuePtrArray;
    idBool             mIsShardQuery;
    sdiSplitMethod     mSplitMethod;  // shard key의 split method
    UInt               mKeyDataType;  // shard key의 mt type id

    /* PROJ-2655 Composite shard key */
    idBool             mSubKeyExists;
    UShort             mSubValuePtrCount;
    sdiValueInfo    ** mSubValuePtrArray;
    sdiSplitMethod     mSubSplitMethod;  // sub sub key의 split method
    UInt               mSubKeyDataType;  // sub shard key의 mt type id

    UInt               mDefaultNodeId;// shard query의 default node id
    sdiRangeInfo       mRangeInfo;    // shard query의 분산정보

    // BUG-45359
    qcShardNodes     * mNodeNames;

    // PROJ-2685 online rebuild
    sdiTableInfoList * mTableInfoList;

    // BUG-49088
    idBool             mTopQueryFlag [ SDI_ANALYSIS_TOP_QUERY_FLAG_MAX ];

    /* BUG-45899 */
    UShort             mNonShardQueryReason;
} sdiAnalyzeInfo;

#define SDI_INIT_ANALYZE_INFO( info )            \
{                                                \
    (info)->mValuePtrCount     = 0;              \
    (info)->mValuePtrArray     = NULL;           \
    (info)->mIsShardQuery      = ID_FALSE;       \
    (info)->mSplitMethod       = SDI_SPLIT_NONE; \
    (info)->mKeyDataType       = ID_UINT_MAX;    \
    (info)->mSubKeyExists      = ID_FALSE;       \
    (info)->mSubValuePtrCount  = 0;              \
    (info)->mSubValuePtrArray  = NULL;           \
    (info)->mSubSplitMethod    = SDI_SPLIT_NONE; \
    (info)->mSubKeyDataType    = ID_UINT_MAX;    \
    (info)->mDefaultNodeId     = SDI_NODE_NULL_ID;    \
    (info)->mRangeInfo.mCount  = 0;              \
    (info)->mRangeInfo.mRanges = NULL;           \
    (info)->mNodeNames         = NULL;           \
    (info)->mTableInfoList     = NULL;           \
    SDI_INIT_ANALYSIS_TOP_QUERY_FLAG( (info)->mTopQueryFlag );  \
    (info)->mNonShardQueryReason = SDI_NON_SHARD_QUERY_REASON_MAX;   \
}

// PROJ-2646
typedef struct sdiShardInfo
{
    UInt             mKeyDataType;
    UInt             mDefaultNodeId;
    sdiSplitMethod   mSplitMethod;
    sdiRangeInfo   * mRangeInfo;

} sdiShardInfo;

typedef struct sdiKeyTupleColumn
{
    UShort mTupleId;
    UShort mColumn;
    idBool mIsNullPadding;
    idBool mIsAntiJoinInner;

} sdiKeyTupleColumn;

typedef struct sdiKeyInfo
{
    UInt                  mKeyTargetCount;
    UShort              * mKeyTarget;

    UInt                  mKeyCount;
    sdiKeyTupleColumn   * mKey;

    UShort                mValuePtrCount;
    sdiValueInfo       ** mValuePtrArray;

    sdiShardInfo          mShardInfo;

    idBool                mIsJoined;

    sdiKeyInfo          * mLeft;
    sdiKeyInfo          * mRight;
    sdiKeyInfo          * mOrgKeyInfo;

    sdiKeyInfo          * mNext;

} sdiKeyInfo;

#define SDI_INIT_KEY_INFO( info )                           \
{                                                           \
    (info)->mKeyTargetCount               = 0;              \
    (info)->mKeyTarget                    = NULL;           \
    (info)->mKeyCount                     = 0;              \
    (info)->mKey                          = NULL;           \
    (info)->mValuePtrCount                = 0;              \
    (info)->mValuePtrArray                = NULL;           \
    (info)->mShardInfo.mKeyDataType       = ID_UINT_MAX;    \
    (info)->mShardInfo.mDefaultNodeId     = SDI_NODE_NULL_ID;    \
    (info)->mShardInfo.mSplitMethod       = SDI_SPLIT_NONE; \
    (info)->mShardInfo.mRangeInfo         = NULL;           \
    (info)->mIsJoined                     = ID_FALSE;       \
    (info)->mLeft                         = NULL;           \
    (info)->mRight                        = NULL;           \
    (info)->mOrgKeyInfo                   = NULL;           \
    (info)->mNext                         = NULL;           \
}

typedef struct sdiShardAnalysis
{
    idBool             mIsShardQuery;
    sdiAnalysisFlag    mAnalysisFlag;
    sdiShardInfo       mShardInfo;
    sdiKeyInfo       * mKeyInfo;
    qtcNode          * mShardPredicate;
    sdiShardAnalysis * mNext;

    /* PROJ-2685 online rebuild */
    sdiTableInfoList    * mTableInfoList;

} sdiShardAnalysis;

#define SDI_INIT_SHARD_ANALYSIS( info )                              \
{                                                                    \
    (info)->mIsShardQuery                        = ID_FALSE;         \
    (info)->mShardInfo.mKeyDataType              = ID_UINT_MAX;      \
    (info)->mShardInfo.mDefaultNodeId            = SDI_NODE_NULL_ID; \
    (info)->mShardInfo.mSplitMethod              = SDI_SPLIT_NONE;   \
    (info)->mShardInfo.mRangeInfo                = NULL;             \
    (info)->mKeyInfo                             = NULL;             \
    (info)->mShardPredicate                      = NULL;             \
    (info)->mTableInfoList                       = NULL;             \
    (info)->mNext                                = NULL;             \
    SDI_INIT_ANALYSIS_FLAG( ( info )->mAnalysisFlag );               \
}

/* BUG-46623
 * sdiBindParam: memcmp 함수를 사용하여 비교하는 구조체 이므로
 * 1. 멤버 변수의 크기에 따라 align 에 맞게 변수를 정의해야 하며
 *    (64 bit system 8 byte align 을 고려해야 한다.)
 * 2. padding 변수를 0으로 초기화 해야 한다.
 *    현재는 qmnSDEX::setParamInfo, qmnSDIN::setParamInfo,
 *    qmnSDSE::setParamInfo 함수에서 초기화 하고 있다.
 */
typedef struct sdiBindParam
{
    void       * mData;
    UInt         mInoutType;
    UInt         mType;
    UInt         mDataSize;
    SInt         mPrecision;
    SInt         mScale;
    UShort       mId;
    UShort       padding;
} sdiBindParam;

/* PROJ-2728
 * BindParam 재사용 여부를 sdi::reuseShardDataInfo 에서
 * memcmp로 검사하므로 execute 시에 변경 가능성이 있는 것들은
 * sdiBindParam 내부에 두지 않고 분리한다.
 */
typedef struct sdiOutBindParam
{
    void       * mShadowData;
    SInt         mIndicator;
} sdiOutBindParam;

#define SDI_GET_SHADOW_DATA( aDataNode, aClientIndex, aBindId )                \
    (UChar *)((aDataNode)->mOutBindParams[(aBindId)].mShadowData) +            \
    ((aClientIndex) * (aDataNode)->mBindParams[(aBindId)].mDataSize)

typedef struct sdlRemoteStmt sdlRemoteStmt;

typedef enum sdiSVPStep
{
    SDI_SVP_STEP_DO_NOT_NEED_SAVEPOINT   = 1,
    SDI_SVP_STEP_NEED_SAVEPOINT          = 2,
    SDI_SVP_STEP_SET_SAVEPOINT           = 3,
    SDI_SVP_STEP_ROLLBACK_TO_SAVEPOINT   = 4,
    SDI_SVP_STEP_PROCEDURE_SAVEPOINT     = 5
} sdiSVPStep;

// PROJ-2638
typedef struct sdiDataNode
{
    sdlRemoteStmt * mRemoteStmt;          // data node stmt

    void          * mBuffer[SDI_NODE_MAX_COUNT];  // data node fetch buffer
    UInt          * mOffset;              // meta node column offset array
    UInt          * mMaxByteSize;         // meta node column max byte size array
    UInt            mBufferLength;        // data node fetch buffer length
    UShort          mColumnCount;         // data node column count

    sdiBindParam  * mBindParams;          // data node parameters
    sdiOutBindParam  * mOutBindParams;    // data node parameters for output

    /* TASK-7219 Non-shard DML */
    void          * mOutRefBindData;      // transformed out ref column bind data
    
    UShort          mBindParamCount;
    idBool          mBindParamChanged;

    SChar         * mPlanText;            // data node plan text (alloc&free는 cli library에서한다.)
    UInt            mExecCount;           // data node execution count

    UChar           mState;               // date node state

    sdiSVPStep      mSVPStep;             // for shard stmt partial rollback
} sdiDataNode;

#define SDI_NODE_STATE_NONE               0    // 초기상태
#define SDI_NODE_STATE_PREPARE_CANDIDATED 1    // prepare 후보노드가 선정된 상태
#define SDI_NODE_STATE_PREPARE_SELECTED   2    // prepare 후보노드에서 prepare 노드로 선택된 생태
#define SDI_NODE_STATE_PREPARED           3    // prepared 상태
#define SDI_NODE_STATE_EXECUTE_CANDIDATED 4    // execute 후보노드가 선정된 상태
#define SDI_NODE_STATE_EXECUTE_SELECTED   5    // execute 후보노드에서 execute 노드로 선택된 상태
#define SDI_NODE_STATE_EXECUTED           6    // executed 상태
#define SDI_NODE_STATE_FETCHED            7    // fetch 완료 상태 (only SELECT)

typedef struct sdiDataNodes
{
    idBool         mInitialized;
    UShort         mCount;
    sdiDataNode    mNodes[SDI_NODE_MAX_COUNT];
} sdiDataNodes;

/* sdiConnectInfo.mFlag */
#define SDI_CONNECT_PLANATTR_CHANGE_MASK         (0x00000001)
#define SDI_CONNECT_PLANATTR_CHANGE_FALSE        (0x00000000)
#define SDI_CONNECT_PLANATTR_CHANGE_TRUE         (0x00000001)

/* sdiConnectInfo.mFlag */
#define SDI_CONNECT_INITIAL_BY_NOTIFIER_MASK     (0x00000002)
#define SDI_CONNECT_INITIAL_BY_NOTIFIER_FALSE    (0x00000000)
#define SDI_CONNECT_INITIAL_BY_NOTIFIER_TRUE     (0x00000002)

/* sdiConnectInfo.mFlag */
#define SDI_CONNECT_REMOTE_TX_CREATE_MASK        (0x00000004)
#define SDI_CONNECT_REMOTE_TX_CREATE_FALSE       (0x00000000)
#define SDI_CONNECT_REMOTE_TX_CREATE_TRUE        (0x00000004)

/* sdiConnectInfo.mFlag */
#define SDI_CONNECT_COMMIT_PREPARE_MASK          (0x00000008)
#define SDI_CONNECT_COMMIT_PREPARE_FALSE         (0x00000000)
#define SDI_CONNECT_COMMIT_PREPARE_TRUE          (0x00000008)

/* sdiConnectInfo.mFlag */
#define SDI_CONNECT_TRANSACTION_END_MASK         (0x00000010)
#define SDI_CONNECT_TRANSACTION_END_FALSE        (0x00000000)
#define SDI_CONNECT_TRANSACTION_END_TRUE         (0x00000010)

/* sdiConnectInfo.mFlag */
#define SDI_CONNECT_USER_AUTOCOMMIT_MODE_MASK    (0x00000040)
#define SDI_CONNECT_USER_AUTOCOMMIT_MODE_ON      (0x00000000)
#define SDI_CONNECT_USER_AUTOCOMMIT_MODE_OFF     (0x00000040)

/* sdiConnectInfo.mFlag */
#define SDI_CONNECT_COORD_AUTOCOMMIT_MODE_MASK   (0x00000080)
#define SDI_CONNECT_COORD_AUTOCOMMIT_MODE_ON     (0x00000000)
#define SDI_CONNECT_COORD_AUTOCOMMIT_MODE_OFF    (0x00000080)

// BUG-47765
// sdiConnectInfo.mFlag
#define SDI_CONNECT_ATTR_CHANGE_MASK             (0x00000100)
#define SDI_CONNECT_ATTR_CHANGE_FALSE            (0x00000000)
#define SDI_CONNECT_ATTR_CHANGE_TRUE             (0x00000100)

// TASK-7244 PSM partial rollback in Sharding
// sdiConnectInfo.mFlag
#define SDI_CONNECT_PSM_SVP_SET_MASK             (0x00000200)
#define SDI_CONNECT_PSM_SVP_SET_FALSE            (0x00000000)
#define SDI_CONNECT_PSM_SVP_SET_TRUE             (0x00000200)


typedef IDE_RC (*sdiMessageCallback)(SChar *aMessage, UInt aLength, void *aArgument);

typedef struct sdiMessageCallbackStruct
{
    sdiMessageCallback   mFunction;
    void               * mArgument;
} sdiMessageCallbackStruct;

typedef enum
{
    SDI_AFFECTED_ROW_INITIAL = -1,
    SDI_AFFECTED_ROW_SETTED  = 0,
} sdiAffectedRow;

typedef enum
{
    SDI_COORDINATOR_SHARD   = 0,
    SDI_COORDINATOR_RESHARD = 1,
} sdiCoordinatorType;

typedef enum
{
    SDI_FAILOVER_SUSPEND_NONE        = 0,
    SDI_FAILOVER_SUSPEND_ALLOW_RETRY = 1,
    SDI_FAILOVER_SUSPEND_ALL         = 2,
} sdiFailoverSuspendType;

typedef struct sdiFailoverSuspend
{
    sdiFailoverSuspendType  mSuspendType;
    UInt                    mNewErrorCode;
} sdiFailoverSuspend;

typedef enum
{
    SDI_LINKER_PARAM_NOT_USED   = 0,
    SDI_LINKER_PARAM_USED       ,
} sdiLinkerParamUsed;

typedef struct sdiLinkerParam
{
    UChar mUsed;
    struct
    {
        sdiMessageCallbackStruct mMessageCallback;
    } param;
} sdiLinkerParam;

#define SDI_LINKER_PARAM_IS_USED( aConnectInfo )                                    \
            ( ( ( (aConnectInfo) != NULL ) &&                                       \
                ( (aConnectInfo)->mLinkerParam != NULL ) &&                         \
                ( (aConnectInfo)->mLinkerParam->mUsed == SDI_LINKER_PARAM_USED ) )  \
              ? ID_TRUE : ID_FALSE )

typedef struct sdiConnectInfo
{
    // 접속정보
    qcSession           * mSession;
    void                * mDkiSession;
    sdiCoordinatorType    mCoordinatorType;
    sdiNode               mNodeInfo;
    UShort                mConnectType;
    sdiShardPin           mShardPin;
    sdiShardClient        mIsShardClient ;
    ULong                 mShardMetaNumber;
    ULong                 mRebuildShardMetaNumber;
    SChar                 mUserName[QCI_MAX_OBJECT_NAME_LEN + 1];
    SChar                 mUserPassword[IDS_MAX_PASSWORD_LEN + 1];

    // 접속결과
    void                * mDbc;           // client connection
    idBool                mLinkFailure;   // client connection state
    UInt                  mTouchCount;
    UInt                  mNodeId;
    SChar                 mNodeName[SDI_NODE_NAME_MAX_SIZE + 1];
    SChar                 mServerIP[SDI_SERVER_IP_SIZE];  // 실제 접속한 data node ip
    UShort                mPortNo;                        // 실제 접속한 data node port
    SChar                 mFullAddress[SDI_FULL_SERVER_ADDRESS_SIZE + 1]; // 실제 접속한 data node ip + port

    // 런타임정보
    UInt                  mFlag;
    UChar                 mPlanAttr;
    UChar                 mReadOnly;
    void                * mRemoteTx;
    ID_XID                mXID;           // for 2PC
    sdiFailOverTarget     mFailoverTarget;
    idBool                mIsConnectSuccess[SDI_FAILOVER_MAX];
    vSLong                mAffectedRowCount;
    sdiFailoverSuspend    mFailoverSuspend;
    
    // BUG-47765
    // shard coordinator에 전파를 위한 property변경 여부 설정
    ULong                 mCoordPropertyFlag;
    sdiInternalOperation  mInternalOP;

    sdiLinkerParam      * mLinkerParam;
} sdiConnectInfo;

/* PROJ-2733-DistTxInfo */
typedef struct sdiGCTxInfo
{
    smSCN            mCoordSCN;
    smSCN            mRequestSCN;
    smSCN            mPrepareSCN;
    smSCN            mGlobalCommitSCN;
    smSCN            mTxFirstStmtSCN;
    SLong            mTxFirstStmtTime;
    sdiDistLevel     mDistLevel;
    SShort           mBeforeExecutedDataNodeIndex;
    UChar           *mSessionTypeString;
} sdiGCTxInfo;

#define SDI_SHARD_COORD_FIX_CTRL_EVENT_EXIT    (0) /* = ULN_SHARD_COORD_FIX_CTRL_EVENT_EXIT */
#define SDI_SHARD_COORD_FIX_CTRL_EVENT_ENTER   (1) /* = ULN_SHARD_COORD_FIX_CTRL_EVENT_ENTER */
typedef void (* sdiShardCoordFixCtrlCallback)( void * aSession,
                                               SInt * aCount,
                                               UInt   aEnterOrExit );
             /* = ulnShardCoordFixCtrlCallback */
typedef struct sdiShardCoordFixCtrlContext
{
    void                         * mSession;
    SInt                           mCount;
    sdiShardCoordFixCtrlCallback   mFunc;
} sdiShardCoordFixCtrlContext;    /* = ulnShardCoordFixCtrlContext */

typedef struct sdiClientInfo
{
    sdiShardCoordFixCtrlContext
                       mShardCoordFixCtrlCtx;
    UInt               mTransactionLevel;
    ULong              mTargetShardMetaNumber;
    sdiGCTxInfo        mGCTxInfo;
    UShort             mCount;          // client count
    sdiConnectInfo     mConnectInfo[SDI_NODE_MAX_COUNT];    // client connection info
    sdiLinkerParam     mLinkerParamBuffer[SDI_NODE_MAX_COUNT];
} sdiClientInfo;

typedef enum
{
    SDI_LINKER_NONE        = 0,
    SDI_LINKER_COORDINATOR = 1
} sdiLinkerType;

typedef enum
{
    SDI_BACKUP_1 = 1,
    SDI_BACKUP_2 = 2,
    SDI_BACKUP_MAX = 3
} sdiBackupNumber;

typedef struct sdiRangeIndex
{
    UShort mRangeIndex;
    UShort mValueIndex;

} sdiRangeIndex;

/* PROJ-2701 Online data rebuild */
typedef enum
{
    SDI_REBUILD_RANGE_NONE    = 0,
    SDI_REBUILD_RANGE_INCLUDE = 1,
    SDI_REBUILD_RANGE_EXCLUDE = 2

} sdiRebuildRangeType;

typedef enum
{
    SDI_REPL_SENDER,
    SDI_REPL_RECEIVER
}sdiReplDirectionalRole;

/* PROJ-2701 Online data rebuild */
typedef struct sdiRebuildInfo
{
    UShort           mMyNodeId;
    UShort           mSessionSMN;
    sdiAnalyzeInfo * mSessionSMNAnalysis;
    UShort           mDataSMN;
    sdiAnalyzeInfo * mDataSMNAnalysis;
} sdiRebuildInfo;

/* PROJ-2701 Online data rebuild */
typedef struct sdiRebuildRangeList
{
    sdiValue            mValue;
    UShort              mFromNode;
    UShort              mToNode;
    sdiRebuildRangeType mType;
    idBool              mIsDefault;

    struct sdiRebuildRangeList * mNext;

} sdiRebuildRangeList;

typedef enum
{
    SDI_COORD_SCN_INIT   = 1,
} sdiCoordSCNConst;

#define SDI_INIT_COORD_SCN( __PTR ) *((smSCN*)(__PTR)) = SDI_COORD_SCN_INIT;

typedef enum
{
    SDI_COORD_PLAN_INDEX_NONE = 0,
} sdiCoordPlanIndex;

/* TASK-7219 Shard Transformer Refactoring */
typedef enum
{
    SDI_QUERYSET_LIST_STATE_MAIN_MAKE         = 0,
    SDI_QUERYSET_LIST_STATE_MAIN_SKIP         = 1,
    SDI_QUERYSET_LIST_STATE_MAIN_ALL          = 2,
    SDI_QUERYSET_LIST_STATE_MAIN_PARTIAL      = 3,
    SDI_QUERYSET_LIST_STATE_DUMMY_MAKE        = 4,
    SDI_QUERYSET_LIST_STATE_DUMMY_SKIP        = 5,
    SDI_QUERYSET_LIST_STATE_DUMMY_ANALYZE     = 6,
    SDI_QUERYSET_LIST_STATE_DUMMY_PRE_ANALYZE = 7,
    SDI_QUERYSET_LIST_STATE_MAIN_ANALYZE_DONE = 8,
    SDI_QUERYSET_LIST_STATE_MAX               = 9
} sdiQuerySetListState;

#define SDI_SET_QUERYSET_LIST_STATE( _list_, _state_ )  \
    (_list_)->mState = (_state_);

#define SDI_CHECK_QUERYSET_LIST_STATE( _list_, _state_ ) \
    ( (_list_)->mState == (_state_) ? ID_TRUE : ID_FALSE )

typedef struct sdiQuerySetEntry
{
    UInt               mId;
    qmsQuerySet      * mQuerySet;
    sdiQuerySetEntry * mNext;
} sdiQuerySetEntry;

typedef struct sdiQuerySetList
{
    UInt                 mCount;
    sdiQuerySetListState mState;
    sdiQuerySetEntry   * mHead;
    sdiQuerySetEntry   * mTail;
} sdiQuerySetList;

#define SDI_SET_PARSETREE( _parse_, _queryset_ )            \
{                                                           \
    (_parse_)->withClause         = NULL;                   \
    (_parse_)->querySet           = (_queryset_);           \
    (_parse_)->orderBy            = NULL;                   \
    (_parse_)->limit              = NULL;                   \
    (_parse_)->loopNode           = NULL;                   \
    (_parse_)->forUpdate          = NULL;                   \
    (_parse_)->queue              = NULL;                   \
    (_parse_)->isTransformed      = ID_FALSE;               \
    (_parse_)->isSiblings         = ID_FALSE;               \
    (_parse_)->isView             = ID_TRUE;                \
    (_parse_)->isShardView        = ID_FALSE;               \
    (_parse_)->common.currValSeqs = NULL;                   \
    (_parse_)->common.nextValSeqs = NULL;                   \
    (_parse_)->common.parse       = qmv::parseSelect;       \
    (_parse_)->common.validate    = qmv::validateSelect;    \
    (_parse_)->common.optimize    = qmo::optimizeSelect;    \
    (_parse_)->common.execute     = qmx::executeSelect;     \
}

typedef enum
{
    SDI_STMT_SHARD_RETRY_NONE       = 0,
    SDI_STMT_SHARD_VIEW_OLD_RETRY   ,
    SDI_STMT_SHARD_REBUILD_RETRY    ,
    SDI_STMT_SHARD_SMN_PROPAGATION  ,
} sdiStmtShardRetryType;

#define SDI_SHARD_RETRY_LOOP_MAX        (10)

#define SDI_REUILD_ERROR_LOG_LEVEL      IDE_SD_6

typedef enum
{
    SDI_REBUILD_SMN_DO_NOT_PROPAGATE    = 0,
    SDI_REBUILD_SMN_PROPAGATE           ,
} sdiRebuildPropaOption;

typedef enum
{
    SDI_DDL_TYPE_TABLE      = 0,
    SDI_DDL_TYPE_PROCEDURE  = 1,
    SDI_DDL_TYPE_INDEX      = 2,
    SDI_DDL_TYPE_PROCEDURE_DROP = 3,
    SDI_DDL_TYPE_DROP = 4,
    SDI_DDL_TYPE_DISJOIN = 5,
    SDI_DDL_TYPE_INDEX_DROP = 6
} sdiDDLType;

/* TASK-7219 Non-shard DML */
typedef struct sdiMyRanges
{
    sdiValue * mValueMin;
    sdiValue * mValueMax;

    idBool     mIsMyNode;

    struct sdiMyRanges * mNext;

} sdiMyRanges;

class sdi
{
private:
    static IDE_RC initializeSession( qcSession * aSession,
                                     void      * aDkiSession,
                                     smiTrans  * aTrans,
                                     idBool      aIsShardMetaChanged,
                                     ULong       aConnectSMN );

    static void freeClientInfo( qcSession * aSession );

    static IDE_RC allocLinkerParam( sdiClientInfo  * aClientInfo,
                                    sdiConnectInfo * aConnectInfo );

    static void freeLinkerParam( sdiConnectInfo * aConnectInfo );

    static void setLinkerParam( sdiConnectInfo * aConnectInfo );
public:

    /*************************************************************************
     * 상위 모듈에서 호출하는 상위 모듈과 연동된 속성 값 설정을 위한 함수
     *************************************************************************/
    static void setReplicationMaxParallelCount(UInt aMaxParallelCount);

    /*************************************************************************
     * 모듈 초기화 함수
     *************************************************************************/
    static IDE_RC initialize();

    static void finalize();

    /*************************************************************************
     * database creation & drop
     **************************************************************************/
    static IDE_RC validateCreateDB(SChar         * aDBName,
                                   SChar         * aDBCharSet,
                                   SChar         * aNationalCharSet);
    static IDE_RC validateDropDB(SChar         * aDBName);

    static IDE_RC addExtMT_Module( void );

    static IDE_RC initSystemTables( void );

    /*************************************************************************
     * shard query 분석
     *************************************************************************/

    static IDE_RC checkStmtTypeBeforeAnalysis( qcStatement * aStatement );

    // PROJ-2727
    static IDE_RC checkDCLStmt( qcStatement * aStatement );

    static IDE_RC analyze( qcStatement * aStatement,
                           ULong         aSMN );

    static IDE_RC setAnalysisResultForInsert( qcStatement    * aStatement,
                                              sdiAnalyzeInfo * aAnalyzeInfo,
                                              sdiObjectInfo  * aShardObjInfo );

    static IDE_RC setAnalysisResultForDML( qcStatement    * aStatement,
                                           sdiAnalyzeInfo * aAnalyzeInfo,
                                           sdiObjectInfo  * aShardObjInfo );

    static IDE_RC setAnalysisResultForTable( qcStatement    * aStatement,
                                             sdiAnalyzeInfo * aAnalyzeInfo,
                                             sdiObjectInfo  * aShardObjInfo );

    static IDE_RC copyAnalyzeInfo( qcStatement    * aStatement,
                                   sdiAnalyzeInfo * aAnalyzeInfo,
                                   sdiAnalyzeInfo * aSrcAnalyzeInfo );

    static sdiAnalyzeInfo *  getAnalysisResultForAllNodes();

    static IDE_RC getExternalNodeInfo( sdiNodeInfo * aNodeInfo,
                                       ULong         aSMN );

    static IDE_RC getInternalNodeInfo( smiTrans    * aTrans,
                                       sdiNodeInfo * aNodeInfo,
                                       idBool        aIsShardMetaChanged,
                                       ULong         aSMN );

    static IDE_RC getAddedNodeInfo( sdiClientInfo * aClientInfo,
                                    sdiNodeInfo   * aNewNodeInfo,
                                    sdiNodeInfo   * aTarget );

    /* PROJ-2655 Composite shard key */
    static IDE_RC getRangeIndexByValue( qcTemplate     * aTemplate,
                                        mtcTuple       * aShardKeyTuple,
                                        sdiAnalyzeInfo * aShardAnalysis,
                                        UShort           aValueIndex,
                                        sdiValueInfo   * aValue,
                                        sdiRangeIndex  * aRangeIndex,
                                        UShort         * aRangeIndexCount,
                                        idBool         * aHasDefaultNode,
                                        idBool           aIsSubKey );

    static IDE_RC checkValuesSame( qcTemplate   * aTemplate,
                                   mtcTuple     * aShardKeyTuple,
                                   UInt           aKeyDataType,
                                   sdiValueInfo * aValue1,
                                   sdiValueInfo * aValue2,
                                   idBool       * aIsSame );

    static IDE_RC validateNodeNames( qcStatement  * aStatement,
                                     qcShardNodes * aNodeNames );

    static IDE_RC allocAndCopyRanges( qcStatement  * aStatement,
                                      sdiRangeInfo * aTo,
                                      sdiRangeInfo * aFrom );

    static IDE_RC allocAndCopyValues( qcStatement       * aStatement,
                                      sdiValueInfo *   ** aTo,
                                      UShort            * aToCount,
                                      sdiValueInfo     ** aFrom,
                                      UShort              aFromCount );

    /*************************************************************************
     * utility
     *************************************************************************/

    static IDE_RC checkShardLinker( qcStatement * aStatement );
    static IDE_RC checkShardLinkerWithoutSQL( qcStatement * aStatement, ULong aSMN, smiTrans * aTrans );

    static IDE_RC checkShardLinkerWithNodeList( qcStatement * aStatement, 
                                                ULong         aSMN, 
                                                smiTrans    * aTrans, 
                                                iduList     * aList );

    static sdiConnectInfo * findConnect( sdiClientInfo * aClientInfo,
                                         UInt            aNodeId );

    static idBool findBindParameter( sdiAnalyzeInfo * aAnalyzeInfo );

    static idBool findRangeInfo( sdiRangeInfo * aRangeInfo,
                                 UInt           aNodeId );


    // TASK-7244 Set shard split method to PSM info
    static IDE_RC getProcedureShardSplitMethod( qcStatement    * aStatement,
                                                UInt             aUserID,
                                                qcNamePosition   aUserName,
                                                qcNamePosition   aProcName,
                                                sdiSplitMethod * aShardSplitMethod );

    static IDE_RC getProcedureInfoWithPlanTree( qcStatement      * aStatement,
                                    UInt               aUserID,
                                    qcNamePosition     aUserName,
                                    qcNamePosition     aProcName,
                                    qsProcParseTree  * aProcPlanTree,
                                    sdiObjectInfo   ** aShardObjInfo );
    static IDE_RC getProcedureInfo( qcStatement      * aStatement,
                                    UInt               aUserID,
                                    qcNamePosition     aUserName,
                                    qcNamePosition     aProcName,
                                    sdiObjectInfo   ** aShardObjInfo );
    static IDE_RC getTableInfo( qcStatement    * aStatement,
                                qcmTableInfo   * aTableInfo,
                                sdiObjectInfo ** aShardObjInfo );

    static IDE_RC getTableInfo( qcStatement    * aStatement,
                                qcmTableInfo   * aTableInfo,
                                sdiObjectInfo ** aShardObjInfo,
                                idBool           aIsRangeMerge );

    static void   charXOR( SChar * aText, UInt aLen );

    static IDE_RC printMessage( SChar * aMessage,
                                UInt    aLength,
                                void  * aArgument );

    static void   setShardMetaTouched( qcSession * aSession );
    static void   unsetShardMetaTouched( qcSession * aSession );

    static void   setInternalTableSwap( qcSession * aSession );
    static void   unsetInternalTableSwap( qcSession * aSession );

    static IDE_RC touchShardNode( qcSession * aSession,
                                  idvSQL    * aStatistics,
                                  smTID       aTransID,
                                  UInt        aNodeId );

    static IDE_RC openAllShardConnections( qcSession * aSession );

    static idBool getNeedToDisconnect( qcSession * aSession );

    static void   setNeedToDisconnect( sdiClientInfo * aClientInfo,
                                       idBool          aNeedToDisconnect );

    /*************************************************************************
     * shard session
     *************************************************************************/

    /* PROJ-2638 */
    static void initOdbcLibrary();
    static void finiOdbcLibrary();

    static IDE_RC initializeSessionWithNodeList( qcSession * aSession,
                                                 void      * aDkiSession,
                                                 smiTrans  * aTrans,
                                                 idBool      aIsShardMetaChanged,
                                                 ULong       aConnectSMN,
                                                 iduList   * aNodeList );

    static void finalizeSession( qcSession * aSession );

    static IDE_RC allocConnect( sdiConnectInfo * aConnectInfo );

    static void freeConnectImmediately( sdiConnectInfo * aConnectInfo );

    static IDE_RC checkNode( sdiConnectInfo * aConnectInfo );

    static idBool isShardEnable();
    static idBool isShardCoordinator( qcStatement * aStatement );

    static idBool checkNeedFailover( sdiConnectInfo * aConnectInfo );

    // BUG-45411
    static IDE_RC endPendingTran( sdiConnectInfo * aConnectInfo,
                                  idBool           aIsCommit );

    static IDE_RC commit( sdiConnectInfo * aConnectInfo );

    static IDE_RC rollback( sdiConnectInfo * aConnectInfo,
                            const SChar    * aSavePoint );

    static IDE_RC savepoint( sdiConnectInfo * aConnectInfo,
                             const SChar    * aSavePoint );

    static void xidInitialize( sdiConnectInfo * aConnectInfo );

    static IDE_RC addPrepareTranCallback( void           ** aCallback,
                                          sdiConnectInfo  * aNode );

    static IDE_RC addEndPendingTranCallback( void           ** aCallback,
                                             sdiConnectInfo  * aNode,
                                             idBool            aIsCommit );

    static IDE_RC addEndTranCallback( void           ** aCallback,
                                      sdiConnectInfo  * aNode,
                                      idBool            aIsCommit );

    static void doCallback( void * aCallback );

    static IDE_RC resultCallback( void           * aCallback,
                                  sdiConnectInfo * aNode,
                                  SChar          * aFuncName,
                                  idBool           aReCall );

    static void removeCallback( void * aCallback );

    static IDE_RC setFailoverSuspend( qcSession              * aSession,
                                      sdiFailoverSuspendType   aSuspendOnType,
                                      UInt                     aNewErrorCode = idERR_IGNORE_NoError );

    static IDE_RC setFailoverSuspend( sdiConnectInfo         * aConnectInfo,
                                      sdiFailoverSuspendType   aSuspendOnType,
                                      UInt                     aNewErrorCode = idERR_IGNORE_NoError );

    static void shardStmtPartialRollbackUsingSavepoint( qcStatement    * aStatement,
                                                        sdiClientInfo  * aClientInfo,
                                                        sdiDataNodes   * aDataInfo );

    /*************************************************************************
     * etc
     *************************************************************************/

    static void setExplainPlanAttr( qcSession * aSession,
                                    UChar       aValue );
    static IDE_RC shardExecDirectNested( qcStatement          * aStatement,
                                         SChar                * aNodeName,
                                         SChar                * aQuery,
                                         UInt                   aQueryLen,
                                         sdiInternalOperation   aInternalLocalOperation,
                                         UInt                 * aExecCount );
    static IDE_RC shardExecDirect( qcStatement               * aStatement,
                                   SChar                     * aNodeName,
                                   SChar                     * aQuery,
                                   UInt                        aQueryLen,
                                   sdiInternalOperation        aInternalLocalOperation,
                                   UInt                      * aExecCount,
                                   ULong                     * aNumResult,
                                   SChar                     * aStrResult,
                                   UInt                        aMaxBuffer,
                                   idBool                    * aFetchResult );

    static IDE_RC shardExecTempDDLWithNewTrans( qcStatement * aStatement,
                                                SChar       * aNodeName,
                                                SChar       * aQuery,
                                                UInt          aQueryLen );

    static IDE_RC shardExecTempDMLOrDCLWithNewTrans( qcStatement * aStatement,
                                                     SChar       * aQuery );

    static IDE_RC setCommitMode( qcSession * aSession,
                                 idBool      aIsAutoCommit,
                                 UInt        aDBLinkGTXLevel,
                                 idBool      aIsGTx,
                                 idBool      aIsGCTx );  /* BUG-48109 */

    static IDE_RC setTransactionLevel( qcSession * aSession,
                                       UInt        aOldDBLinkGTXLevel,
                                       UInt        aNewDBLinkGTXLevel );

    static sdiDatabaseInfo getShardDBInfo();
    static IDE_RC getLocalMetaInfo( sdiLocalMetaInfo * aLocalMetaInfo );
    static IDE_RC getLocalMetaInfoAndCheckKSafety( sdiLocalMetaInfo * aLocalMetaInfo, idBool * aOutIsKSafetyNULL );
    
    static IDE_RC makeShardMeta4NewSMN( qcStatement * aStatement );

    static idBool isUserAutoCommit( sdiConnectInfo * aConnectInfo );
    static idBool isMetaAutoCommit( sdiConnectInfo * aConnectInfo );

    static IDE_RC setTransactionalDDLMode( qcSession * aSession, idBool aTransactionalDDL );

    static void initAffectedRow( sdiConnectInfo   * aConnectInfo );
    static idBool isAffectedRowSetted( sdiConnectInfo * aConnectInfo );

    /* BUG-45967 Rebuild Data 완료 대기 */
    static IDE_RC waitToRebuildData( idvSQL * aStatistics );

    // PROJ-2727
    static IDE_RC setShardSessionProperty( qcSession      * aSession,
                                           sdiClientInfo  * aClientInfo,
                                           sdiConnectInfo * aConnectInfo );

    static IDE_RC setSessionPropertyFlag( qcSession * aSession,
                                          UShort      aSessionPropID );
    
    static void unSetSessionPropertyFlag( qcSession * aSession );

    // BUG-47765
    static IDE_RC copyPropertyFlagToCoordPropertyFlag( qcSession     * aSession,
                                                       sdiClientInfo * aClientInfo );
    
    static IDE_RC getGlobalMetaInfoCore( smiStatement          * aSmiStmt,
                                         sdiGlobalMetaInfo * aMetaNodeInfo );

    static IDE_RC getAllReplicaSetInfoSortedByPName( smiStatement       * aSmiStmt,
                                                     sdiReplicaSetInfo  * aReplicaSetsInfo,
                                                     ULong                aSMN );
    
    static IDE_RC getTableInfoAllObject( qcStatement        * aStatement,
                                         sdiTableInfoList  ** aTableInfoList,
                                         ULong                aSMN );
    
    static IDE_RC getRangeInfo( qcStatement  * aStatement,
                                smiStatement * aSmiStmt,
                                ULong          aSMN,
                                sdiTableInfo * aTableInfo,
                                sdiRangeInfo * aRangeInfo,
                                idBool         aNeedMerge );

    static IDE_RC getNodeByName( smiStatement * aSmiStmt,
                                 SChar        * aNodeName,
                                 ULong          aSMN,
                                 sdiNode      * aNode );
    
    static IDE_RC addReplTable( qcStatement * aStatement,
                                SChar       * aNodeName,
                                SChar       * aReplName,
                                SChar       * aUserName,
                                SChar       * aTableName,
                                SChar       * aPartitionName,
                                sdiReplDirectionalRole aRole,
                                idBool        aIsNewTrans );

    static IDE_RC findSendReplInfoFromReplicaSet( sdiReplicaSetInfo   * aReplicaSetInfo,
                                                  SChar               * aNodeName,
                                                  sdiReplicaSetInfo   * aOutReplicaSetInfo );
    
    static IDE_RC validateSMNForShardDDL( qcStatement * aStatement );

    static IDE_RC compareDataAndSessionSMN( qcStatement * aStatement );

    static IDE_RC checkReferenceObj( qcStatement * aStatement,
                                     SChar       * aNodeName );
    
    /*************************************************************************
     * PROJ-2733-DistTxInfo Distribution Transaction Information
     *************************************************************************/
    
    static IDE_RC propagateDistTxInfoToNodes( qcStatement    * aStatement,
                                              sdiClientInfo  * aClientInfo,
                                              sdiDataNodes   * aDataInfo );
    static IDE_RC updateMaxNodeSCNToCoordSCN( qcStatement    * aStatement,
                                              sdiClientInfo  * aClientInfo,
                                              sdiDataNodes   * aDataInfo );
    static IDE_RC getSCN( sdiConnectInfo * aConnectInfo, smSCN * aSCN );
    static IDE_RC setSCN( sdiConnectInfo * aConnectInfo, smSCN * aSCN );
    static IDE_RC setTxFirstStmtSCN( sdiConnectInfo * aConnectInfo, smSCN * aTxFirstStmtSCN );
    static IDE_RC setTxFirstStmtTime( sdiConnectInfo * aConnectInfo, SLong aTxFirstStmtTime );
    static IDE_RC setDistLevel( sdiConnectInfo * aConnectInfo, sdiDistLevel aDistLevel );
    static inline void initDistTxInfo( sdiClientInfo * aClientInfo );
    static inline void endTranDistTx( sdiClientInfo * aClientInfo, idBool aIsGCTx );  /* BUG-48109 */
    static void calcDistTxInfo( qcTemplate   * aTemplate,
                                sdiDataNodes * aDataInfo,
                                idBool         aCalledByPSM );
    static void decideRequestSCN( qcTemplate * aTemplate,
                                  idBool       aCalledByPSM,
                                  UInt         aPlanIndex );
    static void calculateGCTxInfo( qcTemplate   * aTemplate,
                                   sdiDataNodes * aDataInfo,
                                   idBool         aCalledByPSM,
                                   UInt           aPlanIndex );
    static inline IDE_RC syncSystemSCN4GCTx( smSCN * aLastSystemSCN, smSCN * aNewLastSystemSCN );
    static inline void getSystemSCN4GCTx( smSCN * aLastSystemSCN );
    static IDE_RC buildDataNodes( sdiClientInfo * aClientInfo,
                                  sdiDataNodes  * aDataInfo,
                                  SChar         * aNodeName,
                                  UInt          * aSelectedNodeCnt ); /* BUG-48666 */
    static void checkErrorIsShardRetry( sdiStmtShardRetryType * aRetryType );
    static idBool isNeedRequestSCN( qciStatement * aStatement );

    /*************************************************************************
     * shard statement
     *************************************************************************/

    static void initDataInfo( qcShardExecData * aExecData );

    static IDE_RC allocDataInfo( qcShardExecData * aExecData,
                                 iduVarMemList   * aMemory );

    static void setDataNodePrepared( sdiClientInfo * aClientInfo,
                                     sdiDataNodes  * aDataNode );

    static void closeDataInfo( qcStatement     * aStatement,
                               qcShardExecData * aExecData );

    static void clearDataInfo( qcStatement     * aStatement,
                               qcShardExecData * aExecData );

    static void clearShardDataInfoForRebuild( qcStatement     * aStatement,
                                              qcShardExecData * aExecData );
    // 수행정보 초기화
    static IDE_RC initShardDataInfo( qcTemplate     * aTemplate,
                                     sdiAnalyzeInfo * aShardAnalysis,
                                     sdiClientInfo  * aClientInfo,
                                     sdiDataNodes   * aDataInfo,
                                     sdiDataNode    * aDataArg );

    // 수행정보 재초기화
    static IDE_RC reuseShardDataInfo( qcTemplate     * aTemplate,
                                      sdiClientInfo  * aClientInfo,
                                      sdiDataNodes   * aDataInfo,
                                      sdiBindParam   * aBindParams,
                                      UShort           aBindParamCount,
                                      sdiSVPStep       aSVPStep );

    // 수행노드 결정
    static IDE_RC decideShardDataInfo( qcTemplate     * aTemplate,
                                       mtcTuple       * aShardKeyTuple,
                                       sdiAnalyzeInfo * aShardAnalysis,
                                       sdiClientInfo  * aClientInfo,
                                       sdiDataNodes   * aDataInfo,
                                       qcNamePosition * aShardQuery );

    static IDE_RC getExecNodeRangeIndex( qcTemplate        * aTemplate,
                                         mtcTuple          * aShardKeyTuple,
                                         mtcTuple          * aShardSubKeyTuple,
                                         sdiAnalyzeInfo    * aShardAnalysis,
                                         UShort            * aRangeIndex,
                                         UShort            * aRangeIndexCount,
                                         idBool            * aExecDefaultNode,
                                         idBool            * aExecAllNodes );

    static IDE_RC setPrepareSelected( sdiClientInfo    * aClientInfo,
                                      sdiDataNodes     * aDataInfo,
                                      idBool             aAllNodes,
                                      UInt               aNodeId );

    static IDE_RC prepare( qcTemplate       * aTemplate,
                           sdiClientInfo    * aClientInfo,
                           sdiDataNodes     * aDataInfo,
                           qcNamePosition   * aShardQuery );

    static IDE_RC executeDML( qcStatement    * aStatement,
                              sdiClientInfo  * aClientInfo,
                              sdiDataNodes   * aDataInfo,
                              qmxLobInfo     * aLobInfo,
                              vSLong         * aNumRows );

    static IDE_RC executeInsert( qcStatement    * aStatement,
                                 sdiClientInfo  * aClientInfo,
                                 sdiDataNodes   * aDataInfo,
                                 qmxLobInfo     * aLobInfo,
                                 vSLong         * aNumRows );

    static IDE_RC executeSelect( qcStatement    * aStatement,
                                 sdiClientInfo  * aClientInfo,
                                 sdiDataNodes   * aDataInfo );

    static IDE_RC fetch( sdiConnectInfo  * aConnectInfo,
                         sdiDataNode     * aDataNode,
                         idBool          * aExist );

    static IDE_RC getPlan( sdiConnectInfo  * aConnectInfo,
                           sdiDataNode     * aDataNode );

    /*************************************************************************
     * shard meta info
     *************************************************************************/
    static UInt getShardUserID();

    static void setShardUserID( UInt aShardUserID );

    static UInt getShardStatus();

    static void setShardStatus( UInt aShardStatus );
    
    static IDE_RC loadMetaNodeInfo();

    static void setShardDBInfo();

    static ULong  getSMNForMetaNode();

    static void   setSMNCacheForMetaNode( ULong aNewSMN );

    static void   loadShardPinInfo();

    static IDE_RC getIncreasedSMNForMetaNode( smiTrans * aTrans,
                                              ULong    * aNewSMN );

    static IDE_RC reloadSMNForDataNode( smiStatement * aSmiStmt );

    /*************************************************************************
     * data node info
     *************************************************************************/

    static ULong  getSMNForDataNode();

    static void   setSMNForDataNode( ULong aNewSMN );

    /*************************************************************************
     * shard pin
     *************************************************************************/
    static idBool checkMetaCreated();

    static sdiShardPin makeShardPin();

    static void shardPinToString( SChar *aDst, UInt aLen, sdiShardPin aShardPIn );

    /*************************************************************************
     * ODBC Connect Attribute
     *************************************************************************/
    static UInt getShardInternalConnAttrRetryCount();
    static UInt getShardInternalConnAttrRetryDelay();
    static UInt getShardInternalConnAttrConnTimeout();
    static UInt getShardInternalConnAttrLoginTimeout();

    /*************************************************************************
     * BUG-45899 Print analysis information
     *************************************************************************/
    static idBool isGCTxPlanPrintable( qcStatement * aStatement );

    static idBool isAnalysisInfoPrintable( qcStatement * aStatement );

    static void printGCTxPlan( qcTemplate   * aTemplate,
                               iduVarString * aString );

    static void printAnalysisInfo( qcStatement  * aStatement,
                                   iduVarString * aString );

    static sdiQueryType getQueryType( qciStatement * aStatement );

    static void setNonShardQueryReason( sdiPrintInfo * aPrintInfo,
                                        UShort         aReason );

    static SChar * getNonShardQueryReasonArr( UShort aArrIdx );

    /*************************************************************************
     * shard client fail-over align
     *************************************************************************/
    static void closeShardSessionByNodeId( qcSession * aSession,
                                           UInt        aNodeId,
                                           UChar       aDestination );

    static void setTransactionBroken( idBool     aIsUserAutoCommit,
                                      void     * aDkiSession,
                                      smiTrans * aTrans );

    static idBool isSupportDataType( UInt aModuleID );

    /*************************************************************************
     * online data rebuild
     *************************************************************************/
    static idBool detectShardMetaChange( qcStatement * aStatement );

    /* TASK-7219 Non-shar DML */
    static idBool isPartialCoordinator( qcStatement * aStatement );

    static IDE_RC waitAndSetSMNForMetaNode( idvSQL       * aStatistics,
                                            smiStatement * aSmiStmt,
                                            UInt           aFlag,
                                            ULong          aNeededSMN,
                                            ULong        * aDataSMN );

    static IDE_RC convertRangeValue( SChar       * aValue,
                                     UInt          aLength,
                                     UInt          aKeyType,
                                     sdiValue    * aRangeValue );

    static IDE_RC compareKeyData( UInt       aKeyDataType,
                                  sdiValue * aValue1,
                                  sdiValue * aValue2,
                                  SShort   * aResult );

    static idBool hasShardCoordPlan( qcStatement * aStatement );

    // BUG-49088
    static idBool isShardDML( qcStatement * aStatement );

    static idBool isShardDDL( qcStatement * aStatement );

    // BUG-48616
    static idBool isShardDDLForAddClone( qcStatement * aStatement );
    
    static void isShardSelectExists( qmnPlan * aPlan,
                                     idBool  * aIsSDSEExists  );

    static void getShardObjInfoForSMN( ULong            aSMN,
                                       sdiObjectInfo  * aShardObjectList,
                                       sdiObjectInfo ** aRet );

    static IDE_RC unionNodeInfo( sdiNodeInfo * aSourceNodeInfo,
                                 sdiNodeInfo * aTargetNodeInfo );

    static inline idBool isShardMetaChange( qciStatement    * aQciStatement );
    static inline void printChangeMetaMessage( const SChar      * aQuery,
                                               IDE_RC             aReturnCode );

    /* BUG-47459 */
    static const SChar * getShardSavepointName( qciStmtType aStmtKind );

    static inline sdiSplitType getSplitType( sdiSplitMethod aSplitMethod );

    static IDE_RC  updateLocalMetaInfoForReplication( qcStatement  * aStatement,
                                                      UInt           aKSafety,
                                                      SChar        * aReplicationMode,
                                                      UInt           aParallelCount,
                                                      UInt         * aRowCnt );

    static IDE_RC propagateShardMetaNumber( qcSession * aSession );

    static IDE_RC propagateRebuildShardMetaNumber( qcSession * aSession );

    static IDE_RC updateSession( qcSession * aSession,
                                 void      * aDkiSession,
                                 smiTrans  * aTrans,
                                 idBool      aIsShardMetaChanged,
                                 ULong       aNewSMN );

    static void cleanupShardRebuildSession( qcSession * aSession, idBool * aRemoved );

    static idBool isReshardOccurred( ULong aSessionSMN,
                                     ULong aLastSessionSMN );

    static IDE_RC makeShardSession( qcSession             * aSession,
                                    void                  * aDkiSession,
                                    smiTrans              * aTrans,
                                    idBool                  aIsShardMetaChanged,
                                    ULong                   aSessionSMN,
                                    ULong                   aLastSessionSMN,
                                    sdiRebuildPropaOption   aSendRebuildSMN,
                                    idBool                  aIsPartialCoord );

    static IDE_RC checkTargetSMN( sdiClientInfo * aClientInfo, sdiConnectInfo * aConnectInfo );

    /*************************************************************************
     * PROJ-2728 Sharding LOB
     *************************************************************************/
    static UInt   getRemoteStmtId( sdiDataNode * aDataNode );

    static IDE_RC getSplitMethodCharByType(sdiSplitMethod aSplitMethod, SChar* aOutChar);
    static IDE_RC getSplitMethodCharByStr(SChar* aSplitMethodStr, SChar* aOutChar);

    static inline idBool isSingleShardKeyDistTable(sdiTableInfo * aTableInfo);
    static inline idBool isCompositeShardKeyDistTable(sdiTableInfo * aTableInfo);
    static inline sdiShardObjectType getShardObjectType(sdiTableInfo * aTableInfo);
    static inline void makeReplName(UInt aReplicaSetID, sdiBackupNumber aBackupNumber, SChar *aOutBuf);
    static inline SInt getBackupNodeIdx(SInt aMyIdx, SInt aTotalCnt, sdiBackupOrder aOrder);
    static IDE_RC setInternalOp( sdiInternalOperation    aValue, 
                                 sdiConnectInfo        * aConnectInfo, 
                                 idBool                * aOutIsLinkFailure );
    static IDE_RC validateOneReshardTable( qcStatement * aStatement,
                                           SChar * aUserName,
                                           SChar * aTableName, 
                                           SChar * aPartitionName, 
                                           sdiObjectInfo * aTableObjInfo,
                                           SChar * aOutFromNodeName,
                                           SChar * aOutDefaultNodeName );
    static IDE_RC validateOneReshardProc( qcStatement * aStatement,
                                          SChar * aUserName,
                                          SChar * aProcName,
                                          SChar * aKeyValue,
                                          sdiObjectInfo * aProcObjInfo,
                                          SChar * aOutFromNodeName,
                                          SChar * aOutDefaultNodeName );
    static IDE_RC getReplicaSet( smiTrans          * aTrans,
                                 SChar             * aPrimaryNodeName,
                                 idBool              aIsShardMetaChanged,
                                 ULong               aSMN,
                                 sdiReplicaSetInfo * aReplicaSetsInfo );
    static IDE_RC checkBackupReplicationRunning( UInt            aKSafety,
                                                 sdiReplicaSet * aReplicaSet,
                                                 UInt            aNodeCount );
    static IDE_RC validateObjectSMN( qcStatement    * aStatement,
                                     sdiObjectInfo * aShardObjInfo );
    static IDE_RC getTableInfoAllForDDL( qcStatement    * aStatement,
                                         UInt             aUserID,
                                         UChar          * aTableName,
                                         SInt             aTableNameSize,
                                         idBool           aIsRangeMerge,
                                         qcmTableInfo  ** aTableInfo,
                                         smSCN          * aTableSCN,
                                         void          ** aTableHandle,
                                         sdiObjectInfo ** aShardObjInfo );

    static IDE_RC getRangeValueStrFromPartition(SChar * aPartitionName, sdiObjectInfo * aShardObjectInfo, SChar * aOutValueBuf);
    static IDE_RC getValueStr( UInt       aKeyDataType,
                               sdiValue * aValue,
                               sdiValue * aValueStr );
    static IDE_RC getRemoteRowCount( qcStatement * aStatement,
                                     SChar       * aNodeName,
                                     SChar       * aUserName,
                                     SChar       * aTableName, 
                                     SChar       * aPartitionName,
                                     sdiInternalOperation          aInternalLocalOperation,
                                     UInt        * aRowCount );
    static idBool isSameNode(SChar * aNodeName1, SChar * aNodeName2);

    static IDE_RC shardExecTempSQLWithoutSession( SChar       * aQuery,
                                                  SChar       * aExecNodeName,
                                                  UInt          aDDLLockTimeout,
                                                  qciStmtType   aSQLStmtType );

    static IDE_RC systemPropertyForShard( qcStatement * aStatement,
                                          SChar       * aNodeName,
                                          SChar       * aSqlStr );
    
    static IDE_RC filterNodeInfo( sdiNodeInfo * aSourceNodeInfo,
                                  sdiNodeInfo * aTargetNodeInfo,
                                  iduList     * aNodeList );

    static IDE_RC getAllBranchWithoutSession( void * aDtxInfo );
    static IDE_RC findRequestNodeNGetResultWithoutSession( ID_XID * aXID,
                                                           idBool * aFindRequestNode,
                                                           idBool * aIsCommit,
                                                           smSCN  * aGlobalCommitSCN );


    /*************************************************************************
     * TASK-7218 Multi-Error Handling
     *************************************************************************/
    static inline UInt   getMultiErrorCode();
    static inline SChar* getMultiErrorMsg();

    /* TASK-7219 Shard Transformer Refactoring */
    static IDE_RC allocAndInitQuerySetList( qcStatement * aStatement );

    static IDE_RC makeAndSetQuerySetList( qcStatement * aStatement,
                                          qmsQuerySet * aQuerySet );

    static IDE_RC setQuerySetListState( qcStatement  * aStatement,
                                        qmsParseTree * aParseTree,
                                        idBool       * aIsChanged );

    static IDE_RC unsetQuerySetListState( qcStatement * aStatement,
                                          idBool        aIsChanged );

    static IDE_RC setStatementFlagForShard( qcStatement * aStatement,
                                            UInt          aFlag );

    static IDE_RC isTransformNeeded( qcStatement * aStatement,
                                     idBool      * aIsTransformNeeded );

    static IDE_RC isRebuildTransformNeeded( qcStatement * aStatement,
                                            idBool      * aIsTransformNeeded );

    static IDE_RC setPrintInfoFromTransformAble( qcStatement * aStatement );

    static IDE_RC raiseInvalidShardQueryError( qcStatement * aStatement,
                                               qcParseTree * aParseTree );

    static IDE_RC preAnalyzeQuerySet( qcStatement * aStatement,
                                      qmsQuerySet * aQuerySet,
                                      ULong         aSMN );

    static IDE_RC preAnalyzeSubquery( qcStatement * aStatement,
                                      qtcNode     * aNode,
                                      ULong         aSMN );

    static IDE_RC makeAndSetAnalysis( qcStatement * aSrcStatement,
                                      qcStatement * aDstStatement,
                                      qmsQuerySet * aDstQuerySet );

    static IDE_RC allocAndCopyTableInfoList( qcStatement       * aStatement,
                                             sdiTableInfoList  * aSrcTableInfoList,
                                             sdiTableInfoList ** aDstTableInfoList );

    static IDE_RC makeAndSetAnalyzeInfoFromStatement( qcStatement * aStatement );

    static IDE_RC makeAndSetAnalyzeInfoFromParseTree( qcStatement     * aStatement,
                                                      qcParseTree     * aParseTree,
                                                      sdiAnalyzeInfo ** aAnalyzeInfo );

    static IDE_RC makeAndSetAnalyzeInfoFromQuerySet( qcStatement     * aStatement,
                                                     qmsQuerySet     * aQuerySet,
                                                     sdiAnalyzeInfo ** aAnalyzeInfo );

    static IDE_RC makeAndSetAnalyzeInfoFromObjectInfo( qcStatement     * aStatement,
                                                       sdiObjectInfo   * aShardObjInfo,
                                                       sdiAnalyzeInfo ** aAnalyzeInfo );

    static IDE_RC isShardParseTree( qcParseTree * aParseTree,
                                    idBool      * aIsShardParseTree );

    static IDE_RC isShardQuerySet( qmsQuerySet * aQuerySet,
                                   idBool      * aIsShardQuerySet,
                                   idBool      * aIsTransformAble );

    static IDE_RC isShardObject( qcParseTree * aParseTree,
                                 idBool      * aIsShardObject );

    static IDE_RC isSupported( qmsQuerySet * aQuerySet,
                               idBool      * aIsSupported );

    static IDE_RC setShardPlanStmtVariable( qcStatement    * aStatement,
                                            qcShardStmtType  aStmtType,
                                            qcNamePosition * aStmtPos );

    static IDE_RC setShardPlanCommVariable( qcStatement      * aStatement,
                                            sdiAnalyzeInfo   * aAnalyzeInfo,
                                            UShort             aParamCount,
                                            UShort             aParamOffset,
                                            qcShardParamInfo * aParamInfo );

    static IDE_RC setShardStmtType( qcStatement * aStatement,
                                    qcStatement * aViewStatement );

    static IDE_RC checkShardObjectForDDL( qcStatement  * aQcStmt,
                                          sdiDDLType     aDDLType,
                                          SChar        * aObjectName = NULL );

    static IDE_RC checkShardObjectForDDLInternal( qcStatement *      aQcStmt,
                                                  qcNamePosition     aUserNamePos,
                                                  qcNamePosition     aTableNamePos,
                                                  SChar            * aObjectName );

    static IDE_RC checkShardReplication( qcStatement * aQcStmt );

    static IDE_RC resetShardMetaWithDummyStmt( void );

    static IDE_RC closeSessionForShardDrop( qcStatement * aQcStmt );

    // TASK-7244 PSM partial rollback in Sharding
    static IDE_RC rollbackForPSM( qcStatement   * aStatement,
                                  sdiClientInfo * aClientInfo );
    static IDE_RC clearPsmSvp( sdiClientInfo * aClientInfo );

    // TASK-7219 Non-shard DML
    static IDE_RC getParseTreeAnalysis( qcParseTree       * aParseTree,
                                        sdiShardAnalysis ** aAnalysis );

};

/**
 *
 */
inline SInt sdi::getBackupNodeIdx(SInt aMyIdx, SInt aTotalCnt, sdiBackupOrder aOrder)
{
    SInt sBackupNodeIdx = SDI_NODE_NULL_IDX;

    if ( aTotalCnt > (SInt)aOrder )
    {
        sBackupNodeIdx = (((aMyIdx) + (SInt)(aOrder)) % (aTotalCnt));
    }

    return sBackupNodeIdx;
}
/*
 * "REPL_SET_"+"aReplicaSetID"+"_BACKUP_"+"aBackupNumber"
 * aBackupNumber: n
 */
inline void sdi::makeReplName(UInt aReplicaSetID, sdiBackupNumber aBackupNumber, SChar *aOutBuf)
{
    IDE_DASSERT(aBackupNumber < SDI_BACKUP_MAX);
    idlOS::snprintf(aOutBuf,
                    SDI_REPLICATION_NAME_MAX_SIZE + 1,
                    REPL_NAME_PREFIX"_"
                    "%"ID_INT32_FMT"_"
                    REPL_NAME_POSTFIX"_"
                    "%"ID_INT32_FMT,
                    aReplicaSetID,
                    aBackupNumber);
}

inline idBool sdi::isUserAutoCommit( sdiConnectInfo * aConnectInfo )
{
    idBool sIsAutoCommit;

    sIsAutoCommit = ( ( aConnectInfo->mFlag & SDI_CONNECT_USER_AUTOCOMMIT_MODE_MASK )
                      == SDI_CONNECT_USER_AUTOCOMMIT_MODE_ON )
                    ? ID_TRUE
                    : ID_FALSE;

    return sIsAutoCommit;
}

inline idBool sdi::isMetaAutoCommit( sdiConnectInfo * aConnectInfo )
{
    idBool sIsAutoCommit;

    sIsAutoCommit = ( ( aConnectInfo->mFlag & SDI_CONNECT_COORD_AUTOCOMMIT_MODE_MASK )
                      == SDI_CONNECT_COORD_AUTOCOMMIT_MODE_ON )
                    ? ID_TRUE
                    : ID_FALSE;

    return sIsAutoCommit;
}

inline void sdi::initAffectedRow( sdiConnectInfo    * aConnectInfo )
{
    aConnectInfo->mAffectedRowCount = SDI_AFFECTED_ROW_INITIAL;
}

inline idBool sdi::isAffectedRowSetted( sdiConnectInfo    * aConnectInfo )
{
    idBool  sIsAffectedRowSetted = ID_TRUE;

    if ( aConnectInfo->mAffectedRowCount >= SDI_AFFECTED_ROW_SETTED )
    {
        sIsAffectedRowSetted = ID_TRUE;
    }
    else
    {
        sIsAffectedRowSetted = ID_FALSE;
    }

    return sIsAffectedRowSetted;
}

inline idBool sdi::isShardMetaChange( qciStatement  * aQciStatement )
{
    qcStatement     * sQcStatement = &(aQciStatement->statement);
    idBool            sIsShardMetaChange = ID_FALSE;

    if ( ( sQcStatement->mFlag & QC_STMT_SHARD_META_CHANGE_MASK ) 
         == QC_STMT_SHARD_META_CHANGE_TRUE )
    {
        sIsShardMetaChange = ID_TRUE;
    }
    else
    {
        sIsShardMetaChange = ID_FALSE;
    }

    return sIsShardMetaChange;
}

inline void sdi::printChangeMetaMessage( const SChar    * aQuery,
                                         IDE_RC           aReturnCode )
{

    ideLog::log( IDE_SD_17, "[CHANGE_SHARD_META : %s]", aQuery );

    if ( aReturnCode == IDE_SUCCESS )
    {
        ideLog::log( IDE_SD_17, "[CHANGE_SHARD_META : SUCCESS]" );
    }
    else
    {
        ideLog::log( IDE_SD_17, "[CHANGE_SHARD_META : FAILURE] ERR-<%"ID_xINT32_FMT"> : <%s>", 
                     E_ERROR_CODE( ideGetErrorCode() ),
                     ideGetErrorMsg( ideGetErrorCode() ) );
    }
}

inline sdiSplitType sdi::getSplitType( sdiSplitMethod aSplitMethod )
{
    sdiSplitType sSplitType = SDI_SPLIT_TYPE_NONE;

    switch( aSplitMethod )
    {
        case SDI_SPLIT_HASH  :
        case SDI_SPLIT_RANGE :
        case SDI_SPLIT_LIST  :
            sSplitType = SDI_SPLIT_TYPE_DIST;
            break;
        case SDI_SPLIT_CLONE :
        case SDI_SPLIT_SOLO  :
            sSplitType = SDI_SPLIT_TYPE_NO_DIST;
            break;
        default :
            sSplitType = SDI_SPLIT_TYPE_NONE;
            break;
    }

    return sSplitType;
}

inline void sdi::setReplicationMaxParallelCount(UInt aMaxParallelCount)
{
    sduProperty::setReplicationMaxParallelCount(aMaxParallelCount);
}

inline idBool sdi::isSingleShardKeyDistTable(sdiTableInfo * aTableInfo)
{
    IDE_DASSERT(aTableInfo != NULL);
    if ( ( ( aTableInfo->mSplitMethod == SDI_SPLIT_HASH )  ||
           ( aTableInfo->mSplitMethod == SDI_SPLIT_RANGE ) ||
           ( aTableInfo->mSplitMethod == SDI_SPLIT_LIST ) ) &&
           ( aTableInfo->mSubKeyExists == ID_FALSE ) )
    {
        IDE_DASSERT(aTableInfo->mSubSplitMethod == SDI_SPLIT_NONE);
        return ID_TRUE;
    }
    else
    {
        return ID_FALSE;
    }
}

inline idBool sdi::isCompositeShardKeyDistTable(sdiTableInfo * aTableInfo)
{
    IDE_DASSERT(aTableInfo != NULL);
    if ( aTableInfo->mSubKeyExists == ID_TRUE )
    {
        return ID_TRUE;
    }
    else
    {
        IDE_DASSERT(aTableInfo->mSubSplitMethod == SDI_SPLIT_NONE);
        return ID_FALSE;
    }
}

inline sdiShardObjectType sdi::getShardObjectType(sdiTableInfo * aTableInfo)
{
    sdiShardObjectType sShardTableType;
    IDE_DASSERT(aTableInfo != NULL);

    switch ( aTableInfo->mSplitMethod )
    {
        case SDI_SPLIT_HASH:
        case SDI_SPLIT_RANGE:
        case SDI_SPLIT_LIST:
            if ( aTableInfo->mSubKeyExists == ID_FALSE )
            {
                IDE_DASSERT(aTableInfo->mSubSplitMethod == SDI_SPLIT_NONE);
                sShardTableType = SDI_SINGLE_SHARD_KEY_DIST_OBJECT;
            }
            else //( aTableInfo->mSubKeyExists == ID_TRUE )
            {
                IDE_DASSERT(aTableInfo->mSubSplitMethod != SDI_SPLIT_NONE);
                sShardTableType = SDI_COMPOSITE_SHARD_KEY_DIST_OBJECT;
            }
            break;
        case SDI_SPLIT_CLONE:
            sShardTableType = SDI_CLONE_DIST_OBJECT;
            break;
        case SDI_SPLIT_SOLO:
            sShardTableType = SDI_SOLO_DIST_OBJECT;
            break;
        default:
            sShardTableType = SDI_NON_SHARD_OBJECT;
            break;
    }
    return sShardTableType;
}

/* PROJ-2733-DistTxInfo */
void sdi::initDistTxInfo( sdiClientInfo * aClientInfo )
{
    SM_INIT_SCN( &(aClientInfo->mGCTxInfo.mTxFirstStmtSCN) );
    aClientInfo->mGCTxInfo.mTxFirstStmtTime = 0;
    aClientInfo->mGCTxInfo.mDistLevel       = SDI_DIST_LEVEL_INIT;

    SDI_INIT_COORD_SCN( &(aClientInfo->mGCTxInfo.mCoordSCN) );
    SM_INIT_SCN( &(aClientInfo->mGCTxInfo.mRequestSCN) );
    SM_INIT_SCN( &(aClientInfo->mGCTxInfo.mPrepareSCN) );
    SM_INIT_SCN( &(aClientInfo->mGCTxInfo.mGlobalCommitSCN) );
}

void sdi::endTranDistTx( sdiClientInfo * aClientInfo, idBool aIsGCTx )
{
    SM_INIT_SCN( &(aClientInfo->mGCTxInfo.mTxFirstStmtSCN) );
    aClientInfo->mGCTxInfo.mTxFirstStmtTime = 0;
    aClientInfo->mGCTxInfo.mDistLevel       = SDI_DIST_LEVEL_INIT;

    if ( aIsGCTx == ID_TRUE )  /* BUG-48109 */
    {
        SM_SET_MAX_SCN( &(aClientInfo->mGCTxInfo.mCoordSCN),
                        &(aClientInfo->mGCTxInfo.mRequestSCN) );
        SM_SET_MAX_SCN( &(aClientInfo->mGCTxInfo.mCoordSCN),
                        &(aClientInfo->mGCTxInfo.mPrepareSCN) );
        SM_SET_MAX_SCN( &(aClientInfo->mGCTxInfo.mCoordSCN),
                        &(aClientInfo->mGCTxInfo.mGlobalCommitSCN) );
    }

    SM_INIT_SCN( &(aClientInfo->mGCTxInfo.mRequestSCN) );
    SM_INIT_SCN( &(aClientInfo->mGCTxInfo.mPrepareSCN) );
    SM_INIT_SCN( &(aClientInfo->mGCTxInfo.mGlobalCommitSCN) );

    #if defined(DEBUG)
    ideLog::log( IDE_SD_18, "= [%s] sdi::endTranDistTx"
                            ", CoordSCN : %"ID_UINT64_FMT
                            ", RequestSCN : %"ID_UINT64_FMT
                            ", PrepareSCN : %"ID_UINT64_FMT
                            ", GlobalCommitSCN : %"ID_UINT64_FMT,
                 aClientInfo->mGCTxInfo.mSessionTypeString,
                 aClientInfo->mGCTxInfo.mCoordSCN,
                 aClientInfo->mGCTxInfo.mRequestSCN,
                 aClientInfo->mGCTxInfo.mPrepareSCN,
                 aClientInfo->mGCTxInfo.mGlobalCommitSCN );
    #endif
}

void sdi::getSystemSCN4GCTx( smSCN * aLastSystemSCN )
{
    smiInaccurateGetLastSystemSCN( aLastSystemSCN );
}

IDE_RC sdi::syncSystemSCN4GCTx( smSCN * aLastSystemSCN, smSCN * aNewLastSystemSCN )
{
    smSCN sLstSystemSCN;

    smiInaccurateGetLastSystemSCN( &sLstSystemSCN );

    /* sLstSystemSCN 은 atomic 연산을 사용하지 않아서 실제 값보다 작은 값으로 반환될수 있다.
     * 이런 경우 smiSetLastSystemSCN 함수에 들어가서 정확하게 비교하면 된다.
     * SystemSCN 은 항상 상승하기 때문에 이러한 비교가 가능하다. */
    if ( SM_SCN_IS_LT( &sLstSystemSCN, aLastSystemSCN ) )
    {
        /* sLstSystemSCN < aLastSystemSCN */
        IDE_TEST( smiSetLastSystemSCN( aLastSystemSCN, aNewLastSystemSCN ) != IDE_SUCCESS );
    }
    else
    {
        /* sLstSystemSCN >= aLastSystemSCN
         * LastSystemSCN 이 업데이트 하려는 SCN 보다 이미 크다.
         * 아무것도 하지 말자 */
        if ( aNewLastSystemSCN != NULL )
        {
            SM_SET_SCN( aNewLastSystemSCN, &sLstSystemSCN );
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_ERRLOG( IDE_SD_0 );

    return IDE_FAILURE;
}

/*
 * TASK-7218 Multi-Error Handling 2nd
 *   Multi-Error의 에러 코드를 구한다.
 *   모두 동일한 에러 코드를 가지고 있으면 첫 번째 에러 코드를 반환,
 *   그렇지 않으면 sdERR_ABORT_SHARD_MULTIPLE_ERRORS 반환
 */
UInt   sdi::getMultiErrorCode()
{
    UInt                sErrorCode;

    sErrorCode = ideGetErrorCode();

    if ( ( ( sErrorCode & E_MODULE_MASK ) == E_MODULE_SD ) &&
         ( ideErrorCollectionSize() > 0 ) )
    {
        if ( ideErrorCollectionIsAllTheSame() == ID_TRUE )
        {
            sErrorCode = ideErrorCollectionFirstErrorCode();
        }
        else
        {
            sErrorCode = sdERR_ABORT_SHARD_MULTIPLE_ERRORS;
        }
    }
    else
    {
        /* Nothing to do */
    }
    return sErrorCode;
}

SChar* sdi::getMultiErrorMsg()
{
    UInt                sErrorCode;

    sErrorCode = ideGetErrorCode();

    if ( ( ( sErrorCode & E_MODULE_MASK ) == E_MODULE_SD ) &&
         ( ideErrorCollectionSize() > 0 ) )
    {
        return ideErrorCollectionMultiErrorMsg();
    }
    else
    {
        return ideGetErrorMsg(sErrorCode);
    }
}

#endif /* _O_SDI_H_ */
