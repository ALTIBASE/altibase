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
 * $Id: smxTrans.h 90824 2021-05-13 05:35:21Z minku.kang $
 **********************************************************************/

#ifndef _O_SMX_TRANS_H_
#define _O_SMX_TRANS_H_ 1

#include <ide.h>
#include <idu.h>
#include <idv.h>
#include <idl.h>
#include <smuProperty.h>
#include <sdc.h>
#include <smxOIDList.h>
#include <smxTouchPageList.h>
#include <smxSavepointMgr.h>
#include <smxTableInfoMgr.h>
#include <smxLCL.h>
#include <svrLogMgr.h>
#include <smr.h>

class  smxTransFreeList;
class  smrLogFile;
struct sdrMtxStartInfo;
class  smrLogMgr;

/* BUG-24151: [SC] Update Retry, Delete Retry, Statement Rebuild Count를
 *            AWI로 추가해야 합니다.
 *
 * Transaction의 Statistics의 aValue에 해당하는 State정보의 값을 증가시
 * 킨다. */
#define SMX_INC_SESSION_STATISTIC( aTrans, aSessStatistic, aValue ) \
    if( aTrans != NULL )                                            \
    {                                                               \
        if( ((smxTrans*)aTrans)->mStatistics != NULL )              \
        {                                                           \
             IDV_SESS_ADD( ((smxTrans*)aTrans)->mStatistics->mSess, \
                           aSessStatistic,                          \
                           aValue );                                \
        }                                                           \
    }

/*
 * BUG-39825
 * Infinite SCN 과 Lock Row SCN 둘다 최상의 bit 가 1 입니다.
 * 그래서 SM_SCN_IS_INFINITE 체크만으로는 둘 사이를 구분 할 수 없습니다.
 * Transaction SCN 을 가져오기 전에, Lock Row 에 의한 bit setting 인지 아닌지
 * 체크해 줘야 합니다.
 * */
#define SMX_GET_SCN_AND_TID( aSrcSCN, aDestSCN, aDestTID ) {             \
    SM_GET_SCN( &(aDestSCN), &(aSrcSCN) );                               \
    if ( SM_SCN_IS_INFINITE( aDestSCN ) )                                \
    {                                                                    \
        aDestTID = SMP_GET_TID( (aDestSCN) );                            \
        if ( SM_SCN_IS_NOT_LOCK_ROW( aDestSCN ) )                        \
        {                                                                \
            smxTrans::getTransCommitSCN( (aDestTID),                     \
                                         &(aSrcSCN),                     \
                                         &(aDestSCN) );                  \
        }                                                                \
    }                                                                    \
    else                                                                 \
    {                                                                    \
        aDestTID = SM_NULL_TID;                                          \
    }                                                                    \
}

#define SMX_INDOUBT_FETCH_SLEEP_COUNT  (100000)

typedef enum
{
    SMX_DIST_DEADLOCK_DETECTION_NONE = 0,
    SMX_DIST_DEADLOCK_DETECTION_VIEWSCN,
    SMX_DIST_DEADLOCK_DETECTION_TIME,
    SMX_DIST_DEADLOCK_DETECTION_SHARD_PIN_SEQ,
    SMX_DIST_DEADLOCK_DETECTION_SHARD_PIN_NODE_ID,
    SMX_DIST_DEADLOCK_DETECTION_ALL_EQUAL
} smxDistDeadlockDetection;

#define SMX_DIST_DEADLOCK_IS_DETECTED( aDetection ) \
    ( aDetection != SMX_DIST_DEADLOCK_DETECTION_NONE )
#define SMX_DIST_DEADLOCK_IS_NOT_DETECTED( aDetection ) \
    ( aDetection == SMX_DIST_DEADLOCK_DETECTION_NONE )

typedef struct smxDistDeadlockDetectionInfo
{
    smxDistDeadlockDetection mDetection;
    ULong                    mDieWaitTime;
    ULong                    mElapsedTime;
} smxDistDeadlockDetectionInfo;

class smxTrans
{
// For Operation
public:

    /* PROJ-2734 */
    void setDistTxInfo( smiDistTxInfo * aDistTxInfo );
    void clearDistTxInfo();

    static IDE_RC initializeStatic();
    static IDE_RC destroyStatic();

    IDE_RC initialize(smTID aTransID, UInt aSlotMask);
    IDE_RC destroy();


    //Transaction Management
    inline void lock();
    inline void unlock();

    // For Local & Global Transaction
    inline void initXID();
    //Replication Mode(default/eager/lazy/acked/none)PROJ-1541: Eager Replication

    IDE_RC begin(idvSQL * aStatistics,
                 UInt     aFlag,
                 UInt     aReplID,
                 idBool   aIsServiceTX = ID_FALSE );

    IDE_RC abort( idBool      aIsLegacyTrans,
                  void     ** aLegacyTrans );
    //PROJ-1677 DEQ.
    IDE_RC commit( smSCN   * aCommitSCN = NULL,
                   idBool    aIsLegacyTrans = ID_FALSE,
                   void   ** aLegacyTrans   = NULL );

    // For Layer callback.
    static IDE_RC begin4LayerCall( void* aTrans, UInt aFlag, idvSQL* aStatistics );
    static IDE_RC abort4LayerCall( void* aTrans );
    static IDE_RC commit4LayerCall( void* aTrans );
    static void  updateSkipCheckSCN( void* aTrans, idBool aDoSkipCheckSCN );

    // To know Log Flush Is Need
    static idBool isNeedLogFlushAtCommitAPrepare(void *aTrans);
    inline idBool isNeedLogFlushAtCommitAPrepareInternal();

    // For Global Transaction
    /* BUG-18981 */
    IDE_RC prepare( ID_XID *aXID, smSCN * aPrepareSCN, idBool aLogging );
    //IDE_RC forget(XID *a_pXID, idBool a_bIsRecovery = ID_FALSE);
    IDE_RC getXID(ID_XID *aXID);

    inline idBool isReadOnly();
    inline idBool isVolatileTBSTouched();

    // callback으로 사용될 isReadOnly 함수
    static idBool isReadOnly4Callback(void *aTrans);

    inline idBool isPrepared();
    inline idBool isValidXID();

    /* PROJ-2620 suspend in lock manager spin mode */
    IDE_RC suspend( smxTrans * aWaitTrans,
                    iduMutex * aMutex,
                    ULong      aTotalWaitMicroSec );

    IDE_RC suspend( smxTrans                 * aWaitTrans,
                    iduMutex                 * aMutex,
                    ULong                    * aTotalWaitMicroSec,
                    ULong                    * aElapsedMicroSec,
                    idBool                     aCheckDistDeadlock,
                    smxDistDeadlockDetection   aDistDeadlock,
                    idBool                   * aIsReleasedDistDeadlock,
                    smxDistDeadlockDetection * aNewDistDeadlock );

    IDE_RC resume();

    inline IDE_RC init();
    inline void   init(smTID aTransID);

    inline smSCN getInfiniteSCN();

    IDE_RC incInfiniteSCNAndGet(smSCN *aSCN);

    inline smSCN getMinMemViewSCN() { return mMinMemViewSCN; }

    inline smSCN getMinDskViewSCN() { return mMinDskViewSCN; }

    inline void  setMinViewSCN( UInt   aCursorFlag,
                                smSCN* aMemViewSCN,
                                smSCN* aDskViewSCN );

    inline void  initMinViewSCN( UInt   aCursorFlag );

    // Lock Slot
    inline SInt getSlotID() { return mSlotN; }

    //Set Last LSN
    void setLstUndoNxtLSN( smLSN aLSN );
    smLSN  getLstUndoNxtLSN();

    //For post commit operation
    inline IDE_RC addOID( smOID            aTableOID,
                          smOID            aRecordOID,
                          scSpaceID        aSpaceID,
                          UInt             aFlag );

    // freeSlot하는 OID를 추가한다.
    inline IDE_RC addFreeSlotOID( smOID            aTableOID,
                                  smOID            aRecordOID,
                                  scSpaceID        aSpaceID,
                                  UInt             aFlag,
                                  smSCN            aSCN );

    IDE_RC freeOIDList();
    void   showOIDList();

    smxOIDNode* getCurNodeOfNVL();

    //For save point
    IDE_RC setImpSavepoint(smxSavepoint **aSavepoint,
                           UInt           aStmtDepth);

    IDE_RC unsetImpSavepoint(smxSavepoint *aSavepoint);

    inline IDE_RC abortToImpSavepoint(smxSavepoint *aSavepoint);

    IDE_RC setExpSvpForBackupDDLTargetTableInfo( smOID   aOldTableOID, 
                                                 UInt    aOldPartOIDCount,
                                                 smOID * aOldPartOIDArray,
                                                 smOID   aNewTableOID,
                                                 UInt    aNewPartOIDCount,
                                                 smOID * aNewPartOIDArray );

    idBool isExistExpSavepoint(const SChar *aSavepointName);  /* BUG-48489 */

    void   rollbackDDLTargetTableInfo();

    IDE_RC setExpSavepoint( const SChar * aExpSVPName );

    inline IDE_RC abortToExpSavepoint(const SChar *aExpSVPName);

