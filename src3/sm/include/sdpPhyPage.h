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
 * $Id: sdpPhyPage.h 87929 2020-07-03 00:38:42Z et16 $
 *
 * Description :
 *
 * Page
 *
 * # 개념
 *
 * tablespace의 page에 대한 자료구조 관리
 *
 *
 * # 구조
 *     __________________SM_PAGE_SIZE(32KB)__________________________________
 *    |                                                                      |
 *     __________sdpPageHdr___________________________________________
 *    /_______________________________________________________________\______
 *    |  |  node   |       |   |         |       |           |         |     |
 *    |  |_________|       |   |         |       |           |         |     |
 *    |@ |prev|next|page id|LSN| ext.RID |SMO No |free offset|free size|     |
 *    |__|____|____|_______|___|_________|_______|___________|_________|_____|
 *      |
 *      checksum
 *
 *  - sdpPageHdr(모든 Page들의 공통적인 Physical 헤더)
 *
 *    + checksum :  page 시작 offset에 4bytes값으로 저장
 *
 *    + sdpPageListNode : previous page id와 next page id를 저장
 *
 *    + LSN : 페이지에 대한 마지막 반영한 로그의 LSN (update LSN)
 *            (!!) 페이지를 flush하기전까지는 본 LSN까지 log가 디스크에 반영
 *            되어 있어야 함
 *
 *    + ext.desc.RID : 본 페이지를 할당해 준 extent desc.의 rid를 저장
 *                     페이지 free시 쉽게 처리하기 위해서임.
 *
 *    + SMO No : 인덱스의 진행여부를 알기위한 번호이다.
 *
 *    + Free Offset : 이 페이지의 free인 위치를 가르킨다. 이 위치에서
 *                    새로운 slot을 만들고 이의 주소를 반환한다.
 *                    결국 이는 페이지의 last slot과 같은 의미를 가진다.
 *
 *    + Free Size : 이 페이지의 free인 공간을 나타낸다.
 *
 *
 *  # 관련 자료구조
 *
 *    - sdpPageHdr 구조체
 *
 **********************************************************************/

#ifndef _O_SDP_PHY_PAGE_H_
#define _O_SDP_PHY_PAGE_H_ 1

#include <smDef.h>
#include <sdr.h>
#include <sdb.h>
#include <smu.h>
#include <sdpDef.h>
#include <sdpDblPIDList.h>
#include <sdpSglPIDList.h>
#include <sdpSlotDirectory.h>
#include <smrCompareLSN.h>


class sdrMiniTrans;

class sdpPhyPage
{
public:

    // page 할당시 physical header 초기화 및 로깅
    static IDE_RC initialize( sdpPhyPageHdr  *aPageHdr,
                              scPageID        aPageID,
                              sdpParentInfo  *aParentInfo,
                              UShort          aPageState,
                              sdpPageType     aPageType,
                              smOID           aTableOID,
                              UInt            aIndexID  );

    static IDE_RC logAndInit( sdpPhyPageHdr *aPageHdr,
                              scPageID       aPageID,
                              sdpParentInfo *aParentInfo,
                              UShort         aPageState,
                              sdpPageType    aPageType,
                              smOID          aTableOID,
                              UInt           aIndexID,
                              sdrMtx        *aMtx );

    static IDE_RC create( idvSQL        *aStatistics,
                          scSpaceID      aSpaceID,
                          scPageID       aPageID,
                          sdpParentInfo *aParentInfo,
                          UShort         aPageState,
                          sdpPageType    aPageType,
                          smOID          aTableOID,
                          UInt           aIndexID,
                          sdrMtx        *aCrtMtx,
                          sdrMtx        *aInitMtx,
                          UChar        **aPagePtr);

    static IDE_RC create4DPath( idvSQL        *aStatistics,
                                void          *aTrans,
                                scSpaceID      aSpaceID,
                                scPageID       aPageID,
                                sdpParentInfo *aParentInfo,
                                UShort         aPageState,
                                sdpPageType    aType,
                                smOID          aTableOID,
                                idBool         aIsLogging,
                                UChar        **aPagePtr);

    static IDE_RC setLinkState( sdpPhyPageHdr    * aPageHdr,
                                sdpPageListState   aLinkState,
                                sdrMtx           * aMtx );

    static IDE_RC reset( sdpPhyPageHdr *aPageHdr,
                         UInt           aLogicalHdrSize,
                         sdrMtx        *aMtx );

    static IDE_RC logAndInitLogicalHdr( sdpPhyPageHdr  *aPageHdr,
                                        UInt            aSize,
                                        sdrMtx         *aMtx,
                                        UChar         **aLogicalHdrStartPtr );

    /* logcial page header를 저장하면서 physical page의 header의
     * total free size와 free offset을 logical 하게 처리한다. */
    static UChar* initLogicalHdr( sdpPhyPageHdr  *aPageHdr,
                                  UInt            aSize );

    static inline IDE_RC setIdx( sdpPhyPageHdr * aPageHdr,
                                 UChar           aIdx,
                                 sdrMtx        * aMtx );

    /* slot이 할당 가능한가. */
    static idBool canAllocSlot( sdpPhyPageHdr *aPageHdr,
                                UInt           aSlotSize,
                                idBool         aNeedAllocSlotEntry,
                                UShort         aSlotAlignValue );

    static IDE_RC allocSlot4SID( sdpPhyPageHdr       *aPageHdr,
                                 UShort               aSlotSize,
                                 UChar              **aAllocSlotPtr,
                                 scSlotNum           *aAllocSlotNum,
                                 scOffset            *aAllocSlotOffset );

    static IDE_RC allocSlot( sdpPhyPageHdr       *aPageHdr,
                             scSlotNum            aSlotNum,
                             UShort               aSaveSize,
                             idBool               aNeedAllocSlotEntry,
                             UShort              *aAllowedSize,
                             UChar              **aAllocSlotPtr,
                             scOffset            *aAllocSlotOffset,
                             UChar                aSlotAlignValue );

    static IDE_RC freeSlot4SID( sdpPhyPageHdr   *aPageHdr,
                                UChar           *aSlotPtr,
                                scSlotNum        aSlotNum,
                                UShort           aSlotSize );

    static IDE_RC freeSlot( sdpPhyPageHdr   *aPageHdr,
                            UChar           *aSlotPtr,
                            UShort           aFreeSize,
                            idBool           aNeedFreeSlotEntry,
                            UChar            aSlotAlignValue );

