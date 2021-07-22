/** 
 *  Copyright (c) 1999~2017, Altibase Corp. and/or its affiliates. All rights reserved.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License, version 3,
 *  as published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
 

/***********************************************************************
 * $Id: mtcDef.h 90192 2021-03-12 02:01:03Z jayce.park $
 **********************************************************************/

#ifndef _O_MTC_DEF_H_
# define _O_MTC_DEF_H_ 1

# include <smiDef.h>
# include <iduMemory.h>
# include <iduVarMemList.h>
# include <mtlTerritory.h>

/* PROJ-2429 Dictionary based data compress for on-disk DB */
# define MTD_COLUMN_COPY_FUNC_NORMAL                (0)
# define MTD_COLUMN_COPY_FUNC_COMPRESSION           (1)
# define MTD_COLUMN_COPY_FUNC_MAX_COUNT             (2)

/* Partial Key                                       */
# define MTD_PARTIAL_KEY_MINIMUM                    (0)
# define MTD_PARTIAL_KEY_MAXIMUM         (ID_ULONG_MAX)

//====================================================================
// mtcColumn.flag 정의
//====================================================================

/* mtcColumn.flag                                    */
# define MTC_COLUMN_ARGUMENT_COUNT_MASK    (0x00000003)
# define MTC_COLUMN_ARGUMENT_COUNT_MAXIMUM          (2)

/* mtcColumn.flag                                    */
# define MTC_COLUMN_NOTNULL_MASK           (0x00000004)
# define MTC_COLUMN_NOTNULL_TRUE           (0x00000004)
# define MTC_COLUMN_NOTNULL_FALSE          (0x00000000)

// PROJ-1579 NCHAR
/* mtcColumn.flag                                    */
# define MTC_COLUMN_LITERAL_TYPE_MASK      (0x00003000)
# define MTC_COLUMN_LITERAL_TYPE_NCHAR     (0x00001000)
# define MTC_COLUMN_LITERAL_TYPE_UNICODE   (0x00002000)
# define MTC_COLUMN_LITERAL_TYPE_NORMAL    (0x00000000)

// PROJ-1473
/* mtcColumn.flag                                    */
# define MTC_COLUMN_USE_COLUMN_MASK        (0x00004000)
# define MTC_COLUMN_USE_COLUMN_TRUE        (0x00004000)
# define MTC_COLUMN_USE_COLUMN_FALSE       (0x00000000)

// PROJ-1473
/* mtcColumn.flag                                    */
# define MTC_COLUMN_USE_TARGET_MASK        (0x00008000)
# define MTC_COLUMN_USE_TARGET_TRUE        (0x00008000)
# define MTC_COLUMN_USE_TARGET_FALSE       (0x00000000)

// PROJ-1473
/* mtcColumn.flag                                    */
# define MTC_COLUMN_USE_NOT_TARGET_MASK    (0x00010000)
# define MTC_COLUMN_USE_NOT_TARGET_TRUE    (0x00010000)
# define MTC_COLUMN_USE_NOT_TARGET_FALSE   (0x00000000)

// PROJ-1473
/* mtcColumn.flag                                    */
# define MTC_COLUMN_VIEW_COLUMN_PUSH_MASK  (0x00020000)
# define MTC_COLUMN_VIEW_COLUMN_PUSH_TRUE  (0x00020000)
# define MTC_COLUMN_VIEW_COLUMN_PUSH_FALSE (0x00000000)

/* mtcColumn.flag                                    */
# define MTC_COLUMN_TIMESTAMP_MASK         (0x00040000)
# define MTC_COLUMN_TIMESTAMP_TRUE         (0x00040000)
# define MTC_COLUMN_TIMESTAMP_FALSE        (0x00000000)

/* mtcColumn.flag                                    */
# define MTC_COLUMN_QUEUE_MSGID_MASK       (0x00080000)
# define MTC_COLUMN_QUEUE_MSGID_TRUE       (0x00080000)
# define MTC_COLUMN_QUEUE_MSGID_FALSE      (0x00000000)

// BUG-25470
/* mtcColumn.flag                                    */
# define MTC_COLUMN_OUTER_REFERENCE_MASK   (0x00100000)
# define MTC_COLUMN_OUTER_REFERENCE_TRUE   (0x00100000)
# define MTC_COLUMN_OUTER_REFERENCE_FALSE  (0x00000000)

// PROJ-2204 Join Update, Delete
/* mtcColumn.flag                                    */
# define MTC_COLUMN_KEY_PRESERVED_MASK     (0x00200000)
# define MTC_COLUMN_KEY_PRESERVED_TRUE     (0x00200000)
# define MTC_COLUMN_KEY_PRESERVED_FALSE    (0x00000000)

// PROJ-1784 DML Without Retry
/* mtcColumn.flag                                    */
# define MTC_COLUMN_USE_WHERE_MASK         (0x00400000)
# define MTC_COLUMN_USE_WHERE_TRUE         (0x00400000)
# define MTC_COLUMN_USE_WHERE_FALSE        (0x00000000)

// PROJ-1784 DML Without Retry
/* mtcColumn.flag                                    */
# define MTC_COLUMN_USE_SET_MASK           (0x00800000)
# define MTC_COLUMN_USE_SET_TRUE           (0x00800000)
# define MTC_COLUMN_USE_SET_FALSE          (0x00000000)

/* PROJ-2586 PSM Parameters and return without precision
   mtcColumn.flag
   PSM 객체 생성 시 parameter 및 return의 precision을 default 값으로 변경된 column임을 의미 */
# define MTC_COLUMN_SP_SET_PRECISION_MASK      (0x01000000)
# define MTC_COLUMN_SP_SET_PRECISION_TRUE      (0x01000000)
# define MTC_COLUMN_SP_SET_PRECISION_FALSE     (0x00000000)

/* PROJ-2586 PSM Parameters and return without precision
   mtcColumn.flag
   PSM 실행 시 parameter 및 return의 precision을 argument 및 result value에 맞춰 column 정보가 변경됨을 의미 */
# define MTC_COLUMN_SP_ADJUST_PRECISION_MASK      (0x02000000)
# define MTC_COLUMN_SP_ADJUST_PRECISION_TRUE      (0x02000000)
# define MTC_COLUMN_SP_ADJUST_PRECISION_FALSE     (0x00000000)

/* TASK-7219 */
# define MTC_COLUMN_NULL_TYPE_MASK         (0x04000000)
# define MTC_COLUMN_NULL_TYPE_TRUE         (0x04000000)
# define MTC_COLUMN_NULL_TYPE_FALSE        (0x00000000)

//====================================================================
// mtcNode.flag 정의
//====================================================================

// BUG-34127 Increase maximum arguement count(4095(0x0FFF) -> 12287(0x2FFF))
/* mtcNode.flag                                      */
# define MTC_NODE_ARGUMENT_COUNT_MASK           ID_ULONG(0x000000000000FFFF)
# define MTC_NODE_ARGUMENT_COUNT_MAXIMUM                             (12287)

/* mtfModule.flag                                    */
// 연산자가 차지하는 Column의 개수로 AVG()와 같은 연산은
// SUM()과 COUNT()정보등을 관리하기 위해 여러개의
// Column 을 유지해야 한다.
# define MTC_NODE_COLUMN_COUNT_MASK             ID_ULONG(0x00000000000000FF)

/* mtcNode.flag                                      */
# define MTC_NODE_MASK        ( MTC_NODE_INDEX_MASK | \
                                MTC_NODE_BIND_MASK  | \
                                MTC_NODE_DML_MASK   | \
                                MTC_NODE_EAT_NULL_MASK | \
                                MTC_NODE_FUNCTION_CONNECT_BY_MASK | \
                                MTC_NODE_HAVE_SUBQUERY_MASK )

/* mtcNode.flag, mtfModule.flag                      */
# define MTC_NODE_OPERATOR_MASK                 ID_ULONG(0x00000000000F0000)
# define MTC_NODE_OPERATOR_MISC                 ID_ULONG(0x0000000000000000)
# define MTC_NODE_OPERATOR_AND                  ID_ULONG(0x0000000000010000)
# define MTC_NODE_OPERATOR_OR                   ID_ULONG(0x0000000000020000)
# define MTC_NODE_OPERATOR_NOT                  ID_ULONG(0x0000000000030000)
# define MTC_NODE_OPERATOR_EQUAL                ID_ULONG(0x0000000000040000)
# define MTC_NODE_OPERATOR_NOT_EQUAL            ID_ULONG(0x0000000000050000)
# define MTC_NODE_OPERATOR_GREATER              ID_ULONG(0x0000000000060000)
# define MTC_NODE_OPERATOR_GREATER_EQUAL        ID_ULONG(0x0000000000070000)
# define MTC_NODE_OPERATOR_LESS                 ID_ULONG(0x0000000000080000)
# define MTC_NODE_OPERATOR_LESS_EQUAL           ID_ULONG(0x0000000000090000)
# define MTC_NODE_OPERATOR_RANGED               ID_ULONG(0x00000000000A0000)
# define MTC_NODE_OPERATOR_NOT_RANGED           ID_ULONG(0x00000000000B0000)
# define MTC_NODE_OPERATOR_LIST                 ID_ULONG(0x00000000000C0000)
# define MTC_NODE_OPERATOR_FUNCTION             ID_ULONG(0x00000000000D0000)
# define MTC_NODE_OPERATOR_AGGREGATION          ID_ULONG(0x00000000000E0000)
# define MTC_NODE_OPERATOR_SUBQUERY             ID_ULONG(0x00000000000F0000)

/* mtcNode.flag                                      */
# define MTC_NODE_INDIRECT_MASK                 ID_ULONG(0x0000000000100000)
# define MTC_NODE_INDIRECT_TRUE                 ID_ULONG(0x0000000000100000)
# define MTC_NODE_INDIRECT_FALSE                ID_ULONG(0x0000000000000000)
                                            
/* mtcNode.flag, mtfModule.flag                      */
# define MTC_NODE_COMPARISON_MASK               ID_ULONG(0x0000000000200000)
# define MTC_NODE_COMPARISON_TRUE               ID_ULONG(0x0000000000200000)
# define MTC_NODE_COMPARISON_FALSE              ID_ULONG(0x0000000000000000)

/* mtcNode.flag, mtfModule.flag                      */
# define MTC_NODE_LOGICAL_CONDITION_MASK        ID_ULONG(0x0000000000400000)
# define MTC_NODE_LOGICAL_CONDITION_TRUE        ID_ULONG(0x0000000000400000)
# define MTC_NODE_LOGICAL_CONDITION_FALSE       ID_ULONG(0x0000000000000000)

//----------------------------------------------------
// [MTC_NODE_INDEX_USABLE]의 의미
// mtfModule.mask와 하위 Node들의 flag정보를 이용하여
// 해당 Node의 flag정보를 Setting하여 결정된다.
// MTC_NODE_INDEX_USABLE은 해당 Predicate의
// 다음과 같은 사항을 보장한다.
//     1. Indexable Operator가 사용됨
//         - A3와 달리 A4에서는 system level의
//           indexable operator뿐만 아니라,
//           Node Transform등을 이용하여 인덱스를
//           사용할 수 있는 user level의 indexable
//           operator임을 의미한다.
//     2. Column이 존재함.
//     3. Column에 Expression이 포함되어 있지 않음.
//----------------------------------------------------

/* mtcNode.flag, mtfModule.flag                      */
# define MTC_NODE_INDEX_MASK                    ID_ULONG(0x0000000000800000)
# define MTC_NODE_INDEX_USABLE                  ID_ULONG(0x0000000000800000)
# define MTC_NODE_INDEX_UNUSABLE                ID_ULONG(0x0000000000000000)

/* mtcNode.flag                                      */
// 현재와 하위 노드에 호스트 변수가 존재함을 나타낸다.
# define MTC_NODE_BIND_MASK                     ID_ULONG(0x0000000001000000)
# define MTC_NODE_BIND_EXIST                    ID_ULONG(0x0000000001000000)
# define MTC_NODE_BIND_ABSENT                   ID_ULONG(0x0000000000000000)
                                            
/* mtcNode.flag                                      */
# define MTC_NODE_QUANTIFIER_MASK               ID_ULONG(0x0000000002000000)
# define MTC_NODE_QUANTIFIER_TRUE               ID_ULONG(0x0000000002000000)
# define MTC_NODE_QUANTIFIER_FALSE              ID_ULONG(0x0000000000000000)
                                            
/* mtcNode.flag                                      */
# define MTC_NODE_DISTINCT_MASK                 ID_ULONG(0x0000000004000000)
# define MTC_NODE_DISTINCT_TRUE                 ID_ULONG(0x0000000004000000)
# define MTC_NODE_DISTINCT_FALSE                ID_ULONG(0x0000000000000000)

/* mtcNode.flag                                      */
# define MTC_NODE_GROUP_COMPARISON_MASK         ID_ULONG(0x0000000008000000)
# define MTC_NODE_GROUP_COMPARISON_TRUE         ID_ULONG(0x0000000008000000)
# define MTC_NODE_GROUP_COMPARISON_FALSE        ID_ULONG(0x0000000000000000)

/* mtcNode.flag                                      */
# define MTC_NODE_GROUP_MASK                    ID_ULONG(0x0000000010000000)
# define MTC_NODE_GROUP_ANY                     ID_ULONG(0x0000000010000000)
# define MTC_NODE_GROUP_ALL                     ID_ULONG(0x0000000000000000)
                                            
/* mtcNode.flag                                      */
# define MTC_NODE_FILTER_MASK                   ID_ULONG(0x0000000020000000)
# define MTC_NODE_FILTER_NEED                   ID_ULONG(0x0000000020000000)
# define MTC_NODE_FILTER_NEEDLESS               ID_ULONG(0x0000000000000000)
                                            
/* mtcNode.flag                                      */
# define MTC_NODE_DML_MASK                      ID_ULONG(0x0000000040000000)
# define MTC_NODE_DML_UNUSABLE                  ID_ULONG(0x0000000040000000)
# define MTC_NODE_DML_USABLE                    ID_ULONG(0x0000000000000000)

/* mtcNode.flag                                      */
// fix BUG-13830
# define MTC_NODE_REESTIMATE_MASK               ID_ULONG(0x0000000080000000)
# define MTC_NODE_REESTIMATE_TRUE               ID_ULONG(0x0000000080000000)
# define MTC_NODE_REESTIMATE_FALSE              ID_ULONG(0x0000000000000000)

/* mtfModule.flag                                    */
// To Fix PR-15434 INDEX JOIN이 가능한 연산자인지의 여부
# define MTC_NODE_INDEX_JOINABLE_MASK           ID_ULONG(0x0000000100000000)
# define MTC_NODE_INDEX_JOINABLE_TRUE           ID_ULONG(0x0000000100000000)
# define MTC_NODE_INDEX_JOINABLE_FALSE          ID_ULONG(0x0000000000000000)
// To Fix PR-15291                          
# define MTC_NODE_INDEX_ARGUMENT_MASK           ID_ULONG(0x0000000200000000)
# define MTC_NODE_INDEX_ARGUMENT_BOTH           ID_ULONG(0x0000000200000000)
# define MTC_NODE_INDEX_ARGUMENT_LEFT           ID_ULONG(0x0000000000000000)

