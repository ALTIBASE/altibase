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
* $Id: smmExpandChunk.cpp 86424 2019-12-11 05:19:34Z jiwon.kim $
**********************************************************************/

#include <idl.h>
#include <idm.h>
#include <idu.h>
#include <ideErrorMgr.h>
#include <smm.h>
#include <smErrorCode.h>
#include <smDef.h>
#include <smu.h>
#include <smuUtility.h>
#include <smmReq.h>
#include <smmExpandChunk.h>

UInt   smmExpandChunk::mFLISlotCount;

smmExpandChunk::smmExpandChunk()
{
}

smmExpandChunk::~smmExpandChunk()
{
}


/******************************************************************
 * Private Member Functions
 ******************************************************************/

/* ExpandChunk 관리자를 초기화한다.
 */
IDE_RC smmExpandChunk::initialize(smmTBSNode * aTBSNode)
{

    // 하나의 Free List Info Page에 기록할 수 있는 Slot의 수
    mFLISlotCount = smLayerCallback::getPersPageBodySize()
                    /  SMM_FLI_SLOT_SIZE ;
    // Free List Info Page내의 Slot기록을 시작할 위치 
    aTBSNode->mFLISlotBase  = smLayerCallback::getPersPageBodyOffset();
    
    aTBSNode->mChunkPageCnt        = 0 ;
    
    return IDE_SUCCESS;
}

/*
 * ExpandChunk당 Page수를 설정한다.
 *
 * aChunkPageCnt [IN] 하나의 Expand Chunk가 가지는 Page의 수
 * 
 */
IDE_RC smmExpandChunk::setChunkPageCnt( smmTBSNode * aTBSNode,
                                        vULong       aChunkPageCnt )
{
    aTBSNode->mChunkPageCnt        = aChunkPageCnt ;

    IDE_DASSERT( mFLISlotCount != 0 );
    
    // Expand Chunk안의 모든 Page들의 Next Free Page ID를 기록할 수 있는
    // Free List Info Page의 수를 계산
    aTBSNode->mChunkFLIPageCnt = aTBSNode->mChunkPageCnt / mFLISlotCount ;
    if ( ( aTBSNode->mChunkPageCnt % mFLISlotCount ) != 0 )
    {
        aTBSNode->mChunkFLIPageCnt ++;
    }

    
    return IDE_SUCCESS;
}



/* ExpandChunk 관리자를 파괴한다.
 */
IDE_RC smmExpandChunk::destroy(smmTBSNode * /* aTBSNode */)
{
    return IDE_SUCCESS;
}

/*
 * 최소한 특정 Page수 만큼 저장할 수 있는 Expand Chunk의 수를 계산한다.
 *
 * aRequiredPageCnt    [IN] 여러개의 Expand Chunk에 저장할 총 Page의 수
 */
vULong smmExpandChunk::getExpandChunkCount( smmTBSNode * aTBSNode,
                                            vULong       aRequiredPageCnt )
{
    // setChunkPageCnt가 불렸는지 체크
    IDE_DASSERT( aTBSNode->mChunkPageCnt    != 0 );
    IDE_DASSERT( aRequiredPageCnt           != 0 );
    
    UInt sChunks =  aRequiredPageCnt / aTBSNode->mChunkPageCnt ;

    // 정확히 Expand Chunk Page수로 나누어 떨어지지 않는다면,
    // Expand Chunk를 하나 더 사용한다.
    if ( ( aRequiredPageCnt % aTBSNode->mChunkPageCnt ) != 0 )
    {
        sChunks ++ ;
    }


    return sChunks ;
}



/*
 * Next Free Page ID를 설정한다.
 *
 * aTrans 가 NULL이 아닌 경우, 로깅을 실시한다.
 *
 * 이 함수에서 FLI Page에 latch를 잡지 않는다.
 * 이 함수 호출 전에 FLI Page안의 동일한 Slot에
 * 여러 트랜잭션이 동시에 수정을 가하지 않도록 동시성 제어를 해야 한다.
 *
 * aTrans          [IN] Next Free Page를 설정하려는 트랜잭션
 * aPageID         [IN] Next Free Page를 설정할 Free Page
 * aNextFreePageID [IN] Next Free PAge ID
 */
