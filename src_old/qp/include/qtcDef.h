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
 * $Id: qtcDef.h 90192 2021-03-12 02:01:03Z jayce.park $
 **********************************************************************/

#include <qc.h>
#include <qmsParseTree.h>

#ifndef _O_QTC_DEF_H_
# define _O_QTC_DEF_H_ 1

/* qtcNode.node.flag                                */
/**********************************************************************
 [QTC_NODE_MASK]
 Node를 구성하는 하위 Node의 정보들 중
 다음과 같은 정보가 존재하는 지를 판별할 수 있는 Mask이다.
 즉, 다음과 같은 정보를 추출할 수 있다.
     - MTC_NODE_MASK : 인덱스, Binding, DML 정보
        - MTC_NODE_INDEX_MASK : 인덱스를 사용할 수 있는 Node가 존재함.
        - MTC_NODE_BIND_MASK : Binding이 필요한 Node가 존재함.
        - MTC_NODE_DML_MASK : DML을 수행할 수 없는 Node가 존재함.
     - QTC_NODE_AGGREGATE_MASK : Aggregation Node가 존재함.
     - QTC_NODE_AGGREGATE2_MASK : Nested Aggregation Node가 존재함.
     - QTC_NODE_SUBQUERY_MASK : Subquery Node가 존재함
     - QTC_NODE_SEQUNECE_MASK : Sequence Node가 존재함
     - QTC_NODE_PRIOR_MASK : PRIOR Column이 존재함
     - QTC_NODE_PROC_VAR_MASK : Procedure Variable이 존재함.
     - QTC_NODE_JOIN_OPERATOR_MASK : (+) Outer Join Operator 가 존재함. PROJ-1653
     - QTC_NODE_COLUMN_RID_MASK : RID Node가 존재함 (PROJ-1789)
     - QTC_NODE_SP_SYNONYM_FUNC_MASK : Synonym을 통해 Function에 접근
************************************************************************/

# define QTC_NODE_MASK  ( QTC_NODE_AGGREGATE_MASK     | \
                          QTC_NODE_AGGREGATE2_MASK    | \
                          QTC_NODE_SUBQUERY_MASK      | \
                          QTC_NODE_SEQUENCE_MASK      | \
                          QTC_NODE_PRIOR_MASK         | \
                          QTC_NODE_LEVEL_MASK         | \
                          QTC_NODE_ISLEAF_MASK        | \
                          QTC_NODE_ROWNUM_MASK        | \
                          QTC_NODE_SYSDATE_MASK       | \
                          QTC_NODE_PROC_VAR_MASK      | \
                          QTC_NODE_PROC_FUNCTION_MASK | \
                          QTC_NODE_BINARY_MASK        | \
                          QTC_NODE_TRANS_PRED_MASK    | \
                          QTC_NODE_VAR_FUNCTION_MASK  | \
                          QTC_NODE_LOB_COLUMN_MASK    | \
                          QTC_NODE_ANAL_FUNC_MASK     | \
                          QTC_NODE_JOIN_OPERATOR_MASK | \
                          QTC_NODE_COLUMN_RID_MASK    | \
                          QTC_NODE_PROC_FUNC_DETERMINISTIC_MASK | \
                          QTC_NODE_PKG_MEMBER_MASK    | \
                          QTC_NODE_LOOP_LEVEL_MASK    | \
                          QTC_NODE_LOOP_VALUE_MASK    | \
                          QTC_NODE_SP_SYNONYM_FUNC_MASK )

/* qtcNode.flag                                */
// Aggregation의 존재 여부
# define QTC_NODE_AGGREGATE_MASK          ID_ULONG(0x0000000000000001)
# define QTC_NODE_AGGREGATE_EXIST         ID_ULONG(0x0000000000000001)
# define QTC_NODE_AGGREGATE_ABSENT        ID_ULONG(0x0000000000000000)

/* qtcNode.flag                                */
// Nested Aggregation의 존재 여부
# define QTC_NODE_AGGREGATE2_MASK         ID_ULONG(0x0000000000000002)
# define QTC_NODE_AGGREGATE2_EXIST        ID_ULONG(0x0000000000000002)
# define QTC_NODE_AGGREGATE2_ABSENT       ID_ULONG(0x0000000000000000)

/* qtcNode.flag                                */
// Subquery의 존재 여부
# define QTC_NODE_SUBQUERY_MASK           ID_ULONG(0x0000000000000004)
# define QTC_NODE_SUBQUERY_EXIST          ID_ULONG(0x0000000000000004)
# define QTC_NODE_SUBQUERY_ABSENT         ID_ULONG(0x0000000000000000)

/* qtcNode.flag                                */
// Sequence의 존재 여부
# define QTC_NODE_SEQUENCE_MASK           ID_ULONG(0x0000000000000008)
# define QTC_NODE_SEQUENCE_EXIST          ID_ULONG(0x0000000000000008)
# define QTC_NODE_SEQUENCE_ABSENT         ID_ULONG(0x0000000000000000)

/* qtcNode.flag                                */
// PRIOR Pseudo Column의 존재 여부
# define QTC_NODE_PRIOR_MASK              ID_ULONG(0x0000000000000010)
# define QTC_NODE_PRIOR_EXIST             ID_ULONG(0x0000000000000010)
# define QTC_NODE_PRIOR_ABSENT            ID_ULONG(0x0000000000000000)

/* qtcNode.flag                                */
// LEVEL Pseudo Column의 존재 여부
# define QTC_NODE_LEVEL_MASK              ID_ULONG(0x0000000000000020)
# define QTC_NODE_LEVEL_EXIST             ID_ULONG(0x0000000000000020)
# define QTC_NODE_LEVEL_ABSENT            ID_ULONG(0x0000000000000000)

/* qtcNode.flag                                */
// fix BUG-10524
// SYSDATE pseudo Column의 존재 여부
# define QTC_NODE_SYSDATE_MASK            ID_ULONG(0x0000000000000040)
# define QTC_NODE_SYSDATE_EXIST           ID_ULONG(0x0000000000000040)
# define QTC_NODE_SYSDATE_ABSENT          ID_ULONG(0x0000000000000000)

/* qtcNode.flag                                */
// Procedure Variable 의 존재 여부
# define QTC_NODE_PROC_VAR_MASK           ID_ULONG(0x0000000000000080)
# define QTC_NODE_PROC_VAR_EXIST          ID_ULONG(0x0000000000000080)
# define QTC_NODE_PROC_VAR_ABSENT         ID_ULONG(0x0000000000000000)

/* qtcNode.flag                                */
// PROJ-1364
// indexable predicate 판단시,
// qmoPred::checkSameGroupType() 함수내에서
// 동일계열의 서로 다른 data type에 대해 index 사용가능한지를 판단하며,
// 이때, column node에 이 flag를 설정하게 된다.
// 이유는,
// qmoKeyRange::isIndexable() 함수내에서
//  (1) host 변수가 binding 된 후,
//  (2) sort temp table에 대한 keyRange 생성시,
// 동일계열의 index 사용가능한지를 판단하게 되며,
// 이때, prepare 단계에서 이미 판단된 predicate에 대해, 중복 검사하지 않기 위해
# define QTC_NODE_CHECK_SAMEGROUP_MASK   ID_ULONG(0x0000000000000100)
# define QTC_NODE_CHECK_SAMEGROUP_TRUE   ID_ULONG(0x0000000000000100)
# define QTC_NODE_CHECK_SAMEGROUP_FALSE  ID_ULONG(0x0000000000000000)

