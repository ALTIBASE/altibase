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
 * $Id:$
 **********************************************************************/
/******************************************************************************
 * PROJ-1568 BUFFER MANAGER RENEWAL
 ******************************************************************************/
#ifndef _O_SDB_BUFFER_POOL_H_
#define _O_SDB_BUFFER_POOL_H_ 1

#include <idl.h>
#include <idu.h>
#include <smDef.h>
#include <sdbDef.h>
#include <iduLatch.h>

#include <sdbFlushList.h>
#include <sdbLRUList.h>
#include <sdbPrepareList.h>
#include <sdbCPListSet.h>
#include <sdbBCBHash.h>
#include <sdbBCB.h>
#include <sdbBufferPoolStat.h>
#include <sdbMPRKey.h>

#include <sdsBCB.h>

#define SDB_BUFFER_POOL_INVALIDATE_WAIT_UTIME   (100000)

class sdbBufferPool
{
public:
    IDE_RC initialize(UInt aBCBCnt,
                      UInt aBucketCnt,
                      UInt aBucketCntPerLatch,
                      UInt aLRUListNum,
                      UInt aPrepareListNum,
                      UInt aFlushListNum,
                      UInt aChkptListNum);

    IDE_RC destroy();

    IDE_RC initPrepareList();
    IDE_RC initLRUList();
    IDE_RC initFlushList();

    void destroyPrepareList();
    void destroyLRUList();
    void destroyFlushList();


    inline IDE_RC resizeHashTable( UInt       aNewHashBucketCnt,
                                   UInt       aBucketCntPerLatch);

    inline void expandBufferPool( idvSQL    *aStatistics,
                                  sdbBCB    *aFirstBCB,
                                  sdbBCB    *aLastBCB,
                                  UInt       aLength );


    void distributeBCB2PrepareLsts( idvSQL    *aStatistics,
                                 sdbBCB    *aFirstBCB,
                                 sdbBCB    *aLastBCB,
                                 UInt       aLength );

    IDE_RC addBCB2PrepareLst(idvSQL *aStatistics,
                             sdbBCB *aBCB);

    void addBCBList2PrepareLst(idvSQL    *aStatistics,
                               sdbBCB    *aFirstBCB,
                               sdbBCB    *aLastBCB,
                               UInt       aLength);

    IDE_RC getVictim( idvSQL  *aStatistics,
                      UInt     aListKey,
                      sdbBCB **aReturnedBCB);

    IDE_RC createPage( idvSQL          *aStatistics,
                       scSpaceID        aSpaceID,
                       scPageID         aPageID,
                       UInt             aPageType,
                       void*            aMtx,
                       UChar**          aPagePtr);

    IDE_RC getPage( idvSQL               * aStatistics,
                    scSpaceID              aSpaceID,
                    scPageID               aPageID,
                    sdbLatchMode           aLatchMode,
                    sdbWaitMode            aWaitMode,
                    sdbPageReadMode        aReadMode,
                    void                 * aMtx,
                    UChar               ** aRetPage,
                    idBool               * aTrySuccess,
                    idBool               * aIsCorruptPage );

    IDE_RC fixPage( idvSQL              *aStatistics,
                    scSpaceID            aSpaceID,
                    scPageID             aPageID,
                    UChar              **aRetPage);

    IDE_RC fixPageWithoutIO( idvSQL              *aStatistics,
                             scSpaceID            aSpaceID,
                             scPageID             aPageID,
                             UChar              **aRetPage);

#if 0 //not used
    IDE_RC getPagePtrFromGRID( idvSQL     * aStatistics,
                               scGRID       aGRID,
                               UChar      **aPagePtr );
#endif
    void setDirtyPage( idvSQL *aStatistics,
                       void   *aBCB,
                       void   *aMtx );

    void releasePage( idvSQL * aStatistics,
                      void   * aBCB,
                      UInt     aLatchMode );

    void releasePage( idvSQL          *aStatistics,
                      void            *aBCB );

    void releasePageForRollback( idvSQL       *aStatistics,
                                 void         *aBCB,
                                 UInt          aLatchMode );

    void unfixPage( idvSQL     * aStatistics,
                    UChar      * aPagePtr);

    inline void unfixPage( idvSQL     * aStatistics,
                           sdbBCB     * aBCB);
#ifdef DEBUG
    inline idBool isPageExist( idvSQL       *aStatistics,
                               scSpaceID     aSpaceID,
                               scPageID      aPageID);
#endif
    void discardBCB( idvSQL                 *aStatistics,
                     sdbBCB                 *aBCB,
                     sdbFiltFunc             aFilter,
                     void                   *aObj,
                     idBool                  aIsWait,
                     idBool                 *aIsSuccess,
                     idBool                 *aMakeFree);


