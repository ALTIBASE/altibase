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
 *
 {
 }

 * Copyright 1999-2001, ALTIBase Corporation or its subsidiaries.
 * All rights reserved.
 **********************************************************************/

/***********************************************************************
 * $Id: smlLockMgr.h 89355 2020-11-25 01:48:09Z justin.kwon $
 **********************************************************************/

#ifndef _O_SML_LOCK_MGR_H_
#define _O_SML_LOCK_MGR_H_ 1

#include <idu.h>
#include <smErrorCode.h>
#include <smiDef.h>
#include <smlDef.h>
#include <smu.h>
#include <smxTrans.h>

#define SML_END_ITEM          (ID_USHORT_MAX - 1)
#define SML_EMPTY             ((SInt)-1)
#define SML_FLAG_LIGHT_MODE   (0x00000020)
#define SML_DECISION_TBL_SIZE (32)


/* PROJ-2734 
   분산데드락이라 탐지되었을때, 실제 분산데드락일 가능성 */
typedef enum
{
    SML_DIST_DEADLOCK_RISK_NONE = 0,
    SML_DIST_DEADLOCK_RISK_LOW,
    SML_DIST_DEADLOCK_RISK_MID,
    SML_DIST_DEADLOCK_RISK_HIGH,
} smlDistDeadlockRiskType;

typedef IDE_RC (*smlAllocLockNodeFunc)(SInt, smlLockNode**);
typedef IDE_RC (*smlFreeLockNodeFunc)(smlLockNode*);

class smlLockMgr
{
//For Operation
public:

    /* PROJ-2734 */
    static ULong getDistDeadlockWaitTime( smlDistDeadlockRiskType aRisk );
    static smxDistDeadlockDetection detectDistDeadlock( SInt aWaitSlot, ULong * aWaitTime );
    static smxDistDeadlockDetection compareTx4DistDeadlock( smxTrans * aWaitTx, smxTrans * aHoldTx );
    static void clearTxList4DistDeadlock( SInt aWaitSlot );
    static void dumpTxList4DistDeadlock( SInt aWaitSlot );

    static IDE_RC initialize(UInt               aTransCnt,
                             smiLockWaitFunc    aLockWaitFunc,
                             smiLockWakeupFunc  aLockWakeupFunc );
    static IDE_RC destroy();

    static void   initTransLockList(SInt aSlot);

    /* BUG-28752 lock table ... in row share mode 구문이 먹히지 않습니다.
     * implicit/explicit lock을 구분하여 겁니다. */
    static IDE_RC lockTable(SInt          aSlot,
                            smlLockItem  *aLockItem,
                            smlLockMode   aLockMode,
                            ULong         aLockWaitMicroSec = ID_ULONG_MAX,
                            smlLockMode  *aCurLockMode = NULL,
                            idBool       *aLocked = NULL,
                            smlLockNode **aLockNode = NULL,
                            smlLockSlot **aLockSlot = NULL,
                            idBool        aIsExplicitLock = ID_FALSE );

    static IDE_RC unlockTable( SInt         aSlot ,
                               smlLockNode *aTableLockNode,
                               smlLockSlot *aTableLockSlot,
                               idBool       aDoMutexLock = ID_TRUE);


private:
    static IDE_RC lockTableInternal( idvSQL       * aStatistics,
                                     SInt           aSlot,
                                     smlLockItem  * aLockItem,
                                     smlLockMode    aLockMode,
                                     ULong          aLockWaitMicroSec,
                                     smlLockMode  * aCurLockMode,
                                     idBool       * aLocked,
                                     smlLockNode  * aCurTransLockNode,
                                     smlLockSlot ** aLockSlot,
                                     idBool         aIsExplicit );
    
    static IDE_RC unlockTableInternal( idvSQL      * aStatistics,
                                       SInt          aSlot,
                                       smlLockItem * aLockItem,
                                       smlLockNode  *aLockNode,
                                       smlLockSlot  *aLockSlot,
                                       idBool        aDoMutexLock );

   static IDE_RC toLightMode( idvSQL      * aStatistics,
                              smlLockItem * aLockItem );

   static  IDE_RC wakeupRequestLockNodeInLockItem( idvSQL      * aStatistics,
                                                   smlLockItem * aLockItem );

    static void addLockNodeToHead(smlLockNode *&aFstLockNode,
                                  smlLockNode *&aLstLockNode,
                                  smlLockNode *&aNewLockNode);

    static void addLockNodeToTail(smlLockNode *&aFstLockNode,
                                  smlLockNode *&aLstLockNode,
                                  smlLockNode *&aNewLockNode);

