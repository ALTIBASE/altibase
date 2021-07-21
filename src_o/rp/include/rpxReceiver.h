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
 * $Id: rpxReceiver.h 90690 2021-04-23 06:52:47Z yoonhee.kim $
 **********************************************************************/

#ifndef _O_RPX_RECEIVER_H_
#define _O_RPX_RECEIVER_H_ 1

#include <idl.h>
#include <idtBaseThread.h>
#include <cm.h>
#include <smiTrans.h>
#include <smiDef.h>
#include <rp.h>
#include <rpdMeta.h>
#include <rpsSmExecutor.h>
#include <rpxReceiverApply.h>
#include <rpdQueue.h>
#include <rpxXLogTransfer.h>
#include <rpdXLogfileMgr.h>

typedef enum
{
    RPX_RECEIVER_READ_NETWORK = 0,
    RPX_RECEIVER_READ_XLOGFILE,
    RPX_RECEIVER_READ_UNSET
} RPX_RECEIVER_READ_MODE;

typedef enum RPX_RECEIVER_XLOGFILE_RECOVERY_STATUS
{
    RPX_XF_RECOVERY_NONE = 0,
    RPX_XF_RECOVERY_INIT,
    RPX_XF_RECOVERY_PROCESS,
    RPX_XF_RECOVERY_DONE,
    RPX_XF_RECOVERY_ERROR,
    RPX_XF_RECOVERY_TIMEOUT,
    RPX_XF_RECOVERY_EXIT
} RPX_RECEIVER_XLOGFILE_RECOVERY_STATUS;

typedef struct rpxReceiverReadContext
{
    cmiProtocolContext * mCMContext;
    rpdXLogfileMgr     * mXLogfileContext;

    RPX_RECEIVER_READ_MODE mCurrentMode;
} rpxReceiverReadContext;

#define RPX_RECEIVER_INIT_READ_CONTEXT( aCtx ) \
do \
{\
    (aCtx)->mCMContext = NULL; \
    (aCtx)->mXLogfileContext = NULL;\
    (aCtx)->mCurrentMode = RPX_RECEIVER_READ_UNSET; \
}while(0)

struct rpxReceiverParallelApplyInfo;
class smiStatement;
class rpxReceiverApplier;

typedef struct rpxReceiverErrorInfo
{
    smSN mErrorXSN;
    UInt mErrorStopCount;
}rpxReceiverErrorInfo;

class rpxReceiver : public idtBaseThread
{
   friend class rpxReceiverApply;

public:
    rpxReceiver();
    virtual ~rpxReceiver() {};

    IDE_RC initialize( cmiProtocolContext * aProtocolContext,
                       smiStatement       * aStatement,
                       SChar              * aRepName,
                       rpdMeta            * aRemoteMeta,
                       rpdMeta            * aMeta,
                       rpReceiverStartMode  aStartMode,
                       rpxReceiverErrorInfo aErrorInfo,
                       UInt                 aReplID );

    void   destroy();

    /* BUG-38533 numa aware thread initialize */
    IDE_RC initializeThread();
    void   finalizeThread();

    static IDE_RC checkProtocol( cmiProtocolContext  *aProtocolContext,
                                 idBool              *aExitFlag,
                                 rpRecvStartStatus   *aStatus,
                                 rpdVersion          *aVersion );

    IDE_RC processMetaAndSendHandshakeAck( smiTrans         * aTrans,
                                           rpdMeta          * aMeta );
    IDE_RC checkConditionAndSendResult( );

    IDE_RC handshakeWithoutReconnect( rpdXLog *aXLog );
    SInt   decideFailbackStatus(rpdMeta * aPeerMeta);

    void run();

    IDE_RC lock();
    IDE_RC unlock();

    idBool isYou(const SChar* aRepName);

    idBool isExit() { return mExitFlag; }

    void shutdown(); // called by Replication Executor

    inline idBool isSync() 
    { 
        return (mStartMode == RP_RECEIVER_SYNC) ? ID_TRUE : ID_FALSE; 
    };