// BUG-15995
/* mtfModule.flag                                    */
# define MTC_NODE_VARIABLE_MASK                 ID_ULONG(0x0000000400000000)
# define MTC_NODE_VARIABLE_TRUE                 ID_ULONG(0x0000000400000000)
# define MTC_NODE_VARIABLE_FALSE                ID_ULONG(0x0000000000000000)

// PROJ-1492
// 현재노드의 타입이 validation시에 결정되었음을 나타낸다.
/* mtcNode.flag                                      */
# define MTC_NODE_BIND_TYPE_MASK                ID_ULONG(0x0000000800000000)
# define MTC_NODE_BIND_TYPE_TRUE                ID_ULONG(0x0000000800000000)
# define MTC_NODE_BIND_TYPE_FALSE               ID_ULONG(0x0000000000000000)

/* BUG-44762 case when subquery */
# define MTC_NODE_HAVE_SUBQUERY_MASK            ID_ULONG(0x0000001000000000)
# define MTC_NODE_HAVE_SUBQUERY_TRUE            ID_ULONG(0x0000001000000000)
# define MTC_NODE_HAVE_SUBQUERY_FALSE           ID_ULONG(0x0000000000000000)

//fix BUG-17610
# define MTC_NODE_EQUI_VALID_SKIP_MASK          ID_ULONG(0x0000002000000000)
# define MTC_NODE_EQUI_VALID_SKIP_TRUE          ID_ULONG(0x0000002000000000)
# define MTC_NODE_EQUI_VALID_SKIP_FALSE         ID_ULONG(0x0000000000000000)

// fix BUG-22512 Outer Join 계열 Push Down Predicate 할 경우, 결과 오류
// CASE2, DECODE, DIGEST, DUMP, IS NULL, IS NOT NULL, NVL, NVL2, UserDefined Function 함수는
// 인자로 받은 NULL값을 다른 값으로 바꿀 수 있다.
// CASE2, DECODE, DIGEST, DUMP, IS NULL, IS NOT NULL, NVL, NVL2, UserDefined Function 함수는 
// outer join 하위 노드로 push down 할 수 없는 function임을 표시.
# define MTC_NODE_EAT_NULL_MASK                 ID_ULONG(0x0000004000000000)
# define MTC_NODE_EAT_NULL_TRUE                 ID_ULONG(0x0000004000000000)
# define MTC_NODE_EAT_NULL_FALSE                ID_ULONG(0x0000000000000000)

//----------------------------------------------------
// BUG-19180
// 노드기반의 predicate 출력을 위해 다음과 같이
// built-in function들의 출력방법을 정의한다.
// 1. prefix( 선순위와 괄호
//    예) ABS(...), SUB(...), FUNC1(...)
// 2. prefix_ 선순위와 공백
//    예) EXISTS @, UNIQUE @, NOT @
// 3. infix 중순위
//    예) @+@, @=ANY@
// 4. infix_ 중순위와 공백
//    예) @ AND @, @ IN @
// 5. _postfix 후순위와 공백
//    예) @ IS NULL
// 6. 기타
//    예) between, like, cast, concat, count, minus, list 등
//----------------------------------------------------
# define MTC_NODE_PRINT_FMT_MASK                ID_ULONG(0x00000F0000000000)
# define MTC_NODE_PRINT_FMT_PREFIX_PA           ID_ULONG(0x0000000000000000)
# define MTC_NODE_PRINT_FMT_PREFIX_SP           ID_ULONG(0x0000010000000000)
# define MTC_NODE_PRINT_FMT_INFIX               ID_ULONG(0x0000020000000000)
# define MTC_NODE_PRINT_FMT_INFIX_SP            ID_ULONG(0x0000030000000000)
# define MTC_NODE_PRINT_FMT_POSTFIX_SP          ID_ULONG(0x0000040000000000)
# define MTC_NODE_PRINT_FMT_MISC                ID_ULONG(0x0000050000000000)

// PROJ-1473
# define MTC_NODE_COLUMN_LOCATE_CHANGE_MASK     ID_ULONG(0x0000100000000000)
# define MTC_NODE_COLUMN_LOCATE_CHANGE_FALSE    ID_ULONG(0x0000100000000000)
# define MTC_NODE_COLUMN_LOCATE_CHANGE_TRUE     ID_ULONG(0x0000000000000000)

// PROJ-1473
# define MTC_NODE_REMOVE_ARGUMENTS_MASK         ID_ULONG(0x0000200000000000)
# define MTC_NODE_REMOVE_ARGUMENTS_TRUE         ID_ULONG(0x0000200000000000)
# define MTC_NODE_REMOVE_ARGUMENTS_FALSE        ID_ULONG(0x0000000000000000)

// BUG-28223 CASE expr WHEN .. THEN .. 구문의 expr에 subquery 사용시 에러발생 
// .SIMPLE :
//          select case i1 when 1 then 777
//                         when 2 then 666
//                         when 3 then 555
//                 else 888
//                 end
//            from t1;
// .SEARCHED:
//           select case when i1=1 then 777
//                       when i1=2 then 666
//                       when i1=3 then 555
//                  else 888
//                  end
//             from t1;
// qtc::estimateInternal()함수의 주석 참조.
# define MTC_NODE_CASE_TYPE_MASK                ID_ULONG(0x0000400000000000) 
# define MTC_NODE_CASE_TYPE_SIMPLE              ID_ULONG(0x0000400000000000) 
# define MTC_NODE_CASE_TYPE_SEARCHED            ID_ULONG(0x0000000000000000)

// BUG-38133
// BUG-28223 CASE expr WHEN .. THEN .. 구문의 expr에 subquery 사용시 에러발생 
// qtc::estimateInternal()함수의 주석 참조.
#define MTC_NODE_CASE_EXPRESSION_MAKE_PASSNODE_MASK  ID_ULONG(0x0000800000000000)
#define MTC_NODE_CASE_EXPRESSION_MAKE_PASSNODE_TRUE  ID_ULONG(0x0000800000000000)
#define MTC_NODE_CASE_EXPRESSION_MAKE_PASSNODE_FALSE ID_ULONG(0x0000000000000000)

// BUG-28446 [valgrind], BUG-38133
// qtcCalculate_Pass(mtcNode*, mtcStack*, int, void*, mtcTemplate*)
#define MTC_NODE_CASE_EXPRESSION_PASSNODE_MASK  ID_ULONG(0x0001000000000000)
#define MTC_NODE_CASE_EXPRESSION_PASSNODE_TRUE  ID_ULONG(0x0001000000000000)
#define MTC_NODE_CASE_EXPRESSION_PASSNODE_FALSE ID_ULONG(0x0000000000000000)

// BUG-31777 덧셈과 곱셈 연산자에 대해서 교환법칙을 허용한다.
/* mtcNode.flag, mtfModule.flag */
#define MTC_NODE_COMMUTATIVE_MASK               ID_ULONG(0x0002000000000000)
#define MTC_NODE_COMMUTATIVE_TRUE               ID_ULONG(0x0002000000000000)
#define MTC_NODE_COMMUTATIVE_FALSE              ID_ULONG(0x0000000000000000)

// BUG-33663 Ranking Function
/* mtcNode.flag, mtfModule.flag */
#define MTC_NODE_FUNCTION_RANKING_MASK          ID_ULONG(0x0004000000000000)
#define MTC_NODE_FUNCTION_RANKING_TRUE          ID_ULONG(0x0004000000000000)
#define MTC_NODE_FUNCTION_RANKING_FALSE         ID_ULONG(0x0000000000000000)

/* PROJ-1353 Rollup, Cube */
#define MTC_NODE_FUNCTON_GROUPING_MASK          ID_ULONG(0x0008000000000000)
#define MTC_NODE_FUNCTON_GROUPING_TRUE          ID_ULONG(0x0008000000000000)
#define MTC_NODE_FUNCTON_GROUPING_FALSE         ID_ULONG(0x0000000000000000)

/* BUG-35252 , BUG-39611*/
#define MTC_NODE_FUNCTION_CONNECT_BY_MASK      ID_ULONG(0x0010000000000000)
#define MTC_NODE_FUNCTION_CONNECT_BY_TRUE      ID_ULONG(0x0010000000000000)
#define MTC_NODE_FUNCTION_CONNECT_BY_FALSE     ID_ULONG(0x0000000000000000)

// BUG-35155 Partial CNF
// PARTIAL_NORMALIZE_CNF_TRUE  : use predicate when make CNF
// PARTIAL_NORMALIZE_CNF_FALSE : don't use predicate when make CNF
// PARTIAL_NORMALIZE_DNF_TRUE  : use predicate when make DNF (미구현)
// PARTIAL_NORMALIZE_DNF_FALSE : don't use predicate when make DNF
/* mtcNode.flag, mtfModule.flag                      */
# define MTC_NODE_PARTIAL_NORMALIZE_CNF_MASK     ID_ULONG(0x0020000000000000)
# define MTC_NODE_PARTIAL_NORMALIZE_CNF_USABLE   ID_ULONG(0x0000000000000000)
# define MTC_NODE_PARTIAL_NORMALIZE_CNF_UNUSABLE ID_ULONG(0x0020000000000000)

# define MTC_NODE_PARTIAL_NORMALIZE_DNF_MASK     ID_ULONG(0x0040000000000000)
# define MTC_NODE_PARTIAL_NORMALIZE_DNF_USABLE   ID_ULONG(0x0000000000000000)
# define MTC_NODE_PARTIAL_NORMALIZE_DNF_UNUSABLE ID_ULONG(0x0040000000000000)

/* PROJ-1805 Window Clause */
# define MTC_NODE_FUNCTION_WINDOWING_MASK        ID_ULONG(0x0080000000000000)
# define MTC_NODE_FUNCTION_WINDOWING_TRUE        ID_ULONG(0x0080000000000000)
# define MTC_NODE_FUNCTION_WINDOWING_FALSE       ID_ULONG(0x0000000000000000)

/* PROJ-1805 Window Clause */
# define MTC_NODE_FUNCTION_ANALYTIC_MASK         ID_ULONG(0x0100000000000000)
# define MTC_NODE_FUNCTION_ANALYTIC_TRUE         ID_ULONG(0x0100000000000000)
# define MTC_NODE_FUNCTION_ANALYTIC_FALSE        ID_ULONG(0x0000000000000000)

// PROJ-2527 WITHIN GROUP AGGR
# define MTC_NODE_WITHIN_GROUP_ORDER_MASK        ID_ULONG(0x0200000000000000)
# define MTC_NODE_WITHIN_GROUP_ORDER_DESC        ID_ULONG(0x0200000000000000)
# define MTC_NODE_WITHIN_GROUP_ORDER_ASC         ID_ULONG(0x0000000000000000)

// BUG-41631 NULLS FIRST/LAST
# define MTC_NODE_WITHIN_GROUP_NULLS_MASK        ID_ULONG(0x0400000000000000)
# define MTC_NODE_WITHIN_GROUP_NULLS_FIRST       ID_ULONG(0x0400000000000000)
# define MTC_NODE_WITHIN_GROUP_NULLS_LAST        ID_ULONG(0x0000000000000000)

// BUG-43858 인자가 Undef 타입인 경우 aNode에 표시한다.
# define MTC_NODE_UNDEF_TYPE_MASK                ID_ULONG(0x0800000000000000)
# define MTC_NODE_UNDEF_TYPE_EXIST               ID_ULONG(0x0800000000000000)
# define MTC_NODE_UNDEF_TYPE_ABSENT              ID_ULONG(0x0000000000000000)

// PROJ-2528 Keep Aggregation
# define MTC_NODE_NULLS_OPT_EXIST_MASK           ID_ULONG(0x1000000000000000)
# define MTC_NODE_NULLS_OPT_EXIST_TRUE           ID_ULONG(0x1000000000000000)
# define MTC_NODE_NULLS_OPT_EXIST_FALSE          ID_ULONG(0x0000000000000000)

// TASK-7219 Non-shard DML
# define MTC_NODE_PUSHED_PRED_FORCE_MASK         ID_ULONG(0x2000000000000000)
# define MTC_NODE_PUSHED_PRED_FORCE_TRUE         ID_ULONG(0x2000000000000000)
# define MTC_NODE_PUSHED_PRED_FORCE_FALSE        ID_ULONG(0x0000000000000000)

// PROJ-1492
# define MTC_NODE_IS_DEFINED_TYPE( aNode )                     \
    (                                                          \
       (  ( ( (aNode)->lflag & MTC_NODE_BIND_MASK )            \
            == MTC_NODE_BIND_ABSENT )                          \
          ||                                                   \
          ( ( ( (aNode)->lflag & MTC_NODE_BIND_MASK )          \
              == MTC_NODE_BIND_EXIST )                         \
            &&                                                 \
            ( ( (aNode)->lflag & MTC_NODE_BIND_TYPE_MASK )     \
              == MTC_NODE_BIND_TYPE_TRUE ) )                   \
       )                                                       \
       ? ID_TRUE : ID_FALSE                                    \
    )

# define MTC_NODE_IS_DEFINED_VALUE( aNode )                    \
    (                                                          \
       ( ( (aNode)->lflag & MTC_NODE_BIND_MASK )               \
            == MTC_NODE_BIND_ABSENT )                          \
       ? ID_TRUE : ID_FALSE                                    \
    )

/* mtfExtractRange Von likeExtractRange              */
# define MTC_LIKE_KEY_SIZE      ( idlOS::align(ID_SIZEOF(UShort) + 32, 8) )
# define MTC_LIKE_KEY_PRECISION ( MTC_LIKE_KEY_SIZE - ID_SIZEOF(UShort) )

/* mtvModule.cost                                    */
# define MTV_COST_INFINITE                 ID_ULONG(0x4000000000000000)
# define MTV_COST_GROUP_PENALTY            ID_ULONG(0x0001000000000000)
# define MTV_COST_ERROR_PENALTY            ID_ULONG(0x0000000100000000)
# define MTV_COST_LOSS_PENALTY             ID_ULONG(0x0000000000010000)
# define MTV_COST_DEFAULT                  ID_ULONG(0x0000000000000001)

//----------------------------------------------------
// [NATVIE2NUMERIC_PENALTY]
//     [정수 연산자 정수] 형태의 각종 함수들은
//     Native Type을 우선적으로 사용할 수 있게 임시로
//     Setting하는 Penalty 값으로 mtvTable.cost에 사용함
//----------------------------------------------------
# define MTV_COST_NATIVE2NUMERIC_PENALTY ( MTV_COST_LOSS_PENALTY * 3 )


//--------------------------------------------------------------------
// PROJ-2002 Column Security
// [ MTV_COST_SMALL_LOSS_PENALTY ]
//     암호 데이타 타입에서 원본 데이타 타입으로 변환시 추가되는 비용
//
// ECHAR_TYPE, EVARCHAR_TYPE => CHAR_TYPE, VARCHAR_TYPE
//--------------------------------------------------------------------
# define MTV_COST_SMALL_LOSS_PENALTY      ( MTV_COST_DEFAULT * 2 )


// PROJ-1358
// mtcTemplate.rows 는
// 16개부터 시작하여 65536 개까지 동적으로 증가한다.
#define MTC_TUPLE_ROW_INIT_CNT                     (16)
#define MTC_TUPLE_ROW_MAX_CNT           (ID_USHORT_MAX)   // BUG-17154

//====================================================================
// mtcTuple.flag 정의
//====================================================================

