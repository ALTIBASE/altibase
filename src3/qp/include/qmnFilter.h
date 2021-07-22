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
 * $Id: qmnFilter.h 82075 2018-01-17 06:39:52Z jina.kim $ 
 *
 * Description :
 *     FILT(FILTer) Node
 *
 *     관계형 모델에서 selection을 수행하는 Plan Node 이다.
 *     SCAN 노드와 달리 Storage Manager에 직접 접근하지 않고,
 *     이미 selection된 record에 대한 selection을 수행한다..
 *
 * 용어 설명 :
 *
 * 약어 :
 *
 **********************************************************************/

#ifndef _O_QMN_FILT_H_
#define _O_QMN_FILT_H_ 1

#include <mt.h>
#include <qmc.h>
#include <qmnDef.h>

//-----------------
// Code Node Flags
//-----------------


//-----------------
// Data Node Flags
//-----------------

/* qmndFILT.flag                                     */
// First Initialization Done
#define QMND_FILT_INIT_DONE_MASK           (0x00000001)
#define QMND_FILT_INIT_DONE_FALSE          (0x00000000)
#define QMND_FILT_INIT_DONE_TRUE           (0x00000001)

/*---------------------------------------------------------------------
 *  Example)
 *      SELECT * FROM T1,T2 WHERE T1.i1 = T2.i1 AND T1.i2 > T2.ii + T1.i3;
 *                                                  ^^^^^^^^^^^^^^^^^^^^^
 *                           {qtc > }
 *                               |
 *                       ---------------
 *                       |             |
 *                  {qtc T1.i2}   { qtc + }
 *                                     |
 *                             -----------------
 *                             |                |
 *                       {qtc T2.i1}       { qtc T1.i3 }
 *
 *  in qmncFILT                                       
 *      filter : qtc >
 *  in qmndFILT
 ----------------------------------------------------------------------*/

typedef struct qmncFILT  
{
    //---------------------------------
    // Code 영역 공통 정보
    //---------------------------------

    qmnPlan        plan;
    UInt           flag;
    UInt           planID;

    //---------------------------------
    // Predicate 종류
    //---------------------------------
    
    qtcNode      * constantFilter;    // Constant Filter
    qtcNode      * filter;            // Normal Filter(Subquery Filter포함)
    
} qmncFILT;

typedef struct qmndFILT
{
    //---------------------------------
    // Data 영역 공통 정보
    //---------------------------------
    qmndPlan       plan;
    doItFunc       doIt;
    UInt         * flag;

} qmndFILT;

class qmnFILT
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

    // 항상 조건을 만족할 수 없는 경우
    static IDE_RC doItAllFalse( qcTemplate * aTemplate,
                                qmnPlan    * aPlan,
                                qmcRowFlag * aFlag );
    
    // FILT를 수행
    static IDE_RC doItFirst( qcTemplate * aTemplate,
                             qmnPlan    * aPlan,
                             qmcRowFlag * aFlag );

private:

    //------------------------
    // 초기화 관련 함수
    //------------------------

    // 최초 초기화
    static IDE_RC firstInit( qmncFILT   * aCodePlan,
                             qmndFILT   * aDataPlan );

    //------------------------
    // Plan Display 관련 함수
    //------------------------

    // Predicate의 상세 정보를 출력한다.
    static IDE_RC printPredicateInfo( qcTemplate   * aTemplate,
                                      qmncFILT     * aCodePlan,
                                      ULong          aDepth,
                                      iduVarString * aString );

};

#endif /* _O_QMN_FILT_H_ */
