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
 * - WorkAreaSortMap
 *     SortTemp의 extractNSort&insertNSort시의 SortGroup에 존재하는 KeyMap과
 *     Merge시의 Heap들은 다음 두가지 공통점이 있다.
 *          WAPage상에만 존재한다. 즉 NPage로 내려가지 않는다.
 *          즉 Pointing하기 위한 정보들이 Array/Map식으로 존재한다.
 *     위 세가지 모두 독자구현을 하려 하였으나, 상세설계 도중 중복코드가 될거
 *     같아서 sdtWASortMap으로 하나로 합친다.
 *
 * - 특징
 *     NPage와 Assign 돼지 않는다.
 *     Array를 구성하는 Slot의 크기는 자유이다.
 *     Address를 바탕으로 해당 Array에 Random Access 하는 식이다.
 **********************************************************************/

#ifndef _O_SDT_WA_SORT_MAP_H_
#define _O_SDT_WA_SORT_MAP_H_ 1

#include <idl.h>
#include <smDef.h>
#include <smiDef.h>
#include <sdtDef.h>
#include <iduLatch.h>
#include <sdtSortSegment.h>

class sdtWASortMap
{
public:
    static IDE_RC create( sdtSortSegHdr* aWASegment,
                          sdtGroupID     aWAGID,
                          sdtWMType      aWMType,
                          UInt           aSlotCount,
                          UInt           aVersionCount,
                          sdtWASortMapHdr  * aWAMapHdr );
    static void disable(  sdtSortSegHdr * aWASegment );

    static IDE_RC resetPtrAddr( sdtSortSegHdr* aWASegment );
    inline static IDE_RC set( sdtWASortMapHdr * aWAMapHdr,
                              UInt              aIdx,
                              void            * aValue );
    inline static IDE_RC setNextVersion( sdtWASortMapHdr * aWAMapHdr,
                                         UInt   aIdx,
                                         void * aValue );
    inline static IDE_RC setvULong( sdtWASortMapHdr   * aWAMapHdr,
                                    UInt     aIdx,
                                    vULong * aValue );

    inline static IDE_RC get( sdtWASortMapHdr * aWAMapHdr,
                              UInt              aIdx,
                              void            * aValue );
    inline static IDE_RC getvULong( sdtWASortMapHdr   * aWAMapHdr,
                                    UInt     aIdx,
                                    vULong * aValue );

    inline static IDE_RC getSlotPtr( sdtWASortMapHdr *  aWAMapHdr,
                                     UInt    aIdx,
                                     UInt    aSlotSize,
                                     void ** aSlotPtr );

    inline static IDE_RC getSlotPtrWithVersion( sdtWASortMapHdr *  aWAMapHdr,
                                                UInt    aVersionIdx,
                                                UInt    aIdx,
                                                UInt    aSlotSize,
                                                void ** aSlotPtr );

    static UInt getOffset( UInt   aSlotIdx,
                           UInt   aSlotSize,
                           UInt   aVersionIdx,
                           UInt   aVersionCount )
    {
        return ( ( aSlotIdx * aVersionCount ) + aVersionIdx ) * aSlotSize;
    }

    inline static IDE_RC swap( sdtWASortMapHdr * aWAMapHdr,
                               UInt aIdx1,
                               UInt aIdx2 );
    inline static IDE_RC swapvULong( sdtWASortMapHdr * aWAMapHdr,
                                     UInt   aIdx1,
                                     UInt   aIdx2 );
    inline static IDE_RC incVersionIdx( sdtWASortMapHdr * aWAMapHdr );

    static IDE_RC expand( sdtWASortMapHdr * aWAMapHdr,
                          scPageID   aLimitPID,
                          UInt     * aIdx );

    /* Map의 크기 (Map내부의 Slot개수)를 반환함 */
    static UInt getSlotCount( sdtWASortMapHdr * aWAMapHdr )
    {
        return aWAMapHdr->mSlotCount;
    }

    /* Slot하나의 크기를 반환함. */
    static UInt getSlotSize( sdtWASortMapHdr * aWAMapHdr )
    {
        return aWAMapHdr->mSlotSize;
    }

    /* WAMap이 사용하는 페이지의 개수를 반환*/
    static UInt getWPageCount( sdtWASortMapHdr * aWAMapHdr )
    {
        if ( aWAMapHdr == NULL )
        {
            return 0;
        }
        else
        {
            return ( aWAMapHdr->mSlotCount * aWAMapHdr->mSlotSize
                     * aWAMapHdr->mVersionCount
                     / SD_PAGE_SIZE ) + 1;
        }
    }
    /* WAMap이 사용하는 마지막 페이지를 반환함 */
    static scPageID getEndWPID( sdtWASortMapHdr * aWAMapHdr )
    {
        return aWAMapHdr->mBeginWPID + getWPageCount( aWAMapHdr );
    }


