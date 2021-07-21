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

#ifndef _O_CMP_ARG_RP_CLIENT_H_
#define _O_CMP_ARG_RP_CLIENT_H_ 1


typedef struct cmpArgRPVersion
{
    acp_uint64_t mVersion;           // Replication Version
} cmpArgRPVersion;

typedef struct cmpArgRPMetaRepl
{
    cmtVariable  mRepName;            // Replication Name(40byte)
    acp_sint32_t mItemCnt;            // Replication Table Count
    acp_sint32_t mRole;               // Role
    acp_sint32_t mConflictResolution; // Conflict resolution
    acp_uint32_t mTransTblSize;       // Transaction Table Size
    acp_uint32_t mFlags;              // Flags
    acp_uint32_t mOptions;            // Option Flags(Recovery 0x0000001)
    acp_uint64_t mRPRecoverySN;       // RP Recovery SN for recovery option1
    acp_uint32_t mReplMode;
    acp_uint32_t mParallelID;
    acp_sint32_t mIsStarted;

    /* PROJ-1915 */
    cmtVariable  mOSInfo;
    acp_uint32_t mCompileBit;
    acp_uint32_t mSmVersionID;
    acp_uint32_t mLFGCount;
    acp_uint64_t mLogFileSize;

    cmtVariable  mDBCharSet;         // BUG-23718
    cmtVariable  mNationalCharSet;
    cmtVariable  mServerID;          // BUG-6093 Server ID 추가
    cmtVariable  mRemoteFaultDetectTime;
} cmpArgRPMetaRepl;

//PROJ-1608 recovery from replication
typedef struct cmpArgRPRequestAck
{
    acp_uint32_t mResult;            //OK, NOK
} cmpArgRPRequestAck;

typedef struct cmpArgRPMetaReplTbl
{
    cmtVariable  mRepName;           // Replication Name(40byte)
    cmtVariable  mLocalUserName;     // Local User Name(40byte)
    cmtVariable  mLocalTableName;    // Local Table Name(40byte)
    cmtVariable  mLocalPartName;     // Local Part Name(40byte)
    cmtVariable  mRemoteUserName;    // Remote User Name(40byte)
    cmtVariable  mRemoteTableName;   // Remote Table Name(40byte)
    cmtVariable  mRemotePartName;    // Remote Table Name(40byte)
    cmtVariable  mPartCondMinValues; // Partition Condition Minimum Values(variable size)
    cmtVariable  mPartCondMaxValues; // Partition Condition Maximum Values(variable size)
    acp_uint32_t mPartitionMethod;   // Partition Method
    acp_uint32_t mPartitionOrder;    // Partition Order ( hash partitioned table only )
    acp_uint64_t mTableOID;          // Local Table OID
    acp_uint32_t mPKIndexID;         // Primary Key Index ID
    acp_uint32_t mPKColCnt;          // Primary Key Column Count
    acp_sint32_t mColumnCnt;         // Columnt count
    acp_sint32_t mIndexCnt;          // Index count
    /* PROJ-1915 Invalid Max SN 전송 */
    acp_uint64_t mInvalidMaxSN;
    cmtVariable  mConditionStr;
} cmpArgRPMetaReplTbl;

typedef struct cmpArgRPMetaReplCol
{
    cmtVariable  mColumnName;        // Column Name(40byte)
    acp_uint32_t mColumnID;          // Column ID
    acp_uint32_t mColumnFlag;        // Column Flag
    acp_uint32_t mColumnOffset;      // Fixed record에서의 column offset
    acp_uint32_t mColumnSize;        // Column Size
    acp_uint32_t mDataTypeID;        // Column의 Data type ID
    acp_uint32_t mLangID;            // Column의 Language ID
    acp_uint32_t mFlags;             // mtcColumn의 flag
    acp_sint32_t mPrecision;         // Column의 precision
    acp_sint32_t mScale;             // Column의 scale

    // PROJ-2002 Column Security
    // echar, evarchar 같은 보안 타입을 사용하는 경우
    // 아래 컬럼정보가 추가된다.
    acp_sint32_t mEncPrecision;      // 보안 타입의 precision
    cmtVariable  mPolicyName;        // 보안 타입에 사용된 policy name
    cmtVariable  mPolicyCode;        // 보안 타입에 사용된 policy code
    cmtVariable  mECCPolicyName;     // 보안 타입에 사용된 ecc policy name
    cmtVariable  mECCPolicyCode;     // 보안 타입에 사용된 ecc policy code
} cmpArgRPMetaReplCol;