    inline idBool isApplierExist()
    {
        idBool  sIsApplierExist = ID_FALSE;

        if ( mApplier != NULL )
        {
            sIsApplierExist = ID_TRUE;
        }
        else
        {
            sIsApplierExist = ID_FALSE;
        }

        return sIsApplierExist; 
    };
    //proj-1608 
    inline idBool isRecoveryComplete() 
    { 
        return mIsRecoveryComplete; 
    };

    inline idBool isRecoverySupportReceiver()
    { 
        return ( ( (mMeta.mReplication.mOptions & RP_OPTION_RECOVERY_MASK) == 
                   RP_OPTION_RECOVERY_SET ) && 
                ( (mStartMode == RP_RECEIVER_NORMAL)||
                  (mStartMode == RP_RECEIVER_PARALLEL) ) ) ? ID_TRUE : ID_FALSE;
    }

    void setSNMapMgr(rprSNMapMgr* aSNMapMgr);
    SChar* getRepName(){ return mRepName; };

    /* BUG-22703 thr_join Replace */
    IDE_RC  waitThreadJoin(idvSQL *aStatistics);
    void    signalThreadJoin();
    IDE_RC  threadJoinMutex_lock()    { return mThreadJoinMutex.lock(NULL /* idvSQL* */); }
    IDE_RC  threadJoinMutex_unlock()  { return mThreadJoinMutex.unlock();}

    /* BUG-31545 통계 정보를 시스템에 반영한다. */
    inline void applyStatisticsToSystem()
    {
        idvManager::applyStatisticsToSystem(&mStatSession, &mOldStatSession);
    }

    inline idvSession * getReceiverStatSession()
    {
        return &mStatSession;
    }

    inline idBool isEagerReceiver()
    {
        return ( ( mMeta.mReplication.mReplMode == RP_EAGER_MODE ) &&
                 ( ( mStartMode == RP_RECEIVER_NORMAL ) ||
                   ( mStartMode == RP_RECEIVER_PARALLEL ) ) )
               ? ID_TRUE : ID_FALSE;
    }

    IDE_RC sendAck( rpXLogAck * aAck );

    smSN getApplyXSN( void );

    IDE_RC searchRemoteTable( rpdMetaItem ** aItem, ULong aRemoteTableOID );
    IDE_RC searchTableFromRemoteMeta( rpdMetaItem ** aItem, 
                                      ULong          aTableOID );

    UInt  getParallelApplierCount( void );
    ULong getApplierInitBufferSize( void );

    void enqueueFreeXLogQueue( rpdXLog      * aXLog );
    IDE_RC dequeueFreeXLogQueue( rpdXLog   ** aXLog );

    IDE_RC checkAndWaitAllApplier( smSN   aWaitSN );
    IDE_RC checkAndWaitApplier( UInt          aApplierIndex,
                                smSN          aWaitSN,
                                iduMutex    * aMutex,
                                iduCond     * aCond,
                                idBool      * aIsWait );
    void wakeupReceiverApplier( void );

    IDE_RC enqueueXLogAndSendAck( rpdXLog * aXLog );
    UInt assignApplyIndex( smTID aTID );
    void deassignApplyIndex( smTID aTID );

    smSN    getMinApplyXSNFromApplier( void );

    void   setParallelApplyInfo( UInt aIndex, rpxReceiverParallelApplyInfo * aApplierInfo );

    IDE_RC buildRecordForReplReceiverParallelApply( void                * aHeader,
                                                    void                * aDumpObj,
                                                    iduFixedTableMemory * aMemory );

    IDE_RC buildRecordForReplReceiverTransTbl( void                    * aHeader,
                                               void                    * aDumpObj,
                                               iduFixedTableMemory     * aMemory );

    ULong getApplierInitBufferUsage( void );

    inline UInt     getSqlApplyTableCount()
    {
        return mMeta.getSqlApplyTableCount();
    }

    IDE_RC buildNewMeta( smiStatement * aStatement );
    void   removeNewMeta();

