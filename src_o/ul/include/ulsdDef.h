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
 

/***********************************************************************
 * $Id$
 **********************************************************************/

#ifndef _O_ULSD_DEF_H_
#define _O_ULSD_DEF_H_ 1

#define ULSD_MAX_SERVER_IP_LEN      16
#define ULSD_MAX_ALTERNATE_SERVERS_LEN 256
#define ULSD_MAX_CONN_STR_LEN       512
#define ULSD_MAX_ERROR_MESSAGE_LEN  1024
#define ULSD_MAX_NODE_NAME_LEN      40

#define ULSD_SD_NODE_MAX_COUNT  (128) /* SDI_NODE_MAX_COUNT in sdi.h */

/* BUG-46288 */
#define ULSD_HASH_MAX_VALUE                (1000)   // SDI_HASH_MAX_VALUE
#define ULSD_HASH_MAX_VALUE_FOR_TEST       (100)    // SDI_HASH_MAX_VALUE_FOR_TEST

/* PROJ-2598 Shard pilot(shard analyze) */
#define ULN_SHARD_KEY_MAX_CHAR_BUF_LEN     (200)
#define ULN_SHARD_SPLIT_HASH               (1)
#define ULN_SHARD_SPLIT_RANGE              (2)
#define ULN_SHARD_SPLIT_LIST               (3)
#define ULN_SHARD_SPLIT_CLONE              (4)
#define ULN_SHARD_SPLIT_SOLO               (5)

/* PROJ-2660 */
/* ulnDbc -> mAttrGlobalTransactionLevel (acp_uint8_t) */
typedef enum {
    ULN_GLOBAL_TX_ONE_NODE    = 0,
    ULN_GLOBAL_TX_MULTI_NODE  = 1,
    ULN_GLOBAL_TX_GLOBAL      = 2,
    ULN_GLOBAL_TX_GCTX        = 3,
    ULN_GLOBAL_TX_NONE        = 255,
} ulsdGlobalTxLevel;

/* TASK-7219 Non-shard DML */
/* sdiShardPartialCoordType과 동일 */
typedef enum {
    ULN_SHARD_PARTIAL_EXEC_TYPE_NONE  = 0,
    ULN_SHARD_PARTIAL_EXEC_TYPE_COORD = 1,
    ULN_SHARD_PARTIAL_EXEC_TYPE_QUERY = 2,
} ulsdShardPartialExecType;

ACP_INLINE acp_bool_t ulsdIsGTx( acp_uint8_t aLevel )
{
    return ( ( aLevel == ULN_GLOBAL_TX_GLOBAL ) || ( aLevel == ULN_GLOBAL_TX_GCTX ) )
           ? ACP_TRUE : ACP_FALSE;
}

ACP_INLINE acp_bool_t ulsdIsGCTx( acp_uint8_t aLevel )
{
    return ( aLevel == ULN_GLOBAL_TX_GCTX ) ? ACP_TRUE : ACP_FALSE;
}

typedef acp_uint8_t ulsdShardConnType;

typedef enum {
    ULSD_OFFSET_VERSION       = 56,   /* << 7 byte, sdmShardPinInfo.mVersion                  */
    ULSD_OFFSET_RESERVED      = 48,   /* << 6 byte, sdmShardPinInfo.mReserved                 */
    ULSD_OFFSET_SHARD_NODE_ID = 32,   /* << 4 byte, sdmShardPinInfo.mShardNodeInfo.mShardNodeId */
    ULSD_OFFSET_SEQEUNCE      = 0,    /* << 0 byte, sdmShardPinInfo.mSeq                      */
} ulsdShardPinFactorOffet;

typedef enum {
    ULSD_SHARD_PIN_INVALID   = 0,
} ulsdShardPinStatus;               /* = sdiShardPinStatus */