    static void dumpWASortMap( sdtWASortMapHdr * aMapHdr,
                               SChar           * aOutBuf,
                               UInt              aOutSize );
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
IDE_RC sdtWASortMap::set( sdtWASortMapHdr * aWAMapHdr,
                          UInt              aIdx,
                          void            * aValue )
{
    void        * sSlotPtr;
    UInt          sSlotSize;

    sSlotSize = aWAMapHdr->mSlotSize;

    IDE_TEST( getSlotPtr( aWAMapHdr, aIdx, sSlotSize, &sSlotPtr )
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
IDE_RC sdtWASortMap::setNextVersion( sdtWASortMapHdr * aWAMapHdr,
                                     UInt              aIdx,
                                     void            * aValue )
{
    void        * sSlotPtr;
    UInt          sSlotSize;
    UInt          sVersionIdx;

    sSlotSize = aWAMapHdr->mSlotSize;
    sVersionIdx = ( aWAMapHdr->mVersionIdx + 1 ) % aWAMapHdr->mVersionCount;
    IDE_TEST( getSlotPtrWithVersion( aWAMapHdr,
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
IDE_RC sdtWASortMap::setvULong( sdtWASortMapHdr * aWAMapHdr,
                                UInt              aIdx,
                                vULong          * aValue )
{
    void * sSlotPtr;

    IDE_DASSERT( ID_SIZEOF( vULong ) == getSlotSize( aWAMapHdr ) );
    IDE_TEST( getSlotPtr( aWAMapHdr, aIdx, ID_SIZEOF( vULong ), &sSlotPtr )
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
IDE_RC sdtWASortMap::get( sdtWASortMapHdr * aWAMapHdr,
                          UInt              aIdx,
                          void            * aValue )
{
    void   * sSlotPtr;
    UInt     sSlotSize;

    sSlotSize = aWAMapHdr->mSlotSize;
    IDE_TEST( getSlotPtr( aWAMapHdr, aIdx, sSlotSize, &sSlotPtr )
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
IDE_RC sdtWASortMap::getvULong( sdtWASortMapHdr * aWAMapHdr,
                                UInt              aIdx,
                                vULong          * aValue )
{
    void   * sSlotPtr;

    IDE_DASSERT( ID_SIZEOF( vULong ) == getSlotSize( aWAMapHdr ) );
    IDE_TEST( getSlotPtr( aWAMapHdr, aIdx, ID_SIZEOF( vULong ), &sSlotPtr )
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
IDE_RC sdtWASortMap::getSlotPtr( sdtWASortMapHdr *  aWAMapHdr,
                                 UInt               aIdx,
                                 UInt               aSlotSize,
                                 void            ** aSlotPtr )
{
    return getSlotPtrWithVersion( aWAMapHdr,
                                  aWAMapHdr->mVersionIdx,
                                  aIdx,
                                  aSlotSize,
                                  aSlotPtr );
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
IDE_RC sdtWASortMap::getSlotPtrWithVersion( sdtWASortMapHdr * aWAMapHdr,
                                            UInt              aVersionIdx,
                                            UInt              aIdx,
                                            UInt              aSlotSize,
                                            void           ** aSlotPtr )
{
    UInt        sOffset;
    scPageID    sPageID;
    UChar     * sPagePtr;
    sdtWCB    * sWCBPtr;

    IDE_DASSERT( aIdx < aWAMapHdr->mSlotCount );

    sOffset = getOffset( aIdx,
                         aSlotSize,
                         aVersionIdx,
                         aWAMapHdr->mVersionCount );
    sPageID = aWAMapHdr->mBeginWPID + ( sOffset / SD_PAGE_SIZE ) ;

    sWCBPtr = sdtSortSegment::getWCBWithLnk( aWAMapHdr->mWASegment,
                                             sPageID );

    IDE_TEST( sdtSortSegment::chkAndSetWAPagePtr( aWAMapHdr->mWASegment,
                                                  sWCBPtr ) != IDE_SUCCESS );

    sPagePtr = sdtSortSegment::getWAPagePtr( sWCBPtr );

    *aSlotPtr = sPagePtr + ( sOffset % SD_PAGE_SIZE );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
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
IDE_RC sdtWASortMap::swap( sdtWASortMapHdr * aWAMapHdr,
                           UInt              aIdx1,
                           UInt              aIdx2 )
{
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
        sSlotSize = aWAMapHdr->mSlotSize;

        IDE_TEST( getSlotPtr( aWAMapHdr, aIdx1, sSlotSize, &sSlotPtr1 )
                  != IDE_SUCCESS );
        IDE_TEST( getSlotPtr( aWAMapHdr, aIdx2, sSlotSize, &sSlotPtr2 )
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
IDE_RC sdtWASortMap::swapvULong( sdtWASortMapHdr * aWAMapHdr,
                                 UInt              aIdx1,
                                 UInt              aIdx2 )
{
    UInt          sSlotSize;
    vULong      * sSlotPtr1;
    vULong      * sSlotPtr2;
    vULong        sValue;

    IDE_DASSERT( ID_SIZEOF( vULong ) == getSlotSize( aWAMapHdr ) );

    if ( aIdx1 == aIdx2 )
    {
        /* 같으면 교환할 필요 없음 */
    }
    else
    {
        sSlotSize = ID_SIZEOF( vULong );

        IDE_TEST( getSlotPtr( aWAMapHdr, aIdx1, sSlotSize, (void**)&sSlotPtr1)
                  != IDE_SUCCESS );
        IDE_TEST( getSlotPtr( aWAMapHdr, aIdx2, sSlotSize, (void**)&sSlotPtr2)
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
IDE_RC sdtWASortMap::incVersionIdx( sdtWASortMapHdr * aWAMapHdr )
{
    aWAMapHdr->mVersionIdx = ( aWAMapHdr->mVersionIdx + 1 ) % aWAMapHdr->mVersionCount;

    return IDE_SUCCESS;
}

#endif //_O_SDT_WA_SORT_MAP_H_
