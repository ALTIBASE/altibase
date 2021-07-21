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
 * $Id: rpxSender.h 91226 2021-07-14 08:01:24Z lswhh $
 **********************************************************************/

#ifndef _O_RPX_SENDER_H_
#define _O_RPX_SENDER_H_ 1

#include <idl.h>
#include <idtBaseThread.h>

#include <cm.h>
#include <smrDef.h>
#include <qci.h>
#include <rp.h>
#include <rpdLogAnalyzer.h>
#include <rpdMeta.h>
#include <rpdTransTbl.h>
#include <rpuProperty.h>
#include <rpnComm.h>
#include <rpnMessenger.h>
#include <rpdSenderInfo.h>
#include <rpxSenderApply.h>
#include <rpdLogBufferMgr.h>

#define RPX_SENDER_SVP_NAME           "$$RPX_SENDER_SVPNAME"
#define RPX_INVALID_SENDER_INDEX      (UINT_MAX)


typedef enum
{
    RP_SLEEP_RETRY = 0,
    RP_SLEEP_CPU
} RP_SLEEP_TYPE;

typedef enum
{
    RP_LOG_MGR_REDO,
    RP_LOG_MGR_ARCHIVE
} RP_LOG_MGR_STATUS;

typedef enum
{
    RP_LOG_MGR_INIT_NONE = 0,
    RP_LOG_MGR_INIT_FAIL,
    RP_LOG_MGR_INIT_SUCCESS
} RP_LOG_MGR_INIT_STATUS;

typedef struct rpxSyncItem
{
    iduListNode   mNode;
    rpdMetaItem * mTable;
    ULong         mSyncedTuples;
} rpxSyncItem;

typedef struct rpxSentLogCount
{
    ULong               mTableOID;
    
    UInt                mInsertLogCount;
    UInt                mUpdateLogCount;
    UInt                mDeleteLogCount;
    UInt                mLOBLogCount;

} rpxSentLogCount;

#include <rpxReplicator.h>

/*
//----------------------------------------------------------------//
// Keep Alive Check Time
//  = RP_KEEP_ALIVE_CNT * 10 micro second * alpha(??)
//----------------------------------------------------------------//
#define RP_KEEP_ALIVE_CNT ( 600 )
*/

class smiStatement;
class rpxSender;
class rpxPJMgr;
class rpnMessenger;

class rpxSender : public idtBaseThread
{
public:
    rpxSender();
    virtual ~rpxSender() {};

    IDE_RC initialize(smiStatement    * aSmiStmt,
                      SChar           * aRepName,
                      RP_SENDER_TYPE    aStartType,
                      idBool            aTryHandshakeOnce,
                      idBool            aMetaForUpdateFlag,
                      SInt              aParallelFactor,
                      qciSyncItems    * aSyncItemList,
                      rpdSenderInfo   * aSenderInfoArray,
                      rpdLogBufferMgr * aLogBufMgr,
                      rprSNMapMgr     * aSNMapMgr,
                      smSN              aActiveRPRecoverySN,
                      rpdMeta         * aMeta,
                      UInt              aParallelID,
                      UInt              aSenderListIndex );

    void   destroy();
    void   shutdown();

    /* BUG-38533 numa aware thread initialize */
    IDE_RC initializeThread();
    void   finalizeThread();

    idBool isYou(const SChar * aRepName );

    IDE_RC attemptHandshake(idBool *aHandshakeFlag);    // 연결 시도
    void   releaseHandshake();                          // 연결 해제
 
    IDE_RC handshakeWithoutReconnect( smTID aTID );

    void   run();

    IDE_RC time_lock()   { return mTimeMtxRmt.lock(NULL /* idvSQL* */); }
    IDE_RC time_unlock() { return mTimeMtxRmt.unlock(); }

    IDE_RC threadJoinMutex_lock()    { return mThreadJoinMutex.lock(NULL /* idvSQL* */); }
    IDE_RC threadJoinMutex_unlock()  { return mThreadJoinMutex.unlock(); }

