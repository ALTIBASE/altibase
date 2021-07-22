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
 * $Id: sdpPhyPage.cpp 87929 2020-07-03 00:38:42Z et16 $
 *
 * Description :
 *
 * # physical page의 기능 구현
 *
 * page의 physical header를 관리한다.
 * 또한 요청에 따라 keymap을 생성하고 삭제하며
 * free offset과 free size를 관리한다.
 *
 * sdpPhysicalPage에서  free offset으로 제공되는 기능을
 * sdpPageList에서는 last slot의 관리로 사용하고,
 * 이에 추가하여 페이지의 free인 공간을 위한 관리를 제공할 것이다.
 *
 * 참조 : sdcRecord.cpp
 *
 * # API 원칙
 * - 함수 인자로 받는 UChar * page ptr는 페이지의 임의의 시작점이다.
 * - 함수 인자로 받는 UChar * start ptr는 페이지의 시작지점입니다.
 * - page ptr을 인자로 받을 때는 내부에서는 page 시작점을 구해야 한다.
 * - page ptr를 인자로 받는 이유는 layer간 호출때문이다.
 * - 상위 layer혹은 같은 layer에서 호출되는 모든 함수는 sdpPhyPageHdr*를
 *   인자로 받으며, 이를 호출하는 쪽에서 캐스팅하여 넘겨야 한다.
 * - getHdr 함수로  sdpPhyPageHdr를 구할 수 있다.
 * - getPageStart 함수로 page의 시작지점을 구할 수 있다.
 *
 **********************************************************************/

#include <smDef.h>
#include <ide.h>
#include <sdr.h>
#include <smr.h>
#include <smxTrans.h>
#include <smErrorCode.h>
#include <sdpDef.h>
#include <sdpPhyPage.h>
#include <sdpReq.h>
#include <sdpDblPIDList.h>
#include <sdpSglPIDList.h>


/***********************************************************************
 *
 * Description :
 *  physical page를 초기화한다.
 *
 *  aPageHdr    - [IN] physical page header
 *  aPageID     - [IN] page id
 *  aParentInfo - [IN] parent page info
 *  aPageState  - [IN] page state
 *  aPageType   - [IN] page type
 *  aTableOID   - [IN] table oid
 *  aIndexID    - [IN] index id
 **********************************************************************/
IDE_RC sdpPhyPage::initialize( sdpPhyPageHdr  *aPageHdr,
                               scPageID        aPageID,
                               sdpParentInfo  *aParentInfo,
                               UShort          aPageState,
                               sdpPageType     aPageType,
                               smOID           aTableOID,
                               UInt            aIndexID )
{
    sdbFrameHdr       sFrameHdr;
    sdpPageFooter    *sPageFooter;

    IDE_DASSERT( (UChar*)aPageHdr == getPageStartPtr((UChar*)aPageHdr));

    sFrameHdr  = *((sdbFrameHdr*)aPageHdr);

    // XXX: Index쪽에 문제가 있는것 같습니다. 추후에 검증이 필요합니다.
    idlOS::memset( (void *)aPageHdr, SDP_INIT_PAGE_VALUE, SD_PAGE_SIZE );

    sPageFooter = (sdpPageFooter*) getPageFooterStartPtr( (UChar*)aPageHdr );

    SM_LSN_INIT( sPageFooter->mPageLSN );

    aPageHdr->mFrameHdr = sFrameHdr;

    aPageHdr->mPageID   = aPageID;
    aPageHdr->mPageType = aPageType;

    aPageHdr->mLogicalHdrSize = 0;
    aPageHdr->mSizeOfCTL      = 0;

    aPageHdr->mFreeSpaceBeginOffset =
        getLogicalHdrStartPtr((UChar*)aPageHdr) - (UChar *)aPageHdr;

    aPageHdr->mFreeSpaceEndOffset =
        getPageFooterStartPtr((UChar*)aPageHdr) - (UChar *)aPageHdr;

    aPageHdr->mTotalFreeSize = aPageHdr->mAvailableFreeSize =
        aPageHdr->mFreeSpaceEndOffset - aPageHdr->mFreeSpaceBeginOffset;
    aPageHdr->mPageState     = aPageState;
    aPageHdr->mIsConsistent  = SDP_PAGE_CONSISTENT;

    // 이후 부분은 굳이 초기화할 필요 없으나
    // SDP_INIT_PAGE_VALUE 값이 0이 아닐 경우에
    // 반드시 필요하다. 현재는 0이므로 별 필요없는 부분임.
    aPageHdr->mFrameHdr.mCheckSum   = 0;
    SM_LSN_INIT( aPageHdr->mFrameHdr.mPageLSN );
    aPageHdr->mFrameHdr.mIndexSMONo = 0;

    aPageHdr->mLinkState      = SDP_PAGE_LIST_UNLINK;
    aPageHdr->mListNode.mNext = SD_NULL_PID;
    aPageHdr->mListNode.mPrev = SD_NULL_PID;

    if ( aParentInfo != NULL )
    {
        aPageHdr->mParentInfo = *aParentInfo;
    }
    else
    {
        aPageHdr->mParentInfo.mParentPID   = SD_NULL_PID;
        aPageHdr->mParentInfo.mIdxInParent = ID_SSHORT_MAX;
    }

    aPageHdr->mTableOID = aTableOID;

    aPageHdr->mIndexID  = aIndexID;

    aPageHdr->mSeqNo = 0;

    return IDE_SUCCESS;
}

/* ------------------------------------------------
 * Description :
 *
 * page 할당시 physical header 초기화 및 로깅
 *
 * - 페이지를 SM_PAGE_SIZE만큼 0x00로 초기화한다.
 * - 페이지 free offset을 physical header와 logical header
 *   후의 위치에 set
 *
 * - 남은 공간을 계산하여 free size로 set한다.
 *                      -> sdpPhysicalPage::calcFreeSize
 *                      -> setFreeSize
 *
 * - 페이지 state를 INSERTABLE로 한다.
 * - 기타 멤버 셋
 * - SDR_INIT_FILE_PAGE 타입 로깅
 *
 * - extent RID, page ID
 *   이 값은 반드시 write 되어야 하고, logging 되어야 한다.
 *   page id와 extent rid는 page ptr로부터 space , page id를 구할 때 필요하다.
 *   extent RID는 mtx 로깅시 데이타로 write된다.
 *
 * 이 함수는 sdpSegment::allocPage에서 호출되며
 * page를 create하고자 하는 함수는 이 함수를 호출하게 된다.
 *
 * ----------------------------------------------*/
