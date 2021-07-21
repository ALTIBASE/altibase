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
 * $Id: qmnAntiOuter.h 82075 2018-01-17 06:39:52Z jina.kim $
 *
 * Description :
 *     AOJN(Anti Outer JoiN) Node
 *
 *     관계형 모델에서 Full Outer Join의 처리를 위해
 *     특수한 연산을 수행하는 Plan Node 이다.
 *
 *     Left Outer Join과 흐름은 유사하나 다음과 같은 큰 차이가 있다.
 *        - Left Row에 만족하는 Right Row가 없는 경우를 검색한다.
 *        
 * 용어 설명 :
 *
 * 약어 :
 *
 **********************************************************************/

#ifndef _O_QMN_ANTI_OUTER_H_
#define _O_QMN_ANTI_OUTER_H_ 1

#include <mt.h>
#include <qmc.h>
#include <qmnDef.h>

//-----------------
// Code Node Flags
//-----------------

//-----------------
// Data Node Flags
//-----------------

// qmndAOJN.flag
// First Initialization Done
#define QMND_AOJN_INIT_DONE_MASK           (0x00000001)
#define QMND_AOJN_INIT_DONE_FALSE          (0x00000000)
#define QMND_AOJN_INIT_DONE_TRUE           (0x00000001)

typedef struct qmncAOJN
{
    //---------------------------------
    // Code 영역 공통 정보
    //---------------------------------

    qmnPlan        plan;
    UInt           flag;
    UInt           planID;

    //---------------------------------
    // AOJN 고유 정보
    //---------------------------------
    
    qtcNode      * filter;    // ON Condition내의 Filter 조건
    
} qmncAOJN;

typedef struct qmndAOJN
{
    //---------------------------------
    // Data 영역 공통 정보
    //---------------------------------
    qmndPlan            plan;
    doItFunc            doIt;
    UInt              * flag;

} qmndAOJN;

class qmnAOJN
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

    // 새로운 Left Row에 대한 처리
    static IDE_RC doItLeft( qcTemplate * aTemplate,
                            qmnPlan    * aPlan,
                            qmcRowFlag * aFlag );

private:

    //------------------------
    // 초기화 관련 함수
    //------------------------
    
    // 최초 초기화
    static IDE_RC firstInit( qmndAOJN   * aDataPlan );

};

#endif /* _O_QMN_ANTI_OUTER_H_ */