/* qtcNode.flag                                */
// To Fix PR-11391
// Internal Procedure Variable은 table의 column인지
// procedure의 variable인지 체크할 필요가 없음
#define QTC_NODE_INTERNAL_PROC_VAR_MASK   ID_ULONG(0x0000000000000200)
#define QTC_NODE_INTERNAL_PROC_VAR_EXIST  ID_ULONG(0x0000000000000200)
#define QTC_NODE_INTERNAL_PROC_VAR_ABSENT ID_ULONG(0x0000000000000000)

/* qtcNode.flag                                */
// to Fix BUG-12934
// constant filter에 대해서는
// subquery의 store and search 최적화팁을 적용하지 않는다.
// 예) ? in subquery, 1 in subquery
#define QTC_NODE_CONSTANT_FILTER_MASK     ID_ULONG(0x0000000000000400)
#define QTC_NODE_CONSTANT_FILTER_TRUE     ID_ULONG(0x0000000000000400)
#define QTC_NODE_CONSTANT_FILTER_FALSE    ID_ULONG(0x0000000000000000)

/* qtcNode.flag                                */
// To Fix PR-12743
// NNF Filter 여부를 설정
#define QTC_NODE_NNF_FILTER_MASK          ID_ULONG(0x0000000000000800)
#define QTC_NODE_NNF_FILTER_TRUE          ID_ULONG(0x0000000000000800)
#define QTC_NODE_NNF_FILTER_FALSE         ID_ULONG(0x0000000000000000)

/* qtcNode.flag                                */
// To Fix BUG-13939
// keyRange 생성시, In subquery or subquery keyRange일 경우,
// subquery를 수행할 수 있도록 하기 위해
// 노드변환후, subquery node가 연결된 비교연산자노드에 그 정보를 저장
#define QTC_NODE_SUBQUERY_RANGE_MASK      ID_ULONG(0x0000000000001000)
#define QTC_NODE_SUBQUERY_RANGE_TRUE      ID_ULONG(0x0000000000001000)
#define QTC_NODE_SUBQUERY_RANGE_FALSE     ID_ULONG(0x0000000000000000)

/* qtcNode.flag                                */
// PROJ-1075 PSM변수의 OUTBINDING 여부
#define QTC_NODE_OUTBINDING_MASK          ID_ULONG(0x0000000000002000)
#define QTC_NODE_OUTBINDING_ENABLE        ID_ULONG(0x0000000000002000)
#define QTC_NODE_OUTBINDING_DISABLE       ID_ULONG(0x0000000000000000)

/* qtcNode.flag                                */
// PROJ-1075 PSM변수의 LVALUE 여부
#define QTC_NODE_LVALUE_MASK              ID_ULONG(0x0000000000004000)
#define QTC_NODE_LVALUE_ENABLE            ID_ULONG(0x0000000000004000)
#define QTC_NODE_LVALUE_DISABLE           ID_ULONG(0x0000000000000000)

/* qtcNode.flag                                */
// fix BUG-14257
// node가 function인지의 정보
#define QTC_NODE_PROC_FUNCTION_MASK       ID_ULONG(0x0000000000008000)
#define QTC_NODE_PROC_FUNCTION_TRUE       ID_ULONG(0x0000000000008000)
#define QTC_NODE_PROC_FUNCTION_FALSE      ID_ULONG(0x0000000000000000)

/* qtcNode.flag                                */
// BUG-16000
// Equal연산이 불가능한 TYPE인지의 여부
// (Lob or Binary Type인 경우)
#define QTC_NODE_BINARY_MASK              ID_ULONG(0x0000000000010000)
#define QTC_NODE_BINARY_EXIST             ID_ULONG(0x0000000000010000)
#define QTC_NODE_BINARY_ABSENT            ID_ULONG(0x0000000000000000)

/* qtcNode.flag                                */
// qtcColumn의 estimate 여부 세팅.
#define QTC_NODE_COLUMN_ESTIMATE_MASK     ID_ULONG(0x0000000000020000)
#define QTC_NODE_COLUMN_ESTIMATE_TRUE     ID_ULONG(0x0000000000020000)
#define QTC_NODE_COLUMN_ESTIMATE_FALSE    ID_ULONG(0x0000000000000000)

/* qtcNode.flag                                */
// procedure variable이 estimate되었는지 여부
#define QTC_NODE_PROC_VAR_ESTIMATE_MASK   ID_ULONG(0x0000000000040000)
#define QTC_NODE_PROC_VAR_ESTIMATE_TRUE   ID_ULONG(0x0000000000040000)
#define QTC_NODE_PROC_VAR_ESTIMATE_FALSE  ID_ULONG(0x0000000000000000)

/* qtcNode.flag                                */
// ROWNUM Pseudo Column의 존재 여부
# define QTC_NODE_ROWNUM_MASK             ID_ULONG(0x0000000000080000)
# define QTC_NODE_ROWNUM_EXIST            ID_ULONG(0x0000000000080000)
# define QTC_NODE_ROWNUM_ABSENT           ID_ULONG(0x0000000000000000)

/* qtcNode.flag                                */
// 노드 변환이 발생했는지 여부
#define QTC_NODE_CONVERSION_MASK          ID_ULONG(0x0000000000100000)
#define QTC_NODE_CONVERSION_TRUE          ID_ULONG(0x0000000000100000)
#define QTC_NODE_CONVERSION_FALSE         ID_ULONG(0x0000000000000000)

/* qtcNode.flag                                */
// PROJ-1404
// transformation으로 생성된 transitive predicate인지 여부
# define QTC_NODE_TRANS_PRED_MASK         ID_ULONG(0x0000000000200000)
# define QTC_NODE_TRANS_PRED_EXIST        ID_ULONG(0x0000000000200000)
# define QTC_NODE_TRANS_PRED_ABSENT       ID_ULONG(0x0000000000000000)

/* qtcNode.flag                                */
// PROJ-1404
// random, sendmsg 같은 variable build-in 함수가 사용되었는지 여부
# define QTC_NODE_VAR_FUNCTION_MASK       ID_ULONG(0x0000000000400000)
# define QTC_NODE_VAR_FUNCTION_EXIST      ID_ULONG(0x0000000000400000)
# define QTC_NODE_VAR_FUNCTION_ABSENT     ID_ULONG(0x0000000000000000)

/* qtcNode.flag                                */
// PROJ-1413
// view merging에 의해 변환된 노드인지 여부
# define QTC_NODE_MERGED_COLUMN_MASK      ID_ULONG(0x0000000000800000)
# define QTC_NODE_MERGED_COLUMN_TRUE      ID_ULONG(0x0000000000800000)
# define QTC_NODE_MERGED_COLUMN_FALSE     ID_ULONG(0x0000000000000000)

/* qtcNode.flag                                */
// PROJ-1762
// analytic function에 속하는 node인지 여부 
# define QTC_NODE_ANAL_FUNC_COLUMN_MASK   ID_ULONG(0x0000000001000000)
# define QTC_NODE_ANAL_FUNC_COLUMN_FALSE  ID_ULONG(0x0000000000000000)
# define QTC_NODE_ANAL_FUNC_COLUMN_TRUE   ID_ULONG(0x0000000001000000)

