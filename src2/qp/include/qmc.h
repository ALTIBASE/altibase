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
 * $Id: qmc.h 90785 2021-05-06 07:26:22Z hykim $
 *
 * Description :
 *     Execution에서 사용하는 공통 모듈로
 *     Materialized Column에 대한 처리를 하는 작업이 주를 이룬다.
 *
 * 용어 설명 :
 *
 * 약어 :
 *
 **********************************************************************/

#ifndef _O_QMC_H_
#define _O_QMC_H_ 1

#include <mtcDef.h>
#include <qtcDef.h>
#include <qmoDef.h>
#include <qcmTableInfo.h>

// information about plan node's row

typedef SInt qmcRowFlag;

// qmcRowFlag
#define QMC_ROW_INITIALIZE        (0x00000000)

// qmcRowFlag
#define QMC_ROW_DATA_MASK         (0x00000001)
#define QMC_ROW_DATA_EXIST        (0x00000000)
#define QMC_ROW_DATA_NONE         (0x00000001)

// qmcRowFlag
#define QMC_ROW_VALUE_MASK        (0x00000002)
#define QMC_ROW_VALUE_EXIST       (0x00000000)
#define QMC_ROW_VALUE_NULL        (0x00000002)

// qmcRowFlag
#define QMC_ROW_GROUP_MASK        (0x00000004)
#define QMC_ROW_GROUP_NULL        (0x00000000)
#define QMC_ROW_GROUP_SAME        (0x00000004)

// qmcRowFlag
#define QMC_ROW_HIT_MASK          (0x00000008)
#define QMC_ROW_HIT_FALSE         (0x00000000)
#define QMC_ROW_HIT_TRUE          (0x00000008)

// qmcRowFlag, doit retry
#define QMC_ROW_RETRY_MASK        (0x00000010)
#define QMC_ROW_RETRY_FALSE       (0x00000000)
#define QMC_ROW_RETRY_TRUE        (0x00000010)

// BUG-33663 Ranking Function
// qmcRowFlag
#define QMC_ROW_COMPARE_MASK      (0x00000020)
#define QMC_ROW_COMPARE_SAME      (0x00000000)
#define QMC_ROW_COMPARE_DIFF      (0x00000020)

// PROJ-1071
#define QMC_ROW_QUEUE_EMPTY_MASK  (0x00000040)
#define QMC_ROW_QUEUE_EMPTY_FALSE (0x00000000)
#define QMC_ROW_QUEUE_EMPTY_TRUE  (0x00000040)

// PROJ-2750
#define QMC_ROW_NULL_PADDING_MASK     (0x00000080)
#define QMC_ROW_NULL_PADDING_FALSE    (0x00000000)
#define QMC_ROW_NULL_PADDING_TRUE     (0x00000080)

// information about materialization row

// qmcMtrNode.flag
#define QMC_MTR_INITIALIZE             (0x00000000)

//----------------------------------------------------------------------
// PROJ-2179
// 
// Materialized Node를 구성하는 Column의 종류
//
// 1. Base table을 저장할 때 surrogate key를 저장하는 용도로 사용한다.
// QMC_MTR_TYPE_MEMORY_TABLE             : Memory table의 base table(pointer)
// QMC_MTR_TYPE_MEMORY_PARTITIONED_TABLE : Partitioned memory table의
//                                         base table(pointer & tuple ID)
// QMC_MTR_TYPE_DISK_TABLE               : Disk table의 base table(RID)
// QMC_MTR_TYPE_DISK_PARTITIONED_TABLE   : Partitioned disk table의
//                                         base table(RID & tuple ID)
// QMC_MTR_TYPE_HYBRID_PARTITIONED_TABLE : Hybrid Partitioned Table의 Base Table(Pointer, RID and Tuple ID)
//
// 2. Memory column을 key로 사용하는 경우
// QMC_MTR_TYPE_MEMORY_KEY_COLUMN        : Memory table의 column (sorting/hashing key)
//                                         Memory table의 base table(pointer)
// QMC_MTR_TYPE_MEMORY_PARTITION_KEY_COLUMN : Memory Partitioned table의 column (sorting/hashing key)
//                                            base table(pointer & tuple ID)
//
// 3. 값을 복사해야 하는 경우
// QMC_MTR_TYPE_COPY_VALUE               : Disk table의 column, pseudo column
// QMC_MTR_TYPE_HYBRID_PARTITION_KEY_COLUMN : Hybrid Partitioned Table의 Column
//
// 4. Expression의 경우 calculate를 호출하는 용도로 사용한다.
// QMC_MTR_TYPE_CALCULATE                : 단순 column이 아닌 expression
// QMC_MTR_TYPE_CALCULATE_AND_COPY_VALUE : Subquery 등
//
// 5. MTRNode를 상위 Plan에서 참조(사용)하지 않는 경우
//    불필요한 MTRNode의 Size를 최소화 하기 위한 용도로 사용된다.
// QMC_MTR_TYPE_USELESS_COLUMN           : Dummy MTRNode
//----------------------------------------------------------------------
#define QMC_MTR_TYPE_MASK                           (0x000000FF)
#define QMC_MTR_TYPE_MEMORY_TABLE                   (0x00000001)
#define QMC_MTR_TYPE_DISK_TABLE                     (0x00000002)
// PROJ-1502 PARTITIONED DISK TABLE
#define QMC_MTR_TYPE_MEMORY_PARTITIONED_TABLE       (0x00000003)
#define QMC_MTR_TYPE_DISK_PARTITIONED_TABLE         (0x00000004)
#define QMC_MTR_TYPE_MEMORY_KEY_COLUMN              (0x00000005)
#define QMC_MTR_TYPE_MEMORY_PARTITION_KEY_COLUMN    (0x00000006)
#define QMC_MTR_TYPE_COPY_VALUE                     (0x00000007)
#define QMC_MTR_TYPE_CALCULATE                      (0x00000008)
#define QMC_MTR_TYPE_CALCULATE_AND_COPY_VALUE       (0x00000009)
// PROJ-2469 Optimize View Materialization
#define QMC_MTR_TYPE_USELESS_COLUMN                 (0x0000000A)
/* PROJ-2464 hybrid partitioned table 지원 */
#define QMC_MTR_TYPE_HYBRID_PARTITIONED_TABLE       (0x0000000B)
#define QMC_MTR_TYPE_HYBRID_PARTITION_KEY_COLUMN    (0x0000000C)

