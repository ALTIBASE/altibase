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

#ifndef _O_SDP_SLOT_DIRECTORY_H_
#define _O_SDP_SLOT_DIRECTORY_H_ 1

#include <sdpDef.h>
#include <smiDef.h>
#include <smErrorCode.h>

#define SDP_SLOT_ENTRY_FLAG_UNUSED  (0x8000)
#define SDP_SLOT_ENTRY_OFFSET_MASK  (0x7FFF)

#define SDP_GET_SLOT_ENTRY(aSlotDirPtr, aSlotNum)                            \
    ( (sdpSlotEntry*)                                                        \
      ( (aSlotDirPtr) +                                                      \
        ID_SIZEOF(sdpSlotDirHdr) +                                           \
        ( ID_SIZEOF(sdpSlotEntry) * (aSlotNum) ) ) )

#define SDP_GET_OFFSET(aSlotEntry)                                           \
    ( (scOffset)(*(aSlotEntry) & SDP_SLOT_ENTRY_OFFSET_MASK ) )

#define SDP_SET_OFFSET(aSlotEntry, aOffset)                                  \
    *(aSlotEntry) &= ~SDP_SLOT_ENTRY_OFFSET_MASK;                             \
    *(aSlotEntry) |= ( (aOffset) & SDP_SLOT_ENTRY_OFFSET_MASK )

#define SDP_IS_UNUSED_SLOT_ENTRY(aSlotEntry)                                 \
    ( ( ( *(aSlotEntry) & SDP_SLOT_ENTRY_FLAG_UNUSED )                       \
        == SDP_SLOT_ENTRY_FLAG_UNUSED ) ? ID_TRUE : ID_FALSE )

#define SDP_SET_UNUSED_FLAG(aSlotEntry)                                      \
    ( *(aSlotEntry) |= SDP_SLOT_ENTRY_FLAG_UNUSED )

#define SDP_CLR_UNUSED_FLAG(aSlotEntry)                                      \
    ( *(aSlotEntry) &= ~SDP_SLOT_ENTRY_FLAG_UNUSED )


/* slot directory를 관리한다. */
class sdpSlotDirectory
{
public:

    static IDE_RC logAndInit( sdpPhyPageHdr  *aPageHdr,
                              sdrMtx         *aMtx );

    static void init(sdpPhyPageHdr    *aPhyPageHdr);

    static IDE_RC alloc(sdpPhyPageHdr    *aPhyPageHdr,
                        scOffset          aSlotOffset,
                        scSlotNum        *aAllocSlotNum);

    static void allocWithShift(sdpPhyPageHdr    *aPhyPageHdr,
                               scSlotNum         aSlotNum,
                               scOffset          aSlotOffset);

    static IDE_RC free(sdpPhyPageHdr    *aPhyPageHdr,
                       scSlotNum         aSlotNum);

    static IDE_RC freeWithShift(sdpPhyPageHdr    *aPhyPageHdr,
                                scSlotNum         aSlotNum);

    static IDE_RC find(UChar            *aSlotDirPtr,
                       scOffset          aSlotOffset,
                       SShort          *aSlotNum);

    static idBool isExistUnusedSlotEntry(UChar       *aSlotDirPtr);

    static UShort getSize(sdpPhyPageHdr    *aPhyPageHdr);

    static inline UShort getCount( UChar * aSlotDirPtr );


    /* BUG-31534 [sm-util] dump utility and fixed table do not consider 
     *           unused slot. */
    static IDE_RC getNextUnusedSlot( UChar       *aSlotDirPtr,
                                     scSlotNum    aSlotNum,
                                     scOffset    *aSlotOffset );

    static inline IDE_RC getValue(UChar       *aSlotDirPtr,
                                  scSlotNum    aSlotNum,
                                  scOffset    *aSlotOffset);

    static IDE_RC setValue(UChar       *aSlotDirPtr,
                           scSlotNum    aSlotNum,
                           scOffset     aSlotOffset);

    static inline idBool isUnusedSlotEntry( UChar      * aSlotDirPtr,
                                            scSlotNum    aSlotNum );

    static inline IDE_RC getPagePtrFromSlotNum(UChar       *aSlotDirPtr,
                                               scSlotNum    aSlotNum,
                                               UChar      **aSlotPtr);

