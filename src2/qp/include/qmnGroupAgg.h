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
 * $Id: qmnGroupAgg.h 82075 2018-01-17 06:39:52Z jina.kim $
 *
 * Description :
 *     GRAG(GRoup AGgregation) Node
 *
 *     관계형 모델에서 Hash-based Grouping 연산을 수행하는 Plan Node 이다.
 *
 *     Aggregation과 Group By의 형태에 따라 다음과 같은 수행을 한다.
 *         - Grouping Only
 *         - Aggregation Only
 *         - Group Aggregation
 *
 * 용어 설명 :
 *
 * 약어 :
 *
 **********************************************************************/

#ifndef _O_QMN_GROUG_AGG_H_
#define _O_QMN_GROUG_AGG_H_ 1

#include <mt.h>
#include <qmc.h>
#include <qmcHashTempTable.h>
#include <qmnDef.h>

//-----------------
// Code Node Flags
//-----------------

// PROJ-2444
#define QMNC_GRAG_PARALLEL_STEP_MASK       (0x0000000F)
#define QMNC_GRAG_NOPARALLEL               (0x00000000)
#define QMNC_GRAG_PARALLEL_STEP_AGGR       (0x00000001)
#define QMNC_GRAG_PARALLEL_STEP_MERGE      (0x00000002)

//-----------------
// Data Node Flags
//-----------------

// qmndGRAG.flag
// First Initialization Done
#define QMND_GRAG_INIT_DONE_MASK           (0x00000001)
#define QMND_GRAG_INIT_DONE_FALSE          (0x00000000)
#define QMND_GRAG_INIT_DONE_TRUE           (0x00000001)

/*---------------------------------------------------------------------
 *  Example)
 *      SELECT i0 % 5, SUM(i1), AVG(i2 + 1) FROM T1 GROUP BY i0 % 5;
 *
 *  in qmncGRAG                                       
 *      myNode : SUM(i1)->SUM(i2+1)->COUNT(i2+1)-> i0 % 5
 *  in qmndGRAG
 *      mtrNode  : SUM(i1)->SUM(i2+1)->COUNT(i2+1)-> i0 % 5
 *      aggrNode : SUM(i1)-> .. until groupNode
 *      groupNode : i0 % 5
 ----------------------------------------------------------------------*/

struct qmncGRAG;

typedef struct qmncGRAG
{
    //---------------------------------
    // Code 영역 공통 정보
    //---------------------------------

    qmnPlan        plan;
    UInt           flag;
    UInt           planID;

    //---------------------------------
    // GRAG 관련 정보
    //---------------------------------

    UShort         baseTableCount;      // PROJ-1473    
    qmcMtrNode   * myNode;              // 저장 Column의 정보
    UInt           bucketCnt;           // Bucket의 개수

    UShort         depTupleRowID;       // Dependent Tuple의 ID

    qcComponentInfo * componentInfo;    // PROJ-2462 Result Cache
    //---------------------------------
    // Data 영역 정보
    //---------------------------------
    
    UInt           mtrNodeOffset;       // 저장 Column의 위치
    UInt           aggrNodeOffset;      // Aggr Column의 위치

    //---------------------------------
    // TODO - Grouping Set 관련 정보
    //---------------------------------
    
    // idBool         isMultiGroup;     // Grouping Set의 판단 여부
    // qmsGroupIdNode  * groupIdNode;
    
} qmncGRAG;

typedef struct qmndGRAG
{
    //---------------------------------
    // Data 영역 공통 정보
    //---------------------------------
    qmndPlan            plan;
    doItFunc            doIt;        
    UInt              * flag;        

    //---------------------------------
    // GRAG 고유 정보
    //---------------------------------

    UInt                mtrRowSize;  // 저장 Row의 크기
    
    qmdMtrNode        * mtrNode;     // 저장 Column 정보
    
    UInt                aggrNodeCnt; // Aggr Column 개수
    qmdMtrNode        * aggrNode;    // Aggr Column 정보
    
    qmdMtrNode        * groupNode;   // Grouping Column 의 위치

    qmcdHashTemp      * hashMgr;     // Hash Temp Table

    mtcTuple          * depTuple;    // Dependent Tuple 위치
    UInt                depValue;    // Dependent Value
    idBool              isNoData;    // Grag Data가 없는 경우

    /* PROJ-2462 Result Cache */
    qmndResultCache     resultData;
    void              * resultRow;
} qmndGRAG;

class qmnGRAG
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

    static IDE_RC readyIt( qcTemplate * aTemplate,
                           qmnPlan    * aPlan,
                           UInt         aTID );

    //------------------------
    // mapping by doIt() function pointer
    //------------------------
    
    // 호출되어서는 안됨
    static IDE_RC doItDefault( qcTemplate * aTemplate,
                               qmnPlan    * aPlan,
                               qmcRowFlag * aFlag );

    // 첫번째 수행 함수
    static IDE_RC doItFirst( qcTemplate * aTemplate,
                             qmnPlan    * aPlan,
                             qmcRowFlag * aFlag );

    // 다음 수행 함수
    static IDE_RC doItNext( qcTemplate * aTemplate,
                            qmnPlan    * aPlan,
                            qmcRowFlag * aFlag );

    // Aggregation Only에서의 다음 수행 함수
    static IDE_RC doItNoData( qcTemplate * aTemplate,
                              qmnPlan    * aPlan,
                              qmcRowFlag * aFlag );

