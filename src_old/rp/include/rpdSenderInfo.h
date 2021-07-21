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
 

#ifndef _O_RPD_SENDER_INFO_H_
#define _O_RPD_SENDER_INFO_H_

#include <idl.h>
#include <idu.h>
#include <qci.h>
#include <smDef.h>
#include <rp.h>
#include <rpdTransTbl.h>

typedef struct rpdSyncPKEntry
{
    rpSyncPKType mType;
    ULong        mTableOID;
    UInt         mPKColCnt;
    smiValue     mPKCols[QCI_MAX_KEY_COLUMN_COUNT];

    iduListNode  mNode;
} rpdSyncPKEntry;

typedef struct rpdLastProcessedSNNode
{
    smSN        mLastSN;
    smSN        mAckedSN;
} rpdLastProcessedSNNode;

class rpdSenderInfo
{
public:
    rpdSenderInfo(){};
    virtual ~rpdSenderInfo() {};

    IDE_RC initialize(UInt aSenderListIdx);
    IDE_RC initSyncPKPool( SChar * aRepName );
    void   finalize();
    void   destroy();
    IDE_RC destroySyncPKPool( idBool aSkip );

    void   checkAndRunDeactivate();
    void   deactivate();
    void   activate( rpdTransTbl *aActTransTbl,
                     smSN         aRestartSN,
                     SInt         aReplMode,
                     SInt         aRole,
                     UInt         aSenderListIdx,
                     rpxSender ** aAssignedTransactionTable );
    void   setDDLTransaction(smTID aTransID);
    void   setAbortTransaction(UInt     aAbortTxCount,
                               rpTxAck *aAbortTxList);

    void   setClearTransaction(UInt     aClearTxCount,
                               rpTxAck *aClearTxList);

    smSN   getRestartSN();
    void   setRestartSN(smSN aRestartSN);
    
    smSN   getRmtLastCommitSN();
    void   setRmtLastCommitSN(smSN aRmtLastCommitSN);

    smSN   getLastProcessedSN();
    smSN   getLastArrivedSN();
    
    /* BUG-26482 대기 함수를 CommitLog 기록 전후로 분리하여 호출합니다. */
    idBool serviceWaitBeforeCommit( smSN aLastSN,
                                    UInt aTxReplMode,
                                    smTID aTransID,
                                    UInt aRequestWaitMode,
                                    idBool *aWaitedLastSN );
    void   serviceWaitAfterCommit( smSN aBeginSN,
                                   smSN aLastSN,
                                   UInt aTxReplMode,
                                   smTID aTransID,
                                   UInt aRequestWaitMode );
    
    void   serviceWaitAfterPrepare( smSN   aLastSN, 
                                    smTID  aTransID,
                                    idBool aIsRequestNode );

    void   signalToAllServiceThr( idBool aForceAwake, smTID aTID );
    void   senderWait(PDL_Time_Value aAbsWaitTime);

    void   isTransAbort(smTID   aTID, 
                        UInt    aTxReplMode, 
                        idBool *aIsAbort, 
                        idBool *aIsActive);
    void   signalToAllServiceThr();
    void   setAckedValue( smSN     aLastProcessedSN,
                          smSN     aLastArrivedSN,
                          smSN     aAbortTxCount,
                          rpTxAck *aAbortTxList,
                          smSN     aClearTxCount,
                          rpTxAck *aClearTxList,
                          smTID    aTID );

    idBool isActiveTrans(smTID aTID);

    void                setSenderStatus(RP_SENDER_STATUS aStatus);
    RP_SENDER_STATUS    getSenderStatus();
    inline SInt         getReplMode(){ return mReplMode; }

    inline void         setPeerFailbackEnd(idBool aFlag)
                            { mIsPeerFailbackEnd = aFlag; }
    inline idBool       getPeerFailbackEnd(){ return mIsPeerFailbackEnd; }

    IDE_RC              addLastSyncPK(rpSyncPKType  aType,
                                      ULong         aTableOID,
                                      UInt          aPKColCnt,
                                      smiValue     *aPKCols,
                                      idBool       *aIsFull);
    void                getFirstSyncPKEntry(rpdSyncPKEntry **aSyncPKEntry);
    void                removeSyncPKEntry(rpdSyncPKEntry * aSyncPKEntry);

    void    getRepName( SChar * aOutRepName );
    void    setRepName( SChar * aRepName );

    inline void setFlagRebuildIndex( idBool aFlag ) { mIsRebuildIndex = aFlag; }
    inline idBool getFlagRebuildIndex() { return mIsRebuildIndex; }

    inline void setFlagTruncate( idBool aFlag ) { mIsTruncate = aFlag; }
    inline idBool getFlagTruncate() { return mIsTruncate; }


