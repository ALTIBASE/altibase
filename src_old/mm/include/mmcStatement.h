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

#ifndef _O_MMC_STATEMENT_H_
#define _O_MMC_STATEMENT_H_ 1

#include <idl.h>
#include <ide.h>
#include <idu.h>
#include <smi.h>
#include <qci.h>
#include <sdi.h>
#include <sdiStatement.h>
#include <mmcDef.h>
#include <mmcPCB.h>
#include <mmcStatementManager.h>
#include <mmcTrans.h>
#include <cmpDefDB.h>
#include <idvAudit.h>


class mmcSession;
class mmcStatement;
class mmcPCB;
class mmcParentPCO;

typedef IDE_RC (*mmcStmtBeginFunc)(mmcStatement *aStmt);
typedef IDE_RC (*mmcStmtEndFunc)(mmcStatement *aStmt, idBool aSuccess);
typedef IDE_RC (*mmcStmtExecFunc)(mmcStatement *aStmt, SLong *aAffectedRowCount, SLong *aFetchedRowCount);


typedef struct mmcGCTxStmtShareInfo
{
    smSCN               mTxFirstStmtSCN;
    ULong               mTxFirstStmtTime;
    sdiDistLevel        mDistLevel;
} mmcGCTxStmtShareInfo;

typedef struct mmcGCTxStmtInfo
{
    smSCN                  mRequestSCN;
    mmcGCTxStmtShareInfo * mRoot;
    mmcGCTxStmtShareInfo   mShareBuffer;
} mmcGCTxStmtInfo;

typedef struct mmcStatementInfo
{
    mmcStatement        *mStmt;
    mmcStatement        *mParentStmt;

    mmcStmtID            mStatementID;

    mmcSessID            mSessionID;
    smTID                mTransID;

    mmcStmtExecMode      mStmtExecMode;
    mmcStmtState         mStmtState;

    UShort               mResultSetCount;
    UShort               mEnableResultSetCount;
    UShort               mResultSetHWM;
    mmcResultSet        *mResultSet;
    mmcFetchFlag         mFetchFlag;

    SChar               *mQueryString;
    UInt                 mQueryLen;

    UInt                 mCursorFlag;
    
    idBool               mIsArray;
    idBool               mIsAtomic;
    UInt                 mRowNumber;
    UInt                 mTotalRowNumber; // BUG-41452

    idBool               mIsStmtBegin;
    idBool               mExecuteFlag;

    UInt                 mLastQueryStartTime;
    UInt                 mQueryStartTime;
    UInt                 mFetchStartTime;
    UInt                 mFetchEndTime;  /* BUG-19456 */
    //PROJ-1436
    SChar               *mSQLPlanCacheTextId;
    UInt                 mSQLPlanCachePCOId;
    idvSQL               mStatistics;
    idBool               mPreparedTimeExist; // bug-23991

    /* PROJ-2177 User Interface - Cancel */

    mmcStmtCID           mStmtCID;     /**< Client side Statement ID */
    idBool               mIsExecuting; /**< Execute 명령을 수행중인지 여부. ExecuteFlag와는 나타내는바가 다르므로 새 변수로 사용 */

    /* PROJ-1381 Fetech Across Commits */

    mmcStmtCursorHold    mCursorHold;  /**< Cursor holdability */
    mmcStmtKeysetMode    mKeysetMode;  /**< Keyset mode */

    /* PROJ-2616 IPCDA */
    idBool               mIsSimpleQuery;

    /* BUG-45823 */
    SChar                mShardPinStr[SDI_MAX_SHARD_PIN_STR_LEN + 1];
    UInt                 mShardSessionType;
    UInt                 mShardQueryType;

    UInt                 mFlag;        /* BUG-47238 */

    ULong                mMathTempMem; /* BUG-46892 */

    /* PROJ-2733-DistTxInfo */
    mmcGCTxStmtInfo      mGCTxStmtInfo;

    SInt                 mRebuildCount;

    /* TASK-7219 Non-shard DML */
    sdiShardPartialExecType mShardPartialExecType;

} mmcStatementInfo;

typedef struct mmcEndTransAction4Prepare
{
    mmcTransEndAction       mTransEndAction;
    idBool                  mNeedReleaseLock;
} mmcEndTransAction4Prepare;

typedef enum mmcEndTransResult4Prepare
{
    MMC_END_TRANS_SUCCESS = 0,
    MMC_END_TRANS_FAILURE = 1
} mmcEndTransResult4Prepare;

//PROJ-1436 SQL-Plan Cache.
typedef struct mmcGetSmiStmt4PrepareContext
{
    mmcStatement               *mStatement;
    mmcTransObj                *mTrans;
    smiStatement                mPrepareStmt;
    idBool                      mNeedCommit;
    mmcEndTransAction4Prepare   mEndTransAction;
    mmcSoftPrepareReason        mSoftPrepareReason;
}mmcGetSmiStmt4PrepareContext;

typedef struct mmcAtomicInfo
{
    idBool                mAtomicExecuteSuccess; // 수행중 실패시 FALSE
    // BUG-21489
    UInt                  mAtomicLastErrorCode;
    SChar                 mAtomicErrorMsg[MAX_ERROR_MSG_LEN+256];
    idBool                mIsCursorOpen;
} mmcAtomicInfo;

class mmcStatement
{
private:
    mmcStatementInfo  mInfo;

    mmcSession       *mSession;

    mmcTransObj      *mTrans;

    /* PROJ-2701 Online Data Rebuild: for Statement Serialize */
    mmcTransObj      *mExecutingTrans;
    idBool            mIsShareTransSmiStmtLocked;

    smiStatement      mSmiStmt;
    smiStatement    * mSmiStmtPtr; // PROJ-1386 Dynamic-SQL
    
    qciStatement      mQciStmt;
    qciStmtType       mStmtType;
    qciStmtInfo       mQciStmtInfo;

    mmcStmtBindState  mBindState;