    /* BUG-32292 [sm-util] Self deadlock occur since fixed-table building
     * operation uses another transaction. 
     * LayerCallback을 위한 Wrapping 함수*/
    static IDE_RC setImpSavepoint4LayerCall( void          * aTrans,
                                             void         ** aSavepoint,
                                             UInt            aStmtDepth)
    {
        return ((smxTrans*)aTrans)->setImpSavepoint( (smxSavepoint**) aSavepoint, 
                                                     aStmtDepth );
    }
    static IDE_RC setImpSavepoint4Retry( void          * aTrans,
                                         void         ** aSavepoint )
    {
        return ((smxTrans*)aTrans)->setImpSavepoint(
                    (smxSavepoint**) aSavepoint,
                    ((smxTrans*)aTrans)->mSvpMgr.getStmtDepthFromImpSvpList() + 1 );
    }

    static IDE_RC unsetImpSavepoint4LayerCall( void         * aTrans,
                                               void         * aSavepoint)
    {
        return ((smxTrans*)aTrans)->unsetImpSavepoint( (smxSavepoint*) aSavepoint );
    }

    static IDE_RC abortToImpSavepoint4LayerCall( void         * aTrans,
                                                 void         * aSavepoint)
    {
        return ((smxTrans*)aTrans)->abortToImpSavepoint( (smxSavepoint*) aSavepoint );
    }

    IDE_RC addOIDList2LogicalAger( smSCN       * aCommitSCN,
                                   UInt          aAgingState,
                                   void       ** aAgerNormalList ); 
    IDE_RC end();

    IDE_RC removeAllAndReleaseMutex();


    //For Session Management
    UInt   getFstUpdateTime();

    inline SChar* getLogBuffer(){return mLogBuffer;};

    void   initLogBuffer();

    IDE_RC writeLogToBufferUsingDynArr( smuDynArrayBase  * aLogBuffer,
                                        UInt               aLogSize );

    IDE_RC writeLogToBufferUsingDynArr( smuDynArrayBase  * aLogBuffer,
                                        UInt               aOffset,
                                        UInt               aLogSize );

    IDE_RC writeLogToBuffer(const void *aLog,
                            UInt        aLogSize);

    IDE_RC writeLogToBuffer( const void   * aLog,
                             UInt           aOffset,
                             UInt           aLogSize );
    inline smLSN getBeginLSN(){return mBeginLogLSN;};
    inline smLSN getCommitLSN(){return mCommitLogLSN;};
    inline void setCommitLogLSN( smTID aTID, smLSN aLSN ) // BUG-47865
    {
        if( aTID == mTransID )
        {
            mCommitLogLSN = aLSN;
        }
    };

    static IDE_RC allocTXSegEntry( idvSQL          * aStatistics,
                                   sdrMtxStartInfo * aStartInfo );

    static inline sdcUndoSegment * getUDSegPtr( smxTrans * aTrans );

    static inline UInt getLegacyTransCnt(void * aTrans)
                             { return ((smxTrans*)aTrans)->mLegacyTransCnt; };

    inline sdcTXSegEntry * getTXSegEntry();
    inline void setTXSegEntry( sdcTXSegEntry * aTXSegEntry );

    inline static sdSID getTSSlotSID( void * aTrans );

    inline void  setTSSAllocPos( sdRID aLstExtRID,
                                 sdSID aTSSlotSID );
    inline void setUndoCurPos( sdSID    aUndoRecSID,
                               sdRID    aUndoExtRID );
    inline void getUndoCurPos( sdSID  * aUndoRecSID,
                               sdRID  * aUndoExtRID );


    // 특정 Transaction의 로그 버퍼의 내용을 로그파일에 기록한다.
    static IDE_RC writeTransLog(void *aTrans, smOID aTableOID );

    inline static smSCN  getFstDskViewSCN( void * aTrans );
    static void   setFstDskViewSCN( void  * aTrans,
                                    smSCN * aFstDskViewSCN );
    // BUG-26881 잘못된 CTS stamping으로 acces할 수 없는 row를 접근함
    static smSCN  getOldestFstViewSCN( void * aTrans );

    // api member function list
    static void     getTransInfo( void    * aTrans, 
                                  SChar  ** aTransLogBuffer,
                                  smTID   * aTID, 
                                  UInt    * aTransLogType );
    static smLSN    getTransLstUndoNxtLSN( void  * aTrans );
    /* Transaction의 현재 UndoNxtLSN을 return */
    static smLSN    getTransCurUndoNxtLSN( void  * aTrans );
    /* Transaction의 현재 UndoNxtLSN을 Set */
     static void    setTransCurUndoNxtLSN( void  * aTrans, smLSN  * aLSN );
    /* Transaction의 마지막 UndoNxtLSN을 return */
    static smLSN*   getTransLstUndoNxtLSNPtr( void  * aTrans );
    /* Transaction의 마지막 UndoNxtLSN을 set */
    static void     setTransLstUndoNxtLSN( void  * aTrans,
                                           smLSN   aLSN );
    static void     getTxIDAnLogType( void  * aTrans, 
                                      smTID * aTID,
                                      UInt  * aLogType );

#ifdef PROJ_2181_DBG
    ULong    getTxDebugID() { return    mTransIDDBG;} 

    ULong    mTransIDDBG;   
#endif

    static idBool   isTxBeginStatus(void * aTrans);

    static IDE_RC   addOIDToVerify( void    * aTrans,
                                    smOID     aTableOID,
                                    smOID     aIndexOID,
                                    scSpaceID aSpaceID );

    /* BUG-47367 SCN MAX로 설정되었을 경우 SCN을 통한 isDrop Check가 무시된다. */
    static inline IDE_RC addOID2Trans( void    * aTrans,
                                       smOID     aTblOID,
                                       smOID     aRecOID,
                                       scSpaceID aSpaceID,
                                       UInt      aFlag,
                                       smSCN     aSCN = SM_SCN_MAX );

    static IDE_RC   addOIDByTID( smTID     aTID,
                                 smOID     aTblOID,
                                 smOID     aRecordOID,
                                 scSpaceID aSpaceID,
                                 UInt      aFlag );

    static idBool   isXAPreparedCommitState(void* aTrans);

    static inline smTID getTransID( const void * aTrans );

    static void *   getTransLogFilePtr(void* aTrans);

    static void     setTransLogFilePtr(void* aTrans, void* aLogFile);

    static void     addMutexToTrans (void * aTrans, void * aMutex);

    static inline IDE_RC writeLogToBufferOfTx( void         * aTrans,
                                               const void   * aLog,
                                               UInt           aOffset,
                                               UInt           aLogSize );

    static IDE_RC   writeLogBufferWithOutOffset(void *aTrans,
                                                const void *aLog,
                                                UInt aLogSize);

    static inline void initTransLogBuffer( void * aTrans );

    static SChar *  getTransLogBuffer (void *aTrans);

    static UInt     getLogTypeFlagOfTrans(void * aTrans);

    static void     addToUpdateSizeOfTrans (void *aTrans, UInt aSize);


    static idvSQL * getStatistics( void * aTrans );
    static inline SInt     getTransSlot( void* aTrans );
    static IDE_RC   resumeTrans(void * aTrans);

    static void     getTransCommitSCN( smTID         aRecTID,
                                       const smSCN * aRecSCN,
                                       smSCN       * aOutSCN );

    static void     setTransCommitSCN( void      * aTrans,
                                       smSCN     * aSCN,
                                       void      * aStatus );

    static void     setTransPrepareSCN( void      * aTrans,
                                        smSCN     * aSCN,
                                        void      * aStatus );
 
    static void     setTransStatus( void      * aTrans,
                                    UInt        aStatus );

    static void     setTransSCNnStatus( void     * aTrans,
                                        idBool     aIsLegacyTrans,
                                        smSCN    * aSCN,
                                        void     * aStatus );

    static void trySetupMinMemViewSCN( void  * aTrans,
                                       smSCN * aViewSCN );

    static void trySetupMinDskViewSCN( void  * aTrans,
                                       smSCN * aViewSCN );

    static void trySetupMinAllViewSCN( void  * aTrans,
                                       smSCN * aViewSCN );

    IDE_RC allocViewSCN( UInt  aStmtFlag, smSCN * aSCN, smSCN aRequestSCN );

    IDE_RC executePendingList(idBool aIsCommit );
    inline idBool hasPendingOp();
    static void addPendingOperation( void  * aTrans, smuList  * aRemoveDBF );

    static idBool  getIsFirstLog( void* aTrans )
        {
            return ((smxTrans*)aTrans)->mIsFirstLog;
        };
    static void setIsFirstLog( void* aTrans, idBool aFlag )
        {
            ((smxTrans*)aTrans)->mIsFirstLog = aFlag;
        };
    static idBool checkAndSetImplSVPStmtDepth4Repl(void* aTrans);

    static void setIsTransWaitRepl( void* aTrans, idBool aIsWaitRepl )
    {
        ((smxTrans*)aTrans)->mIsTransWaitRepl = aIsWaitRepl;
    };
    idBool isReplTrans()
    {
          if ( mReplID != SMX_NOT_REPL_TX_ID )
          {
              return ID_TRUE;
          }

          return ID_FALSE; 
    }
    /*
     * BUG-33539
     * receiver에서 lock escalation이 발생하면 receiver가 self deadlock 상태가 됩니다
     */
    ULong getLockTimeoutByUSec( ULong aLockWaitMicroSec );
    ULong getLockTimeoutByUSec( );

    IDE_RC setReplLockTimeout( UInt aReplLockTimeout );
    UInt getReplLockTimeout();

    // For BUG-12512
    static idBool isPsmSvpReserved(void* aTrans)
        {
            return ((smxTrans*)aTrans)->mSvpMgr.isPsmSvpReserved();
        };
    void reservePsmSvp(idBool aIsShard );

    // TASK-7244 PSM partial rollback in Sharding
    static idBool isShardPsmSvpReserved(void* aTrans)
        {
            return ((smxTrans*)aTrans)->mSvpMgr.isShardPsmSvpReserved();
        };

