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
 * $Id: rpcManager.h 91413 2021-08-03 05:01:56Z lswhh $
 **********************************************************************/

#ifndef _O_RPC_MANAGER_H_
#define _O_RPC_MANAGER_H_ 1

#include <idl.h>
#include <idu.h>
#include <idtBaseThread.h>
#include <iduMemory.h>

#include <cm.h>
#include <smi.h>
#include <qci.h>

#include <rp.h>
#include <rpcReceiverList.h>
#include <rpxSender.h>
#include <rpxReceiverApply.h>
#include <rpxReceiver.h>
#include <rpxReceiverApplier.h>
#include <rpcHBT.h>
#include <rpuProperty.h>
#include <rpdSenderInfo.h>
#include <rpdLogBufferMgr.h>
#include <rpdStatistics.h>
#include <rprSNMapMgr.h>
#include <rpcDDLSyncManager.h>
#include <rpxTempSender.h>


class smiStatement;
class rpdLockTableManager;

typedef enum
{
    RP_ALL_THR = 0,
    RP_SEND_THR,
    RP_RECV_THR
} RP_REPL_THR_MODE;

// ** For the X$REPGAP Fixed Table ** //
typedef struct rpxSenderGapInfo
{
    SChar *        mRepName;     // REP_NAME
    RP_SENDER_TYPE mCurrentType; // CURRENT_TYPE
    smSN           mCurrentSN;   // REP_LAST_SN
    smSN           mSenderSN;    // REP_SN
    ULong          mSenderGAP;   // REP_GAP
    ULong          mSenderGAPSize; // REP_GAP_SIZE

    UInt           mLFGID;       // READ_LFG_ID
    UInt           mFileNo;      // READ_FILE_NO
    UInt           mOffset;      // READ_OFFSET
    SInt           mParallelID;
} rpxSenderGapInfo;

// ** For the X$REPSYNC Fixed Table ** //
typedef struct rpxSenderSyncInfo
{
    SChar *     mRepName;       // REP_NAME
    SChar *     mTableName;     // SYNC_TABLE
    SChar *     mPartitionName; // SYNC_PARTITION
    ULong       mRecordCnt;     // SYNC_RECORD_COUNT
    smSN        mRestartSN;     // SYNC_SN
} rpxSenderSyncInfo;

// ** For the X$REPEXEC Fix Table ** //
typedef struct rpcExecInfo
{
    UShort      mPort;                       // PORT
    SInt        mMaxReplSenderCount;         // MAX_SENDER_COUNT
    SInt        mMaxReplReceiverCount;       // MAX_RECEIVER_COUNT
} rpcExecInfo;

// ** For the X$REPSENDER Fix Table ** //
typedef struct rpxSenderInfo
{
    SChar *             mRepName;            // REP_NAME
    RP_SENDER_TYPE      mCurrentType;        // CURRENT_TYPE
    idBool              mRetryError;         // RESTART_ERROR_FLAG
    smSN                mXSN;                // XSN
    smSN                mCommitXSN;          // COMMIT_XSN
    RP_SENDER_STATUS    mStatus;             // STATUS
    SChar *             mMyIP;               // SENDER_IP
    SChar *             mPeerIP;             // PEER_IP
    SInt                mMyPort;             // SENDER_PORT
    SInt                mPeerPort;           // PEER_PORT
    ULong               mReadLogCount;       // READ_LOG_COUNT
    ULong               mSendLogCount;       // SEND_LOG_COUNT
    SInt                mReplMode;
    SInt                mActReplMode;
    SInt                mParallelID;
    SInt                mCurrentFileNo;
    ULong               mThroughput;
    UInt                mNeedConvert;
    ULong               mSendDataSize;
    ULong               mSendDataCount;
} rpxSenderInfo;

/* For the X$REPSENDER STATISTICS Fix Table
 *
 * IDV_STAT_INDEX_OPTM_RP_S_WAIT_NEW_LOG
 * IDV_STAT_INDEX_OPTM_RP_S_READ_LOG_FROM_REPLBUFFER
 * IDV_STAT_INDEX_OPTM_RP_S_READ_LOG_FROM_FILE
 * IDV_STAT_INDEX_OPTM_RP_S_CHECK_USEFUL_LOG
 * IDV_STAT_INDEX_OPTM_RP_S_LOG_ANALYZE
 * IDV_STAT_INDEX_OPTM_RP_S_SEND_XLOG
 * IDV_STAT_INDEX_OPTM_RP_S_RECV_ACK
 * IDV_STAT_INDEX_OPTM_RP_S_SET_ACKEDVALUE
 */
typedef struct rpxSenderStatistics
{
    SChar * mRepName;
    SInt    mParallelID;
    ULong   mStatWaitNewLog;
    ULong   mStatReadLogFromReplBuffer;
    ULong   mStatReadLogFromFile;
    ULong   mStatCheckUsefulLog;
    ULong   mStatLogAnalyze;
    ULong   mStatSendXLog;
    ULong   mStatRecvAck;
    ULong   mStatSetAckedValue;
} rpxSenderStatistics;

// ** For the X$REPRECEIVER Fix Table ** //
typedef struct rpxReceiverInfo
{
    SChar *             mRepName;            // REP_NAME
    SChar *             mMyIP;               // MY_IP
    SInt                mMyPort;             // MY_PORT
    SChar *             mPeerIP;             // PEER_IP
    SInt                mPeerPort;           // PEER_PORT
    smSN                mApplyXSN;           // APPLY_XSN
    ULong               mInsertSuccessCount; // INSERT_SUCCESS_COUNT
    ULong               mInsertFailureCount; // INSERT_FAILURE_COUNT
    ULong               mUpdateSuccessCount; // UPDATE_SUCCESS_COUNT
    ULong               mUpdateFailureCount; // UPDATE_FAILURE_COUNT
    ULong               mDeleteSuccessCount; // DELETE_SUCCESS_COUNT
    ULong               mDeleteFailureCount; // DELETE_FAILURE_COUNT
    ULong               mCommitCount;        // COMMIT COUNT
    ULong               mAbortCount;         // ABORT COUNT
    SInt                mParallelID;
    UInt                mErrorStopCount;
    UInt                mSQLApplyTableCount;
    ULong               mApplierInitBufferUsage;
    ULong               mReceiveDataSize;
    ULong               mReceiveDataCount;
} rpxReceiverInfo;

// ** For the X$REPRECEIVER_PARALLEL_APPLY Fix Table ** //
typedef struct rpxReceiverParallelApplyInfo
{
    SChar *             mRepName;            /* REP_NAME */
    UInt                mParallelApplyIndex; /* PARALLEL_APPLIER_INDEX */
    smSN                mApplyXSN;           /* APPLY_XSN */
    ULong               mInsertSuccessCount; // INSERT_SUCCESS_COUNT
    ULong               mInsertFailureCount; // INSERT_FAILURE_COUNT
    ULong               mUpdateSuccessCount; // UPDATE_SUCCESS_COUNT
    ULong               mUpdateFailureCount; // UPDATE_FAILURE_COUNT
    ULong               mDeleteSuccessCount; // DELETE_SUCCESS_COUNT
    ULong               mDeleteFailureCount; // DELETE_FAILURE_COUNT
    ULong               mCommitCount;        // COMMIT COUNT
    ULong               mAbortCount;         // ABORT COUNT
    UInt                mStatus;
} rpxReceiverParallelApplyInfo;