    static void  removeLockNode(smlLockNode *&aFstLockNode,
                                smlLockNode *&aLstLockNode,
                                smlLockNode *&aLockNode);

    static   void decTblLockModeAndTryUpdate(smlLockItem *, smlLockMode);
    static   void incTblLockModeAndUpdate(smlLockItem *, smlLockMode);

    /* BUG-28752 lock table ... in row share mode 구문이 먹히지 않습니다.
     * implicit/explicit lock을 구분하여 겁니다. */
    static IDE_RC allocLockNodeAndInit(SInt           aSlot,
                                       smlLockMode    aLockMode,
                                       smlLockItem  * aLockItem,
                                       smlLockNode ** aNewLockNode,
                                       idBool         aIsExplicitLock);
    static inline IDE_RC freeLockNode(smlLockNode*  aLockNode);

    static   void   clearWaitTableRows(smlLockNode* aLockNode,
                                       SInt aSlot);


    static  void registTblLockWaitListByReq(SInt aSlot,
                                            smlLockNode* aLockNode);
    static  void registTblLockWaitListByGrant(SInt aSlot,
                                              smlLockNode* aLockNode);

    static  void setLockModeAndAddLockSlot( SInt         aSlot,
                                            smlLockNode* aTxLockNode,
                                            smlLockMode* aCurLockMode,
                                            smlLockMode  aLockMode,
                                            idBool       aIsLocked,
                                            smlLockSlot** aLockSlot );

    static void updateStatistics( idvSQL     * aStatistics,
                                  idvStatIndex aStatIdx );

    //move from smxTrans to smlLockMgr
    static  smlLockNode   * findLockNode( smlLockItem * aLockItem, SInt aSlot );
    static  void            addLockNode(smlLockNode* aLockNode, SInt aSlot);
    static  void            addLockSlot( smlLockSlot *, SInt aSlot );
    static  smlLockMode     getDecision( SInt aFlag )
    {
        IDE_ASSERT( aFlag < SML_DECISION_TBL_SIZE );
        return  mDecisionTBL[ aFlag ];
    };

    static  idBool          isLockNodeExist( smlLockNode *aLockNode );
    static  idBool          isLockNodeExistInGrant( smlLockItem * aLockItem, smlLockNode *aLockNode );
    static  void            validateNodeListInLockItem( smlLockItem * aLockItem );

public:
    static  void registRecordLockWait(SInt aSlot, SInt aWaitSlot);
    // PRJ-1548
    // 테이블스페이스에 대한 잠금요청시 DML,DDL에 따라서
    // Lock Wait 시간을 인자로 내려줄 수 있어야 하므로
    // 인자를 추가하였다.
    inline static IDE_RC lockItem( void       * aTrans,
                                   void       * aLockItem,
                                   idBool       aIsIntent,
                                   idBool       aIsExclusive,
                                   ULong        aLockWaitMicroSec,
                                   idBool     * aLocked,
                                   void      ** aLockSlot );

    /* Don't worry. Finally,it's ok to call unlockItem() directly.
     * If you doubt it, lock at the code of unlockItem().
     */
    inline static IDE_RC unlockItem( void    *aTrans,
                                     void    *aLockSlot );

    static idBool isCycle( SInt aSlot, idBool aIsReadyDistDeadlock );

    static IDE_RC initLockItem( scSpaceID         aSpaceID,
                                ULong             aItemID,
                                smiLockItemType   aLockItemType,
                                void          *   aLockItem );
    static UInt getSlotCount() { return mTransCnt;};
    static smlTransLockList* getTransLockList( UInt aSlot )
    { return mArrOfLockList + aSlot; };

private:
    static void   clearLockItem( smlLockItem  *   aLockItem );
    static void   initLockNode(smlLockNode *aLockNode);

public:
    inline static IDE_RC destroyLockItem(void *aLockItem);

    static void   getTxLockInfo(SInt  aSlot, smTID *aOwnerList,
                                UInt *aOwnerCount);

    static IDE_RC allocLockItem(void **aLockItem);
    static IDE_RC freeLockItem(void *aLockItem)
    {  return iduMemMgr::free(aLockItem); };

    static  void            clearWaitItemColsOfTrans(idBool, SInt aSlot);
    static  void            revertWaitItemColsOfTrans( SInt aSlot );
    static  void            removeLockNode(smlLockNode* aLockNode);
    static  void            removeLockSlot( smlLockSlot * );
    static  IDE_RC          freeAllItemLock(SInt aSlot);
    /* PROJ-1381 Fetch Across Commits */
    static  IDE_RC          freeAllItemLockExceptIS(SInt aSlot);