/* qtcNode.flag                                */
// BUG-25916
// LOB Column의 존재 여부
# define QTC_NODE_LOB_COLUMN_MASK         ID_ULONG(0x0000000002000000)
# define QTC_NODE_LOB_COLUMN_EXIST        ID_ULONG(0x0000000002000000)
# define QTC_NODE_LOB_COLUMN_ABSENT       ID_ULONG(0x0000000000000000)

/* qtcNode.flag                                */
// BUG-27457
// Analytic function의 존재 여부
# define QTC_NODE_ANAL_FUNC_MASK          ID_ULONG(0x0000000004000000)
# define QTC_NODE_ANAL_FUNC_EXIST         ID_ULONG(0x0000000004000000)
# define QTC_NODE_ANAL_FUNC_ABSENT        ID_ULONG(0x0000000000000000)

/* qtcNode.flag                                */
// SP_arrayIndex_variable의 존재 여부
# define QTC_NODE_SP_ARRAY_INDEX_VAR_MASK    ID_ULONG(0x0000000008000000)
# define QTC_NODE_SP_ARRAY_INDEX_VAR_EXIST   ID_ULONG(0x0000000008000000)
# define QTC_NODE_SP_ARRAY_INDEX_VAR_ABSENT  ID_ULONG(0x0000000000000000)

/* qtcNode.lflag
 * PROJ-2586 PSM Parameters and return without precision
 * PSM 객체의 parameter node 또는 return node를 나타낸다.
 */
#define QTC_NODE_SP_PARAM_OR_RETURN_MASK                  ID_ULONG(0x0000000010000000)
#define QTC_NODE_SP_PARAM_OR_RETURN_TRUE                  ID_ULONG(0x0000000010000000)
#define QTC_NODE_SP_PARAM_OR_RETURN_FALSE                 ID_ULONG(0x0000000000000000)

/* qtcNode.lflag
 * PROJ-2586 PSM Parameters and return without precision
 * PSM 객체의 parameter의 precision의 표기여부를 나타낸다.
 *     ex) char    => QTC_NODE_SP_PARAM_OR_RETURN_PRECISION_ABSENT
 *         char(3) => QTC_NODE_SP_PARAM_OR_RETURN_PRECISION_EXIST
 */
#define QTC_NODE_SP_PARAM_OR_RETURN_PRECISION_MASK        ID_ULONG(0x0000000020000000)
#define QTC_NODE_SP_PARAM_OR_RETURN_PRECISION_ABSENT      ID_ULONG(0x0000000020000000)
#define QTC_NODE_SP_PARAM_OR_RETURN_PRECISION_EXIST       ID_ULONG(0x0000000000000000)

// qtcNode.flag
// PROJ-1653 Outer Join Operator (+)
# define QTC_NODE_JOIN_OPERATOR_MASK      ID_ULONG(0x0000000040000000)
# define QTC_NODE_JOIN_OPERATOR_EXIST     ID_ULONG(0x0000000040000000)
# define QTC_NODE_JOIN_OPERATOR_ABSENT    ID_ULONG(0x0000000000000000)

/* PROJ-1715 CONNECT_BY_ISLEAF Pseudo Column의 존재 여부 */
# define QTC_NODE_ISLEAF_MASK             ID_ULONG(0x0000000080000000)
# define QTC_NODE_ISLEAF_EXIST            ID_ULONG(0x0000000080000000)
# define QTC_NODE_ISLEAF_ABSENT           ID_ULONG(0x0000000000000000)

// qtcNode.flag
// PROJ-1789 PROIWD
#define QTC_NODE_COLUMN_RID_MASK          ID_ULONG(0x0000000100000000)
#define QTC_NODE_COLUMN_RID_EXIST         ID_ULONG(0x0000000100000000)
#define QTC_NODE_COLUMN_RID_ABSENT        ID_ULONG(0x0000000000000000)

// qtcNode.flag
// PROJ-1090 Function-based Index
// deterministic user defined function인지 여부
#define QTC_NODE_PROC_FUNC_DETERMINISTIC_MASK  ID_ULONG(0x0000000200000000)
#define QTC_NODE_PROC_FUNC_DETERMINISTIC_FALSE ID_ULONG(0x0000000200000000)
#define QTC_NODE_PROC_FUNC_DETERMINISTIC_TRUE  ID_ULONG(0x0000000000000000)

/* BUG-48336 */
#define QTC_NODE_TRANS_IN_TO_EXISTS_MASK       ID_ULONG(0x0000000400000000)
#define QTC_NODE_TRANS_IN_TO_EXISTS_TRUE       ID_ULONG(0x0000000400000000)
#define QTC_NODE_TRANS_IN_TO_EXISTS_FALSE      ID_ULONG(0x0000000000000000)

/* qtcNode.lflag
 * BUG-35674
 */
#define QTC_NODE_AGGR_ESTIMATE_MASK             ID_ULONG(0x0000000800000000)
#define QTC_NODE_AGGR_ESTIMATE_TRUE             ID_ULONG(0x0000000800000000)
#define QTC_NODE_AGGR_ESTIMATE_FALSE            ID_ULONG(0x0000000000000000)

/* qtcNode.lflag
 * PROJ-2197 PSM Renewal
 */
#define QTC_NODE_COLUMN_CONVERT_MASK            ID_ULONG(0x0000001000000000)
#define QTC_NODE_COLUMN_CONVERT_TRUE            ID_ULONG(0x0000001000000000)
#define QTC_NODE_COLUMN_CONVERT_FALSE           ID_ULONG(0x0000000000000000)

/* qtcNode.lflag
 * PROJ-1718 Subquery unnesting
 */
#define QTC_NODE_JOIN_TYPE_MASK                 ID_ULONG(0x0000006000000000)
#define QTC_NODE_JOIN_TYPE_INNER                ID_ULONG(0x0000000000000000)
#define QTC_NODE_JOIN_TYPE_SEMI                 ID_ULONG(0x0000002000000000)
#define QTC_NODE_JOIN_TYPE_ANTI                 ID_ULONG(0x0000004000000000)
#define QTC_NODE_JOIN_TYPE_UNDEFINED            ID_ULONG(0x0000006000000000)

/* qtcNode.lflag
 * BUG-37253
 */
#define QTC_NODE_PKG_VARIABLE_MASK              ID_ULONG(0x0000010000000000)
#define QTC_NODE_PKG_VARIABLE_TRUE              ID_ULONG(0x0000010000000000)
#define QTC_NODE_PKG_VARIABLE_FALSE             ID_ULONG(0x0000000000000000)

/* qtcNode.lflag
 * PROJ-2219 Row-level before update trigger
 */
#define QTC_NODE_VALIDATE_MASK                  ID_ULONG(0x0000020000000000)
#define QTC_NODE_VALIDATE_TRUE                  ID_ULONG(0x0000020000000000)
#define QTC_NODE_VALIDATE_FALSE                 ID_ULONG(0x0000000000000000)

/* qtcNode.lflag
 * PROJ-2415 Grouping Sets Clause
 */
#define QTC_NODE_EMPTY_GROUP_MASK               ID_ULONG(0x0000040000000000)
#define QTC_NODE_EMPTY_GROUP_TRUE               ID_ULONG(0x0000040000000000)
#define QTC_NODE_EMPTY_GROUP_FALSE              ID_ULONG(0x0000000000000000)