    static IDE_RC writePsmSvp(void* aTrans)
        {
            return ((smxTrans*)aTrans)->mSvpMgr.writePsmSvp((smxTrans*)aTrans);
        };
    void clearPsmSvp( )
        {
            mSvpMgr.clearPsmSvp();
        };
    IDE_RC abortToPsmSvp( )
        {
            return mSvpMgr.abortToPsmSvp(this);
        };

    // PRJ-1496
    inline IDE_RC getTableInfo(smOID          aTableOID,
                               smxTableInfo** aTableInfo,
                               idBool         aIsSearch = ID_FALSE);

    inline IDE_RC undoDelete(smOID aTableOID);
    inline IDE_RC undoInsert(smOID aTableOID);
    inline IDE_RC getRecordCountFromTableInfo(smOID aOIDTable,
                                              SLong* aRecordCnt);
    inline static IDE_RC incRecordCountOfTableInfo( void  * aTrans,
                                                    smOID   aTableOID,
                                                    SLong   aRecordCnt );

    static IDE_RC undoDeleteOfTableInfo(void* aTrans, smOID aTableOID);
    static IDE_RC undoInsertOfTableInfo(void* aTrans, smOID aTableOID);

    inline static IDE_RC setExistDPathIns( void    * aTrans,
                                           smOID     aTableOID,
                                           idBool    aExistDPathIns );

    /* TableInfo를 검색하여 HintDataPID를 설정한다.. */
    static void setHintDataPIDofTableInfo( void       *aTableInfo,
                                           scPageID    aHintDataPID );

    /* TableInfo를 검색하여 HintDataPID를 반환한다. */
    static void  getHintDataPIDofTableInfo( void       *aTableInfo,
                                            scPageID  * aHintDataPID );

    // 특정 Transaction에 RSID를 부여한다.
    static void allocRSGroupID(void             *aTrans,
                               UInt             *aPageListIdx );

    // 특정 Transaction의 RSID를 가져온다.
    static UInt getRSGroupID(void* aTrans);

    // 특정 Transaction에 RSID를  aIdx를 바꾼다.
    static void setRSGroupID(void* aTrans, UInt aIdx);

    // Tx's PrivatePageList의 HashTable의 Hash Function
    inline static UInt hash(void* aKeyPtr);

    // Tx's PrivatePageList의 HashTable의 키값 비교 함수
    inline static SInt isEQ( void *aLhs, void *aRhs );

    // Tx's PrivatePageList를 반환한다.
    static IDE_RC findPrivatePageList(
                            void                     * aTrans,
                            smOID                      aTableOID,
                            smpPrivatePageListEntry ** aPrivatePageList);

    // PrivatePageList를 추가한다.
    static IDE_RC addPrivatePageList(
                            void                     * aTrans,
                            smOID                      aTableOID,
                            smpPrivatePageListEntry  * aPrivatePageList);

    // PrivatePageList를 생성한다.
    static IDE_RC createPrivatePageList(
                            void                     * aTrans,
                            smOID                      aTableOID,
                            smpPrivatePageListEntry ** aPrivatePageList );

    // Tx's PrivatePageList 정리
    IDE_RC finAndInitPrivatePageList();

    //==========================================================
    // PROJ-1594 Volatile TBS를 위해 추가된 함수들
    // Tx's PrivatePageList를 반환한다.
    static IDE_RC findVolPrivatePageList(
                            void                     * aTrans,
                            smOID                      aTableOID,
                            smpPrivatePageListEntry ** aPrivatePageList);

    // PrivatePageList를 추가한다.
    static IDE_RC addVolPrivatePageList(
                            void                     * aTrans,
                            smOID                      aTableOID,
                            smpPrivatePageListEntry  * aPrivatePageList);

    // PrivatePageList를 생성한다.
    static IDE_RC createVolPrivatePageList(
                            void                     * aTrans,
                            smOID                      aTableOID,
                            smpPrivatePageListEntry ** aPrivatePageList );

    // Tx's PrivatePageList 정리
    IDE_RC finAndInitVolPrivatePageList();
    //==========================================================

    /* BUG-30871 When excuting ALTER TABLE in MRDB, the Private Page Lists of
     * new and old table are registered twice. */
    /* PrivatePageList를 때어냅니다.
     * 이 시점에서 이미 해당 Page들은 TableSpace로 반환한 상태이기 때문에
     * Page에 달지 않습니다. */
    static IDE_RC dropMemAndVolPrivatePageList( void           * aTrans,
                                                smcTableHeader * aSrcHeader );

    // Commit이나 Abort후에 FreeSlot을 실제 FreeSlotList에 매단다.
    IDE_RC addFreeSlotPending();

    // PROJ-1362 QP Large Record & Internal LOB
    // memory lob cursor-open
    IDE_RC  openLobCursor( idvSQL           * aStatistics,
                           void             * aTable,
                           smiLobCursorMode   aOpenMode,
                           smSCN              aLobViewSCN,
                           smSCN              aInfinite,
                           void             * aRow,
                           const smiColumn  * aColumn,
                           UInt               aInfo,
                           smLobLocator     * aLobLocator );

    // disk lob cursor-open
    IDE_RC  openLobCursor( idvSQL           * aStatistics,
                           void             * aTable,
                           smiLobCursorMode   aOpenMode,
                           smSCN              aLobViewSCN,
                           smSCN              aInfinite4Disk,
                           scGRID             aRowGRID,
                           smiColumn        * aColumn,
                           UInt               aInfo,
                           smLobLocator     * aLobLocator );

    // close lob cursor
    IDE_RC closeLobCursor(idvSQL        * aStatistics,
                          smLobCursorID   aLobCursorID,
                          idBool          aIsShardLobCursor = ID_FALSE );

    /* PROJ-2728 Sharding LOB */
    // shard lob cursor-open
    IDE_RC openShardLobCursor( idvSQL           * aStatistics,
                               UInt               aMmSessId,
                               UInt               aMmStmtId,
                               UInt               aRemoteStmtId,
                               UInt               aNodeId,
                               SShort             aLobLocatorType,
                               smLobLocator       aRemoteLobLocator,
                               UInt               aInfo,
                               smiLobCursorMode   aOpenMode,
                               smLobLocator     * aLobLocator );

    IDE_RC getLobCursor(smLobCursorID aLobCursorID,
                        smLobCursor** aLobCursor,
                        idBool        aIsShardLobCursor );

    static void updateDiskLobCursors(void* aTrans, smLobViewEnv* aLobViewEnv);

    // PROJ-2068 Direct-Path INSERT 수행 시 필요한 DPathEntry를 할당
    inline static IDE_RC allocDPathEntry( smxTrans* aTrans );
    inline static void* getDPathEntry( void* aTrans );

    // BUG-24821 V$TRANSACTION에 LOB관련 MinSCN이 출력되어야 합니다.
    inline void getMinMemViewSCNwithLOB( smSCN* aSCN );
    inline void getMinDskViewSCNwithLOB( smSCN* aSCN );

    static UInt mAllocRSIdx;

    static UInt getMemLobCursorCnt(void   *aTrans, UInt aColumnID, void *aRow);

    /* Implicit Savepoint까지 IS Lock만을 해제한다.*/
    inline IDE_RC unlockSeveralLock(ULong aLockSequence);

    /* PROJ-1594 Volatile TBS */
    /* Volatile logging을 하기 위해 필요한 environment를 얻는다. */
    inline svrLogEnv *getVolatileLogEnv();

    /* BUG-15396
       Commit 시, log를 disk에 기록할때 기록할때까지 기다려야 할지
       말지에 대한 정보를 가져온다.
    */
    inline idBool isCommitWriteWait();

    static inline UInt getLstReplStmtDepth( void * aTrans );

    // PROJ-1566
    static void setExistAppendInsertInTrans( void * aTrans );

    static void setFreeInsUndoSegFlag( void * aTrans, idBool aFlag );

    /* TASK-2398 로그압축
       트랜잭션의 로그 압축/압축해제에 사용할 리소스를 리턴한다 */
    IDE_RC getCompRes(smrCompRes ** aCompRes);

    // 트랜잭션의 로그 압축/압축해제에 사용할 리소스를 리턴한다
    // (callback용)
    static IDE_RC getCompRes4Callback( void *aTrans, smrCompRes ** aCompRes );

    /* TASK-2401 MMAP Logging환경에서 Disk/Memory Log의 분리
       해당 트랜잭션이 Disk, Memory Tablespace에 접근할 경우 호출됨
     */
    void setDiskTBSAccessed();
    void setMemoryTBSAccessed();
    // 해당 트랜잭션이 Meta Table을 변경할 경우 호출됨
    void setMetaTableModified();

    static void setMemoryTBSAccessed4Callback(void * aTrans);

    static void setXaSegsInfo( void    * aTrans,
                               UInt      aTXSegEntryIdx,
                               sdRID     aExtRID4TSS,
                               scPageID  aFstPIDOfLstExt4TSS,
                               sdRID     aFstExtRID4UDS,
                               sdRID     aLstExtRID4UDS,
                               scPageID  aFstPIDOfLstExt4UDS,
                               scPageID  aFstUndoPID,
                               scPageID  aLstUndoPID );

    // PROJ-1665 Parallel Direct-Path INSERT
    static IDE_RC setTransLogBufferSize(void * aTrans,
                                        UInt   aNeedSize);

    static inline smrRTOI *getRTOI4UndoFailure(void * aTrans );