    IDE_RC wakeup() { return (mTimeCondRmt.signal() == IDE_SUCCESS)
                             ? IDE_SUCCESS : IDE_FAILURE; }

    idBool isExit() { return mExitFlag; }
    void setExitFlag( void );

    static idBool isSyncItem(rpdReplSyncItem *aSyncItems,
                             const SChar  *aUsername,
                             const SChar  *aTablename,
                             const SChar  *aPartname);

    static IDE_RC getKeyRange(smOID               aTableOID,
                              smiValue           *aColArray,
                              smiRange           *aKeyRange,
                              qtcMetaRangeColumn *aRangeColumn);

    static IDE_RC makeFetchColumnList(const smOID          aTableOID,
                                      smiFetchColumnList * aFetchColumnList);

    static RP_META_BUILD_TYPE getMetaBuildType( RP_SENDER_TYPE   aStartType,
                                                UInt             aParallelID );


    //for repl sync parallel
    SInt           getParallelFactor() { return mParallelFactor; };
    idBool         getCompleteFlag()   { return mStartComplete; };
    
    // BUG-35160 wait until mStartComplete is true
    IDE_RC         waitStartComplete( idvSQL * aStatistics );


    smiStatement *mSvcThrRootStmt;
    smiStatement  mPJStmt;

    // for HBT
    void         *mRsc;

    inline void          getSendXSN( smSN *aSN )
        { *aSN = mXSN; }
    inline rpdMeta *     getMeta()
        { return &mMeta    ; }
    inline SInt          getMode()
        { return mMeta.mReplication.mReplMode;}
    inline UInt          getMetaItemCount()
        { return mMeta.mReplication.mItemCount; }
    inline rpdMetaItem * getMetaItem(UInt aIdx)
        { return ( mMeta.mItems ) ? &mMeta.mItems[aIdx] : NULL; }
    inline SInt          getRole()
        { return mMeta.mReplication.mRole; }

    void getAllLFGReadLSN( smLSN * aArrReadLSN );

    idBool isLogMgrInit( void );
    RP_LOG_MGR_INIT_STATUS getLogMgrInitStatus();

    inline idBool        isParallelParent()
    {
        return ((mCurrentType == RP_PARALLEL)&&
                (mParallelID == RP_PARALLEL_PARENT_ID)) ? ID_TRUE : ID_FALSE;
    }
    inline idBool        isParallelChild()
    {
        return ((mCurrentType == RP_PARALLEL)&&
                (mParallelID != RP_PARALLEL_PARENT_ID)) ? ID_TRUE : ID_FALSE;
    }

    inline void          setStatus(RP_SENDER_STATUS aStatus)
        { mStatus = aStatus;
          mSenderInfo->setSenderStatus(aStatus);}
    
    inline RP_SENDER_STATUS getStatus()
    {
        return mStatus;
    }

    inline rpdSenderInfo * getSenderInfo()
        { return mSenderInfo; }

    inline idvSession * getSenderStatSession()
    {
        return &mStatSession;
    }

    //BUG-19970 호출 전 PJ_lock잡아야 함
    ULong getJobCount( SChar *aTableName );

    smSN getRmtLastCommitSN();
    smSN getLastProcessedSN();
    smSN getRestartSN();
    void setRestartSN(smSN aSN);
    smSN getNextRestartSN();
    void setCommitXSNForFileEndLog(smSN aFileEndSN);
    smSN getLastArrivedSN();
    
    void getMinRestartSNFromAllApply( smSN* aRestartSN );

    // BUG-29115
    // checkpoint시 archive log로 전환해야할지 결정한다.
    void checkAndSetSwitchToArchiveLogMgr(const UInt  * aLastArchiveFileNo,
                                          idBool      * aSetLogMgrSwitch);

     // LFG가 1이고 archive log를 읽을 수 있는 ALA인가.
    inline idBool isArchiveALA()
        {
            if ( ( ( getRole() == RP_ROLE_ANALYSIS ) ||
                   ( getRole() == RP_ROLE_ANALYSIS_PROPAGATION ) ) &&
                 ( RPU_REPLICATION_LOG_BUFFER_SIZE == 0 ) &&
                 ( smiGetArchiveMode() == SMI_LOG_ARCHIVE ) )
            {
                return ID_TRUE;
            }
            else
            {
                return ID_FALSE;
            }
        }
                    