IDE_RC smmExpandChunk::logAndSetNextFreePage( smmTBSNode * aTBSNode,
                                              void        *aTrans,
                                              scPageID     aFreePID,
                                              scPageID     aNextFreePID )
{
    scPageID       sFLIPageID;
    UInt           sSlotOffset;
    void         * sFLIPageAddr;
    smmFLISlot   * sSlotAddr;
    scPageID       sOrgNextFreePageID;

    IDE_DASSERT( isDataPageID( aTBSNode, aFreePID ) == ID_TRUE );
    IDE_DASSERT( ( aNextFreePID == SM_NULL_PID ||
                   aNextFreePID == SMM_FLI_ALLOCATED_PID ) ?
                 1 : isDataPageID( aTBSNode, aNextFreePID ) == ID_TRUE );
    
    IDE_DASSERT( aTBSNode->mFLISlotBase != 0 );
    
    // setChunkPageCnt가 불렸는지 체크
    IDE_DASSERT( aTBSNode->mChunkPageCnt != 0 );
    
    // 데이터 페이지의 Slot을 저장하는 FLI Page의 ID를 알아낸다.
    sFLIPageID = getFLIPageID( aTBSNode, aFreePID) ;

    // FLI Page안에서 데이터페이지의 Slot Offset을 구한다.
    sSlotOffset = getSlotOffsetInFLIPage( aTBSNode, aFreePID );

    // Free List Info Page 의 주소를 알아낸다.
    IDE_ASSERT( smmManager::getPersPagePtr( aTBSNode->mTBSAttr.mID, 
                                            sFLIPageID,
                                            &sFLIPageAddr )
                == IDE_SUCCESS );

    // Slot 의 주소를 계산한다.
    sSlotAddr = (smmFLISlot * ) ( ( (SChar* )sFLIPageAddr ) +
                                  aTBSNode->mFLISlotBase + 
                                  sSlotOffset );

    IDE_DASSERT( (SChar*)sSlotAddr == (SChar*)& sSlotAddr->mNextFreePageID );

    // aTrans 가 NULL일 경우에는 로깅하지 않는다.
    // allocNewExpandChunk 가 Logical Redo되는데,
    // 이 때 Free Page Link에 대해 로깅하지 않는다.
    if ( aTrans != NULL )
    {
        sOrgNextFreePageID = sSlotAddr->mNextFreePageID;

        IDE_TEST( smLayerCallback::updateLinkAtFreePage (
                      NULL, /* idvSQL* */
                      aTrans,
                      aTBSNode->mHeader.mID,
                      sFLIPageID,         // Page ID   
                      aTBSNode->mFLISlotBase + sSlotOffset, // Offset In Page
                      sOrgNextFreePageID, // Before Image 
                      aNextFreePID )      // After Image
                  != IDE_SUCCESS );
    
    }
    
    
    sSlotAddr->mNextFreePageID = aNextFreePID ;

    IDE_TEST( smmDirtyPageMgr::insDirtyPage( aTBSNode->mHeader.mID, sFLIPageID ) != IDE_SUCCESS );

    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;

    
    return IDE_FAILURE;
}



/*
 * Next Free Page ID를 가져온다.
 *
 * aPageID         [IN] Next Free Page를 가져올 Free Page
 * aNextFreePageID [IN] Next Free PAge ID
 */