IDE_RC sdpPhyPage::logAndInit( sdpPhyPageHdr *aPageHdr,
                               scPageID       aPageID,
                               sdpParentInfo *aParentInfo,
                               UShort         aPageState,
                               sdpPageType    aPageType,
                               smOID          aTableOID,
                               UInt           aIndexID,
                               sdrMtx        *aMtx )
{
    sdrInitPageInfo  sInitPageInfo;

    IDE_DASSERT( (UChar*)aPageHdr == getPageStartPtr((UChar*)aPageHdr));
    IDE_DASSERT( aMtx != NULL );
    IDE_DASSERT( ID_SIZEOF( aPageID ) == ID_SIZEOF( UInt ) );

    IDE_TEST( initialize( aPageHdr,
                          aPageID,
                          aParentInfo,
                          aPageState,
                          aPageType,
                          aTableOID,
                          aIndexID ) != IDE_SUCCESS );

    makeInitPageInfo( &sInitPageInfo,
                      aPageID,
                      aParentInfo,
                      aPageState,
                      aPageType,
                      aTableOID,
                      aIndexID );

    IDE_TEST( sdrMiniTrans::writeLogRec( aMtx,
                                         (UChar*)aPageHdr,
                                         &sInitPageInfo,
                                         ID_SIZEOF( sInitPageInfo ),
                                         SDR_SDP_INIT_PHYSICAL_PAGE )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* 한번에 Format된 페이지를 필요한 페이지 타입으로 초기화한다. */
IDE_RC sdpPhyPage::setLinkState( sdpPhyPageHdr    * aPageHdr,
                                 sdpPageListState   aLinkState,
                                 sdrMtx           * aMtx )
{
    UShort sLinkState = (UShort)aLinkState;

    IDE_ASSERT( aPageHdr != NULL );
    IDE_ASSERT( aMtx != NULL );

    return sdrMiniTrans::writeNBytes( aMtx,
                                      (UChar*)&(aPageHdr->mLinkState),
                                      &sLinkState,
                                      ID_SIZEOF(UShort) );
}


IDE_RC sdpPhyPage::create( idvSQL        *aStatistics,
                           scSpaceID      aSpaceID,
                           scPageID       aPageID,
                           sdpParentInfo *aParentInfo,
                           UShort         aPageState,
                           sdpPageType    aType,
                           smOID          aTableOID,
                           UInt           aIndexID,
                           sdrMtx        *aCrtMtx,
                           sdrMtx        *aInitMtx,
                           UChar        **aPagePtr )
{
    UChar         *sPagePtr;
    sdpPhyPageHdr *sPageHdr;

    IDE_ASSERT( aPagePtr != NULL );

    /* Tablespace의 0번 meta page 생성 */
    IDE_TEST( sdbBufferMgr::createPage( aStatistics,
                                        aSpaceID,
                                        aPageID,
                                        aType,
                                        aCrtMtx,
                                        &sPagePtr) != IDE_SUCCESS );
    IDE_ASSERT( sPagePtr != NULL );

    sPageHdr = getHdr( sPagePtr );

    /* 생성된 page의 physical header 초기화 */
    IDE_TEST( sdpPhyPage::logAndInit( sPageHdr,
                                      aPageID,
                                      aParentInfo,
                                      aPageState,
                                      aType, 
                                      aTableOID,
                                      aIndexID,
                                      aInitMtx )
              != IDE_SUCCESS );

    if ( aInitMtx != aCrtMtx )
    {
        IDE_TEST( sdrMiniTrans::setDirtyPage( aInitMtx,
                                              sPagePtr )
                  != IDE_SUCCESS );
    }

    *aPagePtr = sPagePtr;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    *aPagePtr = NULL;

    return IDE_FAILURE;
}


IDE_RC sdpPhyPage::create4DPath( idvSQL        *aStatistics,
                                 void          *aTrans,
                                 scSpaceID      aSpaceID,
                                 scPageID       aPageID,
                                 sdpParentInfo *aParentInfo,
                                 UShort         aPageState,
                                 sdpPageType    aType,
                                 smOID          aTableOID,
                                 idBool         aIsLogging,
                                 UChar        **aPagePtr )
{
    UChar         *sPagePtr;
    sdpPhyPageHdr *sPageHdr;

    IDE_ASSERT( aPagePtr != NULL );

    /* Tablespace의 0번 meta page 생성 */
    IDE_TEST( sdbDPathBufferMgr::createPage( aStatistics,
                                             aTrans,
                                             aSpaceID,
                                             aPageID,
                                             aIsLogging,
                                             &sPagePtr) != IDE_SUCCESS );
    IDE_ASSERT( sPagePtr !=NULL );

    sPageHdr = getHdr( sPagePtr );

    /* 생성된 page의 physical header 초기화 */
    IDE_TEST( sdpPhyPage::initialize( sPageHdr,
                                      aPageID,
                                      aParentInfo,
                                      aPageState,
                                      aType,
                                      aTableOID,
                                      SM_NULL_INDEX_ID )
              != IDE_SUCCESS );

    *aPagePtr = sPagePtr;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    *aPagePtr = NULL;

    return IDE_FAILURE;
}


/***********************************************************************
 *
 * Description :
 *
 *  페이지를 초기화 한다.
 *  initialize와 거의 동일하나 physical, logical header
 *  의 데이타를 그대로 두어야 한다.
 *
 *  - logical header 이후의 부분을 전부 초기화 한다.
 *  - keyslot을 전부 삭제한다.
 *  - free offset, free size를 initialize 상태로 set
 *
 *  이 함수는 index SMO과정에서 발생한다.
 *
 *  aPageHdr        - [IN] physical page header
 *  aLogicalHdrSize - [IN] logical header size
 *  aMtx            - [IN] mini 트랜잭션
 *
 **********************************************************************/
IDE_RC sdpPhyPage::reset(
    sdpPhyPageHdr * aPageHdr,
    UInt            aLogicalHdrSize,
    sdrMtx        * aMtx )
{
    IDE_DASSERT( validate(aPageHdr) == ID_TRUE );

    aPageHdr->mLogicalHdrSize = aLogicalHdrSize;
    aPageHdr->mSizeOfCTL      = 0;

    aPageHdr->mFreeSpaceBeginOffset =
        getSlotDirStartPtr( (UChar*)aPageHdr ) - (UChar *)aPageHdr;

    aPageHdr->mFreeSpaceEndOffset =
        getPageFooterStartPtr( (UChar*)aPageHdr )   - (UChar *)aPageHdr;

    aPageHdr->mTotalFreeSize = aPageHdr->mAvailableFreeSize =
        aPageHdr->mFreeSpaceEndOffset - aPageHdr->mFreeSpaceBeginOffset;

    // BUG-17615
    if ( aMtx != NULL )
    {
        IDE_TEST( sdrMiniTrans::writeLogRec( aMtx,
                                             (UChar*)aPageHdr,
                                             &aLogicalHdrSize,
                                             SDR_SDP_4BYTE,
                                             SDR_SDP_RESET_PAGE )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/***********************************************************************
 *
 * Description :
 *  logical header를 초기화한다.(with logging)
 *
 *  aPageHdr            - [IN]  physical page header
 *  aSize               - [IN]  logical header size
 *  aMtx                - [IN]  mini 트랜잭션
 *  aLogicalHdrStartPtr - [OUT] logical header 시작위치
 *
 **********************************************************************/
IDE_RC  sdpPhyPage::logAndInitLogicalHdr(
                                        sdpPhyPageHdr  *aPageHdr,
                                        UInt            aSize,
                                        sdrMtx         *aMtx,
                                        UChar         **aLogicalHdrStartPtr )
{
    IDE_DASSERT( validate(aPageHdr) == ID_TRUE );
    IDE_DASSERT( aSize > 0 );
    IDE_DASSERT( aMtx != NULL );
    IDE_DASSERT( aLogicalHdrStartPtr != NULL );

    initLogicalHdr(aPageHdr, aSize);

    IDE_TEST( sdrMiniTrans::writeLogRec( aMtx,
                                         (UChar*)aPageHdr,
                                         &aSize,
                                         SDR_SDP_4BYTE,
                                         SDR_SDP_INIT_LOGICAL_HDR )
              != IDE_SUCCESS );

    *aLogicalHdrStartPtr = (UChar*)aPageHdr + getPhyHdrSize();

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 *
 * Description :
 *  logical header를 초기화한다.(no logging)
 *
 *  aPageHdr - [IN]  physical page header
 *  aSize    - [IN]  logical header size
 *
 **********************************************************************/
UChar* sdpPhyPage::initLogicalHdr( sdpPhyPageHdr  *aPageHdr,
                                   UInt            aSize )
{
    UInt sAlignedSize;

    IDE_ASSERT( aSize > 0 );
    IDE_DASSERT( validate(aPageHdr) == ID_TRUE );

    sAlignedSize= idlOS::align8(aSize);

    aPageHdr->mAvailableFreeSize -= sAlignedSize;
    aPageHdr->mTotalFreeSize     -= sAlignedSize;

    aPageHdr->mFreeSpaceBeginOffset += sAlignedSize;
    aPageHdr->mLogicalHdrSize       = sAlignedSize;

    IDE_DASSERT( validate(aPageHdr) == ID_TRUE );

    return (UChar*)aPageHdr + getPhyHdrSize();
}

/***********************************************************************
 *
 * Description : Change Transaction Layer 초기화
 *
 * 페이지의 Chained Transaction Layer를 초기화하고,
 * sdpPhyPageHdr의 FreeSize 및 각종 Offset을 설정한다.
 *
 ***********************************************************************/
void sdpPhyPage::initCTL( sdpPhyPageHdr   * aPageHdr,
                          UInt              aHdrSize,
                          UChar          ** aHdrStartPtr )
{
    UInt    sAlignedSize;

    IDE_ASSERT( aHdrSize     > 0 );
    IDE_ASSERT( aPageHdr     != NULL );
    IDE_ASSERT( aHdrStartPtr != NULL );

    sAlignedSize = idlOS::align8( aHdrSize );

    aPageHdr->mSizeOfCTL             = sAlignedSize;
    aPageHdr->mFreeSpaceBeginOffset += sAlignedSize;
    aPageHdr->mTotalFreeSize         =
        aPageHdr->mAvailableFreeSize =
        aPageHdr->mFreeSpaceEndOffset - aPageHdr->mFreeSpaceBeginOffset;

    *aHdrStartPtr = getCTLStartPtr( (UChar*)aPageHdr );

    return;
}


/***********************************************************************
 *
 * Description : Base Touched Transaction Layer 확장
 *
 * CTL은 자동확장시 Available Free Size만을 고려한다.
 * 자동확장으로 인해서 SlotDirEntry가 아래로 이동하며,
 * 패이지 Compaction이 발생할수도 있다.
 *
 ***********************************************************************/
IDE_RC sdpPhyPage::extendCTL( sdpPhyPageHdr * aPageHdr,
                              UInt            aSlotCnt,
                              UInt            aSlotSize,
                              UChar        ** aNewSlotStartPtr,
                              idBool        * aTrySuccess )
{
    UShort  sExtendSize;
    UChar * sSlotDirPtr;
    UChar * sNewSlotStartPtr = NULL;
    idBool  sTrySuccess      = ID_FALSE;

    IDE_ASSERT( aPageHdr  != NULL );
    IDE_ASSERT( aSlotCnt  != 0 );
    IDE_ASSERT( aSlotSize == idlOS::align8( aSlotSize ) );

    sExtendSize = aSlotCnt * aSlotSize;

    /* Check exceed freespace */
    IDE_TEST_CONT( sExtendSize > aPageHdr->mAvailableFreeSize,
                    CONT_ERR_EXTEND );

    if ( sExtendSize > sdpPhyPage::getNonFragFreeSize( aPageHdr ) )
    {
        IDE_TEST_CONT( aPageHdr->mPageType != SDP_PAGE_DATA,
                        CONT_ERR_EXTEND );

        IDE_ERROR( compactPage( aPageHdr, smLayerCallback::getRowPieceSize )
                   == IDE_SUCCESS );
    }

    sSlotDirPtr = getSlotDirStartPtr( (UChar*)aPageHdr );

    IDE_DASSERT( isSlotDirBasedPage(aPageHdr) == ID_TRUE );
    if ( sdpSlotDirectory::getSize(aPageHdr) > 0 )
    {
        shiftSlotDirToBottom(aPageHdr, sExtendSize);
    }

    aPageHdr->mSizeOfCTL            += sExtendSize;
    aPageHdr->mFreeSpaceBeginOffset += sExtendSize;
    aPageHdr->mTotalFreeSize        -= sExtendSize;
    aPageHdr->mAvailableFreeSize    -= sExtendSize;

    /* memmove 하기전 시작 SlotDirPtr이 확장후 확장된 CTS의
     * 시작 Ptr 가된다.*/
    sTrySuccess      = ID_TRUE;
    sNewSlotStartPtr = sSlotDirPtr;

    IDE_EXCEPTION_CONT( CONT_ERR_EXTEND );

    *aTrySuccess      = sTrySuccess;
    *aNewSlotStartPtr = sNewSlotStartPtr;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/***********************************************************************
 *
 * Description :
 *  slot directory 전체를 아래로 이동시킨다.
 *
 *  aPhyPageHdr - [IN] physical page header
 *  aShiftSize  - [IN] shift size
 *
 **********************************************************************/
void sdpPhyPage::shiftSlotDirToBottom(sdpPhyPageHdr     *aPageHdr,
                                      UShort             aShiftSize)
{
    UChar * sSlotDirPtr;

    sSlotDirPtr = getSlotDirStartPtr( (UChar*)aPageHdr );

    idlOS::memmove( sSlotDirPtr + aShiftSize,  /* dest ptr */
                    sSlotDirPtr,               /* src ptr  */
                    sdpSlotDirectory::getSize(aPageHdr) );
}


/******************************************************************************
 * Description :
 *  slot 할당이 가능한지 여부를 알려준다.
 *
 *  aPageHdr            - [IN] physical page header
 *  aSlotSize           - [IN] 저장하려는 slot의 크기
 *  aNeedAllocSlotEntry - [IN] slot entry 할당이 필요한지 여부
 *  aSlotAlignValue     - [IN] align value
 *
 ******************************************************************************/
idBool sdpPhyPage::canAllocSlot( sdpPhyPageHdr *aPageHdr,
                                 UInt           aSlotSize,
                                 idBool         aNeedAllocSlotEntry,
                                 UShort         aSlotAlignValue )
{
    UShort    sAvailableSize        = 0;
    UShort    sExtraSize            = 0;
    UShort    sReserveSize4Chaining;
    UShort    sMinRowPieceSize;

    IDE_DASSERT( validate(aPageHdr) == ID_TRUE );

    IDE_TEST_CONT( aSlotSize > SD_PAGE_SIZE, CONT_EXCEED_PAGESIZE );

    if ( aPageHdr->mPageType == SDP_PAGE_DATA )
    {
        if ( aNeedAllocSlotEntry == ID_TRUE )
        {
            if ( sdpSlotDirectory::isExistUnusedSlotEntry(
                    getSlotDirStartPtr((UChar*)aPageHdr))
                != ID_TRUE )
            {
                sExtraSize = ID_SIZEOF(sdpSlotEntry);
            }

            sMinRowPieceSize = smLayerCallback::getMinRowPieceSize();
            /* BUG-25395
             * [SD] alloc slot, free slot시에
             * row chaining을 위한 공간(6byte)를 계산해야 합니다. */
            if ( aSlotSize < sMinRowPieceSize )
            {
                /* slot의 크기가 minimum rowpiece size보다 작을 경우,
                 * row migration이 발생했을때 페이지에 가용공간이 부족하면
                 * slot 재할당에 실패하여 서버가 사망한다.
                 * 그래서 이런 경우에 대비하여 alloc slot시에
                 * (min rowpiece size - slot size)만큼의 공간을
                 * 추가로 더 확보해둔다.
                 */
                sReserveSize4Chaining  = sMinRowPieceSize - aSlotSize;
                sExtraSize            += sReserveSize4Chaining;
            }
        }

        sAvailableSize = aPageHdr->mAvailableFreeSize;
    }
    else
    {
        if ( aNeedAllocSlotEntry == ID_TRUE )
        {
            sExtraSize = ID_SIZEOF(sdpSlotEntry);
        }

        sAvailableSize =
            aPageHdr->mFreeSpaceEndOffset - aPageHdr->mFreeSpaceBeginOffset;
    }

    if ( sAvailableSize >=
        ( idlOS::align(aSlotSize, aSlotAlignValue) + sExtraSize ) )
    {
        return ID_TRUE;
    }

    IDE_EXCEPTION_CONT( CONT_EXCEED_PAGESIZE );

    return ID_FALSE;
}


/***********************************************************************
 *
 * Description :
 *  페이지에서 slot을 할당한다.
 *
 *  allocSlot() 함수와의 차이점
 *  1. 필요시 자동으로 compact page 연산을 수행한다.
 *  2. slot directory를 shift하지 않는다.
 *  3. align을 고려하지 않는다.
 *
 *  aPageHdr         - [IN]  페이지 헤더
 *  aSlotSize        - [IN]  할당하려는 slot의 크기
 *  aAllocSlotPtr    - [OUT] 할당받은 slot의 시작 pointer
 *  aAllocSlotNum    - [OUT] 할당받은 slot entry number
 *  aAllocSlotOffset - [OUT] 할당받은 slot의 offfset
 *
 **********************************************************************/
IDE_RC sdpPhyPage::allocSlot4SID( sdpPhyPageHdr       *aPageHdr,
                                  UShort               aSlotSize,
                                  UChar              **aAllocSlotPtr,
                                  scSlotNum           *aAllocSlotNum,
                                  scOffset            *aAllocSlotOffset )
{
    UChar       *sAllocSlotPtr;
    scSlotNum    sAllocSlotNum;
    scOffset     sAllocSlotOffset;
    UShort       sReserveSize4Chaining;
    UShort       sMinRowPieceSize;

    IDE_DASSERT( aPageHdr != NULL );
    IDE_DASSERT( validate(aPageHdr) == ID_TRUE );

    /* 현재는 data page에서만 이 함수를 사용한다. */
    IDE_ERROR_MSG( aPageHdr->mPageType == SDP_PAGE_DATA,
                   "aPageHdr->mPageType : %"ID_UINT32_FMT,
                   aPageHdr->mPageType );

    /* 이 함수를 호출하기 전에 canAllocSlot을 통해
     *  반드시 alloc 가능한지 확인하고, 가능할때만 호출해야 한다. */
    IDE_ERROR( canAllocSlot( aPageHdr,
                             aSlotSize,
                             ID_TRUE /* aNeedAllocSlotEntry */,
                             SDP_1BYTE_ALIGN_SIZE /* align value */ )
               == ID_TRUE );

    if ( (aSlotSize + ID_SIZEOF(sdpSlotEntry)) > getNonFragFreeSize(aPageHdr) )
    {
        /* FSEO와 FSBO 사이의 공간이 부족할 경우
         * compact page 연산을 수행하여
         * 단편화된 공간들을 모으고
         * 연속된 freespace를 확보한다. */
        IDE_TEST( compactPage( aPageHdr, smLayerCallback::getRowPieceSize )
                  != IDE_SUCCESS );
    }

    /* aSlotSize 만큼의 공간을 페이지로부터 할당한다. */
    IDE_TEST( allocSlotSpace( aPageHdr,
                              aSlotSize,
                              &sAllocSlotPtr,
                              &sAllocSlotOffset )
              != IDE_SUCCESS );

    IDE_DASSERT( validate(aPageHdr) == ID_TRUE );

    /* slot directory로부터 하나의 slot entry를 할당한다. */
    IDE_ERROR( sdpSlotDirectory::alloc( aPageHdr,
                                        sAllocSlotOffset,
                                        &sAllocSlotNum )
               == IDE_SUCCESS );

    if ( aPageHdr->mPageType == SDP_PAGE_DATA )
    {
        sMinRowPieceSize = smLayerCallback::getMinRowPieceSize();
        /* BUG-25395
         * [SD] alloc slot, free slot시에
         * row chaining을 위한 공간(6byte)를 계산해야 합니다. */
        if ( aSlotSize < sMinRowPieceSize )
        {
            /* slot의 크기가 minimum rowpiece size보다 작을 경우,
             * row migration이 발생했을때 페이지에 가용공간이 부족하면
             * slot 재할당에 실패하여 서버가 사망한다.
             * 그래서 이런 경우에 대비하여
             * (min rowpiece size - slot size)만큼의 공간을
             * 추가로 더 확보해둔다.
             */
            sReserveSize4Chaining = sMinRowPieceSize - aSlotSize;

            IDE_ERROR( aPageHdr->mAvailableFreeSize >= sReserveSize4Chaining );
            IDE_ERROR( aPageHdr->mTotalFreeSize     >= sReserveSize4Chaining );

            aPageHdr->mTotalFreeSize     -= sReserveSize4Chaining;
            aPageHdr->mAvailableFreeSize -= sReserveSize4Chaining;

            IDE_DASSERT( sdpPhyPage::validate(aPageHdr) == ID_TRUE );
        }
    }

    *aAllocSlotPtr    = sAllocSlotPtr;
    *aAllocSlotNum    = sAllocSlotNum;
    *aAllocSlotOffset = sAllocSlotOffset;

    IDE_DASSERT( validate(aPageHdr) == ID_TRUE );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    ideLog::logMem( IDE_DUMP_0, 
                    (UChar *)aPageHdr, 
                    SD_PAGE_SIZE,
                    "===================================================\n"
                    "         slot size : %u\n"
                    "               PID : %u\n",
                    aSlotSize,
                    aPageHdr->mPageID );

    return IDE_FAILURE;
}


/***********************************************************************
 *
 * Description :
 *   페이지에서 slot을 할당한다.
 *
 *  allocSlot4SID() 함수와의 차이점
 *  1. 자동으로 compact page 연산을 수행하지 않는다.
 *  2. slot directory를 shift한다.
 *  3. 저장시에 align을 고려한다.
 *
 *  aPageHdr            - [IN] 페이지 헤더
 *  aSlotNum            - [IN] 할당하려는 slot number
 *  aSaveSize           - [IN] 할당하려는 slot의 크기
 *  aNeedAllocSlotEntry - [IN] slot entry 할당이 필요한지 여부
 *  aAllowedSize        - [OUT] 페이지로부터 할당받은 공간의 크기
 *  aAllocSlotPtr       - [OUT] 할당받은 slot의 시작 pointer
 *  aAllocSlotOffset    - [OUT] 할당받은 slot의 offfset
 *  aSlotAlignValue     - [IN] 고려해야 하는 align 크기
 *
 **********************************************************************/
IDE_RC sdpPhyPage::allocSlot( sdpPhyPageHdr       *aPageHdr,
                              scSlotNum            aSlotNum,
                              UShort               aSaveSize,
                              idBool               aNeedAllocSlotEntry,
                              UShort              *aAllowedSize,
                              UChar              **aAllocSlotPtr,
                              scOffset            *aAllocSlotOffset,
                              UChar                aSlotAlignValue )
{
    UShort    sAlignedSize = idlOS::align(aSaveSize, aSlotAlignValue);
    UChar    *sAllocSlotPtr;
    UShort    sAllocSlotOffset;

    IDE_ERROR( aPageHdr != NULL );
    IDE_ERROR( ( getPageType(aPageHdr) != SDP_PAGE_UNFORMAT ) &&
               ( getPageType(aPageHdr) != SDP_PAGE_LOB_DATA ) &&
               ( getPageType(aPageHdr) != SDP_PAGE_LOB_INDEX ) );
    IDE_DASSERT( validate(aPageHdr) == ID_TRUE );

    /* data page에서는 allocSlot()이 아니고 allocSlot4SID() 함수를 사용해야 한다. */
    IDE_ERROR( aPageHdr->mPageType != SDP_PAGE_DATA );

    /* 이 함수를 호출하기 전에 canAllocSlot을 통해
     *  반드시 alloc 가능한지 확인하고, 가능할때만 호출해야 한다. */
    IDE_ERROR( canAllocSlot( aPageHdr,
                             sAlignedSize,
                             aNeedAllocSlotEntry,
                             aSlotAlignValue )
               == ID_TRUE );

    /* sAlignedSize 만큼의 공간을 페이지로부터 할당한다. */
    IDE_TEST( allocSlotSpace( aPageHdr,
                              sAlignedSize,
                              &sAllocSlotPtr,
                              &sAllocSlotOffset )
              != IDE_SUCCESS );

    /* 필요할 경우, slot directory로부터 하나의 slot entry를 할당한다. */
    if ( aNeedAllocSlotEntry == ID_TRUE )
    {
        sdpSlotDirectory::allocWithShift( aPageHdr,
                                          aSlotNum,
                                          sAllocSlotOffset );
    }
    else
    {
        /* nothing to do */
    }

    *aAllowedSize     = sAlignedSize;
    *aAllocSlotPtr    = sAllocSlotPtr;
    *aAllocSlotOffset = sAllocSlotOffset;

    IDE_DASSERT( validate(aPageHdr) == ID_TRUE );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    ideLog::log( IDE_SERVER_0, 
                 "Aligned size          : %u\n"
                 "Need alloc slot entry : %u\n"
                 "Slot align value      : %u\n"
                 "Slot number           : %u\n"
                 "Save size             : %u\n", 
                 sAlignedSize,
                 aNeedAllocSlotEntry,
                 aSlotAlignValue,
                 aSlotNum,
                 aSaveSize );
    ideLog::logMem( IDE_SERVER_0, (UChar *)aPageHdr, SD_PAGE_SIZE );

    return IDE_FAILURE;

}


/***********************************************************************
 *
 * Description :
 *  slot을 저장할 공간을 할당한다.
 *
 *  aPageHdr         - [IN] 페이지 헤더
 *  aSaveSize        - [IN] 할당하려는 slot의 크기
 *  aAllocSlotPtr    - [OUT] 할당받은 slot의 시작 pointer
 *  aAllocSlotOffset - [OUT] 할당받은 slot의 offfset
 *
 **********************************************************************/
IDE_RC sdpPhyPage::allocSlotSpace( sdpPhyPageHdr       *aPageHdr,
                                   UShort               aSaveSize,
                                   UChar              **aAllocSlotPtr,
                                   scOffset            *aAllocSlotOffset )
{
    scOffset    sSlotOffset   = 0;

    IDE_DASSERT( aPageHdr != NULL );
    IDE_DASSERT( validate(aPageHdr) == ID_TRUE );

    // BUG-30462 Page의 Max Size를 초과하여 삽입하려는 경우로 인하여
    //           검증코드를 추가합니다.
    IDE_ERROR( aPageHdr->mFreeSpaceEndOffset >= aSaveSize );
    IDE_ERROR( aPageHdr->mTotalFreeSize      >= aSaveSize );
    IDE_ERROR( aPageHdr->mAvailableFreeSize  >= aSaveSize );

    aPageHdr->mFreeSpaceEndOffset -= aSaveSize;
    sSlotOffset = aPageHdr->mFreeSpaceEndOffset;

    aPageHdr->mTotalFreeSize      -= aSaveSize;
    aPageHdr->mAvailableFreeSize  -= aSaveSize;

    *aAllocSlotOffset = sSlotOffset;
    *aAllocSlotPtr    = getPagePtrFromOffset( (UChar*)aPageHdr,
                                              sSlotOffset );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    tracePage( IDE_DUMP_0, 
               (UChar*)aPageHdr,
               "failure allocSlotSpace\n"
               "----------------------\n"
               "aSaveSize : %"ID_UINT32_FMT,
               aSaveSize );

    return IDE_FAILURE;

}


/***********************************************************************
 * Description :
 *
 * BUG-47580 1. compact page 디버깅 정보 보강
 *           2. Assert -> Compact 이전 page로 복원하고 예외처리
 *
 **********************************************************************/
IDE_RC sdpPhyPage::compactPage( sdpPhyPageHdr        * aPageHdr,
                                sdpGetSlotSizeFunc     aGetSlotSizeFunc )
{
    ULong       sOldPageImage[ SD_PAGE_SIZE / ID_SIZEOF(ULong) ];
    UChar      *sSlotDirPtr;
    UChar      *sOldSlotPtr;
    UChar      *sNewSlotPtr;
    UShort      sOldSlotOffset = 0;
    UShort      sNewSlotOffset = 0;
    UShort      sFreeSpaceEndOffset = 0;
    UShort      sSlotSize;
    UShort      sCount;
    scSlotNum   sSlotNum;

    IDE_ASSERT( aPageHdr            != NULL );
    IDE_ASSERT( aGetSlotSizeFunc    != NULL );
    IDE_DASSERT( validate(aPageHdr) == ID_TRUE );

    idlOS::memcpy( (UChar*)sOldPageImage, (UChar*)aPageHdr, SD_PAGE_SIZE );

    IDE_TEST( ((aPageHdr->mPageType != SDP_PAGE_DATA) &&
               (aPageHdr->mPageType != SDP_PAGE_TEMP_TABLE_META) &&
               (aPageHdr->mPageType != SDP_PAGE_TEMP_TABLE_DATA)) );

    sFreeSpaceEndOffset =
        getPageFooterStartPtr((UChar*)aPageHdr) - (UChar *)aPageHdr;

    sSlotDirPtr = getSlotDirStartPtr((UChar*)aPageHdr);
    sCount      = sdpSlotDirectory::getCount(sSlotDirPtr);

    for( sSlotNum = 0; sSlotNum < sCount; sSlotNum++ )
    {
        if ( sdpSlotDirectory::isUnusedSlotEntry(sSlotDirPtr, sSlotNum)
            == ID_TRUE )
        {
            IDE_TEST( aPageHdr->mPageType != SDP_PAGE_DATA );
        }
        else
        {
            IDE_TEST( sdpSlotDirectory::getValue( sSlotDirPtr,
                                                  sSlotNum,
                                                  &sOldSlotOffset )
                      != IDE_SUCCESS );
            sOldSlotPtr = (UChar*)sOldPageImage + sOldSlotOffset;

            sSlotSize = aGetSlotSizeFunc( sOldSlotPtr );

            // BUG-47580 compact page 디버깅 정보 보강
            // 1. 남아있는 Free size (end - begin) 보다 slot size가 더 큰 경우
            //   or
            // 2. 남아있는 실질적인 Free size (end - begin - slot size) 가
            //    page header에 기록된 free size 보다 작은 경우
            IDE_TEST_RAISE( ( sSlotSize >
                              (sFreeSpaceEndOffset -
                               aPageHdr->mFreeSpaceBeginOffset )) ||
                            ( aPageHdr->mTotalFreeSize >
                              ( sFreeSpaceEndOffset - sSlotSize -
                                aPageHdr->mFreeSpaceBeginOffset )),
                            err_invalid_slotsize );
            IDU_FIT_POINT_RAISE( "BUG-47580@sdpPhyPage::compactPage", err_invalid_slotsize );

            sFreeSpaceEndOffset -= sSlotSize;
            sNewSlotOffset       = sFreeSpaceEndOffset;
            sNewSlotPtr          = getPagePtrFromOffset( (UChar*)aPageHdr,
                                                         sNewSlotOffset );
            idlOS::memcpy( sNewSlotPtr, sOldSlotPtr, sSlotSize );

            sdpSlotDirectory::setValue( sSlotDirPtr,
                                        sSlotNum,
                                        sNewSlotOffset );
        }
    }

    aPageHdr->mFreeSpaceEndOffset = sFreeSpaceEndOffset;

    IDE_TEST( aPageHdr->mTotalFreeSize >
              (aPageHdr->mFreeSpaceEndOffset -
               aPageHdr->mFreeSpaceBeginOffset) );

    IDE_DASSERT( validate(aPageHdr) == ID_TRUE );

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_invalid_slotsize )
    {
        ideLog::log(IDE_ERR_0,
                    "compactPage Failure\n"
                    "Table OID: %"ID_vULONG_FMT", "
                    "Page ID: %"ID_UINT32_FMT", "
                    "Page Type: %"ID_UINT32_FMT"\n"
                    "Total Free Size: %"ID_UINT32_FMT", "
                    "Free Space Offset: %"ID_UINT32_FMT" - %"ID_UINT32_FMT", "
                    "Slot Offset: %"ID_UINT32_FMT", "
                    "Slot Size: %"ID_UINT32_FMT"\n",
                    aPageHdr->mTableOID,
                    aPageHdr->mPageID,
                    aPageHdr->mPageType,
                    aPageHdr->mTotalFreeSize ,
                    aPageHdr->mFreeSpaceBeginOffset,
                    sFreeSpaceEndOffset,
                    sOldSlotOffset,
                    sSlotSize );
    }
    IDE_EXCEPTION_END;

    // 작업중이던 page 출력
    sdpPhyPage::tracePage( IDE_DUMP_0,
                           (UChar*)aPageHdr,
                           "compactPage Failure\n"
                           "NEW PAGE:");
    // 복원하고 예외처리함.
    idlOS::memcpy( (UChar*)aPageHdr,
                   (UChar*)sOldPageImage,
                   SD_PAGE_SIZE );

    // 작업 이전의 상태로 복원하고 page 출력
    sdpPhyPage::tracePage( IDE_DUMP_0,
                           (UChar*)aPageHdr,
                           "compactPage Failure\n"
                           "OLD PAGE:");

    return IDE_FAILURE;
}

/***********************************************************************
 *
 * Description :
 *  페이지에서 slot을 해제한다.
 *
 *  freeSlot() 함수와의 차이점
 *  1. slot directory를 shift하지 않는다.
 *  2. align을 고려하지 않는다.
 *
 *  aPageHdr  - [IN]  페이지 헤더
 *  aSlotPtr  - [IN]  해제하려는 slot의 pointer
 *  aSlotNum  - [IN]  해제하려는 slot의 entry number
 *  aSlotSize - [IN]  해제하려는 slot의 size
 *
 **********************************************************************/
IDE_RC sdpPhyPage::freeSlot4SID( sdpPhyPageHdr      *aPageHdr,
                                 UChar              *aSlotPtr,
                                 scSlotNum           aSlotNum,
                                 UShort              aSlotSize )
{
    UShort      sReserveSize4Chaining;
    UShort      sMinRowPieceSize;
    scOffset    sSlotOffset;

    IDE_ERROR( aPageHdr != NULL );
    IDE_ERROR( aSlotPtr != NULL );
    IDE_ERROR( aPageHdr->mPageType == SDP_PAGE_DATA );
    IDE_DASSERT( validate(aPageHdr) == ID_TRUE );

    sSlotOffset = getOffsetFromPagePtr( (UChar*) aSlotPtr);

    IDE_ERROR( sdpSlotDirectory::free( aPageHdr, aSlotNum ) 
               == IDE_SUCCESS );

    freeSlotSpace( aPageHdr, sSlotOffset, aSlotSize );

    if ( aPageHdr->mPageType == SDP_PAGE_DATA )
    {
        sMinRowPieceSize = smLayerCallback::getMinRowPieceSize();
        /* BUG-25395
         * [SD] alloc slot, free slot시에
         * row chaining을 위한 공간(6byte)를 계산해야 합니다. */
        if ( aSlotSize < sMinRowPieceSize )
        {
            /* slot의 크기가 minimum rowpiece size보다 작을 경우,
             * alloc slot시에 (min rowpiece size - slot size)만큼의 공간을
             * 추가로 더 할당하였다.
             * ( sdpPageList::allocSlot4SID() 함수 참조 )
             * 그래서 free slot시에 추가로 할당한 공간을 해제해주어야 한다.
             */
            sReserveSize4Chaining = sMinRowPieceSize - aSlotSize;

            aPageHdr->mTotalFreeSize     += sReserveSize4Chaining;
            aPageHdr->mAvailableFreeSize += sReserveSize4Chaining;

            IDE_DASSERT( sdpPhyPage::validate(aPageHdr) == ID_TRUE );
        }
    }

    IDE_DASSERT( validate(aPageHdr) == ID_TRUE );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    tracePage( IDE_DUMP_0, 
               (UChar*)aPageHdr,
               "failure freeSlot4SID\n"
               "----------------------\n"
               "aSlotNum  : %"ID_UINT32_FMT,
               "aSlotSize : %"ID_UINT32_FMT,
               aSlotNum,
               aSlotSize );


    return IDE_FAILURE;
}


/***********************************************************************
 *
 * Description :
 *  페이지에서 slot을 해제한다.
 *
 *  freeSlot4SID() 함수와의 차이점
 *  1. slot directory를 shift한다.
 *  2. 저장시에 align을 고려한다.
 *
 *  aPageHdr           - [IN] 페이지 헤더
 *  aSlotPtr           - [IN] 해제하려는 slot의 pointer
 *  aFreeSize          - [IN] 해제하려는 slot의 size
 *  aNeedFreeSlotEntry - [IN] slot entry를 해제할지 여부
 *  aSlotAlignValue    - [IN] align value
 *
 **********************************************************************/
IDE_RC sdpPhyPage::freeSlot( sdpPhyPageHdr      *aPageHdr,
                             UChar              *aSlotPtr,
                             UShort              aFreeSize,
                             idBool              aNeedFreeSlotEntry,
                             UChar               aSlotAlignValue )
{
    UChar      *sSlotDirPtr;
    SShort      sSlotNum = -1;
    scOffset    sSlotOffset;
    UShort      sAlignedSize = idlOS::align(aFreeSize, aSlotAlignValue);

    IDE_ERROR( aPageHdr != NULL );
    IDE_ERROR( aSlotPtr != NULL );
    IDE_ERROR( ( getPageType(aPageHdr) != SDP_PAGE_UNFORMAT ) &&
               ( getPageType(aPageHdr) != SDP_PAGE_LOB_DATA ) &&
               ( getPageType(aPageHdr) != SDP_PAGE_LOB_INDEX ) );
    IDE_ERROR( aPageHdr->mPageType != SDP_PAGE_DATA );
    IDE_DASSERT( validate(aPageHdr) == ID_TRUE );

    sSlotOffset = getOffsetFromPagePtr(aSlotPtr);

    if ( aNeedFreeSlotEntry == ID_TRUE )
    {
        sSlotDirPtr = getSlotDirStartPtr((UChar*)aPageHdr);

        IDE_ERROR( sdpSlotDirectory::find(sSlotDirPtr, sSlotOffset, &sSlotNum)
                   == IDE_SUCCESS );
        IDE_ERROR( sSlotNum >=0 );

        IDE_ERROR( sdpSlotDirectory::freeWithShift( aPageHdr, sSlotNum )
                   == IDE_SUCCESS );
    }

    freeSlotSpace( aPageHdr, sSlotOffset, sAlignedSize );

    IDE_DASSERT( validate(aPageHdr) == ID_TRUE );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    tracePage( IDE_DUMP_0, 
               (UChar*)aPageHdr,
               "failure freeSlot\n"
               "----------------------\n"
               "aFreeSize          : %"ID_UINT32_FMT,
               "aNeedFreeSlotEntry : %"ID_UINT32_FMT,
               "aSlotAlignValue    : %"ID_UINT32_FMT,
               "sSlotNum           : %"ID_UINT32_FMT,
               aFreeSize,
               aNeedFreeSlotEntry,
               aSlotAlignValue,
               sSlotNum );


    return IDE_FAILURE;
}


/***********************************************************************
 *
 * Description :
 *  페이지에서 slot을 해제한다.
 *
 *  freeSlot4SID() 함수와의 차이점
 *  1. slot directory를 shift한다.
 *  2. 저장시에 align을 고려한다.
 *
 *  aPageHdr           - [IN] 페이지 헤더
 *  aSlotNum           - [IN] 해제하려는 slot의 entry number
 *  aFreeSize          - [IN] 해제하려는 slot의 size
 *  aSlotAlignValue    - [IN] align value
 *
 **********************************************************************/
IDE_RC sdpPhyPage::freeSlot(sdpPhyPageHdr   *aPageHdr,
                            scSlotNum        aSlotNum,
                            UShort           aFreeSize,
                            UChar            aSlotAlignValue)
{
    UChar      *sSlotDirPtr  = NULL;
    scOffset    sSlotOffset  = 0;
    UShort      sAlignedSize = idlOS::align(aFreeSize, aSlotAlignValue);

    IDE_ERROR( aPageHdr != NULL );
    IDE_ERROR( ( getPageType(aPageHdr) != SDP_PAGE_UNFORMAT ) &&
               ( getPageType(aPageHdr) != SDP_PAGE_LOB_DATA ) &&
               ( getPageType(aPageHdr) != SDP_PAGE_LOB_INDEX ) );
    IDE_ERROR( aPageHdr->mPageType != SDP_PAGE_DATA );
    IDE_DASSERT( validate(aPageHdr) == ID_TRUE );

    sSlotDirPtr = getSlotDirStartPtr((UChar*)aPageHdr);
    IDE_TEST( sdpSlotDirectory::getValue( sSlotDirPtr, aSlotNum, &sSlotOffset )
              != IDE_SUCCESS);

    IDE_ERROR( sdpSlotDirectory::freeWithShift( aPageHdr, aSlotNum )
               == IDE_SUCCESS);
    freeSlotSpace( aPageHdr, sSlotOffset, sAlignedSize );

    IDE_DASSERT( validate(aPageHdr) == ID_TRUE );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    tracePage( IDE_DUMP_0, 
               (UChar*)aPageHdr,
               "failure freeSlot4SID\n"
               "----------------------\n"
               "aSlotNum        : %"ID_UINT32_FMT
               "aFreeSize       : %"ID_UINT32_FMT
               "aSlotAlignValue : %"ID_UINT32_FMT,
               aSlotNum,
               aFreeSize,
               aSlotAlignValue );


    return IDE_FAILURE;
}


/***********************************************************************
 *
 * Description :
 *
 *
 * Implementation :
 *
 *
 **********************************************************************/
void sdpPhyPage::freeSlotSpace( sdpPhyPageHdr        *aPageHdr,
                                scOffset              aSlotOffset,
                                UShort                aFreeSize)
{
    IDE_DASSERT( aPageHdr != NULL );
    IDE_DASSERT( validate(aPageHdr) == ID_TRUE );

    if ( aPageHdr->mFreeSpaceEndOffset == aSlotOffset )
    {
        aPageHdr->mFreeSpaceEndOffset += aFreeSize;
    }

    aPageHdr->mTotalFreeSize     += aFreeSize;
    aPageHdr->mAvailableFreeSize += aFreeSize;
}


/***********************************************************************
 *
 * Description : 한 페이지가 제대로 됐는지 검사한다.
 *               BUG-47953 상태 확인 까지만 하는 함수도 필요하다.
 *
 * Implementation :
 * if ( LSN 체크면 )
 *     첫 LSN과 마지막 LSN 비교
 *
 * else
 *     페이지의 체크섬을 구한다.
 *     if ( 다르다면  )
 *         return true
 *     return false
 *
 **********************************************************************/
idBool sdpPhyPage::isPageCorrupted( UChar  * aStartPtr )
{

    idBool isCorrupted = ID_TRUE;
    sdpPhyPageHdr *sPageHdr;
    sdpPageFooter *sPageFooter;
    smLSN          sHeadLSN;
    smLSN          sTailLSN; 

    sPageHdr    = getHdr(aStartPtr);
    sPageFooter = (sdpPageFooter*)
        getPageFooterStartPtr((UChar*)sPageHdr);

    if ( smuProperty::getCheckSumMethod() == SDP_CHECK_LSN )
    {
        sHeadLSN = getPageLSN((UChar*)sPageHdr);
        sTailLSN = sPageFooter->mPageLSN;

        isCorrupted = smrCompareLSN::isEQ( &sHeadLSN, &sTailLSN ) ?
            ID_FALSE : ID_TRUE;
    }
    else
    {
        if ( smuProperty::getCheckSumMethod() == SDP_CHECK_CHECKSUM )
        {
            // BUG-45598: checksum 값이 초기상태 라면, 이번엔  LSN 으로 검사  
            if( getCheckSum(sPageHdr) == SDP_CHECKSUM_INIT_VAL )
            {
                sHeadLSN = getPageLSN((UChar*)sPageHdr);
                sTailLSN = sPageFooter->mPageLSN;
             
                isCorrupted = smrCompareLSN::isEQ( &sHeadLSN, &sTailLSN ) ?
                    ID_FALSE : ID_TRUE;
            }
            else
            {
                isCorrupted =
                    ( getCheckSum(sPageHdr) != calcCheckSum(sPageHdr) ?
                      ID_TRUE : ID_FALSE );
            }
        }
        else
        {
            IDE_DASSERT( ID_FALSE );
        }
    }
    return isCorrupted;
}

/***********************************************************************
 * Description : 한 페이지가 제대로 됐는지 검사하고,
 *               문제가 있으면 inconsistent 로 설정한다.
 *
 * return : sdpPhyPage::isPageCorrupted() 와 동일
 *          ID_TRUE  : Corrupted, Inconsistent Page
 *          ID_FALSE : Consistent Page
 **********************************************************************/
idBool sdpPhyPage::checkAndSetPageCorrupted( scSpaceID  aSpaceID,
                                             UChar     *aStartPtr )
{
    idBool isCorrupted ;
    sdpPhyPageHdr *sPageHdr;

    isCorrupted = isPageCorrupted( aStartPtr );

    // BUG-45598: sdpPhyPage::isPageCorrupted 에서 inconsistent 설정
    if ( isCorrupted == ID_TRUE )
    {
        sPageHdr = getHdr(aStartPtr);
        if ( ( smrRecoveryMgr::isRestart() == ID_FALSE ) &&
             ( isConsistentPage( aStartPtr ) == ID_TRUE ) )
        {
            setPageInconsistency( sPageHdr );
        }

        // To verify CASE-6829
        if ( smuProperty::getSMChecksumDisable() == 1 )
        {
            ideLog::log(IDE_SM_0,
                        "[WARNING!!] Page(%u,%u) is corrupted\n",
                        aSpaceID,
                        sPageHdr->mPageID);
            return ID_FALSE;
        }
    }
    return isCorrupted;
}

/***********************************************************************
 *
 * Description :
 *
 * Implementation :
 *
 *
 **********************************************************************/
IDE_RC sdpPhyPage::verify( sdpPhyPageHdr *aPageHdr,
                           scPageID       aSpaceID,
                           scPageID       aPageID )
{

    const UInt sErrorLen = 512;
    SChar      sErrorMsg[sErrorLen];

    IDE_DASSERT( (UChar*)aPageHdr == getPageStartPtr((UChar*)aPageHdr));

    if ( aSpaceID != getSpaceID((UChar*)aPageHdr) )
    {

        smuUtility::mWriteError(SM_TRC_LOG_LEVEL_FATAL,
                                SM_TRC_DPAGE_PHY_ERROR1,
                                aSpaceID,
                                getSpaceID((UChar*)aPageHdr));

        IDE_RAISE(err_verify_failed);
    }


    if ( aPageID != aPageHdr->mPageID )
    {
        smuUtility::mWriteError(SM_TRC_LOG_LEVEL_FATAL,
                                SM_TRC_DPAGE_PHY_ERROR2,
                                (UInt)aPageID,
                                (UInt)aPageHdr->mPageID);

        IDE_RAISE(err_verify_failed);
    }


    if ( aPageHdr->mTotalFreeSize > SD_PAGE_SIZE )
    {
        smuUtility::mWriteError(SM_TRC_LOG_LEVEL_FATAL,
                                SM_TRC_DPAGE_PHY_ERROR3,
                                aSpaceID,
                                (UInt)aPageHdr->mPageID,
                                SD_PAGE_SIZE,
                                (UInt)aPageHdr->mTotalFreeSize,
                                aPageHdr->mFreeSpaceEndOffset);

        IDE_RAISE(err_verify_failed);
    }

    if ( aPageHdr->mFreeSpaceEndOffset > SD_PAGE_SIZE )
    {
        smuUtility::mWriteError(SM_TRC_LOG_LEVEL_FATAL,
                                SM_TRC_DPAGE_PHY_ERROR4,
                                aSpaceID,
                                (UInt)aPageHdr->mPageID,
                                SD_PAGE_SIZE,
                                (UInt)aPageHdr->mTotalFreeSize,
                                (UInt)aPageHdr->mFreeSpaceEndOffset);

        IDE_RAISE(err_verify_failed);
    }

    if ( aPageHdr->mPageType >= (UShort)SDP_PAGE_TYPE_MAX)
    {
        smuUtility::mWriteError(SM_TRC_LOG_LEVEL_FATAL,
                                SM_TRC_DPAGE_PHY_ERROR5,
                                aSpaceID,
                                (UInt)aPageHdr->mPageID,
                                (UInt)aPageHdr->mPageType);

        IDE_RAISE(err_verify_failed);
    }



    return IDE_SUCCESS;

    IDE_EXCEPTION(err_verify_failed)
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_InvalidDB,
                                sErrorMsg));
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**********************************************************************
 * Description :
 *
 * if ( aIsConsistent == 'F' )
 *     physical header의 mIsConsistent를 'F'로 logging한다.
 * else // aIsConsistent == 'T'
 *     physical header의 mIsConsistent를 'T'로 설정한다. // nologging
 *
 * log없이 생성된 page에 대해, 정상적인 operation(logging mode operation)이
 * 수행된 후 media failure가 발생했을 때 redo 수행도중 server가 죽는 현상이
 * 발생한다. (nologging noforce로 생성된 index의 경우, index build후
 * 갱신연산이 수행되고 failure가 발생했을 때도 redo 수행도중 server가 죽는다)
 * 이를 방지하기 위해 page의 physical header에 page를 logical redo할 수 있는지
 * 여부를 표시하는 mIsConsistent를 추가하고 nologging operation시 'F'로 초기화
 * 해서 logging하고, nologging operation이 완료한 후 physical header에 'T'로
 * 설정한다. 이때 log를 기록하지 않는다.
 *
 **********************************************************************/
IDE_RC  sdpPhyPage::setPageConsistency(
    sdrMtx         *aMtx,
    sdpPhyPageHdr  *aPageHdr,
    UChar          *aIsConsistent )
{
    IDE_TEST( sdrMiniTrans::writeNBytes( aMtx,
                                         (UChar*)&aPageHdr->mIsConsistent,
                                         aIsConsistent,
                                         ID_SIZEOF(*aIsConsistent) )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* PROJ-2162 Mtx없이 알아서 수행하기 */
IDE_RC sdpPhyPage::setPageInconsistency( scSpaceID       aSpaceID,
                                         scPageID        aPageID )
{
    sdrMtx          sMtx;
    idBool          sIsMtxBegin = ID_FALSE;
    idBool          sIsSuccess  = ID_FALSE;
    sdpPhyPageHdr * sPagePtr;
    UChar           sIsConsistent = SDP_PAGE_INCONSISTENT;

    IDE_TEST( sdrMiniTrans::begin( NULL, // idvSQL
                                   &sMtx,
                                   NULL, // Trans
                                   SDR_MTX_LOGGING,
                                   ID_FALSE,/*MtxUndoable(PROJ-2162)*/
                                   SM_DLOG_ATTR_DEFAULT | SM_DLOG_ATTR_DUMMY )
              != IDE_SUCCESS );
    sIsMtxBegin = ID_TRUE;

    IDE_TEST( sdbBufferMgr::getPageByPID( NULL, // idvSQL
                                          aSpaceID,
                                          aPageID,
                                          SDB_X_LATCH,
                                          SDB_WAIT_NORMAL,
                                          SDB_SINGLE_PAGE_READ,
                                          &sMtx,
                                          (UChar**)&sPagePtr,
                                          &sIsSuccess,
                                          NULL ) // IsCorruptPage
              != IDE_SUCCESS );
    IDE_ERROR( sIsSuccess == ID_TRUE );

    /* 잘못된 페이지에 대한 정보를 기록함 */
    sdpPhyPage::tracePage( IDE_DUMP_0,
                           (UChar*)sPagePtr,
                           "DRDB PAGE:");
    IDE_TEST( sdpPhyPage::setPageConsistency( &sMtx,
                                              sPagePtr,
                                              &sIsConsistent )
              != IDE_SUCCESS );

    sIsMtxBegin = ID_FALSE;
    IDE_TEST( sdrMiniTrans::commit(&sMtx) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sIsMtxBegin == ID_TRUE )
    {
        (void)sdrMiniTrans::rollback( &sMtx );
    }
    return IDE_FAILURE;
}

// BUG-45598: 운영중 페이지 corrupt 발생할 경우 체크
IDE_RC sdpPhyPage::setPageInconsistency( sdpPhyPageHdr * aPagePtr )
{
    sdrMtx          sMtx;
    idBool          sIsMtxBegin = ID_FALSE;
    UChar           sIsConsistent = SDP_PAGE_INCONSISTENT;
    
    IDE_TEST( sdrMiniTrans::begin( NULL, // idvSQL
                                   &sMtx,
                                   NULL, // Trans
                                   SDR_MTX_LOGGING,
                                   ID_FALSE,/*MtxUndoable(PROJ-2162)*/
                                   SM_DLOG_ATTR_DEFAULT | SM_DLOG_ATTR_DUMMY )
              != IDE_SUCCESS );
    sIsMtxBegin = ID_TRUE;

    /* 잘못된 페이지에 대한 정보를 기록함 */
    sdpPhyPage::tracePage( IDE_DUMP_0,
                           (UChar*)aPagePtr,
                           "DRDB PAGE:");
    IDE_TEST( sdpPhyPage::setPageConsistency( &sMtx,
                                              aPagePtr,
                                              &sIsConsistent )
              != IDE_SUCCESS );

    sIsMtxBegin = ID_FALSE;
    IDE_TEST( sdrMiniTrans::commit(&sMtx) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sIsMtxBegin == ID_TRUE )
    {
        (void)sdrMiniTrans::rollback( &sMtx );
    }
    return IDE_FAILURE;
}

/***********************************************************************
 *
 * Description :
 *
 * Implementation :
 *
 *
 **********************************************************************/
idBool sdpPhyPage::validate( sdpPhyPageHdr * aPageHdr )
{
    sdpCTL   * sCTL;

    IDE_DASSERT( (UChar*)aPageHdr == getPageStartPtr((UChar*)aPageHdr));

    if ( aPageHdr->mPageType == SDP_PAGE_UNFORMAT )
    {
        return ID_TRUE;
    }

    IDE_DASSERT( aPageHdr->mTotalFreeSize <=
                 ( getEmptyPageFreeSize() - aPageHdr->mLogicalHdrSize ) );

    IDE_DASSERT( aPageHdr->mTotalFreeSize >=
                 aPageHdr->mAvailableFreeSize );

    IDE_DASSERT( aPageHdr->mFreeSpaceBeginOffset >=
                 ( getPhyHdrSize() + aPageHdr->mLogicalHdrSize ) );

    IDE_DASSERT( aPageHdr->mFreeSpaceEndOffset   <=
                 (SD_PAGE_SIZE - ID_SIZEOF(sdpPageFooter)) );

    IDE_DASSERT( aPageHdr->mFreeSpaceBeginOffset <=
                 aPageHdr->mFreeSpaceEndOffset );

    if ( (aPageHdr->mSizeOfCTL != 0) &&
         (aPageHdr->mPageType  == SDP_PAGE_DATA) )
    {
        sCTL = (sdpCTL*)getCTLStartPtr( (UChar*)aPageHdr );

        if ( SM_SCN_IS_MAX( sCTL->mSCN4Aging ) )
        {
            IDE_DASSERT( sCTL->mCandAgingCnt == 0 );
        }
        else
        {
            /* nothing to do */
        }
    }

    return ID_TRUE;

}

/* --------------------------------------------------------------------
 * TASK-4007 [SM]PBT를 위한 기능 추가
 *
 * Description : dump 함수. sdpPhyPageHdr를 양식에 맞게 뿌려준다.
 *
 * BUG-28379 [SD] sdnbBTree::dumpNodeHdr( UChar *aPage ) 내에서
 * local Array의 ptr를 반환하고 있습니다.
 * Local Array대신 OutBuf를 받아 리턴하도록 수정합니다.
 * ----------------------------------------------------------------- */

IDE_RC sdpPhyPage::dumpHdr( const UChar    * aPage ,
                            SChar          * aOutBuf ,
                            UInt             aOutSize )
{
    const sdpPhyPageHdr * sPageHdr;
    smLSN sTailLSN;

    IDE_ERROR( aPage   != NULL );
    IDE_ERROR( aOutBuf != NULL );
    IDE_ERROR( aOutSize > 0 );

    sPageHdr = (sdpPhyPageHdr*)getPageStartPtr( (UChar*)aPage );

    IDE_DASSERT( (UChar*)sPageHdr == aPage );

    sTailLSN = ((sdpPageFooter*)getPageFooterStartPtr((UChar*)sPageHdr))->mPageLSN;

    // TODO: Fixed Table에서 SmoNo를 위한 값이 UInt에서 ULong으로 변경되어도
    //       되는지 확인하기
    idlOS::snprintf( aOutBuf ,
                     aOutSize ,
                     "----------- Physical Page Begin ----------\n"
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
                     "IndexID                   : %"ID_UINT32_FMT"\n"
                     "Seq No                    : %"ID_UINT64_FMT"\n"
                     "TailPageLSN.mFileNo       : %"ID_UINT32_FMT"\n"
                     "TailPageLSN.mOffset       : %"ID_UINT32_FMT"\n"
                     "----------- Physical Page End ----------\n",
                     (UChar*)sPageHdr,
                     sPageHdr->mFrameHdr.mCheckSum,
                     sPageHdr->mFrameHdr.mPageLSN.mFileNo,
                     sPageHdr->mFrameHdr.mPageLSN.mOffset,
                     sPageHdr->mFrameHdr.mIndexSMONo,
                     sPageHdr->mFrameHdr.mSpaceID,
                     sPageHdr->mFrameHdr.mBCBPtr,
                     sPageHdr->mTotalFreeSize,
                     sPageHdr->mAvailableFreeSize,
                     sPageHdr->mLogicalHdrSize,
                     sPageHdr->mSizeOfCTL,
                     sPageHdr->mFreeSpaceBeginOffset,
                     sPageHdr->mFreeSpaceEndOffset,
                     sPageHdr->mPageType,
                     sPageHdr->mPageState,
                     sPageHdr->mPageID,
                     sPageHdr->mIsConsistent,
                     sPageHdr->mLinkState,
                     sPageHdr->mParentInfo.mParentPID,
                     sPageHdr->mParentInfo.mIdxInParent,
                     sPageHdr->mListNode.mPrev,
                     sPageHdr->mListNode.mNext,
                     sPageHdr->mTableOID,
                     sPageHdr->mIndexID,
                     sPageHdr->mSeqNo,
                     sTailLSN.mFileNo,
                     sTailLSN.mOffset );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* --------------------------------------------------------------------
 * PROJ-2118 BUG Reporting
 *
 * Description : Page의 Hexa data 를 지정한 trace file에 뿌려준다.
 *               Page ID 등 해당 Page에 대한 다른 정보를
 *               추가로 입력하여 기록하게 할 수 있다.
 *
 * ----------------------------------------------------------------- */
IDE_RC sdpPhyPage::tracePage( UInt           aChkFlag,
                              ideLogModule   aModule,
                              UInt           aLevel,
                              const UChar  * aPage,
                              const SChar  * aTitle,
                              ... )
{
    va_list ap;
    IDE_RC result;

    va_start( ap, aTitle );

    result = tracePageInternal( aChkFlag,
                                aModule,
                                aLevel,
                                aPage,
                                aTitle,
                                ap );

    va_end( ap );

    return result;
}

IDE_RC sdpPhyPage::tracePageInternal( UInt           aChkFlag,
                                      ideLogModule   aModule,
                                      UInt           aLevel,
                                      const UChar  * aPage,
                                      const SChar  * aTitle,
                                      va_list        ap )
{
    SChar         * sDumpBuf = NULL;
    const UChar   * sPage;
    UInt            sOffset;
    UInt            sState = 0;

    ideLogEntry sLog( aChkFlag,
                      aModule,
                      aLevel );

    IDE_ERROR( aPage != NULL );

    sPage = getPageStartPtr( (UChar*)aPage );

    sOffset = aPage - sPage;

    IDE_TEST( iduMemMgr::calloc( IDU_MEM_SM_SDP, 
                                 1,
                                 ID_SIZEOF( SChar ) * IDE_DUMP_DEST_LIMIT,
                                 (void**)&sDumpBuf )
              != IDE_SUCCESS );
    sState = 1;

    if ( aTitle != NULL )
    {
        idlOS::vsnprintf( sDumpBuf,
                          IDE_DUMP_DEST_LIMIT,
                          aTitle,
                          ap );

        sLog.appendFormat( "%s\n"
                           "Offset : %u[%04x]\n",
                           sDumpBuf,
                           sOffset,
                           sOffset );
    }
    else
    {
        sLog.appendFormat( "[ Dump Page ]\n"
                           "Offset : %u[%04x]\n",
                           sOffset,
                           sOffset );
    }

    (void)sLog.dumpHex( sPage, SD_PAGE_SIZE, IDE_DUMP_FORMAT_FULL );

    sLog.append( "\n" );

    if ( sdpPhyPage::dumpHdr( sPage ,
                              sDumpBuf ,
                              IDE_DUMP_DEST_LIMIT )
        == IDE_SUCCESS )
    {
        sLog.appendFormat( "%s\n",
                           sDumpBuf );
    }
    else
    {
        /* nothing to do */
    }

    sLog.write();

    IDE_TEST( iduMemMgr::free( sDumpBuf ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sState != 0 )
    {
        IDE_ASSERT( iduMemMgr::free( sDumpBuf ) == IDE_SUCCESS );
    }

    return IDE_FAILURE;
}

/* --------------------------------------------------------------------
 * Description : Disk page용 memcpy함수
 *               page의 경계를 넘어서거나, sdpPhyPageHeader에 겹치게
 *               memcpy하는지 검사한다.
 *
 * BUG-32528 disk page header의 BCB Pointer 가 긁혔을 경우에 대한
 * 디버깅 정보 추가.
 * ----------------------------------------------------------------- */
IDE_RC sdpPhyPage::writeToPage( UChar * aDestPagePtr,
                                UChar * aSrcPtr,
                                UInt    aLength )
{
    UInt   sOffset;

    IDE_ASSERT( aDestPagePtr != NULL );
    IDE_ASSERT( aSrcPtr      != NULL );

    sOffset = aDestPagePtr - getPageStartPtr( aDestPagePtr );

    IDE_TEST( ( sOffset + aLength ) > SD_PAGE_SIZE );
    IDE_TEST( sOffset < ID_SIZEOF( sdpPhyPageHdr ) );

    idlOS::memcpy( aDestPagePtr, aSrcPtr, aLength );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    {
        ideLog::log( IDE_DUMP_0,
                     "Invalid memcpy to disk page\n"
                     "PagePtr : %"ID_xPOINTER_FMT"\n"
                     "Offset  : %u\n"
                     "Length  : %u\n"
                     "SrcPtr  : %"ID_xPOINTER_FMT"\n",
                     aDestPagePtr - sOffset,
                     sOffset,
                     aLength,
                     aSrcPtr );

        tracePage( IDE_DUMP_0,
                   aDestPagePtr,
                   NULL );
    }

    return IDE_FAILURE;
}

/* --------------------------------------------------------------------
 * Description : BUG-47429 SMO NO Reset을 리팩토링 합니다.
 *
 * aPageHdr      - [IN] Page의 Header
 * aSpaceID      - [IN] OnlineOffline여부를 판단 해야 하는 Space의 SpaceID
 * aChkOnlineTBS - [IN] Online Offline 여부 판단 해야하는지 유무
 *                      BUG-47439를 진행하면서 기존에 sOnlineTBSLSN4Idx를
 *                      INIT으로 넘기는 경우는 ID_FALSE로 대체하였다.
 * ----------------------------------------------------------------- */
void sdpPhyPage::resetIndexSMONo( sdpPhyPageHdr * aPageHdr,
                                  scSpaceID       aSpaceID,
                                  idBool          aChkOnlineTBS )
{
    smLSN    sStartLSN;
    smLSN    sPageLSN;
    smLSN    sOnlineTBSLSN4Idx;
    void   * sSpaceNode;

    // SMO no를 초기화해야 한다.
    sStartLSN  = smrRecoveryMgr::getIdxSMOLSN();
    sPageLSN   = aPageHdr->mFrameHdr.mPageLSN ;
    // restart 이후에 Runtime 정보인 SMO No를 초기화한다.
    if ( smLayerCallback::isLSNGT( &sPageLSN, &sStartLSN ) == ID_FALSE )
    {
        // recorvery LSN과 비교하여 작은 것만 set한다.
        setIndexSMONo( aPageHdr, 0 );
    }
    else
    {
        /* fix BUG-17456 Disk Tablespace online이후 update 발생시 index 무한루프
         * sPageLSN <= sStartLSN 이거나 sPageLSN <= aOnlineTBSLSN4Idx
         *
         * BUG-17456을 해결하기 위해 추가된 조건이다.
         * 운영중에 offline 되었다 다시 Online 된 TBS의 Index의 SMO No를
         * 보정하였고, aOnlineTBSLSN4Idx이전의 PageLSN을 가진 Page를
         * read한 경우에는 SMO No를 0으로 초기화하여 index traverse할때
         * 무한 loop 에 빠지지 않게 한다. */
        if ( aChkOnlineTBS == ID_TRUE )
        {
            sSpaceNode = sctTableSpaceMgr::findSpaceNodeIncludingDropped( aSpaceID );
            sOnlineTBSLSN4Idx = sddTableSpace::getOnlineTBSLSN4Idx( sSpaceNode );

            if ( (!( SM_IS_LSN_INIT( sOnlineTBSLSN4Idx ) ) ) &&
                 ( smLayerCallback::isLSNGT( &sPageLSN, &sOnlineTBSLSN4Idx )
                   == ID_FALSE ) )
            {
                setIndexSMONo( aPageHdr, 0 );
            }
        }
    }
}
