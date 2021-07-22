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
 
#include <idl.h>
#include <idm.h>
#include <idu.h>
#include <ideErrorMgr.h>
#include <smErrorCode.h>
#include <smDef.h>
#include <smu.h>
#include <smuUtility.h>
#include <svmReq.h>
#include <svmManager.h>
#include <svmDatabase.h>
#include <svmFPLManager.h>
#include <svmExpandChunk.h>
#include <sctTableSpaceMgr.h>


/*======================================================================
 *
 *  PUBLIC MEMBER FUNCTIONS
 *
 *======================================================================*/

iduMutex svmFPLManager::mGlobalAllocChunkMutex;

/*
    Static Initializer
 */
IDE_RC svmFPLManager::initializeStatic()
{
    IDE_TEST( mGlobalAllocChunkMutex.initialize(
                                         (SChar*)"SVM_GLOBAL_ALLOC_CHUNK_MUTEX",
                                         IDU_MUTEX_KIND_POSIX,
                                         IDV_WAIT_INDEX_NULL)
              != IDE_SUCCESS );


    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/*
    Static Destroyer
 */
IDE_RC svmFPLManager::destroyStatic()
{
    IDE_TEST( mGlobalAllocChunkMutex.destroy() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}




/*
 * Free Page List 관리자를 초기화한다.
 */
IDE_RC svmFPLManager::initialize( svmTBSNode * aTBSNode )
{
    UInt i ;
    SChar sMutexName[128];

    idlOS::memset(&aTBSNode->mMemBase, 0, ID_SIZEOF(svmMemBase));

    // 각 Free Page List의 Mutex를 초기화
    /* svmFPLManager_initialize_malloc_ArrFPLMutex.tc */
    IDU_FIT_POINT("svmFPLManager::initialize::malloc::ArrFPLMutex");
    IDE_TEST( iduMemMgr::malloc( IDU_MEM_SM_SVM,
                                 ID_SIZEOF(iduMutex) *
                                 SVM_FREE_PAGE_LIST_COUNT,
                                 (void **) & aTBSNode->mArrFPLMutex,
                                 IDU_MEM_FORCE )
              != IDE_SUCCESS );

    /* BUG-31881 각 Free Page List에 대한 PageReservation 구조체 메모리 할당 */
    aTBSNode->mArrPageReservation = NULL;
    /* svmFPLManager_initialize_malloc_ArrPageReservation.tc */
    IDU_FIT_POINT("svmFPLManager::initialize::malloc::ArrPageReservation");
    IDE_TEST( iduMemMgr::malloc( IDU_MEM_SM_SVM,
                                 ID_SIZEOF( svmPageReservation ) *
                                 SVM_FREE_PAGE_LIST_COUNT,
                                 (void **) & aTBSNode->mArrPageReservation,
                                 IDU_MEM_FORCE )
              != IDE_SUCCESS );

    for ( i =0; i < SVM_FREE_PAGE_LIST_COUNT ; i++ )
    {
        // BUGBUG kmkim check NULL charackter
        idlOS::snprintf( sMutexName,
                         128,
                         "SVM_FREE_PAGE_LIST_%"ID_UINT32_FMT"_%"ID_UINT32_FMT"_MUTEX",
                         aTBSNode->mHeader.mID,
                         i );

        new( & aTBSNode->mArrFPLMutex[i] ) iduMutex();
        IDE_TEST( aTBSNode->mArrFPLMutex[i].initialize(
                      sMutexName,
                      IDU_MUTEX_KIND_NATIVE,
                      IDV_WAIT_INDEX_NULL) != IDE_SUCCESS );

        aTBSNode->mArrPageReservation[ i ].mSize = 0;
    }

    idlOS::snprintf( sMutexName,
                     128,
                     "SVM_ALLOC_CHUNK_%"ID_UINT32_FMT"_MUTEX",
                     aTBSNode->mHeader.mID );

    IDE_TEST( aTBSNode->mAllocChunkMutex.initialize(
                  sMutexName,
                  IDU_MUTEX_KIND_NATIVE,
                  IDV_WAIT_INDEX_NULL ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/*
 * Free Page List 관리자를 파괴한다.
 *
 */
IDE_RC svmFPLManager::destroy(svmTBSNode * aTBSNode)
{
    UInt i;

    IDE_DASSERT( aTBSNode->mArrFPLMutex != NULL );

    // Chunk 할당 Mutex를 파괴.
    IDE_TEST( aTBSNode->mAllocChunkMutex.destroy() != IDE_SUCCESS );

    // 각 Free Page List의 Mutex를 파괴
    for ( i =0; i < SVM_FREE_PAGE_LIST_COUNT; i++ )
    {
        IDE_TEST( aTBSNode->mArrFPLMutex[i].destroy() != IDE_SUCCESS );
    }

    IDE_TEST( iduMemMgr::free( aTBSNode->mArrFPLMutex ) != IDE_SUCCESS );
    aTBSNode->mArrFPLMutex = NULL ;

    IDE_TEST( iduMemMgr::free( aTBSNode->mArrPageReservation ) 
              != IDE_SUCCESS );
    aTBSNode->mArrPageReservation = NULL ;

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
    Create Volatile Tablespace나 DML로 인해 Chunk확장 발생시 Mutex잡기
 */
IDE_RC svmFPLManager::lockGlobalAllocChunkMutex()
{
    IDE_TEST( mGlobalAllocChunkMutex.lock(NULL) != IDE_SUCCESS );

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
    Create Volatile Tablespace나 DML로 인해 Chunk확장 발생시 Mutex풀기
 */
IDE_RC svmFPLManager::unlockGlobalAllocChunkMutex()
{
    IDE_TEST( mGlobalAllocChunkMutex.unlock() != IDE_SUCCESS );

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/* Free Page를 가장 조금 가진 데이터베이스 Free Page List를 찾아낸다.
 *
 * 주의! 동시성 제어를 하지 않기 때문에, 이 함수가 리턴한 Free Page List가
 *       가장 적은 Free Page List를 가지고 있지 않을 수도 있다.
 *
 * aFPLNo [OUT] 찾아낸 Free Page List
 */
IDE_RC svmFPLManager::getShortestFreePageList( svmTBSNode * aTBSNode,
                                               svmFPLNo *   aFPLNo )
{
    UInt     i;
    svmFPLNo sShortestFPL         ;
    vULong   sShortestFPLPageCount;

    IDE_DASSERT( aFPLNo != NULL );

    sShortestFPL          = 0;
    sShortestFPLPageCount = aTBSNode->mMemBase.mFreePageLists[ 0 ].mFreePageCount ;

    for ( i=1; i < aTBSNode->mMemBase.mFreePageListCount; i++ )
    {
        if ( aTBSNode->mMemBase.mFreePageLists[ i ].mFreePageCount <
             sShortestFPLPageCount )
        {
            sShortestFPL = i;
            sShortestFPLPageCount =
                aTBSNode->mMemBase.mFreePageLists[ i ].mFreePageCount;
        }
    }

    *aFPLNo = sShortestFPL ;

    return IDE_SUCCESS;

//    IDE_EXCEPTION_END;
//    return IDE_FAILURE;
}

/* Free Page를 가장 많이 가진 데이터베이스 Free Page List를 찾아낸다.
 *
 * 주의! 동시성 제어를 하지 않기 때문에, 이 함수가 리턴한 Free Page List가
 *       가장 많은 Free Page List를 가지고 있지 않을 수도 있다.
 *
 * aFPLNo [OUT] 찾아낸 Free Page List
 */
IDE_RC svmFPLManager::getLargestFreePageList( svmTBSNode * aTBSNode,
                                              svmFPLNo *   aFPLNo )
{
    UInt     i;
    svmFPLNo sLargestFPL          ;
    vULong   sLargestFPLPageCount ;

    IDE_DASSERT( aFPLNo != NULL );

    sLargestFPL          = 0;
    sLargestFPLPageCount = aTBSNode->mMemBase.mFreePageLists[ 0 ].mFreePageCount ;

    for ( i=1; i < aTBSNode->mMemBase.mFreePageListCount; i++ )
    {
        if ( aTBSNode->mMemBase.mFreePageLists[ i ].mFreePageCount >
             sLargestFPLPageCount )
        {
            sLargestFPL = i;
            sLargestFPLPageCount =
                aTBSNode->mMemBase.mFreePageLists[ i ].mFreePageCount;
        }
    }

    *aFPLNo = sLargestFPL ;

    return IDE_SUCCESS;
//    IDE_EXCEPTION_END;
//    return IDE_FAILURE;
}





/* 하나의 Free Page들을 데이터베이스 Free Page List에 골고루 분배한다.
 *
 * Chunk에 속한 Page를 일정개수씩 각각의 Free Page List에 여러번에 걸쳐서
 * 분배한다.
 *
 * 이를 한번에 분배하지 않는 이유는, 한번에 분배할 경우
 * 모든 Free Page List로부터 Page를 할당받는 경우 데이터파일에 커다란
 * Hole이 생기는데, 이는 DB 파일을 메모리로 적재하는데 있어서
 * 더 높은 비용을 필요로한다.
 *
 * Free Page List에 한번에 분배할 Page의 수가 너무 작으면 Page의 Locality
 * 가 그다지 높지 않아서 운영중에 성능이 저하될 수 있다.
 *
 * Free Page List에 한번에 분배할 Page의 수가 너무 많으면, 데이터베이스
 * 파일에 Hole을 만들어서 데이터베이스 기동시에
 * 데이터베이스 파일을 메모리로 읽어오기 위한 I/O비용이 증가한다.
 *
 * 이 함수는 svmManager::allocNewExpandChunk를 통해
 * Expand Chunk할당시에 호출되는 루틴이며,
 * Expand Chunk할당은 Logical Redo되므로,
 * Expand Chunk할당 과정에 대해서 로깅을 수행하지 않는다.
 *
 * 이 함수는 membase에 접근하지 않기 때문에 동시성 제어가 불필요하다.
 *
 * aChunkFirstFreePID [IN] 새로 추가된 Chunk의 첫번째 Free Page ID
 * aChunkLastFreePID  [IN] 새로 추가된 Chunk의 마지막 Free Page ID
 * aArrFreePageList   [OUT] 분배된 Free Page의 List들을 받을 배열
 */
IDE_RC svmFPLManager::distributeFreePages( svmTBSNode *  aTBSNode,
                                           scPageID      aChunkFirstFreePID,
                                           scPageID      aChunkLastFreePID,
                                           idBool        aSetNextFreePage,
                                           svmPageList * aArrFreePageList)
{
    scPageID      sPID ;
    // 하나의 Free Page List에 분배할 첫번째 Free Page
    scPageID      sDistHeadPID;
    vULong        sLinkedPageCnt = 0;
    svmFPLNo      sFPLNo;
    UInt          i;
    // Expand Chunk할당시 각 Free Page에 한번에 몇개의 Page씩을 분배할 것인지.
    vULong        sDistPageCnt = SVM_PER_LIST_DIST_PAGE_COUNT ;
    vULong        sChunkDataPageCount =
                      aChunkLastFreePID - aChunkFirstFreePID + 1;

    svmPageList * sFreeList ;

    IDE_DASSERT( svmExpandChunk::isDataPageID( aTBSNode,
                                               aChunkFirstFreePID )
                 == ID_TRUE);
    IDE_DASSERT( svmExpandChunk::isDataPageID( aTBSNode,
                                               aChunkLastFreePID )
                 == ID_TRUE);

    IDE_ASSERT( aArrFreePageList != NULL );
    IDE_ASSERT( aChunkFirstFreePID <= aChunkLastFreePID );
    IDE_ASSERT( aTBSNode->mMemBase.mFreePageListCount > 0 );

    // Expand Chunk안의 페이지들이 모든 Free Page List 안에
    // 최소한 한번씩 들어갈 수 있을만한 충분한 양인지 다시한번 검사
    // ( svmManager 에서 초기화 도중에 이미 검사했음 )
    //
    // Free List Info Page를 포함한 Chunk안의 모든 Page수가
    // 2 * PER_LIST_DIST_PAGE_COUNT * m_membase->mFreePageListCount
    // 보다 크거나 같음을 체크하였다.
    //
    // Chunk안의 Free List Info Page가 전체의 50%를 차지할 수는 없으므로,
    // Chunk안의 데이터페이지수 ( Free List Info Page를 제외)는
    // PER_LIST_DIST_PAGE_COUNT * m_membase->mFreePageListCount 보다 항상
    // 크거나 같음을 보장할 수 있다.

    IDE_ASSERT( sChunkDataPageCount >=
                sDistPageCnt * aTBSNode->mMemBase.mFreePageListCount );

    sFPLNo = 0;
    // Free Page List 배열을 초기화
    for ( i = 0 ;
          i < aTBSNode->mMemBase.mFreePageListCount ;
          i ++ )
    {
        SVM_PAGELIST_INIT( & aArrFreePageList[ i ] );
    }

    // 새 Expand Chunk내의 모든 Data Page에 대해서
    for ( // Free Page List에 분배할 첫번째 Page도 함께 초기화한다.
          sPID = sDistHeadPID = aChunkFirstFreePID;
          sPID <= aChunkLastFreePID ;
          sPID ++ )
    {
             // Free Page List에 분배할 마지막 페이지일 경우
        if ( sLinkedPageCnt == sDistPageCnt - 1 ||
             sPID == aChunkLastFreePID )
        {
            if ( aSetNextFreePage == ID_TRUE )
            {
                /*
                   분배할 마지막 페이지이므로 Next Free Page는 NULL이다.

                   PRJ-1548 User Memory Tablespace 개념도입
                   [1] 운영중에 chunk 확장시
                   [2] restart recovery의 Logical Redo
                   [3] media recovery의 Logical Redo
                       복구할 Chunk를 복구해야하는 경우
                */

                IDE_TEST( svmExpandChunk::setNextFreePage(
                          aTBSNode,
                          sPID,
                          SM_NULL_PID ) != IDE_SUCCESS );
            }
            else
            {
                /*
                   media recovery의 Logical Redo 중에서
                   chunk 복구가 필요없는 경우
                */
            }

            sFreeList = & aArrFreePageList[ sFPLNo ];

            IDE_DASSERT( svmFPLManager::isValidFPL(
                             aTBSNode->mTBSAttr.mID,
                             sFreeList->mHeadPID,
                             sFreeList->mPageCount ) == ID_TRUE );

            // sDistHeadPID ~ sPID 까지를 Free List에 매달아준다.
            if ( sFreeList->mHeadPID == (scPageID)0 )
            {
                IDE_DASSERT( sFreeList->mTailPID == (scPageID)0 );
                sFreeList->mHeadPID   = sDistHeadPID ;
                sFreeList->mTailPID   = sPID ;
                sFreeList->mPageCount = sLinkedPageCnt + 1;
            }
            else
            {
                IDE_DASSERT( sFreeList->mTailPID != (scPageID)0 );

                // Free List 의 맨 끝 (Tail) 에 매단다.

                if ( aSetNextFreePage == ID_TRUE )
                {
                    IDE_TEST( svmExpandChunk::setNextFreePage(
                                aTBSNode,
                                sFreeList->mTailPID,
                                sDistHeadPID )
                            != IDE_SUCCESS );
                }
                else
                {
                    // Nothing To do
                }

                sFreeList->mTailPID    = sPID ;
                sFreeList->mPageCount += sLinkedPageCnt + 1;
            }

            IDE_DASSERT( svmFPLManager::isValidFPL(
                             aTBSNode->mTBSAttr.mID,
                             sFreeList->mHeadPID,
                             sFreeList->mPageCount ) == ID_TRUE );

            // 다음번에 Free Page를 받을 Free Page List 결정
            sFPLNo = (sFPLNo + 1) % aTBSNode->mMemBase.mFreePageListCount ;

            // 다음 Page부터 또 다른 Free Page List에 분배할 첫번째 Free Page가 된다
            sDistHeadPID = sPID + 1;
            sLinkedPageCnt = 0;
        }
        else
        {

            if ( aSetNextFreePage == ID_TRUE )
            {
                // Chunk안의 다음 Page를 Next Free Page로 설정한다.
                IDE_TEST( svmExpandChunk::setNextFreePage(
                            aTBSNode,
                            sPID,
                            sPID + 1 )
                        != IDE_SUCCESS );
            }
            else
            {
                // Nothing To do
            }

            sLinkedPageCnt ++;
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 * Free Page 들을 각각의 Free Page List의 맨 앞에 append한다.
 *
 * 여러개의 Free Page들을 distributeFreePages가 Free Page List수만큼의
 * Page List로 쪼개어 만들고 나면, 이 함수가 이를 받아서
 * membase의 각각의 Free Page List에 append한다.
 *
 * Expand Chunk할당시에 호출되며, 로깅하지 않음.
 * ( Expand Chunk할당이 Logical Redo되기 때문 )
 *
 * 주의! 동시성 제어의 책임이 이 함수의 호출자에게 있다.
 *       이 함수에서는 동시성 제어를 하지 않는다.
 *
 * aArrFreePageList [IN] membase의 각 Free Page List 들에 붙일 Page List
 *                       들이 저장된 배열
 *                       Free Page List Info Page 에 Next링크로 연결되어있다.
 *
 */
IDE_RC svmFPLManager::appendPageLists2FPLs( svmTBSNode *  aTBSNode,
                                            svmPageList * aArrFreePageList )
{

    svmFPLNo      sFPLNo ;

    IDE_DASSERT( aArrFreePageList != NULL );

    for ( sFPLNo = 0;
          sFPLNo < aTBSNode->mMemBase.mFreePageListCount  ;
          sFPLNo ++ )
    {
        // Redo 중에 불리는 함수이므로, FPL의 Validity체크를 해서는 안된다.
        IDE_TEST( appendFreePagesToList(
                      aTBSNode,
                      NULL, // Chunk 할당이기 때문에, PageReservation와 상관없음
                      sFPLNo,
                      aArrFreePageList[sFPLNo].mPageCount,
                      aArrFreePageList[sFPLNo].mHeadPID,
                      aArrFreePageList[sFPLNo].mTailPID )
                  != IDE_SUCCESS );

    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}




/*
 * 모든 Free Page에 있는 Free Page 수를 리턴한다.
 *
 * 이 함수는 Latch를 잡지 않기 때문에 정확한 Free Page수를 리턴하지 못한다.
 *
 * aTotalPageCount [OUT] 모든 Free Page List의 페이지수의 총합
 *
 */
IDE_RC svmFPLManager::getTotalFreePageCount(svmTBSNode * aTBSNode,
                                               vULong *  aTotalFreePageCnt)
{
    UInt i ;
    vULong sFreePageCnt = 0;

    IDE_DASSERT( aTotalFreePageCnt != NULL );

    for ( i=0 ; i <  aTBSNode->mMemBase.mFreePageListCount ; i ++ )
    {
        sFreePageCnt += aTBSNode->mMemBase.mFreePageLists[i].mFreePageCount ;
    }

    *aTotalFreePageCnt = sFreePageCnt;

    return IDE_SUCCESS;
}



/*======================================================================
 *
 *  PRIVATE MEMBER FUNCTIONS
 *
 *======================================================================*/



/*
 * Free Page List 에 래치를 건다.
 *
 * aFPLNo [IN] Latch를 걸려고 하는 Free Page List
 */
IDE_RC svmFPLManager::lockFreePageList( svmTBSNode * aTBSNode,
                                        svmFPLNo     aFPLNo )
{
    IDE_ASSERT( aFPLNo < aTBSNode->mMemBase.mFreePageListCount );

    return aTBSNode->mArrFPLMutex[ aFPLNo ].lock(NULL);
}


/*
 * Free Page List 의 래치를 푼다.
 *
 * aFPLNo [IN] Latch를 풀려고 하는 Free Page List
 */
IDE_RC svmFPLManager::unlockFreePageList( svmTBSNode * aTBSNode,
                                          svmFPLNo     aFPLNo )
{
    IDE_ASSERT( aFPLNo < aTBSNode->mMemBase.mFreePageListCount );

    return aTBSNode->mArrFPLMutex[ aFPLNo ].unlock();
}




/*
 * 모든 Free Page List에 latch를 잡는다.
 */
IDE_RC svmFPLManager::lockAllFPLs( svmTBSNode * aTBSNode )
{

    svmFPLNo      sFPLNo ;

    for ( sFPLNo = 0;
          sFPLNo < aTBSNode->mMemBase.mFreePageListCount  ;
          sFPLNo ++ )
    {

        // 바깥에서 특정 List는 이미 latch가 잡힌채로 이 함수를 부를 수 있다.
        // 이미 latch가 잡힌 Free Page List라면 Latch를 잡지 않는다.
        IDE_TEST( lockFreePageList( aTBSNode, sFPLNo ) != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_CALLBACK_FATAL( "Can't lock mutex of DB Free Page List." );

    return IDE_FAILURE;
}


/*
 * 모든 Free Page List에서 latch를 푼다.
 */
IDE_RC svmFPLManager::unlockAllFPLs( svmTBSNode * aTBSNode )
{

    svmFPLNo      sFPLNo ;

    for ( sFPLNo = 0;
          sFPLNo < aTBSNode->mMemBase.mFreePageListCount  ;
          sFPLNo ++ )
    {

        // 바깥에서 특정 List는 이미 latch가 잡힌채로 이 함수를 부를 수 있다.
        // 이미 latch가 잡힌 Free Page List라면 Latch를 잡지 않는다.
        IDE_TEST( unlockFreePageList( aTBSNode, sFPLNo ) != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_CALLBACK_FATAL( "Can't unlock mutex of DB Free Page List." );

    return IDE_FAILURE;
}



/*
 * Free Page List를 변경한다.
 *
 * Free Page List 변경 전에 로깅도 함께 실시한다.
 *
 * 주의! 동시성 제어의 책임이 이 함수의 호출자에게 있다.
 *       이 함수에서는 동시성 제어를 하지 않는다.
 *
 * aFPLNo           [IN] 변경할 Free Page List
 * aFirstFreePageID [IN] 새로 설정할 첫번째 Free Page
 * aFreePageCount   [IN] 새로 설정할 Free Page의 수
 *
 */
IDE_RC svmFPLManager::setFreePageList( svmTBSNode * aTBSNode,
                                       svmFPLNo     aFPLNo,
                                       scPageID     aFirstFreePageID,
                                       vULong       aFreePageCount )
{
    svmDBFreePageList * sFPL ;

    IDE_DASSERT( aFPLNo < aTBSNode->mMemBase.mFreePageListCount );

    IDE_DASSERT( smmManager::isValidPageID( aTBSNode->mTBSAttr.mID,
                                            aFirstFreePageID )
                 == ID_TRUE );

    sFPL = & aTBSNode->mMemBase.mFreePageLists[ aFPLNo ] ;

    IDE_DASSERT( svmFPLManager::isValidFPL( aTBSNode->mTBSAttr.mID, sFPL )
                 == ID_TRUE );

    sFPL->mFirstFreePageID = aFirstFreePageID;
    sFPL->mFreePageCount = aFreePageCount;

    IDE_DASSERT( svmFPLManager::isValidFPL( aTBSNode->mHeader.mID, sFPL ) == ID_TRUE );

    return IDE_SUCCESS;
}




/*
 * Free Page List 에서 하나의 Free Page를 떼어낸다.
 *
 * aHeadPAge부터 aTailPage까지
 * Free List Info Page의 Next Free Page ID로 연결된 채로 리턴한다.
 *
 * 주의! 동시성 제어의 책임이 이 함수의 호출자에게 있다.
 *       이 함수에서는 동시성 제어를 하지 않는다.
 *       또한, 최소한 aPageCount만큼의 Free Page가
 *       aFPLNo Free Page List에 존재함을 확인한 후 이 함수를 호출해야 한다.
 *
 * aFPLNo     [IN] Free Page를 할당받을 Free Page List
 * aPageCount [IN] 할당받고자 하는 Page의 수
 * aHeadPage  [OUT] 할당받은 첫번째 Page
 * aTailPage  [OUT] 할당받은 마지막 Page
 */
IDE_RC svmFPLManager::removeFreePagesFromList( svmTBSNode * aTBSNode,
                                               void *       aTrans,
                                               svmFPLNo     aFPLNo,
                                               vULong       aPageCount,
                                               scPageID *   aHeadPID,
                                               scPageID *   aTailPID )
{
    UInt                 i ;
    svmDBFreePageList  * sFPL ;
    scPageID             sSplitPID ;
    scPageID             sNewFirstFreePageID ;
    UInt                 sSlotNo;
    svmPageReservation * sPageReservation;
    UInt                 sState = 0;


    IDE_DASSERT( aFPLNo < aTBSNode->mMemBase.mFreePageListCount );

    IDE_DASSERT( aHeadPID != NULL );
    IDE_DASSERT( aTailPID != NULL );

    sFPL = & aTBSNode->mMemBase.mFreePageLists[ aFPLNo ] ;

    IDE_DASSERT( svmFPLManager::isValidFPL( aTBSNode->mHeader.mID, sFPL )
                 == ID_TRUE );

    /* BUG-31881 만약 자신이 Page를 예약해뒀다면, TBS에서 FreePage를
     * 가져가면서 가져간 만큼 예약을 취소함 */
    sPageReservation = &( aTBSNode->mArrPageReservation[ aFPLNo ] );
    IDE_TEST( findPageReservationSlot( sPageReservation,
                                       aTrans,
                                       &sSlotNo )
              != IDE_SUCCESS );

    if( sSlotNo != SVM_PAGE_RESERVATION_NULL )
    {
        /* 예약한 페이지를 전부 사용했다면 0으로 만듬. */
        if( sPageReservation->mPageCount[sSlotNo] < (SInt)aPageCount )
        {
            sPageReservation->mPageCount[sSlotNo] = 0;
        }
        else
        {
            sPageReservation->mPageCount[sSlotNo] -= aPageCount;
        }
        sState = 1;
    }

    // aPageCount번째 Page를 찾아서 그 ID를 sSplitPID에 설정한다.
    for ( i = 1, sSplitPID = sFPL->mFirstFreePageID;
          i < aPageCount;
          i++ )
    {
        IDE_ASSERT( sSplitPID != SM_NULL_PID );

        IDE_DASSERT( svmExpandChunk::isDataPageID( aTBSNode, sSplitPID )
                     == ID_TRUE );

        IDE_TEST( svmExpandChunk::getNextFreePage( aTBSNode,
                                                   sSplitPID,
                                                   &sSplitPID )
                  != IDE_SUCCESS );
    }

    // sSplitPID까지 떼어내고, 그 이후의 Page들을 Free Page List에 남겨 놓는다.
    IDE_TEST( svmExpandChunk::getNextFreePage( aTBSNode,
                                               sSplitPID,
                                               & sNewFirstFreePageID )
              != IDE_SUCCESS );

    * aHeadPID = sFPL->mFirstFreePageID ;
    * aTailPID = sSplitPID ;

    IDE_TEST( svmFPLManager::setFreePageList(
                  aTBSNode,
                  aFPLNo,
                  sNewFirstFreePageID,
                  sFPL->mFreePageCount - aPageCount )
              != IDE_SUCCESS );

    IDE_TEST( svmExpandChunk::setNextFreePage( aTBSNode,
                                               sSplitPID,
                                               SM_NULL_PID )
              != IDE_SUCCESS );

    IDE_DASSERT( svmFPLManager::isValidFPL( aTBSNode->mHeader.mID, *aHeadPID, aPageCount )
                 == ID_TRUE );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sState == 1 ) 
    {
        sPageReservation->mPageCount[sSlotNo] += aPageCount;
    }

    return IDE_FAILURE;
}



/*
 * Free Page List 에 하나의 Free Page List를 붙인다.( Tx가 Free Page 반납 )
 *
 * aHeadPAge부터 aTailPage까지
 * Free List Info Page의 Next Free Page ID로 연결되어 있어야 한다..
 *
 * 주의! 동시성 제어의 책임이 이 함수의 호출자에게 있다.
 *       이 함수에서는 동시성 제어를 하지 않는다.
 *
 * aFPLNo     [IN] Free Page가 반납될 Free Page List
 * aPageCount [IN] 반납하고자 하는 Page의 수
 * aHeadPage  [OUT] 반납할 첫번째 Page
 * aTailPage  [OUT] 반납할 마지막 Page
 */
IDE_RC svmFPLManager::appendFreePagesToList  ( svmTBSNode * aTBSNode,
                                               void *       aTrans,
                                               svmFPLNo     aFPLNo,
                                               vULong       aPageCount,
                                               scPageID     aHeadPID,
                                               scPageID     aTailPID )
{
    svmDBFreePageList   * sFPL ;
    UInt                  sSlotNo;
    svmPageReservation * sPageReservation;
    UInt                  sState = 0;


    IDE_DASSERT( aFPLNo < aTBSNode->mMemBase.mFreePageListCount );

    IDE_DASSERT( aPageCount != 0 );
    IDE_DASSERT( svmExpandChunk::isDataPageID( aTBSNode,
                                               aHeadPID )
                 == ID_TRUE );
    IDE_DASSERT( svmExpandChunk::isDataPageID( aTBSNode,
                                               aTailPID )
                 == ID_TRUE );


    sFPL = & aTBSNode->mMemBase.mFreePageLists[ aFPLNo ] ;

    IDE_DASSERT( svmFPLManager::isValidFPL( aTBSNode->mHeader.mID, sFPL )
                 == ID_TRUE );

    /* BUG-31881 만약 자신이 Page를 예약하려했다면,
     * TBS로 Page 반납하면서 동시에 예약함 */
    sPageReservation = &( aTBSNode->mArrPageReservation[ aFPLNo ] );
    IDE_TEST( findPageReservationSlot( sPageReservation,
                                       aTrans,
                                       &sSlotNo )
              != IDE_SUCCESS );

    if( sSlotNo != SVM_PAGE_RESERVATION_NULL )
    {
        sPageReservation->mPageCount[sSlotNo] += aPageCount;
        sState = 1;
    }

    // 현재 Free Page List 의 첫번째 Free Page를
    // 인자로 받은 Tail Page의 다음으로 매단다.
    IDE_TEST( svmExpandChunk::setNextFreePage( aTBSNode,
                                               aTailPID,
                                               sFPL->mFirstFreePageID )
              != IDE_SUCCESS );

    IDE_TEST( setFreePageList( aTBSNode,
                               aFPLNo,
                               aHeadPID,
                               sFPL->mFreePageCount + aPageCount )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sState == 1 ) 
    {
        if( sPageReservation->mPageCount[sSlotNo] < (SInt)aPageCount )
        {
            sPageReservation->mPageCount[sSlotNo] = 0;
        }
        else
        {
            sPageReservation->mPageCount[sSlotNo] -= aPageCount;
        }
    }

    return IDE_FAILURE;
}

/***************************************************************************
 * BUG-31881 [sm-mem-resource] When executing alter table in MRDB and 
 * using space by other transaction,
 * The server can not do restart recovery. 
 *
 * 지금부터 이 Transaction이 반환하는 Page개수 이상은 반드시 FreePage로
 * 가지고 있도록 하여, 이 Transaction이 다시 Page를 요구했을때 반드시
 * 해당 페이지들을 사용할 수 있도록 보장한다.
 *
 * [IN] aTBSNode - 예약될 Tablespace Node
 * [IN] aTrans   - 예약하는 Transaction
 ************************************************************************/
IDE_RC svmFPLManager::beginPageReservation( svmTBSNode * aTBSNode,
                                            void       * aTrans )
{
    UInt                 sFPLNo      = 0;
    UInt                 sSlotNo     = 0;
    svmPageReservation * sPageReservation;
    UInt                 sState      = 0;
    idBool               sSuccess    = ID_FALSE;
    PDL_Time_Value       sTV;
    UInt                 sSleepTime  = 5000;

    smLayerCallback::allocRSGroupID( aTrans, &sFPLNo );

    while( sSuccess == ID_FALSE )
    {
        IDE_TEST( lockFreePageList( aTBSNode, sFPLNo ) != IDE_SUCCESS );
        sState = 1;

        sPageReservation = &( aTBSNode->mArrPageReservation[ sFPLNo ] );

        /* 혹시 이미 등록되어있는지 확인한다. */
        IDE_TEST( findPageReservationSlot( sPageReservation,
                                           aTrans,
                                           &sSlotNo )
                  != IDE_SUCCESS );

        if( sSlotNo == SVM_PAGE_RESERVATION_NULL )
        {
            sSlotNo       = sPageReservation->mSize;
            if( sSlotNo < SVM_PAGE_RESERVATION_MAX )
            {
                sPageReservation->mTrans[ sSlotNo ]     = aTrans;
                sPageReservation->mPageCount[ sSlotNo ] = 0;
                sPageReservation->mSize ++;
                sSuccess = ID_TRUE;
            }
            else
            {
                sSuccess = ID_FALSE;
            }
        }
        else
        {
            /* 이미 등록되어있다면 그 객체를 쓴다. 하지만 예상외의 상황
             * 이기에, Debug모드면 비정상종료 시킨다. */
#if defined(DEBUG)
            IDE_RAISE( error_internal );
#endif
            sSuccess = ID_TRUE;
        }

        IDE_TEST( unlockFreePageList( aTBSNode, sFPLNo ) != IDE_SUCCESS );

        if( sSuccess == ID_FALSE )
        {
            /* 3600초 (1시간) 이상 재시도 했는데도 안되는 경우는 문제
             * 있는 경우이다. 따라서 빠져나간다. */
            IDE_TEST_RAISE( sSleepTime > 3600000, error_internal );
 
            /* 동시에 한 TBS에 SVM_PAGE_RESERVATION_MAX 개수(64)이상의 
             * AlterTable이 일어날 가능성은 거의 없다.
             * 따라서 기다려보고 재시도 한다. */
            sTV.set(0, sSleepTime );
            idlOS::sleep(sTV);

            sSleepTime <<= 1;
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( error_internal );
    {
        ideLog::logCallStack( IDE_SM_0 );

        ideLog::log( IDE_SM_0,
                     "sFPLNo     : %u\n"
                     "sSlotNo    : %u\n",
                     sFPLNo,
                     sSlotNo );

        dumpPageReservation( sPageReservation );

        IDE_SET( ideSetErrorCode( smERR_ABORT_INTERNAL ) );

        IDE_DASSERT( 0 );
    }

    IDE_EXCEPTION_END;

    if( sState == 1 )
    {
        (void) unlockFreePageList( aTBSNode, sFPLNo );
    }

    return IDE_FAILURE;
}

/***************************************************************************
 * 페이지 예약을 종료한다.
 *
 * [IN] aTBSNode - 예약했던 Tablespace Node
 * [IN] aTrans   - 예약했던 Transaction
 ************************************************************************/
IDE_RC svmFPLManager::endPageReservation( svmTBSNode * aTBSNode,
                                          void       * aTrans )
{
    UInt                 sFPLNo;
    UInt                 sSlotNo;
    UInt                 sLastSlotNo;
    svmPageReservation * sPageReservation;
    UInt                 sState = 0;

    smLayerCallback::allocRSGroupID( aTrans, &sFPLNo );

    IDE_TEST( lockFreePageList( aTBSNode, sFPLNo ) != IDE_SUCCESS );
    sState = 1;

    sPageReservation = &( aTBSNode->mArrPageReservation[ sFPLNo ] );

    IDE_TEST( findPageReservationSlot( sPageReservation,
                                       aTrans,
                                       &sSlotNo )
              != IDE_SUCCESS );

    if( sSlotNo != SVM_PAGE_RESERVATION_NULL ) 
    {
        /* 예약한 페이지들이 부족했을 수도 있고 남았을 수도 있다.
         * NewTable이 OldTable보다 Page를 더 사용했을수도, 덜 사용했을 수도
         * 있기 때문이다. */
        /* 마지막 Slot을 지우려는 Slot으로 이동시켜 지울 대상을 정리함*/
        IDE_TEST_RAISE( sPageReservation->mSize == 0, error_internal );

        sLastSlotNo = sPageReservation->mSize - 1;
        sPageReservation->mTrans[     sSlotNo ] 
            = sPageReservation->mTrans[     sLastSlotNo ];
        sPageReservation->mPageCount[ sSlotNo ] 
            = sPageReservation->mPageCount[ sLastSlotNo ];

        sPageReservation->mSize --;
    }
    else
    {
        /* 만약 RestartRecovery등의 경우, Backup은 안하고 restore만 하게 된다.
         * 그러면 Runtime상으로만 존재하는 PageReservation은 없기에 정상상황
         * 이다. */
        /* 참고로 Volatile이지만, Alter Table은 DDL연산이기에 Logging된다*/
        /* BUG-39689  alter table은 성공하여 PageReservation 정리가 끝난다음에
         * tran abort가 되면 PageReservation은 없을 수 있다. 정상 */
    }

    sState = 0;
    IDE_TEST( unlockFreePageList( aTBSNode, sFPLNo ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( error_internal );
    {
        ideLog::logCallStack( IDE_SM_0 );

        ideLog::log( IDE_SM_0,
                     "sFPLNo     : %u\n"
                     "sSlotNo    : %u\n",
                     sFPLNo,
                     sSlotNo );

        dumpPageReservation( sPageReservation );

        IDE_SET( ideSetErrorCode( smERR_ABORT_INTERNAL ) );

        IDE_DASSERT( 0 );
    }

    IDE_EXCEPTION_END;

    if( sState == 1 )
    {
        (void) unlockFreePageList( aTBSNode, sFPLNo );
    }

    return IDE_FAILURE;
}

/***************************************************************************
 * 해당 Transaction이 예약한 Slot을 찾는다.
 *
 *  << mArrFPLMutex가 잡힌 상태로 호출되어야 함!! >>
 *
 * [IN]  aPageReservation - 해당 PageList의 PageReservation 개체
 * [IN]  aTrans           - 예약했던 Transaction
 * [OUT] aSlotNo          - 해당 Transaction이 사용한 Slot의 번호
 ************************************************************************/
IDE_RC svmFPLManager::findPageReservationSlot( 
    svmPageReservation  * aPageReservation,
    void                * aTrans,
    UInt                * aSlotNo )
{
    UInt   sSlotNo; 
    UInt   i;

    /* TSB 확장등의 이유로, aTrans는 Null일 수 있음. */
    IDE_TEST_RAISE( aPageReservation == NULL, error_argument );
    IDE_TEST_RAISE( aSlotNo          == NULL, error_argument );

    sSlotNo = SVM_PAGE_RESERVATION_NULL ;

    for( i = 0 ; i < aPageReservation->mSize ; i ++ )
    {
        if( aPageReservation->mTrans[ i ] == aTrans )
        {
            sSlotNo = i;
            break;
        }
    }
    
    *aSlotNo = sSlotNo;

    return IDE_SUCCESS;

    IDE_EXCEPTION( error_argument );
    {
        ideLog::logCallStack( IDE_SM_0 );

        ideLog::log( IDE_SM_0,
                     "aPageReservation : %lu\n"
                     "aTrans           : %lu\n"
                     "aSlotNo          : %u\n",
                     aPageReservation,
                     aTrans,
                     aSlotNo );

        IDE_SET( ideSetErrorCode( smERR_ABORT_INTERNAL ) );

        IDE_DASSERT( 0 ); 
    }
 
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***************************************************************************
 * 자신 외 다른 Transaction들이 예약해둬서 남겨둬야하는 사용 못하는 Page
 * 의 개수를 가져온다.
 *
 *  << mArrFPLMutex가 잡힌 상태로 호출되어야 함!! >>
 *
 * [IN]  aPageReservation   - 해당 PageList의 PageReservation 개체
 * [IN]  aTrans             - 예약했던 Transaction
 * [OUT] aUnusablePageCount - 자신이 사용 못하는 페이지의 개수
 ************************************************************************/
IDE_RC svmFPLManager::getUnusablePageCount( 
    svmPageReservation  * aPageReservation,
    void                * aTrans,
    UInt                * aUnusablePageCount )
{
    UInt   sUnusablePageCount; 
    UInt   i;

    /* Trans는 Null이어도 됨. */
    IDE_TEST_RAISE( aPageReservation   == NULL, error_argument );
    IDE_TEST_RAISE( aUnusablePageCount == NULL, error_argument );

    sUnusablePageCount = 0;

    for( i = 0 ; i < aPageReservation->mSize ; i ++ )
    {
        if( ( aPageReservation->mTrans[ i ] != aTrans ) &&
            ( aPageReservation->mPageCount[ i ] > 0 ) )
        {
            sUnusablePageCount += aPageReservation->mPageCount[ i ];
        }    
    }
    
    *aUnusablePageCount = sUnusablePageCount;

    return IDE_SUCCESS;

    IDE_EXCEPTION( error_argument );
    {
        ideLog::logCallStack( IDE_SM_0 );
        ideLog::log( IDE_SM_0,
                     "aPageReservation   : %lu\n"
                     "aUnusablePageCount : %u\n",
                     aPageReservation,
                     aUnusablePageCount );
        IDE_SET( ideSetErrorCode( smERR_ABORT_INTERNAL ) );

        IDE_DASSERT( 0 ); 
    }
    
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***************************************************************************
 * 혹시 이 Transaction과 관련된 예약 페이지가 있으면, 모두 정리한다.
 *
 * [IN]  aTrans             - 탐색할 Transaction
 ************************************************************************/
IDE_RC svmFPLManager::finalizePageReservation( void      * aTrans,
                                               scSpaceID   aSpaceID )
{
    sctTableSpaceNode   * sCurTBS;
    svmTBSNode          * sVolTBS;
    UInt                  sFPLNo;
    svmPageReservation  * sPageReservation;
    UInt                  sSlotNo;
    UInt                  sState = 0;

    smLayerCallback::allocRSGroupID( aTrans, &sFPLNo );

    IDE_TEST( sctTableSpaceMgr::findSpaceNodeBySpaceID( aSpaceID,
                                                        (void**)&sCurTBS )
              != IDE_SUCCESS );

    if( (sCurTBS != NULL ) &&
        (sctTableSpaceMgr::isVolatileTableSpace( sCurTBS->mID ) == ID_TRUE ) )
    {
        sVolTBS = (svmTBSNode*)sCurTBS;
        IDE_TEST( lockFreePageList( sVolTBS, sFPLNo ) != IDE_SUCCESS );
        sState = 1;

        sPageReservation = &( sVolTBS->mArrPageReservation[ sFPLNo ] );

        IDE_TEST( findPageReservationSlot( sPageReservation,
                                           aTrans,
                                           &sSlotNo )
                  != IDE_SUCCESS );

        sState = 0;
        IDE_TEST( unlockFreePageList( sVolTBS, sFPLNo ) != IDE_SUCCESS );

        /* 만약 존재할 경우, Release모드에서는 정리해주고 Debug모드
         * 에서는 비정상 종료시켜 상황을 보고한다. */
        if( sSlotNo != SVM_PAGE_RESERVATION_NULL )
        {
            IDE_DASSERT( 0 );
            IDE_TEST( endPageReservation( sVolTBS, aTrans )
                      != IDE_SUCCESS );
        }
    }

    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;

    if( sState == 1 )
    {
        (void) unlockFreePageList( sVolTBS, sFPLNo );
    }

    return IDE_FAILURE;
}

IDE_RC svmFPLManager::dumpPageReservationByBuffer( 
    svmPageReservation * aPageReservation,
    SChar              * aOutBuf,
    UInt                 aOutSize )
{
    UInt i;

    IDE_TEST_RAISE( aPageReservation == NULL, error_argument );
    IDE_TEST_RAISE( aOutBuf          == NULL, error_argument );

    idlOS::snprintf( aOutBuf,
                     aOutSize,
                     "dump - VolPageReservation\n"
                     "size : %"ID_UINT32_FMT"\n",
                     aPageReservation->mSize );
 
    for( i = 0 ; i < SVM_PAGE_RESERVATION_MAX ; i ++ )
    {
        idlVA::appendFormat( aOutBuf,
                             aOutSize,
                             "[%2"ID_UINT32_FMT"] "
                             "TransPtr  :8%"ID_vULONG_FMT" "
                             "PageCount :4%"ID_INT32_FMT"\n",
                             i,
                             aPageReservation->mTrans[ i ],
                             aPageReservation->mPageCount [ i ] );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( error_argument );
    {
        ideLog::logCallStack( IDE_SM_0 );

        ideLog::log( IDE_SM_0,
                     "aPageReservation   : %lu\n"
                     "aOutBuf            : %lu\n"
                     "aOutSize           : %u\n",
                     aPageReservation,
                     aOutBuf,
                     aOutSize );

        IDE_SET( ideSetErrorCode( smERR_ABORT_INTERNAL ) );

        IDE_DASSERT( 0 );
    }   
    IDE_EXCEPTION_END;

    return IDE_FAILURE;   
}

void svmFPLManager::dumpPageReservation( 
    svmPageReservation * aPageReservation )
{
    SChar          * sTempBuffer;

    if( iduMemMgr::calloc( IDU_MEM_SM_SVM,
                           1,
                           ID_SIZEOF( SChar ) * IDE_DUMP_DEST_LIMIT,
                           (void**)&sTempBuffer )
        == IDE_SUCCESS )
    {
        sTempBuffer[0] = '\0';
        (void) dumpPageReservationByBuffer( aPageReservation, 
                                            sTempBuffer,
                                            IDE_DUMP_DEST_LIMIT );

        ideLog::log( IDE_SERVER_0,
                     "%s\n",
                     sTempBuffer );

        (void) iduMemMgr::free( sTempBuffer );
    }
}

/*
 * 특정 Free Page List에 Latch를 획득한 후 특정 갯수 이상의
 * Free Page가 있음을 보장한다.
 */
IDE_RC svmFPLManager::lockListAndPreparePages( svmTBSNode * aTBSNode,
                                               void *       aTrans,
                                               svmFPLNo     aFPLNo,
                                               vULong       aPageCount )
{
    svmPageReservation * sPageReservation;
    UInt              sUnusablePageSize = 0;
    UInt              sStage            = 0;

    IDE_DASSERT( aFPLNo < aTBSNode->mMemBase.mFreePageListCount );
    IDE_DASSERT( aPageCount != 0 );

    IDE_TEST( lockFreePageList( aTBSNode, aFPLNo ) != IDE_SUCCESS );
    sStage = 1;

    /* 다른 Transaction들이 예약해둔 Page는 사용할 수 없음. */
    sPageReservation = &( aTBSNode->mArrPageReservation[ aFPLNo ] );
    IDE_TEST( getUnusablePageCount( sPageReservation,
                                    aTrans,
                                    &sUnusablePageSize )
              != IDE_SUCCESS );

    // 만약 Free Page수가 모자라다면,
    // Free Page List에 Free Page를 새로 달아준다.
    // 이때 사용 불가능한 페이지 이상으로 FreePage가 있어야 한다.
    while ( aTBSNode->mMemBase.mFreePageLists[ aFPLNo ].mFreePageCount 
            < aPageCount + sUnusablePageSize )
    {
        IDE_DASSERT( svmFPLManager::isValidFPL(
                         aTBSNode->mHeader.mID,
                         & aTBSNode->mMemBase.mFreePageLists[ aFPLNo ] ) == ID_TRUE );

        sStage = 0;
        IDE_TEST( unlockFreePageList( aTBSNode, aFPLNo ) != IDE_SUCCESS );

        IDE_TEST( expandFreePageList( aTBSNode, 
                                      aTrans,
                                      aFPLNo, 
                                      aPageCount + sUnusablePageSize )
                  != IDE_SUCCESS );

        IDE_TEST( lockFreePageList( aTBSNode, aFPLNo ) != IDE_SUCCESS );
        sStage = 1;
    }

    IDE_DASSERT( svmFPLManager::isValidFPL(
                     aTBSNode->mHeader.mID,
                     & aTBSNode->mMemBase.mFreePageLists[ aFPLNo ] ) == ID_TRUE );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();

    switch ( sStage )
    {
        case 1:
            IDE_ASSERT( unlockFreePageList(aTBSNode, aFPLNo) == IDE_SUCCESS );
        default:
            break;
    }

    IDE_POP();

    return IDE_FAILURE;
}


/*
 * Free Page List에 Free Page들을 append한다.
 *
 * 우선, 다른 Free Page List 에 Free Page가 충분히 많다면,
 * 해당 Free Page List에서 Free Page 절반을 뚝 떼어다가 붙인다.
 *
 * 그렇지 않다면, 데이터베이스를 확장한다.
 *
 * 주의! 이 함수가 필요한 Page만큼 Free Page를 확보함을 보장하지 못한다.
 * 왜냐하면 aShortFPLNo에 대해 Latch를 풀고 리턴하기 때문임.
 *
 * 모든 Free Page List에 대해 Latch가 풀린채로 불리운다.
 *
 * aShortFPLNo        [IN] Free Page를 필요로 하는 Free Page List
 * aRequiredFreePageCnt [IN] 필요한 Free Page List의 수
 */
IDE_RC svmFPLManager::expandFreePageList( svmTBSNode * aTBSNode,
                                          void       * aTrans,
                                          svmFPLNo     aExpandingFPLNo,
                                          vULong       aRequiredFreePageCnt )
{
    svmFPLNo            sLargestFPLNo ;
    svmDBFreePageList * sLargestFPL ;
    svmDBFreePageList * sExpandingFPL ;
    vULong              sSplitPageCnt;
    scPageID            sSplitHeadPID, sSplitTailPID;
    UInt                sStage = 0;
    svmFPLNo            sListNo1 = 0, sListNo2 = 0;

    IDE_DASSERT( aExpandingFPLNo < aTBSNode->mMemBase.mFreePageListCount );
    IDE_DASSERT( aRequiredFreePageCnt != 0 );

    sExpandingFPL = & aTBSNode->mMemBase.mFreePageLists[ aExpandingFPLNo ];

    // Latch 잡지 않은 상태로 Free Page가장 많이 지닌 Free Page List를 가져온다
    IDE_TEST( getLargestFreePageList( aTBSNode, & sLargestFPLNo )
              != IDE_SUCCESS );

    sLargestFPL = & aTBSNode->mMemBase.mFreePageLists[ sLargestFPLNo ] ;

    // 우선 Latch 안 잡은 채로 Free Page List Split이 가능한지 검사해본다.
    if (
          // Free Page를 필요로 하는 FPL ( aExpandingFPLNo )에
          // Free Page가 가장 많은 경우
          // sLargestFPLNo 계산한 것이 aExpandingFPLNo 과 동일할 수 있음..
          // 이 때에는 Free Page를 빼앗아 올 수가 없다.
          aExpandingFPLNo != sLargestFPLNo
          &&
          // Free Page List가 Split가능한 최소한의 Page를 가지있는지 검사
          // (이 검사를 하지 않으면 Free Page가 몇개 없는 Free Page List까지
          //  Split을 빈번하게 하게 되어 시스템 성능이 떨어질 수 있다 )
          ( sLargestFPL->mFreePageCount > SVM_FREE_PAGE_SPLIT_THRESHOLD )
          &&
          // 절반을 떼었을 때 필요한 Free Page수를 얻을 수 있는지 검사
          ( sLargestFPL->mFreePageCount > aRequiredFreePageCnt * 2 ) )
    {
        // 데드락 방지를 위해 Mutex를 No가 작은 List부터 잡는다.
        if ( aExpandingFPLNo < sLargestFPLNo )
        {
            sListNo1 = aExpandingFPLNo ;
            sListNo2 = sLargestFPLNo ;
        }
        else
        {
            sListNo1 = sLargestFPLNo ;
            sListNo2 = aExpandingFPLNo ;
        }


        IDE_TEST( lockFreePageList( aTBSNode, sListNo1 ) != IDE_SUCCESS );
        sStage = 1;

        IDE_TEST( lockFreePageList( aTBSNode, sListNo2 ) != IDE_SUCCESS );
        sStage = 2;

        IDE_DASSERT( svmFPLManager::isValidFPL( aTBSNode->mHeader.mID,
                                                sLargestFPL ) == ID_TRUE );
        IDE_DASSERT( svmFPLManager::isValidFPL( aTBSNode->mHeader.mID,
                                                sExpandingFPL ) == ID_TRUE );

        // List Split이 가능한지 조건을 다시 검사한다.
        // (위에서 조건 검사한 것은 Latch를 안잡고 검사한 값이라
        //  Split이 가능한지를 확신할 수 없음. )
        if ( ( sLargestFPL->mFreePageCount > SVM_FREE_PAGE_SPLIT_THRESHOLD )
             &&
             ( sLargestFPL->mFreePageCount > aRequiredFreePageCnt * 2 ) )
        {
            // Free Page List의 Page 절반을 떼어낸다.
            sSplitPageCnt = sLargestFPL->mFreePageCount / 2 ;

            IDE_TEST( removeFreePagesFromList( aTBSNode,
                                               aTrans,
                                               sLargestFPLNo,
                                               sSplitPageCnt,
                                               & sSplitHeadPID,
                                               & sSplitTailPID )
                      != IDE_SUCCESS );

            // Free Page 를 필요로 하는 Free Page List에 붙여준다.
            IDE_TEST( appendFreePagesToList ( aTBSNode,
                                              aTrans,
                                              aExpandingFPLNo,
                                              sSplitPageCnt,
                                              sSplitHeadPID,
                                              sSplitTailPID )
                      != IDE_SUCCESS );
        }

        IDE_DASSERT( svmFPLManager::isValidFPL( aTBSNode->mHeader.mID,
                                                sLargestFPL ) == ID_TRUE );
        IDE_DASSERT( svmFPLManager::isValidFPL( aTBSNode->mHeader.mID,
                                                sExpandingFPL ) == ID_TRUE );

        sStage = 1;
        IDE_TEST( unlockFreePageList( aTBSNode, sListNo2 ) != IDE_SUCCESS );

        sStage = 0;
        IDE_TEST( unlockFreePageList( aTBSNode, sListNo1 ) != IDE_SUCCESS );
    }


    // 필요한 Page 수만큼 확보가 될 때까지
    // Expand Chunk를 추가해가며 데이터베이스를 확장한다.
    if( sExpandingFPL->mFreePageCount < aRequiredFreePageCnt )
    {
        // Tablespace의 NEXT크기만큼 Chunk확장을 시도한다.
        // MEM_MAX_DB_SIZE나 Tablespace의 MAXSIZE제한에 걸릴 경우 에러발생
        IDE_TEST( expandOrWait( aTBSNode ) != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();
    {
        switch( sStage )
        {
            case 2 :
                IDE_ASSERT( unlockFreePageList( aTBSNode,
                                                sListNo2 ) == IDE_SUCCESS );
            case 1 :
                IDE_ASSERT( unlockFreePageList( aTBSNode,
                                                sListNo1 ) == IDE_SUCCESS );
            default:
                break;
        }
    }
    IDE_POP();

    return IDE_FAILURE;
}


/*
   svmFPLManager::getTotalPageCount4AllTBS를 위한 Action함수

   [IN]  aTBSNode   - Tablespace Node
   [OUT] aActionArg - Total Page 수를 세어나갈 변수의 포인터
 */
IDE_RC svmFPLManager::aggregateTotalPageCountAction(
                          idvSQL*             /*aStatistics*/,
                          sctTableSpaceNode * aTBSNode,
                          void * aActionArg  )
{
    scPageID * sTotalPageCount = (scPageID*) aActionArg;

    if ( sctTableSpaceMgr::isVolatileTableSpace(aTBSNode->mID) == ID_TRUE )
    {
        if ( sctTableSpaceMgr::hasState( aTBSNode,
                                         SCT_SS_SKIP_COUNTING_TOTAL_PAGES )
             == ID_TRUE )
        {
            // do nothing
            // DROP된  Tablespace는
            // 사용중인 Page Memory가 없으므로,
            // TOTAL PAGE수를 세지 않는다.
        }
        else
        {
            *sTotalPageCount += (UInt) svmDatabase::getAllocPersPageCount(
                &((svmTBSNode*)aTBSNode)->mMemBase);
        }
    }

    return IDE_SUCCESS;

}



/*
   모든 Tablepsace에 대해 OS로부터 할당한 Page의 수의 총합을 반환한다.

   [OUT] aTotalPageCount - 모든 Tablespace의 할당된 Page수의 총합
 */
IDE_RC svmFPLManager::getTotalPageCount4AllTBS( scPageID * aTotalPageCount )
{
    scPageID sTotalPageCount = 0;

    IDE_TEST( sctTableSpaceMgr::doAction4EachTBS(
                                    NULL, /* idvSQL* */
                                    aggregateTotalPageCountAction,
                                    & sTotalPageCount, /* Action Argument*/
                                    SCT_ACT_MODE_NONE)
              != IDE_SUCCESS );

    * aTotalPageCount = sTotalPageCount ;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

/*
    Tablespace의 NEXT 크기만큼 Chunk확장을 시도한다.

    MEM_MAX_DB_SIZE의 한계나 Tablespace의 MAXSIZE한계를 넘어서면 에러

    [IN] aTBSNode - Chunk확장하려는 Tablespace Node

    [동시성] Mutex잡는 순서 : TBSNode.mAllocChunkMutex => mTBSAllocChunkMutex
 */
IDE_RC svmFPLManager::tryToExpandNextSize(svmTBSNode * aTBSNode)
{
    UInt       sState = 0;
    SChar    * sTBSName;
    scPageID   sTotalPageCount   = 0;
    scPageID   sTBSNextPageCount = 0;
    scPageID   sTBSMaxPageCount  = 0;


    vULong     sNewChunkCount;
    ULong      sTBSCurrentSize;
    ULong      sTBSNextSize;
    ULong      sTBSMaxSize;

    scPageID sExpandChunkPageCount = smuProperty::getExpandChunkPageCount();

    // BUGBUG-1548 aTBSNode->mTBSAttr.mName을 0으로 끝나는 문자열로 만들것
    sTBSName          = aTBSNode->mHeader.mName;
    sTBSNextPageCount = aTBSNode->mTBSAttr.mVolAttr.mNextPageCount;
    sTBSMaxPageCount  = aTBSNode->mTBSAttr.mVolAttr.mMaxPageCount;

    IDE_TEST_RAISE( aTBSNode->mTBSAttr.mVolAttr.mIsAutoExtend == ID_FALSE,
                    error_unable_to_expand_cuz_auto_extend_mode );


    // 시스템에서 오직 하나의 Tablespace만이
    // Chunk확장을 하도록 하는 Mutex
    // => 두 개의 Tablespace가 동시에 Chunk확장하는 상황에서는
    //    모든 Tablespace의 할당한 Page 크기가 MEM_MAX_DB_SIZE보다
    //    작은 지 검사할 수 없기 때문
    IDE_TEST( lockGlobalAllocChunkMutex() != IDE_SUCCESS );
    sState = 1;

    // Tablespace를 확장후 Database의 모든 Tablespace에 대해
    // 할당된 Page수의 총합이 MEM_MAX_DB_SIZE 프로퍼티에 허용된
    // 크기보다 더 크면 에러
    IDE_TEST( getTotalPageCount4AllTBS( & sTotalPageCount ) != IDE_SUCCESS );

    IDE_TEST_RAISE( ( sTotalPageCount + sTBSNextPageCount ) >
                    ( smuProperty::getVolMaxDBSize() / SM_PAGE_SIZE ),
                    error_unable_to_expand_cuz_mem_max_db_size );


    // ChunK확장후 크기가 Tablespace의 MAXSIZE한계를 넘어서면 에러
    sTBSCurrentSize =
        svmDatabase::getAllocPersPageCount( &aTBSNode->mMemBase ) *
        SM_PAGE_SIZE;

    sTBSNextSize = sTBSNextPageCount * SM_PAGE_SIZE;
    sTBSMaxSize  = sTBSMaxPageCount * SM_PAGE_SIZE;

    IDE_TEST_RAISE( ( sTBSCurrentSize  + sTBSNextSize )
                    > sTBSMaxSize,
                    error_unable_to_expand_cuz_tbs_max_size );

    // Next Page Count는 Expand Chunk Page수로 나누어 떨어져야 한다.
    IDE_ASSERT( (sTBSNextPageCount % sExpandChunkPageCount) == 0 );
    sNewChunkCount = sTBSNextPageCount / sExpandChunkPageCount ;

    // 데이터베이스가 확장되면서 모든 Free Page List에
    // 골고루 Free Page들이 분배되어 매달린다.
    IDE_TEST( svmManager::allocNewExpandChunks( aTBSNode,
                                                sNewChunkCount )
              != IDE_SUCCESS );

    sState = 0;
    IDE_TEST( mGlobalAllocChunkMutex.unlock() != IDE_SUCCESS );


    return IDE_SUCCESS;

    IDE_EXCEPTION( error_unable_to_expand_cuz_auto_extend_mode );
    {
        IDE_SET(ideSetErrorCode( smERR_ABORT_UNABLE_TO_EXTEND_CHUNK_WHEN_AUTO_EXTEND_OFF, sTBSName ));

        /* BUG-40980 : AUTOEXTEND OFF상태에서 TBS max size에 도달하여 extend 불가능
         *             error 메시지를 altibase_sm.log에도 출력한다. */
        ideLog::log( IDE_SM_0, 
                     "Unable to extend the tablespace(%s) when AUTOEXTEND mode is OFF",
                     sTBSName);

    
    }
    IDE_EXCEPTION( error_unable_to_expand_cuz_mem_max_db_size );
    {
        // KB 단위의 MEM_MAX_DB_SIZE 프로퍼티값
        IDE_SET(ideSetErrorCode( smERR_ABORT_UNABLE_TO_EXTEND_CHUNK_MORE_THAN_VOLATILE_MAX_DB_SIZE, sTBSName, (ULong) (smuProperty::getVolMaxDBSize()/1024)  ));
    }
    IDE_EXCEPTION( error_unable_to_expand_cuz_tbs_max_size );
    {
        // BUG-28521 [SM] 레코드 삽입이 공간 부족으로 에러가 발생할때
        //                에러 메세지가 이상합니다.
        //
        // Size 단위를 kilobyte단위로 통일하기 위해서 1024로 나눕니다.
        IDE_SET(ideSetErrorCode( smERR_ABORT_UNABLE_TO_EXTEND_CHUNK_MORE_THAN_TBS_MAXSIZE, sTBSName, (sTBSCurrentSize/1024), (sTBSMaxSize/1024) ));
    }
    IDE_EXCEPTION_END;

    IDE_PUSH();
    {
        switch( sState )
        {
            case 1:

                IDE_ASSERT( mGlobalAllocChunkMutex.unlock() == IDE_SUCCESS );

                break;

            default:
                break;
        }
    }
    IDE_POP();

    return IDE_FAILURE;

}




/*
 * 동시에 두개의 트랜잭션이 Chunk를 할당받는 일을 방지한다.
 *
 * 모든 Free Page List에 대해 latch가 풀린채로 이 함수가 불린다.
 *
 * [IN] aTBSNode       - Chunk를 확장할 Tablespace의 Node
 *
 * [동시성] Mutex잡는 순서 : TBSNode.mAllocChunkMutex => mTBSAllocChunkMutex
 */

IDE_RC svmFPLManager::expandOrWait(svmTBSNode * aTBSNode)
{

    idBool sIsLocked ;

    IDE_TEST( aTBSNode->mAllocChunkMutex.trylock( sIsLocked ) != IDE_SUCCESS );

    if ( sIsLocked == ID_TRUE ) // 아직 Expand Chunk할당중인 트랜잭션 없을 때
    {
        // Tablespace의 NEXT 크기만큼 Chunk확장 실시
        // MEM_MAX_DB_SIZE의 한계나
        // Tablespace의 MAXSIZE한계를 넘어서면 에러 발생
        IDE_TEST( tryToExpandNextSize( aTBSNode )
                  != IDE_SUCCESS );

        sIsLocked = ID_FALSE;
        IDE_TEST( aTBSNode->mAllocChunkMutex.unlock() != IDE_SUCCESS );
    }
    else // 현재 Expand Chunk를 할당중인 트랜잭션이 있을 때
    {
        // Expand Chunk할당이 끝나기를 기다린다.
        IDE_TEST( lockAllocChunkMutex(aTBSNode) != IDE_SUCCESS );
        // 다른 트랜잭션이 Expand Chunk할당을 하였으므로, 별도로
        // Expand Chunk할당을 하지 않는다.
        IDE_TEST( unlockAllocChunkMutex(aTBSNode) != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();
    {
        if ( sIsLocked == ID_TRUE )
        {
            IDE_ASSERT( unlockAllocChunkMutex(aTBSNode) == IDE_SUCCESS );
        }
    }
    IDE_POP();

    return IDE_FAILURE;
}


/*
 * Free Page List의 첫번째 Page ID와 Page수의 validity를 체크한다.
 *
 * aFirstPID  - [IN] Free Page List의 첫번째 Page ID
 * aPageCount - [IN] Free Page List에 속한 Page의 수
 */
idBool svmFPLManager::isValidFPL(scSpaceID    aSpaceID,
                                 scPageID     aFirstPID,
                                 vULong       aPageCount )
{
    idBool    sIsValid = ID_TRUE;
    svmTBSNode * sTBSNode;

    IDE_ASSERT(sctTableSpaceMgr::findSpaceNodeBySpaceID(aSpaceID,
                                                        (void**)&sTBSNode)
               == IDE_SUCCESS);

    if ( aFirstPID == SM_NULL_PID )
    {
        // PID는 NULL이면서 Page수가 0이 아니면 에러
        if ( aPageCount != 0 )
        {
            sIsValid = ID_FALSE;
        }
    }
    else
    {
        IDE_DASSERT( svmExpandChunk::isDataPageID( sTBSNode,
                                                   aFirstPID ) == ID_TRUE );
    }

#if defined( DEBUG_SVM_PAGE_LIST_CHECK )
    // Recovery중에는 Free Page List의 실제 연결된 Free Page수와
    // Free Page List에 기록된 Free Page 수가 같음을 보장할 수 없다.
    if ( aFirstPID != SM_NULL_PID &&
         smLayerCallback::isRestartRecoveryPhase() == ID_FALSE )
    {
        sPageCount = 0;

        sPID = aFirstPID;

        for(;;)
        {
            sPageCount ++;

            IDE_ASSERT( svmExpandChunk::getNextFreePage( sPID, & sNextPID )
                        == IDE_SUCCESS );
            if ( sNextPID == SM_NULL_PID )
            {
                break;
            }
            sPID = sNextPID ;
        }
#if defined( DEBUG_PAGE_ALLOC_FREE )
        if ( sPageCount != aPageCount )
        {
            idlOS::fprintf( stdout, "===========================================================================\n" );
            idlOS::fprintf( stdout, "Page Count Mismatch(List:%"ID_UINT64_FMT",Count:%"ID_UINT64_FMT")\n", (ULong)sPageCount, (ULong)aPageCount );
            idlOS::fprintf( stdout, "List First PID : %"ID_UINT64_FMT", List Last PID : %"ID_UINT64_FMT"\n", (ULong)aFirstPID, (ULong)sPID );
            idlOS::fprintf( stdout, "FLI Page ID of Last PID : %"ID_UINT64_FMT"\n",
                            (ULong)svmExpandChunk::getFLIPageID( sPID ) );
            idlOS::fprintf( stdout, "Slot offset in FLI Page of Last PID : %"ID_UINT32_FMT"\n",
                            (UInt) svmExpandChunk::getSlotOffsetInFLIPage( sPID ) );
            idlOS::fprintf( stdout, "===========================================================================\n" );
            idlOS::fflush( stdout );
        }
#endif
        // List의 링크를 따라가서 센 페이지 수와
        // 인자로 받은 페이지 수가 다르면 에러
        if ( sPageCount != aPageCount )
        {
            sIsValid = ID_FALSE;
        }
    }
#endif // DEBUG_SVM_PAGE_LIST_CHECK

    return sIsValid;
}


/*
 * Free Page List의 첫번째 Page ID와 Page수의 validity를 체크한다.
 *
 * aFPL - [IN] 검사하고자 하는 Free Page List
 */
idBool svmFPLManager::isValidFPL( scSpaceID           aSpaceID,
                                  svmDBFreePageList * aFPL )
{
    return isValidFPL( aSpaceID,
                       aFPL->mFirstFreePageID,
                       aFPL->mFreePageCount );
}

/*
 * 모든 Free Page List가 Valid한지 체크한다
 *
 */
idBool svmFPLManager::isAllFPLsValid( svmTBSNode * aTBSNode )
{
    svmFPLNo      sFPLNo ;

    for ( sFPLNo = 0;
          sFPLNo < aTBSNode->mMemBase.mFreePageListCount  ;
          sFPLNo ++ )
    {
        IDE_ASSERT( isValidFPL( aTBSNode->mHeader.mID,
                                &aTBSNode->mMemBase.mFreePageLists[ sFPLNo ] )
                    == ID_TRUE );
    }

    return ID_TRUE;
}

/*
 * 모든 Free Page List의 내용을 찍는다.
 *
 */
void svmFPLManager::dumpAllFPLs(svmTBSNode * aTBSNode)
{
    svmFPLNo            sFPLNo ;
    svmDBFreePageList * sFPL ;

    idlOS::fprintf( stdout, "===========================================================================\n" );

    for ( sFPLNo = 0;
          sFPLNo < aTBSNode->mMemBase.mFreePageListCount  ;
          sFPLNo ++ )
    {
        sFPL = & aTBSNode->mMemBase.mFreePageLists[ sFPLNo ];

        idlOS::fprintf(stdout, "FPL[%"ID_UINT32_FMT"]ID=%"ID_UINT64_FMT",COUNT=%"ID_UINT64_FMT"\n",
                       sFPLNo,
                       (ULong) sFPL->mFirstFreePageID,
                       (ULong) sFPL->mFreePageCount );
    }

    idlOS::fprintf( stdout, "===========================================================================\n" );

    idlOS::fflush( stdout );

    IDE_DASSERT( isAllFPLsValid(aTBSNode) == ID_TRUE );
}
