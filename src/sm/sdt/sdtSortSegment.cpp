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

#include <sdpDef.h>
#include <smiMisc.h>
#include <smuProperty.h>
#include <sdtWAExtentMgr.h>
#include <sdtWASortMap.h>
#include <sdtSortSegment.h>

iduMutex      sdtSortSegment::mMutex;
smuMemStack   sdtSortSegment::mWASegmentPool;
smuMemStack   sdtSortSegment::mNHashMapPool;

IDE_RC sdtSortSegment::initializeStatic()
{
    UInt    sState = 0;

    IDE_TEST( mMutex.initialize( (SChar*)"SDT_SORT_SEGMENT_MUTEX",
                                 IDU_MUTEX_KIND_NATIVE,
                                 IDV_WAIT_INDEX_NULL )
              != IDE_SUCCESS);
    sState = 1;

    IDE_TEST( mWASegmentPool.initialize( IDU_MEM_SM_TEMP,
                                         getWASegmentSize(),
                                         smuProperty::getTmpInitWASegCnt())
              != IDE_SUCCESS );
    sState = 2;

    IDE_TEST( mNHashMapPool.initialize( IDU_MEM_SM_TEMP,
                                        getWAPageCount() * ID_SIZEOF(sdtWCB*) /
                                        smuProperty::getTempHashBucketDensity(),
                                        smuProperty::getTmpInitWASegCnt() )
              != IDE_SUCCESS );
    sState = 3;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch( sState )
    {
        case 3:
            (void)mNHashMapPool.destroy();
        case 2:
            (void)mWASegmentPool.destroy();
        case 1:
            mMutex.destroy();
            break;
        default:
            break;
    }

    return IDE_FAILURE;
}

/**************************************************************************
 * Description :
 * 서버 내릴때 호출된다. Mutex와 각종 Pool을 free 한다.
 ***************************************************************************/
void sdtSortSegment::destroyStatic()
{
    (void)mWASegmentPool.destroy();

    (void)mNHashMapPool.destroy();

    mMutex.destroy();
}

/**************************************************************************
 * Description : SORT_AREA_SIZE 변경에 따라 Sort Segment 의 크기를 변경한다.
 *               이미 사용중인 Sort Segment는 이전 크기를 유지하고,
 *               새로 생성되는 Sort Segment부터 새 크기로 생성된다.
 *               lock을 잡은 체로 호출 되어야 한다.
 *
 * aNewWorkAreaSize  - [IN] 변경 할 크기
 ***************************************************************************/
IDE_RC sdtSortSegment::resetWASegmentSize( ULong aNewWorkAreaSize )
{
    UInt   sNewWAPageCount   = calcWAPageCount( aNewWorkAreaSize ) ;
    UInt   sNewWASegmentSize = calcWASegmentSize( sNewWAPageCount );
    UInt   sHashBucketCnt    = sNewWAPageCount  / smuProperty::getTempHashBucketDensity();
    UInt   sState = 0;

    IDE_TEST( mWASegmentPool.resetDataSize( sNewWASegmentSize ) != IDE_SUCCESS );
    sState = 1;

    IDE_TEST( mNHashMapPool.resetDataSize( sHashBucketCnt * ID_SIZEOF(sdtWCB*) ) != IDE_SUCCESS );
    sState = 2;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch( sState )
    {
        case 2:
            (void)mNHashMapPool.resetDataSize( getWAPageCount() / smuProperty::getTempHashBucketDensity()
                                               * ID_SIZEOF(sdtWCB*) );
        case 1:
            (void)mWASegmentPool.resetDataSize( getWASegmentSize() );
        default :
            break;
    }
    return IDE_FAILURE;
}

/**************************************************************************
 * Description : __TEMP_INIT_WASEGMENT_COUNT 변경에 따라
 *               Sort Segment 의 크기를 변경한다. 잘 호출되지 않는다.
 *
 * aNewWASegmentCount  - [IN] 변경 할 수
 ***************************************************************************/
IDE_RC sdtSortSegment::resizeWASegmentPool( UInt   aNewWASegmentCount )
{
    idBool sIsLock = ID_FALSE;

    lock();
    sIsLock = ID_TRUE;

    IDE_TEST( mWASegmentPool.resetStackInitCount( aNewWASegmentCount ) != IDE_SUCCESS );

    sIsLock = ID_FALSE;
    unlock();

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sIsLock == ID_TRUE )
    {
        unlock();
    }

    return IDE_FAILURE;
}


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
IDE_RC sdtSortSegment::createSortSegment( smiTempTableHeader * aHeader,
                                          ULong                aInitWorkAreaSize )
{
    sdtSortSegHdr  * sWASeg = NULL;
    sdtWCB        ** sNPageHashPtr = NULL;
    ULong            sMaxWAExtentCount;
    ULong            sInitWAExtentCount;
    UInt             sNPageHashBucketCnt;
    ULong            sMaxWAPageCount;
    sdtSortGroup   * sInitGrpInfo;
    UInt             sState = 0;
    UInt             i;
    idBool           sIsLock = ID_FALSE;

    lock();
    sIsLock = ID_TRUE;
    sMaxWAExtentCount = getWAExtentCount();
    sMaxWAPageCount   = sMaxWAExtentCount * SDT_WAEXTENT_PAGECOUNT;

    IDE_TEST( mWASegmentPool.getDataSize() != calcWASegmentSize( sMaxWAPageCount ) );

    IDE_TEST( mWASegmentPool.pop( (UChar**)&sWASeg ) != IDE_SUCCESS );
    sState = 1;
    IDE_DASSERT( sWASeg != NULL );

    IDE_TEST( mNHashMapPool.pop( (UChar**)&sNPageHashPtr ) != IDE_SUCCESS );
    sState = 2;
    IDE_DASSERT( sNPageHashPtr != NULL );
    sNPageHashBucketCnt = sMaxWAPageCount / smuProperty::getTempHashBucketDensity();

    sIsLock = ID_FALSE;
    unlock();

    idlOS::memset( (UChar*)sWASeg, 0, SDT_SORT_SEGMENT_HEADER_SIZE );

    sWASeg->mNPageHashBucketCnt = sNPageHashBucketCnt;
    sWASeg->mNPageHashPtr       = sNPageHashPtr;
    sWASeg->mMaxWAExtentCount   = sMaxWAExtentCount;

#ifdef DEBUG
    for( i = 0 ; i < sWASeg->mNPageHashBucketCnt ; i++ )
    {
        IDE_ASSERT( sNPageHashPtr[i] == NULL );
    }
#endif

    if ( aInitWorkAreaSize == 0 )
    {
        sInitWAExtentCount = smuProperty::getTmpMinInitWAExtCnt();
    }
    else
    {
        sInitWAExtentCount = sdtWAExtentMgr::calcWAExtentCount( aInitWorkAreaSize );

        // Sort Area와 Row Area각 1개씩
        // 최소한 4개 이상은 되어야 한다.
        if ( sInitWAExtentCount < smuProperty::getTmpMinInitWAExtCnt() )
        {
            sInitWAExtentCount = smuProperty::getTmpMinInitWAExtCnt();
        }
        else
        {
            if ( sMaxWAExtentCount < sInitWAExtentCount )
            {
                sInitWAExtentCount = sMaxWAExtentCount;
            }
        }
    }

    /***************************** initialize ******************************/
    sWASeg->mSpaceID          = aHeader->mSpaceID;
    sWASeg->mWorkType         = SDT_WORK_TYPE_SORT;
    sWASeg->mStatsPtr         = aHeader->mStatsPtr;
    sWASeg->mNPageCount       = 0;
    sWASeg->mIsInMemory       = SDT_WORKAREA_IN_MEMORY;
    sWASeg->mStatistics       = aHeader->mStatistics;
    sWASeg->mAllocWAPageCount = 0;
    sWASeg->mUsedWCBPtr       = SDT_USED_PAGE_PTR_TERMINATED;
    sWASeg->mNExtFstPIDList.mPageSeqInLFE = SDT_WAEXTENT_PAGECOUNT;

    sWASeg->mStatsPtr->mRuntimeMemSize += mWASegmentPool.getNodeSize((UChar*) sWASeg );
    sWASeg->mStatsPtr->mRuntimeMemSize += mNHashMapPool.getNodeSize((UChar*) sNPageHashPtr );

    IDE_TEST( sdtWAExtentMgr::initWAExtents( sWASeg->mStatistics,
                                             sWASeg->mStatsPtr,
                                             &sWASeg->mWAExtentInfo,
                                             sInitWAExtentCount )
              != IDE_SUCCESS );
    sState = 3;

    sWASeg->mCurFreeWAExtent = sWASeg->mWAExtentInfo.mHead;
    sWASeg->mCurrFreePageIdx = 0;

    sWASeg->mHintWCBPtr = getWCBWithLnk( sWASeg, 1 );
    sState = 4;

    /***************************** InitGroup *******************************/
    for( i = 0 ; i< SDT_WAGROUPID_MAX ; i++ )
    {
        sWASeg->mGroup[ i ].mPolicy = SDT_WA_REUSE_NONE;
    }

    /* InitGroup을 생성한다. */
    sInitGrpInfo = &sWASeg->mGroup[ 0 ];
    /* Segment 이후 WAExtentPtr가 배치된 후 Range의 시작이다. */
    sInitGrpInfo->mBeginWPID = 1;
    sInitGrpInfo->mEndWPID = sMaxWAPageCount;

    sInitGrpInfo->mPolicy       = SDT_WA_REUSE_INMEMORY;
    sInitGrpInfo->mSortMapHdr   = NULL;
    sInitGrpInfo->mReuseWPIDSeq = SC_NULL_PID;
    sInitGrpInfo->mReuseWPIDTop = SC_NULL_PID;
    sInitGrpInfo->mReuseWPIDBot = SC_NULL_PID;

    /***************************** NPageMgr *********************************/
    IDE_TEST( sWASeg->mFreeNPageStack.initialize( IDU_MEM_SM_TEMP,
                                                  ID_SIZEOF( scPageID ) )
              != IDE_SUCCESS );
    sState = 5;

    aHeader->mWASegment = sWASeg;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sIsLock == ID_FALSE )
    {
        lock();
    }

    switch( sState )
    {
        case 5:
            sWASeg->mFreeNPageStack.destroy();
        case 4:
            clearAllWCB( sWASeg );
        case 3:
            (void)sdtWAExtentMgr::freeWAExtents( &sWASeg->mWAExtentInfo );
        case 2:
            (void)mNHashMapPool.push( (UChar*)sNPageHashPtr );
        case 1:
            (void)mWASegmentPool.push( (UChar*)sWASeg );
            break;
        default:
            break;
    }

    aHeader->mWASegment = NULL;

    unlock();

    return IDE_FAILURE;
}

