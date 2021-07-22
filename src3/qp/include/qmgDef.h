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
 * $Id: qmgDef.h 90785 2021-05-06 07:26:22Z hykim $
 *
 * Description :
 *     Graph 공통 정보에 대한 자료 구조 정의
 *
 * 용어 설명 :
 *
 * 약어 :
 *
 **********************************************************************/

#ifndef _O_QMG_DEF_H_
#define _O_QMG_DEF_H_ 1

#include <qc.h>
#include <qmn.h>
#include <qmoStatMgr.h>
#include <smiDef.h>

/***********************************************************************
 * [Graph의 flag을 위한 상수]
 ***********************************************************************/

// qmgGraph.flag
#define QMG_FLAG_CLEAR                     (0x00000000)

// Graph Type이 DISK인지 MEMORY 인지에 대한 정보
// qmgGraph.flag
#define QMG_GRAPH_TYPE_MASK                (0x00000001)
#define QMG_GRAPH_TYPE_MEMORY              (0x00000000)
#define QMG_GRAPH_TYPE_DISK                (0x00000001)

// Grouping, Distinction 수행 Method 방법 :  Sort Based or Hash Based
// qmgGraph.flag
#define QMG_SORT_HASH_METHOD_MASK          (0x00000002)
#define QMG_SORT_HASH_METHOD_SORT          (0x00000000)
#define QMG_SORT_HASH_METHOD_HASH          (0x00000002)

// Indexable Min Max 적용 여부
// qmgGraph.flag
#define QMG_INDEXABLE_MIN_MAX_MASK         (0x00000004)
#define QMG_INDEXABLE_MIN_MAX_FALSE        (0x00000000)
#define QMG_INDEXABLE_MIN_MAX_TRUE         (0x00000004)

// Join Group 포함 여부
// qmgGraph.flag
#define QMG_INCLUDE_JOIN_GROUP_MASK        (0x00000008)
#define QMG_INCLUDE_JOIN_GROUP_FALSE       (0x00000000)
#define QMG_INCLUDE_JOIN_GROUP_TRUE        (0x00000008)

// Preserved Order 정보
// qmgGraph.flag
#define QMG_PRESERVED_ORDER_MASK               (0x00000030)
#define QMG_PRESERVED_ORDER_NOT_DEFINED        (0x00000000)
#define QMG_PRESERVED_ORDER_DEFINED_FIXED      (0x00000010)
#define QMG_PRESERVED_ORDER_DEFINED_NOT_FIXED  (0x00000020)
#define QMG_PRESERVED_ORDER_NEVER              (0x00000030)

// Join Order 결정된 Graph인지 아닌지에 대한 정보
// qmgGraph.flag
#define QMG_JOIN_ORDER_DECIDED_MASK        (0x00000100)
#define QMG_JOIN_ORDER_DECIDED_FALSE       (0x00000000)
#define QMG_JOIN_ORDER_DECIDED_TRUE        (0x00000100)

// qmgGraph.flag
// 하위 preserved order 사용 가능 여부 
#define QMG_CHILD_PRESERVED_ORDER_USE_MASK   (0x00000200)
#define QMG_CHILD_PRESERVED_ORDER_CAN_USE    (0x00000000)
#define QMG_CHILD_PRESERVED_ORDER_CANNOT_USE (0x00000200)

// PROJ-1353 GROUPBY_EXTENSION사용여
#define QMG_GROUPBY_EXTENSION_MASK           (0x00000400)
#define QMG_GROUPBY_EXTENSION_FALSE          (0x00000000)
#define QMG_GROUPBY_EXTENSION_TRUE           (0x00000400)

// PROJ-1353 VALUE용  TEMP를 사용해야하는 지 여부
#define QMG_VALUE_TEMP_MASK                  (0x00000800)
#define QMG_VALUE_TEMP_FALSE                 (0x00000000)
#define QMG_VALUE_TEMP_TRUE                  (0x00000800)

/* PROJ-1805 Window Clause */
#define QMG_GROUPBY_NONE_MASK                (0x00001000)
#define QMG_GROUPBY_NONE_TRUE                (0x00001000)
#define QMG_GROUPBY_NONE_FALSE               (0x00000000)

/* BUG-40354 pushed rank */
/* pushed rank함수가 가능함을 표시 */
#define QMG_WINDOWING_PUSHED_RANK_MASK       (0x00002000)
#define QMG_WINDOWING_PUSHED_RANK_TRUE       (0x00002000)
#define QMG_WINDOWING_PUSHED_RANK_FALSE      (0x00000000)

// BUG-45296 rownum Pred 을 left outer 의 오른쪽으로 내리면 안됩니다.
#define QMG_ROWNUM_PUSHED_MASK               (0x00004000)
#define QMG_ROWNUM_PUSHED_TRUE               (0x00004000)
#define QMG_ROWNUM_PUSHED_FALSE              (0x00000000)

/*
 * qmgGraph.flag
 * PROJ-1071 Parallel Query
 */

/* bottom-up setting */
#define QMG_PARALLEL_EXEC_MASK               (0x00002000)
#define QMG_PARALLEL_EXEC_TRUE               (0x00002000)
#define QMG_PARALLEL_EXEC_FALSE              (0x00000000)