#define ULSD_SHARD_PIN_FORMAT_STR   "%"ACI_UINT32_FMT"-%"ACI_UINT32_FMT"-%"ACI_UINT32_FMT
#define ULSD_SHARD_PIN_FORMAT_ARG( __SHARD_PIN ) \
    ( __SHARD_PIN & ( (acp_uint64_t)0xff       << ULSD_OFFSET_VERSION      ) ) >> ULSD_OFFSET_VERSION, \
    ( __SHARD_PIN & ( (acp_uint64_t)0xffff     << ULSD_OFFSET_SHARD_NODE_ID ) ) >> ULSD_OFFSET_SHARD_NODE_ID, \
    ( __SHARD_PIN & ( (acp_uint64_t)0xffffffff << ULSD_OFFSET_SEQEUNCE     ) ) >> ULSD_OFFSET_SEQEUNCE

typedef enum
{
    ULSD_SHARD_CLIENT_FALSE = 0,
    ULSD_SHARD_CLIENT_TRUE  = 1,
} ulsdShardClient;                  /* = sdiShardClient */

typedef enum
{
    ULSD_SESSION_TYPE_USER  = 0,
    ULSD_SESSION_TYPE_COORD = 1,
    ULSD_SESSION_TYPE_LIB   = 2,
} ulsdSessionType;                  /* = sdiSessionType */

/* BUG-46092 */
typedef enum {
    ULSD_CONN_TO_ACTIVE     = 0,    /* = SDI_FAILOVER_ACTIVE_ONLY */
    ULSD_CONN_TO_ALTERNATE  = 1,    /* = SDI_FAILOVER_ALTERNATE_ONLY */
} ulsdDataNodeConntectTo;

#include <ulnPrivate.h>

typedef struct ulsdAlignInfo            ulsdAlignInfo;
typedef struct ulsdDbcContext           ulsdDbcContext;
typedef struct ulsdDbc                  ulsdDbc;
typedef struct ulsdNodeInfo             ulsdNodeInfo;
typedef struct ulsdNodeReport           ulsdNodeReport;
typedef struct ulsdStmtContext          ulsdStmtContext;
typedef struct ulsdRangeInfo            ulsdRangeInfo;
typedef struct ulsdValueInfo            ulsdValueInfo;
typedef union  ulsdKeyData              ulsdKeyData;
typedef struct ulsdColumnMaxLenInfo     ulsdColumnMaxLenInfo;
typedef union  ulsdValue                ulsdValue;
typedef struct ulsdRangeIndex           ulsdRangeIndex;
typedef struct ulsdPrepareArgs          ulsdPrepareArgs;
typedef struct ulsdExecuteArgs          ulsdExecuteArgs;
typedef struct ulsdPrepareTranArgs      ulsdPrepareTranArgs;
typedef struct ulsdEndPendingTranArgs   ulsdEndPendingTranArgs;
typedef struct ulsdEndTranArgs          ulsdEndTranArgs;
typedef struct ulsdShardEndTranArgs     ulsdShardEndTranArgs;
typedef struct ulsdFuncCallback         ulsdFuncCallback;
typedef struct ulsdConnectAttrInfo      ulsdConnectAttrInfo;
typedef struct ulsdStmtAttrInfo         ulsdStmtAttrInfo;
typedef struct ulsdBindParameterInfo    ulsdBindParameterInfo;
typedef struct ulsdBindColInfo          ulsdBindColInfo;
typedef struct ulsdLobLocator           ulsdLobLocator;
typedef struct ulsdPutLobArgs           ulsdPutLobArgs;
typedef struct ulsdExecDirectArgs       ulsdExecDirectArgs;

struct ulsdDbc
{
    ulnDbc                 *mMetaDbc;       /* Meta Node Dbc */
    acp_bool_t              mTouched;

    acp_bool_t              mIsTestEnable;  /* Shard Test Enable */
    acp_uint16_t            mNodeCount;     /* Data Node Count */
    ulsdNodeInfo          **mNodeInfo;      /* Data Node Info Array */
};