    /* BUG-31545 통계 정보를 시스템에 반영한다. */
    inline void applyStatisticsToSystem()
    {
        idvManager::applyStatisticsToSystem(&mStatSession, &mOldStatSession);
    }

    void getLocalAddress( SChar ** aMyIP,
                          SInt * aMyPort );

    void getRemoteAddress( SChar ** aPeerIP,
                           SInt   * aPeerPort );

    void getRemoteAddressForIB( SChar      ** aPeerIP,
                                SInt        * aPeerPort,
                                rpIBLatency * aIBLatency );

    IDE_RC    addXLogKeepAlive();

    RP_INTR_LEVEL checkInterrupt();

    IDE_RC findAndUpdateInvalidMaxSN( smiStatement    * aSmiStmt,
                                      rpdReplSyncItem * aSyncList,
                                      rpdReplItems    * aReplItem,
                                      smSN              aSN);

    IDE_RC  updateInvalidMaxSN(smiStatement * aSmiStmt,
                               rpdReplItems * aReplItems,
                               smSN aSN);

    ULong getReadLogCount( void );
    ULong getSendLogCount( void );

    void          setNetworkErrorAndDeactivate( void );
    void          setRestartErrorAndDeactivate( void );
    IDE_RC        checkHBTFault( void );

    IDE_RC buildRecordsForSentLogCount(
        idvSQL              * /*aStatistics*/,
        void                * aHeader,
        void                * /* aDumpObj */,
        iduFixedTableMemory * aMemory );

    IDE_RC buildRecordForReplicatedTransGroupInfo(  void                * aHeader,
                                                    void                * aDumpObj,
                                                    iduFixedTableMemory * aMemory );

    IDE_RC buildRecordForReplicatedTransSlotInfo( void                * aHeader,
                                                  void                * aDumpObj,
                                                  iduFixedTableMemory * aMemory );

    IDE_RC buildRecordForAheadAnalyzerInfo( void                * aHeader,
                                            void                * aDumpObj,
                                            iduFixedTableMemory * aMemory );

    void increaseLogCount( rpXLogType aType, smOID aTableOID );
    void rebuildSentLogCount( void );
    IDE_RC updateSentLogCount( smOID aNewTableOID,
                               smOID aOldTableOID );

    IDE_RC sendDDLASyncStart( UInt aType );

    IDE_RC recvDDLASyncStartAck( UInt * aType );

    IDE_RC sendDDLASyncExecute( UInt    aType,
                                SChar * aUserName,
                                UInt    aDDLEnableLevel,
                                UInt    aTargetCount,
                                SChar * aTargetTableName,
                                SChar * aTargetPartNames,
                                smSN    aDDLCommitSN,
                                SChar * aDDLStmt );

    IDE_RC recvDDLASyncExecuteAck( UInt  * aType,
                                   UInt  * aIsSuccess,
                                   UInt  * aErrCode,
                                   SChar * aErrMsg );

    IDE_RC getTargetNamesFromItemMetaEntry( smTID    aTID,
                                            UInt   * aTargetCount,
                                            SChar  * aTargetTableName,
                                            SChar ** aTargetPartNames );

    IDE_RC getDDLInfoFromDDLStmtLog( smTID   aTID,
                                     SInt    aMaxDDLStmtLen,
                                     SChar * aUserName,
                                     SChar * aDDLStmt );

    idBool isSuspendedApply( void );

    void   resumeApply( void );

    IDE_RC checkAndErrorConditionalStart( );
    IDE_RC checkAndSetConditionalSync( );


    IDE_RC checkErrorWithConditionActInfo( rpdConditionActInfo * aConditionInfo, 
                                           UInt                  aItemCount );

    IDE_RC makeSyncItemsWithConditionActInfo( rpdConditionActInfo * aConditionInfo, 
                                              UInt                  aItemCount,
                                              RP_CONDITION_ACTION   aAction,
                                              rpdReplSyncItem       ** aItemList );