/*
 * top-down setting
 * PARALLEL_EXEC_MASK 보다 우선함
 */
#define QMG_PARALLEL_IMPOSSIBLE_MASK         (0x00004000)
#define QMG_PARALLEL_IMPOSSIBLE_TRUE         (0x00004000)
#define QMG_PARALLEL_IMPOSSIBLE_FALSE        (0x00000000)

/*
 * BUG-38410
 * top-down setting
 * make plan 과정에서 하위 graph 에 대한 parallel 허용 여부
 * qmgPartition::makePlan, qmgSelection::makePlan 에서
 * flag 를 보고 parallel plan / non parallel plan 을 만든다.
 * NL join 의 right 와 같이 반복수행되는 plan 에 대해
 * parallel 수행을 금지하기위해 만들어짐
 */
#define QMG_PLAN_EXEC_REPEATED_MASK          (0x00008000)
#define QMG_PLAN_EXEC_REPEATED_TRUE          (0x00008000)
#define QMG_PLAN_EXEC_REPEATED_FALSE         (0x00000000)

/* PROJ-2509 Hierarchy Query Join */
#define QMG_HIERARCHY_JOIN_MASK              (0x00010000)
#define QMG_HIERARCHY_JOIN_TRUE              (0x00010000)
#define QMG_HIERARCHY_JOIN_FALSE             (0x00000000)

/* PROJ-2714 Multiple table Update,Delete support */
#define QMG_JOIN_ONLY_NL_MASK                (0x00020000)
#define QMG_JOIN_ONLY_NL_TRUE                (0x00020000)
#define QMG_JOIN_ONLY_NL_FALSE               (0x00000000)

/* BUG-47752 left outer join의 on절이 OR 조건으로 구성되고있고
 * 오른쪽 view에 aggregaion이 존재할경우 fatal
 */
#define QMG_JOIN_RIGHT_MASK                  (0x00040000)
#define QMG_JOIN_RIGHT_TRUE                  (0x00040000)
#define QMG_JOIN_RIGHT_FALSE                 (0x00000000)

#define QMG_VIEW_PUSH_MASK                   (0x00080000)
#define QMG_VIEW_PUSH_TRUE                   (0x00080000)
#define QMG_VIEW_PUSH_FALSE                  (0x00000000)

// PROJ-2750
#define QMG_LOJN_SKIP_RIGHT_COND_MASK        (0x00100000)
#define QMG_LOJN_SKIP_RIGHT_COND_TRUE        (0x00100000)
#define QMG_LOJN_SKIP_RIGHT_COND_FALSE       (0x00000000)

// Reserved Area
// qmgGraph.flag
#define QMG_RESERVED_AREA_MASK               (0xFF000000)
#define QMG_RESERVED_AREA_CLEAR              (0x00000000)


//------------------------------
//makeColumnMtrNode()함수에 필요한 flag
//------------------------------
#define QMG_MAKECOLUMN_MTR_NODE_NOT_CHANGE_COLUMN_MASK  (0x00000001)
#define QMG_MAKECOLUMN_MTR_NODE_NOT_CHANGE_COLUMN_FALSE (0x00000000)
#define QMG_MAKECOLUMN_MTR_NODE_NOT_CHANGE_COLUMN_TRUE  (0x00000001)

/* BUG-48132 */
#define QMG_HASH_OR_SORT_METHOD_MASK          (0x0000000F)
#define QMG_HASH_OR_SORT_METHOD_NOT_DEFINED   (0x00000000)
#define QMG_HASH_OR_SORT_METHOD_GROUPING_MASK (0x00000003)
#define QMG_HASH_OR_SORT_METHOD_GROUPING_HASH (0x00000001)
#define QMG_HASH_OR_SORT_METHOD_GROUPING_SORT (0x00000002)

/* BUG-48120 __OPTIMIER_INDEX_COST_MODE property */
#define QMG_OPTI_INDEX_COST_MODE_1_MASK    (0x00000001)

/***********************************************************************
 * [Graph의 종류]
 **********************************************************************/

typedef enum
{
    QMG_NONE = 0,

    // One Tuple 관련 Graph
    QMG_SELECTION,       // Selection Graph
    QMG_PROJECTION,      // Projection Graph

    // Multi Tuple 관련 Graph
    QMG_DISTINCTION,     // Duplicate Elimination Graph
    QMG_GROUPING,        // Grouping Graph
    QMG_SORTING,         // Sorting Graph
    QMG_WINDOWING,       // Windowing Graph

    // Product 관련 Graph
    QMG_INNER_JOIN,      // Inner Join Graph
    QMG_SEMI_JOIN,       // Semi Join Graph
    QMG_ANTI_JOIN,       // Anti Join Graph
    QMG_LEFT_OUTER_JOIN, // Left Outer Join Graph
    QMG_FULL_OUTER_JOIN, // Full Outer Join Graph

    // SET 관련 Graph
    QMG_SET,             // SET Graph

    // 특수 연산 관련 Graph
    QMG_HIERARCHY,       // Hierarchy Graph
    QMG_DNF,             // DNF Graph
    QMG_PARTITION,       // PROJ-1502 Partition Graph
    QMG_COUNTING,        // PROJ-1405 Counting Graph

    // PROJ-2205 DML 관련 Graph
    QMG_INSERT,          // insert graph
    QMG_MULTI_INSERT,    // BUG-36596 multi-table insert
    QMG_UPDATE,          // update graph
    QMG_DELETE,          // delete graph
    QMG_MOVE,            // move graph
    QMG_MERGE,           // merge graph    
    QMG_RECURSIVE_UNION_ALL, // PROJ-2582 recursive with

    // PROJ-2638 Shard Graph
    QMG_SHARD_SELECT,    // Shard Select Graph
    QMG_SHARD_DML,       // Shard DML Graph
    QMG_SHARD_INSERT,    // Shard Insert Graph

    QMG_MAX_MAX
} qmgType;

