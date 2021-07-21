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
 * $Id: qmnFullOuter.h 82075 2018-01-17 06:39:52Z jina.kim $
 *
 * Description :
 *     FOJN(Full Outer JoiN) Node
 *
 *     관계형 모델에서 Full Outer Join를 수행하는 Plan Node 이다.
 *     다양한 Join Method들은 하위 노드의 형태에 따라 결정된다.
 *  
 *     다음과 같은 기능을 위해 사용된다.
 *         - Nested Loop Join 계열
 *         - Sort-based Join 계열
 *         - Hash-based Join 계열
 *
 *     Full Outer Join은 크게 다음과 같은 두 단계로 수행된다.
 *         - Left Outer Join Phase
 *             : Left Outer Join과 동일하나, 만족하는 Right Row에 대하여
 *               Hit Flag을 Setting하여 다음 Phase에 대한 처리를 준비한다.
 *         - Right Outer Join Phase
 *             : Right Row중 Hit 되지 않은 Row만 골라 Left에 대한
 *               Null Padding을 수행한다.
 *           
 * 용어 설명 :
 *
 * 약어 :
 *
 **********************************************************************/

#ifndef _O_QMN_FULL_OUTER_H_
#define _O_QMN_FULL_OUTER_H_ 1

#include <mt.h>
#include <qmc.h>
#include <qmnDef.h>

//-----------------
// Code Node Flags
//-----------------

// qmncFOJN.flag
// Right Child Plan의 종류
#define QMNC_FOJN_RIGHT_CHILD_MASK         (0x00000001)
#define QMNC_FOJN_RIGHT_CHILD_HASH         (0x00000000)
#define QMNC_FOJN_RIGHT_CHILD_SORT         (0x00000001)

//-----------------
// Data Node Flags
//-----------------

// qmndFOJN.flag
// First Initialization Done
#define QMND_FOJN_INIT_DONE_MASK           (0x00000001)
#define QMND_FOJN_INIT_DONE_FALSE          (0x00000000)
#define QMND_FOJN_INIT_DONE_TRUE           (0x00000001)

typedef struct qmncFOJN
{
    //---------------------------------
    // Code 영역 공통 정보
    //---------------------------------

    qmnPlan        plan;
    UInt           flag;
    UInt           planID;

    //---------------------------------
    // FOJN 고유 정보
    //---------------------------------

    qtcNode      * filter;     // ON Condition 내의 Filter 조건

} qmncFOJN;

typedef struct qmndFOJN
{
    //---------------------------------
    // Data 영역 공통 정보
    //---------------------------------
    qmndPlan            plan;
    doItFunc            doIt;        
    UInt              * flag;

    //---------------------------------
    // FOJN 고유 정보
    //---------------------------------
    
    setHitFlagFunc      setHitFlag;  // Child(HASH, SORT)에 따른 함수

} qmndFOJN;

class qmnFOJN
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

    // Left Outer Join Phase : 새로운 Left Row에 대한 처리
    static IDE_RC doItLeftHitPhase( qcTemplate * aTemplate,
                                    qmnPlan    * aPlan,
                                    qmcRowFlag * aFlag );

    // Left Outer Join Phase : 새로운 Right Row에 대한 처리
    static IDE_RC doItRightHitPhase( qcTemplate * aTemplate,
                                     qmnPlan    * aPlan,
                                     qmcRowFlag * aFlag );

    // Right Outer Join Phase : 최초 수행 함수
    static IDE_RC doItFirstNonHitPhase( qcTemplate * aTemplate,
                                        qmnPlan    * aPlan,
                                        qmcRowFlag * aFlag );

    // Right Outer Join Phase : 다음 수행 함수
    static IDE_RC doItNextNonHitPhase( qcTemplate * aTemplate,
                                       qmnPlan    * aPlan,
                                       qmcRowFlag * aFlag );

private:

    //------------------------
    // 초기화 관련 함수
    //------------------------

    // 최초 초기화
    static IDE_RC firstInit( qmncFOJN   * aCodePlan,
                             qmndFOJN   * aDataPlan );

};

#endif /* _O_QMN_FULL_OUTER_H_ */
