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

#ifndef _O_CMP_ARG_RP_H_
#define _O_CMP_ARG_RP_H_ 1


typedef struct cmpArgRPVersion
{
    ULong       mVersion;           // Replication Version
} cmpArgRPVersion;

typedef struct cmpArgRPMetaRepl
{
    cmtVariable mRepName;            // Replication Name(40byte)
    SInt        mItemCnt;            // Replication Table Count
    SInt        mRole;               // Role
    SInt        mConflictResolution; // Conflict resolution
    UInt        mTransTblSize;       // Transaction Table Size
    UInt        mFlags;              // Flags
    UInt        mOptions;            // Option Flags(Recovery 0x0000001)
    ULong       mRPRecoverySN;       // RP Recovery SN for recovery option1
    UInt        mReplMode;
    UInt        mParallelID;
    SInt        mIsStarted;

    /* PROJ-1915 */
    cmtVariable mOSInfo;
    UInt        mCompileBit;
    UInt        mSmVersionID;
    UInt        mLFGCount;
    ULong       mLogFileSize;

    cmtVariable mDBCharSet;         // BUG-23718
    cmtVariable mNationalCharSet;
    cmtVariable mServerID;          // BUG-6093 Server ID 추가
    cmtVariable mRemoteFaultDetectTime;
} cmpArgRPMetaRepl;

//PROJ-1608 recovery from replication
typedef struct cmpArgRPRequestAck
{
    UInt        mResult;            //OK, NOK
} cmpArgRPRequestAck;

typedef struct cmpArgRPMetaReplTbl
{
    cmtVariable mRepName;           // Replication Name(40byte)
    cmtVariable mLocalUserName;     // Local User Name(40byte)
    cmtVariable mLocalTableName;    // Local Table Name(40byte)
    cmtVariable mLocalPartName;     // Local Part Name(40byte)
    cmtVariable mRemoteUserName;    // Remote User Name(40byte)
    cmtVariable mRemoteTableName;   // Remote Table Name(40byte)
    cmtVariable mRemotePartName;    // Remote Table Name(40byte)
    cmtVariable mPartCondMinValues; // Partition Condition Minimum Values(variable size)
    cmtVariable mPartCondMaxValues; // Partition Condition Maximum Values(variable size)
    UInt        mPartitionMethod;   // Partition Method
    UInt        mPartitionOrder;    // Partition Order ( hash partitioned table only )
    ULong       mTableOID;          // Local Table OID
    UInt        mPKIndexID;         // Primary Key Index ID
    UInt        mPKColCnt;          // Primary Key Column Count
    SInt        mColumnCnt;         // Columnt count
    SInt        mIndexCnt;          // Index count
    /* PROJ-1915 Invalid Max SN 전송 */
    ULong       mInvalidMaxSN;
    cmtVariable mConditionStr;
} cmpArgRPMetaReplTbl;

typedef struct cmpArgRPMetaReplCol
{
    cmtVariable mColumnName;        // Column Name(40byte)
    UInt        mColumnID;          // Column ID
    UInt        mColumnFlag;        // Column Flag
    UInt        mColumnOffset;      // Fixed record에서의 column offset
    UInt        mColumnSize;        // Column Size
    UInt        mDataTypeID;        // Column의 Data type ID
    UInt        mLangID;            // Column의 Language ID
    UInt        mFlags;             // mtcColumn의 flag
    SInt        mPrecision;         // Column의 precision
    SInt        mScale;             // Column의 scale

    // PROJ-2002 Column Security
    // echar, evarchar 같은 보안 타입을 사용하는 경우
    // 아래 컬럼정보가 추가된다.
    SInt        mEncPrecision;      // 보안 타입의 precision
    cmtVariable mPolicyName;        // 보안 타입에 사용된 policy name
    cmtVariable mPolicyCode;        // 보안 타입에 사용된 policy code
    cmtVariable mECCPolicyName;     // 보안 타입에 사용된 ecc policy name
    cmtVariable mECCPolicyCode;     // 보안 타입에 사용된 ecc policy code
} cmpArgRPMetaReplCol;

typedef struct cmpArgRPMetaReplIdx
{
    cmtVariable mIndexName;         // Index Name(40byte)
    UInt        mIndexID;           // Index ID
    UInt        mIndexTypeID;       // Index Type ID
    UInt        mKeyColumnCnt;      // Index를 구성하는 column 개수
    UInt        mIsUnique;          // Unique index 여부
    UInt        mIsRange;           // Range 여부
} cmpArgRPMetaReplIdx;