// qmcMtrNode.flag
// 대상 Materialized Column이 Sorting이 필요한 지의 여부
#define QMC_MTR_SORT_NEED_MASK                      (0x00000100)
#define QMC_MTR_SORT_NEED_FALSE                     (0x00000000)
#define QMC_MTR_SORT_NEED_TRUE                      (0x00000100)

// qmcMtrNode.flag
// 대상 Materialized Column이 Sorting이 필요한 경우, 그 Order
#define QMC_MTR_SORT_ORDER_MASK                     (0x00000200)
#define QMC_MTR_SORT_ASCENDING                      (0x00000000)
#define QMC_MTR_SORT_DESCENDING                     (0x00000200)

// qmcMtrNode.flag
// 대상 Materialized Column이 Hashing이 필요한 지의 여부
#define QMC_MTR_HASH_NEED_MASK                      (0x00000400)
#define QMC_MTR_HASH_NEED_FALSE                     (0x00000000)
#define QMC_MTR_HASH_NEED_TRUE                      (0x00000400)

// qmcMtrNode.flag
// 대상 Materialized Column이 GROUP BY 에 의한 것인지의 여부
#define QMC_MTR_GROUPING_MASK          ( QMC_MTR_HASH_NEED_MASK)
#define QMC_MTR_GROUPING_FALSE         (QMC_MTR_HASH_NEED_FALSE)
#define QMC_MTR_GROUPING_TRUE          ( QMC_MTR_HASH_NEED_TRUE)

// qmcMtrNode.flag
// 해당 Aggregation이 Distinct Column을 포함한지의 여부
#define QMC_MTR_DIST_AGGR_MASK                      (0x00000800)
#define QMC_MTR_DIST_AGGR_FALSE                     (0x00000000)
#define QMC_MTR_DIST_AGGR_TRUE                      (0x00000800)

// PROJ-1473
// 해당 노드의 column Locate가 변경되어야 한다.
#define QMC_MTR_CHANGE_COLUMN_LOCATE_MASK           (0x00001000)
#define QMC_MTR_CHANGE_COLUMN_LOCATE_FALSE          (0x00000000)
#define QMC_MTR_CHANGE_COLUMN_LOCATE_TRUE           (0x00001000)

// PROJ-1705
// mtrNode 구성시, 
#define QMC_MTR_BASETABLE_TYPE_MASK                 (0x00004000)
#define QMC_MTR_BASETABLE_TYPE_DISKTABLE            (0x00000000)
#define QMC_MTR_BASETABLE_TYPE_DISKTEMPTABLE        (0x00004000)

// BUG-31210
// wndNode의 analytic function result
#define QMC_MTR_ANAL_FUNC_RESULT_OF_WND_NODE_MASK   (0x00008000)
#define QMC_MTR_ANAL_FUNC_RESULT_OF_WND_NODE_FALSE  (0x00000000)
#define QMC_MTR_ANAL_FUNC_RESULT_OF_WND_NODE_TRUE   (0x00008000)

// BUG-31210
// materialization plan에서 mtrNode를 생성하는 경우 
#define QMC_MTR_MTR_PLAN_MASK                       (0x00010000)
#define QMC_MTR_MTR_PLAN_FALSE                      (0x00000000)
#define QMC_MTR_MTR_PLAN_TRUE                       (0x00010000)

// BUG-33663 Ranking Function
// mtrNode가 order by expr에 사용되어 order가 고정되었는지의 여부
#define QMC_MTR_SORT_ORDER_FIXED_MASK               (0x00020000)
#define QMC_MTR_SORT_ORDER_FIXED_FALSE              (0x00000000)
#define QMC_MTR_SORT_ORDER_FIXED_TRUE               (0x00020000)

