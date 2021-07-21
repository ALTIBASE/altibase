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
 * $Id: qmnHash.h 82075 2018-01-17 06:39:52Z jina.kim $
 *
 * Description :
 *     HASH(HASH) Node
 *
 *     관계형 모델에서 hashing 연산을 수행하는 Plan Node 이다.
 *
 *     다음과 같은 기능을 위해 사용된다.
 *         - Hash Join
 *         - Hash-based Left Outer Join
 *         - Hash-based Full Outer Join
 *         - Store And Search
 *
 *     HASH 노드는 Two Pass Hash Join등에 사용될 때,
 *     여러개의 Temp Table을 관리할 수 있다.
 *     따라서, 모든 삽입 및 검색 시 이에 대한 고려가 충분하여야 한다.
 *
 * 용어 설명 :
 *
 * 약어 :
 *
 **********************************************************************/

#ifndef _O_QMN_HASH_H_
#define _O_QMN_HASH_H_ 1

#include <mt.h>
#include <qmc.h>
#include <qmcHashTempTable.h>
#include <qmnDef.h>

//---------------------------------------------------
// - Two-Pass Hash Join등에서
//   Temp Table은 여러 개가 존재할 수 있으며,
//   최초 1개의 Temp Table은 Primary로 사용되며,
//   이후의 Temp Table은 Secondary로 사용된다.
// - HASH 노드는 Master Table을 통해 메모리 할당 및
//   Null Row획득을 수행하게 된다.
//---------------------------------------------------

#define QMN_HASH_PRIMARY_ID                        (0)

//-----------------
// Code Node Flags
//-----------------

//----------------------------------------------------
// qmncHASH.flag
// 저장 방식의 선택
// - HASHING : 일반적인 Hashing으로 저장
// - DISTINCT : Distinct Hashing으로 저장
//              Store And Search로 사용될 때 선택함
//----------------------------------------------------

#define QMNC_HASH_STORE_MASK              (0x00000003)
#define QMNC_HASH_STORE_NONE              (0x00000000)
#define QMNC_HASH_STORE_HASHING           (0x00000001)
#define QMNC_HASH_STORE_DISTINCT          (0x00000002)

//----------------------------------------------------
// qmncHASH.flag
// 검색 방식의 선택
// - SEQUENTIAL : 순차 검색
//                Two-Pass Hash Join의 left에서 사용
// - RANGE      : 범위 검색
//                Hash Join의 Right에서 사용
// - SEARCH     : STORE AND SEARCH 검색
//----------------------------------------------------

#define QMNC_HASH_SEARCH_MASK             (0x00000030)
#define QMNC_HASH_SEARCH_NONE             (0x00000000)
#define QMNC_HASH_SEARCH_SEQUENTIAL       (0x00000010)
#define QMNC_HASH_SEARCH_RANGE            (0x00000020)
#define QMNC_HASH_SEARCH_STORE_SEARCH     (0x00000030)

//----------------------------------------------------
// qmncHASH.flag
// Store And Search시 NULL 존재 여부의 검사
// Hash를 이용한 Store And Search가 가능한 경우는
// 다음과 같은 경우이다.
//    - 한 Column에 대한 Filter
//        - NOT NULL Constraint가 존재하면,
//          NULL검사 필요 없음.
//        - NOT NULL Constraint가 존재하지 않으면,
//          NULL검사 필요함
//    - 여러 Column에 대한 Filter
//        - 반드시 NOT NULL Constraints가 존재해야
//          하며, 따라서 NULL검사 필요 없음
//  NULL_CHECK_FALSE : NULL CHECK할 필요 없음
//  NULL_CHECK_TRUE  : NULL CHECK 필요함.
//----------------------------------------------------

#define QMNC_HASH_NULL_CHECK_MASK         (0x00000100)
#define QMNC_HASH_NULL_CHECK_FALSE        (0x00000000)
#define QMNC_HASH_NULL_CHECK_TRUE         (0x00000100)

//-----------------
// Data Node Flags
//-----------------

// qmndHASH.flag
// First Initialization Done
#define QMND_HASH_INIT_DONE_MASK          (0x00000001)
#define QMND_HASH_INIT_DONE_FALSE         (0x00000000)
#define QMND_HASH_INIT_DONE_TRUE          (0x00000001)

// qmndHash.flag
// 상수에 해당하는 값이 NULL인지의 여부
// Store And Search시 상수 값에 따라 검색 방법을 달리한다.
#define QMND_HASH_CONST_NULL_MASK         (0x00000002)
#define QMND_HASH_CONST_NULL_FALSE        (0x00000000)
#define QMND_HASH_CONST_NULL_TRUE         (0x00000002)

/*---------------------------------------------------------------------
 *  Example)
 *      SELECT * FROM T1,T2 WHERE T1.i1 = T2.i1 + 3;
 *
 *  in qmncHASH
 *      myNode : T2 -> T2.i1 + 3
 *               ^^
 *               baseTableCount : 1
 *      filter   : ( = )
 *      filterConst : T1.i1
 *  in qmndHASH
 *      mtrNode  : T2 -> T2.i1 + 3
 *      hashNode : T2.i1 + 3
 ----------------------------------------------------------------------*/