    // DDL Transaction을 표시하는 Log Record를 기록한다.
    IDE_RC writeDDLLog();

    IDE_RC addTouchedPage( scSpaceID aSpaceID,
                           scPageID  aPageID,
                           SShort    aTSSlotNum );

    void setStatistics( idvSQL * aStatistics );

    // BUG-22576
    IDE_RC addPrivatePageListToTableOnPartialAbort();

    inline void initCommitLog( smrTransCommitLog *aCommitLog,
                               smrLogType         aCommitLogType,   
                               smSCN              aCommitSCN );

    /* BUG-47525 Group Commit */
    IDE_RC addTID4GroupCommit( UInt   * aGCList, 
                               idBool * aWriteCommitLog,
                               smSCN    aCommitSCN  );

    IDE_RC writeGroupCommitLog( UInt aListNumber );

    IDE_RC waitGroupCommit( UInt aGCList );

    // BUG-29262 TSS 할당에 실패한 트랜잭션의 COMMIT 로그를 기록해야 합니다.
    inline idBool isLogWritten();

    inline UInt   getLogSize() { return mLogOffset; };
    
    static inline UInt genHashValueFunc( void* aData );
    static inline SInt compareFunc( void* aLhs, void* aRhs );

    /* BUG-40427 [sm_resource] Closing cost of a LOB cursor 
     * which is used internally is too much expensive */
    IDE_RC closeAllLobCursorsWithRPLog();

    IDE_RC closeAllLobCursors( idvSQL *aStatistics,
                               UInt    aInfo,
                               idBool  aIsClosingShardLobCursors );

    /* PROJ-2728 Sharding LOB */
    IDE_RC closeAllShardLobCursors();

    inline static void setTransactionalDDLCallback( smiTrasactionalDDLCallback * aTransactionalDDLCallback );

    void dumpTransInfo();
    
    /* PROJ-2733 분산 트랜잭션 정합성 */
    static IDE_RC waitPendingTx( smxTrans * aTrans, 
                                 smSCN      aRowSCN, 
                                 smSCN      aViewSCN );
    
    void setGlobalSMNChangeFunc( smTransApplyShardMetaChangeFunc aFunc );

    static SInt getLogBufferSize( void * aTrans );

    /* PROJ-1665 */
    IDE_RC setLogBufferSize( UInt aNeedSize );

    /* BUG-48586 */
    void setInternalTableSwap();

private:

    static void initXID( ID_XID * aXID );
    inline void initAbortLog( smrTransAbortLog *aAbortLog, 
                              smrLogType        aAbortLogType );
    inline void initPreAbortLog( smrTransPreAbortLog *aAbortLog );

    IDE_RC addOIDList2AgingList( SInt       aAgingState,
                                 smxStatus  aStatus,
                                 smLSN    * aEndLSN,
                                 smSCN    * aCommitSCN,
                                 idBool     aIsLegacyTrans );
    /* Commit Log기록 함수 */
    IDE_RC writeCommitLog( smLSN* aEndLSN, smSCN aCommitSCN );
    /* Abort Log기록후 Undo Transaction */
    IDE_RC writeAbortLogAndUndoTrans( smLSN * aEndLSN );

    IDE_RC writeCommitLog4Memory( smLSN * aEndLSN, smSCN aCommitSCN );
    IDE_RC writeCommitLog4Disk( smLSN * aEndLSN, smSCN aCommitSCN );

    // Hybrid Transaction에 대해 Log Flush실시
    IDE_RC flushLog( smLSN *aLSN, idBool aIsCommit );

    /* BUG-40427 [sm_resource] Closing cost of a LOB cursor 
     * which is used internally is too much expensive */
    IDE_RC closeAllLobCursors();

    // close lob cursor
    IDE_RC closeLobCursorInternal(idvSQL      * aStatistics,
                                  smLobCursor * aLobCursor);
    IDE_RC closeAllLobCursors( idvSQL *aStatistics,
                               UInt    aInfo );

    /* PROJ-2728 Sharding LOB */
    IDE_RC deleteShardLobCursor( smLobCursor *aLobCursor );
    IDE_RC closeAllShardLobCursors( idvSQL *aStatistics,
                                    UInt    aInfo );

    /* LobCursor에게 ID를 부여해주기 위한 Sequence 값. 
     * LobCursor가 없으면 0이다. */
    smLobCursorID    mCurLobCursorID;
    smuHashBase      mLobCursorHash;
    /* PROJ-2728 Sharding LOB */
    smLobCursorID    mCurShardLobCursorID;
    smuHashBase      mShardLobCursorHash;

    static iduMemPool   mLobCursorPool;
    static iduMemPool   mLobColBufPool;

// For Member
public:

    smiTrans          * mSmiTransPtr;

    /* PROJ-2734 */
    smiDistTxInfo                mDistTxInfo;      /* 분산데드락 체크를 위해 필요한 분산정보 저장 */ 
    smxDistDeadlockDetectionInfo mDistDeadlock4FT; /* X$DIST_LOCK_WAIT 출력용 */

    /* TASK-7219 Non-shard DML */
    idBool                       mIsPartialStmt;
    UInt                         mStmtSeq;

    /* PROJ-2733 */
    idBool             mIsGCTx; //GlobalConsistentTx:GLOBAL_TRANSACTION_LEVEL=3 으로 설정된 Tx

    // Transaction의 mFlag 정보
    // - Transactional Replication Mode Set ( PROJ-1541 )
    // - Commit Write Wait Mode ( BUG-15396 )
    UInt               mFlag;

    smTID              mTransID;
    //fix BUG-23656 session,xid ,transaction을 연계한 performance view를 제공하고,
    //그들간의 관계를 정확히 유지해야 함.
    //transaction에서 session id를 추가.
    UInt               mSessionID;

    smSCN              mMinMemViewSCN;    // Minimum Memory ViewSCN
    smSCN              mMinDskViewSCN;    // Minimum Disk ViewSCN
    smSCN              mFstDskViewSCN;    // 트랜잭션의 디스크 시작 ViewSCN
    smSCN              mCursorOpenInfSCN; // 해당 Tx에서 cursor가 열렸을때의 Infinite SCN

    // BUG-26881 잘못된 CTS stamping으로 acces할 수 없는 row를 접근함
    // 트랜잭션 시작시점에서 active transaction 중 oldestFstViewSCN을 설정
    smSCN              mOldestFstViewSCN;      

   /* PROJ-2733 */
    smSCN              mLastRequestSCN;   // 요구자 SCN 으로 시작된 마지막 Statementd의 ViewSCN
    smSCN              mPrepareSCN;

    smSCN              mCommitSCN;
    smSCN              mInfinite;

    smxStatus          mStatus;
    smxStatus          mStatus4FT;       /* FixedTable 조회용 Status */
    UInt               mLSLockFlag;

    idBool             mIsUpdate; // durable writing의 여부
    idBool             mIsTransWaitRepl; /* BUG-39143 */
    /* BUG-19245: Transaction이 두번 Free되는 것을 Detect하기 위해 추가됨 */
    idBool             mIsFree;

    // PROJ-1553 Replication self-deadlock
    // transaction을 구동시킨 replication의 ID
    // 만약 replication의 transaction이 아니라면
    // SMX_NOT_REPL_TX이 할당된다.
    UInt               mReplID;

    UInt               mLogTypeFlag;

    // For XA
    /* BUG-18981 */
    ID_XID            mXaTransID;
    smxCommitState    mCommitState;
    timeval           mPreparedTime;

    iduCond           mCondV;
    iduMutex          mMutex;
    smxTrans         *mNxtFreeTrans;
    smxTransFreeList *mTransFreeList;
    smLSN             mFstUndoNxtLSN;
    smLSN             mLstUndoNxtLSN;
    SInt              mSlotN;

    /* BUG-33895 [sm_recovery] add the estimate function 
     * the time of transaction undoing. */
    ULong             mTotalLogCount;
    ULong             mProcessedUndoLogCount;
    UInt              mUndoBeginTime;

    // PROJ-1705 Disk MVCC Renewal
    smxTouchPageList  mTouchPageList;

    // For disk manager pending List
    smuList           mPendingOp;

    smxOIDList       *mOIDList;
    // BUG-14093 FreeSlot이후 addFreeSlotPending할 OIDList
    smxOIDList        mOIDFreeSlotList;

    //For Lock
    ULong             mUpdateSize;
    //For save point
    smxSavepointMgr   mSvpMgr;
    //For xa
    smxTrans         *mPrvPT;
    smxTrans         *mNxtPT;

    // For Recovery
    smxTrans         *mPrvAT;
    smxTrans         *mNxtAT;

    /* BUG-27122 Restart Recovery 시 UTRANS가 접근하는 디스크 인덱스의
     * Verify 기능( __SM_CHECK_DISK_INDEX_INTEGRITY =2 ) 추가 */
    smxOIDList       *mOIDToVerify;

    //For Recovery
    static iduMemPool mMutexListNodePool;
    smuList           mMutexList;
    //For Session Management
    UInt              mFstUpdateTime;
    idvSQL           *mStatistics;

    SChar            *mLogBuffer;
    UInt              mLogBufferSize;

    // TASK-2398 Log Compress
    // 트랜잭션 rollback때 로그 압축 해제를 위한 버퍼의 핸들과
    // 로그기록시 로그 압축을 위한 리소스
    smrCompRes       * mCompRes;
    // 로그 압축 리소스 풀
    static smrCompResPool mCompResPool;

