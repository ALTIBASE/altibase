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
 * $Id: mtcDef.h 38046 2010-02-03 02:34:16Z peh $
 **********************************************************************/

#ifndef _O_MTC_DEF_H_
# define _O_MTC_DEF_H_ 1

#include <acp.h>
#include <acl.h>
#include <aciTypes.h>
#include <aciConv.h>

/* Partial Key                                       */
# define MTD_PARTIAL_KEY_MINIMUM                    (0)
# define MTD_PARTIAL_KEY_MAXIMUM         (ACP_ULONG_MAX)

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

/* mtcColumn.flag                                    */
/* 칼럼이 실행되는 위치 정보                      */
# define MTC_COLUMN_USE_POSITION_MASK      (0x000003F0)
# define MTC_COLUMN_USE_FROM               (0x00000010)
# define MTC_COLUMN_USE_WHERE              (0x00000020)
# define MTC_COLUMN_USE_GROUPBY            (0x00000040)
# define MTC_COLUMN_USE_HAVING             (0x00000080)
# define MTC_COLUMN_USE_ORDERBY            (0x00000100)
# define MTC_COLUMN_USE_WINDOW             (0x00000200)
# define MTC_COLUMN_USE_TARGET             (0x00000300)

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

//====================================================================
// mtcNode.flag 정의
//====================================================================

// BUG-34127 Increase maximum arguement count(4095(0x0FFF) -> 12287(0x2FFF))
/* mtcNode.flag                                      */
# define MTC_NODE_ARGUMENT_COUNT_MASK           ACP_UINT64_LITERAL(0x000000000000FFFF)
# define MTC_NODE_ARGUMENT_COUNT_MAXIMUM                                       (12287)

/* mtfModule.flag                                    */
// 연산자가 차지하는 Column의 개수로 AVG()와 같은 연산은
// SUM()과 COUNT()정보등을 관리하기 위해 여러개의
// Column 을 유지해야 한다.
# define MTC_NODE_COLUMN_COUNT_MASK             ACP_UINT64_LITERAL(0x00000000000000FF)

/* mtcNode.flag                                      */
# define MTC_NODE_MASK        ( MTC_NODE_INDEX_MASK |       \
                                MTC_NODE_BIND_MASK  |       \
                                MTC_NODE_DML_MASK   |       \
                                MTC_NODE_EAT_NULL_MASK |    \
                                MTC_NODE_HAVE_SUBQUERY_MASK )

/* mtcNode.flag, mtfModule.flag                      */
# define MTC_NODE_OPERATOR_MASK                 ACP_UINT64_LITERAL(0x00000000000F0000)
# define MTC_NODE_OPERATOR_MISC                 ACP_UINT64_LITERAL(0x0000000000000000)
# define MTC_NODE_OPERATOR_AND                  ACP_UINT64_LITERAL(0x0000000000010000)
# define MTC_NODE_OPERATOR_OR                   ACP_UINT64_LITERAL(0x0000000000020000)
# define MTC_NODE_OPERATOR_NOT                  ACP_UINT64_LITERAL(0x0000000000030000)
# define MTC_NODE_OPERATOR_EQUAL                ACP_UINT64_LITERAL(0x0000000000040000)
# define MTC_NODE_OPERATOR_NOT_EQUAL            ACP_UINT64_LITERAL(0x0000000000050000)
# define MTC_NODE_OPERATOR_GREATER              ACP_UINT64_LITERAL(0x0000000000060000)
# define MTC_NODE_OPERATOR_GREATER_EQUAL        ACP_UINT64_LITERAL(0x0000000000070000)
# define MTC_NODE_OPERATOR_LESS                 ACP_UINT64_LITERAL(0x0000000000080000)
# define MTC_NODE_OPERATOR_LESS_EQUAL           ACP_UINT64_LITERAL(0x0000000000090000)
# define MTC_NODE_OPERATOR_RANGED               ACP_UINT64_LITERAL(0x00000000000A0000)
# define MTC_NODE_OPERATOR_NOT_RANGED           ACP_UINT64_LITERAL(0x00000000000B0000)
# define MTC_NODE_OPERATOR_LIST                 ACP_UINT64_LITERAL(0x00000000000C0000)
# define MTC_NODE_OPERATOR_FUNCTION             ACP_UINT64_LITERAL(0x00000000000D0000)
# define MTC_NODE_OPERATOR_AGGREGATION          ACP_UINT64_LITERAL(0x00000000000E0000)
# define MTC_NODE_OPERATOR_SUBQUERY             ACP_UINT64_LITERAL(0x00000000000F0000)

/* mtcNode.flag                                      */
# define MTC_NODE_INDIRECT_MASK                 ACP_UINT64_LITERAL(0x0000000000100000)
# define MTC_NODE_INDIRECT_TRUE                 ACP_UINT64_LITERAL(0x0000000000100000)
# define MTC_NODE_INDIRECT_FALSE                ACP_UINT64_LITERAL(0x0000000000000000)

/* mtcNode.flag, mtfModule.flag                      */
# define MTC_NODE_COMPARISON_MASK               ACP_UINT64_LITERAL(0x0000000000200000)
# define MTC_NODE_COMPARISON_TRUE               ACP_UINT64_LITERAL(0x0000000000200000)
# define MTC_NODE_COMPARISON_FALSE              ACP_UINT64_LITERAL(0x0000000000000000)

/* mtcNode.flag, mtfModule.flag                      */
# define MTC_NODE_LOGICAL_CONDITION_MASK        ACP_UINT64_LITERAL(0x0000000000400000)
# define MTC_NODE_LOGICAL_CONDITION_TRUE        ACP_UINT64_LITERAL(0x0000000000400000)
# define MTC_NODE_LOGICAL_CONDITION_FALSE       ACP_UINT64_LITERAL(0x0000000000000000)

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
# define MTC_NODE_INDEX_MASK                    ACP_UINT64_LITERAL(0x0000000000800000)
# define MTC_NODE_INDEX_USABLE                  ACP_UINT64_LITERAL(0x0000000000800000)
# define MTC_NODE_INDEX_UNUSABLE                ACP_UINT64_LITERAL(0x0000000000000000)

/* mtcNode.flag                                      */
// 현재와 하위 노드에 호스트 변수가 존재함을 나타낸다.
# define MTC_NODE_BIND_MASK                     ACP_UINT64_LITERAL(0x0000000001000000)
# define MTC_NODE_BIND_EXIST                    ACP_UINT64_LITERAL(0x0000000001000000)
# define MTC_NODE_BIND_ABSENT                   ACP_UINT64_LITERAL(0x0000000000000000)

/* mtcNode.flag                                      */
# define MTC_NODE_QUANTIFIER_MASK               ACP_UINT64_LITERAL(0x0000000002000000)
# define MTC_NODE_QUANTIFIER_TRUE               ACP_UINT64_LITERAL(0x0000000002000000)
# define MTC_NODE_QUANTIFIER_FALSE              ACP_UINT64_LITERAL(0x0000000000000000)

/* mtcNode.flag                                      */
# define MTC_NODE_DISTINCT_MASK                 ACP_UINT64_LITERAL(0x0000000004000000)
# define MTC_NODE_DISTINCT_TRUE                 ACP_UINT64_LITERAL(0x0000000004000000)
# define MTC_NODE_DISTINCT_FALSE                ACP_UINT64_LITERAL(0x0000000000000000)

