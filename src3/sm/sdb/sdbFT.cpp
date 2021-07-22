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

#include <idl.h>
#include <sdb.h>
#include <sdbBufferPool.h>
#include <sdbLRUList.h>
#include <sdbFlushList.h>
#include <sdbFlushMgr.h>
#include <sdbPrepareList.h>
#include <sdbCPListSet.h>
#include <sdbBufferPoolStat.h>
#include <sdbBCB.h>
#include <sdbBufferMgr.h>

#define SDB_LIST_NAME_LENGTH   (12)

typedef struct sdbBCBListStat
{
    SChar  mName[SDB_LIST_NAME_LENGTH];
    UInt   mNumber;
    UInt   mFirstBCBID;
    UInt   mFirstBCBSpaceID;
    UInt   mFirstBCBPageID;
    UInt   mLastBCBID;
    UInt   mLastBCBSpaceID;
    UInt   mLastBCBPageID;
    UInt   mLength;
} sdbBCBListStat;

static iduFixedTableColDesc gBCBListColDesc[] =
{
    {
        (SChar*)"NAME",
        IDU_FT_OFFSETOF(sdbBCBListStat, mName),
        IDU_FT_SIZEOF(sdbBCBListStat, mName),
        IDU_FT_TYPE_VARCHAR,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"NUMBER",
        IDU_FT_OFFSETOF(sdbBCBListStat, mNumber),
        IDU_FT_SIZEOF(sdbBCBListStat, mNumber),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"FIRST_BCB_ID",
        IDU_FT_OFFSETOF(sdbBCBListStat, mFirstBCBID),
        IDU_FT_SIZEOF(sdbBCBListStat, mFirstBCBID),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"FIRST_BCB_SPACEID",
        IDU_FT_OFFSETOF(sdbBCBListStat, mFirstBCBSpaceID),
        IDU_FT_SIZEOF(sdbBCBListStat, mFirstBCBSpaceID),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"FIRST_BCB_PAGEID",
        IDU_FT_OFFSETOF(sdbBCBListStat, mFirstBCBPageID),
        IDU_FT_SIZEOF(sdbBCBListStat, mFirstBCBPageID),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"LAST_BCB_ID",
        IDU_FT_OFFSETOF(sdbBCBListStat, mLastBCBID),
        IDU_FT_SIZEOF(sdbBCBListStat, mLastBCBID),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"LAST_BCB_SPACEID",
        IDU_FT_OFFSETOF(sdbBCBListStat, mLastBCBSpaceID),
        IDU_FT_SIZEOF(sdbBCBListStat, mLastBCBSpaceID),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"LAST_BCB_PAGEID",
        IDU_FT_OFFSETOF(sdbBCBListStat, mLastBCBPageID),
        IDU_FT_SIZEOF(sdbBCBListStat, mLastBCBPageID),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"LENGTH",
        IDU_FT_OFFSETOF(sdbBCBListStat, mLength),
        IDU_FT_SIZEOF(sdbBCBListStat, mLength),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL
    },
    {
        NULL,
        0,
        0,
        IDU_FT_TYPE_CHAR,
        NULL,
        0, 0, NULL // for internal use
    }
};