private:

    //------------------------
    // 초기화 관련 함수
    //------------------------
    
    // 최초 초기화
    static IDE_RC firstInit( qcTemplate * aTemplate,
                             qmncGRAG   * aCodePlan,
                             qmndGRAG   * aDataPlan );

    // 저장 Column의 초기화
    static IDE_RC initMtrNode( qcTemplate * aTemplate,
                               qmncGRAG   * aCodePlan,
                               qmndGRAG   * aDataPlan );

    // Aggregation Column의 초기화
    static IDE_RC initAggrNode( qcTemplate * aTemplate,
                                qmncGRAG   * aCodePlan,
                                qmndGRAG   * aDataPlan );

    // Grouping Column의 위치 정의
    static IDE_RC initGroupNode( qmncGRAG   * aCodePlan,
                                 qmndGRAG   * aDataPlan );

    // Temp Table 초기화
    static IDE_RC initTempTable( qcTemplate * aTemplate,
                                 qmncGRAG   * aCodePlan,
                                 qmndGRAG   * aDataPlan );
    
    // Dependent Tuple의 변경 여부 검사
    static IDE_RC checkDependency( qcTemplate * aTemplate,
                                   qmncGRAG   * aCodePlan,
                                   qmndGRAG   * aDataPlan,
                                   idBool     * aDependent );

    //-----------------------------
    // 저장 관련 함수
    //-----------------------------

    static IDE_RC storeTempTable( qcTemplate * aTemplate,
                                  qmncGRAG   * aCodePlan,
                                  qmndGRAG   * aDataPlan );

    // Aggregation만을 수행하여 하나의 Row를 구성
    static IDE_RC aggregationOnly( qcTemplate * aTemplate,
                                   qmncGRAG   * aCodePlan,
                                   qmndGRAG   * aDataPlan );
    
    // Grouping만 하여 저장
    static IDE_RC groupingOnly( qcTemplate * aTemplate,
                                qmncGRAG   * aCodePlan,
                                qmndGRAG   * aDataPlan );

    // Group Aggregation을 수행하여 저장
    static IDE_RC groupAggregation( qcTemplate * aTemplate,
                                    qmncGRAG   * aCodePlan,
                                    qmndGRAG   * aDataPlan );

    static IDE_RC aggrOnlyMerge( qcTemplate * aTemplate,
                                 qmncGRAG   * aCodePlan,
                                 qmndGRAG   * aDataPlan );

    static IDE_RC groupOnlyMerge( qcTemplate * aTemplate,
                                  qmncGRAG   * aCodePlan,
                                  qmndGRAG   * aDataPlan );

    static IDE_RC groupAggrMerge( qcTemplate * aTemplate,
                                  qmncGRAG   * aCodePlan,
                                  qmndGRAG   * aDataPlan );

    // PROJ-1473
    static IDE_RC setBaseColumnRow( qcTemplate  * aTemplate,
                                    qmncGRAG    * aCodePlan,
                                    qmndGRAG    * aDataPlan );    

    // Grouping Column의 값을 설정
    static IDE_RC setGroupRow( qcTemplate * aTemplate,
                               qmndGRAG   * aDataPlan );

    // Aggregation Column에 대한 초기화 수행
    static IDE_RC aggrInit( qcTemplate * aTemplate,
                            qmndGRAG   * aDataPlan );

    // Aggregation Column에 대한 연산 수행
    static IDE_RC aggrDoIt( qcTemplate * aTemplate,
                            qmndGRAG   * aDataPlan );

    //-----------------------------
    // 수행 관련 함수
    //-----------------------------

    static IDE_RC aggrMerge( qcTemplate * aTemplate,
                             qmndGRAG   * aDataPlan,
                             void       * aSrcRow );

    // Aggregation Column에 대한 마무리
    static IDE_RC aggrFini( qcTemplate * aTemplate,
                            qmndGRAG   * aDataPlan );

    // 저장 Row를 이용하여 Tuple Set을 복원
    static IDE_RC setTupleSet( qcTemplate * aTemplate,
                               qmncGRAG   * aCodePlan,
                               qmndGRAG   * aDataPlan );

    // TODO - A4 Grouping Set Integration
    /*
    static IDE_RC isCurrentGroupingColumn( qcStatement * aStatement,
                                           qtcNode     * aGroupNode,
                                           qmdMtrNode  * aGroup,
                                           idBool      * aExist );
    */
};

#endif /* _O_QMN_GROUG_AGG_H_ */

