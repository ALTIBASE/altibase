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
 * $Id: qmnWindowSort.h 28920 2008-10-28 00:35:44Z jmkim $
 *
 * Description :
 *    WNST(WiNdow SorT) Node
 *
 * 용어 설명 :
 *    같은 의미를 가지는 서로 다른 단어를 정리하면 아래와 같다.
 *    - Analytic Funtion = Window Function
 *    - Analytic Clause = Window Clause = Over Clause
 *
 * 약어 :
 *    WNST(Window Sort)
 *
 **********************************************************************/

#ifndef _O_QMN_WNST_H_
#define _O_QMN_WNST_H_ 1

#include <mt.h>
#include <mtdTypes.h>
#include <qmc.h>
#include <qmcSortTempTable.h>
#include <qmnDef.h>
#include <qmnAggregation.h>

// BUG-40409 row_number_limit
#define QMN_WNST_PUSH_RANK_MAX      (1024)

//-----------------
// Code Node Flags
//-----------------

/* qmncWNST.flag                                    */
/* WNST의 저장 방식을 지정  */
#define QMNC_WNST_STORE_MASK                   (0x00000007)
#define QMNC_WNST_STORE_NONE                   (0x00000000)
#define QMNC_WNST_STORE_ONLY                   (0x00000001)
#define QMNC_WNST_STORE_PRESERVED_ORDER        (0x00000002)
#define QMNC_WNST_STORE_SORTING                (0x00000003)
#define QMNC_WNST_STORE_LIMIT_PRESERVED_ORDER  (0x00000004)
#define QMNC_WNST_STORE_LIMIT_SORTING          (0x00000005)

//-----------------
// Data Node Flags
//-----------------

/* qmndWNST.flag                                     */
// First Initialization Done
#define QMND_WNST_INIT_DONE_MASK           (0x00000001)
#define QMND_WNST_INIT_DONE_FALSE          (0x00000000)
#define QMND_WNST_INIT_DONE_TRUE           (0x00000001)

// BUG-33663 Ranking Function
// window node의 수행방식(aggregate와 update) 종류
typedef enum qmcWndExecMethod
{
    QMC_WND_EXEC_NONE = 0,
    QMC_WND_EXEC_PARTITION_ORDER_UPDATE,
    QMC_WND_EXEC_PARTITION_UPDATE,
    QMC_WND_EXEC_ORDER_UPDATE,
    QMC_WND_EXEC_AGGR_UPDATE,
    QMC_WND_EXEC_PARTITION_ORDER_WINDOW_UPDATE,
    QMC_WND_EXEC_ORDER_WINDOW_UPDATE,
    QMC_WND_EXEC_PARTITION_ORDER_UPDATE_LAG,
    QMC_WND_EXEC_PARTITION_ORDER_UPDATE_LEAD,
    QMC_WND_EXEC_ORDER_UPDATE_LAG,
    QMC_WND_EXEC_ORDER_UPDATE_LEAD,
    QMC_WND_EXEC_PARTITION_ORDER_UPDATE_NTILE,
    QMC_WND_EXEC_ORDER_UPDATE_NTILE,
    QMC_WND_EXEC_MAX
} qmcWndExecMethod;

typedef enum qmcWndWindowValueType
{
    QMC_WND_WINDOW_VALUE_LONG = 0,
    QMC_WND_WINDOW_VALUE_DOUBLE,
    QMC_WND_WINDOW_VALUE_DATE,
    QMC_WND_WINDOW_VALUE_NUMERIC,
    QMC_WND_WINDOW_VALUE_NULL,
    QMC_WND_WINDOW_VALUE_MAX
} qmcWndWindowValueType;

typedef struct qmcWndWindowValue
{
    qmcWndWindowValueType type;
    SLong                 longType;
    SDouble               doubleType;
    mtdDateType           dateType;
    mtdDateField          dateField;
    SChar                 numericType[MTD_NUMERIC_SIZE_MAXIMUM];
} qmcWndWindowValue;

typedef struct qmcWndWindow
{
    UInt             rowsOrRange;
    qtcOverWindowOpt startOpt;
    qtcWindowValue   startValue;
    qtcOverWindowOpt endOpt;
    qtcWindowValue   endValue;
} qmcWndWindow;

typedef struct qmcWndNode
{
    qmcMtrNode       * overColumnNode;   /* 'PARTITION BY','ORDER BY' 정보 */
    qmcMtrNode       * aggrNode;         /* 분석 함수 정보 */
    qmcMtrNode       * aggrResultNode;   /* 위 분석 함수 결과가 저장될 Sort Temp의 칼럼 정보 */
    qmcWndWindow       window;           /* PROJ-1805 window clause */
    qmcWndExecMethod   execMethod;       /* 분석 함수의 수행방식 */
    qmcWndNode       * next;
} qmcWndNode;

typedef struct qmdWndNode
{
    qmdMtrNode       * overColumnNode;    /* 'PARTITION BY','ORDER BY' 정보 */
    qmdMtrNode       * orderByColumnNode; /* 'ORDER BY' 정보 포인터 */
    qmdAggrNode      * aggrNode;          /* 분석 함수 정보 */
    qmdMtrNode       * aggrResultNode;    /* 위 분석 함수 결과가 저장될 Sort Temp의 칼럼 정보 */
    qmcWndWindow     * window;
    qmcWndExecMethod   execMethod;        /* 분석 함수의 수행방식 */
    qmdWndNode       * next;
} qmdWndNode;