// PROJ-1890 PROWID
// select target 에 prowid 가 있는경우 rid 복원이 필요함
#define QMC_MTR_RECOVER_RID_MASK                    (0x00040000)
#define QMC_MTR_RECOVER_RID_FALSE                   (0x00000000)
#define QMC_MTR_RECOVER_RID_TRUE                    (0x00040000)

// PROJ-1353 Dist Mtr Node를 중복해서 추가하기 위한 옵션
#define QMC_MTR_DIST_DUP_MASK                       (0x00080000)
#define QMC_MTR_DIST_DUP_FALSE                      (0x00000000)
#define QMC_MTR_DIST_DUP_TRUE                       (0x00080000)

// PROJ-2362 memory temp 저장 효율성 개선
#define QMC_MTR_TEMP_VAR_TYPE_ENABLE_MASK           (0x00100000)
#define QMC_MTR_TEMP_VAR_TYPE_ENABLE_FALSE          (0x00000000)
#define QMC_MTR_TEMP_VAR_TYPE_ENABLE_TRUE           (0x00100000)

// PROJ-2362 memory temp 저장 효율성 개선
#define QMC_MTR_BASETABLE_MASK                      (0x00200000)
#define QMC_MTR_BASETABLE_FALSE                     (0x00000000)
#define QMC_MTR_BASETABLE_TRUE                      (0x00200000)

/* PROJ-2435 order by nulls first/last */
#define QMC_MTR_SORT_NULLS_ORDER_MASK               (0x00C00000)
#define QMC_MTR_SORT_NULLS_ORDER_NONE               (0x00000000)
#define QMC_MTR_SORT_NULLS_ORDER_FIRST              (0x00400000)
#define QMC_MTR_SORT_NULLS_ORDER_LAST               (0x00800000)

/* BUG-39837 */
#define QMC_MTR_REMOTE_TABLE_MASK                   (0x01000000)
#define QMC_MTR_REMOTE_TABLE_TRUE                   (0x01000000)
#define QMC_MTR_REMOTE_TABLE_FALSE                  (0x00000000)

/* PROJ-2462 */
#define QMC_MTR_LOB_EXIST_MASK                      (0x02000000)
#define QMC_MTR_LOB_EXIST_TRUE                      (0x02000000)
#define QMC_MTR_LOB_EXIST_FALSE                     (0x00000000)

/* PROJ-2462 */
#define QMC_MTR_PRIOR_EXIST_MASK                    (0x04000000)
#define QMC_MTR_PRIOR_EXIST_TRUE                    (0x04000000)
#define QMC_MTR_PRIOR_EXIST_FALSE                   (0x00000000)

// PROJ-2362 memory temp 저장 효율성 개선
// 1. 가변길이 컬럼
// 2. non-padding type인 경우
// 3. 10 byte이상인 경우
// (10 byte char인 경우 precision 8에서 평균 4글자를 저장한다고 하면
// 6 byte이고 2 byte header를 추가하면 8 byte가 되어 2 byte의 이득이 발생한다.)
#if defined(DEBUG)
#define QMC_IS_MTR_TEMP_VAR_COLUMN( aMtcColumn )                        \
    ( ( ( ( (aMtcColumn).module->flag & MTD_VARIABLE_LENGTH_TYPE_MASK ) \
          == MTD_VARIABLE_LENGTH_TYPE_TRUE ) )                          \
      ? ID_TRUE : ID_FALSE )
#else
#define QMC_IS_MTR_TEMP_VAR_COLUMN( aMtcColumn )                        \
    ( ( ( ( (aMtcColumn).module->flag & MTD_VARIABLE_LENGTH_TYPE_MASK ) \
          == MTD_VARIABLE_LENGTH_TYPE_TRUE ) &&                         \
        ( (aMtcColumn).module->id != MTD_NCHAR_ID ) &&                  \
        ( (aMtcColumn).module->id != MTD_CHAR_ID ) &&                   \
        ( (aMtcColumn).module->id != MTD_BIT_ID ) &&                    \
        ( (aMtcColumn).column.size >= 10 ) )                            \
      ? ID_TRUE : ID_FALSE )
#endif

class qmcMemory;

//
//  qmc : only in Code Plan
//  qmd : only in Data Plan
//

typedef struct qmcMemSortElement
{
    qmcRowFlag flag;
} qmcMemSortElement;

typedef struct qmcMemHashElement
{
    qmcRowFlag        flag;
    UInt              key;
    qmcMemHashElement  * next;
} qmcMemHashElement;

//-------------------------------------------------
// 각 Temp Table의 Record Header Size
//-------------------------------------------------

#define QMC_MEMSORT_TEMPHEADER_SIZE                                     \
    ( idlOS::align(ID_SIZEOF(qmcMemSortElement), ID_SIZEOF(SDouble)) )
#define QMC_MEMHASH_TEMPHEADER_SIZE                                     \
    ( idlOS::align(ID_SIZEOF(qmcMemHashElement), ID_SIZEOF(SDouble)) )

/* PROJ-2201 Innovation in sorting and hashing(temp)
 * DiskTemp는 RowHeader가 없음 */
