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
 * $Id: smxDef.h 90177 2021-03-11 04:29:45Z lswhh $
 **********************************************************************/

#ifndef _O_SMX_DEF_H_
# define _O_SMX_DEF_H_ 1

# include <smDef.h>
# include <svrDef.h>
# include <sdpDef.h>

// smxTrans.mFlag (0x00000030)
# define SMX_REPL_MASK         SMI_TRANSACTION_REPL_MASK
# define SMX_REPL_DEFAULT      SMI_TRANSACTION_REPL_DEFAULT
# define SMX_REPL_NONE         SMI_TRANSACTION_REPL_NONE
# define SMX_REPL_RECOVERY     SMI_TRANSACTION_REPL_RECOVERY
# define SMX_REPL_REPLICATED   SMI_TRANSACTION_REPL_REPLICATED
# define SMX_REPL_PROPAGATION  SMI_TRANSACTION_REPL_PROPAGATION

//---------------------------------------------------------------------
// BUG-15396
// COMMIT WRITE WAIT type
// - SMX_COMMIT_WRITE_WAIT
//   commit 시에 log가 disk에 반영될때까지 기다림
//   따라서 durability 보장
// - SMX_COMMIT_WRITE_NOWAIT
//   commit 시에 log가 disk에 반영될때까지 기다리지 않음
//   따라서 durability 보장 못 함
// ps ) SMI_COMMIT_WRITE_MASK 값과 동일
//---------------------------------------------------------------------
// smxTrans.mFlag (0x00000100)
# define SMX_COMMIT_WRITE_MASK    SMI_COMMIT_WRITE_MASK
# define SMX_COMMIT_WRITE_NOWAIT  SMI_COMMIT_WRITE_NOWAIT
# define SMX_COMMIT_WRITE_WAIT    SMI_COMMIT_WRITE_WAIT

# define SMX_MUTEX_LIST_NODE_POOL_SIZE (10)
# define SMX_PRIVATE_BUCKETCOUNT  (smuProperty::getPrivateBucketCount())

// BUG-29839 재사용된 undo page에서 이전 CTS를 보려고 할 수 있음.
// 재현하기 위해 transaction에 특정 segment entry를 binding하는 기능 추가
// 특정 segment entry에 할당하지 않고 segment free list에서
// round-robin 방식으로 할당하는 방식인 경우(기존 방식)
# define SMX_AUTO_BINDING_TRANSACTION_SEGMENT_ENTRY (512)

// Trans Lock Flag
# define SMX_TRANS_UNLOCKED (0)
# define SMX_TRANS_LOCKED   (1)

// Savepoint Name For PSM
#define SMX_PSM_SAVEPOINT_NAME "$$PSM_SVP"
/**
 * 트랜잭션이 partial rollback을 수행할때, partial rollback의 대상이 되는
 * oidList들의 mFlag에 대해서 mAndMask를 And하고, mOrMask를 or한다.
 * mAndMask는 설정되어있는 flag중에 몇개를 제거하기 위해 쓰이고,
 * mOrMask는 설정되어있는 flag중에 몇개를 추가하고자 하기 위해 쓰인다.
 * 바로 AGER LIST에 매달지 않고, 즉 바로 처리하지 않고 실제 트랜잭션이 commit하거나,
 * rollback할때 처리하게 된다.
 * */
typedef struct smxOidSavePointMaskType
{
    UInt mAndMask;
    UInt mOrMask;
}
smxOidSavePointMaskType;

extern const smxOidSavePointMaskType smxOidSavePointMask[SM_OID_OP_COUNT];

typedef enum
{
    SMX_TX_BEGIN,
    SMX_TX_PRECOMMIT, /* BUG-47367 Tx이 commitSCN을 획득하기 직전 상태 */
    SMX_TX_COMMIT_IN_MEMORY, // not used
    SMX_TX_COMMIT,
    SMX_TX_ABORT,
    SMX_TX_BLOCKED,
    SMX_TX_END,
    SMX_TX_PREPARED,  /* PROJ-2733 XA_PREPARED 된 상태 */
} smxStatus;

struct smcTableHeader;

// For Global Transaction Commit State
typedef smiCommitState smxCommitState;

typedef struct smxOIDInfo
{
    smOID            mTableOID;
    smOID            mTargetOID;
    smSCN            mSCN; /* BUG-47367 FreeSlotList를 위한 SCN이 필요함. */
    UInt             mFlag;
    scSpaceID        mSpaceID;
    UShort           mAlign; 
} smxOIDInfo;

typedef struct smxOIDNode
{
    smxOIDNode      *mPrvNode;
    smxOIDNode      *mNxtNode;
    UInt             mOIDCnt;
    UInt             mAlign;
    smxOIDInfo       mArrOIDInfo[1];
} smxOIDNode;

