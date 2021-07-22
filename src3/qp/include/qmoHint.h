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
 * $Id: qmoHint.h 82075 2018-01-17 06:39:52Z jina.kim $
 *
 * Description :
 *
 *    [Hints 를 위한 정의]
 *
 * 용어 설명 :
 *
 * 약어 :
 *
 ***********************************************************************/

#ifndef  _O_QMO_HINT_H_
#define  _O_QMO_HINT_H_ 1

//-------------------------------------------
// [Hints : Optimization 규칙]
//     - COST
//        : Hint가 지정되지 않는 경우에는
//          Cost Based Optimization을 지정한다.
//     - RULE
//-------------------------------------------

typedef enum qmoOptGoalType
{
    QMO_OPT_GOAL_TYPE_UNKNOWN = 0,
    QMO_OPT_GOAL_TYPE_RULE,
    QMO_OPT_GOAL_TYPE_ALL_ROWS,
    QMO_OPT_GOAL_TYPE_FIRST_ROWS_N // PROJ-2242
} qmoOptGoalType;

//-------------------------------------------
// [Hints : WHERE절의 Normalization Form]
//     - NNF : Hint가 지정되지 않은 경우에는
//             Not Normal Type으로 Cost를 통하여
//             CNF 또는 DNF normal form을 결정한다.
//     - CNF : Conjunctive normal form으로 정규화
//     - DNF : Disjunctive normal form으로 정규화
//-------------------------------------------

typedef enum qmoNormalType
{
    QMO_NORMAL_TYPE_NOT_DEFINED = 0, // Not Defined Hint
    QMO_NORMAL_TYPE_DNF,     // Disjunctive Normal Form
    QMO_NORMAL_TYPE_CNF,     // Conjunctive Normal Form
    QMO_NORMAL_TYPE_NNF      // Not Normal Form  To Fix PR-12743
} qmosNormalType;

//-------------------------------------------
// [Hints : 질의 수행방식 지정]
//      - TUPLESET : tuple set 처리방식
//      - RECORD   : push projection을 적용한 레코드저장방식
//-------------------------------------------

typedef enum qmoMaterializeType
{
    QMO_MATERIALIZE_TYPE_NOT_DEFINED = 0,
    QMO_MATERIALIZE_TYPE_RID,
    QMO_MATERIALIZE_TYPE_VALUE 
} qmoProcesType;

//-------------------------------------------
// [Hints : Join Order Hints의 종류]
//      - OPTIMIZED : Join Order Hint가 지정되지 않은 경우
//      - ORDERED : Join Order Hint가 지정된 경우
//-------------------------------------------

typedef enum qmoJoinOrderType
{
    QMO_JOIN_ORDER_TYPE_OPTIMIZED = 0,
    QMO_JOIN_ORDER_TYPE_ORDERED
} qmoJoinOrderType;

//-------------------------------------------
// [Hints : Subquery unnesting hint의 종류]
//      - UNDEFINED : Optimizer가 판단(현재는 항상 수행)
//      - UNNEST    : 가능한 경우 항상 unnesting 수행
//      - NO_UNNEST : Unnesting 수행하지 않음
//-------------------------------------------

typedef enum qmoSubqueryUnnestType
{
    QMO_SUBQUERY_UNNEST_TYPE_NOT_DEFINED = 0,
    QMO_SUBQUERY_UNNEST_TYPE_UNNEST,
    QMO_SUBQUERY_UNNEST_TYPE_NO_UNNEST
} qmoSubqueryUnnestType;

typedef enum qmoInverseJoinOption
{
    QMO_INVERSE_JOIN_METHOD_ALLOWED = 0,  // (default)
    QMO_INVERSE_JOIN_METHOD_ONLY,         // INVERSE_JOIN
    QMO_INVERSE_JOIN_METHOD_DENIED,       // NO_INVERSE_JOIN
} qmoInverseJoinOption;