/* For the X$REPRECEIVER STATISTICS Fix Table
 *
 * IDV_STAT_INDEX_OPTM_RP_R_RECV_XLOG
 * IDV_STAT_INDEX_OPTM_RP_R_CONVERT_ENDIAN
 * IDV_STAT_INDEX_OPTM_RP_R_TX_BEGIN
 * IDV_STAT_INDEX_OPTM_RP_R_TX_COMMIT
 * IDV_STAT_INDEX_OPTM_RP_R_TX_ABORT
 * IDV_STAT_INDEX_OPTM_RP_R_TABLE_CURSOR_OPEN
 * IDV_STAT_INDEX_OPTM_RP_R_TABLE_CURSOR_CLOSE
 * IDV_STAT_INDEX_OPTM_RP_R_UPDATEROW
 * IDV_STAT_INDEX_OPTM_RP_R_OPEN_LOB_CURSOR
 * IDV_STAT_INDEX_OPTM_RP_R_PREPARE_LOB_WRITE
 * IDV_STAT_INDEX_OPTM_RP_R_WRITE_LOB_PIECE
 * IDV_STAT_INDEX_OPTM_RP_R_FINISH_LOB_WRITE
 * IDV_STAT_INDEX_OPTM_RP_R_CLOSE_LOB_CURSOR
 * IDV_STAT_INDEX_OPTM_RP_R_COMPARE_IMAGE
 * IDV_STAT_INDEX_OPTM_RP_R_SEND_ACK
 */
typedef struct rpxReceiverStatistics
{
    SChar * mRepName;
    SInt    mParallelID;
    ULong   mStatRecvXLog;
    ULong   mStatConvertEndian;
    ULong   mStatTxBegin;
    ULong   mStatTxCommit;
    ULong   mStatTxRollback;
    ULong   mStatTableCursorOpen;
    ULong   mStatTableCursorClose;
    ULong   mStatInsertRow;
    ULong   mStatUpdateRow;
    ULong   mStatDeleteRow;
    ULong   mStatOpenLobCursor;
    ULong   mStatPrepareLobWrite;
    ULong   mStatWriteLobPiece;
    ULong   mStatFinishLobWriete;
    ULong   mStatTrimLob;
    ULong   mStatCloseLobCursor;
    ULong   mStatCompareImage;
    ULong   mStatSendAck;
} rpxReceiverStatistics;

/* PROJ-1442 Replication Online 중 DDL 허용
 * For the X$REPRECEIVER_COLUMN Fix Table
 */
typedef struct rpxReceiverColumnInfo
{
    SChar   *mRepName;          // REP_NAME
    SChar   *mUserName;         // USER_NAME
    SChar   *mTableName;        // TABLE_NAME
    SChar   *mPartitionName;    // PARTITION_NAME
    SChar   *mColumnName;       // COLUMN_NAME
    UInt     mApplyMode;        // APPLY_MODE
} rpxReceiverColumnInfo;

/* For the X$REPSENDER_COLUMN Fix Table */
typedef struct rpxSenderColumnInfo
{
    SChar   *mRepName;          // REP_NAME
    SChar   *mUserName;         // USER_NAME
    SChar   *mTableName;        // TABLE_NAME
    SChar   *mPartitionName;    // PARTITION_NAME
    SChar   *mColumnName;       // COLUMN_NAME
} rpxSenderColumnInfo;

// ** For the X$REPRECOVERY Fix Table ** //
typedef struct rprRecoveryInfo
{
    SChar *          mRepName;
    rpRecoveryStatus mStatus;
    smSN             mStartSN;
    smSN             mXSN;
    smSN             mEndSN;
    SChar *          mRecoverySenderIP;
    SChar *          mPeerIP;
    SInt             mRecoverySenderPort;
    SInt             mPeerPort;
} rprRecoveryInfo;

// ** For the X$REPLOGBUFFER Fix Table ** //
typedef struct rpdLogBufInfo
{
    SChar *          mRepName;
    smSN             mBufMinSN;
    smSN             mSN;
    smSN             mBufMaxSN;
} rpdLogBufInfo;

// ** For the X$REPSENDER_TRANSTBL , X$REPRECEIVER_TRANSTBL
typedef struct rpdTransTblNodeInfo
{
    SChar             *mRepName;
    RP_SENDER_TYPE     mCurrentType;
    smTID              mRemoteTID;
    smTID              mMyTID;
    idBool             mBeginFlag;
    smSN               mBeginSN;
    SInt               mParallelID;
    SInt               mParallelApplyIndex;
} rpdTransTblNodeInfo;

// ** For the X$REPOFFLINE_STATUS Fix Table ** //
typedef struct rpxOfflineInfo
{
    SChar               mRepName[QC_MAX_OBJECT_NAME_LEN + 1]; // REP_NAME
    RP_OFFLINE_STATUS   mStatusFlag;                   // STATUS
    UInt                mSuccessTime;                  // SUCCESS_TIME
    idBool              mCompleteFlag;
} rpxOfflineInfo;

/* For the X$REPSENDER_SENT_LOG_COUNT Fix Table */
typedef struct rpcSentLogCount
{
    SChar             * mRepName;
    ULong               mCurrentType;
    SInt                mParallelID;    
    ULong               mTableOID;
    UInt                mInsertLogCount;
    UInt                mUpdateLogCount;
    UInt                mDeleteLogCount;
    UInt                mLOBLogCount;
    
} rpcSentLogCount;

/* For the X$REPLICATED_TRANS_GROUP Fix Table */
typedef struct rpdReplicatedTransGroupInfo
{
    SChar       * mRepName;
    smTID         mGroupTransID;
    smSN          mGroupTransBeginSN;
    smSN          mGroupTransEndSN;

    UInt          mOperation;

    smTID         mTransID;
    smTID         mBeginSN;
    smTID         mEndSN;
} rpdReplicatedTransGroupInfo;

/* For the X$REPLICATED_TRANS_SLOT fix Table */
typedef struct rpdReplicatedTransSlotInfo
{
    SChar       * mRepName;
    smTID         mGroupTransID;

    UInt          mTransSlotIndex;

    smTID         mNextGroupTransID;
} rpdReplicatedTransSlotInfo;

/* For the X$AHEAD_ANALYZER fix Table */
typedef struct rpdAheadAnalyzerInfo
{
    SChar       * mRepName;
    UInt          mStatus;
    UInt          mReadLogFileNo;
    smSN          mReadSN;
} rpdAheadAnalyzerInfo;

/* For the X$XLOG_TRANSFER fix Table */
typedef struct rpdXLogTransfer
{
    SChar       * mRepName;
    UInt          mStatus;
    smTID         mLastCommitTransactionID;
    smSN          mLastCommitSN;
    smSN          mLastWrittenSN;
    smSN          mRestartSN;
    UInt          mLastWrittenFileNumber;
    UInt          mLastWrittenFileOffset;
    UInt          mNetworkResourceStatus;
} rpdXLogTransfer;

/* For the X$XLOGFILE_MANAGER fix Table */
typedef struct rpdXLogfileManagerInfo
{
    SChar * mRepName;
    UInt    mReadFileNo;
    UInt    mReadOffset;
    UInt    mWriteFileNo;
    UInt    mWriteOffset;
    UInt    mPrepareXLogfileCnt;
    UInt    mLastCreatedFileNo;
} rpdXLogfileManagerInfo;

class rpcManager : public idtBaseThread
{
public:
    static qciManageReplicationCallback   mCallback;

    static rpcManager                   * mMyself;
    static PDL_Time_Value                 mTimeOut;
    static rpdLogBufferMgr              * mRPLogBufMgr;
    static rpcHBT                       * mHBT;

public:
    rpcManager();
    virtual ~rpcManager() {};
    IDE_RC initialize( SInt              aMax,
                       rpdReplications * aReplications,
                       UInt              aReplCount );
    void   destroy();