    static IDE_RC freeSlot( sdpPhyPageHdr   *aPageHdr,
                            scSlotNum        aSlotNum,
                            UShort           aFreeSize,
                            UChar            aSlotAlignValue );

    static IDE_RC compactPage( sdpPhyPageHdr         *aPageHdr,
                               sdpGetSlotSizeFunc     aGetSlotSizeFunc );

    static idBool isPageCorrupted( UChar      *aStartPtr );
    static idBool checkAndSetPageCorrupted( scSpaceID  aSpaceID,
                                            UChar      *aStartPtr );
    //TASK-4007 [SM]PBT를 위한 기능 추가
    //페이지를 순수 Hex코드로 뿌려준다.
    static IDE_RC dumpHdr   ( const UChar *sPage ,
                              SChar       *aOutBuf ,
                              UInt         aOutSize );

    static IDE_RC tracePage( UInt           aChkFlag,
                             ideLogModule   aModule,
                             UInt           aLevel,
                             const UChar  * aPage,
                             const SChar  * aTitle,
                             ... );

    static IDE_RC tracePageInternal( UInt           aChkFlag,
                                     ideLogModule   aModule,
                                     UInt           aLevel,
                                     const UChar  * aPage,
                                     const SChar  * aTitle,
                                     va_list        ap );

    /* BUG-32528 disk page header의 BCB Pointer 가 긁혔을 경우에 대한
     * 디버깅 정보 추가. */
    static IDE_RC writeToPage( UChar * aDestPagePtr,
                               UChar * aSrcPtr,
                               UInt    aLength );

    static idBool validate( sdpPhyPageHdr *aPageHdr );

    static IDE_RC verify( sdpPhyPageHdr * aPageHdr,
                          scPageID        aSpaceID,
                          scPageID        aPageID );

    // BUG-17930 : log없이 page를 생성할 때(nologging index build, DPath insert),
    // operation이 시작 후 완료되기 전까지는 page의 physical header에 있는
    // mIsConsistent를 F로 표시하고, operation이 완료된 후 mIsConsistent를
    // T로 표시하여 server failure 후 recovery될 때 redo를 수행하도록 해야 함
    static IDE_RC setPageConsistency( sdrMtx        *aMtx,
                                      sdpPhyPageHdr *aPageHdr,
                                      UChar         *aIsConsistent );

    /* PROJ-2162 RestartRiskReduction
     * Page를 Inconsistent하다고 설정함 */
    static IDE_RC setPageInconsistency( scSpaceID       aSpaceID,
                                        scPageID        aPID );

    /* BUG-45598 Page를 Inconsistent하다고 설정 ( 페이지 헤더 이용, 운영중 ) */
    static IDE_RC setPageInconsistency( sdpPhyPageHdr   *aPageHdr );

    // PROJ-1665 : Page의 consistent 상태 여부 반환
    static inline idBool isConsistentPage( UChar * aPageHdr );

    static idBool isSlotDirBasedPage( sdpPhyPageHdr    *aPageHdr );

    static inline void makeInitPageInfo( sdrInitPageInfo * aInitInfo,
                                         scPageID          aPageID,
                                         sdpParentInfo   * aParentInfo,
                                         UShort            aPageState,
                                         sdpPageType       aPageType,
                                         smOID             aTableOID,
                                         UInt              aIndexID );

/* inline function */
public:

    /* 페이지의 page id 반환 */
    inline static scPageID  getPageID( UChar*  aStartPtr );

    /* 페이지내의 임의의 Pointer에서 page id 반환 */
    inline static scPageID getPageIDFromPtr( void*  aPtr );

    /* 페이지의 space id 반환*/
    inline static scSpaceID  getSpaceID( UChar* aStartPtr );

    /* 페이지의 page double linked list node 반환 */
    inline static sdpDblPIDListNode* getDblPIDListNode( sdpPhyPageHdr* aPageHdr );

    /* 페이지의 page double linked prev page id 반환 */
    inline static scPageID  getPrvPIDOfDblList( sdpPhyPageHdr* aPageHdr );

    /* 페이지의 page double linked next page id 반환 */
    inline static scPageID  getNxtPIDOfDblList( sdpPhyPageHdr* aPageHdr );

    /* 페이지의 page single linked list node 반환 */
    inline static sdpSglPIDListNode* getSglPIDListNode( sdpPhyPageHdr* aPageHdr );

    /* 페이지의 single linked list next page id 반환 */
    inline static scPageID getNxtPIDOfSglList( sdpPhyPageHdr* aPageHdr );

    /* 페이지의 pageLSN 얻기 */
    inline static smLSN getPageLSN( UChar* aStartPtr );

    /* 페이지의 pageLSN 설정 */
    inline static void setPageLSN( UChar      * aPageHdr,
                                   smLSN      * aPageLSN );

    /* 페이지의 checksum 얻기*/
    inline static UInt getCheckSum( sdpPhyPageHdr* aPageHdr );

    inline static void calcAndSetCheckSum( UChar *aPageHdr );

    /* 페이지의 Parent Node 얻기 */
    inline static sdpParentInfo getParentInfo( UChar * aStartPtr );

    /* smo number를 get한다.**/
    inline static ULong getIndexSMONo( sdpPhyPageHdr * aPageHdr );

    /* 페이지의 index smo no를 set한다. */
    inline static void setIndexSMONo( UChar *aStartPtr, ULong aValue );
    inline static void setIndexSMONo( sdpPhyPageHdr *aPageHdr, ULong aValue );

    /* page 상태를 get한다.*/
    inline static UShort getState( sdpPhyPageHdr * aPageHdr );

    /*  페이지의 page state를 set한다.
     * - SDR_2BYTES 로깅 */
    inline static IDE_RC setState( sdpPhyPageHdr   *aPageHdr,
                                   UShort           aValue,
                                   sdrMtx          *aMtx );

    /* 페이지의 Type을 set한다.
     * - SDR_2BYTES 로깅 */
    inline static IDE_RC setPageType( sdpPhyPageHdr   *aPageHdr,
                                      UShort           aValue,
                                      sdrMtx          *aMtx );

    /* PROJ-2037 TMS: Segment에서 Page의 SeqNo를 설정한다. */
    inline static IDE_RC setSeqNo( sdpPhyPageHdr *aPageHdr,
                                   ULong          aValue,
                                   sdrMtx         *aMtx );