typedef struct smxOIDNodeHead
{
    smxOIDNode      *mPrvNode;
    smxOIDNode      *mNxtNode;
    UInt             mOIDCnt;
    UInt             mAlign;
} smxOIDNodeHead;


/*---------------------------------------------------------------*/
//  Save Point
/*---------------------------------------------------------------*/
#define SMX_SAVEPOINT_POOL_SIZE   (256)
#define SMX_DEF_SAVEPOINT_COUNT   (10)
#define SMX_IMPLICIT_SVP_INDEX    (0)
#define SMX_MAX_SVPNAME_SIZE      (256)

typedef struct smxSavepoint
{
    smLSN           mUndoLSN;

    /* PROJ-1594 Volatile TBS */
    /* volatile TBS 대한 rollback을 위해 필요함 */
    svrLSN          mVolatileLSN;

    /* BUG-15096: non-autocommit모드에서 select완료후 IS_LOCK이 해제되면
       좋겠습니다.: Lock해제시 Transaction의 Lock Slot의 Lock Sequence가
       mLockSequence보다 클때까지 Lock을 해제한다. */
    ULong           mLockSequence;
    SChar           mName[SMX_MAX_SVPNAME_SIZE];
    smxOIDNode     *mOIDNode;
    SInt            mOffset;
    smxSavepoint   *mPrvSavepoint;
    smxSavepoint   *mNxtSavepoint;

    /* For only implicit savepoint */
    /* Savepoint를 찍은 Statement의 Depth */
    UInt            mStmtDepth;
    /* BUG-17033: 최상위 Statement가 아닌 Statment에 대해서도
     * Partial Rollback을 지원해야 합니다.
     *
     * Replication과 Transaction의 Implicit Savepoint의 위치는
     * 다르다. 때문에 Replication을 위해서 기록되는 Savepoint Abort로그
     * 기록시 Savepoint의 이름이 현재 Abort를 수행할 Savepoint와
     * 다른 경우가 존재한다.
     *
     * ex) T1: No Replication , T2: Replication
     * + Stmt1 begin
     *    - update T1(#1)
     *   + Stmt1-1 begin
     *      + Stmt1-1-1 begin
     *        - update T2(#2) <---- Replication SVP(Depth = 3)
     *      + Stmt1-1-1 end(Success)
     *      update T2(#3) <---- Replication SVP(Depth = 2)
     *   + Stmt1-1 end(Failure !!)
     *
     *   위 예제에서 Stmt1-1이 Abort했으므로 Abort로그또한 Stmt1-1이
     *   기록되어야 하나 그렇게 되면 Standby쪽은 #3만 Rollback된다.
     *   왜냐면 Depth=2라는 Abort로그가 찍히고 Dept=2에 해당하는
     *   Implicit SVP는 #3에서 지정이 되었기 때문이다. 이는
     *   Stmt begin후 첫번째 Replication 관련 Table에 대한
     *   Update시 그 로그를 Replication Begin로그로 설정하기때문이다.
     *   위 문제를 해결하기 위해서 mReplImpSvpStmtDepth를 추가했다.
     *   mReplImpSvpStmtDepth는 Partial Abort시 Implicit SVP Abort
     *   로그 기록시 SVP의 이름을 mReplImpSvpStmtDepth으로 지정한다.
     *   위에서 발생하는 문제는 다음과 같이 mReplImpSvpStmtDepth을
     *   이용함으로써 해결될 수 있다.
     *
     * ex) T1: No Replication , T2: Replication
     * 1: + Stmt1 begin : Set SVP1
     * 2:   - update T1(#1)
     * 3:  + Stmt1-1 begin : Set SVP2
     * 4:     + Stmt1-1-1 begin : Set SVP3
     * 5:       - update T2(#2) <---- Replication SVP(Depth = 3)
     * 6:     + Stmt1-1-1 end(Success) : Remove SVP3
     * 7:     update T2(#3) <---- 5line에서 자기앞에 Replication Implicit
     *                            savepoint를 설정하지 않은 부분을 모두설정했기
     *                            때문에 여기서 설정할 필요가 없다.
     * 8:  + Stmt1-1 end(Failure !!)
     *
     * Implicit SVP가 Set될때마다 Transaction의 Implicit SVP List에
     * 추가된다.
     *
     * + SMI_STATEMENT_DEPTH_NULL = 0
     * + SVP(#1, #2) :
     *    - SVP1 <-- Savepoint Name,
     *    - #1: Stmt Depth, #2: mReplImpSvpStmtDepth
     *
     * Trans: SVP1(1, 0)  + SVP2(2, 0) + SVP3(3, 0)
     * 이것은 line 4까지 수행했을때의 결과이다. 여기서 line5를 수행하게
     * 되면
     * Trans: SVP1(1, 1)  + SVP2(2, 1) + SVP3(3, 1)
     * 으로 설정된다. Replication Savepoint가 설정될때 앞에있는 SVP값중
     * 가장 큰값 + 1을 자신의 mReplImpSvpStmtDepth값으로 결정한다.
     * 그리고 자신의 앞의 SVP들을 찾아가면서 0이 아니값을 가진 SVP가
     * 나올때까지 0인값을 가진 SVP를 모두 자신의 mReplImpSvpStmtDepth값으로
     * 설정해 준다.
     * 위 예제에서는 SVP3의 앞의 SVP중 가장 큰값이 0이기 때문에 SVP3의
     * mReplImpSvpStmtDepth값을 (0 + 1)으로 설정한다. 그리고 SVP1과
     * SVP2가 0이므로 이값을 자신의 mReplImpSvpStmtDepth의 값인 1로
     * 모두 바꾸어 준다. 왜냐면 SVP2에서 한번도 Replication SVP가 설정되지
     * 않아서 만약 SVP2가 Abort된다면 실제로는 SVP3까지 Rollback하지만
     * Replication은 이를 알지 못하게 되기 때문이다.
     * Partial Rollback하게 되면 mReplImpSvpStmtDepth값을 Partial Abort
     * Log에 기록해준다.
     * 다시 line 6을 수행하게되면 SVP3가 제거된다.
     * Trans: SVP1(1, 1)  + SVP2(2, 1)
     * 다시 line8을 수행하게 되면 SVP2를 제거하고 Implicit SVP Abort로그
     * 기록시 이름을 Depth=1로 지정하게 된다.
     * */

    UInt            mReplImpSvpStmtDepth;

    struct smiDDLTargetTableInfo * mDDLTargetTableInfo;
} smxSavepoint;