    iduListNode       mStmtListNodeForParent;
    iduListNode       mStmtListNode;
    iduListNode       mFetchListNode;

    /* PROJ-2109 : Remove the bottleneck of alloc/free stmts. */
    /* This mutex will get from the mutex pool in mmcSession */
    iduMutex         *mQueryMutex;

    iduVarString      mPlanString;
    idBool            mPlanPrinted;

    // BUG-17544
    idBool            mSendBindColumnInfo; // Prepare 후 컬럼 정보 전달 여부 플래그

    /* BUG-38472 Query timeout applies to one statement. */
    idBool            mTimeoutEventOccured;

    // PROJ-1386 Dynamic-SQL
    iduList           mChildStmtList;
    // PROJ-1518
    mmcAtomicInfo     mAtomicInfo;
    //PROJ-1436 SQL Plan Cache.
    mmcPCB*           mPCB;

    /* PROJ-2223 Altibase Auditing */
    qciAuditRefObject * mAuditObjects;  
    UInt                mAuditObjectCount;
    idvAuditTrail       mAuditTrail;

    /* PROJ-2616 */
    idBool              mIsSimpleQuerySelectExecuted;

    /* BUG-46092 */
    sdiStatement        mSdStmt;

public:
    IDE_RC initialize(mmcStmtID      aStatementID,
                      mmcSession   * aSession,
                      mmcStatement * aParentStmt);
    IDE_RC finalize();

    void   lockQuery();
    void   unlockQuery();

    idBool isPlanPrinted();
    IDE_RC makePlanTreeText(idBool aCodeOnly);
    IDE_RC clearPlanTreeText();

    /* PROJ-2598 Shard pilot(shard Analyze) */
    IDE_RC shardAnalyze(SChar *aQueryString, UInt aQueryLen);

    IDE_RC prepare(SChar *aQueryString, UInt aQueryLen);
    IDE_RC execute(SLong *aAffectedRowCount, SLong *aFetchedRowCount);
    IDE_RC rebuild();
    IDE_RC shardRetryRebuild();
    IDE_RC checkShardRebuildStatus( idBool * aRebuildDetected );
    IDE_RC reprepare(); // PROJ-2163
    IDE_RC changeStmtState(); // BUG-36203

    IDE_RC closeCursor(idBool aSuccess);

    /* PROJ-2616 */
    void setSimpleQuerySelectExecuted(idBool aVal){mIsSimpleQuerySelectExecuted = aVal;}
    idBool isSimpleQuerySelectExecuted(){return mIsSimpleQuerySelectExecuted;}

    /*
     * Execute과정
     *
     * beginStmt -> execute -> [ beginFetch -> endFetch ] -> clearStmt -> endStmt
     */

    IDE_RC beginStmt();
    // fix BUG-30990
    // clearStmt()의 상태를 명확하게 한다.
    IDE_RC clearStmt(mmcStmtBindState aBindState);
    IDE_RC endStmt(mmcExecutionFlag aExecutionFlag);

    IDE_RC beginFetch(UShort aResultSetID);
    IDE_RC endFetch(UShort aResultSetID);

    IDE_RC freeChildStmt( idBool aSuccess,
                          idBool aIsFree );

    /* BUG-46090 Meta Node SMN 전파 */
    void clearShardDataInfo();

    void clearShardDataInfoForRebuild();

    /* BUG-46092 */
    void freeAllRemoteStatement( UChar aMode );
    void freeRemoteStatement( UInt aNodeId, UChar aMode );

    //PROJ-1436 SQL-Plan Cache.
    IDE_RC beginTransAndLock4Prepare( mmcTransObj                    ** aTrans,
                                      mmcEndTransAction4Prepare       * aEndTransAction );
    IDE_RC endTransAndUnlock4Prepare( mmcTransObj                * aTrans,
                                      mmcEndTransAction4Prepare  * aEndTransAction,
                                      mmcEndTransResult4Prepare    aResult );

    //PROJ-1436 SQL-Plan Cache.
    static IDE_RC  getSmiStatement4PrepareCallback(void* aGetSmiStmtContext,
                                                   smiStatement  **aSmiStatement4Prepare);

    static void makePlanTreeBeforeCloseCursor( mmcStatement * aStatement,
                                               mmcStatement * aResultSetStmt );

    /* PROJ-2223 Altibase Auditing */
    UInt getExecuteResult( mmcExecutionFlag aExecutionFlag );
    
    /* BUG-46756 */
    void   setSmiStmt(smiStatement *aSmiStmt);

    /* BUG-47029 */
    IDE_RC beginSmiStmt(mmcTransObj *aTrans, UInt aFlag);
    IDE_RC endSmiStmt(UInt aFlag);

    /* BUG-47029 */
    UInt getSmiStatementFlag(mmcSession *aSession, mmcStatement *aStmt, mmcTransObj *aTrans);

private:
    IDE_RC parse(SChar* aSQLText);
    IDE_RC doHardPrepare(SChar                   *aSQLText,
                         qciSQLPlanCacheContext  *aPlanCacheContext,
                         smiStatement            *aSmiStmt);
    
    IDE_RC doHardRebuild(mmcPCB                  *aPCB,
                         qciSQLPlanCacheContext  *aPlanCacheContext);
    
    IDE_RC resetCursorFlag();
    
    IDE_RC softPrepare(mmcParentPCO         *aParentPCO,
                       //fix BUG-22061
                       mmcSoftPrepareReason aSoftPrepareReason,
                       mmcPCB               **aPCB,
                       idBool               *aNeedHardPrepare);
    
    IDE_RC hardPrepare(mmcPCB                  *aPCB,
                       qciSQLPlanCacheContext  *aPlanCacheContext);

    /* PROJ-2733-DistTxInfo */
    IDE_RC beginSmiStmtInternal(mmcTransObj *aTrans, UInt aFlag, UChar *aSessionType);

    /* BUG-48115 */
    void buildSmiDistTxInfo( smiDistTxInfo *aDistTxInfo );

