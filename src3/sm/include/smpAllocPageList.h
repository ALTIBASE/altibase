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
 * $Id: smpAllocPageList.h 85392 2019-05-07 06:40:22Z seulki $
 **********************************************************************/

#ifndef _O_SMP_ALLOC_PAGELIST_H_
#define _O_SMP_ALLOC_PAGELIST_H_ 1

#include <smpDef.h>

class smpAllocPageList
{
public:

    // AllocPageList를 초기화한다
    static void     initializePageListEntry(
                                      smpAllocPageListEntry* aAllocPageList );

    // Runtime Item을 NULL로 설정한다.
    static IDE_RC setRuntimeNull( UInt                   aAllocPageListCount,
                                  smpAllocPageListEntry* aAllocPageListArray );
    
    
    // AllocPageList의 Runtime 정보(Mutex) 초기화
    static IDE_RC   initEntryAtRuntime( smpAllocPageListEntry* aAllocPageList,
                                        smOID                  aTableOID,
                                        smpPageType            aPageType );

    // AllocPageList의 Runtime 정보 해제
    static IDE_RC   finEntryAtRuntime( smpAllocPageListEntry* aAllocPageList );

    // aAllocPageHead~aAllocPageTail 를 AllocPageList에 추가
    static IDE_RC   addPageList( void*                  aTrans,
                                 scSpaceID              aSpaceID,
                                 smpAllocPageListEntry* aAllocPageList,
                                 smOID                  aTableOID,
                                 smpPersPage*           aAllocPageHead,
                                 smpPersPage*           aAllocPageTail,
                                 UInt                   aAllocPageCnt );

    // AllocPageList 제거해서 DB로 반납
    static IDE_RC   freePageListToDB( void*                  aTrans,
                                      scSpaceID              aSpaceID,
                                      smpAllocPageListEntry* aAllocPageList,
                                      smOID                  aTableOID );

    // AllocPageList에서 aPageID를 제거해서 DB로 반납
    static IDE_RC   removePage( void*                  aTrans,
                                scSpaceID              aSpaceID,
                                smpAllocPageListEntry* aAllocPageList,
                                scPageID               aPageID,
                                smOID                  aTableOID );
    
    // aHeadPage~aTailPage까지의 PageList의 연결이 올바른지 검사한다.
    static idBool   isValidPageList( scSpaceID aSpaceID,
                                     scPageID  aHeadPage,
                                     scPageID  aTailPage,
                                     vULong    aPageCount );

    // aCurRow다음 유효한 aNxtRow를 리턴한다.
    static IDE_RC   nextOIDall( scSpaceID              aSpaceID,
                                smpAllocPageListEntry* aAllocPageList,
                                SChar*                 aCurRow,
                                vULong                 aSlotSize,
                                SChar**                aNxtRow );

    // 다중화된 aAllocPageList에서 aPageID의 다음 PageID 반환
    static scPageID getNextAllocPageID( scSpaceID              aSpaceID,
                                        smpAllocPageListEntry* aAllocPageList,
                                        scPageID               aPageID );
    // 다중화된 aAllocPageList에서 aPagePtr의 다음 PageID 반환
    // added for dumpci
    static scPageID getNextAllocPageID( smpAllocPageListEntry * aAllocPageList,
                                        smpPersPageHeader     * aPagePtr );

    // 다중화된 aAllocPageList에서 aPageID의 이전 PageID 반환
    static scPageID getPrevAllocPageID( scSpaceID              aSpaceID,
                                        smpAllocPageListEntry* aAllocPageList,
                                        scPageID               aPageID);

    // aAllocPageList의 첫 PageID 반환
    static scPageID getFirstAllocPageID( smpAllocPageListEntry* aAllocPageList );

    // aAllocPageList의 마지막 PageID 반환
    static scPageID getLastAllocPageID( smpAllocPageListEntry* aAllocPageList );

private:

    // TableHeader에서 aAllocPageList의 Offset 계산
    static inline scOffset makeOffsetAllocPageList(
        smOID                  aTableOID,
        smpAllocPageListEntry* aAllocPageList);

    // nextOIDall을 위해 aRow에서 해당 Page를 찾아준다.
    static inline void     initForScan(
        scSpaceID              aSpaceID,
        smpAllocPageListEntry* aAllocPageList,
        SChar*                 aRow,
        vULong                 aSlotSize,
        smpPersPage**          aPage,
        SChar**                aRowPtr );

};

/**********************************************************************
 * TableHeader에서 AllocPageList의 Offset 계산
 *
 * aTableOID      : 계산하려는 테이블 OID
 * aAllocPageList : 계산하려는 AllocPageList
 **********************************************************************/

inline scOffset smpAllocPageList::makeOffsetAllocPageList(
                                    smOID                  aTableOID,
                                    smpAllocPageListEntry* aAllocPageList)
{
    UInt    sOffset;
    void  * sPagePtr;

    // ㅏㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡsOffsetㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅓ
    // [...TableHeader...mFixed.mMMDB.mAllocPageList[0][1]...[n]...]
    // ^ 테이블헤더페이지                                       ^ aAllocPageList
    
    IDE_ASSERT( smmManager::getPersPagePtr( SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                                            SM_MAKE_PID(aTableOID),
                                            &sPagePtr )
                == IDE_SUCCESS );

    sOffset = (UInt)( (SChar*)aAllocPageList - (SChar*)sPagePtr );

    IDE_DASSERT(sOffset > SMP_SLOT_HEADER_SIZE);
    IDE_DASSERT(sOffset < SM_PAGE_SIZE);
    
    return (vULong)sOffset;
}

#endif /* _O_SMP_ALLOC_PAGELIST_H_ */