    /* Page Type을  Get한다.*/
    inline static sdpPageType getPageType( sdpPhyPageHdr *aPageHdr );

    /* BUG-32091 [sm_collection] add TableOID in PageHeader
     * TableOID를 Get한다.*/
    inline static smOID getTableOID( UChar * aPageHdr );

    /* 하위 레이어를 위한 함수로써,페이지 포인터에 해당하는 페이지 타입을
     * 얻는다.  To fix BUG-13462 */
    inline static UInt getPhyPageType( UChar *aPagePtr );

    /* 하위 레이어를 위한 함수로써,지원되는 페이지 타입의 개수를 얻는다.
     * To fix BUG-13462 */
    inline static UInt getPhyPageTypeCount( );

    /* offset으로부터 page pointer를 구한다. */
    inline static UChar* getPagePtrFromOffset( UChar       *aPagePtr,
                                               scOffset     aOffset );

    /* page pointer로부터 offset을 구한다. */
    inline static scOffset getOffsetFromPagePtr( UChar *aPagePtr );

    inline static scOffset getFreeSpaceBeginOffset( sdpPhyPageHdr *aPageHdr );

    inline static scOffset getFreeSpaceEndOffset( sdpPhyPageHdr *aPageHdr );

    /* free size을 get한다. */
    inline static UInt getTotalFreeSize( sdpPhyPageHdr *aPageHdr );

    /* 페이지의 free size를 set한다. */
    /* not used
    inline static void setTotalFreeSize( sdpPhyPageHdr *aPageHdr,
                                         UShort         aValue );
    */
    inline static UShort getAvailableFreeSize( sdpPhyPageHdr *aPageHdr );

    inline static void setAvailableFreeSize( sdpPhyPageHdr *aPageHdr,
                                             UShort         aValue );

    /* page empty 일때 빈 공간을 계산한다. */
    inline static UInt getEmptyPageFreeSize();

    /* Fragment가 발생하지 않은 공간의 사이즈를 얻음 */
    inline static UShort getNonFragFreeSize( sdpPhyPageHdr *aPageHdr );
    
    /* Fragment가 발생하지 않은 공간의 사이즈를 얻음 */
    inline static UShort getAllocSlotOffset( sdpPhyPageHdr * aPageHdr,
                                             UShort          aAlignedSaveSize );

    /* 한 페이지에서 logical hdr가 시작하는 부분을 return */
    inline static UChar* getLogicalHdrStartPtr( UChar   *aPagePtr );

    /* 한 페이지에서 CTL이 시작하는 부분을 return */
    inline static UChar* getCTLStartPtr( UChar   *aPagePtr );

    /* 한 페이지에서 slot directory가 시작하는 부분을 return */
    inline static UChar* getSlotDirStartPtr( UChar   *aPagePtr );

    /* 한 페이지에서 slot directory가 끝나는 부분을 return */
    inline static UChar* getSlotDirEndPtr( sdpPhyPageHdr    *aPhyPageHdr );

    /* 한 페이지에서 page footer가 시작하는 부분을 return */
    inline static UChar* getPageFooterStartPtr( UChar    *aPagePtr );

    /* 한 페이지에서 physical hdr + logical hdr를 제외한
     * data가 실제 저장되는 시작 offset */
    inline static UInt getDataStartOffset( UInt aLogicalHdrSize );

    /* 한 페이지내에 포인터가 주어졌을때 페이지의 시작점을 구한다.
       page내의 특정 pointer  */
    inline static UChar* getPageStartPtr( void *aPagePtr );

    /* page가 temp table type인지 검사한다. */
#if 0  // not used
    inline static idBool isPageTempType( UChar *aStartPtr );
#endif
    /* page가 TSS table type인지 검사한다. */
    inline static idBool isPageTSSType( UChar *aStartPtr );

    /* page가 index type인지 검사한다. */
    inline static idBool isPageIndexType( UChar *aStartPtr );

    /* page가 index 또는 IndexMeta type인지 검사한다. */
    inline static idBool isPageIndexOrIndexMetaType( UChar *aStartPtr );

    /* 한 페이지내에 포인터가 주어졌을때 Phy page hdr를 구한다.
     * page내의 특정 pointer */
    inline static sdpPhyPageHdr* getHdr( UChar *aPagePtr );

    /* physical hdr의 크기를 구한다. */
    inline static UShort getPhyHdrSize();

    /* logical hdr의 크기를 구한다. */
    inline static UShort getLogicalHdrSize( sdpPhyPageHdr *aPageHdr);

    /* CTL 의 크기를 구한다. */
    inline static UShort getSizeOfCTL( sdpPhyPageHdr *aPageHdr );
    
    /* Footer 의 크기를 구한다. */
    inline static UShort getSizeOfFooter();

    /* page pointer로부터 RID를 얻는다. */
    inline static sdRID getRIDFromPtr( void *aPtr );

    /* page pointer로부터 SID를 얻는다. */
    /* not used 
    inline static sdRID getSIDFromPtr( void *aPtr );
    */
    /* page에 하나의 slot entry를 저장할 공간이 있는 지 검사한다. */
    inline static idBool canMakeSlotEntry( sdpPhyPageHdr *aPageHdr );

    /* page가 free 가능한 페이지 인지 검사한다. */
/* not used
    inline static idBool canPageFree( sdpPhyPageHdr *aPageHdr );
*/
    /* PROJ-2037 TMS: Segment에서 Page의 SeqNo를 얻는다. */
    inline static ULong getSeqNo( sdpPhyPageHdr *aPageHdr );

    // PROJ-1568 Buffer Manager Renewal
    // sdb에서 특정 페이지타입 번호가 undo page type인지
    // 알필요가 있다. 이는 통계정보 구축시 사용된다.
    static UInt getUndoPageType();

    static void resetIndexSMONo( sdpPhyPageHdr * aPageHdr,
                                 scSpaceID       aSpaceID,
                                 idBool          aChkOnlineTBS );

    // PROJ-1704 Disk MVCC 리뉴얼
    static void initCTL( sdpPhyPageHdr  * aPageHdr,
                         UInt             aHdrSize,
                         UChar         ** aHdrStartPtr );

