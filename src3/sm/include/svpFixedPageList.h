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
 
#ifndef _O_SVP_FIXED_PAGELIST_H_
#define _O_SVP_FIXED_PAGELIST_H_ 1

#include <smr.h>

class svpFixedPageList
{
public:
    
    // Runtime Item을 NULL로 설정한다.
    static IDE_RC setRuntimeNull( smpPageListEntry* aFixedEntry );

    /* FOR A4 : runtime 정보 및 mutex 초기화 또는 해제 */
    static IDE_RC initEntryAtRuntime( smOID                  aTableOID,
                                      smpPageListEntry*      aFixedEntry,
                                      smpAllocPageListEntry* aAllocPageList );

    /* runtime 정보 및 mutex 해제 */
    static IDE_RC finEntryAtRuntime( smpPageListEntry*  aFixedEntry );

    // PageList를 초기화한다.
    static void   initializePageListEntry( smpPageListEntry* aFixedEntry,
                                           smOID             aTableOID,
                                           vULong            aSlotSize );

    // aFixedEntry를 제거하고 DB로 PageList 반납
    static IDE_RC freePageListToDB( void*             aTrans,
                                    scSpaceID         aSpaceID,
                                    smOID             aTableOID,
                                    smpPageListEntry* aFixedEntry );

    // aPage를 초기화한다.
    static void   initializePage( vULong       aSlotSize,
                                  vULong       aSlotCount,
                                  UInt         aPageListID,
                                  smOID        aTableOID,
                                  smpPersPage* aPage );

    // slot을 할당한다.
    static IDE_RC allocSlot( void*             aTrans,
                             scSpaceID         aSpaceID,
                             void*             aTableInfoPtr,
                             smOID             aTableOID,
                             smpPageListEntry* aFixedEntry,
                             SChar**           aRow,
                             smSCN             aInfinite,
                             ULong             aMaxRow,
                             SInt              aOptFlag);
    
    // FreeSlotList에서 Slot을 꺼내온다.
    static void   removeSlotFromFreeSlotList(
        smpFreePageHeader* aFreePageHeader,
        SChar**            aRow );

    // slot을 free한다.
    static IDE_RC freeSlot ( void*              aTrans,
                             scSpaceID          aSpaceID,
                             smpPageListEntry*  aFixedEntry,
                             SChar*             aRow,
                             smpTableType       aTableType,
                             smSCN              aSCN );

    // FreeSlot 정보를 기록한다.
    static IDE_RC setFreeSlot( void         * aTrans,
                               scSpaceID      aSpaceID,
                               scPageID       aPageID,
                               SChar        * aRow,
                               smpTableType   aTableType );

    // FreePageHeader의 FreeSlotList에 FreeSlot을 추가한다.
    static void   addFreeSlotToFreeSlotList( smpFreePageHeader* aFreePageHeader,
                                             SChar*             aRow );

    // 실제 FreeSlot을 FreeSlotList에 추가한다.
    static IDE_RC addFreeSlotPending( void*             aTrans,
                                      scSpaceID         aSpaceID,
                                      smpPageListEntry* aFixedEntry,
                                      SChar*            aRow );

    /*
     * BUG-25179 [SMM] Full Scan을 위한 페이지간 Scan List가 필요합니다.
     */
    static IDE_RC linkScanList( scSpaceID          aSpaceID,
                                scPageID           aPageID,
                                smpPageListEntry * aFixedEntry );
    
    static IDE_RC unlinkScanList( scSpaceID          aSpaceID,
                                  scPageID           aPageID,
                                  smpPageListEntry * aFixedEntry );
    
    // PageList의 Record 갯수를 리턴한다.
    static ULong getRecordCount( smpPageListEntry* aFixedEntry );

    static IDE_RC setRecordCount( smpPageListEntry* aFixedEntry,
                                  ULong             aRecordCount );

    static void setAllocatedSlot( smSCN  aInfinite,
                                  SChar *aRow );
    
    // Slot Header를 altibase_sm.log에 덤프한다
    static IDE_RC dumpSlotHeader( smpSlotHeader     * aSlotHeader );

    static IDE_RC dumpFixedPage( scSpaceID         aSpaceID,
                                 scPageID          aPageID,
                                 smpPageListEntry *aFixedEntry );
    
private:
    // DB로부터 PersPages(리스트)를 받아와 aFixedEntry에 매단다.
    static IDE_RC allocPersPages( void*             aTrans,
                                  scSpaceID         aSpaceID,
                                  smpPageListEntry* aFixedEntry );
    
    // nextOIDall을 위해 aRow에서 해당 Page를 찾아준다.
    static inline void initForScan( scSpaceID          aSpaceID,
                                    smpPageListEntry*  aFixedEntry,
                                    SChar*             aRow,
                                    smpPersPage**      sPage,
                                    SChar**            sPtr );
    
    // allocSlot하기위해 PrivatePageList를 검사하여 시도
    static IDE_RC tryForAllocSlotFromPrivatePageList(
        void             * aTrans,
        scSpaceID          aSpaceID,
        smOID              aTableOID,
        smpPageListEntry * aFixedEntry,
        SChar           ** aRow );

    // allocSlot하기위해 FreePageList과 FreePagePool을 검사하여 시도
    static IDE_RC tryForAllocSlotFromFreePageList(
        void*             aTrans,
        scSpaceID         aSpaceID,
        smpPageListEntry* aFixedEntry,
        UInt              aPageListID,
        SChar**           aRow );

    // FreeSlotList 구축
    static IDE_RC buildFreeSlotList( void*             aTrans,
                                     scSpaceID         aSpaceID,
                                     UInt              aTableType,
                                     smpPageListEntry* aPageListEntry,
                                     iduPtrList*       aPtrList );

    // Slot에 대한 FreeSlot 확인
    static IDE_RC refineSlot( void*             aTrans,
                              scSpaceID         aSpaceID,
                              UInt              aTableType,
                              smiColumn**       aArrLobColumn,
                              UInt              aLobColumnCnt,
                              smpPageListEntry* aFixedEntry,
                              SChar*            aCurRow,
                              idBool          * aRefined,
                              iduPtrList*       aPtrList );

#ifdef DEBUG
    // Page내의 FreeSlotList의 연결이 올바른지 검사한다.
    static inline idBool isValidFreeSlotList(
                              smpFreePageHeader* aFreePageHeader );
#endif
};

#endif /* _O_SVP_FIXED_PAGELIST_H_ */