    static IDE_RC initREPLICATION();
    static IDE_RC finalREPLICATION();
    static IDE_RC wakeupPeerByIndex( idBool          * aExitFlag,
                                     rpdReplications * aReplication,
                                     SInt              aIndex );
    static IDE_RC wakeupPeer( idBool            * aExitFlag,
                              rpdReplications   * aReplication );

    void   final();
    IDE_RC wakeupManager();


    void   run();

    //BUG-22703 : Begin Statement를 수행한 후에 Hang이 걸리지 않아야 합니다.
    // aStatistics  통계 정보 파라메터를 추가 합니다.
    IDE_RC realize(RP_REPL_THR_MODE   thr_mode,
                   idvSQL           * aStatistics);
    IDE_RC realizeRecoveryItem( idvSQL * aStatistics );

    void   getReceiverErrorInfo( SChar  * aRepName,
                                 rpxReceiverErrorInfo   * aOutErrorInfo );

    rpdMeta * findRemoteMeta( SChar * aRepName );

    static IDE_RC createReplication( void        * aQcStatement );
    static IDE_RC updatePartitionedTblRplRecoveryCnt( void          * aQcStatement,
                                                      qriReplItem   * aReplItem,
                                                      qcmTableInfo  * aTableInfo );
    static IDE_RC insertOneReplObject( void          * aQcStatement,
                                       qriReplItem    * aReplItem,
                                       qcmTableInfo  * aTableInfo,
                                       UInt            aReplMode,
                                       UInt            aReplOptions,
                                       UInt          * aReplItemCount );
    static IDE_RC deleteOneReplObject( void          * aQcStatement,
                                       qriReplItem    * aReplItem,
                                       qcmTableInfo  * aTableInfo,
                                       SChar         * aReplName,
                                       UInt          * aReplItemCount );

    static IDE_RC insertOneReplOldObject( void         * aQcStatement,
                                          qriReplItem  * aReplItem,
                                          qcmTableInfo * aTableInfo,
                                          idBool         aMetaUpdate );
    static IDE_RC deleteOneReplOldObject( void         * aQcStatement,
                                          qriReplItem  * aReplItem,
                                          idBool         aMetaUpdate );

    static IDE_RC insertOneReplHost(void           * aQcStatement,
                                    qriReplHost     * aReplHost,
                                    SInt           * aHostNo,
                                    SInt             aRole);
    static IDE_RC deleteOneReplHost( void           * aQcStatement,
                                     qriReplHost     * aReplHost );
   
    static IDE_RC buildTempMetaItems( SChar           * aRepName,
                                      rpdReplSyncItem * aSyncItemList,
                                      rpdMeta         * aMeta );

    static IDE_RC setOneReplHost( void           * aQcStatement,
                                  qriReplHost     * aReplHost );
    static IDE_RC updateLastUsedHostNo( smiStatement  * aSmiStmt,
                                        SChar         * aRepName,
                                        SChar         * aHostIP,
                                        UInt            aPortNo );

    static IDE_RC alterReplicationFlushWithXLogs( smiStatement  * aSmiStmt,
                                                  SChar         * aReplName,
                                                  rpFlushOption * aFlushOption,
                                                  idvSQL        * aStatistics );
    static IDE_RC waitUntilSenderFlush(SChar       *aRepName,
                                       rpFlushType  aFlushType,
                                       UInt         aTimeout,
                                       idBool       aAlreadyLocked,
                                       idvSQL      *aStatistics);

    static IDE_RC alterReplicationFlushWithXLogfiles( smiStatement  * aSmiStmt,
                                                      SChar         * aReplName,
                                                      idvSQL        * aStatistics );

    static IDE_RC waitUntilReceiverFlushXLogfiles( rpxReceiver   * aReceiver,
                                                   idvSQL        * aStatistics );

    static IDE_RC alterReplicationAddTable( void        * aQcStatement );
    static IDE_RC alterReplicationDropTable( void        * aQcStatement );

    static IDE_RC alterReplicationAddHost( void        * aQcStatement );
    static IDE_RC alterReplicationDropHost( void        * aQcStatement );
    static IDE_RC alterReplicationSetHost( void        * aQcStatement );
    static IDE_RC alterReplicationSetRecovery( void        * aQcStatement );
    /* PROJ-1915 */
    static IDE_RC alterReplicationSetOfflineEnable(void        * aQcStatement);
    static IDE_RC alterReplicationSetOfflineDisable(void        * aQcStatement);

    /* PROJ-1969 */
    static IDE_RC alterReplicationSetGapless(void        * aQcStatement);
    static IDE_RC alterReplicationSetParallel(void        * aQcStatement);
    static IDE_RC alterReplicationSetGrouping(void        * aQcStatement);

    static IDE_RC alterReplicationSetDDLReplicate( void        * aQcStatement );

    static IDE_RC dropReplication( void        * aQcStatement );

    static IDE_RC startSenderThread( idvSQL        * aStatistics,
                                     iduVarMemList * aMemory,
                                     SChar         * aReplName,
                                     RP_SENDER_TYPE  aStartType,
                                     idBool          aTryHandshakeOnce,
                                     smSN            aStartSN,
                                     qciSyncItems  * aSyncItemList,
                                     SInt            aParallelFactor,
                                     void          * aLockTable );

    //BUG-22703 : Begin Statement를 수행한 후에 Hang이 걸리지 않아야 합니다.
    // aStatistics  통계 정보 파라메터를 추가 합니다.
    static IDE_RC startSenderThread( idvSQL                 * aStatistics,
                                     iduVarMemList          * aMemory, 
                                     smiStatement           * aSmiStmt,
                                     SChar                  * aReplName,
                                     RP_SENDER_TYPE           astartType,
                                     idBool                   aTryHandshakeOnce,
                                     smSN                     aStartSN,
                                     qciSyncItems           * aSyncItemList,
                                     SInt                     aParallelFactor,
                                     void                   * aLockTable );
    static IDE_RC stopSenderThread( smiStatement * aSmiStmt,
                                    SChar        * aReplName,
                                    idvSQL       * aStatistics,
                                    idBool         aIsImmediate );
     
    static IDE_RC startTempSync( void * aQcStatement );


    static IDE_RC makeTempSyncItemList( void             * aQcStatement,
                                        rpdReplSyncItem ** aItemList );

    static IDE_RC allocAndFillSyncItem( const qriReplItem        * const aReplItem,
                                        const qcmTableInfo       * const aTableInfo,
                                        const qcmTableInfo       * const aPartInfo,
                                        rpdReplSyncItem         ** aSyncItem );

    static IDE_RC resetReplication(smiStatement * aSmiStmt,
                                   SChar        * aReplName,
                                   idvSQL       * aStatistics);

    static IDE_RC updateIsStarted( smiStatement * aSmiStmt,
                                   SChar        * aRepName,
                                   SInt           aIsActive );

    static IDE_RC updateRemoteFaultDetectTimeAllEagerSender( rpdReplications * aReplications,
                                                             UInt              aMaxReplication );

    static IDE_RC updateRemoteFaultDetectTime(smiStatement * aSmiStmt,
                                              SChar        * aRepName,
                                              SChar        * aTime);

    static IDE_RC removeOldMetaRepl( smiStatement  * aParentStatement,
                                     SChar         * aReplName );

    static IDE_RC resetRemoteFaultDetectTime(smiStatement * aSmiStmt,
                                             SChar        * aRepName);

    static IDE_RC updateGiveupTime( smiStatement * aSmiStmt,
                                    SChar        * aRepName );