/* mtcNode.flag                                      */
# define MTC_NODE_GROUP_COMPARISON_MASK         ACP_UINT64_LITERAL(0x0000000008000000)
# define MTC_NODE_GROUP_COMPARISON_TRUE         ACP_UINT64_LITERAL(0x0000000008000000)
# define MTC_NODE_GROUP_COMPARISON_FALSE        ACP_UINT64_LITERAL(0x0000000000000000)

/* mtcNode.flag                                      */
# define MTC_NODE_GROUP_MASK                    ACP_UINT64_LITERAL(0x0000000010000000)
# define MTC_NODE_GROUP_ANY                     ACP_UINT64_LITERAL(0x0000000010000000)
# define MTC_NODE_GROUP_ALL                     ACP_UINT64_LITERAL(0x0000000000000000)

/* mtcNode.flag                                      */
# define MTC_NODE_FILTER_MASK                   ACP_UINT64_LITERAL(0x0000000020000000)
# define MTC_NODE_FILTER_NEED                   ACP_UINT64_LITERAL(0x0000000020000000)
# define MTC_NODE_FILTER_NEEDLESS               ACP_UINT64_LITERAL(0x0000000000000000)

/* mtcNode.flag                                      */
# define MTC_NODE_DML_MASK                      ACP_UINT64_LITERAL(0x0000000040000000)
# define MTC_NODE_DML_UNUSABLE                  ACP_UINT64_LITERAL(0x0000000040000000)
# define MTC_NODE_DML_USABLE                    ACP_UINT64_LITERAL(0x0000000000000000)

/* mtcNode.flag                                      */
// fix BUG-13830
# define MTC_NODE_REESTIMATE_MASK               ACP_UINT64_LITERAL(0x0000000080000000)
# define MTC_NODE_REESTIMATE_TRUE               ACP_UINT64_LITERAL(0x0000000080000000)
# define MTC_NODE_REESTIMATE_FALSE              ACP_UINT64_LITERAL(0x0000000000000000)

/* mtfModule.flag                                    */
// To Fix PR-15434 INDEX JOIN이 가능한 연산자인지의 여부
# define MTC_NODE_INDEX_JOINABLE_MASK           ACP_UINT64_LITERAL(0x0000000100000000)
# define MTC_NODE_INDEX_JOINABLE_TRUE           ACP_UINT64_LITERAL(0x0000000100000000)
# define MTC_NODE_INDEX_JOINABLE_FALSE          ACP_UINT64_LITERAL(0x0000000000000000)
// To Fix PR-15291
# define MTC_NODE_INDEX_ARGUMENT_MASK           ACP_UINT64_LITERAL(0x0000000200000000)
# define MTC_NODE_INDEX_ARGUMENT_BOTH           ACP_UINT64_LITERAL(0x0000000200000000)
# define MTC_NODE_INDEX_ARGUMENT_LEFT           ACP_UINT64_LITERAL(0x0000000000000000)

// BUG-15995
/* mtfModule.flag                                    */
# define MTC_NODE_VARIABLE_MASK                 ACP_UINT64_LITERAL(0x0000000400000000)
# define MTC_NODE_VARIABLE_TRUE                 ACP_UINT64_LITERAL(0x0000000400000000)
# define MTC_NODE_VARIABLE_FALSE                ACP_UINT64_LITERAL(0x0000000000000000)

// PROJ-1492
// 현재노드의 타입이 validation시에 결정되었음을 나타낸다.
/* mtcNode.flag                                      */
# define MTC_NODE_BIND_TYPE_MASK                ACP_UINT64_LITERAL(0x0000000800000000)
# define MTC_NODE_BIND_TYPE_TRUE                ACP_UINT64_LITERAL(0x0000000800000000)
# define MTC_NODE_BIND_TYPE_FALSE               ACP_UINT64_LITERAL(0x0000000000000000)

/* BUG-44762 case when subquery */
# define MTC_NODE_HAVE_SUBQUERY_MASK            ACP_UINT64_LITERAL(0x0000001000000000)
# define MTC_NODE_HAVE_SUBQUERY_TRUE            ACP_UINT64_LITERAL(0x0000001000000000)
# define MTC_NODE_HAVE_SUBQUERY_FALSE           ACP_UINT64_LITERAL(0x0000000000000000)

//fix BUG-17610
# define MTC_NODE_EQUI_VALID_SKIP_MASK          ACP_UINT64_LITERAL(0x0000002000000000)
# define MTC_NODE_EQUI_VALID_SKIP_TRUE          ACP_UINT64_LITERAL(0x0000002000000000)
# define MTC_NODE_EQUI_VALID_SKIP_FALSE         ACP_UINT64_LITERAL(0x0000000000000000)

// fix BUG-22512 Outer Join 계열 Push Down Predicate 할 경우, 결과 오류
// CASE2, DECODE, DIGEST, DUMP, IS NULL, IS NOT NULL, NVL, NVL2, UserDefined Function 함수는
// 인자로 받은 NULL값을 다른 값으로 바꿀 수 있다.
// CASE2, DECODE, DIGEST, DUMP, IS NULL, IS NOT NULL, NVL, NVL2, UserDefined Function 함수는 
// outer join 하위 노드로 push down 할 수 없는 function임을 표시.
# define MTC_NODE_EAT_NULL_MASK                 ACP_UINT64_LITERAL(0x0000004000000000)
# define MTC_NODE_EAT_NULL_TRUE                 ACP_UINT64_LITERAL(0x0000004000000000)
# define MTC_NODE_EAT_NULL_FALSE                ACP_UINT64_LITERAL(0x0000000000000000)

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
# define MTC_NODE_PRINT_FMT_MASK                ACP_UINT64_LITERAL(0x00000F0000000000)
# define MTC_NODE_PRINT_FMT_PREFIX_PA           ACP_UINT64_LITERAL(0x0000000000000000)
# define MTC_NODE_PRINT_FMT_PREFIX_SP           ACP_UINT64_LITERAL(0x0000010000000000)
# define MTC_NODE_PRINT_FMT_INFIX               ACP_UINT64_LITERAL(0x0000020000000000)
# define MTC_NODE_PRINT_FMT_INFIX_SP            ACP_UINT64_LITERAL(0x0000030000000000)
# define MTC_NODE_PRINT_FMT_POSTFIX_SP          ACP_UINT64_LITERAL(0x0000040000000000)
# define MTC_NODE_PRINT_FMT_MISC                ACP_UINT64_LITERAL(0x0000050000000000)

// PROJ-1473
# define MTC_NODE_COLUMN_LOCATE_CHANGE_MASK     ACP_UINT64_LITERAL(0x0000100000000000)
# define MTC_NODE_COLUMN_LOCATE_CHANGE_FALSE    ACP_UINT64_LITERAL(0x0000100000000000)
# define MTC_NODE_COLUMN_LOCATE_CHANGE_TRUE     ACP_UINT64_LITERAL(0x0000000000000000)

// PROJ-1473
# define MTC_NODE_REMOVE_ARGUMENTS_MASK         ACP_UINT64_LITERAL(0x0000200000000000)
# define MTC_NODE_REMOVE_ARGUMENTS_TRUE         ACP_UINT64_LITERAL(0x0000200000000000)
# define MTC_NODE_REMOVE_ARGUMENTS_FALSE        ACP_UINT64_LITERAL(0x0000000000000000)

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
# define MTC_NODE_CASE_TYPE_MASK                ACP_UINT64_LITERAL(0x0000400000000000) 
# define MTC_NODE_CASE_TYPE_SIMPLE              ACP_UINT64_LITERAL(0x0000400000000000) 
# define MTC_NODE_CASE_TYPE_SEARCHED            ACP_UINT64_LITERAL(0x0000000000000000)

