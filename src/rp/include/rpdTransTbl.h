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
 * $Id: rpdTransTbl.h 91485 2021-08-17 23:46:35Z seulki $
 **********************************************************************/

#ifndef _O_RPD_TRANSTBL_H_
#define _O_RPD_TRANSTBL_H_ 1

#include <smiTrans.h>
#include <smDef.h>
#include <smrDef.h>
#include <rpDef.h>
#include <rpdLogAnalyzer.h>
#include <rprSNMapMgr.h>

typedef enum rpdTransPoolType
{
    RPD_TRANSTBL_USING_TRANS_POOL = 0,
    RPD_TRANSTBL_USING_LOG_ANALYZER,
    RPD_TRANSTBL_USING_MEMORY_ALLOC
} rpdTransPoolType;

class rpdTransEntry
{
public:
    smiTransNode  * mRpdTrans;
    smiTransNode  * mTransForConflictResolution;
    UInt            mStatus;
    idBool          mSetPSMSavepoint;
    rpdTransEntry * mNext;

    smTID  getTransID() { return mRpdTrans->mSmiTrans.getTransID(); }
    IDE_RC commit( smTID aTID );
    IDE_RC rollback( smTID aTID, idBool aTxSlotRelease = ID_FALSE );
    IDE_RC setSavepoint( smTID aTID, rpSavepointType aType, SChar * aSavepointName );
    IDE_RC abortSavepoint( rpSavepointType aType, SChar * aSavepointName );
};

typedef struct rpdLOBLocEntry
{
    smLobLocator            mRemoteLL;
    smLobLocator            mLocalLL;
    struct rpdLOBLocEntry  *mNext;
} rpdLOBLocEntry;

typedef struct rpdItemMetaEntry
{
    smiTableMeta  mItemMeta;
    void         *mLogBody;
    smSN          mLogSN;
    iduListNode   mNode;
} rpdItemMetaEntry;

typedef struct rpdDDLStmtMetaLog
{
    smiDDLStmtMeta   mDDLStmtMeta;
    void           * mLogBody;
} rpdDDLStmtMetaLog;

typedef struct rpdSavepointEntry
{
    smSN            mSN;
    rpSavepointType mType;
    SChar           mName[RP_SAVEPOINT_NAME_LEN + 1];

    iduListNode     mNode;
} rpdSavepointEntry;

typedef struct rpdTransTblNode
{
    SChar             *mRepName;        // Performace View 전용
    RP_SENDER_TYPE     mCurrentType;    // START_FLAG
    smTID              mRemoteTID;
    smTID              mMyTID;          // Performace View 전용
    idBool             mBeginFlag;
    idBool             mAbortFlag;
    idBool             mSkipLobLogFlag; /* BUG-21858 DML + LOB 처리 */
    idBool             mSendLobLogFlag; /* BUG-24398 LOB Cursor Open의 Table OID를 확인 */
    smSN               mBeginSN;
    smSN               mAbortSN;        /* Abort 상태로 만든 XLog의 SN */
    UInt               mTxListIdx;      /* Abort/Clear Transaction List 내의 위치 */
    rpdLogAnalyzer    *mLogAnlz;
    rpdTransEntry      mTrans;
    rpdLOBLocEntry     mLocHead;
    rprSNMapEntry     *mSNMapEntry;     //proj-1608 recovery from replication

    // PROJ-1442 Replication Online 중 DDL 허용
    idBool             mIsDDLTrans;
    iduList            mItemMetaList;

    rpdDDLStmtMetaLog *mDDLStmtMetaLog;
    // BUG-28206 불필요한 Transaction Begin을 방지
    iduList            mSvpList;
    rpdSavepointEntry *mPSMSvp;
    idBool             mIsSvpListSent;

    SInt               mParallelID;
    idBool             mIsConflict;
    idBool             mSendBeginFlag;

    smTID              mGlobalTxID;
    smSN               mGlobalTxAckSN;
} rpdTransTblNode;

class rpdTransTbl
{
/* Member Function */
public:
    rpdTransTbl();
    ~rpdTransTbl();

    /* 초기화 루틴 */
    IDE_RC         initialize( rpdTransPoolType aType, UInt aTransactionPoolSize );
    /* 소멸자 */
    void           destroy();

    /* 새로운 Transaction을 시작 */
    IDE_RC insertTrans( iduMemAllocator * aAllocator,
                        smTID             aRemoteTID,
                        smSN              aBeginSN,
                        iduMemPool      * aChainedValuePool );

