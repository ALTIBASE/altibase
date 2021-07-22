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
 * $Id: smpManager.cpp 82075 2018-01-17 06:39:52Z jina.kim $
 **********************************************************************/

#include <idl.h>
#include <ideErrorMgr.h>
#include <idErrorCode.h>
#include <smErrorCode.h>
#include <smpDef.h>
#include <smpManager.h>
#include <smpAllocPageList.h>
#include <smpFreePageList.h>

smOID    smpManager::getTableOID(void* aPage)
{
    return ((smpPersPageHeader*)aPage)->mTableOID;
}

scPageID smpManager::getPersPageID(void* aPage)
{

    scPageID sPID;

    sPID = SMP_GET_PERS_PAGE_ID(aPage);

    return sPID ;
    
}

scPageID smpManager::getPrevPersPageID(void* aPage)
{

    return SMP_GET_PREV_PERS_PAGE_ID(aPage);

}

scPageID smpManager::getNextPersPageID(void* aPage)
{
    
    return SMP_GET_NEXT_PERS_PAGE_ID(aPage);

}

void  smpManager::setPrevPersPageID(void*     aPage,
                                    scPageID  aPageID)
{
    
    SMP_SET_PREV_PERS_PAGE_ID(aPage, aPageID);

}

void smpManager::setNextPersPageID(void*     aPage,
                                   scPageID  aPageID)
{
    
    SMP_SET_NEXT_PERS_PAGE_ID(aPage, aPageID);

}

void smpManager::linkPersPage(void*     aPage,
                              scPageID  aSelf,
                              scPageID  aPrev,
                              scPageID  aNext)
{

    SMP_SET_PERS_PAGE_ID(aPage, aSelf);
    SMP_SET_PREV_PERS_PAGE_ID(aPage, aPrev);
    SMP_SET_NEXT_PERS_PAGE_ID(aPage, aNext);
    
}

void smpManager::initPersPageType(void* aPage)
{
    
    SMP_SET_PERS_PAGE_TYPE(aPage, SMP_PAGETYPE_NONE);

}

UInt smpManager::getSlotSize()
{
    
    return SMP_SLOT_HEADER_SIZE;

}

/**********************************************************************
 * aPageListEntry의 첫 PageID를 반환
 *
 * aPageListEntry의 AllocPageList가 다중화되어 있기 때문에
 * 0번째 리스트의 Head가 NULL이더라도 1번째 리스트의 Head가 NULL이 아니라면
 * 첫 PageID는 1번째 리스트의 Head가 된다.
 *
 * aPageListEntry : 탐색하려는 PageListEntry
 **********************************************************************/

scPageID smpManager::getFirstAllocPageID(smpPageListEntry* aPageListEntry)
{
    IDE_DASSERT( aPageListEntry != NULL );

    return smpAllocPageList::getFirstAllocPageID(
        aPageListEntry->mRuntimeEntry->mAllocPageList);
}

/**********************************************************************
 * aPageListEntry의 마지막 PageID를 반환
 *
 * 다중화되어 있는 AllocPageList에서 NULL이 아닌 가장 뒤에 있는 Tail이
 * aPageListEntry의 마지막 PageID가 된다.
 *
 * aPageListEntry : 탐색하려는 PageListEntry
 **********************************************************************/

scPageID smpManager::getLastAllocPageID(smpPageListEntry* aPageListEntry)
{
    IDE_DASSERT( aPageListEntry != NULL );

    return smpAllocPageList::getLastAllocPageID(
        aPageListEntry->mRuntimeEntry->mAllocPageList);
}

/**********************************************************************
 * aPageListEntry에서 aPageID 이전 PageID
 *
 * 다중화되어있는 AllocPageList에서 aPageID가 Head라면
 * aPageID의 이전 Page는 이전 리스트의 Tail이 된다.
 *
 * aPageListEntry : 탐색하려는 PageListEntry
 * aPageID        : 탐색하려는 PageID
 **********************************************************************/