/***********************************************************************
 * [공통 비용 정보 자료 구조]
 *
 **********************************************************************/
typedef struct qmgCostInfo
{
    SDouble             recordSize;
    SDouble             selectivity;
    SDouble             inputRecordCnt;
    SDouble             outputRecordCnt;
    SDouble             myAccessCost;
    SDouble             myDiskCost;
    SDouble             myAllCost;
    SDouble             totalAccessCost;
    SDouble             totalDiskCost;
    SDouble             totalAllCost;
}qmgCostInfo;

/***********************************************************************
 * [Preserved Order 자료 구조]
 *
 **********************************************************************/

//--------------------------------------------------------
// Preserved Order의 Direction  정보
//--------------------------------------------------------
typedef enum
{
    QMG_DIRECTION_NOT_DEFINED = 0, // direction 정해지지 않음
    QMG_DIRECTION_SAME_WITH_PREV,  // direction 정해지지 않았으나 이전 칼럼과
                                   // 동일한 방향을 갖게됨
    QMG_DIRECTION_DIFF_WITH_PREV,  // direction 정해지지 않았으나 이전 칼럼과
                                   // 다른 방향을 갖게됨
    QMG_DIRECTION_ASC,             // Ascending Order를 가짐
    QMG_DIRECTION_DESC,            // Descending Order를 가짐
    // BUG-40361 supporting to indexable analytic function
    QMG_DIRECTION_ASC_NULLS_FIRST, // Ascending Order를 Nulls First  가짐
    QMG_DIRECTION_ASC_NULLS_LAST,  // Ascending Order를 Nulls Last  가짐
    QMG_DIRECTION_DESC_NULLS_FIRST,// Descending Order를 Nulls First 가짐
    QMG_DIRECTION_DESC_NULLS_LAST  // Descending Order를 Nulls Last 가짐
} qmgDirectionType;

//--------------------------------------------------------
// Preserved Order의 자료 구조
//    - mtcColumn으로부터 획득
//      column = (mtcColumn.column.id % SMI_COLUMN_ID_MAXIMUM)
//    - qtcNode로부터 획득
//      column = qtcNode.node.column
//--------------------------------------------------------
typedef struct qmgPreservedOrder
{
    UShort              table;        // tuple set에서 해당 table의 position
    UShort              column;       // tuple set에서 해당 column의 position
    qmgDirectionType    direction;    // preserved order direction 정보
    qmgPreservedOrder * next;
} qmgPreservedOrder;

/***********************************************************************
 * [모든 Graph를 위한 함수 포인터]
 * 각 Graph의 최적화 또는 Plan Tree생성을 위한 함수 포인터
 * initialize 과정에서 Graph내의 함수로 Setting하게 된다.
 * 이로서, 상위 Graph에서는 하위 Graph의 종류에 관계 없이
 * 동일한 함수 포인터를 호출함으로서 처리할 수 있게 된다.
 ***********************************************************************/

struct qmgGraph;
struct qmgJOIN;

// 각 Graph의 최적화를 수행하는 함수 포인터
typedef IDE_RC (* optimizeFunc ) ( qcStatement * aStatement,
                                   qmgGraph    * aGraph );

// 각 Graph의 Execution Plan을 생성하는 함수 포인터
typedef IDE_RC (* makePlanFunc ) ( qcStatement    * aStatement,
                                   const qmgGraph * aParent,
                                   qmgGraph       * aGraph );

// 각 Graph의 Graph 출력 정보를 생성하는 함수 포인터
typedef IDE_RC (* printGraphFunc) ( qcStatement  * aTemplate,
                                    qmgGraph     * aGraph,
                                    ULong          aDepth,
                                    iduVarString * aString );

#define QMG_PRINT_LINE_FEED( i, aDepth, aString )       \
    {                                                   \
        iduVarStringAppend( aString, "\n" );            \
        iduVarStringAppend( aString, "|" );             \
        for ( i = 0; i < aDepth; i++ )                  \
        {                                               \
            iduVarStringAppend( aString, "  " );        \
        }                                               \
        iduVarStringAppend( aString, "|" );             \
    }

