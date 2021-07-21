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
 

#ifndef _O_RPX_REPLICATOR_H_
#define _O_RPX_REPLICATOR_H_ 1

#include <idl.h>
#include <ide.h>

#include <smiLogRec.h>
#include <smiReadLogByOrder.h>

#include <rpnMessenger.h>

#include <rpdMeta.h>
#include <rpdDelayedLogQueue.h>

#include <rpxSender.h>

class rpxAheadAnalyzer;

typedef enum RP_REPLICATIED_TRNAS_GROUP_OP
{
    RP_REPLICATIED_TRNAS_GROUP_NONE,
    RP_REPLICATIED_TRNAS_GROUP_NORMAL,
    RP_REPLICATIED_TRNAS_GROUP_SEND,
    RP_REPLICATIED_TRNAS_GROUP_DONT_SEND,
    RP_REPLICATIED_TRNAS_GROUP_KEEP
} RP_REPLICATIED_TRNAS_GROUP_OP;

class rpxReplicator
{

private:

    rpxSender    * mSender;
    rpdSenderInfo * mSenderInfo;

    rpdMeta      * mMeta;
    rpnMessenger * mMessenger;

    idvSQL       * mOpStatistics;
    idvSession   * mStatSession;

    rpdTransTbl  * mTransTbl;
    iduMemPool     mChainedValuePool;

    ULong          mReadLogCount;
    ULong          mSendLogCount;

    UInt           mSleepForKeepAliveCount;

    smiReadLogByOrder mLogMgr;
    RP_LOG_MGR_INIT_STATUS mLogMgrInitStatus;
    idBool            mLogMgrInit;
    RP_LOG_MGR_STATUS mLogMgrStatus;
    iduMutex          mLogSwitchMtx;
    idBool            mNeedSwitchLogMgr;

    SChar   mLogDirBuffer[SM_MAX_FILE_NAME];
    SChar * mFirstArchiveLogDirPath[1];

    UInt mCurrFileNo;

    smSN mNeedSN;

    smSN mFlushSN;

    idBool              mIsRPLogBufMode;
    UInt                mRPLogBufID;
    smSN                mRPLogBufMaxSN;
    smSN                mRPLogBufMinSN;
    rpdLogBufferMgr   * mRPLogBufMgr;

public:
    iduMemAllocator * mAllocator;

private:

    idBool isDMLLog( smiChangeLogType aTypeId );
    idBool isLobControlLog( smiChangeLogType aTypeId );

    idBool needMakeMtdValue( rpdColumn * aRpdColumn );

    IDE_RC makeMtdValue( rpdLogAnalyzer * aLogAnlz,
                         rpdMetaItem * aMetaItem );
    void setMtdValueLen( rpdColumn  * aRpdColumn,
                         smiValue   * aColValueArray,
                         rpValueLen * aLenArray,
                         UInt         aChainedValueTotalLen );

    IDE_RC checkEndOfLogFile( smiLogRec * aLogRec,
                              idBool    * aEndOfLog );

    IDE_RC checkAndWaitForLogSync( smiLogRec * aLog );

    IDE_RC waitForLogSync( smTID  aTID );

    void lockLogSwitch( idBool * aLocked );
    void unlockLogSwitch( idBool * aLocked );
    IDE_RC switchToArchiveLogMgr( smSN * aSN );

    IDE_RC waitForNewLogRecord( smSN * aCurrentSN,
                                RP_ACTION_ON_NOGAP aAction );

    IDE_RC sendXLog( rpdLogAnalyzer * aLogAnlz );

    IDE_RC addXLogSyncPK( rpdMetaItem    * aMetaItem,
                          rpdLogAnalyzer * aLogAnlz );

    IDE_RC addXLogImplSVP( smTID  aTID, smSN aSN, UInt aReplStmpDepth );

    IDE_RC checkAndSendImplSVP( smiLogRec * aLog, smTID aTID );

    IDE_RC addXLog( smiLogRec             * aLogRec,
                    rpdMetaItem           * aMetaItem,
                    RP_ACTION_ON_ADD_XLOG   aAction,
                    iduMemPool            * aSNPool,
                    iduList               * aSNList,
                    RP_REPLICATIED_TRNAS_GROUP_OP
                                            aOperation,
                    smTID                   aTID );

    IDE_RC applyTableMetaLog( smTID aTID,
                              smSN  aDDLBeginSN,
                              smSN  aDDLCommitSN,
                              idBool * aOutNeedHandshake );

    IDE_RC buildNewMeta( smiStatement     * aRootStmt );

    IDE_RC updateOldMeta( smiStatement     * aRootStmt,
                          smSN               aDDLCommitSN,
                          smOID              aOldTableOID,
                          smOID              aNewTableOID,
                          rpdItemMetaEntry * aItemMetaEntry );