typedef struct cmpArgRPMetaReplIdx
{
    cmtVariable  mIndexName;         // Index Name(40byte)
    acp_uint32_t mIndexID;           // Index ID
    acp_uint32_t mIndexTypeID;       // Index Type ID
    acp_uint32_t mKeyColumnCnt;      // Index를 구성하는 column 개수
    acp_uint32_t mIsUnique;          // Unique index 여부
    acp_uint32_t mIsRange;           // Range 여부
} cmpArgRPMetaReplIdx;

typedef struct cmpArgRPMetaReplIdxCol
{
    acp_uint32_t mColumnID;          // Index를 구성하는 column ID
    acp_uint32_t mKeyColumnFlag;     // Index의 sort 순서(ASC/DESC)
} cmpArgRPMetaReplIdxCol;

typedef struct cmpArgRPHandshakeAck
{
    acp_uint32_t mResult;            // 성공/실패 결과
    acp_sint32_t mFailbackStatus;
    acp_uint64_t mXSN;
    cmtVariable  mMsg;               // 실패 메세지(1024 byte)
} cmpArgRPHandshakeAck;

/* PROJ-1663에 의해 사용되지 않음 */
typedef struct cmpArgRPTrBegin
{
    acp_uint32_t mXLogType;          // RP_X_BEGIN
    acp_uint32_t mTransID;           // Transaction ID
    acp_uint64_t mSN;                // 로그의 SN
    acp_uint64_t mSyncSN;
} cmpArgRPTrBegin;

typedef struct cmpArgRPTrCommit
{
    acp_uint32_t mXLogType;          // RP_X_COMMIT
    acp_uint32_t mTransID;           // Transaction ID
    acp_uint64_t mSN;                // 로그의 SN
    acp_uint64_t mSyncSN;
} cmpArgRPTrCommit;

typedef struct cmpArgRPTrAbort
{
    acp_uint32_t mXLogType;          // RP_X_ABORT
    acp_uint32_t mTransID;           // Transaction ID
    acp_uint64_t mSN;                // 로그의 SN
    acp_uint64_t mSyncSN;
} cmpArgRPTrAbort;

typedef struct cmpArgRPSPSet
{
    acp_uint32_t mXLogType;          // RP_X_SP_SET
    acp_uint32_t mTransID;           // Transaction ID
    acp_uint64_t mSN;                // 로그의 SN
    acp_uint64_t mSyncSN;
    acp_uint32_t mSPNameLen;         // Savepoint Name Length
    cmtVariable  mSPName;            // Savepoint Name
} cmpArgRPSPSet;

typedef struct cmpArgRPSPAbort
{
    acp_uint32_t mXLogType;          // RP_X_SP_ABORT
    acp_uint32_t mTransID;           // Transaction ID
    acp_uint64_t mSN;                // 로그의 SN
    acp_uint64_t mSyncSN;
    acp_uint32_t mSPNameLen;         // Savepoint Name Length
    cmtVariable  mSPName;            // Savepoint Name
} cmpArgRPSPAbort;

typedef struct cmpArgRPStmtBegin
{
    acp_uint32_t mXLogType;          // RP_X_STMT_BEGIN
    acp_uint32_t mTransID;           // Transaction ID
    acp_uint64_t mSN;                // 로그의 SN
    acp_uint64_t mSyncSN;
    acp_uint32_t mStmtID;            // Statement ID
} cmpArgRPStmtBegin;

typedef struct cmpArgRPStmtEnd
{
    acp_uint32_t mXLogType;          // RP_X_STMT_END
    acp_uint32_t mTransID;           // Transaction ID
    acp_uint64_t mSN;                // 로그의 SN
    acp_uint64_t mSyncSN;
    acp_uint32_t mStmtID;            // Statement ID
} cmpArgRPStmtEnd;

typedef struct cmpArgRPCursorOpen
{
    acp_uint32_t mXLogType;          // RP_X_CURSOR_OPEN
    acp_uint32_t mTransID;           // Transaction ID
    acp_uint64_t mSN;                // 로그의 SN
    acp_uint64_t mSyncSN;
    acp_uint32_t mCursorID;          // Cursor ID
} cmpArgRPCursorOpen;