/* qtcNode.lflag
 * PROJ-2415 Grouping Sets Clause
 * Grouping Sets Transform으로 Target에 추가된 OrderBy의 Node를 나타낸다. 
 */
#define QTC_NODE_GBGS_ORDER_BY_NODE_MASK        ID_ULONG(0x0000080000000000)
#define QTC_NODE_GBGS_ORDER_BY_NODE_TRUE        ID_ULONG(0x0000080000000000)
#define QTC_NODE_GBGS_ORDER_BY_NODE_FALSE       ID_ULONG(0x0000000000000000)

/* qtcNode.lflag
 * PROJ-2210 Autoincrement column
 */
#define QTC_NODE_DEFAULT_VALUE_MASK             ID_ULONG(0x0000100000000000)
#define QTC_NODE_DEFAULT_VALUE_TRUE             ID_ULONG(0x0000100000000000)
#define QTC_NODE_DEFAULT_VALUE_FALSE            ID_ULONG(0x0000000000000000)

/* qtcNode.lflag
 * BUG-39770
 * package에 포함된 변수, subprogram이 사용된 Node를 나타낸다.
 */
#define QTC_NODE_PKG_MEMBER_MASK                ID_ULONG(0x0000200000000000)
#define QTC_NODE_PKG_MEMBER_EXIST               ID_ULONG(0x0000200000000000)
#define QTC_NODE_PKG_MEMBER_ABSENT              ID_ULONG(0x0000000000000000)

// BUG-41311
// loop_level pseudo column
#define QTC_NODE_LOOP_LEVEL_MASK                ID_ULONG(0x0000400000000000)
#define QTC_NODE_LOOP_LEVEL_EXIST               ID_ULONG(0x0000400000000000)
#define QTC_NODE_LOOP_LEVEL_ABSENT              ID_ULONG(0x0000000000000000)

// loop_value pseudo column
#define QTC_NODE_LOOP_VALUE_MASK                ID_ULONG(0x0000800000000000)
#define QTC_NODE_LOOP_VALUE_EXIST               ID_ULONG(0x0000800000000000)
#define QTC_NODE_LOOP_VALUE_ABSENT              ID_ULONG(0x0000000000000000)

/* qtcNode.lflag
 * BUG-41243
 * PSM Named-based Argument Passing에서, Parameter 이름을 저장한 Node를 나타낸다.
 */
#define QTC_NODE_SP_PARAM_NAME_MASK             ID_ULONG(0x0001000000000000)
#define QTC_NODE_SP_PARAM_NAME_TRUE             ID_ULONG(0x0001000000000000)
#define QTC_NODE_SP_PARAM_NAME_FALSE            ID_ULONG(0x0000000000000000)

/* qtcNode.lflag
 * BUG-41228
 * PSM parameter의 default에 대한 validation 수행 중임을 나타낸다..
 */
#define QTC_NODE_SP_PARAM_DEFAULT_VALUE_MASK    ID_ULONG(0x0002000000000000)
#define QTC_NODE_SP_PARAM_DEFAULT_VALUE_TRUE    ID_ULONG(0x0002000000000000)
#define QTC_NODE_SP_PARAM_DEFAULT_VALUE_FALSE   ID_ULONG(0x0000000000000000)

/* qtcNode.lflag
 * BUG-36728 Check Constraint, Function-Based Index에서 Synonym을 사용할 수 없어야 합니다.
 */
#define QTC_NODE_SP_SYNONYM_FUNC_MASK           ID_ULONG(0x0004000000000000)
#define QTC_NODE_SP_SYNONYM_FUNC_TRUE           ID_ULONG(0x0004000000000000)
#define QTC_NODE_SP_SYNONYM_FUNC_FALSE          ID_ULONG(0x0000000000000000)

// BUG-44518 order by 구문의 ESTIMATE 중복 수행하면 안됩니다.
#define QTC_NODE_ORDER_BY_ESTIMATE_MASK         ID_ULONG(0x0008000000000000)
#define QTC_NODE_ORDER_BY_ESTIMATE_TRUE         ID_ULONG(0x0008000000000000)
#define QTC_NODE_ORDER_BY_ESTIMATE_FALSE        ID_ULONG(0x0000000000000000)

// BUG-45172 semi 조인을 제거할 조건을 검사하여 flag를 설정해 둔다.
// 상위에 서브쿼리가 semi 조인일 경우 flag 를 보고 하위 semi 조인을 제거함
#define QTC_NODE_REMOVABLE_SEMI_JOIN_MASK       ID_ULONG(0x0010000000000000)
#define QTC_NODE_REMOVABLE_SEMI_JOIN_TRUE       ID_ULONG(0x0010000000000000)
#define QTC_NODE_REMOVABLE_SEMI_JOIN_FALSE      ID_ULONG(0x0000000000000000)

/* PROJ-2687 Shard aggregation transform */
#define QTC_NODE_SHARD_VIEW_TARGET_REF_MASK     ID_ULONG(0x0020000000000000)
#define QTC_NODE_SHARD_VIEW_TARGET_REF_TRUE     ID_ULONG(0x0020000000000000)
#define QTC_NODE_SHARD_VIEW_TARGET_REF_FALSE    ID_ULONG(0x0000000000000000)

/* BUG-46032 */
#define QTC_NODE_SP_NESTED_ARRAY_MASK          ID_ULONG(0x0040000000000000)
#define QTC_NODE_SP_NESTED_ARRAY_TRUE          ID_ULONG(0x0040000000000000)
#define QTC_NODE_SP_NESTED_ARRAY_FALSE         ID_ULONG(0x0000000000000000)

/* BUG-46174 */
#define QTC_NODE_SP_INS_UPT_VALUE_REC_MASK     ID_ULONG(0x0080000000000000)
#define QTC_NODE_SP_INS_UPT_VALUE_REC_TRUE     ID_ULONG(0x0080000000000000)
#define QTC_NODE_SP_INS_UPT_VALUE_REC_FALSE    ID_ULONG(0x0000000000000000)

// /* TASK-7219 Non-shard DML */
#define QTC_NODE_OUT_REF_COLUMN_MASK           ID_ULONG(0x0100000000000000)
#define QTC_NODE_OUT_REF_COLUMN_TRUE           ID_ULONG(0x0100000000000000)
#define QTC_NODE_OUT_REF_COLUMN_FALSE          ID_ULONG(0x0000000000000000)

# define QTC_HAVE_AGGREGATE( aNode ) \
    ( ( (aNode)->lflag & QTC_NODE_AGGREGATE_MASK )           \
            == QTC_NODE_AGGREGATE_EXIST ? ID_TRUE : ID_FALSE )

# define QTC_HAVE_AGGREGATE2( aNode ) \
    ( ( aNode->lflag & QTC_NODE_AGGREGATE2_MASK )           \
            == QTC_NODE_AGGREGATE2_EXIST ? ID_TRUE : ID_FALSE )

# define QTC_IS_AGGREGATE( aNode ) \
    ( ( (aNode)->node.lflag & MTC_NODE_OPERATOR_MASK )           \
            == MTC_NODE_OPERATOR_AGGREGATION ? ID_TRUE : ID_FALSE )

# define QTC_HAVE_SUBQUERY( aNode ) \
    ( ( (aNode)->lflag & QTC_NODE_SUBQUERY_MASK )           \
            == QTC_NODE_SUBQUERY_EXIST ? ID_TRUE : ID_FALSE )

