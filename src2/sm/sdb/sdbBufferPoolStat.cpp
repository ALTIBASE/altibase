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
 * $$Id:$
 **********************************************************************/

#include <smDef.h>
#include <sdbDef.h>
#include <sdbBufferPool.h>
#include <sdbBufferPoolStat.h>
#include <sdbReq.h>
#include <smErrorCode.h>


/***********************************************************************
 * Description : 생성자
 *
 *  aBufferPool     - [IN]  버퍼풀
 ***********************************************************************/
IDE_RC sdbBufferPoolStat::initialize(sdbBufferPool *aBufferPool)
{
    mPool = aBufferPool;
    mPageTypeCount = smLayerCallback::getPhyPageTypeCount();

    updateBufferPoolStat();

    /* TC/FIT/Limit/sm/sdb/sdbBufferPoolStat_initialize_malloc.sql */
    IDU_FIT_POINT_RAISE( "sdbBufferPoolStat::initialize::malloc",
                          insufficient_memory );

    IDE_TEST_RAISE(iduMemMgr::malloc(IDU_MEM_SM_SDB,
                                     ID_SIZEOF(sdbPageTypeStatData) *
                                     mPageTypeCount * IDV_OWNER_MAX,
                                     (void**)&mPageTypeStat) != IDE_SUCCESS,
                   insufficient_memory );

    clearAll();

    return IDE_SUCCESS;

    IDE_EXCEPTION( insufficient_memory );
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_InsufficientMemory));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/***********************************************************************
 * Description :
 *  buffer pool 에 통계정보 적용
 ***********************************************************************/
void sdbBufferPoolStat::updateBufferPoolStat()
{
    mPoolStat.mID                   = 0;
    mPoolStat.mPoolSize             = mPool->getPoolSize();
    mPoolStat.mPageSize             = mPool->getPageSize();
    mPoolStat.mHashBucketCount      = mPool->getHashBucketCount();
    mPoolStat.mHashChainLatchCount  = mPool->getHashChainLatchCount();
    mPoolStat.mLRUListCount         = mPool->getLRUListCount();
    mPoolStat.mPrepareListCount     = mPool->getPrepareListCount();
    mPoolStat.mFlushListCount       = mPool->getFlushListCount();
    mPoolStat.mCheckpointListCount  = mPool->getCheckpointListCount();
}


/***********************************************************************
 * Description :
 *  소멸자
 ***********************************************************************/
