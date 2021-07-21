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
 * $Id: qmcMemSortTempTable.h 82075 2018-01-17 06:39:52Z jina.kim $
 *
 * Description :
 *
 * Memory Sort Temp Table
 *   - Memory Sort Temp Table 객체로 Quick Sort Algorithm을 사용한다.
 *
 * 용어 설명 :
 *
 * 약어 :
 *
 *
 **********************************************************************/

#ifndef _O_QMC_MEM_SORT_TEMP_TABLE_H_
#define _O_QMC_MEM_SORT_TEMP_TABLE_H_  1

#include <idl.h>
#include <iduMemory.h>
#include <idv.h>
#include <mt.h>
#include <qmc.h>
#include <qmcMemory.h>

#define QMC_MEM_SORT_MAX_ELEMENTS                       (1024)
#define QMC_MEM_SORT_SHIFT                                (10)
#define QMC_MEM_SORT_MASK          ID_LONG(0x00000000000003FF)

// BUG-41048 Improve orderby sort algorithm.
// 데이타 갯수가 16보다 적을때는 insertion sort 가 유리하다.
#define QMC_MEM_SORT_SMALL_SIZE                          (16)

// BUG-41048 Improve orderby sort algorithm.
// 3중위법에서 중간값을 정할때 소수를 이용한다.
#define QMC_MEM_SORT_PRIME_NUMBER                        (17)

//------------------------------------------------
// TASK-6445 Timsort
//------------------------------------------------

// Run의 최소값인 minrun의 상한선
// minrun의 하한선은 (MINRUN_FENCE / 2) 이다.
#define QMC_MEM_SORT_MINRUN_FENCE                        (64)
// Galloping Mode 진입 Winning Count 초기값
#define QMC_MEM_SORT_GALLOPING_WINCNT                     (7)
// Galloping Mode 진입 Winning Count의 최소값
#define QMC_MEM_SORT_GALLOPING_WINCNT_MIN                 (1)

//------------------------------------------------
// Sorting시 Stack Overflow 방지를 위한 자료 구조
//------------------------------------------------

typedef struct qmcSortStack
{
    SLong low;    // QuickSort Partition의 Low 값
    SLong high;   // QuickSort Partition의 High 값
} qmcSortStack;

//------------------------------------------------
// Record들의 집합을 관리하기 위한 배열
//------------------------------------------------

typedef struct qmcSortArray
{
    qmcSortArray  * parent;
    SLong           depth;
    SLong           numIndex;
    SLong           shift;
    SLong           mask;
    SLong           index;
    qmcSortArray ** elements;
    qmcSortArray  * internalElem[QMC_MEM_SORT_MAX_ELEMENTS];
} qmcSortArray;

//------------------------------------------------
// TASK-6445 Timsort
// 이미 정렬된 Run을 저장할 자료 구조
//------------------------------------------------
typedef struct qmcSortRun 
{
    SLong mStart;  // 시작 지점
    SLong mEnd;    // 끝 지점
    UInt  mLength; // 길이
} qmcSortRun;

//------------------------------------------------
// [Memory Sort Temp Table 객체]
//    * mSortNode
//        - ORDER BY로 사용될 경우, list로 존재할 수 있으나,
//        - Range검색을 위해서는 하나만 존재할 수 있다.
//------------------------------------------------

typedef struct qmcdMemSortTemp
{
    qcTemplate            * mTemplate;   // Template 정보
    qmcMemory             * mMemoryMgr;     // Sort Array를 위한 Memory Mgr
    iduMemory             * mMemory;

    qmcSortArray          * mArray;      // Record관리를 위한 Sort Array
    SLong                   mIndex;      // 현재 Record의 위치

    qmdMtrNode            * mSortNode;   // Sorting할 Column 정보

    smiRange              * mRange;      // Range검색을 위한 Multi-Range 객체
    smiRange              * mCurRange;   // Multi-Range중 단위 Range
    SLong                   mFirstKey;   // Range를 만족하는 Minimum Key
    SLong                   mLastKey;    // Range를 만족하는 Maximum Key
    UInt                    mUseOldSort; // BUG-41398
    idvSQL                * mStatistics;  // Session Event

    qmcSortRun            * mRunStack;     // Run을 저장할 Stack
    UInt                    mRunStackSize; // Run Stack에 저장된 Run의 개수
    qmcSortArray          * mTempArray;    // Merging 시 필요한 임시 공간

} qmcdMemSortTemp;

class qmcMemSort
{
public:

    //------------------------------------------------
    // Memory Sort Temp Table의 관리
    //------------------------------------------------

    // 초기화를 한다.
    static IDE_RC init( qmcdMemSortTemp * aTempTable,
                        qcTemplate      * aTemplate,
                        iduMemory       * aMemory,
                        qmdMtrNode      * aSortNode );