    static IDE_RC extendCTL( sdpPhyPageHdr * aPageHdr,
                             UInt            aSlotCnt,
                             UInt            aSlotSize,
                             UChar        ** aNewSlotStartPtr,
                             idBool        * aTrySuccess );

    static void shiftSlotDirToBottom(sdpPhyPageHdr     *aPageHdr,
                                     UShort             aShiftSize);

    inline static IDE_RC logAndSetAvailableFreeSize( sdpPhyPageHdr *aPageHdr,
                                                     UShort         aValue,
                                                     sdrMtx        *aMtx );

    inline static idBool isValidPageType( scSpaceID aSpaceID,
                                          scPageID  aPageID,
                                          UInt      aPageType );

    /* checksum을 계산 */
    inline static UInt calcCheckSum( sdpPhyPageHdr *aPageHdr );
    
    inline static UInt getSizeOfSlotDir( sdpPhyPageHdr *aPageHdr );

private:


    inline static IDE_RC logAndSetFreeSpaceBeginOffset( sdpPhyPageHdr  *aPageHdr,
                                                        scOffset        aValue,
                                                        sdrMtx         *aMtx );

    inline static IDE_RC logAndSetLogicalHdrSize( sdpPhyPageHdr  *aPageHdr,
                                                  UShort          aValue,
                                                  sdrMtx         *aMtx );

    inline static IDE_RC logAndSetTotalFreeSize( sdpPhyPageHdr *aPageHdr,
                                                 UShort         aValue,
                                                 sdrMtx        *aMtx );

    static IDE_RC allocSlotSpace( sdpPhyPageHdr       *aPageHdr,
                                  UShort               aSaveSize,
                                  UChar              **aAllocSlotPtr,
                                  scOffset            *aAllocSlotOffset );

    static void freeSlotSpace( sdpPhyPageHdr        *aPageHdr,
                               scOffset              aSlotOffset,
                               UShort                aFreeSize);

};

inline UInt sdpPhyPage::getUndoPageType()
{
    return SDP_PAGE_UNDO;
}


/* 페이지의 page id 반환 */
inline scPageID  sdpPhyPage::getPageID( UChar*  aStartPtr )
{
    sdpPhyPageHdr *sPageHdr;

    IDE_DASSERT( aStartPtr == getPageStartPtr(aStartPtr));

    sPageHdr = getHdr(aStartPtr);

    IDE_DASSERT( validate(sPageHdr) == ID_TRUE );

    return ( sPageHdr->mPageID );

}

/* 페이지내의 임의의 Pointer에서 page id 반환 */
inline scPageID sdpPhyPage::getPageIDFromPtr( void*  aPtr )
{
    sdpPhyPageHdr *sPageHdr;
    UChar         *sStartPtr;

    sStartPtr = getPageStartPtr( (UChar*)aPtr );
    sPageHdr  = getHdr( sStartPtr );

    return sPageHdr->mPageID;
}

/* 페이지의 space id 반환*/
inline  scSpaceID  sdpPhyPage::getSpaceID( UChar*   aStartPtr )
{
    sdpPhyPageHdr *sPageHdr;

    IDE_DASSERT( aStartPtr == getPageStartPtr(aStartPtr));

    sPageHdr = getHdr(aStartPtr);

    return ( sPageHdr->mFrameHdr.mSpaceID );
}

/* 페이지의 page double linked list node 반환 */
inline  sdpDblPIDListNode* sdpPhyPage::getDblPIDListNode( sdpPhyPageHdr* aPageHdr )
{
    return &( aPageHdr->mListNode );
}

/* 페이지의 page double linked prev page id 반환 */
inline  scPageID  sdpPhyPage::getPrvPIDOfDblList( sdpPhyPageHdr* aPageHdr )
{
    return ( sdpDblPIDList::getPrvOfNode( getDblPIDListNode( aPageHdr ) ) );
}

/* 페이지의 page double linked next page id 반환 */
inline  scPageID  sdpPhyPage::getNxtPIDOfDblList( sdpPhyPageHdr* aPageHdr )
{
    return sdpDblPIDList::getNxtOfNode( getDblPIDListNode( aPageHdr ) );
}

/* 페이지의 page single linked list node 반환 */
inline  sdpSglPIDListNode* sdpPhyPage::getSglPIDListNode( sdpPhyPageHdr* aPageHdr )
{
    return (sdpSglPIDListNode*)&( aPageHdr->mListNode );
}

/* 페이지의 single linked list next page id 반환 */
inline scPageID sdpPhyPage::getNxtPIDOfSglList( sdpPhyPageHdr* aPageHdr )
{
    return sdpSglPIDList::getNxtOfNode( getSglPIDListNode( aPageHdr ) );
}

/* 페이지의 pageLSN 얻기 */
inline smLSN sdpPhyPage::getPageLSN( UChar* aStartPtr )
{
    IDE_DASSERT( aStartPtr == getPageStartPtr(aStartPtr));
    return ( getHdr(aStartPtr)->mFrameHdr.mPageLSN );
}

/* 페이지의 pageLSN 설정
 * - 버퍼매니저가 flush하기 전에 설정
 * - logging 하지 않음 */
inline void sdpPhyPage::setPageLSN( UChar   * aPageHdr,
                                    smLSN   * aPageLSN )
{
    sdpPhyPageHdr   * sPageHdr = (sdpPhyPageHdr*)aPageHdr;
    sdpPageFooter   * sPageFooter;

    IDE_DASSERT( aPageHdr == getPageStartPtr(aPageHdr) );

    SM_GET_LSN( sPageHdr->mFrameHdr.mPageLSN, *aPageLSN );

    sPageFooter = (sdpPageFooter*)getPageFooterStartPtr(aPageHdr);

    SM_GET_LSN( sPageFooter->mPageLSN, *aPageLSN );

    IDE_DASSERT( validate(sPageHdr) == ID_TRUE );
}

/* 페이지의 checksum 얻기*/
inline UInt sdpPhyPage::getCheckSum( sdpPhyPageHdr* aPageHdr )
{
    return ( aPageHdr->mFrameHdr.mCheckSum );
}

/* 페이지의 Leaf 정보 얻기 */
inline sdpParentInfo sdpPhyPage::getParentInfo( UChar * aStartPtr )
{

    sdpPhyPageHdr *sPageHdr;

    IDE_DASSERT( aStartPtr == getPageStartPtr(aStartPtr));

    sPageHdr = getHdr(aStartPtr);

    return (sPageHdr->mParentInfo);
}