#define QMC_DISKSORT_TEMPHEADER_SIZE (0)
#define QMC_DISKHASH_TEMPHEADER_SIZE ( QMC_DISKSORT_TEMPHEADER_SIZE )

typedef struct qmcViewElement
{
    qmcViewElement * next;
} qmcViewElement;

typedef struct qmdNode
{
    qtcNode      * dstNode;
    mtcTuple     * dstTuple;     // means my Tuple,  just named with qmdMtrNode
    mtcColumn    * dstColumn;    // means my Column, just named with qmdMtrNode
    qmdNode      * next;
} qmdNode;

typedef struct qmcMemPartRowInfo
{
    scGRID         grid;            // BUG-38309 메모리 파티션일때도 rid 를 저장해야 한다.
    UShort         partitionTupleID;

    SChar        * position;
} qmcMemPartRowInfo;

typedef struct qmcDiskPartRowInfo
{
    scGRID         grid;
    UShort         partitionTupleID;

    // index table scan인 경우 index table rid도 원복할 필요가 있다.
    scGRID         indexGrid;
} qmcDiskPartRowInfo;

/* PROJ-2464 hybrid partitioned table 지원 */
typedef struct qmcPartRowInfo
{
    union
    {
        qmcMemPartRowInfo   memory;
        qmcDiskPartRowInfo  disk;
    } unionData;

    idBool         isDisk;
} qmcPartRowInfo;

struct qmcMtrNode;
struct qmdMtrNode;
struct qmdAggrNode;
struct qmdDistNode;

typedef IDE_RC (* qmcSetMtrFunc ) ( qcTemplate  * aTemplate,
                                    qmdMtrNode  * aNode,
                                    void        * aRow );

typedef IDE_RC (* qmcSetTupleFunc ) ( qcTemplate * aTemplate,
                                      qmdMtrNode * aNode,
                                      void       * aRow );

typedef UInt (* qmcGetHashFunc ) ( UInt         aValue,
                                   qmdMtrNode * aNode,
                                   void       * aRow );

typedef idBool (* qmcIsNullFunc ) ( qmdMtrNode * aNode,
                                    void       * aRow );

typedef void (* qmcMakeNullFunc ) ( qmdMtrNode * aNode,
                                    void       * aRow );

typedef void * (* qmcGetRowFunc) ( qmdMtrNode * aNode, const void * aRow );

typedef struct qmdMtrFunction
{
    qmcSetMtrFunc     setMtr;        // Materialized Column 구성 방법
    qmcGetHashFunc    getHash;       // Column의 Hash값 획득 방법
    qmcIsNullFunc     isNull;        // Column의 NULL 여부 판단 방법
    qmcMakeNullFunc   makeNull;      // Column의 NULL Value 생성 방법
    qmcGetRowFunc     getRow;        // Column을 기준으로 해당 Row 획득 방법
    qmcSetTupleFunc   setTuple;      // Column을 원복시키는 방법
    mtdCompareFunc    compare;       // Column의 대소 비교 방법
    mtcColumn       * compareColumn; // Compare의 기준이 되는 Column 정보
} qmdMtrFunction;

//---------------------------------------------
// Code 영역의 저장 Column의 정보
//---------------------------------------------

typedef struct qmcMtrNode
{
    //  materialization dstNode from srcNode
    //  if POINTER materialization, srcNode and dstNode are different
    //  if VALUE materialization, srcNode and dstNode are same
    qtcNode         * srcNode;
    qtcNode         * dstNode;
    qmcMtrNode      * next;

    UInt              flag;

    // only for AGGR
    qmcMtrNode      * myDist;    // only for Distinct Aggregation, or NULL
    UInt              bucketCnt; // only for Distinct Aggregation

    // only for GRST
    // qtcNode         * origNode; // TODO : A4 - Grouping Set
} qmcMtrNode;

//---------------------------------------------
// Data 영역의 저장 Column의 정보
//---------------------------------------------

typedef struct qmdMtrNode
{
    qtcNode               * dstNode;
    mtcTuple              * dstTuple;
    mtcColumn             * dstColumn;
    qmdMtrNode            * next;
    qtcNode               * srcNode;
    mtcTuple              * srcTuple;
    mtcColumn             * srcColumn;    

    qmcMtrNode            * myNode;
    qmdMtrFunction          func;
    UInt                    flag;

    smiFetchColumnList    * fetchColumnList; // PROJ-1705
    mtcTemplate           * tmplate;  // BUG-39896
    
} qmdMtrNode;

// attribute들의 flag

// Expression의 결과를 하위에서 그대로 전달받을지 여부
#define QMC_ATTR_SEALED_MASK            (0x00000001)
#define QMC_ATTR_SEALED_FALSE           (0x00000000)
#define QMC_ATTR_SEALED_TRUE            (0x00000001)

// Hashing/sorting이 필요한지 여부
#define QMC_ATTR_KEY_MASK               (0x00000002)
#define QMC_ATTR_KEY_FALSE              (0x00000000)
#define QMC_ATTR_KEY_TRUE               (0x00000002)

