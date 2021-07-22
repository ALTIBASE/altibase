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
 * $Id: qmnFilter.h 20233 2007-08-06 01:58:21Z sungminee $ 
 *
 * Description :
 *     CNTR(CouNTeR) Node
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

#ifndef _O_QMN_CNTR_H_
#define _O_QMN_CNTR_H_ 1

#include <mt.h>
#include <qmc.h>
#include <qmnDef.h>

//-----------------
// Code Node Flags
//-----------------


//-----------------
// Data Node Flags
//-----------------

/* qmndCNTR.flag                                     */
// First Initialization Done
#define QMND_CNTR_INIT_DONE_MASK           (0x00000001)
#define QMND_CNTR_INIT_DONE_FALSE          (0x00000000)
#define QMND_CNTR_INIT_DONE_TRUE           (0x00000001)

typedef struct qmncCNTR  
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
    
    qtcNode      * stopFilter;     // Stop Filter
    
    //---------------------------------
    // Rownum 노드
    //---------------------------------

    UShort         rownumRowID;    // rownum row
    
} qmncCNTR;

typedef struct qmndCNTR
{
    //---------------------------------
    // Data 영역 공통 정보
    //---------------------------------
    qmndPlan       plan;
    doItFunc       doIt;
    UInt         * flag;

    //---------------------------------
    // CNTR 고유 정보
    //---------------------------------

    mtcTuple     * rownumTuple; // rownum of current row.
    
    SLong        * rownumPtr;
    SLong          rownumValue;
    
} qmndCNTR;

class qmnCNTR
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
    
    // CNTR를 수행
    static IDE_RC doItFirst( qcTemplate * aTemplate,
                             qmnPlan    * aPlan,
                             qmcRowFlag * aFlag );

    // 다음 CNTR를 수행
    static IDE_RC doItNext( qcTemplate * aTemplate,
                            qmnPlan    * aPlan,
                            qmcRowFlag * aFlag );
    
private:

    //------------------------
    // 초기화 관련 함수
    //------------------------

    // 최초 초기화
    static IDE_RC firstInit( qcTemplate * aTemplate,
                             qmncCNTR   * aCodePlan,
                             qmndCNTR   * aDataPlan );

    //------------------------
    // Plan Display 관련 함수
    //------------------------

    // Predicate의 상세 정보를 출력한다.
    static IDE_RC printPredicateInfo( qcTemplate   * aTemplate,
                                      qmncCNTR     * aCodePlan,
                                      qmndCNTR     * aDataPlan,
                                      ULong          aDepth,
                                      iduVarString * aString );

};

#endif /* _O_QMN_CNTR_H_ */