/***********************************************************************
 *  [ Graph 의 구분 ]
 *
 *  ---------------------------------------------
 *    Graph     |  left   |  right  | children
 *  ---------------------------------------------
 *  unary graph  |    O    |  null   |   null
 *  ---------------------------------------------
 *  binary graph |    O    |    O    |   null
 *  ---------------------------------------------
 *  multi  graph |   null  |  null   |    O
 *  ----------------------------------------------
 *
 *  Multi Graph : Parsing, Validation 단계에서는 생성되지 않으며,
 *                Optimization 단계에서 최적화에 의해 생성된다.
 *
 **********************************************************************/

/***********************************************************************
 * Multiple Children 을 구성하기 위한 자료 구조
 *      ( PROJ-1486 Multiple Bag Union )
 **********************************************************************/

typedef struct qmgChildren
{
    struct qmgGraph     * childGraph;     // Child Graph
    struct qmgChildren  * next;
} qmgChildren;

/***********************************************************************
 * [모든 Graph를 위한 공통 자료 구조]
 *
 **********************************************************************/
typedef struct qmgGraph
{
    qmgType           type;          // Graph 의 종류
    UInt              flag;          // Masking 정보, joinGroup 포함 여부

    // Graph의 dependencies
    qcDepInfo         depInfo;

    // 해당 Graph를 위한 함수 포인터
    optimizeFunc      optimize;      // 해당 Graph의 최적화를 수행하는 함수
    makePlanFunc      makePlan;      // 해당 Graph의 Plan을 생성하는 함수
    printGraphFunc    printGraph;    // 해당 Graph의 정보를 출력하는 함수

    //-------------------------------------------------------
    // [Child Graph]
    // Graph간의 연결 관계는 left와 right로 표현된다.
    //-------------------------------------------------------

    qmgGraph        * left;          // left child graph
    qmgGraph        * right;         // right child graph
    qmgChildren     * children;      // multiple child graph

    //-------------------------------------------------------
    // [myPlan]
    // 해당 Graph로부터 생성된 Plan
    // Graph 에 대한 최적화 수행까지는 NULL값을 가지며,
    // makePlan() 함수 포인터를 통하여 생성된다.
    //-------------------------------------------------------
    qmnPlan         * myPlan;        // Graph가 생성한 Plan Tree

    qmoPredicate    * myPredicate;   // 해당 graph에서 처리되어야 할 predicate
    qmoPredicate    * constantPredicate; // CNF로부터 추출된 Constant Predicate
    qmoPredicate    * ridPredicate;      // PROJ-1789 PROWID

    qtcNode         * nnfFilter;     // To Fix PR-12743, NNF filter 지원

    qmsFrom         * myFrom;        // 해당 base graph에 대응되는 qmsFrom
    qmsQuerySet     * myQuerySet;    // 해당 base graph가 속한 querySet

    qmgCostInfo       costInfo;      // 공통 비용 정보
    struct qmoCNF   * myCNF;         // top graph인 경우,
                                     // 해당 graph가 속한 CNF를 가리킴
    qmgPreservedOrder * preservedOrder; // 해당 graph의 preserved order 정보

} qmgGraph;

/* TASK-6744 */
typedef struct qmgRandomPlanInfo
{
    UInt mWeightedValue;      /* 가중치 */
    UInt mTotalNumOfCases;    /* 모든 경우의 수를 합친 수 */
} qmgRandomPlanInfo;

// 모든 Graph가 공통적으로 사용하는 기능

class qmg
{
public:
    //-----------------------------------------
    // Graph 생성시 공통적으로 필요한 함수를 추가
    //-----------------------------------------

    // Graph의 공통 정보를 초기화함.
    static IDE_RC initGraph( qmgGraph * aGraph );

    // Graph의 공통 정보를 출력함.
    static IDE_RC printGraph( qcStatement  * aStatement,
                              qmgGraph     * aGraph,
                              ULong          aDepth,
                              iduVarString * aString );


    // Subquery내의 Graph 정보를 출력함.
    static IDE_RC printSubquery( qcStatement  * aStatement,
                                 qtcNode      * aSubQuery,
                                 ULong          aDepth,
                                 iduVarString * aString );

    // 원하는 Preserved Order가 존재하는 지를 검사
    static IDE_RC checkUsableOrder( qcStatement       * aStatement,
                                    qmgPreservedOrder * aWantOrder,
                                    qmgGraph          * aLeftGraph,
                                    qmoAccessMethod  ** aOriginalMethod,
                                    qmoAccessMethod  ** aSelectMethod,
                                    idBool            * aUsable );

    // Child Graph에 대하여 Preserved Order를 설정함.
    static IDE_RC setOrder4Child( qcStatement       * aStatement,
                                  qmgPreservedOrder * aWantOrder,
                                  qmgGraph          * aLeftGraph );

    // Preserved order 설정 가능한 경우, Preserved Order 설정
    static IDE_RC tryPreservedOrder( qcStatement       * aStatement,
                                     qmgGraph          * aGraph,
                                     qmgPreservedOrder * aWantOrder,
                                     idBool            * aSuccess );

    /* BUG-39298 improve preserved order */
    static IDE_RC retryPreservedOrder(qcStatement       * aStatement,
                                      qmgGraph          * aGraph,
                                      qmgPreservedOrder * aWantOrder,
                                      idBool            * aUsable);