struct qmncHASH;
struct qmndHASH;

typedef IDE_RC (* qmnHashSearchFunc ) (  qcTemplate * aTemplate,
                                         qmncHASH   * aCodePlan,
                                         qmndHASH   * aDataPlan,
                                         qmcRowFlag * aFlag );

typedef struct qmncHASH
{
    //---------------------------------
    // Code 영역 공통 정보
    //---------------------------------

    qmnPlan        plan;
    UInt           flag;
    UInt           planID;

    //---------------------------------
    // HASH 고유 정보
    //---------------------------------

    UShort         baseTableCount;
    qmcMtrNode   * myNode;          // 저장 Column 정보

    UShort         depTupleRowID;   // dependent tuple ID

    //---------------------------------
    // Temp Table 관련 정보
    //---------------------------------

    UInt           bucketCnt;      // Bucket의 개수
    UInt           tempTableCnt;   // Temp Table의 개수

    //---------------------------------
    // Range 검색 정보
    //---------------------------------

    qtcNode      * filter;         // if filter t1.i1 = t2.i1(right)
    qtcNode      * filterConst;    // filterConst is t1.i1
                                   //          JOIN
                                   //        |       |
                                   //    SCAN(t1)   HASH
                                   //                |
                                   //               SCAN(t2)

    qcComponentInfo * componentInfo; // PROJ-2462 Result Cache
    //---------------------------------
    // Data 영역 정보
    //---------------------------------

    UInt           mtrNodeOffset;     // 저장 Column의 위치

} qmncHASH;

typedef struct qmndHASH
{
    //---------------------------------
    // Data 영역 공통 정보
    //---------------------------------

    qmndPlan            plan;
    doItFunc            doIt;
    UInt              * flag;

    //---------------------------------
    // HASH 고유 정보
    //---------------------------------

    UInt                mtrRowSize;   // 저장 Row의 크기

    qmdMtrNode        * mtrNode;      // 저장 Column 정보
    qmdMtrNode        * hashNode;     // Hashing Column의 위치

    mtcTuple          * depTuple;     // Dependent Tuple 위치
    UInt                depValue;     // Dependent Value

    qmcdHashTemp      * hashMgr;      // 여러 개의 Hash Temp Table
    SLong               mtrTotalCnt;  // 저장된 Data의 개수

    qmnHashSearchFunc   searchFirst;  // 검색 함수
    qmnHashSearchFunc   searchNext;   // 검색 함수

    UInt                currTempID;    // 현재 Temp Table ID
    idBool              isNullStore;   // Null을 Store하고 있는지 여부
    /* PROJ-2462 Result Cache */
    qmndResultCache     resultData;
} qmndHASH;

class qmnHASH
{
public:

    //------------------------
    // Base Function Pointer
    //------------------------

    // 초기화
    static IDE_RC init( qcTemplate * aTemplate,
                        qmnPlan    * aPlan );

    // 수행 함수
    static IDE_RC doIt( qcTemplate * aTemplate,
                        qmnPlan    * aPlan,
                        qmcRowFlag * aFlag );

    // Null Padding
    static IDE_RC padNull( qcTemplate * aTemplate,
                           qmnPlan    * aPlan );

    // Plan 정보 출력
    static IDE_RC printPlan( qcTemplate   * aTemplate,
                             qmnPlan      * aPlan,
                             ULong          aDepth,
                             iduVarString * aString,
                             qmnDisplay     aMode );

    //------------------------
    // mapping by doIt() function pointer
    //------------------------

    // 호출되어서는 안됨
    static IDE_RC doItDefault( qcTemplate * aTemplate,
                               qmnPlan    * aPlan,
                               qmcRowFlag * aFlag );

    // 첫번째 수행 함수
    static IDE_RC doItFirst( qcTemplate * aTemplate,
                             qmnPlan    * aPlan,
                             qmcRowFlag * aFlag );

    // 다음 수행 함수
    static IDE_RC doItNext( qcTemplate * aTemplate,
                            qmnPlan    * aPlan,
                            qmcRowFlag * aFlag );

    // Store And Search의 첫번째 수행 함수
    static IDE_RC doItFirstStoreSearch( qcTemplate * aTemplate,
                                        qmnPlan    * aPlan,
                                        qmcRowFlag * aFlag );

    // Store And Search의 다음 수행 함수
    static IDE_RC doItNextStoreSearch( qcTemplate * aTemplate,
                                       qmnPlan    * aPlan,
                                       qmcRowFlag * aFlag );

    //------------------------
    // Direct External Call
    //------------------------

    // Hit 검색 모드로 변경
    static void setHitSearch( qcTemplate * aTemplate,
                              qmnPlan    * aPlan );

    // Non-Hit 검색 모드로 변경
    static void setNonHitSearch( qcTemplate * aTemplate,
                                 qmnPlan    * aPlan );

    // Record에 Hit 표기
    static IDE_RC setHitFlag( qcTemplate * aTemplate,
                              qmnPlan    * aPlan );