    IDE_RC updateSystemSCN();

public:
    mmcStatementInfo *getInfo();

    mmcSession       *getSession();
    
    mmcTransObj      *allocTrans();
    mmcTransObj      *getTransPtr();
    smiStatement     *getSmiStmt();

    qciStatement     *getQciStmt();
    qciStmtType       getStmtType();

    mmcStmtBindState  getBindState();
    void              setBindState(mmcStmtBindState aBindState);

    iduListNode      *getStmtListNodeForParent();
    iduListNode      *getStmtListNode();
    iduListNode      *getFetchListNode();

    iduVarString     *getPlanString();

    idBool            getSendBindColumnInfo();
    void              setSendBindColumnInfo(idBool aSendBindColumnInfo);

    /* BUG-38472 Query timeout applies to one statement. */
    idBool            getTimeoutEventOccured();
    void              setTimeoutEventOccured( idBool aTimeoutEventOccured );

    // PROJ-1518
    idBool            getAtomicExecSuccess();
    void              setAtomicExecSuccess(idBool aAtomicExecuteSuccess);

    mmcAtomicInfo    *getAtomicInfo();

    // BUG-21489
    void              setAtomicLastErrorCode(UInt aErrorCode);
    void              clearAtomicLastErrorCode();

    idBool            isRootStmt();
    mmcStatement      *getRootStmt();

    void              setTrans( mmcTransObj * aTrans );

    iduList          *getChildStmtList();

    // bug-23991: prepared 후 execute 마다 PVO time 값이 그대로 남음
    IDE_RC            clearStatisticsTime();
    idBool            getPreparedTimeExist();
    void              setPreparedTimeExist(idBool aPreparedTimeExist);

    // PROJ-2256
    void              initBaseRow( mmcBaseRow *aBaseRow );
    IDE_RC            allocBaseRow( mmcBaseRow *aBaseRow, UInt aBaseRowSize );
    IDE_RC            freeBaseRow( mmcBaseRow *aBaseRow );

    idvAuditTrail     *getAuditTrail();
    UInt               getAuditObjectCount();
    qciAuditRefObject *getAuditObjects();

    void               resetStatistics();


/*
 * Info Accessor
 */
public:
    mmcStmtID           getStmtID();
    mmcSessID           getSessionID();

    mmcStatement*       getParentStmt();
    smTID               getTransID();
    void                setTransID(smTID aTransID);

    mmcStmtExecMode     getStmtExecMode();
    void                setStmtExecMode(mmcStmtExecMode aStmtExecMode);

    mmcStmtState        getStmtState();
    void                setStmtState(mmcStmtState aStmtState);

    UShort              getResultSetCount();
    void                setResultSetCount(UShort aResultSetCount);

    UShort              getEnableResultSetCount();
    void                setEnableResultSetCount(UShort aEnableResultSetCount);

    UShort              getResultSetHWM();
    void                setResultSetHWM(UShort aResultSetHWM);

    mmcResultSetState   getResultSetState(UShort aResultSetID);
    IDE_RC              setResultSetState(UShort aResultSetID, mmcResultSetState aResultSetState);
    mmcResultSet*       getResultSet( UShort aResultSetID );

    mmcFetchFlag        getFetchFlag();
    void                setFetchFlag(mmcFetchFlag aFetchFlag);

    SChar              *getQueryString();
    UInt                getQueryLen();
    void                setQueryString(SChar *aQuery, UInt aQueryLen);

    UInt                getCursorFlag();
    void                setCursorFlag(UInt aCursorFlag);

    idBool              isArray();
    void                setArray(idBool aArrayFlag);

    idBool              isAtomic();
    void                setAtomic(idBool aAtomicFlag);

    UInt                getRowNumber();
    void                setRowNumber(UInt aRowNumber);

    UInt                getTotalRowNumber();
    void                setTotalRowNumber(UInt aRowNumber);

    idBool              isStmtBegin();
    
    void                setStmtBegin(idBool aBeginFlag);

    idBool              getExecuteFlag();
    void                setExecuteFlag(idBool aExecuteFlag);

    UInt                getLastQueryStartTime();

    UInt                getQueryStartTime();
    void                setQueryStartTime(UInt aQueryStartTime);

    UInt                getFetchStartTime();
    void                setFetchStartTime(UInt aFetchStartTime);

    /* BUG-19456 */
    UInt                getFetchEndTime();
    void                setFetchEndTime(UInt aFetchEndTime);
    
    idvSQL             *getStatistics();

    IDE_RC initializeResultSet(UShort aResultSetCount);
   
    void                applyOpTimeToSession();
    //fix BUG-24364 valgrind direct execute으로 수행시킨 statement의  plan cache정보를 reset시켜야함.
    inline void         releasePlanCacheObject();

    /*
     * [BUG-24187] Rollback될 statement는 Internal CloseCurosr를
     * 수행할 필요가 없습니다.
     */
    void                setSkipCursorClose();
    
    /* PROJ-2177 User Interface - Cancel */
    mmcStmtCID          getStmtCID();
    void                setStmtCID(mmcStmtCID aStmtCID);
    idBool              isExecuting();
    void                setExecuting(idBool aIsExecuting);

    /* PROJ-1381 Fetch Across Commits */
    mmcStmtCursorHold   getCursorHold(void);
    void                setCursorHold(mmcStmtCursorHold aCursorHold);

    /* PROJ-1789 Updatable Scrollable Cursor */
    void                setKeysetMode(mmcStmtKeysetMode aKeysetMode);
    mmcStmtKeysetMode   getKeysetMode(void);
    idBool              isClosableWhenFetchEnd(void);

    /* PROJ-2616 */
    idBool              isSimpleQuery();
    void                setSimpleQuery(idBool aIsSimpleQuery);

    void                setShardQueryType( sdiQueryType aQueryType );

    /* BUG-46092 */
    void *              getShardStatement();