    static  void*           getLastLockSlotPtr(SInt aSlot);
    static  ULong           getLastLockSequence(SInt aSlot);
    static  IDE_RC          partialItemUnlock(SInt   aSlot,
                                              ULong  aLockSequence,
                                              idBool aIsSeveralLock);

    /*
     * PROJ-2620
     * Legacy lock mgr - Mutex
     */
    static   inline idBool  didLockReleased(SInt aSlot, SInt aWaitSlot);
    static          IDE_RC  freeAllRecordLock(SInt aSlot);

    /* BUG-18981 */
    static    IDE_RC        logLocks( smxTrans * aTrans, ID_XID * aXID );

    static         IDE_RC   lockWaitFunc(ULong aWaitDuration, idBool* aFlag );
    static         IDE_RC   lockWakeupFunc();
    static inline  IDE_RC   lockTableModeX(void* aTrans, void* aLockItem);
    static inline  IDE_RC   lockTableModeIX(void *aTrans, void    *aLockItem);
    static inline  IDE_RC   lockTableModeIS(void *aTrans, void    *aLockItem);
    static inline  IDE_RC   lockTableModeIS4FixedTable(void *aTrans, void    *aLockItem);
    static inline  IDE_RC   lockTableModeXAndCheckLocked(void   *aTrans,
                                                         void   *aLockItem,
                                                         idBool *aIsLock );
    /* BUG-33048 [sm_transaction] The Mutex of LockItem can not be the Native
     * mutex.
     * LockItem으로 NativeMutex을 사용할 수 있도록 수정함 */
    static     iduMutex   * getMutexOfLockItem(void *aLockItem)
    {    return &( ((smlLockItem *)aLockItem)->mMutex );};

    static     void         lockTableByPreparedLog(void* aTrans,
                                                   SChar* aLog,
                                                   UInt aLockCnt,
                                                   UInt* aOffset);

    // PRJ-1548 User Memory Tablespace
    // smlLockMode가 S, IS가 아닌 경우 TRUE를 반환한다.
    static idBool isNotISorS( smlLockMode    aLockMode );
    static smlLockMode getConversionLockMode( smlLockMode aBaseLockMode , smlLockMode aNewLockNode )
    { return mConversionTBL[ aBaseLockMode ][ aNewLockNode ]; };
    static idBool isExistLockModeMask( SInt aFlag , smlLockMode aFindLockNode )
    {
        if( aFindLockNode < SML_NUMLOCKTYPES )
        {
            if (( aFlag & smlLockMgr::mLockModeToMask[ aFindLockNode ] ) !=
                smlLockMgr::mLockModeToMask[ aFindLockNode ] )
            {
                return ID_TRUE;
            }
        }
        return ID_FALSE;
    }

    static IDE_RC dumpLockTBL();
    static void getLockItemNodes( smlLockItem * aLockItem );
    static void dumpLockWait();

    static void lockTransNodeList( idvSQL * aStatistics, SInt aSlot )
    {  (void)mArrOfLockListMutex[aSlot].lock( aStatistics ); };
    static void unlockTransNodeList( SInt aSlot )
    {  (void)mArrOfLockListMutex[aSlot].unlock(); };
private:
//For Member

    /* PROJ-2734 */
    static smlDistDeadlockRiskType mDistDeadlockRisk[SMI_DIST_LEVEL_MAX][SMI_DIST_LEVEL_MAX];

    static idBool              mCompatibleTBL[SML_NUMLOCKTYPES][SML_NUMLOCKTYPES];
    // 새로 grant된 lock과 table의 대표락을 가지고,
    // table의 새로운 대표락을 결정할때 사용한다.
    static smlLockMode         mConversionTBL[SML_NUMLOCKTYPES][SML_NUMLOCKTYPES];
    // table lock을 unlock할때 ,table의 대표락을 결정하기
    // 위하여 미리 계산해 놓은 lock mode 결정 배열임.
    static smlLockMode         mDecisionTBL[SML_DECISION_TBL_SIZE];
    static SInt                mLockModeToMask[SML_NUMLOCKTYPES];

public:
    static smlLockMatrixItem** mWaitForTable;
    //add for performance view.
    static smlLockMode2StrTBL  mLockMode2StrTBL[SML_NUMLOCKTYPES];
    static smlTransLockList  * mArrOfLockList;

    /* PROJ-2734 */
    static smlDistDeadlockNode ** mTxList4DistDeadlock;

private:
    static SInt                mTransCnt;
    static iduMemPool          mLockPool;
    static smiLockWaitFunc     mLockWaitFunc;
    static smiLockWakeupFunc   mLockWakeupFunc;
    static iduMutex          * mArrOfLockListMutex;