// BUG-38133
// BUG-28223 CASE expr WHEN .. THEN .. 구문의 expr에 subquery 사용시 에러발생 
// qtc::estimateInternal()함수의 주석 참조.
# define MTC_NODE_CASE_EXPRESSION_MAKE_PASSNODE_MASK  ACP_UINT64_LITERAL(0x0000800000000000)
# define MTC_NODE_CASE_EXPRESSION_MAKE_PASSNODE_TRUE  ACP_UINT64_LITERAL(0x0000800000000000)
# define MTC_NODE_CASE_EXPRESSION_MAKE_PASSNODE_FALSE ACP_UINT64_LITERAL(0x0000000000000000)

// BUG-28446 [valgrind], BUG-38133
// qtcCalculate_Pass(mtcNode*, mtcStack*, int, void*, mtcTemplate*)
# define MTC_NODE_CASE_EXPRESSION_PASSNODE_MASK  ACP_UINT64_LITERAL(0x0001000000000000)
# define MTC_NODE_CASE_EXPRESSION_PASSNODE_TRUE  ACP_UINT64_LITERAL(0x0001000000000000)
# define MTC_NODE_CASE_EXPRESSION_PASSNODE_FALSE ACP_UINT64_LITERAL(0x0000000000000000)

// PROJ-1492
# define MTC_NODE_IS_DEFINED_TYPE( aNode )                  \
    (                                                       \
        (  ( ( (aNode)->lflag & MTC_NODE_BIND_MASK )        \
             == MTC_NODE_BIND_ABSENT )                      \
           ||                                               \
           ( ( ( (aNode)->lflag & MTC_NODE_BIND_MASK )      \
               == MTC_NODE_BIND_EXIST )                     \
             &&                                             \
             ( ( (aNode)->lflag & MTC_NODE_BIND_TYPE_MASK ) \
               == MTC_NODE_BIND_TYPE_TRUE ) )               \
           )                                                \
        ? ACP_TRUE : ACP_FALSE                              \
        )

# define MTC_NODE_IS_DEFINED_VALUE( aNode )         \
    (                                               \
        ( ( (aNode)->lflag & MTC_NODE_BIND_MASK )   \
          == MTC_NODE_BIND_ABSENT )                 \
        ? ACP_TRUE : ACP_FALSE                      \
        )

/* mtfExtractRange Von likeExtractRange              */
# define MTC_LIKE_KEY_SIZE      ( ACP_ALIGN8( sizeof(acp_uint16_t) + 32 )
# define MTC_LIKE_KEY_PRECISION ( MTC_LIKE_KEY_SIZE - sizeof(acp_uint16_t) )

/* mtvModule.cost                                    */
# define MTV_COST_INFINITE                 ACP_UINT64_LITERAL(0x4000000000000000)
# define MTV_COST_GROUP_PENALTY            ACP_UINT64_LITERAL(0x0001000000000000)
# define MTV_COST_ERROR_PENALTY            ACP_UINT64_LITERAL(0x0000000100000000)
# define MTV_COST_LOSS_PENALTY             ACP_UINT64_LITERAL(0x0000000000010000)
# define MTV_COST_DEFAULT                  ACP_UINT64_LITERAL(0x0000000000000001)

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


/* mtcTemplate.rows                                  */
// fix BUG-12500
// # define MTC_TUPLE_ROW_MAXIMUM                   (1024)

// PROJ-1358
// mtcTemplate.rows 는
// 16개부터 시작하여 65536 개까지 동적으로 증가한다.
#define MTC_TUPLE_ROW_INIT_CNT                     (16)
//#define MTC_TUPLE_ROW_INIT_CNT                      (2)
#define MTC_TUPLE_ROW_MAX_CNT           (ACP_USHORT_MAX)   // BUG-17154

//====================================================================
// mtcTuple.flag 정의
//====================================================================

/* mtcTuple.flag                                     */
# define MTC_TUPLE_TYPE_MAXIMUM                     (4)
/* BUG-44490 mtcTuple flag 확장 해야 합니다. */
/* 32bit flag 공간 모두 소모해 64bit로 증가 */
/* type을 UInt에서 ULong으로 변경 */
# define MTC_TUPLE_TYPE_MASK               ACP_UINT64_LITERAL(0x0000000000000003)
# define MTC_TUPLE_TYPE_CONSTANT           ACP_UINT64_LITERAL(0x0000000000000000)
# define MTC_TUPLE_TYPE_VARIABLE           ACP_UINT64_LITERAL(0x0000000000000001)
# define MTC_TUPLE_TYPE_INTERMEDIATE       ACP_UINT64_LITERAL(0x0000000000000002)
# define MTC_TUPLE_TYPE_TABLE              ACP_UINT64_LITERAL(0x0000000000000003)

/* mtcTuple.flag                                     */
# define MTC_TUPLE_COLUMN_SET_MASK         ACP_UINT64_LITERAL(0x0000000000000010)
# define MTC_TUPLE_COLUMN_SET_TRUE         ACP_UINT64_LITERAL(0x0000000000000010)
# define MTC_TUPLE_COLUMN_SET_FALSE        ACP_UINT64_LITERAL(0x0000000000000000)

/* mtcTuple.flag                                     */
# define MTC_TUPLE_COLUMN_ALLOCATE_MASK    ACP_UINT64_LITERAL(0x0000000000000020)
# define MTC_TUPLE_COLUMN_ALLOCATE_TRUE    ACP_UINT64_LITERAL(0x0000000000000020)
# define MTC_TUPLE_COLUMN_ALLOCATE_FALSE   ACP_UINT64_LITERAL(0x0000000000000000)

/* mtcTuple.flag                                     */
# define MTC_TUPLE_COLUMN_COPY_MASK        ACP_UINT64_LITERAL(0x0000000000000040)
# define MTC_TUPLE_COLUMN_COPY_TRUE        ACP_UINT64_LITERAL(0x0000000000000040)
# define MTC_TUPLE_COLUMN_COPY_FALSE       ACP_UINT64_LITERAL(0x0000000000000000)

/* mtcTuple.flag                                     */
# define MTC_TUPLE_EXECUTE_SET_MASK        ACP_UINT64_LITERAL(0x0000000000000080)
# define MTC_TUPLE_EXECUTE_SET_TRUE        ACP_UINT64_LITERAL(0x0000000000000080)
# define MTC_TUPLE_EXECUTE_SET_FALSE       ACP_UINT64_LITERAL(0x0000000000000000)

/* mtcTuple.flag                                     */
# define MTC_TUPLE_EXECUTE_ALLOCATE_MASK   ACP_UINT64_LITERAL(0x0000000000000100)
# define MTC_TUPLE_EXECUTE_ALLOCATE_TRUE   ACP_UINT64_LITERAL(0x0000000000000100)
# define MTC_TUPLE_EXECUTE_ALLOCATE_FALSE  ACP_UINT64_LITERAL(0x0000000000000000)

