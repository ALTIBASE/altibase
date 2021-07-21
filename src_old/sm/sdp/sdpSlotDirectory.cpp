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
 

#include <sdpSlotDirectory.h>
#include <sdpPhyPage.h>


/***********************************************************************
 *
 * Description :
 *  slot directory 영역을 초기화한다.(with logging)
 *
 *  aPhyPageHdr - [IN] physical page header
 *  aMtx        - [IN] mini 트랜잭션
 *
 **********************************************************************/
IDE_RC sdpSlotDirectory::logAndInit( sdpPhyPageHdr  *aPageHdr,
                                     sdrMtx         *aMtx )
{
    IDE_ASSERT( aPageHdr != NULL );
    IDE_ASSERT( aMtx     != NULL );

    init(aPageHdr);

    IDE_TEST( sdrMiniTrans::writeLogRec( aMtx,
                                         (UChar*)aPageHdr,
                                         NULL, /* aValue */
                                         0,    /* aLength */
                                         SDR_SDP_INIT_SLOT_DIRECTORY )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/***********************************************************************
 *
 * Description :
 *  slot directory 영역을 초기화한다.(no logging)
 *
 *  aPhyPageHdr - [IN] physical page header
 *
 **********************************************************************/
void sdpSlotDirectory::init(sdpPhyPageHdr    *aPhyPageHdr)
{
    sdpSlotDirHdr    *sSlotDirHdr;

    IDE_DASSERT( aPhyPageHdr != NULL );
    IDE_DASSERT( aPhyPageHdr->mTotalFreeSize     >= ID_SIZEOF(sdpSlotDirHdr) );
    IDE_DASSERT( aPhyPageHdr->mAvailableFreeSize >= ID_SIZEOF(sdpSlotDirHdr) );

    aPhyPageHdr->mFreeSpaceBeginOffset += ID_SIZEOF(sdpSlotDirHdr);
    aPhyPageHdr->mTotalFreeSize        -= ID_SIZEOF(sdpSlotDirHdr);
    aPhyPageHdr->mAvailableFreeSize    -= ID_SIZEOF(sdpSlotDirHdr);

    sSlotDirHdr = (sdpSlotDirHdr*)
        sdpPhyPage::getSlotDirStartPtr((UChar*)aPhyPageHdr);

    sSlotDirHdr->mSlotEntryCount = 0;
    sSlotDirHdr->mHead           = SDP_INVALID_SLOT_ENTRY_NUM;
}


/***********************************************************************
 *
 * Description :
 *  Slot Directory에서 SlotEntry 하나를 할당하는 함수이다.
 *
 *  aPhyPageHdr   - [IN]  physical page header
 *  aSlotOffset   - [IN]  페이지내에서의 slot offset
 *  aAllocSlotNum - [OUT] 할당한 slot entry number
 *
 **********************************************************************/
IDE_RC sdpSlotDirectory::alloc(sdpPhyPageHdr    *aPhyPageHdr,
                               scOffset          aSlotOffset,
                               scSlotNum        *aAllocSlotNum)
{
    UChar           *sSlotDirPtr;
    sdpSlotDirHdr   *sSlotDirHdr;
    sdpSlotEntry    *sSlotEntry;
    scSlotNum        sAllocSlotNum;

    sSlotDirPtr = sdpPhyPage::getSlotDirStartPtr((UChar*)aPhyPageHdr);
    sSlotDirHdr = (sdpSlotDirHdr*)sSlotDirPtr;

    IDE_DASSERT( aPhyPageHdr != NULL );
    IDE_DASSERT( validate(sSlotDirPtr) == ID_TRUE );

    if( sSlotDirHdr->mHead == SDP_INVALID_SLOT_ENTRY_NUM )
    {
        /* unused slot entry가 없어서,
         * new slot entry를 할당하는 경우 */
        sAllocSlotNum = sSlotDirHdr->mSlotEntryCount;

        sSlotDirHdr->mSlotEntryCount++;
        aPhyPageHdr->mFreeSpaceBeginOffset += ID_SIZEOF(sdpSlotEntry);
        aPhyPageHdr->mTotalFreeSize        -= ID_SIZEOF(sdpSlotEntry);
        aPhyPageHdr->mAvailableFreeSize    -= ID_SIZEOF(sdpSlotEntry);

        sSlotEntry = SDP_GET_SLOT_ENTRY(sSlotDirPtr, sAllocSlotNum);
    }
    else
    {
        /* unused slot entry를 재사용하는 경우 */
        IDE_ERROR( removeFromUnusedSlotEntryList( sSlotDirPtr,
                                                  &sAllocSlotNum )
                   == IDE_SUCCESS );

        sSlotEntry = SDP_GET_SLOT_ENTRY(sSlotDirPtr, sAllocSlotNum);
        IDE_TEST( validateSlot( (UChar*)aPhyPageHdr, 
                                 sAllocSlotNum, 
                                 sSlotEntry,
                                 ID_FALSE ) /* isUnused, Unused여야함 */
                  != IDE_SUCCESS);
    }

    /* unused flag를 해제하여 used 상태로 변경한다. */
    SDP_CLR_UNUSED_FLAG(sSlotEntry);

    /* offset값을 slot entry에 설정한다. */
    SDP_SET_OFFSET(sSlotEntry, aSlotOffset);

    *aAllocSlotNum = sAllocSlotNum;
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;

    return IDE_FAILURE;    
}


/***********************************************************************
 *
 * Description :
 *  Slot Directory에서 SlotEntry 하나를 할당하는 함수이다.
 *  필요할 경우, shift 연산을 수행한다.
 *
 *  aPhyPageHdr - [IN] physical page header
 *  aSlotNum    - [IN] 할당하려고 하는 slot number
 *  aSlotOffset - [IN] 페이지내에서의 slot offset
 *
 **********************************************************************/
void sdpSlotDirectory::allocWithShift(sdpPhyPageHdr    *aPhyPageHdr,
                                      scSlotNum         aSlotNum,
                                      scOffset          aSlotOffset)
{
    UChar           *sSlotDirPtr;
    sdpSlotDirHdr   *sSlotDirHdr;
    sdpSlotEntry    *sSlotEntry;

    sSlotDirPtr = sdpPhyPage::getSlotDirStartPtr((UChar*)aPhyPageHdr);
    sSlotDirHdr = (sdpSlotDirHdr*)sSlotDirPtr;

    IDE_DASSERT( aPhyPageHdr != NULL );
    IDE_DASSERT( validate(sSlotDirPtr) == ID_TRUE );
    IDE_DASSERT( aSlotNum <= sSlotDirHdr->mSlotEntryCount );

    if( aSlotNum != sSlotDirHdr->mSlotEntryCount )
    {
        shiftToBottom(aPhyPageHdr, sSlotDirPtr, aSlotNum);
    }

    sSlotDirHdr->mSlotEntryCount++;
    aPhyPageHdr->mFreeSpaceBeginOffset += ID_SIZEOF(sdpSlotEntry);
    aPhyPageHdr->mTotalFreeSize        -= ID_SIZEOF(sdpSlotEntry);
    aPhyPageHdr->mAvailableFreeSize    -= ID_SIZEOF(sdpSlotEntry);

    sSlotEntry = SDP_GET_SLOT_ENTRY(sSlotDirPtr, aSlotNum);

    /* unused flag를 해제하여 used 상태로 변경한다. */
    SDP_CLR_UNUSED_FLAG(sSlotEntry);

    /* offset값을 slot entry에 설정한다. */
    SDP_SET_OFFSET(sSlotEntry, aSlotOffset);
}


/***********************************************************************
 *
 * Description :
 *  Slot Directory에서 SlotEntry 하나를 해제하는 함수이다.
 *
 *  aPhyPageHdr - [IN] physical page header
 *  aSlotNum    - [IN] 해제하려고 하는 slot number
 *
 **********************************************************************/
IDE_RC sdpSlotDirectory::free(sdpPhyPageHdr    *aPhyPageHdr,
                              scSlotNum         aSlotNum)
{
    UChar           *sSlotDirPtr;
#ifdef DEBUG
    sdpSlotDirHdr   *sSlotDirHdr;
#endif
    sdpSlotEntry    *sSlotEntry;

    sSlotDirPtr = sdpPhyPage::getSlotDirStartPtr((UChar*)aPhyPageHdr);

#ifdef DEBUG
    sSlotDirHdr = (sdpSlotDirHdr*)sSlotDirPtr;
#endif
    IDE_DASSERT( aPhyPageHdr != NULL );
    IDE_DASSERT( validate(sSlotDirPtr) == ID_TRUE );
    IDE_DASSERT( sSlotDirHdr->mSlotEntryCount > 0 );
    IDE_DASSERT( aSlotNum < sSlotDirHdr->mSlotEntryCount );

    sSlotEntry = SDP_GET_SLOT_ENTRY(sSlotDirPtr, aSlotNum);
    /* 해제하려는 slot entry는 반드시 used 상태여야 한다.
     * unused인 slot entry를 해제하려고 하는 것은
     * 로직상의 심각한 오류이다. */
    IDE_TEST( validateSlot( (UChar*)aPhyPageHdr,
                             aSlotNum,
                             sSlotEntry,
                             ID_TRUE ) /* isUnused, Unused면 안됨 */
               != IDE_SUCCESS);

    SDP_SET_OFFSET(sSlotEntry, SDP_INVALID_SLOT_ENTRY_NUM);
    SDP_SET_UNUSED_FLAG(sSlotEntry);

    IDE_TEST( addToUnusedSlotEntryList(sSlotDirPtr,
                                       aSlotNum)
              != IDE_SUCCESS);

    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/***********************************************************************
 *
 * Description :
 *  Slot Directory에서 SlotEntry 하나를 해제하는 함수이다.
 *  필요할 경우, shift 연산을 수행한다.
 *
 *  aPhyPageHdr - [IN] physical page header
 *  aSlotNum    - [IN] 해제하려고 하는 slot number
 *
 **********************************************************************/
IDE_RC sdpSlotDirectory::freeWithShift(sdpPhyPageHdr    *aPhyPageHdr,
                                       scSlotNum         aSlotNum)
{
    UChar           *sSlotDirPtr;
    sdpSlotDirHdr   *sSlotDirHdr;
    sdpSlotEntry    *sSlotEntry;

    sSlotDirPtr = sdpPhyPage::getSlotDirStartPtr((UChar*)aPhyPageHdr);
    sSlotDirHdr = (sdpSlotDirHdr*)sSlotDirPtr;

    IDE_DASSERT( aPhyPageHdr != NULL );
    IDE_DASSERT( validate(sSlotDirPtr) == ID_TRUE );
    IDE_DASSERT( sSlotDirHdr->mSlotEntryCount > 0 );
    IDE_DASSERT( aSlotNum < sSlotDirHdr->mSlotEntryCount );

    sSlotEntry = SDP_GET_SLOT_ENTRY(sSlotDirPtr, aSlotNum);
    /* 해제하려는 slot entry는 반드시 used 상태여야 한다.
     * unused인 slot entry를 해제하려고 하는 것은
     * 로직상의 심각한 오류이다. */
    IDE_TEST( validateSlot( (UChar*)aPhyPageHdr, 
                             aSlotNum,
                             sSlotEntry, 
                             ID_TRUE ) /* isUnused, Unused면 안됨 */
              != IDE_SUCCESS);

    /* 제일 마지막 slot entry를 해제하는 경우,
     * slot entry들을 shift할 필요가 없다. */
    if( aSlotNum != (sSlotDirHdr->mSlotEntryCount - 1) )
    {
        shiftToTop(aPhyPageHdr, sSlotDirPtr, aSlotNum);
    }

    sSlotDirHdr->mSlotEntryCount--;
    aPhyPageHdr->mFreeSpaceBeginOffset -= ID_SIZEOF(sdpSlotEntry);
    aPhyPageHdr->mTotalFreeSize        += ID_SIZEOF(sdpSlotEntry);
    aPhyPageHdr->mAvailableFreeSize    += ID_SIZEOF(sdpSlotEntry);

    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/***********************************************************************
 *
 * Description :
 *  offset 위치의 slot을 가리키는 slot entry를 찾는다.
 *
 *  aSlotDirPtr - [IN] slot directory 시작 포인터
 *  aSlotOffset - [IN] 페이지내에서의 slot offset
 *  aSlotNum    - [OUT]  offset 위치의 slot 번호
 **********************************************************************/
IDE_RC sdpSlotDirectory::find(UChar            *aSlotDirPtr,
                              scOffset          aSlotOffset,
                              SShort           *aSlotNum )
{
    sdpSlotEntry    *sSlotEntry;
    SShort           sSlotNum = -1;

    IDE_DASSERT( aSlotDirPtr != NULL );

    for( sSlotNum = 0; sSlotNum < getCount(aSlotDirPtr); sSlotNum++ )
    {
        sSlotEntry = SDP_GET_SLOT_ENTRY(aSlotDirPtr, sSlotNum);

        if( SDP_GET_OFFSET(sSlotEntry) == aSlotOffset )
        {
            IDE_TEST( validateSlot( aSlotDirPtr, 
                                    sSlotNum,
                                    sSlotEntry, 
                                    ID_TRUE ) /* isUnused, Unused면 안됨 */
                      != IDE_SUCCESS);
            break;
        }
    }
    *aSlotNum = sSlotNum;

    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/***********************************************************************
 *
 * Description :
 *  unused slot entry가 하나라도 존재하는지 여부를 리턴한다.
 *
 *  aSlotDirPtr - [IN] slot directory 시작 포인터
 *
 **********************************************************************/
idBool sdpSlotDirectory::isExistUnusedSlotEntry(UChar       *aSlotDirPtr)
{
    sdpSlotDirHdr *sSlotDirHdr = (sdpSlotDirHdr*)aSlotDirPtr;

    IDE_DASSERT( aSlotDirPtr != NULL );

    if( sSlotDirHdr->mHead == SDP_INVALID_SLOT_ENTRY_NUM )
    {
        return ID_FALSE;
    }
    else
    {
        return ID_TRUE;
    }
}


/***********************************************************************
 *
 * Description :
 *  slot directory의 크기를 반환한다.
 *
 *  aPhyPageHdr - [IN] physical page header
 *
 **********************************************************************/
UShort sdpSlotDirectory::getSize(sdpPhyPageHdr    *aPhyPageHdr)
{
    UShort    sSlotDirSize;

    IDE_DASSERT( aPhyPageHdr != NULL );

    sSlotDirSize =
        sdpPhyPage::getSlotDirEndPtr(aPhyPageHdr) -
        sdpPhyPage::getSlotDirStartPtr((UChar*)aPhyPageHdr);

    return sSlotDirSize;
}

/***********************************************************************
 * BUG-31534 [sm-util] dump utility and fixed table do not consider 
 *           unused slot.
 *
 * Description :
 *           UnusedSlotList에 속한 Slot에서, 다음 UnusedSlot을 가져온다.
 *
 *  aSlotDirPtr - [IN] slot directory 시작 포인터
 *  aSlotNum    - [IN] slot entry number
 *  aSlotOffset - [OUT] slot offset
 **********************************************************************/
IDE_RC sdpSlotDirectory::getNextUnusedSlot(UChar       * aSlotDirPtr,
                                           scSlotNum     aSlotNum,
                                           scOffset    * aSlotOffset )
{
    sdpSlotDirHdr   *sSlotDirHdr = (sdpSlotDirHdr*)aSlotDirPtr;
    sdpSlotEntry    *sSlotEntry;

    IDE_DASSERT( aSlotDirPtr != NULL );
    IDE_ASSERT( aSlotNum < sSlotDirHdr->mSlotEntryCount );

    sSlotEntry = SDP_GET_SLOT_ENTRY(aSlotDirPtr, aSlotNum);
    IDE_TEST( validateSlot( aSlotDirPtr,
                            aSlotNum,
                            sSlotEntry,
                            ID_FALSE ) /* isUnused, Unused여야함 */
              != IDE_SUCCESS);
    *aSlotOffset = SDP_GET_OFFSET(sSlotEntry);

    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/***********************************************************************
 *
 * Description :
 *  slot offset값을 slot entry에 설정한다.
 *
 *  aSlotDirPtr - [IN] slot directory 시작 포인터
 *  aSlotNum    - [IN] slot entry number
 *  aSlotOffset - [IN] slot offset
 *
 **********************************************************************/
IDE_RC sdpSlotDirectory::setValue(UChar       *aSlotDirPtr,
                                  scSlotNum    aSlotNum,
                                  scOffset     aSlotOffset)
{
    sdpSlotDirHdr   *sSlotDirHdr = (sdpSlotDirHdr*)aSlotDirPtr;
    sdpSlotEntry    *sSlotEntry;

    IDE_DASSERT( validate(aSlotDirPtr) == ID_TRUE );
    IDE_ASSERT( aSlotNum < sSlotDirHdr->mSlotEntryCount );

    sSlotEntry = SDP_GET_SLOT_ENTRY(aSlotDirPtr, aSlotNum);
    IDE_TEST( validateSlot( aSlotDirPtr, 
                            aSlotNum,
                            sSlotEntry, 
                            ID_TRUE ) /* isUnused, Unused면 안됨 */
              != IDE_SUCCESS);
    SDP_SET_OFFSET(sSlotEntry, aSlotOffset);
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 *
 * Description :
 *
 *  aPhyPageHdr - [IN] physical page header
 *  aSlotDirPtr - [IN] slot directory 시작 포인터
 *  aSlotNum    - [IN] slot entry number
 *
 **********************************************************************/
void sdpSlotDirectory::shiftToBottom(sdpPhyPageHdr    *aPhyPageHdr,
                                     UChar            *aSlotDirPtr,
                                     UShort            aSlotNum)
{
    sdpSlotEntry   *sSlotEntry;
    UShort          sMoveSize;

    sSlotEntry = SDP_GET_SLOT_ENTRY(aSlotDirPtr, aSlotNum);
    sMoveSize = (UShort)( sdpPhyPage::getSlotDirEndPtr(aPhyPageHdr) -
                          (UChar*)sSlotEntry );

    idlOS::memmove(sSlotEntry+1, sSlotEntry, sMoveSize);
}


/***********************************************************************
 *
 * Description :
 *
 *  aPhyPageHdr - [IN] physical page header
 *  aSlotDirPtr - [IN] slot directory 시작 포인터
 *  aSlotNum    - [IN] slot entry number
 *
 **********************************************************************/
void sdpSlotDirectory::shiftToTop(sdpPhyPageHdr    *aPhyPageHdr,
                                  UChar            *aSlotDirPtr,
                                  UShort            aSlotNum)
{
    sdpSlotEntry   *sNxtSlotEntry;
    UShort          sMoveSize;

    sNxtSlotEntry = SDP_GET_SLOT_ENTRY(aSlotDirPtr, aSlotNum+1);
    sMoveSize = (UShort)( sdpPhyPage::getSlotDirEndPtr(aPhyPageHdr) -
                          (UChar*)sNxtSlotEntry );

    idlOS::memmove(sNxtSlotEntry-1, sNxtSlotEntry, sMoveSize);
}


/***********************************************************************
 *
 * Description :
 *  unused slot entry list에 하나의 slot entry를 추가한다.
 *
 *  aSlotDirPtr - [IN] slot directory start ptr
 *  aSlotNum    - [IN] 추가할 slot entry number
 *
 **********************************************************************/
IDE_RC sdpSlotDirectory::addToUnusedSlotEntryList(UChar            *aSlotDirPtr,
                                                  scSlotNum         aSlotNum)
{
    sdpSlotDirHdr   *sSlotDirHdr = (sdpSlotDirHdr*)aSlotDirPtr;
    sdpSlotEntry    *sSlotEntry;

    sSlotEntry = SDP_GET_SLOT_ENTRY(aSlotDirPtr, aSlotNum);
    IDE_TEST( validateSlot( aSlotDirPtr,
                            aSlotNum,
                            sSlotEntry,
                            ID_FALSE ) /* isUnused, Unused여야함 */
              != IDE_SUCCESS);
    SDP_SET_OFFSET(sSlotEntry, sSlotDirHdr->mHead);

    sSlotDirHdr->mHead = aSlotNum;

    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;

    return IDE_FAILURE;    
}


/***********************************************************************
 *
 * Description :
 *  unused slot entry list에서 하나의 slot entry를 제거한다.
 *
 *  aSlotDirPtr - [IN]  slot directory start ptr
 *  aSlotNum    - [OUT] list에서 제거한 slot entry number
 *
 **********************************************************************/
IDE_RC sdpSlotDirectory::removeFromUnusedSlotEntryList(UChar     *aSlotDirPtr,
                                                       scSlotNum *aSlotNum)
{
    sdpSlotDirHdr   *sSlotDirHdr = (sdpSlotDirHdr*)aSlotDirPtr;
    sdpSlotEntry    *sSlotEntry;
    scSlotNum        sFstUnusedSlotEntryNum;

    IDE_ASSERT( sSlotDirHdr->mHead != SDP_INVALID_SLOT_ENTRY_NUM );

    sFstUnusedSlotEntryNum = sSlotDirHdr->mHead;

    sSlotEntry = SDP_GET_SLOT_ENTRY(aSlotDirPtr, sFstUnusedSlotEntryNum);
    IDE_TEST( validateSlot( aSlotDirPtr, 
                            sFstUnusedSlotEntryNum,
                            sSlotEntry, 
                            ID_FALSE ) /* isUnused, Unused여야함 */
              != IDE_SUCCESS);

    sSlotDirHdr->mHead = SDP_GET_OFFSET(sSlotEntry);

    *aSlotNum = sFstUnusedSlotEntryNum;

    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 *
 * Description :
 *  slot directory의 validity를 검사한다.
 *
 *  aSlotDirPtr - [IN] slot directory start ptr
 *
 **********************************************************************/
#ifdef DEBUG
idBool sdpSlotDirectory::validate(UChar    *aSlotDirPtr)
{
    sdpSlotDirHdr  *sSlotDirHdr = (sdpSlotDirHdr*)aSlotDirPtr;
    sdpSlotEntry   *sSlotEntry;
    UShort          sCount;
    UShort          sLoop;

    IDE_DASSERT( aSlotDirPtr != NULL );
    IDE_DASSERT( aSlotDirPtr ==
                 sdpPhyPage::getSlotDirStartPtr(aSlotDirPtr) );

    sCount      = getCount(aSlotDirPtr);

    for( sLoop = 0; sLoop < sCount; sLoop++ )
    {
        sSlotEntry = SDP_GET_SLOT_ENTRY(aSlotDirPtr, sLoop);

        if( SDP_IS_UNUSED_SLOT_ENTRY(sSlotEntry) == ID_TRUE )
        {
            IDE_DASSERT( sSlotDirHdr->mHead != SDP_INVALID_SLOT_ENTRY_NUM );
        }
        else
        {
            IDE_DASSERT( SDP_GET_OFFSET(sSlotEntry) >=
                         sdpPhyPage::getFreeSpaceEndOffset(sdpPhyPage::getHdr(aSlotDirPtr)) );

            IDE_DASSERT( SDP_GET_OFFSET(sSlotEntry) <=
                         ( SD_PAGE_SIZE - ID_SIZEOF(sdpPageFooter) ) );
        }
    }
 
    return ID_TRUE;
}

#endif

/***********************************************************************
 * TASK-4007 [SM] PBT를 위한 기능 추가
 *
 * Description :
 *  slot directory를 dump하여 보여준다.
 *
 *  aSlotDirPtr - [IN] page start ptr
 *
 * BUG-28379 [SD] sdnbBTree::dumpNodeHdr( UChar *aPage ) 내에서
 * local Array의 ptr를 반환하고 있습니다. 
 * Local Array대신 OutBuf를 받아 리턴하도록 수정합니다. *
 *
 **********************************************************************/
IDE_RC sdpSlotDirectory::dump(UChar    *aPagePtr,
                              SChar    *aOutBuf,
                              UInt      aOutSize )
{
    UChar           *sSlotDirPtr;
    sdpSlotDirHdr   *sSlotDirHdr;
    sdpSlotEntry    *sSlotEntry;
    UShort           sCount;
    UShort           sSize;
    UShort           sLoop;
    UChar           *sPagePtr;

    IDE_ERROR( aPagePtr == NULL );
    IDE_ERROR( aOutBuf  == NULL );
    IDE_ERROR( aOutSize  > 0 );

    sPagePtr = sdpPhyPage::getPageStartPtr(aPagePtr);

    IDE_DASSERT( sPagePtr == aPagePtr );

    sSlotDirPtr = sdpPhyPage::getSlotDirStartPtr((UChar*)sPagePtr);
    sSlotDirHdr = (sdpSlotDirHdr*)sSlotDirPtr;

    // Slot Directory의 크기는 획득
    sSize = getSize( (sdpPhyPageHdr*) sPagePtr );

    idlOS::snprintf( aOutBuf,
                     aOutSize,
                     "----------- Slot Directory Begin ----------\n"
                     "SlotDirectorySize : %"ID_UINT32_FMT"\n",
                     sSize );

    if( sSize != 0 )
    {
        sCount = sSlotDirHdr->mSlotEntryCount;


        /* BUG-31534 [sm-util] dump utility and fixed table do not consider 
         *           unused slot.
         * SlotDirHdr의 mHead값은 UnusedSlot의 HeadSlot을 가리킴 */
        idlVA::appendFormat( aOutBuf,
                             aOutSize,
                             "SlotCount         : %"ID_UINT32_FMT"\n"
                             "SlotHead          : %"ID_UINT32_FMT"\n",
                             sSlotDirHdr->mSlotEntryCount,
                             sSlotDirHdr->mHead );

        // SlotDirectory의 크기는 4b(sdpSlotDirHdr) + SlotCount*2b(sdpSlotEntry)
        if( (sSize - ID_SIZEOF(sdpSlotDirHdr)) / ID_SIZEOF(sdpSlotEntry) !=
            sCount )
        {
            idlVA::appendFormat( aOutBuf,
                                 aOutSize,
                                 "invalid size( slotDirSize != slotCount*%"ID_UINT32_FMT"b+%"ID_UINT32_FMT"b)\n",
                                 ID_SIZEOF(sdpSlotEntry),
                                 ID_SIZEOF(sdpSlotDirHdr) );
        }

        for( sLoop = 0; sLoop < sCount; sLoop++ )
        {
            sSlotEntry = SDP_GET_SLOT_ENTRY(sSlotDirPtr, sLoop);

            idlVA::appendFormat( aOutBuf,
                                 aOutSize,
                                 "[%"ID_UINT32_FMT"] %04x ", sLoop, *sSlotEntry );

            if( SDP_IS_UNUSED_SLOT_ENTRY(sSlotEntry) == ID_TRUE )
            {
                idlVA::appendFormat( aOutBuf,
                                     aOutSize,
                                     "Unused slot\n");
            }
            else
            {
                idlVA::appendFormat( aOutBuf,
                                     aOutSize,
                                     "%"ID_UINT32_FMT"\n", SDP_GET_OFFSET(sSlotEntry));
            }
        }
    }

    idlVA::appendFormat( aOutBuf,
                         aOutSize,
                         "----------- Slot Directory End   ----------\n" );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

/***********************************************************************
 * TASK-6105 함수 inline화를 위해 sdpPhyPage::getHdr 함수를
 * wrapping한 함수를 만든다.
 **********************************************************************/
sdpPhyPageHdr * sdpSlotDirectory::getPageHdr( UChar * aPagePtr )
{
    return sdpPhyPage::getHdr( aPagePtr );
}

/***********************************************************************
 * TASK-6105 함수 inline화를 위해 sdpPhyPage::tracePage 함수를
 * wrapping한 함수를 만든다.
 **********************************************************************/
void sdpSlotDirectory::tracePage( UChar * aSlotDirPtr )
{
    (void)sdpPhyPage::tracePage( IDE_SERVER_0,
                                 aSlotDirPtr,
                                 NULL /*title*/ );
}

