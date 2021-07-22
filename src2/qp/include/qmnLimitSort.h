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
 * $Id: qmnLimitSort.h 82075 2018-01-17 06:39:52Z jina.kim $
 *
 * Description :
 *     LMST(LiMit SorT) Node
 *
 *     관계형 모델에서 sorting 연산을 수행하는 Plan Node 이다.
 *
 *     다음과 같은 기능을 위해 사용된다.
 *         - Limit Order By
 *             : SELECT * FROM T1 ORDER BY I1 LIMIT 10;
 *         - Store And Search
 *             : MIN, MAX 값만을 저장 관리
 *
 * 용어 설명 :
 *
 * 약어 :
 *
 **********************************************************************/

#ifndef _O_QMN_LIMIT_SORT_H_
#define _O_QMN_LIMIT_SORT_H_ 1

#include <mt.h>
#include <qmc.h>
#include <qmcSortTempTable.h>
#include <qmnDef.h>

// Limit Sort를 사용할 수 있는 최대 Limit 개수
#define QMN_LMST_MAXIMUM_LIMIT_CNT              (65536)

//-----------------
// Code Node Flags
//-----------------

// qmncLMST.flag
// LMST 노드의 용도
// - ORDERBY     : Limit Order By를 위한 사용
// - STORESEARCH : Store And Search를 위한 사용

#define QMNC_LMST_USE_MASK                 (0x00000001)
#define QMNC_LMST_USE_ORDERBY              (0x00000000)
#define QMNC_LMST_USE_STORESEARCH          (0x00000001)

//-----------------
// Data Node Flags
//-----------------

// qmndLMST.flag
// First Initialization Done
#define QMND_LMST_INIT_DONE_MASK           (0x00000001)
#define QMND_LMST_INIT_DONE_FALSE          (0x00000000)
#define QMND_LMST_INIT_DONE_TRUE           (0x00000001)

typedef struct qmncLMST
{
    //---------------------------------
    // Code 영역 공통 정보
    //---------------------------------

    qmnPlan        plan;
    UInt           flag;
    UInt           planID;

    //---------------------------------
    // LMST 고유 정보
    //---------------------------------

    UShort         baseTableCount;     // Base Table의 개수
    qmcMtrNode   * myNode;           // 저장 Column 정보

    UShort         depTupleRowID;    // Dependent Tuple ID
    ULong          limitCnt;         // 저장할 Row의 수

    qcComponentInfo * componentInfo; // PROJ-2462 Result Cache
    //---------------------------------
    // Data 영역 정보
    //---------------------------------
    
    UInt           mtrNodeOffset;    // 저장 Column의 위치
    
    
} qmncLMST;

typedef struct qmndLMST
{
    //---------------------------------
    // Data 영역 공통 정보
    //---------------------------------

    qmndPlan            plan;
    doItFunc            doIt;        
    UInt              * flag;        

    //---------------------------------
    // LMST 고유 정보
    //---------------------------------

    UInt                mtrRowSize;   // 저장 Row의 크기

    qmdMtrNode        * mtrNode;      // 저장 Column 정보
    qmdMtrNode        * sortNode;     // 정렬 Column의 위치

    mtcTuple          * depTuple;     // Dependent Tuple 위치
    UInt                depValue;     // Dependent Value 

    qmcdSortTemp      * sortMgr;      // Sort Temp Table
    ULong               mtrTotalCnt;  // 저장 대상 Row의 개수
    idBool              isNullStore; 
    /* PROJ-2462 Result Cache */
    qmndResultCache     resultData;
} qmndLMST;

class qmnLMST
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

    //-----------------------------
    // 초기화 관련 함수
    //-----------------------------

    // 최초 초기화 함수
    static IDE_RC firstInit( qcTemplate * aTemplate,
                             qmncLMST   * aCodePlan,
                             qmndLMST   * aDataPlan );

    // 저장 Column의 초기화
    static IDE_RC initMtrNode( qcTemplate * aTemplate,
                               qmncLMST   * aCodePlan,
                               qmndLMST   * aDataPlan );

    // 정렬 Column의 위치 검색
    static IDE_RC initSortNode( qmncLMST   * aCodePlan,
                                qmndLMST   * aDataPlan );

    // Temp Table 초기화
    static IDE_RC initTempTable( qcTemplate * aTemplate,
                                 qmncLMST   * aCodePlan,
                                 qmndLMST   * aDataPlan );

    // Dependent Tuple의 변경 여부 검사
    static IDE_RC checkDependency( qcTemplate * aTemplate,
                                   qmncLMST   * aCodePlan,
                                   qmndLMST   * aDataPlan,
                                   idBool     * aDependent );

    //------------------------
    // mapping by doIt() function pointer
    //------------------------

    // 호출되어서는 안됨.
    static IDE_RC doItDefault( qcTemplate * aTemplate,
                               qmnPlan    * aPlan,
                               qmcRowFlag * aFlag );

    // LIMIT ORDER BY를 위한 최초 수행
    static IDE_RC doItFirstOrderBy( qcTemplate * aTemplate,
                                    qmnPlan    * aPlan,
                                    qmcRowFlag * aFlag );

    // LIMIT ORDER BY를 위한 다음 수행
    static IDE_RC doItNextOrderBy( qcTemplate * aTemplate,
                                   qmnPlan    * aPlan,
                                   qmcRowFlag * aFlag );

    // Store And Search를 위한 최초 수행
    static IDE_RC doItFirstStoreSearch( qcTemplate * aTemplate,
                                        qmnPlan    * aPlan,
                                        qmcRowFlag * aFlag );

    // Store And Search를 위한 다음 수행
    static IDE_RC doItNextStoreSearch( qcTemplate * aTemplate,
                                       qmnPlan    * aPlan,
                                       qmcRowFlag * aFlag );

    // Store And Search를 위한 마지막 수행
    static IDE_RC doItLastStoreSearch( qcTemplate * aTemplate,
                                       qmnPlan    * aPlan,
                                       qmcRowFlag * aFlag );

    // BUG-37681 limitCnt 0 일때 수행하는 함수
    static IDE_RC doItAllFalse( qcTemplate * aTemplate,
                                qmnPlan    * aPlan,
                                qmcRowFlag * aFlag );

    //-----------------------------
    // 저장 관련 함수
    //-----------------------------

    // ORDER BY를 위한 저장
    static IDE_RC store4OrderBy( qcTemplate * aTemplate,
                                 qmncLMST   * aCodePlan,
                                 qmndLMST   * aDataPlan );

    // Store And Search를 위한 저장
    static IDE_RC store4StoreSearch( qcTemplate * aTemplate,
                                     qmncLMST   * aCodePlan,
                                     qmndLMST   * aDataPlan );

    // 하나의 Row를 삽입
    static IDE_RC addOneRow( qcTemplate * aTemplate,
                             qmndLMST   * aDataPlan );
    
    // 저장 Row의 구성
    static IDE_RC setMtrRow( qcTemplate * aTemplate,
                             qmndLMST   * aDataPlan );

    //-----------------------------
    // 수행 관련 함수
    //-----------------------------

    // 검색된 저장 Row를 기준으로 Tuple Set을 복원한다.
    static IDE_RC setTupleSet( qcTemplate * aTemplate,
                               qmndLMST   * aDataPlan );
    
};



#endif /* _O_QMN_LIMIT_SORT_H_ */

