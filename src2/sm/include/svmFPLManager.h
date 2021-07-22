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
 
#ifndef _O_SVM_FPL_MANAGER_H_
#define _O_SVM_FPL_MANAGER_H_ 1

#include <svm.h>

/*
  PROJ-1490 페이지리스트 다중화및 메모리반납

  여러개로 다중화된 데이터베이스의 Free Page List 들을 관리한다.
  FPL은 Free Page List의 약자이다.
*/


// 여러개로 다중화된 Free Page들에 0부터 순차적으로 1씩 증가하며 매긴 번호
typedef UInt svmFPLNo ;
#define SVM_NULL_FPL_NO ( ID_UINT_MAX )


/*
 * 여러개의 페이지를 FreeListInfo Page안의 Next Free Page로 엮은 페이지 리스트
 */
typedef struct svmPageList
{
    scPageID mHeadPID;      // 첫번째 Page
    scPageID mTailPID;      // 마지막 Page
    vULong   mPageCount;    // 총 Page 수 
} svmPageList ;

// svmPageList를 초기화한다.
#define SVM_PAGELIST_INIT( list )     \
(list)->mHeadPID = SM_NULL_PID ;      \
(list)->mTailPID = SM_NULL_PID ;      \
(list)->mPageCount = 0 ;           

typedef enum svmFPLValidityOp
{
    SVM_FPL_VAL_OP_NORMAL = 1,
    SVM_FPL_VAL_OP_NO_COUNT_CHECK
} svmFPLValidityOp;



class svmFPLManager
{
private:

    // Free Page List를 변경한다.
    static IDE_RC setFreePageList( svmTBSNode * aTBSNode,
                                   svmFPLNo     aFPLNo,
                                   scPageID     aFirstFreePageID,
                                  vULong       aFreePageCount );
    
    // Free Page List에 Free Page들을 append한다.
    static IDE_RC expandFreePageList( svmTBSNode * aTBSNode,
                                      void       * aTrans,
                                      svmFPLNo     aFPLNo ,
                                      vULong       aRequiredFreePageCnt );

    //////////////////////////////////////////////////////////////////////////
    // Expand Chunk확장 관련 함수 
    //////////////////////////////////////////////////////////////////////////
    // svmFPLManager::getTotalPageCount4AllTBS를 위한 Action함수
    static IDE_RC aggregateTotalPageCountAction( idvSQL*  aStatistics,
                                                 sctTableSpaceNode * aTBSNode,
                                                 void * aActionArg  );
    

    // Tablespace의 NEXT 크기만큼 Chunk확장을 시도한다.
    static IDE_RC tryToExpandNextSize(svmTBSNode * aTBSNode);

    // 다른 트랜잭션에 의해서이던,
    // aTrans의해서 이던 상관없이 Tablespace의 NEXT 크기만큼의
    // Chunk가 생성됨을 보장한다.
    static IDE_RC expandOrWait(svmTBSNode * aTBSNode);

    // 모든 Tablespace의 현재 할당된 SIZE < VOLATILE_MAX_DB_SIZE인지
    // 검사하기 위해 Tablespace의 확장시 잡는 Mutex
    static iduMutex mGlobalAllocChunkMutex;
    
public :

    // Static Initializer
    static IDE_RC initializeStatic();

    // Static Destroyer
    static IDE_RC destroyStatic();
    
    // Free Page List 관리자를 초기화한다.
    static IDE_RC initialize(svmTBSNode * aTBSNode);


    // Free Page List 관리자를 파괴한다.
    static IDE_RC destroy(svmTBSNode * aTBSNode);
    
    //  Create Volatile Tablespace나 DML로 인해 Chunk확장 발생시 Mutex잡기
    static IDE_RC lockGlobalAllocChunkMutex();
    
    // Create Volatile Tablespace나 DML로 인해 Chunk확장 발생시 Mutex풀기 
    static IDE_RC unlockGlobalAllocChunkMutex();

    // 모든 Tablepsace에 대해 OS로부터 할당한 Page의 수의 총합을 반환한다.
    static IDE_RC getTotalPageCount4AllTBS( scPageID * aTotalPageCount );
    
    // Free Page를 가장 조금 가진 데이터베이스 Free Page List를 찾아낸다.
    static IDE_RC getShortestFreePageList( svmTBSNode * aTBSNode,
                                           svmFPLNo *   aFPLNo );

    // Free Page를 가장 많이 가진 데이터베이스 Free Page List를 찾아낸다.
    static IDE_RC getLargestFreePageList( svmTBSNode * aTBSNode,
                                          svmFPLNo *   aFPLNo );

    // 하나의 Free Page들을 데이터베이스 여러개의 Page List에 골고루 분배한다.
    static IDE_RC distributeFreePages( svmTBSNode * aTBSNode,
                                       scPageID      aChunkFirstFreePID,
                                       scPageID      aChunkLastFreePID,
                                       idBool        aSetNextFreePage,
                                       svmPageList * aArrFreePageLists );

    // Chunk 할당 Mutex에 Latch를 건다.
    static IDE_RC lockAllocChunkMutex(svmTBSNode * aTBSNode);
    // Chunk 할당 Mutex로부터 Latch를 푼다.
    static IDE_RC unlockAllocChunkMutex(svmTBSNode * aTBSNode);
    