    static IDE_RC resetGiveupTime( smiStatement * aSmiStmt,
                                   SChar        * aRepName );

    static IDE_RC updateGiveupXSN( smiStatement * aSmiStmt,
                                   SChar        * aRepName );

    static IDE_RC resetGiveupXSN( smiStatement * aSmiStmt,
                                  SChar        * aRepName );
    static IDE_RC resetGiveupInfo(RP_SENDER_TYPE aStartType, 
                                  smiStatement * aBegunSmiStmt, 
                                  SChar        * aReplName);

    static IDE_RC updateRemoteXSN( smiStatement * aSmiStmt,
                                   SChar        * aRepName,
                                   smSN           aSN );

    // Replication Minimum XSN을 변경한다.
    static IDE_RC updateXSN( smiStatement * aSmiStmt,
                             SChar        * aRepName,
                             smSN           aSN );

    // update Invalid Max SN of ReplItem
    static IDE_RC updateInvalidMaxSN(smiStatement * aSmiStmt,
                                     rpdReplItems * aReplItems,
                                     smSN           aSN);

    static IDE_RC updateOldInvalidMaxSN( smiStatement * aSmiStmt,
                                         rpdReplItems * aReplItems,
                                         smSN           aSN );

    static IDE_RC selectReplications( smiStatement    * aSmiStmt,
                                      UInt*             aNumReplications,
                                      rpdReplications * aReplications,
                                      UInt              aMaxReplications );

    static IDE_RC updateXLogfileCurrentLSN( smiStatement * aSmiStmt,
                                            SChar        * aRepName,
                                            smLSN          aLSN );

    static IDE_RC selectXLogfileCurrentLSNByName( smiStatement * aSmiStmt,
                                                  SChar        * aRepName,
                                                  smLSN        * aLSN );

    //proj-1608 recovery from replications
    IDE_RC processRPRequest( cmiLink        * aLink,
                             idBool           aIsRecoveryPhase);

    IDE_RC executeStartReceiverThread( cmiProtocolContext   * aProtocolContext,
                                       rpdMeta              * aRemoteMeta );

    // aRepName을 갖는 Sender를 깨운다.
    void   wakeupSender(const SChar* aRepName);

    //BUG-22703 : Begin Statement를 수행한 후에 Hang이 걸리지 않아야 합니다.
    // aStatistics  통계 정보 파라메터를 추가 합니다.
    IDE_RC stopReceiverThread(SChar  * aRepName,
                              idBool   aAlreadyLocked,
                              idvSQL * aStatistics);

    static void applyStatisticsForSystem();

    static void printDebugInfo();
    
    static IDE_RC stopReceiverThreads( smiStatement * aSmiStmt,
                                       idvSQL       * aStatistics,
                                       smOID        * aTableOIDArray,
                                       UInt           aTableOIDCount );

    static IDE_RC findNStopReceiverThreadsByTableOIDWithNewStmt( idvSQL       * aStatistics,
                                                                 smiStatement * aRootStmt,
                                                                 UInt           aStmtFlag,
                                                                 smOID        * aTableOIDArray,
                                                                 UInt           aTableOIDCount );

    static IDE_RC findNStopReceiverThreadsByTableInfo( void * aQcStatement, qciTableInfo * aTableInfo );

    static IDE_RC findNStopReceiverThreadsByTableOIDArray( idvSQL          * aStatistics,
                                                           smiStatement    * aSmiStmt,
                                                           smOID           * aTableOIDArray,
                                                           UInt              aTableOIDCount );

    static idBool isEnableRP();           // BUG-45984 replication port를 확인하여 이중화 사용여부를 확인

    IDE_RC addReplListener( rpLinkImpl aImpl );
    cmiLinkImpl getCMLinkImplByRPLinkImpl( rpLinkImpl aLinkImpl );

    static IDE_RC isEnabled();

    static idBool isExistDatatype( qcmColumn  * aColumns,
                                   UInt         aColumnCount,
                                   UInt         aDataType );

    static IDE_RC isDDLEnableOnReplicatedTable( UInt              aRequireLevel,
                                                qcmTableInfo    * aTableInfo );

    /*************************************************************
     * alter table t1 add column(...) 을 했을 때,
     * eager replication에서는 t1 테이블을 replication하고 있을 때,
     * DDL을 허용하지 않는다. 그러므로, tableOID를 이용하여
     * 해당 테이블을 replication 중인 Sender가 있는지 찾아봐야한다.
     ************************************************************/

    static IDE_RC isRunningEagerSenderByTableOID( smiStatement  * aSmiStmt,
                                                  idvSQL        * aStatistics,
                                                  smOID         * aTableOIDArray,
                                                  UInt            aTableOIDCount,
                                                  idBool        * aIsExist );

    static IDE_RC isRunningEagerReceiverByTableOID( smiStatement  * aSmiStmt,
                                                    idvSQL        * aStatistics,
                                                    smOID         * aTableOIDArray,
                                                    UInt            aTableOIDCount,
                                                    idBool        * aIsExist );

    static IDE_RC isRunningEagerByTableInfoInternal( void          * aQcStatement,
                                                     qciTableInfo  * aTableInfo,
                                                     idBool        * aIsExist );

    static IDE_RC isRunningEagerSenderByTableOIDArrayInternal( smiStatement  * aSmiStmt,
                                                               idvSQL        * aStatistics,
                                                               smOID         * aTableOIDArray,
                                                               UInt            aTableOIDCount,
                                                               idBool        * aIsExist );

    static IDE_RC isRunningEagerReceiverByTableOIDArrayInternal( smiStatement  * aSmiStmt,
                                                                 idvSQL        * aStatistics,
                                                                 smOID         * aTableOIDArray,
                                                                 UInt            aTableOIDCount,
                                                                 idBool        * aIsExist );

    /*************************** Performance Views ***************************/
    static IDE_RC buildRecordForReplManager( idvSQL              * /*aStatistics*/,
                                             void                * aHeader,
                                             void                * aDumpObj,
                                             iduFixedTableMemory * aMemory );
    static IDE_RC buildRecordForReplSender( idvSQL              * /*aStatistics*/,
                                            void                * aHeader,
                                            void                * aDumpObj,
                                            iduFixedTableMemory * aMemory );
    static IDE_RC buildRecordForReplSenderStatistics( idvSQL              * /*aStatistics*/,
                                                      void                * aHeader,
                                                      void                * aDumpObj,
                                                      iduFixedTableMemory * aMemory );
    static IDE_RC buildRecordForReplReceiver( idvSQL              * /*aStatistics*/,
                                              void                * aHeader,
                                              void                * aDumpObj,
                                              iduFixedTableMemory * aMemory );
    static IDE_RC buildRecordForReplReceiverStatistics( idvSQL              * /*aStatistics*/,
                                                        void                * aHeader,
                                                        void                * aDumpObj,
                                                        iduFixedTableMemory * aMemory );
    static IDE_RC buildRecordForReplReceiverParallelApply( idvSQL              * /*aStatistics*/,
                                                           void                * aHeader,
                                                           void                * aDumpObj,
                                                           iduFixedTableMemory * aMemory );
   static IDE_RC buildRecordForReplGap( idvSQL              * /*aStatistics*/,
                                         void                * aHeader,
                                         void                * aDumpObj,
                                         iduFixedTableMemory * aMemory );
    static IDE_RC buildRecordForReplSync( idvSQL              * /*aStatistics*/,
                                          void                * aHeader,
                                          void                * aDumpObj,
                                          iduFixedTableMemory * aMemory );
    static IDE_RC buildRecordForReplSenderTransTbl( idvSQL              * /*aStatistics*/,
                                                    void                * aHeader,
                                                    void                * aDumpObj,
                                                    iduFixedTableMemory * aMemory );
    static IDE_RC buildRecordForReplReceiverTransTbl( idvSQL              * /*aStatistics*/,
                                                      void                * aHeader,
                                                      void                * aDumpObj,
                                                      iduFixedTableMemory * aMemory );