    void invalidateDirtyOfBCB( idvSQL                 *aStatistics,
                               sdbBCB                 *aBCB,
                               sdbFiltFunc             aFilter,
                               void                   *aObj,
                               idBool                  aIsWait,
                               idBool                 *aIsSuccess,
                               idBool                 *aFiltOK);

    idBool makeFreeBCB(idvSQL     *aStatistics,
                       sdbBCB     *aBCB,
                       sdbFiltFunc aFilter,
                       void       *aObj);

    idBool removeBCBFromList(idvSQL *aStatistics, sdbBCB *aBCB);

    inline void setInvalidateWaitTime(UInt  aMicroSec)
    {   mInvalidateWaitTime.set(0, aMicroSec ); };

    // sdbBCBListStat을 위해 리스트를 직접 리턴하는 함수들
    inline sdbLRUList  *   getLRUList(UInt aID);
    inline sdbFlushList*   getFlushList(UInt aID);
    inline sdbPrepareList* getPrepareList(UInt aID);

    sdbLRUList     *getMinLengthLRUList(void);
    sdbPrepareList *getMinLengthPrepareList(void);
    sdbFlushList   *getMaxLengthFlushList(void);
    UInt            getFlushListTotalLength(void);
    UInt            getFlushListNormalLength(void);
    UInt            getDelayedFlushListLength(void);
    UInt            getFlushListLength( UInt aID );
    void            setMaxDelayedFlushListPct( UInt aMaxListPct );
    inline UInt     getDelayedFlushCnt4LstFromPCT( UInt aMaxListPct );

    sdbFlushList   *getDelayedFlushList( UInt aIndex );


    // multi page read관련 함수들
    IDE_RC fetchPagesByMPR( idvSQL               * aStatistics,
                            scSpaceID              aSpaceID,
                            scPageID               aStartPID,
                            UInt                   aPageCount,
                            sdbMPRKey            * aKey );

    void releasePage( sdbMPRKey *aKey,
                      UInt       aLatchMode );

    void cleanUpKey(idvSQL    *aStatistics,
                    sdbMPRKey *aKey,
                    idBool     aCachePage);

    ////////////////////////////////////////////////
    inline IDE_RC findBCB( idvSQL       * aStatistics,
                           scSpaceID      aSpaceID,
                           scPageID       aPageID,
                           sdbBCB      ** aBCB );

    inline void applyStatisticsForSystem();

    inline sdbBufferPoolStat *getBufferPoolStat();

    inline void lockPoolXLatch(idvSQL *aStatistics);
    inline void lockPoolSLatch(idvSQL *aStatistics);
    inline void unlockPoolLatch(void);

    // BufferPool 맴버들의 get 함수들
    inline UInt getID();
    inline UInt getPoolSize();
    inline UInt getPageSize();
    inline sdbCPListSet *getCPListSet();
    inline UInt getHashBucketCount();
    inline UInt getHashChainLatchCount();
    inline UInt getLRUListCount();
    inline UInt getPrepareListCount();
    inline UInt getFlushListCount();
    inline UInt getCheckpointListCount();

    // 통계정보를 얻기위한 함수들
    inline UInt getHashPageCount();
    inline UInt getHotListPageCount();
    inline UInt getColdListPageCount();
    inline UInt getPrepareListPageCount();
    inline UInt getFlushListPageCount();
    inline UInt getCheckpointListPageCount();

    inline void latchPage( idvSQL        * aStatistics,
                           sdbBCB        * aBCB,
                           sdbLatchMode    aLatchMode,
                           sdbWaitMode     aWaitMode,
                           idBool        * aTrySuccess );

    inline void unlatchPage( sdbBCB *aBCB );
    inline UInt getLRUSearchCount();

    inline idBool isHotBCB(sdbBCB *aBCB);
    inline void   setLRUSearchCnt(UInt aLRUSearchPCT);

    static inline idBool isFixedBCB(sdbBCB *aBCB);

    void setHotMax(idvSQL *aStatistics, UInt aHotMaxPCT);
    void setPageLSN( sdbBCB *aBCB, void *aMtx );
    void tryEscalateLatchPage( idvSQL            *aStatistics,
                               sdbBCB            *aBCB,
                               sdbPageReadMode    aReadMode,
                               idBool            *aTrySuccess );