// memory , disk view scn set callback func
typedef void   (*smxTrySetupViewSCNFuncs)( void  * aTrans, smSCN * aSCN );
typedef void   (*smxGetSmmViewSCNFunc)   ( smSCN * aSCN);
typedef IDE_RC (*smxGetSmmCommitSCNFunc) ( void   * aTrans,
                                           idBool   aIsLegacyTrans,
                                           void   * aStatus );


// PRJ-1496
#define SMX_TABLEINFO_CACHE_COUNT (10)
#define SMX_TABLEINFO_HASHTABLE_SIZE (10)

//PROJ-1362
 //fix BUG-21311
#define SMX_LOB_MIN_BUCKETCOUNT  (5)

typedef struct smxTableInfo
{
    smOID         mTableOID;
    scPageID      mDirtyPageID;
    SLong         mRecordCnt;

    /*
     * PROJ-1671 Bitmap-based Tablespace And
     *           Segment Space Management
     * Table Segment에서 마지막 Slot을 할당했던
     * Data 페이지의 PID를 Cache한다.
     */
    scPageID      mHintDataPID;
    idBool        mExistDPathIns;

    smxTableInfo *mPrevPtr;
    smxTableInfo *mNextPtr;
} smxTableInfo;

struct sdpSegMgmtOp;

// PROJ-1553 Replication self-deadlock

#define SMX_NOT_REPL_TX_ID       (ID_SINT_MAX)
#define SMX_LOCK_WAIT_REPL_TX_ID (ID_SINT_MAX - 1)

/* BUG-27709 receiver에 의한 반영 중, 트랜잭션 alloc실패시 해당 receiver 종료 */
#define SMX_IGNORE_TX_ALLOC_TIMEOUT (0)

// Transaction의 Log Buffer크기를 항상 이값으로 Align시킨다.
#define SMX_TRANSACTION_LOG_BUFFER_ALIGN_SIZE (1024)

// Transaction의 초기 Buffer크기.
#define SMX_TRANSACTION_LOG_BUFFER_INIT_SIZE  (2048)

/***********************************************************************
 *
 * PROJ-1704 Disk MVCC 리뉴얼
 *
 * Touch Page List 관련 자료구조 정의
 *
 * 트랜잭션이 갱신한 디스크 페이지 정보
 *
 ***********************************************************************/
typedef struct smxTouchInfo
{
    scPageID         mPageID;      // 갱신된 페이지의 PageID
    scSpaceID        mSpaceID;     // 갱신된 페이지의 SpaceID
    SShort           mCTSlotIdx;   // 갱신된 페이지내의 할당된 CTS 순번
} smxTouchInfo;

/*
 * 갱신된 여러개의 페이지들을 관리하는 노드 정의
 * 메모리 할당/해제 단위로 사용함
 * 관리되는 개수는 TOUCH_PAGE_CNT_BY_NODE 에 의해서 결정된다
 */