    static IDE_RC setReceiverTranTblValue( void                    * aHeader,
                                           void                    * aDumpObj,
                                           iduFixedTableMemory     * aMemory,
                                           SChar                   * aRepName,
                                           UInt                      aParallelID,
                                           SInt                      aParallelApplyIndex,
                                           rpdTransTbl             * aTransTbl );
    static IDE_RC buildRecordForReplRecovery( idvSQL              * /*aStatistics*/,
                                              void                * aHeader,
                                              void                * aDumpObj,
                                              iduFixedTableMemory * aMemory );
    static IDE_RC buildRecordForReplLogBuffer( idvSQL              * /*aStatistics*/,
                                               void                * aHeader,
                                               void                * aDumpObj,
                                               iduFixedTableMemory * aMemory );
    static IDE_RC buildRecordForReplReceiverColumn( idvSQL              * /*aStatistics*/,
                                                    void                * aHeader,
                                                    void                * aDumpObj,
                                                    iduFixedTableMemory * aMemory );
    static IDE_RC buildRecordForReplSenderColumn( idvSQL              * /*aStatistics*/,
                                                  void                * aHeader,
                                                  void                * aDumpObj,
                                                  iduFixedTableMemory * aMemory );
    static IDE_RC buildRecordForReplOfflineSenderInfo( idvSQL              * /*aStatistics*/,
                                                       void                * aHeader,
                                                       void                * aDumpObj,
                                                       iduFixedTableMemory * aMemory );
    static IDE_RC buildRecordForReplSenderSentLogCount( idvSQL              * /*aStatistics*/,
                                                        void                * aHeader,
                                                        void                * aDumpObj,
                                                        iduFixedTableMemory * aMemory );

    static IDE_RC buildRecordForReplicatedTransGroupInfo( idvSQL              * /*aStatistics*/,
                                                          void                * aHeader,
                                                          void                * aDumpObj,
                                                          iduFixedTableMemory * aMemory );

    static IDE_RC buildRecordForReplicatedTransSlotInfo( idvSQL              * /*aStatistics*/,
                                                         void                * aHeader,
                                                         void                * aDumpObj,
                                                         iduFixedTableMemory * aMemory );

    static IDE_RC buildRecordForAheadAnalyzerInfo( idvSQL              * /*aStatistics*/,
                                                   void                * aHeader,
                                                   void                * aDumpObj,
                                                   iduFixedTableMemory * aMemory );

    static IDE_RC buildRecordForXLogTransfer( idvSQL              * /*aStatistics*/,
                                              void                * aHeader,
                                              void                * aDumpObj,
                                              iduFixedTableMemory * aMemory );

    static IDE_RC buildRecordForXLogfileManagerInfo( idvSQL              * /*aStatistics*/,
                                                     void                * aHeader,
                                                     void                * aDumpObj,
                                                     iduFixedTableMemory * aMemory );

    static iduFixedTableColDesc gReplManagerColDesc[];
    static iduFixedTableColDesc gReplGapColDesc[];
    static iduFixedTableColDesc gReplSyncColDesc[];
    static iduFixedTableColDesc gReplReceiverColDesc[];
    static iduFixedTableColDesc gReplReceiverParallelApplyColumnColDesc[];
    static iduFixedTableColDesc gReplReceiverTransTblColDesc[];
    static iduFixedTableColDesc gReplReceiverColumnColDesc[];
    static iduFixedTableColDesc gReplSenderColumnColDesc[];
    static iduFixedTableColDesc gReplSenderColDesc[];
    static iduFixedTableColDesc gReplSenderStatisticsColDesc[];
    static iduFixedTableColDesc gReplReceiverStatisticsColDesc[];
    static iduFixedTableColDesc gReplSenderTransTblColDesc[];
    static iduFixedTableColDesc gReplRecoveryColDesc[];
    static iduFixedTableColDesc gReplLogBufferColDesc[];
    static iduFixedTableColDesc gReplOfflineSenderInfoColDesc[];
    static iduFixedTableColDesc gReplSenderSentLogCountColDesc[];
    static iduFixedTableColDesc gReplicatedTransGroupInfoCol[];
    static iduFixedTableColDesc gReplicatedTransSlotInfoCol[];
    static iduFixedTableColDesc gAheadAnalyzerInfoCol[];
    static iduFixedTableColDesc gXLogTransferColDesc[];
    static iduFixedTableColDesc gXLogfileManagerInfoColDesc[];

    rpxReceiver      * getReceiver(const SChar* aRepName );
    rpxSender        * getSender  (const SChar* aRepName );
    static idBool          isAliveSender( const SChar * aRepName );
    static rpdSenderInfo * getSenderInfo( const SChar * aRepName );

    //----------------------------------------------
    //PROJ-1541
    /* BUG-26482 대기 함수를 CommitLog 기록 전후로 분리하여 호출합니다. */
    static IDE_RC waitForReplicationBeforeCommit( idvSQL          * aStatistics,
                                                  const smTID       aTID,
                                                  const smSN        aLastSN,
                                                  const UInt        aReplModeFlag );

    static IDE_RC waitBeforeCommitInLazy( idvSQL* aStatistics, const smSN aLastSN  );
    static ULong  getMaxWaitTransTime( smSN aLastSN );
    static IDE_RC waitBeforeCommitInEager( idvSQL         * aStatistics,
                                           const smTID      aTID,
                                           const smSN       aLastSN,
                                           const UInt       aReplModeFlag );

    static void  waitForReplicationAfterCommit( idvSQL        * aStatistics,
                                                const smTID     aTID,
                                                const smSN      aBeginSN,
                                                const smSN      aLastSN,
                                                const UInt      aReplModeFlag,
                                                const smiCallOrderInCommitFunc aCallOrder);

    static void waitAfterCommitInEager( idvSQL        * aStatistics,
                                        const smTID     aTID,
                                        const smSN      aLastSN,
                                        const UInt      aReplModeFlag,
                                        const smiCallOrderInCommitFunc aCallOrder );

    static void waitAfterCommitInConsistent( const smTID     aTID,
                                             const smSN      aBeginSN,
                                             const smSN      aLastSN,
                                             const UInt      aReplModeFlag );

    static void servicesWaitAfterCommit( const smTID     aTID,
                                         const smSN      aBeginSN,
                                         const smSN      aLastSN,
                                         const UInt      aReplModeFlag,
                                         const UInt      aRequestWaitMode );

    static IDE_RC serviceWaitAfterCommitAtLeastOneSender( const smTID     aTID,
                                                          const smSN      aLastSN,
                                                          const UInt      aReplModeFlag );

    static IDE_RC waitForReplicationGlobalTxAfterPrepare(  idvSQL       * aStatistics,
                                                           idBool         aIsRequestNode,
                                                           const smTID    aTID,
                                                           const smSN     aSN );
 
    static void   servicesWaitAfterPrepare( idBool      aIsRequestNode,
                                            const smTID aTID, 
                                            const smSN  aLastSN );

    inline idBool isExit() { return mExitFlag; }
    //----------------------------------------------

    // BUG-14898 Replication Give-up
    static IDE_RC getMinimumSN(const UInt * aRestartRedoFileNo,
                               const UInt * aLastArchiveFileNo,
                               smSN       * aSN);