/* mtcTuple.flag                                     */
# define MTC_TUPLE_EXECUTE_COPY_MASK       ACP_UINT64_LITERAL(0x0000000000000200)
# define MTC_TUPLE_EXECUTE_COPY_TRUE       ACP_UINT64_LITERAL(0x0000000000000200)
# define MTC_TUPLE_EXECUTE_COPY_FALSE      ACP_UINT64_LITERAL(0x0000000000000000)

/* mtcTuple.flag                                     */
# define MTC_TUPLE_ROW_SET_MASK            ACP_UINT64_LITERAL(0x0000000000000400)
# define MTC_TUPLE_ROW_SET_TRUE            ACP_UINT64_LITERAL(0x0000000000000400)
# define MTC_TUPLE_ROW_SET_FALSE           ACP_UINT64_LITERAL(0x0000000000000000)

/* mtcTuple.flag                                     */
# define MTC_TUPLE_ROW_ALLOCATE_MASK       ACP_UINT64_LITERAL(0x0000000000000800)
# define MTC_TUPLE_ROW_ALLOCATE_TRUE       ACP_UINT64_LITERAL(0x0000000000000800)
# define MTC_TUPLE_ROW_ALLOCATE_FALSE      ACP_UINT64_LITERAL(0x0000000000000000)

/* mtcTuple.flag                                     */
# define MTC_TUPLE_ROW_COPY_MASK           ACP_UINT64_LITERAL(0x0000000000001000)
# define MTC_TUPLE_ROW_COPY_TRUE           ACP_UINT64_LITERAL(0x0000000000001000)
# define MTC_TUPLE_ROW_COPY_FALSE          ACP_UINT64_LITERAL(0x0000000000000000)

/* mtcTuple.flag                                     */
# define MTC_TUPLE_ROW_SKIP_MASK           ACP_UINT64_LITERAL(0x0000000000002000)
# define MTC_TUPLE_ROW_SKIP_TRUE           ACP_UINT64_LITERAL(0x0000000000002000)
# define MTC_TUPLE_ROW_SKIP_FALSE          ACP_UINT64_LITERAL(0x0000000000000000)

/* mtcTuple.flag                                     */
// Tuple의 저장 매체에 대한 정보
# define MTC_TUPLE_STORAGE_MASK            ACP_UINT64_LITERAL(0x0000000000010000)
# define MTC_TUPLE_STORAGE_MEMORY          ACP_UINT64_LITERAL(0x0000000000000000)
# define MTC_TUPLE_STORAGE_DISK            ACP_UINT64_LITERAL(0x0000000000010000)

/* mtcTuple.flag                                     */
// 해당 Tuple이 VIEW를 위한 Tuple인지를 설정
# define MTC_TUPLE_VIEW_MASK               ACP_UINT64_LITERAL(0x0000000000020000)
# define MTC_TUPLE_VIEW_FALSE              ACP_UINT64_LITERAL(0x0000000000000000)
# define MTC_TUPLE_VIEW_TRUE               ACP_UINT64_LITERAL(0x0000000000020000)

/* mtcTuple.flag */
// 해당 Tuple이 Plan을 위해 생성된 Tuple인지를 설정
# define MTC_TUPLE_PLAN_MASK               ACP_UINT64_LITERAL(0x0000000000040000)
# define MTC_TUPLE_PLAN_FALSE              ACP_UINT64_LITERAL(0x0000000000000000)
# define MTC_TUPLE_PLAN_TRUE               ACP_UINT64_LITERAL(0x0000000000040000)

/* mtcTuple.flag */
// 해당 Tuple이 Materialization Plan을 위해 생성된 Tuple
# define MTC_TUPLE_PLAN_MTR_MASK           ACP_UINT64_LITERAL(0x0000000000080000)
# define MTC_TUPLE_PLAN_MTR_FALSE          ACP_UINT64_LITERAL(0x0000000000000000)
# define MTC_TUPLE_PLAN_MTR_TRUE           ACP_UINT64_LITERAL(0x0000000000080000)

/* mtcTuple.flag */
// PROJ-1502 PARTITIONED DISK TABLE
// 해당 Tuple이 Partitioned Table을 위해 생성된 Tuple
# define MTC_TUPLE_PARTITIONED_TABLE_MASK  ACP_UINT64_LITERAL(0x0000000000100000)
# define MTC_TUPLE_PARTITIONED_TABLE_FALSE ACP_UINT64_LITERAL(0x0000000000000000)
# define MTC_TUPLE_PARTITIONED_TABLE_TRUE  ACP_UINT64_LITERAL(0x0000000000100000)

/* mtcTuple.flag */
// PROJ-1502 PARTITIONED DISK TABLE
// 해당 Tuple이 Partition을 위해 생성된 Tuple
# define MTC_TUPLE_PARTITION_MASK          ACP_UINT64_LITERAL(0x0000000000200000)
# define MTC_TUPLE_PARTITION_FALSE         ACP_UINT64_LITERAL(0x0000000000000000)
# define MTC_TUPLE_PARTITION_TRUE          ACP_UINT64_LITERAL(0x0000000000200000)

/* mtcTuple.flag */
/* BUG-36468 Tuple의 저장매체 위치(Location)에 대한 정보 */
# define MTC_TUPLE_STORAGE_LOCATION_MASK   ACP_UINT64_LITERAL(0x0000000000400000)
# define MTC_TUPLE_STORAGE_LOCATION_LOCAL  ACP_UINT64_LITERAL(0x0000000000000000)
# define MTC_TUPLE_STORAGE_LOCATION_REMOTE ACP_UINT64_LITERAL(0x0000000000400000)

/* mtcTuple.flag */
/* PROJ-2464 hybrid partitioned table 지원 */
// 해당 Tuple이 Hybrid를 위해 생성된 Tuple
# define MTC_TUPLE_HYBRID_PARTITIONED_TABLE_MASK    ACP_UINT64_LITERAL(0x0000000000800000)
# define MTC_TUPLE_HYBRID_PARTITIONED_TABLE_FALSE   ACP_UINT64_LITERAL(0x0000000000000000)
# define MTC_TUPLE_HYBRID_PARTITIONED_TABLE_TRUE    ACP_UINT64_LITERAL(0x0000000000800000)

/* mtcTuple.flag */
// PROJ-1473
// tuple 에 대한 처리방법 지정.
// (1) tuple set 방식으로 처리
// (2) 레코드저장방식으로 처리
# define MTC_TUPLE_MATERIALIZE_MASK        ACP_UINT64_LITERAL(0x0000000001000000)
# define MTC_TUPLE_MATERIALIZE_RID         ACP_UINT64_LITERAL(0x0000000000000000)
# define MTC_TUPLE_MATERIALIZE_VALUE       ACP_UINT64_LITERAL(0x0000000001000000)

# define MTC_TUPLE_BINARY_COLUMN_MASK      ACP_UINT64_LITERAL(0x0000000002000000)
# define MTC_TUPLE_BINARY_COLUMN_ABSENT    ACP_UINT64_LITERAL(0x0000000000000000)
# define MTC_TUPLE_BINARY_COLUMN_EXIST     ACP_UINT64_LITERAL(0x0000000002000000)

# define MTC_TUPLE_VIEW_PUSH_PROJ_MASK     ACP_UINT64_LITERAL(0x0000000004000000)
# define MTC_TUPLE_VIEW_PUSH_PROJ_FALSE    ACP_UINT64_LITERAL(0x0000000000000000)
# define MTC_TUPLE_VIEW_PUSH_PROJ_TRUE     ACP_UINT64_LITERAL(0x0000000004000000)