    static inline UShort getDistance( UChar      *aSlotDirPtr,
                                      scSlotNum   aEntry1,
                                      scSlotNum   aEntry2 );

    static idBool validate(UChar    *aSlotDirPtr);

    /* TASK-4007 [SM] PBT를 위한 기능 추가
     * SlotEntry를 Dump할 수 있는 기능 추가*/
    static IDE_RC dump( UChar *sPage ,
                        SChar *aOutBuf,
                        UInt   aOutSize );

    /* BUG-33543 [sm-disk-page] add debugging information to
     * sdpSlotDirectory::getValue function 
     * Unused Slot이면, 정보 출력하고 에러를 반환한다. */
    static inline IDE_RC validateSlot(UChar        * aPagePtr,
                                      scSlotNum      aSlotNum,
                                      sdpSlotEntry * aSlotEntry,
                                      idBool         aIsUnusedSlot);

    /* TASK-6105 함수 inline화를 위해 sdpPhyPage::getPageStartPtr 함수를
     * sdpSlotDirectory::getPageStartPtr로 중복으로 추가한다. 
     * 한 함수가 수정되면 나머지 함수도 수정해야 한다.*/
    inline static UChar * getPageStartPtr( void * aPagePtr )
    {
        return (UChar *)idlOS::alignDown( (void*)aPagePtr, (UInt)SD_PAGE_SIZE );
    }

    /* TASK-6105 함수 inline화를 위해 sdpPhyPage::getPagePtrFromOffset 함수를
     * sdpSlotDirectory::getPagePtrFromOffset로 중복으로 추가한다. 
     * 한 함수가 수정되면 나머지 함수도 수정해야 한다.*/
    inline static UChar * getPagePtrFromOffset( UChar   * aPagePtr,
                                                scOffset  aOffset)
    {
        return ( sdpSlotDirectory::getPageStartPtr(aPagePtr) + aOffset );
    }

private:

    static void shiftToBottom(sdpPhyPageHdr    *aPhyPageHdr,
                              UChar            *aSlotDirPtr,
                              UShort            aSlotNum);

    static void shiftToTop(sdpPhyPageHdr    *aPhyPageHdr,
                           UChar            *aSlotDirPtr,
                           UShort            aSlotNum);

    static IDE_RC addToUnusedSlotEntryList(UChar            *aSlotDirPtr,
                                           scSlotNum         aSlotNum);

    static IDE_RC removeFromUnusedSlotEntryList(UChar            *aSlotDirPtr,
                                                scSlotNum        *aSlotNum);

    static sdpPhyPageHdr * getPageHdr( UChar * aPagePtr );

    static void tracePage( UChar * aSlotDirPtr );
};

inline UShort sdpSlotDirectory::getDistance( UChar      *aSlotDirPtr,
                                             scSlotNum   aEntry1,
                                             scSlotNum   aEntry2 )
{
    sdpSlotEntry * sEntry1;
    sdpSlotEntry * sEntry2;
    UShort         sDiff;

    sEntry1 = SDP_GET_SLOT_ENTRY( aSlotDirPtr, aEntry1 );
    sEntry2 = SDP_GET_SLOT_ENTRY( aSlotDirPtr, aEntry2 );

    if( SDP_GET_OFFSET( sEntry1 ) > SDP_GET_OFFSET( sEntry2) )
    {
        sDiff = SDP_GET_OFFSET( sEntry1 ) - SDP_GET_OFFSET( sEntry2 );
    }
    else
    {
        sDiff = SDP_GET_OFFSET( sEntry2 ) - SDP_GET_OFFSET( sEntry1 );
    }

    return sDiff;
}

/***********************************************************************
 * Description :
 * TASK-6105 함수 inline화
 *     BUG-33543 [sm-disk-page] add debugging information to
 *     sdpSlotDirectory::getValue function 
 *     Slot이 Used인지 Unused인지 체크하여, 기대와 다르면
 *     정보 출력하고 비정상종료시킴.
 *
 *  aPtr          - [IN] 대상 Page내를 가리키는 Pointer
 *  aSlotNum      - [IN] 대상 SlotNumber
 *  aSlotEntry    - [IN] Used여야할 Slot
 *  aIsUnusedSlot - [IN] 이것은 UnusedSlot인가? (그러면 안된다.)
 **********************************************************************/