    static IDE_RC getDistanceFromCheckPoint( const smSN        aRestartSN,
                                             const UInt      * aRestartRedoFileNo,
                                             SLong           * aDistanceFromChkpt );

    static IDE_RC giveupReplication( iduVarMemList         * aMemory,
                                     smiStatement          * aParentStatement,
                                     rpdReplications       * aReplication,
                                     rpxSender             * aSender,
                                     smSN                    aCurrentSN,
                                     const UInt            * aLastArchiveFileNo,
                                     SLong                   aDistanceFromChkpt,
                                     const idBool            aIsSenderStartAfterGiveup,
                                     rpdLockTableManager   * aLockTable,
                                     smSN                  * aMinNeedSN );

    static IDE_RC checkAndGiveupReplication( iduVarMemList  * aMemory,
                                             rpdReplications * aReplication,
                                             smSN              aCurrentSN,
                                             const UInt      * aRestartRedoFileNo,
                                             const UInt      * aLastArchiveFileNo,
                                             const UInt        aReplicationMaxLogFile,
                                             const idBool      sIsSenderStartAfterGiveup,
                                             smSN            * aMinNeedSN, //proj-1608
                                             idBool          * aIsGiveUp ); 

    static IDE_RC checkAndGiveupRecovery( SChar           * aReplName,
                                          smSN              aCurrentSN,
                                          const UInt      * aRestartRedoFileNo,
                                          const UInt        aReplicationRecoveryMaxLogFile,
                                          smSN            * aMinNeedSN );

    /*PROJ-1670 Log Buffer for Replication*/
    static void copyToRPLogBuf(idvSQL * aStatistics,
                               UInt     aSize,
                               SChar  * aLogPtr,
                               smLSN    aLSN);
    /*PROJ-1608 recovery from replication*/
    IDE_RC loadRecoveryInfos(SChar* aRepName);
    IDE_RC saveAllRecoveryInfos();

    static IDE_RC recoveryRequest( idBool           * aExitFlag,
                                   rpdReplications  * aReplication,
                                   idBool           * aIsError,
                                   rpMsgReturn      * aResult );
    rprRecoveryItem* getRecoveryItem(const SChar* aRepName);
    rprRecoveryItem* getMergedRecoveryItem(const SChar* aRepName, smSN aRecoverySN);
    smSN             getMinReplicatedSNfromRecoveryItems(SChar * aRepName);
    IDE_RC startRecoverySenderThread(SChar         * aReplName,
                                     rprSNMapMgr   * aSNMapMgr,
                                     smSN            aActiveRPRecoverySN,
                                     rpxSender    ** aRecoverySender);
    IDE_RC stopRecoverySenderThread(rprRecoveryItem * aRecoveryItem,
                                    idvSQL          * aStatistics);

    //BUG-22703 : Begin Statement를 수행한 후에 Hang이 걸리지 않아야 합니다.
    // aStatistics  통계 정보 파라메터를 추가 합니다.
    static IDE_RC removeRecoveryItem(rprRecoveryItem * aRecoveryItem, 
                                     idvSQL          * aStatistics);
    static IDE_RC removeRecoveryItemsWithName(SChar  * aRepName,
                                              idvSQL * aStatistics);
    static IDE_RC createRecoveryItem( rprRecoveryItem * aRecoveryItem,
                                      const SChar     * aRepName,
                                      idBool            aNeedLock );
    static IDE_RC updateInvalidRecovery(smiStatement * aSmiStmt,
                                        SChar *        aRepName,
                                        SInt           aValue);
    static IDE_RC updateInvalidRecoverys(rpdReplications * sReplications,
                                          UInt              aCount,
                                          SInt              aValue);
    static IDE_RC updateAllInvalidRecovery( SInt aValue );
    static IDE_RC updateOptions( smiStatement  * aSmiStmt,
                                 SChar         * aRepName,
                                 SInt            aOptions );
    static IDE_RC getMinRecoveryInfos(SChar* aRepName, smSN*  aMinSN);

    static IDE_RC writeTableMetaLog(void        * aQcStatement,
                                    smOID         aOldTableOID,
                                    smOID         aNewTableOID);

    static IDE_RC getReplItemCount(smiStatement     *aSmiStmt,
                                   SChar            *aReplName,
                                   SInt              aItemCount,
                                   SChar            *aLocalUserName,
                                   SChar            *aLocalTableName,
                                   SChar            *aLocalPartName,
                                   rpReplicationUnit aReplicationUnit,
                                   UInt             *aResultItemCount);

    static IDE_RC  addOfflineStatus( SChar * aRepName );
    static void    removeOfflineStatus( SChar * aRepName, idBool *aIsFound );
    static void    setOfflineStatus( SChar * aRepName, RP_OFFLINE_STATUS aStatus, idBool *aIsFound );
    static void    getOfflineStatus( SChar * aRepName, RP_OFFLINE_STATUS * aStatus, idBool *aIsFound );

    static void    setOfflineCompleteFlag( SChar * aRepName, idBool aCompleteFlag, idBool * aIsFound );
    static void    getOfflineCompleteFlag( SChar * aRepName, idBool * aCompleteFlag, idBool * aIsFound );

    static void beginWaitEvent(idvSQL *aStatistics, idvWaitIndex aWaitEventName);
    static void endWaitEvent(idvSQL *aStatistics, idvWaitIndex aWaitEventName);

    inline static SChar * getServerID()
        {
            return (mMyself != NULL) ? mMyself->mServerID : (SChar *)"";
        }

