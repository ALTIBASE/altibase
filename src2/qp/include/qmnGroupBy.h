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
 * $Id: qmnGroupBy.h 82075 2018-01-17 06:39:52Z jina.kim $
 *
 * Description :
 *     GRBY(GRoup BY) Node
 *
 *     관계형 모델에서 다음과 같은 기능을 수행하는 Plan Node 이다.
 *
 *         - Sort-based Distinction
 *         - Sort-based Grouping
 *
 * 용어 설명 :
 *
 * 약어 :
 *
 **********************************************************************/

#ifndef _O_QMN_GROUP_BY_H_
#define _O_QMN_GROUP_BY_H_ 1

#include <mt.h>
#include <qmc.h>
#include <qmnDef.h>

//-----------------
// Code Node Flags
//-----------------

// qmncGRBY.flag
// GRBY 노드의 사용용도
//    - Sort-based Distinction
//    - Sort-based Grouping
#define QMNC_GRBY_METHOD_MASK              (0x00000003)
#define QMNC_GRBY_METHOD_NOT_DEFINED       (0x00000000)
#define QMNC_GRBY_METHOD_DISTINCTION       (0x00000001)
#define QMNC_GRBY_METHOD_GROUPING          (0x00000002)

//-----------------
// Data Node Flags
//-----------------

// qmndGRBY.flag
// First Initialization Done
#define QMND_GRBY_INIT_DONE_MASK           (0x00000001)
#define QMND_GRBY_INIT_DONE_FALSE          (0x00000000)
#define QMND_GRBY_INIT_DONE_TRUE           (0x00000001)

/*---------------------------------------------------------------------
 *  Example)
 *      SELECT i0, i1 SUM(DISTINCT i2) FROM T1 GROUP BY i0, i1;
 *      Plan : PROJ - AGGR - GRBY - SCAN
 *  in qmncGRBY
 *      myNode : i0 -> i1  // no need base table
 *  in qmndGRBY
 *      mtrNode  : T1 -> i0 -> i1
 ----------------------------------------------------------------------*/

typedef struct qmncGRBY
{
    //---------------------------------
    // Code 영역 공통 정보
    //---------------------------------

    qmnPlan        plan;
    UInt           flag;
    UInt           planID;

    //---------------------------------
    // GRBY 관련 정보
    //---------------------------------

    // Base Table Count는 유지하지 않는다.
    // 이는 현재 상태 자체가 상위 Node에서 필요한 정보이기 때문이다.
    UShort         baseTableCount;
    qmcMtrNode   * myNode;

    //---------------------------------
    // Data 영역 관련 정보
    //---------------------------------

    UInt           mtrNodeOffset;

} qmncGRBY;

typedef struct qmndGRBY
{
    //---------------------------------
    // Data 영역 공통 정보
    //---------------------------------

    qmndPlan            plan;
    doItFunc            doIt;
    UInt              * flag;

    //---------------------------------
    // GRBY 고유 정보
    //---------------------------------

    qmdMtrNode        * mtrNode;    // 저장 대상 Column
    qmdMtrNode        * groupNode;  // Group Node의 위치

    //---------------------------------
    // 동일 Group 판단을 위한 자료 구조
    //---------------------------------

    UInt                mtrRowSize; // 저장할 Row의 Size
    void              * mtrRow[2];  // 비교를 위한 두 개의 공간
    UInt                mtrRowIdx;  // swap within mtrRow[0] and mtr2Row[1]

    void              * nullRow;    // Null Row

} qmndGRBY;

class qmnGRBY
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

    // 최초 GRBY를 수행
    static IDE_RC doItFirst( qcTemplate * aTemplate,
                             qmnPlan    * aPlan,
                             qmcRowFlag * aFlag );

    // 동일 Record인지의 여부만 판단
    static IDE_RC doItGroup( qcTemplate * aTemplate,
                             qmnPlan    * aPlan,
                             qmcRowFlag * aFlag );

    // 다른 Record만 추출
    static IDE_RC doItDistinct( qcTemplate * aTemplate,
                                qmnPlan    * aPlan,
                                qmcRowFlag * aFlag );

    // TODO - A4 Grouping Set Integration
    static IDE_RC doItLast( qcTemplate * aTemplate,
                            qmnPlan    * aPlan,
                            qmcRowFlag * aFlag );

private:

    //------------------------
    // 초기화 관련 함수
    //------------------------

    // 최초 초기화
    static IDE_RC firstInit( qcTemplate * aTemplate,
                             qmncGRBY   * aCodePlan,
                             qmndGRBY   * aDataPlan );

    // 저장 Column의 초기화
    static IDE_RC initMtrNode( qcTemplate * aTemplate,
                               qmncGRBY   * aCodePlan,
                               qmndGRBY   * aDataPlan );

    // Grouping 대상 Column의 위치 지정
    static IDE_RC initGroupNode( qmndGRBY   * aDataPlan );

    // 저장할 Row의 공간 할당 및 Null Row 생성
    static IDE_RC allocMtrRow( qcTemplate * aTemplate,
                               qmndGRBY   * aDataPlan );

    //------------------------
    // GRBY 의 doIt() 관련 함수
    //------------------------

    // 저장 Row를 구성.
    static IDE_RC setMtrRow( qcTemplate * aTemplate,
                             qmndGRBY   * aDataPlan );

    // Tuple Set에 설정
    static IDE_RC setTupleSet( qcTemplate * aTemplate,
                               qmndGRBY   * aDataPlan );

    // 두 Row간의 동일 여부 판단
    static SInt   compareRows( qmndGRBY   * aDataPlan );

    //------------------------
    // Grouping Set 관련 함수
    //------------------------

    // TODO - A4 : Grouping Set Integration
    /*
      static IDE_RC isCurrentGroupingColumn( qcStatement * aStatement,
      qtcNode     * aGroupNode,
      qtcNode     * aMtrRelGraphNode,
      idBool      * aExist );
    */

};

#endif /* _O_QMN_GROUP_BY_H_ */
