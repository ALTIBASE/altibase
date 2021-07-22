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
 * $Id $
 **********************************************************************/

#include <sdtWorkArea.h>
#include <sdtWAMap.h>
#include <smuProperty.h>
#include <sdpDef.h>
#include <sdtTempPage.h>
#include <sdpTableSpace.h>
#include <smiMisc.h>


/******************************************************************
 * Segment
 ******************************************************************/
/**************************************************************************
 * Description :
 * Segment를 Size크기대로 WExtent를 가진체 생성한다
 *
 * <IN>
 * aStatistics - 통계정보
 * aStatsPtr   - TempTable용 통계정보
 * aLogging    - Logging여부 (현재는 무효함)
 * aSpaceID    - Extent를 가져올 TablespaceID
 * aSize       - 생성할 WA의 크기
 * <OUT>
 * aWASegment  - 생성된 WASegment
 ***************************************************************************/
IDE_RC sdtWASegment::createWASegment(idvSQL             * aStatistics,
                                     smiTempTableStats ** aStatsPtr,
                                     idBool               aLogging,
                                     scSpaceID            aSpaceID,
                                     ULong                aSize,
                                     sdtWASegment      ** aWASegment )
{
    sdtWASegment   * sWASeg = NULL;
    sdtWCB         * sWCBPtr;
    UInt             sExtentCount;
    ULong            sExtentSize;
    sdtWAGroup     * sInitGrpInfo;
    UChar          * sExtent;
    UInt             sState = 0;
    UInt             i;
    UInt             sHashBucketDensity;
    UInt             sHashBucketCnt;

    /* 아직 Logging 기법 사용 안함 */
    IDE_ASSERT( aLogging == ID_FALSE );

    /* 올림 하여 할당받음 */
    sExtentSize  = ( SD_PAGE_SIZE * SDT_WAEXTENT_PAGECOUNT );
    sExtentCount = ( aSize + sExtentSize - 1 ) / sExtentSize;
    sHashBucketDensity = smuProperty::getTempHashBucketDensity();

    IDE_TEST( sdtWorkArea::allocWASegmentAndExtent( aStatistics,
                                                    &sWASeg,
                                                    aStatsPtr,
                                                    sExtentCount)
              != IDE_SUCCESS);
    sState = 1;
    IDE_TEST( sdtWorkArea::assignWAFlusher( sWASeg ) != IDE_SUCCESS );
    sState = 2;

    /***************************** initialize ******************************/
    sWASeg->mSpaceID                 = aSpaceID;
    sWASeg->mFlushQueue->mStatistics = aStatistics;
    sWASeg->mStatsPtr                = aStatsPtr;
    sWASeg->mNExtentCount            = 0;
    sWASeg->mNPageCount              = 0;
    sWASeg->mLastFreeExtent          = NULL;
    sWASeg->mPageSeqInLFE            = 0;
    sWASeg->mDiscardPage             = ID_FALSE;
    sWASeg->mStatistics              = aStatistics;
    sWASeg->mNPageHashPtr            = NULL;
    sWASeg->mNPageHashBucketCnt      = 0;
    /*************************** init first wcb ****************************/
    sExtent = getWAExtentPtr( sWASeg, 0 );
    sWASeg->mHintWCBPtr      = getWCBInExtent( sExtent, 0 );
    initWCB( sWASeg, sWASeg->mHintWCBPtr, 0 );

    /***************************** InitGroup *******************************/
    for( i = 0 ; i< SDT_WAGROUPID_MAX ; i ++ )
    {
        sWASeg->mGroup[ i ].mPolicy = SDT_WA_REUSE_NONE;
    }

    /* InitGroup을 생성한다. */
    sInitGrpInfo = &sWASeg->mGroup[ 0 ];
    /* Segment 이후 WAExtentPtr가 배치된 후 Range의 시작이다. */
    sInitGrpInfo->mBeginWPID =
        ( ( ID_SIZEOF( sdtWASegment ) +
            ID_SIZEOF(UChar*) * sWASeg->mWAExtentCount )
          / SD_PAGE_SIZE ) + 1;
    sInitGrpInfo->mEndWPID = sExtentCount * SDT_WAEXTENT_PAGECOUNT;
    IDE_ERROR( sInitGrpInfo->mBeginWPID < sInitGrpInfo->mEndWPID );

    for( i = 0 ; i< sExtentCount * SDT_WAEXTENT_PAGECOUNT ; i ++ )
    {
        sWCBPtr = getWCBInternal( sWASeg, i );
        initWCB( sWASeg, sWCBPtr, i );
    }

    sInitGrpInfo->mPolicy     = SDT_WA_REUSE_INMEMORY;
    sInitGrpInfo->mMapHdr     = NULL;
    sInitGrpInfo->mReuseWPID1 = 0;
    sInitGrpInfo->mReuseWPID2 = 0;

    /***************************** NPageMgr *********************************/
    IDE_TEST( sdtWAMap::create( sWASeg,
                                SDT_WAGROUPID_INIT,
                                SDT_WM_TYPE_EXTDESC,
                                smuProperty::getTempMaxPageCount()
                                / SDT_WAEXTENT_PAGECOUNT,
                                1, /* aVersionCount */
                                &sWASeg->mNExtentMap )
              != IDE_SUCCESS );
    IDE_TEST( sWASeg->mFreeNPageStack.initialize( IDU_MEM_SM_TEMP,
                                                  ID_SIZEOF( scPageID ) )
              != IDE_SUCCESS );
    sState = 3;
    /***************************** Hash Bucket ******************************/
    /* BUG-37741-성능이슈
     * NPage를 찾는 HashTable의 크기를 프로퍼티로 조정함*/
    sWASeg->mNPageHashBucketCnt =
        (sExtentCount * SDT_WAEXTENT_PAGECOUNT)/sHashBucketDensity;

    /* BUG-40608
     * mNPageHashBucketCnt가 최소 1 이상이 되도록 보정 */
    if( sWASeg->mNPageHashBucketCnt < 1 )
    {
        sWASeg->mNPageHashBucketCnt = 1;
    }
    else
    {
        /* do nothing */
    }

    sHashBucketCnt = sWASeg->mNPageHashBucketCnt;

    /* sdtWASegment_createWASegment_malloc_NPageHashPtr.tc */
    IDU_FIT_POINT_RAISE("sdtWASegment::createWASegment::malloc::NPageHashPtr", insufficient_memory);
    IDE_TEST_RAISE( iduMemMgr::malloc(
                        IDU_MEM_SM_TEMP,
                        (ULong)ID_SIZEOF(sdtWCB*)
                        * sHashBucketCnt,
                        (void**)&(sWASeg->mNPageHashPtr))
                    != IDE_SUCCESS,
                    insufficient_memory );
    sState = 4;
    idlOS::memset( sWASeg->mNPageHashPtr,
                   0x00,
                   (size_t)ID_SIZEOF( sdtWCB* ) * sHashBucketCnt );

    (*aWASegment) = sWASeg;

    return IDE_SUCCESS;

    IDE_EXCEPTION( insufficient_memory );
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_InsufficientMemory));
    }
    IDE_EXCEPTION_END;

    switch( sState )
    {
        case 4:
            IDE_ASSERT( iduMemMgr::free( sWASeg->mNPageHashPtr ) == IDE_SUCCESS );
        case 3:
            sWASeg->mFreeNPageStack.destroy();
        case 2:
            sdtWorkArea::releaseWAFlusher( sWASeg->mFlushQueue );
        case 1:
            sdtWorkArea::freeWASegmentAndExtent( sWASeg );
            break;
        default:
            break;
    }

    (*aWASegment) = NULL;

    return IDE_FAILURE;
}

/**************************************************************************
 * Description :
 * Segment의 크기를 받는다.
 ***************************************************************************/
UInt sdtWASegment::getWASegmentPageCount(sdtWASegment * aWASegment )
{
    return aWASegment->mWAExtentCount * SDT_WAEXTENT_PAGECOUNT;
}

/**************************************************************************
 * Description :
 * Segment를 Drop하고 안의 Extent도 반환한다.
 *
 * <IN>
 * aWASegment     - 대상 WASegment
 * aWait4Flush    - Dirty한 Page들을 Flush될때까지 기다릴 것인가.
 ***************************************************************************/
