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
 * $id$
 **********************************************************************/

#ifndef _O_DKT_DEF_H_
#define _O_DKT_DEF_H_ 1

#include <idTypes.h>
#include <dkDef.h>
#include <qci.h>
#include <sdi.h>


#define DKT_SQL_ERROR_STATE_LEN                 (5)

#define DKT_REMOTE_STMT_ID_XOR_MASK             (0xFFFF)
#define DKT_REMOTE_STMT_ID_END                  (0x7FFF)
#define DKT_REMOTE_STMT_ID_BEGIN                (0x0001)

#define DKT_FETCH_ROW_COUNT_FOR_LARGE_RECORD    (10)
#define DKT_FETCH_ROW_COUNT_FOR_SMALL_RECORD    (100)

#define DKT_2PC_MAXGTRIDSIZE                    (18)     /* maximum size in bytes of gtrid */
#define DKT_2PC_MAXBQUALSIZE                    (4)      /* maximum size in bytes of bqual */
#define DKT_2PC_XIDDATASIZE                     (DKT_2PC_MAXGTRIDSIZE + DKT_2PC_MAXBQUALSIZE)      /* size in bytes */

#define DKT_2PC_XID_STRING_LEN                  (256)    /* XID_DATA_MAX_LEN 참조 */

/* -------------------------------------------------------------
 * Atomic transaction levels
 * ------------------------------------------------------------*/
typedef enum 
{
    DKT_ADLP_REMOTE_STMT_EXECUTION = 0,
    DKT_ADLP_SIMPLE_TRANSACTION_COMMIT,
    DKT_ADLP_TWO_PHASE_COMMIT,
    DKT_ADLP_GCTX,
    DKT_ADLP_MAX,
} dktAtomicTxLevel;

/* -------------------------------------------------------------
 * Global Transaction's status  
 * ------------------------------------------------------------*/
typedef enum
{
    DKT_GTX_STATUS_NON = 0,
    DKT_GTX_STATUS_BEGIN,
    DKT_GTX_STATUS_PREPARE_READY,
    DKT_GTX_STATUS_PREPARE_REQUEST,
    DKT_GTX_STATUS_PREPARE_WAIT,
    DKT_GTX_STATUS_PREPARED,
    DKT_GTX_STATUS_COMMIT_REQUEST,
    DKT_GTX_STATUS_COMMIT_WAIT,
    DKT_GTX_STATUS_COMMITTED,
    DKT_GTX_STATUS_ROLLBACK_REQUEST,
    DKT_GTX_STATUS_ROLLBACK_WAIT,
    DKT_GTX_STATUS_ROLLBACKED
} dktGTxStatus;

/* -------------------------------------------------------------
 * Remote Transaction's status  
 * ------------------------------------------------------------*/
typedef enum
{
    DKT_RTX_STATUS_NON = 0,
    DKT_RTX_STATUS_BEGIN,
    DKT_RTX_STATUS_PREPARE_READY,
    DKT_RTX_STATUS_PREPARE_WAIT,
    DKT_RTX_STATUS_PREPARED,
    DKT_RTX_STATUS_COMMIT_WAIT,
    DKT_RTX_STATUS_COMMITTED,
    DKT_RTX_STATUS_ROLLBACK_WAIT,
    DKT_RTX_STATUS_ROLLBACKED 
} dktRTxStatus;

/* -------------------------------------------------------------
 * Remote Statement types 
 * ------------------------------------------------------------*/
typedef enum
{
    DKT_STMT_TYPE_REMOTE_TABLE = 0,
    DKT_STMT_TYPE_REMOTE_EXECUTE_IMMEDIATE,
    DKT_STMT_TYPE_REMOTE_EXECUTE_QUERY_STATEMENT,
    DKT_STMT_TYPE_REMOTE_EXECUTE_NON_QUERY_STATEMENT,
    DKT_STMT_TYPE_REMOTE_EXECUTE_STATEMENT,
    DKT_STMT_TYPE_REMOTE_TABLE_STORE,
    DKT_STMT_TYPE_REMOTE_EXECUTE_STATEMENT_BATCH,
    DKT_STMT_TYPE_NONE = UINT_MAX
} dktStmtType;

/* linker type */
typedef enum
{
    DKT_LINKER_TYPE_NONE,
    DKT_LINKER_TYPE_DBLINK,
    DKT_LINKER_TYPE_SHARD
} dktLinkerType;

/* Define global transaction information for performance view */
typedef struct dktGlobalTxInfo
{
    UInt                mGlobalTxId;
    UInt                mLocalTxId;
    UInt                mStatus;
    UInt                mSessionId;
    UInt                mRemoteTxCnt;
    UInt                mAtomicTxLevel;
} dktGlobalTxInfo;

/* Define remote transaction information for performance view */
typedef struct dktRemoteTxInfo
{
    UInt                mGlobalTxId;
    UInt                mLocalTxId;
    UInt                mRTxId;
    ID_XID              mXID;
    SChar               mTargetInfo[DK_NAME_LEN + 1];
    UInt                mStatus;
} dktRemoteTxInfo;

/* Define remote transaction information for performance view */
typedef struct dktRemoteStmtInfo
{
    UInt                mGlobalTxId;
    UInt                mLocalTxId;
    UInt                mRTxId;
    SLong               mStmtId;
    SChar               mStmtStr[DK_MAX_SQLTEXT_LEN];
} dktRemoteStmtInfo;

/* Savepoint */
typedef struct dktSavepoint
{
    SChar               mName[DK_NAME_LEN + 1];
    iduListNode         mNode;
} dktSavepoint;

/* Define error information */
typedef struct dktErrorInfo
{
    /* vendor specific error code */
    UInt                mErrorCode;         
    /* sql error state */ 
    SChar               mSQLState[DKT_SQL_ERROR_STATE_LEN + 1];
    /* vendor specific error description */
    SChar             * mErrorDesc;
} dktErrorInfo;

typedef UChar dktCoordinatorType;

typedef struct dktShardNodeInfo
{
    // 접속정보
    SChar     mNodeName[SDI_NODE_NAME_MAX_SIZE + 1];
    SChar     mUserName[QCI_MAX_OBJECT_NAME_LEN + 1];
    SChar     mUserPassword[IDS_MAX_PASSWORD_LEN + 1];
    SChar     mServerIP[SDI_SERVER_IP_SIZE];
    UShort    mPortNo;
    UShort    mConnectType;
    UChar     mCoordinatorType;
} dktShardNodeInfo;

/* PROJ-2569 */
typedef struct dktDtxBranchTxInfo
{
    ID_XID       mXID;
    SChar        mLinkerType;     /* 'D':dblink, 'S':shard node */
    union
    {
        SChar            mTargetName[DK_NAME_LEN + 1];
        dktShardNodeInfo mNode;   /* for shard */
    } mData;
    
    idBool       mIsValid;
    iduListNode  mNode;
} dktDtxBranchTxInfo;

typedef struct dktNotifierTransactionInfo
{
    UInt   mGlobalTransactionId;
    UInt   mLocalTransactionId;
    ID_XID mXID;
    ID_XID mGlobalXID;
    UInt   mIsRequestNode;
    SChar  mTransactionResult[DK_TX_RESULT_STR_SIZE];
    SChar  mTargetInfo[DK_NAME_LEN + 1];
    ID_XID mSourceXID;
    UInt   mTransactionState;
    smSCN  mGlobalCommitSCN;
} dktNotifierTransactionInfo;

#endif /* _O_DKT_DEF_H_ */
