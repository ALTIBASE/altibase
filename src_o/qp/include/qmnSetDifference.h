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
 * $Id: qmnSetDifference.h 82075 2018-01-17 06:39:52Z jina.kim $
 *
 * Description :
 *     SDIF(Set DIFference) Node
 *
 *     관계형 모델에서 hash-based set difference 연산을
 *     수행하는 Plan Node 이다.
 *
 * 용어 설명 :
 *
 * 약어 :
 *
 **********************************************************************/

#ifndef _O_QMN_SET_DIFFERENCE_H_
#define _O_QMN_SET_DIFFERENCE_H_ 1

#include <mt.h>
#include <qmc.h>
#include <qmcHashTempTable.h>
#include <qmnDef.h>

//-----------------
// Code Node Flags
//-----------------

//-----------------
// Data Node Flags
//-----------------

// qmndSDIF.flag
// First Initialization Done
#define QMND_SDIF_INIT_DONE_MASK           (0x00000001)
#define QMND_SDIF_INIT_DONE_FALSE          (0x00000000)
#define QMND_SDIF_INIT_DONE_TRUE           (0x00000001)

typedef struct qmncSDIF
{
    //---------------------------------
    // Code 영역 공통 정보
    //---------------------------------

    qmnPlan        plan;
    UInt           flag;
    UInt           planID;

    //---------------------------------
    // SDIF 고유 정보
    //---------------------------------

    qmcMtrNode   * myNode;             // 저장 Column의 정보

    UShort         leftDepTupleRowID;  // Left Dependent Tuple
    UShort         rightDepTupleRowID; // Right Dependent Tuple

    UInt           bucketCnt;          // Hash Bucket의 개수

    qcComponentInfo * componentInfo; // PROJ-2462 Result Cache
    //---------------------------------
    // Data 영역 정보
    //---------------------------------
    
    UInt           mtrNodeOffset;      // 저장 Column의 위치
    
} qmncSDIF;

typedef struct qmndSDIF
{
    //---------------------------------
    // Data 영역 공통 정보
    //---------------------------------

    qmndPlan            plan;
    doItFunc            doIt;        
    UInt              * flag;

    //---------------------------------
    // SDIF 고유 정보
    //---------------------------------
    
    UInt                mtrRowSize;    // 저장 Row의 크기

    qmdMtrNode        * mtrNode;       // 저장 Column의 정보

    void              * temporaryRightRow;  // temporary right-side row
    
    mtcTuple          * leftDepTuple;  // Left Dependent Tuple의 위치
    UInt                leftDepValue;  // Left Dependent Value

    mtcTuple          * rightDepTuple; // Right Dependent Tuple의 위치
    UInt                rightDepValue; // Right Dependent Value

    qmcdHashTemp      * hashMgr;       // Hash Temp Table

    /* PROJ-2462 Result Cache */
    qmndResultCache     resultData;
} qmndSDIF;

class qmnSDIF
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

    // 최초 수행 함수
    static IDE_RC doItFirst( qcTemplate * aTemplate,
                             qmnPlan    * aPlan,
                             qmcRowFlag * aFlag );

    // 다음 수행 함수
    static IDE_RC doItNext( qcTemplate * aTemplate,
                            qmnPlan    * aPlan,
                            qmcRowFlag * aFlag );

private:

    //-----------------------------
    // 초기화 관련 함수
    //-----------------------------

    // 최초 초기화
    static IDE_RC firstInit( qcTemplate * aTemplate,
                             qmncSDIF   * aCodePlan,
                             qmndSDIF   * aDataPlan );

    // 저장 Column의 초기화
    static IDE_RC initMtrNode( qcTemplate * aTemplate,
                               qmncSDIF   * aCodePlan,
                               qmndSDIF   * aDataPlan );

    // Temp Table 초기화
    static IDE_RC initTempTable( qcTemplate * aTemplate,
                                 qmncSDIF   * aCodePlan,
                                 qmndSDIF   * aDataPlan );
    
    // Left Dependent Tuple의 변경 여부 검사
    static IDE_RC checkLeftDependency( qmndSDIF   * aDataPlan,
                                       idBool     * aDependent );

    // Right Dependent Tuple의 변경 여부 검사
    static IDE_RC checkRightDependency( qmndSDIF   * aDataPlan,
                                        idBool     * aDependent );

    //-----------------------------
    // 저장 관련 함수
    //-----------------------------
    
    // Left를 수행하여 distinct hashing 으로 저장
    static IDE_RC storeLeft( qcTemplate * aTemplate,
                             qmncSDIF   * aCodePlan,
                             qmndSDIF   * aDataPlan );

    // Child의 결과를 Stack을 이용하여 저장 Row를 구성
    static IDE_RC setMtrRow( qcTemplate * aTemplate,
                             qmndSDIF   * aDataPlan );

    // Differenced Row들을 모두 설정
    static IDE_RC setDifferencedRows( qcTemplate * aTemplate,
                                      qmncSDIF   * aCodePlan,
                                      qmndSDIF   * aDataPlan );

    // PR-24190
    // select i1(varchar(30)) from t1 minus select i1(varchar(250)) from t2;
    // 수행시 서버 비정상종료
    static IDE_RC setRightChildMtrRow( qcTemplate * aTemplate,
                                       qmndSDIF   * aDataPlan,
                                       idBool     * aIsSetMtrRow );

};


#endif /* _O_QMN_SET_DIFFERENCE_H_ */