    /* Transaction을 종료 */
    void           removeTrans(smTID aRemoteTID);

    /* 모든 Active Transaction을 Rollback시킨다.*/
    void           rollbackAllATrans();

    /* Active Transaction이 있는가 */
    inline idBool  isThereATrans() 
    { 
        idBool      sRC = ID_FALSE;

        if ( idCore::acpAtomicGet32( &mATransCnt ) == 0 )
        {
            sRC = ID_FALSE;
        }
        else
        {
            sRC = ID_TRUE;
        }

        return sRC;
    }

    /* Begin된 Transaction의 SN중에서 가장 작은 SN값을
       return */
    void           getMinTransFirstSN( smSN * aSN );
    idBool         isExistActiveTrans();
    inline rpdTransPoolType transPoolType() { return mTransPoolType; }

    inline idBool    isATrans(smTID aRemoteTID);
    inline void      setGTrans( smTID aRemoteTID );
    inline idBool    isGTrans( smTID aRemoteTID );
    inline smiTransNode* getRpdTrans(smTID aRemoteTID);
    inline smiTrans* getSMITrans(smTID aRemoteTID);
    inline void      setBeginFlag(smTID aRemoteTID, idBool aIsBegin);
    inline idBool    getBeginFlag(smTID aRemoteTID);
    inline rpdLogAnalyzer* getLogAnalyzer(smTID aRemoteTID);
    inline rpdTransTblNode* getTransTblNodeArray() { return mTransTbl; }
    inline UInt      getTransTblSize() { return mTblSize; }
    inline idBool    isATransNode(rpdTransTblNode *aHashNode);
    inline rpdTransTblNode *getTrNode(smTID aTID) { return &mTransTbl[getTransSlotID(aTID)]; }
    /*PROJ-1541*/
    inline idBool    getAbortFlag(smTID aRemoteTID);
    inline smSN      getAbortSN(smTID aRemoteTID);
    inline void      setAbortInfo(smTID aRemoteTID, idBool aIsAbort, smSN aAbortSN);
    inline UInt      getTxListIdx(smTID aRemoteTID);
    inline void      setTxListIdx(smTID aRemoteTID, UInt aTxListIdx);
    /* BUG-21858 DML + LOB 처리 */
    inline idBool    getSkipLobLogFlag(smTID aRemoteTID);
    inline void      setSkipLobLogFlag(smTID aRemoteTID, idBool aSet);
    inline smSN      getBeginSN(smTID aRemoteTID);
    //BUG-24398
    inline idBool    getSendLobLogFlag(smTID aRemoteTID);
    inline void      setSendLobLogFlag(smTID aRemoteTID, idBool aSet);

    IDE_RC           insertLocator(smTID aTID, smLobLocator aRemoteLL, smLobLocator aLocalLL);
    void             removeLocator(smTID aTID, smLobLocator aRemoteLL);
    IDE_RC           searchLocator(smTID aTID, smLobLocator aRemoteLL, smLobLocator *aLocalLL, idBool *aIsFound);
    IDE_RC           getFirstLocator(smTID aTID, smLobLocator *aRemoteLL, smLobLocator *aLocalLL, idBool *aIsExist);
    void             setSNMapEntry(smTID aRemoteTID, rprSNMapEntry  *aSNMapEntry);
    rprSNMapEntry*   getSNMapEntry(smTID aRemoteTID);

    // PROJ-1442 Replication Online 중 DDL 허용
    inline void      setDDLTrans(smTID aTID) { mTransTbl[getTransSlotID(aTID)].mIsDDLTrans = ID_TRUE; }
    inline idBool    isDDLTrans(smTID aTID) { return mTransTbl[getTransSlotID(aTID)].mIsDDLTrans; }
    IDE_RC           addItemMetaEntry(smTID          aTID,
                                      smiTableMeta * aItemMeta,
                                      const void   * aItemMetaLogBody,
                                      UInt           aItemMetaLogBodySize,
                                      smSN           aLogSN);
    void             getFirstItemMetaEntry(smTID               aTID,
                                           rpdItemMetaEntry ** aItemMetaEntry);
    void             removeFirstItemMetaEntry(smTID aTID);
    iduList        * getItemMetaList( smTID aTID );
    idBool           existItemMeta(smTID aTID);