# define QTC_IS_SUBQUERY( aNode ) \
    ( ( (aNode)->node.lflag & MTC_NODE_OPERATOR_MASK )           \
            == MTC_NODE_OPERATOR_SUBQUERY ? ID_TRUE : ID_FALSE )

# define QTC_INDEXABLE_COLUMN( aNode ) \
    ( ( ( (aNode)->node.lflag & MTC_NODE_INDEX_MASK ) == MTC_NODE_INDEX_USABLE ) \
      ? ID_TRUE : ID_FALSE )

// BUG-16730
// Subquery도 target count가 2이상이면 List임
# define QTC_IS_LIST( aNode ) \
    ( ( ( (aNode)->node.module == &mtfList ) || \
        ( ( (aNode)->node.module == &qtc::subqueryModule ) && \
          ( ((aNode)->node.lflag & MTC_NODE_ARGUMENT_COUNT_MASK) > 1 ) ) ) \
      ? ID_TRUE : ID_FALSE )

// BUG-17506
# define QTC_IS_VARIABLE( aNode )                               \
    (                                                           \
       (  ( ( (aNode)->lflag & QTC_NODE_SUBQUERY_MASK )         \
            == QTC_NODE_SUBQUERY_EXIST )                        \
          ||                                                    \
          ( ( (aNode)->lflag & QTC_NODE_PRIOR_MASK )            \
            == QTC_NODE_PRIOR_EXIST )                           \
          ||                                                    \
          ( ( (aNode)->lflag & QTC_NODE_LEVEL_MASK )            \
            == QTC_NODE_LEVEL_EXIST )                           \
          ||                                                    \
          ( ( (aNode)->lflag & QTC_NODE_ROWNUM_MASK )           \
            == QTC_NODE_ROWNUM_EXIST )                          \
          ||                                                    \
          ( ( (aNode)->lflag & QTC_NODE_SYSDATE_MASK )          \
            == QTC_NODE_SYSDATE_EXIST )                         \
          ||                                                    \
          ( ( (aNode)->lflag & QTC_NODE_PROC_VAR_MASK )         \
            == QTC_NODE_PROC_VAR_EXIST )                        \
          ||                                                    \
           ( ( (aNode)->node.lflag & MTC_NODE_BIND_MASK )       \
            == MTC_NODE_BIND_EXIST )                            \
          ||                                                    \
           ( ( ( (aNode)->lflag & QTC_NODE_PROC_FUNCTION_MASK )           \
               == QTC_NODE_PROC_FUNCTION_TRUE ) &&                        \
             ( ( (aNode)->lflag & QTC_NODE_PROC_FUNC_DETERMINISTIC_MASK ) \
               == QTC_NODE_PROC_FUNC_DETERMINISTIC_FALSE ) )              \
          ||                                                    \
           ( ( (aNode)->lflag & QTC_NODE_VAR_FUNCTION_MASK )    \
            == QTC_NODE_VAR_FUNCTION_EXIST )                    \
          ||                                                    \
           ( ( (aNode)->lflag & QTC_NODE_ISLEAF_MASK )          \
            == QTC_NODE_ISLEAF_EXIST )                          \
          ||                                                    \
           ( ( (aNode)->lflag & QTC_NODE_LOOP_LEVEL_MASK )      \
            == QTC_NODE_LOOP_LEVEL_EXIST )                      \
          ||                                                    \
           ( ( (aNode)->lflag & QTC_NODE_LOOP_VALUE_MASK )      \
            == QTC_NODE_LOOP_VALUE_EXIST )                      \
       )                                                        \
       ? ID_TRUE : ID_FALSE                                     \
    )

// BUG-17506
#define QTC_IS_DYNAMIC_CONSTANT( aNode )                       \
    (                                                          \
       (                                                       \
          (  ( ( (aNode)->lflag & QTC_NODE_SYSDATE_MASK )      \
               == QTC_NODE_SYSDATE_EXIST )                     \
             ||                                                \
             ( ( (aNode)->lflag & QTC_NODE_PROC_VAR_MASK )     \
               == QTC_NODE_PROC_VAR_EXIST )                    \
             ||                                                \
              ( ( (aNode)->node.lflag & MTC_NODE_BIND_MASK )   \
               == MTC_NODE_BIND_EXIST )                        \
          )                                                    \
          &&                                                   \
          (  ( ( (aNode)->lflag & QTC_NODE_PRIOR_MASK )        \
               == QTC_NODE_PRIOR_ABSENT )                      \
          )                                                    \
          &&                                                   \
          (  ( ( (aNode)->lflag & QTC_NODE_LEVEL_MASK )        \
               == QTC_NODE_LEVEL_ABSENT )                      \
          )                                                    \
          &&                                                   \
          (  ( ( (aNode)->lflag & QTC_NODE_ROWNUM_MASK )       \
               == QTC_NODE_ROWNUM_ABSENT )                     \
          )                                                    \
       )                                                       \
       ? ID_TRUE : ID_FALSE                                    \
    )

// BUG-17949
# define QTC_IS_PSEUDO( aNode )                                \
    (                                                          \
       (  ( ( (aNode)->lflag & QTC_NODE_LEVEL_MASK )           \
            == QTC_NODE_LEVEL_EXIST )                          \
          ||                                                   \
          ( ( (aNode)->lflag & QTC_NODE_ROWNUM_MASK )          \
            == QTC_NODE_ROWNUM_EXIST )                         \
          ||                                                   \
          ( ( (aNode)->lflag & QTC_NODE_ISLEAF_MASK )          \
            == QTC_NODE_ISLEAF_EXIST )                         \
       )                                                       \
       ? ID_TRUE : ID_FALSE                                    \
    )


/******************************************************************
 [Column 여부의 판단]
 기존 A3에서는 아래와 같이 판단하였으나,
 이는 실제 Table내의 Column인지를 판단하는 명확한
 기준이 되지 않는다.
 이러한 처리는 Predicate의 Index사용 가능 여부를
 보다 쉽게 판단할 수 있는 장점은 있으나, Conversion이
 발생하는 Column들에 대하여 Column인지를 판단하지 못하고
 있어 다양한 최적화 Tip 적용에 문제가 되고 있다.(Ex, PR-7960)
 따라서, 실제 Column인지 아닌지만을 판단할 수 있도록
 해당 Macro를 수정한다.  즉, FROM절에 존재하는 Table의
 Column인지를 보고 판단한다.
******************************************************************/

/******************************************************************
# define QTC_IS_COLUMN( aNode )                                 \
    ( ( ( aNode->node.lflag & MTC_NODE_INDEX_MASK )             \
        == MTC_NODE_INDEX_USABLE )                              \
      && aNode->node.arguments == NULL ? ID_TRUE : ID_FALSE )
******************************************************************/
# define QTC_IS_COLUMN( aStatement, aNode ) \
    ( QC_SHARED_TMPLATE(aStatement)->tableMap[(aNode)->node.table].from \
      != NULL ? ID_TRUE : ID_FALSE )

# define QTC_TEMPLATE_IS_COLUMN( aTemplate, aNode ) \
    ( (aTemplate)->tableMap[(aNode)->node.table].from   \
      != NULL ? ID_TRUE : ID_FALSE )

# define QTC_IS_TERMINAL( aNode ) \
    ( (aNode)->node.arguments == NULL ? ID_TRUE : ID_FALSE )