/* smo number를 get한다.**/
inline ULong sdpPhyPage::getIndexSMONo( sdpPhyPageHdr * aPageHdr )
{
    return ( aPageHdr->mFrameHdr.mIndexSMONo );
}

/* 페이지의 index smo no를 set한다. */
inline void sdpPhyPage::setIndexSMONo( UChar *aStartPtr, ULong aValue )
{
    sdpPhyPageHdr *sPageHdr;
    IDE_DASSERT( aStartPtr == getPageStartPtr(aStartPtr) );

    sPageHdr = getHdr(aStartPtr);

    IDE_DASSERT( validate(sPageHdr) == ID_TRUE );
    setIndexSMONo( sPageHdr, aValue );
}

inline void sdpPhyPage::setIndexSMONo( sdpPhyPageHdr *aPageHdr, ULong aValue )
{
    IDE_DASSERT( (UChar*)aPageHdr == getPageStartPtr((UChar*)aPageHdr) );
    aPageHdr->mFrameHdr.mIndexSMONo = aValue;
}
/* page 상태를 get한다.*/
inline UShort sdpPhyPage::getState( sdpPhyPageHdr * aPageHdr )
{
    return aPageHdr->mPageState;
}

/*  페이지의 page state를 set한다.
 * - SDR_2BYTES 로깅 */