    // target의 칼럼들의 cardinality를 이용하여 bucket count 구하는 함수
    static IDE_RC getBucketCntWithTarget( qcStatement * aStatement,
                                          qmgGraph    * aGraph,
                                          qmsTarget   * aTarget,
                                          UInt          aHintBucketCnt,
                                          UInt        * aBucketCnt );

    // distinct aggregation column의 bucket count를 구함
    static IDE_RC getBucketCnt4DistAggr( qcStatement * aStatement,
                                         SDouble       aChildOutputRecordCnt,
                                         UInt          aGroupBucketCnt,
                                         qtcNode     * aNode,
                                         UInt          aHintBucketCnt,
                                         UInt        * aBucketCnt );

    // Graph가 사용할 저장 매체를 판단함
    static IDE_RC isDiskTempTable( qmgGraph    * aGraph,
                                   idBool      * aIsDisk );

    //-----------------------------------------
    // Plan Tree 생성시 공통적으로 필요한 함수를 추가
    //-----------------------------------------

    //-----------------------------------------
    // 저장 Column 관리를 위한 함수
    //-----------------------------------------

    // 저장 Column을 위한 qmcMtrNode를 생성
    static IDE_RC makeColumnMtrNode( qcStatement  * aStatement ,
                                     qmsQuerySet  * aQuerySet ,
                                     qtcNode      * aSrcNode,
                                     idBool         aConverted,
                                     UShort         aNewTupleID ,
                                     UInt           aFlag,
                                     UShort       * aColumnCount ,
                                     qmcMtrNode  ** aMtrNode);

    static IDE_RC makeBaseTableMtrNode( qcStatement  * aStatement ,
                                        qtcNode      * aSrcNode ,
                                        UShort         aDstTupleID ,
                                        UShort       * aColumnCount ,
                                        qmcMtrNode  ** aMtrNode );

    // To Fix PR-11562
    // Indexable MIN-MAX 최적화가 적용된 경우
    // Preserved Order는 방향성을 가짐, 따라서 해당 정보를
    // 설정해줄 필요가 없음.
    // indexable min-max에서 검색 방향을 지정
    // static IDE_RC    setDirection4IndexableMinMax( UInt   * aFlag ,
    //                                                UInt     aMinMaxFlag ,
    //                                                UInt     aOrderFlag );

    //display 정보를 세팅
    static IDE_RC setDisplayInfo( qmsFrom          * aFrom ,
                                  qmsNamePosition  * aTableOwnerName ,
                                  qmsNamePosition  * aTableName ,
                                  qmsNamePosition  * aAliasName );

    //remote object의 display 정보를 세팅
    static IDE_RC setDisplayRemoteInfo( qcStatement      * aStatement ,
                                        qmsFrom          * aFrom ,
                                        qmsNamePosition  * aRemoteUserName ,
                                        qmsNamePosition  * aRemoteObjectName ,
                                        qmsNamePosition  * aTableOwnerName ,
                                        qmsNamePosition  * aTableName ,
                                        qmsNamePosition  * aAliasName );

    //mtcColumn 정보와 mtcExecute정보를 복사한다.
    static IDE_RC copyMtcColumnExecute( qcStatement      * aStatement ,
                                        qmcMtrNode       * aMtrNode );

    // PROJ-2179
    static IDE_RC setCalcLocate( qcStatement * aStatement,
                                 qmcMtrNode  * aMtrNode );

    // PROJ-1762
    // Analytic Function 결과가 저장될 qmcMtrNode를 구성한다.
    static IDE_RC
    makeAnalFuncResultMtrNode( qcStatement      * aStatement,
                               qmsAnalyticFunc  * aAnalFunc,
                               UShort             aTupleID,
                               UShort           * aColumnCount,
                               qmcMtrNode      ** aMtrNode,
                               qmcMtrNode      ** aLastMtrNode );
    
    // PROJ-1762
    // sort mtr node 
    static IDE_RC makeSortMtrNode( qcStatement        * aStatement,
                                   UShort               aTupleID,
                                   qmgPreservedOrder  * aSortKey,
                                   qmsAnalyticFunc    * aAnalFuncList,
                                   qmcMtrNode         * aMtrNode,
                                   qmcMtrNode        ** aSortMtrNode,
                                   UInt               * aSortMtrNodeCount );
    
    // PROJ-1762
    // window mtr node & distinct mtr node 
    static IDE_RC
    makeWndNodeNDistNodeNAggrNode( qcStatement        * aStatement,
                                   qmsQuerySet        * aQuerySet,
                                   UInt                 aFlag,
                                   qmnPlan            * aChildPlan,
                                   qmsAnalyticFunc    * aAnalyticFunc,
                                   qmcMtrNode         * aMtrNode,
                                   qmoDistAggArg      * aDistAggArg,
                                   qmcMtrNode         * aAggrResultNode,
                                   UShort               aAggrTupleID,
                                   qmcWndNode        ** aWndNode,
                                   UInt               * aWndNodeCount,
                                   UInt               * aPtnNodeCount,
                                   qmcMtrNode        ** aDistNode,
                                   UInt               * aDistNodeCount,
                                   qmcMtrNode        ** aAggrNode,
                                   UInt               * aAggrNodeCount );
    