struct ulsdNodeInfo
{
    /* connection info */
    acp_uint64_t            mSMN;
    acp_uint32_t            mNodeId;
    acp_char_t              mNodeName[ULSD_MAX_NODE_NAME_LEN+1];
    acp_char_t              mServerIP[ULSD_MAX_SERVER_IP_LEN];
    acp_uint16_t            mPortNo;
    acp_char_t              mAlternateServerIP[ULSD_MAX_SERVER_IP_LEN];
    acp_uint16_t            mAlternatePortNo;

    /* runtime info */
    ulnDbc                 *mNodeDbc;
    acp_bool_t              mTouched;
};

/* BUG-46257 shardcli에서 Node 추가/제거 지원 */
struct ulsdConnectAttrInfo
{
    acp_sint32_t    mAttribute;
    void          * mValuePtr;
    acp_sint32_t    mStringLength;

    acp_char_t    * mBufferForString;
    acp_list_node_t mListNode;
};

typedef enum ulsdReportType
{
    ULSD_REPORT_TYPE_CONNECTION           = 1,  /* CMP_DB_SHARD_NODE_CONNECTION_REPORT */
    ULSD_REPORT_TYPE_TRANSACTION_BROKEN   = 2   /* CMP_DB_SHARD_NODE_TRANSACTION_BROKEN_REPORT */
}ulsdReportType;

typedef struct ulsdNodeConnectReport
{
    acp_uint32_t            mNodeId;
    ulsdDataNodeConntectTo  mDestination;
} ulsdNodeConnectReport;

/* BUG-46092 */
struct ulsdNodeReport
{
    ulsdReportType            mType;
    union
    {
        ulsdNodeConnectReport   mConnectReport;
    } mArg;
};

/* BUG-46092 */
#define ULSD_ALIGN_INFO_MAX_TEXT_LENGTH     ( 2048 + 256 )  /* = ideErrorMgrStack.LastErrorMsg */
struct ulsdAlignInfo
{
    acp_bool_t          mIsNeedAlign;
    acp_char_t          mSQLSTATE[SQL_SQLSTATE_SIZE+1];
    acp_uint32_t        mNativeErrorCode;
    acp_char_t         *mMessageText;
    acp_sint32_t        mMessageTextAllocLength;    /* Max is ULSD_ALIGN_INFO_MAX_TEXT_LENGTH */
};

typedef enum
{
    ULSD_2PC_NORMAL         = 0,
    ULSD_2PC_COMMIT_FAIL    = 1,
} ulsd2PhaseCommitState;

/*
 * ulnDbc에서 샤드 처리에 관한 정보를 정의한 구조체
 */
struct ulsdDbcContext
{
    ulsdDbc            *mShardDbc;
    ulnDbc             *mParentDbc;     /* Meta Node Dbc for Data Node. Meta Node has NULL */
    ulsdNodeInfo       *mNodeInfo;      /* pointer of MetaDbc->mShardDbcCxt.mShardDbc->mNodeInfo[N] */

    acp_bool_t          mShardNeedNodeDbcRetryExecution;
    acp_bool_t          mShardIsNodeTransactionBegin;
    acp_uint16_t        mShardOnTransactionNodeIndex;

    acp_char_t          mShardTargetDataNodeName[ULSD_MAX_NODE_NAME_LEN + 1];
    acp_uint8_t         mShardLinkerType;
    acp_uint64_t        mShardPin;
    acp_uint64_t        mShardMetaNumber;
    acp_uint64_t        mSentShardMetaNumber;
    acp_uint64_t        mSentRebuildShardMetaNumber;
    acp_uint64_t        mTargetShardMetaNumber;

    /* BUG-45509 nested commit */
    ulsdFuncCallback   *mCallback;

    /* BUG-45411 */
    acp_bool_t          mReadOnlyTx;  /* shard_prepare protocol의 result */

    ulsdShardConnType   mShardConnType;

