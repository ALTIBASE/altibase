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
 * Description :
 *
 * TempPage
 * Persistent Page의 sdpPhyPage에 대응되는 파일 및 구조체이다.
 * TempPage는 LSN, SmoNo, PageState, Consistent 등 sdpPhyPageHdr에 있는 상당
 * 수의 내용과 무관하다.
 * 따라서 공간효율성 등을 위해 이를 모두 무시한다.
 *
 *  # 관련 자료구조
 *
 *    - sdpPageHdr 구조체
 *
 **********************************************************************/

#ifndef _O_SDT_PAGE_H_
#define _O_SDT_PAGE_H_ 1

#include <smDef.h>
#include <sdr.h>
#include <smu.h>
#include <sdtSortDef.h>
#include <smrCompareLSN.h>

class sdtTempPage
{
public:
    static void init( sdtSortPageHdr  * aPageHdr,
                      sdtTempPageType   aType,
                      scPageID   aPrev,
                      scPageID   aSelf,
                      scPageID   aNext );

    inline static scSlotNum allocSlotDir( UChar * aPagePtr );

    inline static UChar * allocSlot( sdtSortPageHdr * aPageHdr,
                                     scSlotNum   aSlotNo,
                                     UInt        aNeedSize,
                                     UInt        aMinSize,
                                     UInt      * aRetSize );

    inline static IDE_RC getSlotPtr( UChar      * aPagePtr,
                                     scSlotNum    aSlotNo,
                                     UChar     ** aSlotPtr );

    inline static UInt getSlotDirOffset( scSlotNum    aSlotNo )
    {
        return SDT_SORT_PAGE_HEADER_SIZE + ( aSlotNo * ID_SIZEOF( scOffset ) );
    };

    inline static scOffset getSlotOffset( UChar      * aPagePtr,
                                          scSlotNum    aSlotNo )
    {
        return ((sdtSortPageHdr*)aPagePtr)->mSlotDir[aSlotNo];
    };

    inline static UInt getSlotCount( UChar * aPagePtr );

    /********************** GetSet ************************/
    static scPageID getPrevPID( UChar * aPagePtr )
    {
        return ( (sdtSortPageHdr*)aPagePtr )->mPrevPID;
    }
    static scPageID getSelfPID( UChar * aPagePtr )
    {
        return ( (sdtSortPageHdr*)aPagePtr )->mSelfPID;
    }
    static scPageID getNextPID( UChar * aPagePtr )
    {
        return ( (sdtSortPageHdr*)aPagePtr )->mNextPID;
    }

    static void setNextPID( UChar * aPagePtr, scPageID aNextNPID )
    {
        ((sdtSortPageHdr*)aPagePtr)->mNextPID = aNextNPID;
    }

    static sdtTempPageType getType( sdtSortPageHdr *aPageHdr )
    {
        return (sdtTempPageType)(aPageHdr->mTypeAndFreeOffset >> SDT_TEMP_TYPE_SHIFT);
    }

    /* FreeSpace가 시작되는 Offset */
    static UInt getBeginFreeOffset( sdtSortPageHdr *aPageHdr )
    {
        return idlOS::align8( SDT_SORT_PAGE_HEADER_SIZE +
                              ( aPageHdr->mSlotCount * ID_SIZEOF( scOffset ) ));
    }

    /* FreeSpace가 끝나는 Offset */
    static UInt getFreeOffset( sdtSortPageHdr *aPageHdr )
    {
        return aPageHdr->mTypeAndFreeOffset & SDT_TEMP_FREEOFFSET_BITMASK;
    }
    static void setFreeOffset( sdtSortPageHdr *aPageHdr,
                               scOffset        aFreeOffset )
    {
        aPageHdr->mTypeAndFreeOffset = ( aPageHdr->mTypeAndFreeOffset & SDT_TEMP_TYPE_BITMASK ) | aFreeOffset;
    }


public:
    static void dumpTempPage( void  * aPagePtr,
                              SChar * aOutBuf,
                              UInt    aOutSize );
    static void dumpWAPageHeaders( void           * aWASegment,
                                   SChar          * aOutBuf,
                                   UInt             aOutSize );
    static SChar mPageName[ SDT_TEMP_PAGETYPE_MAX ][ SMI_TT_STR_SIZE ];
};


/**************************************************************************
 * Description :
 * 슬롯의 Pointer를 가져온다.
 *
 * <IN>
 * aPagePtr       - 초기화할 대상 Page
 * aSlotNo        - 할당받았던 Slot 번호
 * <OUT>
 * aSlotPtr       - Slot의 위치
 ***************************************************************************/