    IDE_RC checkUsefulBySenderTypeNStatus( smiLogRec             * aLog,
                                           idBool                * aIsOk,
                                           RP_ACTION_ON_ADD_XLOG   aAction,
                                           iduMemPool            * aSNPool,
                                           iduList               * aSNList,
                                           smTID                   aTID );
    IDE_RC checkUsefulLog( smiLogRec             * aLogRec,
                           idBool                * aIsOk,
                           rpdMetaItem          ** aMetaItem,
                           RP_ACTION_ON_ADD_XLOG   aAction,
                           iduMemPool            * aSNPool,
                           iduList               * aSNList,
                           UInt                    aTID );

    IDE_RC insertDictionaryValue( smiLogRec  * aLog,  smTID aTID );
    idBool isReplPropagableLog( smiLogRec * aLog );

    IDE_RC updateMeta( smiStatement     * aSmiStmt,
                       rpdMeta          * aOldMeta,
                       rpdItemMetaEntry * aItemMetaEntry,
                       smOID              aOldTableOID,
                       smOID              aNewTableOID,
                       smSN               aDDLCommitSN,
                       idBool           * aOutIsUpdated );
    
    IDE_RC insertNewTableInfo( smiStatement     * aSmiStmt,
                               rpdItemMetaEntry * aItemMetaEntry, 
                               smSN               aDDLCommitSN );
    IDE_RC deleteOldTableInfo( smiStatement     * aSmiStmt, 
                               rpdItemMetaEntry * aItemMetaEntry,
                               smOID              aOldTableOID );
    IDE_RC updateOldTableInfo( smiStatement     * aSmiStmt,
                               rpdItemMetaEntry * aItemMetaEntry,
                               smOID              aOldTableOID,
                               smOID              aNewTableOID,
                               smSN               aDDLCommitSN );

    idBool checkUsefulLogByTableMetaType( smiLogRec * aLog );

    idBool isAlreadyInsertNewItem( smiLogRec * aLog );

    idBool isReplUsingGlobalTx();

public:
    rpxReplicator( void );

    IDE_RC initialize( iduMemAllocator  * aAllocator,
                       rpxSender        * aSender,
                       rpdSenderInfo    * aSenderInfo,
                       rpdMeta          * aMeta,
                       rpnMessenger     * aMessenger,
                       rpdLogBufferMgr  * aRPLogBufMgr,
                       idvSQL           * aOpStatistics,
                       idvSession       * aStatSession,
                       idBool             aIsEnableGrouping );
    void destroy( void );

    IDE_RC replicateLogFiles( RP_ACTION_ON_NOGAP      aActionNoGap,
                              RP_ACTION_ON_ADD_XLOG   aActionAddXLog,
                              iduMemPool            * aSNPool,
                              iduList               * aSNList );


    void resetReadLogCount( void );
    ULong getReadLogCount( void );

    void resetSendLogCount( void );
    ULong getSendLogCount( void );

    IDE_RC initTransTable( void );
    void rollbackAllTrans( void );
    rpdTransTbl * getTransTbl( void );
    idBool isExistActiveTrans();
    void getMinTransFirstSN( smSN * aSN );

    IDE_RC initializeLogMgr( smSN     aInitSN,
                             UInt     aPreOpenFileCnt,
                             idBool   aIsRemoteLog,
                             ULong    aLogFileSize,
                             UInt     aLFGCount,
                             SChar ** aLogDirPath );
    IDE_RC destroyLogMgr( void );
    IDE_RC getRemoteLastUsedGSN( smSN * aSN );
    void getAllLFGReadLSN( smLSN  * aArrReadLSN );
    IDE_RC switchToRedoLogMgr( smSN * aSN );
    idBool isLogMgrInit( void );
    RP_LOG_MGR_INIT_STATUS getLogMgrInitStatus( void );
    void setLogMgrInitStatus( RP_LOG_MGR_INIT_STATUS aLogMgrInitStatus );
    void checkAndSetSwitchToArchiveLogMgr( const UInt  * aLastArchiveFileNo,
                                           idBool      * aSetLogMgrSwitch );

    void setNeedSN( smSN aNeedSN );

    void leaveLogBuffer( void );

    smSN getFlushSN( void );

    /* PROJ-2397 Compressed Table Replication */
    IDE_RC convertAllOIDToValue( rpdMetaItem    * aMetaItem,
                                 rpdLogAnalyzer * aLogAnlz,
                                 smiLogRec      * aLog );

    void convertOIDToValue( smiColumn      * aColumn,
                            rpdLogAnalyzer * aLogAnlz,
                            smiValue       * aValue );

    IDE_RC convertBeforeColDisk( rpdMetaItem    *aMetaItem,
                                 rpdLogAnalyzer *aLogAnlz );