    IDE_RC sendDDLASyncStartAck( UInt aType,  ULong aSendTimeout );

    IDE_RC recvDDLASyncExecute( UInt         * aType,
                                SChar        * aUserName,
                                UInt         * aDDLEnableLevel,
                                UInt         * aTargetCount,
                                SChar        * aTargetTableName,
                                SChar       ** aTargetPartNames,
                                smSN         * aDDLCommitSN,
                                rpdVersion   * aReplVersion,
                                SChar       ** aDDLStmt,
                                ULong          aRecvTimeout );
    IDE_RC sendDDLASyncExecuteAck( UInt    aType,
                                   UInt    aIsSuccess,
                                   UInt    aErrCode,
                                   SChar * aErrMsg,
                                   ULong   aSendTimeout );

    idBool isAlreadyAppliedDDL( smSN aRemoteDDLCommitSN );

    IDE_RC checkLocalAndRemoteNames( SChar  * aUserName,
                                     UInt     aTargetCount,
                                     SChar  * aTargetTableName,
                                     SChar  * aTargetPartNames );

    IDE_RC metaRebuild( smiTrans * aTrans );

    void   setSelfExecuteDDLTransID( smTID aTID )
    {
        idCore::acpAtomicSet64( &mSelfExecuteDDLTransID, aTID );
    }

    smTID   getSelfExecuteDDLTransID()
    {
        return idCore::acpAtomicGet64( &mSelfExecuteDDLTransID );
    }
    
    idBool isSelfExecuteDDLTrans( smTID aTID )
    {
        idBool sResult     = ID_FALSE;

        if ( getSelfExecuteDDLTransID() == aTID )
        {
            sResult = ID_TRUE;
        }
        
        return sResult;
    }

    static ULong  getBaseXLogBufferSize( rpdMeta * aMeta );
    
    static IDE_RC checkAndConvertEndian( void    *aValue,
                                  UInt     aDataTypeId,
                                  UInt     aFlag );

    static IDE_RC convertEndianInsert( rpdMeta * aMeta, rpdXLog *aXLog);

    IDE_RC recoveryCondition( idBool aIsNeedToRebuildMeta );

    IDE_RC sendAckWithTID( rpXLogAck * aAck );

    ULong getReceiveDataSize( void );
    ULong getReceiveDataCount( void );

    idvSQL * getStatistics( void ) { return &mStatistics; }

    /* PROJ-2725 */
    smSN getRestartSNInParallelApplier( void );

    IDE_RC createAndInitializeXLogTransfer( rpxXLogTransfer ** aXLogTransfer, rpdXLogfileMgr * aXLogfileManager );
    void finalizeXLogTransfer();
    void finalizeAndDestroyXLogTransfer();

    rpxXLogTransfer * getXLogTransfer() { return mXLogTransfer; }
    void setXLogTransfer( rpxXLogTransfer * aXLogTransfer ) { mXLogTransfer = aXLogTransfer; }

    IDE_RC createAndInitializeXLogfileManager( smiStatement    * aStatement,
                                               rpdXLogfileMgr ** aXLogfileManager,
                                               smSN              aInitXSN );
    void finalizeAndDestroyXLogfileManager();
    UInt getMinimumUsingXLogFileNo();
    IDE_RC findXLogfileNoByRemoteCheckpointSN( UInt * aFileNo );

    rpdXLogfileMgr * getXLogfileManager() { return mXLogfileManager; }

    IDE_RC startXLogTrasnsfer();

    IDE_RC reqeustRestartXLogtransfer( cmiProtocolContext  );
    UInt getCurrentFileNumberOfRead();

    static IDE_RC getXLogfileLSNFromXSN( smSN aXSN, SChar * aReplName, rpXLogLSN * aXLogLSN );

    /*
     * alter replication rep_name flush from xlogfile;
     */
    IDE_RC processXLogInXLogFile();

    smLSN getLastLSNFromXLogfiles();
    smLSN getCurrentLSNFromXLogfiles();

    void wakeupXLogTansfer();

    void yiled() { idlOS::thr_yield(); }