    /* BUG-45707 */
    acp_uint8_t         mShardClient;
    acp_uint8_t         mShardSessionType;

    /* BUG-46257 shardcli에서 Node 추가/제거 지원 */
    acp_char_t        * mOrgConnString;
    acp_list_t          mConnectAttrList;

    /* BUG-46092 */
    ulsdAlignInfo       mAlignInfo;

    ulsd2PhaseCommitState m2PhaseCommitState;

    /* PROJ-2733-DistTxInfo Meta DBC만 사용한다. */
    acp_uint16_t        mBeforeExecutedNodeDbcIndex;

    /* PROJ-2739 Client-side Sharding LOB */
    acp_thr_mutex_t     mLock4LocatorList;
    acp_list_t          mLobLocatorList;

    ulsdFuncCallback   *mFuncCallback;  /* BUG-46814 */
};

union ulsdValue
{
    /* hash shard */
    acp_uint32_t  mHashMax;

    /* range shard */
    acp_sint8_t   mMax[1];
    acp_sint16_t  mSmallintMax;
    acp_sint32_t  mIntegerMax;
    acp_sint64_t  mBigintMax;
    mtdCharType   mCharMax;
    acp_uint16_t  mCharMaxBuf[(2 + ULN_SHARD_KEY_MAX_CHAR_BUF_LEN + 1) / 2];

    /* bind param Id */
    acp_uint16_t  mBindParamId;
};

struct ulsdValueInfo
{
    acp_uint8_t     mType;
    acp_uint32_t    mDataValueType;
    ulsdValue       mValue;
};

struct ulsdRangeInfo
{
    ulsdValue    mShardRange;
    ulsdValue    mShardSubRange;
    acp_uint32_t mShardNodeID;
};

union ulsdKeyData
{
    acp_sint8_t   mValue[1];  /* 대표값 */
    acp_sint16_t  mSmallintValue;
    acp_sint32_t  mIntegerValue;
    acp_sint64_t  mBigintValue;
    mtdCharType   mCharValue;
    acp_uint16_t  mCharMaxBuf[(2 + ULN_SHARD_KEY_MAX_CHAR_BUF_LEN + 1) / 2];
};

/* PROJ-2638 shard native linker */
#define ULN_COLUMN_ID_MAXIMUM 1024
struct ulsdColumnMaxLenInfo
{
    acp_uint16_t mColumnCnt;
    acp_uint32_t mOffSet[ULN_COLUMN_ID_MAXIMUM];
    acp_uint32_t mMaxByte[ULN_COLUMN_ID_MAXIMUM];
};

/* BUG-46257 shardcli에서 Node 추가/제거 지원 */
struct ulsdStmtAttrInfo
{
    acp_sint32_t    mAttribute;
    void          * mValuePtr;
    acp_sint32_t    mStringLength;

    acp_list_node_t mListNode;
};

/* BUG-46257 shardcli에서 Node 추가/제거 지원 */
struct ulsdBindParameterInfo
{
    acp_uint16_t    mParameterNumber;   // 1부터 시작
    acp_sint16_t    mInputOutputType;   // SQL_PARAM_INPUT, SQL_PARAM_OUTPUT, SQL_PARAM_INPUT_OUTPUT
    acp_sint16_t    mValueType;         // SQL_C_CHAR, ...
    acp_sint16_t    mParameterType;     // SQL_CHAR, SQL_VARCHAR, ...
    ulvULen         mColumnSize;        // Precision
    acp_sint16_t    mDecimalDigits;     // Scale
    void          * mParameterValuePtr; // Buffer
    ulvSLen         mBufferLength;      // Buffer Max Length
    ulvSLen       * mStrLenOrIndPtr;    // Indicator

    /* PROJ-2739 Client-side Sharding LOB 
     *   For SQLBindFileToParam */
    ulvSLen       * mFileNameLengthArray;
    acp_uint32_t  * mFileOptionPtr;