    /* archive log를 읽을 수 있는 ALA인가? */
    inline static idBool isArchiveALA(SInt aRole)
        {
            if ( ( ( aRole == RP_ROLE_ANALYSIS ) || 
                   ( aRole == RP_ROLE_ANALYSIS_PROPAGATION ) ) &&
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

    static void   makeImplSPNameArr();
    inline static SChar * getImplSPName(UInt aDepth)
        {
            IDE_DASSERT((aDepth != 0) &&
                        (aDepth <= SMI_STATEMENT_DEPTH_MAX));
            return mImplSPNameArr[aDepth - 1];
        }

    inline static idBool isStartupFailback()
        {
            if ( mMyself == NULL )
            {
                return ID_FALSE;
            }
            return ( mMyself->mToDoFailbackCount > 0 ) ? ID_TRUE : ID_FALSE;
        }

    inline static idBool isInitRepl() { return mIsInitRepl; }

    static IDE_RC addLastSNEntry( iduMemPool * aSNPool,
                                  smSN         aSN,
                                  iduList    * aSNList );
    static rpxSNEntry * searchSNEntry( iduList * aSNList, smSN aSN );
    static void removeSNEntry( iduMemPool * aSNPool, rpxSNEntry * aSNEntry );
    static void printTcpInfo( cmiProtocolContext   * aProtocolContext,
                              SChar * aRepName );

private:
    rpcDDLSyncManager   mDDLSyncManager;

    iduMutex            mRecoveryMutex;
    iduMutex            mOfflineStatusMutex;
    UShort              mTCPPort;
    UShort              mIBPort;
    SInt                mMaxReplSenderCount;
    SInt                mMaxReplReceiverCount;
    rpxSender         **mSenderList;
    idBool              mExitFlag;

    rpcReceiverList     mReceiverList;

    iduMutex            mTempSenderListMutex;
    iduList             mTempSenderList;

    // BUG-15362 & PROJ-1541
    rpdSenderInfo     **mSenderInfoArrList;
    //PROJ-1608 recovery from replication
    SInt                mMaxRecoveryItemCount;
    //mRecoveryList(운영중)와 mToDoRecoveryCount(시작할때)는 recovery mutex를 이용하여 동기화 함
    rprRecoveryItem    *mRecoveryItemList;
    UInt                mToDoRecoveryCount;
    smSN                mRPRecoverySN;

    cmiLink            *mListenLink[RP_LINK_IMPL_MAX];
    cmiDispatcher      *mDispatcher;

    // Failback for Eager Replication
    UInt                mToDoFailbackCount;

    /* PROJ-1915 Meta 보관 : ActiveServer의 Sender에서 받은 Meta를 보관한다. */
    rpdMeta            *mRemoteMetaArray; /* 리시버 개수 만큼 생성한다. */

    /* BUG-25960 : V$REPOFFLINE_STATUS
     * CREATE REPLICATION options OFFILE / ALTER replication OFFLINE ENABLE 수행시 addOfflineStatus()
     * ALTRE replication OFFLINE DISABLE / DROP replication 수행시 removeOfflineStatus()
     * ALTER replication START with OFFLINE 수행시 setOfflineStatus() (STARTED, 예외시 FAILED)
     * ALTER replication START with OFFLINE 종료시 setOfflineStatus() (END , SUCCESS_COUNT 증가)
     */
    rpxOfflineInfo     *mOfflineStatusList;

    SChar               mServerID[IDU_SYSTEM_INFO_LENGTH + 1];

    UInt                mReplSeq;
    
    static idBool       mIsInitRepl;

    iduLatch            mSenderLatch;

    rpdStatistics       mRpStatistics;

    /* BUG-31374 Implicit Savepoint 이름의 배열 */
    static SChar        mImplSPNameArr[SMI_STATEMENT_DEPTH_MAX][RP_SAVEPOINT_NAME_LEN + 1];

    IDE_RC getUnusedReceiverIndexFromReceiverList( SInt * aReceiverListIndex );

private:

    IDE_RC    setRemoteMeta( SChar         * aRepName,
                             rpdMeta      ** aRemoteMeta );

    void      removeRemoteMeta( SChar    * aRepName );

    IDE_RC getUnusedRecoveryItemIndex( SInt * aRecoveryIndex );

    IDE_RC prepareRecoveryItem( cmiProtocolContext * aProtocolContext,
                                rpxReceiver * aReceiver,
                                SChar       * aRepName,
                                rpdMeta     * aMeta,
                                SInt        * aRecoveryItemIndex );

    IDE_RC createAndInitializeReceiver( cmiProtocolContext   * aProtocolContext,
                                        smiStatement         * aParentStatement,
                                        SChar                * aReceiverRepName,
                                        rpdMeta              * aMeta,
                                        rpReceiverStartMode    aReceiverMode,
                                        rpxReceiver         ** aReceiver );

    void sendHandshakeAckWithErrMsg( cmiProtocolContext * aProtocolContext,
                                     idBool             * aExitFlag,
                                     const SChar        * aErrMsg );

    idBool checkExistConsistentReceiver( SChar * aRepName );
    idBool checkNoHandshakeReceiver( SChar * aRepName );
        
    /* PROJ-2677 DDL synchronization */
    IDE_RC recvOperationInfo( cmiProtocolContext * aProtocolContext,
                              idBool             * aExitFlag,
                              UChar              * aOpCode );

    IDE_RC getReplSeq( SChar                  * aReplName,
                       rpReceiverStartMode      aReceiverMode,
                       UInt                     aParallelID,
                       UInt                   * aReplSeq );

    IDE_RC startNoHandshakeReceiverThread( void  * aQcStatement, 
                                           SChar * aRepName );

    IDE_RC startReceiverThread( cmiProtocolContext      * aProtocolContext,
                                iduVarMemList           * aMemory,
                                SChar                   * aReplName,
                                rpdMeta                 * aRemoteMeta,
                                rpdLockTableManager     * aLockTable );

    static IDE_RC rebuildTableInfo( void            * aQcStatement,
                                    qcmTableInfo    * aOldTableInfo );
    static IDE_RC rebuildTableInfoArray( void                   * aQcStatement,
                                         qciTableInfo          ** aOldTableInfoArray,
                                         UInt                     aCount,
                                         qciTableInfo         *** aOutNewTableInfoArray);

    static IDE_RC recreateTableAndPartitionInfo( void                  * aQcStatement,
                                                 qcmTableInfo          * aOldTableInfo,
                                                 qcmPartitionInfoList  * aOldPartitionInfoList );

    static IDE_RC recreatePartitionInfoList( void                  * aQcStatement,
                                             qcmPartitionInfoList  * aOldPartInfoList,
                                             qcmTableInfo          * aOldTableInfo );

    static void recoveryTableAndPartitionInfoArray( qcmTableInfo           ** aTableInfoArray,
                                                    qcmPartitionInfoList   ** aPartInfoListArray,
                                                    UInt                      aTableCount );

    static void recoveryTableAndPartitionInfo( qcmTableInfo           * aTableInfo,
                                               qcmPartitionInfoList   * aPartInfoList );

    static void destroyTableAndPartitionInfo( qcmTableInfo         * aTableInfo,
                                              qcmPartitionInfoList * aPartInfoList );

    static void destroyTableAndPartitionInfoArray( qcmTableInfo         ** aTableInfoArray,
                                                   qcmPartitionInfoList ** aPartInfoListArray,
                                                   UInt                    aCount );

    /* BUG-42851 */
    static IDE_RC updateReplTransWaitFlag( void                * aQcStatement,
                                           SChar               * aRepName,
                                           idBool                aIsTransWaitFlag,
                                           SInt                  aTableID,
                                           qcmTableInfo        * aPartInfo,
                                           smiTBSLockValidType   aTBSLvType );

    static IDE_RC updateReplicationFlag( void         * aQcStatement,
                                         qcmTableInfo * aTableInfo,
                                         SInt           aEventFlag,
                                         qriReplItem  * aReplItem );

    static void fillRpdReplItems( const qciNamePosition            aRepName,
                                  const qriReplItem        * const aReplItem,
                                  const qcmTableInfo       * const aTableInfo,
                                  const qcmTableInfo       * const aPartInfo,
                                        rpdReplItems       *       aQcmReplItems );

    static void fillRpdReplItemsByOldItem( rpdOldItem * aOldItem, rpdReplItems * aReplItem );

    static IDE_RC validateAndLockAllPartition( void                 * aQcStatement,
                                               qcmPartitionInfoList * aPartInfoList,
                                               smiTableLockMode       aTableLockMode );
    static IDE_RC insertOneReplItem( void              * aQcStatement,
                                     UInt                aReplMode,
                                     UInt                aReplOptions,
                                     rpdReplItems      * aReplItem,
                                     qcmTableInfo      * aTableInfo,
                                     qcmTableInfo      * aPartInfo );

    static IDE_RC deleteOneReplItem( void              * aQcStatement,
                                     rpdReplications   * aReplication,
                                     rpdReplItems      * aReplItem,
                                     qcmTableInfo      * aTableInfo,
                                     qcmTableInfo      * aPartInfo,
                                     idBool              aIsPartition );
    static IDE_RC insertOnePartitionForDDL( void                * aQcStatement,
                                            rpdReplications     * aReplication,
                                            rpdReplItems        * aReplItem,
                                            qcmTableInfo        * aTableInfo,
                                            qcmTableInfo        * aPartInfo );
    static IDE_RC deleteOnePartitionForDDL( void                * aQcStatement,
                                            rpdReplications     * aReplication,
                                            rpdReplItems        * aReplItem,
                                            qcmTableInfo        * aTableInfo,
                                            qcmTableInfo        * aPartInfo );

    static IDE_RC splitPartition( void             * aQcStatement,
                                  rpdReplications  * aReplication,
                                  rpdReplItems     * aSrcReplItem,
                                  rpdReplItems     * aDstReplItem1,
                                  rpdReplItems     * aDstReplItem2,
                                  qcmTableInfo     * aTableInfo,
                                  qcmTableInfo     * aSrcPartInfo,
                                  qcmTableInfo     * aDstPartInfo1,
                                  qcmTableInfo     * aDstPartInfo2 );

    static IDE_RC mergePartition( void             * aQcStatement,
                                  rpdReplications  * aReplication,
                                  rpdReplItems     * aSrcReplItem1,
                                  rpdReplItems     * aSrcReplItem2,
                                  rpdReplItems     * aDstReplItem,
                                  qcmTableInfo     * aTableInfo,
                                  qcmTableInfo     * aSrcPartInfo1,
                                  qcmTableInfo     * aSrcPartInfo2,
                                  qcmTableInfo     * aDstPartInfo );

    static IDE_RC dropPartition( void             * aQcStatement,
                                 rpdReplications  * aReplication,
                                 rpdReplItems     * aSrcReplItem,
                                 qcmTableInfo     * aTableInfo );

    static IDE_RC checkSenderAndRecieverExist( SChar    * aRepName );

public:
    static IDE_RC lockTables( void                * aQcStatement,
                              idBool                aIsValidate,
                              rpdMeta             * aMeta,
                              smiTBSLockValidType   aTBSLvType );

    static IDE_RC getTableInfoArrAndRefCount( smiStatement  *aSmiStmt,
                                              rpdMeta       *aMeta,
                                              qcmTableInfo **aTableInfoArr,
                                              SInt          *aRefCount,
                                              SInt          *aCount );

    static IDE_RC makeTableInfoIndex( void     *aQcStatement,
                                      SInt      aReplItemCnt,
                                      SInt     *aTableInfoIdx,
                                      UInt     *aReplObjectCount );

    static IDE_RC splitPartitionForAllRepl( void         * aQcStatement,
                                            qcmTableInfo * aTableInfo,
                                            qcmTableInfo * aSrcPartInfo,
                                            qcmTableInfo * aDstPartInfo1,
                                            qcmTableInfo * aDstPartInfo2 );

    static IDE_RC mergePartitionForAllRepl( void         * aQcStatement,
                                            qcmTableInfo * aTableInfo,
                                            qcmTableInfo * aSrcPartInfo1,
                                            qcmTableInfo * aSrcPartInfo2,
                                            qcmTableInfo * aDstPartInfo );

    static IDE_RC dropPartitionForAllRepl( void         * aQcStatement,
                                           qcmTableInfo * aTableInfo,
                                           qcmTableInfo * aSrcPartInfo );

    static rpdReplItems * searchReplItem( rpdMetaItem  * aReplMetaItems,
                                          UInt           aItemCount,
                                          smOID          aTableOID );

    static IDE_RC validateFailover( void * aQcStatement );
    static IDE_RC executeFailover( void * aQcStatement );

    /* PTOJ-2677 */
    static IDE_RC ddlSyncBegin( qciStatement * aQciStatement );

    static IDE_RC ddlSyncEnd( smiTrans * aDDLTrans );

    static IDE_RC ddlSyncBeginInternal( idvSQL              * aStatistics,
                                        cmiProtocolContext  * aProtocolContext,
                                        smiTrans            * aDDLTrans,
                                        idBool              * aExitFlag,
                                        rpdVersion          * aVersion,
                                        SChar               * aRepName,
                                        SChar              ** aUserName,
                                        SChar              ** aSql );
    static IDE_RC ddlSyncEndInternal( smiTrans * aDDLTrans );

    static void   ddlSyncException( smiTrans * aDDLTrans ); 

    static IDE_RC checkRemoteNormalReplVersion( cmiProtocolContext * aProtocolContext,
                                                idBool             * aExitFlag,
                                                rpdReplications    * aReplication,
                                                rpMsgReturn        * aResult,
                                                SChar              * aErrMsg,
                                                UInt               * aMsgLen );

    static IDE_RC getReplHosts( smiStatement     * aSmiStmt,
                                rpdReplications  * aReplications,
                                rpdReplHosts    ** aReplHosts );


    static IDE_RC buildReceiverNewMeta( smiStatement * aStatement, SChar * aRepName );
    static void   removeReceiverNewMeta( SChar * aRepName );

    static void   setDDLSyncCancelEvent( SChar * aRepName );

    static IDE_RC  waitLastProcessedSN( idvSQL * aStatistics,
                                        idBool * aExitFlag,
                                        SChar  * aRepName,
                                        smSN     aLastSN );

    static IDE_RC setSkipUpdateXSN( SChar * aRepName, idBool aIsSkip );

    static rpcDDLReplInfo * findDDLReplInfoByName( SChar * aRepName );

    static IDE_RC buildTempSyncMeta( smiStatement * aRootStmt,
                                     SChar        * aRepName,
                                     rpdReplHosts * aRemoteHost,
                                     rpdReplSyncItem * aSyncItemList,
                                     rpdMeta         * aMeta );

    static IDE_RC attemptHandshakeForTempSync( void              ** aHBT,
                                               cmiProtocolContext * aProtocolContext,
                                               rpdReplications    * aReplication, 
                                               rpdReplSyncItem    * aItemList,
                                               rpdVersion         * aVersion );
 
    static void releaseHandshakeForTempSync( void              ** aHBT,
                                             cmiProtocolContext * aProtocolContext );


    static IDE_RC sendTempSyncInfo( void                  * aHBTResource,
                                    cmiProtocolContext    * aProtocolContext,
                                    idBool                * aExitflag,
                                    rpdReplications       * aReplication,
                                    rpdReplSyncItem       * aItemList,
                                    UInt                    aTimeoutSec );

    static IDE_RC recvTempSyncInfo( cmiProtocolContext * aProtocolContext,
                                    idBool             * aExitflag,
                                    rpdReplications    * aReplication,
                                    rpdReplSyncItem   ** aItemList,
                                    UInt                 aTimeoutSec );

    static IDE_RC recvTempSync( cmiProtocolContext * aProtocolContext, 
                                smiTrans           * aTrans,
                                rpdMeta            * aMeta,
                                idBool               aEndianDiff );

    IDE_RC startTempSyncThread( cmiProtocolContext * aProtocolContext, 
                                rpdReplications    * aReplication,
                                rpdVersion         * aVersion,
                                rpdReplSyncItem    * aTempSyncItemList );

    void   addToTempSyncSenderList( rpxTempSender * aTempSyncSender );
    IDE_RC realizeTempSyncSender( idvSQL * aStatistics );

    static IDE_RC recoveryConditionSync( rpdReplications * aReplications,
                                         UInt              aReplCnt );

    static IDE_RC isDDLAsycReplOption( void         * aQcStatement,
                                       qcmTableInfo * aSrcPartInfo,
                                       idBool       * aIsDDLReplOption );

private:
    static void     sendXLog( const SChar * aLogPtr ); 
    static idBool   needReplicationByType( const SChar * aLogPtr );
    static ULong    convertBufferSizeToByte( UChar aType, ULong aBufSize );

    static IDE_RC   initRemoteData( SChar * aRepName );

    IDE_RC startXLogfileFailbackMasterSenderThread( SChar         * aReplName );

    IDE_RC   getUnCompleteGlobalTxList( SChar * aRepName, iduList * aGlobalTxList );

    IDE_RC   waitReceiverThread( idvSQL * aStatistics, SChar * aRepName );
};

#endif  /* _O_RPC_MANAGER_H_ */