    idBool           existDDLStmtMetaLog( smTID aTID );
    IDE_RC           setDDLStmtMetaLog( smTID            aTID,
                                        smiDDLStmtMeta * aDDLStmtMeta,
                                        const void     * aDDLStmtMetaLogBody,
                                        UInt             aDDLStmtMetaLogBodySize );
    void             getDDLStmtMetaLog( smTID aTID, rpdDDLStmtMetaLog ** aDDLStmtMetaLog );
    void             removeDDLStmtMetaLog( smTID aTID );

    // BUG-28206 불필요한 Transaction Begin을 방지
    IDE_RC           addLastSvpEntry(smTID            aTID,
                                     smSN             aSN,
                                     rpSavepointType  aType,
                                     SChar           *aSvpName,
                                     UInt             aImplicitSvpDepth);
    void             applySvpAbort(smTID  aTID,
                                   SChar *aSvpName,
                                   smSN  *aSN);
    void             getFirstSvpEntry(smTID               aTID,
                                      rpdSavepointEntry **aSavepointEntry);
    void             removeSvpEntry(rpdSavepointEntry *aSavepointEntry);
    inline void      setSvpListSent(smTID aRemoteTID);
    inline idBool    isSvpListSent(smTID aRemoteTID);
    inline void      setIsConflictFlag( smTID aTID, idBool aFlag );
    inline idBool    getIsConflictFlag( smTID aTID );

    inline SInt      getATransCnt()
    {
        return idCore::acpAtomicGet32( &mATransCnt );
    }

    IDE_RC buildRecordForReplReceiverTransTbl( void                    * aHeader,
                                               void                    * /* aDumpObj */,
                                               iduFixedTableMemory     * aMemory,
                                               SChar                   * aRepName,
                                               UInt                      aParallelID,
                                               SInt                      aParallelApplyIndex );

    inline smiTransNode * getTransForConflictResolution( smTID aTID );
    inline smiTrans * getSmiTransForConflictResolution( smTID aTID );
    inline idBool isNullTransForConflictResolution( smTID aTID );
    IDE_RC allocConflictResolutionTransNode( smTID aTID );
    void removeTransNode( smiTransNode * aRpdTrans );
    idBool isSetPSMSavepoint( smTID    aTID );

    void setMyTID(smTID aRemoteTID);

    inline void setSendBeginFlag( smTID aTID, idBool aFlag );
    inline idBool getSendBeginFlag( smTID aTID );

    inline smTID getRemoteTID( smTID aTID );

    inline void   setGlobalTxAckRecvSN( smTID aTID, smSN aSN );
    inline smSN   getGlobalTxAckRecvSN( smTID aTID );

/* Member Function */
private:
    inline void   initTransNode(rpdTransTblNode *aHashNode);
    inline UInt   getTransSlotID(smTID aRemoteTID) { return aRemoteTID % mTblSize; }

    // BUG-28206 불필요한 Transaction Begin을 방지
    void          removeLastImplicitSvpEntries(iduList *aSvpList);

    /* BUG-35153 replication receiver performance enhancement 
     * by sm transaction pre-initialize.
     */
    IDE_RC        allocTransNode();
    void          destroyTransPool();
    IDE_RC        initTransPool();
    void          initFreeTransList();
    void          removeFreeTransList();
    void          removeFreeTransNode( smiTransNode * aTrans );
    IDE_RC        getTransNode(smiTransNode** aRpdTrans);

/* Member Variable */
private:
    /* Replication Transaction정보를 가진
       Transaction Table */
    rpdTransTblNode     * mTransTbl;
    rpdTransPoolType      mTransPoolType;
    /* Active Transaction갯수 */
    volatile SInt         mATransCnt;
    /* Transactin Table Size */
    UInt                  mTblSize;
    /* LFG의 갯수 */
    UInt                  mLFGCount;
    /* Last Commit SN */
    smSN                  mEndSN;

    // BUG-28206 불필요한 Transaction Begin을 방지
    iduMemPool            mSvpPool;
    /* BUG-35153 replication receiver performance enhancement 
     * by sm transaction pre-initialize.
     */
    iduMemPool            mTransPool;
    iduList               mFreeTransList;
    UInt                  mOverAllocTransCount;

public:
    iduMutex              mAbortTxMutex;

};

/***********************************************************************
 * Description : aHashNode가 Active Transaction의 노드이면 ID_TRUE, 아니면
 *               ID_FALSE.
 *               BeginSN은 sender만 set하고
 *               이 함수는 sender thread만 호출하므로 mutex를 잡지 않음
 *               receiver는 다른 thread와 동시에 BeginSN을 접근하지 않음
 * aHashNode  - [IN] 테스트를 수행할 rpdTransTblNode
 *
 **********************************************************************/