    // Free Page List 에 래치를 건다.
    static IDE_RC lockFreePageList( svmTBSNode * aTBSNode,
                                    svmFPLNo     aFPLNo );
    
    // Free Page List 로부터 래치를 푼다.
    static IDE_RC unlockFreePageList( svmTBSNode * aTBSNode,
                                      svmFPLNo     aFPLNo );

    // 모든 Free Page List에 latch를 잡는다.
    static IDE_RC lockAllFPLs(svmTBSNode * aTBSNode);
    
    // 모든 Free Page List에서 latch를 푼다.
    static IDE_RC unlockAllFPLs(svmTBSNode * aTBSNode);
    
    /* BUG-31881 [sm-mem-resource] When executing alter table in MRDB and 
     * using space by other transaction,
     * The server can not do restart recovery. 
     * 지금부터 이 Transaction이 반환하는 Page들은 예약해둔다. */
    static IDE_RC beginPageReservation( svmTBSNode * aTBSNode,
                                        void       * aTrans );

    /* Page예약을 멈춘다. */
    static IDE_RC endPageReservation( svmTBSNode * aTBSNode,
                                      void       * aTrans );

    /* mArrFPLMutex가 잡힌 상태로 호출되어야 함 */
    /* 예약해둔 페이지를 찾는다. */
    static IDE_RC findPageReservationSlot( 
        svmPageReservation  * aPageReservation,
        void                * aTrans,
        UInt                * aSlotNo );

    /* mArrFPLMutex가 잡힌 상태로 호출되어야 함 */
    /* 자신 외 다른 Transaction들이 예약해둔 Page의 개수를 가져옴 */
    static IDE_RC getUnusablePageCount( 
        svmPageReservation  * aPageReservation,
        void                * aTrans,
        UInt                * aUnusablePageCount );

    /* 혹시 이 Transaction과 관련된 예약 페이지가 있으면, 모두 정리한다. */
    static IDE_RC finalizePageReservation( void      * aTrans,
                                           scSpaceID   aSpaceID );

    /* pageReservation 관련 내용을 dump 한다. */
    static IDE_RC dumpPageReservationByBuffer
        ( svmPageReservation * aPageReservation,
          SChar              * aOutBuf,
          UInt                 aOutSize );

    /* pageReservation 관련 내용을 SM TrcLog에 출력한다. */
    static void dumpPageReservation( svmPageReservation * aPageReservation );

    // 특정 Free Page List에 Latch를 획득한 후 특정 갯수 이상의
    // Free Page가 있음을 보장한다.
    static IDE_RC lockListAndPreparePages( svmTBSNode * aTBSNode,
                                           void       * aTrans,
                                           svmFPLNo     aFPLNo,
                                           vULong       aPageCount );

    
    // Free Page List 에서 하나의 Free Page를 떼어낸다.
    // aHeadPAge부터 aTailPage까지
    // Free List Info Page의 Next Free Page ID로 연결된 채로 리턴한다.
    static IDE_RC removeFreePagesFromList( svmTBSNode * aTBSNode,
                                           void *       aTrans,
                                           svmFPLNo     aFPLNo,
                                           vULong       aPageCount,
                                           scPageID  *  aHeadPID,
                                           scPageID  *  aTailPID );

    // Free Page List 에 하나의 Free Page List를 붙인다.
    // aHeadPage부터 aTailPage까지
    // Free List Info Page의 Next Free Page ID로 연결되어 있어야 한다..
    static IDE_RC appendFreePagesToList( svmTBSNode * aTBSNode,
                                         void *       aTrans,
                                         svmFPLNo     aFPLNo,
                                         vULong       aPageCount,
                                         scPageID     aHeadPID,
                                         scPageID     aTailPID );
    

    // Free Page 들을 각각의 Free Page List의 맨 앞에 append한다. 
    static IDE_RC appendPageLists2FPLs( svmTBSNode *  aTBSNode,
                                        svmPageList * aArrFreePageList );
    
    // 모든 Free Page List에 있는 Free Page 수를 리턴한다.
    static IDE_RC getTotalFreePageCount( svmTBSNode * aTBSNode,
                                         vULong *     aTotalFreePageCnt );

    // Free Page List의 첫번째 Page ID와 Page수의 validity를 체크한다.
    static idBool isValidFPL(scSpaceID        aSpaceID,
                             scPageID         aFirstPID,
                             vULong           aPageCount);

    // Free Page List의 첫번째 Page ID와 Page수의 validity를 체크한다.
    static idBool isValidFPL( scSpaceID           aSpaceID,
                              svmDBFreePageList * aFPL );

    // 모든 Free Page List가 Valid한지 체크한다
    static idBool isAllFPLsValid(svmTBSNode * aTBSNode);

    // 모든 Free Page List의 내용을 찍는다.
    static void dumpAllFPLs(svmTBSNode * aTBSNode);
};

// Chunk 할당 Mutex에 Latch를 건다.
inline IDE_RC svmFPLManager::lockAllocChunkMutex(svmTBSNode * aTBSNode)
{
    return aTBSNode->mAllocChunkMutex.lock(NULL);
}


// Chunk 할당 Mutex로부터 Latch를 푼다.
inline IDE_RC svmFPLManager::unlockAllocChunkMutex(svmTBSNode * aTBSNode)
{
    return aTBSNode->mAllocChunkMutex.unlock();
}





#endif /* _O_SVM_FPL_MANAGER_H_ */