/* mtcTuple.flag                                     */
# define MTC_TUPLE_TYPE_MAXIMUM                     (4)
/* BUG-44490 mtcTuple flag 확장 해야 합니다. */
/* 32bit flag 공간 모두 소모해 64bit로 증가 */
/* type을 UInt에서 ULong으로 변경 */
# define MTC_TUPLE_TYPE_MASK               ID_ULONG(0x0000000000000003)
# define MTC_TUPLE_TYPE_CONSTANT           ID_ULONG(0x0000000000000000)
# define MTC_TUPLE_TYPE_VARIABLE           ID_ULONG(0x0000000000000001)
# define MTC_TUPLE_TYPE_INTERMEDIATE       ID_ULONG(0x0000000000000002)
# define MTC_TUPLE_TYPE_TABLE              ID_ULONG(0x0000000000000003)

/* mtcTuple.flag                                     */
# define MTC_TUPLE_COLUMN_SET_MASK         ID_ULONG(0x0000000000000010)
# define MTC_TUPLE_COLUMN_SET_TRUE         ID_ULONG(0x0000000000000010)
# define MTC_TUPLE_COLUMN_SET_FALSE        ID_ULONG(0x0000000000000000)

/* mtcTuple.flag                                     */
# define MTC_TUPLE_COLUMN_ALLOCATE_MASK    ID_ULONG(0x0000000000000020)
# define MTC_TUPLE_COLUMN_ALLOCATE_TRUE    ID_ULONG(0x0000000000000020)
# define MTC_TUPLE_COLUMN_ALLOCATE_FALSE   ID_ULONG(0x0000000000000000)

/* mtcTuple.flag                                     */
# define MTC_TUPLE_COLUMN_COPY_MASK        ID_ULONG(0x0000000000000040)
# define MTC_TUPLE_COLUMN_COPY_TRUE        ID_ULONG(0x0000000000000040)
# define MTC_TUPLE_COLUMN_COPY_FALSE       ID_ULONG(0x0000000000000000)

/* mtcTuple.flag                                     */
# define MTC_TUPLE_EXECUTE_SET_MASK        ID_ULONG(0x0000000000000080)
# define MTC_TUPLE_EXECUTE_SET_TRUE        ID_ULONG(0x0000000000000080)
# define MTC_TUPLE_EXECUTE_SET_FALSE       ID_ULONG(0x0000000000000000)

/* mtcTuple.flag                                     */
# define MTC_TUPLE_EXECUTE_ALLOCATE_MASK   ID_ULONG(0x0000000000000100)
# define MTC_TUPLE_EXECUTE_ALLOCATE_TRUE   ID_ULONG(0x0000000000000100)
# define MTC_TUPLE_EXECUTE_ALLOCATE_FALSE  ID_ULONG(0x0000000000000000)

/* mtcTuple.flag                                     */
# define MTC_TUPLE_EXECUTE_COPY_MASK       ID_ULONG(0x0000000000000200)
# define MTC_TUPLE_EXECUTE_COPY_TRUE       ID_ULONG(0x0000000000000200)
# define MTC_TUPLE_EXECUTE_COPY_FALSE      ID_ULONG(0x0000000000000000)

/* mtcTuple.flag                                     */
# define MTC_TUPLE_ROW_SET_MASK            ID_ULONG(0x0000000000000400)
# define MTC_TUPLE_ROW_SET_TRUE            ID_ULONG(0x0000000000000400)
# define MTC_TUPLE_ROW_SET_FALSE           ID_ULONG(0x0000000000000000)

/* mtcTuple.flag                                     */
# define MTC_TUPLE_ROW_ALLOCATE_MASK       ID_ULONG(0x0000000000000800)
# define MTC_TUPLE_ROW_ALLOCATE_TRUE       ID_ULONG(0x0000000000000800)
# define MTC_TUPLE_ROW_ALLOCATE_FALSE      ID_ULONG(0x0000000000000000)

/* mtcTuple.flag                                     */
# define MTC_TUPLE_ROW_COPY_MASK           ID_ULONG(0x0000000000001000)
# define MTC_TUPLE_ROW_COPY_TRUE           ID_ULONG(0x0000000000001000)
# define MTC_TUPLE_ROW_COPY_FALSE          ID_ULONG(0x0000000000000000)

/* mtcTuple.flag                                     */
# define MTC_TUPLE_ROW_SKIP_MASK           ID_ULONG(0x0000000000002000)
# define MTC_TUPLE_ROW_SKIP_TRUE           ID_ULONG(0x0000000000002000)
# define MTC_TUPLE_ROW_SKIP_FALSE          ID_ULONG(0x0000000000000000)

/* PROJ-2160 row 중에 GEOMETRY 타입이 있는지 체크 */
/* mtcTuple.flag                                     */
# define MTC_TUPLE_ROW_GEOMETRY_MASK       ID_ULONG(0x0000000000004000)
# define MTC_TUPLE_ROW_GEOMETRY_TRUE       ID_ULONG(0x0000000000004000)
# define MTC_TUPLE_ROW_GEOMETRY_FALSE      ID_ULONG(0x0000000000000000)

/* BUG-43705 lateral view를 simple view merging을 하지않으면 결과가 다릅니다. */
/* mtcTuple.flag */
# define MTC_TUPLE_LATERAL_VIEW_REF_MASK   ID_ULONG(0x0000000000008000)
# define MTC_TUPLE_LATERAL_VIEW_REF_TRUE   ID_ULONG(0x0000000000008000)
# define MTC_TUPLE_LATERAL_VIEW_REF_FALSE  ID_ULONG(0x0000000000000000)

/* mtcTuple.flag                                     */
// Tuple의 저장 매체에 대한 정보
# define MTC_TUPLE_STORAGE_MASK            ID_ULONG(0x0000000000010000)
# define MTC_TUPLE_STORAGE_MEMORY          ID_ULONG(0x0000000000000000)
# define MTC_TUPLE_STORAGE_DISK            ID_ULONG(0x0000000000010000)

/* mtcTuple.flag                                     */
// 해당 Tuple이 VIEW를 위한 Tuple인지를 설정
# define MTC_TUPLE_VIEW_MASK               ID_ULONG(0x0000000000020000)
# define MTC_TUPLE_VIEW_FALSE              ID_ULONG(0x0000000000000000)
# define MTC_TUPLE_VIEW_TRUE               ID_ULONG(0x0000000000020000)

/* mtcTuple.flag */
// 해당 Tuple이 Plan을 위해 생성된 Tuple인지를 설정
# define MTC_TUPLE_PLAN_MASK               ID_ULONG(0x0000000000040000)
# define MTC_TUPLE_PLAN_FALSE              ID_ULONG(0x0000000000000000)
# define MTC_TUPLE_PLAN_TRUE               ID_ULONG(0x0000000000040000)

/* mtcTuple.flag */
// 해당 Tuple이 Materialization Plan을 위해 생성된 Tuple
# define MTC_TUPLE_PLAN_MTR_MASK           ID_ULONG(0x0000000000080000)
# define MTC_TUPLE_PLAN_MTR_FALSE          ID_ULONG(0x0000000000000000)
# define MTC_TUPLE_PLAN_MTR_TRUE           ID_ULONG(0x0000000000080000)

/* mtcTuple.flag */
// PROJ-1502 PARTITIONED DISK TABLE
// 해당 Tuple이 Partitioned Table을 위해 생성된 Tuple
# define MTC_TUPLE_PARTITIONED_TABLE_MASK  ID_ULONG(0x0000000000100000)
# define MTC_TUPLE_PARTITIONED_TABLE_FALSE ID_ULONG(0x0000000000000000)
# define MTC_TUPLE_PARTITIONED_TABLE_TRUE  ID_ULONG(0x0000000000100000)

/* mtcTuple.flag */
// PROJ-1502 PARTITIONED DISK TABLE
// 해당 Tuple이 Partition을 위해 생성된 Tuple
# define MTC_TUPLE_PARTITION_MASK          ID_ULONG(0x0000000000200000)
# define MTC_TUPLE_PARTITION_FALSE         ID_ULONG(0x0000000000000000)
# define MTC_TUPLE_PARTITION_TRUE          ID_ULONG(0x0000000000200000)

/* mtcTuple.flag */
/* BUG-36468 Tuple의 저장매체 위치(Location)에 대한 정보 */
# define MTC_TUPLE_STORAGE_LOCATION_MASK   ID_ULONG(0x0000000000400000)
# define MTC_TUPLE_STORAGE_LOCATION_LOCAL  ID_ULONG(0x0000000000000000)
# define MTC_TUPLE_STORAGE_LOCATION_REMOTE ID_ULONG(0x0000000000400000)

/* mtcTuple.flag */
/* PROJ-2464 hybrid partitioned table 지원 */
// 해당 Tuple이 Hybrid를 위해 생성된 Tuple
# define MTC_TUPLE_HYBRID_PARTITIONED_TABLE_MASK    ID_ULONG(0x0000000000800000)
# define MTC_TUPLE_HYBRID_PARTITIONED_TABLE_FALSE   ID_ULONG(0x0000000000000000)
# define MTC_TUPLE_HYBRID_PARTITIONED_TABLE_TRUE    ID_ULONG(0x0000000000800000)

/* mtcTuple.flag */
// PROJ-1473
// tuple 에 대한 처리방법 지정.
// (1) tuple set 방식으로 처리
// (2) 레코드저장방식으로 처리
# define MTC_TUPLE_MATERIALIZE_MASK        ID_ULONG(0x0000000001000000)
# define MTC_TUPLE_MATERIALIZE_RID         ID_ULONG(0x0000000000000000)
# define MTC_TUPLE_MATERIALIZE_VALUE       ID_ULONG(0x0000000001000000)

# define MTC_TUPLE_BINARY_COLUMN_MASK      ID_ULONG(0x0000000002000000)
# define MTC_TUPLE_BINARY_COLUMN_ABSENT    ID_ULONG(0x0000000000000000)
# define MTC_TUPLE_BINARY_COLUMN_EXIST     ID_ULONG(0x0000000002000000)

# define MTC_TUPLE_VIEW_PUSH_PROJ_MASK     ID_ULONG(0x0000000004000000)
# define MTC_TUPLE_VIEW_PUSH_PROJ_FALSE    ID_ULONG(0x0000000000000000)
# define MTC_TUPLE_VIEW_PUSH_PROJ_TRUE     ID_ULONG(0x0000000004000000)

// PROJ-1705
/* mtcTuple.flag */
# define MTC_TUPLE_VSCN_MASK               ID_ULONG(0x0000000008000000)
# define MTC_TUPLE_VSCN_FALSE              ID_ULONG(0x0000000000000000)
# define MTC_TUPLE_VSCN_TRUE               ID_ULONG(0x0000000008000000)

/*
 * PROJ-1789 PROWID
 * Target list 또는 where 절에 RID 가 있는지
 */
#define MTC_TUPLE_TARGET_RID_MASK          ID_ULONG(0x0000000010000000)
#define MTC_TUPLE_TARGET_RID_EXIST         ID_ULONG(0x0000000010000000)
#define MTC_TUPLE_TARGET_RID_ABSENT        ID_ULONG(0x0000000000000000)

// PROJ-2204 Join Update Delete
/* mtcTuple.flag */
#define MTC_TUPLE_KEY_PRESERVED_MASK       ID_ULONG(0x0000000020000000)
#define MTC_TUPLE_KEY_PRESERVED_TRUE       ID_ULONG(0x0000000020000000)
#define MTC_TUPLE_KEY_PRESERVED_FALSE      ID_ULONG(0x0000000000000000)

/* BUG-39399 remove search key preserved table */
/* mtcTuple.flag */
#define MTC_TUPLE_TARGET_UPDATE_DELETE_MASK  ID_ULONG(0x0000000040000000)
#define MTC_TUPLE_TARGET_UPDATE_DELETE_TRUE  ID_ULONG(0x0000000040000000)
#define MTC_TUPLE_TARGET_UPDATE_DELETE_FALSE ID_ULONG(0x0000000000000000)

/* BUG-44382 clone tuple 성능개선 */
/* mtcTuple.flag */
# define MTC_TUPLE_ROW_MEMSET_MASK         ID_ULONG(0x0000000080000000)
# define MTC_TUPLE_ROW_MEMSET_TRUE         ID_ULONG(0x0000000080000000)
# define MTC_TUPLE_ROW_MEMSET_FALSE        ID_ULONG(0x0000000000000000)

/* PROJ-2719 Multi table update/delete */
# define MTC_TUPLE_VIEW_PADNULL_MASK       ID_ULONG(0x0000000100000000)
# define MTC_TUPLE_VIEW_PADNULL_TRUE       ID_ULONG(0x0000000100000000)
# define MTC_TUPLE_VIEW_PADNULL_FALSE      ID_ULONG(0x0000000000000000)

# define MTC_TUPLE_COLUMN_ID_MAXIMUM            (1024)
/* to_char할 때나 date -> char 변환시 precision      */
# define MTC_TO_CHAR_MAX_PRECISION                 (64)

/* PROJ-2446 ONE SOURCE XDB REPLACTION
 * XSN_TO_LSN 함수 SN을 LSN으로 변환할 때 max precision */
# define MTC_XSN_TO_LSN_MAX_PRECISION              (40)


/* mtdModule.compare                                 */
/* mtdModule.partialKey                              */
/* aFlag VON mtk::setPartialKey                      */
# define MTD_COMPARE_ASCENDING                      (0)
# define MTD_COMPARE_DESCENDING                     (1)

/* PROJ-2180
   MTD_COMPARE_FIXED_MTDVAL_FIXED_MTDVAL add */
# define MTD_COMPARE_FIXED_MTDVAL_FIXED_MTDVAL       (0)
# define MTD_COMPARE_MTDVAL_MTDVAL                   (1)
# define MTD_COMPARE_STOREDVAL_MTDVAL                (2)
# define MTD_COMPARE_STOREDVAL_STOREDVAL             (3)
# define MTD_COMPARE_INDEX_KEY_FIXED_MTDVAL          (4)
# define MTD_COMPARE_INDEX_KEY_MTDVAL                (5)
# define MTD_COMPARE_FUNC_MAX_CNT                    (6)

//====================================================================
// mtdModule.flag 정의
//====================================================================

/* mtdModule.flag                                    */
# define MTD_GROUP_MASK                    (0x0000000F)
# define MTD_GROUP_MAXIMUM                 (0x00000005)
# define MTD_GROUP_MISC                    (0x00000000)
# define MTD_GROUP_TEXT                    (0x00000001)
# define MTD_GROUP_NUMBER                  (0x00000002)
# define MTD_GROUP_DATE                    (0x00000003)
# define MTD_GROUP_INTERVAL                (0x00000004)

/* mtdModule.flag                                    */
# define MTD_CANON_MASK                    (0x00000030)
# define MTD_CANON_NEEDLESS                (0x00000000)
# define MTD_CANON_NEED                    (0x00000010)
# define MTD_CANON_NEED_WITH_ALLOCATION    (0x00000020)

/* mtdModule.flag                                    */
# define MTD_CREATE_MASK                   (0x00000040)
# define MTD_CREATE_ENABLE                 (0x00000040)
# define MTD_CREATE_DISABLE                (0x00000000)

/* mtdModule.flag                                    */
// PROJ-1904 Extend UDT
# define MTD_PSM_TYPE_MASK                 (0x00000080)
# define MTD_PSM_TYPE_ENABLE               (0x00000080)
# define MTD_PSM_TYPE_DISABLE              (0x00000000)