idBool rpdTransTbl::isATransNode(rpdTransTblNode *aHashNode)
{
    return (aHashNode->mBeginSN == SM_SN_NULL) ? ID_FALSE : ID_TRUE;
}

/***********************************************************************
 * Description : aRemoteTID가 가리키는 Transaction Slot이 Active Transaction의
 *               노드이면 ID_TRUE, 아니면 ID_FALSE.
 *               BeginSN은 sender만 set하고
 *               이 함수는 sender thread만 호출하므로 mutex를 잡지 않음
 *               receiver는 다른 thread와 동시에 BeginSN을 접근하지 않음
 * aRemoteTID  - [IN] 테스트를 수행할 Transaction ID
 *
 **********************************************************************/
inline idBool rpdTransTbl::isATrans(smTID aRemoteTID)
{
    SInt sIndex = getTransSlotID(aRemoteTID);
    return isATransNode( &(mTransTbl[sIndex]) );
}

inline void rpdTransTbl::setGTrans( smTID aRemoteTID )
{
    SInt sIndex = getTransSlotID(aRemoteTID);
    mTransTbl[sIndex].mGlobalTxID = aRemoteTID;
}

inline idBool rpdTransTbl::isGTrans( smTID aRemoteTID )
{
    idBool sIsGTrans = ID_FALSE;
    SInt   sIndex    = getTransSlotID(aRemoteTID);

    if ( mTransTbl[sIndex].mGlobalTxID == aRemoteTID )
    {
        sIsGTrans = ID_TRUE;
    }
    else
    {
        sIsGTrans = ID_FALSE;
    }

    return sIsGTrans;
}

inline smiTransNode* rpdTransTbl::getRpdTrans(smTID aRemoteTID)
{
    SInt sIndex = getTransSlotID(aRemoteTID);
    return mTransTbl[sIndex].mTrans.mRpdTrans;
}

/***********************************************************************
 * Description : aRemoteTID에 해당하는 Transaction Slot을 찾아서 smiTrans를
 *               return한다.
 *
 * aRemoteTID  - [IN] 찾아야 할 Transaction ID
 *
 **********************************************************************/
inline smiTrans* rpdTransTbl::getSMITrans(smTID aRemoteTID)
{
    SInt sIndex = getTransSlotID(aRemoteTID);
    return &(mTransTbl[sIndex].mTrans.mRpdTrans->mSmiTrans);
}
/***********************************************************************
 * Description : aRemoteTID에 해당하는 Transaction Slot을 찾아서
 *               mBeginFlag을 aIsBegin으로 한다.
 * aRemoteTID  - [IN] 찾아야 할 Transaction ID
 * aIsBegin    - [IN] Begin Flag Value.
 *
 **********************************************************************/
inline void rpdTransTbl::setBeginFlag(smTID aRemoteTID, idBool aIsBegin)
{
    SInt sIndex = getTransSlotID(aRemoteTID);
    mTransTbl[sIndex].mBeginFlag = aIsBegin;
}
/***********************************************************************
 * Description : aRemoteTID에 해당하는 Transaction Slot을 찾아서
 *               Abort Transaction 정보를 설정한다.
 *               Sender Apply와 Service thread가 Sender Info를 통해 공유한다.
 * aRemoteTID  - [IN] 찾아야 할 Transaction ID
 * aIsAbort    - [IN] Abort Flag Value
 * aAbortSN    - [IN] Abort SN Value
 **********************************************************************/
inline void rpdTransTbl::setAbortInfo(smTID  aRemoteTID,
                                      idBool aIsAbort,
                                      smSN   aAbortSN)
{
    SInt sIndex = getTransSlotID(aRemoteTID);
    mTransTbl[sIndex].mAbortFlag = aIsAbort;
    mTransTbl[sIndex].mAbortSN   = aAbortSN;
}
/***********************************************************************
 * Description : aRemoteTID에 해당하는 Transaction Slot을 찾아서
 *               mAbortFlag가 Set되어 있는지 반환한다.
 * aRemoteTID  - [IN] 찾아야 할 Transaction ID
 **********************************************************************/
inline idBool rpdTransTbl::getAbortFlag(smTID aRemoteTID)
{
    SInt sIndex = getTransSlotID(aRemoteTID);
    return mTransTbl[sIndex].mAbortFlag;
}
/***********************************************************************
 * Description : aRemoteTID에 해당하는 Transaction Slot을 찾아서
 *               mAbortSN을 반환한다.
 * aRemoteTID  - [IN] 찾아야 할 Transaction ID
 *
 **********************************************************************/
