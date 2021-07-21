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
 * $Id: sdbBufferMgr.h 83116 2018-05-29 05:52:23Z jiwon.kim $
 **********************************************************************/

#ifndef _O_SDB_BUFFER_MGR_H_
#define _O_SDB_BUFFER_MGR_H_ 1

#include <idl.h>
#include <smDef.h>
#include <smiDef.h>
#include <sdbDef.h>
#include <sdbBCB.h>
#include <iduLatch.h>

#include <sdbBufferPool.h>
#include <sdbFlushMgr.h>
#include <sdbBufferArea.h>

// pinning을 위해 쓰이는 자료구조
typedef struct sdbPageID
{
    /* space ID */
    scSpaceID mSpaceID;
    /* page ID */
    scPageID  mPageID;
}  sdbPageID;

typedef struct sdbPinningEnv
{
    // 자신이 접근한 pageID에 대한 기록을 가지고 있다.
    sdbPageID  *mAccessHistory;
    // mAccessHisty가 기록할 최대 pageID갯수
    UInt        mAccessHistoryCount;
    // 현재 접근한 pageID를 기록할 위치
    UInt        mAccessInsPos;

    // 자신이 pinning하고 있는 pageID에 대한 정보를 가진다.
    sdbPageID  *mPinningList;
    // 자신이 pinning하고 있는 BCB포인터를 가지고 있다.
    sdbBCB    **mPinningBCBList;
    // 최대한 pinning할 수 있는 BCB갯수
    UInt        mPinningCount;
    // 현재 BCB를 pinning할 위치
    UInt        mPinningInsPos;
} sdbPinningEnv;

// sdbFiltFunc의 aFiltAgr에 쓰이는 자료구조
typedef struct sdbBCBRange
{
    // mSpaceID에 속하면서
    // mStartPID 보다 크거나 같고,
    // mEndPID보다 작거나 같은 모든 pid를 범위에 속하도록한다.
    scSpaceID   mSpaceID;
    scPageID    mStartPID;
    scPageID    mEndPID;
}sdbBCBRange;

typedef struct sdbDiscardPageObj
{
    /* discard 조건 검사 함수 */
    sdbFiltFunc              mFilter;
    /* 통계정보 */
    idvSQL                  *mStatistics;
    /* 적용할 버퍼풀 */
    sdbBufferPool           *mBufferPool;
    /* mFilter수행에 필요한 환경 */
    void                    *mEnv;
    /* BCB를 모아두는 큐 */
    smuQueueMgr              mQueue;
}sdbDiscardPageObj;


typedef struct sdbPageOutObj
{
    /* page out 조건 검사 함수 */
    sdbFiltFunc              mFilter;
    /* mFilter수행에 필요한 환경 */
    void                    *mEnv;
    /* flush를 해야하는 BCB들을 모아둔 큐 */
    smuQueueMgr              mQueueForFlush;
    /* make free를 해야하는 BCB들을 모아둔 큐 */
    smuQueueMgr              mQueueForMakeFree;
}sdbPageOutObj;

typedef struct sdbFlushObj
{
    /* flush 조건 검사 함수 */
    sdbFiltFunc              mFilter;
    /* mFilter수행에 필요한 환경 */
    void                    *mEnv;
    /* BCB를 모아두는 큐 */
    smuQueueMgr              mQueue;
}sdbFlushObj;

/* BUG-21793: [es50-x32] natc에서 clean하고 server start하는 경우 서버 사망합니다.
 *
 * inline 으로 getPage, fixPage함수들을 처리하면 compiler 오류로 서버가 오동작합니다.
 * 하여 이 함수들을 모두 보통 함수로 처리했습니다.
 *
 * 주의: getPage, fixPage함수들을 inline처리하지 마세여. linux 32bit release에서 비정상
 * 종료합니다.
 * */
class sdbBufferMgr
{
public:
    static IDE_RC initialize();

    static IDE_RC destroy();

    static IDE_RC resize( idvSQL *aStatistics,
                          ULong   aAskedSize,
                          void   *aTrans,
                          ULong  *aNewSize);

    static IDE_RC expand( idvSQL *aStatistics,
                          ULong   aAddedSize,
                          void   *aTrans,
                          ULong  *aExpandedSize);

    static void addBCB(idvSQL    *aStatistics,
                       sdbBCB    *aFirstBCB,
                       sdbBCB    *aLastBCB,
                       UInt       aLength);