# define QTC_COMPOSITE_EQUAL_CLASSIFIED( aNode ) \
    ( \
      (( (aNode)->node.lflag & MTC_NODE_GROUP_COMPARISON_MASK ) \
       == MTC_NODE_GROUP_COMPARISON_FALSE) \
     && \
     ( \
      (( (aNode)->node.lflag & MTC_NODE_OPERATOR_MASK ) \
       == MTC_NODE_OPERATOR_EQUAL) \
       )\
    )

# define QTC_COMPOSITE_INEQUAL_EQUAL_CLASSIFIED( aNode ) \
    ( \
      (( (aNode)->node.lflag & MTC_NODE_GROUP_COMPARISON_MASK ) \
       == MTC_NODE_GROUP_COMPARISON_FALSE) \
     && \
     ( \
      (( (aNode)->node.lflag & MTC_NODE_OPERATOR_MASK ) \
       == MTC_NODE_OPERATOR_GREATER_EQUAL) \
      || \
      (( (aNode)->node.lflag & MTC_NODE_OPERATOR_MASK ) \
       == MTC_NODE_OPERATOR_LESS_EQUAL) \
      ) \
     )

# define QTC_COMPOSITE_INEQUAL_CLASSIFIED( aNode ) \
    ( \
      (( (aNode)->node.lflag & MTC_NODE_GROUP_COMPARISON_MASK ) \
       == MTC_NODE_GROUP_COMPARISON_FALSE) \
     && \
     ( \
      (( (aNode)->node.lflag & MTC_NODE_OPERATOR_MASK ) \
       == MTC_NODE_OPERATOR_GREATER ) \
      || \
      (( (aNode)->node.lflag & MTC_NODE_OPERATOR_MASK ) \
       == MTC_NODE_OPERATOR_LESS ) \
      ) \
     )

// dependencies constants
# define QTC_DEPENDENCIES_END      (-1)

/******************************************************************
 macros for readability of source code.
 naming convension is shown below.
******************************************************************/
#define QTC_NODE_TABLE(_QtcNode_)  ( (_QtcNode_)->node.table )
#define QTC_NODE_COLUMN(_QtcNode_) ( (_QtcNode_)->node.column )
#define QTC_NODE_TMPLATE(_QtcNode_) ( (_QtcNode_)->node.tmplate)

/***********************************************************************
 QTC_XXXX_YYYY : get YYYY contained in XXXX pointed by qtcNode.table,column
                 argument 1 ==> (XXXX *)
                 argument 2 ==> (qtcNode *)
                 return     ==> (YYYY *)
***********************************************************************/
// return mtcTuple *
#define QTC_TMPL_TUPLE( _QcTmpl_, _QtcNode_ )   \
    (  ( (_QcTmpl_)-> tmplate. rows ) +         \
       ( (_QtcNode_)-> node.table )  )

// return mtcTuple *
#define QTC_STMT_TUPLE( _QcStmt_, _QtcNode_ )           \
    (  ( QC_SHARED_TMPLATE(_QcStmt_)-> tmplate. rows) +        \
       ( (_QtcNode_)-> node.table )  )

// return mtcColumn *
#define QTC_TUPLE_COLUMN(_MtcTuple_, _QtcNode_)           \
    (((_QtcNode_)->node.column != MTC_RID_COLUMN_ID)  ?   \
    ((_MtcTuple_)->columns)+(_QtcNode_)->node.column  :   \
    (&gQtcRidColumn))

// if you are accessing columns more than once in a function,
// use QTC_XXXX_TUPLE and QTC_TUPLE_COLUMN.
// return mtcColumn *
#define QTC_TMPL_COLUMN( _QcTmpl_, _QtcNode_ )                          \
    ( QTC_TUPLE_COLUMN( QTC_TMPL_TUPLE( (_QcTmpl_), (_QtcNode_) ),      \
                        (_QtcNode_) ) )
// if you are accessing columns more than once in a function,
// use QTC_XXXX_TUPLE and QTC_TUPLE_COLUMN.
// return mtcColumn *
#define QTC_STMT_COLUMN( _QcStmt_, _QtcNode_ )                          \
    ( QTC_TUPLE_COLUMN( QTC_STMT_TUPLE( (_QcStmt_), (_QtcNode_) ),      \
                        (_QtcNode_) ) )

// return mtcExecute *
#define QTC_TUPLE_EXECUTE( _MtcTuple_, _QtcNode_ )       \
    (((_QtcNode_)->node.column != MTC_RID_COLUMN_ID)   ? \
    ((_MtcTuple_)->execute) + (_QtcNode_)->node.column : \
    (&gQtcRidExecute))

// if you are accessing execute pointers more than once in a function,
// use QTC_XXXX_TUPLE and QTC_TUPLE_EXECUTE.
// return mtcExecute *
#define QTC_TMPL_EXECUTE( _QcTmpl_, _QtcNode_ )                         \
    ( QTC_TUPLE_EXECUTE( QTC_TMPL_TUPLE( (_QcTmpl_), (_QtcNode_) ),     \
                         (_QtcNode_) ) )

// if you are accessing execute pointers more than once in a function,
// use QTC_XXXX_TUPLE and QTC_TUPLE_EXECUTE.
// return mtcExecute *
#define QTC_STMT_EXECUTE( _QcStmt_, _QtcNode_ )                         \
    ( QTC_TUPLE_EXECUTE( QTC_STMT_TUPLE( (_QcStmt_), (_QtcNode_) ),     \
                         (_QtcNode_) ) )


#define QTC_TUPLE_FIXEDDATA( _MtcTuple_, _QtcNode_ )            \
    ( ( (SChar*) (_MtcTuple_)-> row ) +                         \
      QTC_TUPLE_COLUMN(_MtcTuple_, _QtcNode_)->column.offset )

// if you are accessing execute pointers more than once in a function,
// use QTC_XXXX_TUPLE and QTC_TUPLE_EXECUTE.
// return SChar *
#define QTC_TMPL_FIXEDDATA( _QcTmpl_, _QtcNode_ )                       \
    ( QTC_TUPLE_FIXEDDATA( QTC_TMPL_TUPLE( (_QcTmpl_), (_QtcNode_) ),   \
                           (_QtcNode_) ) )

// if you are accessing execute pointers more than once in a function,
// use QTC_XXXX_TUPLE and QTC_TUPLE_EXECUTE.
// return SChar *
#define QTC_STMT_FIXEDDATA( _QcStmt_, _QtcNode_ )                       \
    ( QTC_TUPLE_FIXEDDATA( QTC_STMT_TUPLE( (_QcStmt_), (_QtcNode_) ),   \
                           (_QtcNode_) ) )


// operation code to qtc::makeProcVariable
#define QTC_PROC_VAR_OP_NONE           (0x00000000)
#define QTC_PROC_VAR_OP_NEXT_COLUMN    (0x00000001)
#define QTC_PROC_VAR_OP_SKIP_MODULE    (0x00000002)

/******************************************************************
 Subquery Wrapper Node, Constant Wrapper Node등의
 최초 수행 여부를 판단하기 위한 mask이다.
 mtcTemplate.execInfo 의 영역에 이 값을 Setting한다.
******************************************************************/

#define QTC_WRAPPER_NODE_EXECUTE_MASK          (0x11)
#define QTC_WRAPPER_NODE_EXECUTE_FALSE         (0x00)
#define QTC_WRAPPER_NODE_EXECUTE_TRUE          (0x01)