typedef struct cmpArgRPCursorClose
{
    acp_uint32_t mXLogType;          // RP_X_CURSOR_CLOSE
    acp_uint32_t mTransID;           // Transaction ID
    acp_uint64_t mSN;                // 로그의 SN
    acp_uint64_t mSyncSN;
    acp_uint32_t mCursorID;          // Cursor ID
} cmpArgRPCursorClose;

typedef struct cmpArgRPInsert
{
    acp_uint32_t mXLogType;          // RP_X_INSERT
    acp_uint32_t mTransID;           // Transaction ID
    acp_uint64_t mSN;                // 로그의 SN
    acp_uint64_t mSyncSN;
    acp_uint32_t mImplSPDepth;
    acp_uint64_t mTableOID;          // Insert 대상 Table OID
    acp_uint32_t mColCnt;            // Insert Column Count
} cmpArgRPInsert;

typedef struct cmpArgRPUpdate
{
    acp_uint32_t mXLogType;          // RP_X_UPDATE
    acp_uint32_t mTransID;           // Transaction ID
    acp_uint64_t mSN;                // 로그의 SN
    acp_uint64_t mSyncSN;
    acp_uint32_t mImplSPDepth;
    acp_uint64_t mTableOID;          // Update 대상 Table OID
    acp_uint32_t mPKColCnt;          // Primary Key Column Count
    acp_uint32_t mUpdateColCnt;      // Update가 발생한 Column 개수
} cmpArgRPUpdate;

typedef struct cmpArgRPDelete
{
    acp_uint32_t mXLogType;          // RP_X_DELETE
    acp_uint32_t mTransID;           // Transaction ID
    acp_uint64_t mSN;                // 로그의 SN
    acp_uint64_t mSyncSN;
    acp_uint32_t mImplSPDepth;
    acp_uint64_t mTableOID;          // Delete 대상 Table OID
    acp_uint32_t mPKColCnt;          // Primary Key Column Count
} cmpArgRPDelete;

typedef struct cmpArgRPUIntID
{
    acp_uint32_t mUIntID;            // Column ID : array로 사용위함
} cmpArgRPUIntID;

typedef struct cmpArgRPValue
{
    cmtVariable  mValue;             // 실제 Value
} cmpArgRPValue;

typedef struct cmpArgRPStop
{
    acp_uint32_t mXLogType;          // RP_X_REPL_STOP
    acp_uint32_t mTransID;           // Transaction ID
    acp_uint64_t mSN;                // 로그의 SN
    acp_uint64_t mSyncSN;
    acp_uint64_t mRestartSN;         // BUG-17748 : Restart SN
} cmpArgRPStop;

typedef struct cmpArgRPKeepAlive
{
    acp_uint32_t mXLogType;          // RP_X_KEEP_ALIVE
    acp_uint32_t mTransID;           // Transaction ID
    acp_uint64_t mSN;                // 로그의 SN
    acp_uint64_t mSyncSN;
    acp_uint64_t mRestartSN;         // BUG-17748 : Restart SN
} cmpArgRPKeepAlive;

typedef struct cmpArgRPFlush
{
    acp_uint32_t mXLogType;          // RP_X_FLUSH
    acp_uint32_t mTransID;           // Transaction ID
    acp_uint64_t mSN;                // 로그의 SN
    acp_uint64_t mSyncSN;
    acp_uint32_t mOption;            // Flush Option
} cmpArgRPFlush;

typedef struct cmpArgRPFlushAck
{
    acp_uint32_t mXLogType;          // RP_X_FLUSH_ACK
    acp_uint32_t mTransID;           // Transaction ID
    acp_uint64_t mSN;                // 로그의 SN
    acp_uint64_t mSyncSN;
} cmpArgRPFlushAck;

typedef struct cmpArgRPAck
{
    acp_uint32_t mXLogType;          // RP_X_ACK, RP_X_STOP_ACK, RP_X_HANDSHAKE_READY
    acp_uint32_t mAbortTxCount;      // Abort Transaction Count
    acp_uint32_t mClearTxCount;      // Clear Transaction Count
    acp_uint64_t mRestartSN;         // Transaction Table's Minimum SN
    acp_uint64_t mLastCommitSN;      // 마지막으로 Commit한 로그의 SN
    acp_uint64_t mLastArrivedSN;     // Receiver가 수신한 로그의 마지막 SN (Acked Mode)
    acp_uint64_t mLastProcessedSN;   // 반영을 완료한 마지막 로그의 SN     (Eager Mode)
    acp_uint64_t mFlushSN;           // 디스크에 Flush된 SN
    //cmpArgRPTxAck를 이용해 Abort Transaction List를 수신
    //cmpArgRPTxAck를 이용해 Clear Transaction List를 수신
} cmpArgRPAck;