inline IDE_RC sdpPhyPage::setState( sdpPhyPageHdr   *aPageHdr,
                                    UShort           aValue,
                                    sdrMtx          *aMtx )
{
    UShort sPageState = (UShort)aValue;

    IDE_DASSERT( aMtx != NULL );
    IDE_DASSERT( validate(aPageHdr) == ID_TRUE );

    IDE_TEST( sdrMiniTrans::writeNBytes( aMtx,
                                         (UChar*)&aPageHdr->mPageState,
                                         &sPageState,
                                         ID_SIZEOF(sPageState) )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

inline IDE_RC sdpPhyPage::setPageType( sdpPhyPageHdr   *aPageHdr,
                                       UShort           aValue,
                                       sdrMtx          *aMtx )
{
    UShort sPageType = (UShort)aValue;

    IDE_DASSERT( aMtx != NULL );
    IDE_DASSERT( validate(aPageHdr) == ID_TRUE );

    IDE_TEST( sdrMiniTrans::writeNBytes( aMtx,
                                         (UChar*)&aPageHdr->mPageType,
                                         &sPageType,
                                         ID_SIZEOF(sPageType) )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

inline IDE_RC sdpPhyPage::setSeqNo( sdpPhyPageHdr *aPageHdr,
                                    ULong          aValue,
                                    sdrMtx         *aMtx )
{
    ULong   sSeqNo = (ULong)aValue;

    IDE_DASSERT( aMtx != NULL );
    IDE_DASSERT( validate(aPageHdr) == ID_TRUE );

    IDE_TEST( sdrMiniTrans::writeNBytes( aMtx,
                                         (UChar*)&aPageHdr->mSeqNo,
                                         &sSeqNo,
                                         ID_SIZEOF(sSeqNo) )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* Page Type을  Get한다.*/
inline sdpPageType sdpPhyPage::getPageType( sdpPhyPageHdr *aPageHdr )
{
    return (sdpPageType)(aPageHdr->mPageType);
}
/* BUG-32091 [sm_collection] add TableOID in PageHeader
 * TableOID를 Get한다.*/
inline smOID sdpPhyPage::getTableOID( UChar * aPageHdr )
{
    return (smOID)(((sdpPhyPageHdr *)aPageHdr)->mTableOID);
}

/* 하위 레이어를 위한 함수로써,페이지 포인터에 해당하는 페이지 타입을
 * 얻는다.  To fix BUG-13462 */
inline UInt sdpPhyPage::getPhyPageType( UChar *aPagePtr )
{
    return (UInt)getPageType( (sdpPhyPageHdr*)aPagePtr );
}

/* 하위 레이어를 위한 함수로써,지원되는 페이지 타입의 개수를 얻는다.
 * To fix BUG-13462 */
inline UInt sdpPhyPage::getPhyPageTypeCount( )
{
    return SDP_PAGE_TYPE_MAX;
}

/* offset으로부터 page pointer를 구한다. */
inline UChar* sdpPhyPage::getPagePtrFromOffset( UChar       *aPagePtr,
                                                scOffset     aOffset )
{
    UChar * sPagePtrFromOffset;

    IDE_DASSERT( aPagePtr != NULL );

    sPagePtrFromOffset = ( getPageStartPtr(aPagePtr) + aOffset );

    /* TASK-6105 함수 inline화를 위해 sdpPhyPage::getPagePtrFromOffset 함수를
     * sdpSlotDirectory::getPagePtrFromOffset로 중복으로 추가한다. 
     * 한 함수가 수정되면 나머지 함수도 수정해야 한다.*/
    IDE_DASSERT( sdpSlotDirectory::getPagePtrFromOffset(aPagePtr, aOffset) ==
                 sPagePtrFromOffset );

    return sPagePtrFromOffset;
}

/* page pointer로부터 offset을 구한다. */
inline scOffset sdpPhyPage::getOffsetFromPagePtr( UChar *aPagePtr )
{
    IDE_DASSERT( aPagePtr != NULL );
    return ( aPagePtr - getPageStartPtr(aPagePtr) );
}

inline scOffset sdpPhyPage::getFreeSpaceBeginOffset( sdpPhyPageHdr *aPageHdr )
{
    return ( aPageHdr->mFreeSpaceBeginOffset );
}

inline scOffset sdpPhyPage::getFreeSpaceEndOffset( sdpPhyPageHdr *aPageHdr )
{
    return ( aPageHdr->mFreeSpaceEndOffset );
}

inline IDE_RC sdpPhyPage::logAndSetFreeSpaceBeginOffset( 
                                sdpPhyPageHdr  *aPageHdr,
                                scOffset        aValue,
                                sdrMtx         *aMtx )
{
    UShort sFreeOffset = (UShort)aValue;

    IDE_DASSERT( aMtx != NULL );
    IDE_DASSERT( validate(aPageHdr) == ID_TRUE );

    IDE_TEST( sdrMiniTrans::writeNBytes( 
                            aMtx,
                            (UChar*)&aPageHdr->mFreeSpaceBeginOffset,
                            &sFreeOffset,
                            ID_SIZEOF(sFreeOffset) )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

inline IDE_RC sdpPhyPage::logAndSetLogicalHdrSize( 
                                   sdpPhyPageHdr  *aPageHdr,
                                   UShort          aValue,
                                   sdrMtx         *aMtx )
{
    UShort sSize = (UShort)aValue;

    IDE_DASSERT( aMtx != NULL );
    IDE_DASSERT( validate(aPageHdr) == ID_TRUE );

    IDE_TEST( sdrMiniTrans::writeNBytes( 
                                 aMtx,
                                 (UChar*)&aPageHdr->mLogicalHdrSize,
                                 &sSize,
                                 ID_SIZEOF(sSize) )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


inline IDE_RC sdpPhyPage::logAndSetTotalFreeSize( 
                                sdpPhyPageHdr *aPageHdr,
                                UShort         aValue,
                                sdrMtx        *aMtx )
{
    IDE_DASSERT( aMtx != NULL );
    IDE_DASSERT( validate(aPageHdr) == ID_TRUE );

    IDE_TEST( sdrMiniTrans::writeNBytes( 
                                 aMtx,
                                 (UChar*)&aPageHdr->mTotalFreeSize,
                                 &aValue,
                                 ID_SIZEOF(aValue) )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}



inline IDE_RC sdpPhyPage::logAndSetAvailableFreeSize( 
                                sdpPhyPageHdr *aPageHdr,
                                UShort         aValue,
                                sdrMtx        *aMtx )
{
    IDE_DASSERT( aMtx != NULL );
    IDE_DASSERT( validate(aPageHdr) == ID_TRUE );

    IDE_TEST( sdrMiniTrans::writeNBytes( aMtx,
                                         (UChar*)&aPageHdr->mAvailableFreeSize,
                                         &aValue,
                                         ID_SIZEOF(aValue) )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/* free size을 get한다. */
inline UInt sdpPhyPage::getTotalFreeSize( sdpPhyPageHdr *aPageHdr )
{
    return ( aPageHdr->mTotalFreeSize );
}

inline UShort sdpPhyPage::getAvailableFreeSize( sdpPhyPageHdr *aPageHdr )
{
    return ( aPageHdr->mAvailableFreeSize );
}

inline void sdpPhyPage::setAvailableFreeSize( sdpPhyPageHdr *aPageHdr,
                                              UShort         aValue )
{
    IDE_DASSERT( validate(aPageHdr) == ID_TRUE );

    IDE_DASSERT( aValue <= (SD_PAGE_SIZE - 1) );

    aPageHdr->mAvailableFreeSize = aValue;
}

/* page empty 일때 빈 공간을 계산한다. */
inline UInt sdpPhyPage::getEmptyPageFreeSize()
{
    return ( SD_PAGE_SIZE -
             ( getPhyHdrSize() + ID_SIZEOF(sdpPageFooter) ) );
}

/******************************************************************************
 * Description :
 *    CheckSum 방식에 따라 CheckSum을 설정한다.
 *
 *  aPageHdr  - [IN] Page 시작 포인터
 *
 ******************************************************************************/
inline void sdpPhyPage::calcAndSetCheckSum( UChar *aPageHdr )
{
    sdpPhyPageHdr   * sPageHdr;
    sdpPageFooter   * sPageFooter;

    IDE_DASSERT( aPageHdr == getPageStartPtr(aPageHdr) );

    sPageHdr = getHdr( aPageHdr );
    IDE_DASSERT( validate(sPageHdr) == ID_TRUE );

    if( smuProperty::getCheckSumMethod() == SDP_CHECK_CHECKSUM )
    {
        sPageHdr->mFrameHdr.mCheckSum = calcCheckSum(sPageHdr);
    }
    else
    {
        sPageFooter = (sdpPageFooter*)getPageFooterStartPtr( aPageHdr );

        IDE_ASSERT( smrCompareLSN::isEQ( &sPageHdr->mFrameHdr.mPageLSN,
                                         &sPageFooter->mPageLSN ) 
                    == ID_TRUE );
        
        // BUG-45598: LSN을 사용하면 checksum 값은 0으로 초기화 
        sPageHdr->mFrameHdr.mCheckSum = SDP_CHECKSUM_INIT_VAL;
    }
}

/* Fragment가 발생하지 않은 공간의 사이즈를 얻음 */
inline UShort sdpPhyPage::getNonFragFreeSize( sdpPhyPageHdr *aPageHdr )
{
    return (aPageHdr->mFreeSpaceEndOffset - aPageHdr->mFreeSpaceBeginOffset);
}

/* Fragment가 발생하지 않은 공간의 사이즈를 얻음 */
inline UShort sdpPhyPage::getAllocSlotOffset( sdpPhyPageHdr * aPageHdr,
                                              UShort          aAlignedSaveSize)
{
    return (aPageHdr->mFreeSpaceEndOffset - aAlignedSaveSize);
}

/* 한 페이지에서 logical hdr가 시작하는 부분을 return */
inline UChar* sdpPhyPage::getLogicalHdrStartPtr( UChar   *aPagePtr )
{
    IDE_ASSERT( aPagePtr != NULL );

    return ( getPageStartPtr(aPagePtr) +
             getPhyHdrSize() );
}

/* 한 페이지에서 logical hdr가 시작하는 부분을 return */
inline UChar* sdpPhyPage::getCTLStartPtr( UChar   *aPagePtr )
{
    IDE_DASSERT( aPagePtr != NULL );
    IDE_DASSERT( aPagePtr == getPageStartPtr( aPagePtr ) );

    return ( aPagePtr +
             getPhyHdrSize() +
             getLogicalHdrSize( (sdpPhyPageHdr*)aPagePtr ) );
}


/* 한 페이지에서 slot directory가 시작하는 부분을 return */
inline UChar* sdpPhyPage::getSlotDirStartPtr( UChar   *aPagePtr )
{
    UChar    *sStartPtr;

    IDE_DASSERT( aPagePtr != NULL );

    sStartPtr = getPageStartPtr(aPagePtr);

    return ( sStartPtr +
             getPhyHdrSize() +
             getLogicalHdrSize( (sdpPhyPageHdr*)sStartPtr ) +
             getSizeOfCTL( (sdpPhyPageHdr*)sStartPtr ) );
}


/***********************************************************************
 *
 * Description :
 *  slot directory의 끝위치(end point)를 return.
 *
 *  aPhyPageHdr - [IN] physical page header
 *
 **********************************************************************/
inline UChar* sdpPhyPage::getSlotDirEndPtr( sdpPhyPageHdr    *aPhyPageHdr )
{
    IDE_DASSERT( aPhyPageHdr != NULL );

    return sdpPhyPage::getPagePtrFromOffset((UChar*)aPhyPageHdr,
                                            aPhyPageHdr->mFreeSpaceBeginOffset);
}


/* 한 페이지에서 page footer가 시작하는 부분을 return */
inline UChar* sdpPhyPage::getPageFooterStartPtr( UChar    *aPagePtr )
{
/***********************************************************************
 *
 * Description :
 * +1하여 align 하므로 페이지의 첫 주소에 대해 페이지의 마지막 주소로
 * align된다.
 * 주의 : 만약 페이지의 마지막 주소를 넘기면 그 다음 페이지의 첫
 * 주소로 align 되므로 이런 경우가 생겨서는 안된다.
 *
 **********************************************************************/

    UChar *sPageLastPtr;

    IDE_DASSERT( aPagePtr != NULL );

    sPageLastPtr = (UChar *)idlOS::align( aPagePtr + 1, SD_PAGE_SIZE );

    return ( sPageLastPtr - ID_SIZEOF(sdpPageFooter) );
}


/* 한 페이지에서 physical hdr + logical hdr를 제외한
 * data가 실제 저장되는 시작 offset */
inline UInt sdpPhyPage::getDataStartOffset( UInt aLogicalHdrSize )
{
    return ( getPhyHdrSize() +
             idlOS::align8( aLogicalHdrSize ) );
}

/* 한 페이지내에 포인터가 주어졌을때 페이지의 시작점을 구한다. */
inline UChar* sdpPhyPage::getPageStartPtr( void   *aPagePtr ) // page내의 특정 pointer
{
    UChar * sPageStartPtr;

    IDE_DASSERT( aPagePtr != NULL );

    sPageStartPtr = (UChar *)idlOS::alignDown( (void*)aPagePtr, (UInt)SD_PAGE_SIZE );

    /* TASK-6105 함수 inline화를 위해 sdpPhyPage::getPageStartPtr 함수를
     * sdpSlotDirectory::getPageStartPtr로 중복으로 추가한다.
     * 한 함수가 수정되면 나머지 함수도 수정해야 한다.*/  
    IDE_DASSERT( sdpSlotDirectory::getPageStartPtr( aPagePtr ) == 
                 sPageStartPtr );

    return sPageStartPtr;
}

/* page가 temp table type인지 검사한다. */
#if 0  // not used
inline idBool sdpPhyPage::isPageTempType( UChar   *aStartPtr )
{
    sdpPhyPageHdr *sPageHdr;
    sdpPageType    sPageType;

    sPageHdr  = getHdr(aStartPtr);
    sPageType = getPageType(sPageHdr);

    if( ( sPageType == SDP_PAGE_TEMP_TABLE_DATA ) ||
        ( sPageType == SDP_PAGE_TEMP_TABLE_META ) )
    {
        return ID_TRUE;
    }
    else
    {
        return ID_FALSE;
    }
}
#endif

/* page가 index type인지 검사한다. */
inline idBool sdpPhyPage::isPageIndexType( UChar   *aStartPtr )
{
    sdpPhyPageHdr *sPageHdr;
    sPageHdr = getHdr(aStartPtr);
    return ( ( getPageType(sPageHdr) == SDP_PAGE_INDEX_BTREE ) ||
             ( getPageType(sPageHdr) == SDP_PAGE_INDEX_RTREE ) ? 
             ID_TRUE : ID_FALSE );
}

/* page가 index 또는 index meta type인지 검사한다. */
inline idBool sdpPhyPage::isPageIndexOrIndexMetaType( UChar   *aStartPtr )
{
    sdpPhyPageHdr *sPageHdr;
    sPageHdr = getHdr(aStartPtr);
    return ( ( getPageType(sPageHdr) == SDP_PAGE_INDEX_BTREE ) ||
             ( getPageType(sPageHdr) == SDP_PAGE_INDEX_RTREE ) ||
             ( getPageType(sPageHdr) == SDP_PAGE_INDEX_META_BTREE ) ||
             ( getPageType(sPageHdr) == SDP_PAGE_INDEX_META_RTREE ) ? 
                ID_TRUE : ID_FALSE);
}

/* page가 TSS table type인지 검사한다. */
inline idBool sdpPhyPage::isPageTSSType( UChar *aStartPtr )
{
    sdpPhyPageHdr *sPageHdr;
    sPageHdr = getHdr(aStartPtr);
    return (getPageType(sPageHdr) == SDP_PAGE_TSS ?
            ID_TRUE : ID_FALSE);
}

/* checksum을 계산 */
inline UInt sdpPhyPage::calcCheckSum( sdpPhyPageHdr *aPageHdr )
{
    UChar *sCheckSumStartPtr;
    IDE_DASSERT( validate(aPageHdr) == ID_TRUE );

    sCheckSumStartPtr = (UChar *)(&aPageHdr->mFrameHdr.mCheckSum) +
        ID_SIZEOF(aPageHdr->mFrameHdr.mCheckSum);

    return smuUtility::foldBinary( sCheckSumStartPtr,
                                   SD_PAGE_SIZE -
                                   ID_SIZEOF(aPageHdr->mFrameHdr.mCheckSum) );
}

/* 한 페이지내에 포인터가 주어졌을때 Phy page hdr를 구한다. */
inline sdpPhyPageHdr* sdpPhyPage::getHdr( UChar *aPagePtr ) // page내의 특정 pointer
{
    IDE_DASSERT( aPagePtr != NULL );
    return (sdpPhyPageHdr *)getPageStartPtr(aPagePtr);
}


/* physical hdr의 크기를 구한다. 8에 align 되어야 함.
 * 4로 align 될때 뒤의 헤더를 읽을때 문제가 생길 수 있기 때문이다. */
inline UShort sdpPhyPage::getPhyHdrSize()
{
    return idlOS::align8((UShort)ID_SIZEOF(sdpPhyPageHdr));
}

inline UShort sdpPhyPage::getLogicalHdrSize( sdpPhyPageHdr *aPageHdr )
{
    return (aPageHdr->mLogicalHdrSize);
}

inline UShort sdpPhyPage::getSizeOfCTL( sdpPhyPageHdr *aPageHdr )
{
    return (aPageHdr->mSizeOfCTL);
}

inline UShort sdpPhyPage::getSizeOfFooter()
{
    return ID_SIZEOF(sdpPageFooter);
}

/* page pointer로부터 RID를 얻는다. */
inline sdRID sdpPhyPage::getRIDFromPtr( void *aPtr )
{
    UChar *sPtr =  (UChar*)aPtr;
    UChar *sStartPtr = getPageStartPtr( sPtr );

    IDE_DASSERT( aPtr !=NULL );

    return SD_MAKE_RID( getPageID( sStartPtr ), sPtr - sStartPtr );
}

/* page에 하나의 slot entry를 저장할 공간이 있는 지 검사한다. */
inline idBool sdpPhyPage::canMakeSlotEntry( sdpPhyPageHdr   *aPageHdr )
{

    IDE_DASSERT( validate(aPageHdr) == ID_TRUE );

    if( aPageHdr->mTotalFreeSize >= ID_SIZEOF(sdpSlotEntry) )
    {
        return ID_TRUE;
    }
    else
    {
        return ID_FALSE;
    }
}

inline ULong sdpPhyPage::getSeqNo( sdpPhyPageHdr *aPageHdr )
{
    IDE_DASSERT( validate(aPageHdr) == ID_TRUE );

    return aPageHdr->mSeqNo;
}

inline void sdpPhyPage::makeInitPageInfo( sdrInitPageInfo *aInitPageInfo,
                                          scPageID         aPageID,
                                          sdpParentInfo   *aParentInfo,
                                          UShort           aPageState,
                                          sdpPageType      aPageType,
                                          smOID            aTableOID,
                                          UInt             aIndexID )
{
    aInitPageInfo->mPageID = aPageID;

    if( aParentInfo != NULL )
    {
        aInitPageInfo->mParentPID   = aParentInfo->mParentPID;
        aInitPageInfo->mIdxInParent = aParentInfo->mIdxInParent;
    }
    else
    {
        aInitPageInfo->mParentPID   = SD_NULL_PID;
        aInitPageInfo->mIdxInParent = ID_SSHORT_MAX;
    }

    aInitPageInfo->mPageType    = aPageType;
    aInitPageInfo->mPageState   = aPageState;
    aInitPageInfo->mTableOID    = aTableOID;
    aInitPageInfo->mIndexID     = aIndexID;
}

/***********************************************************************
 *
 * Description :
 *  slot directory를 사용하는 페이지인지 여부를 알려준다.
 *
 *  aPhyPageHdr - [IN] physical page header
 *
 **********************************************************************/
inline idBool sdpPhyPage::isSlotDirBasedPage( sdpPhyPageHdr    *aPageHdr )
{
    if( (aPageHdr->mPageType == SDP_PAGE_INDEX_META_BTREE) ||
        (aPageHdr->mPageType == SDP_PAGE_INDEX_META_RTREE) ||
        (aPageHdr->mPageType == SDP_PAGE_INDEX_BTREE) ||
        (aPageHdr->mPageType == SDP_PAGE_INDEX_RTREE) ||
        (aPageHdr->mPageType == SDP_PAGE_DATA) ||
        (aPageHdr->mPageType == SDP_PAGE_TEMP_TABLE_META) ||
        (aPageHdr->mPageType == SDP_PAGE_TEMP_TABLE_DATA) ||
        (aPageHdr->mPageType == SDP_PAGE_UNDO) )
    {
        return ID_TRUE;
    }
    else
    {
        return ID_FALSE;
    }
}

inline UInt sdpPhyPage::getSizeOfSlotDir( sdpPhyPageHdr *aPageHdr )
{
    UChar     * sSlotDirPtr;
    UShort      sSlotDirSize;

    IDE_DASSERT( validate(aPageHdr) == ID_TRUE );

    sSlotDirPtr = getSlotDirStartPtr((UChar*)aPageHdr);

    sSlotDirSize =  sdpSlotDirectory::getSize(sdpPhyPage::getHdr(sSlotDirPtr));

#ifdef DEBUG
    sdpSlotDirHdr * sSlotDirHdr;
    UInt            sSlotCount;

    sSlotDirHdr = (sdpSlotDirHdr*)sSlotDirPtr;
    sSlotCount = ( sSlotDirSize - ID_SIZEOF(sdpSlotDirHdr) ) 
                      / ID_SIZEOF(sdpSlotEntry);

    IDE_DASSERT( sSlotDirHdr->mSlotEntryCount == sSlotCount );
#endif

    return sSlotDirSize;
}

/********************************************************************
 * Description: aPageType값이 SDP_PAGE_TYPE_MAX보다 크면
 *              Invalid로 ID_FALSE, else ID_TRUE
 *
 * aSpaceID  - [IN] TableSpaceID
 * aPageID   - [IN] PageID
 * aPageType - [IN] PageType
 * *****************************************************************/
inline idBool sdpPhyPage::isValidPageType( scSpaceID aSpaceID,
                                           scPageID  aPageID,
                                           UInt      aPageType )
{
    if ( aPageType >= SDP_PAGE_TYPE_MAX )
    {
        ideLog::log( SM_TRC_LOG_LEVEL_WARNNING,
                     SM_TRC_SDP_INVALID_PAGE_INFO,
                     aSpaceID,
                     aPageID,
                     aPageType );

        return ID_FALSE;
    }
    else
    {
        return ID_TRUE;
    }
}

/***********************************************************************
 * PROJ-1665
 * Description : Consistent Page 여부 결과 반환
 *
 * Implementation :
 *     - In  : Page Header Pointer
 *     - Out : Page consistent 여부
 **********************************************************************/
inline idBool sdpPhyPage::isConsistentPage( UChar * aPageHdr )
{
    sdpPhyPageHdr * sPageHdr;

    sPageHdr = (sdpPhyPageHdr *)aPageHdr;

    if ( sPageHdr->mIsConsistent == SDP_PAGE_CONSISTENT )
    {
        return ID_TRUE;
    }
    else
    {
        return ID_FALSE;
    }
}

#endif // _SDP_PHY_PAGE_H_