    void setFrameInfoAfterRecoverPage( sdbBCB * aBCB,
                                       idBool   aChkOnlineTBS );
    /* PROJ-2102 Secondary Buffer */
    inline idBool isSBufferServiceable( void );
    inline void setSBufferServiceState( idBool aState );

private:
    inline void setDirtyBCB( idvSQL * aStatistics,
                             sdbBCB * aBCB );

    IDE_RC readPage( idvSQL               *aStatistics,
                     scSpaceID             aSpaceID,
                     scPageID              aPageID,
                     sdbPageReadMode       aReadMode,
                     idBool               *aHit,
                     sdbBCB              **aReplacedBCB,
                     idBool               *aIsCorruptPage );

    void getVictimFromPrepareList(idvSQL  *aStatistics,
                                  UInt     aListKey,
                                  sdbBCB **aResidentBCB);

    void getVictimFromLRUList( idvSQL  *aStatistics,
                               UInt     aListKey,
                               sdbBCB **aResidentBCB);

    IDE_RC readPageFromDisk( idvSQL               *aStatistics,
                             sdbBCB               *aBCB,
                             idBool               *aIsCorruptPage );

    void setFrameInfoAfterReadPage( sdbBCB *aBCB,
                                    idBool  aChkOnlineTBS );

    IDE_RC fixPageInBuffer( idvSQL             *aStatistics,
                            scSpaceID           aSpaceID,
                            scPageID            aPageID,
                            sdbPageReadMode     aReadMode,
                            UChar             **aRetPage,
                            idBool             *aHit,
                            sdbBCB            **aFixedBCB,
                            idBool             *aIsCorruptPage);


    // multi page read 관련 함수들
    IDE_RC copyToFrame( idvSQL                 *aStatistics,
                        scSpaceID               aSpaceID,
                        sdbReadUnit            *aReadUnit,
                        UChar                  *aPagePtr );

    IDE_RC fetchMultiPagesNormal( idvSQL                * aStatistics,
                                  sdbMPRKey             * aKey );

    IDE_RC fetchMultiPagesAtOnce( idvSQL              * aStatistics,
                                  scPageID              aStartPID,
                                  UInt                  aPageCount,
                                  sdbMPRKey           * aKey );

    IDE_RC fetchSinglePage( idvSQL                 * aStatistics,
                            scPageID                 aPageID,
                            sdbMPRKey              * aKey );

    idBool analyzeCostToReadAtOnce(sdbMPRKey *aKey);

    inline UInt getHotCnt4LstFromPCT( UInt aHotMaxPCT );
    inline UInt getPrepareListIdx( UInt aListKey );
    inline UInt getLRUListIdx( UInt aListKey );
    inline UInt getFlushListIdx( UInt aListKey );
    inline UInt genListKey( UInt aPageID, idvSQL *aStatistics );

    inline void fixAndUnlockHashAndTouch( idvSQL                   *aStatistics,
                                          sdbBCB                   *aBCB,
                                          sdbHashChainsLatchHandle *aHandle );
#ifdef DEBUG    
    IDE_RC validate( sdbBCB *aBCB );
#endif
public:
    /* 어떤  BCB의 touch count가 mHotTouchCnt와 같거나 클 때,
     * 그 BCB를 hot으로 판단한다.  */
    UInt              mHotTouchCnt;

private:
    /* bufferPool이 다루는 페이지 크기.. 현재는 8K */
    UInt              mPageSize;

    /* 여러 sdbLRUList를 배열 형태로 가지고 있다.*/
    sdbLRUList       *mLRUList;
    /* LRU list의 갯수 */
    UInt              mLRUListCnt;

    /* 여러 sdbPrepareList를 배열 형태로 가지고 있다.  */
    sdbPrepareList   *mPrepareList;
    /* prepare list 갯수 */
    UInt              mPrepareListCnt;

    /* 여러 sdbFlushList를 배열 행태로 가지고 있다. */
    sdbFlushList     *mFlushList;

    /* PROJ-2669
     * 최근에 touch 된 Dirty BCB 를 보관하는 Delayed Flush List */
    sdbFlushList     *mDelayedFlushList;

    /* flush list의 갯수 */
    UInt              mFlushListCnt;

    /* 자체적으로 병렬화 되어있는 sdbCPListSet하나를 가짐 */
    sdbCPListSet      mCPListSet;

    /* 외부에서 트랜잭션이 한번에 BCB를 찾아 갈수 있게 BCB들의 hash table 이
     * 있다. */
    sdbBCBHash        mHashTable;

    /* Dirty BCB를 clean으로 변경하는 연산이 있다.
     * 이때, InIOB나 redirty상태일 경우에 BCB의 상태가 변하길 기다려야 하는데,
     * 얼마만큼 기다릴지 설정 */
    PDL_Time_Value    mInvalidateWaitTime;