typedef struct cmpArgRPTxAck
{
    acp_uint32_t mTID;               // Transaction ID
    acp_uint64_t mSN;                // XLog의 SN

} cmpArgRPTxAck;

typedef struct cmpArgRPLobCursorOpen
{
    acp_uint32_t mXLogType;          // RP_X_LOB_CURSOR_OPEN
    acp_uint32_t mTransID;           // Transaction ID
    acp_uint64_t mSN;                // 로그의 SN
    acp_uint64_t mSyncSN;
    acp_uint64_t mTableOID;          // LOB cursor open 대상 Table OID
    acp_uint64_t mLobLocator;        // LOB locator : 식별자로 사용
    acp_uint32_t mColumnID;          // LOB column ID
    acp_uint32_t mPKColCnt;
} cmpArgRPLobCursorOpen;

typedef struct cmpArgRPLobCursorClose
{
    acp_uint32_t mXLogType;          // RP_X_LOB_CURSOR_CLOSE
    acp_uint32_t mTransID;           // Transaction ID
    acp_uint64_t mSN;                // 로그의 SN
    acp_uint64_t mSyncSN;
    acp_uint64_t mLobLocator;        // Close할 LOB locator
} cmpArgRPLobCursorClose;

typedef struct cmpArgRPLobPrepare4Write
{
    acp_uint32_t mXLogType;          // RP_X_PREPARE4WRITE
    acp_uint32_t mTransID;           // Transaction ID
    acp_uint64_t mSN;                // 로그의 SN
    acp_uint64_t mSyncSN;
    acp_uint64_t mLobLocator;        // Write할 LOB locator
    acp_uint32_t mOffset;            // 변경 시작 offset
    acp_uint32_t mOldSize;           // 변경 전 size
    acp_uint32_t mNewSize;           // 변경 후 size
} cmpArgRPLobPrepare4Write;

typedef struct cmpArgRPLobPartialWrite
{
    acp_uint32_t mXLogType;          // RP_X_LOB_PARTIAL_WRITE
    acp_uint32_t mTransID;           // Transaction ID
    acp_uint64_t mSN;                // 로그의 SN
    acp_uint64_t mSyncSN;
    acp_uint64_t mLobLocator;        // Write할 LOB locator
    acp_uint32_t mOffset;            // Piece를 Write할 LOB 내의 위치
    acp_uint32_t mPieceLen;          // Piece의 길이
    cmtVariable  mPieceValue;        // Piece의 value
} cmpArgRPLobPartialWrite;

typedef struct cmpArgRPLobFinish2Write
{
    acp_uint32_t mXLogType;          // RP_X_LOB_FINISH2WRITE
    acp_uint32_t mTransID;           // Transaction ID
    acp_uint64_t mSN;                // 로그의 SN
    acp_uint64_t mSyncSN;
    acp_uint64_t mLobLocator;        // Write를 끝낼 LOB locator
} cmpArgRPLobFinish2Write;

typedef struct cmpArgRPLobTrim
{
    acp_uint32_t mXLogType;          // RP_X_LOB_TRIM
    acp_uint32_t mTransID;           // Transaction ID
    acp_uint64_t mSN;                // 로그의 SN
    acp_uint64_t mSyncSN;
    acp_uint64_t mLobLocator;        // Trim할 LOB locator
    acp_uint32_t mOffset;            // Trim offset
} cmpArgRPLobTrim;

typedef struct cmpArgRPHandshake
{
    acp_uint32_t mXLogType;          // RP_X_HANDSHAKE
    acp_uint32_t mTransID;           // Transaction ID
    acp_uint64_t mSN;                // 로그의 SN
    acp_uint64_t mSyncSN;
} cmpArgRPHandshake;

typedef struct cmpArgRPSyncPKBegin
{
    acp_uint32_t mXLogType;          // RP_X_SYNC_PK_BEGIN
    acp_uint32_t mTransID;           // Transaction ID
    acp_uint64_t mSN;                // 로그의 SN
    acp_uint64_t mSyncSN;
} cmpArgRPSyncPKBegin;

