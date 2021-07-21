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
 * $Id$
 **********************************************************************/

/***********************************************************************
 * PROJ-2102 The Fast Secondary Buffer 
 **********************************************************************/

#include <idl.h>
#include <ide.h>
#include <iduLatch.h>
#include <smErrorCode.h>

#include <smDef.h>
#include <sdb.h>
#include <smrRecoveryMgr.h>
#include <sds.h>
#include <sdsReq.h>

#define SDS_REMOVE_BCB_WAIT_USEC   (50000)

UInt              sdsBufferMgr::mPageSize;
UInt              sdsBufferMgr::mFrameCntInExtent;
UInt              sdsBufferMgr::mCacheType;
sdbCPListSet      sdsBufferMgr::mCPListSet;
sdbBCBHash        sdsBufferMgr::mHashTable;
sdsBufferArea     sdsBufferMgr::mSBufferArea;
sdsFile           sdsBufferMgr::mSBufferFile;
sdsMeta           sdsBufferMgr::mMeta;
sdsBufferMgrStat  sdsBufferMgr::mStatistics;

sdsSBufferServieStage sdsBufferMgr::mServiceStage;

/******************************************************************************
 * Description : Secondary Buffer Manager 를 초기화 한다.
 ******************************************************************************/
IDE_RC sdsBufferMgr::initialize()
{
    SChar     * sPath;
    UInt        sHashBucketDensity;
    UInt        sHashLatchDensity;
    UInt        sCPListCnt;
    UInt        sSBufferEnable;
    UInt        sSBufferPageCnt; 
    UInt        sSBufferExtentCount;
    UInt        sExtCntInGroup; 
    UInt        sSBufferGroupCnt; 
    UInt        sHashBucketCnt;
    SInt        sState  =  0;
 
    mPageSize = SD_PAGE_SIZE;
    /* Secondary Buffer Enable */
    sSBufferEnable  = smuProperty::getSBufferEnable();
    /* Secondary Buffer Size */
    sSBufferPageCnt = smuProperty::getSBufferSize()/mPageSize;
    /* sBuffer file이있는 디렉토리 */    
    sPath = (SChar *)smuProperty::getSBufferDirectoryName();
    /* 하나의 hash bucket에 들어가는 BCB개수 */
    sHashBucketDensity = smuProperty::getBufferHashBucketDensity();
    /* 하나의 hash chains latch당 BCB개수 */
    sHashLatchDensity = smuProperty::getBufferHashChainLatchDensity();
    /* Check point list */
    sCPListCnt = smuProperty::getBufferCheckPointListCnt();
    /* flush 한번당 내려가는 page 갯수 */    
    mFrameCntInExtent = smuProperty::getBufferIOBufferSize(); 
    /* all/dirty/clean type */
    mCacheType = smuProperty::getSBufferType();

    /* Property가 On 이 되어있고 
     * 계산한 page count와 경로가 valid 한 경우만 identify를 수행한다. */
    if( (sSBufferEnable == SMU_SECONDARY_BUFFER_ENABLE ) &&
        ( (sSBufferPageCnt != 0) && (idlOS::strcmp(sPath, "") != 0) ) )
    {
        setIdentifiable(); 
    }
    else
    {
        setUnserviceable();
        IDE_CONT( SKIP_UNSERVICEABLE );
    }

    if( sSBufferPageCnt < SDS_META_MAX_CHUNK_PAGE_COUNT )
    {
        sSBufferPageCnt = SDS_META_MAX_CHUNK_PAGE_COUNT;
    } 

    /* MetaTable을 구성할수 있는 단위로 크기로 align down */
    sExtCntInGroup      = SDS_META_MAX_CHUNK_PAGE_COUNT / mFrameCntInExtent;
    sSBufferGroupCnt    = sSBufferPageCnt / (sExtCntInGroup * mFrameCntInExtent); 
    sSBufferExtentCount = sSBufferGroupCnt * sExtCntInGroup;
    sSBufferPageCnt     = sSBufferExtentCount * mFrameCntInExtent;
    sHashBucketCnt      = sSBufferPageCnt / sHashBucketDensity;

    IDE_TEST( mHashTable.initialize( sHashBucketCnt,
                                     sHashLatchDensity,
                                     SD_LAYER_SECONDARY_BUFFER )
              != IDE_SUCCESS );
    sState = 1;
    
    IDE_TEST( mCPListSet.initialize( sCPListCnt, SD_LAYER_SECONDARY_BUFFER ) 
              != IDE_SUCCESS );
    sState = 2;

    IDE_TEST( mSBufferArea.initializeStatic( sSBufferExtentCount,
                                             sSBufferPageCnt /*aBCBCount*/)               
              != IDE_SUCCESS );
    sState = 3; 

    IDE_TEST( mSBufferFile.initialize( sSBufferGroupCnt,
                                       sSBufferPageCnt ) != IDE_SUCCESS );
    sState = 4;

    IDE_TEST( mMeta.initializeStatic( sSBufferGroupCnt, 
                                      sExtCntInGroup, 
                                      &mSBufferFile, 
                                      &mSBufferArea, 
                                      &mHashTable ) 
              != IDE_SUCCESS );
    sState = 5;

    mStatistics.initialize( &mSBufferArea, &mHashTable, &mCPListSet );

    IDE_EXCEPTION_CONT( SKIP_UNSERVICEABLE );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch( sState )
    {   
        case 5:
            IDE_ASSERT( mMeta.destroyStatic() == IDE_SUCCESS );
        case 4:
            IDE_ASSERT( mSBufferFile.destroy() == IDE_SUCCESS );
        case 3:
            IDE_ASSERT( mSBufferArea.destroyStatic() == IDE_SUCCESS );
        case 2:
            IDE_ASSERT( mCPListSet.destroy() == IDE_SUCCESS );
        case 1:
            IDE_ASSERT( mHashTable.destroy() == IDE_SUCCESS );
        default:
            break;
    }

    return IDE_FAILURE;
}

/******************************************************************************
 * Description :
 *  Secondary BufferManager를 종료한다.
 ******************************************************************************/
IDE_RC sdsBufferMgr::destroy()
{
    IDE_TEST_CONT( isServiceable() != ID_TRUE, SKIP_SUCCESS );

    IDE_ASSERT( mMeta.destroyStatic() == IDE_SUCCESS );

    IDE_ASSERT( mSBufferFile.destroyStatic() == IDE_SUCCESS );

    IDE_ASSERT( mSBufferArea.destroyStatic() == IDE_SUCCESS );

    IDE_ASSERT( mCPListSet.destroy() == IDE_SUCCESS );

    IDE_ASSERT( mHashTable.destroy() == IDE_SUCCESS );

    IDE_EXCEPTION_CONT( SKIP_SUCCESS );

    return IDE_SUCCESS;
}

/****************************************************************
 * Description :Server Shutdown시 MetaTable 전체를 update 한다
 ****************************************************************/