/* mtdModule.flag                                    */
# define MTD_COLUMN_TYPE_MASK              (0x00000300)
# define MTD_COLUMN_TYPE_FIXED             (0x00000100)
# define MTD_COLUMN_TYPE_VARIABLE          (0x00000000)
# define MTD_COLUMN_TYPE_LOB               (0x00000200)

/* mtdModule.flag                                    */
// Selectivity를 추출할 수 있는 Data Type인지의 여부
# define MTD_SELECTIVITY_MASK              (0x00000400)
# define MTD_SELECTIVITY_ENABLE            (0x00000400)
# define MTD_SELECTIVITY_DISABLE           (0x00000000)

/* mtdModule.flag                                    */
// byte 타입에 대해
// Greatest, Least, Nvl, Nvl2, Case2, Decode등의
// 함수가 Precision, Scale이 동일한 경우에만
// 동작하도록 하기 위한 flag
// byte만 유일하게 length 필드가 없다.
# define MTD_NON_LENGTH_MASK               (0x00000800)
# define MTD_NON_LENGTH_TYPE               (0x00000800)
# define MTD_LENGTH_TYPE                   (0x00000000)

// PROJ-1364
/* mtdModule.flag                                    */
// 숫자형계열의 대표 data type에 대한 flag
//-----------------------------------------------------------------------
//            |                                                | 대표type
// ----------------------------------------------------------------------
// 문자형계열 | CHAR, VARCHAR                                  | VARCHAR
// ----------------------------------------------------------------------
// 숫자형계열 | Native | 정수형 | BIGINT, INTEGER, SMALLINT    | BIGINT
//            |        |-------------------------------------------------
//            |        | 실수형 | DOUBLE, REAL                 | DOUBLE
//            -----------------------------------------------------------
//            | Non-   | 고정소수점형 | NUMERIC, DECIMAL,      |
//            | Native |              | NUMBER(p), NUMBER(p,s) | NUMERIC
//            |        |----------------------------------------
//            |        | 부정소수점형 | FLOAT, NUMBER          |
// ----------------------------------------------------------------------
// "주의 "
// 이 값은 mtd::comparisonNumberType matrix의 배열인자를 구하기 위해
// MTD_NUMBER_GROUP_TYPE_SHIFT 값을 이용해서 right shift를 수행하게 된다.
// 따라서, mtdModule.flag에서 이 flag의 위치가 변경되면,
// MTD_NUMBER_GROUP_TYPE_SHIFT도 그에 맞게 변경되어야 함.
# define MTD_NUMBER_GROUP_TYPE_MASK        (0x00003000)
# define MTD_NUMBER_GROUP_TYPE_NONE        (0x00000000)
# define MTD_NUMBER_GROUP_TYPE_BIGINT      (0x00001000)
# define MTD_NUMBER_GROUP_TYPE_DOUBLE      (0x00002000)
# define MTD_NUMBER_GROUP_TYPE_NUMERIC     (0x00003000)

// PROJ-1362
// ODBC API, SQLGetTypeInfo 지원을 위한 mtdModule 확장
// 이전에는 타입정보가 client에 하드코딩되어 있던것을
// 서버에서 조회하도록 변경함
// mtdModule.flag
# define MTD_CREATE_PARAM_MASK             (0x00030000)
# define MTD_CREATE_PARAM_NONE             (0x00000000)
# define MTD_CREATE_PARAM_PRECISION        (0x00010000)
# define MTD_CREATE_PARAM_PRECISIONSCALE   (0x00020000)

# define MTD_CASE_SENS_MASK                (0x00040000)
# define MTD_CASE_SENS_FALSE               (0x00000000)
# define MTD_CASE_SENS_TRUE                (0x00040000)

# define MTD_UNSIGNED_ATTR_MASK            (0x00080000)
# define MTD_UNSIGNED_ATTR_FALSE           (0x00000000)
# define MTD_UNSIGNED_ATTR_TRUE            (0x00080000)

# define MTD_SEARCHABLE_MASK               (0x00300000)
# define MTD_SEARCHABLE_PRED_NONE          (0x00000000)
# define MTD_SEARCHABLE_PRED_CHAR          (0x00100000)
# define MTD_SEARCHABLE_PRED_BASIC         (0x00200000)
# define MTD_SEARCHABLE_SEARCHABLE         (0x00300000)

# define MTD_SQL_DATETIME_SUB_MASK         (0x00400000)
# define MTD_SQL_DATETIME_SUB_FALSE        (0x00000000)
# define MTD_SQL_DATETIME_SUB_TRUE         (0x00400000)

# define MTD_NUM_PREC_RADIX_MASK           (0x00800000)
# define MTD_NUM_PREC_RADIX_FALSE          (0x00000000)
# define MTD_NUM_PREC_RADIX_TRUE           (0x00800000)

# define MTD_LITERAL_MASK                  (0x01000000)
# define MTD_LITERAL_FALSE                 (0x00000000)
# define MTD_LITERAL_TRUE                  (0x01000000)

// PROJ-1705
// define된 mtdDataType이 
// length 정보를 가지는 데이타타입인지의 여부를 나타낸다.
# define MTD_VARIABLE_LENGTH_TYPE_MASK     (0x02000000)
# define MTD_VARIABLE_LENGTH_TYPE_FALSE    (0x00000000)
# define MTD_VARIABLE_LENGTH_TYPE_TRUE     (0x02000000)

// PROJ-1705
// 디스크테이블인 경우, 
// SM에서 컬럼을 여러 row piece에
// 나누어 저장해도 되는지 여부를 나타낸다. 
# define MTD_DATA_STORE_DIVISIBLE_MASK     (0x04000000)
# define MTD_DATA_STORE_DIVISIBLE_FALSE    (0x00000000)
# define MTD_DATA_STORE_DIVISIBLE_TRUE     (0x04000000)

// PROJ-1705
// 디스크테이블인 경우,
// SM에 mtdValue 형태로 저장되는지의 여부
// nibble, bit, varbit 인 경우에 해당됨.
// 이 세가지 타입은 저장되는 length 정보로 
// 쿼리수행시의 실제 length 정보를 정확하게 구할수 없어
// mtdValueType으로 저장한다. 
# define MTD_DATA_STORE_MTDVALUE_MASK      (0x08000000)
# define MTD_DATA_STORE_MTDVALUE_FALSE     (0x00000000)
# define MTD_DATA_STORE_MTDVALUE_TRUE      (0x08000000)

// PROJ-2002 Column Security
// echar, evarchar 같은 보안 타입 정보
// mtdModule.flag
# define MTD_ENCRYPT_TYPE_MASK             (0x10000000)
# define MTD_ENCRYPT_TYPE_FALSE            (0x00000000)
# define MTD_ENCRYPT_TYPE_TRUE             (0x10000000)

// PROJ-2163 Bind revise
// 내부에서만 쓰이는 타입
// Typed literal 이나 CAST 연산 등 외부에서 
// mtd::moduleByName 을 통해 타입을 찾을 경우
// 에러를 발생한다.
# define MTD_INTERNAL_TYPE_MASK            (0x20000000)
# define MTD_INTERNAL_TYPE_FALSE           (0x00000000)
# define MTD_INTERNAL_TYPE_TRUE            (0x20000000)

// PROJ-1364
// 숫자형 계열간 비교시,
// mtd::comparisonNumberType  matrix에 의해
// 비교기준 data type을 얻게 되는데,
// 이 배열인자를 얻기 위해
// MTD_NUMBER_GROUP_TYPE_XXX을 아래 값만큼 shift
// "주의"
// MTD_NUMBER_GROUP_TYPE_MASK의 위치가 변경되면,
// 이 define 상수도 그에 맞게 변경되어야 함.
# define MTD_NUMBER_GROUP_TYPE_SHIFT       (12)

/* aFlag VON mtdFunctions                            */
# define MTD_OFFSET_MASK                   (0x00000001)
# define MTD_OFFSET_USELESS                (0x00000001)
# define MTD_OFFSET_USE                    (0x00000000)

/* PROJ-2433 Direct Key Index
 * 비교할 direct key가 partial direct key인지 여부 
 * aFlag VON mtdFunctions */
# define MTD_PARTIAL_KEY_MASK              (0x00000002)
# define MTD_PARTIAL_KEY_ON                (0x00000002)
# define MTD_PARTIAL_KEY_OFF               (0x00000000)

/* mtcCallBack.flag                                  */
// Child Node에 대한 Estimation 여부를 표현함
// ENABLE일 경우, Child에 대한 Estimate를 수행하며,
// DISABLE일 경우, 자신에 대한 Estimate만을 수행한다.
# define MTC_ESTIMATE_ARGUMENTS_MASK       (0x00000001)
# define MTC_ESTIMATE_ARGUMENTS_DISABLE    (0x00000001)
# define MTC_ESTIMATE_ARGUMENTS_ENABLE     (0x00000000)

/* mtcCallBack.flag                                  */
// PROJ-2527 WITHIN GROUP AGGR
// initialize시의 여부
# define MTC_ESTIMATE_INITIALIZE_MASK      (0x00000002)
# define MTC_ESTIMATE_INITIALIZE_TRUE      (0x00000002)
# define MTC_ESTIMATE_INITIALIZE_FALSE     (0x00000000)

// Selectivity 계산 시 정확한 계산이 불가능할 경우에
// 선택되는 값
# define MTD_DEFAULT_SELECTIVITY           (   1.0/3.0)

// PROJ-1484
// Selectivity 계산 시 10진수로 변환 가능한 문자열의 최대 길이
# define MTD_CHAR_DIGIT_MAX                (15)

//====================================================================
// mtkRangeCallBack.info 정의
//====================================================================

// Ascending/Descending 정보
#define MTK_COMPARE_DIRECTION_MASK  (0x00000001)
#define MTK_COMPARE_DIRECTION_ASC   (0x00000000)
#define MTK_COMPARE_DIRECTION_DESC  (0x00000001)

// 동일 계열내 서로 다른 data type간의 비교 함수 여부 
#define MTK_COMPARE_SAMEGROUP_MASK  (0x00000002)
#define MTK_COMPARE_SAMEGROUP_FALSE (0x00000000)
#define MTK_COMPARE_SAMEGROUP_TRUE  (0x00000002)

//====================================================================
// mtl 관련 정의
//====================================================================

// fix BUG-13830
// 각 language의 한 문자의 최대 precision
# define MTL_UTF8_PRECISION      3
# define MTL_UTF16_PRECISION     2  // PROJ-1579 NCHAR
# define MTL_ASCII_PRECISION     1
# define MTL_KSC5601_PRECISION   2
# define MTL_MS949_PRECISION     2
# define MTL_SHIFTJIS_PRECISION  2
# define MTL_MS932_PRECISION     2 /* PROJ-2590 support CP932 character set */
# define MTL_GB231280_PRECISION  2
/* PROJ-2414 [기능성] GBK, CP936 character set 추가 */
# define MTL_MS936_PRECISION     2
# define MTL_BIG5_PRECISION      2
# define MTL_EUCJP_PRECISION     3
# define MTL_MAX_PRECISION       3

// PROJ-1579 NCHAR
# define MTL_NCHAR_PRECISION( aMtlModule )              \
    (                                                   \
        ( (aMtlModule)->id == MTL_UTF8_ID )             \
        ? MTL_UTF8_PRECISION : MTL_UTF16_PRECISION      \
    )

// PROJ-1361 : language type에 따른 id
// mtlModule.id
# define MTL_DEFAULT          0
# define MTL_UTF8_ID      10000
# define MTL_UTF16_ID     11000     // PROJ-1579 NCHAR
# define MTL_ASCII_ID     20000
# define MTL_KSC5601_ID   30000
# define MTL_MS949_ID     31000
# define MTL_SHIFTJIS_ID  40000
# define MTL_MS932_ID     42000 /* PROJ-2590 support CP932 character set */
# define MTL_EUCJP_ID     41000
# define MTL_GB231280_ID  50000
# define MTL_BIG5_ID      51000
/* PROJ-2414 [기능성] GBK, CP936 character set 추가 */
# define MTL_MS936_ID     52000
//PROJ-2002 Column Security
# define MTC_POLICY_NAME_SIZE  (16-1)

/* BUG-39237 add sys_guid() function */
#define MTF_SYS_GUID_LENGTH   ((UShort) (32))
#define MTF_SYS_HOSTID_LENGTH ((UShort) ( 8))
#define MTF_SYS_HOSTID_FORMAT "%08"

/* BUG-25198 UNIX */
#define MTF_BASE_UNIX_DATE  ID_LONG(62135769600)
#define MTF_MAX_UNIX_DATE   ID_LONG(64060588799)

typedef struct mtcId            mtcId;
typedef struct mtcCallBack      mtcCallBack;
typedef struct mtcCallBackInfo  mtcCallBackInfo;
typedef struct mtcColumn        mtcColumn;
typedef struct mtcExecute       mtcExecute;
typedef struct mtcName          mtcName;
typedef struct mtcNode          mtcNode;
typedef struct mtcStack         mtcStack;
typedef struct mtcTuple         mtcTuple;
typedef struct mtcTemplate      mtcTemplate;

typedef struct mtkRangeCallBack mtkRangeCallBack;
typedef struct mtvConvert       mtvConvert;
typedef struct mtvTable         mtvTable;

typedef struct mtfSubModule     mtfSubModule;

typedef struct mtdModule        mtdModule;
typedef struct mtfModule        mtfModule;
typedef struct mtlModule        mtlModule;
typedef struct mtvModule        mtvModule;

typedef struct mtkRangeInfo     mtkRangeInfo;

typedef struct mtdValueInfo     mtdValueInfo;

// PROJ-2002 Column Security
typedef struct mtcEncryptInfo   mtcEncryptInfo;

/* PROJ-2632 */
typedef struct mtxModule        mtxModule;
typedef union  mtxEntry         mtxEntry;

typedef enum
{
    NC_VALID = 0,         /* 정상적인 1글자 */
    NC_INVALID,           /* firstbyte가 코드에 안맞는 1글자 */
    NC_MB_INVALID,        /* secondbyte이후가 코드에 안맞는 1글자 */
    NC_MB_INCOMPLETED     /* 멀티바이트 글자인데 byte가 모자른 0글자 */
} mtlNCRet;

#define MTC_CURSOR_PTR  void*

typedef IDE_RC (*mtcInitSubqueryFunc)( mtcNode*     aNode,
                                       mtcTemplate* aTemplate );

typedef IDE_RC (*mtcFetchSubqueryFunc)( mtcNode*     aNode,
                                        mtcTemplate* aTemplate,
                                        idBool*      aRowExist );

typedef IDE_RC (*mtcFiniSubqueryFunc)( mtcNode*     aNode,
                                       mtcTemplate* aTemplate );

/* PROJ-2448 Subquery caching */
typedef IDE_RC (*mtcSetCalcSubqueryFunc)( mtcNode     * aNode,
                                          mtcTemplate * aTemplate );

//PROJ-1583 large geometry
typedef UInt   (*mtcGetSTObjBufSizeFunc)( mtcCallBack * aCallBack );

typedef IDE_RC (*mtcGetOpenedCursorFunc)( mtcTemplate     * aTemplate,
                                          UShort            aTableID,
                                          MTC_CURSOR_PTR  * aCursor,
                                          UShort          * aOrgTableID,
                                          idBool          * aFound );