typedef struct smxTouchNode
{
    smxTouchNode   * mNxtNode;
    UInt             mPageCnt;
    UInt             mAlign;
    smxTouchInfo     mArrTouchInfo[1];
} smxTouchNode;

/*
 * smxTouchPage Hash를 검색한 결과값으로 사용되는 flag
 */

#define SMX_TPH_PAGE_NOT_FOUND     (-2)
#define SMX_TPH_PAGE_EXIST_UNKNOWN (-1)
#define SMX_TPH_PAGE_FOUND         (0)

/*
 * Transaction 정보 X$ 출력 자료구조 정의
 */
typedef struct smxTransInfo4Perf
{
    smTID              mTransID;
    //fix BUG-23656 session,xid ,transaction을 연계한 performance view를 제공하고,
    //그들간의 관계를 정확히 유지해야 함.
    //transaction에서 session id를 추가. 
    UInt               mSessionID;
    smSCN              mMscn;
    smSCN              mDscn;
    smSCN              mFstDskViewSCN;
    // BUG-24821 V$TRANSACTION에 LOB관련 MinSCN이 출력되어야 합니다. 
    smSCN              mMinMemViewSCNwithLOB;
    smSCN              mMinDskViewSCNwithLOB;
    smSCN              mLastRequestSCN;
    smSCN              mPrepareSCN;
    smSCN              mCommitSCN;
    smxStatus          mStatus;
    idBool             mIsUpdate; // durable writing의 여부
    UInt               mLogTypeFlag;
    ID_XID             mXaTransID;
    smxCommitState     mCommitState;
    timeval            mPreparedTime;
    smLSN              mFstUndoNxtLSN;
    smLSN              mCurUndoNxtLSN;
    smLSN              mLstUndoNxtLSN;
    smSN               mCurUndoNxtSN;
    SInt               mSlotN;
    ULong              mUpdateSize;
    idBool             mAbleToRollback;
    UInt               mFstUpdateTime;
    /* BUG-33895 [sm_recovery] add the estimate function 
     * the time of transaction undoing. */
    UInt               mProcessedUndoTime;
    ULong              mTotalLogCount;
    ULong              mProcessedUndoLogCount;
    UInt               mEstimatedTotalUndoTime;
    UInt               mLogBufferSize;
    UInt               mLogOffset;
    idBool             mDoSkipCheck;
    idBool             mDoSkipCheckSCN;
    idBool             mIsDDL;
    sdSID              mTSSlotSID;
    UInt               mRSGroupID;
    idBool             mDPathInsExist;
    UInt               mMemLobCursorCount;
    UInt               mDiskLobCursorCount;
    UInt               mLegacyTransCount;
    // BUG-40213 Add isolation level in V$TRANSACTION
    UInt               mIsolationLevel;

    /* PROJ-2734 */
    PDL_Time_Value     mGCTxFirstStmtTime;
    smSCN              mGCTxFirstStmtViewSCN;
    smiDistLevel       mDistLevel;
    smiShardPin        mShardPin;
    /* PROJ-2733 */
    idBool             mIsGCTx; //GlobalConsistentTx:GLOBAL_TRANSACTION_LEVEL=3 으로 설정된 Tx
    
    SChar              mDetectionStr[64];
    ULong              mDieWaitTime;
    ULong              mElapsedTime;

} smxTransInfo4Perf;

/*
 * Transaction Segment 정보 X$ 출력 자료구조 정의
 */
typedef struct smxTXSeg4Perf
{
    UInt               mEntryID;
    smTID              mTransID;
    smSCN              mMinDskViewSCN;
    smSCN              mCommitSCN;
    smSCN              mFstDskViewSCN;
    sdSID              mTSSlotSID;
    sdRID              mExtRID4TSS;
    sdRID              mFstExtRID4UDS;
    sdRID              mLstExtRID4UDS;
    scPageID           mFstUndoPID;
    scPageID           mLstUndoPID;
    scSlotNum          mFstUndoSlotNum;
    scSlotNum          mLstUndoSlotNum;
} smxTXSeg4Perf;

/* PROJ-1381 Fetch Across Commits
 * FAC commit시의 smxTrans의 복사본 */
typedef struct smxLegacyTrans
{
    smTID       mTransID;
    smLSN       mCommitEndLSN;
    void      * mOIDList;       /* smxOIDList * */
    smSCN       mCommitSCN;
    smSCN       mMinMemViewSCN;
    smSCN       mMinDskViewSCN;
    smSCN       mFstDskViewSCN;
    UChar       mMadeType;
    iduMutex    mWaitForNoAccessAftDropTbl; /* BUG-42760 */
} smxLegacyTrans;

typedef struct smxLegacyStmt
{
    void * mLegacyTrans;  /* smxLegacyTrans * */
    void * mStmtListHead; /* smiStatement *   */
} smxLegacyStmt;

#endif