    acp_list_node_t mListNode;
};

/* BUG-46257 shardcli에서 Node 추가/제거 지원 */
struct ulsdBindColInfo
{
    acp_uint16_t    mColumnNumber;      // 1부터 시작
    acp_sint16_t    mTargetType;        // SQL_C_CHAR, ...
    void          * mTargetValuePtr;    // Buffer or FileName
    ulvSLen         mBufferLength;      // Buffer Max Length
    ulvSLen       * mStrLenOrIndPtr;    // Indicator

    /* PROJ-2739 Client-side Sharding LOB
     *   For SQLBindFileToCol */
    ulvSLen       * mFileNameLengthArray;
    acp_uint32_t  * mFileOptionPtr;

    acp_list_node_t mListNode;
};

struct ulsdStmtContext
{
    ulnStmt               **mShardNodeStmt;

    /* PROJ-2598 Shard pilot(shard analyze) */
    acp_uint8_t             mShardSplitMethod;   /* 1:hash, 2:range, 3:list, 4:clone */
    acp_uint32_t            mShardKeyDataType;   /* mt data type of shard key column */
    acp_uint32_t            mShardDefaultNodeID; /* default node id */
    acp_uint16_t            mShardRangeInfoCnt;
    ulsdRangeInfo          *mShardRangeInfo;

    /* PROJ-2638 shard native linker */
    acp_bool_t              mIsMtDataFetch;
    acp_uint8_t            *mRowDataBuffer; /* Write buffer for mt-type row data. */
    acp_uint32_t            mRowDataBufLen;
    acp_char_t              mShardTargetDataNodeName[ULSD_MAX_NODE_NAME_LEN+1];
    ulsdColumnMaxLenInfo    mColumnOffset;

    /* PROJ-2646 New shard analyzer */
    acp_uint16_t            mShardValueCnt;
    ulsdValueInfo         **mShardValueInfoPtrArray;
    acp_bool_t              mShardIsShardQuery;

    /* PROJ-2655 Composite shard key */
    acp_bool_t           mShardIsSubKeyExists;
    acp_uint8_t          mShardSubSplitMethod;
    acp_uint32_t         mShardSubKeyDataType;
    acp_uint16_t         mShardSubValueCnt;
    ulsdValueInfo      **mShardSubValueInfoPtrArray;

    /* PROJ-2670 nested execution */
    ulsdFuncCallback    *mCallback;

    /* PROJ-2660 hybrid sharding */
    acp_bool_t           mShardCoordQuery;

    /* BUG-45499 result merger */
    acp_uint16_t         mNodeDbcIndexArr[ULSD_SD_NODE_MAX_COUNT];
    acp_uint16_t         mNodeDbcIndexCount;
    acp_sint16_t         mNodeDbcIndexCur;  /* 현재 fetch중인 dbc index */

    /* BUG-46100 Session SMN Update */
    acp_uint64_t         mShardMetaNumber;
    acp_char_t          *mOrgPrepareTextBuf;
    acp_sint32_t         mOrgPrepareTextBufMaxLen;
    acp_sint32_t         mOrgPrepareTextBufLen;

    /* BUG-46257 shardcli에서 Node 추가/제거 지원 */
    acp_list_t           mStmtAttrList;
    acp_list_t           mBindParameterList;
    acp_list_t           mBindColList;
    acp_sint64_t         mRowCount;

    /* PROJ-2739 Client-side Sharding LOB */
    ulnStmt             *mParentStmt; // meta-stmt of node stmt
    acp_sint16_t         mMyNodeDbcIndexCur; /* for node Stmt, 현재 수행중인 dbc index */
    /* C_LOCATOR로 INPUT 바인딩한 param이 있는지 여부 */
    acp_bool_t           mHasLocatorInBoundParam;
    /* C_LOCATOR로 INPUT 바인딩한 param이 OUTPUT으로 바뀐 것이 있는지 여부 */
    acp_bool_t           mHasLocatorParamToCopy;

