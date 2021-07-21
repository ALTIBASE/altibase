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
 * $Id: qmnLeftOuter.h 90785 2021-05-06 07:26:22Z hykim $
 *
 * Description :
 *     LOJN(Left Outer JoiN) Node
 *
 *     관계형 모델에서 Left Outer Join를 수행하는 Plan Node 이다.
 *     다양한 Join Method들은 하위 노드의 형태에 따라 결정된다.
 *  
 *     다음과 같은 기능을 위해 사용된다.
 *         - Nested Loop Join 계열
 *         - Sort-based Join 계열
 *         - Hash-based Join 계열
 *         - Full Outer Join의 Anti-Outer 최적화 적용시
 *
 * 용어 설명 :
 *
 * 약어 :
 *
 **********************************************************************/

#ifndef _O_LEFT_OUTER_H_
#define _O_LEFT_OUTER_H_ 1

#include <mt.h>
#include <qmc.h>
#include <qmnDef.h>

//-----------------
// Code Node Flags
//-----------------
// qmncLOJN.flag
// PROJ-2750
#define QMNC_LOJN_SKIP_RIGHT_COND_MASK     (0x00000001)
#define QMNC_LOJN_SKIP_RIGHT_COND_FALSE    (0x00000000)
#define QMNC_LOJN_SKIP_RIGHT_COND_TRUE     (0x00000001)

//-----------------
// Data Node Flags
//-----------------

// qmndLOJN.flag
// First Initialization Done
#define QMND_LOJN_INIT_DONE_MASK           (0x00000001)
#define QMND_LOJN_INIT_DONE_FALSE          (0x00000000)
#define QMND_LOJN_INIT_DONE_TRUE           (0x00000001)

typedef struct qmncLOJN
{
    //---------------------------------
    // Code 영역 공통 정보
    //---------------------------------

    qmnPlan        plan;
    UInt           flag;
    UInt           planID;

    //---------------------------------
    // LOJN 고유 정보
    //---------------------------------

    qtcNode      * filter;    // ON Condition내의 Filter 조건
    
} qmncLOJN;

typedef struct qmndLOJN
{
    //---------------------------------
    // Data 영역 공통 정보
    //---------------------------------

    qmndPlan            plan;
    doItFunc            doIt;        
    UInt              * flag;

    //---------------------------------
    // LOJN 고유 정보
    //---------------------------------
    
    setHitFlagFunc      setHitFlag;  // Child(HASH)에 따른 함수
    UInt                mSkipRightCnt;   // PROJ-2750

} qmndLOJN;

class qmnLOJN
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

private:

    //------------------------
    // mapping by doIt() function pointer
    //------------------------

    // 호출되면 안됨
    static IDE_RC doItDefault( qcTemplate * aTemplate,
                               qmnPlan    * aPlan,
                               qmcRowFlag * aFlag );

    // 새로운 Left Row에 대한 처리
    static IDE_RC doItLeft( qcTemplate * aTemplate,
                            qmnPlan    * aPlan,
                            qmcRowFlag * aFlag );

    // 새로운 Right Row에 대한 처리
    static IDE_RC doItRight( qcTemplate * aTemplate,
                             qmnPlan    * aPlan,
                             qmcRowFlag * aFlag );

    // Inverse Hash 
    static IDE_RC doItInverseLeft( qcTemplate * aTemplate,
                                   qmnPlan    * aPlan,
                                   qmcRowFlag * aFlag );

    static IDE_RC doItInverseRight( qcTemplate * aTemplate,
                                    qmnPlan    * aPlan,
                                    qmcRowFlag * aFlag );

    static IDE_RC doItInverseNonHitFirst( qcTemplate * aTemplate,
                                          qmnPlan    * aPlan,
                                          qmcRowFlag * aFlag );

    static IDE_RC doItInverseNonHitNext( qcTemplate * aTemplate,
                                         qmnPlan    * aPlan,
                                         qmcRowFlag * aFlag );

    //------------------------
    // 초기화 관련 함수
    //------------------------

    // 최초 초기화
    static IDE_RC firstInit( qmncLOJN   * aCodePlan,
                             qmndLOJN   * aDataPlan );

    //------------------------
    // 출력 함수
    //------------------------

    // PROJ-2750
    static void printSkipRightCnt( qcTemplate   * aTemplate,
                                   iduVarString * aString,
                                   UInt           aFlag,
                                   UInt           aSkipRightCnt,
                                   qmnDisplay     aMode );

};

#endif /* _O_LEFT_OUTER_H_ */