// Sorting이 필요한 경우 순서
#define QMC_ATTR_SORT_ORDER_MASK        (0x00000004)
#define QMC_ATTR_SORT_ORDER_ASC         (0x00000000)
#define QMC_ATTR_SORT_ORDER_DESC        (0x00000004)

// Conversion을 유지한 채 추가할지 여부
#define QMC_ATTR_CONVERSION_MASK        (0x00000008)
#define QMC_ATTR_CONVERSION_FALSE       (0x00000000)
#define QMC_ATTR_CONVERSION_TRUE        (0x00000008)

// Distinct 절이 사용된 expression인지 여부
#define QMC_ATTR_DISTINCT_MASK          (0x00000010)
#define QMC_ATTR_DISTINCT_FALSE         (0x00000000)
#define QMC_ATTR_DISTINCT_TRUE          (0x00000010)

// Terminal인지 여부(push down하지 않는다)
#define QMC_ATTR_TERMINAL_MASK          (0x00000020)
#define QMC_ATTR_TERMINAL_FALSE         (0x00000000)
#define QMC_ATTR_TERMINAL_TRUE          (0x00000020)

// Analytic function인지 여부
#define QMC_ATTR_ANALYTIC_FUNC_MASK     (0x00000040)
#define QMC_ATTR_ANALYTIC_FUNC_FALSE    (0x00000000)
#define QMC_ATTR_ANALYTIC_FUNC_TRUE     (0x00000040)

// Analytic function의 order by인지 여부
#define QMC_ATTR_ANALYTIC_SORT_MASK     (0x00000080)
#define QMC_ATTR_ANALYTIC_SORT_FALSE    (0x00000000)
#define QMC_ATTR_ANALYTIC_SORT_TRUE     (0x00000080)

// ORDER BY절에서 참조되었는지 여부
#define QMC_ATTR_ORDER_BY_MASK          (0x00000100)
#define QMC_ATTR_ORDER_BY_FALSE         (0x00000000)
#define QMC_ATTR_ORDER_BY_TRUE          (0x00000100)

// PROJ-1353 ROLLUP, CUBE의 인자로 사용된 컬럼과 Group by에 사용된 컬럼의 구분용도
#define QMC_ATTR_GROUP_BY_EXT_MASK      (0x00000200)
#define QMC_ATTR_GROUP_BY_EXT_FALSE     (0x00000000)
#define QMC_ATTR_GROUP_BY_EXT_TRUE      (0x00000200)

/* PROJ-2435 order by nulls first/last */
#define QMC_ATTR_SORT_NULLS_ORDER_MASK  (0x00000C00)
#define QMC_ATTR_SORT_NULLS_ORDER_NONE  (0x00000000)
#define QMC_ATTR_SORT_NULLS_ORDER_FIRST (0x00000400)
#define QMC_ATTR_SORT_NULLS_ORDER_LAST  (0x00000800)

/* PROJ-2469 Optimize View Materialization */
#define QMC_ATTR_USELESS_RESULT_MASK    (0x00001000)
#define QMC_ATTR_USELESS_RESULT_FALSE   (0x00000000)
#define QMC_ATTR_USELESS_RESULT_TRUE    (0x00001000)

// qmc::appendAttribute의 옵션

// 중복을 허용할 것인지 여부
#define QMC_APPEND_ALLOW_DUP_MASK       (0x00000001)
#define QMC_APPEND_ALLOW_DUP_FALSE      (0x00000000)
#define QMC_APPEND_ALLOW_DUP_TRUE       (0x00000001)

// 상수 또는 bind 변수를 허용할 것인지 여부
#define QMC_APPEND_ALLOW_CONST_MASK     (0x00000002)
#define QMC_APPEND_ALLOW_CONST_FALSE    (0x00000000)
#define QMC_APPEND_ALLOW_CONST_TRUE     (0x00000002)

// Expression의 경우 그대로 추가할 것인지 개별 구성요소들을 추가할것인지 여부
#define QMC_APPEND_EXPRESSION_MASK      (0x00000004)
#define QMC_APPEND_EXPRESSION_FALSE     (0x00000000)
#define QMC_APPEND_EXPRESSION_TRUE      (0x00000004)

// Analytic function의 analytic절(OVER절 이후) 검사 여부
#define QMC_APPEND_CHECK_ANALYTIC_MASK  (0x00000008)
#define QMC_APPEND_CHECK_ANALYTIC_FALSE (0x00000000)
#define QMC_APPEND_CHECK_ANALYTIC_TRUE  (0x00000008)

#define QMC_NEED_CALC( aQtcNode )                               \
    ( ( ( (aQtcNode)->node.module != &qtc::columnModule ) &&    \
        ( (aQtcNode)->node.module != &gQtcRidModule     ) &&    \
        ( (aQtcNode)->node.module != &qtc::valueModule  ) &&    \
        ( QTC_IS_AGGREGATE(aQtcNode) == ID_FALSE )              \
      ) ? ID_TRUE : ID_FALSE )

typedef struct qmcAttrDesc
{
    UInt                 flag;
    qtcNode            * expr;
    struct qmcAttrDesc * next;
} qmcAttrDesc;

class qmc
{
public:

    //-----------------------------------------------------
    // Default Execute 함수 Pointer
    //-----------------------------------------------------

    static mtcExecute      valueExecute;

    static IDE_RC          calculateValue( mtcNode*     aNode,
                                           mtcStack*    aStack,
                                           SInt         aRemain,
                                           void*        aInfo,
                                           mtcTemplate* aTemplate );

    //-----------------------------------------------------
    // [setMtr 계열 함수]
    // Column을 Materialized Row에 저장하는 방법
    //-----------------------------------------------------

    // 사용되지 않는 함수
    static IDE_RC setMtrNA( qcTemplate  * aTemplate,
                            qmdMtrNode  * aNode,
                            void        * aRow );

    // Pointer를 Materialized Row에 구성.
    static IDE_RC setMtrByPointer( qcTemplate  * aTemplate,
                                   qmdMtrNode  * aNode,
                                   void        * aRow );

    // RID를 Materialized Row에 구성
    static IDE_RC setMtrByRID( qcTemplate  * aTemplate,
                               qmdMtrNode  * aNode,
                               void        * aRow );

    // 연산 수행의 Value 자체를 Materialized Row에 구성
    static IDE_RC setMtrByValue( qcTemplate  * aTemplate,
                                 qmdMtrNode  * aNode,
                                 void        * aRow);

    // Column을 복사하여 Materialized Row에 구성
    static IDE_RC setMtrByCopy( qcTemplate  * aTemplate,
                                qmdMtrNode  * aNode,
                                void        * aRow);

    // Source Column의 연산 결과를 Materialized Row에 구성
    static IDE_RC setMtrByConvert( qcTemplate  * aTemplate,
                                   qmdMtrNode  * aNode,
                                   void        * aRow);

    /* PROJ-2464 hybrid partitioned table 지원 */
    static IDE_RC setMtrByCopyOrConvert( qcTemplate  * aTemplate,
                                         qmdMtrNode  * aNode,
                                         void        * aRow );

    // PROJ-2469 Optimize View Materialization
    static IDE_RC setMtrByNull( qcTemplate * aTemplate,
                                qmdMtrNode * aNode,
                                void       * aRow );

    // PROJ-1502 PARTITIONED DISK TABLE
    // Partitioned table의 경우 tuple id를 저장해야 함.
    static IDE_RC setMtrByPointerAndTupleID( qcTemplate  * aTemplate,
                                             qmdMtrNode  * aNode,
                                             void        * aRow);

    static IDE_RC setMtrByRIDAndTupleID( qcTemplate  * aTemplate,
                                         qmdMtrNode  * aNode,
                                         void        * aRow);

    /* PROJ-2464 hybrid partitioned table 지원 */
    static IDE_RC setMtrByPointerOrRIDAndTupleID( qcTemplate  * aTemplate,
                                                  qmdMtrNode  * aNode,
                                                  void        * aRow );

    // Pointer가 저장된 Column으로부터 row pointer획득
    static void * getRowByPointerAndTupleID( qmdMtrNode * aNode,
                                             const void * aRow );

    // Pointer가 저장된 Column으로부터 NULL 여부 판단
    static idBool   isNullByPointerAndTupleID( qmdMtrNode * aNode,
                                               void       * aRow );
    
    // Pointer가 저장된 Column으로부터 Hash 값 획득
    static UInt   getHashByPointerAndTupleID( UInt         aValue,
                                              qmdMtrNode * aNode,
                                              void       * aRow );
    
    //-----------------------------------------------------
    // [setTuple 계열 함수]
    // Materialized Row에 저장된 Column을 원복하는 방법
    //-----------------------------------------------------

    // 사용되지 않는 함수
    static IDE_RC   setTupleNA( qcTemplate * aTemplate,
                                qmdMtrNode * aNode,
                                void       * aRow );

    // Materialized Row에 Pointer로 저장된 Column을 복원
    static IDE_RC   setTupleByPointer( qcTemplate * aTemplate,
                                       qmdMtrNode * aNode,
                                       void       * aRow );

    // Materialized Row에 Disk Table을 위한 RID로 저장된 Column을 복원
    static IDE_RC   setTupleByRID( qcTemplate * aTemplate,
                                   qmdMtrNode * aNode,
                                   void       * aRow );

    // Materialized Row에 Value자체가 저장된 Column을 복원
    static IDE_RC   setTupleByValue( qcTemplate * aTemplate,
                                     qmdMtrNode *  aNode,
                                     void       * aRow );

    // PROJ-2469 Optmize View Materialization
    static IDE_RC   setTupleByNull( qcTemplate * aTemplate,
                                    qmdMtrNode * aNode,
                                    void       * aRow );    

    // PROJ-1502 PARTITIONED DISK TABLE
    // Partitioned table의 경우 tuple id도 원복해야 함.
    static IDE_RC   setTupleByRIDAndTupleID( qcTemplate * aTemplate,
                                             qmdMtrNode * aNode,
                                             void       * aRow );

    static IDE_RC   setTupleByPointerAndTupleID( qcTemplate * aTemplate,
                                                 qmdMtrNode * aNode,
                                                 void       * aRow );