// BUG-40427
typedef IDE_RC (*mtcAddOpenedLobCursorFunc)( mtcTemplate   * aTemplate,
                                             smLobLocator    aLocator );

/* PROJ-1530 PSM/Trigger에서 LOB 데이타 타입 지원 */
typedef idBool (*mtcIsBaseTable)( mtcTemplate * aTemplate,
                                  UShort        aTable );

typedef IDE_RC (*mtcCloseLobLocator)( smLobLocator   aLocator );

typedef IDE_RC (*mtdInitializeFunc)( UInt sNo );

typedef IDE_RC (*mtdEstimateFunc)(  UInt * aColumnSize,
                                    UInt * aArguments,
                                    SInt * aPrecision,
                                    SInt * aScale );

typedef IDE_RC (*mtdChangeFunc)( mtcColumn* aColumn,
                                 UInt       aFlag );

typedef IDE_RC (*mtdValueFunc)( mtcTemplate* aTemplate,  // fix BUG-15947
                                mtcColumn*   aColumn,
                                void*        aValue,
                                UInt*        aValueOffset,
                                UInt         aValueSize,
                                const void*  aToken,
                                UInt         aTokenLength,
                                IDE_RC*      aResult );

typedef UInt (*mtdActualSizeFunc)( const mtcColumn* aColumn,
                                   const void*      aRow );

typedef IDE_RC (*mtdGetPrecisionFunc)( const mtcColumn * aColumn,
                                       const void      * aRow,
                                       SInt            * aPrecision,
                                       SInt            * aScale );

typedef void (*mtdNullFunc)( const mtcColumn* aColumn,
                             void*            aRow );

typedef UInt (*mtdHashFunc)( UInt             aHash,
                             const mtcColumn* aColumn,
                             const void*      aRow );

typedef idBool (*mtdIsNullFunc)( const mtcColumn* aColumn,
                                 const void*      aRow );

typedef IDE_RC (*mtdIsTrueFunc)( idBool*          aResult,
                                 const mtcColumn* aColumn,
                                 const void*      aRow );

typedef SInt (*mtdCompareFunc)( mtdValueInfo * aValueInfo1,
                                mtdValueInfo * aValueInfo2 );

typedef UChar (*mtsRelationFunc)( const mtcColumn* aColumn1,
                                  const void*      aRow1,
                                  UInt             aFlag1,
                                  const mtcColumn* aColumn2,
                                  const void*      aRow2,
                                  UInt             aFlag2,
                                  const void*      aInfo );

typedef IDE_RC (*mtdCanonizeFunc)( const mtcColumn  *  aCanon,
                                   void             ** aCanonized,
                                   mtcEncryptInfo   *  aCanonInfo,
                                   const mtcColumn  *  aColumn,
                                   void             *  aValue,
                                   mtcEncryptInfo   *  aColumnInfo,
                                   mtcTemplate      *  aTemplate );

typedef void (*mtdEndianFunc)( void* aValue );

typedef IDE_RC (*mtdValidateFunc)( mtcColumn * aColumn,
                                   void      * aValue,
                                   UInt        aValueSize);

typedef IDE_RC (*mtdEncodeFunc) ( mtcColumn  * aColumn,
                                  void       * aValue,
                                  UInt         aValueSize,
                                  UChar      * aCompileFmt,
                                  UInt         aCompileFmtLen,
                                  UChar      * aText,
                                  UInt       * aTextLen,
                                  IDE_RC     * aRet );

typedef IDE_RC (*mtdDecodeFunc) ( mtcTemplate* aTemplate,  // fix BUG-15947
                                  mtcColumn  * aColumn,
                                  void       * aValue,
                                  UInt       * aValueSize,
                                  UChar      * aCompileFmt,
                                  UInt         aCompileFmtLen,
                                  UChar      * aText,
                                  UInt         aTextLen,
                                  IDE_RC     * aRet );

typedef IDE_RC (*mtdCompileFmtFunc)	( mtcColumn  * aColumn,
                                      UChar      * aCompiledFmt,
                                      UInt       * aCompiledFmtLen,
                                      UChar      * aFormatString,
                                      UInt         aFormatStringLength,
                                      IDE_RC     * aRet );

typedef IDE_RC (*mtdValueFromOracleFunc)( mtcColumn*  aColumn,
                                          void*       aValue,
                                          UInt*       aValueOffset,
                                          UInt        aValueSize,
                                          const void* aOracleValue,
                                          SInt        aOracleLength,
                                          IDE_RC*     aResult );

typedef IDE_RC (*mtdMakeColumnInfo) ( void * aStmt,
                                      void * aTableInfo,
                                      UInt   aIdx );

// BUG-28934
// key range를 AND merge하는 함수
typedef IDE_RC (*mtcMergeAndRange)( smiRange* aMerged,
                                    smiRange* aRange1,
                                    smiRange* aRange2 );

// key range를 OR merge하는 함수
typedef IDE_RC (*mtcMergeOrRangeList)( smiRange  * aMerged,
                                       smiRange ** aRangeList,
                                       SInt        aRangeCount );

//-----------------------------------------------------------------------
// Data Type의 Selectivity 추출 함수
// Selectivity 계산식
//     Selectivity = (aValueMax - aValueMin) / (aColumnMax - aColumnMin)
//-----------------------------------------------------------------------

typedef SDouble (*mtdSelectivityFunc) ( void     * aColumnMax,  // Column의 MAX값
                                        void     * aColumnMin,  // Column의 MIN값
                                        void     * aValueMax,   // MAX Value 값
                                        void     * aValueMin,   // MIN Value 값
                                        SDouble    aBoundFactor,
                                        SDouble    aTotalRecordCnt );

typedef IDE_RC (*mtfInitializeFunc)( void );

typedef IDE_RC (*mtfFinalizeFunc)( void );

/***********************************************************************
 * PROJ-1705
 * 디스크테이블컬럼의 데이타를
 * qp 레코드처리영역의 해당 컬럼위치에 복사
 *
 *     aColumnSize : 테이블생성시 정의된 column size
 *     aDestValue : 컬럼의 value가 복사되어야 할 메모리 주소
 *                  qp : mtcTuple.row에 할당된
 *                       디스크레코드를 위한 메모리중 해당 컬럼의 위치
 *                       mtcTuple.row + column.offset    
 *     aDestValueOffset: 컬럼의 value가 복사되어야 할 메모리주소로부터의 offset
 *                       aDestValue의 aOffset 
 *     aLength    : sm에 저장된 레코드의 value로 복사될 컬럼 value의 length
 *     aValue     : sm에 저장된 레코드의 value로 복사될 컬럼 value 
 **********************************************************************/

typedef IDE_RC (*mtdStoredValue2MtdValueFunc) ( UInt              aColumnSize,
                                                void            * aDestValue,
                                                UInt              aDestValueOffset,
                                                UInt              aLength,
                                                const void      * aValue );

/***********************************************************************
 * PROJ-1705
 * 각 데이타타입의 null Value의 크기 반환
 * replication에서 사용.
 **********************************************************************/

typedef UInt (*mtdNullValueSizeFunc) ();

/***********************************************************************
 * PROJ-1705
 * length를 가지는 데이타타입의 length 정보를 저장하는 변수의 크기 반환
 * 예) mtdCharType( UShort length; UChar value[1] ) 에서
 *      length타입인 UShort의 크기를 반환
 *  integer와 같은 고정길이 데이타타입은 0 반환
 **********************************************************************/
typedef UInt (*mtdHeaderSizeFunc) ();


/***********************************************************************
* PROJ-2399 row tmaplate
* sm에 저장되는 데이터의 크기를 반환한다.( mtheader 크기를 뺀다.)
* varchar와 같은 variable 타입의 데이터 타입은 0을 반환
 **********************************************************************/
typedef UInt (*mtdStoreSizeFunc) ( const smiColumn * aColumn );

//----------------------------------------------------------------
// PROJ-1358
// Estimation 과정에서 SEQUENCE, SYSDATE, LEVEL, Pre-Processing,
// Conversion 등을 위하여 Internal Tuple을 할당받을 수 있다.
// 이 때 Internal Tuple Set이 확장되면, 입력 인자로 사용되는
// mtcTuple * 는 유효하지 않게 된다.
// 즉, Estimatation의 인자는 Tuple Set의 확장을 고려하여
// mtcTemplate * 를 인자로 받아 mtcTemplate->rows 를 이용하여야 한다.
// 참고) mtfCalculateFunc, mtfEstimateRangeFunc 등은
//       Internal Tuple Set의 확장으로 인한 문제는 발생하지 않으나,
//       mtcTuple* 를 인자로 취하는 함수를 모두 제거하여
//       문제를 사전에 방지한다.
//----------------------------------------------------------------

//----------------------------------------------------------------
// PROJ-1358
// 아주 불가피한 경우가 아니라면, mtcTuple * 는 함수내의
// 지역변수로도 사용하지 말아야 한다.
//----------------------------------------------------------------

typedef IDE_RC (*mtfEstimateFunc)( mtcNode*     aNode,
                                   mtcTemplate* aTemplate,
                                   mtcStack*    aStack,
                                   SInt         aRemain,
                                   mtcCallBack* aCallBack );

//----------------------------------------------------------------
// PROJ-1358
// 참고) mtfCalculateFunc, mtfEstimateRangeFunc 등은
//       Internal Tuple Set의 확장으로 인한 문제는 발생하지 않으나,
//       mtcTuple* 를 인자로 취하는 함수를 모두 제거하여
//       문제를 사전에 방지한다.
//----------------------------------------------------------------

typedef IDE_RC (*mtfCalculateFunc)( mtcNode*     aNode,
                                    mtcStack*    aStack,
                                    SInt         aRemain,
                                    void*        aInfo,
                                    mtcTemplate* aTemplate );

typedef IDE_RC (*mtfEstimateRangeFunc)( mtcNode*      aNode,
                                        mtcTemplate* aTemplate,
                                        UInt         aArgument,
                                        UInt*        aSize );

/* PROJ-2632 */
typedef IDE_RC (*mtxSerialExecuteFunc)( mtxEntry ** aEntry );

typedef mtxSerialExecuteFunc (*mtxGetSerialExecute)( UInt aMtd1Type,
                                                     UInt aMtd2Type );

//----------------------------------------------------------------
// PROJ-1358
// 참고) mtfCalculateFunc, mtfEstimateRangeFunc 등은
//       Internal Tuple Set의 확장으로 인한 문제는 발생하지 않으나,
//       mtcTuple* 를 인자로 취하는 함수를 모두 제거하여
//       문제를 사전에 방지한다.
//----------------------------------------------------------------

typedef IDE_RC (*mtfExtractRangeFunc)( mtcNode*       aNode,
                                       mtcTemplate*   aTemplate,
                                       mtkRangeInfo * aInfo,
                                       smiRange*      aRange );

typedef IDE_RC (*mtfInitConversionNodeFunc)( mtcNode** aConversionNode,
                                             mtcNode*  aNode,
                                             void*     aInfo );

typedef void (*mtfSetIndirectNodeFunc)( mtcNode* aNode,
                                        mtcNode* aConversionNode,
                                        void*    aInfo );

typedef IDE_RC (*mtfAllocFunc)( void* aInfo,
                                UInt  aSize,
                                void**);

typedef const void* (*mtcGetColumnFunc)( const void*      aRow,
                                         const smiColumn* aColumn,
                                         UInt*            aLength );

typedef const void* (*mtcGetCompressionColumnFunc)( const void*      aRow,
                                                    const smiColumn* aColumn,
                                                    idBool           aUseColumnOffset,
                                                    UInt*            aLength );

typedef IDE_RC (*mtcOpenLobCursorWithRow)( MTC_CURSOR_PTR    aTableCursor,
                                           void*             aRow,
                                           const smiColumn*  aLobColumn,
                                           UInt              aInfo,
                                           smiLobCursorMode  aMode,
                                           smLobLocator*     aLobLocator );

typedef IDE_RC (*mtcOpenLobCursorWithGRID)(MTC_CURSOR_PTR    aTableCursor,
                                           scGRID            aGRID,
                                           smiColumn*        aLobColumn,
                                           UInt              aInfo,
                                           smiLobCursorMode  aMode,
                                           smLobLocator*     aLobLocator);

typedef IDE_RC (*mtcReadLob)( idvSQL*      aStatistics,
                              smLobLocator aLobLoc,
                              UInt         aOffset,
                              UInt         aMount,
                              UChar*       aPiece,
                              UInt*        aReadLength );

typedef IDE_RC (*mtcGetLobLengthWithLocator)( idvSQL*      aStatistics,
                                              smLobLocator aLobLoc,
                                              SLong*       aLobLen,
                                              idBool*      aIsNullLob );

typedef idBool (*mtcIsNullLobColumnWithRow)( const void      * aRow,
                                             const smiColumn * aColumn );

/* PROJ-2446 ONE SOURCE */
typedef idvSQL* (*mtcGetStatistics)( mtcTemplate * aTemplate );

//----------------------------------------------------------------
// PROJ-2002 Column Security
// mtcEncrypt        : 암호화 함수
// mtcDecrypt        : 복호화 함수
// mtcGetDecryptInfo : 복호화시 필요한 정보를 얻는 함수
// mtcEncodeECC      : ECC(Encrypted Comparison Code) encode 함수
//----------------------------------------------------------------

// 암복호화시 보안모듈에 필요한 정보를 전달하기 위한 자료구조
struct mtcEncryptInfo
{
    // session info
    UInt     sessionID;
    SChar  * ipAddr;
    SChar  * userName;

    // statement info
    SChar  * stmtType;   // select/insert/update/delete

    // column info
    SChar  * tableOwnerName;
    SChar  * tableName;
    SChar  * columnName;
};

// 암복호화시 보안모듈에 필요한 정보를 전달하기 위한 자료구조
struct mtcECCInfo
{
    // session info
    UInt     sessionID;
};

// Echar, Evarchar canonize에 사용됨
typedef IDE_RC (*mtcEncrypt) ( mtcEncryptInfo  * aEncryptInfo,
                               SChar           * aPolicyName,
                               UChar           * aPlain,
                               UShort            aPlainSize,
                               UChar           * aCipher,
                               UShort          * aCipherSize );

// Echar->Char, Evarchar->Varchar conversion에 사용됨
typedef IDE_RC (*mtcDecrypt) ( mtcEncryptInfo  * aEncryptInfo,
                               SChar           * aPolicyName,
                               UChar           * aCipher,
                               UShort            aCipherSize,
                               UChar           * aPlain,
                               UShort          * aPlainSize );

// Char->Echar, Varchar->Evarchar conversion에 사용됨
typedef IDE_RC (*mtcEncodeECC) ( mtcECCInfo   * aInfo,
                                 UChar        * aPlain,
                                 UShort         aPlainSize,
                                 UChar        * aCipher,
                                 UShort       * aCipherSize );

// decrypt를 위해 decryptInfo를 생성함
typedef IDE_RC (*mtcGetDecryptInfo) ( mtcTemplate     * aTemplate,
                                      UShort            aTable,
                                      UShort            aColumn,
                                      mtcEncryptInfo  * aDecryptInfo );

// ECC를 위해 ECCInfo를 생성함
typedef IDE_RC (*mtcGetECCInfo) ( mtcTemplate * aTemplate,
                                  mtcECCInfo  * aECCInfo );