    // buffer pool에 존재하는 전체 BCB갯수
    UInt              mBCBCnt;

    /* victim을 LRUList에서 선정시, 탐색할 BCB갯수 */
    UInt              mLRUSearchCnt;

    /* Secondary Buffer 가 사용 가능한지 여부 */
    idBool            mIsSBufferServiceable;

    /* Buffer Pool 관련 통계정보 */
    sdbBufferPoolStat mStatistics;
};

/****************************************************************
 * Description:
 *  리스트가 여러개이므로, 리스트 중 하나를 선택해야 한다. 이때,
 *  aListKey를 이용해 하나를 선택해 준다.
 *  aListKey에 따라 공평하게 리스트를 분배하기 위해 현재는 단순히 mod 연산을
 *  이용한다.
 *
 *  aListKey    - [IN]  여러 리스트중 하나를 선택하기 위한 변수
 ****************************************************************/
inline UInt sdbBufferPool:: getPrepareListIdx( UInt aListKey )
{
    return aListKey % mPrepareListCnt;
}

inline UInt sdbBufferPool:: getLRUListIdx( UInt aListKey )
{
    return aListKey % mLRUListCnt;
}

inline UInt sdbBufferPool::getFlushListIdx( UInt aListKey )
{
    return  aListKey % mFlushListCnt;
}


/****************************************************************
 * Description:
 *  누군가 BCB를 접근하고 있다면, ID_TRUE를 리턴한다.
 * Implemenation:
 *  BCB의 mFixCnt가 1이상이면 true를 리턴.
 *  fixCount값은 특히 hashTable동시성과 깊은 연관이 있다.
 *  fixCount가 1이상인 경우 hashTable에서 절대로 제거해서는 안된다.
 *  같은 이유로, 내가 fixCount를 증가 시켜 놓으면 절대로 hashTable에서 제거되지
 *  않음을 보장할 수 있다.
 *
 *  aBCB        - [IN]  BCB
 ****************************************************************/
inline idBool  sdbBufferPool::isFixedBCB( sdbBCB *aBCB )
{
    if ( aBCB->mFixCnt > 0 )
    {
        return ID_TRUE;
    }
    else
    {
        return ID_FALSE;
    }
}


/****************************************************************
 * Description :
 *  fixPage - unfixPage는 쌍으로 호출된다.
 *
 *  aStatistics - [IN]  통계정보
 *  aBCB        - [IN]  fixPage BCB
 ****************************************************************/
inline void sdbBufferPool::unfixPage( idvSQL     * aStatistics,
                                      sdbBCB     * aBCB )
{
    aBCB->lockBCBMutex( aStatistics);
    aBCB->decFixCnt();
    aBCB->unlockBCBMutex();
    return;
}

/****************************************************************
 * Description :
 *  aBCB가 hash에 존재할때, 이 BCB를 fix시키고, touch count를
 *  변경하고, hashLatch를 푼다.
 *  aBCB를 hit하였을때 호출되는 공통의 연산을 인라인 으로 묶어서
 *  따로 뺀 함수
 *
 *  주의! 반드시 해당 hash 래치를 잡고 들어오며, aBCB는 반드시
 *  해시에 존재할때, 이함수를 호출한다. 이 함수내에서 hash 래치를 해제한다.
 *
 *  aStatistics - [IN]  통계정보
 *  aBCB        - [IN]  BCB
 *  aHandle     - [IN]  unlock을 위해 필요한 hash handle
 ****************************************************************/

inline void sdbBufferPool::fixAndUnlockHashAndTouch(
                                    idvSQL                    *aStatistics,
                                    sdbBCB                    *aBCB,
                                    sdbHashChainsLatchHandle  *aHandle )
{
    aBCB->lockBCBMutex( aStatistics );
    /* fix count 증가는 반드시 hash latch를 잡고 있는 상태에서 한다.
     * 그렇지 않으면 fix count증가 도중 hash에서 제거될 수 있다.
     * 또한 BCBMutex를 잡고 한다. hash S latch를 잡은 두개 이상의
     * 트랜잭션이 동시에 fix를 수행할 수 있기 때문이다. */
    aBCB->incFixCnt();

    /*일단 fix를 해놓으면 절대 hash에서 제거 되지 않기 때문에
     *hash 래치를 푼다.*/
    mHashTable.unlockHashChainsLatch( aHandle );
    aBCB->updateTouchCnt();
    aBCB->unlockBCBMutex();
}