    IDE_RC convertAfterColDisk( rpdMetaItem    *aMetaItem,
                                rpdLogAnalyzer *aLogAnlz );

    IDE_RC getDDLInfoFromDDLStmtLog( smTID   aTID,
                                     SInt    aMaxDDLStmtLen,
                                     SChar * aUserName,
                                     SChar * aDDLStmt );

    IDE_RC getTargetNamesFromItemMetaEntry( smTID    aTID,
                                            UInt   * aTargetCount,
                                            SChar  * aTargetTableName,
                                            SChar ** aTargetPartNames );
    /*
     *  PROJ 19669 GAPLESS
     */
private:
    UInt                mThroughput;
    PDL_Time_Value      mThroughputStartTime;

    void                calculateThroughput( ULong aProcessedXLogSize );

public:
    UInt                getThroughput( void )
    {
        return mThroughput;
    }

    /*
     *  PROJ 19669 REPLICATED TRANS GROUP
     */
private:
    idBool              mIsUsingAheadAnalyzedXLog;
    idBool              mIsGroupingMode;

    rpxAheadAnalyzer  * mAheadAnalyzer;
    rpdDelayedLogQueue  mDelayedLogQueue;


    RP_REPLICATIED_TRNAS_GROUP_OP getReplicatedTransGroupOperation( smiLogRec    * aLog, 
                                                                    smTID          aTID );

    IDE_RC              addXLogInGroupingMode( smiLogRec             * aLog,
                                               SChar                 * aRawLogPtr,
                                               smLSN                   aCurrentLSN,
                                               rpdMetaItem           * aMetaItem,
                                               RP_ACTION_ON_ADD_XLOG   aAction,
                                               iduMemPool            * aSNPool,
                                               iduList               * aSNList,
                                               smTID                   aTID );

    IDE_RC              dequeueAndSend( RP_ACTION_ON_ADD_XLOG   aAction,
                                        iduMemPool            * aSNPool,
                                        iduList               * aSNList );

    IDE_RC              getMetaItemByLogTypeAndTableOID( smiLogType         aLogType,
                                                         ULong              aTableOID,
                                                         rpdMetaItem     ** aMetaItem );

    IDE_RC              processEndOfLogFileInGrouping( RP_ACTION_ON_ADD_XLOG   aActionAddXLog,
                                                       iduMemPool            * aSNPool,
                                                       iduList               * aSNList,
                                                       smSN                    aSN,
                                                       UInt                    aFileNo,
                                                       idBool                * aIsStartedAheadAnalyzer );

public:
    IDE_RC              startAheadAnalyzer( smSN  aInitSN );
    void                joinAheadAnalyzerThread( void );
    void                shutdownAheadAnalyzerThread( void );
    void                setExitFlagAheadAnalyzerThread( void );

    SInt                getCurrentFileNo( void );

    IDE_RC              buildRecordForReplicatedTransGroupInfo( SChar               * aRepName,
                                                                void                * aHeader,
                                                                void                * aDumpObj,
                                                                iduFixedTableMemory * aMemory );

    IDE_RC              buildRecordForReplicatedTransSlotInfo( SChar               * aRepName,
                                                               void                * aHeader,
                                                               void                * aDumpObj,
                                                               iduFixedTableMemory * aMemory );

    IDE_RC              buildRecordForAheadAnalyzerInfo( SChar               * aRepName,
                                                         void                * aHeader,
                                                         void                * aDumpObj,
                                                         iduFixedTableMemory * aMemory );

/* PROJ-2453 Eager Replication Performance Enhancement */
private:
    idBool              isMyLog( smTID  aTransID,
                                 smSN   aCurrentSN );

    IDE_RC              readMyLog( smSN    * aCurrentSN,
                                   smLSN   * aReadLSN,
                                   SChar   * aLogHead,
                                   SChar  ** aLogPtr,
                                   SChar  ** aRawLogPtr,
                                   idBool  * aIsValid );

    IDE_RC              isNeedDecompress( SChar  * aRawLogPtr,
                                          SChar  * aRawHeadLogPtr,
                                          idBool * aIsNeedDecompress );
public:
    IDE_RC              replicateLogWithLogPtr( const SChar * aLogPtr );

    IDE_RC              checkAndAddXLog( RP_ACTION_ON_ADD_XLOG   aActionAddXLog,
                                         iduMemPool            * aSNPool,
                                         iduList               * aSNList,
                                         SChar                 * aRawLogPtr,
                                         smiLogRec             * aLog,
                                         smLSN                   aReadLSN,
                                         idBool                  aIsStartedAheadAnalyzer,
                                         smTID                   aTID );

    UInt                getActiveTransCount( void );
    IDE_RC              sleepForKeepAlive();
};

#endif /* _O_RPX_REPLICATOR_H_ */