    /*
     * PROJ-2620
     * Cache lock node
     */
    static smlLockNode**    mNodeCache;
    static smlLockNode*     mNodeCacheArray;
    static ULong*           mNodeAllocMap;

    static IDE_RC initTransLockNodeCache(SInt, iduList*);

    static IDE_RC allocLockNodeNormal(SInt, smlLockNode**);
    static IDE_RC allocLockNodeList  (SInt, smlLockNode**);
    static IDE_RC allocLockNodeBitmap(SInt, smlLockNode**);
    static IDE_RC freeLockNodeNormal(smlLockNode*);
    static IDE_RC freeLockNodeList  (smlLockNode*);
    static IDE_RC freeLockNodeBitmap(smlLockNode*);

    static smlAllocLockNodeFunc             mAllocLockNodeFunc;
    static smlFreeLockNodeFunc              mFreeLockNodeFunc;
};

/*********************************************************
  function description: didLockReleased
  트랜잭션에 대응하는  aSlot이  aWaitSlot 트랜잭션에게
  record lock wait또는 table lock wait이 의미 clear되어
  있는지 확인한다.
  *********************************************************/
idBool  smlLockMgr::didLockReleased(SInt aSlot, SInt aWaitSlot)
{
    return (mWaitForTable[aSlot][aWaitSlot].mIndex == 0) ?  ID_TRUE: ID_FALSE;
}

inline void*  smlLockMgr::getLastLockSlotPtr(SInt aSlot)
{
    return mArrOfLockList[aSlot].mLockSlotHeader.mPrvLockSlot;
}

inline ULong smlLockMgr::getLastLockSequence(SInt aSlot)
{
    return mArrOfLockList[aSlot].mLockSlotHeader.mPrvLockSlot->mLockSequence;
}

inline  IDE_RC smlLockMgr::lockWaitFunc(ULong aWaitDuration, idBool* aFlag)
{
    return mLockWaitFunc(aWaitDuration, aFlag);
}

inline  IDE_RC smlLockMgr::lockWakeupFunc()
{
    return mLockWakeupFunc();
}

// PRJ-1548 User Memory Tablespace
// smlLockMode가 S, IS가 아닌 경우 TRUE를 반환한다.
inline idBool smlLockMgr::isNotISorS(smlLockMode aLockMode)
{
    if ( (aLockMode == SML_ISLOCK) || (aLockMode == SML_SLOCK) )
    {
        return ID_FALSE;
    }

    return ID_TRUE;
}

inline IDE_RC smlLockMgr::freeLockNode(smlLockNode*  aLockNode)
{
    IDE_ASSERT( NULL == aLockNode->mPrvLockNode );
    IDE_ASSERT( NULL == aLockNode->mNxtLockNode );
    IDE_ASSERT( NULL == aLockNode->mPrvTransLockNode );
    IDE_ASSERT( NULL == aLockNode->mNxtTransLockNode );
    IDE_DASSERT( isLockNodeExist( aLockNode ) == ID_FALSE );

    return mFreeLockNodeFunc( aLockNode );
}

