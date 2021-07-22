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
 * $Id: qmnSort.h 82075 2018-01-17 06:39:52Z jina.kim $
 *
 * Description :
 *     SORT(SORT) Node
 *
 *     관계형 모델에서 sorting 연산을 수행하는 Plan Node 이다.
 *
 *     다음과 같은 기능을 위해 사용된다.
 *         - ORDER BY
 *         - Sort-based Grouping
 *         - Sort-based Distinction
 *         - Sort Join
 *         - Merge Join
 *         - Sort-based Left Outer Join
 *         - Sort-based Full Outer Join
 *         - Store And Search
 *
 * 용어 설명 :
 *
 * 약어 :
 *
 **********************************************************************/

#ifndef _O_QMN_SORT_H_
#define _O_QMN_SORT_H_ 1

#include <mt.h>
#include <qmc.h>
#include <qmcSortTempTable.h>
#include <qmnDef.h>

//-----------------
// Code Node Flags
//-----------------

//-----------------------------------------------------
// qmncSORT.flag
// 저장 방식의 선택
// - ONLY     : 저장만 수행
// - PRESERVE : 별도의 Sorting 없이 순서가 보장됨
// - SORTING  : Sorting을 수행
//-----------------------------------------------------

#define QMNC_SORT_STORE_MASK               (0x00000003)
#define QMNC_SORT_STORE_NONE               (0x00000000)
#define QMNC_SORT_STORE_ONLY               (0x00000001)
#define QMNC_SORT_STORE_PRESERVE           (0x00000002)
#define QMNC_SORT_STORE_SORTING            (0x00000003)

//-----------------------------------------------------
// qmncSORT.flag
// 검색 방식의 선택
// - SEQUENTIAL : 순차 검색
// - RANGE      : 범위 검색
//-----------------------------------------------------

#define QMNC_SORT_SEARCH_MASK              (0x00000004)
#define QMNC_SORT_SEARCH_SEQUENTIAL        (0x00000000)
#define QMNC_SORT_SEARCH_RANGE             (0x00000004)

//-----------------
// Data Node Flags
//-----------------

// qmndSORT.flag
// First Initialization Done
#define QMND_SORT_INIT_DONE_MASK           (0x00000001)
#define QMND_SORT_INIT_DONE_FALSE          (0x00000000)
#define QMND_SORT_INIT_DONE_TRUE           (0x00000001)

/*---------------------------------------------------------------------
 *  Example)
 *      SELECT T1.i1, T1.i3, T2.i2 FROM T1,T2 ORDER BY MOD(T1.i2,10);
 *                                            ^^^^^^^^^^^^^^^^^^^^^^
 *  in qmncSORT
 *      myNode : T1 ->  MOD(T1.i2,10)
 *               ^^
 *               base table count : 1
 *  in qmndSORT
 *      mtrNode  : T1 -> MOD(T1.i2,10)
 *      sortNode : MOD(T1.i2,10)
 ----------------------------------------------------------------------*/

struct qmncSORT;
struct qmndSORT;

typedef IDE_RC (* qmnSortSearchFunc ) (  qcTemplate * aTemplate,
                                         qmncSORT   * aCodePlan,
                                         qmndSORT   * aDataPlan,
                                         qmcRowFlag * aFlag );

typedef struct qmncSORT
{
    //---------------------------------
    // Code 영역 공통 정보
    //---------------------------------

    qmnPlan        plan;
    UInt           flag;
    UInt           planID;

    //---------------------------------
    // SORT 고유 정보
    //---------------------------------

    UShort         baseTableCount;   // Base Table의 개수
    UShort         depTupleRowID;    // Dependent Tuple ID
    SDouble        storeRowCount;    // Sort Node에 저장될 레코드 갯수 ( 예측값임 )
    qmcMtrNode   * myNode;           // 저장 Column 정보
    qtcNode      * range;            // Range 검색을 위한 Key Range

    qcComponentInfo * componentInfo; // PROJ-2462 Result Cache

    //---------------------------------
    // Data 영역 정보
    //---------------------------------

    UInt           mtrNodeOffset;    // size : ID_SIZEOF(qmdMtrNode) * count

} qmncSORT;

typedef struct qmndSORT
{
    //---------------------------------
    // Data 영역 공통 정보
    //---------------------------------

    qmndPlan            plan;
    doItFunc            doIt;
    UInt              * flag;

    //---------------------------------
    // SORT 고유 정보
    //---------------------------------

    UInt                mtrRowSize;   // 저장 Row의 크기

    qmdMtrNode        * mtrNode;      // 저장 Column 정보
    qmdMtrNode        * sortNode;     // 정렬 Column의 위치

    mtcTuple          * depTuple;     // Dependent Tuple 위치
    UInt                depValue;     // Dependent Value

    qmcdSortTemp      * sortMgr;      // Sort Temp Table

    qmnSortSearchFunc   searchFirst;  // 검색 함수
    qmnSortSearchFunc   searchNext;   // 검색 함수

    //---------------------------------
    // Merge Join 관련 정보
    //---------------------------------

    smiCursorPosInfo    cursorInfo;   // 커서 저장 공간

    /* PROJ-2462 Result Cache */
    qmndResultCache     resultData;
} qmndSORT;

