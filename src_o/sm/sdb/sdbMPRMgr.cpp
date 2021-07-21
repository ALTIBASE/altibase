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
#include <sdbReq.h>
#include <sdbMPRMgr.h>


/* BUG-33762 The MPR should uses the mempool instead allocBuff4DirectIO
 * function. */
iduMemPool sdbMPRMgr::mReadIOBPool;

IDE_RC sdbMPRMgr::initializeStatic()
{
    IDE_TEST(mReadIOBPool.initialize(
            IDU_MEM_SM_SDB,
            (SChar*)"SDB_MPR_IOB_POOL",
            ID_SCALABILITY_SYS,                    /* Multi Pooling */
            SDB_MAX_MPR_PAGE_COUNT * SD_PAGE_SIZE, /* Block Size*/
            1,                                     /* BlockCntInChunk*/
            ID_UINT_MAX,                           /* chunk limit */
            ID_TRUE,                               /* use mutex   */
            SD_PAGE_SIZE,                          /* align byte  */
            ID_FALSE,							   /* ForcePooling */
            ID_TRUE,							   /* GarbageCollection */
            ID_TRUE,                               /* HWCacheLine */
            IDU_MEMPOOL_TYPE_LEGACY                /* mempool type */) 
           != IDE_SUCCESS);			

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdbMPRMgr::destroyStatic()
{
    IDE_TEST( mReadIOBPool.destroy() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/****************************************************************
 * Description :
 * Multi Page Read Manager를 초기화합니다. MPR을 수행하려는 Segment
 * 의 Extent 페이지갯수가 DB_FILE_MULTIPAGE_READ_COUNT으로 Align되어
 * 있지않으면 SBR( Single Block Read )을 하게 됩니다. 만약 크다면
 * DB_FILE_MULTIPAGE_READ_COUNT값으로 MPR을 하게 됩니다.
 *
 * aStatistics - [IN] 통계 정보
 * aSpaceID    - [IN] SpaceID
 * aSegPID     - [IN] Segment PID
 * aFilter     - [IN] 읽을 Extent인지 판단하기 위한 Filter
 *                    Null일 경우 다 읽음
 ****************************************************************/
IDE_RC sdbMPRMgr::initialize( idvSQL           * aStatistics,
                              scSpaceID          aSpaceID,
                              sdpSegHandle     * aSegHandle,
                              sdbMPRFilterFunc   aFilter )
{
    UInt sDBFileMRCnt;
    UInt sPageCntInExt;
    UInt sSmallTblThreshold;

    IDE_ASSERT( aSegHandle != NULL );

    mBufferPool   = sdbBufferMgr::getBufferPool();
    mStatistics   = aStatistics;
    mSpaceID      = aSpaceID;
    mSegHandle    = aSegHandle;
    mCurExtRID    = SD_NULL_RID;
    mCurPageID    = SD_NULL_PID;
    mIsFetchPage  = ID_FALSE;
    mIsCachePage  = ID_FALSE;
    mFilter       = aFilter;

    mLstAllocPID       = SD_NULL_PID;
    mLstAllocSeqNo     = 0;
    mFoundLstAllocPage = ID_FALSE;

    mCurExtSeq   = (ULong)-1;

    mSegMgmtOP  = smLayerCallback::getSegMgmtOp( aSpaceID );

    IDE_ERROR( mSegMgmtOP != NULL );

    /* Segment Info */
    IDE_TEST( mSegMgmtOP->mGetSegInfo( aStatistics,
                                       aSpaceID,
                                       aSegHandle->mSegPID,
                                       NULL, /* aTableHeader */
                                       &mSegInfo )
              != IDE_SUCCESS );

    /* Segment Cache Info */
    IDE_TEST( mSegMgmtOP->mGetSegCacheInfo( aStatistics,
                                            aSegHandle,
                                            &mSegCacheInfo )
              != IDE_SUCCESS );

    sDBFileMRCnt  = smuProperty::getDBFileMutiReadCnt();
    sPageCntInExt = mSegInfo.mPageCntInExt;

    /* Segment의 Extent내의 페이지 갯수가 DB_FILE_MULTIPAGE_READ_COUNT
     * 로 Align이 되지 않거나 크다면 Single Blcok Read를 한다. */
    if( sDBFileMRCnt >= sPageCntInExt )
    {
        mMPRCnt = sPageCntInExt;
    }
    else
    {
        mMPRCnt = sDBFileMRCnt;

        if( sPageCntInExt % mMPRCnt != 0 )
        {
            mMPRCnt = 1;
        }
    }

    /* Buffer Size가 너무작으면 SPR을 한다. */
    if( mBufferPool->getPoolSize() < mMPRCnt * 100 )
    {
        mMPRCnt = 1;
    }

    /* 모든 Segment는 생성시 한개이상의 Extent를 가진다. */
    IDE_ASSERT( mSegInfo.mFstExtRID != SD_NULL_RID );

    /* BUG-33720 */
    /* Small Table의 경우엔는 읽은 페이지들을 LRU List의 앞에 넣는다. */
    sSmallTblThreshold = smuProperty::getSmallTblThreshold();
    if( mSegInfo.mFmtPageCnt <= sSmallTblThreshold )
    {
        mIsCachePage = ID_TRUE;
    }
    else
    {
        mIsCachePage = ID_FALSE;

        /* sSmallTblThreshold가 ID_UINT_MAX일경우 MPR로 읽은 페이지들을 무조건
         * 버퍼에 caching한다.
         */
        if( sSmallTblThreshold == ID_UINT_MAX )
        {
            mIsCachePage = ID_TRUE;
        }
    }

    IDE_TEST( initMPRKey() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/****************************************************************
 * Description :
 *  MPR에 사용하는 MPRKey를 정리한다.
 ****************************************************************/
IDE_RC sdbMPRMgr::destroy()
{
    IDE_TEST( destMPRKey() != IDE_SUCCESS );

    if ( (mFoundLstAllocPage == ID_TRUE) && (mLstAllocPID != SD_NULL_PID) )
    {
        IDE_ASSERT( mSegHandle != NULL );

        IDE_TEST( mSegMgmtOP->mSetLstAllocPage( mStatistics,
                                                mSegHandle,
                                                mLstAllocPID,
                                                mLstAllocSeqNo )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/****************************************************************
 * Description :
 *  MPR에 사용하는 MPRKey와 IO Buffer를 할당한다.
 ****************************************************************/
IDE_RC sdbMPRMgr::initMPRKey()
{
    /* sdbMPRMgr_initMPRKey_alloc_ReadIOB.tc */
    IDU_FIT_POINT("sdbMPRMgr::initMPRKey::alloc::ReadIOB");
    IDE_TEST( mReadIOBPool.alloc( (void**)&mMPRKey.mReadIOB ) != IDE_SUCCESS );

    mMPRKey.initFreeBCBList();

    mMPRKey.mState = SDB_MPR_KEY_CLEAN;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/****************************************************************
 * Description :
 *  MPR에 사용하는 MPRKey를 destroy한다. 할당된 IO Buffer를
 *  Free한다.
 ****************************************************************/
IDE_RC sdbMPRMgr::destMPRKey()
{
    sdbBCB *sFirstBCB;
    sdbBCB *sLastBCB;
    UInt    sBCBCount;

    if( mIsFetchPage == ID_TRUE )
    {
        mIsFetchPage = ID_FALSE;
        mBufferPool->cleanUpKey( mStatistics, &mMPRKey, mIsCachePage );
    }

    /* BUG-22294: Buffer Miss환경에서 Hang인 것처럼 보입니다.
     *
     * removeAllBCB하기전에 cleanUpKey를 먼저해야함. 먼저해야지만
     * FreeLst에 반환되고 removeAllBCB시에는 FreeList갯수를 가져와서
     * Free를 하기때문입니다.
     * */
    mMPRKey.removeAllBCB( &sFirstBCB, &sLastBCB, &sBCBCount );

    if (sBCBCount > 0)
    {
        mBufferPool->addBCBList2PrepareLst( mStatistics,
                                            sFirstBCB,
                                            sLastBCB,
                                            sBCBCount );
    }

    IDE_TEST( mReadIOBPool.memfree( mMPRKey.mReadIOB ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/****************************************************************
 * Description :
 * 첫번째 페이지 바로전 위치로 MPRKey의 Index를 위치시킨다.
 * 이 함수후에 getNxtPage를 하게되면 첫번째 페이지가 Get하게 된다.
 ****************************************************************/
IDE_RC sdbMPRMgr::beforeFst()
{
    UInt sMPRCnt;

    if( mIsFetchPage == ID_TRUE )
    {
        mIsFetchPage = ID_FALSE;
        mBufferPool->cleanUpKey( mStatistics, &mMPRKey, mIsCachePage );
    }

    mCurExtRID = mSegInfo.mFstExtRID;
    mCurPageID = SD_NULL_PID;

    IDE_TEST( mSegMgmtOP->mGetExtInfo( mStatistics,
                                       mSpaceID,
                                       mCurExtRID,
                                       &mCurExtInfo )
              != IDE_SUCCESS );

    /* Segment의 첫번째 PID의 SeqNo는 0이다. */
    sMPRCnt = getMPRCnt( mCurExtRID, mCurExtInfo.mFstPID );

    IDE_TEST( mBufferPool->fetchPagesByMPR(
                  mStatistics,
                  mSpaceID,
                  mCurExtInfo.mFstPID,
                  sMPRCnt,
                  &mMPRKey )
              != IDE_SUCCESS );
    mIsFetchPage = ID_TRUE;

    mMPRKey.moveToBeforeFst();
    mCurExtSeq = 0;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/****************************************************************
 * Description :
 * Filtering 및 Parallel Scan을 동시에 지원하기 위해,
 * ExtentSequence를 바탕으로 읽을 Extent인지 아닌지를 Filtering 한다.
 *
 * - Sampling 대상 선정 법 
 * Sampling Percentage가 P라 했을때, 누적되는 값 C를 두고 C에 P를
 * 더했을때 1을 넘어가는 경우에 Sampling 하는 것으로 한다.
 *
 * 예)
 * P=1, C=0
 * 첫번째 페이지 C+=P  C=>0.25
 * 두번째 페이지 C+=P  C=>0.50
 * 세번째 페이지 C+=P  C=>0.75
 * 네번째 페이지 C+=P  C=>1(Sampling!)  C--; C=>0
 * 다섯번째 페이지 C+=P  C=>0.25
 * 여섯번째 페이지 C+=P  C=>0.50
 * 일곱번째 페이지 C+=P  C=>0.75
 * 여덟번째 페이지 C+=P  C=>1(Sampling!)  C--; C=>0 
 *
 * aExtReq      - [IN] Extent의 Sequence번호
 * aFilterData  - [IN] ThreadID & ThreadCnt & SamplingData
 *
 * RETURN       - [OUT] 읽을지 여부
 ****************************************************************/
idBool sdbMPRMgr::filter4SamplingAndPScan( ULong   aExtSeq,
                                           void  * aFilterData )
{
    sdbMPRFilter4SamplingAndPScan * sData;
    idBool                          sRet = ID_FALSE;
    
    sData = (sdbMPRFilter4SamplingAndPScan*)aFilterData;

    if( ( ( aExtSeq % sData->mThreadCnt ) == sData->mThreadID ) &&
        ( ( (UInt)( sData->mPercentage * aExtSeq  + SMI_STAT_SAMPLING_INITVAL ) ) !=
          ( (UInt)( sData->mPercentage * (aExtSeq+1) + SMI_STAT_SAMPLING_INITVAL ) ) ) )
    {
        sRet = ID_TRUE;
    }
    else
    {
        sRet = ID_FALSE;
    }

    return sRet;
}

/****************************************************************
 * Description :
 * Parallel Scan을 위해 ExtentSequence를 Modular하여 내가 읽을
 * Extent인지 아닌지를 Filtering 한다.
 *
 * aExtReq     - [IN] Extent의 Sequence번호
 * aFilterData - [IN] ThreadID & ThreadCnt
 *
 * RETURN      - [OUT] 읽을지 여부
 ****************************************************************/
idBool sdbMPRMgr::filter4PScan( ULong   aExtSeq,
                                void  * aFilterData )
{
    sdbMPRFilter4PScan * sData;
    idBool               sRet = ID_FALSE;
    
    sData = (sdbMPRFilter4PScan*)aFilterData;

    if( ( aExtSeq % sData->mThreadCnt ) == sData->mThreadID )
    {
        sRet = ID_TRUE;
    }
    else
    {
        sRet = ID_FALSE;
    }

    return sRet;
}

/****************************************************************
 * Description :
 * Scan시 mCurPageID의 다음 Page에 대한 PID와 Pointer를 찾아준다.
 * 읽을지 여부는 Filter로 판단한다.
 *
 * aFilterData - [IN] Filter에서 쓰일 참고값
 * aPageID     - [OUT] Next PID
 ****************************************************************/
IDE_RC sdbMPRMgr::getNxtPageID( void           * aFilterData,
                                scPageID       * aPageID )
{
    sdRID     sExtRID;
    scPageID  sNxtPID;

    sExtRID = mCurExtRID;
    sNxtPID = mCurPageID;

    while(1)
    {
        IDE_TEST( mSegMgmtOP->mGetNxtAllocPage( mStatistics,
                                                mSpaceID,
                                                &mSegInfo,
                                                &mSegCacheInfo,
                                                &sExtRID,
                                                &mCurExtInfo,
                                                &sNxtPID )
                  != IDE_SUCCESS );

        IDE_TEST_CONT( sNxtPID == SD_NULL_PID, cont_no_more_page );

        if( sExtRID != mCurExtRID )
        {
            mCurExtSeq++;
        }

        if( mFilter == NULL )
        {
            IDE_ERROR( aFilterData == NULL );
            break; /* Filtering 안하면 전부 OK */
        }
        else
        {
            IDE_ERROR( aFilterData != NULL );
            if( mFilter( mCurExtSeq, aFilterData ) == ID_TRUE )
            {
                break;
            }
            else
            {
                /*Filter에 통과 못함 */
            }
        }

        mCurExtRID = sExtRID;
    }

    IDE_TEST( fixPage( sExtRID,
                       sNxtPID )
              != IDE_SUCCESS );

    *aPageID = sNxtPID;

    return IDE_SUCCESS;

    IDE_EXCEPTION_CONT( cont_no_more_page );

    /* BUG-33719 [sm-disk-resource] set full scan hint for TMS. 
     * NoSampling SerialScan인 경우, Fullscan용 Hint를 설정해야 함 */
    if( aFilterData == NULL )
    {
        mFoundLstAllocPage = ID_TRUE;
    }
    *aPageID     = SD_NULL_PID;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    *aPageID     = SD_NULL_PID;

    return IDE_FAILURE;
}

/****************************************************************
 * Description:
 *  MPR 키를 이용하여 fixPage를 한다.
 *
 *  이미 fixCount를 올려 놓았기 때문에, 반드시 해시(버퍼)에 존재함을
 *  보장 할 수 있다.  그렇기 때문에 여기선 별도의 fix를 하지 않고,
 *  해시도 접근하지 않는다. 또한 touch Count도 증가 시키지 않는다.
 *
 *  getPage인터페이스를 보면 알겠지만, 임의의 페이지에 대해서 접근하는
 *  것이 아니고, 리턴될 BCB가 정해져 있다.
 *
 *  aExtRID     - [IN] aPage가 있는 Extent RID
 *  aPageID     - [IN] PageID
 ****************************************************************/
IDE_RC sdbMPRMgr::fixPage( sdRID            aExtRID,
                           scPageID         aPageID )
{
    sdbBCB          *sBCB;
    sdpPhyPageHdr   *sPageHdr;
    sdpPageType      sPageType;
    scPageID         sFstReadPID;
    sdRID            sPrvExtRID;
    UInt             sMPRCnt;

    sPrvExtRID = mCurExtRID;

    /* MPR로 Read한 페이지중 아직 읽지 않은 페이지가 있는지 조사한다. */
    while(1)
    {
        if( mMPRKey.hasNextPage() == ID_TRUE )
        {
            IDE_ASSERT( mMPRKey.hasNextPage() == ID_TRUE );
            IDE_ASSERT( aPageID != SD_NULL_PID );

            mMPRKey.moveToNext();
        }
        else
        {
            /* 새로운 페이지를 Disk에서 읽어와야 한다. */
            IDE_ASSERT( aExtRID != SD_NULL_RID );

            /* mCurPageID는 이전에 읽었던 Extent에 존재하지 않는다.
             * 다음 Extent로 이동 한다. */
            if( aExtRID != sPrvExtRID )
            {
                /* BUG-29450 - DB_FILE_MULTIPAGE_READ_COUNT가 Extent당 페이지
                 *             개수보다 크거나 align이 맞지 않는 경우
                 *             FullScan시 Hang 걸릴 수 있다.
                 *
                 * MPR의 MPRCnt는 Extent당 페이지 개수에 align 되어 있기 때문에
                 * sFstReadPID = mCurExtInfo.mFstDataPID;
                 * 와 같이 첫번째 데이터 페이지로 설정하면 안된다. */
                sFstReadPID = mCurExtInfo.mFstPID;
                sPrvExtRID  = aExtRID;
            }
            else
            {
                sBCB = mMPRKey.mBCBs[ mMPRKey.mCurrentIndex ].mBCB;

                IDE_ASSERT( sBCB != NULL );

                /* 같은 Extent의 다음페이지 이므로 +1 한 값이 다음에
                 * 읽어야 할 Page ID */
                sFstReadPID = sBCB->mPageID + 1;
            }

            /* 마지막 Extent를 읽을때는 HWM까지만 읽어야 하기때문에
             * MPR Count를 조정해야 한다. */
            sMPRCnt = getMPRCnt( aExtRID, sFstReadPID );

            mIsFetchPage = ID_FALSE;
            mBufferPool->cleanUpKey( mStatistics, &mMPRKey, mIsCachePage );

            IDE_ASSERT( mCurExtInfo.mFstPID <= sFstReadPID );
            IDE_ASSERT( ( sFstReadPID + sMPRCnt ) <=
                        ( mCurExtInfo.mFstPID + mSegInfo.mPageCntInExt ) );

            IDE_TEST( mBufferPool->fetchPagesByMPR(
                          mStatistics,
                          mSpaceID,
                          sFstReadPID,
                          sMPRCnt,
                          &mMPRKey )
                      != IDE_SUCCESS );
            mIsFetchPage = ID_TRUE;
        }

        sBCB = mMPRKey.mBCBs[ mMPRKey.mCurrentIndex ].mBCB;

        if( ( sBCB->mSpaceID == mSpaceID ) &&
            ( sBCB->mPageID  == aPageID  ) )
        {
            IDE_ASSERT( sBCB->mFrame != NULL );

            /* BUG-29005 FullScan 성능 개선 */
            sPageHdr  = sdpPhyPage::getHdr( sBCB->mFrame );
            sPageType = sdpPhyPage::getPageType( sPageHdr );

            if ( (sPageType == SDP_PAGE_DATA) ||
                 (sPageType == SDP_PAGE_INDEX_BTREE) ||
                 (sPageType == SDP_PAGE_INDEX_RTREE) )
            {
                mLstAllocPID    = aPageID;
                mLstAllocSeqNo  = sPageHdr->mSeqNo;
            }

            break;
        }
    } /* While */

    mCurExtRID  = aExtRID;
    mCurPageID  = aPageID;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/****************************************************************
 * Description:
 *  MPR이 현재 가리키고 있는 page의 pointer를 리턴한다.
 *
 ****************************************************************/
UChar* sdbMPRMgr::getCurPagePtr()
{
    sdbBCB   *sBCB;

    sBCB = mMPRKey.mBCBs[ mMPRKey.mCurrentIndex ].mBCB;
    return sBCB->mFrame;
}

/****************************************************************
 * Description:
 *  MPR Count를 구한다. 마지막 Extent를 읽을때는 HWM까지만 읽어야
 *  하기때문에 MPR Count를 조정해야 한다
 *
 * aPagePtr - [IN] Page Pointer
 ****************************************************************/
UInt sdbMPRMgr::getMPRCnt( sdRID aCurReadExtRID, scPageID aFstReadPID )
{
    UInt sMPRCnt;
    UInt sState = 0;

    sMPRCnt = mMPRCnt;

    /* BUG-29005 FullScan 성능 개선 */
    if ( mSegCacheInfo.mUseLstAllocPageHint == ID_TRUE )
    {
        sState = 1;
        
        /* 마지막 할당된 페이지를 포함한 Extent를 read하는 경우 */
        if ( (mCurExtInfo.mFstPID <= mSegCacheInfo.mLstAllocPID) &&
             (mCurExtInfo.mLstPID >= mSegCacheInfo.mLstAllocPID) )
        {
            sState = 2;
            IDE_ASSERT( ( mSegCacheInfo.mLstAllocPID + 1 ) > aFstReadPID );

            if ( ( aFstReadPID + sMPRCnt - 1 ) > mSegCacheInfo.mLstAllocPID )
            {
                sState = 3;
                sMPRCnt = mSegCacheInfo.mLstAllocPID - aFstReadPID + 1;
            }
        }
    }

    if( mSegInfo.mExtRIDHWM == aCurReadExtRID )
    {
        sState = 4;
        IDE_ASSERT( ( mSegInfo.mHWMPID + 1 ) > aFstReadPID );

        if( ( aFstReadPID + sMPRCnt - 1) > mSegInfo.mHWMPID )
        {
            sState = 5;
            sMPRCnt = mSegInfo.mHWMPID - aFstReadPID + 1;
        }
    }

    /* To Analyze BUG-27447, log a few informations */
//    IDE_ASSERT( sMPRCnt != 0 );
    if( sMPRCnt == 0 )
    {
        // !! BUG-27447 is reproduced
        ideLog::log(IDE_SERVER_0, "[BUG-27447] is reproduced");
        ideLog::log(IDE_SERVER_0, "[BUG-27447] this=%x, sState=%u, aCurReadExtRID=%llu, aFstReadPID=%u\n",this, sState, aCurReadExtRID, aFstReadPID);
        ideLog::log(IDE_SERVER_0, "[BUG-27447] mSegInfo.mSegHdrPID=%u, mSegInfo.mType=%u, mSegInfo.mState=%u, mSegInfo.mPageCntInExt=%u, mSegInfo.mFmtPageCnt=%llu, mSegInfo.mExtCnt=%llu, mSegInfo.mExtDirCnt=%llu, mSegInfo.mFstExtRID=%llu, mSegInfo.mLstExtRID=%llu, mSegInfo.mHWMPID=%u, mSegInfo.mLstAllocExtRID=%llu, mSegInfo.mFstPIDOfLstAllocExt=%u\n", mSegInfo.mSegHdrPID, mSegInfo.mType, mSegInfo.mState, mSegInfo.mPageCntInExt, mSegInfo.mFmtPageCnt, mSegInfo.mExtCnt, mSegInfo.mExtDirCnt, mSegInfo.mFstExtRID, mSegInfo.mLstExtRID, mSegInfo.mHWMPID, mSegInfo.mLstAllocExtRID, mSegInfo.mFstPIDOfLstAllocExt);
        IDE_ASSERT( 0 );
    }

    return sMPRCnt;
}