/*---------------------------------------------------------------------
 * Code Plan의 구성
 *
 * SELECT sum(i1) OVER ( PARTITION BY i1 ) AS F1,
 *        sum(i2) OVER ( PARTITION BY i1 ) AS F2,
 *        sum(i2) OVER ( PARTITION BY i2 ) AS F3,
 *        sum(i2) OVER ( PARTITION BY i1, i2 ) AS F4,
 *        sum(DISTINCT i2) OVER ( PARTITION BY i1 ) AS F5,
 *   FROM T1;
 *
 * 위의 쿼리는 아래와 같은 Code Plan을 가진다.
 * Data Plan도 유사한 구조를 가지므로 이를 참고하면 이해하는데 도움이 됨
 * - F1,F2,F3,F4,F5: aggregation 함수 결과만을 저장하는 칼럼
 * - @F1,@F2,@F3,@F4,@F5: aggregation 함수의 중간 결과를 저장하는 칼럼
 *
 *    qmncWNST
 *   +--------+
 *   |  myNode|-->i1-->i2-->F1-->F2-->F3-->F4-->F5
 *   |aggrNode|-->@F1-->@F2-->@F3-->@F4-->@F5
 *   |distNode|-->i2
 *   |sortNode|-------------------->[*][*]-------->i2
 *   | wndNode|-->[*][*]            /
 *   +--------+    |  |            i1-->i2
 *                 |  \____________               qmcWndNode
 *         qmcWndNode              \_____________+--------------+
 *        +--------------+                       |overColumnNode|-->i2
 *        |overColumnNode|-->i1                  |      aggrNode|-->@F3
 *        |      aggrNode|-->@F1-->@F2-->@F5     |aggrResultNode|-->F3
 *        |aggrResultNode|-->F1--->F2--->F5      |          next|
 *        |          next|                       +--------------+
 *        +-----------|--+
 *                    |
 *         qmcWndNode |
 *        +--------------+
 *        |overColumnNode|-->i1-->i2
 *        |      aggrNode|-->@F4
 *        |aggrResultNode|-->F4
 *        |          next|
 *        +--------------+
 *
 ----------------------------------------------------------------------*/

typedef struct qmncWNST
{
    //---------------------------------
    // Code 영역 공통 정보
    //---------------------------------

    qmnPlan        plan;
    UInt           flag;
    UInt           planID;

    //---------------------------------
    // WNST 고유 정보
    //---------------------------------

    // 필요한 mtrNode
    qmcMtrNode    * myNode;       // Sort Temp Table에 저장 칼럼의 정보
    qmcMtrNode    * distNode;     // Aggregation함수의 인자의 distinct 정보
                                  // ex) SUM( DISTINCT i1 )
    qmcMtrNode    * aggrNode;     // Aggregation 중간 결과 저장을 위한 노드
    qmcMtrNode   ** sortNode;     // 정렬키의 집합 (하나의 정렬키는 다수의 칼럼으로 구성)
    qmcWndNode   ** wndNode;      // 위 정렬키에 해당하는 Window Clause 정보
                                  // ( =Analytic Clause 정보 )
    // mtrNode와 관련된 count 정보
    SDouble         storeRowCount;    // Sort Node에 저장될 레코드 갯수 ( 예측값임 )
    UInt            sortKeyCnt; // 정렬키의 개수
    UShort          baseTableCount;

    // dependency
    UShort          depTupleRowID; // Dependent Tuple ID
    qcComponentInfo * componentInfo; // PROJ-2462 Result Cache
    //---------------------------------
    // Data 영역 정보
    //---------------------------------

    UInt           mtrNodeOffset;  // 저장 Column의 Data 영역 위치
    UInt           distNodeOffset; // distinction의 Data 영역 위치
    UInt           aggrNodeOffset; // Aggregation을 위한 Data 영역 위치
    UInt           sortNodeOffset; // 정렬키를 위한 Column의 Data 영역 위치
    UInt           wndNodeOffset;  // qmdWndNode를 위한 Data 영역 위치
    UInt           sortMgrOffset;  // 임시: PROJ-1431 완료 후 반복 정렬
                                   // 기능 적용시 삭제!
                                   // Sort Manager를 위한 Data 영역의 위치
} qmncWNST;