typedef struct cmpArgRPMetaReplIdxCol
{
    UInt        mColumnID;          // Index를 구성하는 column ID
    UInt        mKeyColumnFlag;     // Index의 sort 순서(ASC/DESC)
} cmpArgRPMetaReplIdxCol;

typedef struct cmpArgRPHandshakeAck
{
    UInt        mResult;            // 성공/실패 결과
    SInt        mFailbackStatus;
    ULong       mXSN;               // restart SN
    cmtVariable mMsg;               // 실패 메세지(1024 byte)
} cmpArgRPHandshakeAck;

/* PROJ-1663에 의해 사용되지 않음 */
typedef struct cmpArgRPTrBegin
{
    UInt        mXLogType;          // RP_X_BEGIN
    UInt        mTransID;           // Transaction ID
    ULong       mSN;                // 로그의 SN
    ULong       mSyncSN;
} cmpArgRPTrBegin;

typedef struct cmpArgRPTrCommit
{
    UInt        mXLogType;          // RP_X_COMMIT
    UInt        mTransID;           // Transaction ID
    ULong       mSN;                // 로그의 SN
    ULong       mSyncSN;
} cmpArgRPTrCommit;

typedef struct cmpArgRPTrAbort
{
    UInt        mXLogType;          // RP_X_ABORT
    UInt        mTransID;           // Transaction ID
    ULong       mSN;                // 로그의 SN
    ULong       mSyncSN;
} cmpArgRPTrAbort;

typedef struct cmpArgRPSPSet
{
    UInt        mXLogType;          // RP_X_SP_SET
    UInt        mTransID;           // Transaction ID
    ULong       mSN;                // 로그의 SN
    ULong       mSyncSN;
    UInt        mSPNameLen;         // Savepoint Name Length
    cmtVariable mSPName;            // Savepoint Name
} cmpArgRPSPSet;

typedef struct cmpArgRPSPAbort
{
    UInt        mXLogType;          // RP_X_SP_ABORT
    UInt        mTransID;           // Transaction ID
    ULong       mSN;                // 로그의 SN
    ULong       mSyncSN;
    UInt        mSPNameLen;         // Savepoint Name Length
    cmtVariable mSPName;            // Savepoint Name
} cmpArgRPSPAbort;

typedef struct cmpArgRPStmtBegin
{
    UInt        mXLogType;          // RP_X_STMT_BEGIN
    UInt        mTransID;           // Transaction ID
    ULong       mSN;                // 로그의 SN
    ULong       mSyncSN;
    UInt        mStmtID;            // Statement ID
} cmpArgRPStmtBegin;

typedef struct cmpArgRPStmtEnd
{
    UInt        mXLogType;          // RP_X_STMT_END
    UInt        mTransID;           // Transaction ID
    ULong       mSN;                // 로그의 SN
    ULong       mSyncSN;
    UInt        mStmtID;            // Statement ID
} cmpArgRPStmtEnd;

typedef struct cmpArgRPCursorOpen
{
    UInt        mXLogType;          // RP_X_CURSOR_OPEN
    UInt        mTransID;           // Transaction ID
    ULong       mSN;                // 로그의 SN
    ULong       mSyncSN;
    UInt        mCursorID;          // Cursor ID
} cmpArgRPCursorOpen;

typedef struct cmpArgRPCursorClose
{
    UInt        mXLogType;          // RP_X_CURSOR_CLOSE
    UInt        mTransID;           // Transaction ID
    ULong       mSN;                // 로그의 SN
    ULong       mSyncSN;
    UInt        mCursorID;          // Cursor ID
} cmpArgRPCursorClose;

typedef struct cmpArgRPInsert
{
    UInt        mXLogType;          // RP_X_INSERT
    UInt        mTransID;           // Transaction ID
    ULong       mSN;                // 로그의 SN
    ULong       mSyncSN;
    UInt        mImplSPDepth;
    ULong       mTableOID;          // Insert 대상 Table OID
    UInt        mColCnt;            // Insert Column Count
} cmpArgRPInsert;