IDE_RC smmExpandChunk::getNextFreePage( smmTBSNode * aTBSNode,
                                        scPageID     aFreePageID,
                                        scPageID *   aNextFreePageID )
{
    scPageID       sFLIPageID;
    UInt           sSlotOffset;
    void         * sFLIPageAddr;
    smmFLISlot   * sSlotAddr;

    IDE_DASSERT( isDataPageID( aTBSNode, aFreePageID ) == ID_TRUE );
    IDE_DASSERT( aNextFreePageID != NULL );

    // setChunkPageCnt가 불렸는지 체크
    IDE_DASSERT( aTBSNode->mChunkPageCnt != 0 );

    IDE_DASSERT( aTBSNode->mFLISlotBase != 0 );
    
    // 데이터 페이지의 Slot을 저장하는 FLI Page의 ID를 알아낸다.
    sFLIPageID = getFLIPageID( aTBSNode, aFreePageID) ;

    // FLI Page안에서 데이터페이지의 Slot Offset을 구한다.
    sSlotOffset = getSlotOffsetInFLIPage( aTBSNode, aFreePageID );

    // Free List Info Page 의 주소를 알아낸다.
    IDE_ERROR( smmManager::getPersPagePtr( aTBSNode->mTBSAttr.mID, 
                                           sFLIPageID,
                                           &sFLIPageAddr )
               == IDE_SUCCESS );

    // Slot 의 주소를 계산한다.
    sSlotAddr = (smmFLISlot * ) ( ( (SChar* )sFLIPageAddr ) +
                                  aTBSNode->mFLISlotBase + 
                                  sSlotOffset );

    * aNextFreePageID = sSlotAddr->mNextFreePageID ;

    IDE_DASSERT( ( * aNextFreePageID == SM_NULL_PID ||
                   * aNextFreePageID == SMM_FLI_ALLOCATED_PID ) ?
                 1 : isDataPageID( aTBSNode, *aNextFreePageID ) == ID_TRUE );
    
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/*
 * Data Page인지의 여부를 판단한다.
 * 
 * aPageID [IN] Data Page인지 여부를 판단할 Page의 ID
 * return aPageID에 해당하는 Page가 Data Page이면 ID_TRUE,
 *        Free List Info Page이면 ID_FALSE
 */
idBool smmExpandChunk::isDataPageID( smmTBSNode * aTBSNode,
                                     scPageID     aPageID )
{
    vULong sPageNoInChunk ;
    idBool sIsDataPage = ID_TRUE ;

    IDE_DASSERT( smmManager::isValidPageID( aTBSNode->mTBSAttr.mID, aPageID )
                 == ID_TRUE );
    IDE_DASSERT( aTBSNode->mChunkPageCnt != 0 );
    
    // Membase및 Catalog Table이 있는 Page는 Data Page가 아니다.
    if ( aPageID < SMM_DATABASE_META_PAGE_CNT )
    {
        sIsDataPage = ID_FALSE;
    }
    
    // Chunk 안에서 0부터 시작하는 Page 번호 계산
    sPageNoInChunk = ( aPageID - SMM_DATABASE_META_PAGE_CNT ) %
        aTBSNode->mChunkPageCnt ;

    // Chunk 내에서 Free List Info Page 영역에 위치하면,
    // FLI Page이지, Data Page가 아니다.
    if ( sPageNoInChunk < aTBSNode->mChunkFLIPageCnt )
    {
        sIsDataPage = ID_FALSE;
    }


    return sIsDataPage;
}

// BUG-47487: FLI Page인지의 여부를 판단한다.
idBool smmExpandChunk::isFLIPageID( smmTBSNode * aTBSNode, 
                                    scPageID     aPageID )
{
    vULong sPageNoInChunk ;

    // Membase및 Catalog Table이 있는 Page
    if ( aPageID < SMM_DATABASE_META_PAGE_CNT )
    {
        return ID_FALSE;
    }

    IDE_DASSERT( aPageID >= SMM_DATABASE_META_PAGE_CNT);    

    // Chunk 안에서 0부터 시작하는 Page 번호 계산
    sPageNoInChunk = ( aPageID - SMM_DATABASE_META_PAGE_CNT ) %
        aTBSNode->mChunkPageCnt ;

    // Chunk 내에서 Free List Info Page 영역에 위치하면,
    // FLI Page이다.
    if ( sPageNoInChunk < aTBSNode->mChunkFLIPageCnt )
    {
        return ID_TRUE;
    }
    else
    {
        return ID_FALSE;
    }
}

/* 데이터베이스 Page가 Free Page인지 여부를 리턴한다.
 *
 * aPageID     [IN] 검사하고자 하는 Page의 ID
 * aShouldLoad [OUT] Free Page이면 ID_TRUE, 아니면 ID_FALSE
 */
IDE_RC smmExpandChunk::isFreePageID(smmTBSNode * aTBSNode,
                                    scPageID     aPageID,
                                    idBool *     aIsFreePage )
{
    scPageID sNextFreePID;

    IDE_DASSERT( smmManager::isValidPageID( aTBSNode->mTBSAttr.mID, aPageID )
                 == ID_TRUE );
    IDE_DASSERT( aIsFreePage != NULL );
    
    if ( isDataPageID( aTBSNode, aPageID ) )
    {
        // 테이블에 할당된 Page인지 알아보기 위해
        // Page의 Next Free Page 를 알아낸다.
        IDE_TEST( getNextFreePage( aTBSNode, aPageID, & sNextFreePID )
                  != IDE_SUCCESS );

        // 테이블에 할당된 Page인 경우
        // Free List Info Page에 SMM_FLI_ALLOCATED_PID가 설정된다.
        // ( Page가 테이블로 할당될 때
        //   smmFPLManager::allocFreePageMemoryList 에서 설정됨 )
        if ( sNextFreePID == SMM_FLI_ALLOCATED_PID )
        {
            *aIsFreePage = ID_FALSE;
        }
        else // 데이터 페이지이면서 Free Page인 경우 
        {
            *aIsFreePage = ID_TRUE;
        }
    }
    else // 데이터 페이지가 아니라면 Free Page도 아니다.
    {
        *aIsFreePage = ID_FALSE;
    }

    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}



/******************************************************************
 * Private Member Functions
 ******************************************************************/


/*
 * 특정 Data Page가 속한 Expand Chunk를 알아낸다.
 *
 * 헤더파일의 맨 처음에 기술한 예를 통해 이 함수의 역할을 알아보자.
 * 다음과 같이 Free List Info Page가 두개, Data Page가 네개인 ExpandChunk
 * 두개가 존재하는 데이터베이스의 경우를 생각해보자.
 * 
 * < ExpandChunk #0 >
 *   | FLI-Page#0 | FLI-Page#1 | D-Page#2 | D-Page#3 | D-Page#4  | D-Page#5  |
 * < ExpandChunk #1 >
 *   | FLI-Page#6 | FLI-Page#7 | D-Page#8 | D-Page#9 | D-Page#10 | D-Page#11 |
 * 
 * 이 함수는 특정 Data Page가 속한 ExpandChunk를 알아내는 함수이며,
 * 위의 예에서 D-Page #4의 Expand Chunk는 1번이다.
 * ExpandChunk#1번의 시작 Page ID는 6번 이므로 이 함수는 6을 리턴하게 된다.
 *
 * aDataPageID [IN] Page가 속한 Expand Chunk를 알고자 하는 Page의 ID
 *
 * return aPID가 속한 ExpandChunk의 Page ID
 */
scPageID smmExpandChunk::getExpandChunkPID( smmTBSNode * aTBSNode,
                                            scPageID     aDataPageID )
{
    vULong sChunkNo ;
    scPageID sChunkFirstPID;

    IDE_DASSERT( isDataPageID( aTBSNode, aDataPageID ) == ID_TRUE );
    
    // Page ID를 하나의 Expand Chunk당 저장할 수 있는 Page의 수로 나눈다.
    sChunkNo = (aDataPageID - SMM_DATABASE_META_PAGE_CNT) /
        aTBSNode->mChunkPageCnt ;

    // Expand Chunk안의 첫번째 Page ( Free List Info Page)의 Page ID를 계산한다.
    sChunkFirstPID = sChunkNo * aTBSNode->mChunkPageCnt +
        SMM_DATABASE_META_PAGE_CNT;


    return sChunkFirstPID ;
}

/*
 * 하나의 Expand Chunk안에서 Free List Info Page를 제외한
 * 데이터 페이지들에 대해 순서대로 0부터 1씩 증가하는 번호를 매긴
 * Data Page No 를 리턴한다.
 *
 * 헤더파일의 맨 처음에 기술한 예를 통해 이 함수의 역할을 알아보자.
 * 다음과 같이 Free List Info Page가 두개, Data Page가 네개인 ExpandChunk
 * 두개가 존재하는 데이터베이스의 경우를 생각해보자.
 * 
 * < ExpandChunk #0 >
 *   | FLI-Page#0 | FLI-Page#1 | D-Page#2 | D-Page#3 | D-Page#4  | D-Page#5  |
 * < ExpandChunk #1 >
 *   | FLI-Page#6 | FLI-Page#7 | D-Page#8 | D-Page#9 | D-Page#10 | D-Page#11 |
 *
 * 이 예의 경우 데이터페이지는 D-Page#2,3,4,5,8,9,10,11이며,
 * 이들의 Data Page No 는 0,1,2,3,0,1,2,3 이다
 *
 * aDataPageID [IN] 자신이 속한 Expand Chunk를 알아내고자 하는 Data Page
 * return           Epxand Chunk안에서 Data Page들에 대해
 *                  순서대로 0부터 1씩 증가하도록 매긴 번호 
 */
vULong smmExpandChunk::getDataPageNoInChunk( smmTBSNode * aTBSNode,
                                             scPageID     aDataPageID )
{
    vULong sDataPageNo;
    scPageID sChunkPID;
    scPageID sFirstDataPagePID;

    // 데이터 페이지인지 확인한다.
    IDE_ASSERT( isDataPageID( aTBSNode, aDataPageID ) == ID_TRUE );
    
    // Expand Chunk안의 첫번째 Page의 Page ID를 계산한다.
    sChunkPID = getExpandChunkPID( aTBSNode, aDataPageID );

    // Expand Chunk안의 첫번째 Data Page ID
    // Expand Chunk시작 Page ID + Free List Info Page의 수 
    sFirstDataPagePID = sChunkPID + aTBSNode->mChunkFLIPageCnt;
    
    // Expand Chunk내의 Data Page를 0부터 1씩 증가하여 매긴 번호를 계산한다.
    // Data Page ID에서 Expand Chunk내의 첫번째 Data Page ID를 뺀다.
    sDataPageNo = aDataPageID - sFirstDataPagePID ;


    return sDataPageNo ;
}


/*
 * 특정 Data Page의 Next Free Page ID를 기록할 Slot이 존재하는
 * Free List Info Page를 알아낸다.
 *
 * 헤더파일의 맨 처음에 기술한 예를 통해 이 함수의 역할을 알아보자.
 * 다음과 같이 Free List Info Page가 두개, Data Page가 네개인 ExpandChunk
 * 두개가 존재하는 데이터베이스의 경우를 생각해보자.
 * 
 * < ExpandChunk #0 >
 *   | FLI-Page#0 | FLI-Page#1 | D-Page#2 | D-Page#3 | D-Page#4  | D-Page#5  |
 * < ExpandChunk #1 >
 *   | FLI-Page#6 | FLI-Page#7 | D-Page#8 | D-Page#9 | D-Page#10 | D-Page#11 |
 *
 * 만약 이 함수에 aPID인자로 9가 들어왔다면, D-Page#9의 Free List Info Page는
 * FLI-Page#6 이므로, Data Page 9의 Slot이 기록되는 Free List Info Page의
 * PageID로 7이 리턴된다.
 *
 * 주의! Free List Info Page안의 Slot은
 * Free List Info Page 자체의 Slot을 지니지 않는다
 * 위의 예에서, FLI-Page#0 는 D-Page#2, D-Page#3과 1:1로 대응하는 Slot을 가지며,
 * FLI-Page#0 에 대응하는 Slot은 그 어느곳에도 저장되지 않는다.
 *
 * aDataPageID [IN] Free List Info Page를 알아내고자 하는 Page의 ID
 * return Free List Info Page의 ID
 */
scPageID smmExpandChunk::getFLIPageID( smmTBSNode * aTBSNode,
                                       scPageID     aDataPageID )
{
    vULong sDataPageNo, sFLINo;
    scPageID sChunkPID;

    IDE_DASSERT( mFLISlotCount != 0 );
    
    // 데이터 페이지인지 확인한다.
    IDE_ASSERT( isDataPageID( aTBSNode, aDataPageID ) == ID_TRUE );
    
    // Expand Chunk의 Page ID를 알아낸다.
    sChunkPID = getExpandChunkPID( aTBSNode, aDataPageID );

    // Expand Chunk안의 Data Page No를 알아낸다.
    sDataPageNo = getDataPageNoInChunk( aTBSNode, aDataPageID );

    // Data Page의 Slot이 기록될 Free List Info Page를 계산하기 위해
    // Expand Chunk내의 FLI Page를 0부터 1씩 증가하여 매긴 번호를 계산한다.
    sFLINo = sDataPageNo / mFLISlotCount ;


    // Free List Info Page의 ID를 리턴한다.
    return sChunkPID + sFLINo ;
}

/* FLI Page 내에서 Data Page와 1:1로 매핑되는 Slot의 offset을 계산한다.
 *
 * aDataPageID [IN] Slot Offset을 계산하고자 하는 Page의 ID
 * return Data Page와 1:1로 매핑되는 Slot의 Offset
 */
UInt smmExpandChunk::getSlotOffsetInFLIPage( smmTBSNode * aTBSNode,
                                             scPageID     aDataPageID )
{
    vULong sDataPageNo;
    UInt sSlotNo;

    IDE_DASSERT( isDataPageID( aTBSNode, aDataPageID ) == ID_TRUE );
    IDE_DASSERT( mFLISlotCount != 0 );
    
    // Expand Chunk내의 Data Page를 0부터 1씩 증가하여 매긴 번호를 계산한다. 
    sDataPageNo = getDataPageNoInChunk( aTBSNode, aDataPageID );

    // FLI Page 내에서의 Slot번호를 계산한다.
    sSlotNo = (UInt) ( sDataPageNo % mFLISlotCount );


    // FLI Page 내에서의 Slot offset을 리턴한다.
    return sSlotNo * SMM_FLI_SLOT_SIZE ;
}

/*****************************************************************************
 *
 * BUG-32461 [sm-mem-resource] add getPageState functions to 
 * smmExpandChunk module 
 *
 * Description: Persistent Page의 할당 상태를 리턴한다.
 *
 * Parameters:
 *  - aTBSNode      [IN] 검사대상 Page가 속한 TBSNode
 *  - aPageID       [IN] 검사하고자 하는 Page의 ID
 *  - aPageState    [OUT] 대상 page의 상태를 반환
 * 
 *                      <-----------DataPage--------->
 *        +-------------------------------------------------------
 * Table  | META | FLI | alloc | free | alloc | alloc | FLI | ...
 * Space  +-------------------------------------------------------
 *
 ****************************************************************************/
IDE_RC smmExpandChunk::getPageState( smmTBSNode     * aTBSNode,
                                     scPageID         aPageID,
                                     smmPageState   * aPageState )
{
    smmPageState    sPageState;
    idBool          sIsFreePage;
 
    IDE_ERROR( aPageState != NULL );

    if( isDataPageID( aTBSNode, aPageID ) == ID_TRUE )
    {
        /* DataPage, 즉 Table에 할당할 수 있는 Page일 경우 */
        IDE_TEST( isFreePageID( aTBSNode, 
                                aPageID, 
                                &sIsFreePage )
                  != IDE_SUCCESS );

        if( sIsFreePage == ID_TRUE )
        {
            sPageState = SMM_PAGE_STATE_FREE;
        }
        else
        {
            sPageState = SMM_PAGE_STATE_ALLOC;
        }
    }
    else
    {
        /* Meta 또는 FLI등의 Table에 할당할 수 없는 Page일 경우 */
        if ( aPageID < SMM_DATABASE_META_PAGE_CNT )
        {
            sPageState = SMM_PAGE_STATE_META;
        }
        else
        {
            sPageState = SMM_PAGE_STATE_FLI;
        }

    }

    *aPageState = sPageState;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