    // PROJ-1762
    static IDE_RC makeDistNode( qcStatement    * aStatement,
                                qmsQuerySet    * aQuerySet,
                                UInt             aFlag,
                                qmnPlan        * aChildPlan,
                                UShort           aTupleID,
                                qmoDistAggArg  * aDistAggArg,
                                qtcNode        * aAggrNode,
                                qmcMtrNode     * aNewMtrNode,
                                qmcMtrNode    ** aPlanDistNode,
                                UShort         * aDistNodeCount );

    // LOJN , FOJN의 Filter를 만들어준다.
    static IDE_RC makeOuterJoinFilter(qcStatement   * aStatement ,
                                      qmsQuerySet   * aQuerySet ,
                                      qmoPredicate ** aPredicate ,
                                      qtcNode       * aNnfFilter,
                                      qtcNode      ** aFilter);

    // CNF, DNF cost 판단시, 취소된 method에 대해서
    // optimize때 만들었던 sdf를 모두 제거한다.
    static IDE_RC removeSDF( qcStatement * aStatement, qmgGraph * aGraph );

    // Graph의 Plan Tree 생성시에 호출하는 내부 함수
    //   HASH 생성
    static IDE_RC makeLeftHASH4Join( qcStatement  * aStatement,
                                     qmgGraph     * aGraph,
                                     UInt           aMtrFlag,
                                     UInt           aJoinFlag,
                                     qtcNode      * aRange,
                                     qmnPlan      * aChild,
                                     qmnPlan      * aPlan );

    static IDE_RC makeRightHASH4Join( qcStatement  * aStatement,
                                      qmgGraph     * aGraph,
                                      UInt           aMtrFlag,
                                      UInt           aJoinFlag,
                                      qtcNode      * aRange,
                                      qmnPlan      * aChild,
                                      qmnPlan      * aPlan );

    // Graph의 Plan Tree 생성시에 호출하는 내부 함수
    //   SORT 생성

    static IDE_RC initLeftSORT4Join( qcStatement  * aStatement,
                                     qmgGraph     * aGraph,
                                     UInt           aJoinFlag,
                                     qtcNode      * aRange,
                                     qmnPlan      * aParent,
                                     qmnPlan     ** aPlan );

    static IDE_RC makeLeftSORT4Join( qcStatement  * aStatement,
                                     qmgGraph     * aGraph,
                                     UInt           aMtrFlag,
                                     UInt           aJoinFlag,
                                     qmnPlan      * aChild,
                                     qmnPlan      * aPlan );

    static IDE_RC initRightSORT4Join( qcStatement  * aStatement,
                                      qmgGraph     * aGraph,
                                      UInt           aJoinFlag,
                                      idBool         aOrderCheckNeed,
                                      qtcNode      * aRange,
                                      idBool         aIsRangeSearch,
                                      qmnPlan      * aParent,
                                      qmnPlan     ** aPlan );

    static IDE_RC makeRightSORT4Join( qcStatement  * aStatement,
                                      qmgGraph     * aGraph,
                                      UInt           aMtrFlag,
                                      UInt           aJoinFlag,
                                      idBool         aOrderCheckNeed,
                                      qmnPlan      * aChild,
                                      qmnPlan      * aPlan );

    // BUG-18525
    static IDE_RC isNeedMtrNode4LeftChild( qcStatement  * aStatement,
                                           qmnPlan      * aChild,
                                           idBool       * aIsNeed );

    static IDE_RC usableIndexScanHint( qcmIndex            * aHintIndex,
                                       qmoTableAccessType    aHintAccessType,
                                       qmoIdxCardInfo      * aIdxCardInfo,
                                       UInt                  aIdxCnt,
                                       qmoIdxCardInfo     ** aSelectedIdxInfo,
                                       idBool              * aUsableHint );

    static IDE_RC resetColumnLocate( qcStatement * aStatement, UShort aTupleID );

    static IDE_RC setColumnLocate( qcStatement * aStatement,
                                   qmcMtrNode  * aMtrNode );

    static IDE_RC changeTargetColumnLocate( qcStatement  * aStatement,
                                            qmsQuerySet  * aQuerySet,
                                            qmsTarget    * aTarget );

    // PROJ-1473
    // validation시 설정된 노드의 기본 위치정보를
    // 실제로 참조해야 할 변경된 위치정보로 변경한다.
    static IDE_RC changeColumnLocate( qcStatement  * aStatement,
                                      qmsQuerySet  * aQuerySet,
                                      qtcNode      * aNode,
                                      UShort         aTableID,
                                      idBool         aNext );

    static IDE_RC changeColumnLocateInternal( qcStatement * aStatement,
                                              qmsQuerySet * aQuerySet,
                                              qtcNode     * aNode,
                                              UShort        aTableID );

    static IDE_RC findColumnLocate( qcStatement  * aStatement,
                                    UInt           aParentTupleID,
                                    UShort         aTableID,
                                    UShort         aOrgTable,
                                    UShort         aOrgColumn,
                                    UShort       * aChangeTable,
                                    UShort       * aChangeColumn );