    // Temp Table내의 Sort Array 영역을 제거하고 초기화한다.
    static IDE_RC clear( qmcdMemSortTemp * aTempTable );

    // 모든 Record의 Hit Flag을 Reset함
    static IDE_RC clearHitFlag( qmcdMemSortTemp * aTempTable );

    //------------------------------------------------
    // Memory Sort Temp Table의 구성
    //------------------------------------------------

    // 데이터를 추가한다.
    static IDE_RC attach( qmcdMemSortTemp * aTempTable,
                          void            * aRow );

    // 정렬을 수행한다.
    static IDE_RC sort( qmcdMemSortTemp * aTempTable );

    // Limit Sorting을 수행함
    static IDE_RC shiftAndAppend( qmcdMemSortTemp * aTempTable,
                                  void            * aRow,
                                  void           ** aReturnRow );

    // Min-Max 저장을 위한 Limit Sorting을 수행함
    static IDE_RC changeMinMax( qmcdMemSortTemp * aTempTable,
                                void            * aRow,
                                void           ** aReturnRow );

    //------------------------------------------------
    // Memory Sort Temp Table의 검색
    //------------------------------------------------

    // 해당 위치의 데이터를 가져온다.
    static IDE_RC getElement( qmcdMemSortTemp * aTempTable,
                              SLong aIndex,
                              void ** aElement );

    //----------------------------
    // 순차 검색
    //----------------------------

    // 첫번째 순차 검색
    static IDE_RC getFirstSequence( qmcdMemSortTemp * aTempTable,
                                    void           ** aRow );

    // 다음 순차 검색
    static IDE_RC getNextSequence( qmcdMemSortTemp * aTempTable,
                                   void           ** aRow );
    //----------------------------
    // Range 검색
    //----------------------------

    // 첫번째 Range 검색
    static IDE_RC getFirstRange( qmcdMemSortTemp * aTempTable,
                                 smiRange        * aRange,
                                 void           ** aRow );

    // 다음 Range 검색
    static IDE_RC getNextRange( qmcdMemSortTemp * aTempTable,
                                void           ** aRow );

    //----------------------------
    // Hit 검색
    //----------------------------

    // 첫번째 Hit 검색
    static IDE_RC getFirstHit( qmcdMemSortTemp * aTempTable,
                               void           ** aRow );

    // 다음 Hit 검색
    static IDE_RC getNextHit( qmcdMemSortTemp * aTempTable,
                              void           ** aRow );

    //----------------------------
    // Non-Hit 검색
    //----------------------------

    // 첫번째 Non-Hit 검색
    static IDE_RC getFirstNonHit( qmcdMemSortTemp * aTempTable,
                                  void           ** aRow );

    // 다음 Non-Hit 검색
    static IDE_RC getNextNonHit( qmcdMemSortTemp * aTempTable,
                                 void           ** aRow );

    //------------------------------------------------
    // Memory Sort Temp Table의 기타 함수
    //------------------------------------------------

    // 전체 데이터의 갯수를 돌려준다.
    static IDE_RC getNumOfElements( qmcdMemSortTemp * aTempTable,
                                    SLong           * aNumElements );

    // 현재 Cursor 위치 저장
    static IDE_RC getCurrPosition( qmcdMemSortTemp * aTempTable,
                                   SLong           * aPosition );

    // 현재 Cursor 위치의 변경
    static IDE_RC setCurrPosition( qmcdMemSortTemp * aTempTable,
                                   SLong             aPosition );

    //------------------------------------------------
    // Window Sort (WNST)를 위한 기능
    //------------------------------------------------
     
    // 현재 정렬키(sortNode)를 변경
    static IDE_RC setSortNode( qmcdMemSortTemp  * aTempTable,
                               const qmdMtrNode * aSortNode );

    /* PROJ-2334 PMT memory variable column */
    static void changePartitionColumn( qmdMtrNode  * aNode,
                                       void        * aData );
    
private:

    //------------------------------------------------
    // Record 저장 관련 함수
    //------------------------------------------------

    // 지정된 위치의 Record 저장 위치를 검색한다.
    static void   get( qmcdMemSortTemp * aTempTable,
                       qmcSortArray* aArray,
                       SLong aIndex,
                       void*** aElement );

    // Sort Array의 저장 공간을 증가시킨다.
    static IDE_RC increase( qmcdMemSortTemp * aTempTable,
                            qmcSortArray* aArray );

    // Sort Array의 저장 공간을 증가시킨다.
    static IDE_RC increase( qmcdMemSortTemp * aTempTable,
                            qmcSortArray* aArray,
                            SLong * aReachEnd );

    //------------------------------------------------
    // Sorting 관련 함수
    //------------------------------------------------