typedef enum qmoSemiJoinMethod
{
    QMO_SEMI_JOIN_METHOD_NOT_DEFINED = 0,
    QMO_SEMI_JOIN_METHOD_NL,
    QMO_SEMI_JOIN_METHOD_HASH,
    QMO_SEMI_JOIN_METHOD_MERGE,
    QMO_SEMI_JOIN_METHOD_SORT
} qmoSemiJoinMethod;

typedef enum qmoAntiJoinMethod
{
    QMO_ANTI_JOIN_METHOD_NOT_DEFINED = 0,
    QMO_ANTI_JOIN_METHOD_NL,
    QMO_ANTI_JOIN_METHOD_HASH,
    QMO_ANTI_JOIN_METHOD_MERGE,
    QMO_ANTI_JOIN_METHOD_SORT
} qmoAntiJoinMethod;

//---------------------------------------------------------
// [Hints : Join Method의 종류]
//
// 사용자 Hint : 사용자가 명시적으로 Join Method를
//               지정할 수 있는 Hint로 다음 네가지로 나뉜다.
//    - Nested Loop Join
//    - Hash-based Join
//    - Sort-based Join
//    - Merge Join
//
// 내부 Hint : 내부적으로 기능이 정상적으로 수행하는지, 또는
//             기능에 따른 성능 차이는 어떠한지 알아보기 위해
//             추가한 Hint이다.
//    - Nested Loop Join 계열의 내부 Hint
//      Full Nested Loop Join
//      Full Store Nested Loop Join
//      Index Nested Loop Jon
//      Anti Outer Join
//    - Sort-based Join 계열의 내부 Hint
//      One Pass Sort Join
//      Two Pass Sort Join
//    - Hash-based Join 계열의 내부 Hint
//      One Pass Hash Join
//      Two Pass Hash Join
//------------------------------------------------------------

#define QMO_JOIN_METHOD_MASK                (0x0000FFFF)
#define QMO_JOIN_METHOD_NONE                (0x00000000)

// Nested Loop Join 계열의 Hints
// Nested Loop
//    SQL: USE_NL( table_name, table_name )
// Full Nested Loop Join :
//    SQL: USE_FULL_NL( table_name, table_name )
// Full Store Nested Loop Join :
//    SQL: USE_FULL_STORE_NL( table_name, table_name )
// Index Nested Loop Join :
//    SQL: USE_INDEX_NL( table_name, table_name )
// Anti Outer Join :
//    SQL: USE_ANTI( table_name, table_name )
// Inverse Index Nested Loop Join (PROJ-2385, Semi-Join) :
//    SQL: NL_SJ INVERSE_JOIN
#define QMO_JOIN_METHOD_NL                  (0x0000001F)
#define QMO_JOIN_METHOD_FULL_NL             (0x00000001)
#define QMO_JOIN_METHOD_INDEX               (0x00000002)
#define QMO_JOIN_METHOD_FULL_STORE_NL       (0x00000004)
#define QMO_JOIN_METHOD_ANTI                (0x00000008)
#define QMO_JOIN_METHOD_INVERSE_INDEX       (0x00000010)

// Hash-based Join 계열의 Hints
// Hash-based
//    SQL: USE_HASH( table_name, table_name )
// One Pass Hash Join :
//    SQL: USE_ONE_PASS_HASH( table_name, table_name )
// Two Pass Hash Join :
//    SQL: USE_TWO_PASS_HASH( table_name, table_name [,temp_table_count] )
// Inverse Hash Join (PROJ-2339) :
//    SQL: USE_INVERSE_HASH( table_name, table_name )
#define QMO_JOIN_METHOD_HASH                (0x000000E0)
#define QMO_JOIN_METHOD_ONE_PASS_HASH       (0x00000020)
#define QMO_JOIN_METHOD_TWO_PASS_HASH       (0x00000040)
#define QMO_JOIN_METHOD_INVERSE_HASH        (0x00000080)