    static IDE_RC createPage( idvSQL          *aStatistics,
                              scSpaceID        aSpaceID,
                              scPageID         aPageID,
                              UInt             aPageType,
                              void*            aMtx,
                              UChar**          aPagePtr);

#if 0 //not used.
//    static IDE_RC getPageWithMRU( idvSQL             *aStatistics,
//                                  scSpaceID           aSpaceID,
//                                  scPageID            aPageID,
//                                  sdbLatchMode        aLatchMode,
//                                  sdbWaitMode         aWaitMode,
//                                  void               *aMtx,
//                                  UChar             **aRetPage,
//                                  idBool             *aTrySuccess = NULL);
#endif


    static IDE_RC getPageByPID( idvSQL               * aStatistics,
                                scSpaceID              aSpaceID,
                                scPageID               aPageID,
                                sdbLatchMode           aLatchMode,
                                sdbWaitMode            aWaitMode,
                                sdbPageReadMode        aReadMode,
                                void                 * aMtx,
                                UChar               ** aRetPage,
                                idBool               * aTrySuccess,
                                idBool               * aIsCorruptPage );

    static IDE_RC getPage( idvSQL             *aStatistics,
                           scSpaceID           aSpaceID,
                           scPageID            aPageID,
                           sdbLatchMode        aLatchMode,
                           sdbWaitMode         aWaitMode,
                           sdbPageReadMode     aReadMode,
                           UChar             **aRetPage,
                           idBool             *aTrySuccess = NULL);

    static IDE_RC getPageByRID( idvSQL             *aStatistics,
                                scSpaceID           aSpaceID,
                                sdRID               aPageRID,
                                sdbLatchMode        aLatchMode,
                                sdbWaitMode         aWaitMode,
                                void               *aMtx,
                                UChar             **aRetPage,
                                idBool             *aTrySuccess = NULL);

    static IDE_RC getPage( idvSQL               * aStatistics,
                           scSpaceID              aSpaceID,
                           scPageID               aPageID,
                           sdbLatchMode           aLatchMode,
                           sdbWaitMode            aWaitMode,
                           void                 * aMtx,
                           UChar               ** aRetPage,
                           idBool               * aTrySuccess,
                           sdbBufferingPolicy     aBufferingPolicy );

    static IDE_RC getPage(idvSQL               *aStatistics,
                          void                 *aPinningEnv,
                          scSpaceID             aSpaceID,
                          scPageID              aPageID,
                          sdbLatchMode          aLatchMode,
                          sdbWaitMode           aWaitMode,
                          void                 *aMtx,
                          UChar               **aRetPage,
                          idBool               *aTrySuccess );

    static IDE_RC getPageBySID( idvSQL             *aStatistics,
                                scSpaceID           aSpaceID,
                                sdSID               aPageSID,
                                sdbLatchMode        aLatchMode,
                                sdbWaitMode         aWaitMode,
                                void               *aMtx,
                                UChar             **aRetPage,
                                idBool             *aTrySuccess = NULL);

    static IDE_RC getPageByGRID( idvSQL             *aStatistics,
                                 scGRID              aGRID,
                                 sdbLatchMode        aLatchMode,
                                 sdbWaitMode         aWaitMode,
                                 void               *aMtx,
                                 UChar             **aRetPage,
                                 idBool             *aTrySuccess = NULL);

    static IDE_RC getPageBySID( idvSQL             *aStatistics,
                                scSpaceID           aSpaceID,
                                sdSID               aPageSID,
                                sdbLatchMode        aLatchMode,
                                sdbWaitMode         aWaitMode,
                                void               *aMtx,
                                UChar             **aRetPage,
                                idBool             *aTrySuccess,
                                idBool             *aSkipFetch /* BUG-43844 */ );

    static IDE_RC getPageByGRID( idvSQL             *aStatistics,
                                 scGRID              aGRID,
                                 sdbLatchMode        aLatchMode,
                                 sdbWaitMode         aWaitMode,
                                 void               *aMtx,
                                 UChar             **aRetPage,
                                 idBool             *aTrySuccess,
                                 idBool             *aSkipFetch /* BUG-43844 */ );

    static IDE_RC setDirtyPage( void *aBCB,
                                void *aMtx );

    static void setDirtyPageToBCB( idvSQL * aStatistics,
                                   UChar  * aPagePtr );
    