IDE_RC sdtTempPage::getSlotPtr( UChar      * aPagePtr,
                                scSlotNum    aSlotNo,
                                UChar     ** aSlotPtr )
{
    IDE_ERROR( ((sdtSortPageHdr*)aPagePtr)->mSlotCount > aSlotNo );

    *aSlotPtr = aPagePtr + getSlotOffset( aPagePtr, aSlotNo );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**************************************************************************
 * Description :
 * Slot의 개수를 가져온다. Header에서 가져오면 되지만, 마지막 Slot이
 * UnusedSlot이라면, 사용하지 않은 것으로 치기에 추가 처리를 해야 한다.
 *
 * <IN>
 * aPagePtr       - 초기화할 대상 Page
 * <OUT>
 * aSlotCount     - Slot 개수
 ***************************************************************************/
UInt sdtTempPage::getSlotCount( UChar * aPagePtr )
{
    sdtSortPageHdr * sHdr = (sdtSortPageHdr*)aPagePtr;

    if ( sHdr->mSlotCount > 1 )
    {
        /* Slot이 하나라도 있다면, 그 Slot이 Unused인지 체크한다. */
        if ( getSlotOffset( aPagePtr, sHdr->mSlotCount - 1 ) == SDT_TEMP_SLOT_UNUSED )
        {
            return sHdr->mSlotCount - 1;
        }
    }
    return sHdr->mSlotCount;
}

/**************************************************************************
 * Description :
 * 슬롯을 새로 할당받는다. 이 슬롯은 아무것도 가리치지 않은체,
 * UNUSED(ID_USHORT_MAX)으로 초기화 되어있다.
 *
 * <IN>
 * aPagePtr       - 초기화할 대상 Page
 * <OUT>
 * aSlotNo        - 할당한 Slot 번호
 ***************************************************************************/
scSlotNum sdtTempPage::allocSlotDir( UChar * aPagePtr )
{
    sdtSortPageHdr *sHdr = (sdtSortPageHdr*)aPagePtr;

    if ( getSlotDirOffset( sHdr->mSlotCount + 1 ) < getFreeOffset( sHdr ) )
    {
        /* 할당 가능 */
        sHdr->mSlotDir[sHdr->mSlotCount] = SDT_TEMP_SLOT_UNUSED;

        return sHdr->mSlotCount++;
    }

    return SDT_TEMP_SLOT_UNUSED;
}

/**************************************************************************
 * Description :
 * 슬롯이 영역을 할당받는다. 요청한 크기만큼 할당되어 반환된다.
 *
 * <IN>
 * aPagePtr       - 초기화할 대상 Page
 * aSlotNo        - 할당받았던 Slot 번호
 * aNeedSize      - 요구하는 Byte공간
 * aMinSize       - aNeedSize가 안되면 최소 aMinSize만큼은 할당받야아 한다
 * <OUT>
 * aRetSize       - 실제로 할당받은 크기
 * aRetPtr        - 할당받은 Slot의 Begin지점
 ***************************************************************************/
UChar* sdtTempPage::allocSlot( sdtSortPageHdr  * aPageHdr,
                               scSlotNum   aSlotNo,
                               UInt        aNeedSize,
                               UInt        aMinSize,
                               UInt      * aRetSize )
{
    UInt sEndFreeOffset   = getFreeOffset( aPageHdr );
    UInt sBeginFreeOffset = getBeginFreeOffset( aPageHdr );
    UInt sFreeSize = ( sEndFreeOffset - sBeginFreeOffset );
    scOffset sRetOffset;

    if ( sFreeSize >= aMinSize )
    {
        if ( sFreeSize < aNeedSize )
        {
            /* 최소보단 많지만, 넉넉하진 않음.
             * Align된 BegeinFreeOffset부터 전부를 반환해줌 */
            sRetOffset = sBeginFreeOffset;
        }
        else
        {
            /* 할당 가능함.
             * 필요한 공간만큼 뺀 BeginOffset에 8BteAling으로 당겨줌 */
            sRetOffset = sEndFreeOffset - aNeedSize;
            sRetOffset = sRetOffset - ( sRetOffset & 7 );
        }

        /* Align을 위한 Padding 때문에 sRetOffset이 조금 넉넉하게 될 수
         * 있음. 따라서 Min으로 줄여줌 */
        *aRetSize = IDL_MIN( sEndFreeOffset - sRetOffset,
                             aNeedSize );
        setFreeOffset( aPageHdr, sRetOffset );
        // Slot을 설정한다
        aPageHdr->mSlotDir[aSlotNo] = sRetOffset;

        return (UChar*)aPageHdr + sRetOffset;
    }

    *aRetSize = 0;

    return NULL;
}



#endif // _SDT_PAGE_H_