typedef struct cmpArgRPUpdate
{
    UInt        mXLogType;          // RP_X_UPDATE
    UInt        mTransID;           // Transaction ID
    ULong       mSN;                // 로그의 SN
    ULong       mSyncSN;
    UInt        mImplSPDepth;
    ULong       mTableOID;          // Update 대상 Table OID
    UInt        mPKColCnt;          // Primary Key Column Count
    UInt        mUpdateColCnt;      // Update가 발생한 Column 개수
} cmpArgRPUpdate;

typedef struct cmpArgRPDelete
{
    UInt        mXLogType;          // RP_X_DELETE
    UInt        mTransID;           // Transaction ID
    ULong       mSN;                // 로그의 SN
    ULong       mSyncSN;
    UInt        mImplSPDepth;
    ULong       mTableOID;          // Delete 대상 Table OID
    UInt        mPKColCnt;          // Primary Key Column Count
} cmpArgRPDelete;

typedef struct cmpArgRPUIntID
{
    UInt        mUIntID;            // Column ID : array로 사용위함
} cmpArgRPUIntID;

typedef struct cmpArgRPValue
{
    cmtVariable mValue;             // 실제 Value
} cmpArgRPValue;

typedef struct cmpArgRPStop
{
    UInt        mXLogType;          // RP_X_REPL_STOP
    UInt        mTransID;           // Transaction ID
    ULong       mSN;                // 로그의 SN
    ULong       mSyncSN;
    ULong       mRestartSN;         // BUG-17748 : Restart SN
} cmpArgRPStop;

typedef struct cmpArgRPKeepAlive
{
    UInt        mXLogType;          // RP_X_KEEP_ALIVE
    UInt        mTransID;           // Transaction ID
    ULong       mSN;                // 로그의 SN
    ULong       mSyncSN;
    ULong       mRestartSN;         // BUG-17748 : Restart SN
} cmpArgRPKeepAlive;

typedef struct cmpArgRPFlush
{
    UInt        mXLogType;          // RP_X_FLUSH
    UInt        mTransID;           // Transaction ID
    ULong       mSN;                // 로그의 SN
    ULong       mSyncSN;
    UInt        mOption;            // Flush Option
} cmpArgRPFlush;

typedef struct cmpArgRPFlushAck
{
    UInt        mXLogType;          // RP_X_FLUSH_ACK
    UInt        mTransID;           // Transaction ID
    ULong       mSN;                // 로그의 SN
    ULong       mSyncSN;
} cmpArgRPFlushAck;

typedef struct cmpArgRPAck
{
    UInt        mXLogType;          // RP_X_ACK, RP_X_STOP_ACK, RP_X_HANDSHAKE_READY
    UInt        mAbortTxCount;      // Abort Transaction Count
    UInt        mClearTxCount;      // Clear Transaction Count
    ULong       mRestartSN;         // Transaction Table's Minimum SN
    ULong       mLastCommitSN;      // 마지막으로 Commit한 로그의 SN
    ULong       mLastArrivedSN;     // Receiver가 수신한 로그의 마지막 SN (Acked Mode)
    ULong       mLastProcessedSN;   // 반영을 완료한 마지막 로그의 SN     (Eager Mode)
    ULong       mFlushSN;           // 디스크에 Flush된 SN
    //cmpArgRPTxAck를 이용해 Abort Transaction List를 수신
    //cmpArgRPTxAck를 이용해 Clear Transaction List를 수신
} cmpArgRPAck;

typedef struct cmpArgRPTxAck
{
    UInt        mTID;               // Transaction ID
    ULong       mSN;                // XLog의 SN
} cmpArgRPTxAck;

typedef struct cmpArgRPLobCursorOpen
{
    UInt        mXLogType;          // RP_X_LOB_CURSOR_OPEN
    UInt        mTransID;           // Transaction ID
    ULong       mSN;                // 로그의 SN
    ULong       mSyncSN;
    ULong       mTableOID;          // LOB cursor open 대상 Table OID
    ULong       mLobLocator;        // LOB locator : 식별자로 사용
    UInt        mColumnID;          // LOB column ID
    UInt        mPKColCnt;
} cmpArgRPLobCursorOpen;