/***********************************************************************
   structures.
***********************************************************************/

/******************************************************************
 [qtcNode]

   - indexArgument :
       - used in comparison predicate( for detecting indexing column )
       - expand function for ( qmsTarget, outer reference column flag )
       - expand function for dummyAggs
                aggs1 aggregation used by aggs2. ( distinguish from pure aggs1  )
******************************************************************/

struct qtcOver;
struct qtcWithinGroup;

typedef struct qtcNode {
    mtcNode            node;
    ULong              lflag;
    qcStatement*       subquery;
    UInt               indexArgument;
    UInt               dependency;
    qcDepInfo          depInfo;
    qcNamePosition     position;
    qcNamePosition     userName;
    qcNamePosition     tableName;
    qcNamePosition     columnName;
    qcNamePosition     pkgName;             // PROJ-1073 Package
    qtcOver          * overClause;          // PROJ-1762 : for analytic function
    UShort             shardViewTargetPos;  // PROJ-2687 Shard aggregation transform
} qtcNode;


#define QTC_NODE_INIT(_node_)                  \
{                                              \
    MTC_NODE_INIT( & (_node_)->node );         \
    (_node_)->lflag         = 0;               \
    (_node_)->subquery      = NULL;            \
    (_node_)->indexArgument = 0;               \
    (_node_)->dependency    = 0;               \
    qtc::dependencyClear( &(_node_)->depInfo );\
    SET_EMPTY_POSITION((_node_)->position);    \
    SET_EMPTY_POSITION((_node_)->userName);    \
    SET_EMPTY_POSITION((_node_)->tableName);   \
    SET_EMPTY_POSITION((_node_)->columnName);  \
    SET_EMPTY_POSITION((_node_)->pkgName);     \
    (_node_)->overClause    = NULL;            \
    (_node_)->shardViewTargetPos = ID_USHORT_MAX; \
}

//---------------------------------
// PROJ-1762 : Analytic Function 
//---------------------------------

// for qtcOverColumn.flag
// BUG-33663 Ranking Function
#define QTC_OVER_COLUMN_MASK                    (0x00000001)
#define QTC_OVER_COLUMN_NORMAL                  (0x00000000)
#define QTC_OVER_COLUMN_ORDER_BY                (0x00000001)

// for qtcOverColumn.flag
// BUG-33663 Ranking Function
#define QTC_OVER_COLUMN_ORDER_MASK              (0x00000002)
#define QTC_OVER_COLUMN_ORDER_ASC               (0x00000000)
#define QTC_OVER_COLUMN_ORDER_DESC              (0x00000002)

/* PROJ-2435 order by nulls first/ last */
#define QTC_OVER_COLUMN_NULLS_ORDER_MASK        (0x0000000C)
#define QTC_OVER_COLUMN_NULLS_ORDER_NONE        (0x00000000)
#define QTC_OVER_COLUMN_NULLS_ORDER_FIRST       (0x00000004)
#define QTC_OVER_COLUMN_NULLS_ORDER_LAST        (0x00000008)

typedef struct qtcOverColumn
{
    qtcNode         * node;
    UInt              flag;
    
    qtcOverColumn   * next;
} qtcOverColumn;

/* PROJ-1805 Support window clause */
typedef enum qtcOverWindow
{
    QTC_OVER_WINDOW_NONE = 0,
    QTC_OVER_WINODW_ROWS,
    QTC_OVER_WINODW_RANGE
} qtcOverWindow;

typedef enum qtcOverWindowOpt
{
    QTC_OVER_WINODW_OPT_UNBOUNDED_PRECEDING = 0,
    QTC_OVER_WINODW_OPT_UNBOUNDED_FOLLOWING,
    QTC_OVER_WINODW_OPT_CURRENT_ROW,
    QTC_OVER_WINODW_OPT_N_PRECEDING,
    QTC_OVER_WINODW_OPT_N_FOLLOWING,
    QTC_OVER_WINDOW_OPT_MAX
} qtcOverWindowOpt;

typedef enum qtcOverWindowValueType
{
    QTC_OVER_WINDOW_VALUE_TYPE_NUMBER = 0,
    QTC_OVER_WINDOW_VALUE_TYPE_YEAR,
    QTC_OVER_WINDOW_VALUE_TYPE_MONTH,
    QTC_OVER_WINDOW_VALUE_TYPE_DAY,
    QTC_OVER_WINDOW_VALUE_TYPE_HOUR,
    QTC_OVER_WINDOW_VALUE_TYPE_MINUTE,
    QTC_OVER_WINDOW_VALUE_TYPE_SECOND,
    QTC_OVER_WINDOW_VALUE_TYPE_MAX
} qtcOverWindowValueType;

typedef struct qtcWindowValue
{
    SLong                  number;
    qtcOverWindowValueType type;
} qtcWindowValue;

typedef struct qtcWindowPoint
{
    qtcWindowValue   * value;
    qtcOverWindowOpt   option;
} qtcWindowPoint;

typedef struct qtcWindow
{
    qtcOverWindow     rowsOrRange;
    idBool            isBetween;
    qtcWindowPoint  * start;
    qtcWindowPoint  * end;
} qtcWindow;

//---------------------------------
// BUG-33663 Ranking Function
// rank() over (partition by i1, i2 order by i3)의 경우
// qtcOver의 각 member는 다음과 같이 구성한다.
//
// overColumn ---------->i1->i2->i3->null
//                       /       /
// partitionByColumn ----       /
// orderByColumn ---------------
//
// 사실 partitionByColumn이나 oderByColumn은 단순히 각 절이
// 존재하는지를 표현할 뿐이다. validation이외에는 사용되지 않는다.
//---------------------------------
typedef struct qtcOver
{
    qcNamePosition    overPosition;       // BUG-42337
    qtcOverColumn   * overColumn;         // partition by, order by column list
    
    qtcOverColumn   * partitionByColumn;  // partition by column list
    qtcOverColumn   * orderByColumn;      // order by column list
    qtcWindow       * window;
    qcNamePosition    endPos;
} qtcOver;

// PROJ-2527 WITHIN GROUP AGGR
typedef struct qtcWithinGroup
{
    qtcNode        * expression[2];
    qcNamePosition   withinPosition;    // PROJ-2533
    qcNamePosition   endPos;
} qtcWithinGroup;

// PROJ-2528 Keep Aggregation
typedef struct qtcKeepAggr
{
    qtcNode        * mExpression[2];
    qcNamePosition   mKeepPosition;
    qcNamePosition   mEndPos;
    UChar            mOption;
} qtcKeepAggr;

//---------------------------------
// Filter의 CallBack 호출을 위한 Data
//---------------------------------
typedef struct qtcSmiCallBackData {
    mtcNode*         node;           // Node
    mtcTemplate*     tmplate;        // Template
    mtcTuple*        table;          // 접근할 데이터의 Tuple
    mtcTuple*        tuple;          // Node의 Tuple
    mtfCalculateFunc calculate;      // Filter 수행 함수
    void*            calculateInfo;  // Filter 수행 함수를 위한 정보
    mtcColumn*       column;         // Node의 Column 정보
    mtdIsTrueFunc    isTrue;         // TRUE 판단 함수
} qtcSmiCallBackData;