typedef struct qmndWNST
{
    //---------------------------------
    // Data 영역 공통 정보
    //---------------------------------

    qmndPlan            plan;
    doItFunc            doIt;
    UInt              * flag;

    //---------------------------------
    // WNST 고유 정보
    //---------------------------------

    qmcdSortTemp      * sortMgr;        // 현재 Sort Temp Table Manager
    qmcdSortTemp      * sortMgrForDisk; // Disk Sort의 경우, 미리
                                        // 할당해 놓은 Sort Temp Table
                                        // Manager의 시작 위치 (총
                                        // qmncWNST.sortKeyCnt 만큼
                                        // 할당 )
    qmdMtrNode        * mtrNode; // Sort Temp Table에 저장 칼럼의 정보
    UInt                distNodeCnt; // Distinct Argument의 개수
    qmdDistNode       * distNode;    // Distinct 저장 Column 정보
    qmdAggrNode       * aggrNode;    // 분석 함수 정보    
    qmdMtrNode       ** sortNode; // 정렬키의 집합 (하나의 정렬키는 다수의 칼럼으로 구성)
    qmdWndNode       ** wndNode;

    smiCursorPosInfo    cursorInfo; // Store,Restore시 저장할 위치 정보
    smiCursorPosInfo    partitionCursorInfo;

    mtcTuple          * depTuple; // Dependent Tuple 위치
    UInt                depValue; // Dependent Value

    //---------------------------------
    // 서로 다른 PARTITION을 위한 자료 구조
    //---------------------------------

    UInt                mtrRowSize;  // 저장할 Row의 Size
    void              * mtrRow[2];   // 결과 관리를 위한 두 개의 공간
    UInt                mtrRowIdx;   // 두 저장 공간 중, 현재 row의 offset

    /* PROJ-2462 Result Cache */
    qmndResultCache     resultData;
} qmndWNST;

class qmnWNST
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

    // 최초 수행 함수
    static IDE_RC doItFirst( qcTemplate * aTemplate,
                             qmnPlan    * aPlan,
                             qmcRowFlag * aFlag );

    // 다음 수행 함수
    static IDE_RC doItNext( qcTemplate * aTemplate,
                            qmnPlan    * aPlan,
                            qmcRowFlag * aFlag );
    
