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
 * $Id: smpFreePageList.h 85970 2019-08-01 05:57:45Z justin.kwon $
 **********************************************************************/

#ifndef _O_SMP_FREE_PAGELIST_H_
#define _O_SMP_FREE_PAGELIST_H_ 1

#include <smpDef.h>

class smpFreePageList
{
public:
    // Runtime Item을 NULL로 설정한다.
    static IDE_RC setRuntimeNull( smpPageListEntry* aPageListEntry );

    // PageListEntry에서 FreePage와 관련된 RuntimeEntry에 대한 초기화
    static IDE_RC initEntryAtRuntime( smpPageListEntry* aPageListEntry );

    // RuntimeEntry 제거
    static IDE_RC finEntryAtRuntime( smpPageListEntry* aPageListEntry );

    // PageListEntry에서 RuntimeEntry 정보 해제
    static void   initializeFreePageListAndPool(
                                            smpPageListEntry* aPageListEntry );

    // FreePageList 구축
    static void   distributePagesFromFreePageList0ToTheOthers(
                                            smpPageListEntry* aPageListEntry );

    // FreePagePool 구축
    static IDE_RC distributePagesFromFreePageList0ToFreePagePool(
                                            void*             aTrans,
                                            scSpaceID         aSpaceID,
                                            smpPageListEntry* aPageListEntry );

    // FreePagePool에서 FreePage를 할당받을 수 있는지 시도
    static IDE_RC tryForAllocPagesFromPool( smpPageListEntry* aPageListEntry,
                                            UInt              aPageListID,
                                            idBool*           aIsPageAlloced );
    
    // FreePageList에 FreePage 할당
    // FreePagePool -> FreePageList
    static IDE_RC getPagesFromFreePagePool( smpPageListEntry* aPageListEntry,
                                            UInt              aPageListID );

    // FreePagePool에 FreePage 추가
    static IDE_RC addPageToFreePagePool( smpPageListEntry*  aPageListEntry,
                                         smpFreePageHeader* aFreePageHeader );

    // FreePage들을 PageListEntry에서 제거
    static IDE_RC freePagesFromFreePagePoolToDB(
                                            void             * aTrans,
                                            scSpaceID          aSpaceID,
                                            smpPageListEntry * aPageListEntry,
                                            UInt               aPages );

    // FreePageHeader 생성 혹은 DB에서 페이지 할당시 초기화
    static void   initializeFreePageHeader(
                                         smpFreePageHeader  * aFreePageHeader );

    // PageListEntry의 모든 FreePageHeader 초기화
    static void   initAllFreePageHeader( scSpaceID         aSpaceID,
                                         smpPageListEntry* aPageListEntry );
    
    // FreePageHeader 초기화
    // smmManager에서 PCH초기화시 Callback으로 호출
    static IDE_RC initializeFreePageHeaderAtPCH( scSpaceID aSpaceID,
                                                 scPageID  aPageID );

    // FreePageHeader 해제
    static IDE_RC destroyFreePageHeaderAtPCH( scSpaceID aSpaceID,
                                              scPageID  aPageID );

    // FreePage에 대한 SizeClass 변경
    static IDE_RC modifyPageSizeClass( void*              aTrans,
                                       smpPageListEntry*  aPageListEntry,
                                       smpFreePageHeader* aFreePageHeader );

    // FreePage를 FreePageList에서 제거
    static IDE_RC removePageFromFreePageList(
                                        smpPageListEntry  *  aPageListEntry,
                                        UInt                 aPageListID,
                                        UInt                 aSizeClassID,
                                        smpFreePageHeader  * aFreePageHeader );

    // FreePage를 FreePageList에 추가
    static IDE_RC addPageToFreePageListTail(
                                        smpPageListEntry  *  aPageListEntry,
                                        UInt                 aPageListID,
                                        UInt                 aSizeClassID,
                                        smpFreePageHeader  * aFreePageHeader );

    // refineDB때 FreePage를 FreePageList에 추가
    static IDE_RC addPageToFreePageListAtInit(
                                        smpPageListEntry   * aPageListEntry,
                                        smpFreePageHeader  * aFreePageHeader );