/****************************************************************
 * Description :
 *  aBCB의 mState를 변경하고, 즉, dirty시키고, 체크포인트 리스트에
 *  삽입합니다.
 *
 *  주의! 이함수는 반드시 페이지 x래치를 잡고 호출됩니다.
 *
 *  aStatistics - [IN]  통계정보
 *  aBCB        - [IN]  BCB
 ****************************************************************/
inline void sdbBufferPool::setDirtyBCB( idvSQL *aStatistics,
                                        sdbBCB *aBCB )
{
    /*
     * aBCB->mState를 확인하고 변경하는데 동시성을 보장해 주기 위해서
     * BCBMutex를 잡습니다.
     * 이 함수는 setDirtyPage를 할때 호출됩니다. 그러므로, fix되어 있으면
     * victim으로 선정되지 않기 때문에,
     * getVictim을 하는 트랜잭션과 동시성 문제가 발생하지 않습니다.
     *
     * flusher에서 실제로 디스크 I/O를 한 이후에, BCB들의 상태를 INIOB에서
     * clean으로 변경하는데, 이때와 동시성 문제를 해결하기 위해서 BCBMutex를
     * 잡습니다.
     *
     * flusher또는 다른 setDirty연산과의 동시성은 페이지 x 래치로 합니다.
     **/
    aBCB->lockBCBMutex( aStatistics );
    if ( aBCB->mState == SDB_BCB_CLEAN )
    {
        aBCB->mState = SDB_BCB_DIRTY;
    }
    else
    {
        /* 이 함수는 x래치를 잡고 나서 상태가 CLEAN 또는 INIOB 상태 일때
         * 들어 옵니다. 이 둘을 상태를 변경시킬 여지가 있는 flusher와
         * 다른 setDirty연산과는 x래치로 제어가 되기 때문에,
         * CLEAN 상태가 아니라면, INIOB상태임을 보장할 수 있습니다. */
        // 플러셔에 의해 INIOB상태로 변경된다.
        IDE_ASSERT( aBCB->mState == SDB_BCB_INIOB);
        aBCB->mState = SDB_BCB_REDIRTY;
    }
    aBCB->unlockBCBMutex();

    // recoverLSN이 init이란 말은 no logging으로 갱신된 temp page이다.
    // 이 경우엔 checkpoint list에 달지 않는다.
    if ( !SM_IS_LSN_INIT(aBCB->mRecoveryLSN) )
    {
        mCPListSet.add( aStatistics, aBCB );
    }
    return ;
}

/****************************************************************
 * Description :
 *  hash table크기 변경.. 주의 해야 할건, resizeHashTable중간에
 *  절대로 hashTable에 접근하면 안된다.
 *
 * aNewHashBucketCnt    - [IN] 새로 만들 hashBucketCnt
 * aBucketCntPerLatch   - [IN] 새로 만들 aBucketCntPerLatch
 ****************************************************************/
inline IDE_RC sdbBufferPool::resizeHashTable( UInt aNewHashBucketCnt,
                                              UInt aBucketCntPerLatch )
{
    return mHashTable.resize( aNewHashBucketCnt,
                              aBucketCntPerLatch );
}

inline void sdbBufferPool::applyStatisticsForSystem()
{
    mStatistics.applyStatisticsForSystem();
}

inline sdbBufferPoolStat *sdbBufferPool::getBufferPoolStat()
{
    return &mStatistics;
}

inline UInt sdbBufferPool::getPoolSize()
{
    return mBCBCnt;
}

inline UInt sdbBufferPool::getPageSize()
{
    return mPageSize;
}

inline sdbCPListSet *sdbBufferPool::getCPListSet()
{
    return &mCPListSet;
}

inline UInt sdbBufferPool::getHashBucketCount()
{
    return mHashTable.getBucketCount();
}

inline UInt sdbBufferPool::getHashChainLatchCount()
{
    return mHashTable.getLatchCount();
}

inline UInt sdbBufferPool::getLRUListCount()
{
    return mLRUListCnt;
}

inline UInt sdbBufferPool::getPrepareListCount()
{
    return mPrepareListCnt;
}

inline UInt sdbBufferPool::getFlushListCount()
{
    return mFlushListCnt;
}

inline UInt sdbBufferPool::getCheckpointListCount()
{
    return mCPListSet.getListCount();
}

inline UInt sdbBufferPool::getHashPageCount()
{
    return mHashTable.getBCBCount();
}

inline UInt sdbBufferPool::getHotListPageCount()
{
    UInt sRet = 0;
    UInt i;

    for (i = 0; i < mLRUListCnt; i++)
    {
        sRet += mLRUList[i].getHotLength();
    }
    return sRet;
}