    /* PROJ-2701 Online Data Rebuild: for Statement Serialize */
    mmcTransObj *       getShareTransForSmiStmtLock(mmcTransObj *aTrans);
    mmcTransObj *       getExecutingTrans();
    IDE_RC              acquireShareTransSmiStmtLock(mmcTransObj *aTrans);
    void                releaseShareTransSmiStmtLock(mmcTransObj *aTrans);

    void                getRequestSCN(smSCN *aSCN);
    void                setRequestSCN(smSCN *aSCN);
    void                getTxFirstStmtSCN(smSCN *aTxFirstStmtSCN);
    void                setTxFirstStmtSCN(smSCN *aTxFirstStmtSCN);
    SLong               getTxFirstStmtTime();
    void                setTxFirstStmtTime(SLong aTxFirstStmtTime);
    sdiDistLevel        getDistLevel();
    void                setDistLevel(sdiDistLevel aDistLevel);
    void                clearGCTxStmtInfo();
    mmcGCTxStmtShareInfo * getGCTxStmtShareInfoBuffer();
    void                linkGCTxStmtInfo();
    void                unlinkGCTxStmtInfo();
    idBool              isNeedRequestSCN();

    idBool              getIsShareTransSmiStmtLocked() { return mIsShareTransSmiStmtLocked; }

    void                addRebuildCount();

    /* TASK-7219 Non-shard DML */
    void                    setShardPartialExecType( sdiShardPartialExecType aShardPartialExecType );
    sdiShardPartialExecType getShardPartialExecType( void );

private:
    /*
     * Statement Begin/End/Execute Functions
     */

    static mmcStmtBeginFunc mBeginFunc[];
    static mmcStmtEndFunc   mEndFunc[];
    static mmcStmtExecFunc  mExecuteFunc[];
    static SChar            mNoneSQLCacheTextID[MMC_SQL_CACHE_TEXT_ID_LEN];
    
    static IDE_RC beginDDL(mmcStatement *aStmt);
    static IDE_RC beginDML(mmcStatement *aStmt);
    static IDE_RC beginDCL(mmcStatement *aStmt);
    static IDE_RC beginSP(mmcStatement *aStmt);
    static IDE_RC beginDB(mmcStatement *aStmt);

    static IDE_RC endDDL(mmcStatement *aStmt, idBool aSuccess);
    static IDE_RC endDML(mmcStatement *aStmt, idBool aSuccess);
    static IDE_RC endDCL(mmcStatement *aStmt, idBool aSuccess);
    static IDE_RC endSP(mmcStatement *aStmt, idBool aSuccess);
    static IDE_RC endDB(mmcStatement *aStmt, idBool aSuccess);

    static IDE_RC executeDDL(mmcStatement *aStmt, SLong *aAffectedRowCount, SLong *aFetchedRowCount);
    static IDE_RC executeDML(mmcStatement *aStmt, SLong *aAffectedRowCount, SLong *aFetchedRowCount);
    static IDE_RC executeDCL(mmcStatement *aStmt, SLong *aAffectedRowCount, SLong *aFetchedRowCount);
    static IDE_RC executeSP(mmcStatement *aStmt, SLong *aAffectedRowCount, SLong *aFetchedRowCount);
    static IDE_RC executeDB(mmcStatement *aStmt, SLong *aAffectedRowCount, SLong *aFetchedRowCount);

    /* PROJ-2736 GLOBAL_DDL */    
    static IDE_RC executeLocalDDL( mmcStatement * aStmt );
    static IDE_RC executeGlobalDDL( mmcStatement * aStmt );

    static IDE_RC lockTableAllShardNodes( mmcStatement * aStmt, smiTableLockMode aLockMode );

    /* PROJ-2223 Altibase Auditing */
    static void setAuditTrailStatElapsedTime( idvAuditTrail *aAuditTrail, idvSQL *aStatSQL );

public:
    /* PROJ-2223 Altibase Auditing */
    static void setAuditTrailStatSess( idvAuditTrail *aAuditTrail, idvSQL *aStatSQL );
    static void setAuditTrailStatStmt( idvAuditTrail *aAuditTrail, 
                                       qciStatement  *aQciStmt,
                                       mmcExecutionFlag aExecutionFlag, 
                                       idvSQL *aStatSQL );
    static void setAuditTrailSess( idvAuditTrail *aAuditTrail, mmcSession *aSession );
    static void setAuditTrailStmt( idvAuditTrail *aAuditTrail, mmcStatement *aStmt );

    /* BUG-43113 Autonomous Transaction */
    inline void setSmiStmtForAT( smiStatement * aSmiStmt );
};

inline void mmcStatement::lockQuery()
{
    /* PROJ-2109 : Remove the bottleneck of alloc/free stmts. */
    IDE_ASSERT(mQueryMutex->lock(NULL /* idvSQL* */)
               == IDE_SUCCESS);
}

inline void mmcStatement::unlockQuery()
{
    /* PROJ-2109 : Remove the bottleneck of alloc/free stmts. */
    IDE_ASSERT(mQueryMutex->unlock() == IDE_SUCCESS);
}

inline idBool mmcStatement::isPlanPrinted()
{
    return mPlanPrinted;
}

inline mmcStatementInfo *mmcStatement::getInfo()
{
    return &mInfo;
}

inline mmcSession *mmcStatement::getSession()
{
    return mSession;
}

/* This returns a pointer pointing to mAuditTrail, 
 * one of the members of mmcStatement.
 */
inline idvAuditTrail *mmcStatement::getAuditTrail()
{
    return &mAuditTrail;
}

inline mmcTransObj *mmcStatement::allocTrans()
{
    if (mTrans == NULL)
    {
        IDE_ASSERT(mmcTrans::alloc(NULL, &mTrans) == IDE_SUCCESS);
    }

    return mTrans;
}

inline mmcTransObj *mmcStatement::getTransPtr()
{
    return mTrans;
}

inline void mmcStatement::setTrans( mmcTransObj * aTrans )
{
    mTrans = aTrans;
}