scPageID smpManager::getPrevAllocPageID(scSpaceID         aSpaceID,
                                        smpPageListEntry* aPageListEntry,
                                        scPageID          aPageID)
{
    IDE_DASSERT( aPageListEntry != NULL );

    return smpAllocPageList::getPrevAllocPageID(
        aSpaceID,
        aPageListEntry->mRuntimeEntry->mAllocPageList,
        aPageID);
}

/**********************************************************************
 * aPageListEntry에서 aPageID 다음 PageID
 *
 * 다중화되어있는 AllocPageList에서 aPageID가 Tail이라면
 * aPageID의 다음 Page는 다음 리스트의 Head가 된다.
 * 
 * aPageListEntry : 탐색하려는 PageListEntry
 * aPageID        : 탐색하려는 PageID
 **********************************************************************/

scPageID smpManager::getNextAllocPageID(scSpaceID         aSpaceID,
                                        smpPageListEntry* aPageListEntry,
                                        scPageID          aPageID)
{
    IDE_DASSERT( aPageListEntry != NULL );

    return smpAllocPageList::getNextAllocPageID(
        aSpaceID,
        aPageListEntry->mRuntimeEntry->mAllocPageList,
        aPageID);
}

/**********************************************************************
 *
 * Scan List상의 첫번째 페이지를 리턴한다.
 *
 * aPageListEntry : 탐색하려는 PageListEntry
 * 
 **********************************************************************/
scPageID smpManager::getFirstScanPageID(smpPageListEntry* aPageListEntry)
{
    IDE_DASSERT( aPageListEntry != NULL );

    return aPageListEntry->mRuntimeEntry->mScanPageList.mHeadPageID;
}

/**********************************************************************
 *
 * Scan List상의 마지막 페이지를 리턴한다.
 *
 * aPageListEntry : 탐색하려는 PageListEntry
 * 
 **********************************************************************/
scPageID smpManager::getLastScanPageID(smpPageListEntry* aPageListEntry)
{
    IDE_DASSERT( aPageListEntry != NULL );

    return aPageListEntry->mRuntimeEntry->mScanPageList.mTailPageID;
}

/**********************************************************************
 * 
 * 현재 페이지로부터 이전 페이지를 리턴한다.
 *
 * aSpaceID : Scan List가 속한 테이블스페이스 아이디
 * aPageID  : 현재 페이지 아이디
 * 
 **********************************************************************/
scPageID smpManager::getPrevScanPageID(scSpaceID         aSpaceID,
                                       scPageID          aPageID)
{
    smmPCH * sPCH;
    
    sPCH = (smmPCH*)(smmManager::getPCH( aSpaceID,
                                         aPageID ));
    IDE_DASSERT( sPCH != NULL );
    
    return idCore::acpAtomicGet32( &(sPCH->mPrvScanPID) );
}

/**********************************************************************
 * 
 * 현재 페이지로부터 다음 페이지를 리턴한다.
 * 
 * aSpaceID : Scan List가 속한 테이블스페이스 아이디
 * aPageID  : 현재 페이지 아이디
 * 
 **********************************************************************/
scPageID smpManager::getNextScanPageID(scSpaceID         aSpaceID,
                                       scPageID          aPageID)
{
    smmPCH * sPCH;
    
    sPCH = (smmPCH*)(smmManager::getPCH( aSpaceID,
                                         aPageID ));
    IDE_DASSERT( sPCH != NULL );
    
    return idCore::acpAtomicGet32( &(sPCH->mNxtScanPID) );
}

/**********************************************************************
 * 
 * 현재 페이지의 Modify Sequence를 리턴한다.
 * 
 * aSpaceID : Scan List가 속한 테이블스페이스 아이디
 * aPageID  : 현재 페이지 아이디
 * 
 **********************************************************************/
scPageID smpManager::getModifySeqForScan(scSpaceID         aSpaceID,
                                         scPageID          aPageID)
{
    smmPCH * sPCH;

    sPCH = (smmPCH*)(smmManager::getPCH( aSpaceID,
                                         aPageID ));
    IDE_DASSERT( sPCH != NULL );

    return idCore::acpAtomicGet64( &(sPCH->mModifySeqForScan) );
}