inline UInt sdbBufferPool::getColdListPageCount()
{
    UInt sRet = 0;
    UInt i;

    for (i = 0; i < mLRUListCnt; i++)
    {
        sRet += mLRUList[i].getColdLength();
    }
    return sRet;
}

inline UInt sdbBufferPool::getPrepareListPageCount()
{
    UInt sRet = 0;
    UInt i;

    for (i = 0; i < mPrepareListCnt; i++)
    {
        sRet += mPrepareList[i].getLength();
    }
    return sRet;
}

inline UInt sdbBufferPool::getFlushListPageCount()
{
    UInt sRet = 0;
    UInt i;

    for (i = 0; i < mFlushListCnt; i++)
    {
        /* PROJ-2669 Normal + Delayed Flush List */
        sRet += getFlushListLength( i );
    }
    return sRet;
}

inline UInt sdbBufferPool::getCheckpointListPageCount()
{
    return mCPListSet.getTotalBCBCnt();
}

inline sdbLRUList* sdbBufferPool::getLRUList(UInt aID)
{
    IDE_DASSERT(aID < mLRUListCnt);
    return &mLRUList[aID];
}

inline sdbFlushList* sdbBufferPool::getFlushList(UInt aID)
{
    IDE_DASSERT(aID < mFlushListCnt);
    return &mFlushList[aID];
}

inline sdbPrepareList* sdbBufferPool::getPrepareList(UInt aID)
{
    IDE_DASSERT(aID < mPrepareListCnt);
    return &mPrepareList[aID];
}

inline UInt sdbBufferPool::getHotCnt4LstFromPCT(UInt aHotMaxPCT)
{
    IDE_DASSERT( mBCBCnt > 0 );
    IDE_DASSERT( mLRUListCnt > 0 );

    if (mBCBCnt <= SDB_SMALL_BUFFER_SIZE)
    {
        return 1;
    }
    else
    {
        return (mBCBCnt * aHotMaxPCT ) / (mLRUListCnt * 100);
    }
}

inline void  sdbBufferPool::setLRUSearchCnt(UInt aLRUSearchPCT)
{
    IDE_DASSERT( mBCBCnt > 0 );
    IDE_DASSERT( mLRUListCnt > 0 );
    mLRUSearchCnt = (mBCBCnt * aLRUSearchPCT ) / mLRUListCnt / 100;

    if (mLRUSearchCnt == 0)
    {
        mLRUSearchCnt = 1;
    }
}

/***********************************************************************
 * Description :
 *  여러 리스트중 하나를 선택할때 쓰이는 list key를 생성한다.  현재는 pageID와
 *  aStatistics를 섞어서 사용한다.
 *
 *  aPageID     - [IN]  page ID
 *  aStatistics - [IN]  통계정보
 ***********************************************************************/
inline UInt sdbBufferPool::genListKey( UInt   aPageID,
                                       idvSQL *aStatistics )
{
    return aPageID + ( (UInt)(vULong)aStatistics >> 4);
}

#ifdef DEBUG
/****************************************************************
 * Description :
 *  버퍼에 해당 BCB가 존재하는지 여부를 리턴해주는 함수
 *
 *  aStatistics - [IN]  통계정보
 *  aSpaceID    - [IN]  table space ID
 *  aPageID     - [IN]  page ID
 ****************************************************************/
inline idBool sdbBufferPool::isPageExist( idvSQL       *aStatistics,
                                          scSpaceID     aSpaceID,
                                          scPageID      aPageID )
{
    sdbBCB *sBCB;

    // 위에서 BCB의존재여부로 DASSERT 하니까 결과만 ID_FALSE로 올림.
    (void)findBCB( aStatistics, aSpaceID, aPageID, &sBCB );
    
    if ( sBCB == NULL )
    {
        return ID_FALSE;
    }
    else
    {
        return ID_TRUE;
    }
}
#endif

/****************************************************************
 * Description :
 *  PROJ-2669
 *  Delayed Flush List 의 최대 길이를 리턴한다.
 *
 *  aMaxListPct - [IN]  Delayed Flush List 의 최대 길이(Percent)
 ****************************************************************/
inline UInt sdbBufferPool::getDelayedFlushCnt4LstFromPCT( UInt aMaxListPct )
{
    IDE_DASSERT( mBCBCnt > 0 );
    IDE_DASSERT( mFlushListCnt > 0 );

    if ( mBCBCnt <= SDB_SMALL_BUFFER_SIZE )
    {
        return 1;
    }
    else
    {
        return (UInt)( ( (ULong)mBCBCnt * aMaxListPct ) / ( mFlushListCnt * 100 ) );
    }
}

