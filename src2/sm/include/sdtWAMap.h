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
 * $Id
 *
 * - WorkAreaMap
 *     SortTemp의 extractNSort&insertNSort시의 SortGroup에 존재하는 KeyMap과
 *     Merge시의 Heap, UniquehashTemp의 HashGroup들은 다음 두가지 공통점이
 *     있다.
 *          WAPage상에만 존재한다. 즉 NPage로 내려가지 않는다.
 *          즉 Pointing하기 위한 정보들이 Array/Map식으로 존재한다.
 *     위 세가지 모두 독자구현을 하려 하였으나, 상세설계 도중 중복코드가 될거
 *     같아서 sdtWAMap으로 하나로 합친다.
 *
 * - 특징
 *     NPage와 Assign 돼지 않는다.
 *     Array를 구성하는 Slot의 크기는 자유이다.
 *     Address를 바탕으로 해당 Array에 Random Access 하는 식이다.
 **********************************************************************/

#ifndef _O_SDT_WA_MAP_H_
#define _O_SDT_WA_MAP_H_ 1

#include <idl.h>
#include <smDef.h>
#include <smiDef.h>
#include <sdtDef.h>
#include <sdtWorkArea.h>
#include <iduLatch.h>

class sdtWAMap
{
public:
    static IDE_RC create( sdtWASegment * aWASegment,
                          sdtWAGroupID   aWAGID,
                          sdtWMType      aWMType,
                          UInt           aSlotCount,
                          UInt           aVersionCount,
                          void         * aRet );
    static IDE_RC disable( void         * aMapHdr );

    static IDE_RC resetPtrAddr( void         * aWAMapHdr,
                                sdtWASegment * aWASegment );
    inline static IDE_RC set( void * aWAMapHdr, UInt aIdx, void * aValue );
    inline static IDE_RC setNextVersion( void * aWAMapHdr,
                                         UInt   aIdx,
                                         void * aValue );
    inline static IDE_RC setvULong( void   * aWAMapHdr,
                                    UInt     aIdx,
                                    vULong * aValue );

    inline static IDE_RC get( void * aWAMapHdr, UInt aIdx, void * aValue );
    inline static IDE_RC getvULong( void   * aWAMapHdr,
                                    UInt     aIdx,
                                    vULong * aValue );

    inline static IDE_RC getSlotPtr( void *  aWAMapHdr,
                                     UInt    aIdx,
                                     UInt    aSlotSize,
                                     void ** aSlotPtr );
    inline static IDE_RC getSlotPtrWithCheckState( void  * aWAMapHdr,
                                                   UInt    aIdx,
                                                   idBool  aResetPage,
                                                   void ** aSlotPtr );
    inline static IDE_RC getSlotPtrWithVersion( void *  aWAMapHdr,
                                                UInt    aVersionIdx,
                                                UInt    aIdx,
                                                UInt    aSlotSize,
                                                void ** aSlotPtr );
    inline static UChar * getPtrByOffset( void *  aWAMapHdr,
                                          UInt    aOffset );

    static UInt getOffset( UInt   aSlotIdx,
                           UInt   aSlotSize,
                           UInt   aVersionIdx,
                           UInt   aVersionCount )
    {
        return ( ( aSlotIdx * aVersionCount ) + aVersionIdx ) * aSlotSize;
    }

    inline static IDE_RC swap( void * aWAMapHdr, UInt aIdx1, UInt aIdx2 );
    inline static IDE_RC swapvULong( void * aWAMapHdr,
                                     UInt   aIdx1,
                                     UInt   aIdx2 );
    inline static IDE_RC incVersionIdx( void * aWAMapHdr );

    static IDE_RC expand( void     * aWAMapHdr,
                          scPageID   aLimitPID,
                          UInt     * aIdx );

    /* Map의 크기 (Map내부의 Slot개수)를 반환함 */
    static UInt getSlotCount( void * aWAMapHdr )
    {
        sdtWAMapHdr * sWAMap = (sdtWAMapHdr*)aWAMapHdr;

        return sWAMap->mSlotCount;
    }

