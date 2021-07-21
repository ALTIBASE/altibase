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
 * $Id: qmnAggregation.h 82075 2018-01-17 06:39:52Z jina.kim $
 *
 * Description :
 *     AGGR(AGGRegation) Node
 *
 *     관계형 모델에서 다음과 같은 기능을 수행하는 Plan Node 이다.
 *
 *         - Sort-based Grouping
 *         - Distinct Aggregation
 *
 *    - Distinct Column을 Disk Temp Table을 사용하던, Memory Temp Table을
 *      사용에 관계 없이 AGGR Node의 Row 저장을 위한 Tuple Set의 정보는
 *      Memory Storage여야 한다.
 *
 *    - Distinct Column을  Disk에 저장할 경우, Plan의 flag정보는 DISK가 되며,
 *      각 Distinct Column을 위한 Tuple Set의 Storage 역시 Disk Type이 된다.
 *
 * 용어 설명 :
 *
 * 약어 :
 *
 **********************************************************************/

#ifndef _O_QMN_AGGREGATION_H_
#define _O_QMN_AGGREGATION_H_ 1

#include <mt.h>
#include <qmc.h>
#include <qmcHashTempTable.h>
#include <qmnDef.h>


//-----------------
// Code Node Flags
//-----------------

// qmncAGGR.flag - has child GRBY
// GROUP BY가 존재하는 지의 여부
#define QMNC_AGGR_GROUPED_MASK             (0x00000001)
#define QMNC_AGGR_GROUPED_FALSE            (0x00000000)
#define QMNC_AGGR_GROUPED_TRUE             (0x00000001)

//-----------------
// Data Node Flags
//-----------------

// qmndAGGR.flag
// First Initialization Done
#define QMND_AGGR_INIT_DONE_MASK           (0x00000001)
#define QMND_AGGR_INIT_DONE_FALSE          (0x00000000)
#define QMND_AGGR_INIT_DONE_TRUE           (0x00000001)

//------------------------------------------
// [qmdDistNode]
//    Aggregation의 Distinct Argument에 대한
//    저장 관리를 담당하는 노드이다.
//    (qmdMtrNode)와 강제적으로 Type Casting되므로
//    그 자료 구조의 변경에 주의하여야 한다.
//------------------------------------------

typedef struct qmdDistNode
{
    qtcNode           * dstNode;
    mtcTuple          * dstTuple;
    mtcColumn         * dstColumn;
    qmdDistNode       * next;
    qtcNode           * srcNode;
    mtcTuple          * srcTuple;
    mtcColumn         * srcColumn;

    qmcMtrNode        * myNode;
    qmdMtrFunction      func;
    UInt                flag;

    // qmdMtrNode로 casting하기 때문에 qmdMtrNode와 동일하게 추가한다.
    smiFetchColumnList * fetchColumnList; // PROJ-1705
    mtcTemplate        * tmplate;
    
    // for qmdDistNode
    qmcdHashTemp        hashMgr;    // Hash Temp Table
    void              * mtrRow;
    idBool              isDistinct; // Column의 Distinction 여부
} qmdDistNode;

//------------------------------------------
// [qmdAggrNode]
//    Aggregation을 담당하는 노드로
//    Distinct Argument의 존재 유무에 따라
//    다른 처리를 관리하는 노드이다.
//    (qmdMtrNode)와 강제적으로 Type Casting되므로
//    그 자료 구조의 변경에 주의하여야 한다.
//------------------------------------------

typedef struct qmdAggrNode
{
    qtcNode        * dstNode;
    mtcTuple       * dstTuple;
    mtcColumn      * dstColumn;
    qmdAggrNode    * next;
    qtcNode        * srcNode;
    mtcTuple       * srcTuple;
    mtcColumn      * srcColumn;

    qmcMtrNode     * myNode;
    qmdMtrFunction   func;
    UInt             flag;

    // qmdMtrNode로 casting하기 때문에 qmdMtrNode와 동일하게 추가한다.
    smiFetchColumnList * fetchColumnList; // PROJ-1705
    mtcTemplate        * tmplate;
    
    // for qmdAggrNode
    // if DISTINCT AGGREGATION available, or NULL
    qmdDistNode    * myDist;
} qmdAggrNode;

/*---------------------------------------------------------------------
 *  Example)
 *      SELECT i0, SUM(i2 + 1), i1, COUNT(DISTINCT i2), SUM(DISTINCT i2)
 *      FROM T1 GROUP BY i0, i1;
 *
 *  in qmncAGGR
 *      myNode :   SUM() -> COUNT(D) -> SUM(D) -> i0 -> i1
 *      distNode : DISTINCT i2
 *  in qmndAGGR
 *      mtrNode   : SUM() -> COUNT(D) -> SUM(D) -> i0 -> i1
 *      aggrNode  : SUM() -> COUNT(D) -> SUM(D)
 *      distNode  : DISTINCT i2
 *      groupNode : i0 (->i1)
 ----------------------------------------------------------------------*/

typedef struct qmncAGGR
{
    //---------------------------------
    // Code 영역 공통 정보
    //---------------------------------

    qmnPlan        plan;
    UInt           flag;
    UInt           planID;

    //---------------------------------
    // AGGR 관련 정보
    //---------------------------------

    // Base Table 정보를 제거한다.
    // Aggregation의 경우 자연스럽게 Push Projection효과가 생기기 때문이다.
    UShort         baseTableCount;
    qmcMtrNode   * myNode;
    qmcMtrNode   * distNode;          // distinct aggregation의 argument

    //---------------------------------
    // Data 영역 정보
    //---------------------------------

    UInt           mtrNodeOffset;     // 저장 Column의 Data 영역 위치
    UInt           aggrNodeOffset;    // Aggregation의 Data 영역 위치
    UInt           distNodeOffset;    // distinction의 Data 영역 위치

} qmncAGGR;