inline smiStatement *mmcStatement::getSmiStmt()
{
    return mSmiStmtPtr;
}

inline qciStatement *mmcStatement::getQciStmt()
{
    return &mQciStmt;
}

inline qciStmtType mmcStatement::getStmtType()
{
    return mStmtType;
}

inline mmcStmtBindState mmcStatement::getBindState()
{
    return mBindState;
}

inline void mmcStatement::setBindState(mmcStmtBindState aBindState)
{
    mBindState = aBindState;
}

inline iduListNode *mmcStatement::getStmtListNodeForParent()
{
    return &mStmtListNodeForParent;
}

inline iduListNode *mmcStatement::getStmtListNode()
{
    return &mStmtListNode;
}

inline iduListNode *mmcStatement::getFetchListNode()
{
    return &mFetchListNode;
}

inline iduVarString *mmcStatement::getPlanString()
{
    return &mPlanString;
}

inline idBool mmcStatement::getSendBindColumnInfo()
{
    return mSendBindColumnInfo;
}

inline void mmcStatement::setSendBindColumnInfo(idBool aSendBindColumnInfo)
{
    mSendBindColumnInfo = aSendBindColumnInfo;
}

inline idBool mmcStatement::getTimeoutEventOccured()
{
    return mTimeoutEventOccured;
}

inline void mmcStatement::setTimeoutEventOccured( idBool aTimeoutEventOccured )
{
    mTimeoutEventOccured = aTimeoutEventOccured;
}

inline idBool mmcStatement::getAtomicExecSuccess()
{
    return mAtomicInfo.mAtomicExecuteSuccess;
}

inline void mmcStatement::setAtomicExecSuccess(idBool aAtomicExecuteSuccess)
{
    mAtomicInfo.mAtomicExecuteSuccess = aAtomicExecuteSuccess;
}

// BUG-21489
inline void mmcStatement::clearAtomicLastErrorCode()
{
    mAtomicInfo.mAtomicLastErrorCode = idERR_IGNORE_NoError;
}

inline void mmcStatement::setAtomicLastErrorCode(UInt aErrorCode)
{
    SChar * sErrorMsg = NULL;

    // 1개의 에러 코드만 저장하도록 한다.
    if(mAtomicInfo.mAtomicLastErrorCode == idERR_IGNORE_NoError)
    {
        mAtomicInfo.mAtomicLastErrorCode = aErrorCode;

        sErrorMsg = ideGetErrorMsg(aErrorCode);

        idlOS::memcpy(mAtomicInfo.mAtomicErrorMsg, sErrorMsg, (MAX_ERROR_MSG_LEN+256));
    }
}

inline mmcAtomicInfo *mmcStatement::getAtomicInfo()
{
    return &mAtomicInfo;
}

inline mmcStmtID mmcStatement::getStmtID()
{
    return mInfo.mStatementID;
}

inline mmcSessID mmcStatement::getSessionID()
{
    return mInfo.mSessionID;
}

inline mmcStatement* mmcStatement::getParentStmt()
{
    return mInfo.mParentStmt;
}

inline smTID mmcStatement::getTransID()
{
    return mInfo.mTransID;
}

inline void mmcStatement::setTransID(smTID aTransID)
{
    mInfo.mTransID = aTransID;
}

inline mmcStmtExecMode mmcStatement::getStmtExecMode()
{
    return mInfo.mStmtExecMode;
}

inline void mmcStatement::setStmtExecMode(mmcStmtExecMode aStmtExecMode)
{
    mInfo.mStmtExecMode = aStmtExecMode;
}

inline mmcStmtState mmcStatement::getStmtState()
{
    return mInfo.mStmtState;
}

inline void mmcStatement::setStmtState(mmcStmtState aStmtState)
{
    mInfo.mStmtState = aStmtState;
}

inline UShort mmcStatement::getResultSetCount()
{
    return mInfo.mResultSetCount;
}

inline void mmcStatement::setResultSetCount(UShort aResultSetCount)
{
    mInfo.mResultSetCount = aResultSetCount;
}

inline UShort mmcStatement::getEnableResultSetCount()
{
    return mInfo.mEnableResultSetCount;
}

inline void mmcStatement::setEnableResultSetCount(UShort aEnableResultSetCount)
{
    mInfo.mEnableResultSetCount = aEnableResultSetCount;
}

inline UShort mmcStatement::getResultSetHWM()
{
    return mInfo.mResultSetHWM;
}

inline void mmcStatement::setResultSetHWM(UShort aResultSetHWM)
{
    mInfo.mResultSetHWM = aResultSetHWM;
}

inline mmcResultSetState mmcStatement::getResultSetState(UShort aResultSetID)
{
    if(aResultSetID >= mInfo.mResultSetCount)
    {
        return MMC_RESULTSET_STATE_ERROR;
    }

    return mInfo.mResultSet[aResultSetID].mResultSetState;
}

inline IDE_RC mmcStatement::setResultSetState(UShort aResultSetID, mmcResultSetState aResultSetState)
{
    if(aResultSetID >= mInfo.mResultSetCount)
    {
        return IDE_FAILURE;
    }

    mInfo.mResultSet[aResultSetID].mResultSetState = aResultSetState;

    return IDE_SUCCESS;
}

inline mmcResultSet* mmcStatement::getResultSet( UShort aResultSetID )
{
    if(aResultSetID >= mInfo.mResultSetCount)
    {
        return NULL;
    }

    if( mInfo.mResultSet == NULL )
    {
        return NULL;
    }
    else
    {
        return &mInfo.mResultSet[aResultSetID];
    }
}

inline mmcFetchFlag mmcStatement::getFetchFlag()
{
    return mInfo.mFetchFlag;
}

inline void mmcStatement::setFetchFlag(mmcFetchFlag aFetchFlag)
{
    mInfo.mFetchFlag = aFetchFlag;
}

inline SChar *mmcStatement::getQueryString()
{
    return mInfo.mQueryString;
}