    IDE_RC  waitXlogfileRecoveryDone( idvSQL *aStatistics );

    idBool checkSuccessReoveryXLogfile() { return ( mXFRecoveryStatus == RPX_XF_RECOVERY_DONE ) ? ID_TRUE:ID_FALSE; }
    void checkAndSetXFRecoveryStatusExit( );

    IDE_RC updateCurrentInfoForConsistentMode( smiStatement * aParentStatement );

    IDE_RC checkMeta ( smiTrans         * aTrans,
                       rpdMeta          * aMeta );

    void decideEndianConversion( rpdMeta * aRemoteMeta );

    iduList * getGlobalTxList() { return &mGlobalTxList; }

    idBool    isFailoverStepEnd() { return mIsFailoverStepEnd; }

    IDE_RC failbackSlaveWithXLogfiles();
    IDE_RC readXLogfileAndMakePKList( rpdSenderInfo         * aSenderInfo,
                                      RP_ACTION_ON_ADD_XLOG   aActionAddXLog,
                                      iduMemPool            * aSNPool,
                                      iduList               * aSNList );

    IDE_RC addLastSyncPK( rpdSenderInfo  * aSenderInfo,
                          rpdXLog        * aXLog );

  private:
    rpdMeta           * mNewMeta;
    rpdQueue            mFreeXLogQueue;
    ULong               mApplierQueueSize;
    ULong               mXLogSize;

    rpdXLog           * mXLogPtr;
    UInt                mProcessedXLogCount;
    rprSNMapMgr       * mSNMapMgr;

    smSN                mLastWaitSN;
    UInt                mLastWaitApplierIndex;

    smSN                mRestartSN;

    smSN                mRemoteCheckpointSN;

    smSN                mLastReceivedSN;

    smTID               mSelfExecuteDDLTransID;

    iduList             mGlobalTxList;

    IDE_RC initializeParallelApplier( UInt     aParallelApplierCount );
    void   finalizeParallelApplier( void );

    void   finalize(); // called when thread stop

    IDE_RC runNormal( void );

    IDE_RC runSync( void );

    IDE_RC runSyncInParallelApplier();

    IDE_RC recvXLog( rpdXLog * aXLog, idBool * aIsEnd );

    void setTcpInfo();

    /* Endian Convert functions. */
    IDE_RC convertEndian(rpdXLog *aXLog);
    IDE_RC convertEndianInsert(rpdXLog *aXLog);
    IDE_RC convertEndianUpdate(rpdXLog *aXLog);
    IDE_RC convertEndianPK(rpdXLog *aXLog);

    /* BUG-22703 thr_join Replace */
    idBool             mIsThreadDead;
    iduMutex           mThreadJoinMutex;
    iduCond            mThreadJoinCV;

    /* BUG-31545 수행시간 통계정보, BUG-46548 Receiver 이벤트 제어를 위한 Statistic */
    idvSQL             mStatistics;
    ULong              mEventFlag;
    idvSession         mStatSession;
    idvSession         mOldStatSession;

    rpxReceiverApplier  * mApplier;
    UInt                  mApplierCount;

    SInt getIdleReceiverApplyIndex( void );

    IDE_RC checkOfflineReplAvailable(rpdMeta  * aMeta);

    idBool isLobColumnExist();

    IDE_RC buildRemoteMeta( rpdMeta * aMeta );

    IDE_RC checkSelfReplication( idBool    aIsLocalReplication,
                                 rpdMeta * aMeta );

    IDE_RC buildMeta( smiStatement       * aStatement,
                      SChar              * aRepName );
    IDE_RC copyNewMeta();

    IDE_RC sendHandshakeAckWithFailbackStatus( rpdMeta * aMeta );

    void setTransactionFlag( void );
    void setTransactionFlagReplReplicated( void );
    void setTransactionFlagReplRecovery( void );
    void setTransactionFlagCommitWriteWait( void );
    void setTransactionFlagCommitWriteNoWait( void );