    static IDE_RC releasePage( void            *aBCB,
                               UInt             aLatchMode,
                               void            *aMtx );

    static IDE_RC releasePage( idvSQL * aStatistics,
                               UChar  * aPagePtr );

    static IDE_RC releasePageForRollback( void * aBCB,
                                          UInt   aLatchMode,
                                          void * aMtx );

    static IDE_RC fixPageByPID( idvSQL            * aStatistics,
                                scSpaceID           aSpaceID,
                                scPageID            aPageID,
                                UChar            ** aRetPage,
                                idBool            * aTrySuccess );
#if 0
    static IDE_RC fixPageByGRID( idvSQL            * aStatistics,
                                 scGRID              aGRID,
                                 UChar            ** aRetPage,
                                 idBool            * aTrySuccess = NULL );
#endif
    static IDE_RC fixPageByRID( idvSQL            * aStatistics,
                                scSpaceID           aSpaceID,
                                sdRID               aPageRID,
                                UChar            ** aRetPage,
                                idBool            * aTrySuccess = NULL );

    static IDE_RC fixPageByPID( idvSQL              *aStatistics,
                                scSpaceID            aSpaceID,
                                scPageID             aPageID,
                                UChar              **aRetPage );

    static inline IDE_RC fixPageWithoutIO( idvSQL             *aStatistics,
                                           scSpaceID           aSpaceID,
                                           scPageID            aPageID,
                                           UChar             **aRetPage );

    static inline IDE_RC unfixPage( idvSQL     * aStatistics,
                                    UChar      * aPagePtr);

    static inline void unfixPage( idvSQL     * aStatistics,
                                  sdbBCB     * aBCB);

    static inline sdbBufferPool *getPool() { return &mBufferPool; };
    static inline sdbBufferArea *getArea() { return &mBufferArea; };

    static IDE_RC removePageInBuffer( idvSQL     *aStatistics,
                                      scSpaceID   aSpaceID,
                                      scPageID    aPageID );

    static inline void latchPage( idvSQL        *aStatistics,
                                  UChar         *aPagePtr,
                                  sdbLatchMode   aLatchMode,
                                  sdbWaitMode    aWaitMode,
                                  idBool        *aTrySuccess );

    static inline void unlatchPage ( UChar *aPage );
#ifdef DEBUG
    static inline idBool isPageExist( idvSQL       *aStatistics,
                                      scSpaceID     aSpaceID,
                                      scPageID      aPageID );
#endif
    static void setDiskAgerStatResetFlag(idBool  aValue);

    static inline UInt   getPageCount();

    static void getMinRecoveryLSN( idvSQL   *aStatistics,
                                   smLSN    *aLSN );

    static sdbBCB* getBCBFromPagePtr( UChar * aPage);

#if 0 // not used
    static IDE_RC getPagePtrFromGRID( idvSQL     *aStatistics,
                                      scGRID      aGRID,
                                      UChar     **aPagePtr);
#endif
    static void applyStatisticsForSystem();

    static inline sdbBufferPoolStat *getBufferPoolStat();

    /////////////////////////////////////////////////////////////////////////////

#if 0 //not used
//    static IDE_RC createPinningEnv(void **aEnv);

//    static IDE_RC destroyPinningEnv(idvSQL *aStatistics, void *aEnv);
#endif

    static void discardWaitModeFromQueue( idvSQL      *aStatistics,
                                          sdbFiltFunc  aFilter,
                                          void        *aFiltAgr,
                                          smuQueueMgr *aQueue );

    static IDE_RC discardPages( idvSQL        *aStatistics,
                                sdbFiltFunc    aFilter,
                                void          *aFiltAgr);

    static IDE_RC discardPagesInRange( idvSQL    * aStatistics,
                                       scSpaceID   aSpaceID,
                                       scPageID    aStartPID,
                                       scPageID    aEndPID );

    static IDE_RC discardAllPages( idvSQL *aStatistics);

    static IDE_RC flushPages( idvSQL      *aStatistics,
                              sdbFiltFunc  aFilter,
                              void        *aFiltAgr );

    static IDE_RC flushPagesInRange( idvSQL   *aStatistics,
                                     scSpaceID aSpaceID,
                                     scPageID  aStartPID,
                                     scPageID  aEndPID );


    static IDE_RC pageOutAll( idvSQL *aStatistics );