typedef struct qtcSmiCallBackDataAnd {
    qtcSmiCallBackData  * argu1;
    qtcSmiCallBackData  * argu2;
    qtcSmiCallBackData  * argu3;
} qtcSmiCallBackDataAnd;

/******************************************************************
 [qtcCallBackInfo]

 estimate 의 처리 시
 mtcCallBack의 처리를 위해 사용되는 부가 정보로
 mtcCallBack.info 에 설정된다.
******************************************************************/


typedef struct qtcCallBackInfo {
    qcTemplate*         tmplate;
    iduVarMemList*      memory;         // alloc시 사용할 Memory Mgr
                                        //fix PROJ-1596
    qcStatement*        statement;
    struct qmsQuerySet* querySet;       // for search column in order by
    struct qmsSFWGH*    SFWGH;          // for search column
    struct qmsFrom*     from;           // for search column
} qtcCallBackInfo;

typedef struct qtcMetaRangeColumn {
    qtcMetaRangeColumn* next;
    mtdCompareFunc      compare;
    
    // PROJ-1436
    // columnDesc, valueDesc는 shared template의 column을
    // 사용해서는 안된다. variable type column인 경우
    // 데이터 오염이 발생할 수 있다. 따라서 column 정보를
    // 복사해서 사용한다.
    // 추후 private template의 column 정보를 사용할 수
    // 있도록 수정해야 한다.
    mtcColumn           columnDesc;
    mtcColumn           valueDesc;
    const void*         value;
    // Proj-1872 DiskIndex 저장구조 최적화
    // Stored타입의 Column을 비교하기 위해 Column 인덱스를 저장해야 한다.
    UInt                columnIdx;
} qtcMetaRangeColumn;

/******************************************************************
 [qtcModule] PROJ-1075 PSM structured type 지원

 row type, record type, collection type의 처리 시
 mtdModule이외의 qp에서 사용하는 부가정보를 이용하기 위해
 사용한다.
******************************************************************/

// PROJ-1904 Extend UDT
#define QTC_UD_TYPE_HAS_ARRAY_MASK  (0x00000001)
#define QTC_UD_TYPE_HAS_ARRAY_TRUE  (0x00000001)
#define QTC_UD_TYPE_HAS_ARRAY_FALSE (0x00000000)

struct qsTypes;

typedef struct qtcModule
{
    mtdModule       module;
    qsTypes       * typeInfo;
} qtcModule;

typedef struct qtcColumnInfo
{
    UShort           table;
    UShort           column;
    smOID            objectId;         // PROJ-1073 Package
} qtcColumnInfo;

/* PROJ-2207 Password policy support */
#define QTC_SYSDATE_FOR_NATC_LEN (20)

extern IDE_RC qtcSubqueryCalculateExists( mtcNode     * aNode,
                                          mtcStack    * aStack,
                                          SInt          aRemain,
                                          void        * aInfo,
                                          mtcTemplate * aTemplate );

extern IDE_RC qtcSubqueryCalculateNotExists( mtcNode     * aNode,
                                             mtcStack    * aStack,
                                             SInt          aRemain,
                                             void        * aInfo,
                                             mtcTemplate * aTemplate );

/* PROJ-2632 */
#define QTC_ENTRY_TYPE_NONE          (MTX_ENTRY_TYPE_NONE)
#define QTC_ENTRY_TYPE_VALUE         (MTX_ENTRY_TYPE_VALUE)
#define QTC_ENTRY_TYPE_RID           (MTX_ENTRY_TYPE_RID)
#define QTC_ENTRY_TYPE_COLUMN        (MTX_ENTRY_TYPE_COLUMN)
#define QTC_ENTRY_TYPE_FUNCTION      (MTX_ENTRY_TYPE_FUNCTION)
#define QTC_ENTRY_TYPE_SINGLE        (MTX_ENTRY_TYPE_SINGLE)
#define QTC_ENTRY_TYPE_CONVERT       (MTX_ENTRY_TYPE_CONVERT)
#define QTC_ENTRY_TYPE_CONVERT_CHAR  (MTX_ENTRY_TYPE_CONVERT_CHAR)
#define QTC_ENTRY_TYPE_CONVERT_DATE  (MTX_ENTRY_TYPE_CONVERT_DATE)
#define QTC_ENTRY_TYPE_CHECK         (MTX_ENTRY_TYPE_CHECK)
#define QTC_ENTRY_TYPE_AND           (MTX_ENTRY_TYPE_AND)
#define QTC_ENTRY_TYPE_OR            (MTX_ENTRY_TYPE_OR)
#define QTC_ENTRY_TYPE_AND_SINGLE    (MTX_ENTRY_TYPE_AND_SINGLE)
#define QTC_ENTRY_TYPE_OR_SINGLE     (MTX_ENTRY_TYPE_OR_SINGLE)
#define QTC_ENTRY_TYPE_NOT           (MTX_ENTRY_TYPE_NOT)
#define QTC_ENTRY_TYPE_LNNVL         (MTX_ENTRY_TYPE_LNNVL)

#define QTC_ENTRY_SIZE_ZERO          (MTX_ENTRY_SIZE_ZERO)
#define QTC_ENTRY_SIZE_ONE           (MTX_ENTRY_SIZE_ONE)
#define QTC_ENTRY_SIZE_TWO           (MTX_ENTRY_SIZE_TWO)
#define QTC_ENTRY_SIZE_THREE         (MTX_ENTRY_SIZE_THREE)

#define QTC_ENTRY_COUNT_ZERO         (MTX_ENTRY_COUNT_ZERO)
#define QTC_ENTRY_COUNT_ONE          (MTX_ENTRY_COUNT_ONE)
#define QTC_ENTRY_COUNT_TWO          (MTX_ENTRY_COUNT_TWO)
#define QTC_ENTRY_COUNT_THREE        (MTX_ENTRY_COUNT_THREE)

#define QTC_SET_SERIAL_FILTER_INFO( _info_ ,     \
                                    _node_ ,     \
                                    _execute_ ,  \
                                    _offset_,    \
                                    _left_,      \
                                    _right_)     \
    MTX_SET_SERIAL_FILTER_INFO( ( _info_ ),      \
                                ( _node_ ),      \
                                ( _execute_ ),   \
                                ( _offset_ ),    \
                                ( _left_ ),      \
                                ( _right_ ) )

#define QTC_SET_ENTRY_HEADER( _header_,  \
                              _id_,      \
                              _type_,    \
                              _size_,    \
                              _count_ )  \
    MTX_SET_ENTRY_HEADER( ( _header_ ),  \
                          ( _id_ ),      \
                          ( _type_ ),    \
                          ( _size_ ),    \
                          ( _count_ ) )

#define QTC_SERIAL_EXEUCTE_DATA_INITIALIZE( _data_ ,     \
                                            _template_,  \
                                            _count_,     \
                                            _setcount_,  \
                                            _offset_ )   \
    MTX_SERIAL_EXEUCTE_DATA_INITIALIZE( ( _data_ ),      \
                                        ( _template_ ),  \
                                        ( _count_ ),     \
                                        ( _setcount_ ),  \
                                        ( _offset_ ) )

#define QTC_GET_SERIAL_EXECUTE_DATA_SIZE( _count_ ) \
    MTX_GET_SERIAL_EXECUTE_DATA_SIZE( ( _count_ ) )

#endif /* _O_QTC_DEF_H_ */
