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
 * $Id: qmcMemPartHashTempTable.h 69506 2015-03-09 12:19:36Z interp $
 *
 * Description :
 *     Memory Hash Temp Table을 위한 정의
 *   - Partitioned Hash Algorithm을 사용한다.
 *
 * 용어 설명 :
 *
 * 약어 :
 *
 **********************************************************************/

#ifndef _O_QMC_MEM_PART_HASH_TMEP_TABLE_H_
#define _O_QMC_MEM_PART_HASH_TMEP_TABLE_H_ 1

#include <smi.h>
#include <qtcDef.h>
#include <qcuProperty.h>

// Partition 개수의 최소값 : pow(2,10)
#define QMC_MEM_PART_HASH_MIN_PART_COUNT        ( 1024 )

// Partition 개수의 최대값 : pow(2,24)
#define QMC_MEM_PART_HASH_MAX_PART_COUNT        ( 16777216 )

// Page 크기 : 4KB로 고정해서 간주
#define QMC_MEM_PART_HASH_PAGE_SIZE             ( 4096 )

// TLB Entry 확장 상수.
// TLB Entry 개수에 이 값을 곱한 값까지 페이지를 생성해도
// TLB Miss가 심하게 일어나지 않으므로 버퍼 없이 Fanout 하는 것이 유리하다.
#define QMC_MEM_PART_HASH_TLB_ENTRY_MULTIPLIER  ( 4 )

// Optimal Partition Count를 계산할 때 사용할
// Partition 당 평균 Element 개수 (고정 값)
#define QMC_MEM_PART_HASH_AVG_RECORD_COUNT      ( 10 )

/*************************************************************************
 * qmcdMemPartHashTemp Flag
 *************************************************************************/

#define QMC_MEM_PART_HASH_INITIALIZE            (0x00000000)

// 첫 range search 전 준비 여부
#define QMC_MEM_PART_HASH_SEARCH_READY_MASK     (0x00000001)
#define QMC_MEM_PART_HASH_SEARCH_READY_FALSE    (0x00000000)
#define QMC_MEM_PART_HASH_SEARCH_READY_TRUE     (0x00000001)

//----------------------------------
// 검색 아이템 (qmcMemHashItem)
// 배열의 원소로 구성되며, Hash Key와 실제 Element를 가리킨다.
//----------------------------------
typedef struct qmcMemHashItem
{
    UInt                mKey;
    qmcMemHashElement * mPtr;

} qmcMemHashItem;

//------------------------------------------------------------------------
// [Memory Hash Temp Table (Partitioned Hashing) 의 자료 구조 (PROJ-2553)]
//
//  DISTINCT Hashing에서는 사용하지 않으며, JOIN을 위한 Hashing 에서만 사용한다.
//
//  처음에는 아래와 같이 단일 List로 받는다.
//
//      ---------   --------   --------   --------   ------- 
//      | mHead |-->| elem |-->| elem |-->| elem |-->| ... |
//      ---------   --------   --------   --------   -------
//
//  모두 삽입되면, Element 개수 (= Record 개수)에 따라 Radix Bit를 결정한다.
//  Radix Bit는 Hash Key에 Masking 할 때 사용되며, Radix Bit가 클수록 Partition 개수가 많다.
//
//  다음으로 Radix Bit를 2의 지수로 둬, Partition 개수를 계산한다.
//  그 다음, 아래의 구조를 준비한다.
//
//   - 검색 배열 (Search Array)
//      실제 검색에 사용하는 배열로,
//      삽입된 Element의 Hash Key와 포인터를 가지고 있는 Item의 배열로 이루어져 있다.
//      배열의 크기는 (Item의 크기) * (Element 개수)이다.
//
//   - 히스토그램 (Histogram)
//      검색 배열에서 각 Partition의 '시작 위치'를 저장한 배열이다.
//      배열의 크기는 (unsigned long) * (Partition 개수)가 된다.
//
//                            SearchA.
//                            --------
//                        0   | item | --
//              Hist.         --------   | 
//             -------    1   | item |   | P1 
//          P1 |  0  |        --------   | 
//             -------    2   | item | --
//          P2 |  3  |        --------     
//             -------    3   | item | --
//             | ... |        --------   | P2 
//             -------    4   | item | --
//          Pn | X-1 |        --------     
//             -------        |  ... |       
//                            --------     
//                        X-1 | item | --
//                            --------   | Pn 
//                        X   | item | --
//                            --------    
//
//   Partition 구분은 Radix Bit Masking 으로 이루어지며,
//   Masking 결과 값이 Partition 인덱스가 된다.
//
//      -------- 
//      | elem | -> Key = 17, RadixBit = 3 -> Partition 1
//      --------                              Why? 17 & ( (1<<3)-1 ) = 17 & 7 = 1
//
//------------------------------------------------------------------------