inline IDE_RC sdpSlotDirectory::validateSlot(UChar        * aPagePtr,
                                             scSlotNum      aSlotNum,
                                             sdpSlotEntry * aSlotEntry,
                                             idBool         aIsUnusedSlot)
{
    sdpPhyPageHdr    * sPhyPageHdr;
                             
    IDU_FIT_POINT_RAISE ( "BUG-40472::validateSlot::InvalidSlot", ERR_ABORT_Invalid_slot );
    if ( SDP_IS_UNUSED_SLOT_ENTRY(aSlotEntry) == aIsUnusedSlot )
    {
        IDE_RAISE( ERR_ABORT_Invalid_slot );
    }
    else
    {
        /* nothing to do */
    }
    return IDE_SUCCESS;
     
    IDE_EXCEPTION( ERR_ABORT_Invalid_slot );
    {
        sPhyPageHdr = getPageHdr( aPagePtr );

        ideLog::log( IDE_ERR_0, 
                     "[ Invalid slot access ]\n"
                     "  SpaceID   : %"ID_UINT32_FMT"\n" 
                     "  PageID    : %"ID_UINT32_FMT"\n" 
                     "  Page Type : %"ID_UINT32_FMT"\n"
                     "  Page State: %"ID_UINT32_FMT"\n"
                     "  TableOID  : %"ID_vULONG_FMT"\n"
                     "  IndexID   : %"ID_vULONG_FMT"\n"
                     "  SlotNum   : %"ID_UINT32_FMT"\n",
                     sPhyPageHdr->mFrameHdr.mSpaceID,
                     sPhyPageHdr->mPageID,
                     sPhyPageHdr->mPageType,
                     sPhyPageHdr->mPageState,
                     sPhyPageHdr->mTableOID,
                     sPhyPageHdr->mIndexID,
                     aSlotNum );

        ideLog::logMem( IDE_ERR_0, 
                        (UChar*)sPhyPageHdr,
                        SD_PAGE_SIZE );
        ideLog::logCallStack( IDE_ERR_0 );

        IDE_SET( ideSetErrorCode( smERR_ABORT_Invalid_slot ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 *
 * Description :
 * TASK-6105 함수 inline화
 *  slot entry에 저장된 slot offset값을 반환한다.
 *
 *  aSlotDirPtr - [IN] slot directory 시작 포인터
 *  aSlotNum    - [IN] slot entry number
 *  aSlotOffset - [OUT] slot entry에 저장된 slot offset값
 **********************************************************************/
inline IDE_RC sdpSlotDirectory::getValue(UChar       *aSlotDirPtr,
                                         scSlotNum    aSlotNum,
                                         scOffset    *aSlotOffset )
{
    sdpSlotDirHdr   *sSlotDirHdr = (sdpSlotDirHdr*)aSlotDirPtr;
    sdpSlotEntry    *sSlotEntry;

    IDE_DASSERT( aSlotDirPtr != NULL );
    IDE_ASSERT( aSlotNum < sSlotDirHdr->mSlotEntryCount );

    sSlotEntry = SDP_GET_SLOT_ENTRY(aSlotDirPtr, aSlotNum);
    /* 값을 읽어 오려는 slot entry는 반드시 used 상태여야 한다. 
       아니면 로직상의 오류. */ 
    IDE_TEST( validateSlot( aSlotDirPtr, 
                            aSlotNum,
                            sSlotEntry, 
                            ID_TRUE )  /* isUnused, used 이어야 함 */
              != IDE_SUCCESS );

    *aSlotOffset = SDP_GET_OFFSET(sSlotEntry);        
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;


}

/***********************************************************************
 *
 * Description :
 * TASK-6105 함수 inline화
 *
 *  aSlotDirPtr - [IN] slot directory 시작 포인터
 *  aSlotNum    - [IN] slot entry number
 *  
 **********************************************************************/
inline IDE_RC sdpSlotDirectory::getPagePtrFromSlotNum( UChar       *aSlotDirPtr,
                                                       scSlotNum    aSlotNum,
                                                       UChar      **aSlotPtr )
{
    sdpSlotDirHdr   * sSlotDirHdr = (sdpSlotDirHdr*)aSlotDirPtr;
    UChar           * sPageHdrPtr = sdpSlotDirectory::getPageStartPtr( aSlotDirPtr );
    scOffset          sOffset     =  SC_MAX_OFFSET;

    /* BUG-34499 Undo Page 재활용으로 인하여 Undo Page에서
     * 잘못된 Offset으로 읽어오는 문제에 대한 검증코드 */
    if ( aSlotNum >= sSlotDirHdr->mSlotEntryCount )
    {
        ideLog::log( IDE_SERVER_0,
                     "[ Invalid slot number ]\n"
                     "  Slot Num (%"ID_UINT32_FMT") >="
                     "  Entry Count (%"ID_UINT32_FMT")\n",
                     aSlotNum,
                     sSlotDirHdr->mSlotEntryCount );

        sdpSlotDirectory::tracePage( aSlotDirPtr );

        IDE_ERROR( 0 ); 
    }

   IDE_TEST( sdpSlotDirectory::getValue(aSlotDirPtr, aSlotNum, &sOffset)
             != IDE_SUCCESS );

    /* BUG-34499 Undo Page 재활용으로 인하여 Undo Page에서
     * 잘못된 Offset으로 읽어오는 문제에 대한 검증코드 */
    if (( sOffset <= ( aSlotDirPtr - sPageHdrPtr ) ) ||
       ( sOffset >= SD_PAGE_SIZE ))
    {
        ideLog::log( IDE_SERVER_0,
                     "[ Invalid slot offset ]\n"
                     "  Slot Num    : %"ID_UINT32_FMT"\n"
                     "  Entry Count : %"ID_UINT32_FMT"\n"
                     "  Offset      : %"ID_UINT32_FMT"\n",
                     aSlotNum,
                     sSlotDirHdr->mSlotEntryCount,
                     sOffset );

        sdpSlotDirectory::tracePage( aSlotDirPtr );

        IDE_ERROR( 0 ); 

    }

    *aSlotPtr = sdpSlotDirectory::getPagePtrFromOffset( aSlotDirPtr,
                                                        sOffset );
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 *
 * Description :
 *  slot entry 갯수를 반환한다.
 *
 *  aSlotDirPtr - [IN] slot directory 시작 포인터
 *
 **********************************************************************/
inline UShort sdpSlotDirectory::getCount( UChar * aSlotDirPtr )
{
    sdpSlotDirHdr * sSlotDirHdr = (sdpSlotDirHdr *)aSlotDirPtr;

    IDE_DASSERT( aSlotDirPtr != NULL );

#ifdef DEBUG

    UShort    sSlotDirSize;
    UShort    sSlotEntryCount;

    sSlotDirSize = getSize( (sdpPhyPageHdr *)getPageStartPtr( aSlotDirPtr ) );

    sSlotEntryCount = (sSlotDirSize - ID_SIZEOF(sdpSlotDirHdr)) / ID_SIZEOF(sdpSlotEntry);

    IDE_DASSERT( sSlotDirHdr->mSlotEntryCount == sSlotEntryCount );
#endif

    return sSlotDirHdr->mSlotEntryCount;
}

/***********************************************************************
 *
 * Description :
 *  unused slot entry인지 여부를 반환한다.
 *
 *  aSlotDirPtr - [IN] slot directory 시작 포인터
 *  aSlotNum    - [IN] slot entry number
 *
 **********************************************************************/
inline idBool sdpSlotDirectory::isUnusedSlotEntry( UChar      * aSlotDirPtr,
                                                   scSlotNum    aSlotNum )
{
    sdpSlotEntry * sSlotEntry;

    sSlotEntry = SDP_GET_SLOT_ENTRY( aSlotDirPtr, aSlotNum );

    if ( SDP_IS_UNUSED_SLOT_ENTRY( sSlotEntry ) == ID_TRUE )
    {
        return ID_TRUE;
    }
    else
    {
        return ID_FALSE;
    }
}

#endif /* _O_SDP_SLOT_DIRECTORY_H_ */
