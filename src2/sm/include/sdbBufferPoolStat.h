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

#ifndef  _O_SDB_BUFFER_POOL_STAT_H_
#define  _O_SDB_BUFFER_POOL_STAT_H_ 1

#include <idl.h>
#include <idu.h>
#include <idv.h>
#include <smDef.h>
#include <sdbDef.h>
#include <sdpDef.h>
#include <sdbBCB.h>

typedef struct sdbBufferPoolStatData
{
    UInt      mID;
    UInt      mPoolSize;
    UInt      mPageSize;
    UInt      mHashBucketCount;
    UInt      mHashChainLatchCount;
    UInt      mLRUListCount;
    UInt      mPrepareListCount;
    UInt      mFlushListCount;
    UInt      mCheckpointListCount;
    UInt      mVictimSearchCount;
    UInt      mHashPages;
    UInt      mHotListPages;
    UInt      mColdListPages;
    UInt      mPrepareListPages;
    UInt      mFlushListPages;
    UInt      mCheckpointListPages;
    ULong     mFixPages;
    ULong     mGetPages;
    ULong     mReadPages;
    ULong     mCreatePages;
    SDouble   mHitRatio;
    /* LRU hot에서 hit한 횟수 */
    ULong     mHotHits;
    
    /* LRU Cold에서 hit한 횟수 */
    ULong     mColdHits;
    
    /* prepare List에서 hit한 횟수 */
    ULong     mPrepareHits;
    
    /* flush List에서 hit한 횟수 */
    ULong     mFlushHits;
    
    /* Delayed Flush List에서 hit한 횟수 */
    ULong     mDelayedFlushHits;

    /* prepare, lru, flush list에 없을때 hit한 횟수  */
    ULong     mOtherHits;
    
    /* prepare에서 victim을 찾은 횟수*/
    ULong     mPrepareVictims;
    
    /* LRU에서 victim을 찾은 횟수 */
    ULong     mLRUVictims;
    
    /* LRU에서 victim을 찾지 못한 횟수 */
    ULong     mVictimFails;
    
    /* victim을 찾기 위해 flusher를 기다린 횟수 */
    ULong     mVictimWaits;
    
    /* prepareList에서 대기하다가 BCB를 얻은 횟수 */
    ULong     mPrepareAgainVictims;
    
    /* prepareList에서 대기하다가 BCB를 얻지 못한 횟수 */
    ULong     mVictimSearchWarps;
    
    /* LRU list search 횟수 */
    ULong     mLRUSearchs;
    
    /* LRU List에서 victim을 찾을때, 보통 한번에 몇개의 BCB를 search하는지
     * 통계 */
    UInt      mLRUSearchsAvg;
    
    /* LRU search 도중 LRU Hot으로 BCB를 보낸 횟수 */
    ULong     mLRUToHots;
    
    /* LRU search 도중 LRU Mid로 BCB를 보낸 횟수 */
    ULong     mLRUToColds;
    
    /* LRU search 도중 FlushList로 BCB를 보낸 횟수 */
    ULong     mLRUToFlushs;

    /* LRU Hot에 삽입한 횟수 */
    ULong     mHotInsertions;
    
    /* LRU Cold에 삽입한 횟수 */
    ULong     mColdInsertions;

    /* BUG-32670    [sm-disk-resource] add IO Stat information 
     * for analyzing storage performance. */
    /* 일반 Read시 checksum 계산 시간 */
    ULong     mNormalCalcChecksumTime;

    /* 일반 Read시 Storage로부터 Read한 시간 */
    ULong     mNormalReadTime;

    /* 일반 Read시 Read한 Page 개수  */
    ULong     mNormalReadPageCount;

    /* Fullscan Read시 checksum 계산 시간 */
    ULong     mMPRCalcChecksumTime;

    /* Fullscan Read시 Storage로부터 Read한 시간 */
    ULong     mMPRReadTime;

    /* Fullscan Read시 Read한 Page 개수  */
    ULong     mMPRReadPageCount;
} sdbBufferPoolStatData;

class sdbBufferPool;