    void setApplyPolicy( void );
    void setApplyPolicyCheck( void );
    void setApplyPolicyForce( void );
    void setApplyPolicyByProperty( void );

    void setSendAckFlag( void );
    void setFlagToSendAckForEachTransactionCommit( void );
    void setFlagNotToSendAckForEachTransactionCommit( void );

    IDE_RC receiveAndConvertXLog( rpdXLog * aXLog );

    IDE_RC sendAckWhenConditionMeet( rpdXLog * aXLog );
    IDE_RC sendAckForLooseEagerCommit( rpdXLog * aXLog );
    IDE_RC sendStopAckForReplicationStop( rpdXLog * aXLog );

    IDE_RC applyXLogAndSendAck( rpdXLog * aXLog );

    IDE_RC processXLogInSync( rpdXLog  * aXLog, 
                              idBool   * aIsSyncEnd );
    IDE_RC processXLog( rpdXLog * aXLog, idBool *aIsBool );

    void saveRestartSNAtRemoteMeta( smSN aRestartSN );

    IDE_RC recvSyncTablesInfo( UInt * aSyncTableNumber,
                               rpdMetaItem ** aRemoteTable );

    IDE_RC initializeXLog( void );
    IDE_RC initializeFreeXLogQueue( void );
    void finalizeFreeXLogQueue( void );

    IDE_RC buildXLogAckInParallelAppiler( rpdXLog      * aXLog,
                                          rpXLogAck    * aAck );
    void resetCounterForNextAck();

    IDE_RC startApplier( void );
    void   joinApplier( void );

    IDE_RC getLocalFlushedRemoteSN( smSN aRemoteFlushSN,
                                    smSN aRestartSN,
                                    smSN * aLocalFlushedRemoteSN );
    IDE_RC allocRangeColumn( void );
    smSN getRestartSN( void );

    IDE_RC getLocalFlushedRemoteSNInParallelAppiler( rpdXLog        * aXLog,
                                                     smSN           * aLocalFlushedRemoteSN );
    void getLastCommitAndProcessedSNInParallelAppiler( smSN    * aLastCommitSN,
                                                       smSN    * aLastProcessedSN );

    IDE_RC sendAckWhenConditionMeetInParallelAppier( rpdXLog * aXLog );

    IDE_RC buildXLogAckInParallelAppiler( rpdXLog      * aXLog,
                                          UInt           aApplierCount,
                                          rpXLogAck    * aAck );

    IDE_RC runParallelAppiler( void );
    IDE_RC enqueueXLog( rpdXLog * aXLog );
    IDE_RC applyXLogAndSendAckInParallelAppiler( rpdXLog * aXLog );
    IDE_RC allocRangeColumnInParallelAppiler( void );
    IDE_RC processXLogInParallelApplier( rpdXLog    * aXLog,
                                         idBool     * aIsEnd );

    void setKeepAlive( rpdXLog     * aSrcXLog,
                       rpdXLog     * aDestXLog );
    IDE_RC enqueueAllApplier( rpdXLog  * aXLog );

    idBool isAllApplierFinish( void );
    void   waitAllApplierComplete( void );
    void   waitAllApplierCompleteWhileFailbackSlave( void );
	void   shutdownAllApplier( void );

    ULong getApplierQueueSize( ULong aBufSize, ULong aOptionSize );

    IDE_RC updateRemoteXSN( smSN aSN );

    void wakeup( void );


    void setNetworkResourcesToXLogTransfer( void );
    void getNetworkResourcesFromXLogTransfer( void );
    idBool isGottenNetworkResoucesFromXLogTransfer( void );

    idBool getNetworkErrorInXLogTransfer();

    IDE_RC updateCurrentInfoForConsistentModeWithNewTransaction( void );
    void updateSNsForXLogTransfer( smSN aLastProcessedSN, smSN aRestartSN );
    void setRestartSNAndLastProcessedSN( rpxXLogTransfer * aXLogtransfer );

    IDE_RC runRecoveryXlogfiles( void );
    IDE_RC checkAndSetXFRecoveryStatusEND( RPX_RECEIVER_XLOGFILE_RECOVERY_STATUS aStatus );
    