private:

    //----------------------------------------------------
    // 초기화 관련 함수
    //----------------------------------------------------

    //------------------------
    // 최초 초기화 관련 함수
    // firstInit()
    //------------------------

    // 최초 초기화
    static IDE_RC firstInit( qcTemplate     * aTemplate,
                             const qmncWNST * aCodePlan,
                             qmndWNST       * aDataPlan );

    // 저장 칼럼 정보를 (Materialize 노드) 설정
    static IDE_RC initMtrNode( qcTemplate     * aTemplate,
                               const qmncWNST * aCodePlan,
                               qmndWNST       * aDataPlan );

    // Analytic Function 인자애 DISTINCT가 있는 경우 정보 설정
    static IDE_RC initDistNode( qcTemplate     * aTemplate,
                                const qmncWNST * aCodePlan,
                                qmdDistNode    * aDistNode,
                                UInt           * aDistNodeCnt );

    // Reporting Aggregation을 처리하는 칼럼(중간값) 정보의 설정
    static IDE_RC initAggrNode( qcTemplate         * aTemplate,
                                const qmcMtrNode   * aCodeNode,
                                const qmdDistNode  * aDistNode,
                                const UInt           aDistNodeCnt,
                                qmdAggrNode        * aAggrNode );
    
    // 모든 정렬키의 정보를 설정
    static IDE_RC initSortNode( const qmncWNST * aCodePlan,
                                const qmndWNST * aDataPlan,
                                qmdMtrNode    ** aSortNode );
    
    // 복사해서 사용하는 mtrNode의 정보 설정
    static IDE_RC initCopiedMtrNode( const qmndWNST   * aDataPlan,
                                     const qmcMtrNode * aCodeNode,
                                     qmdMtrNode       * aDataNode );

    // 복사해서 사용하는 aggrNode의 정보 설정
    static IDE_RC initCopiedAggrNode( const qmndWNST   * aDataPlan,
                                      const qmcMtrNode * aCodeNode,
                                      qmdAggrNode      * aAggrNode );

    // aggrResultNode의 정보 설정 
    static IDE_RC initAggrResultMtrNode( const qmndWNST   * aDataPlan,
                                         const qmcMtrNode * aCodeNode,
                                         qmdMtrNode       * aDataNode );
    
    // Window Clause (Analytic Clause) 정보를 담는 qmdWndNode를 설정
    static IDE_RC initWndNode( const qmncWNST    * aCodePlan,
                               const qmndWNST    * aDataPlan,
                               qmdWndNode       ** aWndNode );

    // Sort Temp Table을 초기화함
    static IDE_RC initTempTable( qcTemplate      * aTemplate,
                                 const qmncWNST  * aCodePlan,
                                 qmndWNST        * aDataPlan,
                                 qmcdSortTemp    * aSortMgr );

    
    //------------------------
    // Analytic Function 수행 관련 함수
    // performAnalyticFunctions()
    //------------------------

    // 모든 Analytic Function을 수행하여 Temp Table에 저장
    static IDE_RC performAnalyticFunctions( qcTemplate     * aTemplate,
                                            const qmncWNST * aCodePlan,
                                            qmndWNST       * aDataPlan,
                                            UInt             aFlag );

    // Child를 반복 수행하여 Temp Table을 구축
    static IDE_RC insertRowsFromChild( qcTemplate     * aTemplate,
                                       const qmncWNST * aCodePlan,
                                       qmndWNST       * aDataPlan );

    /* BUG-40354 pushed rank */
    /* Child를 반복 수행하여 상위 n개의 rocord만 갖는 memory temp를 구축 */
    static IDE_RC insertLimitedRowsFromChild( qcTemplate     * aTemplate,
                                              const qmncWNST * aCodePlan,
                                              qmndWNST       * aDataPlan,
                                              SLong            aLimitNum );

    // Reporting Aggregation을 수행하고 결과를 Temp Table에 Update
    static IDE_RC aggregateAndUpdate( qcTemplate       * aTemplate,
                                      qmndWNST         * aDataPlan,
                                      const qmdWndNode * aWndNode );
    
    //------------------------
    // Reporting Aggregation 관련 함수
    //------------------------

    // 파티션 조건이 지정되지 않은 경우, 전체에 대해 aggregation 수행
    static IDE_RC aggregationOnly( qcTemplate        * aTemplate,
                                   qmndWNST          * aDataPlan,
                                   const qmdAggrNode * aAggrNode,
                                   const qmdMtrNode  * aAggrResultNode );
    
    // 파티션 별로 aggregation 수행
    static IDE_RC partitionAggregation( qcTemplate        * aTemplate,
                                        qmndWNST          * aDataPlan,
                                        const qmdMtrNode  * aOverColumnNode,
                                        const qmdAggrNode * aAggrNode,
                                        const qmdMtrNode  * aAggrResultNode );


    // 파티션 별로 order에 따라 aggregation 수행
    static IDE_RC partitionOrderByAggregation( qcTemplate  * aTemplate,
                                               qmndWNST    * aDataPlan,
                                               qmdMtrNode  * aOverColumnNode,
                                               qmdAggrNode * aAggrNode,
                                               qmdMtrNode  * aAggrResultNode );

    static IDE_RC orderByAggregation( qcTemplate  * aTemplate,
                                      qmndWNST    * aDataPlan,
                                      qmdMtrNode  * aOverColumnNode,
                                      qmdAggrNode * aAggrNode,
                                      qmdMtrNode  * aAggrResultNode );

    static IDE_RC updateAggrRows( qcTemplate * aTemplate,
                                  qmndWNST   * aDataPlan,
                                  qmdMtrNode * aAggrResultNode,
                                  qmcRowFlag * aFlag,
                                  SLong        aExecAggrCount );

    // Distinct Column을 위한 Temp Table을 Clear함
    static IDE_RC clearDistNode( qmdDistNode * aDistNode,
                                 const UInt    aDistNodeCnt );

    // Aggregation의 초기화와 Group 값을 설정
    static IDE_RC initAggregation( qcTemplate        * aTemplate,
                                   const qmdAggrNode * aAggrNode );
    
    // Aggregation을 수행
    static IDE_RC execAggregation( qcTemplate         * aTemplate,
                                   const qmdAggrNode  * aAggrNode,
                                   void               * aAggrInfo,
                                   qmdDistNode        * aDistNode,
                                   const UInt           aDistNodeCnt );

    // Distinct Column을 구성
    static IDE_RC setDistMtrColumns( qcTemplate        * aTemplate,
                                     qmdDistNode       * aDistNode,
                                     const UInt          aDistNodeCnt );
    
    // Aggregation을 마무리
    static IDE_RC finiAggregation( qcTemplate        * aTemplate,
                                   const qmdAggrNode * aAggrNode );

    // 저장한 두 Row간의 동일 여부를 판단
    static IDE_RC compareRows( const qmndWNST   * aDataPlan,
                               const qmdMtrNode * aMtrNode,
                               qmcRowFlag       * aFlag );

    // 비교를 위한 공간을 할당
    static IDE_RC allocMtrRow( qcTemplate     * aTemplate,
                               const qmncWNST * aCodePlan,
                               const qmndWNST * aDataPlan,                               
                               void           * aMtrRow[2] );

    // 첫 번째 레코드를 가져옴
    static IDE_RC getFirstRecord( qcTemplate  * aTemplate,
                                  qmndWNST    * aDataPlan,
                                  qmcRowFlag  * aFlag );
    
    // 다음 레코드를 가져옴
    static IDE_RC getNextRecord( qcTemplate  * aTemplate,
                                 qmndWNST    * aDataPlan,
                                 qmcRowFlag  * aFlag );


    //----------------------------------------------------
    // WNST의 doIt() 관련 함수
    //----------------------------------------------------

    // 저장 Row를 구성한다.
    static IDE_RC setMtrRow( qcTemplate     * aTemplate,
                             qmndWNST       * aDataPlan );
    
    // 검색된 저장 Row를 기준으로 Tuple Set을 복원한다.
    static IDE_RC setTupleSet( qcTemplate   * aTemplate,
                               qmdMtrNode   * aMtrNode,
                               void         * aRow );

    
    //----------------------------------------------------
    // 플랜 출력 printPlan() 관련 함수
    //----------------------------------------------------

    static IDE_RC printAnalyticFunctionInfo( qcTemplate     * aTemplate,
                                             const qmncWNST * aCodePlan,
                                                   qmndWNST * aDataPlan,
                                             ULong            aDepth,
                                             iduVarString   * aString,
                                             qmnDisplay       aMode );

    static IDE_RC printLinkedColumns( qcTemplate       * aTemplate,
                                      const qmcMtrNode * aNode,
                                      iduVarString     * aString );
    
    static IDE_RC printLinkedColumns( qcTemplate    * aTemplate,
                                      qmdMtrNode    * aNode,
                                      iduVarString  * aString );
    
    static IDE_RC printWindowNode( qcTemplate       * aTemplate,
                                   const qmcWndNode * aWndNode,
                                   ULong              aDepth,
                                   iduVarString     * aString );

    /* PROJ-1805 Window Clause */
    static IDE_RC windowAggregation( qcTemplate   * aTemplate,
                                     qmndWNST     * aDataPlan,
                                     qmcWndWindow * aWindow,
                                     qmdMtrNode   * aOverColumnNode,
                                     qmdMtrNode   * aOrderByColumn,
                                     qmdAggrNode  * aAggrNode,
                                     qmdMtrNode   * aAggrResultNode,
                                     idBool         aIsPartition );
    /* PROJ-1805 Window Clause */
    static IDE_RC partitionUnPrecedUnFollowRows( qcTemplate  * aTemplate,
                                                 qmndWNST    * aDataPlan,
                                                 qmdMtrNode  * aOverColumnNode,
                                                 qmdAggrNode * aAggrNode,
                                                 qmdMtrNode  * aAggrResultNode );
    /* PROJ-1805 Window Clause */
    static IDE_RC orderUnPrecedUnFollowRows( qcTemplate  * aTemplate,
                                             qmndWNST    * aDataPlan,
                                             qmdAggrNode * aAggrNode,
                                             qmdMtrNode  * aAggrResultNode );
    /* PROJ-1805 Window Clause */
    static IDE_RC partitionUnPrecedCurrentRows( qcTemplate  * aTemplate,
                                                qmndWNST    * aDataPlan,
                                                qmdMtrNode  * aOverColumnNode,
                                                qmdAggrNode * aAggrNode,
                                                qmdMtrNode  * aAggrResultNode );
    /* PROJ-1805 Window Clause */
    static IDE_RC orderUnPrecedCurrentRows( qcTemplate  * aTemplate,
                                            qmndWNST    * aDataPlan,
                                            qmdAggrNode * aAggrNode,
                                            qmdMtrNode  * aAggrResultNode );
    /* PROJ-1805 Window Clause */
    static IDE_RC partitionUnPrecedPrecedRows( qcTemplate  * aTemplate,
                                               qmndWNST    * aDataPlan,
                                               qmdMtrNode  * aOverColumnNode,
                                               SLong         aEndValue,
                                               qmdAggrNode * aAggrNode,
                                               qmdMtrNode  * aAggrResultNode );
    /* PROJ-1805 Window Clause */
    static IDE_RC orderUnPrecedPrecedRows( qcTemplate  * aTemplate,
                                           qmndWNST    * aDataPlan,
                                           SLong         aEndValue,
                                           qmdAggrNode * aAggrNode,
                                           qmdMtrNode  * aAggrResultNode );

    /* PROJ-1805 Window Clause */
    static IDE_RC partitionUnPrecedFollowRows( qcTemplate  * aTemplate,
                                               qmndWNST    * aDataPlan,
                                               qmdMtrNode  * aOverColumnNode,
                                               SLong         aEndValue,
                                               qmdAggrNode * aAggrNode,
                                               qmdMtrNode  * aAggrResultNode );
    /* PROJ-1805 Window Clause */
    static IDE_RC orderUnPrecedFollowRows( qcTemplate  * aTemplate,
                                           qmndWNST    * aDataPlan,
                                           SLong         aEndValue,
                                           qmdAggrNode * aAggrNode,
                                           qmdMtrNode  * aAggrResultNode );
    /* PROJ-1805 Window Clause */
    static IDE_RC partitionCurrentUnFollowRows( qcTemplate  * aTemplate,
                                                qmndWNST    * aDataPlan,
                                                qmdMtrNode  * aOverColumnNode,
                                                qmdAggrNode * aAggrNode,
                                                qmdMtrNode  * aAggrResultNode );
    /* PROJ-1805 Window Clause */
    static IDE_RC orderCurrentUnFollowRows( qcTemplate  * aTemplate,
                                            qmndWNST    * aDataPlan,
                                            qmdAggrNode * aAggrNode,
                                            qmdMtrNode  * aAggrResultNode );

    /* PROJ-1805 Window Clause */
    static IDE_RC currentCurrentRows( qcTemplate  * aTemplate,
                                      qmndWNST    * aDataPlan,
                                      qmdAggrNode * aAggrNode,
                                      qmdMtrNode  * aAggrResultNode );

    /* PROJ-1805 Window Clause */
    static IDE_RC partitionCurrentFollowRows( qcTemplate  * aTemplate,
                                              qmndWNST    * aDataPlan,
                                              qmdMtrNode  * aOverColumnNode,
                                              SLong         aEndValue,
                                              qmdAggrNode * aAggrNode,
                                              qmdMtrNode  * aAggrResultNode );
    /* PROJ-1805 Window Clause */
    static IDE_RC orderCurrentFollowRows( qcTemplate  * aTemplate,
                                          qmndWNST    * aDataPlan,
                                          SLong         aEndValue,
                                          qmdAggrNode * aAggrNode,
                                          qmdMtrNode  * aAggrResultNode );

    /* PROJ-1805 Window Clause */
    static IDE_RC partitionPrecedUnFollowRows( qcTemplate  * aTemplate,
                                               qmndWNST    * aDataPlan,
                                               qmdMtrNode  * aOverColumnNode,
                                               SLong         aStartValue,
                                               qmdAggrNode * aAggrNode,
                                               qmdMtrNode  * aAggrResultNode );
    /* PROJ-1805 Window Clause */
    static IDE_RC orderPrecedUnFollowRows( qcTemplate  * aTemplate,
                                           qmndWNST    * aDataPlan,
                                           SLong         aStartValue,
                                           qmdAggrNode * aAggrNode,
                                           qmdMtrNode  * aAggrResultNode );

    /* PROJ-1805 Window Clause */
    static IDE_RC partitionPrecedCurrentRows( qcTemplate  * aTemplate,
                                              qmndWNST    * aDataPlan,
                                              qmdMtrNode  * aOverColumnNode,
                                              SLong         aStartValue,
                                              qmdAggrNode * aAggrNode,
                                              qmdMtrNode  * aAggrResultNode );
    /* PROJ-1805 Window Clause */
    static IDE_RC orderPrecedCurrentRows( qcTemplate  * aTemplate,
                                          qmndWNST    * aDataPlan,
                                          SLong         aStartValue,
                                          qmdAggrNode * aAggrNode,
                                          qmdMtrNode  * aAggrResultNode );

    /* PROJ-1805 Window Clause */
    static IDE_RC partitionPrecedPrecedRows( qcTemplate  * aTemplate,
                                             qmndWNST    * aDataPlan,
                                             qmdMtrNode  * aOverColumnNode,
                                             SLong         aStartValue,
                                             SLong         aEndValue,
                                             qmdAggrNode * aAggrNode,
                                             qmdMtrNode  * aAggrResultNode );
    /* PROJ-1805 Window Clause */
    static IDE_RC orderPrecedPrecedRows( qcTemplate  * aTemplate,
                                         qmndWNST    * aDataPlan,
                                         SLong         aStartValue,
                                         SLong         aEndValue,
                                         qmdAggrNode * aAggrNode,
                                         qmdMtrNode  * aAggrResultNode );

    /* PROJ-1805 Window Clause */
    static IDE_RC partitionPrecedFollowRows( qcTemplate  * aTemplate,
                                             qmndWNST    * aDataPlan,
                                             qmdMtrNode  * aOverColumnNode,
                                             SLong         aStartValue,
                                             SLong         aEndValue,
                                             qmdAggrNode * aAggrNode,
                                             qmdMtrNode  * aAggrResultNode );
    /* PROJ-1805 Window Clause */
    static IDE_RC orderPrecedFollowRows( qcTemplate  * aTemplate,
                                         qmndWNST    * aDataPlan,
                                         SLong         aStartValue,
                                         SLong         aEndValue,
                                         qmdAggrNode * aAggrNode,
                                         qmdMtrNode  * aAggrResultNode );

    /* PROJ-1805 Window Clause */
    static IDE_RC partitionFollowUnFollowRows( qcTemplate  * aTemplate,
                                               qmndWNST    * aDataPlan,
                                               qmdMtrNode  * aOverColumnNode,
                                               SLong         aStartValue,
                                               qmdAggrNode * aAggrNode,
                                               qmdMtrNode  * aAggrResultNode );
    /* PROJ-1805 Window Clause */
    static IDE_RC orderFollowUnFollowRows( qcTemplate  * aTemplate,
                                           qmndWNST    * aDataPlan,
                                           SLong         aStartValue,
                                           qmdAggrNode * aAggrNode,
                                           qmdMtrNode  * aAggrResultNode );

    /* PROJ-1805 Window Clause */
    static IDE_RC partitionFollowFollowRows( qcTemplate  * aTemplate,
                                             qmndWNST    * aDataPlan,
                                             qmdMtrNode  * aOverColumnNode,
                                             SLong         aStartValue,
                                             SLong         aEndValue,
                                             qmdAggrNode * aAggrNode,
                                             qmdMtrNode  * aAggrResultNode );
    /* PROJ-1805 Window Clause */
    static IDE_RC orderFollowFollowRows( qcTemplate  * aTemplate,
                                         qmndWNST    * aDataPlan,
                                         SLong         aStartValue,
                                         SLong         aEndValue,
                                         qmdAggrNode * aAggrNode,
                                         qmdMtrNode  * aAggrResultNode );

    /* PROJ-1805 Window Clause */
    static IDE_RC partitionCurrentUnFollowRange( qcTemplate  * aTemplate,
                                                 qmndWNST    * aDataPlan,
                                                 qmdMtrNode  * aOverColumnNode,
                                                 qmdAggrNode * aAggrNode,
                                                 qmdMtrNode  * aAggrResultNode );
    /* PROJ-1805 Window Clause */
    static IDE_RC orderCurrentUnFollowRange( qcTemplate  * aTemplate,
                                             qmndWNST    * aDataPlan,
                                             qmdMtrNode  * aOverColumnNode,
                                             qmdAggrNode * aAggrNode,
                                             qmdMtrNode  * aAggrResultNode );

    /* PROJ-1805 Window Clause */
    static IDE_RC currentCurrentRange( qcTemplate  * aTemplate,
                                       qmndWNST    * aDataPlan,
                                       qmdMtrNode  * aOverColumnNode,
                                       qmdAggrNode * aAggrNode,
                                       qmdMtrNode  * aAggrResultNode );
    /* PROJ-1805 Window Clause */
    static IDE_RC calculateInterval( qcTemplate        * aTemplate,
                                     qmcdSortTemp      * aTempTable,
                                     qmdMtrNode        * aNode,
                                     void              * aRow,
                                     SLong               aInterval,
                                     UInt                aIntervalType,
                                     qmcWndWindowValue * aValue,
                                     idBool              aIsPreceding );
    /* PROJ-1805 Window Clause */
    static IDE_RC compareRangeValue( qcTemplate        * aTemplate,
                                     qmcdSortTemp      * aTempTable,
                                     qmdMtrNode        * aNode,
                                     void              * aRow,
                                     qmcWndWindowValue * aValue1,
                                     idBool              aIsSkipMode,
                                     idBool            * aResult );
    /* PROJ-1805 Window Clause */
    static IDE_RC updateOneRowNextRecord( qcTemplate * aTemplate,
                                          qmndWNST   * aDataPlan,
                                          qmdMtrNode * aAggrResultNode,
                                          qmcRowFlag * aFlag );

    /* PROJ-1805 Window Clause */
    static IDE_RC partitionUnPrecedPrecedFollowRange( qcTemplate  * aTemplate,
                                                      qmndWNST    * aDataPlan,
                                                      qmdMtrNode  * aOverColumnNode,
                                                      qmdMtrNode  * aOrderByColumn,
                                                      SLong         aEndValue,
                                                      SInt          aEndlType,
                                                      qmdAggrNode * aAggrNode,
                                                      qmdMtrNode  * aAggrResultNode,
                                                      idBool        aIsPreceding );
    /* PROJ-1805 Window Clause */
    static IDE_RC orderUnPrecedPrecedFollowRange( qcTemplate  * aTemplate,
                                                  qmndWNST    * aDataPlan,
                                                  qmdMtrNode  * aOrderByColumn,
                                                  SLong         aEndValue,
                                                  SInt          aEndType,
                                                  qmdAggrNode * aAggrNode,
                                                  qmdMtrNode  * aAggrResultNode,
                                                  idBool        aIsPreceding );
    /* PROJ-1805 Window Clause */
    static IDE_RC partitionCurrentFollowRange( qcTemplate  * aTemplate,
                                               qmndWNST    * aDataPlan,
                                               qmdMtrNode  * aOverColumnNode,
                                               qmdMtrNode  * aOrderByColumn,
                                               SLong         aEndValue,
                                               SInt          aEndType,
                                               qmdAggrNode * aAggrNode,
                                               qmdMtrNode  * aAggrResultNode );
    /* PROJ-1805 Window Clause */
    static IDE_RC orderCurrentFollowRange( qcTemplate  * aTemplate,
                                           qmndWNST    * aDataPlan,
                                           qmdMtrNode  * aOrderByColumn,
                                           SLong         aEndValue,
                                           SInt          aEndType,
                                           qmdAggrNode * aAggrNode,
                                           qmdMtrNode  * aAggrResultNode );
    /* PROJ-1805 Window Clause */
    static IDE_RC partitionPrecedFollowUnFollowRange( qcTemplate  * aTemplate,
                                                      qmndWNST    * aDataPlan,
                                                      qmdMtrNode  * aOverColumnNode,
                                                      qmdMtrNode  * aOrderByColumn,
                                                      SLong         aStartValue,
                                                      SInt          aStartType,
                                                      qmdAggrNode * aAggrNode,
                                                      qmdMtrNode  * aAggrResultNode,
                                                      idBool        aIsPreceding );
    /* PROJ-1805 Window Clause */
    static IDE_RC orderPrecedFollowUnFollowRange( qcTemplate  * aTemplate,
                                                  qmndWNST    * aDataPlan,
                                                  qmdMtrNode  * aOrderByColumn,
                                                  SLong         aStartValue,
                                                  SInt          aStartType,
                                                  qmdAggrNode * aAggrNode,
                                                  qmdMtrNode  * aAggrResultNode,
                                                  idBool        aIsPreceding );

    /* PROJ-1805 Window Clause */
    static IDE_RC partitionPrecedCurrentRange( qcTemplate  * aTemplate,
                                               qmndWNST    * aDataPlan,
                                               qmdMtrNode  * aOverColumnNode,
                                               qmdMtrNode  * aOrderByColumn,
                                               SLong         aStartValue,
                                               SInt          aStartType,
                                               qmdAggrNode * aAggrNode,
                                               qmdMtrNode  * aAggrResultNode );
    /* PROJ-1805 Window Clause */
    static IDE_RC orderPrecedCurrentRange( qcTemplate  * aTemplate,
                                           qmndWNST    * aDataPlan,
                                           qmdMtrNode  * aOrderByColumn,
                                           SLong         aStartValue,
                                           SInt          aStartType,
                                           qmdAggrNode * aAggrNode,
                                           qmdMtrNode  * aAggrResultNode );

    /* PROJ-1805 Window Clause */
    static IDE_RC partitionPrecedPrecedRange( qcTemplate  * aTemplate,
                                              qmndWNST    * aDataPlan,
                                              qmdMtrNode  * aOverColumnNode,
                                              qmdMtrNode  * aOrderByColumn,
                                              SLong         aStartValue,
                                              SInt          aStartType,
                                              SLong         aEndValue,
                                              SInt          aEndType,
                                              qmdAggrNode * aAggrNode,
                                              qmdMtrNode  * aAggrResultNode );
    /* PROJ-1805 Window Clause */
    static IDE_RC orderPrecedPrecedRange( qcTemplate  * aTemplate,
                                          qmndWNST    * aDataPlan,
                                          qmdMtrNode  * aOrderByColumn,
                                          SLong         aStartValue,
                                          SInt          aStartType,
                                          SLong         aEndValue,
                                          SInt          aEndType,
                                          qmdAggrNode * aAggrNode,
                                          qmdMtrNode  * aAggrResultNode );
    /* PROJ-1805 Window Clause */
    static IDE_RC partitionPrecedFollowRange( qcTemplate  * aTemplate,
                                              qmndWNST    * aDataPlan,
                                              qmdMtrNode  * aOverColumnNode,
                                              qmdMtrNode  * aOrderByColumn,
                                              SLong         aStartValue,
                                              SInt          aStartType,
                                              SLong         aEndValue,
                                              SInt          aEndType,
                                              qmdAggrNode * aAggrNode,
                                              qmdMtrNode  * aAggrResultNode );
    /* PROJ-1805 Window Clause */
    static IDE_RC orderPrecedFollowRange( qcTemplate  * aTemplate,
                                          qmndWNST    * aDataPlan,
                                          qmdMtrNode  * aOrderByColumn,
                                          SLong         aStartValue,
                                          SInt          aStartType,
                                          SLong         aEndValue,
                                          SInt          aEndType,
                                          qmdAggrNode * aAggrNode,
                                          qmdMtrNode  * aAggrResultNode );
    /* PROJ-1805 Window Clause */
    static IDE_RC partitionFollowFollowRange( qcTemplate  * aTemplate,
                                              qmndWNST    * aDataPlan,
                                              qmdMtrNode  * aOverColumnNode,
                                              qmdMtrNode  * aOrderByColumn,
                                              SLong         aStartValue,
                                              SInt          aStartType,
                                              SLong         aEndValue,
                                              SInt          aEndType,
                                              qmdAggrNode * aAggrNode,
                                              qmdMtrNode  * aAggrResultNode );
    /* PROJ-1805 Window Clause */
    static IDE_RC orderFollowFollowRange( qcTemplate  * aTemplate,
                                          qmndWNST    * aDataPlan,
                                          qmdMtrNode  * aOrderByColumn,
                                          SLong         aStartValue,
                                          SInt          aStartType,
                                          SLong         aEndValue,
                                          SInt          aEndType,
                                          qmdAggrNode * aAggrNode,
                                          qmdMtrNode  * aAggrResultNode );

    /* PROJ-1804 Ranking Function */
    static IDE_RC partitionOrderByLagAggr( qcTemplate  * aTemplate,
                                           qmndWNST    * aDataPlan,
                                           qmdMtrNode  * aOverColumnNode,
                                           qmdAggrNode * aAggrNode,
                                           qmdMtrNode  * aAggrResultNode );

    /* PROJ-1804 Ranking Function */
    static IDE_RC orderByLagAggr( qcTemplate  * aTemplate,
                                  qmndWNST    * aDataPlan,
                                  qmdAggrNode * aAggrNode,
                                  qmdMtrNode  * aAggrResultNode );

    /* PROJ-1804 Ranking Function */
    static IDE_RC partitionOrderByLeadAggr( qcTemplate  * aTemplate,
                                            qmndWNST    * aDataPlan,
                                            qmdMtrNode  * aOverColumnNode,
                                            qmdAggrNode * aAggrNode,
                                            qmdMtrNode  * aAggrResultNode );

    /* PROJ-1804 Ranking Function */
    static IDE_RC orderByLeadAggr( qcTemplate  * aTemplate,
                                   qmndWNST    * aDataPlan,
                                   qmdAggrNode * aAggrNode,
                                   qmdMtrNode  * aAggrResultNode );

    /* PROJ-1804 Ranking Function */
    static IDE_RC getPositionValue( qcTemplate  * aTemplate,
                                    qmdAggrNode * aAggrNode,
                                    SLong       * aNumber );
    
    /* BUG-40279 lead, lag with ignore nulls */
    static IDE_RC checkNullAggregation( qcTemplate   * aTemplate,
                                        qmdAggrNode  * aAggrNode,
                                        idBool       * aIsNull );

    /* BUG-40354 pushed rank */
    static IDE_RC getMinLimitValue( qcTemplate * aTemplate,
                                    qmdWndNode * aWndNode,
                                    SLong      * aNumber );

    /* BUG-43086 support Ntile analytic function */
    static IDE_RC getNtileValue( qcTemplate  * aTemplate,
                                 qmdAggrNode * aAggrNode,
                                 SLong       * aNumber );

    /* BUG-43086 support Ntile analytic function */
    static IDE_RC partitionOrderByNtileAggr( qcTemplate  * aTemplate,
                                             qmndWNST    * aDataPlan,
                                             qmdMtrNode  * aOverColumnNode,
                                             qmdAggrNode * aAggrNode,
                                             qmdMtrNode  * aAggrResultNode );

    /* BUG-43086 support Ntile analytic function */
    static IDE_RC orderByNtileAggr( qcTemplate  * aTemplate,
                                    qmndWNST    * aDataPlan,
                                    qmdAggrNode * aAggrNode,
                                    qmdMtrNode  * aAggrResultNode );
};

#endif /* _O_WMN_WNST_H_ */