    // Record에 Hit 표기되었는지 검증
    static idBool isHitFlagged( qcTemplate * aTemplate,
                                qmnPlan    * aPlan );
private:

    //-----------------------------
    // 초기화 관련 함수
    //-----------------------------

    // 최초 초기화 함수
    static IDE_RC firstInit( qcTemplate * aTemplate,
                             qmncHASH   * aCodePlan,
                             qmndHASH   * aDataPlan );

    // 저장 Column의 초기화
    static IDE_RC initMtrNode( qcTemplate * aTemplate,
                               qmncHASH   * aCodePlan,
                               qmndHASH   * aDataPlan );

    // Hashing Column의 시작 위치
    static IDE_RC initHashNode( qmncHASH   * aCodePlan,
                                qmndHASH   * aDataPlan );

    // Temp Table 초기화
    static IDE_RC initTempTable( qcTemplate * aTemplate,
                                 qmncHASH   * aCodePlan,
                                 qmndHASH   * aDataPlan );

    // Dependent Tuple의 변경 여부 검사
    static IDE_RC checkDependency( qcTemplate * aTemplate,
                                   qmncHASH   * aCodePlan,
                                   qmndHASH   * aDataPlan,
                                   idBool     * aDependent );

    //-----------------------------
    // 저장 관련 함수
    //-----------------------------

    // Hash Temp Table을 구성한다.
    static IDE_RC storeAndHashing( qcTemplate * aTemplate,
                                   qmncHASH   * aCodePlan,
                                   qmndHASH   * aDataPlan );

    // 저장 Row를 구성
    static IDE_RC setMtrRow( qcTemplate * aTemplate,
                             qmndHASH   * aDataPlan );

    // 저장 Row의 Null 여부 검사
    static IDE_RC checkNullExist( qmncHASH   * aCodePlan,
                                  qmndHASH   * aDataPlan );

    // 저장 Row의 Hash Key 값 획득
    static IDE_RC getMtrHashKey( qmndHASH   * aDataPlan,
                                 UInt       * aHashKey );

    //-----------------------------
    // 수행 관련 함수
    //-----------------------------

    // 상수 영역의 Hash Key값 획득
    static IDE_RC getConstHashKey( qcTemplate * aTemplate,
                                   qmncHASH   * aCodePlan,
                                   qmndHASH   * aDataPlan,
                                   UInt       * aHashKey );

    // 저장 Row를 이용한 Tuple Set의 복원
    static IDE_RC setTupleSet( qcTemplate * aTemplate,
                               qmndHASH   * aDataPlan );

    //-----------------------------
    // 검색 관련 함수
    //-----------------------------

    // 검색 함수의 결정
    static IDE_RC setSearchFunction( qmncHASH   * aCodePlan,
                                     qmndHASH   * aDataPlan );

    // 호출되어서는 안됨
    static IDE_RC searchDefault( qcTemplate * aTemplate,
                                 qmncHASH   * aCodePlan,
                                 qmndHASH   * aDataPlan,
                                 qmcRowFlag * aFlag );

    // 첫번째 순차 검색
    static IDE_RC searchFirstSequence( qcTemplate * aTemplate,
                                       qmncHASH   * aCodePlan,
                                       qmndHASH   * aDataPlan,
                                       qmcRowFlag * aFlag );
    // 다음 순차 검색
    static IDE_RC searchNextSequence( qcTemplate * aTemplate,
                                      qmncHASH   * aCodePlan,
                                      qmndHASH   * aDataPlan,
                                      qmcRowFlag * aFlag );

    // 첫번째 범위 검색
    static IDE_RC searchFirstFilter( qcTemplate * aTemplate,
                                     qmncHASH   * aCodePlan,
                                     qmndHASH   * aDataPlan,
                                     qmcRowFlag * aFlag );

    // 다음 범위 검색
    static IDE_RC searchNextFilter( qcTemplate * aTemplate,
                                    qmncHASH   * aCodePlan,
                                    qmndHASH   * aDataPlan,
                                    qmcRowFlag * aFlag );

    // 첫번째 Non-Hit 검색
    static IDE_RC searchFirstNonHit( qcTemplate * aTemplate,
                                     qmncHASH   * aCodePlan,
                                     qmndHASH   * aDataPlan,
                                     qmcRowFlag * aFlag );

    // 다음 Non-Hit 검색
    static IDE_RC searchNextNonHit( qcTemplate * aTemplate,
                                    qmncHASH   * aCodePlan,
                                    qmndHASH   * aDataPlan,
                                    qmcRowFlag * aFlag );

    /* PRORJ-2339 */
    // 첫번째 Hit 검색
    static IDE_RC searchFirstHit( qcTemplate * aTemplate,
                                  qmncHASH   * aCodePlan,
                                  qmndHASH   * aDataPlan,
                                  qmcRowFlag * aFlag );

    // 다음 Hit 검색
    static IDE_RC searchNextHit( qcTemplate * aTemplate,
                                 qmncHASH   * aCodePlan,
                                 qmndHASH   * aDataPlan,
                                 qmcRowFlag * aFlag );

};

#endif /* _O_QMN_HASH_H_ */