// PROJ-1705
/* mtcTuple.flag */
# define MTC_TUPLE_VSCN_MASK               ACP_UINT64_LITERAL(0x0000000008000000)
# define MTC_TUPLE_VSCN_FALSE              ACP_UINT64_LITERAL(0x0000000000000000)
# define MTC_TUPLE_VSCN_TRUE               ACP_UINT64_LITERAL(0x0000000008000000)

# define MTC_TUPLE_COLUMN_ID_MAXIMUM            (1024)
/* to_char할 때나 date -> char 변환시 precision      */
# define MTC_TO_CHAR_MAX_PRECISION                 (64)

/* mtdModule.compare                                 */
/* mtdModule.partialKey                              */
/* aFlag VON mtk::setPartialKey                      */
# define MTD_COMPARE_ASCENDING                      (0)
# define MTD_COMPARE_DESCENDING                     (1)

# define MTD_COMPARE_MTDVAL_MTDVAL                  (0)
# define MTD_COMPARE_STOREDVAL_MTDVAL               (1)
# define MTD_COMPARE_STOREDVAL_STOREDVAL            (2)

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

/* mtcCallBack.flag                                  */
// Child Node에 대한 Estimation 여부를 표현함
// ENABLE일 경우, Child에 대한 Estimate를 수행하며,
// DISABLE일 경우, 자신에 대한 Estimate만을 수행한다.
# define MTC_ESTIMATE_ARGUMENTS_MASK       (0x00000001)
# define MTC_ESTIMATE_ARGUMENTS_DISABLE    (0x00000001)
# define MTC_ESTIMATE_ARGUMENTS_ENABLE     (0x00000000)

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
# define MTL_NCHAR_PRECISION( aMtlModule )          \
    (                                               \
        ( (aMtlModule)->id == MTL_UTF8_ID )         \
        ? MTL_UTF8_PRECISION : MTL_UTF16_PRECISION  \
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

typedef struct smiCColumn       smiCColumn;

typedef struct mtcId            mtcId;
typedef struct mtcCallBack      mtcCallBack;
typedef struct mtcCallBackInfo  mtcCallBackInfo;
typedef struct mtcColumn        mtcColumn;
typedef struct mtcColumnLocate  mtcColumnLocate;
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

typedef struct mtdValueInfo 
{
    const mtcColumn * column;
    const void      * value;
    acp_uint32_t      length;
    acp_uint32_t      flag;
} mtdValueInfo;

// PROJ-2002 Column Security
typedef struct mtcEncryptInfo   mtcEncryptInfo;

typedef enum
{
    NC_VALID = 0,         /* 정상적인 1글자 */
    NC_INVALID,           /* firstbyte가 코드에 안맞는 1글자 */
    NC_MB_INVALID,        /* secondbyte이후가 코드에 안맞는 1글자 */
    NC_MB_INCOMPLETED     /* 멀티바이트 글자인데 byte가 모자른 0글자 */
} mtlNCRet;

#define MTC_CURSOR_PTR  void*

typedef ACI_RC (*mtcInitSubqueryFunc)( mtcNode     * aNode,
                                       mtcTemplate * aTemplate );

typedef ACI_RC (*mtcFetchSubqueryFunc)( mtcNode     * aNode,
                                        mtcTemplate * aTemplate,
                                        acp_bool_t  * aRowExist );

typedef ACI_RC (*mtcFiniSubqueryFunc)( mtcNode     * aNode,
                                       mtcTemplate * aTemplate );

/* PROJ-2448 Subquery caching */
typedef ACI_RC (*mtcSetCalcSubqueryFunc)( mtcNode     * aNode,
                                          mtcTemplate * aTemplate );

//PROJ-1583 large geometry
typedef acp_uint32_t   (*mtcGetSTObjBufSizeFunc)( mtcCallBack * aCallBack );

typedef ACI_RC (*mtcGetOpenedCursorFunc)( mtcTemplate    * aTemplate,
                                          acp_sint16_t     aTableID,
                                          MTC_CURSOR_PTR * aCursor,
                                          acp_sint16_t   * aOrgTableID,
                                          acp_bool_t     * aFound );

/* PROJ-1530 PSM/Trigger에서 LOB 데이타 타입 지원 */
typedef acp_bool_t (*mtcIsBaseTable)( mtcTemplate * aTemplate,
                                      acp_uint16_t  aTable );

typedef ACI_RC (*mtdInitializeFunc)( acp_uint32_t sNo );

typedef ACI_RC (*mtdEstimateFunc)(  acp_uint32_t * aColumnSize,
                                    acp_uint32_t * aArguments,
                                    acp_sint32_t * aPrecision,
                                    acp_sint32_t * aScale );

typedef ACI_RC (*mtdChangeFunc)( mtcColumn    * aColumn,
                                 acp_uint32_t   aFlag );

typedef ACI_RC (*mtdValueFunc)( mtcTemplate  * aTemplate,  // fix BUG-15947
                                mtcColumn    * aColumn,
                                void         * aValue,
                                acp_uint32_t * aValueOffset,
                                acp_uint32_t   aValueSize,
                                const void   * aToken,
                                acp_uint32_t   aTokenLength,
                                ACI_RC     * aResult );

typedef acp_uint32_t (*mtdActualSizeFunc)( const mtcColumn * aColumn,
                                           const void      * aRow,
                                           acp_uint32_t      aFlag );

typedef ACI_RC (*mtdGetPrecisionFunc)( const mtcColumn * aColumn,
                                       const void      * aRow,
                                       acp_uint32_t      aFlag,
                                       acp_sint32_t    * aPrecision,
                                       acp_sint32_t    * aScale );

typedef void (*mtdNullFunc)( const mtcColumn * aColumn,
                             void            * aRow,
                             acp_uint32_t      aFlag );

typedef acp_uint32_t (*mtdHashFunc)( acp_uint32_t      aHash,
                                     const mtcColumn * aColumn,
                                     const void      * aRow,
                                     acp_uint32_t      aFlag );

typedef acp_bool_t (*mtdIsNullFunc)( const mtcColumn * aColumn,
                                     const void      * aRow,
                                     acp_uint32_t      aFlag );

typedef ACI_RC (*mtdIsTrueFunc)( acp_bool_t      * aResult,
                                 const mtcColumn * aColumn,
                                 const void      * aRow,
                                 acp_uint32_t      aFlag );

typedef acp_sint32_t (*mtdCompareFunc)( mtdValueInfo * aValueInfo1,
                                        mtdValueInfo * aValueInfo2 );

typedef acp_uint8_t (*mtsRelationFunc)( const mtcColumn * aColumn1,
                                        const void      * aRow1,
                                        acp_uint32_t      aFlag1,
                                        const mtcColumn * aColumn2,
                                        const void      * aRow2,
                                        acp_uint32_t      aFlag2,
                                        const void      * aInfo );

typedef ACI_RC (*mtdCanonizeFunc)( const mtcColumn  *  aCanon,
                                   void             ** aCanonized,
                                   mtcEncryptInfo   *  aCanonInfo,
                                   const mtcColumn  *  aColumn,
                                   void             *  aValue,
                                   mtcEncryptInfo   *  aColumnInfo,
                                   mtcTemplate      *  aTemplate );

typedef void (*mtdEndianFunc)( void* aValue );

typedef ACI_RC (*mtdValidateFunc)( mtcColumn  * aColumn,
                                   void       * aValue,
                                   acp_uint32_t aValueSize);

typedef ACI_RC (*mtdEncodeFunc) ( mtcColumn    * aColumn,
                                  void         * aValue,
                                  acp_uint32_t   aValueSize,
                                  acp_uint8_t  * aCompileFmt,
                                  acp_uint32_t   aCompileFmtLen,
                                  acp_uint8_t  * aText,
                                  acp_uint32_t * aTextLen,
                                  ACI_RC     * aRet );

typedef ACI_RC (*mtdDecodeFunc) ( mtcTemplate  * aTemplate,  // fix BUG-15947
                                  mtcColumn    * aColumn,
                                  void         * aValue,
                                  acp_uint32_t * aValueSize,
                                  acp_uint8_t  * aCompileFmt,
                                  acp_uint32_t   aCompileFmtLen,
                                  acp_uint8_t  * aText,
                                  acp_uint32_t   aTextLen,
                                  ACI_RC     * aRet );

typedef ACI_RC (*mtdCompileFmtFunc)	( mtcColumn    * aColumn,
                                      acp_uint8_t  * aCompiledFmt,
                                      acp_uint32_t * aCompiledFmtLen,
                                      acp_uint8_t  * aFormatString,
                                      acp_uint32_t   aFormatStringLength,
                                      ACI_RC     * aRet );

typedef ACI_RC (*mtdValueFromOracleFunc)( mtcColumn    * aColumn,
                                          void         * aValue,
                                          acp_uint32_t * aValueOffset,
                                          acp_uint32_t   aValueSize,
                                          const void   * aOracleValue,
                                          acp_sint32_t   aOracleLength,
                                          ACI_RC     * aResult );

typedef ACI_RC (*mtdMakeColumnInfo) ( void         * aStmt,
                                      void         * aTableInfo,
                                      acp_uint32_t   aIdx );

//-----------------------------------------------------------------------
// Data Type의 Selectivity 추출 함수
// Selectivity 계산식
//     Selectivity = (aValueMax - aValueMin) / (aColumnMax - aColumnMin)
//-----------------------------------------------------------------------

typedef acp_double_t (*mtdSelectivityFunc) ( void * aColumnMax,  // Column의 MAX값
                                             void * aColumnMin,  // Column의 MIN값
                                             void * aValueMax,   // MAX Value 값
                                             void * aValueMin ); // MIN Value 값

typedef ACI_RC (*mtfInitializeFunc)( void );

typedef ACI_RC (*mtfFinalizeFunc)( void );

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

typedef ACI_RC (*mtdStoredValue2MtdValueFunc) ( acp_uint32_t   aColumnSize,
                                                void         * aDestValue,
                                                acp_uint32_t   aDestValueOffset,
                                                acp_uint32_t   aLength,
                                                const void   * aValue );

/***********************************************************************
 * PROJ-1705
 * 각 데이타타입의 null Value의 크기 반환
 * replication에서 사용.
 **********************************************************************/

typedef acp_uint32_t (*mtdNullValueSizeFunc) ();

/***********************************************************************
 * PROJ-1705
 * length를 가지는 데이타타입의 length 정보를 저장하는 변수의 크기 반환
 * 예) mtdCharType( UShort length; UChar value[1] ) 에서
 *      length타입인 UShort의 크기를 반환
 *  integer와 같은 고정길이 데이타타입은 0 반환
 **********************************************************************/
typedef acp_uint32_t (*mtdHeaderSizeFunc) ();

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

typedef ACI_RC (*mtfEstimateFunc)( mtcNode      * aNode,
                                   mtcTemplate  * aTemplate,
                                   mtcStack     * aStack,
                                   acp_sint32_t   aRemain,
                                   mtcCallBack  * aCallBack );

//----------------------------------------------------------------
// PROJ-1358
// 참고) mtfCalculateFunc, mtfEstimateRangeFunc 등은
//       Internal Tuple Set의 확장으로 인한 문제는 발생하지 않으나,
//       mtcTuple* 를 인자로 취하는 함수를 모두 제거하여
//       문제를 사전에 방지한다.
//----------------------------------------------------------------

typedef ACI_RC (*mtfCalculateFunc)( mtcNode      * aNode,
                                    mtcStack     * aStack,
                                    acp_sint32_t   aRemain,
                                    void         * aInfo,
                                    mtcTemplate  * aTemplate );

typedef ACI_RC (*mtfEstimateRangeFunc)( mtcNode      * aNode,
                                        mtcTemplate  * aTemplate,
                                        acp_uint32_t   aArgument,
                                        acp_uint32_t * aSize );

//----------------------------------------------------------------
// PROJ-1358
// 참고) mtfCalculateFunc, mtfEstimateRangeFunc 등은
//       Internal Tuple Set의 확장으로 인한 문제는 발생하지 않으나,
//       mtcTuple* 를 인자로 취하는 함수를 모두 제거하여
//       문제를 사전에 방지한다.
//----------------------------------------------------------------

typedef ACI_RC (*mtfExtractRangeFunc)( mtcNode      * aNode,
                                       mtcTemplate  * aTemplate,
                                       mtkRangeInfo * aInfo,
                                       void         * );

typedef ACI_RC (*mtfInitConversionNodeFunc)( mtcNode ** aConversionNode,
                                             mtcNode *  aNode,
                                             void    *  aInfo );

typedef void (*mtfSetIndirectNodeFunc)( mtcNode * aNode,
                                        mtcNode * aConversionNode,
                                        void    * aInfo );

typedef ACI_RC (*mtfAllocFunc)( void         *  aInfo,
                                acp_uint32_t    aSize,
                                void         **);

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
    acp_char_t  * ipAddr;
    acp_char_t  * userName;

    // statement info
    acp_char_t  * stmtType;   // select/insert/update/delete

    // column info
    acp_char_t  * tableOwnerName;
    acp_char_t  * tableName;
    acp_char_t  * columnName;
};