IDE_RC sdsBufferMgr::finalize()
{
    IDE_TEST_CONT( isServiceable() != ID_TRUE, SKIP_SUCCESS );

    IDE_TEST( mMeta.finalize( NULL /*aStatistics*/) != IDE_SUCCESS ); 

    IDE_EXCEPTION_CONT( SKIP_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/****************************************************************
 * Description:
 *  BCB를 free 상태로 변경한다. (BCB를 버퍼에서 제거)
 *  free될 BCB 이므로 DIRTY 상태여서는 안된다.
 *  removeBCB랑 비슷하다.
 *  aSBCB      [IN] : FREE 시킬 BCB
 ****************************************************************/
IDE_RC sdsBufferMgr::makeFreeBCB( idvSQL     * aStatistics,
                                  sdsBCB     * aSBCB )
{
    sdsHashChainsLatchHandle  * sHashChainsHandle = NULL;
    PDL_Time_Value   sTV;
    scSpaceID        sSpaceID; 
    scPageID         sPageID; 
    UInt             sState = 0;

    IDE_ASSERT( aSBCB != NULL );

    sSpaceID = aSBCB->mSpaceID; 
    sPageID  = aSBCB->mPageID; 

retry:
    sHashChainsHandle = mHashTable.lockHashChainsXLatch( aStatistics,
                                                         sSpaceID,
                                                         sPageID );
    sState = 1;
    aSBCB->lockBCBMutex( aStatistics );
    sState = 2;

    switch( aSBCB->mState )
    {
        case SDS_SBCB_CLEAN:
            /* 넘겨받은 BCB와 lock 잡은 BCB가 같아야 한다. */
            IDE_DASSERT( ( sSpaceID == aSBCB->mSpaceID ) &&
                         ( sPageID  == aSBCB->mPageID ) );

            mHashTable.removeBCB( aSBCB );

            aSBCB->setFree();
            break;

        case SDS_SBCB_INIOB:
        case SDS_SBCB_OLD:
            /* 넘겨받은 BCB와 lock 잡은 BCB가 같아야 한다. */
            IDE_DASSERT( ( sSpaceID == aSBCB->mSpaceID ) &&
                         ( sPageID  == aSBCB->mPageID ) );

            sState = 1;
            aSBCB->unlockBCBMutex();
            sState = 0;
            mHashTable.unlockHashChainsLatch( sHashChainsHandle );
            sHashChainsHandle = NULL;

            sTV.set( 0, SDS_REMOVE_BCB_WAIT_USEC ); /* microsec */
            idlOS::sleep( sTV );
            goto retry;

            break;

        case SDS_SBCB_FREE:
            /* nothing to do */
            break;

        case SDS_SBCB_DIRTY:
        default:
            IDE_RAISE( ERROR_INVALID_BCD );
            break;
    }

    sState = 1;
    aSBCB->unlockBCBMutex();
    sState = 0;
    mHashTable.unlockHashChainsLatch( sHashChainsHandle );
    sHashChainsHandle = NULL;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERROR_INVALID_BCD );
    {
        ideLog::log( IDE_ERR_0,
                     "invalid bcd to make free \n"
                     "sSpaseID:%u\n"
                     "sPageID :%u\n"
                     "ID      :%u\n"
                     "spaseID :%u\n"
                     "pageID  :%u\n"
                     "state   :%u\n"
                     "CPListNo:%u\n"
                     "HashTableNo:%u\n",
                     sSpaceID,
                     sPageID,
                     aSBCB->mSBCBID,
                     aSBCB->mSpaceID,
                     aSBCB->mPageID,
                     aSBCB->mState,
                     aSBCB->mCPListNo,
                     aSBCB->mHashBucketNo );
        IDE_DASSERT(0);
    }
    IDE_EXCEPTION_END;
    
    switch( sState )
    {
        case 2:
            aSBCB->unlockBCBMutex();
        case 1:
            mHashTable.unlockHashChainsLatch( sHashChainsHandle );
            sHashChainsHandle = NULL;
            break;
    }

    return IDE_FAILURE;
}

/****************************************************************
 * Description:BCB 를 Free 상태로 바꾼다. 
 * 상태는 OLD 만 가능하고  OLD가 되는 시점에 이미 hash에서는
 * 지워졌으므로 hash에서 지우는 과정은 필요하지 않다.
 * aSBCB        [IN] : 지울 BCB
 ****************************************************************/
IDE_RC sdsBufferMgr::makeFreeBCBForce( idvSQL     * aStatistics,
                                       sdsBCB     * aSBCB )
{
    IDE_ASSERT( aSBCB != NULL );

    IDE_TEST_RAISE( aSBCB->mState != SDS_SBCB_OLD, ERROR_INVALID_BCD );

    if( aSBCB->mCPListNo != SDB_CP_LIST_NONE )
    {
        mCPListSet.remove( aStatistics, aSBCB );
    }

    aSBCB->setFree();

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERROR_INVALID_BCD );
    {
        ideLog::log( IDE_ERR_0,
                     "invalid bcd to make free(Force)\n"
                     "ID      :%"ID_UINT32_FMT"\n"
                     "spaseID :%"ID_UINT32_FMT"\n"
                     "pageID  :%"ID_UINT32_FMT"\n"
                     "state   :%"ID_UINT32_FMT"\n"
                     "CPListNo:%"ID_UINT32_FMT"\n"
                     "HashTableNo:%"ID_UINT32_FMT"\n",
                     aSBCB->mSBCBID,
                     aSBCB->mSpaceID,
                     aSBCB->mPageID,
                     aSBCB->mState,
                     aSBCB->mCPListNo,
                     aSBCB->mHashBucketNo );
        IDE_DASSERT( 0 );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/****************************************************************
 * Description : victim을 얻어온다.
 *  aBCBIndex  [IN]  BCB의 index 
 *  aSBCB      [OUT] 쓰기 가능한 BCB
 ****************************************************************/
void sdsBufferMgr::getVictim( idvSQL     * aStatistics,
                              UInt         aBCBIndex,
                              sdsBCB    ** aSBCB )
{
    sdsBCB * sVictimBCB = NULL;

    sVictimBCB = mSBufferArea.getBCB( aBCBIndex );
    /* 읽기 중인 BCB가 victim 대상이 될수없다 */
    sVictimBCB->lockReadIOMutex( aStatistics );
    (void)makeFreeBCB( aStatistics, sVictimBCB );
    sVictimBCB->unlockReadIOMutex();

    *aSBCB = sVictimBCB;
}

/****************************************************************
 * Description : Secondary Buffer에서 Buffer Frame으로 페이지를 복사
 *  aSBCB           [IN]  Secondary Buffer의 BCB
 *  aBCB            [IN]  올릴 대상 BCB
 *  aIsCorruptRead  [Out] 다른 BCB가 아니지 여부 
 *  aIsCorruptPage  [Out] 페이지 Corrupt 여부 
 ****************************************************************/
IDE_RC sdsBufferMgr::moveUpPage( idvSQL     * aStatistics,
                                 sdsBCB    ** aSBCB,
                                 sdbBCB     * aBCB,
                                 idBool     * aIsCorruptRead,
                                 idBool     * aIsCorruptPage )
{
    scSpaceID   sSpaceID = aBCB->mSpaceID;
    scPageID    sPageID  = aBCB->mPageID;
    idvTime     sBeginTime;
    idvTime     sEndTime; 
    ULong       sReadTime         = 0;
    ULong       sCalcChecksumTime = 0;
    UInt        sState            = 0;
    idBool      sIsLock        = ID_FALSE;
    idBool      sIsCorruptRead = ID_FALSE;
    idBool      sIsCorruptPage = ID_FALSE;
    sdsBCB    * sExistSBCB     = *aSBCB;

    IDE_TEST( isServiceable() != ID_TRUE );

    /* 읽기 중인 BCB가 victim 대상이 되면 안된다. */
    sExistSBCB->lockReadIOMutex( aStatistics );
    sIsLock = ID_TRUE;
    
    if ( ( sExistSBCB->mSpaceID != sSpaceID ) ||
         ( sExistSBCB->mPageID != sPageID ) )
    {
        sIsLock = ID_FALSE;
        sExistSBCB->unlockReadIOMutex();

        IDE_TEST( findBCB( aStatistics, sSpaceID, sPageID, &sExistSBCB )
                  != IDE_SUCCESS );

        if ( sExistSBCB == NULL )
        {
            sIsCorruptRead = ID_TRUE;        
            IDE_CONT( SKIP_CORRUPT_READ );
        }
        else 
        {
            *aSBCB = sExistSBCB;

            sExistSBCB->lockReadIOMutex( aStatistics );
            sIsLock = ID_TRUE;
        }
    }

    IDE_ASSERT( sExistSBCB->mState != SDS_SBCB_OLD );

    IDV_TIME_GET(&sBeginTime);

    mStatistics.applyBeforeSingleReadPage( aStatistics );
    sState = 1;

    IDE_TEST_RAISE( mSBufferFile.open( aStatistics ) != IDE_SUCCESS,
                    ERROR_PAGE_MOVEUP );

    IDE_TEST_RAISE( mSBufferFile.read( aStatistics,
                                 sExistSBCB->mSBCBID,   /* ID */
                                 mMeta.getFrameOffset( sExistSBCB->mSBCBID ),  
                                 1,                     /* page count */
                                 aBCB->mFrame )         /* to */
                    != IDE_SUCCESS,
                    ERROR_PAGE_MOVEUP );

    sState = 0;
    mStatistics.applyAfterSingleReadPage( aStatistics );

    IDV_TIME_GET(&sEndTime);
    sReadTime = IDV_TIME_DIFF_MICRO(&sBeginTime, &sEndTime);
    sBeginTime = sEndTime;

    if( sdpPhyPage::checkAndSetPageCorrupted( aBCB->mSpaceID,
                                              aBCB->mFrame ) 
        == ID_TRUE )
    {   
        sIsCorruptPage = ID_TRUE;

        if ( smLayerCallback::getCorruptPageReadPolicy()
             != SDB_CORRUPTED_PAGE_READ_PASS )
        {
            IDE_RAISE( ERROR_PAGE_CORRUPTION );
        }
    }
#ifdef DEBUG
    IDE_TEST_RAISE(
        sdpPhyPage::isConsistentPage( (UChar*) sdpPhyPage::getHdr( aBCB->mFrame ) )
        == ID_FALSE ,
        ERROR_PAGE_INCONSISTENT  );
#endif
    IDE_DASSERT( validate( aBCB ) == IDE_SUCCESS );

    setFrameInfoAfterReadPage( sExistSBCB,
                               aBCB,
                               ID_TRUE ); // check to online tablespace

    IDV_TIME_GET( &sEndTime );
    sCalcChecksumTime = IDV_TIME_DIFF_MICRO( &sBeginTime, &sEndTime );

    mStatistics.applyReadPages( sCalcChecksumTime,
                                sReadTime );

    sIsLock = ID_FALSE;
    sExistSBCB->unlockReadIOMutex();

    IDE_EXCEPTION_CONT( SKIP_CORRUPT_READ );

    if(aIsCorruptRead != NULL )
    {
        *aIsCorruptRead = sIsCorruptRead;
    } 
    if( aIsCorruptPage != NULL ) 
    {
        *aIsCorruptPage = sIsCorruptPage;
    }

    return IDE_SUCCESS;
   
    IDE_EXCEPTION( ERROR_PAGE_MOVEUP );
    {
        setUnserviceable();
        IDE_SET( ideSetErrorCode( smERR_ABORT_PageMoveUpStopped ) );
    }
    IDE_EXCEPTION( ERROR_PAGE_CORRUPTION );
    {
        if( aIsCorruptPage != NULL ) 
        {
            *aIsCorruptPage = ID_TRUE;
        }

        switch ( smLayerCallback::getCorruptPageReadPolicy() )
        {
            case SDB_CORRUPTED_PAGE_READ_FATAL :
                IDE_SET( ideSetErrorCode( smERR_FATAL_PageCorrupted,
                                          aBCB->mSpaceID,
                                          aBCB->mPageID ) );

                break;
            case SDB_CORRUPTED_PAGE_READ_ABORT :
                // PROJ-1665 : ABORT Error를 반환함
                //리뷰: 지워버리고, mFrame을 받는 쪽에서 이걸 호출해서 써라.
                IDE_SET( ideSetErrorCode( smERR_ABORT_PageCorrupted,
                                          aBCB->mSpaceID,
                                          aBCB->mPageID ) );
                break;
            default:
                break;
        }
    }
#ifdef DEBUG
    IDE_EXCEPTION( ERROR_PAGE_INCONSISTENT );
    {
        sdpPhyPageHdr   * sPhyPageHdr;
        sPhyPageHdr = sdpPhyPage::getHdr(aBCB->mFrame);

        ideLog::log( IDE_SM_0,
                 "moveup : Page Dump -----------------\n"
                 "Page Start Pointer        : %"ID_xPOINTER_FMT"\n"
                 "FrameHdr.mCheckSum        : %"ID_UINT32_FMT"\n"
                 "FrameHdr.mPageLSN.mFileNo : %"ID_UINT32_FMT"\n"
                 "FrameHdr.mPageLSN.mOffset : %"ID_UINT32_FMT"\n"
                 "FrameHdr.mIndexSMONo      : %"ID_vULONG_FMT"\n"
                 "FrameHdr.mSpaceID         : %"ID_UINT32_FMT"\n"
                 "FrameHdr.mBCBPtr          : 0x%"ID_xPOINTER_FMT"\n"
                 "Total Free Size           : %"ID_UINT32_FMT"\n"
                 "Available Free Size       : %"ID_UINT32_FMT"\n"
                 "Logical Hdr Size          : %"ID_UINT32_FMT"\n"
                 "Size of CTL               : %"ID_UINT32_FMT"\n"
                 "Free Space Begin Offset   : %"ID_UINT32_FMT"\n"
                 "Free Space End Offset     : %"ID_UINT32_FMT"\n"
                 "Page Type                 : %"ID_UINT32_FMT"\n"
                 "Page State                : %"ID_UINT32_FMT"\n"
                 "Page ID                   : %"ID_UINT32_FMT"\n"
                 "is Consistent             : %"ID_UINT32_FMT"\n"
                 "LinkState                 : %"ID_UINT32_FMT"\n"
                 "ParentInfo.mParentPID     : %"ID_UINT32_FMT"\n"
                 "ParentInfo.mIdxInParent   : %"ID_UINT32_FMT"\n"
                 "Prev Page ID, Nex Page ID : %"ID_UINT32_FMT", %"ID_UINT32_FMT"\n"
                 "TableOID                  : %"ID_vULONG_FMT"\n"
                 "Seq No                    : %"ID_UINT64_FMT"\n"
                 "---------------------------------------------\n",
                 (UChar*)sPhyPageHdr,
                 sPhyPageHdr->mFrameHdr.mCheckSum,
                 sPhyPageHdr->mFrameHdr.mPageLSN.mFileNo,
                 sPhyPageHdr->mFrameHdr.mPageLSN.mOffset,
                 sPhyPageHdr->mFrameHdr.mIndexSMONo,
                 sPhyPageHdr->mFrameHdr.mSpaceID,
                 sPhyPageHdr->mFrameHdr.mBCBPtr,
                 sPhyPageHdr->mTotalFreeSize,
                 sPhyPageHdr->mAvailableFreeSize,
                 sPhyPageHdr->mLogicalHdrSize,
                 sPhyPageHdr->mSizeOfCTL,
                 sPhyPageHdr->mFreeSpaceBeginOffset,
                 sPhyPageHdr->mFreeSpaceEndOffset,
                 sPhyPageHdr->mPageType,
                 sPhyPageHdr->mPageState,
                 sPhyPageHdr->mPageID,
                 sPhyPageHdr->mIsConsistent,
                 sPhyPageHdr->mLinkState,
                 sPhyPageHdr->mParentInfo.mParentPID,
                 sPhyPageHdr->mParentInfo.mIdxInParent,
                 sPhyPageHdr->mListNode.mPrev,
                 sPhyPageHdr->mListNode.mNext,
                 sPhyPageHdr->mTableOID,
                 sPhyPageHdr->mSeqNo );

        IDE_SET( ideSetErrorCode( smERR_ABORT_INCONSISTENT_PAGE,
                                  aBCB->mSpaceID,
                                  aBCB->mPageID) );
    }
#endif
    IDE_EXCEPTION_END;

    if( sIsLock == ID_TRUE )
    {
        sIsLock = ID_FALSE;
        sExistSBCB->unlockReadIOMutex();
    }
 
    if ( sState != 0 )
    {
        mStatistics.applyAfterSingleReadPage( aStatistics );
    }
    
    return IDE_FAILURE;
}

/****************************************************************
 * Description : Buffer Frame에서  Secondary Buffer에 복사
 *  aStatistics - [IN] 통계정보
 *  aBCB          [IN] 내려쓸 대상 BCB  
 *  aBuffer       [IN] 내려쓸 대상 Frame 정보
 *  aExtentIndex  [IN] 내려쓸 위치 (extent)
 *  aFrameIndex   [IN] 내려쓸 위치 (frame)
 *  aSBCB         [OUT] 내려쓴 SBCB
 ****************************************************************/
IDE_RC sdsBufferMgr::moveDownPage( idvSQL       * aStatistics,
                                   sdbBCB       * aBCB,
                                   UChar        * aBuffer,
                                   ULong          aExtentIndex, 
                                   UInt           aFrameIndex,
                                   sdsBCB      ** aSBCB ) 
{
    sdsBCB      * sSBCB = NULL;
    sdsBCB      * sExistSBCB = NULL;
    scSpaceID     sSpaceID = aBCB->mSpaceID;
    scPageID      sPageID  = aBCB->mPageID;
    UInt          sState   = 0;
    idvTime       sBeginTime;
    idvTime       sEndTime;
    ULong         sWriteTime = 0;
    ULong         sCalcChecksumTime = 0;

    sdsHashChainsLatchHandle  * sHashChainsHandle = NULL;

    /* 1.이전에 같은 페이지가 내려온 것이 있는 확인 한다.*/
    IDE_TEST( findBCB( aStatistics, sSpaceID, sPageID, &sExistSBCB )
              != IDE_SUCCESS );

    /* 2.같은 페이지가 있었다면 지워야 한다. */
    if ( sExistSBCB != NULL )
    {
        (void)removeBCB( aStatistics, sExistSBCB );
    }

    /* 3.target Extent에서 빈 Frame을 얻어 온다.*/
    getVictim( aStatistics, 
               (aExtentIndex*mFrameCntInExtent)+aFrameIndex, 
               &sSBCB );

    /* 4.getVictim 은 꼭 성공해야 한다. */
    IDE_ASSERT( sSBCB != NULL );

    IDV_TIME_GET( &sBeginTime );

    IDE_TEST_RAISE( mSBufferFile.open( aStatistics ) != IDE_SUCCESS,
                    ERROR_PAGE_MOVEDOWN );

    mStatistics.applyBeforeSingleWritePage( aStatistics );
    sState = 1;
    
    /* 5.Secondary Buffer 에 내려쓴다. */
    IDE_TEST_RAISE( mSBufferFile.write( aStatistics,
                                        sSBCB->mSBCBID,   /* ID */
                                        mMeta.getFrameOffset( sSBCB->mSBCBID ),
                                        1,                /* page count */ 
                                        aBuffer ) 
                    != IDE_SUCCESS,
                    ERROR_PAGE_MOVEDOWN );         

    sState = 0;
    mStatistics.applyAfterSingleWritePage( aStatistics );

    IDV_TIME_GET( &sEndTime );
    sWriteTime = IDV_TIME_DIFF_MICRO( &sBeginTime, &sEndTime );
    sBeginTime = sEndTime;
   
    /* 6.SBCB 정보 생성 */
    /*  hash 에는 없지만 PBuffer에서 BCB에 있는 정보를 이용해 접근할수있어서 
        lock을 잡고 BCB를 설정 한뒤 hash에 삽입.. */
    sHashChainsHandle = mHashTable.lockHashChainsXLatch( aStatistics,
                                                         sSpaceID,
                                                         sPageID );
    sSBCB->lockBCBMutex( aStatistics );

    sSBCB->mSpaceID = sSpaceID;
    sSBCB->mPageID  = sPageID;
    SM_GET_LSN( sSBCB->mRecoveryLSN, aBCB->mRecoveryLSN );
    SM_GET_LSN( sSBCB->mPageLSN, smLayerCallback::getPageLSN( aBCB->mFrame ) );
    sSBCB->mBCB = aBCB;
    *aSBCB = sSBCB;

    if( aBCB->mPrevState == SDB_BCB_CLEAN ) 
    {
        sSBCB->mState  = SDS_SBCB_CLEAN;
    }
    else
    {
        IDE_DASSERT( aBCB->mPrevState == SDB_BCB_DIRTY ); 
      
        sSBCB->mState  = SDS_SBCB_DIRTY;

        if( ( !SM_IS_LSN_INIT(sSBCB->mRecoveryLSN) ) )
        {
            mCPListSet.add( aStatistics, sSBCB );
        }
    }
   
    /* 7. Hash 에 삽입 */
    mHashTable.insertBCB( sSBCB, (void**)&sExistSBCB );

    if( sExistSBCB != NULL )
    {
#ifdef DEBUG
        /* #2 에서 지웠기에 발생할수 없음 :
         * 문제될건 없으니 Debug에서만 사망처리 하고 진행 */
        IDE_RAISE( ERROR_INVALID_BCD )
#endif
        (void)removeBCB( aStatistics, sExistSBCB );

        mHashTable.insertBCB( aSBCB, (void**)&sExistSBCB );
    }

    sSBCB->unlockBCBMutex();
    mHashTable.unlockHashChainsLatch( sHashChainsHandle );
    sHashChainsHandle = NULL;

    IDV_TIME_GET( &sEndTime );
    sCalcChecksumTime = IDV_TIME_DIFF_MICRO( &sBeginTime, &sEndTime );

    mStatistics.applyWritePages( sCalcChecksumTime,
                                 sWriteTime );

#ifdef DEBUG
    /* 이미 확인하고 복사했지만 디버그에서는 다시 확인 */
    IDE_TEST_RAISE(
        sdpPhyPage::isConsistentPage( (UChar*) sdpPhyPage::getHdr( aBCB->mFrame ) )
        == ID_FALSE ,
        ERROR_PAGE_INCONSISTENT  );
#endif
    return IDE_SUCCESS;

    IDE_EXCEPTION( ERROR_PAGE_MOVEDOWN );
    {
        setUnserviceable();
        IDE_SET( ideSetErrorCode( smERR_ABORT_PageMovedownStopped ) );
    }
#ifdef DEBUG
    IDE_EXCEPTION( ERROR_INVALID_BCD );
    {
        ideLog::log( IDE_ERR_0,
                     "redundant Exist SBCB \n"
                     "ID      :%u\n"
                     "spaseID :%u\n"
                     "pageID  :%u\n"
                     "state   :%u\n"
                     "CPListNo:%u\n"
                     "HashTableNo:%u\n",
                     sExistSBCB->mSBCBID,
                     sExistSBCB->mSpaceID,
                     sExistSBCB->mPageID,
                     sExistSBCB->mState,
                     sExistSBCB->mCPListNo,
                     sExistSBCB->mHashBucketNo );
    }
    IDE_EXCEPTION( ERROR_PAGE_INCONSISTENT );
    {
        sdpPhyPageHdr   * sPhyPageHdr;
        sPhyPageHdr = sdpPhyPage::getHdr(aBCB->mFrame);
        ideLog::log( IDE_SM_0,
                 "movedown : Page Dump -----------------\n"
                 "Page Start Pointer        : %"ID_xPOINTER_FMT"\n"
                 "FrameHdr.mCheckSum        : %"ID_UINT32_FMT"\n"
                 "FrameHdr.mPageLSN.mFileNo : %"ID_UINT32_FMT"\n"
                 "FrameHdr.mPageLSN.mOffset : %"ID_UINT32_FMT"\n"
                 "FrameHdr.mIndexSMONo      : %"ID_vULONG_FMT"\n"
                 "FrameHdr.mSpaceID         : %"ID_UINT32_FMT"\n"
                 "FrameHdr.mBCBPtr          : 0x%"ID_xPOINTER_FMT"\n"
                 "Total Free Size           : %"ID_UINT32_FMT"\n"
                 "Available Free Size       : %"ID_UINT32_FMT"\n"
                 "Logical Hdr Size          : %"ID_UINT32_FMT"\n"
                 "Size of CTL               : %"ID_UINT32_FMT"\n"
                 "Free Space Begin Offset   : %"ID_UINT32_FMT"\n"
                 "Free Space End Offset     : %"ID_UINT32_FMT"\n"
                 "Page Type                 : %"ID_UINT32_FMT"\n"
                 "Page State                : %"ID_UINT32_FMT"\n"
                 "Page ID                   : %"ID_UINT32_FMT"\n"
                 "is Consistent             : %"ID_UINT32_FMT"\n"
                 "LinkState                 : %"ID_UINT32_FMT"\n"
                 "ParentInfo.mParentPID     : %"ID_UINT32_FMT"\n"
                 "ParentInfo.mIdxInParent   : %"ID_UINT32_FMT"\n"
                 "Prev Page ID, Nex Page ID : %"ID_UINT32_FMT", %"ID_UINT32_FMT"\n"
                 "TableOID                  : %"ID_vULONG_FMT"\n"
                 "Seq No                    : %"ID_UINT64_FMT"\n"
                 "---------------------------------------------\n",
                 (UChar*)sPhyPageHdr,
                 sPhyPageHdr->mFrameHdr.mCheckSum,
                 sPhyPageHdr->mFrameHdr.mPageLSN.mFileNo,
                 sPhyPageHdr->mFrameHdr.mPageLSN.mOffset,
                 sPhyPageHdr->mFrameHdr.mIndexSMONo,
                 sPhyPageHdr->mFrameHdr.mSpaceID,
                 sPhyPageHdr->mFrameHdr.mBCBPtr,
                 sPhyPageHdr->mTotalFreeSize,
                 sPhyPageHdr->mAvailableFreeSize,
                 sPhyPageHdr->mLogicalHdrSize,
                 sPhyPageHdr->mSizeOfCTL,
                 sPhyPageHdr->mFreeSpaceBeginOffset,
                 sPhyPageHdr->mFreeSpaceEndOffset,
                 sPhyPageHdr->mPageType,
                 sPhyPageHdr->mPageState,
                 sPhyPageHdr->mPageID,
                 sPhyPageHdr->mIsConsistent,
                 sPhyPageHdr->mLinkState,
                 sPhyPageHdr->mParentInfo.mParentPID,
                 sPhyPageHdr->mParentInfo.mIdxInParent,
                 sPhyPageHdr->mListNode.mPrev,
                 sPhyPageHdr->mListNode.mNext,
                 sPhyPageHdr->mTableOID,
                 sPhyPageHdr->mSeqNo );

        IDE_SET( ideSetErrorCode( smERR_ABORT_INCONSISTENT_PAGE,
                                  aBCB->mSpaceID,
                                  aBCB->mPageID) );
    }
#endif
    IDE_EXCEPTION_END;

    if ( sState != 0 )
    {
        mStatistics.applyAfterSingleWritePage( aStatistics );
    }
    
    IDE_ASSERT( 0 );

    return IDE_FAILURE;
}

/****************************************************************
 * Description :
 *  pageOut을 하기 위해 해당 Queue에 삽입한다.
 *  aBCB        - [IN]  BCB
 *  aObj        - [IN]  본 함수 수행하는데 필요한 자료.
 ****************************************************************/
IDE_RC sdsBufferMgr::makePageOutTargetQueueFunc( sdsBCB *aBCB,
                                                 void   *aObj )
{
    sdsPageOutObj * sObj = (sdsPageOutObj*)aObj;
    sdsSBCBState    sState;

    if( sObj->mFilter( aBCB , sObj->mEnv ) == ID_TRUE )
    {
        sState = aBCB->mState;

        /* Flush 대상의 (+ flush가 완료되기를 기다려야 하는 ) SBCB
          를 찾아 Flush Queue 에 삽입힌다.*/
        if( ( sState == SDS_SBCB_DIRTY ) ||
            ( sState == SDS_SBCB_INIOB ) ||
            ( sState == SDS_SBCB_OLD ) )

        {
            IDE_ASSERT(
                sObj->mQueueForFlush.enqueue( ID_FALSE, //mutex를 잡지 않는다.
                                              (void*)&aBCB )
                        == IDE_SUCCESS );
        }
        else 
        {
            /* nothing to do */
        }
 
        /* 필터를 만족하는 모든 BCB를 Free 대상으로 생각한다 */ 
        IDE_ASSERT(
            sObj->mQueueForMakeFree.enqueue( ID_FALSE, // mutex를 잡지 않는다.
                                             (void*)&aBCB )
            == IDE_SUCCESS );
    }

    return IDE_SUCCESS;
}

/******************************************************************************
 * Description : filter function 을 이용해 dirty인 BCB를 큐에 삽입한다.
 * aFunc    - [IN]  버퍼 area의 각 BCB에 적용할 함수
 * aObj     - [IN]  aFunc수행할때 필요한 변수
 ******************************************************************************/
IDE_RC sdsBufferMgr::makeFlushTargetQueueFunc( sdsBCB * aBCB,
                                               void   * aObj )
{
    sdsFlushObj   * sObj = (sdsFlushObj*)aObj;
    sdsSBCBState    sState;

    if( sObj->mFilter( aBCB , sObj->mEnv) == ID_TRUE )
    {
        sState = aBCB->mState;
        if( sState == SDS_SBCB_DIRTY )
        {
            IDE_ASSERT( sObj->mQueue.enqueue( ID_FALSE, // mutex를 잡지 않는다.
                                              (void*)&aBCB )
                        == IDE_SUCCESS );
        }
    }

    return IDE_SUCCESS;
}

/*****************************************************************
 * Description :
 *  어떤 BCB를 입력으로 받아도 항상 ID_TRUE를 리턴한다.
 ****************************************************************/
idBool sdsBufferMgr::filterAllBCBs( void   * /*aBCB*/,
                                    void   * /*aObj*/ )
{
    return ID_TRUE;
}

#if 0 //not used
/*****************************************************************
 * Description :
 *  aObj에는 spaceID가 들어있다. spaceID가 같은 BCB에 대해서
 *  ID_TRUE를 리턴
 *
 *  aBCB        - [IN]  BCB
 *  aObj        - [IN]  함수 수행할때 필요한 자료구조.
 ****************************************************************/
idBool sdsBufferMgr::filterTBSBCBs( void   *aBCB,
                                    void   *aObj )
{
    sdsBCB    *sBCB     = (sdsBCB*)aBCB;
    scSpaceID *sSpaceID = (scSpaceID*)aObj;

    if( sBCB->mSpaceID == *sSpaceID )
    {
        return ID_TRUE;
    }
    else
    {
        return ID_FALSE;
    }
}
#endif

/*****************************************************************
 * Description :
 *  aObj에는 특정 pid의 범위가 들어있다.
 *  BCB가 그 범위에 속할때만 ID_TRUE 리턴.
 *  aBCB        - [IN]  BCB
 *  aObj        - [IN]  함수 수행할때 필요한 자료구조. sdsBCBRange로 캐스팅해서
 ****************************************************************/
idBool sdsBufferMgr::filterRangeBCBs( void   *aBCB,
                                      void   *aObj )
{
    sdsBCB      *sBCB     = (sdsBCB*)aBCB;
    sdsBCBRange *sRange   = (sdsBCBRange*)aObj;

    if( sRange->mSpaceID == sBCB->mSpaceID )
    {
        if( ( sRange->mStartPID <= sBCB->mPageID ) &&
            ( sBCB->mPageID <= sRange->mEndPID ) )
        {
            return ID_TRUE;
        }
    }
    return ID_FALSE;
}

/************************************************************
 * Description :
 *  Discard:
 *  페이지를 버퍼내에서 제거한다.
    iniob가 아닌 이상 free로 설정...
 *
 *  aSBCB   - [IN]  BCB
 *  aObj    - [IN]  반드시 sdsDiscardPageObj가 들어있어야 한다.
 *                  이 데이터 구조에는 aBCB를 discard할지 말지 결정하는
 *                  함수 및 데이터가 있음
 ************************************************************/
IDE_RC sdsBufferMgr::discardNoWaitModeFunc( sdsBCB    * aSBCB,
                                            void      * aObj )
{
    sdsDiscardPageObj      * sObj = (sdsDiscardPageObj*)aObj;
    idvSQL                 * sStat = sObj->mStatistics;

    sdsHashChainsLatchHandle  * sHashChainsHandle = NULL;
    scSpaceID      sSpaceID; 
    scPageID       sPageID; 
    UInt           sState = 0;

    sSpaceID = aSBCB->mSpaceID; 
    sPageID  = aSBCB->mPageID; 

    /*1. FilterFunc으로 검사하여 대상 BCB를 검사한다.
         대상 BCB가 아니면 SKIP  */
    IDE_TEST_CONT( sObj->mFilter( aSBCB, sObj->mEnv ) 
                    != ID_TRUE,
                    SKIP_SUCCESS );

    sHashChainsHandle = mHashTable.lockHashChainsXLatch( sStat,
                                                         sSpaceID,
                                                         sPageID );
    sState = 1;
    aSBCB->lockBCBMutex( sStat ); 
    sState = 2;

    /* 2. spaceID, pageID로 조건을 통과했는데 FREE 상태라면은 
          그 사이 getvictim 당했을수도 있으니 skip */
    IDE_TEST_CONT( aSBCB->mState == SDS_SBCB_FREE, 
                    SKIP_SUCCESS );

    /* 3. 해당 aSBCB가 다른 것으로 변했는지 여부 검사.
     * 조건을 만족 못한다는 것은 getvictim 당했을수도 있으니..
     * 2와 같은 조건 같아 보이긴 함.  */
    IDE_TEST_CONT( sObj->mFilter( aSBCB, sObj->mEnv ) 
                    != ID_TRUE,
                    SKIP_SUCCESS );
#ifdef DEBUG
    /* 여기까지 왔으면 조건을 다 만족한것. Debug에서만 한번더 검사 */
    IDE_TEST_RAISE( ( sSpaceID != aSBCB->mSpaceID ) ||
                    ( sPageID  != aSBCB->mPageID ),
                    ERROR_INVALID_BCD );
#endif
    /* 4.make free */
    switch( aSBCB->mState )
    {
        case SDS_SBCB_DIRTY:
            if( aSBCB->mCPListNo != SDB_CP_LIST_NONE )
            {
                mCPListSet.remove( sStat, aSBCB );
            }
            mHashTable.removeBCB( aSBCB );

            aSBCB->setFree();
            break;

        case SDS_SBCB_CLEAN:
            mHashTable.removeBCB( aSBCB );

            aSBCB->setFree();
            break;

        case SDS_SBCB_INIOB:
            /* IOB는 상태를 바꿔 놓으면 Flusher가 지움 */
            aSBCB->mState = SDS_SBCB_OLD;
            mHashTable.removeBCB( aSBCB );

            break;  

        case SDS_SBCB_OLD:
            /* Flusher가 지울것임 
             * nothign to do */   
            break;

        case SDS_SBCB_FREE:
        default:
            IDE_RAISE( ERROR_INVALID_BCD );
            break;
    }

    IDE_EXCEPTION_CONT( SKIP_SUCCESS );

    switch ( sState )
    {
        case 2:
            aSBCB->unlockBCBMutex();
        case 1:
            mHashTable.unlockHashChainsLatch( sHashChainsHandle );
            sHashChainsHandle = NULL;
        break;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERROR_INVALID_BCD );
    {
        ideLog::log( IDE_ERR_0,
                "invalid bcd to discard \n"
                "ID      :%"ID_UINT32_FMT"\n"
                "spaseID :%"ID_UINT32_FMT"\n"
                "pageID  :%"ID_UINT32_FMT"\n"
                "state   :%"ID_UINT32_FMT"\n"
                "CPListNo:%"ID_UINT32_FMT"\n"
                "HashTableNo:%"ID_UINT32_FMT"\n",
                aSBCB->mSBCBID,
                aSBCB->mSpaceID,
                aSBCB->mPageID,
                aSBCB->mState,
                aSBCB->mCPListNo,
                aSBCB->mHashBucketNo );

        IDE_DASSERT( 0 );
    }
    IDE_EXCEPTION_END;
    
    switch ( sState )
    {
        case 2:
            aSBCB->unlockBCBMutex();
        case 1:
            mHashTable.unlockHashChainsLatch( sHashChainsHandle );
            sHashChainsHandle = NULL;
        break;
    }

    return IDE_FAILURE;
}

/****************************************************************
 * Description :
 *   Filt조건을 만족하는 모든  BCB들을  discard시킨다.
     for removeFile, shrinkFile ...
 *
 *  aStatistics - [IN]  통계정보
 *  aFilter     - [IN]  aFilter조건에 맞는 BCB만 discard
 *  aFiltAgr    - [IN]  aFilter에 파라미터로 넣어주는 값
 ****************************************************************/
IDE_RC sdsBufferMgr::discardPages( idvSQL        * aStatistics,
                                   sdsFiltFunc     aFilter,
                                   void          * aFiltAgr )
{
    sdsDiscardPageObj sObj;

    sObj.mFilter = aFilter;
    sObj.mStatistics = aStatistics;
    sObj.mEnv = aFiltAgr;

    IDE_ASSERT( mSBufferArea.applyFuncToEachBCBs( discardNoWaitModeFunc,
                                                  &sObj )
                == IDE_SUCCESS );

    return IDE_SUCCESS;
}

/****************************************************************
 * Description :
 *  버퍼매니저의 특정 pid범위에 속하는 모든 BCB를 discard한다.
 *  이때, pid범위에 속하면서 동시에 aSpaceID도 같아야 한다.
 *  removeFilePending, shrinkFilePending 등에서 호출
 *  aStatistics - [IN]  통계정보
 *  aSpaceID    - [IN]  table space ID
 *  aStartPID   - [IN]  pid 범위중 시작
 *  aEndPID     - [IN]  pid 범위중 끝
 ****************************************************************/
IDE_RC sdsBufferMgr::discardPagesInRange( idvSQL    *aStatistics,
                                          scSpaceID  aSpaceID,
                                          scPageID   aStartPID,
                                          scPageID   aEndPID)
{
    sdsBCBRange sRange;

    IDE_TEST_CONT( isServiceable() != ID_TRUE, SKIP )

    sRange.mSpaceID  = aSpaceID;
    sRange.mStartPID = aStartPID;
    sRange.mEndPID   = aEndPID;

    IDE_TEST( sdsBufferMgr::discardPages( aStatistics,
                                          filterRangeBCBs,
                                          (void*)&sRange ) 
              != IDE_SUCCESS );

    IDE_EXCEPTION_CONT( SKIP );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

#if 0 //not used
/****************************************************************
 * Description :
 *  버퍼매니저에 있는 모든 BCB를 discard시킨다.
 *
 *  aStatistics - [IN]  통계정보
 ****************************************************************/
IDE_RC sdsBufferMgr::discardAllPages(idvSQL     *aStatistics)
{
    return sdsBufferMgr::discardPages( aStatistics,
                                       filterAllBCBs,
                                       NULL /* parameter for filter */ ); 
}
#endif

/****************************************************************
 * Description :
 *  filter에 해당하는 페이지를 flush한다. 
 *  discard와 다른건 pageOut인 경우에 flush를 한 이후에 제거를 한다는 점
 * Implementation:
 *   sdsFlushMgr::flushObjectDirtyPages함수를 이용해 flush.
 *   mQueueForFlush : flush 할 BCB list
     mQueueForMakeFree : setfree 할 list
 *  aStatistics - [IN]  통계정보
 *  aFilter     - [IN]  BCB중 aFilter조건에 맞는 BCB만 flush
 *  aFiltArg    - [IN]  aFilter에 파라미터로 넣어주는 값
 ****************************************************************/
IDE_RC sdsBufferMgr::pageOut( idvSQL        * aStatistics,
                              sdsFiltFunc     aFilter,
                              void          * aFiltAgr)
{
    sdsPageOutObj    sObj;
    sdsBCB         * sSBCB;
    smuQueueMgr    * sQueue;
    idBool           sEmpty;

    // BUG-26476
    // 모든 flusher들이 stop 상태이면 abort 에러를 반환한다.
    IDE_TEST_RAISE( sdsFlushMgr::getActiveFlusherCount() == 0,
                    ERROR_ALL_FLUSHERS_STOPPED );

    sObj.mFilter = aFilter;
    sObj.mEnv    = aFiltAgr;

    sObj.mQueueForFlush.initialize( IDU_MEM_SM_SDS, ID_SIZEOF(sdsBCB *) );
    sObj.mQueueForMakeFree.initialize( IDU_MEM_SM_SDS, ID_SIZEOF(sdsBCB *) );

    /* buffer Area에 존재하는 모든 BCB를 돌면서 filt조건에 해당하는 BCB는 모두
     * 1.flush 대상 queue 
     * 2.make free 대상 queue에 모은다.*/
    IDE_ASSERT( mSBufferArea.applyFuncToEachBCBs( makePageOutTargetQueueFunc,
                                                  &sObj )

                == IDE_SUCCESS );

    /* Queue를 flush 한다 */
    IDE_TEST( sdsFlushMgr::flushObjectDirtyPages( aStatistics,
                                                  &(sObj.mQueueForFlush),
                                                  aFilter,
                                                  aFiltAgr )
            != IDE_SUCCESS );
    
    sObj.mQueueForFlush.destroy();

    /* Free 대상 Queue를 받아온다 */
    sQueue = &(sObj.mQueueForMakeFree);

    while(1)
    {
        IDE_ASSERT( sQueue->dequeue( ID_FALSE, // mutex를 잡지 않는다.
                                     (void*)&sSBCB, &sEmpty )
                    == IDE_SUCCESS );

        if( sEmpty == ID_TRUE )
        {
            break;
        }
        /* Free 대상 Queue에서 얻어온 BCB를 검사해서 free 시킴 */
        if( aFilter( sSBCB, aFiltAgr ) == ID_TRUE )
        {
            IDE_TEST( makeFreeBCB( aStatistics, sSBCB ) 
                      != IDE_SUCCESS );
        }
    }

    sQueue->destroy();

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERROR_ALL_FLUSHERS_STOPPED );
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_AllSecondaryFlushersStopped ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/****************************************************************
 * Description :
 *  버퍼매니저에 있는 모든 BCB들에 pageOut을 적용한다.
   -> alter system flush secondary_buffer;
 *  aStatistics - [IN]  통계정보
 ****************************************************************/
IDE_RC sdsBufferMgr::pageOutAll( idvSQL  *aStatistics )
{
    IDE_TEST_CONT( isServiceable() != ID_TRUE, SKIP );
    
    IDE_TEST( pageOut( aStatistics,
                       filterAllBCBs,
                       NULL )
              != IDE_SUCCESS );

    IDE_EXCEPTION_CONT( SKIP );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

#if 0 //not used
/****************************************************************
 * Description :
 *  해당 spaceID에 해당하는 모든 BCB들에게 pageout을 적용한다.
 *
 *  aStatistics - [IN]  통계정보
 *  aSpaceID    - [IN]  table space ID
 ****************************************************************/
IDE_RC sdsBufferMgr::pageOutTBS( idvSQL          *aStatistics,
                                 scSpaceID        aSpaceID )
{
    return pageOut( aStatistics,
                    filterTBSBCBs,
                    (void*)&aSpaceID);
}
#endif

/****************************************************************
 * Description :
 *  버퍼매니저에서 해당 pid범위에 해당하는 BCB를 모두 flush한다.
 *  alter tablespace tbs offline;
 *  aStatistics - [IN]  통계정보
 *  aSpaceID    - [IN]  table space ID
 *  aStartPID   - [IN]  pid 범위중 시작
 *  aEndPID     - [IN]  pid 범위중 끝
 ****************************************************************/
IDE_RC sdsBufferMgr::pageOutInRange( idvSQL         *aStatistics,
                                     scSpaceID       aSpaceID,
                                     scPageID        aStartPID,
                                     scPageID        aEndPID )
{
    sdsBCBRange sRange;

    IDE_TEST_CONT( isServiceable() != ID_TRUE, SKIP );
    
    sRange.mSpaceID     = aSpaceID;
    sRange.mStartPID    = aStartPID;
    sRange.mEndPID      = aEndPID;

    IDE_TEST( pageOut( aStatistics,
                       filterRangeBCBs,
                       (void*)&sRange )
              != IDE_SUCCESS );

    IDE_EXCEPTION_CONT( SKIP );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/****************************************************************
 * Description :
 *  filt에 해당하는 페이지를 flush한다.
 * Implementation:
 *  sdsFlushMgr::flushObjectDirtyPages함수를 이용하기 위해서,
 *  먼저 flush를 해야할 BCB들을 queue에 모아놓는다.
 *  그리고 sdsFlushMgr::flushObjectDirtyPages함수를 이용해 flush.
 *
 *  aStatistics - [IN]  통계정보
 *  aFilter     - [IN]  BCB중 aFilter조건에 맞는 BCB만 flush
 *  aFiltArg    - [IN]  aFilter에 파라미터로 넣어주는 값
 ****************************************************************/
IDE_RC sdsBufferMgr::flushPages( idvSQL            *aStatistics,
                                 sdsFiltFunc        aFilter,
                                 void              *aFiltAgr )
{
    sdsFlushObj sObj;

    // BUG-26476
    // 모든 flusher들이 stop 상태이면 abort 에러를 반환한다.
    IDE_TEST_RAISE( sdsFlushMgr::getActiveFlusherCount() == 0,
                    ERR_ALL_FLUSHERS_STOPPED );

    sObj.mFilter = aFilter;
    sObj.mEnv    = aFiltAgr;

    sObj.mQueue.initialize( IDU_MEM_SM_SDS, ID_SIZEOF(sdsBCB*) );

    IDE_ASSERT( mSBufferArea.applyFuncToEachBCBs( makeFlushTargetQueueFunc,
                                                  &sObj )
                == IDE_SUCCESS );

    IDE_TEST( sdsFlushMgr::flushObjectDirtyPages( aStatistics,
                                                  &(sObj.mQueue),
                                                  aFilter,
                                                  aFiltAgr )
              != IDE_SUCCESS );

    sObj.mQueue.destroy();

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_ALL_FLUSHERS_STOPPED );
    {
        IDE_SET( ideSetErrorCode(smERR_ABORT_AllSecondaryFlushersStopped) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/****************************************************************
 * Description :
 *  버퍼매니저에서 해당 pid범위에 해당하는 BCB를 모두 flush한다.
 *  index bottomUp build
 *  aStatistics - [IN]  통계정보
 *  aSpaceID    - [IN]  table space ID
 *  aStartPID   - [IN]  pid 범위중 시작
 *  aEndPID     - [IN]  pid 범위중 끝
 ****************************************************************/
IDE_RC sdsBufferMgr::flushPagesInRange( idvSQL        * aStatistics,
                                        scSpaceID       aSpaceID,
                                        scPageID        aStartPID,
                                        scPageID        aEndPID )
{
    sdsBCBRange sRange;

    IDE_TEST_CONT( isServiceable() != ID_TRUE, SKIP );

    sRange.mSpaceID     = aSpaceID;
    sRange.mStartPID    = aStartPID;
    sRange.mEndPID      = aEndPID;

    IDE_TEST( flushPages( aStatistics,
                          filterRangeBCBs,
                          (void*)&sRange ) 
              != IDE_SUCCESS );

    IDE_EXCEPTION_CONT( SKIP );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/******************************************************************************
 * Description :
 *  alter system checkpoint;
 *  aStatistics - [IN]  통계정보
 *  aFlushAll   - [IN]  모든 페이지를 flush 해야하는지 여부
 ******************************************************************************/
IDE_RC sdsBufferMgr::flushDirtyPagesInCPList( idvSQL *aStatistics, 
                                              idBool aFlushAll)
{
    IDE_TEST_CONT( isServiceable() != ID_TRUE, SKIP );

    IDE_TEST( sdsFlushMgr::flushDirtyPagesInCPList( aStatistics,
                                                    aFlushAll )
             != IDE_SUCCESS);

    IDE_EXCEPTION_CONT( SKIP );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/******************************************************************************
 * Description : Secondary Buffer에서 mFrame으로 읽어온다( MPR )
 *  aStatistics - [IN]  통계정보
 ******************************************************************************/
IDE_RC sdsBufferMgr::moveUpbySinglePage( idvSQL     * aStatistics,
                                         sdsBCB    ** aSBCB,
                                         sdbBCB     * aBCB,
                                         idBool     * aIsCorruptRead )
{
    scSpaceID   sSpaceID = aBCB->mSpaceID;
    scPageID    sPageID  = aBCB->mPageID;
    idvTime     sBeginTime;
    idvTime     sEndTime;
    ULong       sReadTime         = 0;
    ULong       sCalcChecksumTime = 0;
    SInt        sState            = 0;
    idBool      sIsLock        = ID_FALSE;
    idBool      sIsCorruptRead = ID_FALSE;
    sdsBCB    * sExistSBCB     = *aSBCB;

    sExistSBCB->lockReadIOMutex( aStatistics );
    sIsLock = ID_TRUE;

    if ( ( sExistSBCB->mSpaceID != sSpaceID) ||
         ( sExistSBCB->mPageID != sPageID) )
    { 
        sIsLock = ID_FALSE;
        sExistSBCB->unlockReadIOMutex();

        IDE_TEST( findBCB( aStatistics, sSpaceID, sPageID, &sExistSBCB )
                  != IDE_SUCCESS );

        if ( sExistSBCB == NULL )
        {
            sIsCorruptRead = ID_TRUE;        
            IDE_CONT( SKIP_CORRUPT_READ );
        }
        else 
        {
            *aSBCB = sExistSBCB;

            sExistSBCB->lockReadIOMutex( aStatistics );
            sIsLock = ID_TRUE;
        }
    }

    
    IDE_DASSERT( sExistSBCB->mState != SDS_SBCB_OLD );

    IDV_TIME_GET(&sBeginTime);

    mStatistics.applyBeforeSingleReadPage( aStatistics );
    sState = 1;

    IDE_TEST_RAISE( mSBufferFile.open( aStatistics ) != IDE_SUCCESS,
                    ERROR_PAGE_MOVEUP );

    IDE_TEST_RAISE( mSBufferFile.read( aStatistics,
                                 sExistSBCB->mSBCBID, 
                                 mMeta.getFrameOffset( sExistSBCB->mSBCBID ),
                                 1,
                                 aBCB->mFrame )
                    != IDE_SUCCESS,
                    ERROR_PAGE_MOVEUP );     

    sState = 0;
    mStatistics.applyAfterSingleReadPage( aStatistics );

    IDV_TIME_GET(&sEndTime);
    sReadTime = IDV_TIME_DIFF_MICRO(&sBeginTime, &sEndTime);
    sBeginTime = sEndTime;

    if( sdpPhyPage::checkAndSetPageCorrupted( sExistSBCB->mSpaceID,
                                              aBCB->mFrame ) 
        == ID_TRUE )
    {
        IDE_RAISE( ERROR_PAGE_CORRUPTION );
    }

#ifdef DEBUG
    IDE_TEST_RAISE(
        sdpPhyPage::isConsistentPage( (UChar*) sdpPhyPage::getHdr( aBCB->mFrame ) )
        == ID_FALSE ,
        ERROR_PAGE_INCONSISTENT  );
#endif
    IDE_DASSERT( validate( aBCB ) == IDE_SUCCESS );

    setFrameInfoAfterReadPage( sExistSBCB,
                               aBCB,
                               ID_FALSE ); // check to online tablespace

    IDV_TIME_GET(&sEndTime);
    sCalcChecksumTime = IDV_TIME_DIFF_MICRO(&sBeginTime, &sEndTime);

    mStatistics.applyMultiReadPages( sCalcChecksumTime,
                                     sReadTime,
                                     1 ); /*Page Count */

    sIsLock = ID_FALSE;
    sExistSBCB->unlockReadIOMutex();

    IDE_EXCEPTION_CONT( SKIP_CORRUPT_READ );
    
    if( aIsCorruptRead != NULL )
    {
        *aIsCorruptRead = sIsCorruptRead;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERROR_PAGE_MOVEUP );
    {
        setUnserviceable();
        IDE_SET( ideSetErrorCode( smERR_ABORT_PageMoveUpStopped ) );
    }
    IDE_EXCEPTION( ERROR_PAGE_CORRUPTION );
    {
        switch ( smLayerCallback::getCorruptPageReadPolicy() )
        {
            case SDB_CORRUPTED_PAGE_READ_FATAL :
            IDE_SET( ideSetErrorCode( smERR_FATAL_PageCorrupted,
                                      aBCB->mSpaceID,
                                      aBCB->mPageID ));

                break;
            case SDB_CORRUPTED_PAGE_READ_ABORT :
                IDE_SET( ideSetErrorCode( smERR_ABORT_PageCorrupted,
                                          aBCB->mSpaceID,
                                          aBCB->mPageID));
                break;
            default:
                break;
        }
    }
#ifdef DEBUG
    IDE_EXCEPTION( ERROR_PAGE_INCONSISTENT );
    {
        sdpPhyPageHdr   * sPhyPageHdr;
        sPhyPageHdr = sdpPhyPage::getHdr(aBCB->mFrame);
        ideLog::log( IDE_SM_0,
                 "moveUpbySinglePage : Page Dump -----------------\n"
                 "Page Start Pointer        : %"ID_xPOINTER_FMT"\n"
                 "FrameHdr.mCheckSum        : %"ID_UINT32_FMT"\n"
                 "FrameHdr.mPageLSN.mFileNo : %"ID_UINT32_FMT"\n"
                 "FrameHdr.mPageLSN.mOffset : %"ID_UINT32_FMT"\n"
                 "FrameHdr.mIndexSMONo      : %"ID_vULONG_FMT"\n"
                 "FrameHdr.mSpaceID         : %"ID_UINT32_FMT"\n"
                 "FrameHdr.mBCBPtr          : 0x%"ID_xPOINTER_FMT"\n"
                 "Total Free Size           : %"ID_UINT32_FMT"\n"
                 "Available Free Size       : %"ID_UINT32_FMT"\n"
                 "Logical Hdr Size          : %"ID_UINT32_FMT"\n"
                 "Size of CTL               : %"ID_UINT32_FMT"\n"
                 "Free Space Begin Offset   : %"ID_UINT32_FMT"\n"
                 "Free Space End Offset     : %"ID_UINT32_FMT"\n"
                 "Page Type                 : %"ID_UINT32_FMT"\n"
                 "Page State                : %"ID_UINT32_FMT"\n"
                 "Page ID                   : %"ID_UINT32_FMT"\n"
                 "is Consistent             : %"ID_UINT32_FMT"\n"
                 "LinkState                 : %"ID_UINT32_FMT"\n"
                 "ParentInfo.mParentPID     : %"ID_UINT32_FMT"\n"
                 "ParentInfo.mIdxInParent   : %"ID_UINT32_FMT"\n"
                 "Prev Page ID, Nex Page ID : %"ID_UINT32_FMT", %"ID_UINT32_FMT"\n"
                 "TableOID                  : %"ID_vULONG_FMT"\n"
                 "Seq No                    : %"ID_UINT64_FMT"\n"
                 "---------------------------------------------\n",
                 (UChar*)sPhyPageHdr,
                 sPhyPageHdr->mFrameHdr.mCheckSum,
                 sPhyPageHdr->mFrameHdr.mPageLSN.mFileNo,
                 sPhyPageHdr->mFrameHdr.mPageLSN.mOffset,
                 sPhyPageHdr->mFrameHdr.mIndexSMONo,
                 sPhyPageHdr->mFrameHdr.mSpaceID,
                 sPhyPageHdr->mFrameHdr.mBCBPtr,
                 sPhyPageHdr->mTotalFreeSize,
                 sPhyPageHdr->mAvailableFreeSize,
                 sPhyPageHdr->mLogicalHdrSize,
                 sPhyPageHdr->mSizeOfCTL,
                 sPhyPageHdr->mFreeSpaceBeginOffset,
                 sPhyPageHdr->mFreeSpaceEndOffset,
                 sPhyPageHdr->mPageType,
                 sPhyPageHdr->mPageState,
                 sPhyPageHdr->mPageID,
                 sPhyPageHdr->mIsConsistent,
                 sPhyPageHdr->mLinkState,
                 sPhyPageHdr->mParentInfo.mParentPID,
                 sPhyPageHdr->mParentInfo.mIdxInParent,
                 sPhyPageHdr->mListNode.mPrev,
                 sPhyPageHdr->mListNode.mNext,
                 sPhyPageHdr->mTableOID,
                 sPhyPageHdr->mSeqNo );

        IDE_SET( ideSetErrorCode( smERR_ABORT_INCONSISTENT_PAGE,
                                  aBCB->mSpaceID,
                                  aBCB->mPageID) );
    }
#endif
    IDE_EXCEPTION_END;

    if( sIsLock == ID_TRUE )
    {
        sIsLock = ID_FALSE;
        sExistSBCB->unlockReadIOMutex();
    }

    if( sState != 0 )
    {
        mStatistics.applyAfterSingleReadPage( aStatistics );
    }

    return IDE_FAILURE;
}

/***********************************************************************
 * Descrition:
 *  디스크에서 페이지를 읽어온 후, BCB와 읽어온 frame에 여러가지 정보를 설정한다
 *
 *  aBCB            - [IN]  BCB
 *  aChkOnlineTBS   - [IN] TBS Online 여부를 확인해서 SMO NO 갱신 (Yes/No)
 **********************************************************************/
void sdsBufferMgr::setFrameInfoAfterReadPage( sdsBCB * aSBCB,
                                              sdbBCB * aBCB,
                                              idBool   aChkOnlineTBS )
{
    aBCB->mPageType = smLayerCallback::getPhyPageType( aBCB->mFrame );
    IDV_TIME_GET(&aBCB->mCreateOrReadTime);

    /*  페이지를 읽은 후 반드시 세팅해줘야 하는 정보들 */
    sdbBCB::setBCBPtrOnFrame( (sdbFrameHdr*)aBCB->mFrame, aBCB );
    sdbBCB::setSpaceIDOnFrame( (sdbFrameHdr*)aBCB->mFrame, 
                                aBCB->mSpaceID );
    /* Secondaty Buffer의 SBCB가 가르키는 BCB를 지금 읽은 BCB로 변경 */ 
    aSBCB->mBCB = aBCB;

    // BUG-47429 SMO NO가 0이 아닌 경우에만 SMO NO 초기화 필요 여부를 확인한다.
    if ( smLayerCallback::getIndexSMONo( aBCB->mFrame ) != 0 )
    {
        // SMO no를 초기화해야 한다.
        smLayerCallback::resetIndexSMONo( aBCB->mFrame,
                                          aBCB->mSpaceID,
                                          aChkOnlineTBS );
    }
}

/****************************************************************
 * Description :
 *  현재 버퍼에 존재하는 BCB들의 recoveryLSN중 가장 작은 값을
 *  리턴한다.
 *
 *  aStatistics - [IN]  통계정보
 *  aRet        - [OUT] 요청한 min recoveryLSN
 ****************************************************************/
void sdsBufferMgr::getMinRecoveryLSN( idvSQL * aStatistics,
                                      smLSN  * aMinLSN )
{
    smLSN         sFlusherMinLSN;
    smLSN         sCPListMinLSN;

    if( isServiceable() != ID_TRUE )
    {
        SM_LSN_MAX( *aMinLSN );
    }
    else 
    {
        /* CPlist에 매달린 BCB 중 가장작은  recoveryLSN을 얻는다. */
        mCPListSet.getMinRecoveryLSN( aStatistics, &sCPListMinLSN );
        /* IOB에 복사된 BCB들 중 가장 작은 recoveryLSN을 얻는다. */
        sdsFlushMgr::getMinRecoveryLSN(aStatistics, &sFlusherMinLSN );

        if ( smLayerCallback::isLSNLT( &sCPListMinLSN,
                                       &sFlusherMinLSN )
             == ID_TRUE)
        {
            SM_GET_LSN( *aMinLSN, sCPListMinLSN );
        }
        else
        {
            SM_GET_LSN( *aMinLSN, sFlusherMinLSN );
        }
    }
}

/****************************************************************
 * Description : Secondary Buffer의 node확인 및 생성
    단계 : off -> identifiable -> serviceable
 ************** **************************************************/
IDE_RC sdsBufferMgr::identify( idvSQL * aStatistics )
{
    if( isIdentifiable() == ID_TRUE )
    {
        IDE_TEST( mSBufferFile.identify( aStatistics ) != IDE_SUCCESS );

        setServiceable();
       
        smLayerCallback::setSBufferServiceable();    
    }
    else 
    {
        /* nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/****************************************************************
 * Description:BCB 를 지우자 
 *  지울수 없으면 OLD로 상태 변경 
 *  aSBCB        [IN] : 지울 BCB
 ****************************************************************/
IDE_RC sdsBufferMgr::removeBCB( idvSQL     * aStatistics,
                                sdsBCB     * aSBCB )
{
    sdsHashChainsLatchHandle  * sHashChainsHandle = NULL;
    scSpaceID     sSpaceID; 
    scPageID      sPageID; 
    UInt          sState = 0;

    IDE_ASSERT( aSBCB != NULL );

    sSpaceID = aSBCB->mSpaceID; 
    sPageID  = aSBCB->mPageID; 

    sHashChainsHandle = mHashTable.lockHashChainsXLatch( aStatistics,
                                                         sSpaceID,
                                                         sPageID );
    sState = 1;
    aSBCB->lockBCBMutex( aStatistics );
    sState = 2;

    /* 넘겨받은 BCB와 lock 잡은 BCB가 같아야 한다. */
    IDE_TEST_CONT( (( sSpaceID != aSBCB->mSpaceID ) ||
                     ( sPageID  != aSBCB->mPageID ) ) ,
                    SKIP_REMOVE_BCB );

    switch( aSBCB->mState )
    {
        case SDS_SBCB_DIRTY:
            if( aSBCB->mCPListNo != SDB_CP_LIST_NONE )
            {
                mCPListSet.remove( aStatistics, aSBCB );
            }
            mHashTable.removeBCB( aSBCB );
            aSBCB->setFree();
            break;

        case SDS_SBCB_CLEAN:
            mHashTable.removeBCB( aSBCB );
            aSBCB->setFree();
            break;

        case SDS_SBCB_INIOB:
            aSBCB->mState = SDS_SBCB_OLD;
            mHashTable.removeBCB( aSBCB );
            break;  

        case SDS_SBCB_OLD:
        case SDS_SBCB_FREE:
            /* nothing to do */
            break;

        default:
            IDE_RAISE( ERROR_INVALID_BCD );
            break;
    }

    IDE_EXCEPTION_CONT( SKIP_REMOVE_BCB );

    sState = 1;
    aSBCB->unlockBCBMutex();
    sState = 0;
    mHashTable.unlockHashChainsLatch( sHashChainsHandle );
    sHashChainsHandle = NULL;

    return IDE_SUCCESS;
    
    IDE_EXCEPTION( ERROR_INVALID_BCD );
    {
        ideLog::log( IDE_ERR_0,
                     "Invalid bcd to remove \n"
                     "ID :%u\n"
                     "spaseID :%u\n"
                     "pageID  :%u\n"
                     "state   :%u\n"
                     "CPListNo:%u\n"
                     "HashTableNo:%u\n",
                     aSBCB->mSBCBID,
                     aSBCB->mSpaceID,
                     aSBCB->mPageID,
                     aSBCB->mState,
                     aSBCB->mCPListNo,
                     aSBCB->mHashBucketNo );

        IDE_DASSERT( 0 );
    }
    IDE_EXCEPTION_END;
    
    switch( sState )
    { 
        case 2:
            aSBCB->unlockBCBMutex(); 
        case 1:
            mHashTable.unlockHashChainsLatch( sHashChainsHandle );
            sHashChainsHandle = NULL;
            break;
    }

    return IDE_FAILURE;
}

#ifdef DEBUG
/***********************************************************************
 * Description : 검증용
 **********************************************************************/
IDE_RC sdsBufferMgr::validate( sdbBCB  * aBCB )
{
    IDE_DASSERT( aBCB != NULL );

    sdpPhyPageHdr   * sPhyPageHdr;
    smcTableHeader  * sTableHeader;
    smOID             sTableOID;

    sPhyPageHdr = sdpPhyPage::getHdr(aBCB->mFrame);

    if( sdpPhyPage::getPageType( sPhyPageHdr ) == SDP_PAGE_DATA )
    {
        if( sPhyPageHdr->mPageID != aBCB->mPageID )
        {
            ideLog::log( IDE_ERR_0,
                     "----------------- Page Dump -----------------\n"
                     "Page Start Pointer        : %"ID_xPOINTER_FMT"\n"
                     "FrameHdr.mCheckSum        : %"ID_UINT32_FMT"\n"
                     "FrameHdr.mPageLSN.mFileNo : %"ID_UINT32_FMT"\n"
                     "FrameHdr.mPageLSN.mOffset : %"ID_UINT32_FMT"\n"
                     "FrameHdr.mIndexSMONo      : %"ID_vULONG_FMT"\n"
                     "FrameHdr.mSpaceID         : %"ID_UINT32_FMT"\n"
                     "FrameHdr.mBCBPtr          : 0x%"ID_xPOINTER_FMT"\n"
                     "Total Free Size           : %"ID_UINT32_FMT"\n"
                     "Available Free Size       : %"ID_UINT32_FMT"\n"
                     "Logical Hdr Size          : %"ID_UINT32_FMT"\n"
                     "Size of CTL               : %"ID_UINT32_FMT"\n"
                     "Free Space Begin Offset   : %"ID_UINT32_FMT"\n"
                     "Free Space End Offset     : %"ID_UINT32_FMT"\n"
                     "Page Type                 : %"ID_UINT32_FMT"\n"
                     "Page State                : %"ID_UINT32_FMT"\n"
                     "Page ID                   : %"ID_UINT32_FMT"\n"
                     "is Consistent             : %"ID_UINT32_FMT"\n"
                     "LinkState                 : %"ID_UINT32_FMT"\n"
                     "ParentInfo.mParentPID     : %"ID_UINT32_FMT"\n"
                     "ParentInfo.mIdxInParent   : %"ID_UINT32_FMT"\n"
                     "Prev Page ID, Nex Page ID : %"ID_UINT32_FMT", %"ID_UINT32_FMT"\n"
                     "TableOID                  : %"ID_vULONG_FMT"\n"
                     "Seq No                    : %"ID_UINT64_FMT"\n"
                     "---------------------------------------------\n",
                     (UChar*)sPhyPageHdr,
                     sPhyPageHdr->mFrameHdr.mCheckSum,
                     sPhyPageHdr->mFrameHdr.mPageLSN.mFileNo,
                     sPhyPageHdr->mFrameHdr.mPageLSN.mOffset,
                     sPhyPageHdr->mFrameHdr.mIndexSMONo,
                     sPhyPageHdr->mFrameHdr.mSpaceID,
                     sPhyPageHdr->mFrameHdr.mBCBPtr,
                     sPhyPageHdr->mTotalFreeSize,
                     sPhyPageHdr->mAvailableFreeSize,
                     sPhyPageHdr->mLogicalHdrSize,
                     sPhyPageHdr->mSizeOfCTL,
                     sPhyPageHdr->mFreeSpaceBeginOffset,
                     sPhyPageHdr->mFreeSpaceEndOffset,
                     sPhyPageHdr->mPageType,
                     sPhyPageHdr->mPageState,
                     sPhyPageHdr->mPageID,
                     sPhyPageHdr->mIsConsistent,
                     sPhyPageHdr->mLinkState,
                     sPhyPageHdr->mParentInfo.mParentPID,
                     sPhyPageHdr->mParentInfo.mIdxInParent,
                     sPhyPageHdr->mListNode.mPrev,
                     sPhyPageHdr->mListNode.mNext,
                     sPhyPageHdr->mTableOID,
                     sPhyPageHdr->mSeqNo );
            IDE_DASSERT( 0 );
        }

        sTableOID = sdpPhyPage::getTableOID( (UChar*)sdpPhyPage::getHdr( aBCB->mFrame ));

        (void)smcTable::getTableHeaderFromOID( sTableOID,
                                               (void**)&sTableHeader );

        if( sTableOID != sTableHeader->mSelfOID )
        {
            ideLog::log( IDE_ERR_0,
                     "----------------- Page Dump -----------------\n"
                     "Page Start Pointer        : %"ID_xPOINTER_FMT"\n"
                     "FrameHdr.mCheckSum        : %"ID_UINT32_FMT"\n"
                     "FrameHdr.mPageLSN.mFileNo : %"ID_UINT32_FMT"\n"
                     "FrameHdr.mPageLSN.mOffset : %"ID_UINT32_FMT"\n"
                     "FrameHdr.mIndexSMONo      : %"ID_vULONG_FMT"\n"
                     "FrameHdr.mSpaceID         : %"ID_UINT32_FMT"\n"
                     "FrameHdr.mBCBPtr          : 0x%"ID_xPOINTER_FMT"\n"
                     "Total Free Size           : %"ID_UINT32_FMT"\n"
                     "Available Free Size       : %"ID_UINT32_FMT"\n"
                     "Logical Hdr Size          : %"ID_UINT32_FMT"\n"
                     "Size of CTL               : %"ID_UINT32_FMT"\n"
                     "Free Space Begin Offset   : %"ID_UINT32_FMT"\n"
                     "Free Space End Offset     : %"ID_UINT32_FMT"\n"
                     "Page Type                 : %"ID_UINT32_FMT"\n"
                     "Page State                : %"ID_UINT32_FMT"\n"
                     "Page ID                   : %"ID_UINT32_FMT"\n"
                     "is Consistent             : %"ID_UINT32_FMT"\n"
                     "LinkState                 : %"ID_UINT32_FMT"\n"
                     "ParentInfo.mParentPID     : %"ID_UINT32_FMT"\n"
                     "ParentInfo.mIdxInParent   : %"ID_UINT32_FMT"\n"
                     "Prev Page ID, Nex Page ID : %"ID_UINT32_FMT", %"ID_UINT32_FMT"\n"
                     "TableOID                  : %"ID_vULONG_FMT"\n"
                     "Seq No                    : %"ID_UINT64_FMT"\n"
                     "---------------------------------------------\n",

                     (UChar*)sPhyPageHdr,
                     sPhyPageHdr->mFrameHdr.mCheckSum,
                     sPhyPageHdr->mFrameHdr.mPageLSN.mFileNo,
                     sPhyPageHdr->mFrameHdr.mPageLSN.mOffset,
                     sPhyPageHdr->mFrameHdr.mIndexSMONo,
                     sPhyPageHdr->mFrameHdr.mSpaceID,
                     sPhyPageHdr->mFrameHdr.mBCBPtr,
                     sPhyPageHdr->mTotalFreeSize,
                     sPhyPageHdr->mAvailableFreeSize,
                     sPhyPageHdr->mLogicalHdrSize,
                     sPhyPageHdr->mSizeOfCTL,
                     sPhyPageHdr->mFreeSpaceBeginOffset,
                     sPhyPageHdr->mFreeSpaceEndOffset,
                     sPhyPageHdr->mPageType,
                     sPhyPageHdr->mPageState,
                     sPhyPageHdr->mPageID,
                     sPhyPageHdr->mIsConsistent,
                     sPhyPageHdr->mLinkState,
                     sPhyPageHdr->mParentInfo.mParentPID,
                     sPhyPageHdr->mParentInfo.mIdxInParent,
                     sPhyPageHdr->mListNode.mPrev,
                     sPhyPageHdr->mListNode.mNext,
                     sPhyPageHdr->mTableOID,
                     sPhyPageHdr->mSeqNo );
            IDE_DASSERT( 0 );
        }
    }
    return IDE_SUCCESS;
}
#endif