    static IDE_RC pageOut(  idvSQL            *aStatistics,
                            sdbFiltFunc        aFilter,
                            void              *aFiltAgr);

    static IDE_RC pageOutTBS( idvSQL          *aStatkstics,
                              scSpaceID        aSpaceID);

    static IDE_RC pageOutInRange( idvSQL    *aStatistics,
                                  scSpaceID  aSpaceID,
                                  scPageID   aStartPID,
                                  scPageID   aEndPID);

    static IDE_RC discardNoWaitModeFunc( sdbBCB    *aBCB,
                                         void      *aObj);

    static IDE_RC makePageOutTargetQueueFunc( sdbBCB *aBCB,
                                              void   *aObj);

    static IDE_RC makeFlushTargetQueueFunc( sdbBCB *aBCB,
                                            void   *aObj);

    static IDE_RC flushDirtyPagesInCPList(idvSQL *aStatistics, idBool aFlushAll);

    static IDE_RC flushDirtyPagesInCPListByCheckpoint(idvSQL *aStatistics, idBool aFlushAll);

    static inline void setPageLSN( void     *aBCB,
                                   void     *aMtx );

    static inline void lockBufferMgrMutex(idvSQL  *aStatistics);
    static inline void unlockBufferMgrMutex();

    static inline UInt getPageFixCnt( UChar * aPage);

    static void   setPageTypeToBCB( UChar*  aPagePtr, UInt aPageType );

    static void   tryEscalateLatchPage( idvSQL            *aStatistics,
                                        UChar             *aPagePtr,
                                        sdbPageReadMode    aReadMode,
                                        idBool            *aTrySuccess );

    static inline sdbBufferPool* getBufferPool();
    
    static inline void setSBufferUnserviceable( void );

    static inline void setSBufferServiceable( void );

    static IDE_RC gatherStatFromEachBCB();

    // BUG-45598
    static inline idBool isPageReadError( UChar *aPagePtr );

private:
    // buffer pinning을 위한 함수들 /////////////////////////////////////////////
    static sdbBCB* findPinnedPage(sdbPinningEnv *aEnv,
                                  scSpaceID      aSpaceID,
                                  scPageID       aPageID);

    static void addToHistory(sdbPinningEnv *aEnv,
                             sdbBCB        *aBCB,
                             idBool        *aBCBPinned,
                             sdbBCB       **aBCBToUnpin);
    /////////////////////////////////////////////////////////////////////////////

    /* [BUG-20861] 버퍼 hash resize를 하기 위해서 다른 트랜잭션들ㅇ르 모두 접근
       하지 못하게 해야 합니다. */
    static void blockAllApprochBufHash( void     *aTrans,
                                        idBool   *aSuccess );

    static void unblockAllApprochBufHash();


    static idBool filterAllBCBs( void   *aBCB,
                                 void   */*aObj*/);

    static idBool filterTBSBCBs( void   *aBCB,
                                 void   *aObj);

    static idBool filterBCBRange( void   *aBCB,
                                  void   *aObj);

    static idBool filterTheBCB( void   *aBCB,
                                void   *aObj);
    

private:
    /* 버퍼 메모리를 관리하는 모듈 */
    static sdbBufferArea mBufferArea;
    /* BCB 관리 및 버퍼 교체 정책 모듈  */
    static sdbBufferPool mBufferPool;
    /* 사용자가 alter system 구문 간의 동시성 제어 */
    static iduMutex      mBufferMgrMutex;
    /* 버퍼 매니저가 관리하는  PAGE개수 (BCB개수) */
    static UInt          mPageCount;
};


inline IDE_RC sdbBufferMgr::fixPageWithoutIO( idvSQL             * aStatistics,
                                              scSpaceID            aSpaceID,
                                              scPageID             aPageID,
                                              UChar             ** aRetPage )
{
    IDE_RC rc;

    IDV_SQL_OPTIME_BEGIN( aStatistics, IDV_OPTM_INDEX_DRDB_FIX_PAGE );

    rc = mBufferPool.fixPageWithoutIO( aStatistics,
                                       aSpaceID,
                                       aPageID,
                                       aRetPage );

    IDV_SQL_OPTIME_END( aStatistics, IDV_OPTM_INDEX_DRDB_FIX_PAGE );

    return rc;
}


/****************************************************************
 * Description :
 *  단지 sdbBufferPool의 관련 함수를 호출하는 껍데기 역활만 한다.
 *  자세한 설명은 sdbBufferPool의 관련 함수를 참조
 ****************************************************************/