// Echar, Evarchar의 initializeColumn시 사용됨
typedef IDE_RC (*mtcGetECCSize) ( UInt    aSize,
                                  UInt  * aECCSize );

// PROJ-1579 NCHAR
typedef SChar * (*mtcGetDBCharSet)();
typedef SChar * (*mtcGetNationalCharSet)();

struct mtcExtCallback
{
    mtcGetColumnFunc            getColumn;
    mtcOpenLobCursorWithRow     openLobCursorWithRow;
    mtcOpenLobCursorWithGRID    openLobCursorWithGRID;
    mtcReadLob                  readLob;
    mtcGetLobLengthWithLocator  getLobLengthWithLocator;
    mtcIsNullLobColumnWithRow   isNullLobColumnWithRow;
    /* PROJ-2446 ONE SOURCE */
    mtcGetStatistics            getStatistics;
    // PROJ-1579 NCHAR
    mtcGetDBCharSet             getDBCharSet;
    mtcGetNationalCharSet       getNationalCharSet;

    // PROJ-2264 Dictionary table
    mtcGetCompressionColumnFunc getCompressionColumn;
};

// PROJ-1361 : language에 따라 다음 문자로 이동
// PROJ-1755 : nextChar 함수 최적화
// PROJ-1579,PROJ-1681, BUG-21117
// 정확한 문자열 체크를 위해 fence를 nextchar안에서 체크해야 함.
typedef mtlNCRet (*mtlNextCharFunc)( UChar ** aSource, UChar * aFence );

// PROJ-1361 : language에 따라 그에 맞는 chr() 함수의 수행 모듈
typedef IDE_RC (*mtlChrFunc)( UChar      * aResult,
                              const SInt   aSource );

// PROJ-1361 : mtlExtractFuncSet 또는 mtlNextDayFuncSet에서 사용
typedef SInt (*mtlMatchFunc)( UChar* aSource,
                              UInt   aSourceLen );

// BUG-13830 문자갯수에 대한 language별 최대 precision 계산
typedef SInt (*mtlMaxPrecisionFunc)( SInt aLength );

/* PROJ-2208 : Multi Currency */
typedef IDE_RC ( *mtcGetCurrencyFunc )( mtcTemplate * aTemplate,
                                        mtlCurrency * aCurrency );

struct mtcId {
    UInt dataTypeId;
    UInt languageId;
};

// Estimate 을 위한 부가 정보
struct mtcCallBack {
    void*                       info;     // (mtcCallBackInfo or qtcCallBackInfo)

    UInt                        flag;     // Child에 대한 estimate 여부
    mtfAllocFunc                alloc;    // Memory 할당 함수

    //----------------------------------------------------------
    // [Conversion을 처리하기 위한 함수 포인터]
    //
    // initConversionNode
    //    : Conversion Node의 생성 및 초기화
    //    : Estimate 시점에 호출
    //----------------------------------------------------------

    mtfInitConversionNodeFunc   initConversionNode;
};

// mtcCallBack.info로 사용되며, 현재는 서버 구동시에만 사용된다.
struct mtcCallBackInfo {
    iduMemory* memory;            // 사용하는 Memory Mgr
    UShort     columnCount;       // 현재 Column의 개수
    UShort     columnMaximum;     // Column의 최대 개수
};

// PROJ-2002 Column Security
struct mtcColumnEncryptAttr {
    SInt       mEncPrecision;                      // 암호 데이타 타입의 precision
    SChar      mPolicy[MTC_POLICY_NAME_SIZE+1];    // 보안 정책의 이름 (Null Terminated String)
};

// PROJ-2422 srid
struct mtcColumnSRIDAttr {
    SInt       mSrid;
};

// column의 부가정보
union mtcColumnAttr {
    mtcColumnEncryptAttr  mEncAttr;
    mtcColumnSRIDAttr     mSridAttr;
};

struct mtcColumn {
    smiColumn         column;
    mtcId             type;
    UInt              flag;
    SInt              precision;
    SInt              scale;

    // column의 부가정보
    mtcColumnAttr     mColumnAttr;
    
    const mtdModule * module;
    const mtlModule * language; // PROJ-1361 : add language module
};

// mtcColumn에서 필요한 부분만 복사한다.
#define MTC_COPY_COLUMN_DESC(_dst_, _src_)               \
{                                                        \
    (_dst_)->column.id       = (_src_)->column.id;       \
    (_dst_)->column.flag     = (_src_)->column.flag;     \
    (_dst_)->column.offset   = (_src_)->column.offset;   \
    (_dst_)->column.varOrder = (_src_)->column.varOrder; \
    (_dst_)->column.vcInOutBaseSize = (_src_)->column.vcInOutBaseSize; \
    (_dst_)->column.size     = (_src_)->column.size;     \
    (_dst_)->column.align    = (_src_)->column.align;    \
    (_dst_)->column.maxAlign = (_src_)->column.maxAlign; \
    (_dst_)->column.value    = (_src_)->column.value;    \
    (_dst_)->column.colSpace = (_src_)->column.colSpace; \
    (_dst_)->column.colSeg   = (_src_)->column.colSeg;   \
    (_dst_)->column.colType  = (_src_)->column.colType;  \
    (_dst_)->column.descSeg  = (_src_)->column.descSeg;  \
    (_dst_)->column.mDictionaryTableOID = (_src_)->column.mDictionaryTableOID; \
    (_dst_)->type            = (_src_)->type;            \
    (_dst_)->flag            = (_src_)->flag;            \
    (_dst_)->precision       = (_src_)->precision;       \
    (_dst_)->scale           = (_src_)->scale;           \
    (_dst_)->mColumnAttr.mEncAttr.mEncPrecision    = (_src_)->mColumnAttr.mEncAttr.mEncPrecision;    \
    idlOS::memcpy((_dst_)->mColumnAttr.mEncAttr.mPolicy, (_src_)->mColumnAttr.mEncAttr.mPolicy, MTC_POLICY_NAME_SIZE+1); \
    (_dst_)->module          = (_src_)->module;          \
    (_dst_)->language        = (_src_)->language;        \
}

// 연산의 수행을 위한 각종 함수 정보
struct mtcExecute {
    mtfCalculateFunc     initialize;    // Aggregation 초기화 함수
    mtfCalculateFunc     aggregate;     // Aggregation 수행 함수
    mtfCalculateFunc     merge;         // Aggregation 임시값들을 누적시키는 함수
    mtfCalculateFunc     finalize;      // Aggregation 종료 함수
    mtfCalculateFunc     calculate;     // 연산 함수
    void*                calculateInfo; // 연산을 위한 부가 정보, 현재 미사용
    mtxSerialExecuteFunc mSerialExecute;/* PROJ-2632 */
    mtfEstimateRangeFunc estimateRange; // Key Range 크기 추출 함수
    mtfExtractRangeFunc  extractRange;  // Key Range 생성 함수
};

// 데이터 타입, 연산 등의 이름을 정의
struct mtcName {
    mtcName*    next;   // 다른 이름
    UInt        length; // 이름의 길이, Bytes
    void*       string; // 이름, UTF-8
};

// PROJ-1473
struct mtcColumnLocate {
    UShort      table;
    UShort      column;
};

struct mtcNode {
    const mtfModule* module;
    mtcNode*         conversion;
    mtcNode*         leftConversion;
    mtcNode*         funcArguments;   // PROJ-1492 CAST( operand AS target )
    mtcNode*         orgNode;         // PROJ-1404 original const expr node
    mtcNode*         arguments;
    mtcNode*         next;
    UShort           table;
    UShort           column;
    UShort           baseTable;       // PROJ-2002 column인 경우 base table
    UShort           baseColumn;      // PROJ-2002 column인 경우 base column
    ULong            lflag;
    ULong            cost;
    UInt             info;
    smOID            objectID;        // PROJ-1073 Package
};

#define MTC_NODE_INIT(_node_)               \
{                                           \
    (_node_)->module         = NULL;        \
    (_node_)->conversion     = NULL;        \
    (_node_)->leftConversion = NULL;        \
    (_node_)->funcArguments  = NULL;        \
    (_node_)->orgNode        = NULL;        \
    (_node_)->arguments      = NULL;        \
    (_node_)->next           = NULL;        \
    (_node_)->table          = 0;           \
    (_node_)->column         = 0;           \
    (_node_)->baseTable      = 0;           \
    (_node_)->baseColumn     = 0;           \
    (_node_)->lflag          = 0;           \
    (_node_)->cost           = 0;           \
    (_node_)->info           = ID_UINT_MAX; \
    (_node_)->objectID       = 0;           \
}

struct mtkRangeCallBack
{
    UInt              flag; // range 생성 시 필요한 정보들을 가짐
                            // (1) Asc/Desc 
                            // (2) 동일 계열 내 서로 다른 data type간의
                            //     compare 함수인지 여부
                            // (3) Operator 종류 ( <=, >=, <, > )
    UInt              columnIdx; // STOREDVAL의 비교에 사용되는 칼럼 인덱스
                                 // (칼럼 배치 순서)
    
    mtdCompareFunc    compare;

    // PROJ-1436
    // columnDesc, valueDesc는 shared template의 column을
    // 사용해서는 안된다. variable type column인 경우
    // 데이터 오염이 발생할 수 있다. 따라서 column 정보를
    // 복사해서 사용한다.
    // 추후 private template의 column 정보를 사용할 수
    // 있도록 수정해야 한다.
    mtcColumn         columnDesc;
    mtcColumn         valueDesc;
    const void*       value;


    mtkRangeCallBack* next;
};

// PROJ-2527 WITHIN GROUP AGGR
// mt에서 사용되는 memory pool.
typedef struct mtfFuncDataBasicInfo
{
    iduMemory            * memoryMgr;

    /* BUG-46906 */
    iduMemoryStatus        mMemStatus;
} mtfFuncDataBasicInfo;

// BUG-41944 high precision/performance hint 제공
typedef enum
{
    MTC_ARITHMETIC_OPERATION_PRECISION = 0,           // 산술연산시 정밀도우선
    MTC_ARITHMETIC_OPERATION_PERFORMANCE_LEVEL1 = 1,  // 산술연산시 성능우선
    MTC_ARITHMETIC_OPERATION_PERFORMANCE_LEVEL2 = 2,  // 산술연산시 성능우선 SUM, AVG
    MTC_ARITHMETIC_OPERATION_MAX_PRECISION = 3,       /* BUG-46195 */
    MTC_ARITHMETIC_OPERATION_DEFAULT = MTC_ARITHMETIC_OPERATION_PERFORMANCE_LEVEL1
} mtcArithmeticOpMode;

//----------------------------------------------------------
// Key Range 생성을 위한 정보
// - column : Index Column 정보, Memory/Disk Index를 지원
// - argument : Index Column의 위치 (0: left, 1: right)
// - compValueType : 비교하는 값들의 type
//                   MTD_COMPARE_FIXED_MTDVAL_FIXED_MTDVAL
//                   MTD_COMPARE_MTDVAL_MTDVAL
//                   MTD_COMPARE_STOREDVAL_MTDVAL
//                   MTD_COMPARE_STOREDVAL_STOREDVAL
// - direction : MTD_COMPARE_ASCENDING, MTD_COMPARE_DESCENDING
// - isSameGroupType : sm에 callback으로 내려줄 compare 함수 결정
//                     TRUE : mtd::compareSameGroupAscending
//                            mtd::compareSameGroupDescending
//                     FALSE: index column의 mtdModule.compare
// - columnIdx : STOREDVAL 타입의 Column을 비교할때 사용되는
//               칼럼 인덱스
//----------------------------------------------------------
struct mtkRangeInfo
{
    mtcColumn * column;    
    UInt        argument;   
    UInt        compValueType; 
    UInt        direction;  
    idBool      isSameGroupType;
    UInt        columnIdx;
    UInt        useOffset; /* BUG-47690 Inlist 복합 Index fatal */
};

struct mtkRangeFuncIndex
{
    smiCallBackFunc  rangeFunc;
    UInt             idx;
};

struct mtkCompareFuncIndex
{
    mtdCompareFunc   compareFunc;
    UInt             idx;
};

struct mtcStack {
    mtcColumn* column;
    void*      value;
};

struct mtcTuple {
    UInt                    modify;
    ULong                   lflag;
    UShort                  columnCount;
    UShort                  columnMaximum;
    UShort                  partitionTupleID;
    mtcColumn             * columns;
    mtcColumnLocate       * columnLocate; // PROJ-1473    
    mtcExecute            * execute;
    mtcExecute            * ridExecute;   // PROJ-1789 PROWID
    UInt                    rowOffset;
    UInt                    rowMaximum;
    void                  * tableHandle;
    scGRID                  rid;          // Disk Table을 위한 Record IDentifier
    void                  * row;
    void                  * cursorInfo;   // PROJ-2205 rownum in DML
};

struct mtcTemplate {
    mtcStack              * stackBuffer;
    SInt                    stackCount;
    mtcStack              * stack;
    SInt                    stackRemain;
    UChar                 * data;         // execution정보의 data영역
    UInt                    dataSize;
    UChar                 * execInfo;     // 수행 여부 판단을 위한 영역
    UInt                    execInfoCnt;  // 수행 여부 영역의 개수
    mtcInitSubqueryFunc     initSubquery;
    mtcFetchSubqueryFunc    fetchSubquery;
    mtcFiniSubqueryFunc     finiSubquery;
    mtcSetCalcSubqueryFunc  setCalcSubquery; // PROJ-2448

    mtcGetOpenedCursorFunc  getOpenedCursor; // PROJ-1362
    mtcAddOpenedLobCursorFunc  addOpenedLobCursor; // BUG-40427
    mtcIsBaseTable          isBaseTable;     // PROJ-1362
    mtcCloseLobLocator      closeLobLocator; // PROJ-1362
    mtcGetSTObjBufSizeFunc  getSTObjBufSize; // PROJ-1583 large geometry

    // PROJ-2002 Column Security
    mtcEncrypt              encrypt;
    mtcDecrypt              decrypt;
    mtcEncodeECC            encodeECC;
    mtcGetDecryptInfo       getDecryptInfo;
    mtcGetECCInfo           getECCInfo;
    mtcGetECCSize           getECCSize;

    // PROJ-1358 Internal Tuple은 동적으로 증가한다.
    UShort                  currentRow[MTC_TUPLE_TYPE_MAXIMUM];
    UShort                  rowArrayCount;    // 할당된 internal tuple의 개수
    UShort                  rowCount;         // 사용중인 internal tuple의 개수
    UShort                  rowCountBeforeBinding;
    UShort                  variableRow;
    mtcTuple              * rows;

    SChar                 * dateFormat;
    idBool                  dateFormatRef;
    
    /* PROJ-2208 Multi Currency */
    mtcGetCurrencyFunc      nlsCurrency;
    idBool                  nlsCurrencyRef;

    // BUG-37247
    idBool                  groupConcatPrecisionRef;

    // BUG-41944
    mtcArithmeticOpMode     arithmeticOpMode;
    idBool                  arithmeticOpModeRef;
    
    // PROJ-1579 NCHAR
    const mtlModule       * nlsUse;

    // PROJ-2527 WITHIN GROUP AGGR
    mtfFuncDataBasicInfo ** funcData;
    UInt                    funcDataCnt;
};