    UInt              mLogOffset;
    idBool            mDoSkipCheck;
    // unpin, alter table add column시 disk에 내려간
    // old version  table에 대한 트랜잭션은
    // LogicalAger에서 누락시키기 위하여 사용.
    idBool            mDoSkipCheckSCN;
    idBool            mIsDDL;
    idBool            mIsFirstLog;

    /* PROJ-1381 */
    UInt              mLegacyTransCnt;

    UInt              mTXSegEntryIdx;  // 트랜잭션 세그먼트 엔트리 순번
    sdcTXSegEntry   * mTXSegEntry;     // 트랜잭션 세그먼트 엔트리 Pointer

    smxOIDNode *      mCacheOIDNode4Insert;

    // PRJ-1496
    smxTableInfoMgr   mTableInfoMgr;
    smxTableInfo*     mTableInfoPtr;

    /* Transaction Rollback시 Undo log의 위치를 가리킨다. */
    smLSN              mCurUndoNxtLSN;
    //for Eager Replication PROJ-1541
    smLSN              mLastWritedLSN;
    //PROJ-1608 recovery from replication
    smLSN              mBeginLogLSN;
    smLSN              mCommitLogLSN;

    // For PROJ-1490
    // 트랜잭션이 속한 mRSGroupID
    UInt              mRSGroupID;

    // For PROJ-1464
    // TX's Private Free Page List Entry
    smpPrivatePageListEntry* mPrivatePageListCachePtr;
                             // PrivatePageList의 Cache 포인터
    smuHashBase              mPrivatePageListHashTable;
                             // PrivatePageList의 HashTable
    iduMemPool               mPrivatePageListMemPool;
                             // PrivatePageList의 MemPool

    // PROJ-1594 Volatile TBS
    smpPrivatePageListEntry* mVolPrivatePageListCachePtr;
    smuHashBase              mVolPrivatePageListHashTable;
    iduMemPool               mVolPrivatePageListMemPool;

    //PROJ-1362.
    /* PROJ-2174 Supporting LOB in the volatile tablespace
     * volatile용을 따로 만들지 않고, mMemLCL을 같이 사용한다. */
    smxLCL           mMemLCL;
    smxLCL           mDiskLCL;
    smxLCL           mShardLCL;

    /* PROJ-1594 Volatile TBS */
    /* Volatile logging을 위한 environment */
    svrLogEnv        mVolatileLogEnv;

    // PROJ-2068 Direct-Path INSERT 성능 개선
    void*            mDPathEntry;

    idBool           mFreeInsUndoSegFlag;

    /* PROJ-2162 RestartRiskReduction
     * Undo 실패에 대한 상황을 기록할 변수 */
    smrRTOI          mRTOI4UndoFailure;

    /* PROJ-2694 Fetch Across Rollback */
    idBool           mIsReusableRollback;
    idBool           mIsCursorHoldable;

    /* BUG-47223 */
    idBool           mIsServiceTX;

    /* BUG-47367 DeleteThread와 Tx의 연결점. 연결된 DeleteThread의 checkMutex를 포인팅 한다.
     * DeleteThread의 Tx가 아닌경우 NULL 이다. */
    void *           mConnectDeleteThread;

    /* BUG-48250 : Session Property INDOUBT_FETCH_TIMEOUT, INDOUBT_FETCH_METHOD 값을
                   TX Begin시에 smxTrans에 저장한다. */
    UInt             mIndoubtFetchTimeout;
    UInt             mIndoubtFetchMethod;

    /* BUG-48501 : DK 모듈에서 GlobalTxID를 세팅한다. */
    UInt             mGlobalTxId;

private:
    /* TASK-2401 MMAP Logging환경에서 Disk/Memory Log의 분리
       해당 트랜잭션이 Disk, Memory Tablespace에 접근했는지 여부
     */
    idBool           mDiskTBSAccessed;
    idBool           mMemoryTBSAccessed;

    // 해당 트랜잭션이 Meta Table을 변경하였는지 여부
    idBool           mMetaTableModified;
    UInt             mReplLockTimeout;

    /* BUG-45711 FAST_UNLOCK_LOG_ALLOC_MUTEX=1 이라면 
       로그가 전부 쓰여질때까지 기다려야 한다. */
    idBool           mIsUncompletedLogWait;

    /* BUG-47525 Group Commit */
    static iduMutex  * mGCMutex;    // List 동시성 제어를 위한 Mutex List 당 1개
    static smTID    ** mGCTIDArray; // TID를 모으기 위한 Array. List 당 1 Array
    static smSCN    ** mGCCommitSCNArray;// commitSCN을 모으기 위한 Array. List 당 1 Array
    static UInt      * mGCCnt;      // Array에 모인 TID(commitSCN) 갯수
    static UInt      * mGCListID;   // 각 List의 유효성을 위한 ListID;
    static UInt        mGCList;     // 현재 Collecting 중인 List의 ID
    static UInt        mGroupCnt;   // Property Group화 MAX 값
    static UInt        mGCListCnt;  // Property 총 List의 수
    static UInt      * mGCFlag;     // 대표 Flag를 지정하기 위한 값;

    smTransApplyShardMetaChangeFunc mGlobalSMNChangeFunc;

    /* BUG-48586 */
    idBool             mInternalTableSwap;
};

inline void smxTrans::setGlobalSMNChangeFunc( smTransApplyShardMetaChangeFunc aFunc )
{
    mGlobalSMNChangeFunc = aFunc;
    return ;
}

inline void smxTrans::setTSSAllocPos( sdRID   aCurExtRID,
                                      sdSID   aTSSlotSID )
{
    IDE_ASSERT( mTXSegEntry != NULL );

    mTXSegEntry->mTSSlotSID  = aTSSlotSID;
    mTXSegEntry->mExtRID4TSS = aCurExtRID;
}

inline void smxTrans::setDiskTBSAccessed()
{
    mDiskTBSAccessed = ID_TRUE;
}

inline void smxTrans::setMemoryTBSAccessed()
{
    mMemoryTBSAccessed = ID_TRUE;
}

// 해당 트랜잭션이 Meta Table을 변경할 경우 호출됨
inline void smxTrans::setMetaTableModified()
{
    mMetaTableModified = ID_TRUE;
}

inline idBool smxTrans::isReadOnly()
{
    return ((mIsUpdate == ID_TRUE) ? ID_FALSE : ID_TRUE);
}

inline idBool smxTrans::isVolatileTBSTouched()
{
    return svrLogMgr::isOnceUpdated( &mVolatileLogEnv );
}

inline idBool smxTrans::isPrepared()
{
    return ((mCommitState==SMX_XA_PREPARED) ? ID_TRUE : ID_FALSE);
}

inline void smxTrans::lock()
{
    /* always return true */
    (void)mMutex.lock( NULL );
}

inline void smxTrans::unlock()
{
    /* always return true */
    (void)mMutex.unlock();
}

inline void smxTrans::initXID()
{
    mXaTransID.gtrid_length = (vULong)-1;
    mXaTransID.bqual_length = (vULong)-1;
}

inline idBool smxTrans::isValidXID()
{
    if ( mXaTransID.gtrid_length != (vSLong)-1 &&
         mXaTransID.bqual_length != (vSLong)-1)
    {
        return ID_TRUE;
    }
    else
    {
        return ID_FALSE;
    }
}

inline void smxTrans::init(smTID aTransID)
{
    mTransID = aTransID;
}

inline smxOIDNode* smxTrans::getCurNodeOfNVL()
{
    smxOIDNode *sHeadNode = &(mOIDList->mOIDNodeListHead);

    if(sHeadNode == sHeadNode->mPrvNode)
    {
        return NULL;
    }
    else
    {
        return sHeadNode->mPrvNode;
    }
}