// Echar, Evarchar canonize에 사용됨
typedef ACI_RC (*mtcEncrypt) ( mtcEncryptInfo * aEncryptInfo,
                               acp_char_t     * aPolicyName,
                               acp_uint8_t    * aPlain,
                               acp_uint16_t     aPlainSize,
                               acp_uint8_t    * aCipher,
                               acp_uint16_t   * aCipherSize );

// Echar->Char, Evarchar->Varchar conversion에 사용됨
typedef ACI_RC (*mtcDecrypt) ( mtcEncryptInfo * aEncryptInfo,
                               acp_char_t     * aPolicyName,
                               acp_uint8_t    * aCipher,
                               acp_uint16_t     aCipherSize,
                               acp_uint8_t    * aPlain,
                               acp_uint16_t   * aPlainSize );

// Char->Echar, Varchar->Evarchar conversion에 사용됨
typedef ACI_RC (*mtcEncodeECC) ( acp_uint8_t  * aPlain,
                                 acp_uint16_t   aPlainSize,
                                 acp_uint8_t  * aCipher,
                                 acp_uint16_t * aCipherSize );

// decrypt를 위해 decryptInfo를 생성함
typedef ACI_RC (*mtcGetDecryptInfo) ( mtcTemplate    * aTemplate,
                                      acp_uint16_t     aTable,
                                      acp_sint16_t     aColumn,
                                      mtcEncryptInfo * aDecryptInfo );