/****************************************************************
 * Description :
 *  해당하는 BCB가 버퍼에 존재하는 경우 BCB를 리턴한다.
 *  그렇지 않는 경우엔 NULL을 리턴한다.
 *  주의! 리턴되는 BCB에 대해선 어떤 래치도 유지 하지 않기 때문에,
 *  BCB에 대한 동시성을 외부에서 제어해야 한다.
 *  즉, 리턴 받은 BCB의 내용이 비동기적으로 변할 수 있고, 예측 또한 불가능하다.
 *
 *  aStatistics - [IN]  통계정보
 *  aSpaceID    - [IN]  table space ID
 *  aPageID     - [IN]  page ID
 *  aBCB        - [OUT] find해서 발견한 BCB리턴.
 ****************************************************************/
inline IDE_RC sdbBufferPool::findBCB( idvSQL       * aStatistics,
                                      scSpaceID      aSpaceID,
                                      scPageID       aPageID,
                                      sdbBCB      ** aBCB )
{
    sdbHashChainsLatchHandle    * sHandle = NULL;

    IDE_DASSERT( aBCB != NULL );
    IDE_DASSERT( aSpaceID <= SC_MAX_SPACE_COUNT - 1);

    *aBCB = NULL;

    sHandle = mHashTable.lockHashChainsSLatch( aStatistics,
                                               aSpaceID,
                                               aPageID );

    IDE_TEST( mHashTable.findBCB( aSpaceID,
                                  aPageID,
                                  (void**)aBCB )
              != IDE_SUCCESS );

    mHashTable.unlockHashChainsLatch( sHandle );
    sHandle = NULL;

    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    if ( sHandle != NULL )
    {
        mHashTable.unlockHashChainsLatch( sHandle );
        sHandle = NULL;
    }
    else
    {
        /* nothing to do */
    }
    
    return IDE_FAILURE;
}

inline UInt sdbBufferPool::getLRUSearchCount()
{
    return mLRUSearchCnt;
}

/****************************************************************
 * Description :
 *  BCB의 touch count를 보고 hot인지 여부를 판단한다.
 *  이때, 사용자가 정한 property를 통해서 hot인지 여부를 판단한다.
 *  touch count는 감소하지 않고 항상 증가하는 값이며,
 *  BCB에 접근하는 경우에 증가한다.
 *  연속적으로 접근하는 경우에는 증가시키지 않는다.
 ****************************************************************/
inline idBool sdbBufferPool::isHotBCB(sdbBCB  *aBCB)
{
    if( aBCB->mTouchCnt >= mHotTouchCnt )
    {
        return ID_TRUE;
    }
    else
    {
        return ID_FALSE;
    }
}

/****************************************************************
 * Description :
 *  외부로 부터 BCB리스트를 받아 자신이 보유한 prepareList 에 골고루 달아준다.
 *
 *  aStatistics - [IN]  통계정보
 *  aFirstBCB   - [IN]  BCB리스트의 head
 *  aLastBCB    - [IN]  BCB리스트의 tail
 *  aLength     - [IN]  삽입하는 BCB리스트의 길이
 ****************************************************************/
inline void sdbBufferPool::expandBufferPool( idvSQL    *aStatistics,
                                             sdbBCB    *aFirstBCB,
                                             sdbBCB    *aLastBCB,
                                             UInt       aLength )
{
    IDE_ASSERT( mBCBCnt + aLength > mBCBCnt);

    mBCBCnt += aLength;
    // 버퍼풀의 BCB갯수가 증가 하였으므로, 각 LRUList의 HotMax도 증가 시킨다.
    setHotMax( aStatistics, smuProperty::getHotListPct());

    distributeBCB2PrepareLsts( aStatistics, aFirstBCB, aLastBCB, aLength);

    return;
}

inline void sdbBufferPool::unlatchPage( sdbBCB *aBCB )
{
    aBCB->unlockPageLatch();
}

/***********************************************************************
 * Descrition:
 *  RedoMgr에 의해 Page가 복구되었을때, 해당 Page를 Frame에 설정시
 *  필요한 정보를 설정한다.
 *
 *  aBCB            - [IN]  BCB
 *  aChkOnlineTBS   - [IN] TBS Online 여부를 확인해서 SMO NO 갱신 (Yes/No)
 **********************************************************************/
inline void sdbBufferPool::setFrameInfoAfterRecoverPage(
                                            sdbBCB  * aBCB,
                                            idBool    aChkOnlineTBS )
{
    setFrameInfoAfterReadPage( aBCB, aChkOnlineTBS );
}

/* PROJ-2102 Secondary Buffer */
/****************************************************************
 * Description : Secondary Buffer가 사용 가능한지 확인 
 ****************************************************************/