inline IDE_RC smxTrans::abortToImpSavepoint(smxSavepoint *aSavepoint)
{
    IDE_TEST(removeAllAndReleaseMutex() != IDE_SUCCESS);

    IDE_TEST(mSvpMgr.abortToImpSavepoint(this,
                                         aSavepoint) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

inline IDE_RC smxTrans::abortToExpSavepoint(const SChar *aExpSVPName)
{
    IDE_TEST(removeAllAndReleaseMutex() != IDE_SUCCESS);

    IDE_TEST(mSvpMgr.abortToExpSavepoint(this,aExpSVPName)
             != IDE_SUCCESS);

    /* PROJ-2462 ResultCache */
    mTableInfoMgr.addTablesModifyCount();

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

inline IDE_RC smxTrans::addOID(smOID            aTableOID,
                               smOID            aRecordOID,
                               scSpaceID        aSpaceID,
                               UInt             aFlag)
{
    IDE_ASSERT(mStatus == SMX_TX_BEGIN);
    return mOIDList->add(aTableOID, aRecordOID, aSpaceID, aFlag);
}


/******************************************************
  Description:
          freeSlot하는 OID를 추가한다.

  aTableOID  [IN]  freeSlot하는 테이블OID
  aRecordOID [IN]  freeSlot하는 레코드OID
  aFlag      [IN]  freeSlot하는 유형(Fixed or Var)
********************************************************/
inline IDE_RC smxTrans::addFreeSlotOID(smOID            aTableOID,
                                       smOID            aRecordOID,
                                       scSpaceID        aSpaceID,
                                       UInt             aFlag,
                                       smSCN            aSCN )
{
    IDE_DASSERT(aTableOID != SM_NULL_OID);
    IDE_DASSERT(aRecordOID != SM_NULL_OID);
    IDE_DASSERT((aFlag == SM_OID_TYPE_FREE_FIXED_SLOT) ||
                (aFlag == SM_OID_TYPE_FREE_VAR_SLOT) ||
                (aFlag == SM_OID_TYPE_UNLOCK_FIXED_SLOT));
    IDE_ASSERT(mStatus == SMX_TX_BEGIN);

    return mOIDFreeSlotList.add(aTableOID, aRecordOID, aSpaceID, aFlag, aSCN);
}

inline UInt smxTrans::getFstUpdateTime()
{
    return mFstUpdateTime;
}

inline smLSN smxTrans::getLstUndoNxtLSN()
{
    return mLstUndoNxtLSN;
}


inline IDE_RC smxTrans::getTableInfo(smOID          aOIDTable,
                                     smxTableInfo** aTableInfo,
                                     idBool         aIsSearch)
{
    return mTableInfoMgr.getTableInfo(aOIDTable,
                                      aTableInfo,
                                      aIsSearch);
}

inline IDE_RC smxTrans::undoDelete(smOID aOIDTable)
{
    smxTableInfo     *sTableInfoPtr = NULL;

    while(1)
    {
        if(mTableInfoPtr != NULL)
        {
            if(mTableInfoPtr->mTableOID == aOIDTable)
            {
                break;
            }
        }

        IDE_TEST(getTableInfo(aOIDTable, &sTableInfoPtr, ID_TRUE)
                 != IDE_SUCCESS);

        if(sTableInfoPtr != NULL)
        {
            mTableInfoPtr = sTableInfoPtr;
        }

        break;
    }

    if(mTableInfoPtr != NULL)
    {
        mTableInfoPtr->mRecordCnt++;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

inline IDE_RC smxTrans::undoInsert(smOID aOIDTable)
{
    smxTableInfo     *sTableInfoPtr = NULL;

    while(1)
    {
        if(mTableInfoPtr != NULL)
        {
            if(mTableInfoPtr->mTableOID == aOIDTable)
            {
                break;
            }
        }

        IDE_TEST(getTableInfo(aOIDTable, &sTableInfoPtr, ID_TRUE)
                 != IDE_SUCCESS);

        if(sTableInfoPtr != NULL)
        {
            mTableInfoPtr = sTableInfoPtr;
        }

        break;
    }

    if(mTableInfoPtr != NULL)
    {
        mTableInfoPtr->mRecordCnt--;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

inline IDE_RC smxTrans::getRecordCountFromTableInfo(smOID aOIDTable,
                                                    SLong* aRecordCnt)
{
    smxTableInfo     *sTableInfoPtr;

    sTableInfoPtr = NULL;
    *aRecordCnt = 0;

    while(1)
    {
        if(mTableInfoPtr != NULL)
        {
            if(mTableInfoPtr->mTableOID == aOIDTable)
            {
                break;
            }
        }

        IDE_TEST(getTableInfo(aOIDTable, &sTableInfoPtr, ID_FALSE)
                 != IDE_SUCCESS);

        if(sTableInfoPtr != NULL)
        {
            mTableInfoPtr = sTableInfoPtr;
        }

        break;
    }

    if(mTableInfoPtr != NULL)
    {
        *aRecordCnt = mTableInfoPtr->mRecordCnt;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*******************************************************************************
 * Description : aTrans에 달린 TableInfo에서 aTableOID에 해당하는 테이블의
 *          Record Count를 aRecordCnt만큼 증가시킨다.
 ******************************************************************************/
inline IDE_RC smxTrans::incRecordCountOfTableInfo( void  * aTrans,
                                                   smOID   aTableOID,
                                                   SLong   aRecordCnt )
{
    smxTrans      * sTrans = (smxTrans*)aTrans;
    smxTableInfo  * sTableInfoPtr;

    sTableInfoPtr = NULL;

    //--------------------------------------------------------------
    // mTableInfoPtr 캐시에 미리 aTableOID에 해당하는 Table Info가
    // 저장되어 있으면 바로 사용
    //--------------------------------------------------------------
    if( sTrans->mTableInfoPtr != NULL )
    {
        if( sTrans->mTableInfoPtr->mTableOID == aTableOID )
        {
            IDE_CONT( found_table_info );
        }
    }

    //-------------------------------------------------------------
    // Table Info를 찾을 수 없으면 TableInfoMgr에서 찾아본다.
    // 3번째 인자가 ID_TRUE이므로, TableInfoMgr의 Hash에 없으면 안된다.
    // 따라서 sTableInfoPtr이 NULL이면 예외처리한다.
    //-------------------------------------------------------------
    IDE_TEST( sTrans->getTableInfo(aTableOID, &sTableInfoPtr, ID_TRUE)
              != IDE_SUCCESS );

    IDE_ASSERT( sTableInfoPtr != NULL );

    sTrans->mTableInfoPtr = sTableInfoPtr;

    IDE_EXCEPTION_CONT( found_table_info );

    if( sTrans->mTableInfoPtr != NULL )
    {
        sTrans->mTableInfoPtr->mRecordCnt += aRecordCnt;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*******************************************************************************
 * Description: 대상 TX이 가지고 있는 table info에 DPath INSERT 수행 여부를
 *      설정한다.
 *
 * Parameters:
 *  aTrans          - [IN] 대상 TX의 smxTrans
 *  aTableOID       - [IN] DPath INSERT 수행 여부를 표시할 대상 table의 OID
 *  aExistDPathIns  - [IN] DPath INSERT 수행 여부
 ******************************************************************************/
inline IDE_RC smxTrans::setExistDPathIns( void    * aTrans,
                                          smOID     aTableOID,
                                          idBool    aExistDPathIns )
{
    smxTrans      * sTrans = (smxTrans*)aTrans;
    smxTableInfo  * sTableInfoPtr = NULL;

    //--------------------------------------------------------------
    // mTableInfoPtr 캐시에 미리 aTableOID에 해당하는 Table Info가
    // 저장되어 있으면 바로 사용
    //--------------------------------------------------------------
    if( sTrans->mTableInfoPtr != NULL )
    {
        if( sTrans->mTableInfoPtr->mTableOID == aTableOID )
        {
            sTableInfoPtr = sTrans->mTableInfoPtr;
            IDE_CONT( found_table_info );
        }
    }

    //-------------------------------------------------------------
    // Table Info를 찾을 수 없으면 TableInfoMgr에서 찾아본다.
    // 3번째 인자가 ID_TRUE이므로, TableInfoMgr의 Hash에 없으면 안된다.
    // 따라서 sTableInfoPtr이 NULL이면 예외처리한다.
    //-------------------------------------------------------------
    IDE_TEST( sTrans->mTableInfoMgr.getTableInfo( aTableOID,
                                                  &sTableInfoPtr,
                                                  ID_TRUE ) /* aIsSearch */
              != IDE_SUCCESS );

    IDE_EXCEPTION_CONT( found_table_info );

    IDE_ERROR( sTableInfoPtr != NULL );

    smxTableInfoMgr::setExistDPathIns( sTableInfoPtr,
                                       aExistDPathIns );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Tx's PrivatePageList의 HashTable의 Hash Function
 *
 * aKeyPtr [IN]  Key값에 대한 포인터
 **********************************************************************/
inline UInt smxTrans::hash(void* aKeyPtr)
{
    vULong sKey;

    IDE_DASSERT(aKeyPtr != NULL);

    sKey = *(vULong*)aKeyPtr;

    return sKey & (SMX_PRIVATE_BUCKETCOUNT - 1);
};

/***********************************************************************
 * Tx's PrivatePageList의 HashTable의 키값 비교 함수
 *
 * aLhs [IN]  비교대상1에 대한 포인터
 * aRhs [IN]  비교대상2에 대한 포인터
 **********************************************************************/
inline SInt smxTrans::isEQ( void *aLhs, void *aRhs )
{
    vULong sLhs;
    vULong sRhs;

    IDE_DASSERT( aLhs != NULL );
    IDE_DASSERT( aRhs != NULL );

    sLhs = *(vULong*)aLhs;
    sRhs = *(vULong*)aRhs;

    return (sLhs == sRhs) ? 0 : -1;
};

/***********************************************************************
 *
 * Description : Hash에 대한 Hash Key를 반환하는 함수
 *
 * smuHash 에서 Hash Key를 32비트 값으로 처리하기 때문에 Hash 함수의 반환값은
 * 32비트이다. 만약 입력인자(aData)가 8바이트값으로 들어오게 된다면 엔디안에 따라서
 * Hash Key가 중복되는 경우 많이 발생할 수도 있다.
 *
 * aData [IN] - Hash할 데이타
 *
 **********************************************************************/
inline UInt smxTrans::genHashValueFunc( void* aData )
{
    return *(UInt*)(aData);
}

/***********************************************************************
 *
 * Description : Hash Value에 대한  비교 함수
 *
 **********************************************************************/
inline SInt smxTrans::compareFunc( void* aLhs, void* aRhs )
{

    IDE_ASSERT( aLhs != NULL );
    IDE_ASSERT( aRhs != NULL );

    if (*((UInt*)(aLhs)) == *((UInt*)(aRhs)) )
    {
        return (0);
    }

    return (-1);
}

/*************************************************************************
 * Description: Implicit Savepoint까지 IS Lock만을 해제한다.
 * ***********************************************************************/
inline IDE_RC smxTrans::unlockSeveralLock(ULong aLockSequence)
{
    return mSvpMgr.unlockSeveralLock(this, aLockSequence);
}

/*************************************************************************
 * Description: Volatile logging을 위해 필요한 svrLogEnv 포인터를 얻는다.
 *************************************************************************/
inline svrLogEnv* smxTrans::getVolatileLogEnv()
{
    return &mVolatileLogEnv;
}

/****************************************************************************
 * Description : 트랜잭션 Abort 로그를 초기화한다.
 * 
 * aAbortLog     - [IN] AbortLog 포인터
 * aAbortLogType - [IN] AbortLog 타입
 ****************************************************************************/
inline void smxTrans::initAbortLog( smrTransAbortLog *aAbortLog,
                                    smrLogType        aAbortLogType )
{
    smrLogHeadI::setType( &( aAbortLog->mHead ), aAbortLogType );
    smrLogHeadI::setSize( &( aAbortLog->mHead ),
                          SMR_LOGREC_SIZE(smrTransAbortLog) +
                          ID_SIZEOF(smrLogTail));
    smrLogHeadI::setTransID( &( aAbortLog->mHead ), mTransID);
    smrLogHeadI::setFlag( &( aAbortLog->mHead ), mLogTypeFlag);
    aAbortLog->mDskRedoSize = 0;

    aAbortLog->mGlobalTxId = mGlobalTxId;
}

inline void smxTrans::initPreAbortLog( smrTransPreAbortLog *aAbortLog )
{
    smrLogHeadI::setType( &( aAbortLog->mHead ), SMR_LT_TRANS_PREABORT );
    smrLogHeadI::setSize( &( aAbortLog->mHead ),
                          SMR_LOGREC_SIZE( smrTransPreAbortLog ) );
    smrLogHeadI::setTransID( &( aAbortLog->mHead ), mTransID);
    smrLogHeadI::setFlag( &( aAbortLog->mHead ), mLogTypeFlag);

    aAbortLog->mTail = SMR_LT_TRANS_PREABORT;
}

/****************************************************************************
 * Description : 트랜잭션 Commit 로그를 초기화한다.
 * 
 * aCommitLog     - [IN] CommitLog 포인터
 * aCommitLogType - [IN] CommitLog 타입
 ****************************************************************************/
inline void smxTrans::initCommitLog(  smrTransCommitLog *aCommitLog,
                                      smrLogType         aCommitLogType,
                                      smSCN              aCommitSCN )
{
    SChar *sCurLogPtr;

    smrLogHeadI::setType( &aCommitLog->mHead, aCommitLogType );
    smrLogHeadI::setSize( &aCommitLog->mHead,
                          SMR_LOGREC_SIZE(smrTransCommitLog) + ID_SIZEOF(smrLogTail) );
    smrLogHeadI::setTransID (&aCommitLog->mHead, mTransID );
    smrLogHeadI::setFlag( &aCommitLog->mHead, mLogTypeFlag );

    aCommitLog->mDskRedoSize = 0;

    aCommitLog->mGlobalTxId = mGlobalTxId;
    aCommitLog->mCommitSCN  = aCommitSCN;

    /* BUG-24866
     * [valgrind] SMR_SMC_PERS_WRITE_LOB_PIECE 로그에 대해서
     * Implicit Savepoint를 설정하는데, mReplSvPNumber도 설정해야 합니다. */
    smrLogHeadI::setReplStmtDepth( &aCommitLog->mHead,
                                   SMI_STATEMENT_DEPTH_NULL );

    sCurLogPtr = (SChar*)aCommitLog + SMR_LOGREC_SIZE(smrTransCommitLog);
    smrLogHeadI::copyTail(sCurLogPtr, &aCommitLog->mHead);
}

inline idBool smxTrans::isLogWritten()
{
    return ((mLogOffset > 0)? ID_TRUE : ID_FALSE);
}

/*
   fix BUG-15480

   트랜잭션이 Commit 이나 Abort Pending Operation을
   가지고 있는지 여부를 반환한다.

   [ 인자 ]

   [IN] aTrans  - 트랜잭션 객체
*/
inline idBool smxTrans::hasPendingOp()
{
    idBool sHasPendingOp;

    if( SMU_LIST_IS_EMPTY( &mPendingOp ) )
    {
        sHasPendingOp = ID_FALSE;
    }
    else
    {
        sHasPendingOp = ID_TRUE;
    }

    return sHasPendingOp;
}

/*************************************************************************
  BUG-15396
  Description: Commit 시, log가 disk에 내려갈때까지 기다림 여부에 대한
                정보 반환
 *************************************************************************/
inline idBool smxTrans::isCommitWriteWait()
{
    idBool sIsWait;

    if ( (mFlag & SMX_COMMIT_WRITE_MASK )
         == SMX_COMMIT_WRITE_WAIT )
    {
        sIsWait = ID_TRUE;
    }
    else
    {
        sIsWait = ID_FALSE;
    }

    return sIsWait;
}


/**************************************************************
 * Descrition: Log Flush가 필요한지를 검증
 *
 * aTrans - [IN] Transaction Pointer
 *
**************************************************************/
inline idBool smxTrans::isNeedLogFlushAtCommitAPrepareInternal()
{
    idBool sNeedLogFlush = ID_FALSE;

    // log가 disk에 기록될때까지 기다릴지에 대한 정보 획득
    if ( ( hasPendingOp() == ID_TRUE ) ||
         ( isCommitWriteWait() == ID_TRUE ) )
    {
        sNeedLogFlush = ID_TRUE;
    }
    else
    {
        /* nothing to do ...  */
    }

    return sNeedLogFlush;
}

inline sdcUndoSegment* smxTrans::getUDSegPtr( smxTrans * aTrans )
{
    IDE_ASSERT( aTrans->mTXSegEntry != NULL );

    return &(aTrans->mTXSegEntry->mUDSegmt);
}

inline sdcTXSegEntry* smxTrans::getTXSegEntry()
{
    return mTXSegEntry;
}

inline void smxTrans::setTXSegEntry( sdcTXSegEntry * aTXSegEntry )
{
    mTXSegEntry    = aTXSegEntry;
    if ( aTXSegEntry != NULL )
    {
        mTXSegEntryIdx = aTXSegEntry->mEntryIdx;
    }
    else
    {
        mTXSegEntryIdx = ID_UINT_MAX;
    }
}

inline void smxTrans::setUndoCurPos( sdSID    aUndoRecSID,
                                     sdRID    aUndoExtRID )
{
    IDE_ASSERT( mTXSegEntry != NULL );

    if ( mTXSegEntry->mFstUndoPID == SD_NULL_PID )
    {
        IDE_ASSERT( mTXSegEntry->mFstUndoSlotNum == SC_NULL_SLOTNUM );
        IDE_ASSERT( mTXSegEntry->mFstExtRID4UDS  == SD_NULL_RID );
        IDE_ASSERT( mTXSegEntry->mLstUndoPID     == SD_NULL_PID );
        IDE_ASSERT( mTXSegEntry->mLstUndoSlotNum == SC_NULL_SLOTNUM );
        IDE_ASSERT( mTXSegEntry->mLstExtRID4UDS  == SD_NULL_RID );

        mTXSegEntry->mFstUndoPID     = SD_MAKE_PID( aUndoRecSID );
        mTXSegEntry->mFstUndoSlotNum = SD_MAKE_SLOTNUM( aUndoRecSID );
        mTXSegEntry->mFstExtRID4UDS  = aUndoExtRID;
    }

    mTXSegEntry->mLstUndoPID     = SD_MAKE_PID( aUndoRecSID );
    mTXSegEntry->mLstUndoSlotNum = SD_MAKE_SLOTNUM( aUndoRecSID ) + 1;
    mTXSegEntry->mLstExtRID4UDS = aUndoExtRID;
}

inline void smxTrans::getUndoCurPos( sdSID  * aUndoRecSID,
                                     sdRID  * aUndoExtRID )
{
    if ( mTXSegEntry != NULL )
    {
       if ( mTXSegEntry->mLstUndoPID == SD_NULL_PID )
       {
           *aUndoRecSID = SD_NULL_SID;
           *aUndoExtRID = SD_NULL_RID;
       }
       else
       {
           *aUndoRecSID = SD_MAKE_SID( mTXSegEntry->mLstUndoPID,
                                       mTXSegEntry->mLstUndoSlotNum );
           *aUndoExtRID = mTXSegEntry->mLstExtRID4UDS;
       }
    }
    else
    {
        *aUndoRecSID = SD_NULL_SID;
        *aUndoExtRID = SD_NULL_RID;
    }
}

/***********************************************************************
 *
 * Description :
 *  infinite scn 값을 반환한다.
 *
 **********************************************************************/
inline smSCN smxTrans::getInfiniteSCN()
{
    return mInfinite;
}

/***************************************************************************
 *
 * Description: 트랜잭션의 MemViewSCN 혹은 DskViewSCN 혹은 모두 갱신한다.
 *
 * End하는 statement을 제외하고 나머지 statement들의 SCN 값중 제일 작은 값으로 갱신한다.
 * 위에서 제일 작은 값이 현재 transaction의 viewSCN과 같은 경우에는  갱신을 skip한다.
 * End할때 다른 Stmt가 없으면, 트랜잭션의 ViewSCN을 infinite로 한다.
 *
 * getViewSCNforTransMinSCN에서 트랜잭션의 MinViewSCN이 infinite인 경우에
 * 주어진 SCN값으로 갱신한다.
 *
 * aCursorFlag - [IN] Stmt의 Cursor 타입 Flag (MASK 씌운 값)
 * aMemViewSCN - [IN] 갱신할 트랜잭션의 MemViewSCN
 * aDskViewSCN - [IN] 갱신할 트랜잭션의 DskViewSCN
 *
 ****************************************************************************/
inline void smxTrans::setMinViewSCN( UInt    aCursorFlag,
                                     smSCN * aMemViewSCN,
                                     smSCN * aDskViewSCN )
{
    IDE_ASSERT( (aCursorFlag == SMI_STATEMENT_ALL_CURSOR)    ||
                (aCursorFlag == SMI_STATEMENT_MEMORY_CURSOR) ||
                (aCursorFlag == SMI_STATEMENT_DISK_CURSOR) );

    mLSLockFlag = SMX_TRANS_LOCKED;

    IDL_MEM_BARRIER;

    if ( (aCursorFlag & SMI_STATEMENT_MEMORY_CURSOR) != 0 )
    {
        IDE_ASSERT( aMemViewSCN != NULL );
        IDE_ASSERT( SM_SCN_IS_GE(aMemViewSCN, &mMinMemViewSCN) );

        SM_SET_SCN( &mMinMemViewSCN, aMemViewSCN );
    }

    if ( (aCursorFlag & SMI_STATEMENT_DISK_CURSOR) != 0 )
    {
        IDE_ASSERT( aDskViewSCN != NULL );
        IDE_ASSERT( SM_SCN_IS_GE(aDskViewSCN, &mMinDskViewSCN) );

        SM_SET_SCN( &mMinDskViewSCN, aDskViewSCN );
    }

    IDL_MEM_BARRIER;

    mLSLockFlag = SMX_TRANS_UNLOCKED;
}

/***************************************************************************
 *
 * Description: 트랜잭션의 MemViewSCN 혹은 DskViewSCN을 초기화한다.
 *
 * aCursorFlag - [IN] Stmt의 Cursor 타입 Flag (MASK 씌운 값)
 *
 ****************************************************************************/
inline void  smxTrans::initMinViewSCN( UInt   aCursorFlag )
{
    IDE_ASSERT( (aCursorFlag == SMI_STATEMENT_ALL_CURSOR)    ||
                (aCursorFlag == SMI_STATEMENT_MEMORY_CURSOR) ||
                (aCursorFlag == SMI_STATEMENT_DISK_CURSOR) );

    mLSLockFlag = SMX_TRANS_LOCKED;

    IDL_MEM_BARRIER;

    if ( (aCursorFlag & SMI_STATEMENT_MEMORY_CURSOR) != 0 )
    {
        SM_SET_SCN_INFINITE( &mMinMemViewSCN );
    }

    if ( (aCursorFlag & SMI_STATEMENT_DISK_CURSOR) != 0 )
    {
        SM_SET_SCN_INFINITE( &mMinDskViewSCN );
    }

    IDL_MEM_BARRIER;

    mLSLockFlag = SMX_TRANS_UNLOCKED;
}

/***********************************************************************
 *
 * Description : Lob과 GCTX를 고려하여 MinMemViewSCN을 반환한다.
 *
 * aSCN - [OUT] 트랜잭션의 MinMemViewSCN
 *
 **********************************************************************/
inline void smxTrans::getMinMemViewSCNwithLOB( smSCN  * aSCN )
{
    // for LOB
    mMemLCL.getOldestSCN(aSCN);
    
    // minMemViewSCN
    if ( SM_SCN_IS_GT(aSCN, &mMinMemViewSCN) )
    {
        SM_GET_SCN(aSCN, &mMinMemViewSCN);
    }
    
    // for GCTX
    if ( SM_SCN_IS_GT( aSCN, &mLastRequestSCN ) )
    {
        SM_GET_SCN( aSCN, &mLastRequestSCN );
    }
}

/***********************************************************************
 *
 * Description : Lob과 GCTX를 고려하여 MinDskViewSCN을 반환한다.
 *
 * aSCN - [OUT] 트랜잭션의 MinDskViewSCN
 *
 **********************************************************************/
inline void smxTrans::getMinDskViewSCNwithLOB( smSCN* aSCN )
{
    // for LOB
    mDiskLCL.getOldestSCN(aSCN);

    // minDskViewSCN
    if( SM_SCN_IS_GT(aSCN,&mMinDskViewSCN) )
    {
        SM_GET_SCN(aSCN,&mMinDskViewSCN);
    }

    // for GCTX
    if ( SM_SCN_IS_GT( aSCN, &mLastRequestSCN ) )
    {
        SM_GET_SCN( aSCN, &mLastRequestSCN );
    }
}

/*******************************************************************************
 * Description : [PROJ-2068] Direct-Path INSERT를 위한 Entry Point인
 *          DPathEntry를 할당한다.
 *
 * Implementation : sdcDPathInsertMgr에 요청하여 DPathEntry를 할당 받는다.
 * 
 * Parameters :
 *      aTrans  - [IN] DPathEntry를 할당 받을 Transaction의 포인터
 ******************************************************************************/
inline IDE_RC smxTrans::allocDPathEntry( smxTrans* aTrans )
{
    IDE_ASSERT( aTrans != NULL );

    if( aTrans->mDPathEntry == NULL )
    {
        IDE_TEST( sdcDPathInsertMgr::allocDPathEntry( &aTrans->mDPathEntry )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*******************************************************************************
 * Description : [PROJ-2068] aTrans에 달려있는 DPathEntry를 반환한다.
 *
 * Parameters :
 *      aTrans  - [IN] smxTrans의 포인터
 ******************************************************************************/
inline void* smxTrans::getDPathEntry( void* aTrans )
{
    return ((smxTrans*)aTrans)->mDPathEntry;
}

/*****************************************************************************
 * Description : [PROJ-2162] aTrans에 달려있는 mRTOI4UndoFailure를 반환한다.
 *
 * Parameters :
 *      aTrans  - [IN] smxTrans의 포인터
 ****************************************************************************/
inline smrRTOI * smxTrans::getRTOI4UndoFailure(void * aTrans )
{
    return &((smxTrans*)aTrans)->mRTOI4UndoFailure;
}

inline IDE_RC smxTrans::writeLogToBufferOfTx( void       * aTrans, 
                                              const void * aLog,
                                              UInt         aOffset, 
                                              UInt         aLogSize )
{
    return ((smxTrans*)aTrans)->writeLogToBuffer( aLog, aOffset, aLogSize );
}

inline smTID smxTrans::getTransID( const void * aTrans )
{
    return ((smxTrans*)aTrans)->mTransID;
}

inline void  smxTrans::initTransLogBuffer( void * aTrans )
{
    ((smxTrans*)aTrans)->initLogBuffer();
}

inline IDE_RC smxTrans::addOID2Trans( void    * aTrans,
                                      smOID     aTblOID,
                                      smOID     aRecordOID,
                                      scSpaceID aSpaceID,
                                      UInt      aFlag,
                                      smSCN     aSCN )
{
    IDE_RC sResult = IDE_SUCCESS;

    /* BUG-14558:OID List에 대한 Add는 Transaction Begin되었을 때만
       수행되어야 한다.*/
    if ( ((smxTrans*)aTrans)->mStatus == SMX_TX_BEGIN )
    {
        if ( ( aFlag == SM_OID_TYPE_FREE_FIXED_SLOT ) ||
             ( aFlag == SM_OID_TYPE_FREE_VAR_SLOT ) ||
             ( aFlag == SM_OID_TYPE_UNLOCK_FIXED_SLOT ) )
        {
            /* BUG-47367 SCN Check는 FreeSlot에 대해서만 필요하다
             * 일반적으로 add되는 OID는 아직 commit 되지 않은 상태이다. */
            // BUG-14093 freeSlot한 것들은 addFreeSlotPending할 리스트에 매단다.
            sResult = ((smxTrans*)aTrans)->addFreeSlotOID( aTblOID,
                                                           aRecordOID,
                                                           aSpaceID,
                                                           aFlag,
                                                           aSCN );
        }
        else
        {
            sResult = ((smxTrans*)aTrans)->addOID( aTblOID,
                                                   aRecordOID,
                                                   aSpaceID,
                                                   aFlag );
        }
    }

    return sResult;
}

/***********************************************************************
 * Description : 마지막으로 시작한 Statement의 Depth
 *
 * aTrans - [IN]  Transaction Pointer
 ***********************************************************************/
UInt smxTrans::getLstReplStmtDepth(void* aTrans)
{
    smxTrans * sTrans = (smxTrans*)aTrans;

    return sTrans->mSvpMgr.getLstReplStmtDepth();
}

inline SInt  smxTrans::getTransSlot( void * aTrans )
{
    return ((smxTrans *)aTrans)->mSlotN;
}

/***********************************************************************
 *
 * Description : 트랜잭션의 TSS의 SID를 반환한다.
 *
 **********************************************************************/
inline sdSID smxTrans::getTSSlotSID( void * aTrans )
{
    sdcTXSegEntry * sTXSegEntry;

    IDE_ASSERT( aTrans != NULL );

    sTXSegEntry = ((smxTrans *)aTrans)->mTXSegEntry;

    if ( sTXSegEntry != NULL )
    {
        return sTXSegEntry->mTSSlotSID;
    }

    return SD_NULL_SID;
}

/**********************************************************************
 *
 * Description : 첫번째 Disk Stmt의 ViewSCN을 반환한다.
 *
 * aTrans - [IN] 트랜잭션 포인터
 *
 **********************************************************************/
inline smSCN smxTrans::getFstDskViewSCN( void * aTrans )
{
    smSCN   sFstDskViewSCN;

    SM_GET_SCN( &sFstDskViewSCN, &((smxTrans *)aTrans)->mFstDskViewSCN );

    return sFstDskViewSCN;
}

inline UInt smxTrans::getReplLockTimeout()
{
    return mReplLockTimeout;
}

/* BUG-48586 */
inline void smxTrans::setInternalTableSwap()
{
    mInternalTableSwap = ID_TRUE;
}

#endif
