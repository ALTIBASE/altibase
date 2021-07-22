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
 * $Id: rpxReceiverApply.h 89548 2020-12-16 08:33:05Z yoonhee.kim $
 **********************************************************************/

#ifndef _O_RPX_RECEIVER_APPLY_H_
#define _O_RPX_RECEIVER_APPLY_H_ 1

#include <idl.h>

#include <smiTrans.h>
#include <smiDef.h>

#include <qtc.h>

#include <rp.h>
#include <rpdTransTbl.h>
#include <rpdMeta.h>
#include <rpdQueue.h>
#include <rpdSenderInfo.h>
#include <rpsSmExecutor.h>

#define RP_LOB_MTD_HEADER_SIZE    (8)

#define RP_CONFLICT_ERRLOG2() { IDE_ERRLOG(IDE_RP_2); IDE_ERRLOG(IDE_RP_CONFLICT_2); }

class rpxReceiver;
class smiStatement;

class rpxReceiverApply;
typedef IDE_RC (*rpxApplyFunc)(rpxReceiverApply   *aApply,
                               rpdXLog            *aXLog);

/*
 * if there is no property, then POLICY_BY_PROPERTY is same to POLICY_CHECK.
 */
typedef enum
{
    RPX_APPLY_POLICY_BY_PROPERTY = 0,
    RPX_APPLY_POLICY_CHECK,
    RPX_APPLY_POLICY_FORCE,
} RPX_APPLY_POLICY;

class rpxReceiverApply
{
public:
    rpxReceiverApply();
    virtual ~rpxReceiverApply() {};

    IDE_RC initialize( rpxReceiver * aReceiver, rpReceiverStartMode aStartMode );

    void   destroy();

    /* BUG-38533 numa aware thread initialize */
    IDE_RC initializeInLocalMemory( void );
    void   finalizeInLocalMemory( void );

    IDE_RC apply( rpdXLog * aXLog );

    void   shutdown(); // called by Replication Executor

    inline rpdTransTbl* getTransTbl() { return mTransTbl; }

    void setTransactionFlagReplReplicated( void );
    void setTransactionFlagReplRecovery( void );
    void setTransactionFlagCommitWriteWait( void );
    void setTransactionFlagCommitWriteNoWait( void );

    void setFlagToSendAckForEachTransactionCommit( void );
    void setFlagNotToSendAckForEachTransactionCommit( void );

    void setApplyPolicyCheck( void );
    void setApplyPolicyForce( void );
    void setApplyPolicyByProperty( void );

    smSN getApplyXSN( void );
    ULong getInsertSuccessCount( void );
    ULong getInsertFailureCount( void );
    ULong getUpdateSuccessCount( void );
    ULong getUpdateFailureCount( void );
    ULong getDeleteSuccessCount( void );
    ULong getDeleteFailureCount( void );
    ULong getCommitCount( void );
    ULong getAbortCount( void );

    IDE_RC buildRecordForReplReceiverTransTbl( void                    * aHeader,
                                               void                    * aDumpObj,
                                               iduFixedTableMemory     * aMemory,
                                               SChar                   * aRepName,
                                               UInt                      aParallelID,
                                               SInt                      aApplierIndex );

    /* PROJ-1915 : 리시버가 Ack를 보내기 전에 Restart SN을 보관 */
    smSN           mRestartSN;
    smiStatement   mSmiStmt;
    smiTableCursor mCursor;
    idBool mIsBegunSyncStmt;
    idBool mIsOpenedSyncCursor;
    rpdMetaItem * mRemoteTable;
    UInt mSyncTableNumber;

private:
    void   finalize(); // called when thread stop

    // private member functions.
    IDE_RC execXLog(rpdXLog *aXLog);

    IDE_RC insertSQL( smiTrans      * aSmiTrans,
                      rpdMetaItem   * aMetaItem,
                      rpdXLog       * aXLog,
                      SChar         * aSPName );

    IDE_RC insertXLog( smiTrans         * aSmiTrans,
                       rpdMetaItem      * aMetaItem,
                       rpdXLog          * aXLog,
                       SChar            * aSPName );

    IDE_RC insertReplace( rpdXLog     * aXLog,
                          rpdMetaItem * aMetaItem,
                          rpApplyFailType * aFailType,
                          idBool        aCheckRowExistence,
                          SChar       * aSpName);

    IDE_RC insertSyncXLog( smiTrans     * aSmiTrans,
                           rpdMetaItem  * aMetaItem,
                           rpdXLog      * aXLog );
    IDE_RC updateSQL( smiTrans      * aSmiTrans,
                      rpdMetaItem   * aMetaItem,
                      rpdXLog       * aXLog );
    IDE_RC updateXLog( smiTrans         * aSmiTrans,
                       rpdMetaItem      * aMetaItem,
                       rpdXLog          * aXLog );