IDE_RC sdtWASegment::dropWASegment(sdtWASegment * aWASegment,
                                   idBool         aWait4Flush)
{
    if( aWASegment != NULL )
    {
        if( aWASegment->mNPageHashPtr != NULL )
        {
            IDE_TEST( iduMemMgr::free( aWASegment->mNPageHashPtr ) != IDE_SUCCESS );
        }
        IDE_TEST( dropAllWAGroup( aWASegment, aWait4Flush ) != IDE_SUCCESS );

        /* FlushQueue에게 일이 끝았음을 알림.
         * 이후 sdtWASegment::flusherRun에서 정리해줌. */
        aWASegment->mFlushQueue->mFQDone = ID_TRUE;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**************************************************************************
 * Description :
 * WAExtent들을 Segment에 추가한다.
 *
 * <IN>
 * aWASegment     - 대상 WASegment
 * aWAExtentPtr   - 추가할 ExtentPtr
 ***************************************************************************/
IDE_RC sdtWASegment::addWAExtentPtr(sdtWASegment * aWASegment,
                                    UChar        * aWAExtentPtr )
{
    IDE_TEST( setWAExtentPtr( aWASegment,
                              aWASegment->mWAExtentCount,
                              aWAExtentPtr )
              != IDE_SUCCESS );

    aWASegment->mWAExtentCount ++;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**************************************************************************
 * Description :
 * aIdx번째 WAExtent의 위치(Pointer)를 설정한다.
 * Dump전용으로, Dump한 File을 불러왔을때 WAExtentPtr를 불러온 메모리에 맞게
 * Pointer 주소를 재조정하기 위함이다.
 *
 * <IN>
 * aWASegment     - 대상 WASegment
 * aIdx           - Extent ID
 * aWAExtentPtr   - 추가할 ExtentPtr
 ***************************************************************************/
IDE_RC sdtWASegment::setWAExtentPtr(sdtWASegment  * aWASegment,
                                    UInt            aIdx,
                                    UChar         * aWAExtentPtr )
{
    UInt     sOffset;
    UChar ** sSlotPtr;

    sOffset = ID_SIZEOF( sdtWASegment ) + ID_SIZEOF( UChar* ) * aIdx;

    /* 0번 Extent에 들어야 한다. */
    IDE_ERROR( getExtentIdx(sOffset / SD_PAGE_SIZE) == 0 );
    IDE_ERROR( ( sOffset % ID_SIZEOF( UChar *) )  == 0 );

    /* WASegment는 반드시 WAExtent의 가장 앞쪽에 배치되고,
     * 0번 Extent내에 모두 배치되어 연속성을 가지기 때문에
     * 탐색이 가능하다 */
    sSlotPtr  = (UChar**)(((UChar*)aWASegment) + sOffset);
    *sSlotPtr = aWAExtentPtr;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/******************************************************************
 * Group
 ******************************************************************/
/**************************************************************************
 * Description :
 * Segment내 Group을 생성한다. 이때 InitGroup으로부터 Extent를 가져온다.
 * Size가 0이면, InitGroup전체크기로 설정한다.
 *
 * <IN>
 * aWASegment     - 대상 WASegment
 * aWAGroupID     - 생성할 Group ID
 * aPageCount     - 대상 Group이 가질 Page 개수
 * aPolicy        - 대상 Group이 사용할 FreePage 재활용 정책
 ***************************************************************************/
IDE_RC sdtWASegment::createWAGroup( sdtWASegment     * aWASegment,
                                    sdtWAGroupID       aWAGroupID,
                                    UInt               aPageCount,
                                    sdtWAReusePolicy   aPolicy )
{
    sdtWAGroup     *sInitGrpInfo=getWAGroupInfo(aWASegment,SDT_WAGROUPID_INIT);
    sdtWAGroup     *sGrpInfo    =getWAGroupInfo(aWASegment,aWAGroupID);

    IDE_ERROR( sGrpInfo->mPolicy == SDT_WA_REUSE_NONE );

    /* 크기를 지정하지 않았으면, 남은 용량을 전부 부여함 */
    if( aPageCount == 0 )
    {
        aPageCount = getAllocableWAGroupPageCount( aWASegment,
                                                   SDT_WAGROUPID_INIT );
    }

    /* 다음과 같이 InitGroup에서 때어줌
     * <---InitGroup---------------------->
     * <---InitGroup---><----CurGroup----->*/
    IDE_ERROR( sInitGrpInfo->mEndWPID >= aPageCount );  /* 음수체크 */
    sGrpInfo->mBeginWPID = sInitGrpInfo->mEndWPID - aPageCount;
    sGrpInfo->mEndWPID   = sInitGrpInfo->mEndWPID;
    sInitGrpInfo->mEndWPID -= aPageCount;
    IDE_ERROR( sInitGrpInfo->mBeginWPID <= sInitGrpInfo->mEndWPID );

    sGrpInfo->mHintWPID       = SC_NULL_PID;
    sGrpInfo->mPolicy         = aPolicy;

    IDE_TEST( clearWAGroup( aWASegment, aWAGroupID )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**************************************************************************
 * Description :
 * WAGroup을 재활용 할 수 있도록 리셋한다.
 *
 * <IN>
 * aWASegment     - 대상 WASegment
 * aWAGroupID     - 대상 Group ID
 * aWait4Flush    - Dirty한 Page들을 Flush될때까지 기다릴 것인가.
 ***************************************************************************/
IDE_RC sdtWASegment::resetWAGroup( sdtWASegment      * aWASegment,
                                   sdtWAGroupID        aWAGroupID,
                                   idBool              aWait4Flush )
{
    sdtWAGroup     * sGrpInfo = getWAGroupInfo(aWASegment,aWAGroupID);
    UInt             i;

    if( sGrpInfo->mPolicy != SDT_WA_REUSE_NONE )
    {
        /* 여기서 HintPage를 초기화하여 Unfix하지 않으면,
         * 이후 makeInit단계에서 hintPage를 unassign할 수 없어
         * 오류가 발생한다. */
        IDE_TEST( setHintWPID( aWASegment, aWAGroupID, SC_NULL_PID )
                  != IDE_SUCCESS );

        if( aWait4Flush == ID_TRUE )
        {
            IDE_TEST( writeDirtyWAPages( aWASegment,
                                         aWAGroupID )
                      != IDE_SUCCESS );
            for( i = sGrpInfo->mBeginWPID ; i < sGrpInfo->mEndWPID ; i ++ )
            {
                /* 전부 Init상태로 만든다. */
                IDE_TEST( makeInitPage( aWASegment,
                                        i,
                                        aWait4Flush )
                          != IDE_SUCCESS );
            }
        }
        IDE_TEST( clearWAGroup( aWASegment, aWAGroupID ) != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**************************************************************************
 * Description :
 * WAGroup을 깨끗히 초기화한다.
 *
 * <IN>
 * aWASegment     - 대상 WASegment
 * aWAGroupID     - 대상 Group ID
 ***************************************************************************/
IDE_RC sdtWASegment::clearWAGroup( sdtWASegment      * aWASegment,
                                   sdtWAGroupID        aWAGroupID )
{
    sdtWAGroup     * sGrpInfo = getWAGroupInfo(aWASegment,aWAGroupID);

    sGrpInfo->mMapHdr         = NULL;
    IDE_TEST( setHintWPID( aWASegment, aWAGroupID, SC_NULL_PID )
              != IDE_SUCCESS );

    /* 재활용 정책에 따라 초기값 설정함. */
    switch( sGrpInfo->mPolicy  )
    {
        case SDT_WA_REUSE_INMEMORY:
            initInMemoryGroup( sGrpInfo );
            break;
        case SDT_WA_REUSE_FIFO:
            initFIFOGroup( sGrpInfo );
            break;
        case SDT_WA_REUSE_LRU:
            initLRUGroup( aWASegment, sGrpInfo );
            break;
        case SDT_WA_REUSE_NONE:
        default:
            IDE_ERROR( 0 );
            break;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**************************************************************************
 * Description :
 * InMemoryGroup에 대한 초기화. 페이지 재활용 자료구조를 설정한다.
 *
 * <IN>
 * aWASegment     - 대상 WASegment
 * aGrpInfo       - 대상 Group
 ***************************************************************************/
void sdtWASegment::initInMemoryGroup( sdtWAGroup     * aGrpInfo )
{
    /* 뒤에서 앞으로 가며 페이지 활용함.
     * 왜냐하면 InMemoryGroup은 WAMap을 장착할 수 있으며,
     * WAMap은 앞에서 뒤쪽으로 확장하면서 부딪힐 수 있기 때문임 */
    aGrpInfo->mReuseWPID1 = aGrpInfo->mEndWPID - 1;
    aGrpInfo->mReuseWPID2 = SD_NULL_PID;
}

/**************************************************************************
 * Description :
 * FIFOGroup에 대한 초기화. 페이지 재활용 자료구조를 설정한다.
 *
 * <IN>
 * aWASegment     - 대상 WASegment
 * aGrpInfo       - 대상 Group
 ***************************************************************************/
void sdtWASegment::initFIFOGroup( sdtWAGroup     * aGrpInfo )
{
    /* 앞에서 뒤로 가며 페이지 활용함. MultiBlockWrite를 유도하기위해. */
    aGrpInfo->mReuseWPID1 = aGrpInfo->mBeginWPID;
    aGrpInfo->mReuseWPID2 = SD_NULL_PID;
}

/**************************************************************************
 * Description :
 * LRUGroup에 대한 초기화. 페이지 재활용 자료구조를 설정한다.
 *
 * <IN>
 * aWASegment     - 대상 WASegment
 * aGrpInfo       - 대상 Group
 ***************************************************************************/
void sdtWASegment::initLRUGroup( sdtWASegment   * aWASegment,
                                 sdtWAGroup      * aGrpInfo )
{
    sdtWCB         * sWCBPtr;
    UInt             i;

    aGrpInfo->mReuseWPID1 = aGrpInfo->mEndWPID - 1;
    aGrpInfo->mReuseWPID2 = aGrpInfo->mBeginWPID;

    /* MultiBlockWrite를 유도하기위해 앞에서 뒤로 가며 페이지 활용하도록
     * 일단 리스트를 구성해둠*/
    for( i = aGrpInfo->mBeginWPID ;
         i < aGrpInfo->mEndWPID ;
         i ++ )
    {
        sWCBPtr = getWCB( aWASegment, i );

        sWCBPtr->mLRUNextPID = i - 1;
        sWCBPtr->mLRUPrevPID = i + 1;
    }
    /* 첫/끝 page의 Link를 정리해줌 */
    sWCBPtr              = getWCB( aWASegment, aGrpInfo->mBeginWPID );
    sWCBPtr->mLRUNextPID = SC_NULL_PID;
    sWCBPtr              = getWCB( aWASegment, aGrpInfo->mEndWPID - 1);
    sWCBPtr->mLRUPrevPID = SC_NULL_PID;
}

/**************************************************************************
 * Description :
 * WCB를 초기화한다.
 *
 * <IN>
 * aWASegment     - 대상 WASegment
 * aWCBPtr        - 대상 WCBPtr
 * aWPID          - 해당 WCB가 가질 WPAGEID
 ***************************************************************************/
void sdtWASegment::initWCB( sdtWASegment * aWASegment,
                            sdtWCB       * aWCBPtr,
                            scPageID       aWPID )
{
    (void)idCore::acpAtomicSet32( &aWCBPtr->mWPState, SDT_WA_PAGESTATE_NONE );
    aWCBPtr->mNSpaceID     = SC_NULL_SPACEID;
    aWCBPtr->mNPageID      = SC_NULL_PID;
    aWCBPtr->mFix          = 0;
    aWCBPtr->mBookedFree   = ID_FALSE;
    aWCBPtr->mWPageID      = aWPID;
    aWCBPtr->mNextWCB4Hash = NULL;

    bindWCB( aWASegment, aWCBPtr, aWPID );
}

/**************************************************************************
 * Description :
 * WCB를 Frame과 연결함
 *
 * <IN>
 * aWASegment     - 대상 WASegment
 * aWCBPtr        - 대상 WCBPtr
 * aWPID          - 해당 WCB가 가질 WPAGEID
 ***************************************************************************/
void sdtWASegment::bindWCB( sdtWASegment * aWASegment,
                            sdtWCB       * aWCBPtr,
                            scPageID       aWPID )
{
    UChar        * sWAExtentPtr;

    sWAExtentPtr = getWAExtentPtr( aWASegment, getExtentIdx(aWPID) );

    aWCBPtr->mWAPagePtr =
        getFrameInExtent( sWAExtentPtr, getPageSeqInExtent(aWPID) );

    /* 검증해봄 */
    IDE_DASSERT( getWCBInExtent( sWAExtentPtr, getPageSeqInExtent(aWPID) )
                 == aWCBPtr );
}

/**************************************************************************
 * Description :
 * Group의 크기를 Page개수로 받는다.
 *
 * <IN>
 * aWASegment     - 대상 WASegment
 * aWAGroupID     - 대상 Group ID
 ***************************************************************************/
UInt sdtWASegment::getWAGroupPageCount( sdtWASegment * aWASegment,
                                        sdtWAGroupID   aWAGroupID )
{
    sdtWAGroup     * sGrpInfo = getWAGroupInfo( aWASegment, aWAGroupID );

    return sGrpInfo->mEndWPID - sGrpInfo->mBeginWPID;
}

/**************************************************************************
 * Description :
 * Map이 차지하는 위치를 뺀, 사용 가능한 Group의 크기를 Page개수로 받는다.
 *
 * <IN>
 * aWASegment     - 대상 WASegment
 * aWAGroupID     - 대상 Group ID
 ***************************************************************************/
UInt sdtWASegment::getAllocableWAGroupPageCount( sdtWASegment * aWASegment,
                                                 sdtWAGroupID   aWAGroupID )
{
    sdtWAGroup     * sGrpInfo = getWAGroupInfo( aWASegment, aWAGroupID );

    return sGrpInfo->mEndWPID - sGrpInfo->mBeginWPID
        - sdtWAMap::getWPageCount( sGrpInfo->mMapHdr );
}


/**************************************************************************
 * Description :
 * Group내 첫 PageID
 *
 * <IN>
 * aWASegment     - 대상 WASegment
 * aWAGroupID     - 대상 Group ID
 ***************************************************************************/
scPageID sdtWASegment::getFirstWPageInWAGroup( sdtWASegment * aWASegment,
                                               sdtWAGroupID   aWAGroupID )
{
    return aWASegment->mGroup[ aWAGroupID ].mBeginWPID;
}
/**************************************************************************
 * Description :
 * Group내 마지막 PageID
 * 여기서 Last는 '진짜'마지막 페이지 +1 이다.
 * (10일 경우, 0,1,2,3,4,5,6,7,8,9를 해당 Group이 소유했다는 의미 )
 *
 * <IN>
 * aWASegment     - 대상 WASegment
 * aWAGroupID     - 대상 Group ID
 ***************************************************************************/
scPageID sdtWASegment::getLastWPageInWAGroup( sdtWASegment * aWASegment,
                                              sdtWAGroupID   aWAGroupID )
{
    return aWASegment->mGroup[ aWAGroupID ].mEndWPID;
}

/**************************************************************************
 * Description :
 * 두 Group을 하나로 (aWAGroupID1 쪽으로) 합친다.
 *
 * <IN>
 * aWASegment     - 대상 WASegment
 * aWAGroupID1    - 대상 GroupID ( 이쪽으로 합쳐짐 )
 * aWAGroupID2    - 대상 GroupID ( 소멸됨 )
 * aPolicy        - 합쳐진 group이 선택할 정책
 ***************************************************************************/
IDE_RC sdtWASegment::mergeWAGroup(sdtWASegment     * aWASegment,
                                  sdtWAGroupID       aWAGroupID1,
                                  sdtWAGroupID       aWAGroupID2,
                                  sdtWAReusePolicy   aPolicy )
{
    sdtWAGroup     * sGrpInfo1 = getWAGroupInfo( aWASegment, aWAGroupID1 );
    sdtWAGroup     * sGrpInfo2 = getWAGroupInfo( aWASegment, aWAGroupID2 );

    IDE_ERROR( sGrpInfo1->mPolicy != SDT_WA_REUSE_NONE );
    IDE_ERROR( sGrpInfo2->mPolicy != SDT_WA_REUSE_NONE );

    /* 다음과 같은 상태여야 함.
     * <----Group2-----><---Group1--->*/
    IDE_ERROR( sGrpInfo2->mEndWPID == sGrpInfo1->mBeginWPID );

    /* <----Group2----->
     *                  <---Group1--->
     *
     * 위를 다음과 같이 변경함.
     *
     * <----Group2----->
     * <--------------------Group1---> */
    sGrpInfo1->mBeginWPID = sGrpInfo2->mBeginWPID;

    /* Policy 재설정 */
    sGrpInfo1->mPolicy = aPolicy;
    /* 두번째 Group은 초기화시킴 */
    sGrpInfo2->mPolicy = SDT_WA_REUSE_NONE;

    IDE_TEST( clearWAGroup( aWASegment, aWAGroupID1) != IDE_SUCCESS );
    IDE_DASSERT( validateLRUList( aWASegment, aWAGroupID1 ) == IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**************************************************************************
 * Description :
 * 한 Group을 두 Group으로 나눈다. ( 크기는 동등 )
 *
 * <IN>
 * aWASegment     - 대상 WASegment
 * aSrcWAGroupID  - 쪼갤 원본 GroupID
 * aDstWAGroupID  - 쪼개져서 만들어질 Group
 * aPolicy        - 새 group이 선택할 정책
 ***************************************************************************/
IDE_RC sdtWASegment::splitWAGroup(sdtWASegment     * aWASegment,
                                  sdtWAGroupID       aSrcWAGroupID,
                                  sdtWAGroupID       aDstWAGroupID,
                                  sdtWAReusePolicy   aPolicy )
{
    sdtWAGroup     * sSrcGrpInfo= getWAGroupInfo( aWASegment, aSrcWAGroupID );
    sdtWAGroup     * sDstGrpInfo= getWAGroupInfo( aWASegment, aDstWAGroupID );
    UInt             sPageCount;

    IDE_ERROR( sDstGrpInfo->mPolicy == SDT_WA_REUSE_NONE );

    sPageCount = getAllocableWAGroupPageCount( aWASegment, aSrcWAGroupID )/2;

    IDE_ERROR( sPageCount >= 1 );


    /* 다음과 같이 Split함
     * <------------SrcGroup------------>
     * <---DstGroup----><----SrcGroup--->*/
    IDE_ERROR( sSrcGrpInfo->mEndWPID >= sPageCount );
    sDstGrpInfo->mBeginWPID = sSrcGrpInfo->mBeginWPID;
    sDstGrpInfo->mEndWPID   = sSrcGrpInfo->mBeginWPID + sPageCount;
    sSrcGrpInfo->mBeginWPID+= sPageCount;

    IDE_ERROR( sSrcGrpInfo->mBeginWPID < sSrcGrpInfo->mEndWPID );
    IDE_ERROR( sDstGrpInfo->mBeginWPID < sDstGrpInfo->mEndWPID );

    /* 두번째 Group의 값을 설정함 */
    sDstGrpInfo->mHintWPID       = SC_NULL_PID;
    sDstGrpInfo->mPolicy         = aPolicy;

    IDE_TEST( clearWAGroup( aWASegment, aSrcWAGroupID) != IDE_SUCCESS );
    IDE_TEST( clearWAGroup( aWASegment, aDstWAGroupID) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**************************************************************************
 * Description :
 * 모든 Group을 Drop한다.
 *
 * <IN>
 * aWASegment     - 대상 WASegment
 * aWait4Flush    - Dirty한 Page들을 Flush될때까지 기다릴 것인가.
 ***************************************************************************/
IDE_RC sdtWASegment::dropAllWAGroup( sdtWASegment * aWASegment,
                                     idBool         aWait4Flush )
{
    SInt             i;

    for( i = SDT_WAGROUPID_MAX - 1 ; i >  SDT_WAGROUPID_INIT ; i -- )
    {
        if( aWASegment->mGroup[ i ].mPolicy != SDT_WA_REUSE_NONE )
        {
            IDE_TEST( dropWAGroup( aWASegment, i, aWait4Flush )
                      != IDE_SUCCESS );
        }
        aWASegment->mGroup[ i ].mPolicy = SDT_WA_REUSE_NONE;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**************************************************************************
 * Description :
 * Group을 Drop하고 InitGroup에 Extent를 반환한다. 어차피
 * dropWASegment할거라면 굳이 할 필요 없다.
 *
 * <IN>
 * aWASegment     - 대상 WASegment
 * aWAGroupID     - 대상 Group ID
 * aWait4Flush    - Dirty한 Page들을 Flush될때까지 기다릴 것인가.
 ***************************************************************************/
IDE_RC sdtWASegment::dropWAGroup(sdtWASegment * aWASegment,
                                 sdtWAGroupID   aWAGroupID,
                                 idBool         aWait4Flush)
{
    sdtWAGroup     * sInitGrpInfo =
        getWAGroupInfo( aWASegment, SDT_WAGROUPID_INIT );
    sdtWAGroup     * sGrpInfo = getWAGroupInfo( aWASegment, aWAGroupID );

    IDE_TEST( resetWAGroup( aWASegment, aWAGroupID, aWait4Flush )
              != IDE_SUCCESS );

    /* 가장 최근에 할당한 Group이어야함.
     * 즉 다음과 같은 상태여야 함.
     * <---InitGroup---><----CurGroup----->*/
    IDE_ERROR( sInitGrpInfo->mEndWPID == sGrpInfo->mBeginWPID );

    /* 아래 문장을 통해 다음과 같이 만듬. 어차피 현재의 Group은
     * 무시될 것이기에 상관 없음.
     *                  <----CurGroup----->
     * <----------------------InitGroup---> */
    sInitGrpInfo->mEndWPID = sGrpInfo->mEndWPID;
    sGrpInfo->mPolicy      = SDT_WA_REUSE_NONE;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**************************************************************************
 * Description :
 * Group의 정책에 따라 재사용할 수 있는 WAPage를 반환한다. Reuse가 True면
 * 재활용도 허용된다. 실패하면 aRetPID로 0이 Return된다.
 *
 * <IN>
 * aWASegment     - 대상 WASegment
 * aWAGroupID     - 대상 Group ID
 * <OUT>
 * aRetPID        - 탐색한 FreeWAPID
 ***************************************************************************/
IDE_RC sdtWASegment::getFreeWAPage( sdtWASegment * aWASegment,
                                    sdtWAGroupID   aWAGroupID,
                                    scPageID     * aRetPID )
{
    sdtWAGroup     * sGrpInfo;
    scPageID         sRetPID;
    UInt             sRetryCount = 0;
    UInt             sRetryLimit;

    IDE_ERROR( aWAGroupID != SDT_WAGROUPID_NONE );

    sGrpInfo    = getWAGroupInfo( aWASegment, aWAGroupID );
    sRetryLimit = getAllocableWAGroupPageCount( aWASegment, aWAGroupID );

    while( 1 )
    {
        switch( sGrpInfo->mPolicy )
        {
            case SDT_WA_REUSE_INMEMORY:
                IDE_ERROR( sGrpInfo->mMapHdr != NULL );

                sRetPID = sGrpInfo->mReuseWPID1;
                sGrpInfo->mReuseWPID1--;

                /* Map이 있는 곳까지 썼으면, 다 썼다는 이야기 */
                if( sdtWAMap::getEndWPID( sGrpInfo->mMapHdr ) >= sRetPID )
                {
                    /* InMemoryGroup은 victimReplace가 안된다.
                     * 초기화 하고, NULL을 반환한다.*/
                    sGrpInfo->mReuseWPID1 = sGrpInfo->mEndWPID - 1;
                    sRetPID = SD_NULL_PID;
                }
                break;

            case SDT_WA_REUSE_FIFO:
                IDE_ERROR( sGrpInfo->mMapHdr == NULL );

                sRetPID = sGrpInfo->mReuseWPID1;
                sGrpInfo->mReuseWPID1++;

                /* Group을 한번 전부 순회함 */
                if( sGrpInfo->mReuseWPID1 == sGrpInfo->mEndWPID )
                {
                    sGrpInfo->mReuseWPID1 = sGrpInfo->mBeginWPID;
                }
                break;
            case SDT_WA_REUSE_LRU:
                IDE_ERROR( sGrpInfo->mMapHdr == NULL );

                sRetPID = sGrpInfo->mReuseWPID2;
                IDE_TEST( moveLRUListToTop( aWASegment,
                                            aWAGroupID,
                                            sRetPID )
                          != IDE_SUCCESS );
                break;
            default:
                IDE_ERROR( 0 );
                break;
        }

        if( ( sRetPID != SD_NULL_PID ) &&
            ( isFixedPage( aWASegment, sRetPID ) == ID_FALSE ) )
        {
            /* 제대로 골랐음 */
            break;
        }
        else
        {
            /* 사용할 페이지가 없음*/
            if( sGrpInfo->mPolicy == SDT_WA_REUSE_INMEMORY )
            {
                /* InMemory이 일 경우, 없으면 호출자가 처리해줘야함.
                 * 정상적인 경우이니 반환함 */
                break;
            }
        }

        IDE_ERROR( sRetryCount < sRetryLimit );
        sRetryCount ++;
    }

    /* 할당받은 페이지를 빈 페이지로 만듬 */
    if( sRetPID != SD_NULL_PID )
    {
        IDE_TEST( makeInitPage( aWASegment,
                                sRetPID,
                                ID_TRUE ) /*wait 4 flush */
                  != IDE_SUCCESS );
    }
    (*aRetPID) = sRetPID;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**************************************************************************
 * Description :
 * Segment내 DirtyFlag가 설정된 WAPage들을 FlushQueue에 등록한다.
 *
 * <IN>
 * aWASegment     - 대상 WASegment
 * aWAGroupID     - 대상 Group ID
 ***************************************************************************/
IDE_RC sdtWASegment::writeDirtyWAPages(sdtWASegment * aWASegment,
                                       sdtWAGroupID   aWAGroupID )
{
    sdtWAGroup     * sGrpInfo = getWAGroupInfo( aWASegment, aWAGroupID );
    UInt             i;

    for( i = sGrpInfo->mBeginWPID ; i < sGrpInfo->mEndWPID ; i ++ )
    {
        if( getWAPageState( aWASegment, i ) == SDT_WA_PAGESTATE_DIRTY )
        {
            IDE_TEST( writeNPage( aWASegment, i ) != IDE_SUCCESS );
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**************************************************************************
 * Description :
 * 대상 Page를 초기화한다.
 *
 * <IN>
 * aWASegment     - 대상 WASegment
 * aWPID          - 대상 WPID
 * aWait4Flush    - Dirty한 Page들을 Flush될때까지 기다릴 것인가.
 ***************************************************************************/
IDE_RC sdtWASegment::makeInitPage( sdtWASegment * aWASegment,
                                   scPageID       aWPID,
                                   idBool         aWait4Flush)
{
    sdtWAPageState   sWAPageState;
    sdtWAFlusher   * sWAFlusher;
    sdtWCB         * sWCBPtr;

    /* Page를 할당받았을때, 해당 Page는 아직 Write돼지 않았을 수 있다.
     * 그러면 강제로 Write를 시도한다. */
    while( isFreeWAPage( aWASegment, aWPID ) == ID_FALSE )
    {
        sWAPageState = getWAPageState( aWASegment, aWPID );
        if( ( sWAPageState == SDT_WA_PAGESTATE_IN_FLUSHQUEUE ) &&
            ( sdtWorkArea::isEmptyQueue( aWASegment->mFlushQueue ) == ID_TRUE ) )
        {
            /* retry */
            sWAPageState = getWAPageState( aWASegment, aWPID );
            if( sWAPageState == SDT_WA_PAGESTATE_IN_FLUSHQUEUE )
            {
                ideLog::log( IDE_DUMP_0,
                             "WPID : %u\n",
                             aWPID );

                smuUtility::dumpFuncWithBuffer( IDE_DUMP_0,
                                                dumpFlushQueue,
                                                (void*)aWASegment->mFlushQueue );
                IDE_ERROR( 0 );
            }
        }

        if( aWait4Flush == ID_TRUE )
        {
            switch( sWAPageState )
            {
                case SDT_WA_PAGESTATE_DIRTY:
                    /* Dirty 상태이면, 아직 Flusher와 상관없기 때문에
                     * 스스로 write해도 문제 없다. */
                    IDE_TEST( writeNPage( aWASegment, aWPID )
                              != IDE_SUCCESS );
                    break;
                case SDT_WA_PAGESTATE_IN_FLUSHQUEUE:
                case SDT_WA_PAGESTATE_WRITING:
                    /* Flusher가 Write하려 하는 상황이기에 대기하면 된다. */
                    break;
                default:
                    /* ServiceThrea가 상태를 가져온 직후 Flusher가 Flush하여
                     * Clean으로 만들면 이쪽으로 오게 됨.
                     * 아무것도 안하고 다음 Loop로 가면 됨 */
                    break;
            }
        }
        else
        {
            switch( sWAPageState )
            {
                case SDT_WA_PAGESTATE_DIRTY:
                    /* Dirty 상태이면, 아직 Flusher와 상관없기 때문에
                     * 즉시 상태를 변경하면 된다. */
                    IDE_TEST( setWAPageState( aWASegment,
                                              aWPID,
                                              SDT_WA_PAGESTATE_CLEAN )
                              != IDE_SUCCESS );
                    break;
                case SDT_WA_PAGESTATE_IN_FLUSHQUEUE:
                case SDT_WA_PAGESTATE_WRITING:
                    /* Flusher와 동시성 이슈가 있는 상태.
                     * FlushQueue상태일 경우, Clean으로 변경한 후
                     * 다음 loop에서 다시 체크함*/
                    sWCBPtr = getWCB( aWASegment, aWPID );
                    checkAndSetWAPageState( sWCBPtr,
                                            SDT_WA_PAGESTATE_IN_FLUSHQUEUE,
                                            SDT_WA_PAGESTATE_CLEAN,
                                            &sWAPageState );
                    break;
                default:
                    /* ServiceThrea가 상태를 가져온 직후 Flusher가 Flush하여
                     * Clean으로 만들면 이쪽으로 오게 됨.
                     * 아무것도 안하고 다음 Loop로 가면 됨 */
                    break;
            }
        }

        /* sleep했다가 다시 시도함 */
        (*aWASegment->mStatsPtr)->mWriteWaitCount ++;
        idlOS::thr_yield();

        /* Flusher는 동작중이어야 함 */
        sWAFlusher = sdtWorkArea::getWAFlusher( aWASegment->mFlushQueue->mWAFlusherIdx );
        IDE_ERROR( sWAFlusher->mRun == ID_TRUE );
    }

    /* Assign되어있으면 Assign 된 페이지를 초기화시켜준다. */
    if( getWAPageState( aWASegment, aWPID ) == SDT_WA_PAGESTATE_CLEAN )
    {
        aWASegment->mDiscardPage = ID_TRUE;
        IDE_TEST( unassignNPage( aWASegment,
                                 aWPID )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**************************************************************************
 * Description :
 * WCB를 바탕으로 Page의 Fix여부를 반환한다.
 *
 * <IN>
 * aWASegment     - 대상 WASegment
 * aWPID          - 대상 WPID
 ***************************************************************************/
idBool sdtWASegment::isFixedPage(sdtWASegment * aWASegment,
                                 scPageID       aWPID )
{
    sdtWCB         * sWCBPtr;

    sWCBPtr = getWCB( aWASegment, aWPID );

    return (sWCBPtr->mFix > 0) ? ID_TRUE : ID_FALSE;
}

/**************************************************************************
 * Description :
 * WCB를 바탕으로 Page의 NPID를 반환한다.
 *
 * <IN>
 * aWASegment     - 대상 WASegment
 * aWPID          - 대상 WPID
 ***************************************************************************/
scPageID       sdtWASegment::getNPID( sdtWASegment * aWASegment,
                                      scPageID       aWPID )
{
    sdtWCB         * sWCBPtr;

    sWCBPtr = getWCB( aWASegment, aWPID );

    return sWCBPtr->mNPageID;
}

/**************************************************************************
 * Description :
 * WCB에 해당 SpaceID 및 PageID를 설정한다. 즉 Page를 읽는 것이 아니라
 * 설정만 한 것으로, Disk상에 있는 일반Page의 내용은 '무시'된다.

 * <IN>
 * aWASegment     - 대상 WASegment
 * aWPID          - 대상 WPID
 * aNSpaceID,aNPID- 대상 NPage의 주소
 ***************************************************************************/
IDE_RC sdtWASegment::assignNPage(sdtWASegment * aWASegment,
                                 scPageID       aWPID,
                                 scSpaceID      aNSpaceID,
                                 scPageID       aNPageID)
{
    UInt             sHashValue;
    sdtWCB         * sWCBPtr;

    /* 검증. 이미 Hash이 매달린 것이 아니어야 함 */
    IDE_DASSERT( findWCB( aWASegment, aNSpaceID, aNPageID )
                 == NULL );
    /* 제대로된 WPID여야 함 */
    IDE_DASSERT( aWPID < getWASegmentPageCount( aWASegment ) );

    sWCBPtr = getWCB( aWASegment, aWPID );
    /* NPID 로 연결함 */
    IDE_ERROR( sWCBPtr->mNSpaceID == SC_NULL_SPACEID );
    IDE_ERROR( sWCBPtr->mNPageID  == SC_NULL_PID     );
    sWCBPtr->mNSpaceID = aNSpaceID;
    sWCBPtr->mNPageID  = aNPageID;
    IDE_TEST( setWAPageState( aWASegment, aWPID, SDT_WA_PAGESTATE_CLEAN )
              != IDE_SUCCESS );

    /* Hash에 연결하기 */
    sHashValue = getNPageHashValue( aWASegment, aNSpaceID, aNPageID );
    sWCBPtr->mNextWCB4Hash = aWASegment->mNPageHashPtr[sHashValue];
    aWASegment->mNPageHashPtr[sHashValue] = sWCBPtr;

    /* Hash에 매달려서, 탐색되어야 함 */
    IDE_DASSERT( findWCB( aWASegment, aNSpaceID, aNPageID )
                 == sWCBPtr );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**************************************************************************
 * Description :
 * NPage를 땐다. 즉 WPage상에 올려저 있는 것을 내린다.
 * 하는 일은 값을 초기화하고 Hash에서 제외하는 일 밖에 안한다.

 * <IN>
 * aWASegment     - 대상 WASegment
 * aWPID          - 대상 WPID
 ***************************************************************************/
IDE_RC sdtWASegment::unassignNPage( sdtWASegment * aWASegment,
                                    scPageID       aWPID )
{
    UInt             sHashValue = 0;
    scSpaceID        sTargetTBSID;
    scPageID         sTargetNPID;
    sdtWCB        ** sWCBPtrPtr;
    sdtWCB         * sWCBPtr;

    sWCBPtr = getWCB( aWASegment, aWPID );

    /* Fix되어있지 않아야 함 */
    IDE_ERROR( sWCBPtr->mFix == 0 );
    IDE_ERROR( ( sWCBPtr->mNSpaceID != SC_NULL_SPACEID ) &&
               ( sWCBPtr->mNPageID  != SC_NULL_PID ) );

    /* CleanPage여야 함 */
    IDE_ERROR( getWAPageState( aWASegment, aWPID ) == SDT_WA_PAGESTATE_CLEAN );

    /************************** Hash에서 때어내기 *********************/
    sTargetTBSID = sWCBPtr->mNSpaceID;
    sTargetNPID  = sWCBPtr->mNPageID;
    /* 매달려 있어야 하며, 따라서 탐색할 수 있어야 함 */
    IDE_DASSERT( findWCB( aWASegment, sTargetTBSID, sTargetNPID ) == sWCBPtr );
    sHashValue   = getNPageHashValue( aWASegment, sTargetTBSID, sTargetNPID );

    sWCBPtrPtr   = &aWASegment->mNPageHashPtr[ sHashValue ];

    sWCBPtr      = *sWCBPtrPtr;
    while( sWCBPtr != NULL )
    {
        if( sWCBPtr->mNPageID == sTargetNPID )
        {
            IDE_ERROR( sWCBPtr->mNSpaceID == sTargetTBSID );
            /* 찾았다, 자신의 자리에 다음 것을 매담 */
            (*sWCBPtrPtr) = sWCBPtr->mNextWCB4Hash;
            break;
        }

        sWCBPtrPtr = &sWCBPtr->mNextWCB4Hash;
        sWCBPtr    = *sWCBPtrPtr;
    }

    /* 없어졌어야 함 */
    IDE_DASSERT( findWCB( aWASegment, sTargetTBSID, sTargetNPID ) == NULL );

    /***** Free가 예정되었으면, unassign으로 내려가는 시점에 Free해준다. ****/
    if( sWCBPtr->mBookedFree == ID_TRUE )
    {
        IDE_TEST( pushFreePage( aWASegment,
                                sWCBPtr->mNPageID )
                  != IDE_SUCCESS );
    }

    initWCB( aWASegment, sWCBPtr, aWPID );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    ideLog::log( IDE_DUMP_0,
                 "HASHVALUE : %u\n"
                 "WPID      : %u\n",
                 sHashValue,
                 aWPID );

    return IDE_FAILURE;
}

/**************************************************************************
 * Description :
 * WPage를 다른 곳으로 옮긴다. npage, npagehash등을 고려해야 한다.
 *
 * <IN>
 * aWASegment     - 대상 WASegment
 * aSrcWAGroupID  - 원본 Group ID
 * aSrcWPID       - 원본 WPID
 * aDstWAGroupID  - 대상 Group ID
 * aDstWPID       - 대상 WPID
 ***************************************************************************/
IDE_RC sdtWASegment::moveWPage( sdtWASegment * aWASegment,
                                sdtWAGroupID   aSrcGroupID,
                                scPageID       aSrcWPID,
                                sdtWAGroupID   aDstGroupID,
                                scPageID       aDstWPID )
{
    UChar        * sSrcWAPagePtr;
    UChar        * sDstWAPagePtr;
    sdtWCB       * sWCBPtr;
    scSpaceID      sNSpaceID;
    scPageID       sNPageID;

    // BUG-46383 moveWPage에서 두 Page의 wa group이 같을 때에도 skip되면 안됩니다.
    // 반대로 WPID가 같은 경우에는 같은 page이므로 skip합니다.
    if( aSrcWPID != aDstWPID )
    {
        sSrcWAPagePtr = getWAPagePtr( aWASegment, aSrcGroupID, aSrcWPID );
        sDstWAPagePtr = getWAPagePtr( aWASegment, aDstGroupID, aDstWPID );

        /* PageCopy */
        idlOS::memcpy( sDstWAPagePtr,sSrcWAPagePtr,SD_PAGE_SIZE );

        sWCBPtr = getWCB( aWASegment, aSrcWPID );
        sNSpaceID = sWCBPtr->mNSpaceID;
        sNPageID  = sWCBPtr->mNPageID;

        /* 원본 페이지를 빈 페이지로 만든다. */
        IDE_TEST( makeInitPage( aWASegment,
                                aSrcWPID,
                                ID_TRUE ) /* wait4flush */
                  != IDE_SUCCESS );

        if( ( sNSpaceID != SC_NULL_SPACEID ) && ( sNPageID != SC_NULL_PID ) )
        {
            /* Assign된 페이지였으면, Assign 시킨다. */
            IDE_TEST( assignNPage( aWASegment, aDstWPID, sNSpaceID, sNPageID )
                      != IDE_SUCCESS );
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

/**************************************************************************
 * Description :
 * bindPage하고 Disk상의 일반Page로부터 내용을 읽어 올린다.
 *
 * <IN>
 * aWASegment     - 대상 WASegment
 * aWAGroupID     - 대상 Group ID
 * aWPID          - 읽어드릴 WPID
 * aNSpaceID,aNPID- 대상 NPage의 주소
 ***************************************************************************/
IDE_RC sdtWASegment::readNPage(sdtWASegment * aWASegment,
                               sdtWAGroupID   aWAGroupID,
                               scPageID       aWPID,
                               scSpaceID      aNSpaceID,
                               scPageID       aNPageID)
{
    UChar          * sWAPagePtr;
    smLSN            sOnlineTBSLSN4Idx;
    UInt             sDummy;
    sdtWCB         * sOldWCB;
    sdtWAGroupID     sOldWAGroupID;

    sOldWCB = findWCB( aWASegment, aNSpaceID, aNPageID );
    if( sOldWCB == NULL )
    {
        /* 기존에 없던 페이지면 Read해서 올림 */
        IDE_TEST( assignNPage( aWASegment,
                               aWPID,
                               aNSpaceID,
                               aNPageID )
                  != IDE_SUCCESS );

        sWAPagePtr = getWAPagePtr( aWASegment, aWAGroupID, aWPID );

        SM_LSN_INIT( sOnlineTBSLSN4Idx );
        IDE_TEST( sddDiskMgr::read( aWASegment->mStatistics,
                                    aNSpaceID,
                                    aNPageID,
                                    sWAPagePtr,
                                    &sDummy, /* datafile id */
                                    &sOnlineTBSLSN4Idx )
                  != IDE_SUCCESS );
        IDU_FIT_POINT( "BUG-45263@sdtWASegment::readNPage::iduFileopen" );
        (*aWASegment->mStatsPtr)->mReadCount ++;
    }
    else
    {
        /* 기존에 있던 페이지니, moveWPage로 옮긴다. */
        sOldWAGroupID = findGroup( aWASegment, sOldWCB->mWPageID );
        IDE_TEST( moveWPage( aWASegment,
                             sOldWAGroupID,
                             sOldWCB->mWPageID,
                             aWAGroupID,
                             aWPID )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**************************************************************************
 * Description :
 * 해당 WAPage를 FlushQueue에 등록하거나 직접 Write한다.
 * 반드시 bindPage,readPage등으로 SpaceID,PageID가 설정된 WAPage여야 한다.
 *
 * <IN>
 * aWASegment     - 대상 WASegment
 * aWPID          - 읽어드릴 WPID
 ***************************************************************************/
IDE_RC sdtWASegment::writeNPage(sdtWASegment * aWASegment,
                                scPageID       aWPID )
{
    // FlushPageQueue에 밀어넣는다.
    IDE_TEST( setWAPageState( aWASegment,
                              aWPID,
                              SDT_WA_PAGESTATE_IN_FLUSHQUEUE )
              != IDE_SUCCESS );
    IDE_TEST( sdtWorkArea::pushJob( aWASegment, aWPID ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**************************************************************************
 * Description :
 * 빈 Page를 Stack에 보관한다. 나중에 allocAndAssignNPage 에서 재활용한다.
 *
 * <IN>
 * aWASegment     - 대상 WASegment
 * aWPID          - 읽어드릴 WPID
 ***************************************************************************/
IDE_RC sdtWASegment::pushFreePage(sdtWASegment * aWASegment,
                                  scPageID       aNPID)
{
    IDE_TEST( aWASegment->mFreeNPageStack.push(ID_FALSE, /* lock */
                                               (void*) &aNPID )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**************************************************************************
 * Description :
 * NPage를 할당받고 설정한다.
 *
 * <IN>
 * aWASegment     - 대상 WASegment
 * aTargetPID     - 설정할 WPID
 ***************************************************************************/
IDE_RC sdtWASegment::allocAndAssignNPage(sdtWASegment * aWASegment,
                                         scPageID       aTargetPID )
{
    idBool         sIsEmpty;
    scPageID       sFreeNPID;

    IDE_TEST( aWASegment->mFreeNPageStack.pop( ID_FALSE, /* lock */
                                               (void*) &sFreeNPID,
                                               &sIsEmpty )
              != IDE_SUCCESS );
    if( sIsEmpty == ID_TRUE )
    {
        if( aWASegment->mLastFreeExtent == NULL )
        {
            /* 최초 Alloc 시도 */
            IDE_TEST( allocFreeNExtent( aWASegment ) != IDE_SUCCESS );
        }
        else
        {
            if( aWASegment->mPageSeqInLFE ==
                ((sdpExtDesc*)aWASegment->mLastFreeExtent)->mLength )
            {
                /* 마지막 NExtent를 다썼음 */
                IDE_TEST( allocFreeNExtent( aWASegment ) != IDE_SUCCESS );
            }
            else
            {
                /* 기존에 할당해둔 Extent에서 가져옴 */
            }
        }

/* BUG-37503
   allocFreeNExtent()에서  slot 포인터를 가져온 다음에 IDE_TEST에서 실패하면
   mLastFreeExten가 잘못설정될수 있다. */
        IDE_TEST_RAISE( aWASegment->mPageSeqInLFE >
                        ((sdpExtDesc*)aWASegment->mLastFreeExtent)->mLength,
                        ERROR_INVALID_EXTENT );

        sFreeNPID = ((sdpExtDesc*)aWASegment->mLastFreeExtent)->mExtFstPID
            + aWASegment->mPageSeqInLFE;
        aWASegment->mPageSeqInLFE ++;
        aWASegment->mNPageCount ++;
    }
    else
    {
        /* Stack을 통해 재활용했음 */
    }

    IDE_TEST( assignNPage( aWASegment,
                           aTargetPID,
                           aWASegment->mSpaceID,
                           sFreeNPID )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERROR_INVALID_EXTENT );
    {
        ideLog::log( IDE_SM_0,
                     "LFE->mLength : %d \n"
                     "LFE->mExtFstPID : %d \n"
                     "WASegment->mPageSeqInLastFreeExtent : %d \n"
                     "WASegment->mNExtentCount : %d \n"
                     "WASegment->mNPageCount : %d \n",
                     ((sdpExtDesc*)aWASegment->mLastFreeExtent)->mLength,
                     ((sdpExtDesc*)aWASegment->mLastFreeExtent)->mExtFstPID,
                     aWASegment->mPageSeqInLFE,
                     aWASegment->mNExtentCount,
                     aWASegment->mNPageCount );
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**************************************************************************
 * Description :
 * 해당 SpaceID로부터 빈 NormalExtent를 가져온다. (sdptbExtent::allocExts
 * 사용 )
 *
 * <IN>
 * aWASegment     - 대상 WASegment
 ***************************************************************************/
IDE_RC sdtWASegment::allocFreeNExtent( sdtWASegment * aWASegment )
{
    sdpExtDesc      * sSlotPtr;
    sdrMtxStartInfo   sStartInfo;
    sdrMtx            sMtx;
    UInt              sState = 0;

    IDE_TEST_RAISE( sdtWAMap::getSlotCount( &aWASegment->mNExtentMap )
                    <= aWASegment->mNExtentCount,
                    ERROR_NOT_ENOUGH_NEXTENTSIZE );

    IDU_FIT_POINT_RAISE( "BUG-45857@sdtWASegment::allocFreeNExtent::ERROR_NOT_ENOUGH_NEXTENTSIZE",
                          ERROR_NOT_ENOUGH_NEXTENTSIZE );
    
    IDE_TEST( sdtWAMap::getSlotPtr( &aWASegment->mNExtentMap,
                                    aWASegment->mNExtentCount,
                                    sdtWAMap::getSlotSize( &aWASegment->mNExtentMap ),
                                    (void**)&sSlotPtr )
              != IDE_SUCCESS );

    IDE_TEST(sdrMiniTrans::begin(aWASegment->mStatistics,
                                 &sMtx,
                                 NULL,
                                 SDR_MTX_NOLOGGING,
                                 ID_FALSE,/*MtxUndoable(PROJ-2162)*/
                                 SM_DLOG_ATTR_DEFAULT)
             != IDE_SUCCESS);
    sState = 1;
    sdrMiniTrans::makeStartInfo ( &sMtx, &sStartInfo );

    IDE_TEST( sdpTableSpace::allocExts( aWASegment->mStatistics,
                                        &sStartInfo,
                                        aWASegment->mSpaceID,
                                        1, /*need extent count */
                                        (sdpExtDesc*)sSlotPtr )
              != IDE_SUCCESS );

    sState = 0;
    IDE_TEST(sdrMiniTrans::commit(&sMtx) != IDE_SUCCESS);

    /* BUG-37503
       slot 포인터를 가져온 다음에 IDE_TEST에서 실패하면
       mLastFreeExtent에서 잘못된값을 보게 된다. */
    aWASegment->mLastFreeExtent = (void*)sSlotPtr;
    aWASegment->mNExtentCount ++;
    aWASegment->mPageSeqInLFE = 0;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERROR_NOT_ENOUGH_NEXTENTSIZE );
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_NOT_ENOUGH_NEXTENTSIZE ) );
    }
    IDE_EXCEPTION_END;

    if (sState != 0)
    {
        IDE_ASSERT(sdrMiniTrans::rollback(&sMtx) == IDE_SUCCESS);
    }

    return IDE_FAILURE;
}

/**************************************************************************
 * Description :
 * 이 WASegment가 사용한 모든 WASegment를 Free한다.
 *
 * <IN>
 * aWASegment     - 대상 WASegment
 ***************************************************************************/
IDE_RC sdtWASegment::freeAllNPage( sdtWASegment * aWASegment )
{
    UInt             sFreeExtCnt;
    sdpExtDesc     * sSlot;
    UInt             sSlotSize;
    sdrMtx           sMtx;
    UInt             sState = 0;
    UInt             i;

    sSlotSize = sdtWAMap::getSlotSize( &aWASegment->mNExtentMap );
    for( i = 0 ; i < aWASegment->mNExtentCount ; i ++ )
    {
        IDE_TEST(sdrMiniTrans::begin(aWASegment->mStatistics,
                                     &sMtx,
                                     NULL,
                                     SDR_MTX_NOLOGGING,
                                     ID_FALSE,/*MtxUndoable(PROJ-2162)*/
                                     SM_DLOG_ATTR_DEFAULT)
                 != IDE_SUCCESS);
        sState = 1;

        IDE_TEST( sdtWAMap::getSlotPtr( &aWASegment->mNExtentMap,
                                        i,
                                        sSlotSize,
                                        (void**)&sSlot )
                  != IDE_SUCCESS );
        IDE_TEST( sdpTableSpace::freeExt( aWASegment->mStatistics,
                                          &sMtx,
                                          aWASegment->mSpaceID,
                                          sSlot->mExtFstPID,
                                          &sFreeExtCnt )
                  != IDE_SUCCESS );

        sState = 0;
        IDE_TEST(sdrMiniTrans::commit(&sMtx) != IDE_SUCCESS);
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if (sState != 0)
    {
        IDE_ASSERT(sdrMiniTrans::rollback(&sMtx) == IDE_SUCCESS);
    }

    return IDE_FAILURE;
}

/**************************************************************************
 * Description :
 * 이 WASegment의 NPage 개수를 반환한다.
 *
 * <IN>
 * aWASegment     - 대상 WASegment
 ***************************************************************************/
UInt   sdtWASegment::getNPageCount( sdtWASegment * aWASegment )
{
    return aWASegment->mNPageCount;
}

/**************************************************************************
 * Description :
 * Seq번째의 NPID를 반환한다.
 *
 * <IN>
 * aWASegment     - 대상 WASegment
 * aIdx           - Seq
 * <OUT>
 * aPID           - N번째 NPID
 ***************************************************************************/
IDE_RC sdtWASegment::getNPIDBySeq( sdtWASegment * aWASegment,
                                   UInt           aIdx,
                                   scPageID     * aPID )
{
    sdpExtDesc      * sExtDescPtr;
    UInt              sPageCntInExt;
    UInt              sExtentIdx;
    UInt              sSeq;

    if( getNPageCount( aWASegment ) <= aIdx )
    {
        /*마지막까지 다 찾았음 */
        *aPID = SC_NULL_PID;
    }
    else
    {
        sPageCntInExt = sddDiskMgr::getPageCntInExt( aWASegment->mSpaceID );
        sExtentIdx = aIdx / sPageCntInExt;
        sSeq       = aIdx % sPageCntInExt;

        IDE_TEST( sdtWAMap::getSlotPtr( &aWASegment->mNExtentMap,
                                        sExtentIdx,
                                        sdtWAMap::getSlotSize(
                                            &aWASegment->mNExtentMap ),
                                        (void**)&sExtDescPtr )
                  != IDE_SUCCESS );
        *aPID = sExtDescPtr->mExtFstPID + sSeq;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**************************************************************************
 * Description :
 * LRU 재활용 정책 사용시, 사용한 해당 Page를 Top으로 이동시켜 Drop을 늦춘다.
 *
 * <IN>
 * aWASegment     - 대상 WASegment
 * aWAGroupID     - 대상 Group ID
 * aPID           - 대상 WPID
 ***************************************************************************/
IDE_RC sdtWASegment::moveLRUListToTop( sdtWASegment * aWASegment,
                                       sdtWAGroupID   aWAGroupID,
                                       scPageID       aPID )
{
    sdtWAGroup     * sGrpInfo;
    sdtWCB         * sWCBPtr;
    scPageID         sPrevPID;
    scPageID         sNextPID;
    sdtWCB         * sPrevWCBPtr;
    sdtWCB         * sNextWCBPtr;
    sdtWCB         * sOldTopWCBPtr;

    if( ( aWAGroupID != SDT_WAGROUPID_NONE ) &&
        ( aWAGroupID != SDT_WAGROUPID_INIT ) &&
        ( aWAGroupID != SDT_WAGROUPID_MAX ) )
    {
        sGrpInfo = &aWASegment->mGroup[ aWAGroupID ];
        if( ( sGrpInfo->mPolicy     != SDT_WA_REUSE_LRU ) ||
            ( sGrpInfo->mReuseWPID1 == aPID ) )

        {
            /* 이미 Top이거나 LRU가 아니라 움직일 필요가 없음 */
        }
        else
        {
            IDE_DASSERT( validateLRUList( aWASegment, aWAGroupID )
                         == IDE_SUCCESS);
            sWCBPtr       = getWCB( aWASegment, aPID );
            sOldTopWCBPtr = getWCB( aWASegment, sGrpInfo->mReuseWPID1 );

            IDE_ERROR( sWCBPtr       != NULL );
            IDE_ERROR( sOldTopWCBPtr != NULL );

            sPrevPID = sWCBPtr->mLRUPrevPID;
            sNextPID = sWCBPtr->mLRUNextPID;

            if( sPrevPID == SC_NULL_PID )
            {
                /* Prev가 없다는 것은 Top이라는 이야기. top으로 다시 보낼 필요
                 * 없음 */
            }
            else
            {
                /* PrevPage로부터 현재 Page에 대한 링크를 때어냄 */
                sPrevWCBPtr = getWCB( aWASegment, sPrevPID );
                if( sNextPID == SD_NULL_PID )
                {
                    /* 자신이 마지막 Page였을 경우 */
                    IDE_ERROR( sGrpInfo->mReuseWPID2 == aPID );
                    sGrpInfo->mReuseWPID2 = sPrevPID;
                }
                else
                {
                    sNextWCBPtr = getWCB( aWASegment, sNextPID );
                    sNextWCBPtr->mLRUPrevPID = sPrevPID;
                }
                sPrevWCBPtr->mLRUNextPID = sNextPID;

                /* 선두에 매담 */
                sWCBPtr->mLRUPrevPID  = SD_NULL_PID;
                sWCBPtr->mLRUNextPID  = sGrpInfo->mReuseWPID1;
                sOldTopWCBPtr->mLRUPrevPID = aPID;
                sGrpInfo->mReuseWPID1 = aPID;
            }

            IDE_DASSERT( validateLRUList( aWASegment,
                                          aWAGroupID ) == IDE_SUCCESS);
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    ideLog::log( IDE_DUMP_0,
                 "GroupID : %u\n"
                 "PID     : %u",
                 aWAGroupID,
                 aPID );

    return IDE_FAILURE;
}

/**************************************************************************
 * Description :
 * LRU리스트를 점검한다. Debugging 모드 용
 *
 * <IN>
 * aWASegment     - 대상 WASegment
 * aWAGroupID     - 대상 Group ID
 ***************************************************************************/
IDE_RC sdtWASegment::validateLRUList( sdtWASegment * aWASegment,
                                      sdtWAGroupID   aWAGroupID )
{
    sdtWAGroup     * sGrpInfo;
    sdtWCB         * sWCBPtr;
    UInt             sPageCount = 0;
    scPageID         sPrevWPID;
    scPageID         sNextWPID;

    IDE_ERROR( aWAGroupID != SDT_WAGROUPID_MAX );

    sGrpInfo = &aWASegment->mGroup[ aWAGroupID ];
    IDE_ERROR( sGrpInfo->mPolicy == SDT_WA_REUSE_LRU );

    sPrevWPID = SC_NULL_PID;
    sNextWPID = sGrpInfo->mReuseWPID1;
    while( sNextWPID != SC_NULL_PID )
    {
        sWCBPtr = getWCB( aWASegment, sNextWPID );

        IDE_ASSERT( sWCBPtr != NULL );
        IDE_ASSERT( sWCBPtr->mLRUPrevPID == sPrevWPID );
        IDE_ASSERT( sWCBPtr->mWPageID    == sNextWPID );

        sNextWPID = sWCBPtr->mLRUNextPID;
        sPrevWPID = sWCBPtr->mWPageID;

        sPageCount ++;
    }

    IDE_ASSERT( sPrevWPID == sGrpInfo->mReuseWPID2 );
    IDE_ASSERT( sNextWPID == SC_NULL_PID );
    IDE_ASSERT( sPageCount == getWAGroupPageCount( aWASegment, aWAGroupID ) );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

/**************************************************************************
 * Description :
 * PID를 바탕으로 그 PID를 소유한 Group을 찾는다.
 *
 * <IN>
 * aWASegment     - 대상 WASegment
 * aPID           - 대상 PID
 ***************************************************************************/
sdtWAGroupID sdtWASegment::findGroup( sdtWASegment * aWASegment,
                                      scPageID       aPID )
{
    UInt i;

    for( i = 0 ; i < SDT_WAGROUPID_MAX ; i ++ )
    {
        if( ( aWASegment->mGroup[ i ].mPolicy != SDT_WA_REUSE_NONE ) &&
            ( aWASegment->mGroup[ i ].mBeginWPID <= aPID ) &&
            ( aWASegment->mGroup[ i ].mEndWPID > aPID ) )
        {
            return i;
        }
    }

    /* 찾지 못함 */
    return SDT_WAGROUPID_MAX;
}

/**************************************************************************
 * Description :
 * WASegemnt 구성을 File로 Dump한다.
 *
 * <IN>
 * aWASegment     - 대상 WASegment
 ***************************************************************************/
void sdtWASegment::exportWASegmentToFile( sdtWASegment * aWASegment )
{
    SChar          sFileName[ SM_MAX_FILE_NAME ];
    SChar          sDateString[ SMI_TT_STR_SIZE ];
    iduFile        sFile;
    UInt           sState = 0;
    UChar        * sWAExtentPtr;
    ULong          sOffset = 0;
    UInt           i;

    IDE_ASSERT( aWASegment != NULL );

    IDE_TEST( sFile.initialize( IDU_MEM_SM_TEMP,
                                1, /* Max Open FD Count */
                                IDU_FIO_STAT_OFF,
                                IDV_WAIT_INDEX_NULL )
              != IDE_SUCCESS);
    sState = 1;

    smuUtility::getTimeString( smiGetCurrTime(),
                               SMI_TT_STR_SIZE,
                               sDateString );

    idlOS::snprintf( sFileName,
                     SM_MAX_FILE_NAME,
                     "%s%c%s.td",
                     smuProperty::getTempDumpDirectory(),
                     IDL_FILE_SEPARATOR,
                     sDateString );

    IDE_TEST( sFile.setFileName( sFileName ) != IDE_SUCCESS);
    IDE_TEST( sFile.createUntilSuccess( smiSetEmergency )
              != IDE_SUCCESS);
    IDE_TEST( sFile.open( ID_FALSE ) != IDE_SUCCESS ); //DIRECT_IO
    sState = 2;

    for( i = 0 ; i < aWASegment->mWAExtentCount ; i ++ )
    {
        sWAExtentPtr = getWAExtentPtr( aWASegment, i );
        IDE_TEST( sFile.write( aWASegment->mStatistics,
                               sOffset,
                               sWAExtentPtr,
                               SDT_WAEXTENT_SIZE )
                  != IDE_SUCCESS );
        sOffset += SDT_WAEXTENT_SIZE;
    }

    IDE_TEST( sFile.sync() != IDE_SUCCESS);

    sState = 1;
    IDE_TEST( sFile.close() != IDE_SUCCESS);

    sState = 0;
    IDE_TEST( sFile.destroy() != IDE_SUCCESS);

    ideLog::log( IDE_DUMP_0, "TEMP_DUMP_FILE : %s", sFileName );

    return;

    IDE_EXCEPTION_END;

    switch( sState )
    {
        case 2:
            (void) sFile.close() ;
        case 1:
            (void) sFile.destroy();
            break;
        default:
            break;
    }

    return;
}

/**************************************************************************
 * Description :
 * File로부터 WASegment를 읽어들인다.
 *
 * <IN>
 * aFileName      - 대상 File
 * <OUT>
 * aWASegment     - 읽어드린 WASegment의 위치
 * aPtr           - WA관련 버퍼의 원본 위치
 * aAlignedPtr    - 위 버퍼를 Align에 따라 재조정한 위치
 ***************************************************************************/
void sdtWASegment::importWASegmentFromFile( SChar         * aFileName,
                                            sdtWASegment ** aWASegment,
                                            UChar        ** aPtr,
                                            UChar        ** aAlignedPtr )
{
    iduFile         sFile;
    UInt            i;
    UInt            sState          = 0;
    UInt            sMemAllocState  = 0;
    ULong           sSize = 0;
    sdtWASegment  * sWASegment = NULL;
    sdtWCB        * sWCBPtr;
    UChar         * sPtr;
    UChar         * sAlignedPtr;

    IDE_TEST( sFile.initialize(IDU_MEM_SM_SMU,
                               1, /* Max Open FD Count */
                               IDU_FIO_STAT_OFF,
                               IDV_WAIT_INDEX_NULL )
              != IDE_SUCCESS );
    sState = 1;

    IDE_TEST( sFile.setFileName( aFileName ) != IDE_SUCCESS );
    IDE_TEST( sFile.open() != IDE_SUCCESS );
    sState = 2;

    IDE_TEST( sFile.getFileSize( &sSize ) != IDE_SUCCESS );
    IDE_TEST( iduMemMgr::malloc( IDU_MEM_SM_TEMP,
                                 sSize + SDT_WAEXTENT_SIZE,
                                 (void**)&sPtr )
              != IDE_SUCCESS );
    sMemAllocState = 1;

    sAlignedPtr = (UChar*)idlOS::align( sPtr, SDT_WAEXTENT_SIZE );

    IDE_TEST( sFile.read( NULL,  /* aStatistics */
                          0,     /* aWhere */
                          sAlignedPtr,
                          sSize,
                          NULL ) /* aEmergencyFunc */
              != IDE_SUCCESS );

    /* Pointer 값들은 기존 메모리 위치로 잘못되어 있으니, 재조정한다. */
    sWASegment              =(sdtWASegment*)getFrameInExtent( sAlignedPtr, 0 );
    sWASegment->mHintWCBPtr =getWCBInExtent( sAlignedPtr, 0 );

    /* Extent위치 재조정 */
    for( i = 0 ; i < sWASegment->mWAExtentCount ; i ++ )
    {
        IDE_TEST( setWAExtentPtr( sWASegment,
                                  i,
                                  sAlignedPtr + i * SDT_WAEXTENT_SIZE )
                  != IDE_SUCCESS );
    }

    /* WCB 위치 재조정 */
    for( i = 0 ;
         i < sWASegment->mWAExtentCount * SDT_WAEXTENT_PAGECOUNT ;
         i ++ )
    {
        sWCBPtr = getWCB( sWASegment, i );
        bindWCB( sWASegment, sWCBPtr, i );
    }

    /* Map의 Segment위치 및 Group과의 Pointer 재조정 */
    IDE_TEST( sdtWAMap::resetPtrAddr( (void*)&sWASegment->mNExtentMap,
                                      sWASegment )
              != IDE_SUCCESS );
    IDE_TEST( sdtWAMap::resetPtrAddr( (void*)&sWASegment->mSortHashMapHdr,
                                      sWASegment )
              != IDE_SUCCESS );

    sState = 1;
    IDE_TEST( sFile.close() != IDE_SUCCESS );
    sState = 0;
    IDE_TEST( sFile.destroy() != IDE_SUCCESS );

    *aPtr        = sPtr;
    *aAlignedPtr = sAlignedPtr;
    *aWASegment  = sWASegment;

    return;

    IDE_EXCEPTION_END;

    switch( sMemAllocState )
    {
        case 1:
            IDE_ASSERT( iduMemMgr::free( sPtr ) == IDE_SUCCESS );
            sPtr = NULL;
        default:
            break;
    }

    switch(sState)
    {
        case 2:
            IDE_ASSERT( sFile.close() == IDE_SUCCESS );
        case 1:
            IDE_ASSERT( sFile.destroy() == IDE_SUCCESS );
        default:
            break;
    }


    return;
}

void sdtWASegment::dumpWASegment( void           * aWASegment,
                                  SChar          * aOutBuf,
                                  UInt             aOutSize )
{
    sdtWASegment   * sWASegment = (sdtWASegment*)aWASegment;
    SInt             i;

    idlVA::appendFormat(
        aOutBuf,
        aOutSize,
        "DUMP WASEGMENT:\n"
        "\tWASegPtr        : 0x%"ID_xINT64_FMT"\n"
        "\tNExtentCount    : %"ID_UINT32_FMT"\n"
        "\tWAExtentCount   : %"ID_UINT32_FMT"\n"
        "\tSpaceID         : %"ID_UINT32_FMT"\n"
        "\tPageSeqInLFE    : %"ID_UINT32_FMT"\n"
        "\tLinkPtr         : 0x%"ID_xINT64_FMT", 0x%"ID_xINT64_FMT"\n",
        sWASegment,
        sWASegment->mNExtentCount,
        sWASegment->mWAExtentCount,
        sWASegment->mSpaceID,
        sWASegment->mPageSeqInLFE,
        sWASegment->mNext, sWASegment->mPrev );

    idlVA::appendFormat( aOutBuf, aOutSize, "\n" );

    for( i = 0 ; i < SDT_WAGROUPID_MAX ; i ++ )
    {
        dumpWAGroup( sWASegment, i, aOutBuf, aOutSize );
    }

    return;
}

void sdtWASegment::dumpFlushQueue( void           * aWAFQ,
                                   SChar          * aOutBuf,
                                   UInt             aOutSize )
{
    sdtWAFlushQueue * sWAFQ = (sdtWAFlushQueue*)aWAFQ;

    idlVA::appendFormat( aOutBuf,
                         aOutSize,
                         "FLUSH QUEUE:\n"
                         "Begin      : %"ID_UINT32_FMT"\n"
                         "End        : %"ID_UINT32_FMT"\n"
                         "Index      : %"ID_UINT32_FMT"\n",
                         sWAFQ->mFQBegin,
                         sWAFQ->mFQEnd,
                         sWAFQ->mWAFlusherIdx );
    idlVA::appendFormat( aOutBuf, aOutSize, "\n" );

    return;

}

void sdtWASegment::dumpWCBs( void           * aWASegment,
                             SChar          * aOutBuf,
                             UInt             aOutSize )
{
    sdtWASegment   * sWASegment = (sdtWASegment*)aWASegment;
    sdtWCB         * sWCBPtr;
    SInt             i;

    idlVA::appendFormat( aOutBuf,
                         aOutSize,
                         "WCB:\n%5s %5s %4s %4s %8s %8s %8s %8s %8s\n",
                         "WPID",
                         "STATE",
                         "FIX",
                         "FREE",
                         "SPACE_ID",
                         "PAGE_ID",
                         "LRU_PREV",
                         "LRU_NEXT",
                         "HASHNEXT" );

    for( i = 0 ;
         i < (SInt)getWASegmentPageCount( sWASegment ) ;
         i ++ )
    {
        sWCBPtr = getWCB( sWASegment, i );
        dumpWCB( (void*)sWCBPtr,aOutBuf,aOutSize );
    }

    idlVA::appendFormat( aOutBuf, aOutSize, "\n" );

    return;
}

void sdtWASegment::dumpWCB( void           * aWCB,
                            SChar          * aOutBuf,
                            UInt             aOutSize )
{
    sdtWCB   * sWCBPtr;

    sWCBPtr = (sdtWCB*) aWCB;

    idlVA::appendFormat( aOutBuf,
                         aOutSize,
                         "%5"ID_UINT32_FMT
                         " %5"ID_UINT32_FMT
                         " %4"ID_UINT32_FMT
                         " %4"ID_UINT32_FMT
                         " %8"ID_UINT32_FMT
                         " %8"ID_UINT32_FMT
                         " %8"ID_UINT32_FMT
                         " %8"ID_UINT32_FMT"\n",
                         sWCBPtr->mWPageID,
                         sWCBPtr->mWPState,
                         sWCBPtr->mFix,
                         sWCBPtr->mBookedFree,
                         sWCBPtr->mNSpaceID,
                         sWCBPtr->mNPageID,
                         sWCBPtr->mLRUPrevPID,
                         sWCBPtr->mLRUNextPID );
}

void sdtWASegment::dumpWAPageHeaders( void           * aWASegment,
                                      SChar          * aOutBuf,
                                      UInt             aOutSize )
{
    sdtWASegment   * sWASegment = (sdtWASegment*)aWASegment;
    SChar          * sTypeNamePtr;
    SChar            sInvalidName[] = "INVALID";
    sdtTempPageHdr * sPagePtr;
    sdtTempPageType  sType;
    idBool           sSkip = ID_TRUE;
    UInt             i;

    idlVA::appendFormat( aOutBuf,
                         aOutSize,
                         "TEMP PAGE HEADERS:\n"
                         "%8s %16s %16s %16s %16s %8s %8s %8s\n",
                         "WPID",
                         "TAFO", /*TypeAndFreeOffset*/
                         "TYPE",
                         "TYPENAME",
                         "FREEOFF",
                         "PREVPID",
                         "SELFPID",
                         "NEXTPID",
                         "SLOTCNT" );
    for( i = 0 ;
         i < getWASegmentPageCount( sWASegment ) ;
         i ++ )
    {
        sPagePtr = (sdtTempPageHdr*) getWAPagePtr( sWASegment,
                                                   SDT_WAGROUPID_NONE,
                                                   i );

        if( ( sPagePtr->mTypeAndFreeOffset == 0 ) &&
            ( sPagePtr->mPrevPID == 0 ) &&
            ( sPagePtr->mNextPID == 0 ) &&
            ( sPagePtr->mSlotCount  == 0 ) )
        {
            sSkip = ID_TRUE;
        }
        else
        {
            sSkip = ID_FALSE;
        }

        if( sSkip == ID_FALSE)
        {
            sType = (sdtTempPageType)sdtTempPage::getType( (UChar*)sPagePtr );
            if( ( SDT_TEMP_PAGETYPE_INIT <= sType ) &&
                ( sType < SDT_TEMP_PAGETYPE_MAX ) )
            {
                sTypeNamePtr = sdtTempPage::mPageName[ sType ];
            }
            else
            {
                sTypeNamePtr = sInvalidName;
            }


            idlVA::appendFormat( aOutBuf,
                                 aOutSize,
                                 "%8"ID_UINT32_FMT
                                 " %16"ID_UINT64_FMT
                                 " %16"ID_UINT32_FMT
                                 " %16s"
                                 " %16"ID_UINT32_FMT
                                 " %8"ID_UINT32_FMT
                                 " %8"ID_UINT32_FMT
                                 " %8"ID_UINT32_FMT
                                 " %8"ID_UINT32_FMT"\n",
                                 i,
                                 sPagePtr->mTypeAndFreeOffset,
                                 sdtTempPage::getType( (UChar*)sPagePtr ),
                                 sTypeNamePtr,
                                 sdtTempPage::getFreeOffset( (UChar*)sPagePtr ),
                                 sPagePtr->mPrevPID,
                                 sPagePtr->mSelfPID,
                                 sPagePtr->mNextPID,
                                 sPagePtr->mSlotCount );
        }
    }

    idlVA::appendFormat( aOutBuf, aOutSize, "\n" );

    return;
}

void sdtWASegment::dumpWAGroup( sdtWASegment   * aWASegment,
                                sdtWAGroupID     aWAGroupID,
                                SChar          * aOutBuf,
                                UInt             aOutSize )
{
    sdtWAGroup     * sGrpInfo = getWAGroupInfo( aWASegment, aWAGroupID );

    if( sGrpInfo->mPolicy != SDT_WA_REUSE_NONE )
    {
        idlVA::appendFormat(
            aOutBuf,
            aOutSize,
            "DUMP WAGROUP:\n"
            "\tID     : %"ID_UINT32_FMT"\n"
            "\tPolicy : %-4"ID_UINT32_FMT
            "(0:None,1:InMemory,2:FIFO,3:LRU)\n"
            "\tRange  : %"ID_UINT32_FMT" <-> %"ID_UINT32_FMT"\n"
            "\tReuseP : %"ID_UINT32_FMT", %"ID_UINT32_FMT"\n"
            "\tHint   : %"ID_UINT32_FMT"\n"
            "\tMapPtr : 0x%"ID_xINT64_FMT"\n\n",
            aWAGroupID,
            sGrpInfo->mPolicy,
            sGrpInfo->mBeginWPID,
            sGrpInfo->mEndWPID,
            sGrpInfo->mReuseWPID1,
            sGrpInfo->mReuseWPID2,
            sGrpInfo->mHintWPID,
            sGrpInfo->mMapHdr );
    }

    return;
}