inline UInt mmcStatement::getQueryLen()
{
    return mInfo.mQueryLen;
}

inline void mmcStatement::setQueryString(SChar *aQuery, UInt aQueryLen)
{
    lockQuery();

    if (mInfo.mQueryString != NULL)
    {
        // fix BUG-28267 [codesonar] Ignored Return Value
        IDE_ASSERT(iduMemMgr::free(mInfo.mQueryString) == IDE_SUCCESS);
    }

    mInfo.mQueryString = aQuery;
    mInfo.mQueryLen    = aQueryLen;

    unlockQuery();
}

inline UInt mmcStatement::getCursorFlag()
{
    return mInfo.mCursorFlag;
}

inline void mmcStatement::setCursorFlag(UInt aCursorFlag)
{
    mInfo.mCursorFlag = aCursorFlag;
}

inline idBool mmcStatement::isArray()
{
    return mInfo.mIsArray;
}

inline void mmcStatement::setArray(idBool aArrayFlag)
{
    mInfo.mIsArray = aArrayFlag;
}

inline idBool mmcStatement::isAtomic()
{
    return mInfo.mIsAtomic;
}

inline void mmcStatement::setAtomic(idBool aAtomicFlag)
{
    mInfo.mIsAtomic = aAtomicFlag;
}

inline UInt mmcStatement::getRowNumber()
{
    return mInfo.mRowNumber;
}

inline void mmcStatement::setRowNumber(UInt aRowNumber)
{
    mInfo.mRowNumber = aRowNumber;
}

// BUG-41452
inline UInt mmcStatement::getTotalRowNumber()
{
    return mInfo.mTotalRowNumber;
}

inline void mmcStatement::setTotalRowNumber(UInt aTotalRowNumber)
{
    mInfo.mTotalRowNumber = aTotalRowNumber;
}

inline idBool mmcStatement::isStmtBegin()
{
    return mInfo.mIsStmtBegin;
}

inline void mmcStatement::setStmtBegin(idBool aBeginFlag)
{
    mInfo.mIsStmtBegin = aBeginFlag;
}

inline idBool mmcStatement::getExecuteFlag()
{
    return mInfo.mExecuteFlag;
}

inline void mmcStatement::setExecuteFlag(idBool aExecuteFlag)
{
    mInfo.mExecuteFlag = aExecuteFlag;
}

inline UInt mmcStatement::getLastQueryStartTime()
{
    return mInfo.mLastQueryStartTime;
}

inline UInt mmcStatement::getQueryStartTime()
{
    return mInfo.mQueryStartTime;
}

inline void mmcStatement::setQueryStartTime(UInt aQueryStartTime)
{
    mInfo.mQueryStartTime = aQueryStartTime;

    if (aQueryStartTime > 0)
    {
        mInfo.mLastQueryStartTime = aQueryStartTime;
    }
}

inline UInt mmcStatement::getFetchStartTime()
{
    return mInfo.mFetchStartTime;
}

inline void mmcStatement::setFetchStartTime(UInt aFetchStartTime)
{
    mInfo.mFetchStartTime = aFetchStartTime;
}

/* BUG-19456 */
inline UInt mmcStatement::getFetchEndTime()
{
    return mInfo.mFetchEndTime;
}
/* BUG-19456 */
inline void mmcStatement::setFetchEndTime(UInt aFetchEndTime)
{
    mInfo.mFetchEndTime = aFetchEndTime;
}

inline idvSQL *mmcStatement::getStatistics()
{
    return &mInfo.mStatistics;
}

inline idBool mmcStatement::isRootStmt()
{
    if( mInfo.mParentStmt != NULL )
    {
        return ID_FALSE;
    }
    else
    {
        return ID_TRUE;
    }
}

inline mmcStatement * mmcStatement::getRootStmt()
{
    mmcStatement    * sStatement = NULL;

    sStatement = this;

    while ( sStatement->isRootStmt() != ID_TRUE )
    {
        sStatement = sStatement->getParentStmt();
    }

    return sStatement;
}

inline iduList* mmcStatement::getChildStmtList()
{
    return &mChildStmtList;
}

// bug-23991: prepared 후 execute 마다 PVO time 값이 그대로 남음
inline idBool mmcStatement::getPreparedTimeExist()
{
    return mInfo.mPreparedTimeExist;
}

inline void mmcStatement::setPreparedTimeExist(idBool aPreparedTimeExist)
{
    mInfo.mPreparedTimeExist = aPreparedTimeExist;
}

//fix BUG-24364 valgrind direct execute으로 수행시킨 statement의  plan cache정보를 reset시켜야함.
inline void mmcStatement::releasePlanCacheObject()
{
    mInfo.mSQLPlanCacheTextId = (SChar*)(mmcStatement::mNoneSQLCacheTextID);
    mInfo.mSQLPlanCachePCOId  = 0;
    mPCB->planUnFix(getStatistics());
    mPCB = NULL;
}

/*
 * [BUG-24187] Rollback될 statement는 Internal CloseCurosr를
 * 수행할 필요가 없습니다.
 */
inline void mmcStatement::setSkipCursorClose()
{
    smiStatement::setSkipCursorClose(mSmiStmtPtr);
}

// PROJ-2256 Communication protocol for efficient query result transmission
inline void mmcStatement::initBaseRow( mmcBaseRow *aBaseRow )
{
    aBaseRow->mBaseRow       = NULL;
    aBaseRow->mBaseRowSize   = 0;
    aBaseRow->mBaseColumnPos = 0;
    aBaseRow->mCompressedSize4CurrentRow = 0;
    aBaseRow->mIsFirstRow    = ID_TRUE;
    aBaseRow->mIsRedundant   = ID_FALSE;
}