    IDE_RC deleteXLog( smiTrans        *aSmiTrans,
                       rpdXLog         *aXLog );
    IDE_RC openLOBCursor( smiTrans     *aSmiTrans,
                          rpdXLog      *aXLog,
                          rpdTransTbl  *aTransTbl );
    IDE_RC closeLOBCursor( smiTrans    *aSmiTrans,
                           rpdXLog     *aXLog,
                           rpdTransTbl  *aTransTbl );
    IDE_RC prepareLOBWrite( smiTrans   *aSmiTrans,
                            rpdXLog    *aXLog,
                            rpdTransTbl  *aTransTbl );
    IDE_RC finishLOBWrite( smiTrans    *aSmiTrans,
                           rpdXLog     *aXLog,
                           rpdTransTbl  *aTransTbl );
    IDE_RC trimLOB( smiTrans    *aSmiTrans,
                    rpdXLog     *aXLog,
                    rpdTransTbl *aTransTbl );
    IDE_RC writeLOBPiece( smiTrans     *aSmiTrans,
                          rpdXLog      *aXLog,
                          rpdTransTbl  *aTransTbl );
    IDE_RC closeAllLOBCursor( smiTrans     *aSmiTrans,
                              smTID        aTID );

    void   changeToHex(SChar * aResult,
                       SChar * aSource,
                       SInt    aLength);

    IDE_RC getKeyRange( rpdMetaItem        *aMetaItem,
                        smiRange           *aKeyRange,
                        smiValue           *aColArray,
                        idBool              aIsPKColArray );

    void   printPK( ideLogEntry   & aLog,
                    const SChar   * aPrefix,
                    rpdMetaItem   * aMetaItem,
                    smiValue      * aSmiValueArray );
    
    IDE_RC insertErrLog( ideLogEntry     &aLog,
                         rpdMetaItem     *aMetaItem,
                         rpdXLog         *aXLog,
                         rpApplyFailType aFailType,
                         const SChar     * = "" );

    IDE_RC updateErrLog( ideLogEntry     &aLog,
                         rpdMetaItem     *aMetaItem,
                         rpdMetaItem     *aMetaItemForPK,
                         rpdXLog         *aXLog,
                         idBool           aIsCmpBeforeImg /* BUG-36555 */, 
                         rpApplyFailType  aFailType,
                         const SChar     * = "" );
    
    IDE_RC deleteErrLog( ideLogEntry     &aLog,
                         rpdMetaItem     *aMetaItem,
                         rpdXLog         *aXLog,
                         rpApplyFailType  aFailType,
                         const SChar     * = "" );
 
    void lobErrLog( ideLogEntry     &aLog,
                    rpdMetaItem     *aMetaItem,
                    rpdXLog         *aXLog,
                    rpApplyFailType  aFailType,
                    const SChar     * = "" );
   
    void   commitConflictLog( ideLogEntry &aLog,
                              rpdXLog     *aXLog );
    void   abortConflictLog( ideLogEntry &aLog,
                             rpdXLog     *aXLog );
    void   abortToSavepointConflictLog( ideLogEntry &aLog,
                                        rpdXLog     *aXLog,
                                        SChar       *aSavepointName );

    void   commitErrLog();
    void   abortErrLog();

    IDE_RC begin( smiTrans * aTrans, idBool aIsConflictResolutionTX );
    IDE_RC abort( rpdXLog * aXLog );
    IDE_RC commit( rpdXLog * aXLog );

    IDE_RC convertValue( SChar     * aDest,
                         UInt        aDestMax,
                         mtcColumn * aColumn,
                         smiValue   * aSmiValue );

    void   addAbortTx(smTID aTID, smSN aSN);
    void   addClearTx(smTID aTID, smSN aSN);

    IDE_RC getLocalFlushedRemoteSN( smSN aRemoteFlushSN,
                                    smSN aRestartSN,
                                    smSN * aLocalFlushedRemoteSN );

    void insertSingleQuotation( SChar     *aValueData,
                                mtcColumn *aColumn, 
                                smiValue  *aSmiValue );

    IDE_RC runDML( smiStatement     * aRootSmiStmt,
                   rpdMetaItem      * aRemoteMetaItem,
                   rpdMetaItem      * aLocalMetaItem,
                   rpdXLog          * aXLog,
                   SChar            * aQueryString,
                   idBool             aCompareBeforeImage,
                   SLong            * aRowCount,
                   rpApplyFailType  * aFailType );