static IDE_RC buildRecordForBCBListStat(idvSQL              * /*aStatistics*/,
                                        void                *aHeader,
                                        void                */*aDumpObj*/,
                                        iduFixedTableMemory *aMemory)
{
    UInt            i;
    sdbBufferPool  *sPool;
    sdbLRUList     *sLRUList;
    sdbFlushList   *sFlushList;
    sdbPrepareList *sPrepareList;
    sdbCPListSet   *sCPListSet;
    sdbBCBListStat  sStat;
    sdbBCB         *sFirstBCB;
    sdbBCB         *sLastBCB;
    smuList        *sLastHotBCBListItem;

    sPool = sdbBufferMgr::getPool();

    for (i = 0; i < sPool->getLRUListCount(); i++)
    {
        sLRUList = sPool->getLRUList(i);
        sFirstBCB = sLRUList->getFirst();
        sLastBCB = sLRUList->getMid();

        if (sLastBCB != NULL)
        {
            /*
             *  BUG-25614 [SD] X$BCB_LIST 조회시 동시성 문제로 
             *            NULL Pointer를 참조할수 있습니다. 
             */
            sLastHotBCBListItem = SMU_LIST_GET_PREV(&sLastBCB->mBCBListItem);
            if( sLastHotBCBListItem != NULL)
            {
                sLastBCB = (sdbBCB*)sLastHotBCBListItem->mData;
            }
        }

        idlOS::sprintf(sStat.mName, "LRU_HOT");
        sStat.mNumber = sLRUList->getID();
        if (sFirstBCB != NULL)
        {
            sStat.mFirstBCBID      = sFirstBCB->mID;
            sStat.mFirstBCBSpaceID = sFirstBCB->mSpaceID;
            sStat.mFirstBCBPageID  = sFirstBCB->mPageID;
        }
        else
        {
            sStat.mFirstBCBID      = ID_UINT_MAX;
            sStat.mFirstBCBSpaceID = ID_UINT_MAX;
            sStat.mFirstBCBPageID  = ID_UINT_MAX;
        }
        if (sLastBCB != NULL)
        {
            sStat.mLastBCBID       = sLastBCB->mID;
            sStat.mLastBCBSpaceID  = sLastBCB->mSpaceID;
            sStat.mLastBCBPageID   = sLastBCB->mPageID;
        }
        else
        {
            sStat.mLastBCBID       = ID_UINT_MAX;
            sStat.mLastBCBSpaceID  = ID_UINT_MAX;
            sStat.mLastBCBPageID   = ID_UINT_MAX;
        }
        sStat.mLength = sLRUList->getHotLength();

        IDE_TEST(iduFixedTable::buildRecord(aHeader,
                                            aMemory,
                                            (void *)&sStat)
                 != IDE_SUCCESS);
    }

    for (i = 0; i < sPool->getLRUListCount(); i++)
    {
        sLRUList = sPool->getLRUList(i);
        sFirstBCB = sLRUList->getMid();
        sLastBCB = sLRUList->getLast();

        idlOS::sprintf(sStat.mName, "LRU_COLD");
        sStat.mNumber = sLRUList->getID();
        if (sFirstBCB != NULL)
        {
            sStat.mFirstBCBID      = sFirstBCB->mID;
            sStat.mFirstBCBSpaceID = sFirstBCB->mSpaceID;
            sStat.mFirstBCBPageID  = sFirstBCB->mPageID;
        }
        else
        {
            sStat.mFirstBCBID      = ID_UINT_MAX;
            sStat.mFirstBCBSpaceID = ID_UINT_MAX;
            sStat.mFirstBCBPageID  = ID_UINT_MAX;
        }
        if (sLastBCB != NULL)
        {
            sStat.mLastBCBID       = sLastBCB->mID;
            sStat.mLastBCBSpaceID  = sLastBCB->mSpaceID;
            sStat.mLastBCBPageID   = sLastBCB->mPageID;
        }
        else
        {
            sStat.mLastBCBID       = ID_UINT_MAX;
            sStat.mLastBCBSpaceID  = ID_UINT_MAX;
            sStat.mLastBCBPageID   = ID_UINT_MAX;
        }
        sStat.mLength = sLRUList->getColdLength();

        IDE_TEST(iduFixedTable::buildRecord(aHeader,
                                            aMemory,
                                            (void *)&sStat)
                 != IDE_SUCCESS);
    }

    for (i = 0; i < sPool->getFlushListCount(); i++)
    {
        sFlushList = sPool->getDelayedFlushList(i);
        sFirstBCB = sFlushList->getFirst();
        sLastBCB = sFlushList->getLast();

        idlOS::sprintf(sStat.mName, "DELAY_FLUSH");
        sStat.mNumber = sFlushList->getID();
        if (sFirstBCB != NULL)
        {
            sStat.mFirstBCBID      = sFirstBCB->mID;
            sStat.mFirstBCBSpaceID = sFirstBCB->mSpaceID;
            sStat.mFirstBCBPageID  = sFirstBCB->mPageID;
        }
        else
        {
            sStat.mFirstBCBID      = ID_UINT_MAX;
            sStat.mFirstBCBSpaceID = ID_UINT_MAX;
            sStat.mFirstBCBPageID  = ID_UINT_MAX;
        }
        if (sLastBCB != NULL)
        {
            sStat.mLastBCBID       = sLastBCB->mID;
            sStat.mLastBCBSpaceID  = sLastBCB->mSpaceID;
            sStat.mLastBCBPageID   = sLastBCB->mPageID;
        }
        else
        {
            sStat.mLastBCBID       = ID_UINT_MAX;
            sStat.mLastBCBSpaceID  = ID_UINT_MAX;
            sStat.mLastBCBPageID   = ID_UINT_MAX;
        }
        sStat.mLength = sFlushList->getPartialLength();

        IDE_TEST(iduFixedTable::buildRecord(aHeader,
                                            aMemory,
                                            (void *)&sStat)
                 != IDE_SUCCESS);
    }

    for (i = 0; i < sPool->getFlushListCount(); i++)
    {
        sFlushList = sPool->getFlushList(i);
        sFirstBCB = sFlushList->getFirst();
        sLastBCB = sFlushList->getLast();

        idlOS::sprintf(sStat.mName, "FLUSH");
        sStat.mNumber = sFlushList->getID();
        if (sFirstBCB != NULL)
        {
            sStat.mFirstBCBID      = sFirstBCB->mID;
            sStat.mFirstBCBSpaceID = sFirstBCB->mSpaceID;
            sStat.mFirstBCBPageID  = sFirstBCB->mPageID;
        }
        else
        {
            sStat.mFirstBCBID      = ID_UINT_MAX;
            sStat.mFirstBCBSpaceID = ID_UINT_MAX;
            sStat.mFirstBCBPageID  = ID_UINT_MAX;
        }
        if (sLastBCB != NULL)
        {
            sStat.mLastBCBID       = sLastBCB->mID;
            sStat.mLastBCBSpaceID  = sLastBCB->mSpaceID;
            sStat.mLastBCBPageID   = sLastBCB->mPageID;
        }
        else
        {
            sStat.mLastBCBID       = ID_UINT_MAX;
            sStat.mLastBCBSpaceID  = ID_UINT_MAX;
            sStat.mLastBCBPageID   = ID_UINT_MAX;
        }
        sStat.mLength = sFlushList->getPartialLength();

        IDE_TEST(iduFixedTable::buildRecord(aHeader,
                                            aMemory,
                                            (void *)&sStat)
                 != IDE_SUCCESS);
    }

    for (i = 0; i < sPool->getPrepareListCount(); i++)
    {
        sPrepareList = sPool->getPrepareList(i);
        sFirstBCB = sPrepareList->getFirst();
        sLastBCB = sPrepareList->getLast();

        idlOS::sprintf(sStat.mName, "PREPARE");
        sStat.mNumber = sPrepareList->getID();
        if (sFirstBCB != NULL)
        {
            sStat.mFirstBCBID      = sFirstBCB->mID;
            sStat.mFirstBCBSpaceID = sFirstBCB->mSpaceID;
            sStat.mFirstBCBPageID  = sFirstBCB->mPageID;
        }
        else
        {
            sStat.mFirstBCBID      = ID_UINT_MAX;
            sStat.mFirstBCBSpaceID = ID_UINT_MAX;
            sStat.mFirstBCBPageID  = ID_UINT_MAX;
        }
        if (sLastBCB != NULL)
        {
            sStat.mLastBCBID       = sLastBCB->mID;
            sStat.mLastBCBSpaceID  = sLastBCB->mSpaceID;
            sStat.mLastBCBPageID   = sLastBCB->mPageID;
        }
        else
        {
            sStat.mLastBCBID       = ID_UINT_MAX;
            sStat.mLastBCBSpaceID  = ID_UINT_MAX;
            sStat.mLastBCBPageID   = ID_UINT_MAX;
        }
        sStat.mLength = sPrepareList->getLength();

        IDE_TEST(iduFixedTable::buildRecord(aHeader,
                                            aMemory,
                                            (void *)&sStat)
                 != IDE_SUCCESS);
    }

    sCPListSet = sPool->getCPListSet();
    for (i = 0; i < sCPListSet->getListCount(); i++)
    {
        sFirstBCB = (sdbBCB*)sCPListSet->getFirst(i);
        sLastBCB  = (sdbBCB*)sCPListSet->getLast(i);

        idlOS::sprintf(sStat.mName, "CHECKPOINT");
        sStat.mNumber = i;
        if (sFirstBCB != NULL)
        {
            sStat.mFirstBCBID      = sFirstBCB->mID;
            sStat.mFirstBCBSpaceID = sFirstBCB->mSpaceID;
            sStat.mFirstBCBPageID  = sFirstBCB->mPageID;
        }
        else
        {
            sStat.mFirstBCBID      = ID_UINT_MAX;
            sStat.mFirstBCBSpaceID = ID_UINT_MAX;
            sStat.mFirstBCBPageID  = ID_UINT_MAX;
        }
        if (sLastBCB != NULL)
        {
            sStat.mLastBCBID       = sLastBCB->mID;
            sStat.mLastBCBSpaceID  = sLastBCB->mSpaceID;
            sStat.mLastBCBPageID   = sLastBCB->mPageID;
        }
        else
        {
            sStat.mLastBCBID       = ID_UINT_MAX;
            sStat.mLastBCBSpaceID  = ID_UINT_MAX;
            sStat.mLastBCBPageID   = ID_UINT_MAX;
        }
        sStat.mLength = sCPListSet->getLength(i);

        IDE_TEST(iduFixedTable::buildRecord(aHeader,
                                            aMemory,
                                            (void *)&sStat)
                 != IDE_SUCCESS);
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

iduFixedTableDesc gBCBListStat =
{
    (SChar *)"X$BCB_LIST",
    buildRecordForBCBListStat,
    gBCBListColDesc,
    IDU_STARTUP_META,
    0,
    0,
    IDU_FT_DESC_TRANS_NOT_USE,
    NULL
};

static iduFixedTableColDesc gBufferPoolStatTableColDesc[] =
{
    {
        (SChar*)"ID",
        IDU_FT_OFFSETOF(sdbBufferPoolStatData, mID),
        IDU_FT_SIZEOF(sdbBufferPoolStatData, mID),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"POOL_SIZE",
        IDU_FT_OFFSETOF(sdbBufferPoolStatData, mPoolSize),
        IDU_FT_SIZEOF(sdbBufferPoolStatData, mPoolSize),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"PAGE_SIZE",
        IDU_FT_OFFSETOF(sdbBufferPoolStatData, mPageSize),
        IDU_FT_SIZEOF(sdbBufferPoolStatData, mPageSize),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"HASH_BUCKET_COUNT",
        IDU_FT_OFFSETOF(sdbBufferPoolStatData, mHashBucketCount),
        IDU_FT_SIZEOF(sdbBufferPoolStatData, mHashBucketCount),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"HASH_CHAIN_LATCH_COUNT",
        IDU_FT_OFFSETOF(sdbBufferPoolStatData, mHashChainLatchCount),
        IDU_FT_SIZEOF(sdbBufferPoolStatData, mHashChainLatchCount),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"LRU_LIST_COUNT",
        IDU_FT_OFFSETOF(sdbBufferPoolStatData, mLRUListCount),
        IDU_FT_SIZEOF(sdbBufferPoolStatData, mLRUListCount),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"PREPARE_LIST_COUNT",
        IDU_FT_OFFSETOF(sdbBufferPoolStatData, mPrepareListCount),
        IDU_FT_SIZEOF(sdbBufferPoolStatData, mPrepareListCount),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"FLUSH_LIST_COUNT",
        IDU_FT_OFFSETOF(sdbBufferPoolStatData, mFlushListCount),
        IDU_FT_SIZEOF(sdbBufferPoolStatData, mFlushListCount),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"CHECKPOINT_LIST_COUNT",
        IDU_FT_OFFSETOF(sdbBufferPoolStatData, mCheckpointListCount),
        IDU_FT_SIZEOF(sdbBufferPoolStatData, mCheckpointListCount),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"VICTIM_SEARCH_COUNT",
        IDU_FT_OFFSETOF(sdbBufferPoolStatData, mVictimSearchCount),
        IDU_FT_SIZEOF(sdbBufferPoolStatData, mVictimSearchCount),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"HASH_PAGES",
        IDU_FT_OFFSETOF(sdbBufferPoolStatData, mHashPages),
        IDU_FT_SIZEOF(sdbBufferPoolStatData, mHashPages),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"HOT_LIST_PAGES",
        IDU_FT_OFFSETOF(sdbBufferPoolStatData, mHotListPages),
        IDU_FT_SIZEOF(sdbBufferPoolStatData, mHotListPages),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"COLD_LIST_PAGES",
        IDU_FT_OFFSETOF(sdbBufferPoolStatData, mColdListPages),
        IDU_FT_SIZEOF(sdbBufferPoolStatData, mColdListPages),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"PREPARE_LIST_PAGES",
        IDU_FT_OFFSETOF(sdbBufferPoolStatData, mPrepareListPages),
        IDU_FT_SIZEOF(sdbBufferPoolStatData, mPrepareListPages),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"FLUSH_LIST_PAGES",
        IDU_FT_OFFSETOF(sdbBufferPoolStatData, mFlushListPages),
        IDU_FT_SIZEOF(sdbBufferPoolStatData, mFlushListPages),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"CHECKPOINT_LIST_PAGES",
        IDU_FT_OFFSETOF(sdbBufferPoolStatData, mCheckpointListPages),
        IDU_FT_SIZEOF(sdbBufferPoolStatData, mCheckpointListPages),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"FIX_PAGES",
        IDU_FT_OFFSETOF(sdbBufferPoolStatData, mFixPages),
        IDU_FT_SIZEOF(sdbBufferPoolStatData, mFixPages),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"GET_PAGES",
        IDU_FT_OFFSETOF(sdbBufferPoolStatData, mGetPages),
        IDU_FT_SIZEOF(sdbBufferPoolStatData, mGetPages),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"READ_PAGES",
        IDU_FT_OFFSETOF(sdbBufferPoolStatData, mReadPages),
        IDU_FT_SIZEOF(sdbBufferPoolStatData, mReadPages),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"CREATE_PAGES",
        IDU_FT_OFFSETOF(sdbBufferPoolStatData, mCreatePages),
        IDU_FT_SIZEOF(sdbBufferPoolStatData, mCreatePages),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"HIT_RATIO",
        IDU_FT_OFFSETOF(sdbBufferPoolStatData, mHitRatio),
        IDU_FT_SIZEOF(sdbBufferPoolStatData, mHitRatio),
        IDU_FT_TYPE_DOUBLE,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"HOT_HITS",
        IDU_FT_OFFSETOF(sdbBufferPoolStatData, mHotHits),
        IDU_FT_SIZEOF(sdbBufferPoolStatData, mHotHits),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"COLD_HITS",
        IDU_FT_OFFSETOF(sdbBufferPoolStatData, mColdHits),
        IDU_FT_SIZEOF(sdbBufferPoolStatData, mColdHits),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"PREPARE_HITS",
        IDU_FT_OFFSETOF(sdbBufferPoolStatData, mPrepareHits),
        IDU_FT_SIZEOF(sdbBufferPoolStatData, mPrepareHits),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"FLUSH_HITS",
        IDU_FT_OFFSETOF(sdbBufferPoolStatData, mFlushHits),
        IDU_FT_SIZEOF(sdbBufferPoolStatData, mFlushHits),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"DELAYED_FLUSH_HITS",
        IDU_FT_OFFSETOF(sdbBufferPoolStatData, mDelayedFlushHits),
        IDU_FT_SIZEOF(sdbBufferPoolStatData, mDelayedFlushHits),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"OTHER_HITS",
        IDU_FT_OFFSETOF(sdbBufferPoolStatData, mOtherHits),
        IDU_FT_SIZEOF(sdbBufferPoolStatData, mOtherHits),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"PREPARE_VICTIMS",
        IDU_FT_OFFSETOF(sdbBufferPoolStatData, mPrepareVictims),
        IDU_FT_SIZEOF(sdbBufferPoolStatData, mPrepareVictims),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"LRU_VICTIMS",
        IDU_FT_OFFSETOF(sdbBufferPoolStatData, mLRUVictims),
        IDU_FT_SIZEOF(sdbBufferPoolStatData, mLRUVictims),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"VICTIM_FAILS",
        IDU_FT_OFFSETOF(sdbBufferPoolStatData, mVictimFails),
        IDU_FT_SIZEOF(sdbBufferPoolStatData, mVictimFails),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"PREPARE_AGAIN_VICTIMS",
        IDU_FT_OFFSETOF(sdbBufferPoolStatData, mPrepareAgainVictims),
        IDU_FT_SIZEOF(sdbBufferPoolStatData, mPrepareAgainVictims),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"VICTIM_SEARCH_WARP",
        IDU_FT_OFFSETOF(sdbBufferPoolStatData, mVictimSearchWarps),
        IDU_FT_SIZEOF(sdbBufferPoolStatData, mVictimSearchWarps),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"VICTIM_WAITS",
        IDU_FT_OFFSETOF(sdbBufferPoolStatData, mVictimWaits),
        IDU_FT_SIZEOF(sdbBufferPoolStatData, mVictimWaits),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"LRU_SEARCHS",
        IDU_FT_OFFSETOF(sdbBufferPoolStatData, mLRUSearchs),
        IDU_FT_SIZEOF(sdbBufferPoolStatData, mLRUSearchs),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"LRU_SEARCHS_AVG",
        IDU_FT_OFFSETOF(sdbBufferPoolStatData, mLRUSearchsAvg),
        IDU_FT_SIZEOF(sdbBufferPoolStatData, mLRUSearchsAvg),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"LRU_TO_HOTS",
        IDU_FT_OFFSETOF(sdbBufferPoolStatData, mLRUToHots),
        IDU_FT_SIZEOF(sdbBufferPoolStatData, mLRUToHots),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"LRU_TO_COLDS",
        IDU_FT_OFFSETOF(sdbBufferPoolStatData, mLRUToColds),
        IDU_FT_SIZEOF(sdbBufferPoolStatData, mLRUToColds),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"LRU_TO_FLUSHS",
        IDU_FT_OFFSETOF(sdbBufferPoolStatData, mLRUToFlushs),
        IDU_FT_SIZEOF(sdbBufferPoolStatData, mLRUToFlushs),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"HOT_INSERTIONS",
        IDU_FT_OFFSETOF(sdbBufferPoolStatData, mHotInsertions),
        IDU_FT_SIZEOF(sdbBufferPoolStatData, mHotInsertions),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"COLD_INSERTIONS",
        IDU_FT_OFFSETOF(sdbBufferPoolStatData, mColdInsertions),
        IDU_FT_SIZEOF(sdbBufferPoolStatData, mColdInsertions),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"NORMAL_READ_USEC",
        IDU_FT_OFFSETOF(sdbBufferPoolStatData, mNormalReadTime),
        IDU_FT_SIZEOF(sdbBufferPoolStatData, mNormalReadTime),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"NORMAL_READ_PAGE_COUNT",
        IDU_FT_OFFSETOF(sdbBufferPoolStatData, mNormalReadPageCount),
        IDU_FT_SIZEOF(sdbBufferPoolStatData, mNormalReadPageCount),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"NORMAL_CALC_CHECKSUM_USEC",
        IDU_FT_OFFSETOF(sdbBufferPoolStatData, mNormalCalcChecksumTime),
        IDU_FT_SIZEOF(sdbBufferPoolStatData, mNormalCalcChecksumTime),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"MPR_READ_USEC",
        IDU_FT_OFFSETOF(sdbBufferPoolStatData, mMPRReadTime),
        IDU_FT_SIZEOF(sdbBufferPoolStatData, mMPRReadTime),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"MPR_READ_PAGE_COUNT",
        IDU_FT_OFFSETOF(sdbBufferPoolStatData, mMPRReadPageCount),
        IDU_FT_SIZEOF(sdbBufferPoolStatData, mMPRReadPageCount),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"MPR_CALC_CHECKSUM_USEC",
        IDU_FT_OFFSETOF(sdbBufferPoolStatData, mMPRCalcChecksumTime),
        IDU_FT_SIZEOF(sdbBufferPoolStatData, mMPRCalcChecksumTime),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0, NULL
    },
    {
        NULL,
        0,
        0,
        IDU_FT_TYPE_CHAR,
        NULL,
        0, 0, NULL
    }
};

static iduFixedTableColDesc gBufferPageInfoTableColDesc[] =
{
    {
        (SChar*)"WHO",
        IDU_FT_OFFSETOF(sdbPageTypeStatData, mOwner),
        IDU_FT_SIZEOF(sdbPageTypeStatData, mOwner),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"PAGE_TYPE",
        IDU_FT_OFFSETOF(sdbPageTypeStatData, mPageType),
        IDU_FT_SIZEOF(sdbPageTypeStatData, mPageType),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"READ_PAGES",
        IDU_FT_OFFSETOF(sdbPageTypeStatData, mReadPages),
        IDU_FT_SIZEOF(sdbPageTypeStatData, mReadPages),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"GET_PAGES",
        IDU_FT_OFFSETOF(sdbPageTypeStatData, mGetPages),
        IDU_FT_SIZEOF(sdbPageTypeStatData, mGetPages),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"FIX_PAGES",
        IDU_FT_OFFSETOF(sdbPageTypeStatData, mFixPages),
        IDU_FT_SIZEOF(sdbPageTypeStatData, mFixPages),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"CREATE_PAGES",
        IDU_FT_OFFSETOF(sdbPageTypeStatData, mCreatePages),
        IDU_FT_SIZEOF(sdbPageTypeStatData, mCreatePages),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"VICTIM_PAGES_FROM_LRU",
        IDU_FT_OFFSETOF(sdbPageTypeStatData, mVictimPagesFromLRU),
        IDU_FT_SIZEOF(sdbPageTypeStatData, mVictimPagesFromLRU),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"VICTIM_PAGES_FROM_PREPARE",
        IDU_FT_OFFSETOF(sdbPageTypeStatData, mVictimPagesFromPrepare),
        IDU_FT_SIZEOF(sdbPageTypeStatData, mVictimPagesFromPrepare),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"SKIP_PAGES_FROM_LRU",
        IDU_FT_OFFSETOF(sdbPageTypeStatData, mSkipPagesFromLRU),
        IDU_FT_SIZEOF(sdbPageTypeStatData, mSkipPagesFromLRU),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"SKIP_PAGES_FROM_PREPARE",
        IDU_FT_OFFSETOF(sdbPageTypeStatData, mSkipPagesFromPrepare),
        IDU_FT_SIZEOF(sdbPageTypeStatData, mSkipPagesFromPrepare),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"HIT_RATIO",
        IDU_FT_OFFSETOF(sdbPageTypeStatData, mHitRatio),
        IDU_FT_SIZEOF(sdbPageTypeStatData, mHitRatio),
        IDU_FT_TYPE_DOUBLE,
        NULL,
        0, 0, NULL
    },
    {
        NULL,
        0,
        0,
        IDU_FT_TYPE_CHAR,
        NULL,
        0, 0, NULL
    }
};

/***********************************************************************
 * Description :
 *  fixed table을 만드는 함수
 ***********************************************************************/
IDE_RC sdbFT::buildRecordForBufferPoolStat(
    idvSQL              * /*aStatistics*/,
    void                *aHeader,
    void                */*aDumpObj*/,
    iduFixedTableMemory *aMemory)
{
    sdbBufferPoolStat *sPoolStat = sdbBufferMgr::getPool()->getBufferPoolStat();
    IDE_TEST(sPoolStat->buildRecord(aHeader, aMemory) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description :
 *  fixed table을 만드는 함수
 ***********************************************************************/
IDE_RC sdbFT::buildRecordForBufferPageInfo(
    idvSQL              * /*aStatistics*/,
    void                *aHeader,
    void                */*aDumpObj*/,
    iduFixedTableMemory *aMemory)
{
    sdbBufferPoolStat *sPoolStat = sdbBufferMgr::getPool()->getBufferPoolStat();
    IDE_TEST(sPoolStat->buildPageInfoRecord(aHeader, aMemory) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

iduFixedTableDesc gBufferPoolStatTableDesc =
{
    (SChar *)"X$BUFFER_POOL_STAT",
    sdbFT::buildRecordForBufferPoolStat,
    gBufferPoolStatTableColDesc,
    IDU_STARTUP_META,
    0,
    0,
    IDU_FT_DESC_TRANS_NOT_USE,
    NULL
};

iduFixedTableDesc gBufferPageInfoTableDesc =
{
    (SChar *)"X$BUFFER_PAGE_INFO",
    sdbFT::buildRecordForBufferPageInfo,
    gBufferPageInfoTableColDesc,
    IDU_STARTUP_META,
    0,
    0,
    IDU_FT_DESC_TRANS_NOT_USE,
    NULL
};

typedef struct sdbFlushMgrStat
{
    /* flush list길이가 이 길이 이하면 full flush하지 않는다. */
    UInt mLowFlushLength;

    /* full flush 해야 하는 flush list length */
    UInt mHighFlushLength;

    /* full flush해야 하는 prepare list length */
    UInt mLowPrepareLength;

    /* checkpoint flush 횟수 */
    ULong mCheckpointFlushCount;

    /* restart recovery시 redo할 page count */
    ULong mFastStartIoTarget;

    /* restart recovery시 redo할 LogFile count */
    UInt  mFastStartLogFileTarget;

    /* 현재 요청한 job 갯수 */
    UInt mReqJobCount;
} sdbFlushMgrStat;

static iduFixedTableColDesc gFlushMgrStatTableColDesc[] =
{
    {
        (SChar*)"LOW_FLUSH_LENGTH",
        IDU_FT_OFFSETOF(sdbFlushMgrStat, mLowFlushLength),
        IDU_FT_SIZEOF(sdbFlushMgrStat, mLowFlushLength),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"HIGH_FLUSH_LENGTH",
        IDU_FT_OFFSETOF(sdbFlushMgrStat, mHighFlushLength),
        IDU_FT_SIZEOF(sdbFlushMgrStat, mHighFlushLength),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"LOW_PREPARE_LENGTH",
        IDU_FT_OFFSETOF(sdbFlushMgrStat, mLowPrepareLength),
        IDU_FT_SIZEOF(sdbFlushMgrStat, mLowPrepareLength),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"CHECKPOINT_FLUSH_COUNT",
        IDU_FT_OFFSETOF(sdbFlushMgrStat, mCheckpointFlushCount),
        IDU_FT_SIZEOF(sdbFlushMgrStat, mCheckpointFlushCount),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"FAST_START_IO_TARGET",
        IDU_FT_OFFSETOF(sdbFlushMgrStat, mFastStartIoTarget),
        IDU_FT_SIZEOF(sdbFlushMgrStat, mFastStartIoTarget),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"FAST_START_LOGFILE_TARGET",
        IDU_FT_OFFSETOF(sdbFlushMgrStat, mFastStartLogFileTarget),
        IDU_FT_SIZEOF(sdbFlushMgrStat, mFastStartLogFileTarget),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"REQ_JOB_COUNT",
        IDU_FT_OFFSETOF(sdbFlushMgrStat, mReqJobCount),
        IDU_FT_SIZEOF(sdbFlushMgrStat, mReqJobCount),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL
    },
    {
        NULL,
        0,
        0,
        IDU_FT_TYPE_CHAR,
        NULL,
        0, 0, NULL
    }
};

/***********************************************************************
 * Description :
 *  X$FLUSH_MGR을 만들기 위한 함수
 ***********************************************************************/
static IDE_RC buildRecordForFlushMgrStat(idvSQL              * /*aStatistics*/,
                                         void                *aHeader,
                                         void                */*aDumpObj*/,
                                         iduFixedTableMemory *aMemory)
{
    sdbFlushMgrStat sStat;

    sStat.mLowFlushLength         = sdbFlushMgr::getLowFlushLstLen4FF();
    sStat.mHighFlushLength        = sdbFlushMgr::getHighFlushLstLen4FF();
    sStat.mLowPrepareLength       = sdbFlushMgr::getLowPrepareLstLen4FF();
    sStat.mCheckpointFlushCount   = smuProperty::getCheckpointFlushCount();
    sStat.mFastStartIoTarget      = smuProperty::getFastStartIoTarget();
    sStat.mFastStartLogFileTarget = smuProperty::getFastStartLogFileTarget();
    sStat.mReqJobCount            = sdbFlushMgr::getReqJobCount();

    IDE_TEST(iduFixedTable::buildRecord(aHeader,
                                        aMemory,
                                        (void *)&sStat)
             != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

iduFixedTableDesc gFlushMgrStatTableDesc =
{
    (SChar *)"X$FLUSH_MGR",
    buildRecordForFlushMgrStat,
    gFlushMgrStatTableColDesc,
    IDU_STARTUP_META,
    0,
    0,
    IDU_FT_DESC_TRANS_NOT_USE,
    NULL
};

static iduFixedTableColDesc gFlusherStatColDesc[] =
{
    {
        (SChar*)"ID",
        IDU_FT_OFFSETOF(sdbFlusherStatData, mID),
        IDU_FT_SIZEOF(sdbFlusherStatData, mID),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"ALIVE",
        IDU_FT_OFFSETOF(sdbFlusherStatData, mAlive),
        IDU_FT_SIZEOF(sdbFlusherStatData, mAlive),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"CURRENT_JOB",
        IDU_FT_OFFSETOF(sdbFlusherStatData, mCurrentJob),
        IDU_FT_SIZEOF(sdbFlusherStatData, mCurrentJob),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"DOING_IO",
        IDU_FT_OFFSETOF(sdbFlusherStatData, mIOing),
        IDU_FT_SIZEOF(sdbFlusherStatData, mIOing),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"INIOB_COUNT",
        IDU_FT_OFFSETOF(sdbFlusherStatData, mINIOBCount),
        IDU_FT_SIZEOF(sdbFlusherStatData, mINIOBCount),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"REPLACE_FLUSH_JOBS",
        IDU_FT_OFFSETOF(sdbFlusherStatData, mReplacementFlushJobs),
        IDU_FT_SIZEOF(sdbFlusherStatData, mReplacementFlushJobs),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"REPLACE_FLUSH_PAGES",
        IDU_FT_OFFSETOF(sdbFlusherStatData, mReplacementFlushPages),
        IDU_FT_SIZEOF(sdbFlusherStatData, mReplacementFlushPages),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"REPLACE_SKIP_PAGES",
        IDU_FT_OFFSETOF(sdbFlusherStatData, mReplacementSkipPages),
        IDU_FT_SIZEOF(sdbFlusherStatData, mReplacementSkipPages),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"REPLACE_ADD_DELAYED_PAGES",
        IDU_FT_OFFSETOF(sdbFlusherStatData, mReplacementAddDelayedPages),
        IDU_FT_SIZEOF(sdbFlusherStatData, mReplacementAddDelayedPages),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"REPLACE_SKIP_DELAYED_PAGES",
        IDU_FT_OFFSETOF(sdbFlusherStatData, mReplacementSkipDelayedPages),
        IDU_FT_SIZEOF(sdbFlusherStatData, mReplacementSkipDelayedPages),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"REPLACE_OVERFLOW_DELAYED_PAGES",
        IDU_FT_OFFSETOF(sdbFlusherStatData, mReplacementOverflowDelayedPages),
        IDU_FT_SIZEOF(sdbFlusherStatData, mReplacementOverflowDelayedPages),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"CHECKPOINT_FLUSH_JOBS",
        IDU_FT_OFFSETOF(sdbFlusherStatData, mCheckpointFlushJobs),
        IDU_FT_SIZEOF(sdbFlusherStatData, mCheckpointFlushJobs),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"CHECKPOINT_FLUSH_PAGES",
        IDU_FT_OFFSETOF(sdbFlusherStatData, mCheckpointFlushPages),
        IDU_FT_SIZEOF(sdbFlusherStatData, mCheckpointFlushPages),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"CHECKPOINT_SKIP_PAGES",
        IDU_FT_OFFSETOF(sdbFlusherStatData, mCheckpointSkipPages),
        IDU_FT_SIZEOF(sdbFlusherStatData, mCheckpointSkipPages),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"OBJECT_FLUSH_JOBS",
        IDU_FT_OFFSETOF(sdbFlusherStatData, mObjectFlushJobs),
        IDU_FT_SIZEOF(sdbFlusherStatData, mObjectFlushJobs),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"OBJECT_FLUSH_PAGES",
        IDU_FT_OFFSETOF(sdbFlusherStatData, mObjectFlushPages),
        IDU_FT_SIZEOF(sdbFlusherStatData, mObjectFlushPages),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"OBJECT_SKIP_PAGES",
        IDU_FT_OFFSETOF(sdbFlusherStatData, mObjectSkipPages),
        IDU_FT_SIZEOF(sdbFlusherStatData, mObjectSkipPages),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"LAST_SLEEP_SEC",
        IDU_FT_OFFSETOF(sdbFlusherStatData, mLastSleepSec),
        IDU_FT_SIZEOF(sdbFlusherStatData, mLastSleepSec),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"TIMEOUT",
        IDU_FT_OFFSETOF(sdbFlusherStatData, mWakeUpsByTimeout),
        IDU_FT_SIZEOF(sdbFlusherStatData, mWakeUpsByTimeout),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"SIGNALED",
        IDU_FT_OFFSETOF(sdbFlusherStatData, mWakeUpsBySignal),
        IDU_FT_SIZEOF(sdbFlusherStatData, mWakeUpsBySignal),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"TOTAL_SLEEP_SEC",
        IDU_FT_OFFSETOF(sdbFlusherStatData, mTotalSleepSec),
        IDU_FT_SIZEOF(sdbFlusherStatData, mTotalSleepSec),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"TOTAL_FLUSH_PAGES",
        IDU_FT_OFFSETOF(sdbFlusherStatData, mTotalFlushPages),
        IDU_FT_SIZEOF(sdbFlusherStatData, mTotalFlushPages),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"TOTAL_FLUSH_DELAYED_PAGES",
        IDU_FT_OFFSETOF(sdbFlusherStatData, mTotalFlushDelayedPages),
        IDU_FT_SIZEOF(sdbFlusherStatData, mTotalFlushDelayedPages),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"TOTAL_LOG_SYNC_USEC",
        IDU_FT_OFFSETOF(sdbFlusherStatData, mTotalLogSyncTime),
        IDU_FT_SIZEOF(sdbFlusherStatData, mTotalLogSyncTime),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"TOTAL_DW_USEC",
        IDU_FT_OFFSETOF(sdbFlusherStatData, mTotalDWTime),
        IDU_FT_SIZEOF(sdbFlusherStatData, mTotalDWTime),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"TOTAL_WRITE_USEC",
        IDU_FT_OFFSETOF(sdbFlusherStatData, mTotalWriteTime),
        IDU_FT_SIZEOF(sdbFlusherStatData, mTotalWriteTime),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"TOTAL_SYNC_USEC",
        IDU_FT_OFFSETOF(sdbFlusherStatData, mTotalSyncTime),
        IDU_FT_SIZEOF(sdbFlusherStatData, mTotalSyncTime),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"TOTAL_FLUSH_TEMP_PAGES",
        IDU_FT_OFFSETOF(sdbFlusherStatData, mTotalFlushTempPages),
        IDU_FT_SIZEOF(sdbFlusherStatData, mTotalFlushTempPages),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"TOTAL_TEMP_WRITE_USEC",
        IDU_FT_OFFSETOF(sdbFlusherStatData, mTotalTempWriteTime),
        IDU_FT_SIZEOF(sdbFlusherStatData, mTotalTempWriteTime),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"TOTAL_CALC_CHECKSUM_USEC",
        IDU_FT_OFFSETOF(sdbFlusherStatData, mTotalCalcChecksumTime),
        IDU_FT_SIZEOF(sdbFlusherStatData, mTotalCalcChecksumTime),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0, NULL
    },

    {
        NULL,
        0,
        0,
        IDU_FT_TYPE_CHAR,
        NULL,
        0, 0, NULL // for internal use
    }
};

static IDE_RC buildRecordForFlusherStat(idvSQL              * /*aStatistics*/,
                                        void                *aHeader,
                                        void                */*aDumpObj*/,
                                        iduFixedTableMemory *aMemory)
{
    sdbFlusherStat  *sStat;
    UInt             i;

    for (i = 0; i < sdbFlushMgr::getFlusherCount(); i++)
    {
        sStat = sdbFlushMgr::getFlusherStat(i);

        IDE_TEST(iduFixedTable::buildRecord(aHeader,
                                            aMemory,
                                            (void *)sStat->getStatData())
                 != IDE_SUCCESS);
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

iduFixedTableDesc gBufferFlusherStatTableDesc =
{
    (SChar *)"X$FLUSHER",
    buildRecordForFlusherStat,
    gFlusherStatColDesc,
    IDU_STARTUP_META,
    0,
    0,
    IDU_FT_DESC_TRANS_NOT_USE,
    NULL
};

static UInt wasLocked(iduMutex *aMutex)
{
    idBool sLocked;

    IDE_ASSERT(aMutex->trylock(sLocked) == IDE_SUCCESS);
    if (sLocked == ID_TRUE)
    {
        IDE_ASSERT(aMutex->unlock() == IDE_SUCCESS);
    }

    return sLocked == ID_TRUE ? 0 : 1;
}

static SInt getLatchState(iduLatch *aLatch)
{
    iduLatchMode sLatchMode = aLatch->getLatchMode();

    if (sLatchMode == IDU_LATCH_NONE)
    {
        return 0;
    }
    else if (sLatchMode == IDU_LATCH_READ)
    {
        return 1;
    }
    return -1; // IDU_LATCH_WRITE
}

typedef struct sdbBCBStat
{
    UInt  mID;
    UInt  mState;
    ULong mFramePtr;
    UInt  mSpaceID;
    UInt  mPageID;
    UChar* mPageLatchPtr;
    UInt  mPageType;
    UInt  mRecvLSNFileNo;
    UInt  mRecvLSNOffset;
    UInt  mBCBListType;
    SInt  mCPListNo;
    UInt  mBCBMutexLocked;
    UInt  mReadIOMutexLocked;
    SInt  mPageLatchState;
    UInt  mTouchCount;
    UInt  mFixCount;
    SInt  mTouchedTimeSec;
    SInt  mReadTimeSec;
    UInt  mWriteCount;
    ULong mLatchReadGets;
    ULong mLatchWriteGets;
    ULong mLatchReadMisses;
    ULong mLatchWriteMisses;
    ULong mLatchSleeps;
    UInt  mHashBucketNo;
} sdbBCBStat;

static iduFixedTableColDesc gBufferBCBColDesc[] =
{
    {
        (SChar*)"ID",
        IDU_FT_OFFSETOF(sdbBCBStat, mID),
        IDU_FT_SIZEOF(sdbBCBStat, mID),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"STATE",
        IDU_FT_OFFSETOF(sdbBCBStat, mState),
        IDU_FT_SIZEOF(sdbBCBStat, mState),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"FRAME_PTR",
        IDU_FT_OFFSETOF(sdbBCBStat, mFramePtr),
        IDU_FT_SIZEOF(sdbBCBStat, mFramePtr),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"SPACE_ID",
        IDU_FT_OFFSETOF(sdbBCBStat, mSpaceID),
        IDU_FT_SIZEOF(sdbBCBStat, mSpaceID),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"PAGE_LATCH_PTR",
        IDU_FT_OFFSETOF(sdbBCBStat, mPageLatchPtr),
        16,
        IDU_FT_TYPE_CHAR | IDU_FT_TYPE_POINTER,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"PAGE_ID",
        IDU_FT_OFFSETOF(sdbBCBStat, mPageID),
        IDU_FT_SIZEOF(sdbBCBStat, mPageID),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"PAGE_TYPE",
        IDU_FT_OFFSETOF(sdbBCBStat, mPageType),
        IDU_FT_SIZEOF(sdbBCBStat, mPageType),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"RECV_LSN_FILENO",
        IDU_FT_OFFSETOF(sdbBCBStat, mRecvLSNFileNo),
        IDU_FT_SIZEOF(sdbBCBStat, mRecvLSNFileNo),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"RECV_LSN_OFFSET",
        IDU_FT_OFFSETOF(sdbBCBStat, mRecvLSNOffset),
        IDU_FT_SIZEOF(sdbBCBStat, mRecvLSNOffset),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"BCB_LIST_TYPE",
        IDU_FT_OFFSETOF(sdbBCBStat, mBCBListType),
        IDU_FT_SIZEOF(sdbBCBStat, mBCBListType),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"CPLIST_NO",
        IDU_FT_OFFSETOF(sdbBCBStat, mCPListNo),
        IDU_FT_SIZEOF(sdbBCBStat, mCPListNo),
        IDU_FT_TYPE_INTEGER,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"BCB_MUTEX",
        IDU_FT_OFFSETOF(sdbBCBStat, mBCBMutexLocked),
        IDU_FT_SIZEOF(sdbBCBStat, mBCBMutexLocked),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"READIO_MUTEX",
        IDU_FT_OFFSETOF(sdbBCBStat, mReadIOMutexLocked),
        IDU_FT_SIZEOF(sdbBCBStat, mReadIOMutexLocked),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"PAGE_LATCH",
        IDU_FT_OFFSETOF(sdbBCBStat, mPageLatchState),
        IDU_FT_SIZEOF(sdbBCBStat, mPageLatchState),
        IDU_FT_TYPE_INTEGER,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"TOUCH_COUNT",
        IDU_FT_OFFSETOF(sdbBCBStat, mTouchCount),
        IDU_FT_SIZEOF(sdbBCBStat, mTouchCount),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"FIX_COUNT",
        IDU_FT_OFFSETOF(sdbBCBStat, mFixCount),
        IDU_FT_SIZEOF(sdbBCBStat, mFixCount),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"TOUCHED_TIME_SEC",
        IDU_FT_OFFSETOF(sdbBCBStat, mTouchedTimeSec),
        IDU_FT_SIZEOF(sdbBCBStat, mTouchedTimeSec),
        IDU_FT_TYPE_INTEGER,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"READ_TIME_SEC",
        IDU_FT_OFFSETOF(sdbBCBStat, mReadTimeSec),
        IDU_FT_SIZEOF(sdbBCBStat, mReadTimeSec),
        IDU_FT_TYPE_INTEGER,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"WRITE_COUNT",
        IDU_FT_OFFSETOF(sdbBCBStat, mWriteCount),
        IDU_FT_SIZEOF(sdbBCBStat, mWriteCount),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"LATCH_READ_GETS",
        IDU_FT_OFFSETOF(sdbBCBStat, mLatchReadGets),
        IDU_FT_SIZEOF(sdbBCBStat, mLatchReadGets),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"LATCH_WRITE_GETS",
        IDU_FT_OFFSETOF(sdbBCBStat, mLatchWriteGets),
        IDU_FT_SIZEOF(sdbBCBStat, mLatchWriteGets),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"LATCH_READ_MISSES",
        IDU_FT_OFFSETOF(sdbBCBStat, mLatchReadMisses),
        IDU_FT_SIZEOF(sdbBCBStat, mLatchReadMisses),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"LATCH_WRITE_MISSES",
        IDU_FT_OFFSETOF(sdbBCBStat, mLatchWriteMisses),
        IDU_FT_SIZEOF(sdbBCBStat, mLatchWriteMisses),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"LATCH_SLEEPS",
        IDU_FT_OFFSETOF(sdbBCBStat, mLatchSleeps),
        IDU_FT_SIZEOF(sdbBCBStat, mLatchSleeps),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"HASH_BUCKET_NO",
        IDU_FT_OFFSETOF(sdbBCBStat, mHashBucketNo),
        IDU_FT_SIZEOF(sdbBCBStat, mHashBucketNo),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL
    },
    {
        NULL,
        0,
        0,
        IDU_FT_TYPE_CHAR,
        NULL,
        0, 0, NULL // for internal use
    }
};

typedef struct sdbBCBStatArg
{
    void                *mHeader;
    iduFixedTableMemory *mMemory;
} sdbBCBStatArg;

static IDE_RC buildRecordForOneBCB(sdbBCB *aBCB,
                                   void   *aObj)
{
    sdbBCBStatArg *sArg = (sdbBCBStatArg*)aObj;
    sdbBCBStat     sBCBStat;
    idvTime        sCurTime;
    UChar          sMutexPtrString[100];
    
    IDV_TIME_GET(&sCurTime);

    sBCBStat.mID                   = aBCB->mID;
    sBCBStat.mState                = (UInt)aBCB->mState;
    sBCBStat.mFramePtr             = (vULong)aBCB->mFrame;
    sBCBStat.mSpaceID              = (UInt)aBCB->mSpaceID;
    sBCBStat.mPageID               = (UInt)aBCB->mPageID;
    sBCBStat.mPageType             = aBCB->mPageType;
    sBCBStat.mRecvLSNFileNo        = aBCB->mRecoveryLSN.mFileNo;
    sBCBStat.mRecvLSNOffset        = aBCB->mRecoveryLSN.mOffset;
    sBCBStat.mBCBListType          = aBCB->mBCBListType;
    sBCBStat.mCPListNo             = aBCB->mCPListNo == ID_UINT_MAX ?
                                        -1 : (SInt)aBCB->mCPListNo;
    sBCBStat.mBCBMutexLocked       = wasLocked(&aBCB->mMutex);
    sBCBStat.mReadIOMutexLocked    = wasLocked(&aBCB->mReadIOMutex);
    sBCBStat.mPageLatchState       = getLatchState(&aBCB->mPageLatch);
    sBCBStat.mTouchCount           = aBCB->mTouchCnt;
    sBCBStat.mFixCount             = aBCB->mFixCnt;
    sBCBStat.mTouchedTimeSec       = IDV_TIME_IS_INIT(&aBCB->mLastTouchedTime) == ID_TRUE ?
                                        0 : IDV_TIME_DIFF_MICRO(&aBCB->mLastTouchedTime, &sCurTime)/1000000;
    sBCBStat.mReadTimeSec          = IDV_TIME_IS_INIT(&aBCB->mCreateOrReadTime) == ID_TRUE ?
                                        0 : IDV_TIME_DIFF_MICRO(&aBCB->mCreateOrReadTime, &sCurTime)/1000000;
    sBCBStat.mWriteCount           = aBCB->mWriteCount;
    sBCBStat.mLatchReadGets        = aBCB->mPageLatch.getReadCount();
    sBCBStat.mLatchWriteGets       = aBCB->mPageLatch.getWriteCount();
    sBCBStat.mLatchReadMisses      = aBCB->mPageLatch.getReadMisses();
    sBCBStat.mLatchWriteMisses     = aBCB->mPageLatch.getWriteMisses();
    sBCBStat.mLatchSleeps          = aBCB->mPageLatch.getSleepCount();
    sBCBStat.mHashBucketNo         = aBCB->mHashBucketNo;


    (void)idlOS::snprintf( (SChar*)sMutexPtrString,
                           ID_SIZEOF(sMutexPtrString),
                           "%"ID_XPOINTER_FMT,
                           aBCB->mPageLatch.getLatchCore() );

    sBCBStat.mPageLatchPtr = sMutexPtrString;

    IDE_TEST(iduFixedTable::buildRecord(sArg->mHeader,
                                        sArg->mMemory,
                                        (void *)&sBCBStat)
             != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC buildRecordForBCBStat(idvSQL              * /*aStatistics*/,
                                    void                *aHeader,
                                    void                */*aDumpObj*/,
                                    iduFixedTableMemory *aMemory)
{
    sdbBCBStatArg   sArg;
    sdbBufferArea  *sArea;

    sArg.mHeader = aHeader;
    sArg.mMemory = aMemory;

    sArea = sdbBufferMgr::getArea();
    IDE_TEST(sArea->applyFuncToEachBCBs( NULL,
                                         buildRecordForOneBCB,
                                         &sArg)
             != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

iduFixedTableDesc gBufferBCBStatTableDesc =
{
    (SChar *)"X$BCB",
    buildRecordForBCBStat,
    gBufferBCBColDesc,
    IDU_STARTUP_META,
    0,
    0,
    IDU_FT_DESC_TRANS_NOT_USE,
    NULL
};

/***********************************************
 *  frame dump를 위한 fixed table 정의
 ***********************************************/

#define SDB_FRAME_DUMP_UNIT_SIZE   SD_PAGE_SIZE

typedef struct sdbFrameDumpStat
{
    UInt    mBCBID;
    UInt    mSpaceID;
    UInt    mPageID;
    SChar   mFrameDump[SDB_FRAME_DUMP_UNIT_SIZE*2];
} sdbFrameDumpStat;

static iduFixedTableColDesc gBufferFrameDumpColDesc[] =
{
    {
        (SChar*)"ID",
        IDU_FT_OFFSETOF(sdbFrameDumpStat, mBCBID),
        IDU_FT_SIZEOF(sdbFrameDumpStat, mBCBID),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"SPACE_ID",
        IDU_FT_OFFSETOF(sdbFrameDumpStat, mSpaceID),
        IDU_FT_SIZEOF(sdbFrameDumpStat, mSpaceID),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"PAGE_ID",
        IDU_FT_OFFSETOF(sdbFrameDumpStat, mPageID),
        IDU_FT_SIZEOF(sdbFrameDumpStat, mPageID),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"FRAME_DUMP",
        IDU_FT_OFFSETOF(sdbFrameDumpStat, mFrameDump),
        IDU_FT_SIZEOF(sdbFrameDumpStat, mFrameDump),
        IDU_FT_TYPE_VARCHAR,
        NULL,
        0, 0, NULL
    },
    {
        NULL,
        0,
        0,
        IDU_FT_TYPE_CHAR,
        NULL,
        0, 0, NULL // for internal use
    }
};

typedef struct sdbFrameStatArg
{
    void                *mHeader;
    iduFixedTableMemory *mMemory;
    sdbFrameDumpStat     mStat;
} sdbFrameStatArg;

static SChar getHexChar(UInt a)
{
    if (a < 10)
    {
        return '0' + a;
    }
    else
    {
        IDE_DASSERT(a < 0x10);
        return 'A' + a - 10;
    }
}

static IDE_RC buildRecordForOneFrame(sdbBCB *aBCB,
                                     void   *aObj)
{
    UInt i;
    sdbFrameStatArg *sArg = (sdbFrameStatArg*)aObj;

    sArg->mStat.mBCBID   = aBCB->mID;
    sArg->mStat.mSpaceID = (UInt)aBCB->mSpaceID;
    sArg->mStat.mPageID  = (UInt)aBCB->mPageID;

    for (i = 0; i < SDB_FRAME_DUMP_UNIT_SIZE; i++)
    {
        sArg->mStat.mFrameDump[i<<1]     = getHexChar(aBCB->mFrame[i] >> 4);
        sArg->mStat.mFrameDump[(i<<1)+1] = getHexChar(aBCB->mFrame[i] & 0x0F);
    }
    IDE_TEST(iduFixedTable::buildRecord(sArg->mHeader,
                                        sArg->mMemory,
                                        (void *)&sArg->mStat)
             != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC buildRecordForFrameDumpStat(idvSQL              * /*aStatistics*/,
                                          void                * aHeader,
                                          void                * /*aDumpObj*/,
                                          iduFixedTableMemory * aMemory)
{
    sdbFrameStatArg   sArg;
    sdbBufferArea    *sArea;

    IDE_ERROR( aHeader != NULL );
    IDE_ERROR( aMemory != NULL );

    sArg.mHeader = aHeader;
    sArg.mMemory = aMemory;

    sArea = sdbBufferMgr::getArea();
    IDE_TEST(sArea->applyFuncToEachBCBs( NULL,
                                         buildRecordForOneFrame,
                                        &sArg)
             != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

iduFixedTableDesc gBufferFrameDumpStatTableDesc =
{
    (SChar *)"X$FRAME_DUMP",
    buildRecordForFrameDumpStat,
    gBufferFrameDumpColDesc,
    IDU_STARTUP_META,
    0,
    0,
    IDU_FT_DESC_TRANS_NOT_USE,
    NULL
};


