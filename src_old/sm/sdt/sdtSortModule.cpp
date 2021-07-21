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

#include <sdtDef.h>
#include <smiMisc.h>
#include <sdtSortDef.h>
#include <sdtTempRow.h>
#include <sdtSortModule.h>
#include <sdtSortSegment.h>


/**************************************************************************
 * Description :
 * WorkArea를 할당받고 temptableHeader등을 초기화 해준다.
 * 직후 insertNSort, insert 둘 중 하나의 상태가
 * 된다.
 *
 * <IN>
 * aHeader        - 대상 Table
 ***************************************************************************/
IDE_RC sdtSortModule::init( smiTempTableHeader * aHeader )
{
    sdtSortSegHdr      * sWASeg;
    UInt                 sInitGroupPageCount;
    UInt                 sSortGroupPageCount;
    UInt                 sFlushGroupPageCount;
    smiColumnList      * sKeyColumn;

    IDE_ERROR( aHeader != NULL );

    sWASeg  = (sdtSortSegHdr*)aHeader->mWASegment;
    sKeyColumn = (smiColumnList*) aHeader->mKeyColumnList;

    /* SortTemp는 unique 체크 안함 */
    IDE_ERROR( SM_IS_FLAG_OFF( aHeader->mTTFlag, SMI_TTFLAG_UNIQUE )
               == ID_TRUE );

    /************************** Group설정 *******************************/
    sInitGroupPageCount = sdtSortSegment::getAllocableWAGroupPageCount(
        sWASeg, SDT_WAGROUPID_INIT );

    /* BUG-46427 각 group당 최소 2개 이상의 page가 있어야 한다.
     * Flush group은 이 후 sub flush group이 분리 될 수도 있다.
     * 즉 3개 group이 될 수도 있다. */
    IDE_ERROR( sInitGroupPageCount >= ( SDT_WAGROUP_MIN_PAGECOUNT * 3 ) );

    sSortGroupPageCount = sInitGroupPageCount * aHeader->mWorkGroupRatio / 100;

    if ( sSortGroupPageCount < SDT_WAGROUP_MIN_PAGECOUNT )
    {
        /* BUG-46427 각 group당 2개 이상의 page가 있어야 한다. */
        sSortGroupPageCount = SDT_WAGROUP_MIN_PAGECOUNT;
    }

    sFlushGroupPageCount= ( sInitGroupPageCount - sSortGroupPageCount);

    if ( sFlushGroupPageCount < ( SDT_WAGROUP_MIN_PAGECOUNT * 2 ) )
    {
        /* BUG-46427 각 group당 2개 이상의 page가 있어야 한다.
         * Flush Group은 이 후 sub flush로 둘로 나누어 질 수 있다.
         * 미리 4개 이상의 page를 할당해 놓는다. */
        sFlushGroupPageCount = SDT_WAGROUP_MIN_PAGECOUNT * 2 ;
        sSortGroupPageCount = sInitGroupPageCount - sFlushGroupPageCount;
    }
    IDE_TEST( sdtSortSegment::createWAGroup( sWASeg,
                                             SDT_WAGROUPID_SORT,
                                             sSortGroupPageCount,
                                             SDT_WA_REUSE_INMEMORY )
              != IDE_SUCCESS );

    IDE_TEST( sdtSortSegment::createWAGroup( sWASeg,
                                             SDT_WAGROUPID_FLUSH,
                                             sFlushGroupPageCount,
                                             SDT_WA_REUSE_FIFO )
              != IDE_SUCCESS );

    aHeader->mSortGroupID       = SDT_WAGROUPID_SORT;
    aHeader->mInitMergePosition = NULL;
    aHeader->mScanPosition      = NULL;
    SC_MAKE_NULL_GRID( aHeader->mGRID );

    IDE_TEST( sdtWASortMap::create( sWASeg,
                                    aHeader->mSortGroupID,
                                    SDT_WM_TYPE_POINTER,
                                    0, /* Slot Count */
                                    2, /* aVersionCount */
                                    &sWASeg->mSortMapHdr )
              != IDE_SUCCESS );

    IDE_TEST( aHeader->mSortStack.initialize( IDU_MEM_SM_TEMP,
                                              ID_SIZEOF(smnSortStack) )
              != IDE_SUCCESS);

    /* BUG-39440 삽입한 순서대로 추출이 가능하도록 keyColumn이 없는 경우
     * 정렬없는 insert를 함
     */
    if ( sKeyColumn != NULL )
    {
        aHeader->mTTState = SMI_TTSTATE_SORT_INSERTNSORT;
    }
    else
    {
        aHeader->mTTState = SMI_TTSTATE_SORT_INSERTONLY;
    }
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**************************************************************************
 * Description :
 * 삽입된 Row들을 정렬한다.
 * 이미 Flush된 Run이 Queue에 존재하면 Merge가 필요하고,
 * 그 외의 경우는 InMemorySort로 수행한다.
 *
 * 만약 WindowSort를 하는 상황이면, 이미 저장된 Row를 재정렬한다.
 *
 * <IN>
 * aHeader        - 대상 Table
 ***************************************************************************/
IDE_RC sdtSortModule::sort(smiTempTableHeader * aHeader)
{
    switch( aHeader->mTTState )
    {
        /********************** 정렬이 필요한 상황 ***********************/
        case SMI_TTSTATE_SORT_INSERTNSORT:
            /* keyColumn이 없는게 의도된 상황이라면 assert 를 제거 해야 한다. */
            IDE_ERROR( aHeader->mKeyColumnList != NULL );

            if ( aHeader->mRunQueue.getQueueLength() > 0 )
            {
                /* Run을 생성한 적이 있음. InMemory가 아니라는 이야기 */
                IDE_TEST( merge( aHeader ) != IDE_SUCCESS );

                if ( SM_IS_FLAG_ON( aHeader->mTTFlag, SMI_TTFLAG_RANGESCAN ) )
                {
                    IDE_TEST( makeIndex( aHeader ) != IDE_SUCCESS );
                    IDE_TEST( indexScan( aHeader ) != IDE_SUCCESS );
                }
                else
                {
                    IDE_TEST( mergeScan( aHeader ) != IDE_SUCCESS );
                }
            }
            else
            {
                IDE_TEST( inMemoryScan( aHeader ) != IDE_SUCCESS );
            }
            break;

            /******************** 그냥 순서대로 읽어봅시다 *******************/
        case SMI_TTSTATE_SORT_INSERTONLY:
        case SMI_TTSTATE_SORT_SCAN:
            IDE_ERROR( aHeader->mKeyColumnList == NULL );

            if ( aHeader->mRunQueue.getQueueLength() > 0 )
            {
                IDE_TEST( scan( aHeader ) != IDE_SUCCESS );
            }
            else
            {
                IDE_TEST( inMemoryScan( aHeader ) != IDE_SUCCESS );
            }
            break;

            /********************** 재정렬을 요구한 경우 *********************/
        case SMI_TTSTATE_SORT_INMEMORYSCAN:
            /* 재정렬은 RangeScan만 요구함 */
            IDE_ERROR( SM_IS_FLAG_ON( aHeader->mTTFlag, SMI_TTFLAG_RANGESCAN ) );
            /* 어차피 InMemory이므로, 그냥 변경된 Column에 따라 재정렬하면 됨*/
            IDE_TEST( inMemoryScan( aHeader ) != IDE_SUCCESS );
            break;
        case SMI_TTSTATE_SORT_INDEXSCAN:
            /* 재정렬은 RangeScan만 요구함 */
            IDE_ERROR( SM_IS_FLAG_ON( aHeader->mTTFlag, SMI_TTFLAG_RANGESCAN ) );
            IDE_TEST( extractNSort( aHeader ) != IDE_SUCCESS );
            break;
        default:
        case SMI_TTSTATE_SORT_EXTRACTNSORT:
        case SMI_TTSTATE_SORT_MERGE:
        case SMI_TTSTATE_SORT_MAKETREE:
        case SMI_TTSTATE_SORT_MERGESCAN:
            IDE_ERROR( 0 );
            break;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**************************************************************************
 * Description :
 * 마지막으로 예측 통계치를 계산한다
 *
 * <IN>
 * aHeader        - 대상 Table
 ***************************************************************************/
IDE_RC sdtSortModule::calcEstimatedStats( smiTempTableHeader * aHeader )
{
    smiTempTableStats * sStatsPtr;
    ULong               sDataPageCount;  /* 총 Data의 Page개수 */
    UInt                sRowPageCount;   /* Row가 차지할 최대 Page개수 */
    UInt                sRunPageCount;   /* Run의 Page 크기 */

    sStatsPtr      = aHeader->mStatsPtr;
    sDataPageCount = ( SDT_TR_HEADER_SIZE_FULL + aHeader->mRowSize )
        * aHeader->mRowCount / SD_PAGE_SIZE;
    sRowPageCount  = ( aHeader->mRowSize / SD_PAGE_SIZE ) + 2;
    sRunPageCount  = sRowPageCount * 2;

    /* Optimal(InMemory)은 모든 데이터가 SortArea에 담길 크기면 된다. */
    sStatsPtr->mEstimatedOptimalSize = sDataPageCount;

    /* OnePass는,
     * N개의 Page를 가진 Run이 만들어진 후 ( InserttNSort )
     * M개의 Run을 Merge하여 완료되면 Onepass이다.
     *
     * N = SortAreaSize / SD_PAGE_SIZE => SortArea에 쌓은 후 Row를 만드니까
     * M = SortAreaSize / RunSize      => SortArea에 Run을 쌓으니까
     * Data = N * M                    => N개의 Page를 가진 Run M개
     *
     * Data = SortAreaSize * SortAreaSize / SD_PAGE_SIZE / RunSize
     * SortAreaSize^2 = Data * RunSize * SD_PAGE_SIZE
     * SortAreaSize = sqrt( Data*RunSize ) */
    sStatsPtr->mEstimatedSubOptimalSize = (ULong)idlOS::sqrt(
        (SDouble) sDataPageCount * sRunPageCount );

    /* SortGroupRatio를 고려하고, PageHeader나 WCB등을 고려하여,
     * 1.3배 정도 곱해줌 */
    sStatsPtr->mEstimatedOptimalSize =
        (ULong)( sStatsPtr->mEstimatedOptimalSize
                 * SD_PAGE_SIZE * 100 / aHeader->mWorkGroupRatio * 1.3);
    sStatsPtr->mEstimatedSubOptimalSize =
        (ULong)( sStatsPtr->mEstimatedSubOptimalSize
                 * SD_PAGE_SIZE * 100 / aHeader->mWorkGroupRatio * 1.3);

    return IDE_SUCCESS;
}


/**************************************************************************
 * Description :
 * 정리한다. WorkArea및 Cursor등은 smiTemp에서 알아서 한다.
 *
 * <IN>
 * aHeader        - 대상 Table
 ***************************************************************************/
IDE_RC sdtSortModule::destroy( smiTempTableHeader * aHeader )
{
    if ( aHeader->mTTState != SMI_TTSTATE_INIT )
    {
        IDE_TEST( aHeader->mSortStack.destroy() != IDE_SUCCESS );
    }

    if ( aHeader->mInitMergePosition != NULL )
    {
        IDE_TEST( iduMemMgr::free( aHeader->mInitMergePosition )
                  != IDE_SUCCESS );
        aHeader->mInitMergePosition = NULL;
    }

    if ( aHeader->mScanPosition != NULL )
    {
        IDE_TEST( iduMemMgr::free( aHeader->mScanPosition )
                  != IDE_SUCCESS );
        aHeader->mScanPosition = NULL;
    }

    /* 종료되면서 예측 통계치를 계산한다. */
    IDE_TEST( calcEstimatedStats( aHeader ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***************************************************************************
 * Description :
 * InMemoryScan용 커서를 엽니다.
 *
 * <IN>
 * aHeader        - 대상 Table
 * <OUT>
 * aCursor        - 반환값
 ***************************************************************************/
IDE_RC sdtSortModule::openCursorInMemoryScan( smiTempTableHeader * aHeader,
                                              smiSortTempCursor  * aCursor )
{
    sdtSortSegHdr      * sWASeg  = (sdtSortSegHdr*)aHeader->mWASegment;

    if ( aHeader->mTTState == SMI_TTSTATE_SORT_INSERTNSORT )
    {
        /*Sort 없이 접근한 경우 */
        IDE_ERROR( aHeader->mRunQueue.getQueueLength() == 0 );
        IDE_ERROR( aHeader->mKeyColumnList != NULL );

        aHeader->mTTState = SMI_TTSTATE_SORT_INMEMORYSCAN;
    }
    else
    {
        if ( aHeader->mTTState == SMI_TTSTATE_SORT_INSERTONLY )
        {
            IDE_ERROR( aHeader->mRunQueue.getQueueLength() == 0 );
            IDE_ERROR( aHeader->mKeyColumnList == NULL );

            aHeader->mTTState = SMI_TTSTATE_SORT_INMEMORYSCAN;
        }
        else
        {
            IDE_ERROR( aHeader->mTTState == SMI_TTSTATE_SORT_INMEMORYSCAN );
        }
    }

    SC_MAKE_NULL_GRID( aCursor->mGRID );
    aHeader->mFetchGroupID  = SDT_WAGROUPID_NONE;
    aCursor->mWAGroupID     = SDT_WAGROUPID_NONE;
    aCursor->mStoreCursor   = storeCursorInMemoryScan;
    aCursor->mRestoreCursor = restoreCursorInMemoryScan;

    if ( SM_IS_FLAG_ON( aCursor->mTCFlag, SMI_TCFLAG_FORWARD ) )
    {
        aCursor->mFetch   = fetchInMemoryScanForward;
        aCursor->mSeq     = -1;
        aCursor->mLastSeq = sdtWASortMap::getSlotCount( &sWASeg->mSortMapHdr );
    }
    else
    {
        aCursor->mFetch   = fetchInMemoryScanBackward;
        aCursor->mSeq     = sdtWASortMap::getSlotCount( &sWASeg->mSortMapHdr );
        aCursor->mLastSeq = -1;
    }

    if ( SM_IS_FLAG_ON( aCursor->mTCFlag, SMI_TCFLAG_ORDEREDSCAN ) )
    {
        /* nothing to do ... */
    }
    else
    {
        IDE_ERROR( SM_IS_FLAG_ON( aCursor->mTCFlag, SMI_TCFLAG_RANGESCAN ) );

        /* RangeScan을 하기 위해 BeforeFirst, 또는 AfterLast를 탐색함 */
        if ( SM_IS_FLAG_ON( aCursor->mTCFlag, SMI_TCFLAG_FORWARD ) )
        {
            IDE_TEST( traverseInMemoryScan( aHeader,
                                            &aCursor->mRange->minimum,
                                            ID_TRUE, /* aDirection */
                                            &aCursor->mSeq )
                      != IDE_SUCCESS );
            IDE_TEST( traverseInMemoryScan( aHeader,
                                            &aCursor->mRange->maximum,
                                            ID_FALSE, /* aDirection */
                                            &aCursor->mLastSeq )
                      != IDE_SUCCESS );
            aCursor->mLastSeq++;
        }
        else
        {
            IDE_TEST( traverseInMemoryScan( aHeader,
                                            &aCursor->mRange->maximum,
                                            ID_FALSE, /* aDirection */
                                            &aCursor->mSeq )
                      != IDE_SUCCESS );
            IDE_TEST( traverseInMemoryScan( aHeader,
                                            &aCursor->mRange->minimum,
                                            ID_TRUE, /* aDirection */
                                            &aCursor->mLastSeq )
                      != IDE_SUCCESS );
            aCursor->mLastSeq--;
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***************************************************************************
 * Description :
 * MergeScan용 커서를 엽니다.
 *
 * <IN>
 * aHeader        - 대상 Table
 * <OUT>
 * aCursor        - 반환값
 ***************************************************************************/
IDE_RC sdtSortModule::openCursorMergeScan( smiTempTableHeader * aHeader,
                                           smiSortTempCursor     * aCursor )
{
    IDE_ERROR( SM_IS_FLAG_ON( aCursor->mTCFlag, SMI_TCFLAG_FORWARD ) );
    IDE_ERROR( SM_IS_FLAG_ON( aCursor->mTCFlag, SMI_TCFLAG_ORDEREDSCAN ) );

    aCursor->mWAGroupID     = SDT_WAGROUPID_SORT;
    aHeader->mFetchGroupID  = SDT_WAGROUPID_NONE;
    IDE_TEST( makeMergeRuns( aHeader,
                             aHeader->mInitMergePosition )
              != IDE_SUCCESS );

    /* RangeScan 불가능 */
    IDE_ERROR( aCursor->mRange == smiGetDefaultKeyRange() );

    SC_MAKE_NULL_GRID( aCursor->mGRID );
    aCursor->mFetch         = fetchMergeScan;
    aCursor->mStoreCursor   = storeCursorMergeScan;
    aCursor->mRestoreCursor = restoreCursorMergeScan;

    aCursor->mMergePosition = NULL;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***************************************************************************
 * Description :
 * IndexScan용 커서를 엽니다.
 *
 * <IN>
 * aHeader        - 대상 Table
 * <OUT>
 * aCursor        - 반환값
 ***************************************************************************/
IDE_RC sdtSortModule::openCursorIndexScan( smiTempTableHeader * aHeader,
                                           smiSortTempCursor     * aCursor )
{
    IDE_ERROR( aHeader->mTTState == SMI_TTSTATE_SORT_INDEXSCAN );

    aCursor->mWAGroupID     = SDT_WAGROUPID_LNODE;
    aHeader->mFetchGroupID  = SDT_WAGROUPID_LNODE;
    if ( aHeader->mHeight == 0 )
    {
        SC_MAKE_NULL_GRID( aCursor->mGRID );
    }
    else
    {
        IDE_TEST( traverseIndexScan( aHeader, aCursor ) != IDE_SUCCESS );
    }

    if ( SM_IS_FLAG_ON( aCursor->mTCFlag, SMI_TCFLAG_FORWARD ) )
    {
        aCursor->mFetch = fetchIndexScanForward;
    }
    else
    {
        aCursor->mFetch = fetchIndexScanBackward;
    }

    aCursor->mStoreCursor   = storeCursorIndexScan;
    aCursor->mRestoreCursor = restoreCursorIndexScan;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***************************************************************************
 * Description :
 * Scan용 커서를 엽니다. (BUG-39450)
 *
 * <IN>
 * aHeader        - 대상 Table
 * <OUT>
 * aCursor        - 반환값
 ***************************************************************************/
IDE_RC sdtSortModule::openCursorScan( smiTempTableHeader * aHeader,
                                      smiSortTempCursor     * aCursor )
{
    sdtSortSegHdr      * sWASeg  = (sdtSortSegHdr*)aHeader->mWASegment;

    IDE_ERROR( aHeader->mTTState == SMI_TTSTATE_SORT_SCAN );
    /* RangeScan 불가능 */
    IDE_ERROR( aCursor->mRange == smiGetDefaultKeyRange() );

    IDE_ERROR( SM_IS_FLAG_ON( aCursor->mTCFlag, SMI_TCFLAG_FORWARD ) );
    IDE_ERROR( SM_IS_FLAG_ON( aCursor->mTCFlag, SMI_TCFLAG_ORDEREDSCAN ) );
    IDE_ERROR( aHeader->mScanPosition != NULL );

    /*Fetch시 사용하는 GroupID 설정 */
    aCursor->mWAGroupID    = SDT_WAGROUPID_SCAN;

    aCursor->mFetch         = fetchScan;
    aCursor->mStoreCursor   = storeCursorScan;
    aCursor->mRestoreCursor = restoreCursorScan;

    aCursor->mPinIdx        = 0;
    aHeader->mScanPosition[ SDT_TEMP_SCANPOS_PINIDX ] = 0;

    SC_MAKE_GRID( aCursor->mGRID,
                  aHeader->mSpaceID,
                  aHeader->mScanPosition[ SDT_TEMP_SCANPOS_HEADERIDX ],
                  0 /* offset */);

    IDE_TEST( sdtSortSegment::getPageWithFix( sWASeg,
                                              aCursor->mWAGroupID,
                                              aCursor->mGRID,
                                              &aCursor->mWPID,
                                              &aCursor->mWAPagePtr,
                                              &aCursor->mSlotCount )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***************************************************************************
 * Description :
 * InMemory용으로 대상 Row를 탐색합니다.
 *
 * <IN>
 * aHeader        - 대상 Table
 * aCallBack      - 비교하는데 쓰일 콜백
 * aDirection     - 방향
 * <OUT>
 * aSeq           - 찾은 위치
 ***************************************************************************/
IDE_RC sdtSortModule::traverseInMemoryScan( smiTempTableHeader * aHeader,
                                            const smiCallBack  * aCallBack,
                                            idBool               aDirection,
                                            SInt               * aSeq )
{
    sdtSortSegHdr      * sWASeg = (sdtSortSegHdr*)aHeader->mWASegment;
    SInt                 sLeft;
    SInt                 sRight;
    SInt                 sMid;
    idBool               sResult;
    sdtSortScanInfo      sScanInfo;

    sLeft  = 0;
    sRight = sdtWASortMap::getSlotCount( &sWASeg->mSortMapHdr ) - 1;

    sScanInfo.mFetchEndOffset = aHeader->mRowSize;

    while( sLeft <= sRight )
    {
        sMid   = ( sLeft + sRight ) >> 1;

        IDE_TEST( sdtWASortMap::getvULong( &sWASeg->mSortMapHdr,
                                           sMid,
                                           (vULong*)&sScanInfo.mTRPHeader )
                  != IDE_SUCCESS );

        IDE_TEST( sdtTempRow::fetch( sWASeg,
                                     SDT_WAGROUPID_NONE,
                                     aHeader->mRowBuffer4Compare,
                                     &sScanInfo )
                  != IDE_SUCCESS );

        IDE_TEST( aCallBack->callback( &sResult,
                                       sScanInfo.mValuePtr,
                                       NULL,
                                       0,
                                       SC_NULL_GRID,
                                       aCallBack->data )
                  != IDE_SUCCESS );

        if ( sResult == aDirection )
        {
            sRight = sMid - 1;
        }
        else
        {
            sLeft = sMid + 1;
        }
    }
    sMid   = ( sLeft + sRight ) >> 1;
    *aSeq  = sMid;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***************************************************************************
 * Description :
 * IndexScan용으로 대상 Row를 탐색합니다.
 *
 * <IN>
 * aHeader        - 대상 Table
 * aCursor        - 대상 커서
 ***************************************************************************/
IDE_RC sdtSortModule::traverseIndexScan( smiTempTableHeader * aHeader,
                                         smiSortTempCursor  * aCursor )
{
    sdtSortSegHdr      * sWASeg = (sdtSortSegHdr*)aHeader->mWASegment;
    scPageID             sNPID;
    const smiCallBack  * sCallBack;
    idBool               sResult;
    sdtSortScanInfo      sScanInfo;
    idBool               sIsValidSlot;
    UInt                 sHeight;
    SInt                 sLeft;
    SInt                 sRight;
    SInt                 sMid;

    /* RangeScan을 하기 위해 BeforeFirst, 또는 AfterLast를 탐색함 */
    aCursor->mWAGroupID = SDT_WAGROUPID_INODE;
    sNPID               = aHeader->mRootWPID;
    sHeight             = aHeader->mHeight;
    sScanInfo.mFetchEndOffset = aHeader->mRowSize;

    if ( SM_IS_FLAG_ON( aCursor->mTCFlag, SMI_TCFLAG_FORWARD ) )
    {
        sCallBack = &aCursor->mRange->minimum;
    }
    else
    {
        IDE_ERROR( SM_IS_FLAG_ON( aCursor->mTCFlag, SMI_TCFLAG_BACKWARD ) )
            sCallBack = &aCursor->mRange->maximum;
    }

    while ( sHeight > 0 )
    {
        if ( sHeight == 1 )
        {
            aCursor->mWAGroupID = SDT_WAGROUPID_LNODE;
        }
        else
        {
            /* nothing to do */
        }
        SC_MAKE_GRID( aCursor->mGRID, aHeader->mSpaceID, sNPID, 0 );

        IDE_TEST( sdtSortSegment::getPageWithFix( sWASeg,
                                                  aCursor->mWAGroupID,
                                                  aCursor->mGRID,
                                                  &aCursor->mWPID,
                                                  &aCursor->mWAPagePtr,
                                                  &aCursor->mSlotCount )
                  != IDE_SUCCESS );

        sLeft  = 0;
        sRight = aCursor->mSlotCount - 1;
        IDE_ERROR( aCursor->mSlotCount > 0 );

        while ( sLeft <= sRight )
        {
            sMid   = ( sLeft + sRight ) >> 1;

            sdtSortSegment::getSlot( aCursor->mWAPagePtr,
                                     aCursor->mSlotCount,
                                     sMid,
                                     (UChar**)&sScanInfo.mTRPHeader,
                                     &sIsValidSlot );
            IDE_ERROR( sIsValidSlot == ID_TRUE );

            IDE_TEST( sdtTempRow::fetch( sWASeg,
                                         aCursor->mWAGroupID,
                                         aHeader->mRowBuffer4Compare,
                                         &sScanInfo )
                      != IDE_SUCCESS );

            IDE_TEST( sCallBack->callback( &sResult,
                                           sScanInfo.mValuePtr,
                                           NULL,        /* aDirectKey */
                                           0,           /* aDirectKeyPartialSize */
                                           SC_NULL_GRID,
                                           sCallBack->data )
                      != IDE_SUCCESS );

            if ( SM_IS_FLAG_ON( aCursor->mTCFlag, SMI_TCFLAG_FORWARD ) )
            {
                if ( sResult == ID_TRUE )
                {
                    sRight = sMid - 1;
                }
                else
                {
                    sLeft = sMid + 1;
                }
            }
            else
            {
                IDE_ERROR( SM_IS_FLAG_ON( aCursor->mTCFlag, SMI_TCFLAG_BACKWARD ) );
                if ( sResult == ID_TRUE )
                {
                    sLeft = sMid + 1;
                }
                else
                {
                    sRight = sMid - 1;
                }
            }
        } // while ( sLeft <= sRight )

        sMid = ( sLeft + sRight ) >> 1;

        if ( sHeight > 1 )
        {
            /* LeftMost가 Value를 갖는 방식이기 때문에, -1일 수 있음
             * 그러므로 0으로 재조정해줌 */
            if ( sMid < 0 )
            {
                sMid = 0;
            }
            else
            {
                /* nothing to do */
            }
            SC_MAKE_GRID( aCursor->mGRID, aHeader->mSpaceID, sNPID, sMid );

            /* ChildNode 탐색. */
            IDE_TEST( sdtTempRow::fetchByGRID( sWASeg,
                                               aCursor->mWAGroupID,
                                               aCursor->mGRID,
                                               aHeader->mRowSize,
                                               aHeader->mRowBuffer4Fetch,
                                               &sScanInfo )
                      != IDE_SUCCESS );
            IDE_ERROR( SM_IS_FLAG_ON( sScanInfo.mTRPHeader->mTRFlag,
                                      SDT_TRFLAG_CHILDGRID ) );

            sNPID = SC_MAKE_PID( sScanInfo.mTRPHeader->mChildGRID );
        }
        else
        {
            /* 탐색 끝났음 */
            SC_MAKE_GRID( aCursor->mGRID, aHeader->mSpaceID, sNPID, sMid );
        }
        sHeight--;
    }// while ( sHeight > 0 )

    /* 탐색할 대상이 있으면 */
    if ( SC_GRID_IS_NOT_NULL( aCursor->mGRID ) )
    {
        /* getPage해둠 */
        IDE_TEST( sdtSortSegment::getPageWithFix( sWASeg,
                                                  aHeader->mFetchGroupID,
                                                  aCursor->mGRID,
                                                  &aCursor->mWPID,
                                                  &aCursor->mWAPagePtr,
                                                  &aCursor->mSlotCount )
                  != IDE_SUCCESS );
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
 * 커서로부터 Row를 가져옵니다.
 *
 * <IN>
 * aCursor        - 대상 Cursor
 * <OUT>
 * aRow           - 대상 Row
 * aGRID          - 가져온 Row의 GRID
 ***************************************************************************/
IDE_RC sdtSortModule::fetchInMemoryScanForward( smiSortTempCursor* aCursor,
                                                UChar  ** aRow,
                                                scGRID  * aRowGRID )
{
    sdtSortSegHdr      * sWASeg = (sdtSortSegHdr*)aCursor->mTTHeader->mWASegment;
    sdtSortTRPHdr      * sTRPHeader;
    idBool               sResult = ID_FALSE;

    do
    {
        aCursor->mSeq++;
        if ( aCursor->mSeq >= aCursor->mLastSeq )
        {
            break;
        }

        IDE_TEST( sdtWASortMap::getvULong( &sWASeg->mSortMapHdr,
                                           aCursor->mSeq,
                                           (vULong*)&sTRPHeader )
                  != IDE_SUCCESS );

        /* InMemoryGroup에서 가져올 경우, Page가 아닌 SortHashMap으로
         * Pointing해야 하기 때문에, 다음과 같이 설정한다. */
        SC_MAKE_GRID( aCursor->mGRID,
                      SDT_SPACEID_WAMAP,
                      aCursor->mSeq,
                      0 );
        IDE_TEST( sdtTempRow::filteringAndFetch( aCursor,
                                                 sTRPHeader,
                                                 aRow,
                                                 aRowGRID,
                                                 &sResult )
                  != IDE_SUCCESS );
    }
    while( sResult == ID_FALSE );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**************************************************************************
 * Description :
 * 커서로부터 Row를 가져옵니다. ( backward로 )
 *
 * <IN>
 * aCursor        - 대상 Cursor
 * <OUT>
 * aRow           - 대상 Row
 * aGRID          - 가져온 Row의 GRID
 ***************************************************************************/
IDE_RC sdtSortModule::fetchInMemoryScanBackward( smiSortTempCursor* aCursor,
                                                 UChar  ** aRow,
                                                 scGRID  * aRowGRID )
{
    sdtSortSegHdr      * sWASeg = (sdtSortSegHdr*)aCursor->mTTHeader->mWASegment;
    sdtSortTRPHdr      * sTRPHeader;
    idBool               sResult = ID_FALSE;

    do
    {
        aCursor->mSeq--;
        if ( aCursor->mSeq <= aCursor->mLastSeq )
        {
            break;
        }

        IDE_TEST( sdtWASortMap::getvULong( &sWASeg->mSortMapHdr,
                                           aCursor->mSeq,
                                           (vULong*)&sTRPHeader )
                  != IDE_SUCCESS );

        /* InMemoryGroup에서 가져올 경우, Page가 아닌 SortHashMap으로
         * Pointing해야 하기 때문에, 다음과 같이 설정한다. */
        SC_MAKE_GRID( aCursor->mGRID,
                      SDT_SPACEID_WAMAP,
                      aCursor->mSeq,
                      0 );
        IDE_TEST( sdtTempRow::filteringAndFetch( aCursor,
                                                 sTRPHeader,
                                                 aRow,
                                                 aRowGRID,
                                                 &sResult )
                  != IDE_SUCCESS );
    }
    while( sResult == ID_FALSE );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**************************************************************************
 * Description :
 * 커서로부터 Row를 가져옵니다.
 *
 * <IN>
 * aCursor        - 대상 Cursor
 * <OUT>
 * aRow           - 대상 Row
 * aGRID          - 가져온 Row의 GRID
 ***************************************************************************/
IDE_RC sdtSortModule::fetchMergeScan( smiSortTempCursor* aCursor,
                                      UChar  ** aRow,
                                      scGRID  * aRowGRID )
{
    smiTempTableHeader * sHeader = aCursor->mTTHeader;
    sdtSortSegHdr      * sWASeg = (sdtSortSegHdr*)sHeader->mWASegment;
    sdtTempMergeRunInfo sTopRunInfo;
    scGRID               sGRID;
    idBool               sResult = ID_FALSE;

    /* 이전에 패치된 GRID와 테이블에서 마지막으로 접근한 GRID가 다르면
       커서에 매달린 런정보를 이용해 map 재구축 */
    if ( SC_GRID_IS_NOT_NULL(aCursor->mGRID) &&
         SC_GRID_IS_NOT_EQUAL(aCursor->mGRID, sHeader->mGRID) )
    {
        IDE_TEST( makeMergeRuns( sHeader,
                                 aCursor->mMergePosition )
                  != IDE_SUCCESS );
    }
    else
    {
        /* nothing to do */
    }

    while( sResult == ID_FALSE )
    {
        /* Heap의 Top Slot을 뽑아냄 */
        IDE_TEST( sdtWASortMap::get( &sWASeg->mSortMapHdr,
                                     1,   /* aIdx */
                                     (void*)&sTopRunInfo )
                  != IDE_SUCCESS );

        if ( sTopRunInfo.mRunNo == SDT_TEMP_RUNINFO_NULL )
        {
            /* Run에서 Row들을 전부 뽑아냄 */
            SC_MAKE_NULL_GRID( aCursor->mGRID );
            break;
        }

        getGRIDFromRunInfo( sHeader, &sTopRunInfo, &sGRID );
        /* 패치된 GRID */
        SC_COPY_GRID( sGRID, aCursor->mGRID );
        /* 테이블에서 마지막으로 접근한 GRID */
        SC_COPY_GRID( sGRID, sHeader->mGRID );

        IDE_TEST( sdtTempRow::filteringAndFetchByGRID( aCursor,
                                                       aCursor->mGRID,
                                                       aRow,
                                                       aRowGRID,
                                                       &sResult )
                  != IDE_SUCCESS );
        IDE_TEST( heapPop( sHeader ) != IDE_SUCCESS );
    }

    /* BUG-41284 : 두개 이상의 커서가 접근하면 커서가 봐야 하는 페이지와
     *  테이블에서 열려 있는 페이지가 다를수 있으므로
     *  나중에 복원할수 있도록 런정보를 저장해둠 */
    IDE_TEST( makeMergePosition( sHeader,
                                 &aCursor->mMergePosition )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**************************************************************************
 * Description :
 * 커서로부터 Row를 가져옵니다.
 *
 * <IN>
 * aCursor        - 대상 Cursor
 * <OUT>
 * aRow           - 대상 Row
 * aGRID          - 가져온 Row의 GRID
 ***************************************************************************/
IDE_RC sdtSortModule::fetchIndexScanForward( smiSortTempCursor* aCursor,
                                             UChar  ** aRow,
                                             scGRID  * aRowGRID )
{
    smiTempTableHeader * sHeader = aCursor->mTTHeader;
    sdtSortSegHdr      * sWASeg = (sdtSortSegHdr*)sHeader->mWASegment;
    scPageID             sNPID;
    idBool               sIsValidSlot;
    const smiCallBack  * sCallBack = &aCursor->mRange->maximum;
    idBool               sResult = ID_FALSE;
    sdtSortScanInfo      sScanInfo;

    sScanInfo.mFetchEndOffset = sHeader->mRowSize;

    while ( ( sResult == ID_FALSE ) && ( SC_GRID_IS_NOT_NULL( aCursor->mGRID ) ) )
    {
        aCursor->mGRID.mOffset++;
        sdtSortSegment::getSlot( aCursor->mWAPagePtr,
                                 aCursor->mSlotCount,
                                 SC_MAKE_OFFSET( aCursor->mGRID ),
                                 (UChar**)&sScanInfo.mTRPHeader,
                                 &sIsValidSlot );

        if ( sIsValidSlot == ID_FALSE )
        {
            /*다음 페이지로*/
            sNPID = sdtTempPage::getNextPID( aCursor->mWAPagePtr );
            if ( sNPID == SC_NULL_PID )
            {
                SC_MAKE_NULL_GRID( aCursor->mGRID );
                break;
            }
            SC_MAKE_GRID( aCursor->mGRID, sHeader->mSpaceID, sNPID, -1 );

            IDE_TEST( sdtSortSegment::getPageWithFix( sWASeg,
                                                      sHeader->mFetchGroupID,
                                                      aCursor->mGRID,
                                                      &aCursor->mWPID,
                                                      &aCursor->mWAPagePtr,
                                                      &aCursor->mSlotCount )
                      != IDE_SUCCESS );
        }
        else
        {
            if ( SM_IS_FLAG_ON( sScanInfo.mTRPHeader->mTRFlag, SDT_TRFLAG_HEAD ) )
            {
#ifdef DEBUG
                sdtWCB * sWCBPtr = sdtSortSegment::getWCBInternal( sWASeg,
                                                                   aCursor->mWPID );
                IDE_DASSERT( sWCBPtr->mWAPagePtr == aCursor->mWAPagePtr ) ;
                IDE_DASSERT( sWCBPtr->mNPageID   == SC_MAKE_PID( aCursor->mGRID ) ) ;
                IDE_DASSERT( sdtSortSegment::isFixedPage( sWCBPtr) == ID_TRUE );
#endif

                IDE_TEST( sdtTempRow::fetch( sWASeg,
                                             aCursor->mWAGroupID,
                                             sHeader->mRowBuffer4Fetch,
                                             &sScanInfo )
                          != IDE_SUCCESS );

                /******************** Range 체크 *****************************/
                /* 이 Node가 Range 조건을 만족한는 마지막 node인지 검사가 필요함 */
                IDE_TEST( sCallBack->callback( &sResult,
                                               sScanInfo.mValuePtr,
                                               NULL,        /* aDirectKey */
                                               0,           /* aDirectKeyPartialSize */
                                               SC_NULL_GRID,
                                               sCallBack->data )
                          != IDE_SUCCESS );
                if ( sResult == ID_FALSE )
                {
                    /* 이 Node 내에서 Range가 끝난다. */
                    /* 마지막까지 탐색 완료하였음 */
                    break;
                }
                else
                {
                    /* 마지막 Slot도 MaxRange에 포함된다. 따라서 이 Node내의 모든
                     * Data는 탐색 대상에 포함된다. */
                }

                IDE_TEST( sdtTempRow::filteringAndFetch( aCursor,
                                                         sScanInfo.mTRPHeader,
                                                         aRow,
                                                         aRowGRID,
                                                         &sResult )
                          != IDE_SUCCESS );
            }
            else
            {
                /* nothing to do */
            }
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**************************************************************************
 * Description :
 * 커서로부터 Row를 가져옵니다.
 *
 * <IN>
 * aCursor        - 대상 Cursor
 * <OUT>
 * aRow           - 대상 Row
 * aGRID          - 가져온 Row의 GRID
 ***************************************************************************/
IDE_RC sdtSortModule::fetchIndexScanBackward( smiSortTempCursor* aCursor,
                                              UChar  ** aRow,
                                              scGRID  * aRowGRID )
{
    smiTempTableHeader * sHeader = aCursor->mTTHeader;
    sdtSortSegHdr      * sWASeg = (sdtSortSegHdr*)sHeader->mWASegment;
    scPageID             sNPID;
    idBool               sIsValidSlot;
    const smiCallBack  * sCallBack = &aCursor->mRange->minimum;
    idBool               sResult = ID_FALSE;
    sdtSortScanInfo      sScanInfo;

    sScanInfo.mFetchEndOffset = sHeader->mRowSize;

    while ( ( sResult == ID_FALSE ) &&
            ( SC_GRID_IS_NOT_NULL( aCursor->mGRID ) ) )
    {
        if ( aCursor->mGRID.mOffset == 0 )
        {
            /*이전 페이지로*/
            sNPID = sdtTempPage::getPrevPID( aCursor->mWAPagePtr );
            if ( sNPID == SC_NULL_PID )
            {
                SC_MAKE_NULL_GRID( aCursor->mGRID );
                break;
            }
            SC_MAKE_GRID( aCursor->mGRID, sHeader->mSpaceID, sNPID,  0 );
            IDE_TEST( sdtSortSegment::getPageWithFix( sWASeg,
                                                      sHeader->mFetchGroupID,
                                                      aCursor->mGRID,
                                                      &aCursor->mWPID,
                                                      &aCursor->mWAPagePtr,
                                                      &aCursor->mSlotCount )
                      != IDE_SUCCESS );
            aCursor->mGRID.mOffset = aCursor->mSlotCount;
        }
        else
        {
            aCursor->mGRID.mOffset--;
            sdtSortSegment::getSlot( aCursor->mWAPagePtr,
                                     aCursor->mSlotCount,
                                     SC_MAKE_OFFSET( aCursor->mGRID ),
                                     (UChar**)&sScanInfo.mTRPHeader,
                                     &sIsValidSlot );
            IDE_ERROR( sIsValidSlot == ID_TRUE );

            if ( SM_IS_FLAG_ON( sScanInfo.mTRPHeader->mTRFlag, SDT_TRFLAG_HEAD ) )
            {
                IDE_TEST( sdtTempRow::fetch( sWASeg,
                                             aCursor->mWAGroupID,
                                             sHeader->mRowBuffer4Fetch,
                                             &sScanInfo )
                          != IDE_SUCCESS );

                /******************** Range 체크 *****************************/
                /* 이 Node가 Range를 만족하는 마지막 node인지 확인이 필요함 */
                IDE_TEST( sCallBack->callback( &sResult,
                                               sScanInfo.mValuePtr,
                                               NULL,        /* aDirectKey */
                                               0,           /* aDirectKeyPartialSize */
                                               SC_NULL_GRID,
                                               sCallBack->data )
                          != IDE_SUCCESS );

                if ( sResult == ID_FALSE )
                {
                    /* 이 Node 내에서 Range가 끝난다. */
                    /* 마지막까지 탐색 완료하였음 */
                    break;
                }
                else
                {
                    /* MaxRange에 포함된다. 따라서 이 Node내의 모든
                     * Data는 탐색 대상에 포함된다. */
                }

                IDE_TEST( sdtTempRow::filteringAndFetch( aCursor,
                                                         sScanInfo.mTRPHeader,
                                                         aRow,
                                                         aRowGRID,
                                                         &sResult )
                          != IDE_SUCCESS );
            }
            else
            {
                /* nothing to do */
            }
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**************************************************************************
 * Description :
 * 커서로부터 Row를 가져옵니다. (BUG-39450)
 *
 * <IN>
 * aCursor        - 대상 Cursor
 * <OUT>
 * aRow           - 대상 Row
 * aGRID          - 가져온 Row의 GRID
 ***************************************************************************/
IDE_RC sdtSortModule::fetchScan( smiSortTempCursor* aCursor,
                                 UChar  ** aRow,
                                 scGRID  * aRowGRID )
{
    smiTempTableHeader * sHeader;
    sdtSortSegHdr      * sWASeg;
    idBool               sResult = ID_FALSE;
    scPageID             sNPID;
    UChar              * sPtr = NULL;
    idBool               sIsValidSlot;
    scPageID           * sScanPos;
    UInt                 sIdx;

    IDE_ASSERT( aCursor != NULL );

    sHeader = (smiTempTableHeader *)aCursor->mTTHeader;
    sWASeg  = (sdtSortSegHdr*)sHeader->mWASegment;
    sScanPos = sHeader->mScanPosition;

    IDE_ERROR( sHeader->mTTState == SMI_TTSTATE_SORT_SCAN );
    IDE_ERROR( sHeader->mKeyColumnList == NULL );

    while( SC_GRID_IS_NOT_NULL( aCursor->mGRID ) )
    {
        sdtSortSegment::getSlot( aCursor->mWAPagePtr,
                                 aCursor->mSlotCount,
                                 SC_MAKE_OFFSET( aCursor->mGRID ),
                                 &sPtr,
                                 &sIsValidSlot );

        if ( sIsValidSlot == ID_FALSE )
        {
            /*다음 페이지로*/
            sNPID = sdtTempPage::getNextPID( aCursor->mWAPagePtr );

            if ( sNPID == SC_NULL_PID )
            {
                sIdx = sScanPos[ SDT_TEMP_SCANPOS_PINIDX ];

                if ( aCursor->mPinIdx == sIdx )
                {
                    /* nothing to do */
                }
                else
                {
                    /* 이전에 패치된 page와 테이블에서 마지막으로 접근한 page가 다르면
                     *  cursor가 봐야 하는 값으로 복원 */
                    sIdx = aCursor->mPinIdx;
                }
                sIdx++;

                /* 더이상 남은 Run 도 없음 */
                if ( sScanPos[SDT_TEMP_SCANPOS_SIZEIDX] == sIdx )
                {
                    SC_MAKE_NULL_GRID( aCursor->mGRID );
                    break;
                }
                else
                {
                    /* 패치된 idx */
                    aCursor->mPinIdx = sIdx;
                    /* 테이블에서 마지막으로 접근한 idx */
                    sScanPos[ SDT_TEMP_SCANPOS_PINIDX ] = sIdx;
                    sNPID = sScanPos[ SDT_TEMP_SCANPOS_PIDIDX( sIdx ) ];
                }
            }
            else
            {
                /* nothing to do */
            }

            SC_MAKE_GRID( aCursor->mGRID, sHeader->mSpaceID, sNPID, 0 );
            IDE_TEST( sdtSortSegment::getPageWithFix( sWASeg,
                                                      aCursor->mWAGroupID,
                                                      aCursor->mGRID,
                                                      &aCursor->mWPID,
                                                      &aCursor->mWAPagePtr,
                                                      &aCursor->mSlotCount )
                      != IDE_SUCCESS );
        }
        else
        {
            if ( SM_IS_FLAG_ON( ( (sdtSortTRPHdr*)sPtr )->mTRFlag, SDT_TRFLAG_HEAD ) )
            {
                /* 해당 Row를 가져옴 */
                IDE_TEST( sdtTempRow::filteringAndFetchByGRID( aCursor,
                                                               aCursor->mGRID,
                                                               aRow,
                                                               aRowGRID,
                                                               &sResult )

                          != IDE_SUCCESS );
            }
            else
            {
                /* nothing to do */
            }

            aCursor->mGRID.mOffset++;

            if (sResult == ID_TRUE )
            {
                break;
            }
            else
            {
                /* nothing to do */
            }
        }

    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**************************************************************************
 * Description :
 * 커서의 위치를 저장합니다.
 *
 * <IN>
 * aCursor        - 대상 Cursor
 * <OUT>
 * aPosition      - 저장한 위치
 ***************************************************************************/
IDE_RC sdtSortModule::storeCursorInMemoryScan( smiSortTempCursor* aCursor,
                                               smiTempPosition * aPosition )
{
    IDE_ERROR( aPosition->mOwner == aCursor );
    IDE_ERROR( aPosition->mTTState == SMI_TTSTATE_SORT_INMEMORYSCAN );
    IDE_ERROR( SM_IS_FLAG_ON( ((sdtSortTRPHdr*)aCursor->mRowPtr)->mTRFlag,
                              SDT_TRFLAG_HEAD ) );

    SC_COPY_GRID( aCursor->mGRID, aPosition->mGRID );
    aPosition->mRowPtr = aCursor->mRowPtr;
    aPosition->mSeq    = aCursor->mSeq;


    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**************************************************************************
 * Description :
 * 커서를 저장한 위치로 되돌립니다.
 *
 * <IN>
 * aCursor        - 대상 Cursor
 * aPosition      - 저장한 위치
 ***************************************************************************/
IDE_RC sdtSortModule::restoreCursorInMemoryScan( smiSortTempCursor* aCursor,
                                                 smiTempPosition * aPosition )
{
    IDE_ERROR( aPosition->mOwner == aCursor );
    IDE_ERROR( aPosition->mTTState == SMI_TTSTATE_SORT_INMEMORYSCAN );

    SC_COPY_GRID( aPosition->mGRID, aCursor->mGRID );
    aCursor->mRowPtr = aPosition->mRowPtr;
    aCursor->mSeq    = aPosition->mSeq;

    IDE_ERROR( SM_IS_FLAG_ON( ((sdtSortTRPHdr*)aCursor->mRowPtr)->mTRFlag,
                              SDT_TRFLAG_HEAD ) );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**************************************************************************
 * Description :
 * 커서의 위치를 저장합니다.
 *
 * <IN>
 * aCursor        - 대상 Cursor
 * <OUT>
 * aPosition      - 저장한 위치
 ***************************************************************************/
IDE_RC sdtSortModule::storeCursorMergeScan( smiSortTempCursor* aCursor,
                                            smiTempPosition * aPosition )
{
    smiTempTableHeader * sHeader   = aCursor->mTTHeader;
    sdtSortSegHdr      * sWASeg    = (sdtSortSegHdr*)sHeader->mWASegment;

    IDE_ERROR( aPosition->mOwner == aCursor );
    IDE_ERROR( aPosition->mTTState == SMI_TTSTATE_SORT_MERGESCAN );
    IDE_ERROR( SM_IS_FLAG_ON( ((sdtSortTRPHdr*)aCursor->mRowPtr)->mTRFlag,
                              SDT_TRFLAG_HEAD ) );

    IDE_TEST( makeMergePosition( sHeader,
                                 &aPosition->mExtraInfo )
              != IDE_SUCCESS );

    SC_COPY_GRID( aCursor->mGRID, aPosition->mGRID );
    aPosition->mRowPtr = aCursor->mRowPtr;

    sdtSortSegment::convertFromWGRIDToNGRID( sWASeg,
                                             aPosition->mGRID,
                                             &aPosition->mGRID );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**************************************************************************
 * Description :
 * 커서를 저장한 위치로 되돌립니다.
 *
 * <IN>
 * aCursor        - 대상 Cursor
 * aPosition      - 저장한 위치
 ***************************************************************************/
IDE_RC sdtSortModule::restoreCursorMergeScan( smiSortTempCursor* aCursor,
                                              smiTempPosition * aPosition )
{
    smiTempTableHeader * sHeader = aCursor->mTTHeader;
    sdtSortSegHdr      * sWASeg  = (sdtSortSegHdr*)sHeader->mWASegment;
    sdtWCB             * sWCBPtr;
    idBool               sIsValid;

    IDE_ERROR( aPosition->mOwner == aCursor );
    IDE_ERROR( aPosition->mTTState == SMI_TTSTATE_SORT_MERGESCAN );

    IDE_TEST( makeMergeRuns( sHeader,
                             aPosition->mExtraInfo )
              != IDE_SUCCESS );
    SC_COPY_GRID( aPosition->mGRID, aCursor->mGRID );

    /* 테이블에서 마지막으로 접근한 GRID
       정확하게는 smiTempTable::restoreCursor에서 sHeader->mGRID 갱신됨. */
    SC_COPY_GRID( aPosition->mGRID, sHeader->mGRID );

    if ( aPosition->mRowPtr == NULL )
    {
        aCursor->mRowPtr = NULL;
    }
    else
    {
        sWCBPtr = sdtSortSegment::findWCB( sWASeg, SC_MAKE_PID( aPosition->mGRID ) );

        // makeMergeRuns()에서 load되었다. 반드시 있어야 한다.
        IDE_ERROR( sWCBPtr != NULL );

        if (( aPosition->mRowPtr > sWCBPtr->mWAPagePtr ) &&
            ( aPosition->mRowPtr < ( sWCBPtr->mWAPagePtr + SD_PAGE_SIZE )))
        {
            // aPosition->mRowPtr이 NULL이거나,
            // WAPagePtr범위 안이면 Cursor에 설정한다.
            // PageHdr나 Row Hdr이 있으므로 <=,>= 로 검사할 필요는 없다.
            aCursor->mRowPtr = aPosition->mRowPtr;
        }
        else
        {
            IDE_ERROR( sdtTempPage::getSelfPID( sWCBPtr->mWAPagePtr ) == SC_MAKE_PID( aCursor->mGRID ) );

            // BUG-46322 WAPage의 Pointer가 변경 되었다면, 다시 읽어온다.
            sdtSortSegment::getSlot( sWCBPtr->mWAPagePtr,
                                     sdtTempPage::getSlotCount( sWCBPtr->mWAPagePtr ),
                                     SC_MAKE_OFFSET( aCursor->mGRID ),
                                     &aCursor->mRowPtr,
                                     &sIsValid );

            IDE_ERROR( sIsValid == ID_TRUE );
        }

        IDE_ERROR( SM_IS_FLAG_ON( ((sdtSortTRPHdr*)aCursor->mRowPtr)->mTRFlag,
                                  SDT_TRFLAG_HEAD ) );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**************************************************************************
 * Description :
 * 커서의 위치를 저장합니다.
 *
 * <IN>
 * aCursor        - 대상 Cursor
 * <OUT>
 * aPosition      - 저장한 위치
 ***************************************************************************/
IDE_RC sdtSortModule::storeCursorIndexScan( smiSortTempCursor* aCursor,
                                            smiTempPosition * aPosition )
{
    sdtSortSegHdr      * sWASeg    = (sdtSortSegHdr*)aCursor->mTTHeader->mWASegment;

    IDE_ERROR( aPosition->mOwner == aCursor );
    IDE_ERROR( aPosition->mTTState == SMI_TTSTATE_SORT_INDEXSCAN );
    IDE_ERROR( SM_IS_FLAG_ON( ((sdtSortTRPHdr*)aCursor->mRowPtr)->mTRFlag,
                              SDT_TRFLAG_HEAD ) );

    SC_COPY_GRID( aCursor->mGRID, aPosition->mGRID );
    aPosition->mRowPtr    = aCursor->mRowPtr;
    aPosition->mWAPagePtr = aCursor->mWAPagePtr;
    aPosition->mSlotCount = aCursor->mSlotCount;
    aPosition->mWPID      = aCursor->mWPID;

    IDE_DASSERT( aPosition->mWPID != SM_NULL_PID );

    sdtSortSegment::convertFromWGRIDToNGRID( sWASeg,
                                             aPosition->mGRID,
                                             &aPosition->mGRID );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**************************************************************************
 * Description :
 * 커서를 저장한 위치로 되돌립니다.
 *
 * <IN>
 * aCursor        - 대상 Cursor
 * aPosition      - 저장한 위치
 ***************************************************************************/
IDE_RC sdtSortModule::restoreCursorIndexScan( smiSortTempCursor* aCursor,
                                              smiTempPosition * aPosition )
{
    smiTempTableHeader * sHeader = aCursor->mTTHeader;
    sdtSortSegHdr      * sWASeg = (sdtSortSegHdr*)sHeader->mWASegment;
    UChar              * sPtr;
    sdtWCB             * sWCBPtr;
    idBool               sIsValidSlot;

    IDE_ERROR( aPosition->mOwner == aCursor );
    IDE_ERROR( aPosition->mTTState == SMI_TTSTATE_SORT_INDEXSCAN );

    SC_COPY_GRID( aPosition->mGRID, aCursor->mGRID );
    aCursor->mRowPtr    = aPosition->mRowPtr;
    aCursor->mWAPagePtr = aPosition->mWAPagePtr;
    aCursor->mSlotCount = aPosition->mSlotCount;

    IDE_DASSERT( aCursor->mWPID   != SM_NULL_PID );
    IDE_DASSERT( aPosition->mWPID != SM_NULL_PID );

    sWCBPtr = sdtSortSegment::getWCBInternal( sWASeg,
                                              aPosition->mWPID );

    IDE_DASSERT( sWCBPtr->mWAPagePtr == aCursor->mWAPagePtr );

    if ( sWCBPtr->mNPageID != aCursor->mGRID.mPageID )
    {
        /* 그 사이 이전 Position에서 읽었던 page가 변경되었음.
         * 다시 읽음.*/
        IDE_TEST( sdtSortSegment::getPageWithFix( sWASeg,
                                                  sHeader->mFetchGroupID,
                                                  aCursor->mGRID,
                                                  &aCursor->mWPID,
                                                  &aCursor->mWAPagePtr,
                                                  &aCursor->mSlotCount )
                  != IDE_SUCCESS );
        sdtSortSegment::getSlot( aCursor->mWAPagePtr,
                                 aCursor->mSlotCount,
                                 SC_MAKE_OFFSET( aCursor->mGRID ),
                                 &sPtr,
                                 &sIsValidSlot );
        IDE_ERROR( sIsValidSlot == ID_TRUE );

        aCursor->mRowPtr = sPtr;
    }
    else
    {
        if ( aCursor->mWPID != aPosition->mWPID )
        {
            sdtSortSegment::unfixWAPage( sWASeg, aCursor->mWPID );
            aCursor->mWPID = aPosition->mWPID;
            sdtSortSegment::fixWAPage( sWCBPtr );
        }
    }

    IDE_ERROR( SM_IS_FLAG_ON( ((sdtSortTRPHdr*)aCursor->mRowPtr)->mTRFlag,
                              SDT_TRFLAG_HEAD ) );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**************************************************************************
 * Description :
 * 커서의 위치를 저장합니다. (BUG-39450)
 *
 * <IN>
 * aCursor        - 대상 Cursor
 * <OUT>
 * aPosition      - 저장한 위치
 ***************************************************************************/
IDE_RC sdtSortModule::storeCursorScan( smiSortTempCursor* aCursor,
                                       smiTempPosition * aPosition )
{
    sdtSortSegHdr * sWASeg = (sdtSortSegHdr*)aCursor->mTTHeader->mWASegment;

    IDE_ERROR( aPosition->mOwner == aCursor );
    IDE_ERROR( aPosition->mTTState == SMI_TTSTATE_SORT_SCAN );
    IDE_ERROR( SM_IS_FLAG_ON( ((sdtSortTRPHdr*)aCursor->mRowPtr)->mTRFlag,
                              SDT_TRFLAG_HEAD ) );

    SC_COPY_GRID( aCursor->mGRID, aPosition->mGRID );
    aPosition->mRowPtr = aCursor->mRowPtr;
    aPosition->mPinIdx = aCursor->mPinIdx;
    aPosition->mWPID   = aCursor->mWPID;

    IDE_DASSERT( aPosition->mWPID != SM_NULL_PID );

    sdtSortSegment::convertFromWGRIDToNGRID( sWASeg,
                                             aPosition->mGRID,
                                             &aPosition->mGRID );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**************************************************************************
 * Description :
 * 커서를 저장한 위치로 되돌립니다. (BUG-39450)
 *
 * <IN>
 * aCursor        - 대상 Cursor
 * aPosition      - 저장한 위치
 ***************************************************************************/
IDE_RC sdtSortModule::restoreCursorScan( smiSortTempCursor* aCursor,
                                         smiTempPosition * aPosition )
{
    smiTempTableHeader * sHeader   = aCursor->mTTHeader;
    sdtSortSegHdr      * sWASeg    = (sdtSortSegHdr*)sHeader->mWASegment;
    UChar              * sPtr;
    sdtWCB             * sWCBPtr;
    idBool               sIsValidSlot;

    IDE_ERROR( aPosition->mOwner == aCursor );
    IDE_ERROR( aPosition->mTTState == SMI_TTSTATE_SORT_SCAN );
    IDE_ERROR( sHeader->mScanPosition != NULL);

    SC_COPY_GRID( aPosition->mGRID, aCursor->mGRID );
    aCursor->mRowPtr = aPosition->mRowPtr;
    aCursor->mPinIdx = aPosition->mPinIdx;

    IDE_DASSERT( aCursor->mWPID   != SM_NULL_PID );
    IDE_DASSERT( aPosition->mWPID != SM_NULL_PID );

    sWCBPtr = sdtSortSegment::getWCBInternal( sWASeg,
                                              aPosition->mWPID );

    IDE_DASSERT( sWCBPtr->mWAPagePtr == aCursor->mWAPagePtr );

    if ( sWCBPtr->mNPageID != aCursor->mGRID.mPageID )
    {
        /* 그 사이 이전 Position에서 읽었던 page가 변경되었음.
         * 다시 읽음.*/
        IDE_TEST( sdtSortSegment::getPageWithFix( sWASeg,
                                                  sHeader->mFetchGroupID,
                                                  aCursor->mGRID,
                                                  &aCursor->mWPID,
                                                  &aCursor->mWAPagePtr,
                                                  &aCursor->mSlotCount )

                  != IDE_SUCCESS );
        sdtSortSegment::getSlot( aCursor->mWAPagePtr,
                                 aCursor->mSlotCount,
                                 SC_MAKE_OFFSET( aCursor->mGRID ),
                                 &sPtr,
                                 &sIsValidSlot );
        IDE_ERROR( sIsValidSlot == ID_TRUE );

        aCursor->mRowPtr = sPtr;
    }
    else
    {
        if ( aCursor->mWPID != aPosition->mWPID )
        {
            sdtSortSegment::unfixWAPage( sWASeg, aCursor->mWPID );
            aCursor->mWPID = aPosition->mWPID;
            sdtSortSegment::fixWAPage( sWCBPtr );
        }
    }

    IDE_ERROR( SM_IS_FLAG_ON( ((sdtSortTRPHdr*)aCursor->mRowPtr)->mTRFlag,
                              SDT_TRFLAG_HEAD ) );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/**************************************************************************
 * Description :
 * 이전에 삽입한 Row를 추출하여 재정렬합니다.
 *
 * 데이터를 추출해 정렬하는 extractNSort 작업을 수행한다.
 * 완료되면 스스로 merge 함수를 호출한다.
 ***************************************************************************/
IDE_RC sdtSortModule::extractNSort(smiTempTableHeader * aHeader)
{
    scPageID            sNPID;
    scPageID            sNextNPID;
    scPageID            sWPID;
    UInt                sInitGroupPageCount;
    UInt                sSortGroupPageCount;
    UInt                sFlushGroupPageCount;
    UChar             * sWAPagePtr = NULL;
    scGRID              sGRID ;
    UInt                sSlotCount = 0;
    UInt                sWAMapIdx  = 0;
    sdtSortSegHdr     * sWASeg     = (sdtSortSegHdr*)aHeader->mWASegment;
    sdtSortInsertResult   sTRInsertResult;
    UInt                i;

    /* Index단계에서의 모든 Group을 땐다. */
    IDE_TEST( sdtSortSegment::dropWAGroup( sWASeg,
                                           SDT_WAGROUPID_LNODE,
                                           ID_TRUE ) /*wait4flush */
              != IDE_SUCCESS );

    IDE_TEST( sdtSortSegment::dropWAGroup( sWASeg,
                                           SDT_WAGROUPID_INODE,
                                           ID_TRUE ) /*wait4flush */
              != IDE_SUCCESS );

    /* InsertNSort와 같은 모양의 Group으로 재생성 */
    sInitGroupPageCount = sdtSortSegment::getAllocableWAGroupPageCount(
        sWASeg, SDT_WAGROUPID_INIT );

    /* BUG-46427 각 group당 2개 이상의 page가 있어야 한다. */
    IDE_ERROR( sInitGroupPageCount >= ( SDT_WAGROUP_MIN_PAGECOUNT * 3 ) );

    sSortGroupPageCount = sInitGroupPageCount * aHeader->mWorkGroupRatio / 100;

    if ( sSortGroupPageCount < SDT_WAGROUP_MIN_PAGECOUNT )
    {
        /* BUG-46427 각 group당 2개 이상의 page가 있어야 한다. */
        sSortGroupPageCount = SDT_WAGROUP_MIN_PAGECOUNT;
    }

    sFlushGroupPageCount = ( sInitGroupPageCount - sSortGroupPageCount ) / 2;

    if ( sFlushGroupPageCount < SDT_WAGROUP_MIN_PAGECOUNT )
    {
        /* BUG-46427 각 group당 2개 이상의 page가 있어야 한다. */
        sFlushGroupPageCount = SDT_WAGROUP_MIN_PAGECOUNT;
        sSortGroupPageCount = sInitGroupPageCount - ( sFlushGroupPageCount * 2 );
    }

    IDE_TEST( sdtSortSegment::createWAGroup( sWASeg,
                                             SDT_WAGROUPID_SORT,
                                             sSortGroupPageCount,
                                             SDT_WA_REUSE_INMEMORY )
              != IDE_SUCCESS );

    IDE_TEST( sdtSortSegment::createWAGroup( sWASeg,
                                             SDT_WAGROUPID_FLUSH,
                                             sFlushGroupPageCount,
                                             SDT_WA_REUSE_FIFO )
              != IDE_SUCCESS );

    IDE_TEST( sdtSortSegment::createWAGroup( sWASeg,
                                             SDT_WAGROUPID_SUBFLUSH,
                                             sFlushGroupPageCount,
                                             SDT_WA_REUSE_FIFO )
              != IDE_SUCCESS );

    IDE_TEST( sdtWASortMap::create( sWASeg,
                                    aHeader->mSortGroupID,
                                    SDT_WM_TYPE_POINTER,
                                    0, /* Slot Count */
                                    2, /* aVersionCount */
                                    &sWASeg->mSortMapHdr )
              != IDE_SUCCESS );

    aHeader->mTTState = SMI_TTSTATE_SORT_EXTRACTNSORT;

    sNPID = aHeader->mRowHeadNPID;
    while( sNPID != SC_NULL_PID )
    {
        SC_MAKE_GRID( sGRID, aHeader->mSpaceID, sNPID, 0 );
        IDE_TEST( sdtSortSegment::getPage( sWASeg,
                                           SDT_WAGROUPID_SUBFLUSH,
                                           sGRID,
                                           &sWPID,
                                           &sWAPagePtr )
                  != IDE_SUCCESS );
        sSlotCount = sdtTempPage::getSlotCount(sWAPagePtr);
        sNextNPID = sdtTempPage::getNextPID( sWAPagePtr );

        for( i = 0 ; i < sSlotCount ; i++ )
        {
            while( 1 )
            {
                SC_MAKE_GRID( sGRID, aHeader->mSpaceID, sNPID, i );
                IDE_TEST( copyRowByGRID( aHeader,
                                         sGRID,
                                         SDT_COPY_EXTRACT_ROW,
                                         SDT_TEMP_PAGETYPE_INMEMORYGROUP,
                                         SC_NULL_GRID, /* ChildRID */
                                         &sTRInsertResult )
                          != IDE_SUCCESS );

                /* WAMap에 슬롯 삽입 */
                if ( sTRInsertResult.mComplete == ID_TRUE )
                {
                    /* Row를 온전히 삽입하였음 */
                    IDE_TEST( sdtWASortMap::expand(
                                  &sWASeg->mSortMapHdr,
                                  SC_MAKE_PID( sTRInsertResult.mHeadRowpieceGRID ),
                                  &sWAMapIdx )
                              != IDE_SUCCESS );
                    if ( sWAMapIdx != SDT_WASLOT_UNUSED )
                    {
                        IDE_TEST( sdtWASortMap::setvULong(
                                      &sWASeg->mSortMapHdr,
                                      sWAMapIdx,
                                      (vULong*)&sTRInsertResult.mHeadRowpiecePtr )
                                  != IDE_SUCCESS );
                        break;
                    }
                }

                /* BUG-46438 한번에 한 건도 넣지 못할 정도로 공간이 작다면,
                 * sort temp를 사용 할 수 없다.*/
                IDE_TEST_RAISE( sdtWASortMap::getSlotCount( &sWASeg->mSortMapHdr ) == 0 ,
                                error_invalid_sortareasize );

                IDE_TEST( sortSortGroup( aHeader ) != IDE_SUCCESS );
                IDE_TEST( storeSortedRun( aHeader ) != IDE_SUCCESS );
            }
        }

        sNPID = sNextNPID;
        if ( sNPID == SC_NULL_PID )
        {
            break;
        }
    } // end of while ( sNPID != SC_NULL_PID )
    IDE_TEST( sdtSortSegment::dropWAGroup( sWASeg,
                                           SDT_WAGROUPID_SUBFLUSH,
                                           ID_TRUE ) /*wait4flush */
              != IDE_SUCCESS );

    /* 다 끝나면, merge, makeIndex, indexScan을 수행함 */
    IDE_TEST( merge( aHeader ) != IDE_SUCCESS );
    IDE_TEST( makeIndex( aHeader ) != IDE_SUCCESS );
    IDE_TEST( indexScan( aHeader ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( error_invalid_sortareasize);
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_INVALID_SORTAREASIZE ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**************************************************************************
 * Description :
 * run들을 merge한다.
 **************************************************************************/
IDE_RC sdtSortModule::merge(smiTempTableHeader * aHeader)
{
    sdtTempMergeRunInfo   sTopRunInfo;
    scGRID                sGRID;
    scPageID              sHeadNPID = SD_NULL_PID;
    sdtSortSegHdr       * sWASeg = (sdtSortSegHdr*)aHeader->mWASegment;
    sdtSortInsertResult sTRInsertResult;

    IDE_ERROR( ( aHeader->mTTState == SMI_TTSTATE_SORT_INSERTNSORT  ) ||
               ( aHeader->mTTState == SMI_TTSTATE_SORT_EXTRACTNSORT ) );
    IDE_ERROR( aHeader->mKeyColumnList != NULL );

    /* 마지막 Run의 Slot 개수 */
    aHeader->mStatsPtr->mExtraStat2 =
        sdtWASortMap::getSlotCount( &sWASeg->mSortMapHdr );

    /* 마지막 Run을 생성해서 내림 */
    if ( sdtWASortMap::getSlotCount( &sWASeg->mSortMapHdr ) > 0 )
    {
        IDE_TEST( sortSortGroup( aHeader ) != IDE_SUCCESS );
        IDE_TEST( storeSortedRun( aHeader ) != IDE_SUCCESS );
    }

    aHeader->mTTState = SMI_TTSTATE_SORT_MERGE;

    /* SortGroup을 초기화한다. Merge할 공간으로 만들기 위함이다. */
    IDE_TEST( sdtSortSegment::resetWAGroup( sWASeg,
                                            SDT_WAGROUPID_SORT,
                                            ID_FALSE/*wait4flsuh*/ )
              != IDE_SUCCESS );
    IDE_TEST( sdtSortSegment::resetWAGroup( sWASeg,
                                            SDT_WAGROUPID_FLUSH,
                                            ID_TRUE/*wait4flsuh*/ )
              != IDE_SUCCESS );

    while( 1 )
    {
        IDE_TEST( heapInit( aHeader ) != IDE_SUCCESS );

        if ( aHeader->mRunQueue.getQueueLength() == 0 )
        {
            /* 더이상 남은 Run이 없음. 즉 이번 Merge로 완료됨
             * 다음 상태로 진입함 */
            break;
        }

        sHeadNPID = SD_NULL_PID;
        while(1)
        {
            /* Heap의 Top Slot을 뽑아냄 */
            IDE_TEST( sdtWASortMap::get( &sWASeg->mSortMapHdr,
                                         1,
                                         (void*)&sTopRunInfo )
                      != IDE_SUCCESS );

            if ( sTopRunInfo.mRunNo == SDT_TEMP_RUNINFO_NULL )
            {
                /* Run에서 Row들을 전부 뽑아냄 */
                break;
            }

            getGRIDFromRunInfo( aHeader, &sTopRunInfo, &sGRID );

            IDE_TEST( copyRowByGRID( aHeader,
                                     sGRID,
                                     SDT_COPY_NORMAL,
                                     SDT_TEMP_PAGETYPE_SORTEDRUN,
                                     SC_NULL_GRID, /* ChildRID */
                                     &sTRInsertResult )
                      != IDE_SUCCESS );

            if ( sHeadNPID == SD_NULL_PID )
            {
                sHeadNPID = SC_MAKE_PID( sTRInsertResult.mHeadRowpieceGRID );
            }

            /* 복사 과정으로 인한 Row재배치를 통해 CurRowPageCount가
             * 증가할 수 있으니, 다시 계산함 */
            aHeader->mMaxRowPageCount = IDL_MAX(
                aHeader->mMaxRowPageCount, sTRInsertResult.mRowPageCount );

            IDE_TEST( heapPop( aHeader ) != IDE_SUCCESS );
        }

        IDE_TEST( aHeader->mRunQueue.enqueue(ID_FALSE, (void*)&sHeadNPID )
                  != IDE_SUCCESS );

        IDE_TEST( sdtSortSegment::resetWAGroup( sWASeg,
                                                SDT_WAGROUPID_FLUSH,
                                                ID_TRUE/*wait4flsuh*/ )
                  != IDE_SUCCESS );

        IDE_TEST(iduCheckSessionEvent(aHeader->mStatistics) != IDE_SUCCESS);
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/**************************************************************************
 * Description :
 * Index를 생성한다.
 **************************************************************************/
IDE_RC sdtSortModule::makeIndex(smiTempTableHeader * aHeader)
{

    UInt                   sInitGroupPageCount;
    UInt                   sINodeGroupPageCount;
    UInt                   sLNodeGroupPageCount;
    scPageID               sChildNPID;
    idBool                 sNeedMore = ID_FALSE;
    sdtSortSegHdr        * sWASeg = (sdtSortSegHdr*)aHeader->mWASegment;

    IDE_ERROR( aHeader->mTTState == SMI_TTSTATE_SORT_MERGE );
    aHeader->mTTState  = SMI_TTSTATE_SORT_MAKETREE;

    /* 이미 MergeRun이 구성되어있는 상태로 옴. 따라서 RunQueue는 비어있음 */
    IDE_ERROR( aHeader->mRunQueue.getQueueLength() == 0 );

    sInitGroupPageCount = sdtSortSegment::getAllocableWAGroupPageCount(
        sWASeg,
        SDT_WAGROUPID_INIT );

    if ( sInitGroupPageCount >= SDT_WAGROUP_MIN_PAGECOUNT )
    {
        /* BUG-46427 임시로 sub flush를 만들었다 해제하는 경우가 있다.
         * 그러므로 init group에 이전 sub flush group을 해제한
         * 비어있는 page가 있는지 확인해서 있다면 이 남은 page로
         * sub flush group을 만든다.*/
        IDE_TEST( sdtSortSegment::createWAGroup( sWASeg,
                                                 SDT_WAGROUPID_SUBFLUSH,
                                                 sInitGroupPageCount,
                                                 SDT_WA_REUSE_FIFO )
              != IDE_SUCCESS );
    }
    else
    {
        /* ExtraRow를 저장하기 위해 잠시 생성한다. */
        IDE_TEST( sdtSortSegment::splitWAGroup( sWASeg,
                                                SDT_WAGROUPID_FLUSH,
                                                SDT_WAGROUPID_SUBFLUSH,
                                                SDT_WA_REUSE_FIFO )
                  != IDE_SUCCESS );
    }

    IDE_TEST( makeLNodes( aHeader, &sNeedMore ) != IDE_SUCCESS );

    /*********************** Group 재구성 *****************************/
    IDE_TEST( sdtSortSegment::dropWAGroup( sWASeg,
                                           SDT_WAGROUPID_SUBFLUSH,
                                           ID_FALSE ) /*wait4flush */
              != IDE_SUCCESS );

    IDE_TEST( sdtSortSegment::dropWAGroup( sWASeg,
                                           SDT_WAGROUPID_FLUSH,
                                           ID_FALSE ) /*wait4flush */
              != IDE_SUCCESS );

    IDE_TEST( sdtSortSegment::dropWAGroup( sWASeg,
                                           SDT_WAGROUPID_SORT,
                                           ID_FALSE ) /*wait4flush */
              != IDE_SUCCESS );

    sInitGroupPageCount = sdtSortSegment::getAllocableWAGroupPageCount(
        sWASeg,
        SDT_WAGROUPID_INIT );

    /* BUG-46427 각 group당 2개 이상의 page가 있어야 한다. */
    IDE_ERROR( sInitGroupPageCount >= ( SDT_WAGROUP_MIN_PAGECOUNT * 2 ) );

    sLNodeGroupPageCount = sInitGroupPageCount * aHeader->mWorkGroupRatio / 100;

    if ( sLNodeGroupPageCount < SDT_WAGROUP_MIN_PAGECOUNT )
    {
        /* BUG-46427 각 group당 2개 이상의 page가 있어야 한다. */
        sLNodeGroupPageCount = SDT_WAGROUP_MIN_PAGECOUNT;
    }

    sINodeGroupPageCount = sInitGroupPageCount - sLNodeGroupPageCount;

    if ( sINodeGroupPageCount < SDT_WAGROUP_MIN_PAGECOUNT )
    {
        /* BUG-46427 각 group당 2개 이상의 page가 있어야 한다. */
        sINodeGroupPageCount = SDT_WAGROUP_MIN_PAGECOUNT;
        sLNodeGroupPageCount = sInitGroupPageCount - sINodeGroupPageCount ;
    }

    IDE_TEST( sdtSortSegment::createWAGroup( sWASeg,
                                             SDT_WAGROUPID_INODE,
                                             sINodeGroupPageCount,
                                             SDT_WA_REUSE_LRU )
              != IDE_SUCCESS );

    IDE_TEST( sdtSortSegment::createWAGroup( sWASeg,
                                             SDT_WAGROUPID_LNODE,
                                             sLNodeGroupPageCount,
                                             SDT_WA_REUSE_LRU )
              != IDE_SUCCESS );

    /* IndexScan에서는 Map을 더이상 사용 안함 */
    sdtWASortMap::disable( sWASeg );

    sChildNPID = aHeader->mRowHeadNPID;
    while( sNeedMore == ID_TRUE )
    {
        IDE_TEST( makeINodes( aHeader, &sChildNPID, &sNeedMore )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**************************************************************************
 * Description :
 * Index의 LeafNode를 생성한다.
 *
 * <IN>
 * aHeader        - 대상 Table
 * <OUT>
 * aNeedMoreINode - 추가적으로 INode를 만들어야 하는가?
 **************************************************************************/
IDE_RC sdtSortModule::makeLNodes( smiTempTableHeader * aHeader,
                                  idBool             * aNeedMoreINode )
{
    sdtTempMergeRunInfo   sTopRunInfo;
    scPageID               sHeadNPID = SD_NULL_PID;
    scGRID                 sGRID;
    scPageID               sInsertedNPID;
    sdtSortInsertResult  sTRInsertResult;
    sdtSortSegHdr        * sWASeg = (sdtSortSegHdr*)aHeader->mWASegment;

    *aNeedMoreINode = ID_FALSE;

    /*  Heap에서 Top Slot을 뽑아냄 */
    IDE_TEST( sdtWASortMap::get( &sWASeg->mSortMapHdr,
                                 1,
                                 (void*)&sTopRunInfo )
              != IDE_SUCCESS );
    while( sTopRunInfo.mRunNo != SDT_TEMP_RUNINFO_NULL )
    {
        getGRIDFromRunInfo( aHeader, &sTopRunInfo, &sGRID );

        IDE_TEST( copyRowByGRID( aHeader,
                                 sGRID,
                                 SDT_COPY_MAKE_LNODE,
                                 SDT_TEMP_PAGETYPE_LNODE,
                                 SC_NULL_GRID, /*ChildRID*/
                                 &sTRInsertResult )
                  != IDE_SUCCESS );
        sInsertedNPID = SC_MAKE_PID( sTRInsertResult.mHeadRowpieceGRID );

        if ( sHeadNPID == SD_NULL_PID ) /* 최초 삽입 */
        {
            sHeadNPID = sInsertedNPID;

            aHeader->mRowHeadNPID = sHeadNPID;
            aHeader->mRootWPID    = sHeadNPID;
            aHeader->mHeight      = 1;
        }
        else
        {
            if ( sHeadNPID != sInsertedNPID ) /* 노드가 2개 이상 */
            {
                *aNeedMoreINode = ID_TRUE;
            }
        }

        IDE_TEST( heapPop( aHeader ) != IDE_SUCCESS ); /* 다음 row를 가져옴*/

        IDE_TEST( sdtWASortMap::get( &sWASeg->mSortMapHdr,
                                     1,
                                     (void*)&sTopRunInfo )
                  != IDE_SUCCESS );
    } /*while */

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**************************************************************************
 * Description :
 * Index의 INode를 생성한다.
 *
 * - INode를 만드는 Logic 설명
 * 다음 트리를 바탕으로 설명한다.
 *       D-----+
 *    +--|1 3 7|
 *    |  +-----+
 *    |   |    |
 * A---+ B---+ C---+
 * |1 2|-|3 5|-|7 9|
 * +---+ +---+ +---+
 *
 * 1) resetWAGroup의 필요성
 *   A,B,C 노드를 만든 후 D노드를 만들기 위해 현 지점에 왔을때,
 * resetWAGroup을 하지 않으면, C와 D를 이웃 노드로 묶게 된다.
 * 따라서 resetWAGroup을 통해 HintPage를 초기화, 이웃이 아니게 한다.
 *
 * 2) INode생성 방법.
 *  INode는 간단하게 ChildNode들의 0번째 Key값만 위로올리면 된다.
 * 위의 경우, A,B,C 각 노드의 1,3,7 이 올라오게 된다.
 *  이중 1번의 경우 LeftMost의 역할을 수행한다.
 *
 * * 이 방법은 BufferMiss가 일어나기 쉬워 효율은 그다지 좋지 않다. 하지만
 * RangeScan을 쓰는 경우는 SortJoin뿐이며, 그 경우도 대부분 InMemory로 풀린다.
 * 따라서 INode를 만드는 경우는 적고, 또 INode의 Page수도 적기에 이를 택한다.
 *   물론 이후 개량도 가능하다. ( sdtBUBuild::makeTree참조 )
 *
 * <IN>
 * aHeader        - 대상 Table
 * <IN/OUT>
 * aChildNPID     - 만들 대상 ChildNPID. 끝나면, 다음 ChildNPID를 반환함.
 * <OUT>
 * aNeedMoreINode - 추가적으로 INode를 만들어야 하는가?
 *****************************************************************************/
IDE_RC sdtSortModule::makeINodes( smiTempTableHeader * aHeader,
                                  scPageID           * aChildNPID,
                                  idBool             * aNeedMoreINode )
{
    scPageID               sHeadNPID = SD_NULL_PID;
    scPageID               sChildNPID;
    scPageID               sWPID;
    scPageID               sInsertedNPID;
    scGRID                 sGRID;
    sdtSortInsertResult  sTRInsertResult;
    UChar                * sWAPagePtr = NULL;
    sdtSortSegHdr        * sWASeg     = (sdtSortSegHdr*)aHeader->mWASegment;

    IDE_TEST( sdtSortSegment::resetWAGroup( sWASeg,
                                            SDT_WAGROUPID_INODE,
                                            ID_TRUE/*wait4flsuh*/ )
              != IDE_SUCCESS );

    sChildNPID      = *aChildNPID;
    sHeadNPID       = SD_NULL_PID;
    *aNeedMoreINode = ID_FALSE;
    while( sChildNPID != SC_NULL_PID )
    {
        /* 이전에 만든 Node들에서부터 순차 탐색을 시도함 */
        SC_MAKE_GRID( sGRID, aHeader->mSpaceID, sChildNPID, 0 );
        IDE_TEST( copyRowByGRID( aHeader,
                                 sGRID,
                                 SDT_COPY_MAKE_INODE,
                                 SDT_TEMP_PAGETYPE_INODE,
                                 sGRID,        /*ChildRID*/
                                 &sTRInsertResult )
                  != IDE_SUCCESS );
        sInsertedNPID = SC_MAKE_PID( sTRInsertResult.mHeadRowpieceGRID );

        if ( sHeadNPID == SD_NULL_PID )
        {
            sHeadNPID = sInsertedNPID;

            aHeader->mRootWPID = sHeadNPID;
            aHeader->mHeight++;

            /* 아무리 생각해도, 1024이상 높이가 되긴 어려움 */
            IDE_ERROR( aHeader->mHeight <= 1024 );
        }
        else
        {
            if ( sHeadNPID != sInsertedNPID )
            {
                *aNeedMoreINode = ID_TRUE;
            }
        }

        /* 다음 페이지의 ID를 가져온다. */
        IDE_TEST( sdtSortSegment::getPage( sWASeg,
                                           SDT_WAGROUPID_LNODE,
                                           sGRID,
                                           &sWPID,
                                           &sWAPagePtr )
                  != IDE_SUCCESS );
        sChildNPID = sdtTempPage::getNextPID( sWAPagePtr );
    } /*while */

    *aChildNPID = sHeadNPID;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**************************************************************************
 * Description :
 * InMemoryScan상태로 만든다. 현재 삽입된 Wa상의 Row들을 정렬한다.
 ***************************************************************************/
IDE_RC sdtSortModule::inMemoryScan(smiTempTableHeader * aHeader)
{
    smiColumnList  * sColumn = (smiColumnList*)aHeader->mKeyColumnList;

    if ( sColumn != NULL )
    {
        IDE_TEST( sortSortGroup( aHeader ) != IDE_SUCCESS );
    }
    aHeader->mTTState = SMI_TTSTATE_SORT_INMEMORYSCAN;

    aHeader->mOpenCursor    = openCursorInMemoryScan;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**************************************************************************
 * Description :
 * mergeScan 상태로 만든다.
 ***************************************************************************/
IDE_RC sdtSortModule::mergeScan(smiTempTableHeader * aHeader)
{
    aHeader->mTTState = SMI_TTSTATE_SORT_MERGESCAN;

    IDE_TEST( makeMergePosition( aHeader,
                                 &aHeader->mInitMergePosition )
              != IDE_SUCCESS );

    aHeader->mOpenCursor    = openCursorMergeScan;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**************************************************************************
 * Description :
 * indexScan상태로 만든다.
 ***************************************************************************/
IDE_RC sdtSortModule::indexScan(smiTempTableHeader * aHeader)
{
    aHeader->mTTState = SMI_TTSTATE_SORT_INDEXSCAN;

    aHeader->mOpenCursor    = openCursorIndexScan;

    return IDE_SUCCESS;
}

/**************************************************************************
 * Description :
 * fullScan상태로 만든다.
 ***************************************************************************/
IDE_RC sdtSortModule::scan(smiTempTableHeader * aHeader)
{
    aHeader->mTTState = SMI_TTSTATE_SORT_SCAN;

    IDE_TEST( makeScanPosition( aHeader,
                                &aHeader->mScanPosition )
              != IDE_SUCCESS );

    aHeader->mOpenCursor    = openCursorScan;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**************************************************************************
 * Description :
 * SortGroup의 Row들을 정렬한다.
 *
 * * 구현
 * Quicksort로 부분정렬 한 후, Merge한다.
 * 왜냐하면 QuickSort의 경우 최악의 경우 n^2 만큼 Compare하기 때문에 Row가
 * 많을때 정렬하면 되려 느려질 수 있기 때문이다. 따라서 PartitionSize에 따라
 * 나누어 해당 크기씩 QuickSort로 정렬한 후, 해당 Partition들을 merge한다.
 *
 * * MergeSort와 Version
 *   MergeSort는, 정렬 전 Slot의 위치를 유지하면서, 정렬된 Slot들을 저장할
 * 위치를 또 필요로 한다. 따라서 WAMap을 Versioning한다.
 *
 * - 예
 *           <-Left> <-Right>
 *           @       @
 * Version0: 1 5 6 9 2 3 4 8 3 5 7 8
 * Version1: . . . . . . . .
 *
 *             @     @
 * Version0: 1 5 6 9 2 3 4 8 3 5 7 8
 * Version1: 1 . . . . . . .
 *
 *             @       @
 * Version0: 1 5 6 9 2 3 4 8 3 5 7 8
 * Version1: 1 2 . . . . . .
 *
 *             @         @
 * Version0: 1 5 6 9 2 3 4 8 3 5 7 8
 * Version1: 1 2 3 . . . . .
 *
 *             @           @
 * Version0: 1 5 6 9 2 3 4 8 3 5 7 8
 * Version1: 1 2 3 4 . . . .
 *
 *               @         @
 * Version0: 1 5 6 9 2 3 4 8 3 5 7 8
 * Version1: 1 2 3 4 5 . . .
 *
 * ...
 *
 *                  @       @
 * Version0: 1 5 6 9 2 3 4 8 3 5 7 8
 * Version1: 1 2 3 4 5 6 8 9
 *
 * -- incVersionIdx() --
 *
 *           @               @
 * Version1: 1 2 3 4 5 6 8 9 3 5 7 8
 * Version2: 1 5 6 9 2 3 4 8         <- 이전에 Version0이었음
 * 다시 Merge를 반복함
 *
 ***************************************************************************/
IDE_RC sdtSortModule::sortSortGroup(smiTempTableHeader * aHeader)
{
    sdtSortSegHdr      * sWASeg = (sdtSortSegHdr*)aHeader->mWASegment;
    sdtWASortMapHdr    * sMapHdr = &sWASeg->mSortMapHdr;
    UInt                 sSlotCount;
    UInt                 sPartitionSize;
    vULong               sPtr;
    UInt                 i;
    UInt                 j;

    IDE_ERROR( ( aHeader->mTTState == SMI_TTSTATE_SORT_INSERTNSORT ) ||
               ( aHeader->mTTState == SMI_TTSTATE_SORT_EXTRACTNSORT )||
               ( aHeader->mTTState == SMI_TTSTATE_SORT_INMEMORYSCAN ) );

    IDE_ERROR ( aHeader->mKeyColumnList != NULL );

    sPartitionSize = smuProperty::getTempSortPartitionSize();
    sSlotCount     = sdtWASortMap::getSlotCount( sMapHdr );

    if ( sSlotCount > 0 )
    {
        if ( sPartitionSize == 0 )
        {
            IDE_TEST( quickSort( aHeader, 0, sSlotCount - 1 )
                      != IDE_SUCCESS );
        }
        else
        {
            /* Partition별로 나누어서 부분 QuickSort함 */
            for( i = 0 ; i < sSlotCount ; i += sPartitionSize )
            {
                j = IDL_MIN ( i + sPartitionSize,
                              sSlotCount );

                IDE_TEST( quickSort( aHeader, i, j - 1 )
                          != IDE_SUCCESS );
            }

            /* MergeSort를 수행함 */
            for( ; sPartitionSize < sSlotCount; sPartitionSize *= 2 )
            {
                for( i = 0 ; i < sSlotCount; i += sPartitionSize * 2 )
                {
                    if ( i + sPartitionSize < sSlotCount )
                    {
                        if ( i + sPartitionSize*2 > sSlotCount )
                        {
                            /* |----+-|
                             *
                             *  <--> <> */
                            IDE_TEST( mergeSort( aHeader,
                                                 i,
                                                 i + sPartitionSize - 1,
                                                 i + sPartitionSize,
                                                 sSlotCount - 1)
                                      != IDE_SUCCESS );
                        }
                        else
                        {
                            /* |----+----+--|
                             *
                             *  <--> <--> */

                            IDE_TEST( mergeSort( aHeader,
                                                 i,
                                                 i + sPartitionSize - 1,
                                                 i + sPartitionSize,
                                                 i + sPartitionSize * 2 -1 )
                                      != IDE_SUCCESS );
                        }
                    }
                    else
                    {
                        /* |--|
                         *
                         *  ~~~ <- 비교할, Right쪽이 없어 비교안함
                         * 다만 Map내에서 다음 Version으로 옮겨는 줘야함 */
                        for ( j = i ; j < sSlotCount ; j++ )
                        {
                            IDE_TEST( sdtWASortMap::getvULong( sMapHdr,
                                                               j,
                                                               &sPtr )
                                      != IDE_SUCCESS );
                            IDE_TEST( sdtWASortMap::setNextVersion( sMapHdr,
                                                                    j,
                                                                    &sPtr)
                                      != IDE_SUCCESS );

                        }
                    }
                }

                IDE_TEST( sdtWASortMap::incVersionIdx( sMapHdr )
                          != IDE_SUCCESS );
            }
        }
    }
    IDE_TEST(iduCheckSessionEvent(aHeader->mStatistics) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**************************************************************************
 * Description :
 * QuickSort를 수행한다.
 ***************************************************************************/
IDE_RC sdtSortModule::quickSort( smiTempTableHeader * aHeader,
                                 UInt                 aLeftPos,
                                 UInt                 aRightPos )
{
    smnSortStack         sCurStack;
    smnSortStack         sNewStack;
    idBool               sEmpty;
    SInt                 sLeft;
    SInt                 sRight;
    SInt                 sMid;
    sdtSortScanInfo      sPivotTRPInfo;
    sdtSortScanInfo      sTargetTRPInfo;
    SInt                 sResult = 0;
    idBool               sEqualSet = ID_TRUE;
    sdtSortSegHdr      * sWASeg = (sdtSortSegHdr*)aHeader->mWASegment;
    UInt                 sCompareCount = 0;

    sCurStack.mLeftPos  = aLeftPos;
    sCurStack.mRightPos = aRightPos;

    sPivotTRPInfo.mFetchEndOffset  = aHeader->mRowSize;
    sTargetTRPInfo.mFetchEndOffset = aHeader->mRowSize;

    IDE_TEST( aHeader->mSortStack.push( ID_FALSE, /*Lock*/
                                        &sCurStack ) != IDE_SUCCESS );

    sEmpty = ID_FALSE;
    while( 1 )
    {
        IDE_TEST(aHeader->mSortStack.pop(ID_FALSE, &sCurStack, &sEmpty)
                 != IDE_SUCCESS);

        // QuickSort의 알고리즘상, CallStack은 ItemCount보다 많아질 수 없다
        // 따라서 이보다 많으면, 무한루프에 빠졌을 가능성이 높다.
        IDE_ERROR( aRightPos*2 >= aHeader->mSortStack.getTotItemCnt() );

        if ( sEmpty == ID_TRUE)
        {
            break;
        }

        sLeft  = sCurStack.mLeftPos;
        sRight = sCurStack.mRightPos + 1;
        sMid   = (sCurStack.mLeftPos + sCurStack.mRightPos)/2;

        if ( sCurStack.mLeftPos < sCurStack.mRightPos )
        {
            IDE_TEST( sdtWASortMap::swapvULong( &sWASeg->mSortMapHdr,
                                                sMid,
                                                sLeft )
                      != IDE_SUCCESS );

            IDE_TEST( sdtWASortMap::getvULong( &sWASeg->mSortMapHdr,
                                               sLeft,
                                               (vULong*)&sPivotTRPInfo.mTRPHeader )
                      != IDE_SUCCESS );

            IDE_TEST( sdtTempRow::fetch( sWASeg,
                                         SDT_WAGROUPID_NONE,
                                         aHeader->mRowBuffer4Compare,
                                         &sPivotTRPInfo )
                      != IDE_SUCCESS );

            sEqualSet = ID_TRUE;

            while( 1 )
            {
                while( ( ++sLeft ) <= sCurStack.mRightPos )
                {
                    IDE_TEST( sdtWASortMap::getvULong( &sWASeg->mSortMapHdr,
                                                       sLeft,
                                                       (vULong*)&sTargetTRPInfo.mTRPHeader )
                              != IDE_SUCCESS );

                    IDE_TEST( sdtTempRow::fetch(
                                  sWASeg,
                                  SDT_WAGROUPID_NONE,
                                  aHeader->mRowBuffer4CompareSub,
                                  &sTargetTRPInfo )
                              != IDE_SUCCESS );

                    IDE_TEST( compare( aHeader,
                                       &sTargetTRPInfo,
                                       &sPivotTRPInfo,
                                       &sResult )
                              != IDE_SUCCESS );
                    sCompareCount++;
                    if ( sResult != 0 )
                    {
                        sEqualSet = ID_FALSE;
                    }

                    if ( sResult > 0 )
                    {
                        break;
                    }
                }

                while( ( --sRight ) > sCurStack.mLeftPos )
                {
                    IDE_TEST( sdtWASortMap::getvULong( &sWASeg->mSortMapHdr,
                                                       sRight,
                                                       (vULong*)&sTargetTRPInfo.mTRPHeader )
                              != IDE_SUCCESS );

                    IDE_TEST( sdtTempRow::fetch( sWASeg,
                                                 SDT_WAGROUPID_NONE,
                                                 aHeader->mRowBuffer4CompareSub,
                                                 &sTargetTRPInfo )
                              != IDE_SUCCESS );

                    IDE_TEST( compare( aHeader,
                                       &sTargetTRPInfo,
                                       &sPivotTRPInfo,
                                       &sResult )
                              != IDE_SUCCESS );
                    sCompareCount++;
                    if ( sResult != 0 )
                    {
                        sEqualSet = ID_FALSE;
                    }

                    if ( sResult < 0 )
                    {
                        break;
                    }
                }

                if ( sLeft < sRight )
                {
                    IDE_TEST( sdtWASortMap::swapvULong(
                                  &sWASeg->mSortMapHdr,sLeft,sRight)
                              != IDE_SUCCESS );
                }
                else
                {
                    break;
                }
            }

            /* Pivot을 복구함 */
            IDE_TEST( sdtWASortMap::swapvULong( &sWASeg->mSortMapHdr,
                                                sLeft - 1,
                                                sCurStack.mLeftPos )
                      != IDE_SUCCESS );

            if ( sEqualSet == ID_FALSE )
            {
                if ( sLeft > (sCurStack.mLeftPos + 2 ) )
                {
                    sNewStack.mLeftPos = sCurStack.mLeftPos;
                    sNewStack.mRightPos = sLeft - 2;

                    IDE_TEST(aHeader->mSortStack.push( ID_FALSE, &sNewStack)
                             != IDE_SUCCESS);
                }
                if ( sLeft < sCurStack.mRightPos )
                {
                    sNewStack.mLeftPos  = sLeft;
                    sNewStack.mRightPos = sCurStack.mRightPos;

                    IDE_TEST(aHeader->mSortStack.push(ID_FALSE, &sNewStack)
                             != IDE_SUCCESS);
                }
            }
        }
    }

    aHeader->mStatsPtr->mExtraStat1 += sCompareCount;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**************************************************************************
 * Description :
 * MergeSort를 수행한다.
 ***************************************************************************/
IDE_RC sdtSortModule::mergeSort( smiTempTableHeader * aHeader,
                                 SInt                 aLeftBeginPos,
                                 SInt                 aLeftEndPos,
                                 SInt                 aRightBeginPos,
                                 SInt                 aRightEndPos )
{
    sdtSortSegHdr      * sWASeg = (sdtSortSegHdr*)aHeader->mWASegment;
    sdtWASortMapHdr    * sMapHdr = &sWASeg->mSortMapHdr;
    SInt                 sLeft;
    SInt                 sRight;
    SInt                 sNext;
    vULong               sLeftPtr;
    vULong               sRightPtr;
    sdtSortScanInfo      sLeftTRPInfo;
    sdtSortScanInfo      sRightTRPInfo;
    SInt                 sResult = 0;
    UInt                 sCompareCount = 0;
    SInt                 i;

    /* Left/Right 둘은 맞닿아있어야 함 */
    IDE_ERROR( aLeftEndPos +1 == aRightBeginPos );

    sLeft   = aLeftBeginPos;
    sRight  = aRightBeginPos;
    sNext   = aLeftBeginPos;  /* Merge한 결과를 둘 NextVersion의 SlotIdx */

    IDE_TEST( sdtWASortMap::getvULong( sMapHdr,
                                       sLeft,
                                       (vULong*)&sLeftTRPInfo.mTRPHeader )
              != IDE_SUCCESS );
    IDE_TEST( sdtWASortMap::getvULong( sMapHdr,
                                       sRight,
                                       (vULong*)&sRightTRPInfo.mTRPHeader )
              != IDE_SUCCESS );

    sLeftTRPInfo.mFetchEndOffset  = aHeader->mRowSize;
    sRightTRPInfo.mFetchEndOffset = aHeader->mRowSize;

    IDE_TEST( sdtTempRow::fetch( sWASeg,
                                 SDT_WAGROUPID_NONE,
                                 aHeader->mRowBuffer4Compare,
                                 &sLeftTRPInfo )
              != IDE_SUCCESS );

    IDE_TEST( sdtTempRow::fetch( sWASeg,
                                 SDT_WAGROUPID_NONE,
                                 aHeader->mRowBuffer4CompareSub,
                                 &sRightTRPInfo )
              != IDE_SUCCESS );

    while( 1 )
    {
        IDE_TEST( compare( aHeader,
                           &sLeftTRPInfo,
                           &sRightTRPInfo,
                           &sResult )
                  != IDE_SUCCESS );
        sCompareCount++;
        if ( sResult > 0 )
        {
            IDE_TEST( sdtWASortMap::setNextVersion( sMapHdr,
                                                    sNext,
                                                    &sRightTRPInfo.mTRPHeader )
                      != IDE_SUCCESS );
            sNext++;
            sRight++;
            if ( sRight > aRightEndPos )
            {
                break;
            }

            IDE_TEST( sdtWASortMap::getvULong( sMapHdr,
                                               sRight,
                                               (vULong*)&sRightTRPInfo.mTRPHeader )
                      != IDE_SUCCESS );
            IDE_TEST( sdtTempRow::fetch( sWASeg,
                                         SDT_WAGROUPID_NONE,
                                         aHeader->mRowBuffer4CompareSub,
                                         &sRightTRPInfo )
                      != IDE_SUCCESS );

        }
        else
        {
            IDE_TEST( sdtWASortMap::setNextVersion( sMapHdr,
                                                    sNext,
                                                    &sLeftTRPInfo.mTRPHeader  )
                      != IDE_SUCCESS );
            sNext++;
            sLeft++;
            if ( sLeft > aLeftEndPos )
            {
                break;
            }

            IDE_TEST( sdtWASortMap::getvULong( sMapHdr,
                                               sLeft,
                                               (vULong*)&sLeftTRPInfo.mTRPHeader)
                      != IDE_SUCCESS );

            IDE_TEST( sdtTempRow::fetch( sWASeg,
                                         SDT_WAGROUPID_NONE,
                                         aHeader->mRowBuffer4Compare,
                                         &sLeftTRPInfo )
                      != IDE_SUCCESS );
        }
    }

    if ( sRight > aRightEndPos )
    {
        /* 오른쪽 Group이 모두 바닥난 경우로, 왼쪽의 남은 것들은 가장 큰 값들
         * 이 되게 된다. 따라서 이를 배치한 후, NextSlotArray에 정렬된
         * 값들을 가져오면 된다.
         *
         * 예) 다음과 같은 경우를 정렬하게 되면,
         *  Version0 : 3 4 5 6 7 8 1 2 3 4 5
         *                   ^              ^
         *                sLeft         sRight
         *  Version1 : 1 2 3 3 4 4 5 5 . . .
         *
         * 즉, sRight만 EndPos에 도달하고, Left는 못한다.
         * 따라서 sLeftPos뒤의 6,7,8을 NextVersion에 설정해주면 된다. */
        i = sNext;
        for( ; sLeft <= aLeftEndPos ; sLeft++, i++ )
        {
            IDE_TEST( sdtWASortMap::getvULong( sMapHdr, sLeft, &sLeftPtr )
                      != IDE_SUCCESS );
            IDE_TEST( sdtWASortMap::setNextVersion( sMapHdr,
                                                    i,
                                                    &sLeftPtr )
                      != IDE_SUCCESS );
        }
    }
    else
    {
        IDE_ERROR( sLeft > aLeftEndPos );
        /* 왼쪽 Group이 바닥난 경우로, 오른쪽의 남은 것들이 큰 경우이다. */

        i = sNext;
        for( ; sRight <= aRightEndPos ; sRight++, i++ )
        {
            IDE_TEST( sdtWASortMap::getvULong( sMapHdr, sRight, &sRightPtr )
                      != IDE_SUCCESS );
            IDE_ERROR( i == sRight );
            IDE_TEST( sdtWASortMap::setNextVersion( sMapHdr,
                                                    i,
                                                    &sRightPtr )
                      != IDE_SUCCESS );
        }
    }
    aHeader->mStatsPtr->mExtraStat1 += sCompareCount;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**************************************************************************
 * Description :
 * row를 얻어 compare한다.
 ***************************************************************************/
IDE_RC sdtSortModule::compareGRIDAndGRID(smiTempTableHeader * aHeader,
                                         sdtGroupID           aGroupID,
                                         scGRID               aSrcGRID,
                                         scGRID               aDstGRID,
                                         SInt               * aResult )
{
    sdtSortSegHdr     * sWASeg     = (sdtSortSegHdr*)aHeader->mWASegment;
    sdtSortScanInfo     sSrcTRPInfo;
    sdtSortScanInfo     sDstTRPInfo;

    IDE_TEST( sdtTempRow::fetchByGRID( sWASeg,
                                       aGroupID,
                                       aSrcGRID,
                                       aHeader->mRowSize,
                                       aHeader->mRowBuffer4Compare,
                                       &sSrcTRPInfo )
              != IDE_SUCCESS );

    IDE_TEST( sdtTempRow::fetchByGRID( sWASeg,
                                       aGroupID,
                                       aDstGRID,
                                       aHeader->mRowSize,
                                       aHeader->mRowBuffer4CompareSub,
                                       &sDstTRPInfo )
              != IDE_SUCCESS );

    IDE_TEST( compare( aHeader, &sSrcTRPInfo, &sDstTRPInfo, aResult )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    smuUtility::dumpFuncWithBuffer( IDE_DUMP_0,
                                    smuUtility::dumpGRID,
                                    &aSrcGRID );
    smuUtility::dumpFuncWithBuffer( IDE_DUMP_0,
                                    smuUtility::dumpGRID,
                                    &aDstGRID );

    return IDE_FAILURE;
}

/**************************************************************************
 * Description :
 * row를 얻어 compare한다.
 ***************************************************************************/
IDE_RC sdtSortModule::compare( smiTempTableHeader * aHeader,
                               sdtSortScanInfo    * aSrcTRPInfo,
                               sdtSortScanInfo    * aDstTRPInfo,
                               SInt               * aResult )
{
    smiTempColumn * sColumn;
    smiValueInfo    sValueInfo1;
    smiValueInfo    sValueInfo2;
    idBool          sIsNull1;
    idBool          sIsNull2;
    UInt            sColumnCount = 0;

    *aResult = 0;

    sValueInfo1.value  = aSrcTRPInfo->mValuePtr;
    sValueInfo1.flag   = SMI_OFFSET_USE;
    sValueInfo2.value  = aDstTRPInfo->mValuePtr;
    sValueInfo2.flag   = SMI_OFFSET_USE;

    sColumn = aHeader->mKeyColumnList;
    while( sColumn != NULL )
    {
        IDE_ERROR( ++sColumnCount <= aHeader->mColumnCount );

        sValueInfo1.column = &sColumn->mColumn;
        sValueInfo2.column = &sColumn->mColumn;

        /* PROJ-2435 order by nulls first/last */
        if ( ( sColumn->mColumn.flag & SMI_COLUMN_NULLS_ORDER_MASK )
             == SMI_COLUMN_NULLS_ORDER_NONE )
        {
            /* NULLS first/last 가 없을경우 기존동작 수행*/
            *aResult = sColumn->mCompare( &sValueInfo1,
                                          &sValueInfo2 );
        }
        else
        {
            IDE_DASSERT( ( sColumn->mColumn.flag & SMI_COLUMN_TYPE_MASK )
                         == SMI_COLUMN_TYPE_FIXED );
            /* 1. 2개의 value의 Null 여부를 조사한다. */
            sIsNull1 = sColumn->mIsNull( (const smiColumn *)&sColumn->mColumn,
                                         ((UChar*)sValueInfo1.value + sColumn->mColumn.offset) );
            sIsNull2 = sColumn->mIsNull( (const smiColumn *)&sColumn->mColumn,
                                         ((UChar*)sValueInfo2.value + sColumn->mColumn.offset) );

            if ( sIsNull1 == sIsNull2 )
            {
                if ( sIsNull1 == ID_FALSE  )
                {
                    /* 2. NULL이없을경우 기존동작 수행*/
                    *aResult = sColumn->mCompare( &sValueInfo1,
                                                  &sValueInfo2 );
                }
                else
                {
                    /* 3. 둘다 NULL 일 경우 0 */
                    *aResult = 0;
                }
            }
            else
            {
                if ( ( sColumn->mColumn.flag & SMI_COLUMN_NULLS_ORDER_MASK )
                     == SMI_COLUMN_NULLS_ORDER_FIRST )
                {
                    /* 4. NULLS FIRST 일경우 Null을 최소로 한다. */
                    if ( sIsNull1 == ID_TRUE )
                    {
                        *aResult = -1;
                    }
                    else
                    {
                        *aResult = 1;
                    }
                }
                else
                {
                    /* 5. NULLS LAST 일경우 Null을 최대로 한다. */
                    if ( sIsNull1 == ID_TRUE )
                    {
                        *aResult = 1;
                    }
                    else
                    {
                        *aResult = -1;
                    }
                }
            }
        }

        if ( *aResult != 0 )
        {
            break;
        }
        sColumn = sColumn->mNextKeyColumn;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**************************************************************************
 * Description :
 * 두 자식 Run을 비교한다.
 * 자식이 없으면, ID_UINT_MAX를 반환한다.
 *
 * <IN>
 * aHeader        - 대상 Table
 * aPos           - 부모 Slot ID
 * <OUT>
 * aChild         - 비교 결과 선택한 Child의 Slot ID
 ***************************************************************************/
IDE_RC sdtSortModule::findAndSetLoserSlot( smiTempTableHeader * aHeader,
                                           UInt                 aPos,
                                           UInt               * aChild )
{
    sdtTempMergeRunInfo sRunInfo;
    sdtTempMergeRunInfo sChildRunInfo[2];
    UInt                 sChildPos[2];
    scGRID               sChildGRID[2];
    UInt                 sSelectedChild = ID_UINT_MAX;
    UInt                 sSelectedChildPos = ID_UINT_MAX;
    SInt                 sResult = 0;
    sdtSortSegHdr      * sWASeg = (sdtSortSegHdr*)aHeader->mWASegment;
    UInt                 i;

    IDE_ERROR( ( aHeader->mTTState == SMI_TTSTATE_SORT_MERGE ) ||
               ( aHeader->mTTState == SMI_TTSTATE_SORT_MERGESCAN ) ||
               ( aHeader->mTTState == SMI_TTSTATE_SORT_MAKETREE ) );
    IDE_ERROR( aPos < sdtWASortMap::getSlotCount( &sWASeg->mSortMapHdr ) );

    sChildPos[0] = aPos * 2;
    sChildPos[1] = aPos * 2 + 1;

    if ( (aPos * 2) >= sdtWASortMap::getSlotCount( &sWASeg->mSortMapHdr ) )
    {
        /* ( sChildPos[ 0 ] >=
         *   sdtWASortMap::getSlotCount( &sWASeg->mSortMapHdr ) ) &&
         * ( sChildPos[ 1 ] >=
         *   sdtWASortMap::getSlotCount( &sWASeg->mSortMapHdr ) )  ) */
        /* 가져올 자식이 없음 */
    }
    else
    {
        /* 자식으로부터 선택하면 됨 */
        for( i = 0 ; i < 2 ; i++ )
        {
            if ( sChildPos[ i ] >=
                 sdtWASortMap::getSlotCount( &sWASeg->mSortMapHdr ) )
            {
                sChildRunInfo[i].mRunNo = SDT_TEMP_RUNINFO_NULL;
            }
            else
            {
                IDE_TEST( sdtWASortMap::get( &sWASeg->mSortMapHdr,
                                             sChildPos[i],
                                             (void*)&sChildRunInfo[i] )
                          != IDE_SUCCESS );
            }

            if ( sChildRunInfo[i].mRunNo != SDT_TEMP_RUNINFO_NULL )
            {
                SC_MAKE_GRID( sChildGRID[i],
                              SDT_SPACEID_WORKAREA,
                              getWPIDFromRunInfo( aHeader,
                                                  sChildRunInfo[i].mRunNo,
                                                  sChildRunInfo[i].mPIDSeq ),
                              sChildRunInfo[i].mSlotNo );
            }
        }

        if ( ( sChildRunInfo[0].mRunNo != SDT_TEMP_RUNINFO_NULL ) &&
             ( sChildRunInfo[1].mRunNo != SDT_TEMP_RUNINFO_NULL ) )
        {
            /* Both values are not null, ( not null ). */
            /* 항상 InMemory연산이어야 하기에, None Group */
            IDE_TEST( compareGRIDAndGRID( aHeader,
                                          SDT_WAGROUPID_NONE,
                                          sChildGRID[0],
                                          sChildGRID[1],
                                          &sResult )
                      != IDE_SUCCESS );

            if ( sResult <= 0 )
            {
                /* 왼쪽이 작거나 둘이 같으면 */
                sSelectedChild = 0;
            }
            else
            {
                sSelectedChild = 1;
            }
        }
        else
        {
            if ( ( sChildRunInfo[0].mRunNo == SDT_TEMP_RUNINFO_NULL ) &&
                 ( sChildRunInfo[1].mRunNo != SDT_TEMP_RUNINFO_NULL ) )
            {
                /* 1만 유효, 0은 유효하지 않음 */
                sSelectedChild = 1;
            }
            else
            {
                if ( ( sChildRunInfo[0].mRunNo!=SDT_TEMP_RUNINFO_NULL ) &&
                     ( sChildRunInfo[1].mRunNo==SDT_TEMP_RUNINFO_NULL ) )
                {
                    sSelectedChild = 0;
                }
                else
                {
                    /* 양쪽다 Null이니 뭘 올려도 상관 없음 */
                    IDE_ERROR( ( sChildRunInfo[0].mRunNo ==
                                 SDT_TEMP_RUNINFO_NULL ) &&
                               ( sChildRunInfo[1].mRunNo ==
                                 SDT_TEMP_RUNINFO_NULL ) );
                }
            }
        }
    }

    if ( sSelectedChild != ID_UINT_MAX )
    {
        /* Child로 선택될 만한 Slot이 있음 */
        IDE_TEST( sdtWASortMap::set( &sWASeg->mSortMapHdr,
                                     aPos,
                                     (void*)&sChildRunInfo[ sSelectedChild ] )
                  != IDE_SUCCESS );
        sSelectedChildPos = sChildPos[ sSelectedChild ];
    }
    else
    {
        /* 가져올 자식이 없으니, 자신이 Source Slot이 되어, Run의 다음
         * 페이지를 가져옴 */
        IDE_TEST( sdtWASortMap::get( &sWASeg->mSortMapHdr,
                                     aPos,
                                     (void*)&sRunInfo )
                  != IDE_SUCCESS );
        IDE_TEST( readNextRowByRun( aHeader,
                                    &sRunInfo )
                  != IDE_SUCCESS );
        IDE_TEST( sdtWASortMap::set( &sWASeg->mSortMapHdr,
                                     aPos,
                                     (void*)&sRunInfo )
                  != IDE_SUCCESS );

    }

    if ( aChild != NULL )
    {
        ( * aChild ) = sSelectedChildPos;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**************************************************************************
 * Description :
 * SortGroup의 정렬된 Key(또는 Row)들을 Run의 형태로 저장한다
 *
 * <IN>
 * aHeader        - 대상 Table
 ***************************************************************************/
IDE_RC sdtSortModule::storeSortedRun( smiTempTableHeader * aHeader )
{
    scPageID              sHeadNPID = SD_NULL_PID;
    UChar               * sSrcPtr;
    sdtSortSegHdr       * sWASeg = (sdtSortSegHdr*)aHeader->mWASegment;
    sdtSortInsertResult sTRInsertResult;
    UInt                  sSlotCount;
    UInt                  i;

    sSlotCount = sdtWASortMap::getSlotCount( &sWASeg->mSortMapHdr );

    if ( sSlotCount > 0 )
    {
        /* FlushGroup을 초기화하지 않으면, Run끼리 섞인다. */
        IDE_TEST( sdtSortSegment::resetWAGroup( sWASeg,
                                                SDT_WAGROUPID_FLUSH,
                                                ID_TRUE/*wait4flsuh*/ )
                  != IDE_SUCCESS );

        IDE_ERROR( ( aHeader->mTTState == SMI_TTSTATE_SORT_INSERTNSORT ) ||
                   ( aHeader->mTTState == SMI_TTSTATE_SORT_INSERTONLY ) ||
                   ( aHeader->mTTState == SMI_TTSTATE_SORT_EXTRACTNSORT ) ||
                   ( aHeader->mTTState == SMI_TTSTATE_SORT_SCAN ) );

        /* SortGroup의 Row들을 FlushGroup으로 정렬순 대로 복사함. */
        for( i = 0 ;
             i < sSlotCount ;
             i++ )
        {
            IDE_TEST( sdtWASortMap::getvULong( &sWASeg->mSortMapHdr,
                                               i,
                                               (vULong*)&sSrcPtr )
                      != IDE_SUCCESS );

            IDE_TEST( copyRowByPtr( aHeader,
                                    sSrcPtr,
                                    SDT_COPY_NORMAL,
                                    SDT_TEMP_PAGETYPE_SORTEDRUN,
                                    SC_NULL_GRID, /* ChildRID */
                                    &sTRInsertResult )
                      != IDE_SUCCESS );

            if ( sHeadNPID == SD_NULL_PID )
            {
                sHeadNPID = SC_MAKE_PID( sTRInsertResult.mHeadRowpieceGRID );
            }
            aHeader->mMaxRowPageCount = IDL_MAX( aHeader->mMaxRowPageCount,
                                                 sTRInsertResult.mRowPageCount );
        }

        IDE_ERROR( sHeadNPID != SD_NULL_PID );
        IDE_ERROR( aHeader->mSortGroupID == SDT_WAGROUPID_SORT );

        IDE_TEST( aHeader->mRunQueue.enqueue(ID_FALSE, (void*)&sHeadNPID )
                  != IDE_SUCCESS );

        /* Run으로 만들어 전부 Flush했으니, SortGroup을 초기화함 */
        IDE_TEST( sdtSortSegment::resetWAGroup( sWASeg,
                                                SDT_WAGROUPID_SORT,
                                                ID_FALSE/*wait4flsuh*/ )
                  != IDE_SUCCESS );

        /* WAMap을 초기화 */
        IDE_TEST( sdtWASortMap::create( sWASeg,
                                        SDT_WAGROUPID_SORT,
                                        SDT_WM_TYPE_POINTER,
                                        0, /* Slot Count */
                                        2, /* aVersionCount */
                                        &sWASeg->mSortMapHdr )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**************************************************************************
 * Description :
 * Row를 다른 Group으로 복사한다.
 * 복사 목적 (aPurpose)에 따라, 원본/사본 Group, Row처리등이 달라진다.
 *
 * <IN>
 * aHeader         - 대상 Table
 * aSrcGRID        - 복사할 원본 Row
 * aPurpose        - 복사하는 목적
 * aPageType       - 복사한 대상 Page의 Type
 * aChildGRID      - 치환할 ChildGRID(원본 Row의 다른 부분은 두고, ChildGRID만
 *                   변경함 )
 * aTRInsertResult - 복사한 결과.
 ***************************************************************************/
IDE_RC sdtSortModule::copyRowByGRID( smiTempTableHeader * aHeader,
                                     scGRID               aSrcGRID,
                                     sdtCopyPurpose       aPurpose,
                                     sdtTempPageType      aPageType,
                                     scGRID               aChildGRID,
                                     sdtSortInsertResult  * aTRInsertResult )
{
    sdtGroupID     sSrcGroupID  = SDT_WAGROUPID_SORT;
    sdtSortSegHdr* sWASeg       = (sdtSortSegHdr*)aHeader->mWASegment;
    UChar        * sCursor      = NULL;
    sdtWCB       * sWCBPtr;
    idBool         sIsFixedPage = ID_FALSE;

    /* 어느 그룹에서 어느 그룹으로의 이동인가  */
    switch( aPurpose )
    {
        case SDT_COPY_NORMAL:
        case SDT_COPY_MAKE_KEY:
        case SDT_COPY_MAKE_LNODE:
            sSrcGroupID   = SDT_WAGROUPID_SORT;
            break;
        case SDT_COPY_MAKE_INODE:
            sSrcGroupID   = SDT_WAGROUPID_LNODE;
            break;
        case SDT_COPY_EXTRACT_ROW:
            sSrcGroupID  = SDT_WAGROUPID_SUBFLUSH;
            break;
        default:
            break;
    }

    IDE_TEST( sdtSortSegment::getPagePtrByGRID( sWASeg,
                                                sSrcGroupID,
                                                aSrcGRID,
                                                &sCursor )
              != IDE_SUCCESS );

    if ( sSrcGroupID != SDT_WAGROUPID_SORT )
    {
        IDE_DASSERT( ( SC_MAKE_SPACE( aSrcGRID ) != SDT_SPACEID_WAMAP ) &&
                     ( SC_MAKE_SPACE( aSrcGRID ) != SDT_SPACEID_WORKAREA ) );

        sWCBPtr = sdtSortSegment::findWCB( sWASeg, SC_MAKE_PID( aSrcGRID ) );

        IDE_DASSERT( sWCBPtr != NULL ); // 방금 get page한 놈이 NULL일리 없다.

        sdtSortSegment::fixWAPage( sWCBPtr );
        sIsFixedPage = ID_TRUE;
    }

    IDE_TEST( copyRowByPtr( aHeader,
                            sCursor,
                            aPurpose,
                            aPageType,
                            aChildGRID,
                            aTRInsertResult )
              != IDE_SUCCESS );

    if ( sIsFixedPage == ID_TRUE )
    {
        sIsFixedPage = ID_FALSE;
        sdtSortSegment::unfixWAPage( sWCBPtr );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sIsFixedPage == ID_TRUE )
    {
        sdtSortSegment::unfixWAPage( sWCBPtr );
    }
    return IDE_FAILURE;
}

/**************************************************************************
 * Description :
 * Row를 다른 Group으로 복사한다.
 * 복사 목적 (aPurpose)에 따라, 원본/사본 Group, Row처리등이 달라진다.
 *
 * <IN>
 * aHeader         - 대상 Table
 * aSrcPtr         - 복사할 원본 Row
 * aPurpose        - 복사하는 목적
 * aPageType       - 복사한 대상 Page의 Type
 * aChildGRID      - 치환할 ChildGRID(원본 Row의 다른 부분은 두고, ChildGRID만
 *                   변경함 )
 * aTRInsertResult - 복사한 결과.
 ***************************************************************************/
IDE_RC sdtSortModule::copyRowByPtr(smiTempTableHeader * aHeader,
                                   UChar              * aSrcPtr,
                                   sdtCopyPurpose      aPurpose,
                                   sdtTempPageType      aPageType,
                                   scGRID               aChildGRID,
                                   sdtSortInsertResult  * aTRInsertResult )
{
    sdtSortScanInfo     sScanInfo4Select;
    sdtSortInsertInfo   sScanInfo4Insert;
    smiValue            sValueList[ SMI_COLUMN_ID_MAXIMUM ];
    sdtSortTRPHdr     * sTRPHeader;
    sdtGroupID          sSrcGroupID  = SDT_WAGROUPID_SORT;
    sdtGroupID          sDestGroupID = SDT_WAGROUPID_FLUSH;
    sdtSortSegHdr     * sWASeg = (sdtSortSegHdr*)aHeader->mWASegment;
    UInt                i;

    /* 어느 그룹에서 어느 그룹으로의 이동인가  */
    switch( aPurpose )
    {
        case SDT_COPY_NORMAL:
            sSrcGroupID   = SDT_WAGROUPID_SORT;
            sDestGroupID  = SDT_WAGROUPID_FLUSH;
            break;
        case SDT_COPY_MAKE_LNODE:
            sSrcGroupID   = SDT_WAGROUPID_SORT;
            sDestGroupID  = SDT_WAGROUPID_FLUSH;
            break;
        case SDT_COPY_MAKE_INODE:
            sSrcGroupID   = SDT_WAGROUPID_LNODE;
            sDestGroupID  = SDT_WAGROUPID_INODE;
            break;
        case SDT_COPY_EXTRACT_ROW:
            sSrcGroupID  = SDT_WAGROUPID_SUBFLUSH;
            sDestGroupID = SDT_WAGROUPID_SORT;
            break;
        default:
            break;
    }

    sScanInfo4Select.mTRPHeader      = (sdtSortTRPHdr*)aSrcPtr;
    sScanInfo4Select.mFetchEndOffset = aHeader->mRowSize;

    /*************** SortGroup에서 해당 Row를 가져옴 ********************/
    IDE_TEST( sdtTempRow::fetch( sWASeg,
                                 sSrcGroupID,
                                 aHeader->mRowBuffer4Fetch,
                                 &sScanInfo4Select )
              != IDE_SUCCESS );

    /**************** TRPInfo4select로 TRPInfo4Insert만듬 ****************/

    /* TRPHeader를 복제 후 Flag를 조작한다. */
    idlOS::memcpy( &sScanInfo4Insert.mTRPHeader,
                   sScanInfo4Select.mTRPHeader,
                   ID_SIZEOF( sdtSortTRPHdr) );
    sTRPHeader = &sScanInfo4Insert.mTRPHeader;
    /* 하나로 합쳐서 읽기 때문에, Next는 때어줘야 한다. */
    SM_SET_FLAG_OFF( sTRPHeader->mTRFlag, SDT_TRFLAG_NEXTGRID );
    SC_MAKE_NULL_GRID( sTRPHeader->mNextGRID );
    sScanInfo4Insert.mColumnCount  = aHeader->mColumnCount;
    sScanInfo4Insert.mColumns      = aHeader->mColumns;
    sScanInfo4Insert.mValueLength  = aHeader->mRowSize;
    sScanInfo4Insert.mValueList    = sValueList;
    for( i = 0; i < aHeader->mColumnCount; i++ )
    {
        sScanInfo4Insert.mValueList[ i ].length =
            aHeader->mColumns[ i ].mColumn.size;
        sScanInfo4Insert.mValueList[ i ].value  =
            sScanInfo4Select.mValuePtr +
            aHeader->mColumns[ i ].mColumn.offset;
    }
    if ( SC_GRID_IS_NOT_NULL( aChildGRID ) )
    {
        SC_COPY_GRID( aChildGRID, sTRPHeader->mChildGRID ) ;
        SM_SET_FLAG_ON( sTRPHeader->mTRFlag, SDT_TRFLAG_CHILDGRID );
    }

    /*************************** 전처리 과정 ****************************/
    switch( aPurpose )
    {
        case SDT_COPY_NORMAL:
        case SDT_COPY_EXTRACT_ROW:
            /* LeafKey에서 추출하는 것이기에, Unsplit이 있다.
             * 이를 빼줘야 알아서 쪼개서 저장한다. */
            SM_SET_FLAG_OFF( sTRPHeader->mTRFlag, SDT_TRFLAG_UNSPLIT );
            break;
        case SDT_COPY_MAKE_KEY:
            /* Key이기 때문에 ChildGRID가 있어야 한다. */
            IDE_ERROR( SC_GRID_IS_NOT_NULL( aChildGRID ) );
            break;
        case SDT_COPY_MAKE_LNODE:
            /* BUG-48226 appendRowPiece()에서 header size 이하의 작은 slot이 할당되는 문제가 발생
             * header size는 제외하고 value size가 property 설정 크기보다 커야한다. */
            if ( sScanInfo4Insert.mValueLength > smuProperty::getTempMaxKeySize() )
            {
                /* 키가 너무 크니 ExtraPage에 여분을 넣고 나머지만 삽입 */
                IDE_TEST( copyExtraRow( aHeader, &sScanInfo4Insert )
                          != IDE_SUCCESS );
            }
            SM_SET_FLAG_ON( sTRPHeader->mTRFlag, SDT_TRFLAG_UNSPLIT );
            IDE_ERROR( SC_GRID_IS_NULL( aChildGRID ) );
            break;
        case SDT_COPY_MAKE_INODE:
            /* INode를 생성할때, LNode와 같은 ExtraRow를 갖으면 된다.
             * 따라서 NextGRID를 가져오고, ValueLength를 LNode의 FirstRowPiece만큼
             * 축소시킨다. */
            if ( SM_IS_FLAG_ON( sScanInfo4Select.mTRPHeader->mTRFlag,
                                SDT_TRFLAG_NEXTGRID ) )
            {
                SC_COPY_GRID( sScanInfo4Select.mTRPHeader->mNextGRID, sTRPHeader->mNextGRID );
                sScanInfo4Insert.mValueLength =
                    sScanInfo4Select.mTRPHeader->mValueLength;
                SM_SET_FLAG_ON( sTRPHeader->mTRFlag, SDT_TRFLAG_NEXTGRID );
            }
            SM_SET_FLAG_ON( sTRPHeader->mTRFlag, SDT_TRFLAG_UNSPLIT );
            break;
        default:
            IDE_ERROR( 0 );
            break;
    }

    /******************************* 복사 ********************************/
    IDE_TEST( sdtTempRow::append( sWASeg,
                                  sDestGroupID,
                                  aPageType,
                                  0, /* CuttingOffset */
                                  &sScanInfo4Insert,
                                  aTRInsertResult )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**************************************************************************
 * Description :
 * Row를 Key로 만들기 위해, Row의 뒤쪽 부분을 ExtraGroup으로 복사한다.
 *
 * <IN>
 * aHeader         - 대상 Table
 * aTRInsertResult - 복사한 결과.
 ***************************************************************************/
IDE_RC sdtSortModule::copyExtraRow( smiTempTableHeader * aHeader,
                                    sdtSortInsertInfo  * aTRPInfo )
{
    sdtSortInsertResult  sTRInsertResult;
    IDE_TEST( sdtTempRow::append( (sdtSortSegHdr*)aHeader->mWASegment,
                                  SDT_WAGROUPID_SUBFLUSH,
                                  SDT_TEMP_PAGETYPE_INDEX_EXTRA,
                                  smuProperty::getTempMaxKeySize(),
                                  aTRPInfo,
                                  &sTRInsertResult )
              != IDE_SUCCESS );
    IDE_ERROR( sTRInsertResult.mComplete == ID_TRUE );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/**************************************************************************
 * Description :
 * run들을 읽어 Heap을 구축한다.
 * ( mergeScan도중 restoreCursor 에서도 사용한다.)
 *
 * - 구현
 * +-+-+-+-+-+-+-+---+-------+-------+-------+-------+
 * |1|1|2|1|2|3|4|   |   4   |   3   |   2   |   1   |
 * +-+-+-+-+-+-+-+---+-------+-------+-------+-------+
 * <----Slot----->   <-------------Run--------------->
 *
 * Heap은 위와같이 구성된다.
 *
 * - Slot ( sdtTempMergeRunInfo )
 *   Slot은 Run들간 대소관계를 가지고 있는 Array로 WaMap으로 표현된다. 위
 *   예제에서 Slot들은 논리적으로 다음과 같은 관계를 가진다.
 *      1
 *    1   2
 *   1 2 3 4
 *
 * - Run
 *   Run은 mMaxRowPageCount만큼의 Page개수로 구성된다.
 ***************************************************************************/
IDE_RC sdtSortModule::heapInit(smiTempTableHeader * aHeader)
{
    UInt                   sLeftPos;
    UInt                   sRunNo;
    scPageID               sNextNPID;
    scPageID               sPageSeq;
    idBool                 sIsEmpty;
    UInt                   sCurLimitRunNo;
    sdtTempMergeRunInfo   sRunInfo;
    sdtSortSegHdr        * sWASeg = (sdtSortSegHdr*)aHeader->mWASegment;
    UInt                   i;

    aHeader->mMergeRunCount = calcMaxMergeRunCount(
        aHeader, aHeader->mMaxRowPageCount );
    IDE_TEST_RAISE( aHeader->mMergeRunCount <= 1, error_invalid_sortareasize );

    /* Queue길이보다 많이 Merge할 필요는 없음 */
    aHeader->mMergeRunCount = IDL_MIN( aHeader->mMergeRunCount,
                                       aHeader->mRunQueue.getQueueLength() );

    /* HeapMap을 구성함.  */
    IDE_TEST( sdtWASortMap::create( sWASeg,
                                    SDT_WAGROUPID_SORT,
                                    SDT_WM_TYPE_RUNINFO,
                                    aHeader->mMergeRunCount*3 + 2 , /*count*/
                                    1, /* aVersionCount */
                                    &sWASeg->mSortMapHdr )
              != IDE_SUCCESS );

    /* Slot들을 일단 NULL RunNo로 초기화시킴 */
    for( i = 0 ; i < aHeader->mMergeRunCount*3 + 2; i++ )
    {
        sRunInfo.mRunNo  = SDT_TEMP_RUNINFO_NULL;
        sRunInfo.mPIDSeq = 0;
        sRunInfo.mSlotNo = 0;

        IDE_TEST( sdtWASortMap::set( &sWASeg->mSortMapHdr,
                                     i,
                                     (void*)&sRunInfo )
                  != IDE_SUCCESS );
    }

    if ( aHeader->mMergeRunCount == 0 )
    {
        aHeader->mMergeRunSize = 1;
    }
    else
    {
        /* Run 하나의 크기를 계산함 */
        aHeader->mMergeRunSize =
            sdtSortSegment::getAllocableWAGroupPageCount( sWASeg,
                                                          SDT_WAGROUPID_SORT )
            / aHeader->mMergeRunCount;

        /* 가장 왼쪽의 Run이  가장 오른쪽의 MapSlot과 만나면 안됨 */
        IDE_ERROR( getWPIDFromRunInfo( aHeader,
                                       aHeader->mMergeRunCount - 1,
                                       aHeader->mMergeRunSize - 1 )
                   > sdtWASortMap::getEndWPID( &sWASeg->mSortMapHdr ) -1 );

        /* LeftPos는 동일한 높이의 한 Line에서 가장 왼쪽 Slot의 위치이다.
         * 예)        1
         *      2           3
         *   4     5     6     7
         *  8  9 10 11 12 13 14 15
         * 여기서 LetPos가 될 수 있는 값이 1,2,4,8이다.
         * 그리고 이 값은 동시에 해당 Line의 크기이다. 위에서 세번째 4에는
         * 4,5,6,7 네개의 Slot이 있고, 네번째 8에는 8개의 Slot이 있다.
         * 따라서 sLeftPos가 MergeRunCount를 넘는 순간이 바닥이다.*/
        for( sLeftPos = 1 ;
             sLeftPos < aHeader->mMergeRunCount;
             sLeftPos *= 2 )
        {
            /* nothing to do!!! */
        }
        aHeader->mLeftBottomPos = sLeftPos;

        /* 가장 밑바닥에 대한 설정을 해줌 */
        sCurLimitRunNo = IDL_MIN( sLeftPos, aHeader->mMergeRunCount );
        for( sRunNo = 0 ; sRunNo < sCurLimitRunNo ; sRunNo++ )
        {
            IDE_TEST( aHeader->mRunQueue.dequeue( ID_FALSE, /* a_bLock */
                                                  (void*)&sNextNPID,
                                                  &sIsEmpty )
                      != IDE_SUCCESS );
            IDE_ERROR( sIsEmpty == ID_FALSE );

            /* Run의 내용을 Page에 Loading */
            sPageSeq = aHeader->mMergeRunSize;
            while( sPageSeq-- )
            {
                IDE_TEST( readRunPage( aHeader,
                                       sRunNo,
                                       sPageSeq,
                                       &sNextNPID,
                                       ID_FALSE ) /*aReadNextNPID*/
                          != IDE_SUCCESS );

                if ( sNextNPID == SD_NULL_PID )
                {
                    /* 해당 run 에 다음 Row가 없음 */
                    break;
                }
                else
                {
                    /* nothing to do */
                }
            }

            /* Bottom에 Run정보 설정 */
            sRunInfo.mRunNo  = sRunNo;
            sRunInfo.mPIDSeq = aHeader->mMergeRunSize - 1;
            sRunInfo.mSlotNo = 0;
            IDE_TEST( sdtWASortMap::set( &sWASeg->mSortMapHdr,
                                         aHeader->mLeftBottomPos + sRunNo,
                                         (void*)&sRunInfo )
                      != IDE_SUCCESS );
        }

        IDE_TEST( buildLooserTree( aHeader ) != IDE_SUCCESS );
    }

    /* Merge한단계 할때마다 IOPass가 증가함 */
    aHeader->mStatsPtr->mIOPassNo++;

    return IDE_SUCCESS;

    IDE_EXCEPTION( error_invalid_sortareasize);
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_INVALID_SORTAREASIZE ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**************************************************************************
 * Description :
 * Run을 Child부터 읽어 비교하며, LooserTree를 구성한다
 ***************************************************************************/
IDE_RC sdtSortModule::buildLooserTree(smiTempTableHeader * aHeader)
{
    UInt                   sLeftPos;
    UInt                   sRunNo;

    IDE_ERROR( aHeader->mMergeRunSize > 0 );

    sLeftPos = aHeader->mLeftBottomPos / 2;

    /* 아래에서부터 올라오면서 설정해줌. */
    while( sLeftPos > 0 )
    {
        for( sRunNo = 0 ;
             sRunNo < sLeftPos ;
             sRunNo++ )
        {
            IDE_TEST( findAndSetLoserSlot( aHeader,
                                           sLeftPos + sRunNo,
                                           NULL /*ChildIdx*/ )
                      != IDE_SUCCESS );
        }
        sLeftPos /= 2;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**************************************************************************
 * Description :
 * heap에서 최상단의 Row, 즉 LooserRow를 제거한다.
 ***************************************************************************/
IDE_RC sdtSortModule::heapPop(smiTempTableHeader * aHeader )
{
    sdtSortSegHdr        * sWASeg = (sdtSortSegHdr*)aHeader->mWASegment;
    sdtTempMergeRunInfo   sTopRunInfo;
    UInt                   sPos;

    /* Heap의 Top Slot을 뽑아냄 */
    IDE_TEST( sdtWASortMap::get( &sWASeg->mSortMapHdr,
                                 1,  /* aIdx */
                                 (void*)&sTopRunInfo )
              != IDE_SUCCESS );

    IDE_ERROR( sTopRunInfo.mRunNo != SDT_TEMP_RUNINFO_NULL );

    /* 아래에서부터 Top으로 가장 작은 Row를 찾아 올린다. */
    sPos = aHeader->mLeftBottomPos + sTopRunInfo.mRunNo;
    do
    {
        IDE_TEST( findAndSetLoserSlot( aHeader,
                                       sPos,
                                       NULL )
                  != IDE_SUCCESS );
        sPos /= 2;
    }
    while( sPos > 0 );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**************************************************************************
 * Description :
 * Run에서 다음 Row를 읽는다.
 ***************************************************************************/
IDE_RC sdtSortModule::readNextRowByRun( smiTempTableHeader   * aHeader,
                                        sdtTempMergeRunInfo * aRun )
{
    sdtSortSegHdr        * sWASeg = (sdtSortSegHdr*)aHeader->mWASegment;
    sdtSortTRPHdr        * sTRPHeader;
    scGRID                 sGRID;
    scPageID               sNPID;
    scPageID               sWPID;
    scPageID               sReadPIDCount = 0;
    sdtTempMergeRunInfo   sCurRunInfo;
    UChar                * sPtr = NULL;
    idBool                 sIsValidSlot = ID_FALSE;
    UChar                * sWAPagePtr = NULL;
    UInt                   sSlotCount;
    /* Head
     * +---+    +---+
     * | A |    | C |
     * +---+    +---+
     * | B | -> |   |
     * +---+    +   |
     * | C`|    |   |
     * +---+    +---+
     * (`는 Chain된 NextPage임을 의미. 즉 C -> C` 이렇게 되고 C가 First)
     *
     * A부터 탐색을 시작하며, NextSlot을 향할때 찾아야할 다음 Row의 꼬리
     * 부터 찾아가게 된다. 따라서 FirstRow를 찾는 순간 탐색을 종료하면
     * 된다. */
    sCurRunInfo = *aRun;

    /* FirstRowPiece를 찾을때까지, 즉 다음 Row를 읽을때까지 읽는다. */
    while ( 1 )
    {
        sCurRunInfo.mSlotNo++;
        if ( sCurRunInfo.mRunNo == SDT_TEMP_RUNINFO_NULL )
        {
            break;
            /* 다 뽑아냄 */
        }

        getGRIDFromRunInfo( aHeader, &sCurRunInfo, &sGRID );

        sWAPagePtr = NULL;
        sWPID      = SC_NULL_PID;

        IDE_TEST( sdtSortSegment::getPage( (sdtSortSegHdr*)
                                           aHeader->mWASegment,
                                           SDT_WAGROUPID_NONE,
                                           sGRID,
                                           &sWPID,
                                           &sWAPagePtr )
                  != IDE_SUCCESS );

        sSlotCount = sdtTempPage::getSlotCount(sWAPagePtr);

        sdtSortSegment::getSlot( sWAPagePtr,
                                 sSlotCount,
                                 SC_MAKE_OFFSET( sGRID ),
                                 &sPtr,
                                 &sIsValidSlot );

        if ( sIsValidSlot == ID_FALSE )
        {
            /* Page내 Slot을 전부 순회함. 다음 페이지를 가져옴. */
            sNPID = sdtTempPage::getNextPID( sPtr );

            /* 나중에 Free하라고 표시해둠. 바로 표시 했다가는,
             * 위 그림에서 Row C를 읽을때 C`를 가진 페이지가 없을 수 있음*/
            sWPID = getWPIDFromRunInfo( aHeader,
                                        sCurRunInfo.mRunNo,
                                        sCurRunInfo.mPIDSeq );
            sdtSortSegment::bookedFree( sWASeg, sWPID );

            if ( sNPID == SD_NULL_PID )
            {
                /*다음 Row를 발견하지 못하던 상황에서, 종료됨 */
                aRun->mRunNo = SDT_TEMP_RUNINFO_NULL;
                break;
            }
            else
            {
                /* 무한 Loop를 도는건 아닌지 검사한다.
                 * UShortMax의 크기면, 65536*8192 = 512MB로 이정도 크기의
                 * Row는 없다. */
                sReadPIDCount++;
                IDE_ERROR( sReadPIDCount < ID_USHORT_MAX );

                /* 다음 페이지로 */
                sCurRunInfo.mPIDSeq++;
                sCurRunInfo.mSlotNo = -1;

                IDE_TEST( readRunPage( aHeader,
                                       sCurRunInfo.mRunNo,
                                       sCurRunInfo.mPIDSeq,
                                       &sNPID,
                                       ID_TRUE ) /*aReadNextNPID*/
                          != IDE_SUCCESS );
                continue;
            }
        }
        else
        {
            sTRPHeader = (sdtSortTRPHdr*)sPtr;
            if ( SM_IS_FLAG_ON( sTRPHeader->mTRFlag , SDT_TRFLAG_HEAD ) )
            {
                *aRun = sCurRunInfo;
                break;
            }
        } /*if */
    } /* while */

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**************************************************************************
 * Description :
 * Run에서 다음 Row를 읽기 위해, Run의 다음 Page를 유효한
 * 장소에 올린다.
 *
 * <IN>
 * aHeader        - 대상 Table
 * aRunNo         - 대상 Run의 번호
 * aPageSeq       - Run내에서 몇번째 Page인가.
 * <IN/OUT>
 * aNextPID       - 읽을 다음 PID. 그리고 다시 다음 PID를 반환함
 * aReadNextNPID  - NextPID를 읽어야 하나? (False면 PrevPID를 읽음 )
 ***************************************************************************/
IDE_RC sdtSortModule::readRunPage( smiTempTableHeader   * aHeader,
                                   UInt                   aRunNo,
                                   UInt                   aPageSeq,
                                   scPageID             * aNextPID,
                                   idBool                 aReadNextNPID )
{
    UChar        * sWAPagePtr;
    scPageID       sRunNPID;
    scPageID       sWPID;
    sdtSortSegHdr* sWASeg = (sdtSortSegHdr*)aHeader->mWASegment;
    sdtWCB       * sWCBPtr;
    sdtWCB       * sOldWCB;

    sRunNPID = *aNextPID;
    if ( sRunNPID != SD_NULL_PID )
    {
        sWPID = getWPIDFromRunInfo( aHeader, aRunNo, aPageSeq );

        /* 이미 해당 페이지가 Loading 돼어 있는 경우는 제외한다.
         * 이전에 읽은 Page의 Row가 Chaining돼어 있으면, 이미 같은 페이지가
         * 읽어져 있었을 수도 있다. */
        sWCBPtr = sdtSortSegment::getWCBWithLnk( sWASeg, sWPID );
        sOldWCB = sdtSortSegment::findWCB( sWASeg, sRunNPID );

        if ( sWCBPtr != sOldWCB )
        {
            // sOldWCB가 null이거나,
            // sWCBPtr->mNPage != sRunNPID 이거나
            if ( sdtSortSegment::getNPID( sWCBPtr ) != SC_NULL_PID )
            {
                IDE_TEST( sdtSortSegment::makeInitPage( sWASeg,
                                                        sWCBPtr,
                                                        ID_TRUE ) /*Flush */
                          != IDE_SUCCESS );
            }
            else
            {
                IDE_TEST( sdtSortSegment::chkAndSetWAPagePtr( sWASeg,
                                                              sWCBPtr ) != IDE_SUCCESS );
            }

            IDE_TEST( sdtSortSegment::readNPage( sWASeg,
                                                 SDT_WAGROUPID_SORT,
                                                 sWCBPtr,
                                                 sRunNPID )
                      != IDE_SUCCESS );
        }

        sWAPagePtr = sdtSortSegment::getWAPagePtr( sWCBPtr );

        // BUG-46383 Disk Sort Temp에서 moveWAPage가 skip되는 경우가 있습니다.
        // 즉 readNPage에서 NPID를 WPID 에 옮놓지 못하는 경우가 있음.
        // 제대로 읽어 왔는지 확인해야 함.
        IDE_ERROR( sdtTempPage::getSelfPID( sWAPagePtr ) == sRunNPID );

        /* 다음에 읽을 Page를 읽은다. 다만 Page에게 있어,
         * NextPID 링크를 따라갈지, PrevPID 링크를 따라갈지는
         * 다음 Boolean에 따라 결정된다. */
        if ( aReadNextNPID == ID_TRUE )
        {
            *aNextPID = sdtTempPage::getNextPID( sWAPagePtr );
        }
        else
        {
            *aNextPID = sdtTempPage::getPrevPID( sWAPagePtr );
        }
    }


    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/**************************************************************************
 * Description :
 * MergeCursor를 만든다.
 *
 * +------+------+------+------+------+
 * | Size |0 PID |0 Slot|1 PID |1 Slot|
 * +------+------+------+------+------+
 * MergeRun은 위와같은 모양의 Array로, MergeRunCount * 2 + 1 개로 구성된다.
 ***************************************************************************/
IDE_RC sdtSortModule::makeMergePosition( smiTempTableHeader  * aHeader,
                                         void               ** aMergePos )
{
    sdtTempMergeRunInfo    sRunInfo;
    sdtTempMergePos      * sMergePos = (sdtTempMergePos*)*aMergePos;
    scPageID                sWPID;
    sdtSortSegHdr         * sWASeg = (sdtSortSegHdr*)aHeader->mWASegment;
    UInt                    i;
    UInt                    sState = 0;
    ULong                   sSize  = 0;

    IDE_ERROR( ( aHeader->mTTState == SMI_TTSTATE_SORT_MERGE ) ||
               ( aHeader->mTTState == SMI_TTSTATE_SORT_MERGESCAN ) );

    if ( sMergePos == NULL )
    {
        sSize  = ID_SIZEOF( sdtTempMergePos ) * SDT_TEMP_MERGEPOS_SIZE(aHeader->mMergeRunCount );

        /* TC/FIT/Server/sm/Project/PROJ-2458/Memory/sdt/
         * sdtSortModule_makeMergePosition_malloc_MergePos.tc */
        IDU_FIT_POINT_RAISE("sdtSortModule::makeMergePosition::malloc::MergePos",
                            insufficient_memory);
        IDE_TEST_RAISE( iduMemMgr::malloc( IDU_MEM_SM_TEMP,
                                           sSize,
                                           (void**)&sMergePos )
                        != IDE_SUCCESS,
                        insufficient_memory );
        sState = 1;
    }
    else
    {
        /* nothing to do */
    }

    for( i = 0 ; i < aHeader->mMergeRunCount ; i++ )
    {
        IDE_TEST( sdtWASortMap::get( &sWASeg->mSortMapHdr,
                                     aHeader->mLeftBottomPos + i,
                                     (void*)&sRunInfo )
                  != IDE_SUCCESS );

        sWPID = getWPIDFromRunInfo( aHeader,
                                    sRunInfo.mRunNo,
                                    sRunInfo.mPIDSeq );
        if ( sWPID != SC_NULL_PID )
        {
            sMergePos[ SDT_TEMP_MERGEPOS_PIDIDX(i) ] =
                sdtSortSegment::getNPID( sWASeg, sWPID );
            sMergePos[ SDT_TEMP_MERGEPOS_SLOTIDX(i) ] =
                sRunInfo.mSlotNo;
        }
        else
        {
            /* Run이 비어있음 */
            sMergePos[ SDT_TEMP_MERGEPOS_PIDIDX(i) ] = SC_NULL_PID;
            sMergePos[ SDT_TEMP_MERGEPOS_SLOTIDX(i) ] = SC_NULL_PID;
        }
    }

    sMergePos[ SDT_TEMP_MERGEPOS_SIZEIDX ] = i;
    *aMergePos = (void*)sMergePos;

    return IDE_SUCCESS;

    IDE_EXCEPTION( insufficient_memory );
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_InsufficientMemory));
    }
    IDE_EXCEPTION_END;

    switch( sState )
    {
        case 1:
            if ( sMergePos != NULL )
            {
                IDE_ASSERT( iduMemMgr::free( sMergePos ) == IDE_SUCCESS );
                sMergePos = NULL;
            }
        default:
            break;
    }

    return IDE_FAILURE;
}

/**************************************************************************
 * Description :
 * MergeCursor를 만든다.
 *
 * MergePosition으로 MergeRun들을 만든다.
 ***************************************************************************/
IDE_RC sdtSortModule::makeMergeRuns( smiTempTableHeader  * aHeader,
                                     void                * aMergePosition )
{
    sdtTempMergeRunInfo   sRunInfo;
    sdtTempMergePos     * sMergePos;
    scPageID               sWPID;
    sdtSortSegHdr        * sWASeg = (sdtSortSegHdr*)aHeader->mWASegment;
    scPageID               sPageSeq;
    scPageID               sNPID;
    UInt                   sSlotIdx;
    UInt                   i;
    UInt                   j;

    sMergePos = (sdtTempMergePos*)aMergePosition;

    IDE_ERROR( sMergePos[ SDT_TEMP_MERGEPOS_SIZEIDX ]
               == aHeader->mMergeRunCount );
    IDE_ERROR( aHeader->mMergeRunSize > 0 );

    /* Run개수 만큼 순회 */
    for( i = 0 ; i < sMergePos[ SDT_TEMP_MERGEPOS_SIZEIDX ] ; i++ )
    {
        sNPID    = sMergePos[ SDT_TEMP_MERGEPOS_PIDIDX( i ) ];
        sSlotIdx = sMergePos[ SDT_TEMP_MERGEPOS_SLOTIDX(i) ];

        if ( sNPID == SC_NULL_PID )
        {
            sRunInfo.mRunNo  = SDT_TEMP_RUNINFO_NULL;
            sRunInfo.mPIDSeq = 0;
            sRunInfo.mSlotNo = 0;
        }
        else
        {
            /* 이번 Run에서 읽어야할 페이지가 Load되어 있는지 확인한다 */
            for( j = 0 ; j < aHeader->mMergeRunSize ; j++ )
            {
                sWPID = getWPIDFromRunInfo( aHeader, i, j );
                if ( sNPID == sdtSortSegment::getNPID( sWASeg, sWPID ) )
                {
                    break;
                }
            }

            /* 못찾았음. 그러면 0번부터 차례로 불러오면 됨. */
            if ( j == aHeader->mMergeRunSize )
            {
                j = 0;
            }

            /* Run정보 설정 */
            sRunInfo.mRunNo  = i;
            sRunInfo.mPIDSeq = j;
            sRunInfo.mSlotNo = sSlotIdx;

            /* 한 페이지에 1024개 이상의 Slot은 불가능. 8byte align을
             * 맞추니, 8192/8 =1024니까 */
            IDE_DASSERT( sSlotIdx <= 1024 );

            /* Load된 페이지부터, MaxRowPageCount*2만큼 Load해주면 된다. */
            sPageSeq = aHeader->mMaxRowPageCount*2;
            while( sPageSeq-- )
            {
                IDE_TEST( readRunPage( aHeader,
                                       i,
                                       j,
                                       &sNPID,
                                       ID_FALSE ) /*aReadNextNPID*/
                          != IDE_SUCCESS );
                if ( j == 0 )
                {
                    j = aHeader->mMergeRunSize;
                }
                j--;

                if ( sNPID == SD_NULL_PID )
                {
                    /* 해당 run 에 다음 Row가 없음 */
                    break;
                }
                else
                {
                    /* nothing to do */
                }
            }
        }

        IDE_TEST( sdtWASortMap::set( &sWASeg->mSortMapHdr,
                                     aHeader->mLeftBottomPos + i,
                                     (void*)&sRunInfo )
                  != IDE_SUCCESS );
    }

    /* Run을 적제시켰으니 LoserTree를 구성한다. */
    IDE_TEST( buildLooserTree( aHeader ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**************************************************************************
 * Description :
 *
 ***************************************************************************/
IDE_RC sdtSortModule::makeScanPosition( smiTempTableHeader  * aHeader,
                                        scPageID           ** aScanPosition )
{
    sdtSortSegHdr      * sWASeg  = (sdtSortSegHdr*)aHeader->mWASegment;
    UInt                 sRunCnt = aHeader->mRunQueue.getQueueLength();
    sdtTempScanPos    * sScanPos;
    UInt                 i;
    UInt                 sState  = 0;
    scPageID             sNPID   = SC_NULL_PID ;
    idBool               sIsEmpty;

    IDE_ERROR( aHeader->mTTState == SMI_TTSTATE_SORT_SCAN );

    /* run이 없는데 이쪽으로 왔을리가 없음. */
    IDE_ERROR( aHeader->mRunQueue.getQueueLength() != 0 );

    /* 마지막 Run의 Slot 개수 */
    aHeader->mStatsPtr->mExtraStat2 =
        sdtWASortMap::getSlotCount( &sWASeg->mSortMapHdr );

    /* 마지막 Run을 생성해서 내림 */
    if ( sdtWASortMap::getSlotCount( &sWASeg->mSortMapHdr ) > 0 )
    {
        IDE_TEST( storeSortedRun( aHeader ) != IDE_SUCCESS );
        sRunCnt++;
    }

    /* 검증용 */
    IDE_DASSERT ( sRunCnt == aHeader->mRunQueue.getQueueLength() );

    /* Scan Group으로 재생성 */
    IDE_TEST( sdtSortSegment::dropWAGroup( sWASeg,
                                           SDT_WAGROUPID_FLUSH,
                                           ID_FALSE ) /*wait4flush */
              != IDE_SUCCESS );

    IDE_TEST( sdtSortSegment::dropWAGroup( sWASeg,
                                           SDT_WAGROUPID_SORT,
                                           ID_FALSE ) /*wait4flush */
              != IDE_SUCCESS );

    /* Scan Group으로 재생성 */
    IDE_TEST( sdtSortSegment::createWAGroup( sWASeg,
                                             SDT_WAGROUPID_SCAN,
                                             0, /* aPageCount */
                                             SDT_WA_REUSE_FIFO )
              != IDE_SUCCESS );

    /* Map을 더이상 사용 안함 */
    sdtWASortMap::disable( sWASeg );

    /* sdtSortModule_makeScanPosition_malloc_ScanPos.tc */
    IDU_FIT_POINT("sdtSortModule::makeScanPosition::malloc::ScanPos");
    IDE_TEST( iduMemMgr::malloc( IDU_MEM_SM_TEMP,
                                 ID_SIZEOF( sdtTempScanPos ) *
                                 SDT_TEMP_SCANPOS_SIZE( sRunCnt ),
                                 (void**)&sScanPos )
              != IDE_SUCCESS );
    sState = 1;

    for( i = 0 ; i < sRunCnt ; i++ )
    {
        /* Queue 에서 페이지를 하나씩 꺼냄*/
        IDE_TEST( aHeader->mRunQueue.dequeue( ID_FALSE, /* a_bLock */
                                              (void*)&sNPID,
                                              &sIsEmpty ) );
        if ( sIsEmpty == ID_FALSE )
        {
            IDE_ERROR( sNPID != SC_NULL_PID );
            sScanPos[ SDT_TEMP_SCANPOS_PIDIDX(i) ] = sNPID;
        }
        else
        {
            /* Run이 비어있음 */
            sScanPos[ SDT_TEMP_SCANPOS_PIDIDX(i) ] = SC_NULL_PID;
        }
    }

    sScanPos[ SDT_TEMP_SCANPOS_PINIDX ] = 0;
    sScanPos[ SDT_TEMP_SCANPOS_SIZEIDX ] = i;
    *aScanPosition = sScanPos;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch( sState )
    {
        case 1:
            IDE_ASSERT( iduMemMgr::free( sScanPos ) == IDE_SUCCESS );
            sScanPos = NULL;
        default:
            break;
    }

    return IDE_FAILURE;
}