/**************************************************************************
 * Description : Segment를 Drop하고 안의 Extent도 반환한다.
 *
 * <IN> aWASegment     - 대상 WASegment
 ***************************************************************************/
IDE_RC sdtSortSegment::dropSortSegment( sdtSortSegHdr* aWASegment )
{
    if ( aWASegment != NULL )
    {
        IDE_TEST( dropAllWAGroup( aWASegment ) != IDE_SUCCESS );

        IDE_TEST( clearSortSegment( aWASegment ) != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**************************************************************************
 * Description : Segment를 재활용한다.
 *
 * <IN>
 * aWASegment     - 대상 WASegment
 ***************************************************************************/
IDE_RC sdtSortSegment::clearSortSegment( sdtSortSegHdr* aWASegment )
{
    UInt     sState = 0;
    idBool   sIsLock = ID_FALSE;

    if ( aWASegment != NULL )
    {
        sState = 1;

        aWASegment->mNExtFstPIDList.mLastFreeExtFstPID = SM_NULL_PID;

        clearAllWCB( aWASegment );

        sState = 2;
        IDE_TEST( sdtWAExtentMgr::freeWAExtents( &aWASegment->mWAExtentInfo ) != IDE_SUCCESS );

        sState = 3;
        IDE_TEST( sdtWAExtentMgr::freeAllNExtents( aWASegment->mSpaceID,
                                                   &aWASegment->mNExtFstPIDList ) != IDE_SUCCESS );

        IDE_TEST( aWASegment->mFreeNPageStack.destroy() != IDE_SUCCESS );

        lock();
        sIsLock = ID_TRUE;

        sState = 4;
        IDE_TEST( mNHashMapPool.push( (UChar*)(aWASegment->mNPageHashPtr) ) != IDE_SUCCESS );

        aWASegment->mNPageHashPtr       = NULL;
        aWASegment->mNPageHashBucketCnt = 0;

        sState = 5;
        IDE_TEST( mWASegmentPool.push( (UChar*)aWASegment ) != IDE_SUCCESS );

        sIsLock = ID_FALSE;
        unlock();
    }
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch ( sState )
    {
        /* BUG-42751 종료 단계에서 예외가 발생하면 해당 자원이 정리되기를 기다리면서
           server stop 시 HANG이 발생할수 있으므로 정리해줘야 함. */
        case 1:
            clearAllWCB( aWASegment );
            (void)sdtWAExtentMgr::freeWAExtents( &aWASegment->mWAExtentInfo );
        case 2:
            (void)sdtWAExtentMgr::freeAllNExtents( aWASegment->mSpaceID,
                                                   &aWASegment->mNExtFstPIDList );
        case 3:
            if ( sIsLock == ID_FALSE )
            {
                lock();
                sIsLock = ID_TRUE;
            }
            (void)mNHashMapPool.push( (UChar*)(aWASegment->mNPageHashPtr) );
        case 4:
            if ( sIsLock == ID_FALSE )
            {
                lock();
                sIsLock = ID_TRUE;
            }
            (void)mWASegmentPool.push( (UChar*)aWASegment );
        default:
            break;
    }
    if ( sIsLock == ID_TRUE )
    {
        unlock();
    }
    return IDE_FAILURE;
}

/**************************************************************************
 * Description : Segment를 재활용한다.
 *
 * <IN>
 * aWASegment     - 대상 WASegment
 ***************************************************************************/
IDE_RC sdtSortSegment::truncateSortSegment( sdtSortSegHdr* aWASegment )
{
    sdtWCB         * sWCBPtr;
    sdtWCB         * sNxtWCBPtr;
    UInt             sCount = 0;

    IDE_ERROR( aWASegment != NULL );

    IDE_TEST( dropAllWAGroup( aWASegment ) != IDE_SUCCESS );

    /* 전부 Init상태로 만든다. */
    sWCBPtr = aWASegment->mUsedWCBPtr;
    while( sWCBPtr != SDT_USED_PAGE_PTR_TERMINATED )
    {
        if ( sWCBPtr->mWPState == SDT_WA_PAGESTATE_INIT )
        {
            initWCB( sWCBPtr );

            IDE_DASSERT( aWASegment->mGroup[1].mHintWCBPtr != sWCBPtr );
        }
        else
        {
            IDE_DASSERT( ( sWCBPtr->mWPState == SDT_WA_PAGESTATE_CLEAN ) ||
                         ( sWCBPtr->mWPState == SDT_WA_PAGESTATE_DIRTY ));

            /* Initpage로 만들 필요가 있음.
             * assign 돼어있기 때문 */
            IDE_TEST( makeInitPage( aWASegment, 
                                    sWCBPtr, 
                                    ID_FALSE ) // aWait4Flush
                      != IDE_SUCCESS );
        }

        sCount++;
        sNxtWCBPtr = sWCBPtr->mNxtUsedWCBPtr;

        sWCBPtr->mNxtUsedWCBPtr = NULL;
        sWCBPtr->mWAPagePtr     = NULL;

        sWCBPtr = sNxtWCBPtr;
    }
    aWASegment->mUsedWCBPtr      = SDT_USED_PAGE_PTR_TERMINATED;
    aWASegment->mCurFreeWAExtent = aWASegment->mWAExtentInfo.mHead;
    aWASegment->mCurrFreePageIdx = 0;
    aWASegment->mHintWCBPtr      = getWCBWithLnk( aWASegment, 1 );

    aWASegment->mAllocWAPageCount = 0;

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
IDE_RC sdtSortSegment::createWAGroup( sdtSortSegHdr    * aWASegment,
                                      sdtGroupID         aWAGroupID,
                                      UInt               aPageCount,
                                      sdtWAReusePolicy   aPolicy )
{
    sdtSortGroup   *sInitGrpInfo = getWAGroupInfo( aWASegment, SDT_WAGROUPID_INIT );
    sdtSortGroup   *sGrpInfo     = getWAGroupInfo( aWASegment, aWAGroupID );

    IDE_ERROR( sGrpInfo->mPolicy == SDT_WA_REUSE_NONE );

    /* 크기를 지정하지 않았으면, 남은 용량을 전부 부여함 */
    if ( aPageCount == 0 )
    {
        aPageCount = getAllocableWAGroupPageCount( sInitGrpInfo );
    }

    IDE_ERROR( aPageCount >= SDT_WAGROUP_MIN_PAGECOUNT );

    /* 다음과 같이 InitGroup에서 때어줌
     * <---InitGroup---------------------->
     * <---InitGroup---><----CurGroup----->*/
    IDE_ERROR( sInitGrpInfo->mEndWPID >= aPageCount );  /* 음수체크 */
    sGrpInfo->mBeginWPID = sInitGrpInfo->mEndWPID - aPageCount;
    sGrpInfo->mEndWPID   = sInitGrpInfo->mEndWPID;
    sInitGrpInfo->mEndWPID -= aPageCount;
    IDE_ERROR( sInitGrpInfo->mBeginWPID <= sInitGrpInfo->mEndWPID );

    sGrpInfo->mHintWCBPtr = NULL;
    sGrpInfo->mPolicy     = aPolicy;

    IDE_TEST( clearWAGroup( sGrpInfo ) != IDE_SUCCESS );
    IDE_DASSERT( validateLRUList( aWASegment, sGrpInfo ) == IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**************************************************************************
 * Description : WAGroup을 재활용 할 수 있도록 리셋한다.
 *
 * <IN>
 * aWASegment     - 대상 WASegment
 * aWAGroupID     - 대상 Group ID
 * aWait4Flush    - Dirty한 Page들을 Flush될때까지 기다릴 것인가.
 ***************************************************************************/
IDE_RC sdtSortSegment::resetWAGroup( sdtSortSegHdr     * aWASegment,
                                     sdtGroupID          aWAGroupID,
                                     idBool              aWait4Flush )
{
    sdtSortGroup   * sGrpInfo = getWAGroupInfo(aWASegment,aWAGroupID);
    sdtWCB         * sWCBPtr;
    sdtWCB         * sEndWCBPtr;

    if ( sGrpInfo->mPolicy != SDT_WA_REUSE_NONE )
    {
        if ( aWait4Flush == ID_TRUE )
        {
            /* 여기서 HintPage를 초기화하여 Unfix하지 않으면,
             * 이후 makeInit단계에서 hintPage를 unassign할 수 없어
             * 오류가 발생한다. */
            setHintWCB( sGrpInfo, NULL );

            sWCBPtr = getWCBInternal( aWASegment,
                                      sGrpInfo->mBeginWPID );

            sEndWCBPtr = getWCBInternal( aWASegment,
                                         sGrpInfo->mEndWPID - 1 );

            /* Write가 필요한 Page를 모두 Write 한다.
             * makeInitPage에서 하나씩 하면서 대기하면 느리다. */
            /* Disk에 Bind되는 Page는 역순 할당 되면 안된다. */
            while(( sWCBPtr <= sEndWCBPtr ) &&
                  ( sWCBPtr->mNxtUsedWCBPtr != NULL ))
            { 
                if ( sWCBPtr->mWPState == SDT_WA_PAGESTATE_DIRTY )
                {
                    IDE_TEST( writeNPage( aWASegment, sWCBPtr ) != IDE_SUCCESS );
                }
                sWCBPtr++;
            }

            sWCBPtr = getWCBInternal( aWASegment,
                                      sGrpInfo->mBeginWPID );

            while(( sWCBPtr <= sEndWCBPtr ) &&
                  ( sWCBPtr->mNxtUsedWCBPtr != NULL ))
            {
                IDE_TEST( makeInitPage( aWASegment,
                                        sWCBPtr,
                                        ID_TRUE )
                          != IDE_SUCCESS );
                sWCBPtr++;
            }
        }
        IDE_TEST( clearWAGroup( sGrpInfo ) != IDE_SUCCESS );
        IDE_DASSERT( validateLRUList( aWASegment, sGrpInfo ) == IDE_SUCCESS );
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
IDE_RC sdtSortSegment::clearWAGroup( sdtSortGroup      * aGrpInfo )
{
    aGrpInfo->mSortMapHdr = NULL;
    setHintWCB( aGrpInfo, NULL );

    /* 재활용 정책에 따라 초기값 설정함. */
    switch( aGrpInfo->mPolicy  )
    {
        case SDT_WA_REUSE_INMEMORY:
            /* InMemoryGroup에 대한 초기화. 페이지 재활용 자료구조를 설정한다.
             * 뒤에서 앞으로 가며 페이지 활용함.
             * 왜냐하면 InMemoryGroup은 WAMap을 장착할 수 있으며,
             * WAMap은 앞에서 뒤쪽으로 확장하면서 부딪힐 수 있기 때문임 */
            aGrpInfo->mReuseWPIDSeq = aGrpInfo->mEndWPID - 1;
            break;
        case SDT_WA_REUSE_FIFO:
            /* FIFOGroup에 대한 초기화. 페이지 재활용 자료구조를 설정한다.
             * 앞에서 뒤로 가며 페이지 활용함. MultiBlockWrite를 유도하기위해. */
            aGrpInfo->mReuseWPIDSeq = aGrpInfo->mBeginWPID;
            break;
        case SDT_WA_REUSE_LRU:
            /* LRUGroup에 대한 초기화. 페이지 재활용 자료구조를 설정한다.
             * MultiBlockWrite를 유도하기위해 앞에서 뒤로 가며 페이지 활용하도록
             * 유도해둠 */
            aGrpInfo->mReuseWPIDSeq = aGrpInfo->mBeginWPID;
            aGrpInfo->mReuseWPIDTop = SC_NULL_PID;
            aGrpInfo->mReuseWPIDBot = SC_NULL_PID;
            break;
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
 * 두 Group을 하나로 (aWAGroupID1 쪽으로) 합친다.
 *
 * <IN>
 * aWASegment     - 대상 WASegment
 * aWAGroupID1    - 대상 GroupID ( 이쪽으로 합쳐짐 )
 * aWAGroupID2    - 대상 GroupID ( 소멸됨 )
 * aPolicy        - 합쳐진 group이 선택할 정책
 ***************************************************************************/
IDE_RC sdtSortSegment::mergeWAGroup(sdtSortSegHdr    * aWASegment,
                                    sdtGroupID         aWAGroupID1,
                                    sdtGroupID         aWAGroupID2,
                                    sdtWAReusePolicy   aPolicy )
{
    sdtSortGroup   * sGrpInfo1 = getWAGroupInfo( aWASegment, aWAGroupID1 );
    sdtSortGroup   * sGrpInfo2 = getWAGroupInfo( aWASegment, aWAGroupID2 );

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

    IDE_TEST( clearWAGroup( sGrpInfo1 ) != IDE_SUCCESS );
    IDE_DASSERT( validateLRUList( aWASegment, sGrpInfo1 ) == IDE_SUCCESS );

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
IDE_RC sdtSortSegment::splitWAGroup(sdtSortSegHdr    * aWASegment,
                                    sdtGroupID         aSrcWAGroupID,
                                    sdtGroupID         aDstWAGroupID,
                                    sdtWAReusePolicy   aPolicy )
{
    sdtSortGroup   * sSrcGrpInfo= getWAGroupInfo( aWASegment, aSrcWAGroupID );
    sdtSortGroup   * sDstGrpInfo= getWAGroupInfo( aWASegment, aDstWAGroupID );
    UInt             sPageCount;

    IDE_ERROR( sDstGrpInfo->mPolicy == SDT_WA_REUSE_NONE );

    sPageCount = getAllocableWAGroupPageCount( sSrcGrpInfo )/2;

    IDE_ERROR( sPageCount >= SDT_WAGROUP_MIN_PAGECOUNT );

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
    sDstGrpInfo->mHintWCBPtr  = NULL;
    sDstGrpInfo->mPolicy      = aPolicy;

    IDE_TEST( clearWAGroup( sSrcGrpInfo ) != IDE_SUCCESS );
    IDE_TEST( clearWAGroup( sDstGrpInfo ) != IDE_SUCCESS );
    IDE_DASSERT( validateLRUList( aWASegment, sSrcGrpInfo ) == IDE_SUCCESS );
    IDE_DASSERT( validateLRUList( aWASegment, sDstGrpInfo ) == IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**************************************************************************
 * Description : 모든 Group을 Drop한다.
 *
 * <IN>
 * aWASegment     - 대상 WASegment
 ***************************************************************************/
IDE_RC sdtSortSegment::dropAllWAGroup( sdtSortSegHdr* aWASegment )
{
    SInt      sWAGroupID;

    for( sWAGroupID = SDT_WAGROUPID_MAX - 1 ; sWAGroupID >  SDT_WAGROUPID_INIT ; sWAGroupID-- )
    {
        if ( aWASegment->mGroup[ sWAGroupID ].mPolicy != SDT_WA_REUSE_NONE )
        {
            IDE_TEST( dropWAGroup( aWASegment,
                                   sWAGroupID,
                                   ID_FALSE ) != IDE_SUCCESS );

            aWASegment->mGroup[ sWAGroupID ].mPolicy = SDT_WA_REUSE_NONE;
        }
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
IDE_RC sdtSortSegment::dropWAGroup(sdtSortSegHdr* aWASegment,
                                   sdtGroupID     aWAGroupID,
                                   idBool         aWait4Flush)
{
    sdtSortGroup   * sInitGrpInfo =
        getWAGroupInfo( aWASegment, SDT_WAGROUPID_INIT );
    sdtSortGroup   * sGrpInfo = getWAGroupInfo( aWASegment, aWAGroupID );

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
 * Description : Group의 정책에 따라 재사용할 수 있는 WAPage를 반환한다.
 *               실패하면 NULL Pointer가 Return된다.
 *
 * <IN>
 * aWASegment     - 대상 WASegment
 * aGrpInfo       - 대상 Group
 * <OUT>
 * aWCBPtr        - 탐색한 FreeWAPage의 WCB
 ***************************************************************************/
IDE_RC sdtSortSegment::getFreeWAPageINMEMORY( sdtSortSegHdr* aWASegment,
                                              sdtSortGroup * aGrpInfo,
                                              sdtWCB      ** aWCBPtr )
{
    sdtWCB * sWCBPtr;
    scPageID sMapEndPID;

    /* sort area , hash group */
    IDE_ERROR( aGrpInfo->mSortMapHdr != NULL );
    IDE_DASSERT( aGrpInfo->mPolicy == SDT_WA_REUSE_INMEMORY );

    sMapEndPID = sdtWASortMap::getEndWPID( aGrpInfo->mSortMapHdr );

    /* Map이 있는 곳까지 썼으면, 다 썼다는 이야기 */
    if ( sMapEndPID >= aGrpInfo->mReuseWPIDSeq )
    {
        /* InMemoryGroup은 victimReplace가 안된다.
         * 초기화 하고, NULL을 반환한다.*/
        aGrpInfo->mReuseWPIDSeq = aGrpInfo->mEndWPID - 1;

        *aWCBPtr = NULL;
    }
    else
    {
        IDE_ASSERT( aGrpInfo->mReuseWPIDSeq != SD_NULL_PID );

        sWCBPtr = getWCBWithLnk( aWASegment,
                                 aGrpInfo->mReuseWPIDSeq );

        if ( sWCBPtr->mWAPagePtr == NULL )
        {
            IDE_TEST( setWAPagePtr(aWASegment, sWCBPtr) != IDE_SUCCESS );
            IDE_ASSERT( sWCBPtr->mWAPagePtr != NULL );
        }

        aGrpInfo->mReuseWPIDSeq--;

        /* 할당받은 페이지를 빈 페이지로 만듬 */
        IDE_TEST( makeInitPage( aWASegment,
                                sWCBPtr,
                                ID_TRUE ) /*wait 4 flush */
                  != IDE_SUCCESS );

        (*aWCBPtr) = sWCBPtr;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**************************************************************************
 * Description : Group의 정책에 따라 재사용할 수 있는 WAPage를 반환한다.
 *               성공 할 때 까지 재시도 된다.
 *
 * <IN>
 * aWASegment     - 대상 WASegment
 * aGrpInfo       - 대상 Group
 * <OUT>
 * aWCBPtr        - 탐색한 FreeWAPage의 WCB
 ***************************************************************************/
IDE_RC sdtSortSegment::getFreeWAPageFIFO( sdtSortSegHdr* aWASegment,
                                          sdtSortGroup * aGrpInfo,
                                          sdtWCB      ** aWCBPtr )
{
    scPageID     sRetPID;
    sdtWCB     * sWCBPtr;

    IDE_DASSERT( aGrpInfo->mPolicy ==  SDT_WA_REUSE_FIFO );
    IDE_ERROR( aGrpInfo->mSortMapHdr == NULL );

    while( 1 )
    {
        /* hash sub group */
        sRetPID = aGrpInfo->mReuseWPIDSeq;
        aGrpInfo->mReuseWPIDSeq++;

        /* Group을 한번 전부 순회함 */
        if ( aGrpInfo->mReuseWPIDSeq == aGrpInfo->mEndWPID )
        {
            aGrpInfo->mReuseWPIDSeq = aGrpInfo->mBeginWPID;
        }

        sWCBPtr = getWCBWithLnk( aWASegment,
                                 sRetPID );

        if ( sWCBPtr->mWAPagePtr == NULL )
        {
            IDE_TEST( setWAPagePtr( aWASegment,
                                    sWCBPtr ) != IDE_SUCCESS );

            IDE_ASSERT( isFixedPage( sWCBPtr ) == ID_FALSE );
            break;
        }

        if ( isFixedPage( sWCBPtr ) == ID_FALSE )
        {
            /* dirty page는 write한다.
             * clean한 page가 하나도 없을 경우에도 한바퀴 돌아서 제자리에 오면
             * write된 page를 할당 받을 수 있게된다. */
            if ( sWCBPtr->mWPState == SDT_WA_PAGESTATE_DIRTY )
            {
                IDE_TEST( writeNPage( aWASegment, sWCBPtr ) != IDE_SUCCESS );
            }
            break;
        }
        else
        {
            /* Fix된 page는 new page로 할당 받을 수 없다.
             * 수정 가능성이 있으므로 writeNPage도 하지 않는다. */   
        }
    }

    IDE_DASSERT( sRetPID != SD_NULL_PID );
    /* 할당받은 페이지를 빈 페이지로 만듬 */

    /* Assign되어있으면 Assign 된 페이지를 초기화시켜준다. */
    if ( sWCBPtr->mWPState == SDT_WA_PAGESTATE_CLEAN )
    {
        IDE_TEST( unassignNPage( aWASegment,
                                 sWCBPtr )
                  != IDE_SUCCESS );
    }

    initWCB( sWCBPtr );

    (*aWCBPtr) = sWCBPtr;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**************************************************************************
 * Description : Group의 정책에 따라 재사용할 수 있는 WAPage를 반환한다.
 *               성공 할 때 까지 재시도 된다.
 *
 * <IN>
 * aWASegment     - 대상 WASegment
 * aGrpInfo       - 대상 Group
 * <OUT>
 * aWCBPtr        - 탐색한 FreeWAPage의 WCB
 ***************************************************************************/
IDE_RC sdtSortSegment::getFreeWAPageLRU( sdtSortSegHdr* aWASegment,
                                         sdtSortGroup * aGrpInfo,
                                         sdtWCB      ** aWCBPtr )
{
    scPageID     sRetPID;
    sdtWCB     * sWCBPtr = NULL;
    sdtWCB     * sOldTopWCBPtr;

    IDE_DASSERT( aGrpInfo->mPolicy ==  SDT_WA_REUSE_LRU );
    IDE_DASSERT( aGrpInfo->mSortMapHdr == NULL );

    while(1)
    {
        /* clearGroup 후 처음에는 순차적으로 할당한다,
         * 이후 EndWPageID까지 도달하면 LRU List를 따라 할당한다.*/
        /* clearGroup은 하였지만, 아직 Dirty Page가 존재하는 경우
         * Free Page 탐색 도중에 순차 할당에서 LRU 할당으로 넘어 갈 수도 있다.
         * 즉 Reuse 확인은 while()안에서 해야 한다.*/
        if ( aGrpInfo->mReuseWPIDSeq < aGrpInfo->mEndWPID )
        {
            /* 아직 한번도 재활용 안된 페이지가 남았음 */
            sRetPID = aGrpInfo->mReuseWPIDSeq;
            sWCBPtr = getWCBWithLnk( aWASegment, sRetPID );

            aGrpInfo->mReuseWPIDSeq++;

            if ( aGrpInfo->mReuseWPIDTop == SC_NULL_PID )
            {
                /* 최초 등록인 경우 */
                IDE_ERROR( aGrpInfo->mReuseWPIDBot == SC_NULL_PID );

                sWCBPtr->mLRUPrevPID    = SC_NULL_PID;
                sWCBPtr->mLRUNextPID    = SC_NULL_PID;
                aGrpInfo->mReuseWPIDTop = sRetPID;
                aGrpInfo->mReuseWPIDBot = sRetPID;
            }
            else
            {
                /* 이미 List가 구성되었을 경우, 선두에 매담 */
                sOldTopWCBPtr = getWCBInternal( aWASegment,
                                                aGrpInfo->mReuseWPIDTop );

                sWCBPtr->mLRUPrevPID       = SD_NULL_PID;
                sWCBPtr->mLRUNextPID       = aGrpInfo->mReuseWPIDTop;
                sOldTopWCBPtr->mLRUPrevPID = sRetPID;
                aGrpInfo->mReuseWPIDTop    = sRetPID;
            }

            IDE_DASSERT( sRetPID != SD_NULL_PID );

            if ( sWCBPtr->mWAPagePtr == NULL )
            {
                IDE_TEST( setWAPagePtr( aWASegment,
                                        sWCBPtr ) != IDE_SUCCESS );
                IDE_DASSERT( isFixedPage( sWCBPtr ) == ID_FALSE );

                (*aWCBPtr) = sWCBPtr;
                break;
            }

            if ( isFixedPage( sWCBPtr ) == ID_FALSE )
            {
                // Dirty이면 Write하고 가져감
                if ( sWCBPtr->mWPState == SDT_WA_PAGESTATE_DIRTY )
                {
                    IDE_TEST( writeNPage( aWASegment, sWCBPtr ) != IDE_SUCCESS );
                }
                (*aWCBPtr) = sWCBPtr;
                break;
            }
            else
            {
                /* Fix된 page는 new page로 할당 받을 수 없다.
                 * 수정 될 가능성이 있으므로 writeNPage도 하지 않는다. */   
            }
        }
        else
        {
            IDE_TEST( getFreeWAPageLRUfromDisk( aWASegment,
                                                aGrpInfo,
                                                aWCBPtr )
                      != IDE_SUCCESS );
            sWCBPtr = (*aWCBPtr);
            break;
        }
    }

    /* 할당받은 페이지를 빈 페이지로 만듬 */

    IDE_TEST( makeInitPage( aWASegment,
                            sWCBPtr,
                            ID_TRUE ) /*wait 4 flush */
              != IDE_SUCCESS );

    aWASegment->mHintWCBPtr = sWCBPtr;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdtSortSegment::getFreeWAPageLRUfromDisk( sdtSortSegHdr* aWASegment,
                                                 sdtSortGroup * aGrpInfo,
                                                 sdtWCB      ** aWCBPtr )
{
    sdtWCB     * sWCBPtr;
    sdtWCB     * sBeginWCBPtr;

    // List에 따라 재활용함 
    sBeginWCBPtr = getWCBInternal( aWASegment,
                                   aGrpInfo->mReuseWPIDBot );
    sWCBPtr = sBeginWCBPtr;

    while( isFixedPage( sWCBPtr ) == ID_TRUE )
    {
        if ( sWCBPtr->mLRUPrevPID == SD_NULL_PID )
        {
            // 찾지 못했다, 처음부터 다시 한다.
            sWCBPtr = sBeginWCBPtr;

            continue;
        }

        sWCBPtr = getWCBInternal( aWASegment,
                                  sWCBPtr->mLRUPrevPID );
    }

    // Dirty이면 Clean하고 가져감
    if ( sWCBPtr->mWPState == SDT_WA_PAGESTATE_DIRTY )
    {
        IDE_TEST( writeNPage( aWASegment, sWCBPtr ) != IDE_SUCCESS );
    }

    if ( aGrpInfo->mReuseWPIDTop != getWPageID( aWASegment,
                                                sWCBPtr ) )
    {
        moveLRUListToTopInternal( aWASegment,
                                  aGrpInfo,
                                  sWCBPtr );
    }

    (*aWCBPtr) = sWCBPtr;

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
IDE_RC sdtSortSegment::makeInitPage( sdtSortSegHdr* aWASegment,
                                     sdtWCB       * aWCBPtr,
                                     idBool         aWait4Flush )
{
    /* Page를 할당받았을때, 해당 Page는 아직 Write돼지 않았을 수 있다.
     * 그러면 강제로 Write를 시도한다. */
    if ( aWCBPtr->mWPState == SDT_WA_PAGESTATE_DIRTY )
    {
        if ( aWait4Flush == ID_TRUE )
        {
            IDE_TEST( writeNPage( aWASegment,
                                  aWCBPtr ) != IDE_SUCCESS );
        }
        else
        {
            aWCBPtr->mWPState = SDT_WA_PAGESTATE_CLEAN ;
        }
    }

    /* Assign되어있으면 Assign 된 페이지를 초기화시켜준다. */
    if ( aWCBPtr->mWPState == SDT_WA_PAGESTATE_CLEAN )
    {
        aWASegment->mIsInMemory = SDT_WORKAREA_OUT_MEMORY;
        IDE_TEST( unassignNPage( aWASegment,
                                 aWCBPtr )
                  != IDE_SUCCESS );
        initWCB( aWCBPtr );
    }
    else
    {
        /* nothing to do */
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
IDE_RC sdtSortSegment::writeNPage( sdtSortSegHdr* aWASegment,
                                   sdtWCB       * aWCBPtr )
{
    sdtWCB    * sPrvWCBPtr ;
    sdtWCB    * sCurWCBPtr;
    sdtWCB    * sEndWCBPtr;
    UInt        sPageCount = 1 ;
    
    /* dirty인 것 만 flush queue에 들어간다.
     * 하지만 아니라도 Write하면 된다.*/
    IDE_DASSERT( aWCBPtr->mWPState == SDT_WA_PAGESTATE_DIRTY );

    sEndWCBPtr = getWCBInternal( aWASegment,
                                 getMaxWAPageCount( aWASegment ) - 1 );

    sPrvWCBPtr = aWCBPtr;
    sCurWCBPtr = aWCBPtr + 1;

    while(( sCurWCBPtr <= sEndWCBPtr ) &&
          ( sCurWCBPtr->mWPState   == SDT_WA_PAGESTATE_DIRTY ) &&
          ( sCurWCBPtr->mNPageID   == ( sPrvWCBPtr->mNPageID + 1 ) ) &&
          ( sCurWCBPtr->mWAPagePtr == ( sPrvWCBPtr->mWAPagePtr + SD_PAGE_SIZE )))
    {
        sCurWCBPtr->mWPState = SDT_WA_PAGESTATE_CLEAN;
        sCurWCBPtr++;
        sPageCount++;
        sPrvWCBPtr++;
    }

    IDE_TEST( sddDiskMgr::writeMultiPage( aWASegment->mStatistics,
                                          aWASegment->mSpaceID,
                                          aWCBPtr->mNPageID,
                                          sPageCount,
                                          aWCBPtr->mWAPagePtr )
              != IDE_SUCCESS );

    aWCBPtr->mWPState = SDT_WA_PAGESTATE_CLEAN;

    aWASegment->mStatsPtr->mWriteCount++;
    aWASegment->mStatsPtr->mWritePageCount += sPageCount;

    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;

    /* sPageCount 만큼 Dirty -> Clean으로 세팅 되었다.
     * 다시 Dirty로 되돌려준다.*/
    sEndWCBPtr = aWCBPtr + sPageCount;

    for( sCurWCBPtr = aWCBPtr ;
         sCurWCBPtr < sEndWCBPtr ;
         sCurWCBPtr++ )
    {
        sCurWCBPtr->mWPState = SDT_WA_PAGESTATE_DIRTY;
    }

    ideLog::log( IDE_DUMP_0,
                 "Sort Temp WriteNPage Error\n"
                 "write page count : %"ID_UINT32_FMT"\n",
                 sPageCount );

    smuUtility::dumpFuncWithBuffer( IDE_DUMP_0,
                                    sdtSortSegment::dumpWCB,
                                    aWCBPtr );

    return IDE_FAILURE;
}

/**************************************************************************
 * Description :
 * WCB에 해당 SpaceID 및 PageID를 설정한다. 즉 Page를 읽는 것이 아니라
 * 설정만 한 것으로, Disk상에 있는 일반Page의 내용은 '무시'된다.

 * <IN>
 * aWASegment     - 대상 WASegment
 * aWPID          - 대상 WPID
 * aNPID          - 대상 NPage의 주소
 ***************************************************************************/
IDE_RC sdtSortSegment::assignNPage(sdtSortSegHdr* aWASegment,
                                   sdtWCB       * aWCBPtr,
                                   scPageID       aNPageID)
{
    UInt             sHashValue;

    /* 검증. 이미 Hash이 매달린 것이 아니어야 함 */
    IDE_DASSERT( findWCB( aWASegment, aNPageID )
                 == NULL );
    /* 제대로된 WPID여야 함 */
    IDE_DASSERT( getWPageID( aWASegment, aWCBPtr ) < getMaxWAPageCount( aWASegment ) );

    /* NPID 로 연결함 */
    IDE_ERROR( aWCBPtr->mNPageID  == SC_NULL_PID );

    aWCBPtr->mNPageID    = aNPageID;
    aWCBPtr->mBookedFree = ID_FALSE;
    aWCBPtr->mWPState    = SDT_WA_PAGESTATE_CLEAN;

    /* Hash에 연결하기 */
    sHashValue = getNPageHashValue( aWASegment, aNPageID );
    aWCBPtr->mNextWCB4Hash = aWASegment->mNPageHashPtr[sHashValue];
    aWASegment->mNPageHashPtr[sHashValue] = aWCBPtr;

    /* Hash에 매달려서, 탐색되어야 함 */
    IDE_DASSERT( findWCB( aWASegment, aNPageID )
                 == aWCBPtr );

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
IDE_RC sdtSortSegment::unassignNPage( sdtSortSegHdr* aWASegment,
                                      sdtWCB       * aWCBPtr )
{
    UInt             sHashValue = 0;
    scPageID         sTargetNPID;
    sdtWCB        ** sWCBPtrPtr;
    sdtWCB         * sWCBPtr = aWCBPtr;

    /* Fix되어있지 않아야 함 */
    IDE_ERROR( sWCBPtr->mFix == 0 );
    IDE_ERROR( sWCBPtr->mNPageID != SC_NULL_PID );

    /* CleanPage여야 함 */
    IDE_ERROR( sWCBPtr->mWPState == SDT_WA_PAGESTATE_CLEAN );

    /************************** Hash에서 때어내기 *********************/
    sTargetNPID  = sWCBPtr->mNPageID;
    /* 매달려 있어야 하며, 따라서 탐색할 수 있어야 함 */
    IDE_DASSERT( findWCB( aWASegment, sTargetNPID ) == sWCBPtr );
    sHashValue   = getNPageHashValue( aWASegment, sTargetNPID );

    sWCBPtrPtr   = &aWASegment->mNPageHashPtr[ sHashValue ];

    sWCBPtr      = *sWCBPtrPtr;
    while( sWCBPtr != NULL )
    {
        if ( sWCBPtr->mNPageID == sTargetNPID )
        {
            /* 찾았다, 자신의 자리에 다음 것을 매담 */
            (*sWCBPtrPtr) = sWCBPtr->mNextWCB4Hash;
            break;
        }

        sWCBPtrPtr = &sWCBPtr->mNextWCB4Hash;
        sWCBPtr    = *sWCBPtrPtr;
    }

    /* 없어졌어야 함 */
    IDE_DASSERT( findWCB( aWASegment, sTargetNPID ) == NULL );

    /***** Free가 예정되었으면, unassign으로 내려가는 시점에 Free해준다. ****/
    if ( sWCBPtr->mBookedFree == ID_TRUE )
    {
        IDE_TEST( pushFreePage( aWASegment,
                                sWCBPtr->mNPageID )
                  != IDE_SUCCESS );
    }
    else
    {
        /* nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    ideLog::log( IDE_DUMP_0,
                 "HASHVALUE : %"ID_UINT32_FMT"\n"
                 "WPID      : %"ID_UINT32_FMT"\n",
                 sHashValue,
                 getWPageID( aWASegment, aWCBPtr ) );

    return IDE_FAILURE;
}

/**************************************************************************
 * Description :
 * WPage를 다른 곳으로 옮긴다. npage, npagehash등을 고려해야 한다.

 * <IN>
 * aWASegment     - 대상 WASegment
 * aSrcWAGroupID  - 원본 Group ID
 * aSrcWPID       - 원본 WPID
 * aDstWAGroupID  - 대상 Group ID
 * aDstWPID       - 대상 WPID
 ***************************************************************************/
IDE_RC sdtSortSegment::moveWAPage( sdtSortSegHdr* aWASegment,
                                   sdtWCB       * aSrcWCBPtr,
                                   sdtWCB       * aDstWCBPtr )
{
    UChar        * sSrcWAPagePtr;
    scPageID       sNPageID;
    sdtWAPageState sWAPageState;

    /* Dest Page는 NPage가 할당되어 있지 않아야 한다. */
    IDE_ERROR( aDstWCBPtr->mWPState == SDT_WA_PAGESTATE_INIT );

    sNPageID = getNPID( aSrcWCBPtr );
 
    if ( sNPageID != SC_NULL_PID )
    {
        /* 원본 페이지가 만약 Flush중이 아니면 State도 함께 옮겨준다. */
        sWAPageState = aSrcWCBPtr->mWPState;
        aSrcWCBPtr->mWPState = SDT_WA_PAGESTATE_CLEAN;

        IDE_TEST( unassignNPage( aWASegment,
                                 aSrcWCBPtr )
                  != IDE_SUCCESS );

        initWCB( aSrcWCBPtr );

        IDE_TEST( assignNPage( aWASegment,
                               aDstWCBPtr,
                               sNPageID )
                  != IDE_SUCCESS );

        aDstWCBPtr->mWPState = sWAPageState;
    }

    /* PagePtr을 서로 교환한다. */
    // XXX Null 체크 필요
    sSrcWAPagePtr          = aSrcWCBPtr->mWAPagePtr;
    aSrcWCBPtr->mWAPagePtr = aDstWCBPtr->mWAPagePtr;
    aDstWCBPtr->mWAPagePtr = sSrcWAPagePtr;

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
 * aNPID          - 대상 NPage의 주소
 ***************************************************************************/
IDE_RC sdtSortSegment::readNPageFromDisk( sdtSortSegHdr* aWASegment,
                                          sdtGroupID     aWAGroupID,
                                          sdtWCB       * aWCBPtr,
                                          scPageID       aNPageID )
{
    UChar        * sWAPagePtr;

    IDE_DASSERT( aWCBPtr->mWAPagePtr != NULL );
    IDE_DASSERT( findWCB( aWASegment, aNPageID ) == NULL );

    /* 기존에 없던 페이지면 Read해서 올림 */
    IDE_TEST( assignNPage( aWASegment,
                           aWCBPtr,
                           aNPageID ) != IDE_SUCCESS );

    sWAPagePtr = getWAPagePtr( aWASegment,
                               aWAGroupID,
                               aWCBPtr );

    // TC/FIT/Server/sm/Bugs/BUG-45263/BUG-45263.tc
    IDU_FIT_POINT( "BUG-45263@sdtSortSegment::readNPageFromDisk::iduFileopen" );

    IDE_TEST( sddDiskMgr::read( aWASegment->mStatistics,
                                aWASegment->mSpaceID,
                                aNPageID,
                                1,
                                sWAPagePtr,
                                ID_FALSE ) /* aFatal */
              != IDE_SUCCESS );

    aWASegment->mStatsPtr->mReadCount++;

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
 * aNPID          - 대상 NPage의 주소
 ***************************************************************************/
IDE_RC sdtSortSegment::readNPage( sdtSortSegHdr* aWASegment,
                                  sdtGroupID     aWAGroupID,
                                  sdtWCB       * aWCBPtr,
                                  scPageID       aNPageID )
{
    sdtWCB * sOldWCB;

    sOldWCB = findWCB( aWASegment, aNPageID );
    if ( sOldWCB == NULL )
    {
        IDE_TEST( readNPageFromDisk( aWASegment,
                                     aWAGroupID,
                                     aWCBPtr,
                                     aNPageID )
                  != IDE_SUCCESS );
    }
    else
    {
        /* 기존에 있던 페이지니, movePage로 옮긴다. */
        if ( sOldWCB != aWCBPtr )
        {
            IDE_TEST( moveWAPage( aWASegment,
                                  sOldWCB,
                                  aWCBPtr )
                      != IDE_SUCCESS );
        }
    }

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
IDE_RC sdtSortSegment::pushFreePage(sdtSortSegHdr* aWASegment,
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
IDE_RC sdtSortSegment::allocAndAssignNPage(sdtSortSegHdr* aWASegment,
                                           sdtWCB       * aTargetWCBPtr )
{
    idBool         sIsEmpty;
    scPageID       sFreeNPID;

    IDE_TEST( aWASegment->mFreeNPageStack.pop( ID_FALSE, /* lock */
                                               (void*) &sFreeNPID,
                                               &sIsEmpty )
              != IDE_SUCCESS );

    if ( sIsEmpty == ID_TRUE )
    {
        if ( aWASegment->mNExtFstPIDList.mLastFreeExtFstPID == SM_NULL_PID )
        {
            /* 최초 Alloc 시도 */
            IDE_TEST( sdtWAExtentMgr::allocFreeNExtent( aWASegment->mStatistics,
                                                        aWASegment->mStatsPtr,
                                                        aWASegment->mSpaceID,
                                                        &aWASegment->mNExtFstPIDList )
                      != IDE_SUCCESS );
        }
        else
        {
            if ( aWASegment->mNExtFstPIDList.mPageSeqInLFE == SDT_WAEXTENT_PAGECOUNT )
            {
                /* 마지막 NExtent를 다썼음 */
                IDE_TEST( sdtWAExtentMgr::allocFreeNExtent( aWASegment->mStatistics,
                                                            aWASegment->mStatsPtr,
                                                            aWASegment->mSpaceID,
                                                            &aWASegment->mNExtFstPIDList )
                          != IDE_SUCCESS );
            }
            else
            {
                /* 기존에 할당해둔 Extent에서 가져옴 */
            }
        }

        sFreeNPID = aWASegment->mNExtFstPIDList.mLastFreeExtFstPID
            + aWASegment->mNExtFstPIDList.mPageSeqInLFE;
        aWASegment->mNExtFstPIDList.mPageSeqInLFE++;
        aWASegment->mNPageCount++;
    }
    else
    {
        /* Stack을 통해 재활용했음 */
    }

    IDE_TEST( assignNPage( aWASegment,
                           aTargetWCBPtr,
                           sFreeNPID )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**************************************************************************
 * Description :
 * 이 WASegment가 사용한 모든 WASegment를 Free한다.
 *
 * <IN>
 * aWASegment     - 대상 WASegment
 ***************************************************************************/
void sdtSortSegment::clearAllWCB( sdtSortSegHdr* aWASegment )
{
    sdtWCB   * sNxtWCBPtr;
    sdtWCB   * sCurWCBPtr;
    UInt       sHashValue;
#ifdef DEBUG
    UInt       i;
#endif

    if (( smuProperty::getWCBCleanMemset() == ID_FALSE ) &&
        ( aWASegment->mIsInMemory == SDT_WORKAREA_IN_MEMORY ))
    {
        IDE_ASSERT( aWASegment->mNPageHashPtr != NULL );

#ifdef DEBUG
        sCurWCBPtr = &aWASegment->mWCBMap[0];
        for( i = 0 ;
             i< getMaxWAPageCount( aWASegment ) ;
             i++ , sCurWCBPtr++ )
        {
            IDE_ASSERT( sCurWCBPtr->mFix >= 0 );
        }
#endif
        sCurWCBPtr = aWASegment->mUsedWCBPtr;

        while( sCurWCBPtr != SDT_USED_PAGE_PTR_TERMINATED )
        {
            sNxtWCBPtr = sCurWCBPtr->mNxtUsedWCBPtr;

            if ( sCurWCBPtr->mNPageID != SD_NULL_PID )
            {
                sHashValue = getNPageHashValue( aWASegment, sCurWCBPtr->mNPageID );

                aWASegment->mNPageHashPtr[ sHashValue ] = NULL;
            }
#ifdef DEBUG
            if ( aWASegment->mHintWCBPtr == sCurWCBPtr )
            {
                aWASegment->mHintWCBPtr = NULL;
            }
#endif
            initWCB(sCurWCBPtr);

            sCurWCBPtr->mNxtUsedWCBPtr = NULL;
            sCurWCBPtr->mWAPagePtr     = NULL;

            sCurWCBPtr = sNxtWCBPtr;
        }

        aWASegment->mUsedWCBPtr = SDT_USED_PAGE_PTR_TERMINATED;
        IDE_DASSERT( aWASegment->mHintWCBPtr == NULL );
#ifdef DEBUG
        sCurWCBPtr = aWASegment->mWCBMap;
        for( i = 0 ;
             i< getMaxWAPageCount( aWASegment ) ;
             i++ , sCurWCBPtr++ )
        {
            IDE_ASSERT( sCurWCBPtr->mWPState == SDT_WA_PAGESTATE_INIT );
            IDE_ASSERT( sCurWCBPtr->mFix     == 0 );
            IDE_ASSERT( sCurWCBPtr->mBookedFree == ID_FALSE );
            IDE_ASSERT( sCurWCBPtr->mNPageID    == SM_NULL_PID );
            IDE_ASSERT( sCurWCBPtr->mNextWCB4Hash  == NULL );
            IDE_ASSERT( sCurWCBPtr->mNxtUsedWCBPtr == NULL );
            IDE_ASSERT( sCurWCBPtr->mWAPagePtr     == NULL );
        }
#endif
    }
    else
    {
        idlOS::memset( &aWASegment->mWCBMap,
                       0,
                       getMaxWAPageCount( aWASegment ) * ID_SIZEOF(sdtWCB) );
        idlOS::memset( aWASegment->mNPageHashPtr,
                       0,
                       aWASegment->mNPageHashBucketCnt * ID_SIZEOF(sdtWCB*) );

        aWASegment->mUsedWCBPtr = SDT_USED_PAGE_PTR_TERMINATED;
    }
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
void sdtSortSegment::moveLRUListToTopInternal( sdtSortSegHdr* aWASegment,
                                               sdtSortGroup * aGrpInfo,
                                               sdtWCB       * aWCBPtr )
{
    scPageID         sCurWPID = getWPageID( aWASegment, aWCBPtr );
    scPageID         sPrevPID;
    scPageID         sNextPID;
    sdtWCB         * sPrevWCBPtr;
    sdtWCB         * sNextWCBPtr;
    sdtWCB         * sOldTopWCBPtr;

    IDE_DASSERT( validateLRUList( aWASegment, aGrpInfo )
                 == IDE_SUCCESS );

    sOldTopWCBPtr = getWCBInternal( aWASegment, aGrpInfo->mReuseWPIDTop );

    IDE_DASSERT( sOldTopWCBPtr != NULL );

    sPrevPID = aWCBPtr->mLRUPrevPID;
    sNextPID = aWCBPtr->mLRUNextPID;

    /* PrevPage로부터 현재 Page에 대한 링크를 때어냄 */

    if ( sNextPID == SD_NULL_PID )
    {
        /* 자신이 마지막 Page였을 경우 */
        IDE_ASSERT( aGrpInfo->mReuseWPIDBot == sCurWPID );
        aGrpInfo->mReuseWPIDBot = sPrevPID;
    }
    else
    {
        sNextWCBPtr = getWCBInternal( aWASegment, sNextPID );
        sNextWCBPtr->mLRUPrevPID = sPrevPID;
    }
    sPrevWCBPtr = getWCBInternal( aWASegment, sPrevPID );
    sPrevWCBPtr->mLRUNextPID = sNextPID;

    /* 선두에 매담 */
    aWCBPtr->mLRUPrevPID       = SD_NULL_PID;
    aWCBPtr->mLRUNextPID       = aGrpInfo->mReuseWPIDTop;
    sOldTopWCBPtr->mLRUPrevPID = sCurWPID;
    aGrpInfo->mReuseWPIDTop    = sCurWPID;

    IDE_DASSERT( validateLRUList( aWASegment,
                                  aGrpInfo ) == IDE_SUCCESS );
    return;
}


#ifdef DEBUG
/**************************************************************************
 * Description :
 * LRU리스트를 점검한다. Debugging 모드 용
 *
 * <IN>
 * aWASegment     - 대상 WASegment
 * aWAGroupID     - 대상 Group ID
 ***************************************************************************/
IDE_RC sdtSortSegment::validateLRUList( sdtSortSegHdr* aWASegment,
                                        sdtSortGroup * aGrpInfo )
{
    sdtWCB         * sWCBPtr;
    UInt             sPageCount = 0;
    scPageID         sPrevWPID;
    scPageID         sNextWPID;

    if ( aGrpInfo->mPolicy == SDT_WA_REUSE_LRU )
    {
        sPrevWPID = SC_NULL_PID;
        sNextWPID = aGrpInfo->mReuseWPIDTop;
        while( sNextWPID != SC_NULL_PID )
        {
            sWCBPtr = getWCBInternal( aWASegment, sNextWPID );

            IDE_ASSERT( sWCBPtr != NULL );
            IDE_ASSERT( sWCBPtr->mLRUPrevPID == sPrevWPID );

            sPrevWPID = sNextWPID;
            sNextWPID = sWCBPtr->mLRUNextPID;

            sPageCount++;
        }
        IDE_ASSERT( sPrevWPID == aGrpInfo->mReuseWPIDBot );
        IDE_ASSERT( sNextWPID == SC_NULL_PID );
    }
    return IDE_SUCCESS;
}
#endif

/**************************************************************************
 * Description :
 * PID를 바탕으로 그 PID를 소유한 Group을 찾는다.
 *
 * <IN>
 * aWASegment     - 대상 WASegment
 * aPID           - 대상 PID
 ***************************************************************************/
sdtGroupID   sdtSortSegment::findGroup( sdtSortSegHdr* aWASegment,
                                        scPageID       aWPageID )
{
    UInt     i;

    for( i = 0 ; i < SDT_WAGROUPID_MAX ; i++ )
    {
        if ( ( aWASegment->mGroup[ i ].mPolicy != SDT_WA_REUSE_NONE ) &&
             ( aWASegment->mGroup[ i ].mBeginWPID <= aWPageID ) &&
             ( aWASegment->mGroup[ i ].mEndWPID   >  aWPageID ) )
        {
            return i;
        }
    }

    /* 찾지 못함 */
    return SDT_WAGROUPID_MAX;
}

/**************************************************************************
 * Description : 해당 page가 속한 그룹의 Page 재활용 정책을 확인한다.
 *
 * aWASegment     - [IN] 대상 WASegment
 * aWAPageID      - [IN] 대상 WPageID
 ***************************************************************************/
sdtWAReusePolicy sdtSortSegment::getReusePolicy( sdtSortSegHdr* aWASegment,
                                                 scPageID       aWAPageID )
{
    sdtGroupID       sWAGroupID;
    sdtSortGroup   * sWAGroupInfo;
    sdtWAReusePolicy sReusePolicy = SDT_WA_REUSE_NONE;

    sWAGroupID = sdtSortSegment::findGroup( aWASegment,
                                            aWAPageID );
    if ( sWAGroupID != SDT_WAGROUPID_MAX )
    {
        sWAGroupInfo = getWAGroupInfo( aWASegment,
                                       sWAGroupID );

        sReusePolicy = sWAGroupInfo->mPolicy;
    }
    return sReusePolicy;
}

/**************************************************************************
 * Description : Total WA에서 WAExtent를 할당받아 온다.
 *
 * aWASegment     - [IN] 대상 WASegment
 ***************************************************************************/
IDE_RC sdtSortSegment::expandFreeWAExtent( sdtSortSegHdr* aWASeg )
{
    SInt        sExtentCount;

    sExtentCount = smuProperty::getTmpAllocWAExtCnt();

    if ( aWASeg->mMaxWAExtentCount < ( sExtentCount + aWASeg->mWAExtentInfo.mCount ) )
    {
        sExtentCount = aWASeg->mMaxWAExtentCount - aWASeg->mWAExtentInfo.mCount;
    }
    IDE_ERROR( sExtentCount > 0 );

    if ( sdtWAExtentMgr::allocWAExtents( &aWASeg->mWAExtentInfo,
                                         sExtentCount )
         != IDE_SUCCESS )
    {
        // BUGBUG 무조건 할당받도록 되어 있다. 고쳐야함.
        IDE_TEST( ideGetErrorCode() != smERR_ABORT_NOT_ENOUGH_WORKAREA );

        IDE_TEST( sdtWAExtentMgr::memAllocWAExtent( &aWASeg->mWAExtentInfo )
                  != IDE_SUCCESS );
        aWASeg->mStatsPtr->mOverAllocCount++;
    }
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**************************************************************************
 * Description : 한개의 WAPage를 WCB에 할당한다
 *
 * aWASegment  - [IN] 대상 WASegment
 * aWCBPtr     - [IN] 할당할 첫번째 WCB의 Pointer  
 ***************************************************************************/
IDE_RC sdtSortSegment::setWAPagePtr( sdtSortSegHdr * aWASegment,
                                     sdtWCB        * aWCBPtr  )
{
    IDE_ASSERT( aWCBPtr->mWAPagePtr == NULL );
    IDE_DASSERT( aWCBPtr->mNxtUsedWCBPtr != NULL );

    if ( aWASegment->mCurrFreePageIdx >= SDT_WAEXTENT_PAGECOUNT )
    {
        if ( aWASegment->mCurFreeWAExtent->mNextExtent == NULL )
        {
            IDE_ERROR( aWASegment->mMaxWAExtentCount > aWASegment->mWAExtentInfo.mCount );

            IDE_TEST( expandFreeWAExtent( aWASegment ) != IDE_SUCCESS );
            IDE_DASSERT( aWASegment->mCurFreeWAExtent->mNextExtent != NULL );
        }

        aWASegment->mCurrFreePageIdx = 0;
        aWASegment->mCurFreeWAExtent = aWASegment->mCurFreeWAExtent->mNextExtent;
    }
    aWCBPtr->mWAPagePtr = aWASegment->mCurFreeWAExtent->mPage[aWASegment->mCurrFreePageIdx];
    aWASegment->mCurrFreePageIdx++;
    aWASegment->mAllocWAPageCount++;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**************************************************************************
 * Description :
 * Map이 차지하는 위치를 뺀, 사용 가능한 Group의 크기를 Page개수로 받는다.
 *
 * <IN>
 * aWASegment     - 대상 WASegment
 * aWAGroupID     - 대상 Group ID
 ***************************************************************************/
UInt sdtSortSegment::getAllocableWAGroupPageCount( sdtSortGroup * aGrpInfo )
{
    return aGrpInfo->mEndWPID - aGrpInfo->mBeginWPID
        - sdtWASortMap::getWPageCount( aGrpInfo->mSortMapHdr );
}

/**************************************************************************
 * Description :
 * WASegemnt 구성을 File로 Dump한다.
 *
 * <IN>
 * aWASegment     - 대상 WASegment
 ***************************************************************************/
void sdtSortSegment::exportSortSegmentToFile( sdtSortSegHdr* aWASegment )
{
    SChar       sFileName[ SM_MAX_FILE_NAME ];
    SChar       sDateString[ SMI_TT_STR_SIZE ];
    iduFile     sFile;
    UInt        sState = 0;
    ULong       sOffset = 0;
    ULong       sSize;
    ULong       sMaxPageCount;
    scPageID    sWPageID;
    sdtWCB    * sWCBPtr;
    sdtWCB    * sEndWCBPtr;
    sdtNExtentArr * sNExtentArr;

    IDE_ASSERT( aWASegment != NULL );

    IDE_TEST( sFile.initialize( IDU_MEM_SM_TEMP,
                                1, // Max Open FD Count
                                IDU_FIO_STAT_OFF,
                                IDV_WAIT_INDEX_NULL )
              != IDE_SUCCESS);
    sState = 1;

    smuUtility::getTimeString( smiGetCurrTime(),
                               SMI_TT_STR_SIZE,
                               sDateString );

    idlOS::snprintf( sFileName,
                     SM_MAX_FILE_NAME,
                     "%s%c%s_sort.td",
                     smuProperty::getTempDumpDirectory(),
                     IDL_FILE_SEPARATOR,
                     sDateString );

    IDE_TEST( sFile.setFileName( sFileName ) != IDE_SUCCESS);
    IDE_TEST( sFile.createUntilSuccess( smiSetEmergency )
              != IDE_SUCCESS);
    IDE_TEST( sFile.open( ID_FALSE ) != IDE_SUCCESS ); //DIRECT_IO
    sState = 2;

    // sdtSortSegHdr포인터 주소
    IDE_TEST( sFile.write( aWASegment->mStatistics,
                           0,
                           (UChar*)&aWASegment,
                           ID_SIZEOF(sdtSortSegHdr*) )
              != IDE_SUCCESS );
    sOffset += ID_SIZEOF(sdtSortSegHdr*);

    // sdtSortSegHdr+ WCB Map
    sMaxPageCount = getMaxWAPageCount( aWASegment );
    sSize = calcWASegmentSize( sMaxPageCount );

    IDE_TEST( sFile.write( aWASegment->mStatistics,
                           sOffset,
                           (UChar*)aWASegment,
                           sSize )
              != IDE_SUCCESS );
    sOffset += sSize;

    // Normal page Hash Map
    sSize = aWASegment->mNPageHashBucketCnt * ID_SIZEOF(sdtWCB*);

    IDE_TEST( sFile.write( aWASegment->mStatistics,
                           sOffset,
                           (UChar*)aWASegment->mNPageHashPtr,
                           sSize )
              != IDE_SUCCESS );
    sOffset += sSize;

    // Normal Extent Map
    for ( sNExtentArr =  aWASegment->mNExtFstPIDList.mHead;
          sNExtentArr != NULL;
          sNExtentArr = sNExtentArr->mNextArr )
    {
        IDE_TEST( sFile.write( aWASegment->mStatistics,
                               sOffset,
                               (UChar*)sNExtentArr,
                               ID_SIZEOF( sdtNExtentArr ) )
                  != IDE_SUCCESS );
        sOffset += ID_SIZEOF( sdtNExtentArr );
    }

    // WA Extents

    sWCBPtr    = aWASegment->mWCBMap;
    sEndWCBPtr = sWCBPtr + sMaxPageCount;
    sWPageID   = 0;

    while( sWCBPtr < sEndWCBPtr )
    {
        if ( sWCBPtr->mWAPagePtr != NULL )
        {
            IDE_TEST( sFile.write( aWASegment->mStatistics,
                                   sOffset,
                                   (UChar*)&sWPageID,
                                   ID_SIZEOF(scPageID) )
                      != IDE_SUCCESS );
            sOffset += ID_SIZEOF(ULong);

            IDE_TEST( sFile.write( aWASegment->mStatistics,
                                   sOffset,
                                   (UChar*)sWCBPtr->mWAPagePtr,
                                   SD_PAGE_SIZE )
                      != IDE_SUCCESS );
            sOffset += SD_PAGE_SIZE;
        }

        sWCBPtr++;
        sWPageID++;
    }

    sWPageID = SM_SPECIAL_PID;
    IDE_TEST( sFile.write( aWASegment->mStatistics,
                           sOffset,
                           (UChar*)&sWPageID,
                           ID_SIZEOF(scPageID) )
              != IDE_SUCCESS );

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
 * aSortSegHdr    - 읽어드린 WASegment의 위치
 * aRawSortSegHdr - WA관련 버퍼의 원본 위치
 * aPtr           - 위 버퍼를 Align에 따라 재조정한 위치
 ***************************************************************************/
IDE_RC sdtSortSegment::importSortSegmentFromFile( SChar         * aFileName,
                                                  sdtSortSegHdr** aSortSegHdr,
                                                  sdtSortSegHdr * aRawSortSegHdr,
                                                  UChar        ** aPtr )
{
    iduFile         sFile;
    UInt            sState          = 0;
    UInt            sMemAllocState  = 0;
    ULong           sSize = 0;
    sdtSortSegHdr * sWASegment = NULL;
    UChar         * sPtr;
    ULong           sMaxPageCount;
    UChar         * sOldWASegPtr;
    sdtNExtentArr * sNExtentArr;
    UInt            i;

    IDE_ERROR( sFile.initialize(IDU_MEM_SM_SMU,
                                1, // Max Open FD Count
                                IDU_FIO_STAT_OFF,
                                IDV_WAIT_INDEX_NULL )
               == IDE_SUCCESS );
    sState = 1;

    IDE_ERROR( sFile.setFileName( aFileName ) == IDE_SUCCESS );
    IDE_ERROR( sFile.open() == IDE_SUCCESS );
    sState = 2;

    IDE_ERROR( sFile.getFileSize( &sSize ) == IDE_SUCCESS );

    IDE_ERROR( iduMemMgr::malloc( IDU_MEM_SM_TEMP,
                                  sSize,
                                  (void**)&sPtr )
               == IDE_SUCCESS );
    sMemAllocState = 1;

    IDE_ERROR( sFile.read( NULL,  // aStatistics
                           0,     // aWhere
                           sPtr,
                           sSize,
                           NULL ) // aEmergencyFunc
               == IDE_SUCCESS );

    // Pointer 값들은 기존 메모리 위치로 잘못되어 있으니, 재조정한다.
    *aPtr        = sPtr;
    // sdtSortSegHdr+ WCB Map
    sOldWASegPtr = *(UChar**)sPtr;

    sPtr += ID_SIZEOF(sdtSortSegHdr*);

    sWASegment = (sdtSortSegHdr*)sPtr;

    // 값을 있는 그대로 출력 할 필요도 있다.
    idlOS::memcpy((UChar*)aRawSortSegHdr,
                  (UChar*)sWASegment,
                  SDT_SORT_SEGMENT_HEADER_SIZE);

    sMaxPageCount = getMaxWAPageCount( sWASegment ) ;
    sPtr += calcWASegmentSize( sMaxPageCount );

    IDE_ERROR( (ULong)sPtr % ID_SIZEOF(ULong) == 0);

    // Normal page Hash Map
    sWASegment->mNPageHashPtr = (sdtWCB**)sPtr;
    sPtr += sWASegment->mNPageHashBucketCnt * ID_SIZEOF(sdtWCB*);

    // Normal Extent Map
    if ( sWASegment->mNExtFstPIDList.mHead != NULL )
    {
        sWASegment->mNExtFstPIDList.mHead = (sdtNExtentArr*)sPtr;
        sPtr += ID_SIZEOF( sdtNExtentArr );

        for ( sNExtentArr =  sWASegment->mNExtFstPIDList.mHead;
              sNExtentArr->mNextArr != NULL;
              sNExtentArr = sNExtentArr->mNextArr )
        {
            sNExtentArr->mNextArr = (sdtNExtentArr*)sPtr;
            sPtr += ID_SIZEOF( sdtNExtentArr );
        }
        sWASegment->mNExtFstPIDList.mTail = sNExtentArr;
    }

    IDE_TEST( resetWCBInfo4Dump( sWASegment,
                                 sOldWASegPtr,
                                 sMaxPageCount,
                                 &sPtr )
              != IDE_SUCCESS );

    for( i = 0 ; i < SDT_WAGROUPID_MAX ; i++ )
    {
        if ( sWASegment->mGroup[ i ].mHintWCBPtr != NULL )
        {
            sWASegment->mGroup[ i ].mHintWCBPtr =
                (sdtWCB*)(((UChar*)sWASegment->mGroup[ i ].mHintWCBPtr)
                          - sOldWASegPtr + (UChar*)sWASegment) ;
        }
    }

    if ( sWASegment->mUsedWCBPtr != NULL )
    {
        sWASegment->mUsedWCBPtr =
            (sdtWCB*)(((UChar*)sWASegment->mUsedWCBPtr)
                      - sOldWASegPtr + (UChar*)sWASegment) ;
    }

// Map의 Segment위치 및 Group과의 Pointer 재조정
    IDE_ERROR( sdtWASortMap::resetPtrAddr( sWASegment )
               == IDE_SUCCESS );

    if ( sWASegment->mSortMapHdr.mWASegment != NULL )
    {
        sWASegment->mSortMapHdr.mWASegment = sWASegment ;
    }

    sState = 1;
    IDE_ERROR( sFile.close() == IDE_SUCCESS );
    sState = 0;
    IDE_ERROR( sFile.destroy() == IDE_SUCCESS );

    *aSortSegHdr  = sWASegment;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sMemAllocState == 1  )
    {
        IDE_ASSERT( iduMemMgr::free( sPtr ) == IDE_SUCCESS );
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

    return IDE_FAILURE;
}


/***************************************************************************
 * Description : WAMap의 WCB의 Pointer를 Load한 Memory로 재구축한다.
 *
 * aWASegment     - [IN] 재구축 중인 Sort Segment
 * aOldWASegPtr   - [IN] Dump 하기 전의 Sort Segment의 Pointer
 * aMaxPageCount  - [IN] Page의 Count
 * aPtr           - [IN] 읽어들인 Buffer
 ***************************************************************************/
IDE_RC sdtSortSegment::resetWCBInfo4Dump( sdtSortSegHdr * aWASegment,
                                          UChar         * aOldWASegPtr,
                                          UInt            aMaxPageCount,
                                          UChar        ** aPtr )
{
    sdtWCB        * sWCBPtr;
    sdtWCB        * sEndWCBPtr;
    scPageID        sWPageID;
    scPageID        sNxtPageID;
    UChar         * sPtr = *aPtr;

    sWCBPtr    = aWASegment->mWCBMap;
    sEndWCBPtr = sWCBPtr + aMaxPageCount;
    sWPageID   = 0;
    sNxtPageID = *(scPageID*)sPtr;
    sPtr += ID_SIZEOF(ULong);

    while( sWCBPtr < sEndWCBPtr )
    {
        if ( sWCBPtr->mNextWCB4Hash != NULL )
        {
            sWCBPtr->mNextWCB4Hash  = (sdtWCB*)(((UChar*)sWCBPtr->mNextWCB4Hash)
                                                - aOldWASegPtr + (UChar*)aWASegment) ;
        }
        if ( sWCBPtr->mNxtUsedWCBPtr > SDT_USED_PAGE_PTR_TERMINATED )
        {
            sWCBPtr->mNxtUsedWCBPtr = (sdtWCB*)(((UChar*)sWCBPtr->mNxtUsedWCBPtr)
                                                - aOldWASegPtr + (UChar*)aWASegment) ;
        }

        // 6. wa page ( wapageID, page )
        if ( sWPageID == sNxtPageID )
        {
            sWCBPtr->mWAPagePtr = sPtr;
            sPtr += SD_PAGE_SIZE;

            sNxtPageID = *(scPageID*)sPtr;
            sPtr += ID_SIZEOF(ULong);

            IDE_ERROR(( sWPageID < sNxtPageID ) ||
                      ( sNxtPageID == SM_NULL_PID ));
        }
        else
        {
            sWCBPtr->mWAPagePtr = NULL;
        }
        sWCBPtr++;
        sWPageID++;
    }

    *aPtr = sPtr;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

void sdtSortSegment::dumpWASegment( void           * aWASegment,
                                    SChar          * aOutBuf,
                                    UInt             aOutSize )
{
    sdtSortSegHdr  * sWASegment = (sdtSortSegHdr*)aWASegment;
    SInt             i;

    IDE_ERROR( sWASegment != NULL );

    (void)idlVA::appendFormat(
                aOutBuf,
                aOutSize,
                "DUMP WASEGMENT:\n"
                "WASegPtr         : 0x%"ID_xINT64_FMT"\n"
                "SpaceID          : %"ID_UINT32_FMT"\n"
                "Type             : SORT\n"
                "NExtentCount     : %"ID_UINT32_FMT"\n"
                "WAExtentCount    : %"ID_UINT32_FMT"\n"
                "MaxWAExtentCount : %"ID_UINT32_FMT"\n"
                "PageSeqInLFE     : %"ID_UINT32_FMT"\n"

                "HintWCBPtr       : 0x%"ID_xPOINTER_FMT"\n"
                "WAExtentListHead : 0x%"ID_xPOINTER_FMT"\n"
                "WAExtentListTail : 0x%"ID_xPOINTER_FMT"\n"
                "CurFreeWAExtent  : 0x%"ID_xPOINTER_FMT"\n"
                "mCurrFreePageIdx : %"ID_UINT32_FMT"\n"

                "AllocPageCount   : %"ID_UINT32_FMT"\n"
                "NPageCount       : %"ID_UINT32_FMT"\n"
                "LastFreeExtFstPID: %"ID_UINT32_FMT"\n"
                "InMemory         : %s\n"
                "NPageHashBucketCnt: %"ID_UINT32_FMT"\n"
                "UsedWCBPtr       : 0x%"ID_xPOINTER_FMT"\n",
                sWASegment,
                sWASegment->mSpaceID,
                getNExtentCount( sWASegment ),
                sWASegment->mWAExtentInfo.mCount,
                sWASegment->mMaxWAExtentCount,
                sWASegment->mNExtFstPIDList.mPageSeqInLFE,

                sWASegment->mHintWCBPtr,
                sWASegment->mWAExtentInfo.mHead,
                sWASegment->mWAExtentInfo.mTail,
                sWASegment->mCurFreeWAExtent,
                sWASegment->mCurrFreePageIdx,

                sWASegment->mAllocWAPageCount,
                sWASegment->mNPageCount,
                sWASegment->mNExtFstPIDList.mLastFreeExtFstPID,
                sWASegment->mIsInMemory == SDT_WORKAREA_OUT_MEMORY ? "OutMemory" : "InMemory",
                sWASegment->mNPageHashBucketCnt,
                sWASegment->mUsedWCBPtr );

    (void)idlVA::appendFormat( aOutBuf, aOutSize, "\n" );

    for( i = 0 ; i < SDT_WAGROUPID_MAX ; i++ )
    {
        dumpWAGroup( sWASegment, i, aOutBuf, aOutSize );
    }

    return;

    IDE_EXCEPTION_END;

    return;
}

void sdtSortSegment::dumpWAGroup( sdtSortSegHdr  * aWASegment,
                                  sdtGroupID       aWAGroupID,
                                  SChar          * aOutBuf,
                                  UInt             aOutSize )
{
    sdtSortGroup   * sGrpInfo = getWAGroupInfo( aWASegment, aWAGroupID );
    scPageID         sHintPageID;

    if ( sGrpInfo->mPolicy != SDT_WA_REUSE_NONE )
    {
        sHintPageID = (sGrpInfo->mHintWCBPtr == NULL ) ? 0 : getWPageID( aWASegment,
                                                                         sGrpInfo->mHintWCBPtr );
        (void)idlVA::appendFormat(
            aOutBuf,
            aOutSize,
            "DUMP WAGROUP:\n"
            "ID     : %"ID_UINT32_FMT"\n"
            "Policy : %-4"ID_UINT32_FMT
            "(0:None, 1:InMemory, 2:FIFO, 3:LRU, 4:HASH)\n"
            "Range  : %"ID_UINT32_FMT" <-> %"ID_UINT32_FMT"\n"
            "ReuseP : Seq: %"ID_UINT32_FMT", "
            "Top: %"ID_UINT32_FMT", Bot: %"ID_UINT32_FMT"\n"
            "Hint   : %"ID_UINT32_FMT"\n"
            "HintPtr: 0x%"ID_xPOINTER_FMT"\n"
            "MapPtr : 0x%"ID_xINT64_FMT"\n\n",
            aWAGroupID,
            sGrpInfo->mPolicy,
            sGrpInfo->mBeginWPID,
            sGrpInfo->mEndWPID,
            sGrpInfo->mReuseWPIDSeq,
            sGrpInfo->mReuseWPIDTop,
            sGrpInfo->mReuseWPIDBot,
            sHintPageID,
            sGrpInfo->mHintWCBPtr,
            sGrpInfo->mSortMapHdr );
    }

    return;
}

/**************************************************************************
 * Description : Sort Segment의 모든 WCB의 정보를 출력함
 *
 * aWASegment - [IN] WCB를 Dump 할 Sort Segment
 * aOutBuf    - [IN] Dump한 내용을 담아둘 출력 Buffer
 * aOutSize   - [IN] 출력 Buffer의 Size
 ***************************************************************************/
void sdtSortSegment::dumpWCBs( void           * aWASegment,
                               SChar          * aOutBuf,
                               UInt             aOutSize )
{
    sdtWCB         * sWCBPtr;
    sdtWCB         * sEndWCBPtr;
    scPageID         sWPageID;

    (void)idlVA::appendFormat( aOutBuf,
                               aOutSize,
                               "WCB:\n%5s %5s %4s %8s %8s %8s %-18s %-18s %-18s %-18s\n",
                               "WPID",
                               "STATE",
                               "FIX",
                               "NPAGE_ID",
                               "LRU_PREV",
                               "LRU_NEXT",
                               "HASH_NEXT",
                               "USED_NEXT",
                               "WA_PAGE_PTR" );

    sWCBPtr    = ((sdtSortSegHdr*)aWASegment)->mWCBMap;
    sEndWCBPtr = sWCBPtr + sdtSortSegment::getMaxWAPageCount( (sdtSortSegHdr*)aWASegment );
 
    sWPageID   = 0;

    while( sWCBPtr < sEndWCBPtr )
    {
        (void)idlVA::appendFormat( aOutBuf,
                                   aOutSize,
                                   "%5"ID_UINT32_FMT"",
                                   sWPageID );
        dumpWCB( (void*)sWCBPtr,aOutBuf,aOutSize );
        sWPageID++;
        sWCBPtr++;
    }

    (void)idlVA::appendFormat( aOutBuf, aOutSize, "\n" );

    return;
}

/**************************************************************************
 * Description : 1개의 WCB의 정보를 출력함
 *
 * aWCBPtr    - [IN] Dump 할 WCB의 Pointer
 * aOutBuf    - [IN] Dump한 내용을 담아둘 출력 Buffer
 * aOutSize   - [IN] 출력 Buffer의 Size
 ***************************************************************************/
void sdtSortSegment::dumpWCB( void           * aWCB,
                              SChar          * aOutBuf,
                              UInt             aOutSize )
{
    sdtWCB   * sWCBPtr;

    sWCBPtr = (sdtWCB*) aWCB;

    (void)idlVA::appendFormat( aOutBuf,
                               aOutSize,
                               " %5"ID_UINT32_FMT
                               " %4"ID_UINT32_FMT
                               " %8"ID_UINT32_FMT
                               " %8"ID_UINT32_FMT
                               " %8"ID_UINT32_FMT
                               " %s%-16"ID_xPOINTER_FMT
                               " %s%-16"ID_xPOINTER_FMT
                               " %s%-16"ID_xPOINTER_FMT"\n",
                               sWCBPtr->mWPState,
                               sWCBPtr->mFix,
                               sWCBPtr->mNPageID,
                               sWCBPtr->mLRUPrevPID,
                               sWCBPtr->mLRUNextPID,
                               (sWCBPtr->mNextWCB4Hash != NULL ) ?  "0x" : "  ",
                               sWCBPtr->mNextWCB4Hash,
                               (sWCBPtr->mNxtUsedWCBPtr > (sdtWCB*)1 ) ? "0x" : "  ",
                               sWCBPtr->mNxtUsedWCBPtr,
                               (sWCBPtr->mWAPagePtr != NULL ) ? "0x" : "  ",
                               sWCBPtr->mWAPagePtr );
}