    static IDE_RC findColumnLocate( qcStatement  * aStatement,
                                    UShort         aTableID,
                                    UShort         aOrgTable,
                                    UShort         aOrgColumn,
                                    UShort       * aChangeTable,
                                    UShort       * aChangeColumn );
    
    // 동일한 방향성을 갖는 Column인지를 비교
    static IDE_RC checkSameDirection( qmgPreservedOrder * aWantOrder,
                                      qmgPreservedOrder * aPresOrder,
                                      qmgDirectionType    aPrevDirection,
                                      qmgDirectionType  * aNowDirection,
                                      idBool            * aUsable );

    // BUG-42145 Index에서의 Nulls Option 처리를 포함해서
    // 동일한 방향성을 갖는 Column인지를 비교
    static IDE_RC checkSameDirection4Index( qmgPreservedOrder * aWantOrder,
                                            qmgPreservedOrder * aPresOrder,
                                            qmgDirectionType    aPrevDirection,
                                            qmgDirectionType  * aNowDirection,
                                            idBool            * aUsable );
    //Sort 저장 컬럼의 정렬 방향을 결정한다.
    static IDE_RC setDirection4SortColumn(
        qmgPreservedOrder  * aPreservedOrder,
        UShort               aColumnID,
        UInt               * aFlag );

    // BUG-32303
    static IDE_RC finalizePreservedOrder( qmgGraph * aGraph );

    // Preserved order의 direction을 복사한다.
    static IDE_RC copyPreservedOrderDirection(
        qmgPreservedOrder * aDstOrder,
        qmgPreservedOrder * aSrcOrder );

    static idBool isSamePreservedOrder(
            qmgPreservedOrder * aDstOrder,
            qmgPreservedOrder * aSrcOrder );

    static IDE_RC makeDummyMtrNode( qcStatement  * aStatement ,
                                    qmsQuerySet  * aQuerySet,
                                    UShort         aTupleID,
                                    UShort       * aColumnCount,
                                    qmcMtrNode  ** aMtrNode );

    static idBool getMtrMethod( qcStatement * aStatement,
                                UShort        aSrcTupleID,
                                UShort        aDstTupleID );

    static idBool existBaseTable( qmcMtrNode * aMtrNode,
                                  UInt         aFlag,
                                  UShort       aTable );

    static UInt getBaseTableType( ULong aTupleFlag );

    static idBool isTempTable( ULong aTupleFlag );

    // PROJ-2242
    static void setPlanInfo( qcStatement  * aStatement,
                             qmnPlan      * aPlan,
                             qmgGraph     * aGraph );

    // PROJ-2418 
    static IDE_RC getGraphLateralDepInfo( qmgGraph  * aGraph,
                                          qcDepInfo * aGraphLateralDepInfo );

    static IDE_RC resetDupNodeToMtrNode( qcStatement * aStatement,
                                         UShort        aOrgTable,
                                         UShort        aOrgColumn,
                                         qtcNode     * aChangeNode,
                                         qtcNode     * aNode );

    // BUG-43493
    static IDE_RC lookUpMaterializeGraph( qmgGraph * aGraph,
                                          idBool   * aFound );
    
    /* TASK-6744 */
    static void initializeRandomPlanInfo( qmgRandomPlanInfo * aRandomPlanInfo );

    /* TASK-7219 */
    static IDE_RC getNodeOffset( qtcNode * aNode,
                                 idBool    aIsRecursive,
                                 SInt    * aStart,
                                 SInt    * aEnd );

    static IDE_RC getFromOffset( qmsFrom * aFrom,
                                 SInt    * aStart,
                                 SInt    * aEnd );

    static IDE_RC makeShardParamOffsetArrayForGraph( qcStatement       * aStatement,
                                                     qcParamOffsetInfo * aParamOffsetInfo,
                                                     UShort            * aOutParamCount,
                                                     qcShardParamInfo ** aOutParamInfo );

    static IDE_RC makeShardParamOffsetArrayWithInfo( qcStatement        * aStatement,
                                                     sdiAnalyzeInfo     * aAnalyzeInfo,
                                                     qcParamOffsetInfo  * aParamOffsetInfo,
                                                     UShort             * aOutParamCount,
                                                     UShort             * aOutParamOffset,
                                                     qcShardParamInfo  ** aOutShardParamInfo );

    static IDE_RC makeShardParamOffsetArray( qcStatement       * aStatement,
                                             qcNamePosition    * aParsePosition,
                                             UShort            * aOutParamCount,
                                             UShort            * aOutParamOffset,
                                             qcShardParamInfo ** aOutShardParamInfo );

    static IDE_RC findAndCollectParamOffset( qcStatement       * aStatement,
                                             qtcNode           * aNode,
                                             qcParamOffsetInfo * aParamOffsetInfo );

    static IDE_RC collectReaminParamOffset( qcStatement       * aStatement,
                                            SInt                aStartOffset,
                                            SInt                aEndOffset,
                                            qcParamOffsetInfo * aParamOffsetInfo );

    static IDE_RC copyAndCollectParamOffset( qcStatement       * aStatement,
                                             SInt                aStartOffset,
                                             SInt                aEndOffset,
                                             UShort              aParamOffset,
                                             UShort              aParamCount,
                                             qcShardParamInfo  * aShardParamInfo,
                                             qcParamOffsetInfo * aParamOffsetInfo );