    IDE_RC getCheckRowExistenceAndResolutionNeed( smiTrans        * aSmiTrans,
                                                  rpdMetaItem     * aMetaItem,
                                                  smiRange        * aKeyRange,
                                                  rpdXLog         * aXLog,
                                                  idBool          * aCheckRowExistence,
                                                  idBool          * aIsResolutionNeed,
                                                  rpApplyFailType * aFailType );

    IDE_RC insertReplaceSQL( rpdMetaItem        * aLocalMetaItem,
                             rpdMetaItem        * aRemoteMetaItem,
                             rpdXLog            * aXLog,
                             smiRange           * aKeyRange,
                             idBool               aCheckRowExistence,
                             SChar              * aSPName,
                             SChar              * aQueryString,
                             rpApplyFailType    * aFailType );

    void printInsertErrLog( rpdMetaItem    * aMetaItem,
                            rpdMetaItem    * aMetaItemForPK,
                            rpdXLog        * aXLog,
                            rpApplyFailType  aFailType );
                            
    void printUpdateErrLog( rpdMetaItem   * aMetaItem,
                            rpdMetaItem   * aMetaItemForPK,
                            rpdXLog       * aXLog,
                            idBool          aCompareBeforeImage,
                            rpApplyFailType aFailType );

    void printDeleteErrLog( rpdMetaItem    * aMetaItem,
                            rpdXLog        * aXLog,
                            rpApplyFailType  aFailType );

    void printLobErrLog( rpdMetaItem    * aMetaItem,
                         rpdXLog        * aXLog,
                         rpApplyFailType  aFailType );
                            
    IDE_RC executeSQL( smiStatement   * aSmiStatement,
                       rpdMetaItem    * aRemoteMeta,
                       rpdMetaItem    * aLocalMeta,
                       rpdXLog        * aXLog,
                       SLong          * sRowCount );

    IDE_RC prepare( qciStatement              * aQciStatement,
                    smiStatement              * aSmiStatement,
                    qciSQLPlanCacheContext    * aPlanCacheContext,
                    SChar                     * aSQLBuffer,
                    UInt                        aSQLBufferLength );

    IDE_RC bind( qciStatement     * aQciStatement,
                 rpdMetaItem      * aMeta,
                 rpdXLog          * aXLog,
                 qciBindParam     * aBindParam,
                 smiValue         * aConvertCols );

    IDE_RC execute( qciStatement      * aQciStatement,
                    smiStatement      * aSmiStatement,
                    SLong             * aRowCount );

    IDE_RC makeBindSQL( qciStatement      * aQciStatement,
                        rpdMetaItem       * aLocalMeta,
                        rpdXLog           * aXLog,
                        SChar             * aSQLBuffer,
                        UInt                aSQLBufferLength );

    IDE_RC getConfictResolutionTransaction( smTID        aTID,
                                            smiTrans  ** aTrans );

    IDE_RC executeInsert( smiTrans         * aSmiTrans,
                          rpdXLog          * aXLog,
                          SChar            * aSpName );
    
    IDE_RC executeSyncInsert( smiTrans    * aSmiTrans,
                              rpdXLog     * aXLog );
    
    IDE_RC executeUpdate( smiTrans         * aSmiTrans,
                          rpdXLog          * aXLog );
    
    void insertSkip( rpdMetaItem    * aMetaItem,
                     rpdXLog        * aXLog );
    
    void updateSkip( rpdMetaItem    * aMetaItem,
                     rpdXLog        * aXLog );
    
    // private member data
    SChar           * mRepName;

    rpxReceiver     * mReceiver;

    rpdSenderInfo       * mSenderInfo;

    rpTxAck             * mAbortTxList;
    UInt                  mAbortTxCount;
    rpTxAck             * mClearTxList;
    UInt                  mClearTxCount;
    rpsSmExecutor         mSmExecutor;

    rpdTransTbl         * mTransTbl;

    smSN                  mLastCommitSN;

    //proj-1608 recovery from replication
    rprSNMapMgr*          mSNMapMgr;

    /* BUG-31545 수행시간 통계정보 */
    idvSQL              * mStatistics;

    UInt                  mTransactionFlag;

    idBool                mAckForTransactionCommit;

    RPX_APPLY_POLICY mPolicy;

    RP_ROLE               mRole;

    // Performace View 전용
    smSN        mApplyXSN;
    ULong       mInsertSuccessCount;
    ULong       mInsertFailureCount;
    ULong       mUpdateSuccessCount;
    ULong       mUpdateFailureCount;
    ULong       mDeleteSuccessCount;
    ULong       mDeleteFailureCount;
    ULong       mCommitCount;
    ULong       mAbortCount;