// Sort-based Join 계열의 Hints
// Sort- based :
//     SQL: USE_SORT( table_name, table_name )
// One Pass Sort Join :
//     SQL: USE_ONE_PASS_SORT( table_name, table_name )
// Two Pass Sort Join :
//     SQL: USE_TWO_PASS_SORT( table_name, table_name )
// Inverse Sort Join (PROJ-2385, Semi/Anti-Join) :
//    SQL: SORT_SJ/AJ INVERSE_JOIN 
#define QMO_JOIN_METHOD_SORT                (0x00000F00)
#define QMO_JOIN_METHOD_ONE_PASS_SORT       (0x00000100)
#define QMO_JOIN_METHOD_TWO_PASS_SORT       (0x00000200)
#define QMO_JOIN_METHOD_INVERSE_SORT        (0x00000400)

// Merge Join Hints
// Merge :
//     SQL: USE_MERGE( table_name, table_name)

#define QMO_JOIN_METHOD_MERGE               (0x00001000)

// Inverse Join Hints (PROJ-2385)
#define QMO_JOIN_METHOD_INVERSE         \
    ( QMO_JOIN_METHOD_INVERSE_INDEX     \
      | QMO_JOIN_METHOD_INVERSE_HASH    \
      | QMO_JOIN_METHOD_INVERSE_SORT )

//-------------------------------------------
// [Hints : Table Access 방법]
// 테이블 접근 방법을 결정하는 Hint 이다.
//    - OPTIMIZED SCAN : Table Access Hint가 주어지지 않은 경우,
//                       optimizer가 Table Access Cost에 따라 선택
//    - FULL SCAN : 인덱스 사용 가능하더라고 테이블 전체를 검색
//    - INDEX     : 나열된 인덱스로 해당 테이블 검색
//    - INDEX ASC : 나열된 인덱스로 해당 테이블을 오름차순으로만 검색
//    - INDEX DESC: 나열된 인덱스로 해당 테이블을 내림차순으로만 검색
//    - NO INDEX  : 검색 시에 나열된 인덱스를 사용하지 않음
//-------------------------------------------

typedef enum qmoTableAccessType
{
    QMO_ACCESS_METHOD_TYPE_OPTIMIZED_SCAN = 0,
    QMO_ACCESS_METHOD_TYPE_FULLACCESS_SCAN,
    QMO_ACCESS_METHOD_TYPE_NO_INDEX_SCAN,
    QMO_ACCESS_METHOD_TYPE_INDEXACCESS_SCAN,
    QMO_ACCESS_METHOD_TYPE_INDEX_ASC_SCAN,
    QMO_ACCESS_METHOD_TYPE_INDEX_DESC_SCAN
} qmoTableAccessType;

//-------------------------------------------
// [Hint : 중간 결과의 종류]
// 중간 결과가 생성되는 경우, 중간 결과를 disk temp table에 저장할 지
// memory temp table 에 저장할 지 결정하는 Hint
//    - NOT DEFIENED : 중간 결과 Hint가 주어지지 않은 경우
//    - MEMORY       : 중간 결과가 생성되면, Memory Temp Table에 저장
//        SQL : TEMP_TBS_MEMORY
//    - DISK         : 중간 결과가 생성되면, Disk Temp Table에 저장
//        SQL : TEMP_TBS_DISK
//-------------------------------------------

typedef enum qmoInterResultType
{
    QMO_INTER_RESULT_TYPE_NOT_DEFINED = 0,
    QMO_INTER_RESULT_TYPE_MEMORY,
    QMO_INTER_RESULT_TYPE_DISK
} qmoInterResultType;

//-------------------------------------------
// [Hint : Grouping Method 종류]
// Grouping을 Sort-based로 수행할 지, Hash-based로 수행할 지를
// 결정할 수 있는 내부 Hint
//    - NOT DEFINED : Hint가 주어지지 않은 경우
//    - HASH        : Hash-based로 Grouping을 수행
//        SQL : GROUP_HASH
//    - SORT        : Sort-based로 Grouping을 수행
//        SQL : GROUP_SORT
//-------------------------------------------