    static IDE_RC setHostVarOffset( qcStatement * aStatement );

    static IDE_RC checkStackOverflow( mtcNode * aNode,
                                      SInt      aRemain,
                                      idBool  * aIsOverflow );

    static IDE_RC adjustParamOffsetForAnalyzeInfo( sdiAnalyzeInfo    * aAnalyzeInfo,
                                                   UShort              aParamCount,
                                                   qcShardParamInfo ** aOutShardParamInfo );
private:

    //-----------------------------------------
    // Preserved Order 관련 함수
    //-----------------------------------------

    // Graph내에서 해당 Order를 사용할 수 있는 지 검사.
    static IDE_RC checkOrderInGraph ( qcStatement       * aStatement,
                                      qmgGraph          * aGraph,
                                      qmgPreservedOrder * aWantOrder,
                                      qmoAccessMethod  ** aOriginalMethod,
                                      qmoAccessMethod  ** aSelectMethod,
                                      idBool            * aUsable );

    // 원하는 Order에 부합하는 Index가 존재하는 지 검사
    static IDE_RC    checkUsableIndex4Selection( qmgGraph          * aGraph,
                                                 qmgPreservedOrder * aWantOrder,
                                                 idBool              aOrderNeed,
                                                 qmoAccessMethod  ** aOriginalMethod,
                                                 qmoAccessMethod  ** aSelectMethod,
                                                 idBool            * aUsable );

    // PROJ-2242 qmoJoinMethodMgr 의 것을 옴겨옴
    static IDE_RC checkUsableIndex( qcStatement       * aStatement,
                                    qmgGraph          * aGraph,
                                    qmgPreservedOrder * aWantOrder,
                                    idBool              aOrderNeed,
                                    qmoAccessMethod  ** aOriginalMethod,
                                    qmoAccessMethod  ** aSelectMethod,
                                    idBool            * aUsable );

    // 원하는 Order에 부합하는 Index가 존재하는 지 검사
    static IDE_RC    checkUsableIndex4Partition( qmgGraph          * aGraph,
                                                 qmgPreservedOrder * aWantOrder,
                                                 idBool              aOrderNeed,
                                                 qmoAccessMethod  ** aOriginalMethod,
                                                 qmoAccessMethod  ** aSelectMethod,
                                                 idBool            * aUsable );

    // 원하는 Order와 해당 Index의 Order가 동일한 지 검사
    static IDE_RC checkIndexOrder( qmoAccessMethod   * aMethod,
                                   UShort              aTableID,
                                   qmgPreservedOrder * aWantOrder,
                                   idBool            * aUsable );

    // 원하는 Order가 해당 Index의 모든 Key Column에 포함되는 지 검사
    static IDE_RC checkIndexColumn( qcmIndex          * aIndex,
                                    UShort              aTableID,
                                    qmgPreservedOrder * aWantOrder,
                                    idBool            * aUsable );

    // 원하는 Order의 ID를 Target의 ID로 변경
    static IDE_RC refineID4Target( qmgPreservedOrder * aWantOrder,
                                   qmsTarget         * aTarget );

    // 해당 Graph에 대하여 Preserved Order를 생성함
    static IDE_RC makeOrder4Graph( qcStatement       * aStatement,
                                   qmgGraph          * aGraph,
                                   qmgPreservedOrder * aWantOrder );

    // Selection Graph에 대하여 Index를 이용한 Preserved Order를 생성함
    static IDE_RC makeOrder4Index( qcStatement       * aStatement,
                                   qmgGraph          * aGraph,
                                   qmoAccessMethod   * aMethod,
                                   qmgPreservedOrder * aWantOrder );

    // Non Leaf Graph에 대하여 Preserved Order를 생성함
    static IDE_RC makeOrder4NonLeafGraph( qcStatement       * aStatement,
                                          qmgGraph          * aGraph,
                                          qmgPreservedOrder * aWantOrder );

    // SYSDATE 등의 현재 날짜 관련  pseudo column인지 확인
    static idBool isDatePseudoColumn( qcStatement * aStatement,
                                      qtcNode     * aNode );
    
    // BUG-37355 node tree에서 pass node를 독립시킨다.
    static IDE_RC isolatePassNode( qcStatement * aStatement,
                                   qtcNode     * aSource );

    /* TASK-7219 */
    static IDE_RC getFromEnd( qmsFrom * aFrom,
                              SInt    * aFromWhereEnd );

    static IDE_RC getFromStart( qmsFrom * aFrom,
                                SInt    * aFromWhereStart );

    static IDE_RC getParamOffsetAndCount( qcStatement * aStatement,
                                          SInt          aStartOffset,
                                          SInt          aEndOffset,
                                          UShort        aStartParamOffset,
                                          UShort        aEndParamCount,
                                          UShort      * aParamOffset,
                                          UShort      * aParamCount );

    static IDE_RC getHostVarOffset( qcStatement * aStatement,
                                    UShort      * aParamOffset );
};

#endif /* _O_QMG_DEF_H_ */
