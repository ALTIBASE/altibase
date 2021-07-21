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

/***********************************************************************
 * PROJ-2102 The Fast Secondary Buffer 
 **********************************************************************/

#include <idl.h>

#include <sds.h>


/***********************************************************************
 * secondary buffer stat의 fixed table을 만드는 함수
 ***********************************************************************/
static iduFixedTableColDesc gSecondatyBufferStatTableColDesc[] =
{
    {
        (SChar*)"BUFFER_PAGES",
        IDU_FT_OFFSETOF(sdsBufferMgrStatData, mBufferPages),
        IDU_FT_SIZEOF(sdsBufferMgrStatData, mBufferPages),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"HASH_BUCKET_COUNT",
        IDU_FT_OFFSETOF(sdsBufferMgrStatData, mHashBucketCount),
        IDU_FT_SIZEOF(sdsBufferMgrStatData, mHashBucketCount),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"HASH_CHAIN_LATCH_COUNT",
        IDU_FT_OFFSETOF(sdsBufferMgrStatData, mHashChainLatchCount),
        IDU_FT_SIZEOF(sdsBufferMgrStatData, mHashChainLatchCount),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"CHECKPOINT_LIST_COUNT",
        IDU_FT_OFFSETOF(sdsBufferMgrStatData, mCheckpointListCount),
        IDU_FT_SIZEOF(sdsBufferMgrStatData, mCheckpointListCount),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"HASH_PAGES",
        IDU_FT_OFFSETOF(sdsBufferMgrStatData, mHashPages),
        IDU_FT_SIZEOF(sdsBufferMgrStatData, mHashPages),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"MOVEDOWN_ING_PAGES",
        IDU_FT_OFFSETOF(sdsBufferMgrStatData, mMovedoweIngPages),
        IDU_FT_SIZEOF(sdsBufferMgrStatData, mMovedoweIngPages),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"MOVEDOWN_DONE_PAGES",
        IDU_FT_OFFSETOF(sdsBufferMgrStatData, mMovedownDonePages),
        IDU_FT_SIZEOF(sdsBufferMgrStatData, mMovedownDonePages),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"FLUSH_ING_PAGES",
        IDU_FT_OFFSETOF(sdsBufferMgrStatData, mFlushIngPages),
        IDU_FT_SIZEOF(sdsBufferMgrStatData, mFlushIngPages),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"FLUSH_DONE_PAGES",
        IDU_FT_OFFSETOF(sdsBufferMgrStatData, mFlushDonePages),
        IDU_FT_SIZEOF(sdsBufferMgrStatData, mFlushDonePages),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"CHECKPOINT_LIST_PAGES",
        IDU_FT_OFFSETOF(sdsBufferMgrStatData, mCheckpointListPages),
        IDU_FT_SIZEOF(sdsBufferMgrStatData, mCheckpointListPages),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"GET_PAGES",
        IDU_FT_OFFSETOF(sdsBufferMgrStatData, mGetPages),
        IDU_FT_SIZEOF(sdsBufferMgrStatData, mGetPages),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"READ_PAGES",
        IDU_FT_OFFSETOF(sdsBufferMgrStatData, mReadPages),
        IDU_FT_SIZEOF(sdsBufferMgrStatData, mReadPages),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0, NULL
    },
   {
        (SChar*)"WRITE_PAGES",
        IDU_FT_OFFSETOF(sdsBufferMgrStatData, mWritePages),
        IDU_FT_SIZEOF(sdsBufferMgrStatData, mWritePages),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"HIT_RATIO",
        IDU_FT_OFFSETOF(sdsBufferMgrStatData, mHitRatio),
        IDU_FT_SIZEOF(sdsBufferMgrStatData, mHitRatio),
        IDU_FT_TYPE_DOUBLE,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"SINGLE_PAGE_READ_USEC",
        IDU_FT_OFFSETOF(sdsBufferMgrStatData, mReadTime),
        IDU_FT_SIZEOF(sdsBufferMgrStatData, mReadTime),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"SINGLE_PAGE_WRITE_USEC",
        IDU_FT_OFFSETOF(sdsBufferMgrStatData, mWriteTime),
        IDU_FT_SIZEOF(sdsBufferMgrStatData, mWriteTime),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"MPR_READ_USEC",
        IDU_FT_OFFSETOF(sdsBufferMgrStatData, mMPRReadTime),
        IDU_FT_SIZEOF(sdsBufferMgrStatData, mMPRReadTime),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"MPR_READ_PAGES",
        IDU_FT_OFFSETOF(sdsBufferMgrStatData, mMPRReadPages),
        IDU_FT_SIZEOF(sdsBufferMgrStatData, mMPRReadPages),
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

IDE_RC buildRecordForSBufferStat( idvSQL              * /*aStatistics*/,
                                  void                * aHeader,
                                  void                * /*aDumpObj*/,
                                  iduFixedTableMemory * aMemory)
{
    sdsBufferMgrStat *sStat;

    IDE_TEST_CONT( sdsBufferMgr::isServiceable() != ID_TRUE, SKIP );

    sStat = sdsBufferMgr::getSBufferStat();

    sStat->buildRecord();
    IDE_TEST( iduFixedTable::buildRecord( aHeader,
                                          aMemory,
                                          (void*)sStat->getStatData() )
              != IDE_SUCCESS);

    
    IDE_EXCEPTION_CONT( SKIP );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

iduFixedTableDesc gSBufferStatTableDesc =
{
    (SChar *)"X$SBUFFER_STAT",
    buildRecordForSBufferStat,
    gSecondatyBufferStatTableColDesc,
    IDU_STARTUP_META,
    0,
    0,
    IDU_FT_DESC_TRANS_NOT_USE,
    NULL
};

/***********************************************
 *  BCB를 위한 fixed table 정의
 ***********************************************/
static iduFixedTableColDesc gBufferSBCBColDesc[] =
{
    {
        (SChar*)"ID",
        IDU_FT_OFFSETOF(sdsBCBStat, mSBCBID),
        IDU_FT_SIZEOF(sdsBCBStat, mSBCBID),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"SPACE_ID",
        IDU_FT_OFFSETOF(sdsBCBStat, mSpaceID),
        IDU_FT_SIZEOF(sdsBCBStat, mSpaceID),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"PAGE_ID",
        IDU_FT_OFFSETOF(sdsBCBStat, mPageID),
        IDU_FT_SIZEOF(sdsBCBStat, mPageID),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"STATE",
        IDU_FT_OFFSETOF(sdsBCBStat, mState),
        IDU_FT_SIZEOF(sdsBCBStat, mState),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"HASH_BUCKET_NO",
        IDU_FT_OFFSETOF(sdsBCBStat, mHashBucketNo),
        IDU_FT_SIZEOF(sdsBCBStat, mHashBucketNo),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"CPLIST_NO",
        IDU_FT_OFFSETOF(sdsBCBStat, mCPListNo),
        IDU_FT_SIZEOF(sdsBCBStat, mCPListNo),
        IDU_FT_TYPE_INTEGER,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"BCB_MUTEX",
        IDU_FT_OFFSETOF(sdsBCBStat, mBCBMutexLocked),
        IDU_FT_SIZEOF(sdsBCBStat, mBCBMutexLocked),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"RECV_LSN_FILENO",
        IDU_FT_OFFSETOF(sdsBCBStat, mRecvLSNFileNo),
        IDU_FT_SIZEOF(sdsBCBStat, mRecvLSNFileNo),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"RECV_LSN_OFFSET",
        IDU_FT_OFFSETOF(sdsBCBStat, mRecvLSNOffset),
        IDU_FT_SIZEOF(sdsBCBStat, mRecvLSNOffset),
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

static UInt isLocked( iduMutex *aMutex )
{
    idBool sLocked;

    IDE_ASSERT( aMutex->trylock( sLocked ) == IDE_SUCCESS );

    if( sLocked == ID_TRUE )
    {
        IDE_ASSERT( aMutex->unlock() == IDE_SUCCESS );
    }

    return sLocked == ID_TRUE ? 0 : 1;
}

static IDE_RC buildRecordForOneSBCB( sdsBCB * aSBCB,
                                     void   * aObj )
{
    sdsBCBStatArg *sArg = (sdsBCBStatArg*)aObj;
    sdsBCBStat     sBCBStat;

    sBCBStat.mSBCBID               = aSBCB->mSBCBID;
    sBCBStat.mSpaceID              = (UInt)aSBCB->mSpaceID;
    sBCBStat.mPageID               = (UInt)aSBCB->mPageID;
    sBCBStat.mHashBucketNo         = aSBCB->mHashBucketNo;
    sBCBStat.mCPListNo             = aSBCB->mCPListNo == ID_UINT_MAX ?
                                        -1 : (SInt)aSBCB->mCPListNo;
    sBCBStat.mRecvLSNFileNo        = aSBCB->mRecoveryLSN.mFileNo;
    sBCBStat.mRecvLSNOffset        = aSBCB->mRecoveryLSN.mOffset;

    sBCBStat.mState                = (UInt)aSBCB->mState;
    sBCBStat.mBCBMutexLocked       = isLocked( &aSBCB->mBCBMutex );
    sBCBStat.mPageLSNFileNo        = aSBCB->mPageLSN.mFileNo;
    sBCBStat.mPageLSNOffset        = aSBCB->mPageLSN.mOffset;

    IDE_TEST( iduFixedTable::buildRecord( sArg->mHeader,
                                          sArg->mMemory,
                                          (void *)&sBCBStat )
              != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC buildRecordForSBCBStat( idvSQL              * /*aStatistics*/,
                                      void                *aHeader,
                                      void                */*aDumpObj*/,
                                      iduFixedTableMemory *aMemory )
{
    sdsBCBStatArg   sArg;
    sdsBufferArea  *sArea;

    IDE_TEST_CONT( sdsBufferMgr::isServiceable() != ID_TRUE, SKIP );

    sArea = sdsBufferMgr::getSBufferArea();
    sArg.mHeader = aHeader;
    sArg.mMemory = aMemory;

    IDE_TEST( sArea->applyFuncToEachBCBs( buildRecordForOneSBCB,
                                          &sArg )
              != IDE_SUCCESS);

    IDE_EXCEPTION_CONT( SKIP );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

iduFixedTableDesc gSBufferBCBStatTableDesc =
{
    (SChar *)"X$SBCB",
    buildRecordForSBCBStat,
    gBufferSBCBColDesc,
    IDU_STARTUP_META,
    0,
    0,
    IDU_FT_DESC_TRANS_NOT_USE,
    NULL
};

/***********************************************************************
 *  Flush Mgr 를 만들기 위한 함수
 ***********************************************************************/
static iduFixedTableColDesc gSBufferFlushMgrStatTableColDesc[] =
{
    {
        (SChar*)"FLUSHER_COUNT",
        IDU_FT_OFFSETOF(sdsFlushMgrStat, mFluserCount),
        IDU_FT_SIZEOF(sdsFlushMgrStat, mFluserCount),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"CHECKPOINT_LIST_COUNT",
        IDU_FT_OFFSETOF(sdsFlushMgrStat, mCPList),
        IDU_FT_SIZEOF(sdsFlushMgrStat, mCPList),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"REQ_JOB_COUNT",
        IDU_FT_OFFSETOF(sdsFlushMgrStat, mReqJobCount),
        IDU_FT_SIZEOF(sdsFlushMgrStat, mReqJobCount),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"REPLACE_PAGES",
        IDU_FT_OFFSETOF(sdsFlushMgrStat, mReqJobCount),
        IDU_FT_SIZEOF(sdsFlushMgrStat, mReqJobCount),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"CHECKPOINT_PAGES",
        IDU_FT_OFFSETOF(sdsFlushMgrStat, mReqJobCount),
        IDU_FT_SIZEOF(sdsFlushMgrStat, mReqJobCount),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"MIN_BCB_ID",
        IDU_FT_OFFSETOF(sdsFlushMgrStat, mMinBCBID),
        IDU_FT_SIZEOF(sdsFlushMgrStat, mMinBCBID),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"MIN_SPACEID",
        IDU_FT_OFFSETOF(sdsFlushMgrStat, mMinBCBSpaceID),
        IDU_FT_SIZEOF(sdsFlushMgrStat, mMinBCBSpaceID),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"MIN_PAGEID",
        IDU_FT_OFFSETOF(sdsFlushMgrStat, mMinBCBPageID),
        IDU_FT_SIZEOF(sdsFlushMgrStat, mMinBCBPageID),
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

static IDE_RC buildRecordForSBufferFlushMgrStat( 
    idvSQL              * /*aStatistics*/,
    void                * aHeader,
    void                * /*aDumpObj*/,
    iduFixedTableMemory * aMemory )
{
    sdsFlushMgrStat     sStat;
    sdbCPListSet      * sCPListSet;
    sdsBufferArea     * sBufferArea;
    sdsBCB            * sMinBCB;
    UInt                sMovedownIngPages  = 0;
    UInt                sMovedownDonePages = 0;
    
    IDE_TEST_CONT( sdsBufferMgr::isServiceable() != ID_TRUE, SKIP );

    sCPListSet  = sdsBufferMgr::getCPListSet();
    sBufferArea = sdsBufferMgr::getSBufferArea(); 

    sStat.mFluserCount = sdsFlushMgr::getFlusherCount();
    sStat.mCPList      = sCPListSet->getListCount();
    sStat.mReqJobCount = sdsFlushMgr::getReqJobCount();

    sBufferArea->getExtentCnt( &sMovedownIngPages,
                               &sMovedownDonePages,
                               NULL, /* aFlushIng */
                               NULL  /* aFlushDone */ );
                                    
    sStat.mReplacementFlushPages = sMovedownIngPages + sMovedownDonePages; 
    sStat.mCheckpointFlushPages  = sCPListSet->getTotalBCBCnt();

    sMinBCB = (sdsBCB*)sCPListSet->getMin();

    if( sMinBCB != NULL )
    {
        sStat.mMinBCBID      = sMinBCB->mSBCBID;
        sStat.mMinBCBSpaceID = sMinBCB->mSpaceID;
        sStat.mMinBCBPageID  = sMinBCB->mPageID;
    }
    else
    {
        sStat.mMinBCBID      = ID_UINT_MAX;
        sStat.mMinBCBSpaceID = ID_UINT_MAX;
        sStat.mMinBCBPageID  = ID_UINT_MAX;
    }

    IDE_TEST( iduFixedTable::buildRecord( aHeader,
                                          aMemory,
                                          (void *)&sStat )
             != IDE_SUCCESS);

    IDE_EXCEPTION_CONT( SKIP );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

iduFixedTableDesc gSBufferFlushMgrStatTableDesc =
{
    (SChar *)"X$SBUFFER_FLUSH_MGR",
    buildRecordForSBufferFlushMgrStat,
    gSBufferFlushMgrStatTableColDesc,
    IDU_STARTUP_META,
    0,
    0,
    IDU_FT_DESC_TRANS_NOT_USE,
    NULL
};

/***********************************************
 *  Fluser stat 를 위한 fixed table 정의
 ***********************************************/
static iduFixedTableColDesc gSBufferFlusherStatColDesc[] =
{
    {
        (SChar*)"ID",
        IDU_FT_OFFSETOF(sdsFlusherStatData, mID),
        IDU_FT_SIZEOF(sdsFlusherStatData, mID),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"ALIVE",
        IDU_FT_OFFSETOF(sdsFlusherStatData, mAlive),
        IDU_FT_SIZEOF(sdsFlusherStatData, mAlive),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"CURRENT_JOB",
        IDU_FT_OFFSETOF(sdsFlusherStatData, mCurrentJob),
        IDU_FT_SIZEOF(sdsFlusherStatData, mCurrentJob),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"DOING_IO",
        IDU_FT_OFFSETOF(sdsFlusherStatData, mIOing),
        IDU_FT_SIZEOF(sdsFlusherStatData, mIOing),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"INIOB_COUNT",
        IDU_FT_OFFSETOF(sdsFlusherStatData, mINIOBCount),
        IDU_FT_SIZEOF(sdsFlusherStatData, mINIOBCount),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"REPLACE_FLUSH_JOBS",
        IDU_FT_OFFSETOF(sdsFlusherStatData, mReplacementFlushJobs),
        IDU_FT_SIZEOF(sdsFlusherStatData, mReplacementFlushJobs),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"REPLACE_FLUSH_PAGES",
        IDU_FT_OFFSETOF(sdsFlusherStatData, mReplacementFlushPages),
        IDU_FT_SIZEOF(sdsFlusherStatData, mReplacementFlushPages),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"REPLACE_SKIP_PAGES",
        IDU_FT_OFFSETOF(sdsFlusherStatData, mReplacementSkipPages),
        IDU_FT_SIZEOF(sdsFlusherStatData, mReplacementSkipPages),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"CHECKPOINT_FLUSH_JOBS",
        IDU_FT_OFFSETOF(sdsFlusherStatData, mCheckpointFlushJobs),
        IDU_FT_SIZEOF(sdsFlusherStatData, mCheckpointFlushJobs),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"CHECKPOINT_FLUSH_PAGES",
        IDU_FT_OFFSETOF(sdsFlusherStatData, mCheckpointFlushPages),
        IDU_FT_SIZEOF(sdsFlusherStatData, mCheckpointFlushPages),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"CHECKPOINT_SKIP_PAGES",
        IDU_FT_OFFSETOF(sdsFlusherStatData, mCheckpointSkipPages),
        IDU_FT_SIZEOF(sdsFlusherStatData, mCheckpointSkipPages),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"OBJECT_FLUSH_JOBS",
        IDU_FT_OFFSETOF(sdsFlusherStatData, mObjectFlushJobs),
        IDU_FT_SIZEOF(sdsFlusherStatData, mObjectFlushJobs),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"OBJECT_FLUSH_PAGES",
        IDU_FT_OFFSETOF(sdsFlusherStatData, mObjectFlushPages),
        IDU_FT_SIZEOF(sdsFlusherStatData, mObjectFlushPages),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"OBJECT_SKIP_PAGES",
        IDU_FT_OFFSETOF(sdsFlusherStatData, mObjectSkipPages),
        IDU_FT_SIZEOF(sdsFlusherStatData, mObjectSkipPages),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"LAST_SLEEP_SEC",
        IDU_FT_OFFSETOF(sdsFlusherStatData, mLastSleepSec),
        IDU_FT_SIZEOF(sdsFlusherStatData, mLastSleepSec),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"TIMEOUT",
        IDU_FT_OFFSETOF(sdsFlusherStatData, mWakeUpsByTimeout),
        IDU_FT_SIZEOF(sdsFlusherStatData, mWakeUpsByTimeout),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"SIGNALED",
        IDU_FT_OFFSETOF(sdsFlusherStatData, mWakeUpsBySignal),
        IDU_FT_SIZEOF(sdsFlusherStatData, mWakeUpsBySignal),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"TOTAL_SLEEP_SEC",
        IDU_FT_OFFSETOF(sdsFlusherStatData, mTotalSleepSec),
        IDU_FT_SIZEOF(sdsFlusherStatData, mTotalSleepSec),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"TOTAL_FLUSH_PAGES",
        IDU_FT_OFFSETOF(sdsFlusherStatData, mTotalFlushPages),
        IDU_FT_SIZEOF(sdsFlusherStatData, mTotalFlushPages),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"TOTAL_DW_USEC",
        IDU_FT_OFFSETOF(sdsFlusherStatData, mTotalDWTime),
        IDU_FT_SIZEOF(sdsFlusherStatData, mTotalDWTime),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"TOTAL_WRITE_USEC",
        IDU_FT_OFFSETOF(sdsFlusherStatData, mTotalWriteTime),
        IDU_FT_SIZEOF(sdsFlusherStatData, mTotalWriteTime),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"TOTAL_SYNC_USEC",
        IDU_FT_OFFSETOF(sdsFlusherStatData, mTotalSyncTime),
        IDU_FT_SIZEOF(sdsFlusherStatData, mTotalSyncTime),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"TOTAL_FLUSH_TEMP_PAGES",
        IDU_FT_OFFSETOF(sdsFlusherStatData, mTotalFlushTempPages),
        IDU_FT_SIZEOF(sdsFlusherStatData, mTotalFlushTempPages),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0, NULL
    },
    {
        (SChar*)"TOTAL_TEMP_WRITE_USEC",
        IDU_FT_OFFSETOF(sdsFlusherStatData, mTotalTempWriteTime),
        IDU_FT_SIZEOF(sdsFlusherStatData, mTotalTempWriteTime),
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

static IDE_RC buildRecordForSecondaryFlusherStat( idvSQL              * /*aStatistics*/,
                                                  void                * aHeader,
                                                  void                * /*aDumpObj*/,
                                                  iduFixedTableMemory * aMemory )
{
    sdsFlusherStat  *sStat;
    UInt             i;

    IDE_TEST_CONT( sdsBufferMgr::isServiceable() != ID_TRUE, SKIP );

    for( i = 0; i < sdsFlushMgr::getFlusherCount(); i++ )
    {
        sStat = sdsFlushMgr::getFlusherStat( i );

        IDE_TEST( iduFixedTable::buildRecord( aHeader,
                                              aMemory,
                                              (void*)sStat->getStatData() )
                 != IDE_SUCCESS);
    }

    IDE_EXCEPTION_CONT( SKIP );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

iduFixedTableDesc gSBufferFlusherStatTableDesc =
{
    (SChar *)"X$SBUFFER_FLUSHER",
    buildRecordForSecondaryFlusherStat,
    gSBufferFlusherStatColDesc,
    IDU_STARTUP_META,
    0,
    0,
    IDU_FT_DESC_TRANS_NOT_USE,
    NULL
};

/***********************************************************************
 *  Secondary File node 를 위한 fixed table 정의
 ***********************************************************************/
static iduFixedTableColDesc gSBufferFileTableColDesc[] =
{
    {
        (SChar*)"NAME",
        offsetof(sdsFileNodeStat, mName),
        256,
        IDU_FT_TYPE_VARCHAR | IDU_FT_TYPE_POINTER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"REDO_LSN_FILENO",
        offsetof(sdsFileNodeStat, mRedoLSN)    +
        offsetof(smLSN, mFileNo),
        IDU_FT_SIZEOF(smLSN, mFileNo),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"REDO_LSN_OFFSET",
        offsetof(sdsFileNodeStat, mRedoLSN)    +
        offsetof(smLSN, mOffset),
        IDU_FT_SIZEOF(smLSN, mOffset),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"CREATE_LSN_FILENO",
        offsetof(sdsFileNodeStat, mCreateLSN)    +
        offsetof(smLSN, mFileNo),
        IDU_FT_SIZEOF(smLSN, mFileNo),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"CREATE_LSN_OFFSET",
        offsetof(sdsFileNodeStat, mCreateLSN)    +
        offsetof(smLSN, mOffset),
        IDU_FT_SIZEOF(smLSN, mOffset),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"SM_VERSION",
        offsetof(sdsFileNodeStat, mSmVersion),
        IDU_FT_SIZEOF(sdsFileNodeStat, mSmVersion),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"PAGE_COUNT",
        offsetof(sdsFileNodeStat, mFilePages),
        IDU_FT_SIZEOF(sdsFileNodeStat, mFilePages),
        IDU_FT_TYPE_BIGINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"OPENED",
        offsetof(sdsFileNodeStat, mIsOpened), // BUGBUG : 4byte인지 확인
        IDU_FT_SIZEOF(sdsFileNodeStat, mIsOpened), // BUGBUG : 4byte인지 확,
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"STATE",
        offsetof(sdsFileNodeStat, mState), // BUGBUG : 4byte인지 확인
        IDU_FT_SIZEOF(sdsFileNodeStat, mState), // BUGBUG : 4byte인지 확,
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        NULL,
        0,
        0,
        IDU_FT_TYPE_CHAR,
        NULL,
        0, 0,NULL // for internal use
    }
};

IDE_RC buildRecordForSBufferFiles( idvSQL              * /*aStatistics*/,
                                   void                * aHeader,
                                   void                * /* aDumpObj */,
                                   iduFixedTableMemory * aMemory )
{
    sdsFileNode       * sFileNode;
    sdsFileNodeStat     sFileStat;

    IDE_TEST_CONT( sdsBufferMgr::isServiceable() != ID_TRUE, SKIP );

    sdsBufferMgr::getFileNode( &sFileNode );

    if( sFileNode == NULL )
    {
        IDE_RAISE( SKIP );
    }

    sFileStat.mName = sFileNode->mName;

    SM_GET_LSN( sFileStat.mRedoLSN, sFileNode->mFileHdr.mRedoLSN );
    SM_GET_LSN( sFileStat.mCreateLSN, sFileNode->mFileHdr.mCreateLSN );

    sFileStat.mSmVersion = sFileNode->mFileHdr.mSmVersion;

    sFileStat.mFilePages = sFileNode->mPageCnt;
    sFileStat.mIsOpened  = sFileNode->mIsOpened;
    sFileStat.mState     = sFileNode->mState;

    IDE_TEST( iduFixedTable::buildRecord( aHeader,
                                          aMemory,
                                          (void *)&sFileStat)
              != IDE_SUCCESS);

    IDE_EXCEPTION_CONT( SKIP ) ;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

iduFixedTableDesc gSBufferFileTableDesc =
{
    (SChar *)"X$SBUFFER_FILE",
    buildRecordForSBufferFiles,
    gSBufferFileTableColDesc,
    IDU_STARTUP_META,
    0,
    0,
    IDU_FT_DESC_TRANS_NOT_USE,
    NULL
};