    IDE_RC truncateStart();

private:
    IDE_RC    initXSN( smSN aReceiverXSN );
    void      initReadLogCount();
    void      finalize();
    IDE_RC    execOnceAtStart();
    IDE_RC    prepareForRunning();
    IDE_RC    prepareForParallel();
    IDE_RC    prepareForConsistent();
    IDE_RC    xlogfileFailbackMaster();
    IDE_RC    xlogfileFailbackSlave();
    void      cleanupForParallel();
    void      initializeAssignedTransTbl();
    void      initializeServiceTrans();

    IDE_RC    quickStart();
    IDE_RC    syncStart();
    IDE_RC    syncALAStart();

    IDE_RC    connectPeer(SInt aIndex);
    IDE_RC    checkReplAvailable(rpMsgReturn *aRC, 
                                 SInt        *aFailbackStatus,
                                 smSN        *aReceiverXSN);
    void      disconnectPeer();

    IDE_RC    doReplication();
    //IDE_RC    makeXLog( smiLogRec * aLogRec);

    /* Control 관련 XLog 전송 */
    IDE_RC    addXLogHandshake( smTID aTID );

    /* Incremental Sync 관련 XLog 전송 */
    IDE_RC    addXLogSyncPKBegin();
    IDE_RC    addXLogSyncPKEnd();
    IDE_RC    addXLogFailbackEnd();

    IDE_RC    getNextLastUsedHostNo( SInt *aIndex = NULL );

    IDE_RC    addXLogSyncRow(rpdSyncPKEntry *aSyncPKEntry);
    IDE_RC    addXLogSyncCommit();
    IDE_RC    addXLogSyncAbort();
    IDE_RC    syncRow(rpdMetaItem *aMetaItem,
                      smiValue    *aPKCols);

    /* PROJ-1442 Replication Online 중 DDL 허용
     * Meta 구성 및 반영
     */
    IDE_RC    buildMeta(smiStatement  * aSmiStmt,
                        SChar         * aRepName,
                        RP_SENDER_TYPE  aStartType,
                        idBool          aMetaForUpdateFlag,
                        rpdMeta       * aMeta);

    IDE_RC rebuildWithUpdateOldMeta( smiStatement     * aSmiStmt,
                                     SChar            * aRepName,
                                     idBool             aMetaForUpdateFlag,
                                     rpdMeta          * aMeta,
                                     rpdReplSyncItem  * aSyncItemList );

    // Replication의 Minimum XSN을 변경한다.
    IDE_RC  updateXSN(smSN aSN);

    void    sleepForNextConnect();
    void    checkXSNAndSleep();
    void    sleepForSenderSleepTime();
    void    updateOldMaxXSN();

    IDE_RC  final_lock()  { return mFinalMtx.lock(NULL /* idvSQL* */); }
    IDE_RC  final_unlock(){ return mFinalMtx.unlock();}

    void    checkValidityOfXLSN(void      *a_pLogFile,
                                smLSN     *a_pLSN);


    IDE_RC  startSenderApply();
    void    shutdownSenderApply();
    void    finalizeSenderApply();
    //proj-1608 update invalid recovery
    IDE_RC  updateInvalidRecovery(SChar* aRepName, SInt aValue);

    /*PROJ-2067 parallel sender*/
    IDE_RC        doRunning();
    void          changeSndrType();
    IDE_RC        createNStartChildren();
    void          shutdownNDestroyChildren();
    IDE_RC        failbackNormal();
    IDE_RC        failbackMaster();
    IDE_RC        failbackSlave();
    
    IDE_RC        incrementalSyncMaster();
    IDE_RC        removeReplGapOnLazyMode();

    inline UInt   makeChildID(UInt aChildIdxNum){ return aChildIdxNum + 1; }

    idBool        isFailbackComplete(smSN aLastSN);