/* 각 page type별로 정보를 따로 유지하기 위해 필요한 자료구조 */
typedef struct sdbPageTypeStatData
{
    /* idvOwner */
    UInt      mOwner;
    UInt      mPageType;
    ULong     mFixPages;
    ULong     mGetPages;
    ULong     mReadPages;
    ULong     mCreatePages;
    
    /* LRU list로 부터 Victim을 찾은 회수 */
    ULong     mVictimPagesFromLRU;     

    /* Prepare list로 부터 Victim을 찾은 회수 */
    ULong     mVictimPagesFromPrepare; 

    /* LRU list에서 Victim을 찾을 동안 Skip된 페이지의 개수 */
    ULong     mSkipPagesFromLRU;
    
    /* Prepare list에서 Victim을 찾을 동안 Skip된 페이지의 개수 */
    ULong     mSkipPagesFromPrepare;
    
    SDouble   mHitRatio;

    /* LRU Hot에서 hit한 횟수 */
    ULong     mHotHits;

    /* LRU Cold에서 hit한 횟수 */
    ULong     mColdHits;

    /* prepare list에서 hit한 횟수 */
    ULong     mPrepareHits;

    /* flush list에서 hit한 횟수 */
    ULong     mFlushHits;

    /* Delayed Flush List에서 hit한 횟수 */
    ULong     mDelayedFlushHits;

    /* list외 에서 hit한 횟수 */
    ULong     mOtherHits;
} sdbPageTypeStatData;

class sdbBufferPoolStat
{
public:
    IDE_RC initialize(sdbBufferPool *aBufferPool);
    IDE_RC destroy();

    void   updateBufferPoolStat();
    // 페이지 타입별 정보 갱신
    void applyFixPages( idvSQL   * aStatistics,
                        scSpaceID  aSpaceID,
                        scPageID   aPageID,
                        UInt       aPageType );

    void applyGetPages( idvSQL    * aStatistics,
                        scSpaceID   aSpaceID,
                        scPageID    aPageID,
                        UInt        aPageType );

    void applyReadPages( idvSQL    *aStatistics,
                         scSpaceID  aSpaceID,
                         scPageID   aPageID,
                         UInt       aPageType );

    void applyCreatePages(idvSQL   * aStatistics,
                          scSpaceID  aSpaceID,
                          scPageID   aPageID,
                          UInt       aPageType);

    inline void applyHits(idvSQL *aStatistics, UInt aPageType, UInt aListType);
    
    inline void applyBeforeMultiReadPages(idvSQL *aStatistics);
    inline void applyAfterMultiReadPages(idvSQL *aStatistics);
    inline void applyBeforeSingleReadPage(idvSQL *aStatistics);
    inline void applyAfterSingleReadPage(idvSQL *aStatistics);

    void applyMultiReadPages( idvSQL    * aStatistics,
                              scSpaceID   aSpaceID,
                              scPageID    aPageID,
                              UInt        aPageType);

    void applyVictimPagesFromLRU( idvSQL   * aStatistics,
                                  scSpaceID  aSpaceID,
                                  scPageID   aPageID,
                                  UInt       aPageType );

    void applyVictimPagesFromPrepare( idvSQL    * aStatistics,
                                      scSpaceID   aSpaceID,
                                      scPageID    aPageID,
                                      UInt        aPageType );

    void applySkipPagesFromLRU( idvSQL   * aStatistics,
                                scSpaceID  aSpaceID,
                                scPageID   aPageID,
                                UInt       aPageType );

    void applySkipPagesFromPrepare(idvSQL   * aStatistics,
                                   scSpaceID  aSpaceID,
                                   scPageID   aPageID,
                                   UInt       aPageType);


    // find victim 통계 정보 갱신
    inline void applyPrepareVictims();
    inline void applyLRUVictims();
    inline void applyPrepareAgainVictims();
    inline void applyVictimWaits();
    inline void applyVictimSearchWarps();

    // victim search 통계 정보 갱신
    inline void applyVictimSearchs();
    inline void applyVictimSearchsToHot();
    inline void applyVictimSearchsToCold();
    inline void applyVictimSearchsToFlush();

    // hot, cold 삽입 비중 정보 갱신
    inline void applyHotInsertions();
    inline void applyColdInsertions();

    /* BUG-32670    [sm-disk-resource] add IO Stat information 
     * for analyzing storage performance. */
    inline void applyReadByNormal( ULong  aChecksumTime,
                                   ULong  aReadTime,
                                   ULong  aReadPageCount );
    inline void applyReadByMPR( ULong  aChecksumTime,
                                ULong  aReadTime,
                                ULong  aReadPageCount );