inline smSN rpdTransTbl::getAbortSN(smTID aRemoteTID)
{
    SInt sIndex = getTransSlotID(aRemoteTID);
    return mTransTbl[sIndex].mAbortSN;
}
/***********************************************************************
 * Description : aRemoteTID에 해당하는 Transaction Slot을 찾아서
 *               Abort/Clear Tx List 내의 위치를 설정한다.
 * aRemoteTID  - [IN] 찾아야 할 Transaction ID
 * aTxListIdx  - [IN] Abort/Clear Tx List 내의 위치
 **********************************************************************/
inline void rpdTransTbl::setTxListIdx(smTID aRemoteTID, UInt aTxListIdx)
{
    SInt sIndex = getTransSlotID(aRemoteTID);
    mTransTbl[sIndex].mTxListIdx = aTxListIdx;
}
/***********************************************************************
 * Description : aRemoteTID에 해당하는 Transaction Slot을 찾아서
 *               Abort/Clear Tx List 내의 위치를 반환한다.
 * aRemoteTID  - [IN] 찾아야 할 Transaction ID
 **********************************************************************/
inline UInt rpdTransTbl::getTxListIdx(smTID aRemoteTID)
{
    SInt sIndex = getTransSlotID(aRemoteTID);
    return mTransTbl[sIndex].mTxListIdx;
}

/* BUG-21858 DML + LOB 처리 */
inline void rpdTransTbl::setSkipLobLogFlag(smTID aRemoteTID, idBool aSet)
{
    SInt sIndex = getTransSlotID(aRemoteTID);
    mTransTbl[sIndex].mSkipLobLogFlag = aSet;
}
inline idBool rpdTransTbl::getSkipLobLogFlag(smTID aRemoteTID)
{
    SInt sIndex = getTransSlotID(aRemoteTID);
    return mTransTbl[sIndex].mSkipLobLogFlag;
}

/***********************************************************************
 * Description : aRemoteTID에 해당하는 Transaction Slot을 찾아서
 *               mBeginFlag을 return 한다.
 * aRemoteTID  - [IN] 찾아야 할 Transaction ID
 *
 **********************************************************************/
inline idBool rpdTransTbl::getBeginFlag(smTID aRemoteTID)
{
    SInt sIndex = getTransSlotID(aRemoteTID);
    return mTransTbl[sIndex].mBeginFlag;
}

/***********************************************************************
 * Description : aRemoteTID에 해당하는 Transaction Slot을 찾아서
 *               mLogAnlz을 return 한다.
 * aRemoteTID  - [IN] 찾아야 할 Transaction ID
 *
 **********************************************************************/
inline rpdLogAnalyzer* rpdTransTbl::getLogAnalyzer(smTID aRemoteTID)
{
    SInt sIndex = getTransSlotID(aRemoteTID);
    return mTransTbl[sIndex].mLogAnlz;
}

/***********************************************************************
 * Description : aRemoteTID에 해당하는 Transaction Slot을 찾아서
 *               mBeginSN을 return 한다.
 * aRemoteTID  - [IN] 찾아야 할 Transaction ID
 *
 **********************************************************************/
inline smSN rpdTransTbl::getBeginSN(smTID aRemoteTID)
{
    SInt sIndex = getTransSlotID(aRemoteTID);
    return mTransTbl[sIndex].mBeginSN;
}
/***********************************************************************
 * Description : aRemoteTID에 해당하는 Transaction Slot을 찾아서
 *               mSNMapEntry를 Set 한다.
 * aRemoteTID  - [IN] 찾아야 할 Transaction ID
 * aIsAbort    - [IN] aSNMapEntry.
 **********************************************************************/
inline void rpdTransTbl::setSNMapEntry(smTID aRemoteTID, rprSNMapEntry  *aSNMapEntry)
{
    SInt sIndex = getTransSlotID(aRemoteTID);
    mTransTbl[sIndex].mSNMapEntry = aSNMapEntry;
}
inline rprSNMapEntry* rpdTransTbl::getSNMapEntry(smTID aRemoteTID)
{
    SInt sIndex = getTransSlotID(aRemoteTID);
    return mTransTbl[sIndex].mSNMapEntry;
}