// Echar, Evarchar의 initializeColumn시 사용됨
typedef ACI_RC (*mtcGetECCSize) ( acp_uint32_t   aSize,
                                  acp_uint32_t * aECCSize );

// PROJ-1579 NCHAR
typedef acp_char_t * (*mtcGetDBCharSet)();
typedef acp_char_t * (*mtcGetNationalCharSet)();

// PROJ-1361 : language에 따라 다음 문자로 이동
// PROJ-1755 : nextChar 함수 최적화
// PROJ-1579,PROJ-1681, BUG-21117
// 정확한 문자열 체크를 위해 fence를 nextchar안에서 체크해야 함.
typedef mtlNCRet (*mtlNextCharFunc)( acp_uint8_t ** aSource, acp_uint8_t * aFence );

// PROJ-1361 : language에 따라 그에 맞는 chr() 함수의 수행 모듈
typedef ACI_RC (*mtlChrFunc)( acp_uint8_t        * aResult,
                              const acp_sint32_t   aSource );

// PROJ-1361 : mtlExtractFuncSet 또는 mtlNextDayFuncSet에서 사용
typedef acp_sint32_t (*mtlMatchFunc)( acp_uint8_t  * aSource,
                                      acp_uint32_t   aSourceLen );

// BUG-13830 문자갯수에 대한 language별 최대 precision 계산
typedef acp_sint32_t (*mtlMaxPrecisionFunc)( acp_sint32_t aLength );

struct mtcId {
    acp_uint32_t dataTypeId;
    acp_uint32_t languageId;
};

// Estimate 등을 위한 부가 정보
struct mtcCallBack {
    void*                       info;     // (mtcCallBackInfo or qtcCallBackInfo)

    acp_uint32_t                flag;     // Child에 대한 estimate 여부
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
// PROJ-2002 Column Security
#define MTC_POLICY_NAME_SIZE  (16-1)

// BUGBUG
// smiColumn을 client porting을 위해 재정의한다.
struct smiCColumn {
    acp_uint32_t      id;
    acp_uint32_t      flag;
    acp_uint32_t      offset;
    acp_uint32_t      size;
    void            * value;
};

// PROJ-2002 Column Security
struct mtcColumnEncryptAttr {
    acp_sint32_t      mEncPrecision;                      // 암호 데이타 타입의 precision
    acp_char_t        mPolicy[MTC_POLICY_NAME_SIZE+1];    // 보안 정책의 이름 (Null Terminated String)
};

// PROJ-2422 srid
struct mtcColumnSRIDAttr {
    acp_sint32_t      mSrid;
};

// column의 부가정보
union mtcColumnAttr {
    struct mtcColumnEncryptAttr  mEncAttr;
    struct mtcColumnSRIDAttr     mSridAttr;
};

struct mtcColumn {
    smiCColumn        column;
    mtcId             type;
    acp_uint32_t      flag;
    acp_sint32_t      precision;
    acp_sint32_t      scale;

    // column의 부가정보
    union mtcColumnAttr  mColumnAttr;
    
    const mtdModule * module;
    const mtlModule * language; // PROJ-1361 : add language module
};

// 연산의 수행을 위한 각종 함수 정보
struct mtcExecute {
    mtfCalculateFunc     initialize;    // Aggregation 초기화 함수
    mtfCalculateFunc     aggregate;     // Aggregation 수행 함수
    mtfCalculateFunc     finalize;      // Aggregation 종료 함수
    mtfCalculateFunc     calculate;     // 연산 함수
    void*                calculateInfo; // 연산을 위한 부가 정보, 현재 미사용
    mtfEstimateRangeFunc estimateRange; // Key Range 크기 추출 함수
    mtfExtractRangeFunc  extractRange;  // Key Range 생성 함수
};

// 데이터 타입, 연산 등의 이름을 정의
struct mtcName {
    mtcName*         next;   // 다른 이름
    acp_uint32_t     length; // 이름의 길이, Bytes
    void*            string; // 이름, UTF-8
};

// PROJ-1473
struct mtcColumnLocate {
    acp_uint16_t     table;
    acp_uint16_t     column;
};

struct mtcNode {
    const mtfModule* module;
    mtcNode*         conversion;
    mtcNode*         leftConversion;
    mtcNode*         funcArguments;   // PROJ-1492 CAST( operand AS target )
    mtcNode*         orgNode;         // PROJ-1404 original const expr node
    mtcNode*         arguments;
    mtcNode*         next;
    acp_uint16_t     table;
    acp_uint16_t     column;
    acp_uint16_t     baseTable;   // PROJ-2002 column인 경우 base table
    acp_uint16_t     baseColumn;  // PROJ-2002 column인 경우 base column
    acp_uint64_t     lflag;
    acp_uint64_t     cost;
    acp_uint32_t     info;
    acp_ulong_t      objectID;    // PROJ-1073 Package
};

#define MTC_NODE_INIT(_node_)                       \
    {                                               \
        (_node_)->module         = NULL;            \
        (_node_)->conversion     = NULL;            \
        (_node_)->leftConversion = NULL;            \
        (_node_)->funcArguments  = NULL;            \
        (_node_)->orgNode        = NULL;            \
        (_node_)->arguments      = NULL;            \
        (_node_)->next           = NULL;            \
        (_node_)->table          = 0;               \
        (_node_)->column         = 0;               \
        (_node_)->baseTable      = 0;               \
        (_node_)->baseColumn     = 0;               \
        (_node_)->lflag          = 0;               \
        (_node_)->cost           = 0;               \
        (_node_)->info           = ACP_UINT32_MAX;  \
        (_node_)->objectID       = 0;               \
    }

//----------------------------------------------------------
// Key Range 생성을 위한 정보
// - column : Index Column 정보, Memory/Disk Index를 지원
// - argument : Index Column의 위치 (0: left, 1: right)
// - compValueType : 비교하는 값들의 type
//                   MTD_COMPARE_MTDVAL_MTDVAL
//                   MTD_COMPARE_MTDVAL_STOREDVAL
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
    mtcColumn    * column;    
    acp_uint32_t   argument;   
    acp_uint32_t   compValueType; 
    acp_uint32_t   direction;  
    acp_bool_t     isSameGroupType;
    acp_uint32_t   columnIdx;
    acp_uint32_t   useOffset; /* BUG-47690 Inlist 복합 Index fatal */
};

struct mtcStack {
    mtcColumn * column;
    void      * value;
};

struct mtcTuple {
    acp_uint32_t      modify;
    acp_uint64_t      lflag;
    acp_uint16_t      columnCount;
    acp_uint16_t      columnMaximum;
    mtcColumn       * columns;
    mtcColumnLocate * columnLocate; // PROJ-1473    
    mtcExecute      * execute;
    acp_uint32_t      rowOffset;
    acp_uint32_t      rowMaximum;
    void*             tableHandle;
    void*             row;
    acp_uint16_t      partitionTupleID;
};