    inline void applyVictimWaitTimeMsec( ULong aMSec );

    void clearAll();

    void applyStatisticsForSystem();

    IDE_RC buildRecord(void                *aHeader,
                       iduFixedTableMemory *aMemory);

    IDE_RC buildPageInfoRecord(void                *aHeader,
                               iduFixedTableMemory *aMemory);


    SDouble getSingleReadPerf() /* USec단위*/
    {
        SDouble sRet = 0;
        if( mPoolStat.mNormalReadTime == 0 )
        {
            sRet = 0;
        }
        else
        {
            sRet = (SDouble)mPoolStat.mNormalReadTime /
                   (SDouble)mPoolStat.mNormalReadPageCount;
        }
        return sRet;
    }
    SDouble getMultiReadPerf() /* USec단위*/
    {
        SDouble sRet = 0;
        if( mPoolStat.mMPRReadTime == 0 )
        {
            sRet = 0;
        }
        else
        {
            sRet = (SDouble)mPoolStat.mMPRReadTime /
                   (SDouble)mPoolStat.mMPRReadPageCount;
        }
        return sRet;
    }

private:
    inline SDouble getHitRatio( ULong aGetFixPages, ULong aGetPages );

private:
    /* 통계정보를 유지할 buffer pool */
    sdbBufferPool         *mPool;
    
    /* buffer pool 전체 자료구조 */
    sdbBufferPoolStatData  mPoolStat;
    
    /* mPageTypeStat[ PageType ][ owner ].mHitRatio와 같이 접근 */
    sdbPageTypeStatData  (*mPageTypeStat)[IDV_OWNER_MAX];
    
    /* page type갯수*/
    UInt                   mPageTypeCount;
};

void sdbBufferPoolStat::applyHits(idvSQL *aStatistics,
                                  UInt    aPageType,
                                  UInt    aListType)
{
    //IDE_DASSERT( aPageType < mPageTypeCount);
    //TODO: 1568
    idvOwner sOwner;

    if( aStatistics == NULL )
    {
        sOwner = IDV_OWNER_UNKNOWN;
    }
    else
    {
        sOwner = aStatistics->mOwner;
    }

    if( aPageType >= mPageTypeCount )
    {
        /* aPageType이 아직 초기화 되지 않은 경우가 있다.
         * 즉, hash Table에 존재하므로, hit를 하여서 본 함수를 호출하였었겠지만,
         * replace또는 MPR인 경우를 생각해보면,
         * 이둘은 먼저 hash테이블에 삽입을 하고 난 이후에,
         * 실제로 데이터를 읽어 오기 때문에(실제 데이터를 읽어 오기 전까지는
         * 어떤 type인지 알 수 없다)
         * pageType이 제대로 설정되어 있지 않을 수 있다.
         * BUGBUG: 이경우 undefinedPageType으로 따로 설정해 놓고,
         * 따로 통계정보를 모아도 좋을 듯 싶다.
         */
        return;
    }
    
    switch (aListType)
    {
        case SDB_BCB_LRU_HOT:
            mPageTypeStat[aPageType][sOwner].mHotHits++;
            break;
        case SDB_BCB_LRU_COLD:
            mPageTypeStat[aPageType][sOwner].mColdHits++;
            break;
        case SDB_BCB_PREPARE_LIST:
            mPageTypeStat[aPageType][sOwner].mPrepareHits++;
            break;
        case SDB_BCB_FLUSH_LIST:
            mPageTypeStat[aPageType][sOwner].mFlushHits++;
            break;
        case SDB_BCB_DELAYED_FLUSH_LIST:
            mPageTypeStat[aPageType][sOwner].mDelayedFlushHits++;
            break;
        default:
            mPageTypeStat[aPageType][sOwner].mOtherHits++;
            break;
    }
}

void sdbBufferPoolStat::applyBeforeMultiReadPages(idvSQL *aStatistics)
{
    idvWeArgs   sWeArgs;

    IDV_WEARGS_SET( &sWeArgs,
                    IDV_WAIT_INDEX_DB_FILE_MULTI_PAGE_READ,
                    0, /* WaitParam1 */
                    0, /* WaitParam2 */
                    0  /* WaitParam3 */ );

    IDV_BEGIN_WAIT_EVENT(aStatistics, &sWeArgs );
}