IDE_RC sdbBufferPoolStat::destroy()
{
    IDE_ASSERT( mPageTypeStat != NULL );

    IDE_TEST(iduMemMgr::free(mPageTypeStat) != IDE_SUCCESS);

    mPageTypeStat = NULL;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : Fix Page 통계정보를 갱신한다.
 *
 * aStatistics - [IN] 통계정보
 * aSpaceID    - [IN] SpaceID
 * aPageID     - [IN] PageID
 * aPageType   - [IN] Page Type
 ***********************************************************************/
void sdbBufferPoolStat::applyFixPages( idvSQL    * aStatistics,
                                       scSpaceID   aSpaceID,
                                       scPageID    aPageID,
                                       UInt        aPageType )
{
    idvSession *sSess = (aStatistics == NULL ? NULL : aStatistics->mSess);

    if( aSpaceID == SMI_ID_TABLESPACE_SYSTEM_DISK_UNDO )
    {
        IDV_SQL_ADD(aStatistics, mUndoFixPageCount, 1);
        IDV_SESS_ADD(sSess, IDV_STAT_INDEX_UNDO_FIX_PAGE, 1);
    }
    else
    {
        IDV_SQL_ADD(aStatistics, mFixPageCount, 1);
        IDV_SESS_ADD(sSess, IDV_STAT_INDEX_FIX_PAGE, 1);
    }

    if ( smLayerCallback::isValidPageType( aSpaceID,
                                           aPageID,
                                           aPageType )
         == ID_TRUE )
    {
        if( aStatistics == NULL )
        {
            mPageTypeStat[aPageType][IDV_OWNER_UNKNOWN].mFixPages++;
        }
        else
        {
            mPageTypeStat[aPageType][aStatistics->mOwner].mFixPages++;
        }
    }
}

/***********************************************************************
 * Description : Get Page 통계정보를 갱신한다.
 *
 * aStatistics - [IN] 통계정보
 * aSpaceID    - [IN] SpaceID
 * aPageID     - [IN] PageID
 * aPageType   - [IN] Page Type
 ***********************************************************************/
void sdbBufferPoolStat::applyGetPages( idvSQL    *aStatistics,
                                       scSpaceID  aSpaceID,
                                       scPageID   aPageID,
                                       UInt       aPageType )
{
    idvSession *sSess = (aStatistics == NULL ? NULL : aStatistics->mSess);

    if( aSpaceID == SMI_ID_TABLESPACE_SYSTEM_DISK_UNDO )
    {
        IDV_SQL_ADD(aStatistics, mUndoGetPageCount, 1);
        IDV_SESS_ADD(sSess, IDV_STAT_INDEX_UNDO_GET_PAGE, 1);
    }
    else
    {
        IDV_SQL_ADD(aStatistics, mGetPageCount, 1);
        IDV_SESS_ADD(sSess, IDV_STAT_INDEX_GET_PAGE, 1);
    }

    if ( smLayerCallback::isValidPageType( aSpaceID,
                                           aPageID,
                                           aPageType )
         == ID_TRUE )
    {
        if( aStatistics == NULL )
        {
            mPageTypeStat[aPageType][IDV_OWNER_UNKNOWN].mGetPages++;
        }
        else
        {
            mPageTypeStat[aPageType][aStatistics->mOwner].mGetPages++;
        }
    }
}

/***********************************************************************
 * Description : Physical Read Page 통계정보를 갱신한다.
 *
 * aStatistics - [IN] 통계정보
 * aSpaceID    - [IN] SpaceID
 * aPageID     - [IN] PageID
 * aPageType   - [IN] Page Type
 ***********************************************************************/
void sdbBufferPoolStat::applyReadPages( idvSQL    *aStatistics,
                                        scSpaceID  aSpaceID,
                                        scPageID   aPageID,
                                        UInt       aPageType )
{
    idvSession *sSess = (aStatistics == NULL ? NULL : aStatistics->mSess);

    if( aSpaceID == SMI_ID_TABLESPACE_SYSTEM_DISK_UNDO )
    {
        IDV_SQL_ADD(aStatistics, mUndoReadPageCount, 1);
        IDV_SESS_ADD(sSess, IDV_STAT_INDEX_UNDO_READ_PAGE, 1);
    }
    else
    {
        IDV_SQL_ADD(aStatistics, mReadPageCount, 1);
        IDV_SESS_ADD(sSess, IDV_STAT_INDEX_READ_PAGE, 1);
    }

    if ( smLayerCallback::isValidPageType( aSpaceID,
                                           aPageID,
                                           aPageType )
         == ID_TRUE )
    {
        if( aStatistics == NULL )
        {
            mPageTypeStat[aPageType][IDV_OWNER_UNKNOWN].mReadPages++;
        }
        else
        {
            mPageTypeStat[aPageType][aStatistics->mOwner].mReadPages++;
        }
    }
}

/***********************************************************************
 * Description : Create Page 통계정보를 갱신한다.
 *
 * aStatistics - [IN] 통계정보
 * aSpaceID    - [IN] SpaceID
 * aPageID     - [IN] PageID
 * aPageType   - [IN] Page Type
 ***********************************************************************/
void sdbBufferPoolStat::applyCreatePages( idvSQL    *aStatistics,
                                          scSpaceID  aSpaceID,
                                          scPageID   aPageID,
                                          UInt       aPageType )
{
    idvSession *sSess = (aStatistics == NULL ? NULL : aStatistics->mSess);

    if( aSpaceID == SMI_ID_TABLESPACE_SYSTEM_DISK_UNDO )
    {
        IDV_SQL_ADD(aStatistics, mUndoCreatePageCount, 1);
        IDV_SESS_ADD(sSess, IDV_STAT_INDEX_UNDO_CREATE_PAGE, 1);
    }
    else
    {
        IDV_SQL_ADD(aStatistics, mCreatePageCount, 1);
        IDV_SESS_ADD(sSess, IDV_STAT_INDEX_CREATE_PAGE, 1);
    }

    if ( smLayerCallback::isValidPageType( aSpaceID,
                                           aPageID,
                                           aPageType )
         == ID_TRUE )
    {
        if( aStatistics == NULL )
        {
            mPageTypeStat[aPageType][IDV_OWNER_UNKNOWN].mCreatePages++;
        }
        else
        {
            mPageTypeStat[aPageType][aStatistics->mOwner].mCreatePages++;
        }
    }
}

/***********************************************************************
 * Description : Multi Read Page 통계정보를 갱신한다.
 *
 * aStatistics - [IN] 통계정보
 * aSpaceID    - [IN] SpaceID
 * aPageID     - [IN] PageID
 * aPageType   - [IN] Page Type
 ***********************************************************************/
void sdbBufferPoolStat::applyMultiReadPages( idvSQL    *aStatistics,
                                             scSpaceID  aSpaceID,
                                             scPageID   aPageID,
                                             UInt       aPageType )
{
    idvSession *sSess = (aStatistics == NULL ? NULL : aStatistics->mSess);

    if( aSpaceID == SMI_ID_TABLESPACE_SYSTEM_DISK_UNDO )
    {
        IDV_SQL_ADD(aStatistics, mUndoReadPageCount, 1);
        IDV_SESS_ADD(sSess, IDV_STAT_INDEX_UNDO_READ_PAGE, 1);
    }
    else
    {
        IDV_SQL_ADD(aStatistics, mReadPageCount, 1);
        IDV_SESS_ADD(sSess, IDV_STAT_INDEX_READ_PAGE, 1);
    }

    if ( smLayerCallback::isValidPageType( aSpaceID,
                                           aPageID,
                                           aPageType )
         == ID_TRUE )
    {
        if( aStatistics == NULL )
        {
            mPageTypeStat[aPageType][IDV_OWNER_UNKNOWN].mReadPages++;
        }
        else
        {
            mPageTypeStat[aPageType][aStatistics->mOwner].mReadPages++;
        }
    }
}

/***********************************************************************
 * Description : LRU 리스트로부터 Vitim찾는 횟수 통계정보를 증가시킨다.
 *
 * aStatistics - [IN] 통계정보
 * aSpaceID    - [IN] SpaceID
 * aPageID     - [IN] PageID
 * aPageType   - [IN] Page Type
 ***********************************************************************/
void sdbBufferPoolStat::applyVictimPagesFromLRU( idvSQL    * aStatistics,
                                                 scSpaceID   aSpaceID,
                                                 scPageID    aPageID,
                                                 UInt        aPageType )
{
    if ( smLayerCallback::isValidPageType( aSpaceID,
                                           aPageID,
                                           aPageType )
         == ID_TRUE )
    {
        if( aStatistics == NULL )
        {
            mPageTypeStat[aPageType][IDV_OWNER_UNKNOWN].mVictimPagesFromLRU++;
        }
        else
        {
            mPageTypeStat[aPageType][aStatistics->mOwner].mVictimPagesFromLRU++;
        }
    }
}

/***********************************************************************
 * Description : Prepare 리스트로부터 Vitim찾는 횟수 통계정보를 증가시킨다.
 *
 * aStatistics - [IN] 통계정보
 * aSpaceID    - [IN] SpaceID
 * aPageID     - [IN] PageID
 * aPageType   - [IN] Page Type
 ***********************************************************************/
void sdbBufferPoolStat::applyVictimPagesFromPrepare( idvSQL    * aStatistics,
                                                     scSpaceID   aSpaceID,
                                                     scPageID    aPageID,
                                                     UInt        aPageType )
{
    if ( smLayerCallback::isValidPageType( aSpaceID,
                                           aPageID,
                                           aPageType )
         == ID_TRUE )
    {
        if( aStatistics == NULL )
        {
            mPageTypeStat[aPageType][IDV_OWNER_UNKNOWN].mVictimPagesFromPrepare++;
        }
        else
        {
            mPageTypeStat[aPageType][aStatistics->mOwner].mVictimPagesFromPrepare++;
        }
    }
}

/***********************************************************************
 * Description : LRU List에서 Victim을 찾을때 선택한 BCB가 Pinned이거나
 *               Hot이어서 Skip한 페이지 갯수 통계정보를 증가시킨다.
 *
 * aStatistics - [IN] 통계정보
 * aSpaceID    - [IN] SpaceID
 * aPageID     - [IN] PageID
 * aPageType   - [IN] Page Type
 ***********************************************************************/
void sdbBufferPoolStat::applySkipPagesFromLRU( idvSQL    * aStatistics,
                                               scSpaceID   aSpaceID,
                                               scPageID    aPageID,
                                               UInt        aPageType)
{
    if ( smLayerCallback::isValidPageType( aSpaceID,
                                           aPageID,
                                           aPageType )
         == ID_TRUE )
    {
        if( aStatistics == NULL )
        {
            mPageTypeStat[aPageType][IDV_OWNER_UNKNOWN].mSkipPagesFromLRU++;
        }
        else
        {
            mPageTypeStat[aPageType][aStatistics->mOwner].mSkipPagesFromLRU++;
        }
    }
}

/***********************************************************************
 * Description : Prepare List에서 Victim을 찾을때 선택한 BCB가 Pinned이거나
 *               Hot이어서 Skip한 페이지 갯수 통계정보를 증가시킨다.
 *
 * aStatistics - [IN] 통계정보
 * aSpaceID    - [IN] SpaceID
 * aPageID     - [IN] PageID
 * aPageType   - [IN] Page Type
 ***********************************************************************/
void sdbBufferPoolStat::applySkipPagesFromPrepare( idvSQL    * aStatistics,
                                                   scSpaceID   aSpaceID,
                                                   scPageID    aPageID,
                                                   UInt        aPageType )
{
    if ( smLayerCallback::isValidPageType( aSpaceID,
                                           aPageID,
                                           aPageType )
         == ID_TRUE )
    {
        if( aStatistics == NULL )
        {
            mPageTypeStat[aPageType][IDV_OWNER_UNKNOWN].mSkipPagesFromPrepare++;
        }
        else
        {
            mPageTypeStat[aPageType][aStatistics->mOwner].mSkipPagesFromPrepare++;
        }
    }
}

/***********************************************************************
 * Description :
 *  멤버 변수 초기화
 ***********************************************************************/
void sdbBufferPoolStat::clearAll()
{
    UInt i, j;

    mPoolStat.mVictimSearchCount = 0;

    for (i = 0 ; i < mPageTypeCount; i++)
    {
        for (j = 0; j < IDV_OWNER_MAX; j++)
        {
            mPageTypeStat[i][j].mOwner = j;
            mPageTypeStat[i][j].mPageType = i;
            mPageTypeStat[i][j].mFixPages = 0;
            mPageTypeStat[i][j].mGetPages = 0;
            mPageTypeStat[i][j].mReadPages = 0;
            mPageTypeStat[i][j].mCreatePages = 0;
            mPageTypeStat[i][j].mVictimPagesFromLRU = 0;
            mPageTypeStat[i][j].mVictimPagesFromPrepare = 0;
            mPageTypeStat[i][j].mSkipPagesFromLRU = 0;
            mPageTypeStat[i][j].mSkipPagesFromPrepare = 0;
            mPageTypeStat[i][j].mHitRatio = 0;
            mPageTypeStat[i][j].mHotHits = 0;
            mPageTypeStat[i][j].mColdHits = 0;
            mPageTypeStat[i][j].mPrepareHits = 0;
            mPageTypeStat[i][j].mFlushHits = 0;
            mPageTypeStat[i][j].mDelayedFlushHits = 0;
            mPageTypeStat[i][j].mOtherHits = 0;
        }
    }

    mPoolStat.mFixPages = 0;
    mPoolStat.mGetPages = 0;
    mPoolStat.mReadPages = 0;
    mPoolStat.mCreatePages = 0;
    mPoolStat.mHitRatio = 0;
    mPoolStat.mHotHits = 0;
    mPoolStat.mColdHits = 0;
    mPoolStat.mPrepareHits = 0;
    mPoolStat.mFlushHits = 0;
    mPoolStat.mDelayedFlushHits = 0;
    mPoolStat.mOtherHits = 0;

    mPoolStat.mHashPages = 0;
    mPoolStat.mHotListPages = 0;
    mPoolStat.mColdListPages = 0;
    mPoolStat.mPrepareListPages = 0;
    mPoolStat.mFlushListPages = 0;
    mPoolStat.mCheckpointListPages = 0;

    mPoolStat.mPrepareVictims = 0;
    mPoolStat.mLRUVictims = 0;
    mPoolStat.mVictimFails = 0;
    mPoolStat.mPrepareAgainVictims = 0;
    mPoolStat.mVictimWaits = 0;
    mPoolStat.mVictimSearchWarps = 0;

    mPoolStat.mLRUSearchs = 0;
    mPoolStat.mLRUSearchsAvg = 0;
    mPoolStat.mLRUToHots = 0;
    mPoolStat.mLRUToColds = 0;
    mPoolStat.mLRUToFlushs = 0;

    mPoolStat.mHotInsertions = 0;
    mPoolStat.mColdInsertions = 0;
}


/***********************************************************************
 * Description :
 *
 *  sdbBufferPool에서 직접 v$sysstat으로 반영되어야 하는 멤버에 대해
 *  바로 반영한다.
 **********************************************************************/
void sdbBufferPoolStat::applyStatisticsForSystem()
{
    UInt  i, j;
    ULong sReadPageCount        = 0;
    ULong sGetPageCount         = 0;
    ULong sCreatePageCount      = 0;
    ULong sFixPageCount         = 0;

    ULong sUndoReadPageCount    = 0;
    ULong sUndoGetPageCount     = 0;
    ULong sUndoCreatePageCount  = 0;
    ULong sUndoFixPageCount     = 0;

    for (i = 0; i < mPageTypeCount; i++)
    {
        for (j = 0; j < IDV_OWNER_MAX; j++)
        {
            if ( i == smLayerCallback::getUndoPageType() )
            {
                sUndoReadPageCount += mPageTypeStat[i][j].mReadPages;
                sUndoGetPageCount += mPageTypeStat[i][j].mGetPages;
                sUndoCreatePageCount += mPageTypeStat[i][j].mCreatePages;
                sUndoFixPageCount += mPageTypeStat[i][j].mFixPages;
            }
            else
            {
                sReadPageCount += mPageTypeStat[i][j].mReadPages;
                sGetPageCount += mPageTypeStat[i][j].mGetPages;
                sCreatePageCount += mPageTypeStat[i][j].mCreatePages;
                sFixPageCount += mPageTypeStat[i][j].mFixPages;
            }
        }
    }

    IDV_SYS_SET(IDV_STAT_INDEX_READ_PAGE,     (ULong)sReadPageCount);
    IDV_SYS_SET(IDV_STAT_INDEX_FIX_PAGE,      (ULong)sFixPageCount);
    IDV_SYS_SET(IDV_STAT_INDEX_GET_PAGE,      (ULong)sGetPageCount);
    IDV_SYS_SET(IDV_STAT_INDEX_CREATE_PAGE,   (ULong)sCreatePageCount);

    IDV_SYS_SET(IDV_STAT_INDEX_UNDO_READ_PAGE,   (ULong)sUndoReadPageCount);
    IDV_SYS_SET(IDV_STAT_INDEX_UNDO_FIX_PAGE,    (ULong)sUndoFixPageCount);
    IDV_SYS_SET(IDV_STAT_INDEX_UNDO_GET_PAGE,    (ULong)sUndoGetPageCount);
    IDV_SYS_SET(IDV_STAT_INDEX_UNDO_CREATE_PAGE, (ULong)sUndoCreatePageCount);
}

/***********************************************************************
 * Description :
 *  fixed table을 만드는 함수
 ***********************************************************************/
IDE_RC sdbBufferPoolStat::buildRecord(
            void                *aHeader,
            iduFixedTableMemory *aMemory)
{
    UInt  i, j;
    ULong sGetFixPages;

    updateBufferPoolStat();

    mPoolStat.mVictimSearchCount = mPool->getLRUSearchCount();
    mPoolStat.mHashPages = mPool->getHashPageCount();
    mPoolStat.mHotListPages = mPool->getHotListPageCount();
    mPoolStat.mColdListPages = mPool->getColdListPageCount();
    mPoolStat.mPrepareListPages = mPool->getPrepareListPageCount();
    mPoolStat.mFlushListPages = mPool->getFlushListPageCount();
    mPoolStat.mCheckpointListPages = mPool->getCheckpointListPageCount();

    mPoolStat.mFixPages    = 0;
    mPoolStat.mGetPages    = 0;
    mPoolStat.mReadPages   = 0;
    mPoolStat.mCreatePages = 0;
    mPoolStat.mHotHits     = 0;
    mPoolStat.mColdHits    = 0;
    mPoolStat.mPrepareHits = 0;
    mPoolStat.mFlushHits   = 0;
    mPoolStat.mDelayedFlushHits = 0;
    mPoolStat.mOtherHits   = 0;

    for (i = 0; i < mPageTypeCount; i++)
    {
        for (j = 0; j < IDV_OWNER_MAX; j++)
        {
            mPoolStat.mFixPages    += mPageTypeStat[i][j].mFixPages;
            mPoolStat.mGetPages    += mPageTypeStat[i][j].mGetPages;
            mPoolStat.mReadPages   += mPageTypeStat[i][j].mReadPages;
            mPoolStat.mCreatePages += mPageTypeStat[i][j].mCreatePages;
            mPoolStat.mHotHits     += mPageTypeStat[i][j].mHotHits;
            mPoolStat.mColdHits    += mPageTypeStat[i][j].mColdHits;
            mPoolStat.mPrepareHits += mPageTypeStat[i][j].mPrepareHits;
            mPoolStat.mFlushHits   += mPageTypeStat[i][j].mFlushHits;
            mPoolStat.mDelayedFlushHits += mPageTypeStat[i][j].mDelayedFlushHits;
            mPoolStat.mOtherHits   += mPageTypeStat[i][j].mOtherHits;

            /* BUG-23962: sdbBufferPoolStat::buildRecord에서 valgrind오류가 발생합니다. */
            sGetFixPages = mPageTypeStat[i][j].mFixPages + mPageTypeStat[i][j].mGetPages;
            if (sGetFixPages == 0)
            {
                mPageTypeStat[i][j].mHitRatio = 0;
            }
            else
            {
                mPageTypeStat[i][j].mHitRatio = getHitRatio( sGetFixPages,
                                                             mPageTypeStat[i][j].mReadPages );
            }
        }
    }

    sGetFixPages = mPoolStat.mFixPages + mPoolStat.mGetPages;
    if (sGetFixPages == 0)
    {
        mPoolStat.mHitRatio = 0;
    }
    else
    {
        mPoolStat.mHitRatio = getHitRatio( sGetFixPages, mPoolStat.mReadPages );
    }

    if (mPoolStat.mLRUVictims + mPoolStat.mVictimFails == 0)
    {
        mPoolStat.mLRUSearchsAvg = 0;
    }
    else
    {
        mPoolStat.mLRUSearchsAvg =
            mPoolStat.mLRUSearchs / (mPoolStat.mLRUVictims + mPoolStat.mVictimFails);
    }

    IDE_TEST(iduFixedTable::buildRecord(aHeader,
                                        aMemory,
                                        (void*)&mPoolStat)
             != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description :
 *  fixed table을 만드는 함수
 ***********************************************************************/
IDE_RC sdbBufferPoolStat::buildPageInfoRecord(
            void                *aHeader,
            iduFixedTableMemory *aMemory)
{
    UInt i, j;
    UInt sGetFixPages;

    for (i = 0; i < mPageTypeCount; i++)
    {
        for (j = 0; j < IDV_OWNER_MAX; j++)
        {
            sGetFixPages = mPageTypeStat[i][j].mFixPages + mPageTypeStat[i][j].mGetPages;
            if (sGetFixPages == 0)
            {
                mPageTypeStat[i][j].mHitRatio = 0;
            }
            else
            {
                mPageTypeStat[i][j].mHitRatio = getHitRatio( sGetFixPages,
                                                             mPageTypeStat[i][j].mReadPages );
            }

            IDE_TEST(iduFixedTable::buildRecord(aHeader,
                                                aMemory,
                                                (void*)&mPageTypeStat[i][j])
                     != IDE_SUCCESS);
        }
    }
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