typedef struct qmcdMemPartHashTemp
{
    //----------------------------------------------------
    // 기본 정보
    //----------------------------------------------------

    UInt                     mFlag;          // 검색 준비 여부
    qcTemplate             * mTemplate;      // Template
    iduMemory              * mMemory;
    qmdMtrNode             * mRecordNode;    // Record 구성 정보
    qmdMtrNode             * mHashNode;      // Hashing 할 Column 정보
    UInt                     mBucketCnt;     // 입력받은 Bucket의 개수

    //----------------------------------------------------
    // Partitioned Hashing 에 쓰일 정보
    //----------------------------------------------------

    UInt                     mRadixBit;      // Partition을 정할 RBit
    UInt                     mPartitionCnt;  // Partition의 개수
    ULong                  * mHistogram;     // Histogram 
    qmcMemHashItem         * mSearchArray;   // 검색 배열

    //----------------------------------------------------
    // 삽입을 위한 정보
    //----------------------------------------------------
    
    qmcMemHashElement      * mHead;          // 삽입된 Element의 처음
    qmcMemHashElement      * mTail;          // 삽입된 Element의 마지막
    SLong                    mRecordCnt;     // 총 저장 Record 개수

    //----------------------------------------------------
    // 검색을 위한 정보
    //----------------------------------------------------

    qmcMemHashElement      * mNextElem;      // 순차 탐색을 위한 다음 Element
    UInt                     mKey;           // 범위 검색을 위한 Hash Key 값
    qtcNode                * mFilter;        // 범위 검색을 위한 Filter
    ULong                    mNextIdx;       // Search Array에서 검색될 Index
    ULong                    mBoundaryIdx;   // Search Array에서 다음 파티션의 Index

} qmcdMemPartHashTemp;

class qmcMemPartHash
{
public:

    //------------------------------------------------
    // Memory Partitioned Hash Temp Table의 관리
    //------------------------------------------------
    
    // 초기화를 한다.
    static IDE_RC init( qmcdMemPartHashTemp * aTempTable,
                        qcTemplate          * aTemplate,
                        iduMemory           * aMemory,
                        qmdMtrNode          * aRecordNode,
                        qmdMtrNode          * aHashNode,
                        UInt                  aBucketCnt );
    
    // Temp Table내의 Bucket을 초기화한다.
    static IDE_RC clear( qmcdMemPartHashTemp * aTempTable );

    // 모든 Record의 Hit Flag을 Reset
    static IDE_RC clearHitFlag( qmcdMemPartHashTemp * aTempTable );

    //------------------------------------------------
    // Memory Partitioned Hash Table의 삽입
    //------------------------------------------------

    // 무조건 Record를 삽입한다.
    static IDE_RC insertAny( qmcdMemPartHashTemp * aTempTable,
                             UInt                  aHash, 
                             void                * aElement,
                             void               ** aOutElement );

    //------------------------------------------------
    // Memory Partitioned Hash Temp Table의 검색
    //------------------------------------------------

    //----------------------------
    // 순차 검색
    //----------------------------

    static IDE_RC getFirstSequence( qmcdMemPartHashTemp  * aTempTable,
                                    void                ** aElement );
    
    static IDE_RC getNextSequence( qmcdMemPartHashTemp  * aTempTable,
                                   void                ** aElement);

    //----------------------------
    // Range 검색
    //----------------------------

    static IDE_RC getFirstRange( qmcdMemPartHashTemp  * aTempTable,
                                 UInt                   aHash,
                                 qtcNode              * aFilter,
                                 void                ** aElement );

    static IDE_RC getNextRange( qmcdMemPartHashTemp  * aTempTable,
                                void                ** aElement );

    //----------------------------
    // Same Row & NonHit 검색
    // - Set Operation 에서 사용하므로, 여기에서는 지원하지 않는다.
    //----------------------------

    //----------------------------
    // Hit 검색
    //----------------------------

    static IDE_RC getFirstHit( qmcdMemPartHashTemp  * aTempTable,
                               void                ** aElement );

    static IDE_RC getNextHit( qmcdMemPartHashTemp  * aTempTable,
                              void                ** aElement );
    
    //----------------------------
    // NonHit 검색
    //----------------------------

    static IDE_RC getFirstNonHit( qmcdMemPartHashTemp  * aTempTable,
                                  void                ** aElement );

    static IDE_RC getNextNonHit( qmcdMemPartHashTemp  * aTempTable,
                                 void                ** aElement );
    
    //----------------------------
    // 기타 함수
    //----------------------------

    // 수행 비용 정보 획득
    static IDE_RC getDisplayInfo( qmcdMemPartHashTemp * aTempTable,
                                  SLong               * aRecordCnt,
                                  UInt                * aBucketCnt );

private:

    // Range 검색시 조건 검사
    static IDE_RC judgeFilter( qmcdMemPartHashTemp * aTempTable,
                               void                * aElem,
                               idBool              * aResult );

    // 검색 준비
    static IDE_RC readyForSearch( qmcdMemPartHashTemp * aTempTable );

    // RBit 계산
    static UInt calcRadixBit( qmcdMemPartHashTemp * aTempTable );

    // 한 번에 검색 함수 구성
    static IDE_RC fanoutSingle( qmcdMemPartHashTemp * aTempTable );

    // 두 번에 걸쳐 검색 함수 구성
    static IDE_RC fanoutDouble( qmcdMemPartHashTemp * aTempTable );

    // ceil(log2()) 
    static UInt ceilLog2( ULong aNumber );
};

#endif /* _O_QMC_MEM_PART_HASH_TMEP_TABLE_H_ */
