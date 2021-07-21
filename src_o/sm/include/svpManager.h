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
 
#ifndef _O_SVP_MANAGER_H_
#define _O_SVP_MANAGER_H_ 1

#include <smm.h>

class svpManager
{
public:

    // aPage의 PageID 반환
    static scPageID getPersPageID     (void* aPage);

    // aPage의 PrevPageID 반환
    static scPageID getPrevPersPageID (void* aPage);

    // aPage의 NextPageID 반환
    static scPageID getNextPersPageID (void* aPage);

    // aPage의 PrevPageID 설정
    static void     setPrevPersPageID (void*    aPage,
                                       scPageID aPageID);

    // aPage의 NextPageID 설정
    static void     setNextPersPageID (void*     aPage,
                                       scPageID  aPageID);

    // aPage의 SelfID, PrevID, NextID 설정
    static void     linkPersPage(void*     aPage,
                                 scPageID  aSelf,
                                 scPageID  aPrev,
                                 scPageID  aNext);

    // aPage의 PageType 설정
    static void     initPersPageType(void* aPage);

    // PROJ-1490 : PageList 병렬화
    //             병렬화된 PageList에서 Head,Tail,Prev,Next를 반환
    // aPageListEntry의 첫 PageID 반환
    static scPageID getFirstAllocPageID(
        smpPageListEntry* aPageListEntry);

    // aPageListEntry의 마지막 PageID 반환
    static scPageID getLastAllocPageID(
        smpPageListEntry* aPageListEntry);

    // 다중화된 aPageListEntry에서 aPageID의 이전 PageID 반환
    static scPageID getPrevAllocPageID(scSpaceID         aSpaceID,
                                       smpPageListEntry* aPageListEntry,
                                       scPageID          aPageID);

    // 다중화된 aPageListEntry에서 aPageID의 다음 PageID 반환
    static scPageID getNextAllocPageID(scSpaceID         aSpaceID,
                                       smpPageListEntry* aPageListEntry,
                                       scPageID          aPageID);

    /*
     * BUG-25179 [SMM] Full Scan을 위한 페이지간 Scan List가 필요합니다.
     */
    static scPageID getFirstScanPageID( smpPageListEntry* aPageListEntry );
    static scPageID getLastScanPageID( smpPageListEntry* aPageListEntry );
    static scPageID getPrevScanPageID( scSpaceID         aSpaceID,
                                       scPageID          aPageID );
    static scPageID getModifySeqForScan(scSpaceID         aSpaceID,
                                        scPageID          aPageID);
    static scPageID getNextScanPageID( scSpaceID         aSpaceID,
                                       scPageID          aPageID );
    static idBool validateScanList( scSpaceID  aSpaceID,
                                    scPageID   aPageID,
                                    scPageID   aPrevPID,
                                    scPageID   aNextPID );
    
    // aAllocPageList의 PageCount, Head, Tail 정보 반환
    // smrUpdate에서 콜백으로 사용하기 때문에 aAllocPageList를 void*로 사용
    static void     getAllocPageListInfo(void*     aAllocPageList,
                                         vULong*   aPageCount,
                                         scPageID* aHeadPID,
                                         scPageID* aTailPID);

    // SMP_SLOT_HEADER_SIZE를 리턴
    static UInt     getSlotSize();

    // PageList에 달린 모든 Page 갯수
    static vULong   getAllocPageCount(smpPageListEntry* aPageListEntry);

    static UInt     getPersPageBodyOffset();
    static UInt     getPersPageBodySize();
};

#endif /* _O_SVP_MANAGER_H_ */