    /* Slot하나의 크기를 반환함. */
    static UInt getSlotSize( void * aWAMapHdr )
    {
        sdtWAMapHdr * sWAMap = (sdtWAMapHdr*)aWAMapHdr;

        return mWMTypeDesc[ sWAMap->mWMType ].mSlotSize;
    }

    /* WAMap이 사용하는 페이지의 개수를 반환*/
    static UInt getWPageCount( void * aWAMapHdr )
    {
        sdtWAMapHdr * sWAMap = (sdtWAMapHdr*)aWAMapHdr;

        if ( sWAMap == NULL )
        {
            return 0;
        }
        else
        {
            return ( sWAMap->mSlotCount * sWAMap->mSlotSize
                     * sWAMap->mVersionCount
                     / SD_PAGE_SIZE ) + 1;
        }
    }
    /* WAMap이 사용하는 마지막 페이지를 반환함 */
    static scPageID getEndWPID( void * aWAMapHdr )
    {
        sdtWAMapHdr * sWAMap = (sdtWAMapHdr*)aWAMapHdr;

        return sWAMap->mBeginWPID + getWPageCount( aWAMapHdr );
    }


    static void dumpWAMap( void        * aMapHdr,
                           SChar       * aOutBuf,
                           UInt          aOutSize );
private:
    static sdtWMTypeDesc mWMTypeDesc[ SDT_WM_TYPE_MAX ];
};

/**************************************************************************
 * Description :
 * 해당 WAMap에 Index번 Slot에 Value를 설정한다.
 *
 * <IN>
 * aWAMap         - 대상 WAMap
 * aIdx           - 대상 Index
 * aValue         - 설정할 Value
 ***************************************************************************/