    /* PROJ-2464 hybrid partitioned table 지원 */
    static IDE_RC   setTupleByPointerOrRIDAndTupleID( qcTemplate * aTemplate,
                                                      qmdMtrNode * aNode,
                                                      void       * aRow );

    //-----------------------------------------------------
    // [getHash 계열 함수]
    // Materialized Row에 저장된 Column의 Hash값을 얻는 방법
    //-----------------------------------------------------

    // 사용되지 않는 함수
    static UInt   getHashNA( UInt         aValue,
                             qmdMtrNode * aNode,
                             void       * aRow );

    // Pointer가 저장된 Column으로부터 Hash 값 획득
    static UInt   getHashByPointer( UInt         aValue,
                                    qmdMtrNode * aNode,
                                    void       * aRow );

    // Value가 저장된 Column으로부터 Hash 값 획득
    static UInt   getHashByValue( UInt         aValue,
                                  qmdMtrNode * aNode,
                                  void       * aRow );

    //-----------------------------------------------------
    // [isNull 계열 함수]
    // Materialized Row에 저장된 Column의 NULL여부 판단 방법
    //-----------------------------------------------------

    // 사용되지 않는 함수
    static idBool   isNullNA( qmdMtrNode * aNode,
                              void       * aRow );

    // Pointer가 저장된 Column으로부터 NULL 여부 판단
    static idBool   isNullByPointer( qmdMtrNode * aNode,
                                     void       * aRow );

    // Value가 저장된 Column으로부터 NULL 여부 판단
    static idBool   isNullByValue( qmdMtrNode * aNode,
                                   void       * aRow );

    //-----------------------------------------------------
    // [makeNull 계열 함수]
    // Materialized Row에 저장된 Column의 NULL여부 판단 방법
    //-----------------------------------------------------

    // 사용되지 않는 함수
    static void   makeNullNA( qmdMtrNode * aNode,
                              void       * aRow );

    // Null Value를 생성하지 않음
    static void   makeNullNothing( qmdMtrNode * aNode,
                                   void       * aRow );

    // Null Value를 생성
    static void   makeNullValue( qmdMtrNode * aNode,
                                 void       * aRow );

    //-----------------------------------------------------
    // [getRow 계열 함수]
    // Materialized Row의 저장된 Column의 실제 row pointer획득 함수
    //-----------------------------------------------------

    // 사용되지 않는 함수
    static void * getRowNA( qmdMtrNode * aNode,
                            const void * aRow );

    // Pointer가 저장된 Column으로부터 row pointer획득
    static void * getRowByPointer ( qmdMtrNode * aNode,
                                    const void * aRow );

    // Value가 저장된 Column으로부터 row pointer획득
    static void * getRowByValue ( qmdMtrNode * aNode,
                                  const void * aRow );

    //-----------------------------------------------------
    // [기타 지원 함수]
    //-----------------------------------------------------

    //---------------------------------------------
    // 저장 Node들의 올바른 사용을 위해서는 다음과 같은
    // 순서를 유지하며 처리하여야 한다.
    //     ::linkMtrNode()
    //     ::initMtrNode()
    //     ::refineOffsets()
    //     ::setRowSize()
    //     ::getRowSize()
    //---------------------------------------------

    // 1. Data 영역의 저장 Node를 연결
    static IDE_RC linkMtrNode( const qmcMtrNode * aCodeNode,
                               qmdMtrNode       * aDataNode );

    // 2. Data 영역의 저장 Node를 초기화
    static IDE_RC initMtrNode( qcTemplate * aTemplate,
                               qmdMtrNode * aDataNode,
                               UShort       aBaseTableCount );

    // 3. 저장되는 Column들의 offset을 재조정함.
    static IDE_RC refineOffsets( qmdMtrNode * aNode, UInt aStartOffset );

    // 4. Tuple 공간의 Size를 계산하고, Memory를 할당함.
    static IDE_RC setRowSize( iduMemory  * aMemory,
                              mtcTemplate* aTemplate,
                              UShort       aTupleID );

    // 5. 저장 Row의 Size를 구함
    static UInt   getMtrRowSize( qmdMtrNode * aNode );

    // Materialized Column을 처리할 함수 포인터를 결정
    static IDE_RC setFunctionPointer( qmdMtrNode * aDataNode );

    // Materialized Column의 비교 함수 포인터를 결정
    static IDE_RC setCompareFunction( qmdMtrNode * aDataNode );

    /* PROJ-2464 hybrid partitioned table 지원 */
    static UInt getRowOffsetForTuple( mtcTemplate * aTemplate,
                                      UShort        aTupleID );

    // PROJ-2179
    // Result descriptor를 다루기 위한 함수들
    static IDE_RC findAttribute( qcStatement  * aStatement,
                                 qmcAttrDesc  * aResult,
                                 qtcNode      * aExpr,
                                 idBool         aMakePassNode,
                                 qmcAttrDesc ** aAttr );

    static IDE_RC makeReference( qcStatement  * aStatement,
                                 idBool         aMakePassNode,
                                 qtcNode      * aRefNode,
                                 qtcNode     ** aDepNode );