void sdbBufferPoolStat::applyAfterMultiReadPages(idvSQL *aStatistics)
{
    idvWeArgs   sWeArgs;

    IDV_WEARGS_SET( &sWeArgs,
                    IDV_WAIT_INDEX_DB_FILE_MULTI_PAGE_READ,
                    0, /* WaitParam1 */
                    0, /* WaitParam2 */
                    0  /* WaitParam3 */ );

    IDV_END_WAIT_EVENT( aStatistics, &sWeArgs );
}

void sdbBufferPoolStat::applyBeforeSingleReadPage(idvSQL *aStatistics)
{
    idvWeArgs   sWeArgs;

    IDV_WEARGS_SET( &sWeArgs,
                    IDV_WAIT_INDEX_DB_FILE_SINGLE_PAGE_READ,
                    0, /* WaitParam1 */
                    0, /* WaitParam2 */
                    0  /* WaitParam3 */ );

    IDV_BEGIN_WAIT_EVENT(aStatistics, &sWeArgs );
}

void sdbBufferPoolStat::applyAfterSingleReadPage(idvSQL *aStatistics)
{
    idvWeArgs   sWeArgs;

    IDV_WEARGS_SET( &sWeArgs,
                    IDV_WAIT_INDEX_DB_FILE_SINGLE_PAGE_READ,
                    0, /* WaitParam1 */
                    0, /* WaitParam2 */
                    0  /* WaitParam3 */ );

    IDV_END_WAIT_EVENT( aStatistics, &sWeArgs );
}



void sdbBufferPoolStat::applyPrepareVictims()
{
    mPoolStat.mPrepareVictims++;
}

void sdbBufferPoolStat::applyLRUVictims()
{
    mPoolStat.mLRUVictims++;
}

void sdbBufferPoolStat::applyPrepareAgainVictims()
{
    mPoolStat.mVictimFails++;
    mPoolStat.mPrepareAgainVictims++;
}

void sdbBufferPoolStat::applyVictimWaits()
{
    mPoolStat.mVictimWaits++;
}

void sdbBufferPoolStat::applyVictimSearchWarps()
{
    mPoolStat.mVictimFails++;
    mPoolStat.mVictimSearchWarps++;
}

void sdbBufferPoolStat::applyVictimSearchs()
{
    mPoolStat.mLRUSearchs++;
}

void sdbBufferPoolStat::applyVictimSearchsToHot()
{
    mPoolStat.mLRUToHots++;
}

void sdbBufferPoolStat::applyVictimSearchsToCold()
{
    mPoolStat.mLRUToColds++;
}

void sdbBufferPoolStat::applyVictimSearchsToFlush()
{
    mPoolStat.mLRUToFlushs++;
}

void sdbBufferPoolStat::applyHotInsertions()
{
    mPoolStat.mHotInsertions++;
}

void sdbBufferPoolStat::applyColdInsertions()
{
    mPoolStat.mColdInsertions++;
}

void sdbBufferPoolStat::applyReadByNormal( ULong  aChecksumTime,
                                           ULong  aReadTime,
                                           ULong  aReadPageCount )
{
    mPoolStat.mNormalCalcChecksumTime += aChecksumTime;
    mPoolStat.mNormalReadTime         += aReadTime;
    mPoolStat.mNormalReadPageCount    += aReadPageCount;
}

void sdbBufferPoolStat::applyReadByMPR( ULong  aChecksumTime,
                                        ULong  aReadTime,
                                        ULong  aReadPageCount )
{
    mPoolStat.mMPRCalcChecksumTime += aChecksumTime;
    mPoolStat.mMPRReadTime         += aReadTime;
    mPoolStat.mMPRReadPageCount    += aReadPageCount;
}

/* BUG-21307: VS6.0에서 Compile Error발생.
 *
 * ULong이 double로 casting시 win32에서 에러 발생 */
inline SDouble sdbBufferPoolStat::getHitRatio( ULong aGetFixPages,
                                               ULong aReadPages )
{
    SDouble sGetFixPages = UINT64_TO_DOUBLE( aGetFixPages );
    SDouble sMissGetPages = sGetFixPages - UINT64_TO_DOUBLE( aReadPages );

    return sMissGetPages / sGetFixPages * 100.0;
}

#endif//   _O_SDB_BUFFER_POOL_STAT_H_