struct mtvConvert{
    UInt              count;
    mtfCalculateFunc* convert;
    mtcColumn*        columns;
    mtcStack*         stack;
};

struct mtvTable {
    ULong             cost;
    UInt              count;
    const mtvModule** modules;
    const mtvModule*  module;
};

struct mtfSubModule {
    mtfSubModule*   next;
    mtfEstimateFunc estimate;
};

typedef struct mtfNameIndex {
    const mtcName*   name;
    const mtfModule* module;
} mtfNameIndex;

//--------------------------------------------------------
// mtdModule
//    - name       : data type 명
//    - column     : default column 정보
//    - id         : data type에 따른 id
//    - no         : data type에 따른 number
//    - idxType    : data type이 사용할 수 있는 인덱스의 종류(최대8개)
//                 : idxType[0] 은 Default Index Type
//    - initialize : no, column 초기화
//    - estimate   : mtcColumn의 data type 정보 설정 및 semantic 검사
//--------------------------------------------------------

# define MTD_MAX_USABLE_INDEX_CNT                  (8)

struct mtdModule {
    mtcName*               names;
    mtcColumn*             column;

    UInt                   id;
    UInt                   no;
    UChar                  idxType[MTD_MAX_USABLE_INDEX_CNT];
    UInt                   align;
    UInt                   flag;
    UInt                   maxStorePrecision;   // BUG-28921 store precision maximum
    SInt                   minScale;     // PROJ-1362
    SInt                   maxScale;     // PROJ-1362
    void*                  staticNull;
    mtdInitializeFunc      initialize;
    mtdEstimateFunc        estimate;
    mtdValueFunc           value;
    mtdActualSizeFunc      actualSize;
    mtdGetPrecisionFunc    getPrecision;  // PROJ-1877 value의 precision,scale
    mtdNullFunc            null;
    mtdHashFunc            hash;
    mtdIsNullFunc          isNull;
    mtdIsTrueFunc          isTrue;
    mtdCompareFunc         logicalCompare[2];   // Type의 논리적 비교
    mtdCompareFunc         keyCompare[MTD_COMPARE_FUNC_MAX_CNT][2]; // 인덱스 Key 비교
    mtdCanonizeFunc        canonize;
    mtdEndianFunc          endian;
    mtdValidateFunc        validate;
    mtdSelectivityFunc     selectivity;  // A4 : Selectivity 계산 함수
    mtdEncodeFunc          encode;
    mtdDecodeFunc          decode;
    mtdCompileFmtFunc      compileFmt;
    mtdValueFromOracleFunc valueFromOracle;
    mtdMakeColumnInfo      makeColumnInfo;

    // BUG-28934
    // data type의 특성에 맞게 key range를 생성한 후, 이를 merge할 때도
    // key range의 특성에 맞게 merge하는 방법이 필요하다. merge는 결국
    // compare의 문제이므로 compare함수를 정의한 mtdModule에 정의한다.
    mtcMergeAndRange       mergeAndRange;
    mtcMergeOrRangeList    mergeOrRangeList;
    
    //-------------------------------------------------------------------
    // PROJ-1705
    // 디스크테이블의 레코드 저장구조 변경으로 인해
    // 디스크 테이블 레코드 패치시 
    // 컬럼단위로 qp 레코드처리영역의 해당 컬럼위치에 복사 
    // PROJ-2429 Dictionary based data compress for on-disk DB
    // disk의 dictionary column을 fetch할때 사용하는 함수 포인터를 추가로
    // 저장 한다.(mtd::mtdStoredValue2MtdValue4CompressColumn)
    // char, varchar, nchar nvarchar, byte, nibble, bit, varbit, data 타입이
    // 해단되며 나머지 data type은 null이 저장된다.
    //-------------------------------------------------------------------
    mtdStoredValue2MtdValueFunc  storedValue2MtdValue[MTD_COLUMN_COPY_FUNC_MAX_COUNT];    

    //-------------------------------------------------------------------
    // PROJ-1705
    // 각 데이타타입의 null Value의 크기 반환
    // replication에서 사용
    //-------------------------------------------------------------------
    mtdNullValueSizeFunc        nullValueSize;

    //-------------------------------------------------------------------
    // PROJ-1705
    // length를 가지는 데이타타입의 length 정보를 저장하는 변수의 크기 반환
    // 예) mtdCharType( UShort length; UChar value[1] ) 에서
    //      length타입인 UShort의 크기를 반환
    // integer와 같은 고정길이 데이타타입은 0 반환
    //-------------------------------------------------------------------
    mtdHeaderSizeFunc           headerSize;

    //-------------------------------------------------------------------
    // PROJ-2399 row tmaplate
    // varchar와 같은 variable 타입의 데이터 타입은 ID_UINT_MAX를 반환
    // mtheader가 sm에 저장된경우가 아니면 mtheader크기를 빼서 반환
    //-------------------------------------------------------------------
    mtdStoreSizeFunc             storeSize;
};  

// 연산자의 특징을 기술
struct mtfModule {
    ULong             lflag; // 할당하는 Column의 개수, 연산자의 정보
    ULong             lmask; // 하위 Node의 flag과 조합하여 해당 연산자의
                             // Index 사용 가능 여부를 flag에 Setting
    SDouble           selectivity;      // 연산자의 default selectivity
    const mtcName*    names;            // 연산자의 이름 정보
    const mtfModule*  counter;          // 해당 연산자의 Counter 연산자
    mtfInitializeFunc initialize;       // 서버 구동시 연산자를 위한 초기화
    mtfFinalizeFunc   finalize;         // 서버 종료시 연산자를 위한 종료 함수
    mtfEstimateFunc   estimate;         // PROJ-2163 : 연산자의 estimation을 위한 함수
};

// PROJ-1361
//    language에 따라 '년,월,일,시,분,초,마이크로초' 문자가 다르다.
//    따라서 EXTRACT 함수 수행 시,
//    language에 맞게 '년,월,일,시,분,초,마이크로초' 일치여부 검사
//    '세기, 분기, 주' 추가
typedef struct mtlExtractFuncSet {
    mtlMatchFunc   matchCentury;
    mtlMatchFunc   matchYear;
    mtlMatchFunc   matchQuarter;
    mtlMatchFunc   matchMonth;
    mtlMatchFunc   matchWeek;
    mtlMatchFunc   matchWeekOfMonth;
    mtlMatchFunc   matchDay;
    mtlMatchFunc   matchDayOfYear;
    mtlMatchFunc   matchDayOfWeek;
    mtlMatchFunc   matchHour;
    mtlMatchFunc   matchMinute;
    mtlMatchFunc   matchSecond;
    mtlMatchFunc   matchMicroSec;
    /* BUG-45730 ROUND, TRUNC 함수에서 DATE 포맷 IW 추가 지원 */
    mtlMatchFunc   matchISOWeek;
} mtlExtractFuncSet;

// PROJ-1361
// language에 따라 요일을 나타내는 문자가 다르다.
// 따라서 NEXT_DAY 함수 수행 시, language에 맞게 요일 일치 여부 검사
typedef struct mtlNextDayFuncSet {
    mtlMatchFunc   matchSunDay;
    mtlMatchFunc   matchMonDay;
    mtlMatchFunc   matchTueDay;
    mtlMatchFunc   matchWedDay;
    mtlMatchFunc   matchThuDay;
    mtlMatchFunc   matchFriDay;
    mtlMatchFunc   matchSatDay;
} mtlNextDayFuncSet;

struct mtlModule {
    const mtcName*        names;
    UInt                  id;
    
    mtlNextCharFunc       nextCharPtr;    // Language에 맞게 다음 문자로 이동
    mtlMaxPrecisionFunc   maxPrecision;   // Language의 최대 precision계산
    mtlExtractFuncSet   * extractSet;     // Language에 맞는 Date Type 이름
    mtlNextDayFuncSet   * nextDaySet;     // Language에 맞는 요일 이름
    
    UChar              ** specialCharSet;
    UChar                 specialCharSize;
};

struct mtvModule {
    const mtdModule* to;
    const mtdModule* from;
    ULong            cost;
    mtfEstimateFunc  estimate;
};

// PROJ-1362
// ODBC API, SQLGetTypeInfo 지원을 위한 mtdModule 확장
// 이전에는 타입정보가 client에 하드코딩되어 있던것을
// 서버에서 조회하도록 변경함
#define MTD_TYPE_NAME_LEN            40
#define MTD_TYPE_LITERAL_LEN         4
#define MTD_TYPE_CREATE_PARAM_LEN    20

// BUG-17202
struct mtdType
{
    SChar    * mTypeName;
    SShort     mDataType;
    SShort     mODBCDataType; // mDataType을 ODBC SPEC으로 변경
    SInt       mColumnSize;
    SChar    * mLiteralPrefix;
    SChar    * mLiteralSuffix;
    SChar    * mCreateParam;
    SShort     mNullable;
    SShort     mCaseSens;
    SShort     mSearchable;
    SShort     mUnsignedAttr;
    SShort     mFixedPrecScale;
    SShort     mAutoUniqueValue;
    SChar    * mLocalTypeName;
    SShort     mMinScale;
    SShort     mMaxScale;
    SShort     mSQLDataType;
    SShort     mODBCSQLDataType; // mSQLDataType을 ODBC SPEC으로 변경
    SShort     mSQLDateTimeSub;
    SInt       mNumPrecRadix;
    SShort     mIntervalPrecision;
};

// PROJ-1362
// LobCursor.Info
#define MTC_LOB_COLUMN_NOTNULL_MASK        (0x00000001)
#define MTC_LOB_COLUMN_NOTNULL_TRUE        (0x00000001)
#define MTC_LOB_COLUMN_NOTNULL_FALSE       (0x00000000)

#define MTC_LOB_LOCATOR_CLOSE_MASK         (0x00000002)
#define MTC_LOB_LOCATOR_CLOSE_TRUE         (0x00000002)
#define MTC_LOB_LOCATOR_CLOSE_FALSE        (0x00000000)

// BUG-40427
// LobCursor.Info
// - 내부에서만 사용하는 경우   : FALSE
// - Client에서도 사용하는 경우 : TRUE
#define MTC_LOB_LOCATOR_CLIENT_MASK        (0x00000004)
#define MTC_LOB_LOCATOR_CLIENT_TRUE        (0x00000004)
#define MTC_LOB_LOCATOR_CLIENT_FALSE       (0x00000000)

// PROJ-1362
// Lob에 대해 LENGTH, SUBSTR을 수행할 때 Lob데이터를 읽는 단위
#define MTC_LOB_BUFFER_SIZE     (32000)

// PROJ-1753 One Pass Like Algorithm
#define MTC_LIKE_PATTERN_MAX_SIZE (4000)
#define MTC_LIKE_HASH_KEY         (0x000FFFFF)
#define MTC_LIKE_SHIFT            (8)

#define MTC_FORMAT_BLOCK_STRING   (1)
#define MTC_FORMAT_BLOCK_PERCENT  (2)
#define MTC_FORMAT_BLOCK_UNDER    (3)

// 16byte에 맞추었음
struct mtcLikeBlockInfo
{    
    UChar   * start;

    // MTC_FORMAT_BLOCK_STRING인 경우 string byte size
    // MTC_FORMAT_BLOCK_PERCENT인 경우 갯수
    // MTC_FORMAT_BLOCK_UNDER인 경우 갯수
    UShort    sizeOrCnt;
    
    UChar     type;
};

// PROJ-1755
struct mtcLobCursor
{
    UInt     offset;
    UChar  * index;
};

// PROJ-1755
struct mtcLobBuffer
{
    smLobLocator   locator;
    UInt           length;
    
    UChar        * buf;
    UChar        * fence;
    UInt           offset;
    UInt           size;
};

// PROJ-1755
typedef enum
{                                   // format string에 대하여
    MTC_FORMAT_NULL = 1,            // case 1) null 혹은 ''로 구성
    MTC_FORMAT_NORMAL,              // case 2) 일반문자로만 구성
    MTC_FORMAT_UNDER,               // case 3) '_'로만 구성
    MTC_FORMAT_PERCENT,             // case 4) '%'로만 구성
    MTC_FORMAT_NORMAL_UNDER,        // case 5) 일반문자와 '_'로 구성
    MTC_FORMAT_NORMAL_ONE_PERCENT,  // case 6-1) 일반문자와 '%' 1개 로 구성
    MTC_FORMAT_NORMAL_MANY_PERCENT, // case 6-2) 일반문자와 '%' 2개이상으로 구성
    MTC_FORMAT_UNDER_PERCENT,       // case 7) '_'와 '%'로 구성
    MTC_FORMAT_ALL                  // case 8) 일반문자와 '_', '%'로 구성
} mtcLikeFormatType;

// PROJ-1755
struct mtcLikeFormatInfo
{
    // escape문자를 제거한 format pattern
    UChar             * pattern;
    UShort              patternSize;

    // format pattern의 분류정보
    mtcLikeFormatType   type;
    UShort              underCnt;
    UShort              percentCnt;
    UShort              charCnt;

    // '%'가 1개인 경우의 부가정보
    // [head]%[tail]
    UChar             * firstPercent;
    UChar             * head;     // pattern에서의 head 위치
    UShort              headSize;
    UChar             * tail;     // pattern에서의 tail 위치
    UShort              tailSize;

    // PROJ-1753 one pass like
    // case 5,6-2,8에 대한 최적화
    //
    // i1 like '%abc%%__\_%de_' escape '\'
    // => 8 blocks
    //   1. percent '%',1
    //   2. string  'abc',3
    //   3. percent '%%',2
    //   4. under   '__',2
    //   5. string  '_',1
    //   6. percent '%',1
    //   7. string  'de',2
    //   8. under   '_',1
    //
    UInt                blockCnt;
    mtcLikeBlockInfo  * blockInfo;
    UChar             * refinePattern;    // escaped pattern
};

/* BUG-32622 inlist operator */
#define MTC_INLIST_ELEMENT_COUNT_MAX  (1000)
#define MTC_INLIST_KEYRANGE_COUNT_MAX (MTC_INLIST_ELEMENT_COUNT_MAX)
#define MTC_INLIST_ELEMRNT_LENGTH_MAX (4000)
/* 문자열 최대길이 + (char_align-1)*MTC_INLIST_ELEMENT_COUNT_MAX + (char_header_size)*MTC_INLIST_ELEMENT_COUNT_MAX */
#define MTC_INLIST_VALUE_BUFFER_MAX   (32000 + 1*MTC_INLIST_ELEMENT_COUNT_MAX + 2*MTC_INLIST_ELEMENT_COUNT_MAX)

struct mtcInlistInfo
{
    UInt          count;                                         /* token 개수               */
    mtcColumn     valueDesc;                                     /* value 의 column descirpt */
    void        * valueArray[MTC_INLIST_ELEMENT_COUNT_MAX];      /* value pointer array      */
    ULong         valueBuf[MTC_INLIST_VALUE_BUFFER_MAX / 8 + 1]; /* 실제 value buffer        */
};

// PROJ-1579 NCHAR
typedef enum
{
    MTC_COLUMN_NCHAR_LITERAL = 1,
    MTC_COLUMN_UNICODE_LITERAL,
    MTC_COLUMN_NORMAL_LITERAL
} mtcLiteralType;