    SChar          mImplSvpName[RP_SAVEPOINT_NAME_LEN + 1];

    /* BUG-38533 numa aware thread initialize */
    rpReceiverStartMode   mStartMode;

    /* Key range 구성을 할때 사용하는 메모리 영역,
     * Performance를 위해 rpxReceiverApply 내에 미리 메모리를 할당해 놓고,
     * 반복하여 그 메모리를 재사용하도록 한다.
     * Used Only - rpxReceiverApply::getKeyRange()
     */
    qriMetaRangeColumn  * mRangeColumn;
    UInt                  mRangeColumnCount;

    SChar               * mSQLBuffer;
    UInt                  mSQLBufferLength;

    ULong       mSyncTupleSuccessCount;

public:
    //proj-1608 recovery from replication
    void   setSNMapMgr( rprSNMapMgr * aSNMapMgr );

    IDE_RC buildXLogAck( rpdXLog * aXLog, rpXLogAck * aAck );

    idBool isTimeToSendAck( void );

    void resetCounterForNextAck( void );
    void checkAndResetCounter( void );
        
    inline SInt getATransCntFromTransTbl( void )
    {
        return mTransTbl->getATransCnt();
    }

    IDE_RC allocRangeColumn( UInt     aCount );

    smSN getRestartSN( void );
    smSN getLastCommitSN( void );

    static IDE_RC applyRebuildIndices( rpxReceiverApply * );
    static IDE_RC applyUpdateConditionAct( rpxReceiverApply *, idBool );
    static IDE_RC executeTruncate( rpxReceiver      * aReceiver,
                                   rpdMetaItem      * aMetaItem,
                                   idBool             aIsConditionSynced );

    // Apply function list
    static rpxApplyFunc mApplyFunc[RP_X_MAX];

    static IDE_RC applyTrBegin(rpxReceiverApply *, rpdXLog *);
    static IDE_RC applyTrCommit(rpxReceiverApply *, rpdXLog *);
    static IDE_RC applyTrAbort(rpxReceiverApply *, rpdXLog *);
    static IDE_RC applyInsert(rpxReceiverApply *, rpdXLog *);
    static IDE_RC applySyncInsert( rpxReceiverApply *, rpdXLog * );
    static IDE_RC applyUpdate(rpxReceiverApply *, rpdXLog *);
    static IDE_RC applyDelete(rpxReceiverApply *, rpdXLog *);
    static IDE_RC applySPSet(rpxReceiverApply *, rpdXLog *);
    static IDE_RC applySPAbort(rpxReceiverApply *, rpdXLog *);
    static IDE_RC applyLobCursorOpen(rpxReceiverApply *, rpdXLog *);
    static IDE_RC applyLobCursorClose(rpxReceiverApply *, rpdXLog *);
    static IDE_RC applyLobPrepareWrite(rpxReceiverApply *, rpdXLog *);
    static IDE_RC applyLobPartialWrite(rpxReceiverApply *, rpdXLog *);
    static IDE_RC applyLobFinishWrite(rpxReceiverApply *, rpdXLog *);
    static IDE_RC applyLobTrim(rpxReceiverApply *, rpdXLog *);
    static IDE_RC applySyncPKBegin(rpxReceiverApply *, rpdXLog *);
    static IDE_RC applySyncPK(rpxReceiverApply *, rpdXLog *);
    static IDE_RC applySyncPKEnd(rpxReceiverApply *, rpdXLog *);
    static IDE_RC applyFailbackEnd(rpxReceiverApply *, rpdXLog *);
    static IDE_RC applyIgnore(rpxReceiverApply *, rpdXLog *);
    static IDE_RC applyNA(rpxReceiverApply *, rpdXLog *);
    static IDE_RC applySyncStart( rpxReceiverApply *, rpdXLog * );
    static IDE_RC applySyncEnd( rpxReceiverApply *, rpdXLog * );
    static IDE_RC applyAckOnDML( rpxReceiverApply *, rpdXLog * ); /* PROJ-2453 */
    static IDE_RC applyTruncate( rpxReceiverApply * aApply, rpdXLog * aXLog );
    SChar* getSvpNameAndEtc( SChar           *  aXLogSPName,
                             rpSavepointType *  aType,
                             UInt            *  aImplicitSvpDepth,
                             SChar           *  aImplSvpName  );
    SChar* getSvpName( UInt aDepth, SChar * aImplSvpName  );

};

#endif  /* _O_RPX_RECEIVER_APPLY_H_ */

