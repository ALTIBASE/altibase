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
 * $Id: qmnHashDist.h 82075 2018-01-17 06:39:52Z jina.kim $ 
 *
 * Description :
 *     HSDS(HaSh DiStinct) Node
 *
 *     관계형 모델에서 hash-based distinction 연산을 수행하는 Plan Node 이다.
 *
 *     다음과 같은 기능을 위해 사용된다.
 *         - Hash-based Distinction
 *         - Set Union
 *
 * 용어 설명 :
 *
 * 약어 :
 *
 **********************************************************************/

#ifndef _O_QMN_HASH_DIST_H_
#define _O_QMN_HASH_DIST_H_ 1

#include <mt.h>
#include <qmc.h>
#include <qmcHashTempTable.h>
#include <qmnDef.h>

//-----------------
// Code Node Flags
//-----------------

// qmncHSDS.flag
// HSDS 노드가 Top Query에서 사용되는지의 여부
// Top Query에서 사용될 경우 즉시 결과를 생성할 수 있는 반면,
// Subquery나 View내에 존재하는 경우는 모든 결과를 생성한 후
// 처리하여야 한다.
#define QMNC_HSDS_IN_TOP_MASK                (0x00000001)
#define QMNC_HSDS_IN_TOP_TRUE                (0x00000000)
#define QMNC_HSDS_IN_TOP_FALSE               (0x00000001)

//-----------------
// Data Node Flags
//-----------------

// qmndHSDS.flag
// First Initialization Done
#define QMND_HSDS_INIT_DONE_MASK             (0x00000001)
#define QMND_HSDS_INIT_DONE_FALSE            (0x00000000)
#define QMND_HSDS_INIT_DONE_TRUE             (0x00000001)

/*---------------------------------------------------------------------
 *  Example)
 *      SELECT DISTINCT i1, i2 FROM T1;
 *                                    
 *  in qmncHSDS                                       
 *      myNode : T1 -> T1.i1 -> T1.i2
 *  in qmndHSDS
 *      mtrNode  : T1 -> T1.i1 -> T1.i2
 *      hashNode : T1.i1 -> T1.i2
 ----------------------------------------------------------------------*/

typedef struct qmncHSDS
{
    //---------------------------------
    // Code 영역 공통 정보
    //---------------------------------

    qmnPlan        plan;
    UInt           flag;
    UInt           planID;

    //---------------------------------
    // HASH 고유 정보
    //---------------------------------

    UShort         baseTableCount;  // PROJ-1473    
    qmcMtrNode   * myNode;          // 저장 Column의 정보
    UShort         depTupleRowID;   // Dependent Tuple ID

    UInt           bucketCnt;       // Hash Bucket의 개수

    qcComponentInfo * componentInfo; // PROJ-2462 Result Cache
    //---------------------------------
    // Data 영역 정보
    //---------------------------------

    UInt           mtrNodeOffset;   // 저장 Column의 위치
    
} qmncHSDS;

typedef struct qmndHSDS
{
    //---------------------------------
    // Data 영역 공통 정보
    //---------------------------------

    qmndPlan            plan;
    doItFunc            doIt;        
    UInt              * flag;        

    //---------------------------------
    // HSDS 고유 정보
    //---------------------------------
    
    UInt                mtrRowSize;  // 저장 Row의 크기
    
    qmdMtrNode        * mtrNode;     // 저장 Column의 정보
    qmdMtrNode        * hashNode;    // Hashing Column의 위치

    mtcTuple          * depTuple;    // Dependent Tuple의 위치
    UInt                depValue;    // Dependent Value
    
    qmcdHashTemp      * hashMgr;     // Hash Temp Table

    /* PROJ-2462 Result Cache */
    qmndResultCache     resultData;
} qmndHSDS;

class qmnHSDS
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

    // Top Query에서 사용될 경우의 호출 함수
    static IDE_RC doItDependent( qcTemplate * aTemplate,
                                 qmnPlan    * aPlan,
                                 qmcRowFlag * aFlag );

    // Subquery 내에서 사용될 경우의 최초 수행 함수
    static IDE_RC doItFirstIndependent( qcTemplate * aTemplate,
                                        qmnPlan    * aPlan,
                                        qmcRowFlag * aFlag );

    // Subquery 내에서 사용될 경우의 다음 수행 함수
    static IDE_RC doItNextIndependent( qcTemplate * aTemplate,
                                       qmnPlan    * aPlan,
                                       qmcRowFlag * aFlag );

private:

    //-----------------------------
    // 초기화 관련 함수
    //-----------------------------
    
    // 최초 초기화
    static IDE_RC firstInit( qcTemplate * aTemplate,
                             qmncHSDS   * aCodePlan,
                             qmndHSDS   * aDataPlan );

    // 저장 Column의 초기화
    static IDE_RC initMtrNode( qcTemplate * aTemplate,
                               qmncHSDS   * aCodePlan,
                               qmndHSDS   * aDataPlan );

    // Hashing Column의 시작 위치
    static IDE_RC initHashNode( qmndHSDS   * aDataPlan );

    // Temp Table 초기화
    static IDE_RC initTempTable( qcTemplate * aTemplate,
                                 qmncHSDS   * aCodePlan,
                                 qmndHSDS   * aDataPlan );

    // Dependent Tuple의 변경 여부 검사
    static IDE_RC checkDependency( qcTemplate * aTemplate,
                                   qmncHSDS   * aCodePlan,
                                   qmndHSDS   * aDataPlan,
                                   idBool     * aDependent );

    //-----------------------------
    // 저장 관련 함수
    //-----------------------------

    // 저장 Row의 구성
    static IDE_RC setMtrRow( qcTemplate * aTemplate,
                             qmndHSDS   * aDataPlan );

    //-----------------------------
    // 수행 관련 함수
    //-----------------------------

    // 검색된 Row를 이용한 Tuple Set의 복원
    static IDE_RC setTupleSet( qcTemplate * aTemplate,
                               qmndHSDS   * aDataPlan );
};

#endif /*  _O_QMN_HSDS_DIST_H_ */