inline IDE_RC sdbBufferMgr::unfixPage( idvSQL     * aStatistics,
                                       UChar      * aPagePtr)
{
    mBufferPool.unfixPage( aStatistics, aPagePtr );

    return IDE_SUCCESS;
}

inline void sdbBufferMgr::unfixPage( idvSQL     * aStatistics,
                                     sdbBCB*      aBCB)
{
    mBufferPool.unfixPage( aStatistics, aBCB );
}

#ifdef DEBUG
inline idBool sdbBufferMgr::isPageExist( idvSQL       * aStatistics,
                                         scSpaceID      aSpaceID,
                                         scPageID       aPageID )
{
    
    return mBufferPool.isPageExist( aStatistics, 
                                    aSpaceID, 
                                    aPageID );
}
#endif

inline UInt sdbBufferMgr::getPageCount()
{
    return mPageCount;
}

inline sdbBufferPoolStat *sdbBufferMgr::getBufferPoolStat()
{
    return mBufferPool.getBufferPoolStat();
}

/****************************************************************
 * Description :
 *  이 함수는 주로 사용자가 smuProperty를 통해 sdbBufferManager내의
 *  변수 값을 바꿀 때 사용한다. 즉, 사용자들 간의 동시성 제어를 위해 사용하고,
 *  내부에서는 사용하지 않는다.
 ****************************************************************/
inline void sdbBufferMgr::lockBufferMgrMutex( idvSQL  *aStatistics )
{
    IDE_ASSERT( mBufferMgrMutex.lock(aStatistics)
                == IDE_SUCCESS );
}

inline void sdbBufferMgr::unlockBufferMgrMutex()
{
    IDE_ASSERT( mBufferMgrMutex.unlock()
                == IDE_SUCCESS );
}

inline UInt sdbBufferMgr::getPageFixCnt( UChar * aPage)
{
    sdbBCB * sBCB = getBCBFromPagePtr( aPage );
    return sBCB->mFixCnt;
}

inline void sdbBufferMgr::setPageLSN( void     *aBCB,
                                      void     *aMtx )
{
    mBufferPool.setPageLSN( (sdbBCB*)aBCB, aMtx );
}

inline void sdbBufferMgr::latchPage( idvSQL        *aStatistics,
                                     UChar         *aPagePtr,
                                     sdbLatchMode   aLatchMode,
                                     sdbWaitMode    aWaitMode,
                                     idBool        *aTrySuccess )
{
    sdbBCB *sBCB = getBCBFromPagePtr( aPagePtr );

    mBufferPool.latchPage( aStatistics,
                           sBCB,
                           aLatchMode,
                           aWaitMode,
                           aTrySuccess );

}


inline void sdbBufferMgr::tryEscalateLatchPage( idvSQL            *aStatistics,
                                                UChar             *aPagePtr,
                                                sdbPageReadMode    aReadMode,
                                                idBool            *aTrySuccess )
{
    sdbBCB *sBCB = getBCBFromPagePtr( aPagePtr );

    mBufferPool.tryEscalateLatchPage( aStatistics,
                                      sBCB,
                                      aReadMode,
                                      aTrySuccess );
}

inline void sdbBufferMgr::unlatchPage( UChar *aPagePtr )
{
    sdbBCB *sBCB = getBCBFromPagePtr( aPagePtr );

    mBufferPool.unlatchPage( sBCB );
}

inline sdbBufferPool* sdbBufferMgr::getBufferPool()
{
    return &mBufferPool;
}

// BUG-45598
inline idBool sdbBufferMgr::isPageReadError( UChar *aPagePtr )
{
    sdbBCB *sBCB = getBCBFromPagePtr( aPagePtr );

    return sBCB->mPageReadError;
}

/* PROJ-2102 Secondary Buffer */
/****************************************************************
 * Description : Secondary Buffer를 사용 못함 
 ****************************************************************/
inline void sdbBufferMgr::setSBufferUnserviceable( void )
{
     mBufferPool.setSBufferServiceState( ID_FALSE );
}

/****************************************************************
 * Description : Secondary Buffer가 사용 가능
 ****************************************************************/
inline void sdbBufferMgr::setSBufferServiceable( void )
{
     mBufferPool.setSBufferServiceState( ID_TRUE );
}


#endif //_O_SDB_BUFFER_MGR_H_