//BUG-24398
inline void rpdTransTbl::setSendLobLogFlag(smTID aRemoteTID, idBool aSet)
{
    SInt sIndex = getTransSlotID(aRemoteTID);
    mTransTbl[sIndex].mSendLobLogFlag = aSet;
}
inline idBool rpdTransTbl::getSendLobLogFlag(smTID aRemoteTID)
{
    SInt sIndex = getTransSlotID(aRemoteTID);
    return mTransTbl[sIndex].mSendLobLogFlag;
}

/***********************************************************************
 * Description : aRemoteTID에 해당하는 Transaction Slot을 찾아서
 *               mIsSvpListSent를 ID_TRUE로 설정한다.
 * aRemoteTID  - [IN] 찾아야 할 Transaction ID
 *
 **********************************************************************/
inline void rpdTransTbl::setSvpListSent(smTID aRemoteTID)
{
    SInt sIndex = getTransSlotID(aRemoteTID);
    mTransTbl[sIndex].mIsSvpListSent = ID_TRUE;
}

/***********************************************************************
 * Description : aRemoteTID에 해당하는 Transaction Slot을 찾아서
 *               mIsSvpListSent를 반환한다.
 * aRemoteTID  - [IN] 찾아야 할 Transaction ID
 *
 **********************************************************************/
inline idBool rpdTransTbl::isSvpListSent(smTID aRemoteTID)
{
    SInt sIndex = getTransSlotID(aRemoteTID);
    return mTransTbl[sIndex].mIsSvpListSent;
}
inline void rpdTransTbl::setIsConflictFlag( smTID aTID, idBool aFlag )
{
    SInt sIndex = getTransSlotID( aTID );
    mTransTbl[sIndex].mIsConflict = aFlag;
}
inline idBool rpdTransTbl::getIsConflictFlag( smTID aTID )
{
    SInt sIndex = getTransSlotID( aTID );
    return mTransTbl[sIndex].mIsConflict;
}

inline smiTransNode * rpdTransTbl::getTransForConflictResolution( smTID aTID )
{
    SInt sIndex = getTransSlotID( aTID );
    return mTransTbl[sIndex].mTrans.mTransForConflictResolution;
}

inline smiTrans * rpdTransTbl::getSmiTransForConflictResolution( smTID aTID )
{
    smiTransNode * sTrans = NULL;

    sTrans =  getTransForConflictResolution( aTID );

    return ( sTrans == NULL ) ? NULL : &(sTrans->mSmiTrans);
}

inline idBool rpdTransTbl::isNullTransForConflictResolution( smTID aTID )
{
    smiTransNode * sTrans = getTransForConflictResolution( aTID );

    return ( sTrans == NULL ) ? ID_TRUE : ID_FALSE;
}

inline idBool rpdTransTbl::isSetPSMSavepoint( smTID    aTID )
{
    SInt    sIndex = getTransSlotID( aTID );

    return mTransTbl[sIndex].mTrans.mSetPSMSavepoint;
}

inline void rpdTransTbl::setSendBeginFlag( smTID aTID, idBool aFlag )
{
    SInt sIndex = getTransSlotID( aTID );
    mTransTbl[sIndex].mSendBeginFlag = aFlag;
}

inline idBool rpdTransTbl::getSendBeginFlag( smTID aTID )
{
    SInt sIndex = getTransSlotID( aTID );
    return mTransTbl[sIndex].mSendBeginFlag;
}

inline smTID rpdTransTbl::getRemoteTID( smTID aTID )
{
    SInt sIndex = getTransSlotID( aTID );
    return mTransTbl[sIndex].mRemoteTID;
}

inline void rpdTransTbl::setGlobalTxAckRecvSN( smTID aTID, smSN aSN )
{
    SInt sIndex = getTransSlotID( aTID );
 
    if ( mTransTbl[sIndex].mGlobalTxID == aTID )
    {
        mTransTbl[sIndex].mGlobalTxAckSN = aSN;
    }
    else
    {
        /* nothing to do */
        /* 평소에는 서비스가 commit 을 기다리기 때문에 여기 올일이 없지만
         * Failback 시에는 여기에 올 수 있다. */
    }
}

inline smSN rpdTransTbl::getGlobalTxAckRecvSN( smTID aTID )
{
    SInt sIndex = getTransSlotID( aTID );
    smSN sSN    = SM_SN_MAX;

    if ( mTransTbl[sIndex].mGlobalTxID == aTID )
    {
        sSN =  mTransTbl[sIndex].mGlobalTxAckSN;
    }

    return sSN;
}

#endif /* _O_RPD_TRANSTBL_H_ */