    // PrivatePageList의 FreePage들을 Table의 PageListEntry로 병합
    static IDE_RC addFreePagesToTable( void*              aTrans,
                                       smpPageListEntry*  aPageListEntry,
                                       smpFreePageHeader* aFreePageHead );

    // FreeSlotList 초기화
    static void   initializeFreeSlotListAtPage(
                                        scSpaceID            aSpaceID,
                                        smpPageListEntry   * aPageListEntry,
                                        smpPersPage        * aPagePtr );

    // PCH영역에 있는 aPage의 FreePageHeader를 리턴한다.
    static inline smpFreePageHeader* getFreePageHeader( scSpaceID    aSpaceID,
                                                        smpPersPage* aPage );

    // PCH영역에 있는 aPageID의 FreePageHeader를 리턴한다.
    static inline smpFreePageHeader* getFreePageHeader( scSpaceID aSpaceID,
                                                        scPageID  aPageID );

    // PrivatePageList에 FreePage 연결
    static void   addFreePageToPrivatePageList( scSpaceID aSpaceID,
                                                scPageID  aCurPID,
                                                scPageID  aPrevPID,
                                                scPageID  aNextPID );

    // PrivatePageList에서 FreePage제거
    static void   removeFixedFreePageFromPrivatePageList(
                                    smpPrivatePageListEntry  * aPrivatePageList,
                                    smpFreePageHeader        * aFreePageHeader );

#ifdef DEBUG    
    /* aHeadFreePage~aTailFreePage까지의 FreePageList의 연결이 올바른지 검사한다. */
    static idBool isValidFreePageList( smpFreePageHeader* aHeadFreePage,
                                       smpFreePageHeader* aTailFreePage,
                                       vULong             aPageCount );
#endif

private:

    // SizeClass를 계산한다.
    static inline UInt   getSizeClass( smpRuntimeEntry * aEntry,
                                       UInt              aTotalSlotCnt,
                                       UInt              aFreeSlotCnt );
};

/**********************************************************************
 * Page의 SizeClass를 계산한다.
 *
 * aTotalSlotCnt : Page의 전체 Slot 갯수
 * aFreeSlotCnt  : Page의 FreeSlot 갯수
 **********************************************************************/

UInt smpFreePageList::getSizeClass( smpRuntimeEntry * aEntry,
                                    UInt              aTotalSlotCnt,
                                    UInt              aFreeSlotCnt )
{
    // aFreeSlotCnt
    // ------------- * (SMP_LAST_SIZECLASSID)
    // aTotalSlotCnt
    
    return (aFreeSlotCnt * SMP_LAST_SIZECLASSID(aEntry))/ aTotalSlotCnt;
}

/**********************************************************************
 * PCH영역에 있는 aPageID의 FreePageHeader를 리턴한다.
 *
 * aPageID : FreePageHeader가 필요한 PageID
 **********************************************************************/

smpFreePageHeader* smpFreePageList::getFreePageHeader( scSpaceID aSpaceID,
                                                       scPageID  aPageID )
{
    smmPCH* sPCH;

    // BUGBUG : aPageID에 대한 DASSERT 필요

    sPCH = (smmPCH*)(smmManager::getPCH( aSpaceID,
                                         aPageID ));

    IDE_DASSERT( sPCH != NULL );

    return (smpFreePageHeader*)(sPCH->mFreePageHeader);
}

/**********************************************************************
 * PCH영역에 있는 aPage의 FreePageHeader를 리턴한다.
 *
 * aPage : FreePageHeader가 필요한 Page
 **********************************************************************/

smpFreePageHeader* smpFreePageList::getFreePageHeader( scSpaceID    aSpaceID,
                                                       smpPersPage* aPage )
{
    IDE_DASSERT( aPage != NULL );
    
    return getFreePageHeader( aSpaceID, aPage->mHeader.mSelfPageID );
}

#endif /* _O_SMP_FREE_PAGELIST_H_ */