inline idBool sdbBufferPool::isSBufferServiceable( void )
{
    return mIsSBufferServiceable;
}

/****************************************************************
 * Description : Secondary Buffer를 사용상태 확인  
 ****************************************************************/
inline void sdbBufferPool::setSBufferServiceState( idBool aState )
{
    mIsSBufferServiceable = aState;
}

/****************************************************************
 * Description:
 *  aBCB에 페이지 래치를 건다.
 *
 *  aStatistics - [IN]  통계정보
 *  aBCB        - [IN]  BCB
 *  aLatchMode  - [IN]  래치 모드, shared, exclusive, nolatch가 있다.
 *  aWaitMode   - [IN]  대기할지 말지 여부
 *  aSuccess    - [OUT] lock을 성공적으로 잡았는지 여부를 리턴한다.
 ****************************************************************/
inline void sdbBufferPool::latchPage( idvSQL        * aStatistics,
                                      sdbBCB        * aBCB,
                                      sdbLatchMode    aLatchMode,
                                      sdbWaitMode     aWaitMode,
                                      idBool        * aSuccess )
{
    idBool sSuccess = ID_FALSE;
    IDE_DASSERT( isFixedBCB( aBCB ) == ID_TRUE );

    if ( aLatchMode == SDB_NO_LATCH )
    {
        /* 아무리 no latch로 dirty read를 한다고 해도, 현재 디스크에서
         * read I/O중인 버퍼를 읽으면 않된다. 그렇기 때문에 read I/O가
         * 진행중인 경우 이것이 끝날때 까지 대기한다.
         * read I/O중인 트랜잭션은 페이지 x래치 뿐만 아니라 mReadIOMutex도
         * 잡기때문에 no_latch로 동작하는 트랜잭션의 경우에는 mReadIOMutex
         * 에서 대기하게 된다.*/
        /* 사실상 mReadyToRead 와 mReadIOMutex 둘 중 하나만 있어도 된다.
         * 하지만, mReadyToRead가 없다면 매번 mReadIOMutex를 잡아야 하므로
         * 비용이 들고,
         * mReadIOMutex가 없다면, 언제 자신이 읽어야 하는지 알아보기 위해서
         * loop를 돌아야 하는 수 밖에 없다.
         * 그렇기 때문에 둘다 유지 한다.*/
        if ( aBCB->mReadyToRead == ID_FALSE )
        {
            IDE_ASSERT ( aBCB->mReadIOMutex.lock( aStatistics ) == IDE_SUCCESS );
            IDE_ASSERT ( aBCB->mReadIOMutex.unlock() == IDE_SUCCESS );
        }
        else
        {
            /* Read Page에 대한 mReadyToRead와 Frame에 적재하는 연산의 순서가 뒤바뀔수 있는데
             * read하는 트랜잭션은 ReadIoMutex와 latch로 순서를 보장하지만, No-latch로
             * latchPage하는 트랜잭션은 순서가 뒤바뀔수 있어서 mReadyToRead를 TRUE라고 하더라도
             * mFrame이 제대로 판독할 수 있도록 Barrier를 쳐주어야 한다.
             */
            IDL_MEM_BARRIER;
        }

        /* mReadIOMutex 잡았다 풀었으면, 현재 BCB가 절대 read I/O가 아님을
         * 보장할수 있으므로 마음껏 읽는다. 왜냐면 fix를 했기때문이다. */
        sSuccess = ID_TRUE;
    }
    else
    {
        if ( aWaitMode == SDB_WAIT_NORMAL )
        {
            if ( aLatchMode == SDB_S_LATCH )
            {
                aBCB->lockPageSLatch( aStatistics );
            }
            else
            {
                aBCB->lockPageXLatch( aStatistics );
            }
            sSuccess = ID_TRUE;
        }
        else
        {
            /*no wait.. */
            if ( aLatchMode == SDB_S_LATCH )
            {
                aBCB->tryLockPageSLatch( &sSuccess );
            }
            else
            {
                IDE_DASSERT( aLatchMode == SDB_X_LATCH );
                aBCB->tryLockPageXLatch( &sSuccess );
            }
        }
    }

    /* BUG-28423 - [SM] sdbBufferPool::latchPage에서 ASSERT로 비정상
     * 종료합니다. */
    IDE_ASSERT ( ( sSuccess == ID_FALSE ) || ( aBCB->mReadyToRead == ID_TRUE ) );

    if ( aSuccess != NULL )
    {
        *aSuccess = sSuccess;
    }
}

#endif  //_O_SDB_BUFFER_POOL_H_