typedef struct qmndAGGR
{
    //---------------------------------
    // Data 영역 공통 정보
    //---------------------------------
    qmndPlan            plan;
    doItFunc            doIt;
    UInt              * flag;

    //---------------------------------
    // AGGR 고유 정보
    //---------------------------------

    qmdMtrNode        * mtrNode;     // 저장 Column 정보
    qmdMtrNode        * groupNode;   // Group Column의 위치

    UInt                distNodeCnt; // Distinct Argument의 개수
    qmdDistNode       * distNode;    // Distinct 저장 Column 정보
    UInt                aggrNodeCnt; // Aggr Column 개수
    qmdAggrNode       * aggrNode;    // Aggr Column 정보

    //---------------------------------
    // 서로 다른 Group을 위한 자료 구조
    //---------------------------------

    UInt                mtrRowSize;  // 저장할 Row의 Size
    void              * mtrRow[2];   // 결과 관리를 위한 두 개의 공간
    UInt                mtrRowIdx;   // wap within mtrRow[0] and mtr2Row[1]

    void              * nullRow;     // Null Row
} qmndAGGR;

class qmnAGGR
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

    // One-Group Aggregation을 수행
    static IDE_RC doItAggregation( qcTemplate * aTemplate,
                                   qmnPlan    * aPlan,
                                   qmcRowFlag * aFlag );

    // Multi-Group Aggregation을 최초 수행
    static IDE_RC doItGroupAggregation( qcTemplate * aTemplate,
                                        qmnPlan    * aPlan,
                                        qmcRowFlag * aFlag );

    // Multi-Group Aggregation의 다음 수행
    static IDE_RC doItNext( qcTemplate * aTemplate,
                            qmnPlan    * aPlan,
                            qmcRowFlag * aFlag );

    // Record 없음을 리턴
    static IDE_RC doItLast( qcTemplate * aTemplate,
                            qmnPlan    * aPlan,
                            qmcRowFlag * aFlag );

private:

    //------------------------
    // 초기화 관련 함수
    //------------------------

    // 최초 초기화
    static IDE_RC firstInit( qcTemplate * aTemplate,
                             qmncAGGR   * aCodePlan,
                             qmndAGGR   * aDataPlan );

    // 저장 Column의 초기화
    static IDE_RC initMtrNode( qcTemplate * aTemplate,
                               qmncAGGR   * aCodePlan,
                               qmndAGGR   * aDataPlan );

    // Distinct Column의 초기화
    static IDE_RC initDistNode( qcTemplate * aTemplate,
                                qmncAGGR   * aCodePlan,
                                qmndAGGR   * aDataPlan );

    // Aggregation Column의 초기화
    static IDE_RC initAggrNode( qcTemplate * aTemplate,
                                qmncAGGR   * aCodePlan,
                                qmndAGGR   * aDataPlan );

    // Aggregation Column정보의 연결
    static IDE_RC linkAggrNode( qmncAGGR   * aCodeNode,
                                qmndAGGR   * aDataNode );

    // Group Column의 초기화
    static IDE_RC initGroupNode( qmncAGGR   * aCodePlan,
                                 qmndAGGR   * aDataPlan );

    // 두 Row의 저장 공간 마련
    static IDE_RC allocMtrRow( qcTemplate * aTemplate,
                               qmndAGGR   * aDataPlan );

    // Null Row의 생성
    static IDE_RC makeNullRow(qcTemplate * aTemplate,
                              qmndAGGR   * aDataPlan );

    //----------------------------------------------------
    // AGGR 의 doIt() 관련 함수
    //----------------------------------------------------

    //-----------------------------
    // Aggregation 초기화 과정
    //-----------------------------

    // Distinct Column을 위한 Temp Table을 Clear함
    static IDE_RC clearDistNode( qmndAGGR   * aDataPlan );

    // Aggregation의 초기화와 Group 값을 설정
    static IDE_RC initAggregation( qcTemplate * aTemplate,
                                   qmndAGGR   * aDataPlan );

    // Grouping 값을 설정
    static IDE_RC setGroupColumns( qcTemplate * aTemplate,
                                   qmndAGGR   * aDataPlan );

    //-----------------------------
    // Aggregation 수행 과정
    //-----------------------------

    // Aggregation을 수행한다.
    static IDE_RC execAggregation( qcTemplate * aTemplate,
                                   qmndAGGR   * aDataPlan );

    // Aggregation Argument의 Distinction 여부를 판단
    static IDE_RC setDistMtrColumns( qcTemplate * aTemplate,
                                     qmndAGGR * aDataPlan );

    //-----------------------------
    // Aggregation 마무리 과정
    //-----------------------------

    // Aggregation을 마무리
    static IDE_RC finiAggregation( qcTemplate * aTemplate,
                                   qmndAGGR   * aDataPlan );

    // 결과를 Tuple Set에 설정
    static IDE_RC setTupleSet( qcTemplate * aTemplate,
                               qmndAGGR   * aDataPlan );

    // 새로운 Group이 올라온 경우의 처리
    static IDE_RC setNewGroup( qcTemplate * aTemplate,
                               qmndAGGR   * aDataPlan );

    //-----------------------------
    // A4 - Grouping Set 관련 함수
    //-----------------------------

    /*
      static IDE_RC isCurrentGroupingColumn( qcStatement * aStatement,
      qtcNode     * aGroupNode,
      qtcNode     * aMtrRelGraphNode,
      idBool      * aExist );
    */
};

#endif /* _O_QMN_AGGREGATION_H_ */