inline IDE_RC smlLockMgr::destroyLockItem( void *aLockItem )
{
    IDE_TEST( ((smlLockItem*)(aLockItem))->mMutex.destroy() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_SET(ideSetErrorCode(smERR_FATAL_ThrMutexDestroy));

    return IDE_FAILURE;
}

inline IDE_RC smlLockMgr::lockItem( void        *aTrans,
                                    void        *aLockItem,
                                    idBool       aIsIntent,
                                    idBool       aIsExclusive,
                                    ULong        aLockWaitMicroSec,
                                    idBool     * aLocked,
                                    void      ** aLockSlot )
{
    smlLockMode sLockMode;

    if ( aIsIntent == ID_TRUE )
    {
        sLockMode = ( aIsExclusive == ID_TRUE ) ? SML_IXLOCK : SML_ISLOCK;
    }
    else
    {
        sLockMode = ( aIsExclusive == ID_TRUE ) ? SML_XLOCK : SML_SLOCK;
    }

    return lockTable( smxTrans::getTransSlot( aTrans ) ,
                      (smlLockItem *)aLockItem,
                      sLockMode,
                      aLockWaitMicroSec, /* wait micro second */
                      NULL,      /* current lock mode */
                      aLocked,   /* is locked */
                      NULL,      /* get locked node */
                      (smlLockSlot**)aLockSlot ) ;
}
inline IDE_RC smlLockMgr::unlockItem( void *aTrans,
                                      void *aLockSlot )
{
    IDE_DASSERT( aLockSlot != NULL );

    return unlockTable( smxTrans::getTransSlot( aTrans ) ,
                        NULL,
                        (smlLockSlot*)aLockSlot );
}

inline IDE_RC smlLockMgr::lockTableModeX( void     *aTrans,
                                          void     *aLockItem )
{
    return lockTable( smxTrans::getTransSlot( aTrans ),
                      (smlLockItem *)aLockItem,
                      SML_XLOCK );
}
/*********************************************************
  function description: lockTableModeIX
  IX lock mode으로 table lock을 건다.
 ***********************************************************/
inline IDE_RC smlLockMgr::lockTableModeIX( void    *aTrans,
                                           void    *aLockItem )
{
    return lockTable( smxTrans::getTransSlot( aTrans ),
                      (smlLockItem *)aLockItem,
                      SML_IXLOCK );
}
/*********************************************************
  function description: lockTableModeIS
  IS lock mode으로 table lock을 건다.
 ***********************************************************/
inline IDE_RC smlLockMgr::lockTableModeIS(void    *aTrans,
                                          void    *aLockItem )
{
    return lockTable( smxTrans::getTransSlot( aTrans ),
                      (smlLockItem *)aLockItem,
                      SML_ISLOCK );
}

/***********************************************************
  function description: lockTableModeIS4FixedTable
  IS lock mode으로 table lock을 건다.
  smuProperty::getSkipLockedTableAtFixedTable() == 1 에서
  lock을 바로 걸 수 없을 때 실패를 반환한다.
 ***********************************************************/
IDE_RC smlLockMgr::lockTableModeIS4FixedTable( void    *aTrans,
                                               void    *aLockItem )
{
    if ( smuProperty::getSkipLockedTableAtFixedTable() == 1 )
    {
        /* try lock */
        return lockTable( smxTrans::getTransSlot( aTrans ),
                          (smlLockItem *)aLockItem,
                          SML_ISLOCK,
                          0 );
    }
    else
    {
        /* lock wait */
        return lockTable( smxTrans::getTransSlot( aTrans ),
                          (smlLockItem *)aLockItem,
                          SML_ISLOCK );
    }
}

/*********************************************************
  function description: lockTableModeXAndCheckLocked
  X lock mode으로 table lock을 건다.
 ***********************************************************/
inline IDE_RC smlLockMgr::lockTableModeXAndCheckLocked( void   *aTrans,
                                                        void   *aLockItem,
                                                        idBool *aIsLock )
{
    return lockTable( smxTrans::getTransSlot( aTrans ),
                      (smlLockItem *)aLockItem,
                      SML_XLOCK,
                      sctTableSpaceMgr::getDDLLockTimeOut((smxTrans*)aTrans),
                      NULL,
                      aIsLock );
}

inline void smlLockMgr::clearTxList4DistDeadlock( SInt aWaitSlot )
{
    SInt i;

    for ( i = 0;
          i < mTransCnt;
          i++ )
    {
        mTxList4DistDeadlock[aWaitSlot][i].mTransID    = SM_NULL_TID;
        mTxList4DistDeadlock[aWaitSlot][i].mDistTxType = SML_DIST_DEADLOCK_TX_NON_DIST_INFO;
    }
}

inline void smlLockMgr::dumpTxList4DistDeadlock( SInt aWaitSlot )
{
    SInt i;
    SChar sBuffer[2048] = {0, };
    SInt  sLen = 0;

    sLen = idlOS::snprintf( sBuffer,
                            ID_SIZEOF(sBuffer),
                            "<DUMP mTxList4DistDeadlock[%"ID_INT32_FMT"]>\n", aWaitSlot );

    for ( i = 0;
          i < mTransCnt;
          i++ )
    {
        if (  mTxList4DistDeadlock[aWaitSlot][i].mTransID == SM_NULL_TID )
        {
            continue;
        }

        sLen += idlOS::snprintf( sBuffer + sLen,
                                 ID_SIZEOF(sBuffer) - sLen,
                                 "[HolderSlot %"ID_INT32_FMT"] HolderTxID : %"ID_UINT32_FMT", DistTxType : %"ID_INT32_FMT"\n", 
                                 i,
                                 mTxList4DistDeadlock[aWaitSlot][i].mTransID,
                                 mTxList4DistDeadlock[aWaitSlot][i].mDistTxType );
    }

    ideLog::log( IDE_SD_19, "%s", sBuffer );
}

#endif