typedef struct cmpArgRPLobCursorClose
{
    UInt        mXLogType;          // RP_X_LOB_CURSOR_CLOSE
    UInt        mTransID;           // Transaction ID
    ULong       mSN;                // 로그의 SN
    ULong       mSyncSN;
    ULong       mLobLocator;        // Close할 LOB locator
} cmpArgRPLobCursorClose;

typedef struct cmpArgRPLobPrepare4Write
{
    UInt        mXLogType;          // RP_X_PREPARE4WRITE
    UInt        mTransID;           // Transaction ID
    ULong       mSN;                // 로그의 SN
    ULong       mSyncSN;
    ULong       mLobLocator;        // Write할 LOB locator
    UInt        mOffset;            // 변경 시작 offset
    UInt        mOldSize;           // 변경 전 size
    UInt        mNewSize;           // 변경 후 size
} cmpArgRPLobPrepare4Write;

typedef struct cmpArgRPLobPartialWrite
{
    UInt        mXLogType;          // RP_X_LOB_PARTIAL_WRITE
    UInt        mTransID;           // Transaction ID
    ULong       mSN;                // 로그의 SN
    ULong       mSyncSN;
    ULong       mLobLocator;        // Write할 LOB locator
    UInt        mOffset;            // Piece를 Write할 LOB 내의 위치
    UInt        mPieceLen;          // Piece의 길이
    cmtVariable mPieceValue;        // Piece의 value
} cmpArgRPLobPartialWrite;

typedef struct cmpArgRPLobFinish2Write
{
    UInt        mXLogType;          // RP_X_LOB_FINISH2WRITE
    UInt        mTransID;           // Transaction ID
    ULong       mSN;                // 로그의 SN
    ULong       mSyncSN;
    ULong       mLobLocator;        // Write를 끝낼 LOB locator
} cmpArgRPLobFinish2Write;

typedef struct cmpArgRPLobTrim
{
    UInt        mXLogType;          // RP_X_LOB_TRIM
    UInt        mTransID;           // Transaction ID
    ULong       mSN;                // 로그의 SN
    ULong       mSyncSN;
    ULong       mLobLocator;        // Trim할 LOB locator
    UInt        mOffset;            // Trim offset
} cmpArgRPLobTrim;

typedef struct cmpArgRPHandshake
{
    UInt        mXLogType;          // RP_X_HANDSHAKE
    UInt        mTransID;           // Transaction ID
    ULong       mSN;                // 로그의 SN
    ULong       mSyncSN;
} cmpArgRPHandshake;

typedef struct cmpArgRPSyncPKBegin
{
    UInt        mXLogType;          // RP_X_SYNC_PK_BEGIN
    UInt        mTransID;           // Transaction ID
    ULong       mSN;                // 로그의 SN
    ULong       mSyncSN;
} cmpArgRPSyncPKBegin;

typedef struct cmpArgRPSyncPK
{
    UInt        mXLogType;          // RP_X_SYNC_PK
    UInt        mTransID;           // Transaction ID
    ULong       mSN;                // 로그의 SN
    ULong       mSyncSN;
    ULong       mTableOID;          // Delete 대상 Table OID
    UInt        mPKColCnt;          // Primary Key Column Count
} cmpArgRPSyncPK;

typedef struct cmpArgRPSyncPKEnd
{
    UInt        mXLogType;          // RP_X_SYNC_PK_END
    UInt        mTransID;           // Transaction ID
    ULong       mSN;                // 로그의 SN
    ULong       mSyncSN;
} cmpArgRPSyncPKEnd;

typedef struct cmpArgRPFailbackEnd
{
    UInt        mXLogType;          // RP_X_FAILBACK_END
    UInt        mTransID;           // Transaction ID
    ULong       mSN;                // 로그의 SN
    ULong       mSyncSN;
} cmpArgRPFailbackEnd;

typedef struct cmpArgRPSyncTableNumber
{
    UInt        mSyncTableNumber;
} cmpArgRPSyncTableNumber;

typedef struct cmpArgRPSyncStart
{
    UInt        mXLogType;          /* RP_X_SYNC_START */
    UInt        mTransID;           /* Transaction ID  */
    ULong       mSN;                /* 로그의 SN      */
} cmpArgRPSyncStart;

typedef struct cmpArgRPSyncEnd
{
    UInt        mXLogType;          /* RP_X_DROP_INDEX */
    UInt        mTransID;           /* Transaction ID  */
    ULong       mSN;                /* 로그의 SN      */
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