    static IDE_RC makeReferenceResult( qcStatement * aStatement,
                                       idBool        aMakePassNode,
                                       qmcAttrDesc * aParent,
                                       qmcAttrDesc * aChild );

    static IDE_RC appendPredicate( qcStatement  * aStatement,
                                   qmsQuerySet  * aQuerySet,
                                   qmcAttrDesc ** aResult,
                                   qtcNode      * aPredicate,
                                   UInt           aFlag = 0 );

    static IDE_RC appendPredicate( qcStatement  * aStatement,
                                   qmsQuerySet  * aQuerySet,
                                   qmcAttrDesc ** aResult,
                                   qmoPredicate * aPredicate,
                                   UInt           aFlag = 0 );

    static IDE_RC appendAttribute( qcStatement  * aStatement,
                                   qmsQuerySet  * aQuerySet,
                                   qmcAttrDesc ** aResult,
                                   qtcNode      * aExpr,
                                   UInt           aFlag,
                                   UInt           aOption,
                                   idBool         aMakePassNode );

    static IDE_RC createResultFromQuerySet( qcStatement  * aStatement,
                                            qmsQuerySet  * aQuerySet,
                                            qmcAttrDesc ** aResult );

    static IDE_RC copyResultDesc( qcStatement        * aStatement,
                                  const qmcAttrDesc  * aParent,
                                  qmcAttrDesc       ** aChild );

    static IDE_RC pushResultDesc( qcStatement  * aStatement,
                                  qmsQuerySet  * aQuerySet,
                                  idBool         aMakePassNode,
                                  qmcAttrDesc  * aParent,
                                  qmcAttrDesc ** aChild );

    static IDE_RC filterResultDesc( qcStatement  * aStatement,
                                    qmcAttrDesc ** aPlan,
                                    qcDepInfo    * aDepInfo,
                                    qcDepInfo    * aDepInfo2 );

    static IDE_RC appendViewPredicate( qcStatement  * aStatement,
                                       qmsQuerySet  * aQuerySet,
                                       qmcAttrDesc ** aResult,
                                       UInt           aFlag );

    static IDE_RC duplicateGroupExpression( qcStatement * aStatement,
                                            qtcNode     * aPredicate );

    static void disableSealTrueFlag( qmcAttrDesc * aResult );
private :

    //-----------------------------------------------------
    // 저장 Node 초기화 관련 함수
    //-----------------------------------------------------

    // 저장 Column들의 정보를 설정한다.
    static IDE_RC setMtrNode( qcTemplate * aTemplate,
                              qmdMtrNode * aDataNode,
                              qmcMtrNode * aCodeNode,
                              idBool       aBaseTable );

    //---------------------------
    // 함수 포인터 초기화 관련 함수
    //---------------------------

    // Memory Column에 대한 함수 포인터 결정
    static IDE_RC setFunction4MemoryColumn( qmdMtrNode * aDataNode );

    // Memory Partition Column에 대한 함수 포인터 결정
    static IDE_RC setFunction4MemoryPartitionColumn( qmdMtrNode * aDataNode );

    // Disk Column에 대한 함수 포인터 결정
    static IDE_RC setFunction4DiskColumn( qmdMtrNode * aDataNode );

    // Constant에 대한 함수 포인터 결정
    static IDE_RC setFunction4Constant( qmdMtrNode * aDataNode );

    // PROJ-1502 PARTITIONED DISK TABLE
    // Disk Table의 Key Column에 대한 함수 포인터 결정
    static IDE_RC setFunction4DiskKeyColumn( qmdMtrNode * aDataNode );

    //-----------------------------------------------------
    // Offset 조정 관련 함수
    //-----------------------------------------------------

    // Memory Column이 저장될 때의 offset 조정
    static IDE_RC refineOffset4MemoryColumn( qmdMtrNode * aListNode,
                                             qmdMtrNode * aColumnNode,
                                             UInt       * aOffset );

    // Memory Partition Column이 저장될 때의 offset 조정
    static IDE_RC refineOffset4MemoryPartitionColumn( qmdMtrNode * aListNode,
                                                      qmdMtrNode * aColumnNode,
                                                      UInt       * aOffset );

    // Disk Column이 저장될 때의 offset 조정
    static IDE_RC refineOffset4DiskColumn( qmdMtrNode * aListNode,
                                           qmdMtrNode * aColumnNode,
                                           UInt       * aOffset );

    static IDE_RC setTupleByPointer4Rid(qcTemplate  * aTemplate,
                                        qmdMtrNode  * aNode,
                                        void        * aRow);

    static IDE_RC appendQuerySetCorrelation( qcStatement  * aStatement,
                                             qmsQuerySet  * aQuerySet,
                                             qmcAttrDesc ** aResult,
                                             qmsQuerySet  * aSubquerySet,
                                             qcDepInfo    * aDepInfo );

    static IDE_RC appendSubqueryCorrelation( qcStatement  * aStatement,
                                             qmsQuerySet  * aQuerySet,
                                             qmcAttrDesc ** aResult,
                                             qtcNode      * aSubqueryNode );
};

#endif /* _O_QMC_H_ */