struct mtcTemplate {
    mtcStack*               stackBuffer;
    acp_sint32_t            stackCount;
    mtcStack*               stack;
    acp_sint32_t            stackRemain;
    acp_uint8_t*            data;         // execution정보의 data영역
    acp_uint32_t            dataSize;
    acp_uint8_t*            execInfo;     // 수행 여부 판단을 위한 영역
    acp_uint32_t            execInfoCnt;  // 수행 여부 영역의 개수
    mtcInitSubqueryFunc     initSubquery;
    mtcFetchSubqueryFunc    fetchSubquery;
    mtcFiniSubqueryFunc     finiSubquery;
    mtcSetCalcSubqueryFunc  setCalcSubquery; // PROJ-2448

    mtcGetOpenedCursorFunc  getOpenedCursor; // PROJ-1362
    mtcIsBaseTable          isBaseTable;     // PROJ-1362
    mtcGetSTObjBufSizeFunc  getSTObjBufSize; // PROJ-1583 large geometry

    // PROJ-2002 Column Security
    mtcEncrypt              encrypt;
    mtcDecrypt              decrypt;
    mtcEncodeECC            encodeECC;
    mtcGetDecryptInfo       getDecryptInfo;
    mtcGetECCSize           getECCSize;

    // PROJ-1358 Internal Tuple은 동적으로 증가한다.
    acp_uint16_t                    currentRow[MTC_TUPLE_TYPE_MAXIMUM];
    acp_uint16_t                    rowArrayCount;    // 할당된 internal tuple의 개수
    acp_uint16_t                    rowCount;         // 사용중인 internal tuple의 개수
    acp_uint16_t                    rowCountBeforeBinding;
    acp_uint16_t                    variableRow;
    mtcTuple                      * rows;

    acp_char_t                    * dateFormat;
    acp_bool_t                      dateFormatRef;
    
    acp_sint32_t                    id; //fix PROJ-1596

    // PROJ-1579 NCHAR
    const mtlModule       * nlsUse;
};

struct mtvConvert{
    acp_uint32_t       count;
    mtfCalculateFunc * convert;
    mtcColumn        * columns;
    mtcStack         * stack;
};

struct mtvTable {
    acp_uint64_t       cost;
    acp_uint32_t       count;
    const mtvModule ** modules;
    const mtvModule *  module;
};

struct mtfSubModule {
    mtfSubModule    * next;
    mtfEstimateFunc   estimate;
};

typedef struct mtfNameIndex {
    const mtcName   * name;
    const mtfModule * module;
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

    acp_uint32_t           id;
    acp_uint32_t           no;
    acp_uint8_t            idxType[MTD_MAX_USABLE_INDEX_CNT];
    acp_uint32_t           align;
    acp_uint32_t           flag;
    acp_uint32_t           columnSize;   // PROJ-1362 column size
    // 혹은 max precision
    acp_sint32_t           minScale;     // PROJ-1362
    acp_sint32_t           maxScale;     // PROJ-1362
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
    mtdCompareFunc         logicalCompare;   // Type의 논리적 비교
    mtdCompareFunc         keyCompare[3][2]; // 인덱스 Key 비교
    mtdCanonizeFunc        canonize;
    mtdEndianFunc          endian;
    mtdValidateFunc        validate;
    mtdSelectivityFunc     selectivity;  // A4 : Selectivity 계산 함수
    mtdEncodeFunc          encode;
    mtdDecodeFunc          decode;
    mtdCompileFmtFunc      compileFmt;
    mtdValueFromOracleFunc valueFromOracle;
    mtdMakeColumnInfo      makeColumnInfo;

    //-------------------------------------------------------------------
    // PROJ-1705
    // 디스크테이블의 레코드 저장구조 변경으로 인해
    // 디스크 테이블 레코드 패치시 
    // 컬럼단위로 qp 레코드처리영역의 해당 컬럼위치에 복사 
    //-------------------------------------------------------------------
    mtdStoredValue2MtdValueFunc  storedValue2MtdValue;    

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
};  

// 연산자의 특징을 기술
struct mtfModule {
    acp_uint64_t             lflag; // 할당하는 Column의 개수, 연산자의 정보
    acp_uint64_t             lmask; // 하위 Node의 flag과 조합하여 해당 연산자의
    // Index 사용 가능 여부를 flag에 Setting
    acp_double_t      selectivity;      // 연산자의 default selectivity
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
    acp_uint32_t          id;
    
    mtlNextCharFunc       nextCharPtr;    // Language에 맞게 다음 문자로 이동
    mtlMaxPrecisionFunc   maxPrecision;   // Language의 최대 precision계산
    mtlExtractFuncSet   * extractSet;     // Language에 맞는 Date Type 이름
    mtlNextDayFuncSet   * nextDaySet;     // Language에 맞는 요일 이름
    
    acp_uint8_t        ** specialCharSet;
    acp_uint8_t           specialCharSize;
};

struct mtvModule {
    const mtdModule* to;
    const mtdModule* from;
    acp_uint64_t            cost;
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
    acp_char_t    * mTypeName;
    acp_sint16_t    mDataType;
    acp_sint16_t    mODBCDataType; // mDataType을 ODBC SPEC으로 변경
    acp_sint32_t    mColumnSize;
    acp_char_t    * mLiteralPrefix;
    acp_char_t    * mLiteralSuffix;
    acp_char_t    * mCreateParam;
    acp_sint16_t    mNullable;
    acp_sint16_t    mCaseSens;
    acp_sint16_t    mSearchable;
    acp_sint16_t    mUnsignedAttr;
    acp_sint16_t    mFixedPrecScale;
    acp_sint16_t    mAutoUniqueValue;
    acp_char_t    * mLocalTypeName;
    acp_sint16_t    mMinScale;
    acp_sint16_t    mMaxScale;
    acp_sint16_t    mSQLDataType;
    acp_sint16_t    mODBCSQLDataType; // mSQLDataType을 ODBC SPEC으로 변경
    acp_sint16_t    mSQLDateTimeSub;
    acp_sint32_t    mNumPrecRadix;
    acp_sint16_t    mIntervalPrecision;
};

// PROJ-1755
typedef enum
{                                // format string에 대하여
    MTC_FORMAT_NULL = 1,         // null 혹은 ''로 구성
    MTC_FORMAT_NORMAL,           // 일반문자로만 구성
    MTC_FORMAT_UNDER,            // '_'로만 구성
    MTC_FORMAT_PERCENT,          // '%'로만 구성
    MTC_FORMAT_NORMAL_UNDER,     // 일반문자와 '_'로 구성
    MTC_FORMAT_NORMAL_PERCENT,   // 일반문자와 '%'로 구성
    MTC_FORMAT_UNDER_PERCENT,    // '_'와 '%'로 구성
    MTC_FORMAT_ALL               // 일반문자와 '_', '%'로 구성
} mtcLikeFormatType;

// PROJ-1755
struct mtcLikeFormatInfo
{
    // escape문자를 제거한 format pattern
    acp_uint8_t             * pattern;
    acp_uint16_t              patternSize;

    // format pattern의 분류정보
    mtcLikeFormatType   type;
    acp_uint16_t              underCnt;
    acp_uint16_t              percentCnt;
    acp_uint16_t              charCnt;

    // '%'가 1개인 경우의 부가정보
    // [head]%[tail]
    acp_uint8_t             * firstPercent;
    acp_uint8_t             * head;     // pattern에서의 head 위치
    acp_uint16_t              headSize;
    acp_uint8_t             * tail;     // pattern에서의 tail 위치
    acp_uint16_t              tailSize;
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



#endif /* _O_MTC_DEF_H_ */