inline IDE_RC mmcStatement::allocBaseRow( mmcBaseRow *aBaseRow, UInt aBaseRowSize )
{
    if ( aBaseRow->mIsFirstRow == ID_TRUE )
    {
        if ( aBaseRow->mBaseRowSize < aBaseRowSize )
        {
            aBaseRow->mBaseRowSize = idlOS::align8( aBaseRowSize );

            if ( aBaseRow->mBaseRow != NULL )
            {
                IDU_FIT_POINT_RAISE("mmcStatement::allocBaseRow::realloc::BaseRow", InsufficientMemory );
                IDE_TEST_RAISE( iduMemMgr::realloc( IDU_MEM_MMC,
                                                    aBaseRow->mBaseRowSize,
                                                    (void **)&( aBaseRow->mBaseRow ) ) 
                                != IDE_SUCCESS, InsufficientMemory );
            }
            else
            {
                IDU_FIT_POINT_RAISE("mmcStatement::allocBaseRow::malloc::BaseRow", InsufficientMemory );
                IDE_TEST_RAISE( iduMemMgr::malloc( IDU_MEM_MMC,
                                                   aBaseRow->mBaseRowSize,
                                                   (void **)&( aBaseRow->mBaseRow ) )
                                != IDE_SUCCESS, InsufficientMemory );
            }
        }
        else
        {
            /* do nothing */
        }
    }
    else
    {
        /* do nothing */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( InsufficientMemory );
    {
        IDE_SET( ideSetErrorCode( idERR_ABORT_InsufficientMemory ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

inline IDE_RC mmcStatement::freeBaseRow( mmcBaseRow *aBaseRow )
{

    if ( aBaseRow->mBaseRow != NULL )
    {
        IDE_TEST(  iduMemMgr::free( aBaseRow->mBaseRow ) );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* PROJ-2177 User Interface - Cancel */

/**
 * EXECUTE 중인지 여부를 확인한다.
 *
 * @return EXECUTE 중이면 ID_TRUE, 아니면 ID_FALSE
 */
inline idBool mmcStatement::isExecuting()
{
    return mInfo.mIsExecuting;
}

/**
 * EXECUTE 중인지 여부를 설정한다.
 *
 * @param aExecuting EXECUTE 중인지 여부
 */
inline void mmcStatement::setExecuting(idBool aIsExecuting)
{
    mInfo.mIsExecuting = aIsExecuting;
}

/**
 * StmtCID를 얻는다.
 *
 * @return StmtCID. 설정된 값이 없으면 MMC_STMT_CID_NONE
 */
inline mmcStmtCID mmcStatement::getStmtCID()
{
    return mInfo.mStmtCID;
}

/**
 * StmtCID를 설정한다.
 *
 * @param aStmtCID StmtCID
 */
inline void mmcStatement::setStmtCID(mmcStmtCID aStmtCID)
{
    mInfo.mStmtCID = aStmtCID;
}

/* PROJ-1381 Fetch Across Commits */

/**
 * Cursor holdability를 설정한다.
 *
 * @param aCursorHold[IN] Cursor holdability
 */
inline void mmcStatement::setCursorHold(mmcStmtCursorHold aCursorHold)
{
    mInfo.mCursorHold = aCursorHold;
}

/**
 * Cursor holdability 설정을 얻는다.
 *
 * @return Cursor holdability. MMC_STMT_CURSOR_HOLD_ON or MMC_STMT_CURSOR_HOLD_OFF
 */
inline mmcStmtCursorHold mmcStatement::getCursorHold(void)
{
    return mInfo.mCursorHold;
}

/* PROJ-1789 Updatalbe Scrollable Cursor */

/**
 * Keyset mode 설정을 얻는다.
 *
 * @return Keyset mode
 */
inline mmcStmtKeysetMode mmcStatement::getKeysetMode(void)
{
    return mInfo.mKeysetMode;
}

/**
 * Keyset mode를 설정한다.
 *
 * @param[in] aKeysetMode Keyset mode
 */
inline void mmcStatement::setKeysetMode(mmcStmtKeysetMode aKeysetMode)
{
    mInfo.mKeysetMode = aKeysetMode;
}

/* TASK-7219 Non-shard DML */
/**
 * shard partial execution type을 얻는다.
 *
 * @return mShardPartialExecType
 */
inline sdiShardPartialExecType mmcStatement::getShardPartialExecType(void)
{
    return mInfo.mShardPartialExecType;
}

/**
 *  * shard partial execution type을 설정한다.
 *
 * @param[in] aShardPartialExecType
 */
inline void mmcStatement::setShardPartialExecType( sdiShardPartialExecType aShardPartialExecType )
{
    mInfo.mShardPartialExecType = aShardPartialExecType;
}

/**
 * SimpleQuery 상태를 설정한다. 이 값은 qci에서 받는다.
 *
 * @param[in] aIsSimpleQuery  : state of SimpleQuery
 */
inline void mmcStatement::setSimpleQuery(idBool aIsSimpleQuery)
{
    mInfo.mIsSimpleQuery = aIsSimpleQuery;
}

/**
 * SimpleQuery 상태를 가져온다.
 *
 * @return mIsSimpleQuery
 */
inline idBool mmcStatement::isSimpleQuery()
{
    return mInfo.mIsSimpleQuery;
}

/**
 * FetchEnd 때 커서를 닫아도 되는지 여부를 얻는다.
 *
 * @return FetchEnd 때 커서를 닫아도 되는지 여부
 */
inline idBool mmcStatement::isClosableWhenFetchEnd(void)
{
    idBool sIsClosable = ID_FALSE;

    if ( (getCursorHold() != MMC_STMT_CURSOR_HOLD_ON)
     &&  (getKeysetMode() != MMC_STMT_KEYSETMODE_ON) )
    {
        sIsClosable = ID_TRUE;
    }

    return sIsClosable;
}

/* PROJ-2223 Altibase Auditing */
inline UInt mmcStatement::getExecuteResult( mmcExecutionFlag aExecutionFlag )
{
    UInt sExecuteResult;

    if( aExecutionFlag == MMC_EXECUTION_FLAG_FAILURE )
    {
        sExecuteResult = E_ERROR_CODE(ideGetErrorCode());
    }
    else
    {
        sExecuteResult = E_ERROR_CODE(idERR_IGNORE_NoError);
    }

    return sExecuteResult;
}

inline UInt mmcStatement::getAuditObjectCount( void )
{
    return mAuditObjectCount;
}

inline qciAuditRefObject *mmcStatement::getAuditObjects( void )
{
    return mAuditObjects;
}

inline void mmcStatement::resetStatistics()
{
    idvManager::resetSQL(getStatistics());
}

/* BUG-43113 Autonomous Transaction */
inline void mmcStatement::setSmiStmtForAT( smiStatement * aSmiStmt )
{
    mSmiStmtPtr = aSmiStmt;
}

inline void mmcStatement::setShardQueryType( sdiQueryType aQueryType )
{
    mInfo.mShardQueryType = (UInt)aQueryType;
}

inline void * mmcStatement::getShardStatement()
{
    return (void *)&mSdStmt;
}

/* PROJ-2733-DistTxInfo */
inline void mmcStatement::getRequestSCN(smSCN *aSCN)
{
    SM_GET_SCN(aSCN, &mInfo.mGCTxStmtInfo.mRequestSCN);
}
inline void mmcStatement::setRequestSCN(smSCN *aSCN)
{
    SM_SET_SCN(&mInfo.mGCTxStmtInfo.mRequestSCN, aSCN);
}

inline void mmcStatement::getTxFirstStmtSCN(smSCN *aTxFirstStmtSCN)
{
    IDE_DASSERT( mInfo.mGCTxStmtInfo.mRoot != NULL );

    if ( mInfo.mGCTxStmtInfo.mRoot != NULL )
    {
        SM_GET_SCN( aTxFirstStmtSCN, &mInfo.mGCTxStmtInfo.mRoot->mTxFirstStmtSCN );
    }
    else
    {
        SM_INIT_SCN( aTxFirstStmtSCN );
    }
}
inline void mmcStatement::setTxFirstStmtSCN(smSCN *aTxFirstStmtSCN)
{
    IDE_DASSERT( mInfo.mGCTxStmtInfo.mRoot != NULL );

    if ( mInfo.mGCTxStmtInfo.mRoot != NULL )
    {
        SM_SET_SCN( &mInfo.mGCTxStmtInfo.mRoot->mTxFirstStmtSCN, aTxFirstStmtSCN );
    }
    else
    {
        /* Nothing to do */
    }
}

inline SLong mmcStatement::getTxFirstStmtTime()
{
    SLong sRet = 0;

    IDE_DASSERT( mInfo.mGCTxStmtInfo.mRoot != NULL );

    if ( mInfo.mGCTxStmtInfo.mRoot != NULL )
    {
        sRet = mInfo.mGCTxStmtInfo.mRoot->mTxFirstStmtTime;
    }
    else
    {
        /* Nothing to do */
    }

    return sRet;
}
inline void mmcStatement::setTxFirstStmtTime(SLong aTxFirstStmtTime)
{
    IDE_DASSERT( mInfo.mGCTxStmtInfo.mRoot != NULL );

    if ( mInfo.mGCTxStmtInfo.mRoot != NULL )
    {
        mInfo.mGCTxStmtInfo.mRoot->mTxFirstStmtTime = aTxFirstStmtTime;
    }
    else
    {
        /* Nothing to do */
    }
}

inline sdiDistLevel mmcStatement::getDistLevel()
{
    sdiDistLevel sRet = SDI_DIST_LEVEL_INIT;

    IDE_DASSERT( mInfo.mGCTxStmtInfo.mRoot != NULL );

    if ( mInfo.mGCTxStmtInfo.mRoot != NULL )
    {
        sRet = mInfo.mGCTxStmtInfo.mRoot->mDistLevel;
    }
    else
    {
        /* Nothing to do */
    }

    return sRet;
}
inline void mmcStatement::setDistLevel(sdiDistLevel aDistLevel)
{
    IDE_DASSERT( mInfo.mGCTxStmtInfo.mRoot != NULL );

    if ( mInfo.mGCTxStmtInfo.mRoot != NULL )
    {
        mInfo.mGCTxStmtInfo.mRoot->mDistLevel = aDistLevel;
    }
    else
    {
        /* Nothing to do */
    }
}

inline void mmcStatement::clearGCTxStmtInfo()
{
    SM_INIT_SCN( &mInfo.mGCTxStmtInfo.mRequestSCN );

    SM_INIT_SCN( &mInfo.mGCTxStmtInfo.mShareBuffer.mTxFirstStmtSCN );
    mInfo.mGCTxStmtInfo.mShareBuffer.mTxFirstStmtTime = 0;
    mInfo.mGCTxStmtInfo.mShareBuffer.mDistLevel       = SDI_DIST_LEVEL_INIT;
}

inline mmcGCTxStmtShareInfo * mmcStatement::getGCTxStmtShareInfoBuffer()
{
    return &mInfo.mGCTxStmtInfo.mShareBuffer;
}

inline void mmcStatement::linkGCTxStmtInfo()
{
    if ( isRootStmt() == ID_TRUE )
    {
        mInfo.mGCTxStmtInfo.mRoot = getGCTxStmtShareInfoBuffer();
    }
    else
    {
        mInfo.mGCTxStmtInfo.mRoot = getParentStmt()->getGCTxStmtShareInfoBuffer();
    }
}

inline void mmcStatement::unlinkGCTxStmtInfo()
{
    mInfo.mGCTxStmtInfo.mRoot = NULL;
}

inline void mmcStatement::addRebuildCount()
{
    ++mInfo.mRebuildCount;
}

inline idBool mmcStatement::isNeedRequestSCN()
{
    return ( ( isRootStmt() == ID_TRUE ) &&
             ( sdi::isNeedRequestSCN( getQciStmt() ) == ID_TRUE ) )
           ? ID_TRUE
           : ID_FALSE;
}
#endif