    // 지정된 위치 내에서의 quick sorting을 수행한다.
    static IDE_RC quicksort( qmcdMemSortTemp * aTempTable,
                             SLong aLow,
                             SLong aHigh );

    // Partition내에서의 정렬을 수행한다.
    static IDE_RC partition( qmcdMemSortTemp * aTempTable,
                             SLong    aLow,
                             SLong    aHigh,
                             SLong  * aPartition,
                             idBool * aAllEqual );

    // 두 Record간의 비교 함수(Sorting 시에만 사용)
    static SInt   compare( qmcdMemSortTemp * aTempTable,
                           const void      * aElem1,
                           const void      * aElem2 );

    //------------------------------------------------
    // Sequential 검색 관련 함수
    //------------------------------------------------

    // 검색을 위해 최초 Record의 위치로 이동한다.
    static IDE_RC beforeFirstElement( qmcdMemSortTemp * aTempTable );

    // 다음 Record를 검색한다.
    static IDE_RC getNextElement( qmcdMemSortTemp * aTempTable,
                                  void ** aElement );

    //------------------------------------------------
    // Range 검색 관련 함수
    //------------------------------------------------

    // Key Range를 설정한다.
    static void   setKeyRange( qmcdMemSortTemp * aTempTable,
                               smiRange * aRange );

    // Key Range를 만족하는 최초의 Record를 검색한다.
    static IDE_RC getFirstKey( qmcdMemSortTemp * aTempTable,
                               void ** aElement );

    // Key Range를 만족하는 다음 Record를 검색한다.
    static IDE_RC getNextKey( qmcdMemSortTemp * aTempTable,
                              void ** aElement );

    // Range를 만족하는 Minimum Record의 위치를 설정
    static IDE_RC setFirstKey( qmcdMemSortTemp * aTempTable,
                               idBool * aResult );

    // Range를 만족하는 Maximum Record의 위치를 설정
    static IDE_RC setLastKey( qmcdMemSortTemp * aTempTable );

    // Limit Sorting에서 삽입할 위치 검색
    static IDE_RC findInsertPosition( qmcdMemSortTemp * aTempTable,
                                      void            * aRow,
                                      SLong           * aInsertPos );

    //--------------------------------------------------
    // TASK-6445 Timsort
    //--------------------------------------------------

    static IDE_RC timsort( qmcdMemSortTemp * aTempTable,
                           SLong aLow,
                           SLong aHigh );

    static SLong calcMinrun( SLong aLength );


    static void searchRun( qmcdMemSortTemp * aTempTable,
                           SLong             aStart,
                           SLong             aFence,
                           SLong           * aRunLength,
                           idBool          * aReverseOrder );

    static IDE_RC insertionSort( qmcdMemSortTemp * aTempTable,
                                 SLong             aLow, 
                                 SLong             aHigh );

    static void reverseOrder( qmcdMemSortTemp * aTempTable,
                              SLong             aLow, 
                              SLong             aHigh );

    static IDE_RC collapseStack( qmcdMemSortTemp * aTempTable );

    static IDE_RC collapseStackForce( qmcdMemSortTemp * aTempTable );

    static IDE_RC mergeRuns( qmcdMemSortTemp * aTempTable,
                             UInt              aRunStackIdx );

    static SLong gallopLeft( qmcdMemSortTemp  * aTempTable,
                             qmcSortArray     * aArray,
                             void            ** aKey,
                             SLong              aBase,
                             SLong              aLength,
                             SLong              aHint );

    static SLong gallopRight( qmcdMemSortTemp  * aTempTable,
                              qmcSortArray     * aArray,
                              void            ** aKey,
                              SLong              aBase,
                              SLong              aLength,
                              SLong              aHint );

    static IDE_RC mergeLowerRun( qmcdMemSortTemp * aTempTable,
                                 SLong             aBase1,
                                 SLong             aLen1,
                                 SLong             aBase2,
                                 SLong             aLen2 );

    static IDE_RC mergeHigherRun( qmcdMemSortTemp * aTempTable,
                                  SLong             aBase1,
                                  SLong             aLen1,
                                  SLong             aBase2,
                                  SLong             aLen2 );

    // Utility
    static IDE_RC moveArrayToArray( qmcdMemSortTemp * aTempTable,
                                    qmcSortArray    * aSrcArray,
                                    SLong             aSrcBase,
                                    qmcSortArray    * aDestArray,
                                    SLong             aDestBase,
                                    SLong             aMoveLength,
                                    idBool            aIsOverlapped ); 

    static void getLeafArray( qmcSortArray  * aArray,
                              SLong           aIndex,
                              qmcSortArray ** aRetArray,
                              SLong         * aRetIdx );
};

#endif /* _O_QMC_MEM_SORT_TEMP_TABLE_H_ */