    /* Sync Parallel */
    IDE_RC        sendSyncStart();
    IDE_RC        syncParallel();
    IDE_RC        allocSCN( smiStatement ** aParallelStatements,
	                        smiTrans     ** aSyncParallelTrans );
    void          destroySCN( smiStatement * aParallelStatements,
	                          smiTrans     * aSyncParallelTrans );
    IDE_RC        lockTableforSync( smiStatement * aStatementForLock );
    IDE_RC        setRestartSNforSync();
    IDE_RC        allocNBeginParallelStatements( smiStatement ** aParallelStatements );
    void          destroyParallelStatements( smiStatement * aParallelStatements );
    IDE_RC sendSyncEnd( void );
    IDE_RC sendSyncTableInfo( void );

    IDE_RC ddlASyncStart( void );

    IDE_RC processForExecuteDDLASync( void );

    void   getTargetInfoFromDDLStmtLog( const void * aLogBody, SChar * aDDLStmt );

public: // need public for FIX TABLE

    SChar                mRepName[QCI_MAX_NAME_LEN + 1];

    RP_SENDER_TYPE       mCurrentType;
    RP_SENDER_TYPE       mStartType;
    UInt                 mParallelID;
    
    UInt                 mApplyRetryCount;

    idBool               mStartComplete;
    idBool               mRetryError;
    idBool               mSetHostFlag;
    idBool               mStartError;

    /* For Parallel Logging: LSN -> SN으로 변경 */
    smSN                 mXSN;
    smSN                 mOldMaxXSN;
    smSN                 mCommitXSN;
    smSN                 mSkipXSN;
    UInt                 mReadFileNo;

    RP_SENDER_STATUS     mStatus;

    rpdTransTbl * getTransTbl( void );
    void   setCompleteCheckFlag(idBool aIsChecking)
        { mCheckingStartComplete = aIsChecking; };
    idBool getCompleteCheckFlag( void )
        { return mCheckingStartComplete; };
    IDE_RC  waitComplete( idvSQL     * aStatistics );

    SChar * getRepName(){ return mRepName; };

    UInt getThroughput( void )
    {
        return mReplicator.getThroughput();
    }

    SInt getCurrentFileNo( void );
    idBool isSkipLog( smSN  aSN );

public:
    SChar              mRCMsg[RP_ACK_MSG_LEN];
 
    /* BUG-22703 thr_join Replace */
    IDE_RC    waitThreadJoin(idvSQL *aStatistics);

    void      signalThreadJoin();
    /* PROJ-1915 RemoteLog에 마지막 SN을 리턴 */
    IDE_RC    getRemoteLastUsedGSN(smSN * aSN);
    /* PROJ-1915 RemoteLog 정보를 검증 */
    IDE_RC    checkOffLineLogInfo();
    IDE_RC    setHostForNetwork( SChar* aIP, SInt aPort );

    IDE_RC    updateRemoteFaultDetectTime();

    IDE_RC    allocAndRebuildNewSentLogCount( void );

private:
    idBool             mIsRemoteFaultDetect;
    //sync start일 경우 mStartComplete를 체크하고 있는지 확인
    idBool             mCheckingStartComplete;

    SInt               mMetaIndex;

    idBool             mExitFlag;
    idBool             mApplyFaultFlag;

    iduMutex           mTimeMtxRmt;
    iduCond            mTimeCondRmt;
    iduMutex           mFinalMtx;

    /* BUG-22703 thr_join Replace */
    idBool             mIsThreadDead;
    iduMutex           mThreadJoinMutex;
    iduCond            mThreadJoinCV;

    rpdMeta            mMeta;

    idBool             mTryHandshakeOnce;
    UInt               mRetry;

    static PDL_Time_Value mTvRecvTimeOut;
    static PDL_Time_Value mTvRetry;
    static PDL_Time_Value mTvTimeOut;


    // for repl sync parallel
    SInt               mParallelFactor;

    rpxPJMgr         * mSyncer;
    iduMutex           mSyncerMutex;

    rpdReplSyncItem     * mSyncInsertItems;
    rpdReplSyncItem     * mSyncTruncateItems;

    //PROJ-1541
    rpdSenderInfo     *mSenderInfoArray;
    rpxSenderApply    *mSenderApply;