typedef struct cmpArgRPSyncPK
{
    acp_uint32_t mXLogType;          // RP_X_SYNC_PK
    acp_uint32_t mTransID;           // Transaction ID
    acp_uint64_t mSN;                // 로그의 SN
    acp_uint64_t mSyncSN;
    acp_uint64_t mTableOID;          // Delete 대상 Table OID
    acp_uint32_t mPKColCnt;          // Primary Key Column Count
} cmpArgRPSyncPK;

typedef struct cmpArgRPSyncPKEnd
{
    acp_uint32_t mXLogType;          // RP_X_SYNC_PK_END
    acp_uint32_t mTransID;           // Transaction ID
    acp_uint64_t mSN;                // 로그의 SN
    acp_uint64_t mSyncSN;
} cmpArgRPSyncPKEnd;

typedef struct cmpArgRPFailbackEnd
{
    acp_uint32_t mXLogType;          // RP_X_FAILBACK_END
    acp_uint32_t mTransID;           // Transaction ID
    acp_uint64_t mSN;                // 로그의 SN
    acp_uint64_t mSyncSN;
} cmpArgRPFailbackEnd;

typedef struct cmpArgRPSyncTableNumber
{
    acp_uint32_t mSyncTableNumber;
} cmpArgRPSyncTableNumber;

typedef struct cmpArgRPSyncStart
{
    acp_uint32_t mXLogType;          /* RP_X_DROP_INDEX */
    acp_uint32_t mTransID;           /* Transaction ID  */
    acp_uint64_t mSN;                /* 로그의 SN      */
} cmpArgRPSyncStart;

typedef struct cmpArgRPSyncEnd
{
    acp_uint32_t mXLogType;          /* RP_X_DROP_INDEX */
    acp_uint32_t mTransID;           /* Transaction ID  */
    acp_uint64_t mSN;                /* 로그의 SN      */
} cmpArgRPSyncEnd;

typedef union cmpArgRP
{
    cmpArgRPVersion          mVersion;
    cmpArgRPMetaRepl         mMetaRepl;
    cmpArgRPMetaReplTbl      mMetaReplTbl;
    cmpArgRPMetaReplCol      mMetaReplCol;
    cmpArgRPMetaReplIdx      mMetaReplIdx;
    cmpArgRPMetaReplIdxCol   mMetaReplIdxCol;
    cmpArgRPHandshakeAck     mHandshakeAck;
    cmpArgRPTrBegin          mTrBegin;
    cmpArgRPTrCommit         mTrCommit;
    cmpArgRPTrAbort          mTrAbort;
    cmpArgRPSPSet            mSPSet;
    cmpArgRPSPAbort          mSPAbort;
    cmpArgRPStmtBegin        mStmtBegin;
    cmpArgRPStmtEnd          mStmtEnd;
    cmpArgRPCursorOpen       mCursorOpen;
    cmpArgRPCursorClose      mCursorClose;
    cmpArgRPInsert           mInsert;
    cmpArgRPUpdate           mUpdate;
    cmpArgRPDelete           mDelete;
    cmpArgRPUIntID           mUIntID;
    cmpArgRPValue            mValue;
    cmpArgRPStop             mStop;
    cmpArgRPKeepAlive        mKeepAlive;
    cmpArgRPFlush            mFlush;
    cmpArgRPFlushAck         mFlushAck;
    cmpArgRPAck              mAck;
    cmpArgRPLobCursorOpen    mLobCursorOpen;
    cmpArgRPLobCursorClose   mLobCursorClose;
    cmpArgRPLobPrepare4Write mLobPrepare4Write;
    cmpArgRPLobPartialWrite  mLobPartialWrite;
    cmpArgRPLobFinish2Write  mLobFinish2Write;
    cmpArgRPTxAck            mTxAck;
    cmpArgRPRequestAck       mRequestAck;
    cmpArgRPHandshake        mHandshake;
    cmpArgRPSyncPKBegin      mSyncPKBegin;
    cmpArgRPSyncPK           mSyncPK;
    cmpArgRPSyncPKEnd        mSyncPKEnd;
    cmpArgRPFailbackEnd      mFailbackEnd;
    cmpArgRPSyncTableNumber  mSyncTableNumber;
    cmpArgRPSyncStart        mSyncStart;
    cmpArgRPSyncEnd          mSyncEnd;
    cmpArgRPLobTrim          mLobTrim;
} cmpArgRP;


#endif