/**********************************************************************
 * 
 * 현재 페이지의 Next & Previous Link가 파라미터로 들어온 값들과
 * 동일한지 검사한다.
 * 
 * aSpaceID : Scan List가 속한 테이블스페이스 아이디
 * aPageID  : 현재 페이지 아이디
 * aPrevPID : 현재 페이지의 이전 페이지 아이디
 * aNextPID : 현재 페이지의 이후 페이지 아이디
 * 
 **********************************************************************/
idBool smpManager::validateScanList( scSpaceID  aSpaceID,
                                     scPageID   aPageID,
                                     scPageID   aPrevPID,
                                     scPageID   aNextPID )
{
    smmPCH * sPCH;
    
    sPCH = (smmPCH*)(smmManager::getPCH( aSpaceID,
                                         aPageID ));
    IDE_DASSERT( sPCH != NULL );

    if( (sPCH->mNxtScanPID == aNextPID) && (sPCH->mPrvScanPID == aPrevPID) )
    {
        return ID_TRUE;
    }
    
    return ID_FALSE;
}

/**********************************************************************
 * aAllocPageList 정보 반환
 *
 * aAllocPageList : 탐색하려는 AllocPageList
 * aPageCount     : aAllocPageList의 PageCount
 * aHeadPID       : aAllocPageList의 Head PID
 * aTailPID       : aAllocPageList의 Tail PID
 **********************************************************************/

void smpManager::getAllocPageListInfo(void*     aAllocPageList,
                                      vULong*   aPageCount,
                                      scPageID* aHeadPID,
                                      scPageID* aTailPID)
{
    smpAllocPageListEntry* sAllocPageList;

    IDE_DASSERT( aAllocPageList != NULL );
    IDE_DASSERT( aPageCount != NULL );
    IDE_DASSERT( aHeadPID != NULL );
    IDE_DASSERT( aTailPID != NULL );
    
    sAllocPageList = (smpAllocPageListEntry*)aAllocPageList;

    *aPageCount = sAllocPageList->mPageCount;
    *aHeadPID   = sAllocPageList->mHeadPageID;
    *aTailPID   = sAllocPageList->mTailPageID;
}

/**********************************************************************
 * PageList에 달린 모든 AllocPage 갯수를 리턴
 *
 * aPageListEntry : 탐색하려는 PageListEntry
 **********************************************************************/
vULong smpManager::getAllocPageCount(smpPageListEntry* aPageListEntry)
{
    UInt   sPageListID;
    vULong sPageCount = 0;

    IDE_DASSERT( aPageListEntry != NULL );
    
    for( sPageListID = 0;
         sPageListID < SMP_PAGE_LIST_COUNT;
         sPageListID++ )
    {
        sPageCount += aPageListEntry->mRuntimeEntry->mAllocPageList[sPageListID].mPageCount;
    }

    return sPageCount;
}

UInt smpManager::getPersPageBodyOffset()
{
    return SMP_PERS_PAGE_BODY_OFFSET ;
}

UInt smpManager::getPersPageBodySize()
{
    return SMP_PERS_PAGE_BODY_SIZE ;
}

/* PROJ-2162 RestartRiskReduction */
IDE_RC smpManager::setPersPageInconsistency( scSpaceID   aSpaceID,
                                             scPageID    aPageID )
{
    smpPersPageHeader * sMemPagePtr;

    IDE_TEST( smmManager::getPersPagePtr( aSpaceID, 
                                          aPageID,
                                          (void**)&sMemPagePtr )
              != IDE_SUCCESS );

    /* 잘못된 페이지에 대한 정보를 기록함 */
    ideLog::logMem( IDE_DUMP_0,
                    (UChar*)sMemPagePtr,
                    SM_PAGE_SIZE,
                    "MRDB PAGE:");

    IDE_TEST( smrUpdate::setPersPageInconsistency( aSpaceID,
                                                   aPageID,
                                                   sMemPagePtr->mType )
              != IDE_SUCCESS );

    SMP_SET_PERS_PAGE_TYPE( sMemPagePtr,
                            SMP_GET_PERS_PAGE_TYPE( sMemPagePtr ) |
                            SMP_PAGEINCONSISTENCY_TRUE );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