class qmnSORT
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

    // 호출되어서는 안됨.
    static IDE_RC doItDefault( qcTemplate * aTemplate,
                               qmnPlan    * aPlan,
                               qmcRowFlag * aFlag );

    // 최초 수행 함수
    static IDE_RC doItFirst( qcTemplate * aTemplate,
                             qmnPlan    * aPlan,
                             qmcRowFlag * aFlag );

    // 다음 수행 함수
    static IDE_RC doItNext( qcTemplate * aTemplate,
                            qmnPlan    * aPlan,
                            qmcRowFlag * aFlag );

    // TODO - A4 Grouping Set Integration
    static IDE_RC doItLast( qcTemplate * aTemplate,
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

    // Merge Join에서의 Cursor 저장
    static IDE_RC storeCursor( qcTemplate * aTemplate,
                               qmnPlan    * aPlan );

    // Merge Join에서의 Cursor 복원
    static IDE_RC restoreCursor( qcTemplate * aTemplate,
                                 qmnPlan    * aPlan );

private:

    //-----------------------------
    // 초기화 관련 함수
    //-----------------------------

    // 최초 초기화 함수
    static IDE_RC firstInit( qcTemplate * aTemplate,
                             qmncSORT   * aCodePlan,
                             qmndSORT   * aDataPlan );

    // 저장 Column의 초기화
    static IDE_RC initMtrNode( qcTemplate * aTemplate,
                               qmncSORT   * aCodePlan,
                               qmndSORT   * aDataPlan );

    // 정렬 Column의 위치 검색
    static IDE_RC initSortNode( qmncSORT   * aCodePlan,
                                qmndSORT   * aDataPlan );

    // Temp Table 초기화
    static IDE_RC initTempTable( qcTemplate * aTemplate,
                                 qmncSORT   * aCodePlan,
                                 qmndSORT   * aDataPlan );

    // Dependent Tuple의 변경 여부 검사
    static IDE_RC checkDependency( qcTemplate * aTemplate,
                                   qmncSORT   * aCodePlan,
                                   qmndSORT   * aDataPlan,
                                   idBool     * aDependent );

    //-----------------------------
    // 저장 관련 함수
    //-----------------------------

    // 저장 및 정렬 수행
    static IDE_RC storeAndSort( qcTemplate * aTemplate,
                                qmncSORT   * aCodePlan,
                                qmndSORT   * aDataPlan );

    // 하나의 Row를 삽입
    static IDE_RC addOneRow( qcTemplate * aTemplate,
                             qmndSORT   * aDataPlan );

    // 저장 Row의 구성
    static IDE_RC setMtrRow( qcTemplate * aTemplate,
                             qmndSORT   * aDataPlan );

    //-----------------------------
    // 수행 관련 함수
    //-----------------------------

    // 검색된 저장 Row를 기준으로 Tuple Set을 복원한다.
    static IDE_RC setTupleSet( qcTemplate * aTemplate,
                               qmndSORT   * aDataPlan );

    //-----------------------------
    // 검색 관련 함수
    //-----------------------------

    // 검색 함수의 결정
    static IDE_RC setSearchFunction( qmncSORT   * aCodePlan,
                                     qmndSORT   * aDataPlan );

    // 호출되어서는 안됨
    static IDE_RC searchDefault( qcTemplate * aTemplate,
                                 qmncSORT   * aCodePlan,
                                 qmndSORT   * aDataPlan,
                                 qmcRowFlag * aFlag );

    // 첫번째 순차 검색
    static IDE_RC searchFirstSequence( qcTemplate * aTemplate,
                                       qmncSORT   * aCodePlan,
                                       qmndSORT   * aDataPlan,
                                       qmcRowFlag * aFlag );

    // 다음 순차 검색
    static IDE_RC searchNextSequence( qcTemplate * aTemplate,
                                      qmncSORT   * aCodePlan,
                                      qmndSORT   * aDataPlan,
                                      qmcRowFlag * aFlag );

    // 첫번째 Range 검색
    static IDE_RC searchFirstRangeSearch( qcTemplate * aTemplate,
                                          qmncSORT   * aCodePlan,
                                          qmndSORT   * aDataPlan,
                                          qmcRowFlag * aFlag );

    // 다음 Range 검색
    static IDE_RC searchNextRangeSearch( qcTemplate * aTemplate,
                                         qmncSORT   * aCodePlan,
                                         qmndSORT   * aDataPlan,
                                         qmcRowFlag * aFlag );

    // 첫번째 Hit 검색
    static IDE_RC searchFirstHit( qcTemplate * aTemplate,
                                  qmncSORT   * aCodePlan,
                                  qmndSORT   * aDataPlan,
                                  qmcRowFlag * aFlag );

    // 다음 Hit 검색
    static IDE_RC searchNextHit( qcTemplate * aTemplate,
                                 qmncSORT   * aCodePlan,
                                 qmndSORT   * aDataPlan,
                                 qmcRowFlag * aFlag );

    // 첫번째 Non-Hit 검색
    static IDE_RC searchFirstNonHit( qcTemplate * aTemplate,
                                     qmncSORT   * aCodePlan,
                                     qmndSORT   * aDataPlan,
                                     qmcRowFlag * aFlag );

    // 다음 Non-Hit 검색
    static IDE_RC searchNextNonHit( qcTemplate * aTemplate,
                                    qmncSORT   * aCodePlan,
                                    qmndSORT   * aDataPlan,
                                    qmcRowFlag * aFlag );

    //-----------------------------
    // A4 - Grouping Set 관련 함수
    //-----------------------------

    /*
      static IDE_RC isCurrentGroupingColumn( qcStatement * aStatement,
      qtcNode     * aGroupNode,
      qtcNode     * aMtrRelGraphNode,
      idBool      * aExist );
    */

};


#endif /* _O_QMN_SORT_H_ */