typedef enum qmoGroupMethodType
{
    QMO_GROUP_METHOD_TYPE_NOT_DEFINED = 0,
    QMO_GROUP_METHOD_TYPE_HASH,
    QMO_GROUP_METHOD_TYPE_SORT
}qmoGroupMethodType;

//-------------------------------------------
// [Hint : Distinction 종류]
// Distinction을 Sort-based로 수행할 지, Hash-based로 수행할 지를
// 결정할 수 있는 내부 Hint
//    - NOT DEFINED : Hint가 주어지지 않은 경우
//    - HASH        : Hash-based로 Distinction을 수행
//        SQL : DISTINCT_HASH
//    - SORT        : Sort-based로 Distinction을 수행
//        SQL : DISTINCT_SORT
//-------------------------------------------

typedef enum qmoDistinctMethodType
{
    QMO_DISTINCT_METHOD_TYPE_NOT_DEFINED = 0,
    QMO_DISTINCT_METHOD_TYPE_HASH,
    QMO_DISTINCT_METHOD_TYPE_SORT
}qmoDistinctMethodType;

//-------------------------------------------
// [Hint : View 최적화 종류]
// View 최적화를 View Materialization으로 수행할 지,
// Push Selection으로 수행할 지를 결정할 수 있는 내부 Hint
//    - NOT DEFINED : Hint가 주어지지 않은 경우
//    - VMTR        : View Materialization 기법으로 View 최적화
//        SQL : NO_PUSH_SELECT_VIEW
//    - PUSH        : Push Selection 기법으로 View 최적화
//        SQL : PUSH_SELECT_VIEW
//-------------------------------------------

typedef enum qmoViewOptType
{
    QMO_VIEW_OPT_TYPE_NOT_DEFINED = 0,
    QMO_VIEW_OPT_TYPE_VMTR,
    QMO_VIEW_OPT_TYPE_PUSH,
    QMO_VIEW_OPT_TYPE_FORCE_VMTR,
    QMO_VIEW_OPT_TYPE_CMTR,
    // BUG-38022
    // view를 한번만 사용해서 이후 사용여부에 따라 VMTR로 바뀔 수 있는 경우
    QMO_VIEW_OPT_TYPE_POTENTIAL_VMTR
} qmoViewOptType;

//-------------------------------------------
// PROJ-2206
// [Hint : View 최적화 종류]
// MATERIALIZE
// NO_MATERIALIZE
// ex)
// select * from (select /*+ materialize */ * from t1);
// select * from (select /*+ no_materialize */ * from t2);
// select /*+ no_push_select_view(v1) */ * from (select /*+ materialize */ * from t1) v1;
// no_push_select_view(view name), push_select_view(view name)과 별도로
// 주어진 query set에 영향을 주는 materialize, no_materialize hint를 위한 enum 타입이다.
//-------------------------------------------
typedef enum qmoViewOptMtrType
{
    QMO_VIEW_OPT_MATERIALIZE_NOT_DEFINED = 0,
    QMO_VIEW_OPT_MATERIALIZE,
    QMO_VIEW_OPT_NO_MATERIALIZE
}qmoViewOptMtrType;

//-------------------------------------------
// PROJ-1404
// [Hints : Transitive Predicate Generation Hints의 종류]
// Transitive Predicate을 생성할 지를 결정할 수 있는 Hint
//      - ENABLE  : Hint가 주어지지 않은 경우
//                  Transitive Predicate을 생성
//      - DISABLE : Transitive Predicate을 생성하지 않음
//          SQL : NO_TRANSITIVE_PRED
//-------------------------------------------

typedef enum qmoTransitivePredType
{
    QMO_TRANSITIVE_PRED_TYPE_ENABLE = 0,
    QMO_TRANSITIVE_PRED_TYPE_DISABLE
} qmoTransitivePredType;

typedef enum qmoResultCacheType
{
    QMO_RESULT_CACHE_NOT_DEFINED = 0,
    QMO_RESULT_CACHE,
    QMO_RESULT_CACHE_NO
} qmoResultCacheType;

#endif /* _O_QMO_HINT_H_ */