    ULong getWaitTransactionTime( smSN aCurrentSN );
    void setThroughput( UInt aThroughput );

    inline void increaseReconnectCount( void ) { mReconnectCount++; }
    inline void initReconnectCount( void ) { mReconnectCount = 0; }
    inline void setOriginReplMode( SInt aReplMode ) { mOriginReplMode = aReplMode; }
    void        serviceWaitForNetworkError( void );

    rpdSenderInfo * getAssignedSenderInfo( smTID     aTransID );

    idBool isWrittenCommitXLog( smSN aLastSN, smTID aTID);
    idBool isActive() { return mIsActive; }

    void   setGlobalTxAckRecvSN( smTID aTID, smSN aSN );
    smSN   getGlobalTxAckRecvSN( smTID aTID );

private:
    smSN calcurateCurrentGap( smSN aCurrentSN );

private:
    iduMutex            mServiceSNMtx;
    iduCond             mServiceWaitCV;
    smSN                mLastProcessedSN;
    smSN                mLastArrivedSN;
    
    //Transaction의 Commit성공 여부와 관련된 자료구조
    iduMutex            mActTransTblMtx;
    rpdTransTbl        *mActiveTransTbl;

    /*
     *  PROJ-2453
     */
    iduMutex            mAssignedTransTableMutex;
    rpxSender        ** mAssignedTransTable;

    UInt                mSenderListIndex;

    UInt                mTransTableSize;

    //Sender의 정보를 다른 스레드들과 공유, Flush시 Service Thread와 공유
    //일반적인 상황에서 SenderApply와 Sender가 공유
    iduMutex            mSenderSNMtx;
    iduCond             mSenderWaitCV;
    smSN                mRestartSN;
    smSN                mRmtLastCommitSN;   // RemoteLastCommitSN
    SInt                mReplMode;
    SInt                mRole;
    idBool              mIsSenderSleep;

    //mIsActive를 update하기 위해서는 모든 Mutex를 획득해야함(activate,deActivate)
    idBool              mIsActive;

    // Failback을 위해 필요한 데이터
    RP_SENDER_STATUS    mSenderStatus;
    idBool              mIsPeerFailbackEnd;
    iduMutex            mSyncPKMtx;
    iduMemPool          mSyncPKPool;
    iduList             mSyncPKList;
    UInt                mSyncPKListMaxSize;
    UInt                mSyncPKListCurSize;
    idBool              mIsSyncPKPoolAllocated;

    /* SYS_REPLICATIONS_ Meta Table과 같은 생명 주기를 가진 Replication Name */
    iduMutex            mRepNameMtx;
    SChar               mRepName[QCI_MAX_NAME_LEN + 1];
    /* PROJ-2184 sync성능 개선 */
    idBool               mIsRebuildIndex;
    idBool               mIsTruncate;

    UInt                mThroughput;
    
    idBool              mIsSkipUpdateXSN;

    idBool              mIsWaitOnFailback;

public:
    SInt                mOriginReplMode;
    UInt                mReconnectCount;

public:
    void wakeupEagerSender();

    /* PROJ-2453 */
private:
    rpdLastProcessedSNNode * mLastProcessedSNTable;

private:
    void        checkAndWaitToApply( smTID      aTransID,
                                     smSN       aBeginSN,
                                     smSN       aLastSN,
                                     idBool     aIsRequestNode,
                                     UInt       aRequestWaitMode,
                                     idBool   * aWaitedLastSN );
    idBool      needWait( RP_SENDER_STATUS  aSenderStatus,
                          smTID             aTransID,
                          smSN              aBeginSN,
                          smSN              aLastSN,
                          idBool            aIsRequestNode,
                          UInt              aRequestWaitMode,
                          idBool            aAlreadyLocked,
                          idBool          * aWaitedLastSN );

    void        removeAssignedSender( smTID    aTransID );

public:
    void        setLastSN( smTID aTID, smSN aLastSN );
    void        setAckedSN( smTID aTID, smSN aAckedSN );
    idBool      isTransAcked( smTID aTransID );
    idBool      isAllTransFlushed( smSN aCurrentSN );
    void        initializeLastProcessedSNTable( void );
    IDE_RC      waitLastProcessedSN( idvSQL * aStatistics,
                                     idBool * aExitFlag,
                                     smSN     aLastSN );
    idBool      isReplicationDDLTrans(smTID    aTID);
    void        setSkipUpdateXSN( idBool aIsSkip );
    idBool      getSkipUpdateXSN( void );

    inline UInt getSenderListIndex( void )
    {
        return mSenderListIndex;
    }
};

#endif //_O_RPD_SENDER_INFO_H_