IDE_RC sdtWAMap::set( void * aWAMapHdr, UInt aIdx, void * aValue )
{
    sdtWAMapHdr * sWAMapHdr = (sdtWAMapHdr*)aWAMapHdr;
    void        * sSlotPtr;
    UInt          sSlotSize;

    sSlotSize = sWAMapHdr->mSlotSize;

    IDE_TEST( getSlotPtr( sWAMapHdr, aIdx, sSlotSize, &sSlotPtr )
              != IDE_SUCCESS );

    idlOS::memcpy( sSlotPtr, aValue, sSlotSize );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**************************************************************************
 * Description :
 * 해당 WAMap에 Index번 Slot에 Value를 설정하되, '다음 버전'에 설정한다.
 * sdtSortModule::mergeSort 참조
 *
 * <IN>
 * aWAMap         - 대상 WAMap
 * aIdx           - 대상 Index
 * aValue         - 설정할 Value
 ***************************************************************************/
IDE_RC sdtWAMap::setNextVersion( void * aWAMapHdr,
                                 UInt   aIdx,
                                 void * aValue )
{
    sdtWAMapHdr * sWAMapHdr = (sdtWAMapHdr*)aWAMapHdr;
    void        * sSlotPtr;
    UInt          sSlotSize;
    UInt          sVersionIdx;

    sSlotSize = sWAMapHdr->mSlotSize;
    sVersionIdx = ( sWAMapHdr->mVersionIdx + 1 ) % sWAMapHdr->mVersionCount;
    IDE_TEST( getSlotPtrWithVersion( sWAMapHdr,
                                     sVersionIdx,
                                     aIdx,
                                     sSlotSize,
                                     &sSlotPtr )
              != IDE_SUCCESS );

    idlOS::memcpy( sSlotPtr, aValue, sSlotSize );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/**************************************************************************
 * Description :
 * 해당 WAMap에 Index번 Slot에서 vULongValue를 설정한다.
 *
 * <IN>
 * aWAMap         - 대상 WAMap
 * aIdx           - 대상 Index
 * aValue         - 설정할 Value
 ***************************************************************************/
IDE_RC sdtWAMap::setvULong( void * aWAMapHdr, UInt aIdx, vULong * aValue )
{
    sdtWAMapHdr * sWAMapHdr = (sdtWAMapHdr*)aWAMapHdr;
    void        * sSlotPtr;


    IDE_DASSERT( sWAMapHdr->mWMType == SDT_WM_TYPE_POINTER );
    IDE_DASSERT( ID_SIZEOF( vULong ) == getSlotSize( aWAMapHdr ) );
    IDE_TEST( getSlotPtr( sWAMapHdr, aIdx, ID_SIZEOF( vULong ), &sSlotPtr )
              != IDE_SUCCESS );

    *(vULong*)sSlotPtr = *aValue;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**************************************************************************
 * Description :
 * 해당 WAMap에 Index번 Slot에서 Value를 가져온다.
 *
 * <IN>
 * aWAMap         - 대상 WAMap
 * aIdx           - 대상 Index
 * aValue         - 설정할 Value
 ***************************************************************************/
IDE_RC sdtWAMap::get( void * aWAMapHdr, UInt aIdx, void * aValue )
{
    sdtWAMapHdr * sWAMapHdr = (sdtWAMapHdr*)aWAMapHdr;
    void        * sSlotPtr;
    UInt          sSlotSize;

    sSlotSize = sWAMapHdr->mSlotSize;
    IDE_TEST( getSlotPtr( sWAMapHdr, aIdx, sSlotSize, &sSlotPtr )
              != IDE_SUCCESS );

    idlOS::memcpy( aValue, sSlotPtr, sSlotSize );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**************************************************************************
 * Description :
 * 해당 WAMap에 Index번 Slot에서 vULongValue를 가져온다.
 *
 * <IN>
 * aWAMap         - 대상 WAMap
 * aIdx           - 대상 Index
 * aValue         - 설정할 Value
 ***************************************************************************/
IDE_RC sdtWAMap::getvULong( void * aWAMapHdr, UInt aIdx, vULong * aValue )
{
    sdtWAMapHdr * sWAMapHdr = (sdtWAMapHdr*)aWAMapHdr;
    void        * sSlotPtr;

    IDE_DASSERT( sWAMapHdr->mWMType == SDT_WM_TYPE_POINTER );
    IDE_DASSERT( ID_SIZEOF( vULong ) == getSlotSize( aWAMapHdr ) );
    IDE_TEST( getSlotPtr( sWAMapHdr, aIdx, ID_SIZEOF( vULong ), &sSlotPtr )
              != IDE_SUCCESS );

    *aValue = *(vULong*)sSlotPtr;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**************************************************************************
 * Description :
 * WAMap의 해당 Slot의 주소값을 반환한다.
 *
 * <IN>
 * aWAMap         - 대상 WAMap
 * aIdx           - 대상 Index
 * aSlotSize      - 대상 WAMap에서 한 Slot의 크기
 * <OUT>
 * aSlotPtr       - 찾은 Slot의 위치
 ***************************************************************************/
IDE_RC sdtWAMap::getSlotPtr( void *  aWAMapHdr,
                             UInt    aIdx,
                             UInt    aSlotSize,
                             void ** aSlotPtr )
{
    sdtWAMapHdr * sWAMapHdr = (sdtWAMapHdr*)aWAMapHdr;

    return getSlotPtrWithVersion( aWAMapHdr,
                                  sWAMapHdr->mVersionIdx,
                                  aIdx,
                                  aSlotSize,
                                  aSlotPtr );
}

/**************************************************************************
 * Description :
 * getSlotPtr과 같되, WAMap Slot을 소유한 Page의 상태를 확인하고 가져온다.
 * PageState가 none이면 최초 접근이라 초기화되어있지 않아 DirtyValue를
 * 읽기 때문이다.
 * 그러면 초기화 해주고 PageState를 Init으로 변경한다.
 * sdtUniqueHashModule::insert 참조
 *
 * <IN>
 * aWAMap         - 대상 WAMap
 * aIdx           - 대상 Index
 * <OUT>
 * aSlotPtr       - 찾은 Slot의 위치
 ***************************************************************************/
IDE_RC sdtWAMap::getSlotPtrWithCheckState( void    * aWAMapHdr,
                                           UInt      aIdx,
                                           idBool    aResetPage,
                                           void   ** aSlotPtr )
{
    sdtWAMapHdr    * sWAMapHdr = (sdtWAMapHdr*)aWAMapHdr;
    sdtWASegment   * sWASeg = (sdtWASegment*)sWAMapHdr->mWASegment;
    UInt             sSlotSize;
    UInt             sOffset;
    void           * sSlotPtr;
    scPageID         sWPID;
    sdtWAPageState   sWAPageState;
    sdtWCB         * sWCBPtr;

    sSlotSize   = sWAMapHdr->mSlotSize;
    sOffset     = getOffset( aIdx,
                             sSlotSize,
                             sWAMapHdr->mVersionIdx,
                             sWAMapHdr->mVersionCount );
    sWPID       = sWAMapHdr->mBeginWPID + ( sOffset / SD_PAGE_SIZE );

    sWCBPtr      = sdtWASegment::getWCB( sWASeg, sWPID );
    sWAPageState = sdtWASegment::getWAPageState( sWCBPtr );
    sSlotPtr     = sWCBPtr->mWAPagePtr + ( sOffset % SD_PAGE_SIZE );

    if ( sWAPageState == SDT_WA_PAGESTATE_NONE )
    {
        if ( aResetPage == ID_TRUE )
        {
            idlOS::memset( sWCBPtr->mWAPagePtr, 0, SD_PAGE_SIZE );
            IDE_TEST( sdtWASegment::setWAPageState( sWASeg,
                                                    sWPID,
                                                    SDT_WA_PAGESTATE_INIT )
                      != IDE_SUCCESS );
        }
        else
        {
            idlOS::memset( sSlotPtr, 0, sSlotSize );
        }
    }

    *aSlotPtr = sSlotPtr;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/**************************************************************************
 * Description :
 * WAMap의 해당 Slot의 주소값을 반환한다. Version을 고려하여 가져온다.
 *
 * <IN>
 * aWAMap         - 대상 WAMap
 * aIdx           - 대상 Index
 * aSlotSize      - 대상 WAMap에서 한 Slot의 크기
 * <OUT>
 * aSlotPtr       - 찾은 Slot의 위치
 ***************************************************************************/
IDE_RC sdtWAMap::getSlotPtrWithVersion( void *  aWAMapHdr,
                                        UInt    aVersionIdx,
                                        UInt    aIdx,
                                        UInt    aSlotSize,
                                        void ** aSlotPtr )
{
    sdtWAMapHdr * sWAMapHdr = (sdtWAMapHdr*)aWAMapHdr;

    IDE_DASSERT( aIdx < sWAMapHdr->mSlotCount );

    *aSlotPtr = (void*)
        getPtrByOffset( aWAMapHdr,
                        getOffset( aIdx,
                                   aSlotSize,
                                   aVersionIdx,
                                   sWAMapHdr->mVersionCount ) );

    return IDE_SUCCESS;
}

/**************************************************************************
 * Description :
 * WAMap의 해당 Slot의 주소값을 Offset을 바탕으로 찾는다.
 *
 * <IN>
 * aWAMap         - 대상 WAMap
 * aOffset        - 대상 Offset
 * <OUT>
 * aSlotPtr       - 찾은 Slot의 위치
 ***************************************************************************/
UChar * sdtWAMap::getPtrByOffset( void *  aWAMapHdr,
                                  UInt    aOffset )
{
    sdtWAMapHdr * sWAMapHdr = (sdtWAMapHdr*)aWAMapHdr;

    return sdtWASegment::getWAPagePtr( (sdtWASegment*)sWAMapHdr->mWASegment,
                                        SDT_WAGROUPID_NONE,
                                        sWAMapHdr->mBeginWPID
                                        + ( aOffset / SD_PAGE_SIZE ) )
                                        + ( aOffset % SD_PAGE_SIZE );
}

/**************************************************************************
 * Description :
 * 두 Slot의 값을 교환한다.
 * sdtTempTable::quickSort 참조
 *
 * <IN>
 * aWAMap         - 대상 WAMap
 * aIdx1,aIdx2    - 치환할 두 Slot
 ***************************************************************************/
IDE_RC sdtWAMap::swap( void * aWAMapHdr, UInt aIdx1, UInt aIdx2 )
{
    sdtWAMapHdr * sWAMapHdr = (sdtWAMapHdr*)aWAMapHdr;
    UChar         sBuffer[ SDT_WAMAP_SLOT_MAX_SIZE ];
    UInt          sSlotSize;
    void        * sSlotPtr1;
    void        * sSlotPtr2;

    if ( aIdx1 == aIdx2 )
    {
        /* 같으면 교환할 필요 없음 */
    }
    else
    {
        sSlotSize = sWAMapHdr->mSlotSize;

        IDE_TEST( getSlotPtr( sWAMapHdr, aIdx1, sSlotSize, &sSlotPtr1 )
                  != IDE_SUCCESS );
        IDE_TEST( getSlotPtr( sWAMapHdr, aIdx2, sSlotSize, &sSlotPtr2 )
                  != IDE_SUCCESS );

#if defined(DEBUG)
        /* 다른 두 값을 비교해야 한다. */
        IDE_ERROR( sSlotPtr1 != sSlotPtr2 );
        /* 두 Slot간의 차이가 Slot의 크기보다 같거나 커서,
         * 겹치지 않도록 해야 한다. */
        if ( sSlotPtr1 < sSlotPtr2 )
        {
            IDE_ERROR( ((UChar*)sSlotPtr2) - ((UChar*)sSlotPtr1) >= sSlotSize );
        }
        else
        {
            IDE_ERROR( ((UChar*)sSlotPtr1) - ((UChar*)sSlotPtr2) >= sSlotSize );
        }
#endif

        idlOS::memcpy( sBuffer, sSlotPtr1, sSlotSize );
        idlOS::memcpy( sSlotPtr1, sSlotPtr2, sSlotSize );
        idlOS::memcpy( sSlotPtr2, sBuffer, sSlotSize );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**************************************************************************
 * Description :
 * 두 vULongSlot의 값을 교환한다.
 * sdtTempTable::quickSort 참조
 *
 * <IN>
 * aWAMap         - 대상 WAMap
 * aIdx1,aIdx2    - 치환할 두 Slot
 ***************************************************************************/
IDE_RC sdtWAMap::swapvULong( void * aWAMapHdr, UInt aIdx1, UInt aIdx2 )
{
    sdtWAMapHdr * sWAMapHdr = (sdtWAMapHdr*)aWAMapHdr;
    UInt          sSlotSize;
    vULong      * sSlotPtr1;
    vULong      * sSlotPtr2;
    vULong        sValue;

    IDE_DASSERT( sWAMapHdr->mWMType == SDT_WM_TYPE_POINTER );
    IDE_DASSERT( ID_SIZEOF( vULong ) == getSlotSize( aWAMapHdr ) );

    if ( aIdx1 == aIdx2 )
    {
        /* 같으면 교환할 필요 없음 */
    }
    else
    {
        sSlotSize = ID_SIZEOF( vULong );

        IDE_TEST( getSlotPtr( sWAMapHdr, aIdx1, sSlotSize, (void**)&sSlotPtr1)
                  != IDE_SUCCESS );
        IDE_TEST( getSlotPtr( sWAMapHdr, aIdx2, sSlotSize, (void**)&sSlotPtr2)
                  != IDE_SUCCESS );

#if defined(DEBUG)
        /* 다른 두 값을 비교해야 한다. */
        IDE_ERROR( sSlotPtr1 != sSlotPtr2 );
        /* 두 Slot간의 차이가 Slot의 크기보다 같거나 커서,
         * 겹치지 않도록 해야 한다. */
        if ( sSlotPtr1 < sSlotPtr2 )
        {
            IDE_ERROR( ((UChar*)sSlotPtr2) - ((UChar*)sSlotPtr1) >= sSlotSize );
        }
        else
        {
            IDE_ERROR( ((UChar*)sSlotPtr1) - ((UChar*)sSlotPtr2) >= sSlotSize );
        }
#endif

        sValue     = *sSlotPtr1;
        *sSlotPtr1 = *sSlotPtr2;
        *sSlotPtr2 = sValue;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**************************************************************************
 * Description :
 * Map의 버전을 증가시킨다.
 *
 * <IN>
 * aWAMap         - 대상 WAMap
 ***************************************************************************/
IDE_RC sdtWAMap::incVersionIdx( void * aWAMapHdr )
{
    sdtWAMapHdr * sWAMap = (sdtWAMapHdr*)aWAMapHdr;

    sWAMap->mVersionIdx = ( sWAMap->mVersionIdx + 1 ) % sWAMap->mVersionCount;

    return IDE_SUCCESS;
}

#endif //_O_SDT_WA_MAP_H_