    // PROJ-1537
    RP_SOCKET_TYPE     mSocketType;
    SChar              mSocketFile[RP_SOCKET_FILE_LEN];

    /* PROJ-1670 replication log buffer */
    rpdLogBufferMgr   *mRPLogBufMgr;


    //for recovery sender
    smSN               mActiveRPRecoverySN; 

    SInt               mFailbackStatus;   // Handshake 시 결정한 Failback 상태

    /* BUG-31545 수행시간 통계정보 */
    idvSQL             mOpStatistics;
    idvSession         mStatSession;
    idvSession         mOldStatSession;

    static rpValueLen  mSyncPKMtdValueLen[QCI_MAX_KEY_COLUMN_COUNT];

    rpxReplicator      mReplicator;

    rpnMessenger       mMessenger;

    idBool             mIsGroupingMode;

    /* Key range 구성을 할때 사용하는 메모리 영역,
     * Performance를 위해 rpxSender 내에 미리 메모리를 할당해 놓고,
     * 반복하여 그 메모리를 재사용하도록 한다.
     * Used Only - rpxSender::getKeyRange()
     * failbackMaster
     */                             
    qriMetaRangeColumn  * mRangeColumn;
    UInt                  mRangeColumnCount;

    UInt                   mSentLogCountArraySize;
    rpxSentLogCount      * mSentLogCountArray;
    rpxSentLogCount     ** mSentLogCountSortedArray;
   
    idBool               mIsSetRestartSNforSync;

    IDE_RC allocSentLogCount( void );
    void freeSentLogCount( void );

    void copySentLogCount( rpxSentLogCount * aSrc, UInt aSentLogCountArraySize );

    void searchSentLogCount( smOID                  aTableOID,
                             rpxSentLogCount     ** aSentLogCount );
    
    void increaseInsertLogCount( smOID aTableOID );
    void increaseDeleteLogCount( smOID aTableOID );
    void increaseUpdateLogCount( smOID aTableOID );
    void increaseLOBLogCount( smOID aTableOID );

    void checkAndSetGroupingMode( void );

public:
    /* PROJ-1915 */
    rpdMeta        * mRemoteMeta; /* Executor에 보관 된 메타를 받는 메타
                                     이걸 mMeta에 클론으로 하여 offline sender가 종작 한다. */
    SChar          * mLogDirPath[SM_LFG_COUNT];
    UInt             mRemoteLFGCount;

    rpxSender*       mChildArray;
    iduMutex         mChildArrayMtx;
    UInt             mChildCount;

    rpdSenderInfo    * mSenderInfo;

    rprSNMapMgr      * mSNMapMgr;

    iduMemAllocator  * mAllocator;
    SInt               mAllocatorState;

    UInt               mSenderListIndex;

    /*
     *  PROJ-2453
     */
private:
    idBool          mIsServiceFail;
    UInt            mTransTableSize;
    rpxSender    ** mAssignedTransTbl;
    iduMutex        mStatusMutex;
    smSN            mFailbackEndSN;

    ULong           mServiceThrRefCount;

private:
    IDE_RC          addXLogAckOnDML();
    IDE_RC          replicateLogWithLogPtr( const SChar  * aLogPtr );
    rpxSender *     getAssignedSender( smTID        aTransID, 
                                       smSN         aCurrentSN,
                                       idBool       aIsBeginLog );
    rpxSender *     assignSenderBySlotIndex( UInt     aSlotIndex,
                                             smSN     aCurrentSN,
                                             idBool   aIsBeginLog );
    rpxSender *     getLessBusySender( void );
    rpxSender *     getLessBusyChildSender( void );
    UInt            getActiveTransCount( void );

    void            waitUntilSendingByServiceThr( void );

public:
    void            sendXLog( const SChar * aLogPtr,
                              smTID         aTransID,
                              smSN          aCurrentSN,
                              idBool        aIsBeginLog );

    idBool          waitUntilFlushFailbackComplete( void );

    smSN            getFailbackEndSN( void );

    ULong           getSendDataSize( void );
    ULong           getSendDataCount( void );   
};

#endif  /* _O_RPX_SENDER_H_ */
