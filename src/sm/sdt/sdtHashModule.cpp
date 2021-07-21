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
#include <smuProperty.h>
#include <sdtTempPage.h>
#include <sdtHashModule.h>
#include <sdtWAExtentMgr.h>

smiColumn     sdtHashModule::mBlankColumn;
UInt          sdtHashModule::mSubHashPageCount;
UInt          sdtHashModule::mHashSlotPageCount;
iduMutex      sdtHashModule::mMutex;
smuMemStack   sdtHashModule::mWASegmentPool;
smuMemStack   sdtHashModule::mNHashMapPool;
UInt          sdtHashModule::mMaxWAExtentCount;

/**************************************************************************
 * Description : Hash SegPool을 만들고 Mutex를 생성 한다.
 *               서버 올릴때 호출된다.
 ***************************************************************************/
IDE_RC sdtHashModule::initializeStatic()
{    
    UInt    sState = 0;

    idlOS::memset( &mBlankColumn, 0, ID_SIZEOF( smiColumn ) );

    IDE_TEST( mMutex.initialize( (SChar*)"SDT_HASH_SEGMENT_MUTEX",
                                 IDU_MUTEX_KIND_NATIVE,
                                 IDV_WAIT_INDEX_NULL )
              != IDE_SUCCESS );
    sState = 1;

    mMaxWAExtentCount = calcWAExtentCount( smuProperty::getHashAreaSize() );
    resetTempHashGroupPageCount();
    resetTempSubHashGroupPageCount();
    
    IDE_TEST( mWASegmentPool.initialize( IDU_MEM_SM_TEMP,
                                         getWASegmentSize(),
                                         smuProperty::getTmpInitWASegCnt())
              != IDE_SUCCESS );
    sState = 2;

    IDE_TEST( mNHashMapPool.initialize( IDU_MEM_SM_TEMP,
                                        getMaxWAExtentCount() * SDT_WAEXTENT_PAGECOUNT *
                                        ID_SIZEOF(sdtWCB*) / smuProperty::getTempHashBucketDensity(),
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
 * Description : Mutex와 각종 Pool을 free 한다.
 *               서버 내릴때 호출된다.
 ***************************************************************************/
void sdtHashModule::destroyStatic()
{
    (void)mWASegmentPool.destroy();

    (void)mNHashMapPool.destroy();

    mMutex.destroy();
}

/**************************************************************************
 * Description : Hash Group Page Count를 초기화 한다.
 ***************************************************************************/
void sdtHashModule::resetTempHashGroupPageCount()
{
    mHashSlotPageCount = SDT_WAEXTENT_PAGECOUNT *
        calcTempHashGroupExtentCount( getMaxWAExtentCount() );
}

/**************************************************************************
 * Description : Hash Group Extent Count를 계산 한다.
 *               HashSlot의 수는 65535의 배수가 되어야 한다. 이렇게 하면,
 *               Hash Value 4Byte 중 하위 2Byte는 서로 일치하게 된다.
 *               그럼 Sub Hash는 남은 상위 2Byte만으로 서로 구분 가능하게 된다.
 ***************************************************************************/
UInt sdtHashModule::calcTempHashGroupExtentCount( UInt    aMaxWAExtentCount )
{
    UInt sHashGroupExtentCount;

    sHashGroupExtentCount = aMaxWAExtentCount * smuProperty::getTempHashGroupRatio() / 100;

    if ( sHashGroupExtentCount > ( aMaxWAExtentCount
                                   - SDT_MIN_HASHROW_EXTENTCOUNT
                                   - SDT_MIN_SUBHASH_EXTENTCOUNT ) )
    {
        sHashGroupExtentCount = ( aMaxWAExtentCount
                                  - SDT_MIN_HASHROW_EXTENTCOUNT
                                  - SDT_MIN_SUBHASH_EXTENTCOUNT );
    }

    if ( sHashGroupExtentCount <= SDT_MIN_HASHSLOT_EXTENTCOUNT )
    {
        /* 최소값 설정*/
        sHashGroupExtentCount = SDT_MIN_HASHSLOT_EXTENTCOUNT;
    }
    else
    {
        /* 2개 WAExtent의 배수로 할당 하여야 한다. */
        if (( sHashGroupExtentCount % SDT_MIN_HASHSLOT_EXTENTCOUNT ) > 0 )
        {
            sHashGroupExtentCount -= ( sHashGroupExtentCount % SDT_MIN_HASHSLOT_EXTENTCOUNT );
        }
    }
    return sHashGroupExtentCount;
}

/**************************************************************************
 * Description : Sub Hash Group Page Count를 초기화 한다.
 ***************************************************************************/
void sdtHashModule::resetTempSubHashGroupPageCount()
{
    mSubHashPageCount = SDT_WAEXTENT_PAGECOUNT *
        calcTempSubHashGroupExtentCount( getMaxWAExtentCount() ) ;
}

/**************************************************************************
 * Description : Sub Hash Group Extent Count를 계산 한다.
 ***************************************************************************/
UInt sdtHashModule::calcTempSubHashGroupExtentCount( UInt    aMaxWAExtentCount )
{
    SInt sSubHashGroupExtentCount;
    SInt sHashGroupExtentCount = (SInt)getHashSlotPageCount() / SDT_WAEXTENT_PAGECOUNT;

    sSubHashGroupExtentCount = (SInt)aMaxWAExtentCount * smuProperty::getTempSubHashGroupRatio() / 100;

    if ( sSubHashGroupExtentCount > ( (SInt)aMaxWAExtentCount
                                      - sHashGroupExtentCount
                                      - SDT_MIN_HASHROW_EXTENTCOUNT ) )
    {
        sSubHashGroupExtentCount = ( (SInt)aMaxWAExtentCount
                                     - sHashGroupExtentCount
                                     - SDT_MIN_HASHROW_EXTENTCOUNT );
    }

    if ( sSubHashGroupExtentCount < SDT_MIN_SUBHASH_EXTENTCOUNT )
    {
        sSubHashGroupExtentCount = SDT_MIN_SUBHASH_EXTENTCOUNT;
    }

    return sSubHashGroupExtentCount ;
}

/**************************************************************************
 * Description : Row Extent의 수를 기준으로 적정한 Hash Extent의 수를 계산하여 반환
 ***************************************************************************/
ULong sdtHashModule::calcOptimalWorkAreaSize( UInt  aRowPageCount )
{
    UInt     sRowExtentCount;
    UInt     sAllExtentCount;
    UInt     sHashExtentCount;
    UInt     sSubHashExtentCount;

    sRowExtentCount = ( aRowPageCount + SDT_WAEXTENT_PAGECOUNT - 1 ) / SDT_WAEXTENT_PAGECOUNT;

    if ( sRowExtentCount < SDT_MIN_HASHROW_EXTENTCOUNT )
    {
        sRowExtentCount = SDT_MIN_HASHROW_EXTENTCOUNT ;
    }

    sAllExtentCount = SDT_GET_FULL_SIZE( sRowExtentCount,
                                         ( 100 - smuProperty::getTempHashGroupRatio() - smuProperty::getTempSubHashGroupRatio() ) );

    sHashExtentCount    = calcTempHashGroupExtentCount( sAllExtentCount );
    sSubHashExtentCount = calcTempSubHashGroupExtentCount( sAllExtentCount );

    sAllExtentCount = ( sRowExtentCount + sHashExtentCount + sSubHashExtentCount );

    if ( sAllExtentCount < SDT_MIN_HASH_EXTENTCOUNT )
    {
        sAllExtentCount = SDT_MIN_HASH_EXTENTCOUNT;
    }

    return (ULong)sAllExtentCount * SDT_WAEXTENT_SIZE;
}

/**************************************************************************
 * Description : Row Count 를 기준으로 적정한 Work Area Size를 계산하여 반환
 ***************************************************************************/
ULong sdtHashModule::calcSubOptimalWorkAreaSizeByRowCnt( ULong aRowCount )
{
    UInt     sKeyCount;
    UInt     sHashPageCount;
    UInt     sHashExtentCount;

    sKeyCount = aRowCount * 5 / 4; /* SubHashNodeHeader 를 약 20%로 가정*/

    /* SubHash를 모두 load하는데 필요한 Extent*/
    sHashPageCount   = ( sKeyCount * ID_SIZEOF(sdtSubHashKey) + SD_PAGE_SIZE - 1 ) / SD_PAGE_SIZE;
    sHashExtentCount = ( sHashPageCount + SDT_WAEXTENT_PAGECOUNT - 1 ) / SDT_WAEXTENT_PAGECOUNT ;

    return calcSubOptimalWorkAreaSizeBySubHashExtCnt( sHashExtentCount ) ;
}

/**************************************************************************
 * Description : Sub Hash의 NExtent Count 를 기준으로
 *               적정한 Work Area Size 를 계산하여 반환
 *               onepass 가 되려면 Sub Hash는 한번에 load가능해야 한다.
 *               Hash Group은 6%이고 Key가 16Byte인데 반해서
 *               Sub Hash는 Fetch 할 때 Row Area 의 90%까지 확장되고,
 *               Sub Hash Key는 8Byte이다,
 *               Hash Slot 보다 더 많은 Sub Hash Key를 담을 수 있다.
 *               Hash Group에서 모두 커버 가능 할 경우는 In Memory나 다름없다.
 *               그래서 Sub Hash 기준으로 계산한다.
 ***************************************************************************/
ULong sdtHashModule::calcSubOptimalWorkAreaSizeBySubHashExtCnt( UInt aSubHashExtentCount )
{
    UInt     sAllExtentCount;
    UInt     sRowExtentCount;
    UInt     sHashExtentCount;

    /* HashScan 시점에 SubHash를 모두 Load하기 위한 Row Area의 크기*/
    sRowExtentCount  = SDT_GET_FULL_SIZE( aSubHashExtentCount,
                                          smuProperty::getTmpHashFetchSubHashMaxRatio() );

    if ( sRowExtentCount > SDT_MIN_HASHROW_EXTENTCOUNT )
    {
        /* Hash Slot Group Extent Count 계산*/
        sAllExtentCount  = SDT_GET_FULL_SIZE( sRowExtentCount,
                                              ( 100 - smuProperty::getTempHashGroupRatio() ) );
        sHashExtentCount = calcTempHashGroupExtentCount( sAllExtentCount );

        /* Hash Group과 Row Area를 합하여 HASH_AREA_SIZE를 얻음 */
        sAllExtentCount  = sHashExtentCount + sRowExtentCount;

        if ( sAllExtentCount < SDT_MIN_HASH_EXTENTCOUNT )
        {
            sAllExtentCount = SDT_MIN_HASH_EXTENTCOUNT;
        }
    }
    else
    {
        // Default Size보다 작다면 Min Size를 넘긴다.
        sAllExtentCount = SDT_MIN_HASH_EXTENTCOUNT;
    }

    return (ULong)sAllExtentCount * SDT_WAEXTENT_SIZE;
}


/**************************************************************************
 * Description : HASH_AREA_SIZE 변경에 따라 Hash Segment 의 크기를 변경한다.
 *               이미 사용중인 Hash Segment는 이전 크기를 유지하고,
 *               새로 생성되는 Hash Segment부터 새 크기로 생성된다.
 *               lock을 잡은 체로 호출 되어야 한다.
 *
 * aNewWorkAreaSize  - [IN] 변경 할 크기
 ***************************************************************************/
IDE_RC sdtHashModule::resetWASegmentSize( ULong aNewWorkAreaSize )
{
    UInt   sNewWAExtentCount = calcWAExtentCount( aNewWorkAreaSize ) ;
    UInt   sNewWAPageCount   = sNewWAExtentCount * SDT_WAEXTENT_PAGECOUNT ;
    UInt   sNewWASegmentSize = calcWASegmentSize( sNewWAPageCount );
    UInt   sOldWAExtentCount = getMaxWAExtentCount();
    UInt   sHashBucketCnt    = sNewWAPageCount  / smuProperty::getTempHashBucketDensity();
    UInt   sState = 0;

    IDE_TEST( mWASegmentPool.resetDataSize( sNewWASegmentSize ) != IDE_SUCCESS );
    sState = 1;

    IDE_TEST( mNHashMapPool.resetDataSize( sHashBucketCnt * ID_SIZEOF(sdtWCB*) ) != IDE_SUCCESS );
    sState = 2;

    mMaxWAExtentCount = sNewWAExtentCount;
    sState = 3;

    /* HASH AREA SIZE가 변경되면, group size도 변경 해야 한다.*/
    sdtHashModule::resetTempHashGroupPageCount();
    sdtHashModule::resetTempSubHashGroupPageCount();

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch( sState )
    {
        case 3:
            mMaxWAExtentCount = sOldWAExtentCount;
            sdtHashModule::resetTempHashGroupPageCount();
            sdtHashModule::resetTempSubHashGroupPageCount();
        case 2:
            (void)mNHashMapPool.resetDataSize( sOldWAExtentCount * SDT_WAEXTENT_PAGECOUNT /
                                               smuProperty::getTempHashBucketDensity()
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
 *               Hash Segment 의 크기를 변경한다. 잘 호출되지 않는다.
 *
 * aNewWASegmentCount  - [IN] 변경 할 수
 ***************************************************************************/
IDE_RC sdtHashModule::resizeWASegmentPool( UInt   aNewWASegmentCount )
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
 * Description : temp table header에 필요한 값을 설정한다.
 *
 * aHeader  - [IN] 대상 Table
 ***************************************************************************/
void sdtHashModule::initHashTempHeader( smiTempTableHeader * aHeader )
{
    sdtHashSegHdr   * sWASeg = (sdtHashSegHdr*)aHeader->mWASegment;

    if ( SM_IS_FLAG_ON( aHeader->mTTFlag,
                        SMI_TTFLAG_UNIQUE ) == ID_TRUE )
    {
        sWASeg->mUnique = SDT_HASH_UNIQUE ;
    }
    else
    {
        sWASeg->mUnique = SDT_HASH_NORMAL ;
    }
    // temp header
    aHeader->mStatsPtr->mExtraStat1 = sWASeg->mHashSlotCount;

    return ;
}

/**************************************************************************
 * Description : WA Group을 초기화 한다.
 *               [ HashSlot ][      Insert Group     ][ SubHash ]
 *               최초 생성시점엔 fetch group은 사용되지 않는다.
 *
 * aWASeg  - [IN] 대상 HashSegment
 ***************************************************************************/
void sdtHashModule::initHashGrp( sdtHashSegHdr   * aWASeg )
{
    IDE_DASSERT(( aWASeg->mSubHashPageCount % SDT_WAEXTENT_PAGECOUNT ) == 0 );
    IDE_DASSERT(( aWASeg->mHashSlotPageCount % ( SDT_WAEXTENT_PAGECOUNT * SDT_MIN_HASHSLOT_EXTENTCOUNT )) == 0 );

    aWASeg->mInsertGrp.mBeginWPID    = aWASeg->mHashSlotPageCount;
    aWASeg->mInsertGrp.mReuseSeqWPID = aWASeg->mHashSlotPageCount;
    aWASeg->mInsertGrp.mEndWPID      = aWASeg->mEndWPID - aWASeg->mSubHashPageCount;

    /* discard가 되기 전 까지는 fetch할 page는 모두 inmemory에 있다.
     * 그러므로 fetch group은 최초 discard 가 발생 한 후에 설정한다.*/
    aWASeg->mFetchGrp.mBeginWPID    = aWASeg->mEndWPID;
    aWASeg->mFetchGrp.mReuseSeqWPID = aWASeg->mEndWPID;
    aWASeg->mFetchGrp.mEndWPID      = aWASeg->mEndWPID;

    /* 후방에 Sub Hash구축을 위한 공간을 남겨둔다.*/
    aWASeg->mSubHashGrp.mBeginWPID    = aWASeg->mEndWPID - aWASeg->mSubHashPageCount;
    aWASeg->mSubHashGrp.mReuseSeqWPID = aWASeg->mEndWPID - aWASeg->mSubHashPageCount;
    aWASeg->mSubHashGrp.mEndWPID      = aWASeg->mEndWPID;

    IDE_DASSERT( aWASeg->mInsertGrp.mEndWPID  > aWASeg->mInsertGrp.mBeginWPID );
    IDE_DASSERT( aWASeg->mSubHashGrp.mEndWPID > aWASeg->mSubHashGrp.mBeginWPID );

    return ;
}

/**************************************************************************
 * Description : Hash Segment를 생성하여 Header에 달아준다.
 *
 * aHeader           - [IN] TempTable Header
 * aInitWorkAreaSize - [IN] 최초 할당할 WA의 크기
 ***************************************************************************/
IDE_RC sdtHashModule::createHashSegment( smiTempTableHeader * aHeader,
                                         ULong                aInitWorkAreaSize )
{
    sdtHashSegHdr  * sWASeg = NULL;
    UInt             sMaxWAExtentCount;
    UInt             sInitWAExtentCount;
    UInt             sMaxWAPageCount;
    UInt             sSubHashPageCount;
    UInt             sHashSlotPageCount;
    UInt             sState  = 0;
    idBool           sIsLock = ID_FALSE;

    lock();
    sIsLock = ID_TRUE;
    sMaxWAExtentCount = getMaxWAExtentCount();
    sMaxWAPageCount   = sMaxWAExtentCount * SDT_WAEXTENT_PAGECOUNT;

    IDE_TEST( mWASegmentPool.getDataSize() != calcWASegmentSize( sMaxWAPageCount ) );

    IDE_TEST( mWASegmentPool.pop( (UChar**)&sWASeg ) != IDE_SUCCESS );
    sState = 1;

    /* sMaxWAPageCount, sSubHashPageCount, sHashSlotPageCount는
     * 서로 연관이 있는 크기이므로 lock 안에서 한번에 받아와야 한다.*/
    sSubHashPageCount  = getSubHashPageCount();
    sHashSlotPageCount = getHashSlotPageCount();

    IDE_DASSERT( sWASeg != NULL );

    sIsLock = ID_FALSE;
    unlock();

    idlOS::memset( (UChar*)sWASeg, 0, SDT_HASH_SEGMENT_HEADER_SIZE );

    sWASeg->mMaxWAExtentCount = sMaxWAExtentCount ;

    if ( aInitWorkAreaSize == 0 )
    {
        sInitWAExtentCount = smuProperty::getTmpMinInitWAExtCnt();
    }
    else
    {
        sInitWAExtentCount = sdtWAExtentMgr::calcWAExtentCount( aInitWorkAreaSize );

        // Hash Area와 Row Area각 1개씩
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
    sWASeg->mSpaceID           = aHeader->mSpaceID;
    sWASeg->mWorkType          = SDT_WORK_TYPE_HASH;
    sWASeg->mStatsPtr          = aHeader->mStatsPtr;
    sWASeg->mNPageCount        = 0;
    sWASeg->mIsInMemory        = SDT_WORKAREA_IN_MEMORY;
    sWASeg->mStatistics        = aHeader->mStatistics;
    sWASeg->mAllocWAPageCount  = 0;
    sWASeg->mUsedWCBPtr        = SDT_USED_PAGE_PTR_TERMINATED;
    sWASeg->mEndWPID           = sMaxWAPageCount;
    sWASeg->mSubHashPageCount  = sSubHashPageCount;
    sWASeg->mHashSlotPageCount = sHashSlotPageCount;
    sWASeg->mNExtFstPIDList4Row.mPageSeqInLFE     = SDT_WAEXTENT_PAGECOUNT;
    sWASeg->mNExtFstPIDList4SubHash.mPageSeqInLFE = SDT_WAEXTENT_PAGECOUNT;

    sWASeg->mHashSlotCount = sWASeg->mHashSlotPageCount * (SD_PAGE_SIZE/ID_SIZEOF(sdtHashSlot));
    IDE_DASSERT( sWASeg->mHashSlotCount % (ID_USHORT_MAX+1) == 0 );

    sWASeg->mStatsPtr->mRuntimeMemSize += mWASegmentPool.getNodeSize((UChar*) sWASeg );

    IDE_TEST( sdtWAExtentMgr::initWAExtents( sWASeg->mStatistics,
                                             sWASeg->mStatsPtr,
                                             &sWASeg->mWAExtentInfo,
                                             sInitWAExtentCount )
              != IDE_SUCCESS );
    sState = 2;

    sWASeg->mCurFreeWAExtent = sWASeg->mWAExtentInfo.mHead;
    sWASeg->mCurrFreePageIdx = 0;
    sWASeg->mNxtFreeWAExtent = sWASeg->mCurFreeWAExtent->mNextExtent;

    aHeader->mWASegment = sWASeg;

    initHashGrp( sWASeg );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sIsLock == ID_FALSE )
    {
        lock();
    }

    switch( sState )
    {
        case 2:
            (void)sdtWAExtentMgr::freeWAExtents( &sWASeg->mWAExtentInfo );
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
 * Description : Segment를 Drop 하고 자원을 정리한다.
 *
 * aWASegment  - [IN] 대상 WASegment
 ***************************************************************************/
IDE_RC sdtHashModule::dropHashSegment( sdtHashSegHdr* aWASegment )
{
    if ( aWASegment != NULL )
    {
        setInsertHintWCB( aWASegment, NULL );

        IDE_TEST( clearHashSegment( aWASegment ) != IDE_SUCCESS );
    }
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**************************************************************************
 * Description : Segment를 Drop하고 안의 Extent도 반환한다.
 *
 * aWASegment  - [IN] 대상 WASegment
 ***************************************************************************/
IDE_RC sdtHashModule::clearHashSegment( sdtHashSegHdr* aWASegment )
{
    UInt   sState = 0;
    idBool sIsLock = ID_FALSE;

    if ( aWASegment != NULL )
    {
        sState = 1;

        aWASegment->mNExtFstPIDList4Row.mLastFreeExtFstPID = SM_NULL_PID;

        clearAllWCBs( aWASegment );

        sState = 2;
        IDE_TEST( sdtWAExtentMgr::freeWAExtents( &aWASegment->mWAExtentInfo ) != IDE_SUCCESS );

        sState = 3;
        IDE_TEST( sdtWAExtentMgr::freeAllNExtents( aWASegment->mSpaceID,
                                                   &aWASegment->mNExtFstPIDList4Row )
                  != IDE_SUCCESS );

        sState = 4;
        IDE_TEST( sdtWAExtentMgr::freeAllNExtents( aWASegment->mSpaceID,
                                                   &aWASegment->mNExtFstPIDList4SubHash )
                  != IDE_SUCCESS );

        lock();
        sIsLock = ID_TRUE;

        sState = 5;
        if ( aWASegment->mNPageHashPtr != NULL )
        {
#ifdef DEBUG
            for( UInt i = 0 ; i < aWASegment->mNPageHashBucketCnt ; i++ )
            {
                IDE_ASSERT( aWASegment->mNPageHashPtr[i] == NULL );
            }
#endif
            IDE_TEST( mNHashMapPool.push( (UChar*)(aWASegment->mNPageHashPtr) )
                      != IDE_SUCCESS );

            aWASegment->mNPageHashPtr = NULL;
            aWASegment->mNPageHashBucketCnt = 0;
        }

        sState = 6;
        IDE_TEST( mWASegmentPool.push( (UChar*)aWASegment )
                  != IDE_SUCCESS );

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
            clearAllWCBs( aWASegment );
            (void)sdtWAExtentMgr::freeWAExtents( &aWASegment->mWAExtentInfo );
        case 2:
            (void)sdtWAExtentMgr::freeAllNExtents( aWASegment->mSpaceID,
                                                   &aWASegment->mNExtFstPIDList4Row );
        case 3:
            (void)sdtWAExtentMgr::freeAllNExtents( aWASegment->mSpaceID,
                                                   &aWASegment->mNExtFstPIDList4SubHash );
        case 4:
            if ( sIsLock == ID_FALSE )
            {
                lock();
                sIsLock = ID_TRUE;
            }
            if ( aWASegment->mNPageHashPtr != NULL )
            {
                (void)mNHashMapPool.push( (UChar*)(aWASegment->mNPageHashPtr) );
            }
        case 5:
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
 * Description : 이 WASegment가 사용한 모든 WCB를 Free한다.
 *
 * aWASegment  - [IN] 대상 WASegment
 ***************************************************************************/
void sdtHashModule::clearAllWCBs( sdtHashSegHdr* aWASegment )
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
        sCurWCBPtr = aWASegment->mUsedWCBPtr;

        while( sCurWCBPtr != SDT_USED_PAGE_PTR_TERMINATED )
        {
            sNxtWCBPtr = sCurWCBPtr->mNxtUsedWCBPtr;

            if (( sCurWCBPtr->mNPageID != SD_NULL_PID )&&
                (  aWASegment->mNPageHashPtr != NULL ))
            {
                sHashValue = getNPageHashValue( aWASegment, sCurWCBPtr->mNPageID );

                aWASegment->mNPageHashPtr[ sHashValue ] = NULL;
            }

            initWCB(sCurWCBPtr);
            IDE_DASSERT( aWASegment->mInsertHintWCBPtr != sCurWCBPtr );
            sCurWCBPtr->mNxtUsedWCBPtr = NULL;
            sCurWCBPtr->mWAPagePtr     = NULL;

            sCurWCBPtr = sNxtWCBPtr;
        }

        aWASegment->mUsedWCBPtr = SDT_USED_PAGE_PTR_TERMINATED;
#ifdef DEBUG
        sCurWCBPtr = &aWASegment->mWCBMap[0];
        for( i = 0 ;
             i< getMaxWAPageCount( aWASegment ) ;
             i++ , sCurWCBPtr++ )
        {
            IDE_DASSERT( sCurWCBPtr->mWPState == SDT_WA_PAGESTATE_INIT );
            IDE_DASSERT( sCurWCBPtr->mFix     == 0 );
            IDE_DASSERT( sCurWCBPtr->mNPageID    == SM_NULL_PID );
            IDE_DASSERT( sCurWCBPtr->mNextWCB4Hash  == NULL );
            IDE_DASSERT( sCurWCBPtr->mNxtUsedWCBPtr == NULL );
            IDE_DASSERT( sCurWCBPtr->mWAPagePtr     == NULL );
        }
        for( i = 0 ; i < aWASegment->mNPageHashBucketCnt ; i++ )
        {
            IDE_ASSERT( aWASegment->mNPageHashPtr[i] == NULL );
        }
#endif
    }
    else
    {
        /* 모두 memset하는 쪽이 느리지만 , 만약 BUG로 제대로 정리 안될때를 대비해서
         * memset으로 초기화 하는 모드도 남겨준다. */
        idlOS::memset( &aWASegment->mWCBMap,
                       0,
                       aWASegment->mMaxWAExtentCount *
                       SDT_WAEXTENT_PAGECOUNT * ID_SIZEOF(sdtWCB) );

        if ( aWASegment->mNPageHashPtr != NULL )
        {
            idlOS::memset( aWASegment->mNPageHashPtr,
                           0,
                           aWASegment->mNPageHashBucketCnt * ID_SIZEOF(sdtWCB*) );
        }

        aWASegment->mUsedWCBPtr = SDT_USED_PAGE_PTR_TERMINATED;
    }
}

/**************************************************************************
 * Description : Segment를 재활용하기 위해 초기화 한다.
 *
 * aWASegment  - [IN] 대상 WASegment
 ***************************************************************************/
IDE_RC sdtHashModule::truncateHashSegment( sdtHashSegHdr* aWASegment )
{
    IDE_ERROR( aWASegment != NULL );

    setInsertHintWCB( aWASegment, NULL );

    initHashGrp( aWASegment );

    aWASegment->mOpenCursorType = SMI_HASH_CURSOR_NONE;

    clearAllWCBs( aWASegment );

#ifdef DEBUG
    if ( aWASegment->mNPageHashPtr != NULL )
    {
        for( UInt i = 0 ; i < aWASegment->mNPageHashBucketCnt ; i++ )
        {
            IDE_ASSERT( aWASegment->mNPageHashPtr[i] == NULL );
        }
    }
#endif
    aWASegment->mUsedWCBPtr        = SDT_USED_PAGE_PTR_TERMINATED;
    aWASegment->mCurFreeWAExtent   = aWASegment->mWAExtentInfo.mHead;
    aWASegment->mCurrFreePageIdx   = 0;
    aWASegment->mNxtFreeWAExtent   = aWASegment->mCurFreeWAExtent->mNextExtent;
    aWASegment->mAllocWAPageCount  = 0;
    aWASegment->mIsInMemory        = SDT_WORKAREA_IN_MEMORY;
    aWASegment->mOpenCursorType    = SMI_HASH_CURSOR_NONE;
    aWASegment->mSubHashBuildCount = 0;
    aWASegment->mHashSlotAllocPageCount = 0;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/**************************************************************************
 * Description : HASH_AREA_SIZE 최적값을 계산한다.
 *               x$tempinfo 에서 확인 가능하다.
 *
 * aHeader   - [IN] 대상 Table
 ***************************************************************************/
void sdtHashModule::estimateHashSize( smiTempTableHeader * aHeader )
{
    sdtHashSegHdr     * sWASegment = (sdtHashSegHdr*)aHeader->mWASegment;
    smiTempTableStats * sStatsPtr  = aHeader->mStatsPtr;

    if ( aHeader->mRowCount > 0 )
    {
        sWASegment = (sdtHashSegHdr*)aHeader->mWASegment;

        IDE_DASSERT( sWASegment->mNPageCount > 0 );

        /* 종료되면서 예측 통계치를 계산한다. */
        /* Optimal(InMemory)은 모든 데이터가 HashArea에 담길 크기면 된다. */
        sStatsPtr->mEstimatedOptimalSize = calcOptimalWorkAreaSize( sWASegment->mNPageCount );

        /* Onepass 는 HashSlot을 한번에 Load 할 수 있는 크기이다.
         * Row Area는 Disk에서 읽어야 한다.*/
        if ( sWASegment->mNExtFstPIDList4SubHash.mCount > 0 )
        {
            /* Out Memory, 계산값은 지금 Size보다 작다. In Memory 로 하기에는 공간이 부족했다. */
            /* Unique Insert에서는 추가 필요한 부분이 Fetch여서 Size계산에서 별도로 크게는 신경 안써도 된다. */
            sStatsPtr->mEstimatedSubOptimalSize =
                calcSubOptimalWorkAreaSizeBySubHashExtCnt( sWASegment->mNExtFstPIDList4SubHash.mCount );
        }
        else
        {
            if ( sStatsPtr->mEstimatedOptimalSize == SDT_MIN_HASH_SIZE )
            {
                /* optimal 이 min값이면 계산 필요없다.*/
                sStatsPtr->mEstimatedSubOptimalSize = SDT_MIN_HASH_SIZE ;
            }
            else
            {
                /* In Memory, 계산값은 지금 Size보다 작다. 공간이 넉넉했다. */
                /* Sub Hash가 할당되지 않았었다고 해서 없는것은 아니다.
                 * Row Count를 기준으로 적당히 만들어낸다. */
                sStatsPtr->mEstimatedSubOptimalSize = calcSubOptimalWorkAreaSizeByRowCnt( aHeader->mRowCount );            
            }
        }
    }
    else
    {
        /* 사용한 적이 없는 테이블은 계산 하지 않는다. */
        IDE_DASSERT( aHeader->mRowCount == 0 );
        sStatsPtr->mEstimatedOptimalSize = 0;
        sStatsPtr->mEstimatedSubOptimalSize = 0;
    }

    sStatsPtr->mExtraStat1 = sWASegment->mHashSlotAllocPageCount ;

    return;
}

/**************************************************************************
 * Description : unique violation 확인
 *               모두 inmemory에 있을 때 탐색 함수,
 *               sub hash등을 고려하지 않는다.
 *               insert 4 update에서는 검증 하고 insert하므로
 *               충돌이 발생하면 안된다.
 *
 * aHeader     - [IN] 대상 Temp Table Header
 * aValueList  - [IN] 중복 키 검증 할 Value
 * aHashValue  - [IN] 중복 키 검증 할 HashValue
 * aTRPHeader  - [IN] 탐색 시작 row
 * aResult     - [OUT] insert가능하면 IDE_TRUE, 충돌이 있으면 ID_FALSE
 ***************************************************************************/
IDE_RC sdtHashModule::chkUniqueInsertableInMem( smiTempTableHeader  * aHeader,
                                                const smiValue      * aValueList,
                                                UInt                  aHashValue,
                                                sdtHashTRPHdr       * aTRPHeader,
                                                idBool              * aResult )
{
    sdtHashScanInfo  sTRPInfo;
    sdtHashSegHdr  * sWASeg = (sdtHashSegHdr*)aHeader->mWASegment;
    UInt             sLoop = 0;

    *aResult = ID_TRUE;
    /* fetch 해 올 크기, 검증에 필요한 위치 까지만 복사한다.*/
    sTRPInfo.mFetchEndOffset = aHeader->mKeyEndOffset;

    do
    {
        sLoop++;

        IDE_DASSERT( SM_IS_FLAG_ON( aTRPHeader->mTRFlag, SDT_TRFLAG_HEAD ) );

        if ( aTRPHeader->mHashValue == aHashValue )
        {
            sTRPInfo.mTRPHeader = aTRPHeader;

            IDE_TEST( fetch( sWASeg,
                             aHeader->mRowBuffer4Fetch,
                             &sTRPInfo )
                      != IDE_SUCCESS );

            if ( compareRowAndValue( aHeader,
                                     sTRPInfo.mValuePtr,
                                     aValueList ) == ID_TRUE )
            {
                /* for update에서는 찾는 경우가 있으면 안된다 */
                IDE_DASSERT( sWASeg->mOpenCursorType != SMI_HASH_CURSOR_HASH_UPDATE );
                *aResult = ID_FALSE;
                break;
            }
        }

        /* to next */
        aTRPHeader = aTRPHeader->mChildRowPtr;

    } while( aTRPHeader != NULL );

    aHeader->mStatsPtr->mExtraStat2 = IDL_MAX( aHeader->mStatsPtr->mExtraStat2, sLoop );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**************************************************************************
 * Description : unique violation 확인
 *               hash area를 넘어서 out memory 일 때 탐색 함수,
 *               sub hash, single row등을 고려해야 한다.
 *               insert 4 update에서는 검증 하고 insert하므로
 *               충돌이 발생하면 안된다.
 *
 * aHeader     - [IN] 대상 Temp Table Header
 * aValueList  - [IN] 중복 키 검증 할 Value
 * aHashValue  - [IN] 중복 키 검증 할 HashValue
 * aTRPHeader  - [IN] 탐색 시작 row
 * aResult     - [OUT] insert가능하면 IDE_TRUE, 충돌이 있으면 ID_FALSE
 ***************************************************************************/
IDE_RC sdtHashModule::chkUniqueInsertableOutMem( smiTempTableHeader  * aHeader,
                                                 const smiValue      * aValueList,
                                                 UInt                  aHashValue,
                                                 sdtHashSlot         * aHashSlot,
                                                 idBool              * aResult )
{
    UInt             sLoop = 0;
    sdtHashSegHdr  * sWASeg = (sdtHashSegHdr*)aHeader->mWASegment;
    sdtHashScanInfo  sTRPInfo;
    sdtHashTRPHdr  * sTRPHeader;
    sdtHashTRPHdr  * sPrvTRPHeader;
    scPageID         sSingleRowPageID = SM_NULL_PID;
    scOffset         sSingleRowOffset;
    scPageID         sSubHashPageID = SM_NULL_PID;
    scOffset         sSubHashOffset;

    IDE_DASSERT( sWASeg->mIsInMemory == SDT_WORKAREA_OUT_MEMORY );

    sTRPInfo.mFetchEndOffset = aHeader->mKeyEndOffset;

    if ( SDT_IS_NOT_SINGLE_ROW( aHashSlot->mOffset ) )
    {
        if ( aHashSlot->mRowPtrOrHashSet == (ULong)NULL )
        {
            /* in memory row는 없다.
             * 바로 sub hash탐색으로 넘어간다.
             * small hash set을 보고 넘어왔으니, null pid일 리는 없다.*/
            sSubHashPageID = aHashSlot->mPageID;
            sSubHashOffset = aHashSlot->mOffset;

            IDE_DASSERT( sSubHashPageID != SD_NULL_PID );
        }
        else
        {
            /* in memory를 먼저 탐색한다. */

            sTRPHeader = (sdtHashTRPHdr*)aHashSlot->mRowPtrOrHashSet;

            do
            {
                sLoop++;

                IDE_DASSERT( SM_IS_FLAG_ON( sTRPHeader->mTRFlag, SDT_TRFLAG_HEAD ) );

                if ( sTRPHeader->mHashValue == aHashValue )
                {
                    sTRPInfo.mTRPHeader = sTRPHeader;

                    IDE_TEST( fetch( sWASeg,
                                     aHeader->mRowBuffer4Fetch,
                                     &sTRPInfo )
                              != IDE_SUCCESS );

                    if ( compareRowAndValue( aHeader,
                                             sTRPInfo.mValuePtr,
                                             aValueList ) == ID_TRUE )
                    {
                        /* 찾았다, 즉 충돌이 발생 하였다. */
                        *aResult = ID_FALSE;

                        /* for Update에서는 이미 탐색 하고 들어왔다, 충돌이 발생하면 안된다. */
                        IDE_DASSERT( sWASeg->mOpenCursorType != SMI_HASH_CURSOR_HASH_UPDATE );
                        IDE_CONT( CONT_COMPLETE );
                    }
                }

                sPrvTRPHeader = sTRPHeader;
                sTRPHeader    = sPrvTRPHeader->mChildRowPtr ;

                /* Next Offset이 Single Row인지 확인
                 * Single Row 여부는 Prev Row의 Offset으로 확인한다.
                 * Single Row의 Child Row는 hash value이다. */
                if ( SDT_IS_SINGLE_ROW( sPrvTRPHeader->mChildOffset ) )
                {
                    break;
                }

            } while( sTRPHeader != NULL );

            if ( SDT_IS_SINGLE_ROW( sPrvTRPHeader->mChildOffset ) )
            {
                /* in memory row의 마지막에 연결된 single row의 hash value값이 일치한다.
                 * Disk 에 저장 되어 있는 row를 확인 해 봐야 한다.*/
                if ( (UInt)((ULong)sPrvTRPHeader->mChildRowPtr) == aHashValue )
                {
                    sSingleRowPageID = sPrvTRPHeader->mChildPageID;
                    sSingleRowOffset = SDT_DEL_SINGLE_FLAG_IN_OFFSET( sPrvTRPHeader->mChildOffset );

                    IDE_DASSERT( sSingleRowPageID != SD_NULL_PID );
                }
                else
                {
                    /* Hash value가 일치 하지 않는다. 찾지 못했다.*/
                }
            }
            else
            {
                /* Single row는 아니다. 설정되는 값은 sub hash 이거나 null pid 이다. */
                sSubHashPageID = sPrvTRPHeader->mChildPageID;
                sSubHashOffset = sPrvTRPHeader->mChildOffset;
            }
        }
    }
    else
    {
        if ( (UInt)aHashSlot->mRowPtrOrHashSet == aHashValue )
        {
            /* Single Row 하나 있는 경우이며, hash value도 일치한다.
             * 읽어서 확인해 봐야 한다. */
            sSingleRowPageID = aHashSlot->mPageID;
            sSingleRowOffset = SDT_DEL_SINGLE_FLAG_IN_OFFSET( aHashSlot->mOffset );

            IDE_DASSERT( sSingleRowPageID != SD_NULL_PID );
        }
        else
        {
            /* Single Row이나 hash value가 일치하지 않는다.*/
        }
    }

    /* 아직 찾지 못했다. Insert 할 수도 있다. */
    *aResult = ID_TRUE;

    if ( sSingleRowPageID != SM_NULL_PID )
    {
        IDE_TEST( chkUniqueInsertableSingleRow( aHeader,
                                                sWASeg,
                                                aValueList,
                                                sSingleRowPageID,
                                                sSingleRowOffset,
                                                aResult ) != IDE_SUCCESS );

        IDE_CONT( CONT_COMPLETE );
    }

    if ( sSubHashPageID != SD_NULL_PID )
    {
        IDE_TEST( chkUniqueInsertableSubHash( aHeader,
                                              sWASeg,
                                              aValueList,
                                              aHashValue,
                                              sSubHashPageID,
                                              sSubHashOffset,
                                              aResult ) != IDE_SUCCESS );
    }

    IDE_EXCEPTION_CONT( CONT_COMPLETE );

    aHeader->mStatsPtr->mExtraStat2 = IDL_MAX( aHeader->mStatsPtr->mExtraStat2, sLoop );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**************************************************************************
 * Description : unique violation 확인 single row일 때 확인
 *               Hash Value까지 확인하고 들어온다.
 *
 * aHeader          - [IN] 대상 Temp Table Header
 * aWASegment       - [IN] Hash Segment
 * aValueList       - [IN] 중복 키 검증 할 Value
 * aSingleRowPageID - [IN] 확인 할 Row의 page ID
 * aSingleRowOffset - [IN] 확인 할 Row의 Offset
 * aResult          - [OUT] insert가능하면 IDE_TRUE, 충돌이 있으면 ID_FALSE
 ***************************************************************************/
IDE_RC sdtHashModule::chkUniqueInsertableSingleRow( smiTempTableHeader  * aHeader,
                                                    sdtHashSegHdr       * aWASegment,
                                                    const smiValue      * aValueList,
                                                    scPageID              aSingleRowPageID,
                                                    scOffset              aSingleRowOffset,
                                                    idBool              * aResult )
{
    sdtHashScanInfo  sTRPInfo;
    sdtWCB         * sWCBPtr;
    idBool           sIsFixedPage = ID_FALSE;

    sTRPInfo.mFetchEndOffset = aHeader->mKeyEndOffset;
    
    IDU_FIT_POINT( "BUG-49107@sdtHashModule::chkUniqueInsertableSingleRow" );
    IDE_TEST( getSlotPtr( aWASegment,
                          &aWASegment->mFetchGrp,
                          aSingleRowPageID,
                          aSingleRowOffset,
                          (UChar**)&sTRPInfo.mTRPHeader,
                          &sWCBPtr )
              != IDE_SUCCESS );

    IDE_DASSERT( (sTRPInfo.mTRPHeader)->mIsRow == SDT_HASH_ROW_HEADER );
    IDE_DASSERT( SM_IS_FLAG_ON( (sTRPInfo.mTRPHeader)->mTRFlag, SDT_TRFLAG_CHILDGRID ) );
    IDE_DASSERT( SM_IS_FLAG_ON( (sTRPInfo.mTRPHeader)->mTRFlag, SDT_TRFLAG_HEAD ) );
    IDE_DASSERT( (sTRPInfo.mTRPHeader)->mChildPageID == SM_NULL_PID );

    fixWAPage( sWCBPtr );
    sIsFixedPage = ID_TRUE;

    IDE_TEST( fetch( aWASegment,
                     aHeader->mRowBuffer4Fetch,
                     &sTRPInfo )
              != IDE_SUCCESS );

    sIsFixedPage = ID_FALSE;
    unfixWAPage( sWCBPtr );

    if ( compareRowAndValue( aHeader,
                             sTRPInfo.mValuePtr,
                             aValueList ) == ID_TRUE )
    {
        /* 찾았다!, Unique Insert 할 수 없다. */
        IDE_DASSERT( aWASegment->mOpenCursorType != SMI_HASH_CURSOR_HASH_UPDATE );
        *aResult = ID_FALSE;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sIsFixedPage == ID_TRUE )
    {
        unfixWAPage( sWCBPtr );
    }

    return IDE_FAILURE;
}

/**************************************************************************
 * Description : unique violation 확인 single row일 때 확인
 *               Hash Value까지 확인하고 들어온다.
 *
 * aHeader        - [IN] 대상 Temp Table Header
 * aWASegment     - [IN] Hash Segment
 * aValueList     - [IN] 중복 키 검증 할 Value
 * aHashValue     - [IN] 중복 키 검증 할 HashValue
 * aSubHashPageID - [IN] 확인 할 Sub Hash의 page ID
 * aSubHashOffset - [IN] 확인 할 Sub Hash의 Offset
 * aResult        - [OUT] insert가능하면 IDE_TRUE, 충돌이 있으면 ID_FALSE
 ***************************************************************************/
IDE_RC sdtHashModule::chkUniqueInsertableSubHash( smiTempTableHeader  * aHeader,
                                                  sdtHashSegHdr       * aWASegment,
                                                  const smiValue      * aValueList,
                                                  UInt                  aHashValue,
                                                  scPageID              aSubHashPageID,
                                                  scOffset              aSubHashOffset,
                                                  idBool              * aResult )
{
    UInt             i;
    UInt             sLoop = 0;
    UShort           sHashHigh;
    sdtSubHash     * sSubHash;
    sdtHashScanInfo  sTRPInfo;
    sdtWCB         * sWCBPtr;
    idBool           sIsFixedPage = ID_FALSE;

    sTRPInfo.mFetchEndOffset = aHeader->mKeyEndOffset;

    /* Sub Hash에는 상위 2Byte정보만 저장 되어 있다.
     * 상위 2Byte정보값으로 비교 한다.*/
    sHashHigh = SDT_MAKE_HASH_HIGH( aHashValue );

    do
    {
        sLoop++;

        /* Read Sub hash*/
        IDE_TEST( getSlotPtr( aWASegment,
                              &aWASegment->mFetchGrp,
                              aSubHashPageID,
                              aSubHashOffset,
                              (UChar**)&sSubHash,
                              &sWCBPtr )
                  != IDE_SUCCESS );

        IDE_DASSERT( ((sdtHashTRPHdr*)sSubHash)->mIsRow != SDT_HASH_ROW_HEADER );
        IDE_DASSERT( sSubHash->mHashLow == SDT_MAKE_HASH_LOW( aHashValue ) );

        for( i = 0 ; i < sSubHash->mKeyCount ; i++ )
        {
            if ( sSubHash->mKey[i].mHashHigh == sHashHigh )
            {
                IDU_FIT_POINT( "BUG-49107@sdtHashModule::chkUniqueInsertableSubHash" );
                sLoop++;
                /* Read Row */
                IDE_TEST( getSlotPtr( aWASegment,
                                      &aWASegment->mFetchGrp,
                                      sSubHash->mKey[i].mPageID ,
                                      sSubHash->mKey[i].mOffset ,
                                      (UChar**)&sTRPInfo.mTRPHeader,
                                      &sWCBPtr )
                          != IDE_SUCCESS );

                IDE_DASSERT( SM_IS_FLAG_ON( sTRPInfo.mTRPHeader->mTRFlag, SDT_TRFLAG_CHILDGRID ) );
                IDE_DASSERT( SM_IS_FLAG_ON( sTRPInfo.mTRPHeader->mTRFlag, SDT_TRFLAG_HEAD ) );
                IDE_DASSERT( sTRPInfo.mTRPHeader->mIsRow == SDT_HASH_ROW_HEADER );
                IDE_DASSERT( sTRPInfo.mTRPHeader->mHashValue == aHashValue );

                fixWAPage( sWCBPtr );
                sIsFixedPage = ID_TRUE;

                IDE_TEST( fetch( aWASegment,
                                 aHeader->mRowBuffer4Fetch,
                                 &sTRPInfo )
                          != IDE_SUCCESS );

                sIsFixedPage = ID_FALSE;
                unfixWAPage( sWCBPtr );

                if ( compareRowAndValue( aHeader,
                                         sTRPInfo.mValuePtr,
                                         aValueList ) == ID_TRUE )
                {
                    /* 찾았다! Insert 할 수 없다. */
                    *aResult = ID_FALSE;

                    IDE_DASSERT( aWASegment->mOpenCursorType == SMI_HASH_CURSOR_NONE );
                    IDE_CONT( CONT_COMPLETE );
                }
            }
        }

        IDE_DASSERT( ( i+1 ) >= sSubHash->mKeyCount );

        /* sub hash에서 찾지 못했거나, 마지막 key에서 hit한 경우 */
        aSubHashPageID = sSubHash->mNextPageID;
        aSubHashOffset = sSubHash->mNextOffset;
    } while( aSubHashPageID != SM_NULL_PID );

    IDE_EXCEPTION_CONT( CONT_COMPLETE );

    aHeader->mStatsPtr->mExtraStat2 = IDL_MAX( aHeader->mStatsPtr->mExtraStat2, sLoop );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sIsFixedPage == ID_TRUE )
    {
        unfixWAPage( sWCBPtr );
    }

    return IDE_FAILURE;
}

/**************************************************************************
 * Description :Unique 충돌 검사용 Filter
 *
 * aHeader     - [IN] Temp Table Header
 * aRow        - [IN] 대상 Row
 * aValueList  - [IN] 비교할 Filter Data
 ***************************************************************************/
idBool sdtHashModule::compareRowAndValue( smiTempTableHeader * aHeader,
                                          const void         * aRow,
                                          const smiValue     * aValueList )
{
    smiTempColumn * sKeyColumn = NULL;
    smiValueInfo    sValue1;
    smiValueInfo    sValue2;
    UInt            sColumnCount = 0;
    idBool          sCompResult = ID_TRUE;

    sKeyColumn = aHeader->mKeyColumnList;

    IDE_DASSERT( sKeyColumn != NULL );

    while( sKeyColumn != NULL )
    {
        IDE_ASSERT( ++sColumnCount <= aHeader->mColumnCount );
        IDE_ASSERT( sKeyColumn->mIdx < aHeader->mColumnCount );

        /* PROJ-2180 valueForModule
           SMI_OFFSET_USELESS 로 비교하는 컬럼은 mBlankColumn 을 사용해야 한다.
           Compare 함수에서 valueForModule 을 호출하지 않고
           offset 을 사용할수 있기 때문이다. */
        sValue1.column = &mBlankColumn;
        sValue1.value  = aValueList[ sKeyColumn->mIdx ].value;
        sValue1.length = aValueList[ sKeyColumn->mIdx ].length;
        sValue1.flag   = SMI_OFFSET_USELESS;

        sValue2.column = &aHeader->mColumns[ sKeyColumn->mIdx ].mColumn;
        sValue2.value  = aRow;
        sValue2.length = 0;
        sValue2.flag   = SMI_OFFSET_USE;

        if ( sKeyColumn->mCompare( &sValue1,
                                   &sValue2 ) != 0 )
        {
            sCompResult = ID_FALSE;
            break;
        }
        else
        {
            /* nothing  to do */
        }
        sKeyColumn = sKeyColumn->mNextKeyColumn;
    }

    return sCompResult;
}

/**************************************************************************
 * Description : HintPage를 설정함.
 *               HintPage는 unfix되면 안되기에 고정시켜둠
 *
 * aWASegment    - [IN] 대상 WASegment
 * aHintWCBPtr   - [IN] Hint 설정 할 WCB
 ***************************************************************************/
void sdtHashModule::setInsertHintWCB( sdtHashSegHdr * aSegHdr,
                                      sdtWCB        * aHintWCBPtr )
{
    if ( aSegHdr->mInsertHintWCBPtr != aHintWCBPtr )
    {
        if ( aSegHdr->mInsertHintWCBPtr != NULL )
        {
            unfixWAPage( aSegHdr->mInsertHintWCBPtr );
        }

        aSegHdr->mInsertHintWCBPtr = aHintWCBPtr;

        if ( aHintWCBPtr != NULL )
        {
            fixWAPage( aHintWCBPtr );
        }
    }
}

/**************************************************************************
 * Description : 해당 WAPage에 Row를 삽입한다.
 *
 * aWASegment      - [IN] 대상 WASegment
 * aTRPInfo        - [IN] 삽입할 Row 및 정보
 * aTRInsertResult - [OUT] 삽입한 결과 및 정보
 ***************************************************************************/
IDE_RC sdtHashModule::append( sdtHashSegHdr        * aWASegment,
                              sdtHashInsertInfo    * aTRPInfo,
                              sdtHashInsertResult  * aTRInsertResult )
{
    sdtWCB        * sWCBPtr;

    IDE_DASSERT( SDT_HASH_TR_HEADER_SIZE_FULL ==
                 ( IDU_FT_SIZEOF( sdtHashTRPHdr, mTRFlag )
                   + IDU_FT_SIZEOF( sdtHashTRPHdr, mIsRow )
                   + IDU_FT_SIZEOF( sdtHashTRPHdr, mHitSequence )
                   + IDU_FT_SIZEOF( sdtHashTRPHdr, mValueLength )
                   + IDU_FT_SIZEOF( sdtHashTRPHdr, mHashValue )
                   + IDU_FT_SIZEOF( sdtHashTRPHdr, mNextOffset )
                   + IDU_FT_SIZEOF( sdtHashTRPHdr, mNextPageID )
                   + IDU_FT_SIZEOF( sdtHashTRPHdr, mChildOffset )
                   + IDU_FT_SIZEOF( sdtHashTRPHdr, mChildPageID )
                   + IDU_FT_SIZEOF( sdtHashTRPHdr, mChildRowPtr )
                   + IDU_FT_SIZEOF( sdtHashTRPHdr, mNextRowPiecePtr )) );

    IDE_DASSERT( SDT_HASH_TR_HEADER_SIZE_FULL  == ID_SIZEOF( sdtHashTRPHdr ) );

    idlOS::memset( aTRInsertResult, 0, ID_SIZEOF( sdtHashInsertResult ) );

    if ( aWASegment->mInsertHintWCBPtr != NULL )
    {
        sWCBPtr = aWASegment->mInsertHintWCBPtr;
    }
    else
    {
        /* 최초 삽입이면 page할당 */
        IDE_TEST( allocNewPage( aWASegment,
                                aTRPInfo,
                                &sWCBPtr )
                  != IDE_SUCCESS );

        /* 최초 삽입이면, 반드시 존재해야 한다. */
        IDE_ASSERT( sWCBPtr != NULL );
    }
    IDE_TEST( appendRowPiece( sWCBPtr,
                              aTRPInfo,
                              aTRInsertResult )
              != IDE_SUCCESS );

    while( aTRInsertResult->mComplete == ID_FALSE  )
    {
        IDE_TEST( allocNewPage( aWASegment,
                                aTRPInfo,
                                &sWCBPtr )
                  != IDE_SUCCESS );

        IDE_ASSERT( sWCBPtr != NULL );

        IDE_TEST( appendRowPiece( sWCBPtr,
                                  aTRPInfo,
                                  aTRInsertResult )
                  != IDE_SUCCESS );
    }

    setInsertHintWCB( aWASegment, sWCBPtr );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    smuUtility::dumpFuncWithBuffer( IDE_DUMP_0,
                                    dumpTempTRPInfo4Insert,
                                    aTRPInfo );

    return IDE_FAILURE;
}

/**************************************************************************
 * Description : 삽입할 새 페이지를 할당받음
 *
 * aWASegment  - [IN] Hash WASegment
 * aTRPInfo    - [IN] Insert 정보
 * aNewWCBPtr  - [OUT] 새로 할당 받은 page의 WCB
 ***************************************************************************/
IDE_RC sdtHashModule::allocNewPage( sdtHashSegHdr    * aWASegment,
                                    sdtHashInsertInfo* aTRPInfo,
                                    sdtWCB          ** aNewWCBPtr )
{
    sdtWCB     * sTargetWCBPtr;

    if (( aWASegment->mInsertGrp.mReuseSeqWPID % SDT_WAEXTENT_PAGECOUNT ) == 0 )
    {
        /* Extent 단위로 flush 하거나, Discard 여부를 확인한다. */
        IDE_TEST( checkExtentTerm4Insert( aWASegment,
                                          aTRPInfo ) != IDE_SUCCESS );
    }

    sTargetWCBPtr = getWCBPtr( aWASegment,
                               aWASegment->mInsertGrp.mReuseSeqWPID );

    aWASegment->mInsertGrp.mReuseSeqWPID++;

    IDE_ASSERT( SDT_IS_NOT_FIXED_PAGE( sTargetWCBPtr ) );

    /* update 중에 dirty page 가 발생 할 수 있다.
     * 새로 가져가려는 page가 dirty이면 Write한다. */
    if ( sTargetWCBPtr->mWPState == SDT_WA_PAGESTATE_DIRTY )
    {
        IDE_TEST( writeNPage( aWASegment,
                              sTargetWCBPtr ) != IDE_SUCCESS );
    }

    /* NPage Hash가 존재하고, 이미 사용 및 flush된 흔적이 있으면
     * NPage Hash에서 제거해 준다.*/
    if (( aWASegment->mNPageHashPtr != NULL ) &&
        ( sTargetWCBPtr->mWPState == SDT_WA_PAGESTATE_CLEAN ))
    {
        IDE_TEST( unassignNPage( aWASegment,
                                 sTargetWCBPtr )
                  != IDE_SUCCESS );
    }

    initWCB( sTargetWCBPtr );

    /* NPage를 할당해서 Binding시켜야 함 */
    IDE_TEST( allocAndAssignNPage( aWASegment,
                                   &aWASegment->mNExtFstPIDList4Row,
                                   sTargetWCBPtr )
              != IDE_SUCCESS );

    /* Row에 사용된 NPage만 Counting */
    aWASegment->mNPageCount++;

    initTempPage( (sdtHashPageHdr*)sTargetWCBPtr->mWAPagePtr,
                  SDT_TEMP_PAGETYPE_HASHROWS,
                  sTargetWCBPtr->mNPageID );/* Next */

    (*aNewWCBPtr) = sTargetWCBPtr;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**************************************************************************
 * Description : Dirty 된 page를 Write한다.
 *
 * aWASegment - [IN] Hash WASegment
 * aWCBPtr    - [IN] Dirty page의 WCB
 ***************************************************************************/
IDE_RC sdtHashModule::writeNPage( sdtHashSegHdr* aWASegment,
                                  sdtWCB       * aWCBPtr )
{
    sdtWCB    * sPrvWCBPtr ;
    sdtWCB    * sCurWCBPtr;
    sdtWCB    * sEndWCBPtr;
    UInt        sPageCount = 1 ;
    
    /* dirty인 것 만 flush queue에 들어간다.
     * 하지만 아니라도 Write하면 된다.*/
    IDE_DASSERT( aWCBPtr->mWPState == SDT_WA_PAGESTATE_DIRTY );

    /* 첫번째 위치는 무조건 self PID */
    IDE_ASSERT_MSG( aWCBPtr->mNPageID == ((sdtHashPageHdr*)aWCBPtr->mWAPagePtr)->mSelfNPID,
                    "%"ID_UINT32_FMT" != %"ID_UINT32_FMT"\n",
                    aWCBPtr->mNPageID,
                    ((sdtHashPageHdr*)aWCBPtr->mWAPagePtr)->mSelfNPID );

    sEndWCBPtr = getWCBPtr( aWASegment,
                            aWASegment->mEndWPID - 1 );

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
                 "Hash Temp WriteNPage Error\n"
                 "write page count : %"ID_UINT32_FMT"\n",
                 sPageCount );

    smuUtility::dumpFuncWithBuffer( IDE_DUMP_0,
                                    sdtHashModule::dumpWCB,
                                    aWCBPtr );

    return IDE_FAILURE;
}

/**************************************************************************
 * Description : RowPiece하나를 한 페이지에 삽입한다.
 *               Append에서 사용한다.
 *
 * aWCBPtr         - [IN] 삽입할 대상 Page의 WCB
 * aTRPInfo        - [IN] 삽입할 Row Info
 * aTRInsertResult - [OUT] 삽입한 결과
 ***************************************************************************/
IDE_RC sdtHashModule::appendRowPiece( sdtWCB              * aWCBPtr,
                                      sdtHashInsertInfo   * aTRPInfo,
                                      sdtHashInsertResult * aTRInsertResult )
{
    UInt            sRowPieceSize;
    const UInt      sRowPieceHeaderSize = SDT_HASH_TR_HEADER_SIZE_FULL ;
    UInt            sSlotSize;
    UChar         * sSlotPtr;
    UChar         * sRowPtr;
    UInt            sBeginOffset;
    UChar         * sWAPagePtr;
    sdtHashTRPHdr * sTRPHeader;

    IDE_ERROR( aWCBPtr != NULL );
    IDE_DASSERT( sRowPieceHeaderSize == ID_SIZEOF(sdtHashTRPHdr));

    sWAPagePtr = aWCBPtr->mWAPagePtr;

    /* FreeSpace계산 */
    sRowPieceSize = sRowPieceHeaderSize + aTRPInfo->mValueLength;

    sSlotSize = allocSlot( (sdtHashPageHdr*)sWAPagePtr,
                           sRowPieceSize,
                           &sSlotPtr );

    /* Slot 할당 실패 */
    IDE_TEST_CONT( sSlotSize == 0, SKIP );

    sTRPHeader = (sdtHashTRPHdr*)sSlotPtr;
    aTRInsertResult->mHeadRowpiecePtr = sTRPHeader;

    /* Row Piece Header를 먼저 복사 */
    idlOS::memcpy( sTRPHeader,
                   &aTRPInfo->mTRPHeader,
                   sRowPieceHeaderSize );

    aTRInsertResult->mHeadPageID = aWCBPtr->mNPageID;
    aTRInsertResult->mHeadOffset = sSlotPtr - sWAPagePtr;

    if ( sSlotSize == sRowPieceSize )
    {
        /* Cutting할 것도 없고, 그냥 전부 Copy하면 됨 */

        /* 마지막에 기록되는 것이 Row Header이다. */
        SM_SET_FLAG_ON( sTRPHeader->mTRFlag,
                         SDT_TRFLAG_HEAD );

        sTRPHeader->mValueLength = aTRPInfo->mValueLength;

        /* 원본 Row의 시작 위치 */
        sRowPtr = ((UChar*)aTRPInfo->mValueList[ 0 ].value)
            - aTRPInfo->mColumns[ 0 ].mColumn.offset;

        /* 삽입 완료함 */
        aTRInsertResult->mComplete = ID_TRUE;
    }
    else
    {
        IDE_DASSERT( sRowPieceSize > sSlotSize );
        sBeginOffset = sRowPieceSize - sSlotSize; /* 요구량이 부족한 만큼
                                                   * 기록하는 OffsetRange를 계산함 */
        IDE_ERROR( aTRPInfo->mValueLength > sBeginOffset );
        sTRPHeader->mValueLength = aTRPInfo->mValueLength - sBeginOffset;

        /* 남은 앞쪽 부분이 남아는 있으면,
         * 리턴할 TRPHeader에는 NextGRID를 붙이고 (다음에 연결하라고) */
        SM_SET_FLAG_ON( aTRPInfo->mTRPHeader.mTRFlag,
                        SDT_TRFLAG_NEXTGRID );

        /* 원본 Row의 시작 위치 */
        sRowPtr = ((UChar*)aTRPInfo->mValueList[ 0 ].value)
            - aTRPInfo->mColumns[ 0 ].mColumn.offset + sBeginOffset;

        aTRPInfo->mValueLength -= sTRPHeader->mValueLength;

        aTRPInfo->mTRPHeader.mNextPageID = aTRInsertResult->mHeadPageID;
        aTRPInfo->mTRPHeader.mNextOffset = aTRInsertResult->mHeadOffset;
        aTRPInfo->mTRPHeader.mNextRowPiecePtr = sTRPHeader;
    }

    /* Row Value를 복사 */
    idlOS::memcpy( sSlotPtr + sRowPieceHeaderSize,
                   sRowPtr,
                   sTRPHeader->mValueLength );

    IDE_DASSERT( aWCBPtr->mWPState != SDT_WA_PAGESTATE_INIT );
    aWCBPtr->mWPState = SDT_WA_PAGESTATE_DIRTY;

    IDE_EXCEPTION_CONT( SKIP );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**************************************************************************
 * Description : Page에서 Slot을 하나 할당받아 온다.
 *               MinSize < Return Size < NeedSize
 *               Min과 Need사이에 최대한 큰 값을 반환한다.
 *               Min보다 작으면 할당 실패,
 *
 * aPageHdr   - [IN] 삽입할 대상 Page
 * aNeedSize  - [IN] 필요한 크기
 * aRetSize   - [OUT] 할당 받은 크기
 ***************************************************************************/
UInt sdtHashModule::allocSlot( sdtHashPageHdr  * aPageHdr,
                               UInt              aNeedSize,
                               UChar          ** aSlotPtr )
{
    UInt sBeginFreeOffset = getFreeOffset( aPageHdr );
    UInt sFreeSize        = ( SD_PAGE_SIZE - sBeginFreeOffset );
    UInt sSlotSize;

    /* Align을 위한 Padding 때문에 sRetOffset이 조금 넉넉하게 될 수
     * 있음. 따라서 Min으로 줄여줌 */
    if ( sFreeSize > aNeedSize )
    {
        sFreeSize = idlOS::align8( aNeedSize );

        IDE_DASSERT( ( sFreeSize & 0x07 ) == 0  ) ;
        IDE_DASSERT( sFreeSize >= aNeedSize ) ;

        setFreeOffset( aPageHdr,
                       sBeginFreeOffset + sFreeSize );

        *aSlotPtr = (UChar*)aPageHdr + sBeginFreeOffset;

        sSlotSize = aNeedSize;
    }
    else
    {
        if ( sFreeSize >= smuProperty::getTempRowSplitThreshold() )
        {
            setFreeOffset( aPageHdr,
                           sBeginFreeOffset + sFreeSize );

            *aSlotPtr = (UChar*)aPageHdr + sBeginFreeOffset;

            sSlotSize = sFreeSize;
        }
        else
        {
            sSlotSize = 0;
        }
    }

    return sSlotSize;
}

/**************************************************************************
 * Description : 새로운 WA Extent를 할당 하지 못하였을 때 필요한 동작을 모아둠
 *               1. OutMemory 로 변경
 *               2. sub hash를 build하고, TRPInfo 를 변경,
 *               TOTAL WA가 부족한 경우는 일반적으로 잘 발생하지 않으므로
 *               때문에 따로 빼 두었다.
 *
 * aWASegment    - [IN] 대상 WASegment
 * aTRPInfo      - [IN] 현재 insert중인 row의 정보
 ***************************************************************************/
IDE_RC sdtHashModule::lackTotalWAPage( sdtHashSegHdr    * aWASegment,
                                       sdtHashInsertInfo* aTRPInfo )
{
    IDE_TEST( ideGetErrorCode() != smERR_ABORT_NOT_ENOUGH_WORKAREA );

    IDE_ASSERT( aWASegment->mInsertGrp.mReuseSeqWPID >=
                ( aWASegment->mInsertGrp.mBeginWPID + SDT_WAEXTENT_PAGECOUNT ) );

    if ( aWASegment->mIsInMemory == SDT_WORKAREA_IN_MEMORY )
    {
        if (( aWASegment->mNPageHashPtr == NULL ) &&
            ( aWASegment->mUnique == SDT_HASH_UNIQUE ))
        {
            /* Unique insert이거나 scan과 insert를 함께 하는 경우에는(이것도 unique)
             * 어느정도 차면 hash map을 만든다.반대로, unique가 아닌 normal insert 인 경우에는
             * insert완료 될 때 까지 scan이 없다. */
            IDE_TEST( buildNPageHashMap( aWASegment ) != IDE_SUCCESS );
        }
    }

    /* 더 할당 받지 못해서 첫 page로 돌아가기 전에 sub hash를 빌드 한다.
     * Sub Hash는 마지막 page까지 모두 사용 된 뒤에 구축 해야 한다. */
    IDE_TEST( buildSubHashAndResetInsertInfo( aWASegment,
                                              aTRPInfo )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**************************************************************************
 * Description : 새로운 insert용 page를 할당 하는 중 Extent단위로 실행
 * extent단위로 page를 할당하거나, extent flush 등 자주 사용되지 않는 것들을
 * 모아두었다.
 *
 * aWASegment  - [IN] 대상 WASegment
 * aTRPInfo    - [IN] 현재 insert중인 row의 정보
 ***************************************************************************/
IDE_RC sdtHashModule::checkExtentTerm4Insert( sdtHashSegHdr    * aWASegment,
                                              sdtHashInsertInfo* aTRPInfo )
{
    sdtWCB * sCurWCBPtr;
    scPageID sCurWPageID = aWASegment->mInsertGrp.mReuseSeqWPID;
    scPageID sEndWPageID = aWASegment->mInsertGrp.mEndWPID;

    if (( sCurWPageID + SDT_WAEXTENT_PAGECOUNT ) >= sEndWPageID )
    {
        if ( aWASegment->mIsInMemory == SDT_WORKAREA_IN_MEMORY )
        {
            /* 마지막 extent할당에 도달하면 NPage Hash를 만든다 */
            if (( aWASegment->mNPageHashPtr == NULL ) &&
                ( aWASegment->mUnique == SDT_HASH_UNIQUE ))
            {
                /* Unique insert이거나 scan과 insert를 함께 하는 경우에는(이것도 unique)
                 * 어느정도 차면 hash map을 만든다.반대로, unique가 아닌 normal insert 인 경우에는
                 * insert완료 될 때 까지 scan이 없다. */
                IDE_TEST( buildNPageHashMap( aWASegment ) != IDE_SUCCESS );
            }
        }
    }

    if ( sCurWPageID >= sEndWPageID )
    {
        /* insert group을 모두 사용하였다.
         * sub hash를 구성하고, 변경된 hash slot에 따라 TRPInfo를 갱신한다.*/
        IDE_TEST( buildSubHashAndResetInsertInfo( aWASegment,
                                                  aTRPInfo )
                  != IDE_SUCCESS );
    }

    if ( sCurWPageID != sEndWPageID )
    {
        sCurWCBPtr = getWCBPtr( aWASegment,
                                sCurWPageID );

        /* InMemory인지 확인하지 않고 확인해 본다.
         * TOTAL WA가 부족해서 할당 받지 못했을 경우, 여기에서 다시 확인 할당 받는다.*/
        if ( sCurWCBPtr->mWAPagePtr == NULL )
        {
            if ( set64WAPagesPtr( aWASegment,
                                  sCurWCBPtr ) != IDE_SUCCESS )
            {
                /* Insert 중에 Total WA Page가 부족하면,
                 * 있는 만큼만 사용해서 처리한다. */
                IDE_TEST( lackTotalWAPage( aWASegment,
                                           aTRPInfo ) != IDE_SUCCESS );
            }
        }
    }
    else
    {
        /* EndWPageID 까지 도달 했다면, InsertGroup에는 모든 page를 할당 한 것이다. */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**************************************************************************
 * Description : sub hash를 만들고 insert info를 갱신한다.
 *               sub hash를 만들면 hash slot의 값이 변경되기 때문에
 *               insert info도 변경 해 줘야 한다.
 *               Unique Insert의 경우에는 work area구성이 변경된다.
 *
 * aWASegment  - [IN] 대상 WASegment
 * aTRPInfo    - [IN] 현재 insert중인 row의 정보
 ***************************************************************************/
IDE_RC sdtHashModule::buildSubHashAndResetInsertInfo( sdtHashSegHdr    * aWASegment,
                                                      sdtHashInsertInfo* aTRPInfo )
{
    UInt         sPageCount;
    sdtHashSlot* sHashSlot;

    if ( aWASegment->mIsInMemory == SDT_WORKAREA_IN_MEMORY )
    {
        if ( aWASegment->mUnique == SDT_HASH_UNIQUE )
        {
            IDE_DASSERT( aWASegment->mFetchGrp.mBeginWPID == aWASegment->mFetchGrp.mEndWPID );

            /* Insert와 fetch가 동시에 이루어지며, 처음 discard 영역에 도달하였다.
             * Bfr [ Hash |     Row     | SubHash ] Row영역과 Fetch영역을
             * Aft [ Hash | Fetch | Row | SubHash ] 절반씩으로 구성 변경
             * 만약 extent가 홀수이면 fetch쪽에 조금 더 둔다. */
            sPageCount = ( aWASegment->mSubHashGrp.mBeginWPID - aWASegment->mInsertGrp.mBeginWPID ) / 2;
            sPageCount = (( sPageCount +  SDT_WAEXTENT_PAGECOUNT - 1 )
                          / SDT_WAEXTENT_PAGECOUNT
                          * SDT_WAEXTENT_PAGECOUNT ); 

            IDE_ASSERT( sPageCount % SDT_WAEXTENT_PAGECOUNT == 0 );

            aWASegment->mFetchGrp.mBeginWPID = aWASegment->mInsertGrp.mBeginWPID + sPageCount;
            aWASegment->mFetchGrp.mEndWPID   = aWASegment->mSubHashGrp.mBeginWPID;
            aWASegment->mInsertGrp.mEndWPID  = aWASegment->mFetchGrp.mBeginWPID;

            IDE_ASSERT( aWASegment->mInsertGrp.mBeginWPID  < aWASegment->mInsertGrp.mEndWPID );
            IDE_ASSERT( aWASegment->mFetchGrp.mBeginWPID   < aWASegment->mFetchGrp.mEndWPID );
            IDE_ASSERT( aWASegment->mSubHashGrp.mBeginWPID < aWASegment->mSubHashGrp.mEndWPID );
        }
        aWASegment->mIsInMemory = SDT_WORKAREA_OUT_MEMORY;
    }
    aWASegment->mInsertGrp.mReuseSeqWPID = aWASegment->mInsertGrp.mBeginWPID;

    /* 한번 모두 순환하면서 sub hash를 build한다.
       Sub Hash는 마지막 page까지 모두 사용 된 뒤에 구축 해야 한다. */
    IDE_TEST( buildSubHash( aWASegment ) != IDE_SUCCESS );

    IDE_TEST( getHashSlotPtr( aWASegment,
                              aTRPInfo->mTRPHeader.mHashValue,
                              (void**)&sHashSlot )
              != IDE_SUCCESS );

    aTRPInfo->mTRPHeader.mChildRowPtr = (sdtHashTRPHdr*)sHashSlot->mRowPtrOrHashSet;
    aTRPInfo->mTRPHeader.mChildPageID = sHashSlot->mPageID;
    aTRPInfo->mTRPHeader.mChildOffset = sHashSlot->mOffset;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/****************************************************************************
 * Description : sub hash를 만든다.
 *
 * aWASegment  - [IN] 대상 WASegment
 ***************************************************************************/
IDE_RC sdtHashModule::buildSubHash( sdtHashSegHdr * aWASegment )
{
    UInt            i;
    UInt            sLoop = 0;
    UInt            sSlotMaxCount;
    sdtWCB        * sWCBPtr;
    sdtWCB        * sEndWCBPtr;
    sdtHashSlot   * sHashSlot;
    sdtHashTRPHdr * sRowHdrPtr;
    sdtSubHash    * sSubHash;
    sdtSubHash    * sNextSubHash;
    scPageID        sSubHashPageID;
    scOffset        sSubHashOffset;
#ifdef DEBUG
    sdtHashTRPHdr * sPrvRowHdrPtr;
#endif
    // reset된 경우는 이미 할당 되어 있다.
    aWASegment->mSubHashBuildCount++;

    sWCBPtr    = getWCBPtr( aWASegment, 0 );
    sEndWCBPtr = getWCBPtr( aWASegment,
                            aWASegment->mHashSlotPageCount );

    if ( aWASegment->mSubHashWCBPtr == NULL )
    {
        /* 최초 할당 */
        IDE_TEST( getFreeWAPage( aWASegment,
                                 &aWASegment->mSubHashGrp,
                                 &aWASegment->mSubHashWCBPtr ) != IDE_SUCCESS );


        IDE_TEST( allocAndAssignNPage( aWASegment,
                                       &aWASegment->mNExtFstPIDList4SubHash,
                                       aWASegment->mSubHashWCBPtr ) != IDE_SUCCESS );

        initTempPage( (sdtHashPageHdr*)aWASegment->mSubHashWCBPtr->mWAPagePtr,
                      SDT_TEMP_PAGETYPE_SUBHASH,
                      aWASegment->mSubHashWCBPtr->mNPageID );
    }

    for( ; sWCBPtr < sEndWCBPtr ; sWCBPtr++ )
    {
        if ( sWCBPtr->mWAPagePtr == NULL )
        {
            /* 접근한 적이 없는 Page */
            continue;
        }
        sHashSlot = (sdtHashSlot*)sWCBPtr->mWAPagePtr;

        for( i = 0 ; i < SD_PAGE_SIZE/ID_SIZEOF(sdtHashSlot) ; i++ )
        {
            if ( sHashSlot[i].mRowPtrOrHashSet == (ULong)NULL )
            {
                // 0. 이번텀에 받은 row가 없다, ChildPageID가 있다 하더라도 지난텀 SubHash이다.
                continue;
            }

            if ( SDT_IS_SINGLE_ROW( sHashSlot[i].mOffset )  )
            {
                // 1개의 row만 있다. 지난텀에 만들었다.
                continue;
            }

            sRowHdrPtr = (sdtHashTRPHdr*)sHashSlot[i].mRowPtrOrHashSet;

            if (( sRowHdrPtr->mChildRowPtr == NULL ) &&
                ( sRowHdrPtr->mChildPageID == SM_NULL_PID ))
            {
                // 이번 텀에 Next 가 없는 1개의 Row가 있다.
                sHashSlot[i].mRowPtrOrHashSet = sRowHdrPtr->mHashValue;
                SDT_SET_SINGLE_ROW( sHashSlot[i].mOffset );
                continue;
            }

            sHashSlot[i].mRowPtrOrHashSet = (ULong)NULL ;

            // subHashMap을 가져온다.

            IDE_TEST( reserveSubHash( aWASegment,
                                      &aWASegment->mSubHashGrp,
                                      5,
                                      &sSubHash,
                                      &sSlotMaxCount,
                                      &sSubHashPageID,
                                      &sSubHashOffset ) != IDE_SUCCESS );

            IDE_DASSERT( sSlotMaxCount <= SDT_SUBHASH_MAX_KEYCOUNT );

            // sub hash 설정

            sSubHash->mKeyCount         = 1;
            sSubHash->mHashLow          = SDT_MAKE_HASH_LOW( sRowHdrPtr->mHashValue );
            sSubHash->mKey[0].mPageID   = sHashSlot[i].mPageID;
            sSubHash->mKey[0].mOffset   = sHashSlot[i].mOffset;
            sSubHash->mKey[0].mHashHigh = SDT_MAKE_HASH_HIGH( sRowHdrPtr->mHashValue );
            sLoop++;

            IDE_DASSERT( sSubHashPageID != SD_NULL_PID );
            IDE_DASSERT( SDT_IS_NOT_SINGLE_ROW(sSubHashOffset) );

            sHashSlot[i].mPageID = sSubHashPageID;
            sHashSlot[i].mOffset = sSubHashOffset;

            while( sRowHdrPtr->mChildRowPtr != NULL )
            {
                if ( sSubHash->mKeyCount == sSlotMaxCount )
                {
                    addSubHashPageFreeOffset( aWASegment,
                                              ((sSubHash->mKeyCount * ID_SIZEOF(sdtSubHashKey)) +
                                               SDT_SUB_HASH_SIZE));

                    // 공간 확인, 없으면 다음 page
                    IDE_TEST( reserveSubHash( aWASegment,
                                              &aWASegment->mSubHashGrp,
                                              5,
                                              &sNextSubHash,
                                              &sSlotMaxCount,
                                              &sSubHash->mNextPageID,
                                              &sSubHash->mNextOffset ) != IDE_SUCCESS );

                    IDE_DASSERT( sSlotMaxCount <= SDT_SUBHASH_MAX_KEYCOUNT );

                    sNextSubHash->mHashLow = sSubHash->mHashLow;
                    sSubHash               = sNextSubHash;
                    sSubHash->mKeyCount    = 0;
                }

                if ( SDT_IS_SINGLE_ROW( sRowHdrPtr->mChildOffset ) )
                {
                    /* 현재 page에서 밀려난 지난 텀의 row이다.
                     * child row ptr ( high_hash 0x0001 )
                     * row가 하나 뿐이어서 sub hash를 만들지 않고 넘겼다. */
                    sSubHash->mKey[sSubHash->mKeyCount].mPageID = sRowHdrPtr->mChildPageID;
                    sSubHash->mKey[sSubHash->mKeyCount].mOffset = SDT_DEL_SINGLE_FLAG_IN_OFFSET( sRowHdrPtr->mChildOffset );
                    sSubHash->mKey[sSubHash->mKeyCount].mHashHigh = SDT_MAKE_HASH_HIGH( (ULong)sRowHdrPtr->mChildRowPtr );

                    IDE_DASSERT( sSubHash->mKeyCount < SDT_SUBHASH_MAX_KEYCOUNT );

                    sSubHash->mKeyCount++;
                    sLoop++;

                    break;
                }
                else
                {
                    sSubHash->mKey[sSubHash->mKeyCount].mPageID = sRowHdrPtr->mChildPageID;
                    sSubHash->mKey[sSubHash->mKeyCount].mOffset = sRowHdrPtr->mChildOffset;
                }
#ifdef DEBUG
                sPrvRowHdrPtr = sRowHdrPtr;
#endif
                sRowHdrPtr    = sRowHdrPtr->mChildRowPtr;

                IDE_DASSERT( ( sPrvRowHdrPtr->mHashValue & 0x0000FFFF ) == ( sRowHdrPtr->mHashValue & 0x0000FFFF ));
                IDE_DASSERT( sSubHash->mHashLow == SDT_MAKE_HASH_LOW( sRowHdrPtr->mHashValue ) );

                sSubHash->mKey[sSubHash->mKeyCount].mHashHigh = SDT_MAKE_HASH_HIGH( sRowHdrPtr->mHashValue );

                IDE_DASSERT( sSubHash->mKeyCount < SDT_SUBHASH_MAX_KEYCOUNT );
                sSubHash->mKeyCount++;
                sLoop++;
            }

            addSubHashPageFreeOffset( aWASegment,
                                      ( (sSubHash->mKeyCount * ID_SIZEOF(sdtSubHashKey)) +
                                        SDT_SUB_HASH_SIZE) );

            if (( sRowHdrPtr->mChildPageID != SM_NULL_PID ) &&
                ( SDT_IS_NOT_SINGLE_ROW( sRowHdrPtr->mChildOffset ) ))
            {
                // ChildPointer는 NULL이고, ChildPageID는 있다.
                // 지난 텀의 sub hash map 혹은 row의 주소다
                sSubHash->mNextPageID = sRowHdrPtr->mChildPageID;
                sSubHash->mNextOffset = sRowHdrPtr->mChildOffset ;
            }
            else
            {
                sSubHash->mNextPageID = SM_NULL_PID;
            }
        }
    }

    IDE_DASSERT( aWASegment->mSubHashWCBPtr->mWPState != SDT_WA_PAGESTATE_INIT );
    aWASegment->mSubHashWCBPtr->mWPState = SDT_WA_PAGESTATE_DIRTY;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/****************************************************************************
 * Description : sub hash key array를 할당받아 온다.
 *
 * aWASegment       - [IN] 대상 WASegment
 * aGroup           - [IN] 구축 할 Group
 * aReqMinSlotCount - [IN] 최소 요청 크기
 * aSubHashPtr      - [OUT] 반환 SubHashArray의 Pointer
 * aSlotMaxCount    - [OUT] 반환한 Sub Hash Array의 Slot갯수
 * aPageID          - [OUT] SubHashArray가 포함된 PageID
 * aOffset          - [OUT] SubHashArray의 위치 Offset
 ***************************************************************************/
IDE_RC sdtHashModule::reserveSubHash( sdtHashSegHdr * aWASegment,
                                      sdtHashGroup  * aGroup,
                                      SInt            aReqMinSlotCount,
                                      sdtSubHash   ** aSubHashPtr,
                                      UInt          * aSlotMaxCount,
                                      scPageID      * aPageID,
                                      scOffset      * aOffset  )
{
    sdtHashPageHdr * sPageHdr;
    SInt             sSlotMaxCount;
    sdtWCB         * sPrvWCBPtr;
    idBool           sIsFixedPage = ID_FALSE;

    sPageHdr = (sdtHashPageHdr*)aWASegment->mSubHashWCBPtr->mWAPagePtr;

    IDE_ASSERT( sPageHdr->mFreeOffset <= SD_PAGE_SIZE );

    if ( aReqMinSlotCount > SDT_ALLOC_SUBHASH_MIN_KEYCOUNT )
    {
        aReqMinSlotCount = SDT_ALLOC_SUBHASH_MIN_KEYCOUNT;
    }

    sSlotMaxCount = ( SD_PAGE_SIZE - sPageHdr->mFreeOffset - SDT_SUB_HASH_SIZE )/ID_SIZEOF(sdtSubHashKey);


    if ( sSlotMaxCount < aReqMinSlotCount )
    {
        sPrvWCBPtr = aWASegment->mSubHashWCBPtr;
        fixWAPage( sPrvWCBPtr );
        sIsFixedPage = ID_TRUE;

        IDE_TEST( getFreeWAPage( aWASegment,
                                 aGroup,
                                 &aWASegment->mSubHashWCBPtr ) != IDE_SUCCESS );

        IDE_TEST( allocAndAssignNPage( aWASegment,
                                       &aWASegment->mNExtFstPIDList4SubHash,
                                       aWASegment->mSubHashWCBPtr ) != IDE_SUCCESS );

        sPageHdr = (sdtHashPageHdr*)aWASegment->mSubHashWCBPtr->mWAPagePtr;
        initTempPage( sPageHdr,
                      SDT_TEMP_PAGETYPE_SUBHASH,
                      aWASegment->mSubHashWCBPtr->mNPageID );

        *aSlotMaxCount = SDT_SUBHASH_MAX_KEYCOUNT;

        /* 두 포인터 aPageID, aOffset은 sPrvPage안에 위치하고 있다.
         * 반드시 aPageID, aOffset 기록 후에 sPrevWCBPtr을 dirty, write 해야 한다. */
        *aPageID = sPageHdr->mSelfNPID;
        *aOffset = sPageHdr->mFreeOffset ;

        sIsFixedPage = ID_FALSE;
        unfixWAPage( sPrvWCBPtr );

        IDE_DASSERT( sPrvWCBPtr->mWPState != SDT_WA_PAGESTATE_INIT );
        sPrvWCBPtr->mWPState = SDT_WA_PAGESTATE_DIRTY;
    }
    else
    {
        *aPageID = sPageHdr->mSelfNPID;
        *aOffset = sPageHdr->mFreeOffset ;

        if ( sSlotMaxCount > SDT_SUBHASH_MAX_KEYCOUNT )
        {
            *aSlotMaxCount = SDT_SUBHASH_MAX_KEYCOUNT;
        }
        else
        {
            *aSlotMaxCount = sSlotMaxCount;
        }
    }
    IDE_ASSERT( aWASegment->mSubHashWCBPtr->mNPageID == *aPageID );
    IDE_ASSERT( sPageHdr->mFreeOffset < SD_PAGE_SIZE );
    IDE_ASSERT( *aSlotMaxCount <= SDT_SUBHASH_MAX_KEYCOUNT );
    IDE_ASSERT(( sPageHdr->mFreeOffset + SDT_SUB_HASH_SIZE + (ID_SIZEOF(sdtSubHashKey)*(*aSlotMaxCount)) )
               <= SD_PAGE_SIZE );

    *aSubHashPtr = (sdtSubHash*)((UChar*)sPageHdr + sPageHdr->mFreeOffset );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sIsFixedPage == ID_TRUE )
    {
        unfixWAPage( sPrvWCBPtr );
    }

    return IDE_FAILURE;
}

/****************************************************************************
 * Description : insert가 모두 완료 된 시점에 sub hash를 모아서 merge한다.
 *               hash slot의 hash set을 small hash set에서 large hash set으로
 *               재구축하여 정확도를 높인다.
 *
 * aWASegment - [IN] 대상 WASegment
 ***************************************************************************/
IDE_RC sdtHashModule::mergeSubHash( sdtHashSegHdr* aWASegment )
{
    sdtNExtFstPIDList sSubHashNExtent;
    sdtHashSlot     * sHashSlot;
    sdtHashSlot     * sLstHashSlot;
    sdtWCB          * sEndWCBPtr;
    sdtWCB          * sWCBPtr;
    UInt              sSlotIdx;

    IDE_DASSERT( aWASegment->mSubHashWCBPtr != NULL );

    aWASegment->mSubHashWCBPtr = NULL;

    idlOS::memcpy( &sSubHashNExtent,
                   &aWASegment->mNExtFstPIDList4SubHash,
                   ID_SIZEOF(sdtNExtFstPIDList));

    idlOS::memset( &aWASegment->mNExtFstPIDList4SubHash,
                   0,
                   ID_SIZEOF(sdtNExtFstPIDList));

    aWASegment->mNExtFstPIDList4SubHash.mPageSeqInLFE = SDT_WAEXTENT_PAGECOUNT;

    IDE_TEST( getFreeWAPage( aWASegment,
                             &aWASegment->mInsertGrp,
                             &aWASegment->mSubHashWCBPtr ) != IDE_SUCCESS );

    IDE_TEST( allocAndAssignNPage( aWASegment,
                                   &aWASegment->mNExtFstPIDList4SubHash,
                                   aWASegment->mSubHashWCBPtr ) != IDE_SUCCESS );

    initTempPage( (sdtHashPageHdr*)aWASegment->mSubHashWCBPtr->mWAPagePtr,
                  SDT_TEMP_PAGETYPE_SUBHASH,
                  aWASegment->mSubHashWCBPtr->mNPageID );

    sWCBPtr    = getWCBPtr( aWASegment, 0 );
    sEndWCBPtr = getWCBPtr( aWASegment,
                            aWASegment->mHashSlotPageCount );
    sSlotIdx = 0;

    for( ; sWCBPtr < sEndWCBPtr ; sWCBPtr++ )
    {
        if ( sWCBPtr->mWAPagePtr == NULL )
        {
            sSlotIdx += SD_PAGE_SIZE/ID_SIZEOF(sdtHashSlot);
            continue;
        }
        sHashSlot    = (sdtHashSlot*)sWCBPtr->mWAPagePtr;
        sLstHashSlot = (sdtHashSlot*)(sWCBPtr->mWAPagePtr + SD_PAGE_SIZE);

        for( ; sHashSlot < sLstHashSlot ; sHashSlot++,sSlotIdx++ )
        {
            if ( sHashSlot->mPageID == SM_NULL_PID )
            {
                // 아무것도 없다
                continue;
            }

            if ( SDT_IS_SINGLE_ROW( sHashSlot->mOffset ) )
            {
                // 1개의 row만 있다. 지난텀에 만들었다.
                continue;
            }

            IDE_TEST( mergeSubHashOneSlot( aWASegment,
                                           sHashSlot,
                                           sSlotIdx ) != IDE_SUCCESS );
        }
    }

    IDE_DASSERT( aWASegment->mSubHashWCBPtr->mWPState != SDT_WA_PAGESTATE_INIT );
    aWASegment->mSubHashWCBPtr->mWPState = SDT_WA_PAGESTATE_DIRTY;

    // 정리한다. // 이전버전의 오래된 sub hash slot이다. 모두 버린다.
    sWCBPtr    = getWCBPtr( aWASegment,
                            aWASegment->mSubHashGrp.mBeginWPID );
    sEndWCBPtr = getWCBPtr( aWASegment,
                            aWASegment->mSubHashGrp.mEndWPID - 1 );
    for( ; sWCBPtr <= sEndWCBPtr ; sWCBPtr++ )
    {
        if ( sWCBPtr->mWPState == SDT_WA_PAGESTATE_INIT )
        {
            /* 이후로 모두 INIT이다. */
            IDE_DASSERT( sWCBPtr->mNPageID == SD_NULL_PID );
            break;
        }
                
        switch ( sWCBPtr->mWPState )
        {
            case SDT_WA_PAGESTATE_DIRTY:
                /* unassignNPage()는 Clean 만 받아들인다.
                 * Dirty이면 Clean으로 state를 변경한다. */
                sWCBPtr->mWPState = SDT_WA_PAGESTATE_CLEAN;
            case SDT_WA_PAGESTATE_CLEAN:
                IDE_TEST( unassignNPage( aWASegment,
                                         sWCBPtr )
                          != IDE_SUCCESS );
                initWCB( sWCBPtr );
                break;
            default:
                break;
        }

        IDE_DASSERT( sWCBPtr->mNPageID == SD_NULL_PID );
        IDE_DASSERT( sWCBPtr->mWPState == SDT_WA_PAGESTATE_INIT );
    }
#ifdef DEBUG
    /* 아직 사용되지 않은 부분은 모두 INIT이어야 한다. */
    for( sWCBPtr++ ; sWCBPtr <= sEndWCBPtr ; sWCBPtr++ )
    {
        IDE_ASSERT( sWCBPtr->mWPState == SDT_WA_PAGESTATE_INIT );
    }
#endif

    IDE_TEST( sdtWAExtentMgr::freeAllNExtents( aWASegment->mSpaceID,
                                               &sSubHashNExtent )
              != IDE_SUCCESS );
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***************************************************************************
 * Description : sub hash를 정렬 (BUG-48235)
 *
 * aSubHashArray  - [IN] Sub Hash Key Array
 * aLeft          - [IN] quick sort left
 * aRight         - [IN] quick sort Right
 ***************************************************************************/
void sdtHashModule::quickSortSubHash( sdtSubHashKey aSubHashArray[], SInt aLeft, SInt aRight )
{
    SInt      i     = aLeft;
    SInt      j     = aRight;
    UShort    pivot = aSubHashArray[ ( aLeft + aRight ) / 2 ].mHashHigh;
    ULong     temp;

    do
    {
        while ( aSubHashArray[i].mHashHigh < pivot )
        {
            i++;
        }
        while ( aSubHashArray[j].mHashHigh > pivot )
        {
            j--;
        }

        if ( i <= j )
        {
            if( i != j )
            {
                temp = *(ULong*)(aSubHashArray+i);
                *(ULong*)(aSubHashArray+i) = *(ULong*)(aSubHashArray+j);
                *(ULong*)(aSubHashArray+j) = temp;
            }
            i++;
            j--;
        }
    } while ( i <= j );

    /* recursion */
    if ( aLeft < j )
    {
        quickSortSubHash( aSubHashArray, aLeft, j );
    }

    if ( i < aRight )
    {
        quickSortSubHash( aSubHashArray, i, aRight );
    }
}

/****************************************************************************
 * Description : 1개의 hash slot에 연결되어있는 sub hash 들을 merge 한다.
 *
 * aWASegment - [IN] 대상 WASegment
 * aHashSlot  - [IN] 대상 HashSlot
 * aSlotIdx   - [IN] 대상 Hash Slot의 Index
 ***************************************************************************/
IDE_RC sdtHashModule::mergeSubHashOneSlot( sdtHashSegHdr* aWASegment,
                                           sdtHashSlot  * aHashSlot,
                                           UInt           aSlotIdx )
{
    sdtSubHash      * sNewSubHash;
    sdtSubHash      * sOldSubHash;
    sdtWCB          * sOldSubWCBPtr;
    idBool            sIsFixed = ID_FALSE;
    UInt              sSlotMaxCount;
    SInt              sCopiedKeyCount;
    SInt              sRemainKeyCount;
    UChar             sHashLow;
    UInt              i;

    aHashSlot->mRowPtrOrHashSet = 0;

    IDE_TEST( getSlotPtr( aWASegment,
                          &aWASegment->mSubHashGrp,
                          aHashSlot->mPageID,
                          aHashSlot->mOffset,
                          (UChar**)&sOldSubHash,
                          &sOldSubWCBPtr )
              != IDE_SUCCESS );

    fixWAPage( sOldSubWCBPtr );
    sIsFixed = ID_TRUE;

    sCopiedKeyCount = 0;
    sRemainKeyCount = sOldSubHash->mKeyCount;

    sHashLow = sOldSubHash->mHashLow;

    IDE_ASSERT_MSG( SDT_MAKE_HASH_LOW( aSlotIdx ) == sHashLow,
                    "%"ID_UINT32_FMT" %"ID_UINT32_FMT" %"ID_UINT32_FMT", "
                    "%"ID_UINT32_FMT" %"ID_UINT32_FMT" %"ID_UINT32_FMT"\n",     
                    aSlotIdx,
                    SDT_MAKE_HASH_LOW( aSlotIdx ),
                    sHashLow,
                    sOldSubHash->mKeyCount,
                    sOldSubHash->mNextPageID,
                    sOldSubHash->mNextOffset );

    IDE_TEST( reserveSubHash( aWASegment,
                              &aWASegment->mInsertGrp,
                              20,
                              &sNewSubHash,
                              &sSlotMaxCount,
                              &aHashSlot->mPageID,
                              &aHashSlot->mOffset ) != IDE_SUCCESS );

    sNewSubHash->mHashLow  = sHashLow;
    sNewSubHash->mKeyCount = 0;

    while(1)
    {
        while( sRemainKeyCount > 0 )
        {
            if ( sRemainKeyCount <= (SInt)sSlotMaxCount )
            {
                idlOS::memcpy( sNewSubHash->mKey + sNewSubHash->mKeyCount,
                               sOldSubHash->mKey + sCopiedKeyCount,
                               ID_SIZEOF( sdtSubHashKey ) * sRemainKeyCount );

                // 아직 빈 new slot이 남아있다.
                sSlotMaxCount          -= sRemainKeyCount ;
                sNewSubHash->mKeyCount += sRemainKeyCount ;
                // sCopiedKeyCount += sRemainKeyCount;
                sRemainKeyCount  = 0;
                break;
            }
            else // ( sRemainKeyCount >= sSlotMaxCount )
            {
                idlOS::memcpy( sNewSubHash->mKey + sNewSubHash->mKeyCount ,
                               sOldSubHash->mKey + sCopiedKeyCount ,
                               ID_SIZEOF( sdtSubHashKey ) * sSlotMaxCount );

                sNewSubHash->mKeyCount += sSlotMaxCount ;
                sCopiedKeyCount        += sSlotMaxCount;
                sRemainKeyCount        -= sSlotMaxCount;
                // new sub hash의 key를 모두 사용하였다.
                // 새로 할당 받아서 채워 넣어야 한다.

                if (( sRemainKeyCount > 0 ) ||
                    ( sOldSubHash->mNextPageID != SM_NULL_PID ))
                {
                    addSubHashPageFreeOffset( aWASegment,
                                              ((sNewSubHash->mKeyCount * ID_SIZEOF(sdtSubHashKey)) +
                                               SDT_SUB_HASH_SIZE));

                    for( i = 0 ; i < sNewSubHash->mKeyCount ; i++ )
                    {
                        SDT_ADD_LARGE_HASHSET( aHashSlot, sNewSubHash->mKey[i].mHashHigh );
                    }
                    quickSortSubHash( sNewSubHash->mKey, 0, sNewSubHash->mKeyCount -1 );
                    IDE_TEST( reserveSubHash( aWASegment,
                                              &aWASegment->mInsertGrp,
                                              20,
                                              &sNewSubHash,
                                              &sSlotMaxCount,
                                              &sNewSubHash->mNextPageID,
                                              &sNewSubHash->mNextOffset ) != IDE_SUCCESS );

                    sNewSubHash->mKeyCount = 0;
                    sNewSubHash->mHashLow  = sHashLow;
                }
            }
        }

        if ( sOldSubHash->mNextPageID == SM_NULL_PID )
        {
            break;
        }

        // slot을 하나 읽어와서 처리
        sIsFixed = ID_FALSE;
        unfixWAPage( sOldSubWCBPtr );

        IDU_FIT_POINT( "BUG-49107@sdtHashModule::mergeSubHashOneSlot" );
        IDE_TEST( getSlotPtr( aWASegment,
                              &aWASegment->mSubHashGrp,
                              sOldSubHash->mNextPageID ,
                              sOldSubHash->mNextOffset ,
                              (UChar**)&sOldSubHash,
                              &sOldSubWCBPtr )
                  != IDE_SUCCESS );

        fixWAPage( sOldSubWCBPtr );
        sIsFixed = ID_TRUE;

        IDE_ASSERT_MSG( sOldSubHash->mHashLow == sHashLow,
                        "%"ID_UINT32_FMT" %"ID_UINT32_FMT" %"ID_UINT32_FMT", "
                        "%"ID_UINT32_FMT" %"ID_UINT32_FMT" %"ID_UINT32_FMT" %"ID_UINT32_FMT"\n",
                        aSlotIdx,
                        SDT_MAKE_HASH_LOW( aSlotIdx ),
                        sHashLow,
                        sOldSubHash->mHashLow,
                        sOldSubHash->mKeyCount,
                        sOldSubHash->mNextPageID,
                        sOldSubHash->mNextOffset );

        sCopiedKeyCount = 0;
        sRemainKeyCount = sOldSubHash->mKeyCount;
    }

    for( i = 0 ; i < sNewSubHash->mKeyCount ; i++ )
    {
        SDT_ADD_LARGE_HASHSET( aHashSlot, sNewSubHash->mKey[i].mHashHigh );
    }
    quickSortSubHash( sNewSubHash->mKey, 0, sNewSubHash->mKeyCount -1 );

    IDE_ASSERT( sRemainKeyCount == 0 );
    IDE_ASSERT( sOldSubHash->mNextPageID == SM_NULL_PID );

    sIsFixed = ID_FALSE;
    unfixWAPage( sOldSubWCBPtr );

    addSubHashPageFreeOffset( aWASegment,
                              ((sNewSubHash->mKeyCount * ID_SIZEOF(sdtSubHashKey)) + SDT_SUB_HASH_SIZE));

    sNewSubHash->mNextPageID = SM_NULL_PID;
    sNewSubHash->mNextOffset = 0;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sIsFixed == ID_TRUE )
    {
        unfixWAPage( sOldSubWCBPtr );
    }

    return IDE_FAILURE;
}
/***************************************************************************
 * Description : NPageHash를 뒤져서 해당 NPage를 가진 WPage를 찾아냄
 *
 * aWASegment  - [IN] 대상 WASegment
 ***************************************************************************/
IDE_RC sdtHashModule::buildNPageHashMap( sdtHashSegHdr* aWASegment )
{
    sdtWCB * sWCBPtr;
    sdtWCB * sEndWCBPtr;
    UInt     sHashValue;
    idBool   sIsLock  = ID_FALSE;

    /* Null 일 때에만 호출됨, 어차피 자주 호출 되진 않으므로 한번 더 검사한다.*/
    if ( aWASegment->mNPageHashPtr == NULL )
    {
        lock();
        sIsLock = ID_TRUE;

        aWASegment->mNPageHashBucketCnt = mNHashMapPool.getDataSize()/ID_SIZEOF(sdtWCB*);

        IDE_TEST( mNHashMapPool.pop( (UChar**)&(aWASegment->mNPageHashPtr) ) != IDE_SUCCESS );
        IDE_DASSERT( aWASegment->mNPageHashPtr != NULL );

        sIsLock = ID_FALSE;
        unlock();

#ifdef DEBUG
        for( UInt i = 0 ; i < aWASegment->mNPageHashBucketCnt ; i++ )
        {
            /* 할당받은 NPageHashMap이 모두 Null인지 확인 */
            IDE_ASSERT( aWASegment->mNPageHashPtr[i] == NULL );
        }
#endif
        sWCBPtr    = getWCBPtr( aWASegment, aWASegment->mInsertGrp.mBeginWPID );
        sEndWCBPtr = getWCBPtr( aWASegment, aWASegment->mEndWPID - 1 );

        for( ; sWCBPtr <= sEndWCBPtr ; sWCBPtr++ )
        {
            if ( sWCBPtr->mNPageID != SM_NULL_PID )
            {
                IDE_DASSERT( sWCBPtr->mNextWCB4Hash == NULL );

                sHashValue = getNPageHashValue( aWASegment,
                                                sWCBPtr->mNPageID );

                sWCBPtr->mNextWCB4Hash = aWASegment->mNPageHashPtr[ sHashValue ] ;
#ifdef DEBUG
                if ( sWCBPtr->mNextWCB4Hash  != NULL )
                {
                    /* 앞서 있던 WCB가 있다면 그 WCB의 NPageID의 Hash값도 같아야 한다. */
                    IDE_DASSERT( getNPageHashValue( aWASegment, sWCBPtr->mNextWCB4Hash->mNPageID )
                                 == sHashValue );
                }
#endif
                aWASegment->mNPageHashPtr[ sHashValue ] = sWCBPtr;
            }
        }
        aWASegment->mStatsPtr->mIOPassNo++;
        aWASegment->mStatsPtr->mRuntimeMemSize += mNHashMapPool.getNodeSize( (UChar*)aWASegment->mNPageHashPtr );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sIsLock == ID_TRUE )
    {
        unlock();
        sIsLock = ID_FALSE;
    }

    /* 예외가 발생 하였다면, 함수 진입 전에 NULL이었을 것이다.*/
    if ( aWASegment->mNPageHashPtr != NULL )
    {
        /* Push전에 모두 초기화 되어 있어야 한다.
         * 예외적인 상황이니 간단하게 Memset으로 처리 */
        idlOS::memset( aWASegment->mNPageHashPtr,
                       0,
                       aWASegment->mNPageHashBucketCnt * ID_SIZEOF(sdtWCB*) );
        IDE_ASSERT( mNHashMapPool.push( (UChar*)(aWASegment->mNPageHashPtr) )
                    != IDE_SUCCESS );
        aWASegment->mNPageHashPtr = NULL;
        aWASegment->mNPageHashBucketCnt = 0;
    }

    return IDE_FAILURE;
}

/***************************************************************************
 * Description : cursor alloc 시점에 Insert를 마무리 하는 작업을 한다.
 *               Insert End가 따로 없으므로, Fetch 시작 시점에 정리해준다.
 *               최초 cursor open 시에 sub hash 를 재구축 하거나,
 *               Insert 후 남은 Dirty Page를 flush하거나,
 *               NPageHashMap이 없다면 구축하는 역할도 한다.
 *
 * aCursor     - [IN] open한 cursor
 * aCursorType - [IN] open한 cursor의 Type ()
 ***************************************************************************/
IDE_RC sdtHashModule::initHashCursor( smiHashTempCursor  * aCursor,
                                      UInt                 aCursorType )
{
    sdtHashSegHdr * sWASegment = (sdtHashSegHdr*)aCursor->mWASegment;
    sdtWCB        * sWCBPtr;
    sdtWCB        * sEndWCBPtr;

    IDE_DASSERT( aCursorType != SMI_HASH_CURSOR_NONE );

    /* check Hash Slot에서 WASegment에 접근하지 않고 빠르게 처리 하기 위해
     * Cursor에 복사해둔다. */
    aCursor->mIsInMemory    = sWASegment->mIsInMemory;
    aCursor->mHashSlotCount = sWASegment->mHashSlotCount;

    if (( sWASegment->mIsInMemory == SDT_WORKAREA_OUT_MEMORY ) &&
        ( sWASegment->mNPageHashPtr == NULL ))
    {
        /* Normal Insert 할 때에는 Insert중 NPageHashMap을 만들지 않는다.
         * Cursor Open시점에 NPageHashMap을 만들어 줘야한다.
         * InMemory 에서는 NPageHashMap을 사용하지 않는다.*/
        IDE_TEST( buildNPageHashMap( sWASegment ) != IDE_SUCCESS );
    }

    if ( aCursorType == SMI_HASH_CURSOR_HASH_UPDATE )
    {
        /* Update Cursor는 Insert, Fetch, Update가 동시에 이루어지므로 예외적으로 사용 */
        sWASegment->mOpenCursorType  = SMI_HASH_CURSOR_HASH_UPDATE;
        aCursor->mTTHeader->mTTState = SMI_TTSTATE_HASH_FETCH_UPDATE;
    }

    /* Update용 Cursor가 아니고, OutMemory이고,
     * 처음으로 Cursor를 여는 상황이다. */
    if (( sWASegment->mIsInMemory == SDT_WORKAREA_OUT_MEMORY ) &&
        ( sWASegment->mOpenCursorType == SMI_HASH_CURSOR_NONE ))
    {
        /* Insert가 끝났다. Hint 제거 */
        setInsertHintWCB( sWASegment, NULL );

        /* 마지막 Extent를 Flush한다.
         * ReuseWPID %1~63 = 0~62 Write 즉 0부터 ReuseWPID - 1 까지 Write
         * ReuseWPID %0    = -64 ~ -1 - Insert중 Write시점이 AllocPage 이므로,
         *                 그 때 Write를 했다면 ReuseWPID는 0을 할당하고 1이 되었을 것이다.
         *                 0이라는 것은 Write직전인 경우로 Extent단위로 Write해 줘야 한다. */
        if (( sWASegment->mInsertGrp.mReuseSeqWPID % SDT_WAEXTENT_PAGECOUNT ) != 0 )
        {
            sWCBPtr = getWCBPtr( sWASegment,
                                 sWASegment->mInsertGrp.mReuseSeqWPID -
                                 ( sWASegment->mInsertGrp.mReuseSeqWPID %
                                   SDT_WAEXTENT_PAGECOUNT ) );
        }
        else
        {
            /* ReuseSeqWPID % Extent가 0 인 경우에 ReuseSeqWPID 가 BeginWPID와 같아서
             * - ExtentCount 하면 BeginWPID 보다 앞이 되지 않는가 하는 우려에 대한 설명,
             * 한 건도 Insert되지 않은 경우를 제외하고는 이 시점에 ReuseWPID == BeginWPID인 경우는 없다.
             *   (EndWPID에 도달하여 BeginWPID 대입해서 page할당 그리고 +1 세팅 까지 한번에 한다. )
             * 그리고 한 건도 Insert되지 않았다면, OutMemory가 아니었을 것이다.
             * 그러므로 ReuseSeqWPID % Extent가 0 인 경우는 BeginWPID 이지 않으므로,
             * ReuseSeqWPID - ExtentCount 하여도 BeginWPID 앞으로 넘어가는 경우가 아니다.*/
            sWCBPtr = getWCBPtr( sWASegment,
                                 sWASegment->mInsertGrp.mReuseSeqWPID -
                                 SDT_WAEXTENT_PAGECOUNT );  
        }
        sEndWCBPtr = getWCBPtr( sWASegment,
                                sWASegment->mInsertGrp.mReuseSeqWPID );

        for( ; sWCBPtr < sEndWCBPtr ; sWCBPtr++ )
        {
            if ( sWCBPtr->mWPState == SDT_WA_PAGESTATE_DIRTY )
            {
                IDE_TEST( writeNPage( sWASegment,
                                      sWCBPtr ) != IDE_SUCCESS );
            }
        }

        /* 아직 만들어지 지지 않은 마지막 In Memory상의 Row의 SubHash를 만들고 */
        IDE_TEST( buildSubHash( sWASegment ) != IDE_SUCCESS );

        /* 만들어둔 SubHash를 모두 Merge한다. */
        IDE_TEST( mergeSubHash( sWASegment ) != IDE_SUCCESS );

        /* BUG-48313 mergeSubHash가 두번 호출되는 문제 해결 */
        sWASegment->mOpenCursorType = SMI_HASH_CURSOR_INIT;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***************************************************************************
 * Description : cursor open시점에 만약 HashScan 하던 것이 아니라면
 *               Hash Work Area를 Hash 탐색하기 좋게 조정 한다.
 *               Hash Scan Cursor는 자주 반복적으로 Open하므로,
 *               최초 1번 Cursor Open시점에 조정 해 줄 필요가 있다.
 *
 * aWASegment  - [IN] 대상 WASegment
 ***************************************************************************/
void sdtHashModule::initToHashScan( sdtHashSegHdr* aWASegment )
{
    UInt    sAllExtCount;
    UInt    sFetchRowExtCount;
    UInt    sSubHashExtCount;

    setInsertHintWCB( aWASegment, NULL );

    sAllExtCount = ( aWASegment->mEndWPID - aWASegment->mHashSlotPageCount )
        / SDT_WAEXTENT_PAGECOUNT;

    if ( aWASegment->mNExtFstPIDList4SubHash.mCount < sAllExtCount )
    {
        sSubHashExtCount = aWASegment->mNExtFstPIDList4SubHash.mCount;
        sFetchRowExtCount = sAllExtCount - sSubHashExtCount;
    }
    else
    {
        if ( aWASegment->mStatsPtr->mIOPassNo < 2 )
        {
            aWASegment->mStatsPtr->mIOPassNo++;
        }

        sSubHashExtCount = sAllExtCount * smuProperty::getTmpHashFetchSubHashMaxRatio() / 100 ;
        if ( sSubHashExtCount == 0 )
        {
            sSubHashExtCount++; // 최소 한개
        }
        else if( sSubHashExtCount == sAllExtCount )
        {
            sSubHashExtCount--; // fetch영역도 있어야 한다. 
        }
        sFetchRowExtCount = sAllExtCount - sSubHashExtCount ;
    }

    aWASegment->mSubHashGrp.mBeginWPID    = aWASegment->mHashSlotPageCount;
    aWASegment->mSubHashGrp.mReuseSeqWPID = aWASegment->mHashSlotPageCount;
    aWASegment->mSubHashGrp.mEndWPID      = aWASegment->mHashSlotPageCount;
    aWASegment->mSubHashGrp.mEndWPID     += sSubHashExtCount * SDT_WAEXTENT_PAGECOUNT;

    aWASegment->mFetchGrp.mBeginWPID    = aWASegment->mSubHashGrp.mEndWPID;
    aWASegment->mFetchGrp.mReuseSeqWPID = aWASegment->mSubHashGrp.mEndWPID;
    aWASegment->mFetchGrp.mEndWPID      = aWASegment->mSubHashGrp.mEndWPID;
    aWASegment->mFetchGrp.mEndWPID     += sFetchRowExtCount * SDT_WAEXTENT_PAGECOUNT;

    IDE_DASSERT( aWASegment->mFetchGrp.mEndWPID == aWASegment->mEndWPID );

    aWASegment->mInsertGrp.mBeginWPID    = aWASegment->mHashSlotPageCount;
    aWASegment->mInsertGrp.mReuseSeqWPID = aWASegment->mHashSlotPageCount;
    aWASegment->mInsertGrp.mEndWPID      = aWASegment->mHashSlotPageCount;

    return ;
}

/***************************************************************************
 * Description : cursor open시점에 만약 FullScan 하던 것이 아니라면
 *               Hash Work Area를 Full 탐색하기 좋게 조정 한다.
 *               FullScan Cursor Open은 처음 한번만 하므로,
 *               Cursor Open에서 함께 하면 되긴 하지만,
 *               
 * aWASegment  - [IN] 대상 WASegment
 ***************************************************************************/
void sdtHashModule::initToFullScan( sdtHashSegHdr* aWASegment )
{
    setInsertHintWCB( aWASegment, NULL );

    aWASegment->mFetchGrp.mBeginWPID = aWASegment->mHashSlotPageCount;
    aWASegment->mFetchGrp.mEndWPID   = aWASegment->mEndWPID;

    if ( aWASegment->mOpenCursorType == SMI_HASH_CURSOR_INIT )
    {
        /* 직전까지 insert중이었다. */
        aWASegment->mFetchGrp.mReuseSeqWPID = aWASegment->mInsertGrp.mReuseSeqWPID;
    }
    else
    {
        IDE_ASSERT( aWASegment->mOpenCursorType != SMI_HASH_CURSOR_NONE );

        /* 직전까지 hash scan등을 했었다.
         * 이전 fetch reuse를 그대로 놔 두어도 된다.*/
    }

    if ( aWASegment->mFetchGrp.mReuseSeqWPID >= aWASegment->mFetchGrp.mEndWPID )
    {
        aWASegment->mFetchGrp.mReuseSeqWPID = aWASegment->mFetchGrp.mBeginWPID;
    }

    IDE_DASSERT( aWASegment->mFetchGrp.mBeginWPID <= aWASegment->mFetchGrp.mReuseSeqWPID );
    IDE_DASSERT( aWASegment->mFetchGrp.mEndWPID   >  aWASegment->mFetchGrp.mReuseSeqWPID );

    aWASegment->mSubHashGrp.mBeginWPID    = aWASegment->mEndWPID;
    aWASegment->mSubHashGrp.mReuseSeqWPID = aWASegment->mEndWPID;
    aWASegment->mSubHashGrp.mEndWPID      = aWASegment->mEndWPID;

    aWASegment->mInsertGrp.mBeginWPID    = aWASegment->mEndWPID;
    aWASegment->mInsertGrp.mReuseSeqWPID = aWASegment->mEndWPID;
    aWASegment->mInsertGrp.mEndWPID      = aWASegment->mEndWPID;

    return ;
}

/***************************************************************************
 * Description : FullScan Cursor를 엽니다.
 *
 * aHeader   - [IN] 대상 Table
 * aCursor   - [OUT] 반환값
 * aRowGRID  - [OUT] Row의 GRID
 ***************************************************************************/
IDE_RC sdtHashModule::openFullScanCursor( smiHashTempCursor  * aCursor,
                                          UChar              ** aRow,
                                          scGRID              * aRowGRID )
{
    sdtHashSegHdr * sWASegment;
    /* Row의 위치 : page를 fix 하지 않으므로 바뀔수 있어서 사용시 주의 필요 */

    sWASegment = (sdtHashSegHdr*)aCursor->mWASegment;

    sWASegment->mOpenCursorType = SMI_HASH_CURSOR_FULL_SCAN;

    aCursor->mRowPtr      = NULL;
    aCursor->mChildRowPtr = NULL;

    aCursor->mSeq = 0;

    aCursor->mTTHeader->mTTState = SMI_TTSTATE_HASH_FETCH_FULLSCAN;

    // 두가지
    // 1. w page만 사용하였다면 w page 만 사용
    // 2. n page 도 사용 하였다면 n page 기준,

    aCursor->mChildOffset  = ID_SIZEOF(sdtHashPageHdr);

    if ( aCursor->mIsInMemory == SDT_WORKAREA_IN_MEMORY )
    {
        IDE_DASSERT( sWASegment->mInsertGrp.mReuseSeqWPID >= sWASegment->mInsertGrp.mBeginWPID );

        aCursor->mWCBPtr    = getWCBPtr( sWASegment,
                                         sWASegment->mInsertGrp.mBeginWPID );

        aCursor->mWAPagePtr = ((sdtWCB*)aCursor->mWCBPtr)->mWAPagePtr;

        if ( aCursor->mWAPagePtr != NULL )
        {
            /* beginWPID로 받은 WCB의 page ptr이 Null 이면,
             * 1개의 row도 insert되지 않았다는 것이다.
             * 그러면 end wcb ptr도 필요없다.*/
            aCursor->mEndWCBPtr = getWCBPtr( sWASegment,
                                             sWASegment->mInsertGrp.mReuseSeqWPID ) ;
        }

        aCursor->mChildPageID = ((sdtWCB*)aCursor->mWCBPtr)->mNPageID;
    }
    else
    {
        initToFullScan( sWASegment );

        aCursor->mSeq           = 0;
        aCursor->mChildPageID   = sWASegment->mNExtFstPIDList4Row.mHead->mMap[0];
        aCursor->mCurNExtentArr = sWASegment->mNExtFstPIDList4Row.mHead;
        aCursor->mNExtLstPID    = aCursor->mChildPageID + SDT_WAEXTENT_PAGECOUNT - 1;
        aCursor->mSeq++;

        aCursor->mWCBPtr = findWCB( sWASegment, aCursor->mChildPageID );

        if ( aCursor->mWCBPtr == NULL )
        {
            /* Sub hash group등에 아직 할당받지 않은 wa page가 있을 수 있다. */
            IDE_TEST( getFreeWAPage( sWASegment,
                                     &sWASegment->mFetchGrp,
                                     (sdtWCB**)&(aCursor->mWCBPtr) )
                      != IDE_SUCCESS );

            IDE_TEST( readNPage( sWASegment,
                                 (sdtWCB*)aCursor->mWCBPtr,
                                 aCursor->mChildPageID )
                      != IDE_SUCCESS );
        }

        aCursor->mWAPagePtr = ((sdtWCB*)aCursor->mWCBPtr)->mWAPagePtr;

        IDE_TEST( preReadNExtents( sWASegment,
                                   aCursor->mChildPageID,
                                   SDT_WAEXTENT_PAGECOUNT )
                  != IDE_SUCCESS );
    }

    IDE_TEST( fetchFullNext( aCursor,
                             aRow,
                             aRowGRID) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***************************************************************************
 * Description : HashScan Cursor의 ChildPageID를 기준으로 Row를 1개 가져온다..
 *               Single Row용으로 HashValue를 이미 확인 했다.
 *
 * aCursor   - [IN]  탐색에 사용중인 Hash Cursor
 * aRow      - [OUT] 찾은 Row
 * aRowGRID  - [OUT] 찾은 Row의 GRID
 ***************************************************************************/
IDE_RC sdtHashModule::fetchHashScanInternalGRID( smiHashTempCursor * aCursor,
                                                 UChar            ** aRow,
                                                 scGRID            * aRowGRID )
{
    idBool           sResult = ID_FALSE;
    sdtHashTRPHdr  * sTRPHeader;
    sdtWCB         * sWCBPtr = NULL;
    idBool           sIsFixedPage = ID_FALSE;

    IDU_FIT_POINT( "BUG-49107@sdtHashModule::fetchHashScanInternalGRID" );
    IDE_TEST( getSlotPtr( (sdtHashSegHdr*)aCursor->mWASegment,
                          &((sdtHashSegHdr*)aCursor->mWASegment)->mFetchGrp,
                          aCursor->mChildPageID ,
                          aCursor->mChildOffset ,
                          (UChar**)&sTRPHeader,
                          &sWCBPtr )
              != IDE_SUCCESS );

    IDE_ASSERT( sTRPHeader->mHashValue == aCursor->mHashValue );

    //  읽어온 값은 SubHash일 수도있고, row 일수도 있다.
    IDE_ASSERT ( sTRPHeader->mIsRow == SDT_HASH_ROW_HEADER );
    // Row를 읽어온 경우

    fixWAPage( sWCBPtr );
    sIsFixedPage = ID_TRUE;

    IDE_TEST( filteringAndFetch( aCursor,
                                 sTRPHeader,
                                 *aRow,
                                 &sResult )
              != IDE_SUCCESS );

    sIsFixedPage = ID_FALSE;
    unfixWAPage( sWCBPtr );

    IDE_DASSERT( sTRPHeader->mChildPageID == SM_NULL_PID );

    if ( sResult == ID_TRUE )
    {
        SC_MAKE_GRID( *aRowGRID,
                      aCursor->mTTHeader->mSpaceID,
                      aCursor->mChildPageID,
                      aCursor->mChildOffset );
    }
    else
    {
        sWCBPtr = NULL;
        *aRow   = NULL;
        SC_MAKE_NULL_GRID(*aRowGRID);
    }

    aCursor->mWCBPtr      = sWCBPtr;
    aCursor->mChildPageID = SM_NULL_PID;
    aCursor->mChildOffset = 0;

    aCursor->mTTHeader->mStatsPtr->mExtraStat2 = IDL_MAX( aCursor->mTTHeader->mStatsPtr->mExtraStat2, 1 );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sIsFixedPage == ID_TRUE )
    {
        unfixWAPage( sWCBPtr );
    }

    smiTempTable::checkAndDump( aCursor->mTTHeader );

    return IDE_FAILURE;
}

/***************************************************************************
 * Description : sub hash에서 hash의 시작 부분을 탐색 (BUG-48235)
 *
 * aSubHash   - [IN] Sub Hash Node
 * aHashValue - [IN] 찾을 hash value
 ***************************************************************************/
UInt sdtHashModule::bSearchSubHash( sdtSubHash * aSubHash, UInt aHashValue )
{
    sdtSubHashKey * sSubHashArray = aSubHash->mKey;
    SInt            sMid          = SDT_SUBHASH_MAX_KEYCOUNT ;
    SInt            sFirst        = 0;
    SInt            sLast         = aSubHash->mKeyCount - 1;

    while( sFirst <= sLast )
    {
        sMid = ( sFirst + sLast ) / 2;

        if (( aHashValue == sSubHashArray[ sMid ].mHashHigh ) &&
            (( sFirst == sMid ) || ( aHashValue != sSubHashArray[ sMid-1 ].mHashHigh )))
        {
            break;
        }

        if ( aHashValue > sSubHashArray[ sMid ].mHashHigh )
        {
            sFirst = sMid + 1;
        }
        else
        {
            sLast = sMid - 1 ;
        }
    }

    return sMid;
}


/***************************************************************************
 * Description : HashScan Cursor의 ChildPageID를 기준으로 SubHash를 탐색해서
 *               Next Row를 가져온다.
 *
 * aCursor   - [IN]  탐색에 사용중인 Hash Cursor
 * aRow      - [OUT] 찾은 Row
 * aRowGRID  - [OUT] 찾은 Row의 GRID
 ***************************************************************************/
IDE_RC sdtHashModule::fetchHashScanInternalSubHash( smiHashTempCursor * aCursor,
                                                    UChar            ** aRow,
                                                    scGRID            * aRowGRID )
{
    idBool           sResult = ID_FALSE;
    idBool           sIsSorted = ((sdtHashSegHdr*)aCursor->mWASegment)->mUnique == ID_TRUE ? ID_FALSE : ID_TRUE;
    sdtSubHash     * sSubHash;
    sdtHashTRPHdr  * sTRPHeader;
    UInt             sLoop;
    UInt             i;
    UShort           sHashHigh;
    sdtWCB         * sSubWCBPtr = NULL;
    sdtWCB         * sRowWCBPtr;
    idBool           sIsFixedSubHash = ID_FALSE;
    idBool           sIsFixedRowPage = ID_FALSE;

    sLoop = 0;
    /* 최초 탐색 */

    if( aCursor->mTTHeader->mCheckCnt > SMI_TT_STATS_INTERVAL )
    {
        IDE_TEST( iduCheckSessionEvent( aCursor->mTTHeader->mStatistics ) != IDE_SUCCESS);
        aCursor->mTTHeader->mCheckCnt = 0;
    }

    do
    {
        if ( aCursor->mSubHashPtr == NULL )
        {
            sLoop++;
            IDE_TEST( getSlotPtr( (sdtHashSegHdr*)aCursor->mWASegment,
                                  &((sdtHashSegHdr*)aCursor->mWASegment)->mSubHashGrp,
                                  aCursor->mChildPageID ,
                                  aCursor->mChildOffset ,
                                  (UChar**)&sSubHash,
                                  &sSubWCBPtr )
                      != IDE_SUCCESS );
        }
        else
        {
            sSubHash   = (sdtSubHash*)aCursor->mSubHashPtr;
            sSubWCBPtr = (sdtWCB*)aCursor->mSubHashWCBPtr4Fetch;
        }

        //  읽어온 값은 SubHash일 수도있고, row 일수도 있다.
        IDE_ASSERT( ((sdtHashTRPHdr*)sSubHash)->mIsRow != SDT_HASH_ROW_HEADER );

        // SubHash 를 읽은 경우
        IDE_DASSERT( sSubHash->mHashLow ==  SDT_MAKE_HASH_LOW( aCursor->mHashValue ) );

        sHashHigh = SDT_MAKE_HASH_HIGH( aCursor->mHashValue );

        fixWAPage( sSubWCBPtr );
        sIsFixedSubHash = ID_TRUE;

        /* BUG-48235 정렬되어 있다면 이진탐색으로 찾는다.*/
        if (( sIsSorted == ID_TRUE ) && ( aCursor->mSubHashIdx == 0 ))
        {
            aCursor->mSubHashIdx = bSearchSubHash( sSubHash, sHashHigh );
            if ( aCursor->mSubHashIdx == SDT_SUBHASH_MAX_KEYCOUNT )
            {
                aCursor->mSubHashIdx = sSubHash->mKeyCount ;
            }
        }

        for( i = aCursor->mSubHashIdx ; i < sSubHash->mKeyCount ; i++ )
        {
            if ( sSubHash->mKey[i].mHashHigh != sHashHigh )
            {
                if ( sIsSorted == ID_FALSE )
                {
                    continue;
                }
                else
                {
                    i = aCursor->mSubHashIdx = sSubHash->mKeyCount ; 
                    break;
                }
            }
            sLoop++;

            IDU_FIT_POINT( "BUG-49107@sdtHashModule::fetchHashScanInternalSubHash" );
            IDE_TEST( getSlotPtr( (sdtHashSegHdr*)aCursor->mWASegment,
                                  &((sdtHashSegHdr*)aCursor->mWASegment)->mFetchGrp,
                                  sSubHash->mKey[i].mPageID ,
                                  sSubHash->mKey[i].mOffset ,
                                  (UChar**)&sTRPHeader,
                                  &sRowWCBPtr )
                      != IDE_SUCCESS );

            IDE_ASSERT_MSG(( ((sdtHashPageHdr*)sRowWCBPtr->mWAPagePtr)->mSelfNPID ==
                             sSubHash->mKey[i].mPageID ),
                           "%"ID_UINT32_FMT" != %"ID_UINT32_FMT" %"ID_UINT32_FMT"\n",
                           ((sdtHashPageHdr*)sRowWCBPtr->mWAPagePtr)->mSelfNPID ,
                           sSubHash->mKey[i].mPageID,
                           sRowWCBPtr->mNPageID );
            IDE_ASSERT_MSG(( sRowWCBPtr->mNPageID == sSubHash->mKey[i].mPageID ),
                           "%"ID_UINT32_FMT" != %"ID_UINT32_FMT"\n",
                           sRowWCBPtr->mNPageID ,
                           sSubHash->mKey[i].mPageID );

            IDE_ASSERT( SM_IS_FLAG_ON( sTRPHeader->mTRFlag, SDT_TRFLAG_CHILDGRID ) );
            IDE_ASSERT( SM_IS_FLAG_ON( sTRPHeader->mTRFlag, SDT_TRFLAG_HEAD ) );
            IDE_ASSERT( sTRPHeader->mIsRow == SDT_HASH_ROW_HEADER );
            IDE_ASSERT( sTRPHeader->mHashValue == aCursor->mHashValue );

            fixWAPage( sRowWCBPtr );
            sIsFixedRowPage = ID_TRUE;

            IDE_TEST( filteringAndFetch( aCursor,
                                         sTRPHeader,
                                         *aRow,
                                         &sResult )
                      != IDE_SUCCESS );

            sIsFixedRowPage = ID_FALSE;
            unfixWAPage( sRowWCBPtr );

            if ( sResult == ID_TRUE )
            {
                break;
            }
        }
        sIsFixedSubHash = ID_FALSE;
        unfixWAPage( sSubWCBPtr );

        if ( ( i+1 ) >= sSubHash->mKeyCount )
        {
            // sub hash에서 찾지 못했거나, 마지막 key에서 hit한 경우
            aCursor->mChildPageID = sSubHash->mNextPageID;
            aCursor->mChildOffset = sSubHash->mNextOffset;
            aCursor->mSubHashIdx  = 0;
            aCursor->mSubHashPtr  = NULL;
            aCursor->mSubHashWCBPtr4Fetch = NULL;

            if ( sResult == ID_TRUE )
            {
                break;
            }
        }
        else
        {
            IDE_ASSERT( sResult == ID_TRUE );

            // 찾은경우
            aCursor->mSubHashIdx    = i+1;
            aCursor->mSubHashPtr    = sSubHash;
            aCursor->mSubHashWCBPtr4Fetch = sSubWCBPtr;

            break;
        }

    }while( aCursor->mChildPageID != SM_NULL_PID );

    if ( sResult == ID_TRUE )
    {
        IDE_DASSERT( sRowWCBPtr != NULL );

        SC_MAKE_GRID( *aRowGRID,
                      aCursor->mTTHeader->mSpaceID,
                      sSubHash->mKey[i].mPageID,
                      sSubHash->mKey[i].mOffset );
    }
    else
    {
        sRowWCBPtr = NULL;
        *aRow      = NULL;
        SC_MAKE_NULL_GRID(*aRowGRID);
    }

    aCursor->mWCBPtr = sRowWCBPtr;
    aCursor->mTTHeader->mStatsPtr->mExtraStat2 = IDL_MAX( aCursor->mTTHeader->mStatsPtr->mExtraStat2, sLoop );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sIsFixedSubHash == ID_TRUE )
    {
        unfixWAPage( sSubWCBPtr );
    }

    if ( sIsFixedRowPage == ID_TRUE )
    {
        unfixWAPage( sRowWCBPtr );
    }

    smiTempTable::checkAndDump( aCursor->mTTHeader );

    return IDE_FAILURE;
}

/***************************************************************************
 * Description : HashScan Cursor의 ChildPageID를 기준으로 SubHash를 탐색해서
 *               Next Row를 가져온다.
 *
 * aCursor   - [IN]  탐색에 사용중인 Cursor
 ***************************************************************************/
IDE_RC sdtHashModule::getNextNPageForFullScan( smiHashTempCursor  * aCursor )
{
    sdtHashSegHdr * sWASeg = (sdtHashSegHdr*)aCursor->mWASegment;
    sdtWCB        * sWCBPtr;
    scPageID        sNPageID;

    IDE_DASSERT( aCursor->mIsInMemory == SDT_WORKAREA_OUT_MEMORY );

    sWCBPtr = (sdtWCB*)aCursor->mWCBPtr;

    IDE_ASSERT( SDT_IS_FIXED_PAGE( sWCBPtr ) );
    unfixWAPage( sWCBPtr );

    /* 0. 이전 Page Write요청
     *
     * 다음번 읽어야 하는 NPageID먼저 확인
     * 1. 아직 Extent안이면 +1
     * 2. Extent마지막 Page이면 다음 Extent의 FirstPageID를 가져온다.
     * 3. 미리 64개 Page를 읽어둔다.*/
    if ( aCursor->mChildPageID < aCursor->mNExtLstPID )
    {
        sNPageID = aCursor->mChildPageID + 1;
    }
    else
    {
        if ( aCursor->mSeq < sWASeg->mNExtFstPIDList4Row.mCount )
        {
            // BUG-47544 Next Normal Extent Array로 넘겨주지 않는 문제 해결
            if (( aCursor->mSeq % SDT_NEXTARR_EXTCOUNT ) == 0 )
            {
                // seq == 0 는 Open Cursor에서 이미 처리되었다.
                // next array로 넘어가는 이 위치에서 0일 수 없다.
                IDE_ERROR( aCursor->mSeq > 0 );

                aCursor->mCurNExtentArr = ((sdtNExtentArr*)aCursor->mCurNExtentArr)->mNextArr;
            }

            sNPageID = ((sdtNExtentArr*)aCursor->mCurNExtentArr)->mMap[ aCursor->mSeq % SDT_NEXTARR_EXTCOUNT ];

            if ( sNPageID != sWASeg->mNExtFstPIDList4Row.mLastFreeExtFstPID )
            {
                aCursor->mNExtLstPID = sNPageID + SDT_WAEXTENT_PAGECOUNT - 1;

                IDE_TEST( preReadNExtents( sWASeg,
                                           sNPageID,
                                           SDT_WAEXTENT_PAGECOUNT )
                          != IDE_SUCCESS );
            }
            else
            {
                aCursor->mNExtLstPID = sNPageID + sWASeg->mNExtFstPIDList4Row.mPageSeqInLFE - 1;

                IDE_TEST( preReadNExtents( sWASeg,
                                           sNPageID,
                                           sWASeg->mNExtFstPIDList4Row.mPageSeqInLFE  )
                          != IDE_SUCCESS );
            }

            aCursor->mSeq++;
        }
        else
        {
            // Full Scan이 모두 완료됨
            aCursor->mChildPageID = SM_NULL_PID;
            aCursor->mWCBPtr    = NULL;
            aCursor->mWAPagePtr = NULL;

            IDE_CONT( SKIP );
        }
    }

    sWCBPtr = findWCB( sWASeg, sNPageID );
    IDE_ASSERT( sWCBPtr != NULL );

    /* pre read extent로 미리 읽어 뒀는데 없을수도 있나?
     * 확인해봐야지 */
    IDE_DASSERT( sWCBPtr->mWAPagePtr != NULL );
    IDE_DASSERT( sWCBPtr->mNPageID == sNPageID );

    aCursor->mWCBPtr      = sWCBPtr;
    aCursor->mWAPagePtr   = sWCBPtr->mWAPagePtr;
    aCursor->mChildPageID = sNPageID;
    aCursor->mChildOffset = ID_SIZEOF(sdtHashPageHdr);

    IDE_DASSERT_MSG(( ((sdtHashPageHdr*)aCursor->mWAPagePtr)->mSelfNPID == aCursor->mChildPageID ),
                    "%"ID_UINT32_FMT" != %"ID_UINT32_FMT"\n",
                    ((sdtHashPageHdr*)aCursor->mWAPagePtr)->mSelfNPID ,
                    aCursor->mChildPageID );

    IDE_DASSERT_MSG(( ((sdtWCB*)aCursor->mWCBPtr)->mNPageID == aCursor->mChildPageID ),
                    "%"ID_UINT32_FMT" != %"ID_UINT32_FMT"\n",
                    ((sdtWCB*)aCursor->mWCBPtr)->mNPageID ,
                    aCursor->mChildPageID );

    IDE_EXCEPTION_CONT( SKIP );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/**************************************************************************
 * Description : fetch하고 맞는지 확인 함.
 *
 * aTempCursor  - [IN] 대상 커서
 * aTargetPtr   - [IN] 대상 Row의 Ptr
 * aRow         - [OUT] 대상 Row를 담을 Bufer
 * aResult      - [OUT] Filtering 통과 여부
 ***************************************************************************/
IDE_RC sdtHashModule::filteringAndFetch( smiHashTempCursor  * aTempCursor,
                                         sdtHashTRPHdr      * aTRPHeader,
                                         UChar              * aRow,
                                         idBool             * aResult )
{
    smiTempTableHeader * sHeader;
    sdtHashSegHdr      * sWASeg;
    sdtHashScanInfo      sTRPInfo;
    UInt                 sFlag;

    *aResult = ID_FALSE;

    sHeader = aTempCursor->mTTHeader;
    sWASeg  = (sdtHashSegHdr*)aTempCursor->mWASegment;

    IDE_DASSERT( SM_IS_FLAG_ON( aTRPHeader->mTRFlag, SDT_TRFLAG_HEAD ) );

    sFlag = aTempCursor->mTCFlag;

    /* HashScan인데 HashValue가 다르면 안됨 */
    if ( sFlag & SMI_TCFLAG_HASHSCAN )
    {
        IDE_TEST_CONT( aTRPHeader->mHashValue != aTempCursor->mHashValue, SKIP );
    }
    else
    {
        /* nothing to do */
    }

    /* HItFlag체크가 필요한가? */
    if ( sFlag & SMI_TCFLAG_HIT_MASK )
    {
        /* Hit연산은 크게 세 Flag이다.
         * IgnoreHit -> Hit자체를 상관 안함
         * Hit       -> Hit된 것만 가져옴
         * NoHit     -> Hit 안된 것만 가져옴
         *
         * 위 if문은 IgnoreHit가 아닌지를 구분하는 것이고.
         * 아래 IF문은 Hit인지 NoHit인지 구분하는 것이다. */
        if ( sFlag & SMI_TCFLAG_HIT )
        {
            /* HIT 를 찾는다. NOHIT이면 SKIP */
            IDE_DASSERT( !( sFlag & SMI_TCFLAG_NOHIT ) );/*Hit Nohit 둘다 세팅 되어 있으면 안된다.*/
            IDE_TEST_CONT( aTRPHeader->mHitSequence != sHeader->mHitSequence, SKIP );
        }
        else
        {
            /* NOHIT 를 찾는다. HIT이면 SKIP */
            IDE_DASSERT( sFlag & SMI_TCFLAG_NOHIT );/*Hit Nohit 둘다 세팅 안되어 있으면 안된다.*/
            IDE_TEST_CONT( aTRPHeader->mHitSequence == sHeader->mHitSequence, SKIP );
        }
    }
    else
    {
        /* nothing to do */
    }

    sTRPInfo.mTRPHeader = aTRPHeader;
    sTRPInfo.mFetchEndOffset = sHeader->mRowSize;

    IDE_TEST( fetch( sWASeg,
                     aRow,
                     &sTRPInfo ) != IDE_SUCCESS );

    IDE_DASSERT( sTRPInfo.mValueLength <= sHeader->mRowSize );

    if ( sFlag & SMI_TCFLAG_FILTER_ROW )
    {
        IDE_TEST( aTempCursor->mRowFilter->callback(
                      aResult,
                      sTRPInfo.mValuePtr,
                      NULL,
                      0,
                      SC_NULL_GRID,
                      aTempCursor->mRowFilter->data )
                  != IDE_SUCCESS );
        IDE_TEST_CONT( *aResult == ID_FALSE, SKIP );
    }
    else
    {
        /* nothing to do */
    }

    *aResult             = ID_TRUE;
    aTempCursor->mRowPtr = (UChar*)aTRPHeader;

    if ( aRow != sTRPInfo.mValuePtr )
    {
        IDE_DASSERT( SM_IS_FLAG_OFF( sFlag, SDT_TRFLAG_NEXTGRID ) );
        idlOS::memcpy( aRow,
                       sTRPInfo.mValuePtr,
                       sTRPInfo.mValueLength );
    }

    IDE_EXCEPTION_CONT( SKIP );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**************************************************************************
 * Description : Row가 Chaining, 즉 다른 Page에 걸쳐 기록될 경우 사용되며,
 *               걸친 부분을 Fetch해온다.
 *
 * aWASegment   - [IN] 대상 WASegment
 * aRowBuffer   - [IN] 쪼개진 Column을 저장하기 위한 Buffer
 * aTRPInfo     - [OUT] Fetch한 결과
 ***************************************************************************/
IDE_RC sdtHashModule::fetchChainedRowPiece( sdtHashSegHdr   * aWASegment,
                                            UChar           * aRowBuffer,
                                            sdtHashScanInfo * aTRPInfo )
{
    UChar        * sCursor;
    sdtHashTRPHdr* sTRPHeader;
    UChar          sFlag;
    UInt           sRowBufferOffset = 0;
    UInt           sTRPHeaderSize;

    sTRPHeaderSize =  SDT_HASH_TR_HEADER_SIZE_FULL ;

    /* Chainig Row일경우, 자신이 ReadPage하면서
     * HeadRow가 unfix될 수 있기에, Header를 복사해두고 이용함*/
#ifdef DEBUG
    sdtHashTRPHdr  sTRPHeaderBuffer;
    idlOS::memcpy( &sTRPHeaderBuffer,
                   aTRPInfo->mTRPHeader,
                   sTRPHeaderSize );
#endif

    sCursor = ((UChar*)aTRPInfo->mTRPHeader) + sTRPHeaderSize;

    sTRPHeader           = aTRPInfo->mTRPHeader;
    aTRPInfo->mValuePtr  = aRowBuffer;

    idlOS::memcpy( aRowBuffer,
                   sCursor,
                   sTRPHeader->mValueLength );

    IDE_ASSERT( sTRPHeader->mValueLength < SD_PAGE_SIZE );

    sRowBufferOffset       = sTRPHeader->mValueLength;
    aTRPInfo->mValueLength = sTRPHeader->mValueLength;

    do
    {
        if ( aWASegment->mIsInMemory == SDT_WORKAREA_OUT_MEMORY )
        {
            IDU_FIT_POINT( "BUG-49107@sdtHashModule::fetchChainedRowPiece" );
            IDE_TEST( getSlotPtr( aWASegment,
                                  &aWASegment->mFetchGrp,
                                  sTRPHeader->mNextPageID,
                                  sTRPHeader->mNextOffset,
                                  &sCursor,
                                  NULL )
                      != IDE_SUCCESS );
        }
        else
        {
            sCursor = (UChar*)sTRPHeader->mNextRowPiecePtr;
        }

        sFlag      = ((sdtHashTRPHdr*)sCursor)->mTRFlag;
        sTRPHeader = (sdtHashTRPHdr*)sCursor;

        IDE_ASSERT( sTRPHeader->mValueLength < SD_PAGE_SIZE );

        sCursor   +=  SDT_HASH_TR_HEADER_SIZE_FULL ;

        IDE_DASSERT( sTRPHeaderBuffer.mHashValue == sTRPHeader->mHashValue );

        idlOS::memcpy( aRowBuffer + sRowBufferOffset,
                       sCursor,
                       sTRPHeader->mValueLength );

        sRowBufferOffset       += sTRPHeader->mValueLength;
        aTRPInfo->mValueLength += sTRPHeader->mValueLength;

        if ( aTRPInfo->mFetchEndOffset <= sRowBufferOffset )
        {
            /* 여기까지만 필요하다, 이후는 Scan하지 않는다.*/
            break;
        }

        IDE_ERROR ( SM_IS_FLAG_ON( sFlag, SDT_TRFLAG_NEXTGRID ) )
    }
    while( sTRPHeader->mNextPageID != SM_NULL_PID );

#ifdef DEBUG
    IDE_DASSERT( idlOS::memcmp( &sTRPHeaderBuffer,
                                aTRPInfo->mTRPHeader,
                                sTRPHeaderSize ) == 0 );
#endif
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    smuUtility::dumpFuncWithBuffer( IDE_DUMP_0,
                                    dumpTempTRPHeader,
                                    (void*)aTRPInfo->mTRPHeader );

    return IDE_FAILURE;
}

/**************************************************************************
 * Description : Row가 Chaining, 즉 다른 Page에 걸쳐 기록될 경우 사용되며,
 *               걸친 부분을 Update 한다.
 *
 * aWASegment   - [IN] 대상 WASegment
 * aValueList   - [IN] 
 ***************************************************************************/
IDE_RC sdtHashModule::updateChainedRowPiece( smiHashTempCursor * aTempCursor,
                                             smiValue          * aValueList )
{
    const smiColumnList * sUpdateColumn;
    const smiColumn * sColumn;
    smiValue        * sValueList;
    UChar           * sRowPos;
    sdtHashTRPHdr   * sTRPHeader;
    sdtWCB          * sWCBPtr;
    UInt              sBeginOffset = 0;
    UInt              sEndOffset;
    UInt              sDstOffset;
    UInt              sSrcOffset;
    UInt              sSize;
    idBool            sIsWrite;

    sRowPos = aTempCursor->mRowPtr;
    sWCBPtr = (sdtWCB*)aTempCursor->mWCBPtr;

    while( 1 )
    {
        sTRPHeader = (sdtHashTRPHdr*)sRowPos;
        sEndOffset = sBeginOffset + sTRPHeader->mValueLength;

        sRowPos   +=  SDT_HASH_TR_HEADER_SIZE_FULL ;

        sUpdateColumn = aTempCursor->mUpdateColumns;
        sValueList    = aValueList;
        sIsWrite      = ID_FALSE;

        while( sUpdateColumn != NULL )
        {
            sColumn = ( smiColumn*)sUpdateColumn->column;

            if (( sEndOffset > sColumn->offset ) &&                          // 시작점이 end 보다 작고
                (( sColumn->offset + sValueList->length ) >= sBeginOffset )) // 끝 점이 Begin보다 크다
            {
                sSize = sValueList->length;
                if ( sColumn->offset < sBeginOffset ) /* 왼쪽 시작 부분이 잘렸는가? */
                {
                    // 잘렸다.
                    sDstOffset = 0;
                    sSrcOffset = sBeginOffset - sColumn->offset;
                    sSize -= sSrcOffset;
                }
                else
                {
                    // 안잘렸다.
                    sDstOffset = sColumn->offset - sBeginOffset;
                    sSrcOffset = 0;
                }

                /* 오른쪽 끝 부분이 잘렸는가? */
                if ( (sColumn->offset + sValueList->length ) > sEndOffset )
                {
                    // 잘렸다.
                    sSize -= (sColumn->offset + sValueList->length - sEndOffset);
                }

                idlOS::memcpy( sRowPos + sDstOffset,
                               ((UChar*)sValueList->value) + sSrcOffset,
                               sSize );
                sIsWrite = ID_TRUE;
            }

            sValueList++;
            sUpdateColumn = sUpdateColumn->next;
        }

        sBeginOffset = sEndOffset;

        if (( sIsWrite == ID_TRUE ) && ( aTempCursor->mWCBPtr != NULL ))
        {
            IDE_DASSERT( sWCBPtr->mWPState != SDT_WA_PAGESTATE_INIT );

            sWCBPtr->mWPState = SDT_WA_PAGESTATE_DIRTY;
        }

        if ( aTempCursor->mUpdateEndOffset <= sEndOffset )
        {
            // update 완료
            break;
        }

        IDE_ASSERT( SM_IS_FLAG_ON( sTRPHeader->mTRFlag, SDT_TRFLAG_NEXTGRID ) );

        if ( aTempCursor->mIsInMemory == SDT_WORKAREA_OUT_MEMORY )
        {
            IDU_FIT_POINT( "BUG-49107@sdtHashModule::updateChainedRowPiece" );
            IDE_TEST( getSlotPtr( (sdtHashSegHdr*)aTempCursor->mWASegment,
                                  &((sdtHashSegHdr*)aTempCursor->mWASegment)->mFetchGrp,
                                  sTRPHeader->mNextPageID,
                                  sTRPHeader->mNextOffset,
                                  &sRowPos,
                                  &sWCBPtr )
                      != IDE_SUCCESS );
        }
        else
        {
            sRowPos = (UChar*)sTRPHeader->mNextRowPiecePtr;
        }
    }
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**************************************************************************
 * Description : WCB를 초기화한다.
 *
 * aWCBPtr   - [IN] 대상 WCBPtr
 ***************************************************************************/
void sdtHashModule::initWCB( sdtWCB       * aWCBPtr )
{
    aWCBPtr->mWPState   = SDT_WA_PAGESTATE_INIT;
    aWCBPtr->mNPageID   = SC_NULL_PID;
    aWCBPtr->mFix       = 0;
    aWCBPtr->mNextWCB4Hash = NULL;
}

/**************************************************************************
 * Description : NPageHash를 뒤져서 해당 NPage를 가진 WPage를 찾아냄
 *
 * aWASegment - [IN] 대상 WASegment
 * aNPID      - [IN] 찾을 Normal Page의 ID
 ***************************************************************************/
sdtWCB * sdtHashModule::findWCB( sdtHashSegHdr* aWASegment,
                                 scPageID       aNPID )
{
    UInt       sHashValue = getNPageHashValue( aWASegment, aNPID );
    sdtWCB   * sWCBPtr;

    IDE_DASSERT( aWASegment->mNPageHashPtr != NULL );

    sWCBPtr = aWASegment->mNPageHashPtr[ sHashValue ];

    while( sWCBPtr != NULL )
    {
        IDE_DASSERT( getNPageHashValue( aWASegment, sWCBPtr->mNPageID )
                     == sHashValue );

        if ( sWCBPtr->mNPageID == aNPID )
        {
            break;
        }
        else
        {
            /* nothing to do */
        }

        sWCBPtr  = sWCBPtr->mNextWCB4Hash;
    }

    return sWCBPtr;
}

/**************************************************************************
 * Description : 지정한 그룹에서 빈 Page를 하나 할당받는다.
 *
 * aWASegment  - [IN] 대상 WASegment
 * aGroup      - [IN] 대상 Group
 * aWCBPtr     - [OUT] 할당받은 page의 WCBPtr
 ***************************************************************************/
IDE_RC sdtHashModule::getFreeWAPage( sdtHashSegHdr* aWASegment,
                                     sdtHashGroup * aGroup,
                                     sdtWCB      ** aWCBPtr )
{
    sdtWCB   * sWCBPtr;

    IDE_ASSERT( aGroup->mBeginWPID < aGroup->mEndWPID );

    /* Group을 한번 전부 순회함 */

    if ( aGroup->mReuseSeqWPID >= aGroup->mEndWPID )
    {
        aGroup->mReuseSeqWPID = aGroup->mBeginWPID;
    }

    while( 1 )
    {
        sWCBPtr = getWCBPtr( aWASegment, aGroup->mReuseSeqWPID );
        aGroup->mReuseSeqWPID++;

        /* Group을 한번 전부 순회함 */
        if ( aGroup->mReuseSeqWPID == aGroup->mEndWPID )
        {
            aGroup->mReuseSeqWPID = aGroup->mBeginWPID;
        }

        if ( sWCBPtr->mWAPagePtr == NULL )
        {
            if ( set64WAPagesPtr( aWASegment,
                                  sWCBPtr ) == IDE_SUCCESS )
            {
                /* 처음 할당된 Page는 아직 사용된 적이 없는 Page이다.
                 * State를 확인 하지 않고 바로 사용해도 된다.*/
                IDE_DASSERT( SDT_IS_NOT_FIXED_PAGE( sWCBPtr ) );
                break;
            }
            else
            {
                /* 할당에 실패 하였다 TOTAL WA 가 부족한 상황이다. */
                IDE_TEST( ideGetErrorCode() != smERR_ABORT_NOT_ENOUGH_WORKAREA );

                if ( aGroup->mBeginWPID == getWPageID( aWASegment,
                                                       sWCBPtr ) )
                {
                    /* 하나도 할당 안되어 있으면 하나 할당한다.*/
                    IDE_TEST( sdtWAExtentMgr::memAllocWAExtent( &aWASegment->mWAExtentInfo )
                              != IDE_SUCCESS );
                    aWASegment->mNxtFreeWAExtent = aWASegment->mWAExtentInfo.mTail;
                    aWASegment->mStatsPtr->mOverAllocCount++;

                    // 이건 반드시 성공해야 한다.
                    IDE_TEST( set64WAPagesPtr( aWASegment,
                                               sWCBPtr ) != IDE_SUCCESS );
                    break;
                }
                else
                {
                    /* 하나라도 할당되어 있다면, 처음부터 다시 시도한다. */
                    aGroup->mReuseSeqWPID = aGroup->mBeginWPID;
                    continue;
                }
            }
        }

        if ( SDT_IS_NOT_FIXED_PAGE( sWCBPtr ) )
        {
            /* 제대로 골랐음 */
            if ( sWCBPtr->mWPState == SDT_WA_PAGESTATE_DIRTY )
            {
                IDE_TEST( writeNPage( aWASegment,
                                      sWCBPtr ) != IDE_SUCCESS );
            }
            break;
        }
    }

    /* 할당받은 페이지를 빈 페이지로 만듬 */

    /* Assign되어있으면 Assign 된 페이지를 초기화시켜준다. */
    if (( aWASegment->mNPageHashPtr != NULL ) &&
        ( sWCBPtr->mWPState == SDT_WA_PAGESTATE_CLEAN ))
    {
        IDE_TEST( unassignNPage( aWASegment,
                                 sWCBPtr )
                  != IDE_SUCCESS );
    }

    initWCB( sWCBPtr );

    IDE_DASSERT( aWASegment->mInsertHintWCBPtr != sWCBPtr );

    (*aWCBPtr) = sWCBPtr;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
//XXX 이름 변경 getSlotPtr

/**************************************************************************
 * Description :
 *     WGRID를 바탕으로 해당 Slot의 포인터를 반환한다.
 *
 * aWASegment   - [IN] 대상 Hash Segment
 * aFetchGrp    - [IN] 대상 Group 객체
 * aPageID      - [IN] 대상 PageID
 * aOffset      - [IN] 대상 Offset
 * aNSlotPtr    - [OUT] 얻은 슬롯
 * aFixedWCBPtr - [OUT] Slot이 포함된 Page의 WCB
 ***************************************************************************/
IDE_RC sdtHashModule::getSlotPtr( sdtHashSegHdr * aWASegment,
                                  sdtHashGroup  * aFetchGrp,
                                  scPageID        aNPageID,
                                  scOffset        aOffset,
                                  UChar        ** aNSlotPtr,
                                  sdtWCB       ** aFixedWCBPtr )
{
    sdtWCB        * sWCBPtr;

    sWCBPtr = findWCB( aWASegment, aNPageID );

    if ( sWCBPtr == NULL )
    {
        IDE_TEST( getFreeWAPage( aWASegment,
                                 aFetchGrp,
                                 &sWCBPtr )
                  != IDE_SUCCESS );

        IDE_TEST( readNPage( aWASegment,
                             sWCBPtr,
                             aNPageID )
                  != IDE_SUCCESS );
    }

    IDE_DASSERT( sWCBPtr->mWAPagePtr != NULL );
    IDE_DASSERT( sWCBPtr->mNPageID == aNPageID );

    if ( aFixedWCBPtr != NULL )
    {
        *aFixedWCBPtr = sWCBPtr;
    }

    IDE_DASSERT( aOffset < SD_PAGE_SIZE );
    *aNSlotPtr = sWCBPtr->mWAPagePtr + aOffset ;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    ideLog::log( IDE_DUMP_0,
                 "aNSpaceID  : %"ID_UINT32_FMT"\n"
                 "aNPID      : %"ID_UINT32_FMT"\n"
                 "sWPID      : %"ID_UINT32_FMT"\n",
                 aWASegment->mSpaceID,
                 aNPageID,
                 (sWCBPtr != NULL) ? sdtHashModule::getWPageID( aWASegment, sWCBPtr ) : SD_NULL_PID );

    return IDE_FAILURE;
}

/**************************************************************************
 * Description : TempPage의 Header를 초기화 한다.
 *
 * aPageHdr   - [IN] 초기화 할 Page의 Pointer
 * aType      - [IN] Page의 Type
 * aSelfPID   - [IN] Page의 Normal PageID
 ***************************************************************************/
void sdtHashModule::initTempPage( sdtHashPageHdr  * aPageHdr,
                                  sdtTempPageType   aType,
                                  scPageID          aSelfPID )
{
    aPageHdr->mSelfNPID   = aSelfPID;
    aPageHdr->mType       = aType ;
    aPageHdr->mFreeOffset = ID_SIZEOF( sdtHashPageHdr );
}

/**************************************************************************
 * Description : WCB에 해당 SpaceID 및 PageID를 설정한다.
 *               즉 Page를 읽는 것이 아니라 설정만 한 것으로,
 *               Disk상에 있는 일반Page의 내용은 '무시'된다.
 *
 * aWASegment   - [IN] 대상 WASegment
 * aWCBPtr      - [IN] 대상 WCBPtr
 * aNPageID     - [IN] 대상 NPage의 주소
 ***************************************************************************/
IDE_RC sdtHashModule::assignNPage(sdtHashSegHdr* aWASegment,
                                  sdtWCB       * aWCBPtr,
                                  scPageID       aNPageID)
{
    UInt             sHashValue;

    /* 검증. 이미 Hash이 매달린 것이 아니어야 함 */
    IDE_DASSERT( findWCB( aWASegment, aNPageID ) == NULL );
    /* 제대로된 WPID여야 함 */
    IDE_DASSERT( getWPageID( aWASegment, aWCBPtr ) < getMaxWAPageCount( aWASegment ) );

    /* NPID 로 연결함 */
    IDE_ERROR( aWCBPtr->mNPageID   == SC_NULL_PID );
    IDE_ERROR( aWCBPtr->mWAPagePtr != NULL );

    aWCBPtr->mNPageID = aNPageID;
    aWCBPtr->mWPState = SDT_WA_PAGESTATE_CLEAN;

    /* Hash에 연결하기 */
    sHashValue = getNPageHashValue( aWASegment, aNPageID );
    aWCBPtr->mNextWCB4Hash = aWASegment->mNPageHashPtr[ sHashValue ];
    aWASegment->mNPageHashPtr[ sHashValue ] = aWCBPtr;

    /* Hash에 매달려서, 탐색되어야 함 */
    IDE_DASSERT( findWCB( aWASegment, aNPageID ) == aWCBPtr );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**************************************************************************
 * Description : NPage를 땐다. 즉 WPage상에 올려저 있는 것을 내린다.
 *               값을 초기화하고 Hash에서 제외하는 일 밖에 안한다.
 *
 * aWASegment  - [IN] 대상 WASegment
 * aWPID       - [IN] 대상 WPID
 ***************************************************************************/
IDE_RC sdtHashModule::unassignNPage( sdtHashSegHdr* aWASegment,
                                     sdtWCB       * aWCBPtr )
{
    UInt             sHashValue = 0;
    scPageID         sTargetNPID;
    sdtWCB        ** sWCBPtrPtr;
    sdtWCB         * sWCBPtr = aWCBPtr;

    /* Fix되어있지 않아야 함 */
    IDE_ERROR( sWCBPtr->mFix == 0 );
    IDE_ERROR( sWCBPtr->mNPageID  != SC_NULL_PID );

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
 * Description : bindPage하고 Disk상의 일반 Page로부터 내용을 읽어 올린다.
 *
 * aWASegment   - [IN] 대상 WASegment
 * aWCBPtr      - [IN] 읽어드릴때 사용 할 WCB
 * aNPageID     - [IN] 대상 NPage의 주소
 ***************************************************************************/
IDE_RC sdtHashModule::readNPage( sdtHashSegHdr* aWASegment,
                                 sdtWCB       * aWCBPtr,
                                 scPageID       aNPageID )
{
    IDE_DASSERT( aWCBPtr->mNxtUsedWCBPtr != NULL );
    IDE_DASSERT( aWCBPtr->mWAPagePtr != NULL );

    /* 기존에 없던 페이지면 Read해서 올림 */
    IDE_TEST( assignNPage( aWASegment,
                           aWCBPtr,
                           aNPageID ) != IDE_SUCCESS );

    // TC/FIT/Server/sm/Bugs/BUG-45263/BUG-45263.tc
    IDU_FIT_POINT( "BUG-45263@sdtHashModule::readNPage::iduFileopen" );

    IDE_TEST( sddDiskMgr::read( aWASegment->mStatistics,
                                aWASegment->mSpaceID,
                                aNPageID,
                                1,
                                aWCBPtr->mWAPagePtr,
                                ID_FALSE ) /* aFatal */
              != IDE_SUCCESS );

    // 첫번째 위치는 무조건 self PID
    IDE_ASSERT( aNPageID == ((sdtHashPageHdr*)aWCBPtr->mWAPagePtr)->mSelfNPID );

    aWASegment->mStatsPtr->mReadCount++;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**************************************************************************
 * Description : Disk상의 일반 Page로부터 연속된 여러 page의 내용을 읽어 올린다.
 *
 * aWASegment   - [IN] 대상 WASegment
 * aFstNPageID  - [IN] 읽어들일 연속된 대상 NPage 들의 첫번째 Page의 주소
 * aPageCount   - [IN] 읽어들일 page의 수
 * aFstWCBPtr   - [IN] 읽어드릴때 사용 할 WCB
 * aNeedFixPage - [IN] Page Fix 유무를 나타낸다.
 ***************************************************************************/
IDE_RC sdtHashModule::readNPages( sdtHashSegHdr* aWASegment,
                                  scPageID       aFstNPageID,
                                  UInt           aPageCount,
                                  sdtWCB       * aFstWCBPtr,
                                  idBool         aNeedFixPage )
{
    scPageID     sNPageID;
    scPageID     sLstNPageID;
    sdtWCB     * sWCBPtr;
    sdtWCB     * sNxtWCBPtr;

    IDE_DASSERT( aFstWCBPtr->mNxtUsedWCBPtr != NULL );
    IDE_DASSERT( aFstWCBPtr->mWAPagePtr     != NULL );
    
    
    IDE_TEST( sddDiskMgr::read( aWASegment->mStatistics,
                                aWASegment->mSpaceID,
                                aFstNPageID,
                                aPageCount,
                                aFstWCBPtr->mWAPagePtr,
                                ID_FALSE ) /* aFatal */
              != IDE_SUCCESS );

    aWASegment->mStatsPtr->mReadCount += aPageCount;

    sWCBPtr = aFstWCBPtr;
    sLstNPageID = aFstNPageID + aPageCount;

    for( sNPageID = aFstNPageID ; sNPageID < sLstNPageID ; sNPageID++ )
    {
        IDE_ASSERT( sWCBPtr != NULL );
        IDE_ASSERT( sWCBPtr->mWAPagePtr != NULL );
        IDE_ASSERT( sNPageID == ((sdtHashPageHdr*)sWCBPtr->mWAPagePtr)->mSelfNPID );

        sNxtWCBPtr = sWCBPtr->mNextWCB4Hash;
        sWCBPtr->mNextWCB4Hash = NULL;

        IDE_TEST( assignNPage( aWASegment,
                               sWCBPtr,
                               sNPageID ) != IDE_SUCCESS );

        if ( aNeedFixPage == ID_TRUE )
        {
            fixWAPage( sWCBPtr );
        }

        sWCBPtr = sNxtWCBPtr;
    }
    IDE_DASSERT( sWCBPtr == NULL );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**************************************************************************
 * Description : Disk상의 Page를 Extent 단위로 내용을 읽어 올린다.
 *               AIX는 preadv 를 사용 할 수 없으므로 pread를 사용한다.
 *
 * aWASegment   - [IN] 대상 WASegment
 * aFstNPageID  - [IN] 읽어드릴 첫번째 NPage의 주소
 * aPageCount   - [IN] Page의 수
 ***************************************************************************/
IDE_RC sdtHashModule::preReadNExtents( sdtHashSegHdr * aWASegment,
                                       scPageID        aFstNPageID,
                                       UInt            aPageCount )
{
    UInt         sCount = 0;
    scPageID     sCurNPageID;
    scPageID     sFstNPageID = SM_NULL_PID;
    scPageID     sEndNPageID = aFstNPageID + aPageCount;
    sdtWCB     * sWCBPtr;
    sdtWCB     * sPrvWCBPtr = NULL;
    sdtWCB     * sFstWCBPtr = NULL;

    for( sCurNPageID = aFstNPageID;
         sCurNPageID < sEndNPageID ;
         sCurNPageID++ )
    {
        sWCBPtr = findWCB( aWASegment, sCurNPageID );

        // WorkArea에 Page가 없다.
        if ( sWCBPtr == NULL )
        {
            if ( sFstNPageID == SM_NULL_PID )
            {
                /* 연속된 첫번째 page이다. */
                sFstNPageID = sCurNPageID;
            }

            sWCBPtr = getFreePage4PreRead( aWASegment,
                                           aFstNPageID,
                                           sEndNPageID,
                                           sPrvWCBPtr );

            IDE_DASSERT( sWCBPtr != sPrvWCBPtr );

            if ( sWCBPtr->mNPageID != SM_NULL_PID )
            {
                IDE_TEST( unassignNPage( aWASegment,
                                         sWCBPtr ) != IDE_SUCCESS );

                initWCB( sWCBPtr );
            }

            IDE_DASSERT( sWCBPtr->mNextWCB4Hash == NULL );

            if ( sPrvWCBPtr == NULL )
            {
                // 첫번째 WCB
                sFstWCBPtr = sWCBPtr;
            }
            else
            {
                if (( sPrvWCBPtr->mWAPagePtr + SD_PAGE_SIZE ) == sWCBPtr->mWAPagePtr )
                {
                    // sPrevWCB와 mWAPagePtr의 Pointer가 연속되어 있다.
                    IDE_DASSERT( sPrvWCBPtr->mNextWCB4Hash == NULL );
                    sPrvWCBPtr->mNextWCB4Hash = sWCBPtr;
                }
                else
                {
                    // sPrevWCB와 mWAPagePtr의 Pointer가 연속되지 않았다.
                    // FstWCB 부터 PrevWCB 까지 읽어들이고,
                    // 방금 가져온 sWCBPtr 부터 다시 counting 한다.
                    IDE_TEST( readNPages( aWASegment,
                                          sFstNPageID,
                                          sCount,
                                          sFstWCBPtr,
                                          ID_TRUE ) != IDE_SUCCESS );
                    sCount = 0;
                    sFstNPageID = sCurNPageID;
                    sFstWCBPtr = sWCBPtr;
                }
            }
            sCount++;
            sPrvWCBPtr = sWCBPtr;
        }
        else
        {
            // Work Area에 해당 Page가 있다면 Fix해 둔다.
            fixWAPage( sWCBPtr );

            if ( sCount > 0 )
            {
                // TC/FIT/Server/sm/Bugs/BUG-45263/BUG-45263.tc
                IDU_FIT_POINT( "1.BUG-45263@sdtHashModule::preReadNExtent::iduFileopen" );

                IDE_TEST( readNPages( aWASegment,
                                      sFstNPageID,
                                      sCount,
                                      sFstWCBPtr,
                                      ID_TRUE ) != IDE_SUCCESS );
                sCount = 0;
                sFstNPageID = SM_NULL_PID;
                sPrvWCBPtr  = NULL;
                sFstWCBPtr  = NULL;
            }
        }
    }

    if ( sCount > 0 )
    {
        // TC/FIT/Server/sm/Bugs/BUG-45263/BUG-45263.tc
        IDU_FIT_POINT( "2.BUG-45263@sdtHashModule::preReadNExtent::iduFileopen" );
        IDE_TEST( readNPages( aWASegment,
                              sFstNPageID,
                              sCount,
                              sFstWCBPtr,
                              ID_TRUE ) != IDE_SUCCESS );
    }

#ifdef DEBUG
    for( sCurNPageID = aFstNPageID ; sCurNPageID < sEndNPageID ; sCurNPageID++ )
    {
        sWCBPtr = findWCB( aWASegment,
                           sCurNPageID );
        if ( sWCBPtr != NULL )
        {
            IDE_ASSERT(((sdtHashPageHdr*)sWCBPtr->mWAPagePtr)->mSelfNPID == sCurNPageID );
            IDE_ASSERT( sWCBPtr->mNPageID == sCurNPageID );
        }
    }
#endif

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**************************************************************************
 * Description : preReadNExtents() 용 빈 Page를 찾는다.
 *
 * aWASegment   - [IN] 대상 WASegment
 * aFstNPageID  - [IN] skip대상 NPageID의 첫번째 pageID
 * aEndNPageID  - [IN] skip대상 NPageID의 마지막 pageID
 * aTailWCBPtr  - [IN] 앞의 Page의 Pointer
 ***************************************************************************/
sdtWCB * sdtHashModule::getFreePage4PreRead( sdtHashSegHdr * aWASegment,
                                             scPageID        aFstSkipNPageID,
                                             scPageID        aEndSkipNPageID,
                                             sdtWCB        * aTailWCBPtr )
{
    sdtWCB     * sWCBPtr;
    sdtWCB     * sEmptyWCBPtr;

    sWCBPtr = getWCBPtr( aWASegment,
                         aWASegment->mFetchGrp.mReuseSeqWPID );

    while (1)
    {
        if ( sWCBPtr->mWAPagePtr != NULL )
        {
            if( sWCBPtr->mNPageID != SD_NULL_PID )
            {
                /* 만약 dirty라면 write한다,
                 * Write 되었는지 확인 할 필요는 없다.*/
                if ( sWCBPtr->mWPState == SDT_WA_PAGESTATE_DIRTY )
                {
                    (void)writeNPage( aWASegment,
                                      sWCBPtr );
                }
                /* 할당되어있는 page이면 가용 page인지 확인한다.*/
                if(( sWCBPtr->mWPState != SDT_WA_PAGESTATE_DIRTY ) &&
                   ( SDT_IS_NOT_FIXED_PAGE( sWCBPtr ) ) &&
                   (( sWCBPtr->mNPageID <  aFstSkipNPageID ) ||
                    ( sWCBPtr->mNPageID >= aEndSkipNPageID )))
                {
                    /* 찾았다 */
                    break;
                }
            }
            else
            {
                /* NPage가 NULL인데 Next Hash가 NULL이 아니면
                 * 이미 vec에 배정된 WCB이다.
                 * 한바퀴 돌아서 aTailWCBPtr을 만날수도 있다.*/
                if (( sWCBPtr->mNextWCB4Hash == NULL ) &&
                    ( sWCBPtr != aTailWCBPtr ))
                {
                    break;
                }
                else
                {
                    /* 한바퀴 돌아서 sFstWCBPtr 까지 와버렸다.
                     * Hash Area Size가 작으면 발생 할 수 있다.
                     * 다시 찾는다.*/
                }
            }

            /* 다음 page로 넘어간다. */
            aWASegment->mFetchGrp.mReuseSeqWPID++;
            sWCBPtr++;
        }
        else
        {
            /* 빈 page이다. wapage는 extent단위로 할당하므로
             * sWCBPtr이 extent 첫번째 page일 것으로 생각되지만,
             * 같은 page 일 것으로 생각되지만, 그래도 확인한다. */
            sEmptyWCBPtr = sWCBPtr - ( aWASegment->mFetchGrp.mReuseSeqWPID % SDT_WAEXTENT_PAGECOUNT );
            IDE_DASSERT( sEmptyWCBPtr->mWAPagePtr == NULL );
            IDE_DASSERT( (sEmptyWCBPtr+SDT_WAEXTENT_PAGECOUNT-1)->mWAPagePtr == NULL );

            /* Total wa가 부족 할 경우 할당 실패 할 수 있다.
             * reused seq를 begin으로 되돌려서 처음부터 다시 확인한다.
             * 여기까지 왔으면 insert에 사용 되던 page가 있을것이다.*/
            if( set64WAPagesPtr( aWASegment,
                                 sEmptyWCBPtr ) == IDE_SUCCESS )
            {
                /* 할당 성공 했으면 빈 page이다.
                 * 아니면 처음부터 다시 찾는다.*/
                break;
            }
        }

        /* 다음 page로 넘어가려는데 End를 넘어갔거나
         * wapage를 할당받는데 실패한 경우
         * 처음부터 다시 찾는다. */
        if (( aWASegment->mFetchGrp.mReuseSeqWPID >= aWASegment->mFetchGrp.mEndWPID ) ||
            ( sWCBPtr->mWAPagePtr == NULL ))
        {
            aWASegment->mFetchGrp.mReuseSeqWPID = aWASegment->mFetchGrp.mBeginWPID;

            sWCBPtr = getWCBPtr( aWASegment,
                                 aWASegment->mFetchGrp.mReuseSeqWPID );

        }
    }

    aWASegment->mFetchGrp.mReuseSeqWPID++;

    if ( aWASegment->mFetchGrp.mReuseSeqWPID >= aWASegment->mFetchGrp.mEndWPID )
    {
        aWASegment->mFetchGrp.mReuseSeqWPID = aWASegment->mFetchGrp.mBeginWPID;
    }

    return sWCBPtr;
}

/**************************************************************************
 * Description : NPage를 할당받고 설정한다.
 *
 * aWASegment      - [IN] 대상 WASegment
 * aNExtFstPIDList - [IN] 할당 받을 NExtent 
 * aTargetWCBPtr   - [IN] 설정할 WCBPtr
 ***************************************************************************/
IDE_RC sdtHashModule::allocAndAssignNPage( sdtHashSegHdr    * aWASegment,
                                           sdtNExtFstPIDList* aNExtFstPIDList,
                                           sdtWCB           * aTargetWCBPtr )
{
    scPageID sFreeNPID;

    if ( aNExtFstPIDList->mPageSeqInLFE == SDT_WAEXTENT_PAGECOUNT )
    {
        /* 마지막 NExtent를 다썼음 , 디스크 extent는 하나  할 당 받아온다. */
        IDE_TEST( sdtWAExtentMgr::allocFreeNExtent( aWASegment->mStatistics,
                                                    aWASegment->mStatsPtr,
                                                    aWASegment->mSpaceID,
                                                    aNExtFstPIDList )!= IDE_SUCCESS );
        
    }
    else
    {
        /* 기존에 할당해둔 Extent에서 가져옴 */
    }

    sFreeNPID = aNExtFstPIDList->mLastFreeExtFstPID
        + aNExtFstPIDList->mPageSeqInLFE;
    aNExtFstPIDList->mPageSeqInLFE++;

    ((sdtHashPageHdr*)aTargetWCBPtr->mWAPagePtr)->mSelfNPID = sFreeNPID;

    if ( aWASegment->mNPageHashPtr == NULL )
    {
        aTargetWCBPtr->mNPageID = sFreeNPID;
        aTargetWCBPtr->mWPState = SDT_WA_PAGESTATE_CLEAN;
    }
    else
    {
        IDE_TEST( assignNPage( aWASegment,
                               aTargetWCBPtr,
                               sFreeNPID ) != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**************************************************************************
 * Description : Total WA에서 WAExtent를 할당받아 온다.
 *
 * aWASegment     - [IN] 대상 WASegment
 ***************************************************************************/
IDE_RC sdtHashModule::expandFreeWAExtent( sdtHashSegHdr* aWASeg )
{
    SInt          sExtentCount;
    sdtWAExtent * sTailExtent;

    IDE_ERROR( aWASeg->mMaxWAExtentCount > aWASeg->mWAExtentInfo.mCount );

    sExtentCount = smuProperty::getTmpAllocWAExtCnt();

    if ( aWASeg->mMaxWAExtentCount < ( sExtentCount + aWASeg->mWAExtentInfo.mCount ) )
    {
        sExtentCount = aWASeg->mMaxWAExtentCount - aWASeg->mWAExtentInfo.mCount;
    }
    IDE_ERROR( sExtentCount > 0 );

    sTailExtent = aWASeg->mWAExtentInfo.mTail;

    IDE_DASSERT( sTailExtent->mNextExtent == NULL );

    IDE_TEST( sdtWAExtentMgr::allocWAExtents( &aWASeg->mWAExtentInfo,
                                              sExtentCount ) != IDE_SUCCESS );

    IDE_DASSERT( sTailExtent->mNextExtent != NULL );

    aWASeg->mNxtFreeWAExtent = sTailExtent->mNextExtent;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**************************************************************************
 * Description : HashSlot용으로 한개의 WAPage를 WCB에 할당한다
 *
 * aWASegment  - [IN] 대상 WASegment
 * aWCBPtr     - [IN] 할당할 첫번째 WCB의 Pointer  
 ***************************************************************************/
IDE_RC sdtHashModule::setWAPagePtr4HashSlot( sdtHashSegHdr * aWASegment,
                                             sdtWCB        * aWCBPtr )
{
    IDE_DASSERT( aWCBPtr->mWAPagePtr == NULL );
    IDE_DASSERT( aWCBPtr->mNxtUsedWCBPtr == NULL );

    if ( aWASegment->mCurrFreePageIdx >= SDT_WAEXTENT_PAGECOUNT )
    {
        if ( aWASegment->mNxtFreeWAExtent == NULL )
        {
            // Extent를 50%씩 늘인다.
            if ( expandFreeWAExtent( aWASegment ) != IDE_SUCCESS )
            {
                // 용량 부족에서만 재시도 한다.
                IDE_TEST( ideGetErrorCode() != smERR_ABORT_NOT_ENOUGH_WORKAREA );

                IDE_TEST( sdtWAExtentMgr::memAllocWAExtent( &aWASegment->mWAExtentInfo )
                          != IDE_SUCCESS );
                aWASegment->mStatsPtr->mOverAllocCount++;

                // 바로 아래에서 cur로 옮겨지고 null로 재설정 된다.
                aWASegment->mNxtFreeWAExtent = aWASegment->mWAExtentInfo.mTail;
            }
        }

        aWASegment->mCurrFreePageIdx = 0;
        aWASegment->mCurFreeWAExtent = aWASegment->mNxtFreeWAExtent;
        aWASegment->mNxtFreeWAExtent = aWASegment->mNxtFreeWAExtent->mNextExtent;
    }

    aWCBPtr->mWAPagePtr     = aWASegment->mCurFreeWAExtent->mPage[ aWASegment->mCurrFreePageIdx ];
    aWCBPtr->mNxtUsedWCBPtr = aWASegment->mUsedWCBPtr;
    aWASegment->mUsedWCBPtr = aWCBPtr;

    idlOS::memset( aWCBPtr->mWAPagePtr, 0, SD_PAGE_SIZE );

    aWASegment->mCurrFreePageIdx++;
    aWASegment->mAllocWAPageCount++;
    aWASegment->mHashSlotAllocPageCount++;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**************************************************************************
 * Description : WCB에 WAPage를 WAExtent단위로 할당한다
 *
 * aWASegment  - [IN] 대상 WASegment
 * aWCBPtr     - [IN] 할당할 첫번째 WCB의 Pointer  
 ***************************************************************************/
IDE_RC sdtHashModule::set64WAPagesPtr( sdtHashSegHdr * aWASegment,
                                       sdtWCB        * aWCBPtr )
{
    UInt i;

    IDE_DASSERT(( getWPageID( aWASegment, aWCBPtr ) % SDT_WAEXTENT_PAGECOUNT ) == 0 );

    if ( aWASegment->mNxtFreeWAExtent == NULL )
    {
        IDE_TEST( expandFreeWAExtent( aWASegment ) != IDE_SUCCESS );
    }

    for( i = 0;
         i < SDT_WAEXTENT_PAGECOUNT;
         aWCBPtr++, i++ )
    {
        IDE_DASSERT( aWCBPtr->mWAPagePtr     == NULL );
        IDE_DASSERT( aWCBPtr->mNxtUsedWCBPtr == NULL );

        aWCBPtr->mNxtUsedWCBPtr = aWASegment->mUsedWCBPtr;
        aWASegment->mUsedWCBPtr = aWCBPtr;

        aWCBPtr->mWAPagePtr = aWASegment->mNxtFreeWAExtent->mPage[i];
    }

    aWASegment->mNxtFreeWAExtent = aWASegment->mNxtFreeWAExtent->mNextExtent;

    aWASegment->mAllocWAPageCount += SDT_WAEXTENT_PAGECOUNT ;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**************************************************************************
 * Description : WASegemnt 구성을 File로 Dump한다.
 *
 * aWASegment   - [IN] 대상 WASegment
 ***************************************************************************/
void sdtHashModule::exportHashSegmentToFile( sdtHashSegHdr* aWASegment )
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
              != IDE_SUCCESS );
    sState = 1;

    smuUtility::getTimeString( smiGetCurrTime(),
                               SMI_TT_STR_SIZE,
                               sDateString );

    idlOS::snprintf( sFileName,
                     SM_MAX_FILE_NAME,
                     "%s%c%s_hash.td",
                     smuProperty::getTempDumpDirectory(),
                     IDL_FILE_SEPARATOR,
                     sDateString );

    IDE_TEST( sFile.setFileName( sFileName ) != IDE_SUCCESS );
    IDE_TEST( sFile.createUntilSuccess( smiSetEmergency )
              != IDE_SUCCESS );
    IDE_TEST( sFile.open( ID_FALSE ) != IDE_SUCCESS ); //DIRECT_IO
    sState = 2;

    // 1. sdtHashSegHdr Pointer (나중에 pointer 계산용으로 사용)
    IDE_TEST( sFile.write( aWASegment->mStatistics,
                           0,
                           (UChar*)&aWASegment,
                           ID_SIZEOF(sdtHashSegHdr*) )
              != IDE_SUCCESS );
    sOffset += ID_SIZEOF(sdtHashSegHdr*);

    // sdtHashSegHdr+ WCB Map
    sMaxPageCount = getMaxWAPageCount( aWASegment );

    sSize = calcWASegmentSize( sMaxPageCount );

    // 2. sdtHashSegHdr + WCB Array + WAFlushQueue
    IDE_TEST( sFile.write( aWASegment->mStatistics,
                           sOffset,
                           (UChar*)aWASegment,
                           sSize )
              != IDE_SUCCESS );
    sOffset += sSize;

    // Normal page Hash Map
    if ( aWASegment->mNPageHashPtr != NULL )
    {
        // 3. n page hash map
        sSize = aWASegment->mNPageHashBucketCnt * ID_SIZEOF(sdtWCB*);
        IDE_TEST( sFile.write( aWASegment->mStatistics,
                               sOffset,
                               (UChar*)aWASegment->mNPageHashPtr,
                               sSize )
                  != IDE_SUCCESS );
        sOffset += sSize;
    }

    // 4. Normal Extent Map for Row
    for ( sNExtentArr =  aWASegment->mNExtFstPIDList4Row.mHead;
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

    // 5. Normal Extent Map for SubHash
    for ( sNExtentArr =  aWASegment->mNExtFstPIDList4SubHash.mHead;
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

    // 6. wa page ( wapageID, page )
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

    // 7. special pid (the end)
    sWPageID = SM_SPECIAL_PID;
    IDE_TEST( sFile.write( aWASegment->mStatistics,
                           sOffset,
                           (UChar*)&sWPageID,
                           ID_SIZEOF(scPageID) )
              != IDE_SUCCESS );

    IDE_TEST( sFile.sync() != IDE_SUCCESS );

    sState = 1;
    IDE_TEST( sFile.close() != IDE_SUCCESS );

    sState = 0;
    IDE_TEST( sFile.destroy() != IDE_SUCCESS );

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

/***************************************************************************
 * Description : File로부터 WASegment를 읽어들인다.
 *
 * aFileName      - [IN]  대상 File
 * aHashSegHdr    - [OUT] 읽어들인 WASegment를 보정한 결과
 * aRawHashSegHdr - [OUT] 읽어들인 내용을 그대로 복사한다.
 * aPtr           - [OUT] 읽어들일 Buffer
 ***************************************************************************/
IDE_RC sdtHashModule::importHashSegmentFromFile( SChar         * aFileName,
                                                 sdtHashSegHdr** aHashSegHdr,
                                                 sdtHashSegHdr * aRawHashSegHdr,
                                                 UChar        ** aPtr )
{
    iduFile         sFile;
    UInt            sState          = 0;
    UInt            sMemAllocState  = 0;
    ULong           sSize = 0;
    sdtHashSegHdr * sWASegment = NULL;
    UChar         * sPtr;
    ULong           sMaxPageCount;
    UChar         * sOldWASegPtr;
    sdtNExtentArr * sNExtentArr;

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
    // 1. Old sdtHashSegHdr Pointer
    sOldWASegPtr = *(UChar**)sPtr;

    sPtr += ID_SIZEOF(sdtHashSegHdr*);

    // 2. sdtHashSegHdr + WCB Map
    sWASegment = (sdtHashSegHdr*)sPtr;

    // 값을 있는 그대로 출력 할 필요도 있다.
    idlOS::memcpy((UChar*)aRawHashSegHdr,
                  (UChar*)sWASegment,
                  SDT_HASH_SEGMENT_HEADER_SIZE);

    sMaxPageCount = getMaxWAPageCount( sWASegment ) ;
    sPtr += calcWASegmentSize( sMaxPageCount );

    IDE_ERROR( (ULong)sPtr % ID_SIZEOF(ULong) == 0);

    // 3. Normal page Hash Map
    if ( sWASegment->mNPageHashPtr != NULL )
    {
        sWASegment->mNPageHashPtr = (sdtWCB**)sPtr;
        sPtr += sWASegment->mNPageHashBucketCnt * ID_SIZEOF(sdtWCB*);
    }

    // 4. Normal Extent Map for Row
    if ( sWASegment->mNExtFstPIDList4Row.mHead != NULL )
    {
        sWASegment->mNExtFstPIDList4Row.mHead = (sdtNExtentArr*)sPtr;
        sPtr += ID_SIZEOF( sdtNExtentArr );

        for ( sNExtentArr =  sWASegment->mNExtFstPIDList4Row.mHead;
              sNExtentArr->mNextArr != NULL;
              sNExtentArr = sNExtentArr->mNextArr )
        {
            sNExtentArr->mNextArr = (sdtNExtentArr*)sPtr;
            sPtr += ID_SIZEOF( sdtNExtentArr );
        }
        sWASegment->mNExtFstPIDList4Row.mTail = sNExtentArr;
    }

    // 5. Normal Extent Map for SubHash
    if ( sWASegment->mNExtFstPIDList4SubHash.mHead != NULL )
    {
        sWASegment->mNExtFstPIDList4SubHash.mHead = (sdtNExtentArr*)sPtr;
        sPtr += ID_SIZEOF( sdtNExtentArr );

        for ( sNExtentArr =  sWASegment->mNExtFstPIDList4SubHash.mHead;
              sNExtentArr->mNextArr != NULL;
              sNExtentArr = sNExtentArr->mNextArr )
        {
            sNExtentArr->mNextArr = (sdtNExtentArr*)sPtr;
            sPtr += ID_SIZEOF( sdtNExtentArr );
        }
        sWASegment->mNExtFstPIDList4SubHash.mTail = sNExtentArr;
    }

    IDE_TEST( resetWCBInfo4Dump( sWASegment,
                                 sOldWASegPtr,
                                 sMaxPageCount,
                                 &sPtr )
              != IDE_SUCCESS );

    if ( sWASegment->mUsedWCBPtr != NULL )
    {
        sWASegment->mUsedWCBPtr =
            (sdtWCB*)(((UChar*)sWASegment->mUsedWCBPtr)
                      - sOldWASegPtr + (UChar*)sWASegment) ;
    }

    if ( sWASegment->mInsertHintWCBPtr != NULL )
    {
        sWASegment->mInsertHintWCBPtr =
            (sdtWCB*)(((UChar*)sWASegment->mInsertHintWCBPtr)
                      - sOldWASegPtr + (UChar*)sWASegment) ;
    }
    if ( sWASegment->mSubHashWCBPtr != NULL )
    {
        sWASegment->mSubHashWCBPtr =
            (sdtWCB*)(((UChar*)sWASegment->mSubHashWCBPtr)
                      - sOldWASegPtr + (UChar*)sWASegment) ;
    }

    sState = 1;
    IDE_ERROR( sFile.close() == IDE_SUCCESS );
    sState = 0;
    IDE_ERROR( sFile.destroy() == IDE_SUCCESS );

    *aHashSegHdr  = sWASegment;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sMemAllocState == 1 )
    {
        IDE_ASSERT( iduMemMgr::free( sPtr ) == IDE_SUCCESS );
        sPtr = NULL;
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
 * aWASegment     - [IN] 재구축 중인 Hash Segment
 * aOldWASegPtr   - [IN] Dump 하기 전의 Hash Segment의 Pointer
 * aMaxPageCount  - [IN] Page의 Count
 * aPtr           - [IN] 읽어들인 Buffer
 ***************************************************************************/
IDE_RC sdtHashModule::resetWCBInfo4Dump( sdtHashSegHdr * aWASegment,
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
/***************************************************************************
 * Description : Hash Segment를 Dump 한다.
 *
 * aWASegment - [IN] dump 대상 Hash WASegment
 * aOutBuf    - [IN] dump한 정보를 담을 Buffer 
 * aOutSize   - [IN] Buffer의 크기
 ***************************************************************************/
void sdtHashModule::dumpHashSegment( sdtHashSegHdr * aWASegment,
                                     SChar         * aOutBuf,
                                     UInt            aOutSize )
{
    IDE_ERROR( aWASegment != NULL );

    (void)idlVA::appendFormat(
                aOutBuf,
                aOutSize,
                "DUMP HASH WASEGMENT:\n"
                "WASegPtr              : 0x%"ID_xINT64_FMT"\n"
                "SpaceID               : %"ID_UINT32_FMT"\n"
                "Type                  : %"ID_UINT32_FMT" %s\n"
                "HashSlotCount         : %"ID_UINT32_FMT"\n"

                "InMemory              : %s\n"

                "Unique                : %"ID_UINT32_FMT"\n"
                "OpenCursorType        : %"ID_UINT32_FMT"\n"
                "EndWPID               : %"ID_UINT32_FMT"\n"

                "InsertGroup BeginWPID : %"ID_UINT32_FMT"\n"
                "InsertGroup EndWPID   : %"ID_UINT32_FMT"\n"
                "InsertGroup ReuseWPID : %"ID_UINT32_FMT"\n"

                "FetchGroup BeginWPID  : %"ID_UINT32_FMT"\n"
                "FetchGroup EndWPID    : %"ID_UINT32_FMT"\n"
                "FetchGroup ReuseWPID  : %"ID_UINT32_FMT"\n"

                "SubHashGroup BeginWPID : %"ID_UINT32_FMT"\n"
                "SubHashGroup EndWPID   : %"ID_UINT32_FMT"\n"
                "SubHashGroup ReuseWPID : %"ID_UINT32_FMT"\n"

                "WAExtentListCount     : %"ID_UINT32_FMT"\n"

                "MaxWAExtentCount      : %"ID_UINT32_FMT"\n"
                "AllocWAPageCount      : %"ID_UINT32_FMT"\n"

                "NPageHashBucketCnt    : %"ID_UINT32_FMT"\n"

                "NPageCount            : %"ID_UINT32_FMT"\n"

                "SubHashPageCount      : %"ID_UINT32_FMT"\n"
                "SubHashBuildCount     : %"ID_UINT32_FMT"\n"
                "HashSlotPageCount     : %"ID_UINT32_FMT"\n",
                aWASegment,
                aWASegment->mSpaceID,
                aWASegment->mWorkType,
                ( aWASegment->mWorkType == SDT_WORK_TYPE_HASH ) ? "HASH" : "-",
                aWASegment->mHashSlotCount,

                aWASegment->mIsInMemory == SDT_WORKAREA_OUT_MEMORY ? "OutMemory" : "InMemory",

                aWASegment->mUnique,
                aWASegment->mOpenCursorType,
                aWASegment->mEndWPID,

                aWASegment->mInsertGrp.mBeginWPID,
                aWASegment->mInsertGrp.mEndWPID,
                aWASegment->mInsertGrp.mReuseSeqWPID,

                aWASegment->mFetchGrp.mBeginWPID,
                aWASegment->mFetchGrp.mEndWPID,
                aWASegment->mFetchGrp.mReuseSeqWPID,

                aWASegment->mSubHashGrp.mBeginWPID,
                aWASegment->mSubHashGrp.mEndWPID,
                aWASegment->mSubHashGrp.mReuseSeqWPID,

                aWASegment->mWAExtentInfo.mCount,

                aWASegment->mMaxWAExtentCount,
                aWASegment->mAllocWAPageCount,

                aWASegment->mNPageHashBucketCnt,

                aWASegment->mNPageCount,

                aWASegment->mSubHashPageCount,
                aWASegment->mSubHashBuildCount,
                aWASegment->mHashSlotPageCount );
    return;

    IDE_EXCEPTION_END;

    return;
}

/***************************************************************************
 * Description : hash row header를 dump 한다.
 *
 * aTRPHeader - [IN] dump 대상 Row Header
 * aOutBuf    - [IN] dump한 정보를 담을 Buffer 
 * aOutSize   - [IN] Buffer의 크기
 ****************************************************************************/
void sdtHashModule::dumpTempTRPHeader( void     * aTRPHeader,
                                       SChar    * aOutBuf,
                                       UInt       aOutSize )
{
    sdtHashTRPHdr* sTRPHeader = (sdtHashTRPHdr*)aTRPHeader;

    (void)idlVA::appendFormat( aOutBuf,
                               aOutSize,
                               "DUMP TRPHeader:\n"
                               "mFlag        : %"ID_UINT32_FMT"\n"
                               "mHitSequence : %"ID_UINT32_FMT"\n"
                               "mValueLength : %"ID_UINT32_FMT"\n"
                               "mHashValue   : %"ID_XINT32_FMT"\n"
                               "mNext    : <%"ID_UINT32_FMT
                               ",%"ID_UINT32_FMT">\n"
                               "mChild   : <%"ID_UINT32_FMT
                               ",%"ID_UINT32_FMT">\n",
                               sTRPHeader->mTRFlag,
                               sTRPHeader->mHitSequence,
                               sTRPHeader->mValueLength,
                               sTRPHeader->mHashValue,
                               sTRPHeader->mNextPageID,
                               sTRPHeader->mNextOffset,
                               sTRPHeader->mChildPageID,
                               sTRPHeader->mChildOffset);
    return;
}

/***************************************************************************
 * Description : sub hash를 dump 한다.
 *
 * aSubHash   - [IN] dump 대상 SubHash
 * aOutBuf    - [IN] dump한 정보를 담을 Buffer 
 * aOutSize   - [IN] Buffer의 크기
 ***************************************************************************/
void sdtHashModule::dumpSubHash( sdtSubHash * aSubHash,
                                 SChar      * aOutBuf,
                                 UInt         aOutSize )
{
    UInt i;

    (void)idlVA::appendFormat( aOutBuf,
                               aOutSize,
                               "DUMP SubHash :\n"
                               "mHashLow    : %"ID_XINT32_FMT"..\n"
                               "mKeyCount   : %"ID_UINT32_FMT"\n"
                               "mNextPageID : %"ID_UINT32_FMT"\n"
                               "mNextOffset : %"ID_UINT32_FMT"\n"
                               "%6s %8s %9s %6s %6s\n",
                               (UInt)aSubHash->mHashLow,
                               aSubHash->mKeyCount,
                               aSubHash->mNextPageID,
                               aSubHash->mNextOffset,
                               "KeyIdx",
                               "HashHigh",
                               "HashValue",
                               "PageID",
                               "Offset");

    for( i = 0 ; i < aSubHash->mKeyCount ; i++ )
    {
        (void)idlVA::appendFormat( aOutBuf,
                                   aOutSize,
                                   " [%3"ID_XINT32_FMT"] "
                                   "%8"ID_XINT32_FMT" "
                                   "%7"ID_XINT32_FMT".. "
                                   "%6"ID_UINT32_FMT" "
                                   "%6"ID_UINT32_FMT"\n",
                                   i,
                                   aSubHash->mKey[i].mHashHigh,
                                   (((UInt)aSubHash->mKey[i].mHashHigh << 16) +
                                    ( (UInt)aSubHash->mHashLow << 8 )),
                                   aSubHash->mKey[i].mPageID,
                                   aSubHash->mKey[i].mOffset );
    }

    return;
}

/***************************************************************************
 * Description : hash segment의 특정 WA Page를 dump한다.
 *
 * aWASegment - [IN] dump 대상 Page가 있는 Hash Table의 WASegment
 * aWAPageID  - [IN] dump 대상 Page
 * aOutBuf    - [IN] dump한 정보를 담을 Buffer 
 * aOutSize   - [IN] Buffer의 크기
 ***************************************************************************/
void sdtHashModule::dumpHashTempPage( sdtHashSegHdr  * aWASegment,
                                      scPageID         aWAPageID,
                                      SChar          * aOutBuf,
                                      UInt             aOutSize )
{
    UChar * sPagePtr;
    UInt    sSize;

    if ( aWAPageID >= aWASegment->mEndWPID )
    {
        (void)idlVA::appendFormat( aOutBuf,
                                   aOutSize,
                                   "\nWAPageID %"ID_UINT32_FMT" is too big.\n",
                                   aWAPageID );
        IDE_TEST( ID_TRUE );
    }

    sPagePtr = getWCBPtr( aWASegment,
                          aWAPageID )->mWAPagePtr;

    if ( sPagePtr == NULL )
    {
        (void)idlVA::appendFormat( aOutBuf,
                                   aOutSize,
                                   "\nWAPageID %"ID_UINT32_FMT" is empty.\n",
                                   aWAPageID );
        IDE_TEST( ID_TRUE );
    }

    if ( aWAPageID < aWASegment->mHashSlotPageCount )
    {
        (void)idlVA::appendFormat( aOutBuf,
                                   aOutSize,
                                   "\nDUMP HASH TEMP PAGE:\n" );

        sSize = idlOS::strlen( aOutBuf );

        IDE_TEST( ideLog::ideMemToHexStr( (UChar*)sPagePtr,
                                          SD_PAGE_SIZE,
                                          IDE_DUMP_FORMAT_FULL,
                                          aOutBuf + sSize,
                                          aOutSize - sSize )
                  != IDE_SUCCESS );

        (void)idlVA::appendFormat( aOutBuf, aOutSize, "\n\n");

        // Hash Slot Page
        dumpWAHashPage( sPagePtr,
                        aWAPageID,
                        ID_TRUE,
                        aOutBuf,
                        aOutSize);
    }
    else
    {
        dumpHashRowPage( sPagePtr,
                         aOutBuf,
                         aOutSize );
    }

    (void)idlVA::appendFormat( aOutBuf, aOutSize, "\n\n");

    return;

    IDE_EXCEPTION_END;

    return;
}

/***************************************************************************
 * Description : Hash segment의 모든 hash page의 header를 dump한다.
 *
 * aWASegment - [IN] dump 대상 Hash WASegment
 * aOutBuf    - [IN] dump한 정보를 담을 Buffer 
 * aOutSize   - [IN] Buffer의 크기
 ***************************************************************************/
void sdtHashModule::dumpHashPageHeaders( sdtHashSegHdr  * aWASegment,
                                         SChar          * aOutBuf,
                                         UInt             aOutSize )
{
    sdtHashPageHdr * sPagePtr;
    sdtWCB         * sWCBPtr;
    sdtWCB         * sEndWCBPtr;
    scPageID         sWPageID;

    (void)idlVA::appendFormat( aOutBuf,
                               aOutSize,
                               "HASH TEMP PAGE HEADERS:\n"
                               "%10s %10s %16s %10s %10s %10s\n",
                               "WPID",
                               "TYPE",
                               "TYPENAME",
                               "FREEOFF",
                               "SELFPID",
                               "WCB->NPID" );

    sWCBPtr    = aWASegment->mWCBMap;
    sEndWCBPtr = sWCBPtr + aWASegment->mHashSlotPageCount ;

    for( sWPageID = 0 ; sWCBPtr < sEndWCBPtr; sWCBPtr++, sWPageID++ )
    {
        if ( sWCBPtr->mWAPagePtr != NULL )
        {
            (void)idlVA::appendFormat( aOutBuf,
                                       aOutSize,
                                       "%10"ID_UINT32_FMT
                                       " %27s\n",
                                       sWPageID,
                                       "HASH_SLOT_PAGE");
        }
    }

    sEndWCBPtr = aWASegment->mWCBMap + (aWASegment->mMaxWAExtentCount * SDT_WAEXTENT_PAGECOUNT);

    for( ; sWCBPtr < sEndWCBPtr; sWCBPtr++, sWPageID++ )
    {
        sPagePtr = (sdtHashPageHdr*)sWCBPtr->mWAPagePtr;

        if ( sPagePtr == NULL )
        {
            continue;
        }

        if ( sWCBPtr->mNPageID == SM_NULL_PID )
        {
            continue;
        }

        (void)idlVA::appendFormat( aOutBuf,
                                   aOutSize,
                                   "%10"ID_UINT32_FMT
                                   " %10"ID_UINT32_FMT,
                                   sWPageID,
                                   sPagePtr->mType );

        if ( SDT_TEMP_PAGETYPE_HASHROWS == sPagePtr->mType )
        {
            (void)idlVA::appendFormat( aOutBuf,
                                       aOutSize,
                                       "    HASH_ROW_PAGE" );

        }
        else if ( SDT_TEMP_PAGETYPE_SUBHASH == sPagePtr->mType )
        {
            (void)idlVA::appendFormat( aOutBuf,
                                       aOutSize,
                                       "    SUB_HASH_PAGE" );
        }
        else
        {
            (void)idlVA::appendFormat( aOutBuf,
                                       aOutSize,
                                       "                 " );
        }
        (void)idlVA::appendFormat( aOutBuf,
                                   aOutSize,
                                   " %10"ID_UINT32_FMT
                                   " %10"ID_UINT32_FMT
                                   " %10"ID_UINT32_FMT"\n",
                                   sPagePtr->mFreeOffset,
                                   sPagePtr->mSelfNPID,
                                   sWCBPtr->mNPageID );
    }

    (void)idlVA::appendFormat( aOutBuf, aOutSize, "\n" );

    return;
}

/***************************************************************************
 * Description : Hash Row Page를 dump한다.
 *
 * aPagePtr   - [IN] dump 대상 Hash Row Page
 * aOutBuf    - [IN] dump한 정보를 담을 Buffer 
 * aOutSize   - [IN] Buffer의 크기
 ***************************************************************************/
void sdtHashModule::dumpHashRowPage( void   * aPagePtr,
                                     SChar  * aOutBuf,
                                     UInt     aOutSize )
{
    sdtHashPageHdr * sPageHdr;
    UChar          * sPagePtr = (UChar*)aPagePtr;
    sdtSubHash     * sSubHash;
    sdtHashTRPHdr  * sTRPHeader;
    UInt             sIdx;
    UInt             sSize;
    scOffset         sCurOffset;


    (void)idlVA::appendFormat( aOutBuf,
                               aOutSize,
                               "\nDUMP HASH TEMP PAGE:\n" );
    sSize = idlOS::strlen( aOutBuf );
    IDE_TEST( ideLog::ideMemToHexStr( (UChar*)sPagePtr,
                                      SD_PAGE_SIZE,
                                      IDE_DUMP_FORMAT_FULL,
                                      aOutBuf + sSize,
                                      aOutSize - sSize )
              != IDE_SUCCESS );

    sPageHdr = (sdtHashPageHdr*)sPagePtr;
    (void)idlVA::appendFormat( aOutBuf,
                               aOutSize,
                               "\n\nDUMP TEMP PAGE HEADER:\n"
                               "mSelfNPID   : %"ID_UINT32_FMT"\n"
                               "mFreeOffset : %"ID_UINT32_FMT"\n"
                               "mType       : %"ID_UINT32_FMT"\n",
                               sPageHdr->mType,
                               sPageHdr->mFreeOffset,
                               sPageHdr->mSelfNPID );

    sIdx = 0;
    sCurOffset = ID_SIZEOF(sdtHashPageHdr) ;

    switch( sPageHdr->mType )
    {
        case SDT_TEMP_PAGETYPE_HASHROWS:
            {
                (void)idlVA::appendFormat( aOutBuf,
                                           aOutSize,
                                           "TypeName    : HASH_ROW_PAGE\n\n"
                                           "Row :\n" );

                while( sCurOffset < sPageHdr->mFreeOffset )
                {
                    sTRPHeader  = (sdtHashTRPHdr*)((UChar*)sPagePtr + sCurOffset );

                    (void)idlVA::appendFormat( aOutBuf,
                                               aOutSize,
                                               "[%4"ID_UINT32_FMT":%4"ID_UINT32_FMT"] ",
                                               sIdx++,
                                               sCurOffset );

                    sCurOffset += ID_SIZEOF(sdtHashTRPHdr) + sTRPHeader->mValueLength;

                    if ( ( sCurOffset & 0x07 ) != 0 )
                    {
                        sCurOffset += ( 0x08 - ( sCurOffset & 0x07 )) ; // XXX Align 함수로 변경
                    }
                    dumpTempTRPHeader( sTRPHeader,
                                       aOutBuf,
                                       aOutSize );

                    (void)idlVA::appendFormat( aOutBuf,
                                               aOutSize,
                                               "Value : ");

                    sSize = idlOS::strlen( aOutBuf );

                    IDE_TEST( ideLog::ideMemToHexStr( (UChar*)sTRPHeader + ID_SIZEOF( sdtHashTRPHdr ),
                                                      sTRPHeader->mValueLength,
                                                      IDE_DUMP_FORMAT_VALUEONLY,
                                                      aOutBuf + sSize,
                                                      aOutSize - sSize )
                              != IDE_SUCCESS );

                    (void)idlVA::appendFormat( aOutBuf, aOutSize, "\n\n" );
                }
            }
            break;
        case SDT_TEMP_PAGETYPE_SUBHASH:
            {
                (void)idlVA::appendFormat( aOutBuf,
                                           aOutSize,
                                           "TypeName    : SUB_HASH_PAGE\n\n"
                                           "Sub Hash :\n" );

                while( sCurOffset < sPageHdr->mFreeOffset )
                {
                    sSubHash    = (sdtSubHash*)((UChar*)sPagePtr + sCurOffset );

                    (void)idlVA::appendFormat( aOutBuf,
                                               aOutSize,
                                               "[%4"ID_UINT32_FMT":%4"ID_UINT32_FMT"] ",
                                               sIdx++,
                                               sCurOffset );

                    sCurOffset += SDT_SUB_HASH_SIZE + (sSubHash->mKeyCount * ID_SIZEOF(sdtSubHashKey));
                    if ( ( sCurOffset & 0x07 ) != 0 )
                    {
                        sCurOffset += ( 0x08 - ( sCurOffset & 0x07 )) ;
                    }
                    dumpSubHash( sSubHash,
                                 aOutBuf,
                                 aOutSize );

                    (void)idlVA::appendFormat( aOutBuf, aOutSize, "\n\n" );
                }
            }
            break;
        default:
            break;

    }

    (void)idlVA::appendFormat( aOutBuf, aOutSize, "\n\n");

    return;

    IDE_EXCEPTION_END;

    return;
}

/***************************************************************************
 * Description : Hash Map을 dump한다.
 *
 * aWASegment - [IN] dump 대상 Hash WASegment
 * aOutBuf    - [IN] dump한 정보를 담을 Buffer 
 * aOutSize   - [IN] Buffer의 크기
 ***************************************************************************/
void sdtHashModule::dumpWAHashMap( sdtHashSegHdr * aWASeg,
                                   SChar         * aOutBuf,
                                   UInt            aOutSize )
{
    sdtWCB     * sWCBPtr;
    sdtWCB     * sEndWCBPtr;
    scPageID     sWAPageID;
    idBool       sTitle = ID_TRUE;

    if ( aWASeg != NULL )
    {
        (void)idlVA::appendFormat( aOutBuf,
                                   aOutSize,
                                   "DUMP WAMAP: HASH\n"
                                   "       BeginWPID:0\n"
                                   "       EndWPID  :%"ID_UINT32_FMT"\n"
                                   "       AllocPage:%"ID_UINT32_FMT"\n",
                                   aWASeg->mHashSlotPageCount,
                                   aWASeg->mHashSlotAllocPageCount );

        sWCBPtr          = aWASeg->mWCBMap;
        sEndWCBPtr       = sWCBPtr +  aWASeg->mHashSlotPageCount;

        for( sWAPageID = 0 ;
             sWCBPtr < sEndWCBPtr ;
             sWCBPtr++ , sWAPageID++ )
        {
            if ( sWCBPtr->mWAPagePtr != NULL )
            {
                dumpWAHashPage( sWCBPtr->mWAPagePtr,
                                sWAPageID,
                                sTitle,
                                aOutBuf,
                                aOutSize );
                sTitle = ID_FALSE;
            }
        }
        (void)idlVA::appendFormat( aOutBuf, aOutSize, "\n" );
    }

    return;
}

/***************************************************************************
 * Description : Hash Map 중 한 Page를 dump한다.
 *
 * aPagePtr   - [IN] dump 대상 WA Hash Page       
 * aWAPageID  - [IN] dump 대상 hash page의 WAPageID, 출력용
 * aTitle     - [IN] Title을 출력 할 것인지 여부
 * aOutBuf    - [IN] dump한 정보를 담을 Buffer 
 * aOutSize   - [IN] Buffer의 크기
 ***************************************************************************/
void sdtHashModule::dumpWAHashPage( void        * aPagePtr,
                                    scPageID      aWAPageID,
                                    idBool        aTitle,
                                    SChar       * aOutBuf,
                                    UInt          aOutSize )
{
    sdtHashSlot * sSlotPtr;
    UInt          sCurSlotIdx;
    UInt          sEndSlotIdx;

    if ( aPagePtr != NULL )
    {
        if ( aTitle == ID_TRUE )
        {
            (void)idlVA::appendFormat( aOutBuf,
                                       aOutSize,
                                       "[%8s] %7s <%7s,%6s> : %s\n",
                                       "SLOTNO",
                                       "HASHSET",
                                       "NPAGEID",
                                       "OFFSET",
                                       "POINTER");
        }

        (void)idlVA::appendFormat( aOutBuf,
                                   aOutSize,
                                   "WAPageID : %"ID_UINT32_FMT"\n",
                                   aWAPageID );


        sSlotPtr = (sdtHashSlot*)aPagePtr;

        sCurSlotIdx = aWAPageID * (SD_PAGE_SIZE / ID_SIZEOF( sdtHashSlot ));
        sEndSlotIdx = sCurSlotIdx + (SD_PAGE_SIZE / ID_SIZEOF( sdtHashSlot ));

        for(  ;
              sCurSlotIdx < sEndSlotIdx ;
              sSlotPtr++ , sCurSlotIdx++ )
        {
            if ( sSlotPtr->mHashSet != 0 )
            {
                (void)idlVA::appendFormat( aOutBuf,
                                           aOutSize,
                                           "[%8"ID_UINT32_FMT"] "
                                           "%7"ID_XINT32_FMT
                                           " <%8"ID_UINT32_FMT
                                           ", %4"ID_INT32_FMT"> : "
                                           "0x%016"ID_XPOINTER_FMT
                                           " %s\n",
                                           sCurSlotIdx,
                                           sSlotPtr->mHashSet,
                                           sSlotPtr->mPageID,
                                           SDT_DEL_SINGLE_FLAG_IN_OFFSET( sSlotPtr->mOffset ),
                                           (void*)sSlotPtr->mRowPtrOrHashSet,
                                           SDT_IS_SINGLE_ROW( sSlotPtr->mOffset ) ? "(Single Row)" : "" );
            }
        }

        (void)idlVA::appendFormat( aOutBuf, aOutSize, "\n" );
    }

    return;
}

/***************************************************************************
 * Description : Insert용 Temp Row Info를 Dump한다.
 *
 * aTRPInfo   - [IN] dump 할 TRPInfo
 * aOutBuf    - [IN] dump한 정보를 담을 Buffer 
 * aOutSize   - [IN] Buffer의 크기
 ***************************************************************************/
void sdtHashModule::dumpTempTRPInfo4Insert( void       * aTRPInfo,
                                            SChar      * aOutBuf,
                                            UInt         aOutSize )
{
    sdtHashInsertInfo * sTRPInfo = (sdtHashInsertInfo*)aTRPInfo;
    UInt                sSize;
    UInt                i;

    dumpTempTRPHeader( &sTRPInfo->mTRPHeader, aOutBuf, aOutSize );

    for( i = 0 ; i < sTRPInfo->mColumnCount ; i++ )
    {
        (void)idlVA::appendFormat( aOutBuf,
                                    aOutSize,
                                    "[%2"ID_UINT32_FMT"] Length:%4"ID_UINT32_FMT"\n",
                                    i,
                                    sTRPInfo->mValueList[ i ].length );

        (void)idlVA::appendFormat( aOutBuf,
                                    aOutSize,
                                    "Value : ");
        sSize = idlOS::strlen( aOutBuf );
        IDE_TEST( ideLog::ideMemToHexStr(
                      (UChar*)sTRPInfo->mValueList[ i ].value,
                      sTRPInfo->mValueList[ i ].length ,
                      IDE_DUMP_FORMAT_VALUEONLY,
                      aOutBuf + sSize,
                      aOutSize - sSize )
                  != IDE_SUCCESS );
        (void)idlVA::appendFormat( aOutBuf, aOutSize, "\n\n" );
    }

    return;

    IDE_EXCEPTION_END;

    return;
}


/**************************************************************************
 * Description : Hash Segment의 모든 WCB의 정보를 출력함
 *
 * aWASegment - [IN] WCB를 Dump 할 Hash Segment
 * aOutBuf    - [IN] Dump한 내용을 담아둘 출력 Buffer
 * aOutSize   - [IN] 출력 Buffer의 Size
 ***************************************************************************/
void sdtHashModule::dumpWCBs( void    * aWASegment,
                              SChar   * aOutBuf,
                              UInt      aOutSize )
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

    sWCBPtr    = ((sdtHashSegHdr*)aWASegment)->mWCBMap;
    sEndWCBPtr = sWCBPtr + sdtHashModule::getMaxWAPageCount( (sdtHashSegHdr*)aWASegment );

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
void sdtHashModule::dumpWCB( void    * aWCBPtr,
                             SChar   * aOutBuf,
                             UInt      aOutSize )
{
    sdtWCB   * sWCBPtr;

    sWCBPtr = (sdtWCB*) aWCBPtr;

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