//-------------------------------------------------------------------
// PROJ-1872
// compare 할때 필요한 정보
// - mtd type 으로 비교할때
//   column, value, flag 정보 이용함 
//   value의 column.offset 위치에서 해당 column의 value를 읽어옴 
// - stored type으로 비교할때
//   column, value, length 정보 이용함
//   value를 length 만큼 읽어옴 
//-------------------------------------------------------------------
typedef struct mtdValueInfo 
{
    const mtcColumn * column;
    const void      * value;
    UInt              length;
    UInt              flag;
} mtdValueInfo;

//-------------------------------------------------------------------
// BUG-33663
// ranking function에 사용하는 define
//-------------------------------------------------------------------
typedef enum
{
    MTC_RANK_VALUE_FIRST = 1,  // 첫번째 값
    MTC_RANK_VALUE_SAME,       // 이전 값과 같다.
    MTC_RANK_VALUE_DIFF        // 이전 값과 다르다.
} mtcRankValueType;

// PROJ-1789 PROWID
#define MTC_RID_COLUMN_ID ID_USHORT_MAX

// PROJ-1861 REGEXP_LIKE

#define MTC_REGEXP_APPLY_FROM_START (0x00000001)
#define MTC_REGEXP_APPLY_AT_END     (0x00000002)
#define MTC_REGEXP_NORMAL           (0x00000000)

typedef enum
{
    R_PC_IDX = 0,   // error
    R_DOLLAR_IDX,   // '$'
    R_CARET_IDX,    // '^'
    R_BSLASH_IDX,   // '\\'
    R_PLUS_IDX,     // '+'
    R_MINUS_IDX,    // '-'
    R_STAR_IDX,     // '*'
    R_QMARK_IDX,    // '?' 
    R_COMMA_IDX,    // ','
    R_PRIOD_IDX,    // '.'
    R_VBAR_IDX,     // '|'
    R_LPAREN_IDX,   // '('
    R_RPAREN_IDX,   // ')'
    R_LCBRACK_IDX,  // '{'
    R_RCBRACK_IDX,  // '}'
    R_LBRACKET_IDX, // '['
    R_RBRACKET_IDX, // '],
    R_ALNUM_IDX,    // :alnum:
    R_ALPHA_IDX,    // :alpha:
    R_BLANK_IDX,    // :blank:
    R_CNTRL_IDX,    // :cntrl:
    R_DIGIT_IDX,    // :digit:
    R_GRAPH_IDX,    // :graph:
    R_LOWER_IDX,    // :lower:
    R_PRINT_IDX,    // :print:
    R_PUNCT_IDX,    // :punct:
    R_SPACE_IDX,    // :space:
    R_UPPER_IDX,    // :upper:
    R_XDIGIT_IDX,   // :xdigit:
    R_MAX_IDX
} mtcRegPatternType;

struct mtcRegPatternTokenInfo
{
    UChar            * ptr;    
    mtcRegPatternType  type;
    UInt               size;
};

struct mtcRegBlockInfo
{
    mtcRegPatternTokenInfo * start;
    mtcRegPatternTokenInfo * end;
    UInt                     minRepeat;
    UInt                     maxRepeat;    
};

struct mtcRegBracketInfo
{
    mtcRegPatternType       type;
    mtcRegPatternTokenInfo *tokenPtr;
    mtcRegBracketInfo      *bracketPair;    
};

struct mtcRegPatternInfo
{
    UInt                     tokenCount;    
    UInt                     blockCount;
    mtcRegBlockInfo        * blockInfo;
    UInt                     flag;    
    mtcRegPatternTokenInfo * patternToken;
};

/* PROJ-2180 */
typedef struct mtfQuantFuncArgData {
	mtfQuantFuncArgData * mSelf;			/* for safe check           */
	UInt                  mElements;		/* # of in_list             */
	idBool *              mIsNullList;      /* idBool[mElements]        */
	mtcStack *            mStackList;       /* mStackList[mElements]    */
} mtfQuantFuncArgData;

typedef enum {
	MTF_QUANTIFY_FUNC_UNKNOWN = 0,
	MTF_QUANTIFY_FUNC_NORMAL_CALC,
	MTF_QUANTIFY_FUNC_FAST_CALC,
} mtfQuantFuncCalcType;

#define MTC_TUPLE_EXECUTE(aTuple, aNode)      \
    ((aNode)->column != MTC_RID_COLUMN_ID) ?  \
    ((aTuple)->execute + (aNode)->column)  :  \
    ((aTuple)->ridExecute)

/* PROJ-2632 */
#define MTX_SERIAL_EXECUTE_DATA_SIZE  ( ID_SIZEOF( mtxSerialExecuteData ) )
#define MTX_SERIAL_ENTRY_SIZE         ( ID_SIZEOF( mtxEntry ) )

#define MTX_ENTRY_TYPE_NONE           ( 0x00000000 )
#define MTX_ENTRY_TYPE_VALUE          ( 0x00000001 )
#define MTX_ENTRY_TYPE_RID            ( 0x00000002 )
#define MTX_ENTRY_TYPE_COLUMN         ( 0x00000003 )
#define MTX_ENTRY_TYPE_FUNCTION       ( 0x00000004 )
#define MTX_ENTRY_TYPE_SINGLE         ( 0x00000005 )
#define MTX_ENTRY_TYPE_CONVERT        ( 0x00000006 )
#define MTX_ENTRY_TYPE_CONVERT_CHAR   ( 0x00000007 )
#define MTX_ENTRY_TYPE_CONVERT_DATE   ( 0x00000008 )
#define MTX_ENTRY_TYPE_CHECK          ( 0x00000009 )
#define MTX_ENTRY_TYPE_AND            ( 0x0000000A )
#define MTX_ENTRY_TYPE_OR             ( 0x0000000B )
#define MTX_ENTRY_TYPE_AND_SINGLE     ( 0x0000000C )
#define MTX_ENTRY_TYPE_OR_SINGLE      ( 0x0000000D )
#define MTX_ENTRY_TYPE_NOT            ( 0x0000000E )
#define MTX_ENTRY_TYPE_LNNVL          ( 0x0000000F )
#define MTX_ENTRY_TYPE_ERROR          ( 0x000000FF )

#define MTX_ENTRY_SIZE_ZERO           ( 0x00000000 )
#define MTX_ENTRY_SIZE_ONE            ( 0x00000001 )
#define MTX_ENTRY_SIZE_TWO            ( 0x00000002 )
#define MTX_ENTRY_SIZE_THREE          ( 0x00000003 )

#define MTX_ENTRY_COUNT_ZERO          ( 0x00000000 )
#define MTX_ENTRY_COUNT_ONE           ( 0x00000001 )
#define MTX_ENTRY_COUNT_TWO           ( 0x00000002 )
#define MTX_ENTRY_COUNT_THREE         ( 0x00000003 )

#define MTX_ENTRY_FLAG_INITIALIZE     ( 0x00000000 )
#define MTX_ENTRY_FLAG_CALCULATE_MASK ( 0x00000001 )
#define MTX_ENTRY_FLAG_CALCULATE      ( 0x00000001 )

typedef enum
{
    MTX_EXECUTOR_ID_DATE,
    MTX_EXECUTOR_ID_SMALLINT,
    MTX_EXECUTOR_ID_INTEGER,
    MTX_EXECUTOR_ID_BIGINT,
    MTX_EXECUTOR_ID_REAL,
    MTX_EXECUTOR_ID_DOUBLE,
    MTX_EXECUTOR_ID_FLOAT,
    MTX_EXECUTOR_ID_NUMERIC,
    MTX_EXECUTOR_ID_CHAR,
    MTX_EXECUTOR_ID_VARCHAR,
    MTX_EXECUTOR_ID_NCHAR,
    MTX_EXECUTOR_ID_NVARCHAR,
    MTX_EXECUTOR_ID_INTERVAL,
    MTX_EXECUTOR_ID_NIBBLE,
    MTX_EXECUTOR_ID_BIT,
    MTX_EXECUTOR_ID_VARBIT,
    MTX_EXECUTOR_ID_BYTE,
    MTX_EXECUTOR_ID_VARBYTE,
    MTX_EXECUTOR_ID_RID,
    MTX_EXECUTOR_ID_COLUMN,
    MTX_EXECUTOR_ID_AND,
    MTX_EXECUTOR_ID_OR,
    MTX_EXECUTOR_ID_NOT,
    MTX_EXECUTOR_ID_EQUAL,
    MTX_EXECUTOR_ID_NOT_EQUAL,
    MTX_EXECUTOR_ID_GREATER_THAN,
    MTX_EXECUTOR_ID_GREATER_EQUAL,
    MTX_EXECUTOR_ID_LESS_THAN,
    MTX_EXECUTOR_ID_LESS_EQUAL,
    MTX_EXECUTOR_ID_ADD,
    MTX_EXECUTOR_ID_SUBTRACT,
    MTX_EXECUTOR_ID_MULTIPLY,
    MTX_EXECUTOR_ID_DIVIDE,
    MTX_EXECUTOR_ID_IS_NOT_NULL,
    MTX_EXECUTOR_ID_LNNVL,
    MTX_EXECUTOR_ID_MAX
} mtxSerialExecuteId;

struct mtxModule
{
    mtxSerialExecuteId   mId;
    mtxSerialExecuteFunc mCommon;
    mtxGetSerialExecute  mGetExecute;
};

typedef struct mtxEntryHeader
{
    UShort mId;     /* 2B : Identify Entry Set*/
    UChar  mType;   /* 3B : Entry Set Type*/
    UChar  mSize;   /* 4B : Entry Size for Entry Set*/
    UChar  mCount;  /* 5B : Arugment Count*/
    UChar  mFlag;   /* 6B : Flag*/
    UChar  mRSV[2]; /* 8B */
} mtxEntryHeader;

union mtxEntry
{
    mtxEntryHeader         mHeader; 
    void                 * mAddress;
    mtxSerialExecuteFunc   mExecute;
};

#define MTX_SET_ENTRY_HEAD( _entry_, _header_ ) \
{                                               \
    ( _entry_ )->mHeader = ( _header_ );        \
    ( _entry_ )++;                              \
}

#define MTX_SET_ENTRY_ADDR( _entry_, _val_ ) \
{                                            \
    ( _entry_ )->mAddress = ( _val_ );       \
    ( _entry_ )++;                           \
}

#define MTX_SET_ENTRY_RETN( _entry_, _val_ ) \
    MTX_SET_ENTRY_ADDR( ( _entry_ ), ( _val_ ) )

#define MTX_SET_ENTRY_EXEC( _entry_, _val_ ) \
{                                            \
    ( _entry_ )->mExecute = ( _val_ );       \
    ( _entry_ )++;                           \
}

#define MTX_SET_ENTRY_FLAG( _entry_, _flag_ ) \
{                                             \
    ( _entry_ )->mHeader.mFlag |= ( _flag_ ); \
}

#define MTX_INITAILZE_ENTRY_FLAG( _entry_ ) \
{                                           \
    ( _entry_ )->mHeader.mFlag &=           \
        ~MTX_ENTRY_FLAG_CALCULATE;          \
}

#define MTX_GET_TUPLE_ADDR( _template_, _node_ )        \
    (void*)&(( _template_ )->rows[( _node_ )->table])

#define MTX_GET_COLUNM_ADDR( _template_, _node_ )             \
    (void*)&((( _template_ )->rows[( _node_ )->table].columns \
              + ( _node_ )->column)->column)

#define MTX_GET_MTC_COLUMN( _template_, _node_ )            \
    (void*)(( _template_ )->rows[( _node_ )->table].columns \
            + ( _node_ )->column)

#define MTX_GET_ROW_ADDR( _template_, _node_ )              \
    (UChar*)(( _template_ )->rows[( _node_ )->table].row)

#define MTX_GET_ROW_OFFSET( _template_, _node_ )      \
    ((( _template_ )->rows[( _node_ )->table].columns \
      + ( _node_ )->column)->column.offset)

#define MTX_GET_RETURN_ADDR( _template_, _node_ )               \
    (void*)(MTX_GET_ROW_ADDR( ( _template_ ), ( _node_ ) )      \
            + MTX_GET_ROW_OFFSET( ( _template_ ), ( _node_ )))

#define MTX_GET_SERIAL_EXECUTE_DATA_SIZE( _count_ ) \
    ( MTX_SERIAL_EXECUTE_DATA_SIZE                  \
      + ( MTX_SERIAL_ENTRY_SIZE * ( _count_ ) ) )

typedef struct mtxSerialExecuteData
{
    mtcTuple             * mTable;
    mtxEntry             * mEntry;
    UInt                   mEntryTotalSize;
    UInt                   mEntrySetCount;
} mtxSerialExecuteData;

#define MTX_SERIAL_EXEUCTE_DATA_INITIALIZE( _data_ ,                \
                                            _template_,             \
                                            _count_,                \
                                            _setcount_,             \
                                            _offset_ )              \
{                                                                   \
    ( _data_ )                  =                                   \
        (mtxSerialExecuteData*)( ( _template_ ) + ( _offset_ ) );   \
    ( _data_ )->mTable          = NULL;                             \
    ( _data_ )->mEntry          =                                   \
        (mtxEntry*)( ( _template_ ) + ( _offset_ )                  \
                     + MTX_SERIAL_EXECUTE_DATA_SIZE );              \
    ( _data_ )->mEntryTotalSize = ( _count_ );                      \
    ( _data_ )->mEntrySetCount  = ( _setcount_ );                   \
}

typedef struct mtxSerialFilterInfo
{
    mtxEntryHeader        mHeader;
    mtcNode             * mNode;
    mtxSerialExecuteFunc  mExecute;
    UInt                  mOffset;
    mtxSerialFilterInfo * mLeft;
    mtxSerialFilterInfo * mRight;
} mtxSerialFilterInfo;

#define MTX_SET_SERIAL_FILTER_INFO( _info_ ,     \
                                    _node_ ,     \
                                    _execute_ ,  \
                                    _offset_,    \
                                    _left_,      \
                                    _right_ )    \
{                                                \
    ( _info_ )->mNode    = ( _node_ );           \
    ( _info_ )->mExecute = ( _execute_ );        \
    ( _info_ )->mOffset  = ( _offset_ );         \
    ( _info_ )->mLeft    = ( _left_ );           \
    ( _info_ )->mRight   = ( _right_ );          \
}

#define MTX_SET_ENTRY_HEADER( _header_,  \
                              _id_,      \
                              _type_,    \
                              _size_,    \
                              _count_ )  \
{                                        \
    ( _header_ ).mId     = ( _id_ );     \
    ( _header_ ).mType   = ( _type_ );   \
    ( _header_ ).mSize   = ( _size_ );   \
    ( _header_ ).mCount  = ( _count_ );  \
    ( _header_ ).mFlag   =               \
        MTX_ENTRY_FLAG_INITIALIZE;       \
    ( _header_ ).mRSV[0] = 0;            \
    ( _header_ ).mRSV[1] = 0;            \
}

#define MTC_COLUMN_IS_NOT_SAME( aColumn, aColumnPtr )                       \
(                                                                           \
    ( ( ( aColumn.column.id )       != ( aColumnPtr->column.id ) ) ||       \
      ( ( aColumn.column.colSpace ) != ( aColumnPtr->column.colSpace ) ) || \
      ( ( aColumn.module )          != ( aColumnPtr->module ) ) ) ?         \
      ID_TRUE : ID_FALSE                                                    \
)

#endif /* _O_MTC_DEF_H_ */
