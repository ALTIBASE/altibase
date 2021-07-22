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
 * $Id: qmnMultiBagUnion.h 82075 2018-01-17 06:39:52Z jina.kim $
 *
 * Description :
 *     Multiple BUNI(Multiple Bag Union) Node
 *
 *     관계형 모델에서 Bag Union을 수행하는 Plan Node 이다.
 *  
 *     다음과 같은 기능을 위해 사용된다.
 *         - Multiple Bag Union
 *
 *     Multi Children 에 대한 Data를 모두 리턴한다.
 *
 * 용어 설명 :
 *
 * 약어 :
 *
 **********************************************************************/

#ifndef _O_QMN_MULTI_BAG_UNION_H_
#define _O_QMN_MULTI_BAG_UNION_H_ 1

#include <mt.h>
#include <qmc.h>
#include <qmnDef.h>

//-----------------
// Code Node Flags
//-----------------

//-----------------
// Data Node Flags
//-----------------

// qmndMultiBUNI.flag
// First Initialization Done
#define QMND_MULTI_BUNI_INIT_DONE_MASK           (0x00000001)
#define QMND_MULTI_BUNI_INIT_DONE_FALSE          (0x00000000)
#define QMND_MULTI_BUNI_INIT_DONE_TRUE           (0x00000001)

typedef struct qmncMultiBUNI
{
    //---------------------------------
    // Code 영역 공통 정보
    //---------------------------------

    qmnPlan        plan;
    UInt           flag;
    UInt           planID;
} qmncMultiBUNI;

typedef struct qmndMultiBUNI
{
    //---------------------------------
    // Data 영역 공통 정보
    //---------------------------------

    qmndPlan            plan;
    doItFunc            doIt;        
    UInt              * flag;

    //---------------------------------
    // Multi BUNI 고유 정보
    //---------------------------------

    qmnChildren       * curChild;  // 현재 수행 중인 child
    
} qmndMultiBUNI;

class qmnMultiBUNI
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

private:

    // 최초 초기화
    static IDE_RC firstInit( qmndMultiBUNI   * aDataPlan );

};

#endif /* _O_QMN_MULTI_BAG_UNION_H_ */