    IDE_RC runFailoverUsingXLogFiles( void );
    
    rpxReceiverReadContext setReadContext( cmiProtocolContext * aCMContext, rpdXLogfileMgr * aXLogfileContext );
    cmiProtocolContext * getCMReadContext( rpxReceiverReadContext * aReadContext );
    rpdXLogfileMgr * getXLogfileReadContext( rpxReceiverReadContext * aReadContext );

    void setReadNetworkMode();
    void setReadXLogfileMode();

    IDE_RC processRemainXLogInXLogfile( idBool aIsSkipStopXLog );

    IDE_RC collectUnCompleteGlobalTxList( iduList * aGlobalTxList );
    IDE_RC applyUnCompleteGlobalTxLog( iduList * aGlobalTxList );

    IDE_RC createNAddGlobalTxNodeToList( ID_XID        * aXID,
                                         smTID           aTID,
                                         smiDtxLogType   aResultType,
                                         iduList       * aGlobalTxList,
                                         idBool          aIsRequestNode );

    void   removeGlobalTxList( iduList * aGlobalTxList );
    void   removeGlobalTxNode( void * aGlobalTxNode );

    void * findGlobalTxNodeFromList( smTID aTID, iduList * aGlobalTxList );

    idBool isAckWithTID( rpdXLog * aXLog );

    void buildDummyXLogAckForConsistent( rpdXLog * aXLog, rpXLogAck * aAck );

    idBool useNetworkResourceMode();

    IDE_RC getInitLSN( rpXLogLSN * aInitLSN );

    IDE_RC initializeXLogfileContents( void );
    IDE_RC initializeXLogfileContents( smiStatement * aStatement );

public:
    SChar               mRepName[QCI_MAX_OBJECT_NAME_LEN + 1];
    SChar               mMyIP[RP_IP_ADDR_LEN];
    SInt                mMyPort;
    SChar               mPeerIP[RP_IP_ADDR_LEN];
    SInt                mPeerPort;

    cmiProtocolContext * mProtocolContext;
    cmiLink            * mLink;
    rpdMeta             mMeta;

    iduMutex            mHashMutex;
    iduCond             mHashCV;
    idBool              mIsWait;

    rpxReceiverApply    mApply;

    idBool              mExitFlag;
    idBool              mNetworkError;

    idBool              mEndianDiff;

    // PROJ-1553 Self Deadlock (Unique Index Wait)
    UInt                mReplID;
    //proj-1608
    rpReceiverStartMode mStartMode;

    idBool              mIsFailoverStepEnd;
    idBool              mIsRecoveryComplete;

    UInt                mParallelID;

    /* PROJ-1915 리시버에서 restart sn을 보관 mRemoteMeta->mReplication.mXSN에 할당
     * PROJ-1915 리시버에서 Meta를 보관
     */
    rpdMeta           * mRemoteMeta; //rpcExecutor에서 지정 한것
    rpxReceiverErrorInfo mErrorInfo;

    iduMemAllocator   * mAllocator;
    SInt                mAllocatorState;

    SInt              * mTransToApplierIndex;  /* Transaction 별로 분배된 Thread Index */
    UInt                mTransactionTableSize;

    /* PROJ-2453 */
    idBool              mAckEachDML;
    
    /* PROJ-2725 */
    rpdXLogfileMgr  * mXLogfileManager;
    rpxXLogTransfer * mXLogTransfer;
    rpxReceiverReadContext mReadContext;

    RPX_RECEIVER_XLOGFILE_RECOVERY_STATUS mXFRecoveryStatus;
    iduCond             mXFRecoveryWaitCV;
    iduMutex            mXFRecoveryWaitMutex;
};

inline IDE_RC rpxReceiver::lock()
{
    return mHashMutex.lock(NULL /* idvSQL* */);
}

inline IDE_RC rpxReceiver::unlock()
{
    return mHashMutex.unlock();
}

#endif  /* _O_RPX_RECEIVER_H_ */