    /* TASK-7219 Non-shard DML */
    ulsdShardPartialExecType mPartialExecType;
};

struct ulsdRangeIndex
{
    acp_uint16_t mRangeIndex;
    acp_uint16_t mValueIndex;
};

typedef enum
{
    ULSD_FUNC_PREPARE                  = 1,
    ULSD_FUNC_EXECUTE_FOR_MT_DATA_ROWS = 2,
    ULSD_FUNC_EXECUTE_FOR_MT_DATA      = 3,
    ULSD_FUNC_EXECUTE                  = 4,
    ULSD_FUNC_PREPARE_TRAN             = 5,
    ULSD_FUNC_END_PENDING_TRAN         = 6,
    ULSD_FUNC_END_TRAN                 = 7,
    ULSD_FUNC_SHARD_END_TRAN           = 8,
    ULSD_FUNC_PUT_LOB                  = 9,
    ULSD_FUNC_EXECUTE_DIRECT           = 10
} ulsdFuncType;

struct ulsdPrepareArgs
{
    acp_char_t   *mQuery;
    acp_sint32_t  mQueryLen;
};

struct ulsdExecuteArgs
{
    acp_char_t   *mOutBuf;
    acp_uint32_t  mOutBufLen;
    acp_uint32_t *mOffSets;
    acp_uint32_t *mMaxBytes;
    acp_uint16_t  mColumnCount;
};

struct ulsdPrepareTranArgs
{
    acp_uint32_t   mXIDSize;
    acp_uint8_t   *mXID;
    acp_uint8_t   *mReadOnly;
};

struct ulsdEndPendingTranArgs
{
    acp_uint32_t   mXIDSize;
    acp_uint8_t   *mXID;
    acp_sint16_t   mCompletionType;
};

struct ulsdEndTranArgs
{
    acp_sint16_t  mCompletionType;
};

struct ulsdShardEndTranArgs
{
    acp_sint16_t  mCompletionType;
};

struct ulsdExecDirectArgs
{
    acp_char_t   *mQuery;
    acp_sint32_t  mQueryLen;
};
/* PROJ-2739 Client-side Sharding LOB */
struct ulsdPutLobArgs
{
    acp_sint16_t  mLocatorCType;
    acp_uint64_t  mLocator;
    acp_uint32_t  mFromPosition;
    acp_uint32_t  mForLength;
    acp_sint16_t  mSourceCType;
    void         *mBuffer;
    acp_uint32_t  mBufferSize;
};

struct ulsdFuncCallback
{
    acp_bool_t    mInUse;  /* BUG-46814 */

    ulsdFuncType  mFuncType;

    acp_uint32_t  mCount;
    acp_uint32_t  mIndex;
    SQLRETURN     mRet;

    ulnStmt      *mStmt;
    ulnDbc       *mDbc;
    union
    {
        ulsdPrepareArgs        mPrepare;
        ulsdExecuteArgs        mExecute;
        ulsdPrepareTranArgs    mPrepareTran;
        ulsdEndTranArgs        mEndTran;
        ulsdShardEndTranArgs   mShardEndTran;
        ulsdEndPendingTranArgs mEndPendingTran;
        ulsdPutLobArgs         mPutLob; // PROJ-2739
        ulsdExecDirectArgs     mExecDirect;
    } mArg;

    struct ulsdFuncCallback *mNext;
};

#define ULSD_CALLBACK_DEPTH_MAX  10

/* PROJ-2739 Client-side Sharding LOB */
struct ulsdLobLocator
{
    acp_list_node_t mList;
    acp_uint64_t    mLobLocator;
    acp_sint16_t    mNodeDbcIndex;
    acp_uint16_t    mArraySize; /* == 수행 노드개수 */
    ulsdLobLocator *mDestLobLocators;
};

#endif /* _O_ULSD_DEF_H_ */
